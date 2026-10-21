#pragma once
#ifdef _WIN32
#include <Windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <thread>
#include <condition_variable>
#include <set>
#include <deque>
#include <map>
#include <filesystem>
#include "fast/Prewarm.h"
#include "fast/ReliefCache.h"
#include "fast/interpreter.h"
#include "fast/resource/type/Texture.h"
#include "soh/SaveManager.h"
#include "soh/resource/type/Scene.h"
#include "soh/resource/type/scenecommand/SetObjectList.h"
#include "soh/resource/type/scenecommand/SetSpecialObjects.h"
#include "soh/resource/type/scenecommand/SetExitList.h"

namespace ProgressionPrewarm {
using Json=nlohmann::json;
static const char* const scenes[]={
#define DEFINE_SCENE(segment, title, id, draw, a, b) #segment,
#include "tables/scene_table.h"
#undef DEFINE_SCENE
};
static const char* const objects[]={
#define DEFINE_OBJECT(segment,id) #segment,
#define DEFINE_OBJECT_UNSET(id) "",
#define DEFINE_OBJECT_NULL(segment,id) "",
#include "tables/object_table.h"
#undef DEFINE_OBJECT
#undef DEFINE_OBJECT_UNSET
#undef DEFINE_OBJECT_NULL
};
static constexpr int entrances[]={
#define DEFINE_ENTRANCE(id,scene,spawn,bgm,title,in,out) scene,
#include "tables/entrance_table.h"
#undef DEFINE_ENTRANCE
};
inline int Entrance(int i) {return i>=0&&i<int(std::size(entrances))?entrances[i]:SCENE_LINKS_HOUSE;}
inline std::vector<int> Seeds(int saved,int entrance,int age,uint32_t quest) {
    std::vector<int> out{saved,Entrance(entrance)};
    // Match vanilla resume locations as well as the last saved scene. Dungeons
    // retain their entrance; most child/adult overworld saves resume at home/Temple.
    out.push_back(age==LINK_AGE_CHILD?SCENE_LINKS_HOUSE:SCENE_TEMPLE_OF_TIME);
    if(age==LINK_AGE_CHILD) {
        if(!(quest&(1u<<QUEST_KOKIRI_EMERALD)))out.push_back(SCENE_DEKU_TREE);
        else if(!(quest&(1u<<QUEST_GORON_RUBY)))out.push_back(SCENE_DODONGOS_CAVERN);
        else if(!(quest&(1u<<QUEST_ZORA_SAPPHIRE)))out.push_back(SCENE_JABU_JABU);
        else out.push_back(SCENE_TEMPLE_OF_TIME);
    } else {
        // These are priorities, never assumed access requirements; optional routes
        // are found from real scene exits rather than a single forced story order.
        for(int i: {0,1,2,4,3})if(!(quest&(1u<<i))) {
            static const int temples[]={SCENE_FOREST_TEMPLE,SCENE_FIRE_TEMPLE,SCENE_WATER_TEMPLE,
                SCENE_SPIRIT_TEMPLE,SCENE_SHADOW_TEMPLE};out.push_back(temples[i]);break;
        }
    }
    return out;
}
struct Request {int slot=-1,scene=-1,age=LINK_AGE_CHILD,entrance=0;uint32_t quest=0;bool active=false,filter=false,srgb=false,mq=false;};
class Worker {
    struct Job {std::string path;int scene,rank;uint64_t a=0,b=0;bool shader=false;};
    std::mutex mutex;std::condition_variable wake;std::thread thread;
    bool stop=false;Request request;uint64_t revision=0,seenRevision=~0ull;
    std::array<std::string,3> saves;
    std::shared_ptr<Ship::ResourceManager> rm;
    uintptr_t owner=reinterpret_cast<uintptr_t>(this);
    std::map<std::string,std::vector<std::string>> families;
    std::map<int,int> wanted;
    std::map<int,std::set<int>> edges;
    std::map<int,std::set<std::string>> objectDependencies;
    std::map<std::string,std::set<std::pair<uint64_t,uint64_t>>> candidates;
    std::deque<Job> jobs;
    std::set<std::string> queued,done;
    std::set<Fast::Prewarm::SourceKey> neededSources;
    std::set<std::tuple<uint64_t,uint64_t,bool,bool>> shadersDone;
    std::map<int,std::set<std::pair<uint64_t,uint64_t>>> observed;
    size_t completed=0,materials=0,shaderJobs=0,errors=0,cancelled=0,changes=0;
    size_t rejectedVariants=0;std::string lastRejected;
    size_t shaderTurns=0;int currentScene=-1,neighborCursor=-2;
    double workMs=0,maxWorkMs=0;int slot=-1;bool playing=false;
    std::string lastError,fingerprint;bool allowNormal=true,allowMQ=false;
    static std::string Family(std::string path) {
        if(path.starts_with("alt/"))path.erase(0,4);
        auto start=path.find('/');if(start==path.npos)return {};
        if(path.starts_with("scenes/"))start=path.find('/',start+1);
        if(start==path.npos)return {};auto end=path.find('/',start+1);
        return end==path.npos?std::string{}:path.substr(start+1,end-start-1);
    }
    void Push(Job job) {
        if(job.shader)neededSources.insert(Fast::Prewarm::Key(job.a,job.b,Fast::Prewarm::threePoint,Fast::Prewarm::srgb));
        const auto key=job.shader?"S"+std::to_string(job.a)+":"+std::to_string(job.b):job.path;
        if(job.shader&&shadersDone.contains({job.a,job.b,Fast::Prewarm::threePoint.load(),Fast::Prewarm::srgb.load()}))return;
        if((!job.shader&&done.contains(key))||!queued.insert(key).second)return;
        auto at=std::find_if(jobs.begin(),jobs.end(),[&](const auto& other){return other.rank>job.rank;});
        jobs.insert(at,std::move(job));
    }
    static int CandidateRank(int rank,uint64_t b) {
        return rank+3+(((b>>8)&15)==15?24:0)+
            ((b&(SHADER_OPT(TEXTURE_EDGE)|SHADER_OPT(ALPHA_THRESHOLD)))?12:0);
    }
    void FamilyJobs(const std::string& family,int scene,int rank) {
        for(auto [a,b]:candidates[family])Push({"",scene,CandidateRank(rank,b),a,b,true});
        auto it=families.find(family);if(it==families.end())return;
        for(const auto& path:it->second) {
            if((!allowMQ&&path.find("/mq/")!=path.npos)||(!allowNormal&&path.find("/nonmq/")!=path.npos))continue;
            const auto basename=path.substr(path.find_last_of('/')+1);
            auto roomPrefix=family;
            if(roomPrefix.ends_with("_scene"))roomPrefix.resize(roomPrefix.size()-6);
            roomPrefix+="_room_";
            const bool roomHeader=basename.starts_with(roomPrefix)&&basename.size()>roomPrefix.size()&&
                std::all_of(basename.begin()+roomPrefix.size(),basename.end(),[](char c){return c>='0'&&c<='9';});
            const bool header=basename==family||roomHeader||basename.find("SceneCommands")!=basename.npos;
            Push({path,scene,rank+(header?0:2)});
        }
    }
    void Want(int scene,int rank) {
        if(scene<0||scene>=int(std::size(scenes)))return;
        auto it=wanted.find(scene);if(it!=wanted.end()&&it->second<=rank)return;
        wanted[scene]=rank;FamilyJobs(scenes[scene],scene,rank);
        for(const auto& family:objectDependencies[scene])FamilyJobs(family,scene,rank+1);
        if(auto known=observed.find(scene);known!=observed.end())
            for(auto [a,b]:known->second)Push({"",scene,rank-100,a,b,true});
        if(rank<20)for(int next:edges[scene])Want(next,rank+10);
    }
    static std::string PortableFingerprint(const std::string& value) {
        struct Part {std::filesystem::path path;std::string stamp;};
        std::vector<Part> parts;std::filesystem::path appRoot;
        std::istringstream input(value);std::string item;
        while(std::getline(input,item,';')) {
            const auto last=item.rfind(':');if(last==item.npos||last==0)continue;
            const auto size=item.rfind(':',last-1);if(size==item.npos)continue;
            auto path=std::filesystem::path(item.substr(0,size)).lexically_normal();
            if(path.filename()=="soh.o2r"&&path.is_absolute())appRoot=path.parent_path();
            parts.push_back({std::move(path),item.substr(size)});
        }
        std::string result;
        for(auto& part:parts) {
            if(part.path.is_absolute()&&!appRoot.empty()) {
                auto relative=part.path.lexically_relative(appRoot);
                if(!relative.empty()&&*relative.begin()!="..")part.path=std::move(relative);
            }
            result+=part.path.generic_string()+part.stamp+";";
        }
        return result;
    }
    void Report() {
        Json ranks=Json::array();for(auto [scene,rank]:wanted)ranks.push_back({{"scene",scene},{"rank",rank}});
        Json manifest=Json::array();for(auto& [scene,entries]:observed)
            for(auto [a,b]:entries)manifest.push_back({scene,a,b});
        Json graph=Json::array();for(auto& [a,entries]:edges)for(auto b:entries)graph.push_back({a,b});
        Json catalog=Json::array();for(auto& [family,entries]:candidates)
            for(auto [a,b]:entries)catalog.push_back({family,a,b});
        Json deps=Json::array();for(auto& [scene,entries]:objectDependencies)
            for(const auto& family:entries)deps.push_back({scene,family});
        Json j={{"archive_fingerprint",fingerprint},{"catalog",catalog},{"object_dependencies",deps},{"version",1},{"selected_slot",slot},{"playing",playing},{"completed",completed},
            {"queued",jobs.size()},{"materials",materials},{"shader_jobs",shaderJobs},{"errors",errors},
            {"cancelled",cancelled},{"rejected_variants",rejectedVariants},{"last_rejected",lastRejected},{"plan_changes",changes},{"work_ms",workMs},{"max_job_ms",maxWorkMs},
            {"wanted_scenes",ranks},{"observed",manifest},{"edges",graph},{"last_error",lastError}};
        std::error_code ec;std::filesystem::create_directories("cache",ec);
        if(!ec) {std::ofstream out("cache/prewarm-status.tmp");out<<j.dump(2);out.close();
            MoveFileExW(L"cache/prewarm-status.tmp",L"cache/prewarm-status.json",MOVEFILE_REPLACE_EXISTING);}
    }
    void Plan(const Request& r) {
        cancelled+=jobs.size();jobs.clear();queued.clear();wanted.clear();neededSources.clear();++changes;
        currentScene=r.active?r.scene:-1;neighborCursor=-2;shaderTurns=0;
        slot=r.slot;playing=r.active;allowMQ=r.mq;allowNormal=!r.mq;
        bool warmChild=r.age==LINK_AGE_CHILD,warmAdult=!warmChild;
        if(playing)for(int seed:Seeds(r.scene,r.entrance,r.age,r.quest))Want(seed,seed==r.scene?0:5);
        else {
            bool valid=false;std::vector<int> allSeeds;allowNormal=allowMQ=false;warmChild=warmAdult=false;
            for(auto& path:saves)try {
                std::ifstream file(path);if(!file)continue;auto j=Json::parse(file);
                auto& b=j.at("sections").at("base").at("data");
                auto seeds=Seeds(b.value("savedSceneNum",SCENE_LINKS_HOUSE),b.value("entranceIndex",0),
                    b.value("linkAge",LINK_AGE_CHILD),b.at("inventory").value("questItems",0u));
                allSeeds.insert(allSeeds.end(),seeds.begin(),seeds.end());
                const bool child=b.value("linkAge",LINK_AGE_CHILD)==LINK_AGE_CHILD;warmChild|=child;warmAdult|=!child;
                const bool mq=b.value("isMasterQuest",false);allowMQ|=mq;allowNormal|=!mq;valid=true;
            } catch(const std::exception&) {++errors;}
            if(!valid){allowNormal=true;warmChild=true;allSeeds=Seeds(SCENE_LINKS_HOUSE,0,LINK_AGE_CHILD,0);}
            for(int seed:allSeeds)Want(seed,0);
        }
        // Startup/UI and first-world programs can be observed before a scene
        // owns the frame. They are shared history, not tied to an unused slot.
        for(auto [a,b]:observed[-1])Push({"",-1,-100,a,b,true});
        // Shared player/effect resources are useful to all three slots.
        for(const char* family:{"gameplay_keep","gameplay_field_keep","gameplay_dangeon_keep"})FamilyJobs(family,-1,1);
        if(warmChild)FamilyJobs("object_link_child",-1,1);
        if(warmAdult)FamilyJobs("object_link_boy",-1,1);
        Fast::Prewarm::TrimSources(neededSources);
        Report();
    }
    void Scene(const std::shared_ptr<SOH::Scene>& scene,int sceneId,int rank) {
        for(auto& cmd:scene->commands) {
            if(auto list=std::dynamic_pointer_cast<SOH::SetObjectList>(cmd))
                for(int id:list->objects)if(id>0&&id<int(std::size(objects))) {
                    objectDependencies[sceneId].insert(objects[id]);FamilyJobs(objects[id],sceneId,rank+1);
                }
            if(auto special=std::dynamic_pointer_cast<SOH::SetSpecialObjects>(cmd)) {
                int id=special->specialObjects.globalObject;
                if(id>0&&id<int(std::size(objects))) {
                    objectDependencies[sceneId].insert(objects[id]);FamilyJobs(objects[id],sceneId,rank+1);
                }
            }
            if(auto exits=std::dynamic_pointer_cast<SOH::SetExitList>(cmd))
                for(int exit:exits->exits)if(exit<int(std::size(entrances))) {
                    int next=Entrance(exit);edges[sceneId].insert(next);
                    if(rank<20)Want(next,rank+10);
                }
        }
    }
    void Texture(const std::shared_ptr<Fast::Texture>& tex) {
        const auto name=tex->GetInitData()->Path;
        if(Fast::ReliefTextureScene(name)<0||!tex->ImageData)return;
        const auto count=size_t(tex->Width)*tex->Height;
        if(!count||count>4096ull*4096)return;
        // HD replacements already contain exactly the RGBA submitted to DX11.
        // Palette/partial-tile textures need render-state context and remain on
        // the validated ordinary path; never manufacture different material pixels.
        const bool palette=tex->Type==Fast::TextureType::Palette4bpp||tex->Type==Fast::TextureType::Palette8bpp;
        const bool rgba=(tex->Flags&TEX_FLAG_LOAD_AS_IMG)||
            ((tex->Flags&TEX_FLAG_LOAD_AS_RAW)&&!palette)||tex->Type==Fast::TextureType::RGBA32bpp;
        if(rgba&&tex->ImageDataSize==count*4) {
            Fast::Prewarm::relief(tex->ImageData,tex->Width,tex->Height,name);++materials;
        }
    }
    void DisplayList(const std::shared_ptr<Fast::DisplayList>& dl,int scene,int rank,const std::string& family) {
        // Inspect commands only; never execute display lists or touch gameplay.
        for(const auto& command:dl->Instructions) {
            const uint32_t w0=uint32_t(command.words.w0),w1=uint32_t(command.words.w1);
            if((w0>>24)!=0xFC)continue;
            auto c=[](uint32_t a,uint32_t b,uint32_t c,uint32_t d){return uint64_t(a|(b<<4)|(c<<8)|(d<<13));};
            auto a=[](uint32_t a,uint32_t b,uint32_t c,uint32_t d){return uint64_t(a|(b<<3)|(c<<6)|(d<<9));};
            uint64_t combine=c((w0>>20)&15,w1>>28,(w0>>15)&31,(w1>>15)&7)|
                (a((w0>>12)&7,(w1>>12)&7,(w0>>9)&7,(w1>>9)&7)<<16)|
                (c((w0>>5)&15,(w1>>24)&15,w0&31,(w1>>6)&7)<<28)|
                (a((w1>>21)&7,(w1>>3)&7,(w1>>18)&7,w1&7)<<44);
            // Common opaque, translucent, fogged and cutout passes, both cycles.
            // Exact observed variants get higher priority on later visits/launches.
            for(uint64_t cycle:{0ull,SHADER_OPT(_2CYC)})for(uint64_t pass:
                {0ull,SHADER_OPT(ALPHA),SHADER_OPT(FOG),SHADER_OPT(ALPHA)|SHADER_OPT(FOG),
                 SHADER_OPT(ALPHA)|SHADER_OPT(TEXTURE_EDGE),SHADER_OPT(ALPHA)|SHADER_OPT(ALPHA_THRESHOLD),
                 SHADER_OPT(ALPHA)|SHADER_OPT(FOG)|SHADER_OPT(TEXTURE_EDGE),
                 SHADER_OPT(ALPHA)|SHADER_OPT(FOG)|SHADER_OPT(ALPHA_THRESHOLD),
                 SHADER_OPT(ALPHA)|SHADER_OPT(NOISE),SHADER_OPT(ALPHA)|SHADER_OPT(FOG)|SHADER_OPT(NOISE)}) {
                Fast::ColorCombiner comb{};ColorCombinerKey key{};key.combine_mode=combine;
                key.options=cycle|pass|(0xffffull<<Fast::SHADER_ID_SHIFT);
                Fast::Interpreter::GenerateCC(&comb,key);
                CCFeatures features{};gfx_cc_get_features(comb.shader_id0,comb.shader_id1,&features);
                const unsigned enabled=(features.usedTextures[0]?3:0)|(features.usedTextures[1]?12:0);
                for(uint64_t clamp=0;clamp<16;++clamp) {
                    if(clamp&~uint64_t(enabled))continue;
                    const auto b=comb.shader_id1|(clamp<<8);
                    candidates[family].insert({comb.shader_id0,b});Push({"",scene,CandidateRank(rank,b),comb.shader_id0,b,true});
                }
            }
        }
    }
    void Observe() {
        std::vector<std::tuple<int,uint64_t,uint64_t>> entries;
        {std::lock_guard lock(Fast::Prewarm::observationMutex);entries.swap(Fast::Prewarm::observations);}
        for(auto [scene,a,b]:entries)observed[scene].insert({a,b});
    }
    void Run() {
        SetThreadPriority(GetCurrentThread(),THREAD_MODE_BACKGROUND_BEGIN);
        Fast::D3DShaderCache::background=true;
        try {
            const auto archives=rm->GetArchiveManager()->GetArchives();
            for(const auto& archive:*archives) {
                const std::filesystem::path path=archive->GetPath();std::error_code ec;
                fingerprint+=path.generic_string()+":";
                fingerprint+=std::to_string(std::filesystem::file_size(path,ec))+":";
                fingerprint+=std::to_string(std::filesystem::last_write_time(path,ec).time_since_epoch().count())+";";
            }
            fingerprint=PortableFingerprint(fingerprint);
            const auto list=rm->GetArchiveManager()->ListFiles();
            std::set<std::string> paths;
            for(auto path:*list) {
                if(path.ends_with(".meta"))path.resize(path.size()-5);
                if(path.find('.')!=path.npos||(!path.starts_with("scenes/")&&!path.starts_with("objects/")&&
                   !path.starts_with("alt/scenes/")&&!path.starts_with("alt/objects/")))continue;
                paths.insert(std::move(path));
            }
            for(const auto& path:paths)families[Family(path)].push_back(path);
            try {std::ifstream in("cache/prewarm-status.json");if(in) {
                if(std::filesystem::file_size("cache/prewarm-status.json")>16*1024*1024)throw std::runtime_error("Oversized prewarm catalog");
                auto j=Json::parse(in);
                if(j.value("version",0)!=1||PortableFingerprint(j.value("archive_fingerprint",std::string{}))!=fingerprint)
                    j=Json::object();
                for(auto& e:j.value("catalog",Json::array()))candidates[e.at(0).get<std::string>()].insert(
                    {e.at(1).get<uint64_t>(),e.at(2).get<uint64_t>()});
                for(auto& e:j.value("object_dependencies",Json::array()))objectDependencies[e.at(0).get<int>()].insert(e.at(1).get<std::string>());
                for(auto& e:j.value("observed",Json::array()))observed[e.at(0).get<int>()].insert(
                    {e.at(1).get<uint64_t>(),e.at(2).get<uint64_t>()});
                for(auto& e:j.value("edges",Json::array()))edges[e.at(0).get<int>()].insert(e.at(1).get<int>());
            }}catch(const std::exception&) { /* Cache metadata is expendable. */ }
            auto preparedEpoch=Fast::Prewarm::sourceEpoch.load();
            while(true) {
                Request r;uint64_t rev;
                {std::unique_lock lock(mutex);if(stop)break;r=request;rev=revision;}
                const auto epoch=Fast::Prewarm::sourceEpoch.load();
                if(epoch!=preparedEpoch){preparedEpoch=epoch;shadersDone.clear();seenRevision=~0ull;}
                if(rev!=seenRevision) {seenRevision=rev;Plan(r);}
                Observe();
                if(jobs.empty()||Fast::Prewarm::Clock()<Fast::Prewarm::yieldUntil.load()) {
                    std::unique_lock lock(mutex);wake.wait_for(lock,std::chrono::milliseconds(100),[&]{return stop||revision!=seenRevision;});continue;
                }
                // Keep decoding/discovery moving even when current-area shader
                // variants remain queued. One of every four shader turns rotates
                // through other wanted scenes so neighboring areas cannot starve.
                auto asset=std::find_if(jobs.begin(),jobs.end(),[](const auto& j){return !j.shader;});
                auto shader=std::find_if(jobs.begin(),jobs.end(),[](const auto& j){return j.shader;});
                auto next=asset;
                if((completed%8==7||asset==jobs.end())&&shader!=jobs.end()) {
                    next=shader;
                    if(++shaderTurns%4==0) {
                        std::set<int> others;
                        for(const auto& job:jobs)if(job.shader&&job.scene!=currentScene)others.insert(job.scene);
                        if(!others.empty()) {
                            auto chosen=others.upper_bound(neighborCursor);if(chosen==others.end())chosen=others.begin();
                            neighborCursor=*chosen;
                            next=std::find_if(jobs.begin(),jobs.end(),[&](const auto& j){return j.shader&&j.scene==neighborCursor;});
                        }
                    }
                }
                if(next==jobs.end())next=jobs.begin();
                Job job=std::move(*next);jobs.erase(next);
                const auto key=job.shader?"S"+std::to_string(job.a)+":"+std::to_string(job.b):job.path;
                queued.erase(key);const auto start=std::chrono::steady_clock::now();
                try {
                    if(job.shader) {
                        const bool filter=Fast::Prewarm::threePoint,srgb=Fast::Prewarm::srgb;
                        if(shadersDone.insert({job.a,job.b,filter,srgb}).second) {
                            if(Fast::Prewarm::compile(job.a,job.b,filter,srgb))++shaderJobs;
                            else {++rejectedVariants;lastRejected=Fast::Prewarm::rejectedReason;}
                        }
                    } else {
                        Ship::ResourceIdentifier id(job.path,owner,nullptr);
                        auto resource=rm->LoadResourceAsync(id,false,BS::pr::lowest).get();
                        if(auto scene=std::dynamic_pointer_cast<SOH::Scene>(resource))Scene(scene,job.scene,job.rank);
                        if(auto tex=std::dynamic_pointer_cast<Fast::Texture>(resource))Texture(tex);
                        if(auto dl=std::dynamic_pointer_cast<Fast::DisplayList>(resource))DisplayList(dl,job.scene,job.rank,Family(job.path));
                        // The worker owns only its current asset. It never evicts a
                        // renderer/gameplay cache entry, nor pins entire scenes in RAM.
                        rm->UnloadResource(id);
                        rm->UnloadResource(Ship::ResourceIdentifier("alt/"+job.path,owner,nullptr));
                    }
                    if(!job.shader)done.insert(key);++completed;
                }catch(const std::exception& e) {++errors;lastError=e.what();}
                const double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
                workMs+=ms;maxWorkMs=std::max(maxWorkMs,ms);
                if(completed%100==0)Report();
                // At most ~8% duty in play, 50% on title, on one background-priority
                // thread. Long jobs accrue their full debt instead of running in bursts.
                const int rest=int(std::clamp(ms*(playing?12.0:1.0),playing?15.0:1.0,10000.0));
                std::unique_lock lock(mutex);wake.wait_for(lock,std::chrono::milliseconds(rest),[&]{return stop||revision!=seenRevision;});
            }
            Observe();Report();
        }catch(const std::exception& e) {++errors;lastError=e.what();Report();}
        rm->UnloadResources(Ship::ResourceFilter({"*"},{},owner,nullptr));
        SetThreadPriority(GetCurrentThread(),THREAD_MODE_BACKGROUND_END);
    }
  public:
    Worker(std::shared_ptr<Ship::ResourceManager> manager,std::array<std::string,3> paths):saves(std::move(paths)),rm(std::move(manager)) {
        thread=std::thread([this]{Run();});
    }
    ~Worker(){Stop();}
    void Stop() {{std::lock_guard lock(mutex);stop=true;}wake.notify_all();if(thread.joinable())thread.join();}
    void Update(const Request& r) {
        std::lock_guard lock(mutex);
        if(r.slot==request.slot&&r.scene==request.scene&&r.age==request.age&&r.quest==request.quest&&
           r.active==request.active&&r.entrance==request.entrance&&r.filter==request.filter&&r.srgb==request.srgb&&r.mq==request.mq)return;
        request=r;++revision;wake.notify_all();
    }
};
inline std::unique_ptr<Worker> worker;
inline int selected=-1;
inline bool nativePreparing=false;
inline void Frame() {
    const int hold=CVarGetInteger("gEnhancements.Prewarm.NativeHoldSeconds",0);
    if(hold>0&&CVarGetInteger("gEnhancements.Prewarm.NativeTest",0)==1) {
        static auto nativeStart=std::chrono::steady_clock::now();
        nativePreparing=std::chrono::steady_clock::now()-nativeStart<std::chrono::seconds(hold);
        CVarSetInteger("gSettings.GraphicsMode",3);
        CVarSetInteger("gSettings.GraphicsTargetFPS",CVarGetInteger("gEnhancements.Prewarm.NativeFPS",7));
    }
    Fast::Prewarm::frameBudgetMs=1000.0/std::clamp(CVarGetInteger("gSettings.GraphicsTargetFPS",60),1,240);
    if(!Fast::Prewarm::compile||!SaveManager::Instance||CVarGetInteger("gEnhancements.Prewarm.Disabled",0))return;
    static auto first=std::chrono::steady_clock::now();
    if(std::chrono::steady_clock::now()-first<std::chrono::seconds(2))return;
    if(!worker) {
        std::array<std::string,3> paths;for(int i=0;i<3;++i)paths[i]=(std::filesystem::path(Ship::Context::GetPathRelativeToAppDirectory("Save"))/("file"+std::to_string(i+1)+".sav")).string();
        worker=std::make_unique<Worker>(Ship::Context::GetRawInstance()->GetResourceManager(),paths);
        Fast::Prewarm::shutdown=[] {if(worker){worker->Stop();worker.reset();}};
    }
    Request r;r.slot=selected;r.filter=Fast::Prewarm::threePoint;r.srgb=Fast::Prewarm::srgb;
    const int test=CVarGetInteger("gEnhancements.Prewarm.NativeTest",0);
    if(test==2) {
        CVarSetInteger("gSettings.GraphicsMode",3);
        CVarSetInteger("gSettings.GraphicsTargetFPS",CVarGetInteger("gEnhancements.Prewarm.NativeFPS",7));
        Fast::DialoguePerformance::enabled=true;
        const auto elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-first).count();
        if(elapsed>=60) {
            r.active=true;r.slot=1;r.scene=elapsed<90?SCENE_DEKU_TREE:SCENE_HYRULE_FIELD;
            r.entrance=elapsed<90?0:205;r.age=LINK_AGE_CHILD;
        }
        Fast::Prewarm::scene=r.active?r.scene:-1;worker->Update(r);
        if(elapsed>=120) {
            Fast::Prewarm::Stop();Fast::DialoguePerformance::Save();
            Ship::Context::GetRawInstance()->GetWindow()->Close();
        }
        return;
    }
    const bool fixture=CVarGetInteger("gEnhancements.Prewarm.NativeTest",0)!=0;
    r.active=gPlayState&&(selected>=0||fixture)&&!nativePreparing;
    r.mq=gSaveContext.ship.quest.id==QUEST_MASTER||gSaveContext.ship.quest.id==QUEST_SPEEDRUN_MASTER;
    if(r.active) {
        r.scene=gPlayState->sceneNum;r.age=gSaveContext.linkAge;r.entrance=gSaveContext.entranceIndex;
        r.quest=gSaveContext.inventory.questItems;
    }
    Fast::Prewarm::scene=r.active?r.scene:-1;
    worker->Update(r);
}
inline void Register() {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnGameFrameUpdate>(Frame);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnLoadGame>([](int32_t file){selected=file<3?file:-1;});
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnExitGame>([](int32_t){selected=-1;});
}
}
#else
namespace ProgressionPrewarm { inline void Register() {} }
#endif
