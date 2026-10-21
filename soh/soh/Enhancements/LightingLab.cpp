#include <chrono>
#include <iomanip>
#include "fast/LightingLab.h"
#include "fast/resource/type/DisplayList.h"
#include "fast/GraphicsProfiles.h"
#include "ship/resource/GraphicsResourceMode.h"
#include "ship/resource/ResourceManager.h"
#include "fast/MaterialRelief.h"
#include "soh/ShipInit.hpp"
#include "soh/frame_interpolation.h"
#include "soh/ActorDB.h"
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include <libultraship/bridge/consolevariablebridge.h>
#include <ship/Context.h>
#include <ship/window/Window.h>
#include <ship/window/gui/Gui.h>
#include <ship/window/gui/GuiWindow.h>
#include <imgui.h>
#include <fstream>
#include <cmath>
#include <vector>
#include <nlohmann/json.hpp>
extern "C" {
#include "global.h"
#include "src/overlays/actors/ovl_En_Elf/z_en_elf.h"
#include "src/overlays/actors/ovl_En_Ko/z_en_ko.h"
#include "src/overlays/actors/ovl_Bg_Dy_Yoseizo/z_bg_dy_yoseizo.h"
extern PlayState* gPlayState;
}
namespace { std::vector<const void*> replacedFairyLights; bool templeWindowLight=false,forestShaftLight=false; }
extern "C" int LightingLab_ReplacesTempleLightPlane() {
    return Fast::LightingLabActive()&&Fast::gLightingLab.dx11&&templeWindowLight;
}
extern "C" int LightingLab_ReplacesNativeLight(const void* info) {
    if(!Fast::LightingLabActive()||!Fast::gLightingLab.dx11)return 0;
    return std::find(replacedFairyLights.begin(),replacedFairyLights.end(),info)!=replacedFairyLights.end();
}

extern "C" int LightingLab_ShouldCast(float x,float y,float z,float radius) {
    const auto& lab=Fast::gLightingLab;
    if(!lab.dx11||!Fast::LightingLabActive()||!lab.offCamera)return 0;
    if(!lab.shadows) {
        if(!lab.naviShadows)return 0;
        for(int i=0;i<lab.fairyCount;i++) {
            const float* source=lab.fairyPosition[i];
            float dx=x-source[0],dy=y-source[1],dz=z-source[2],range=source[3]+std::max(radius,0.0f);
            if(dx*dx+dy*dy+dz*dz<range*range)return 1;
        }
        return 0;
    }
    const float range=lab.shadowDistance*1.5f+6500+std::max(radius,0.0f);
    const float dx=x-lab.center[0],dy=y-lab.center[1],dz=z-lab.center[2];
    return dx*dx+dy*dy+dz*dz<range*range;
}
extern "C" void LightingLab_RecordShadowActor(){Fast::gLightingLab.shadowActors++;}
extern "C" void LightingLab_RecordShadowRoom(){Fast::gLightingLab.shadowRooms++;}
extern "C" int LightingLab_ShouldDrawNativeActorShadow() {
    if(!Fast::LightingLabActive()||CVarGetInteger("gEnhancements.LightingLab.OriginalActorShadows",0))return 1;
    Fast::gLightingLab.nativeBlobsSuppressed++;return 0;
}
#include "ProgressionPrewarm.h"
namespace {
constexpr const char* comparisonCodes[]={"A0","S1","S2","P3","H4","G5","L6","T7","N8"};
constexpr const char* comparisonNames[]={"Your current settings","Sun shadows OFF","Off-camera shadow casters OFF","Material lighting OFF","HDRI OFF","Color grading + glow OFF","New lighting OFF","Normals + parallax OFF","Lighting + surface depth OFF"};
int comparisonCase=0;
class ComparisonWindow final:public Ship::GuiWindow {
public:
    using GuiWindow::GuiWindow;
    void InitElement() override {} void UpdateElement() override {} void DrawElement() override {}
    void Draw() override {
        if(!CVarGetInteger("gEnhancements.LightingLab.Comparison",0))return;
        static double changed=0;static bool hold=false;
        double now=ImGui::GetTime();if(!changed)changed=now;
        if(ImGui::IsKeyPressed(ImGuiKey_F8,false)) {
            CVarSetInteger("gEnhancements.LightingLab.Comparison",0);
            Ship::Context::GetRawInstance()->GetWindow()->GetGui()->SaveConsoleVariablesNextFrame();
            comparisonCase=0;changed=0;return;
        }
        if(ImGui::IsKeyPressed(ImGuiKey_F7,false)){hold=!hold;changed=now;}
        if(ImGui::IsKeyPressed(ImGuiKey_F6,false)){comparisonCase=(comparisonCase+1)%9;changed=now;}
        if(!gPlayState)changed=now;
        if(gPlayState&&!hold&&now-changed>=15){comparisonCase=(comparisonCase+1)%9;changed=now;}
        auto vp=ImGui::GetMainViewport();auto* draw=ImGui::GetForegroundDrawList(vp);
        float scale=std::clamp(vp->Size.y/720.0f,.75f,1.8f);ImVec2 a(vp->Pos.x+vp->Size.x*.40f,vp->Pos.y+24*scale);
        ImVec2 b(a.x+410*scale,a.y+142*scale);
        draw->AddRectFilled(a,b,IM_COL32(0,0,0,235),8*scale);
        draw->AddText(ImGui::GetFont(),68*scale,ImVec2(a.x+14*scale,a.y),IM_COL32(255,235,45,255),comparisonCodes[comparisonCase]);
        draw->AddText(ImGui::GetFont(),20*scale,ImVec2(a.x+14*scale,a.y+74*scale),IM_COL32_WHITE,comparisonNames[comparisonCase]);
        char text[120];snprintf(text,sizeof(text),"F6 next   F7 %s   F8 finish",hold?"resume":"hold");
        draw->AddText(ImGui::GetFont(),16*scale,ImVec2(a.x+14*scale,a.y+103*scale),IM_COL32_WHITE,text);
        snprintf(text,sizeof(text),"%s",!gPlayState?"Load your save, then walk + orbit":hold?"HOLDING this code":"Auto changes every 15 seconds");
        draw->AddText(ImGui::GetFont(),16*scale,ImVec2(a.x+14*scale,a.y+122*scale),IM_COL32(150,220,255,255),text);
    }
};
int testFrame=0,chuDraws=0;
float option(const char* suffix,float fallback,float low,float high) {
    return std::clamp(CVarGetFloat((std::string("gEnhancements.LightingLab.")+suffix).c_str(),fallback),low,high);
}
bool enabled(const char* suffix,bool fallback=true) {
    return CVarGetInteger((std::string("gEnhancements.LightingLab.")+suffix).c_str(),fallback)!=0;
}
// Rebuilt from live actors each frame: no pointers survive room transitions or despawns.
void CollectFairyLights() {
    auto& lab=Fast::gLightingLab;lab.fairyCount=0;lab.naviIndex=-1;replacedFairyLights.clear();
    lab.naviShadows=enabled("NaviShadows");lab.naviShadowDebug=false;
    lab.fairyBrightness=option("FairyBrightness",1.8f,0,6);
    lab.fairyRange=option("FairyRange",450,50,1600);
    // Native clock: sunrise 06:00, sunset 18:00. Fade over twilight, independent of sun sliders.
    const float sunHeight=-std::cos(float(gSaveContext.dayTime)*6.28318530718f/65536);
    float twilight=std::clamp((.08f-sunHeight)/.20f,0.0f,1.0f);
    lab.fairyNight=twilight*twilight*(3-2*twilight);
    // A glowing, visible fairy illuminates shaded daytime areas too unless night-only is selected.
    const float activation=enabled("FairyNightOnly",false)?lab.fairyNight:1.0f;
    const bool fixture=CVarGetInteger("gEnhancements.LightingLab.NativeTest",0)==5;
    if(fixture) {
        if(lab.phase==2)lab.fairyBrightness=0;
        if(lab.phase==3)lab.fairyBrightness=3.6f;
        if(lab.phase==4)lab.fairyRange=150;
        if(lab.phase==5)lab.fairyRange=900;
    }
    if(!enabled("FairyLights")||(fixture&&lab.phase==0)||lab.fairyBrightness<=0||activation<=0)return;
    struct Source {Vec3f pos;float fade,distance;bool navi;Actor* actor;};
    std::vector<Source> sources;
    for(int category=0;category<ACTORCAT_MAX;category++) {
        for(Actor* actor=gPlayState->actorCtx.actorLists[category].head;actor;actor=actor->next) {
            if(!actor->update||!actor->draw)continue;
            Vec3f pos=actor->world.pos;float fade=1;
            if(actor->id==ACTOR_EN_ELF) {
                auto* fairy=reinterpret_cast<EnElf*>(actor);
                if(actor->params==FAIRY_SPAWNER)continue;
                const bool hidden=fairy->unk_2A8==8||(fairy->fairyFlags&8);
                // Match EnElf_Draw: a tucked-away or otherwise hidden fairy emits no added light.
                if(hidden)continue;
                fade=std::clamp(actor->scale.x/.008f,0.0f,1.0f);
                if(fairy->disappearTimer<0)fade*=std::clamp(1+fairy->disappearTimer*(7.0f/6000),0.0f,1.0f);
            } else if(actor->id==ACTOR_BG_DY_YOSEIZO) {
                auto* fairy=reinterpret_cast<BgDyYoseizo*>(actor);
                fade=std::clamp(fairy->scale/.034f,0.0f,1.0f);
                pos.y+=fairy->heightOffset*.5f;
            } else continue;
            if(fade<=.001f||!std::isfinite(pos.x)||!std::isfinite(pos.y)||!std::isfinite(pos.z))continue;
            float dx=pos.x-lab.center[0],dy=pos.y-lab.center[1],dz=pos.z-lab.center[2];
            sources.push_back({pos,fade,dx*dx+dy*dy+dz*dz,actor->id==ACTOR_EN_ELF&&actor->params==FAIRY_NAVI,actor});
        }
    }
    std::stable_sort(sources.begin(),sources.end(),[](const Source& a,const Source& b){return a.navi!=b.navi?a.navi:a.distance<b.distance;});
    for(const auto& source:sources) {
        if(lab.fairyCount==Fast::LightingLabState::MaxFairyLights)break;
        int i=lab.fairyCount++;if(source.navi)lab.naviIndex=i;
        lab.fairyKey[i]=reinterpret_cast<uintptr_t>(source.actor);
        std::fill(lab.fairyCone[i],lab.fairyCone[i]+4,0.0f);
        if(source.actor->id==ACTOR_EN_ELF) {
            auto* fairy=reinterpret_cast<EnElf*>(source.actor);
            replacedFairyLights.push_back(&fairy->lightInfoGlow);
            replacedFairyLights.push_back(&fairy->lightInfoNoGlow);
        }
        float position[]={source.pos.x,source.pos.y,source.pos.z,lab.fairyRange};
        std::copy(position,position+4,lab.fairyPosition[i]);
        float energy=lab.fairyBrightness*activation*source.fade;
        lab.fairyColor[i][0]=energy*.65f;lab.fairyColor[i][1]=energy*.82f;lab.fairyColor[i][2]=energy;lab.fairyColor[i][3]=0;
    }
    if(CVarGetInteger("gEnhancements.LightingLab.NativeTest",0)==6&&lab.naviIndex>=0) {
        // Only this explicit save-free fixture places the source for opposite-side comparisons.
        if(lab.phase!=6) {
            float* source=lab.fairyPosition[lab.naviIndex];
            source[0]=lab.center[0]+(lab.phase>=3&&lab.phase<=5?-100:100);
            source[1]=lab.center[1]+90;source[2]=lab.center[2];
        }
        lab.naviShadows=lab.phase!=0&&lab.phase!=5;
        lab.naviShadowDebug=lab.phase==2||lab.phase==4;
    }
}
// Scene-authored lights are independent of the fairy brightness setting.
void CollectArchitecturalLights() {
    auto& lab=Fast::gLightingLab;templeWindowLight=false;forestShaftLight=false;
    if(!lab.dx11||!Fast::LightingLabActive()||Fast::OriginalGraphics()||!enabled("RealLightCards"))return;
    auto spot=[&](uintptr_t key,Vec3f p,Vec3f direction,float cosine,float range,Vec3f color) {
        if(lab.fairyCount>=Fast::LightingLabState::MaxFairyLights)return false;
        int i=lab.fairyCount++;
        lab.fairyKey[i]=key;
        float position[]={p.x,p.y,p.z,range};std::copy(position,position+4,lab.fairyPosition[i]);
        float cone[]={direction.x,direction.y,direction.z,cosine};std::copy(cone,cone+4,lab.fairyCone[i]);
        float energy[]={color.x,color.y,color.z,0};std::copy(energy,energy+4,lab.fairyColor[i]);
        return true;
    };
    if(gPlayState->sceneNum==SCENE_LOST_WOODS&&gPlayState->roomCtx.curRoom.num==1)
        forestShaftLight=spot(1,{1192,640,383},{0,-1,0},.965f,1800,{4.2f,4.0f,3.2f});
    if(gPlayState->sceneNum==SCENE_TEMPLE_OF_TIME&&
       (gPlayState->csCtx.state==CS_STATE_IDLE||gSaveContext.sceneLayer>=4)) {
        for(int category=0;category<ACTORCAT_MAX;category++)
        for(Actor* actor=gPlayState->actorCtx.actorLists[category].head;actor;actor=actor->next) {
            if(actor->id!=ACTOR_DEMO_KANKYO||actor->params!=14||!actor->draw)continue;
            // The original window beam's local vertices run from (0,300,-68)
            // toward (-40,-939,900). Keep the source just inside the window.
            float angle=actor->shape.rot.y*.0000958737992f;
            Vec3f p={actor->world.pos.x-58*std::sin(angle),actor->world.pos.y+300,
                     actor->world.pos.z-58*std::cos(angle)};
            Vec3f d={.615f*std::sin(angle),-.7885f,.615f*std::cos(angle)};
            templeWindowLight|=spot(reinterpret_cast<uintptr_t>(actor),p,d,.94f,2600,{5.0f,4.8f,4.4f});
        }
    }
}
// Hold the exact loaded resource while patched. This also handles an alternate
// resource being exchanged during the original/enhanced graphics hotkey cycle.
void UpdateLightCardGeometry() {
    static std::shared_ptr<Fast::DisplayList> patched;
    static Gfx original={};
    auto& lab=Fast::gLightingLab;
    bool replace=forestShaftLight&&lab.dx11&&Fast::LightingLabActive()&&!Fast::OriginalGraphics()&&enabled("RealLightCards")&&
        gPlayState&&gPlayState->sceneNum==SCENE_LOST_WOODS&&gPlayState->roomCtx.curRoom.num==1;
    std::shared_ptr<Fast::DisplayList> target;
    if(replace)target=std::dynamic_pointer_cast<Fast::DisplayList>(Ship::Context::GetRawInstance()->GetResourceManager()->
        LoadResource("scenes/shared/spot10_scene/spot10_room_1DL_007C18"));
    if(target==patched)return;
    if(patched){patched->Instructions[0]=original;patched.reset();}
    if(target&&!target->Instructions.empty()) {
        original=target->Instructions[0];target->Instructions[0]=gsSPEndDisplayList();patched=target;
    }
}
// Isolated authored-door visual fixture; never active in ordinary play.
Actor* FixtureDoor() {
    if(!gPlayState || CVarGetInteger("gEnhancements.LightingLab.NativeTest",0)!=8 ||
       !CVarGetInteger("gEnhancements.LightingLab.NativeDoorFocus",0))return nullptr;
    Actor* best=nullptr;float distance=1e20f;auto* player=GET_PLAYER(gPlayState);
    for(int category=0;category<ACTORCAT_MAX;category++)for(Actor* actor=gPlayState->actorCtx.actorLists[category].head;actor;actor=actor->next) {
        if(actor->id!=ACTOR_EN_DOOR || !actor->draw)continue;
        float dx=actor->world.pos.x-player->actor.world.pos.x,dz=actor->world.pos.z-player->actor.world.pos.z;
        if(dx*dx+dz*dz<distance){distance=dx*dx+dz*dz;best=actor;}
    }
    return best;
}
void Frame() {
    auto& lab=Fast::gLightingLab;lab.scene=-1;
    if(!gPlayState)return;
    const int nativeMode=CVarGetInteger("gEnhancements.LightingLab.NativeTest",0);
    lab.performanceTest=nativeMode==9;
    lab.optimize=!lab.performanceTest||CVarGetInteger("gEnhancements.LightingLab.NativeReference",0)==0;
    const bool fairyTest=nativeMode==5,shadowTest=nativeMode==6;
    const bool visibilityTest=nativeMode==7;
    if(fairyTest||shadowTest)lab.phase=std::min(testFrame/90,8);
    if(visibilityTest)lab.phase=std::min(testFrame/90,3);
    const int forcedTime=visibilityTest?CVarGetInteger("gEnhancements.LightingLab.NativeTime",0):shadowTest?(lab.phase==7?32768:0):fairyTest?(lab.phase==6?32768:0):CVarGetInteger("gEnhancements.LightingLab.NativeTime",-1);
    if(CVarGetInteger("gEnhancements.LightingLab.NativeTest",0)&&forcedTime>=0) {
        gSaveContext.dayTime=gSaveContext.skyboxTime=uint16_t(forcedTime);
        float angle=(forcedTime-32768)*6.28318530718f/65536;
        gPlayState->envCtx.sunPos={-std::sin(angle)*3000,std::cos(angle)*3000,std::cos(angle)*500};
    }
    if(gPlayState->sceneNum>=0&&gPlayState->sceneNum<SCENE_ID_MAX)lab.scene=gPlayState->sceneNum+2;
    if(gPlayState->sceneNum==SCENE_TEMPLE_OF_TIME)lab.scene=0;
    if(gPlayState->sceneNum==SCENE_KOKIRI_FOREST)lab.scene=1;
    lab.enabled=enabled("Enabled",true);lab.hdri=enabled("HDRI");lab.pbr=enabled("Materials");lab.shadows=enabled("SunShadows");
    lab.grading=enabled("Grading");lab.glow=enabled("Glow");lab.jelly=enabled("JellyShader");lab.spawnChuchu=false;
    lab.nativeTest=enabled("NativeTest",false);lab.offCamera=enabled("OffCameraShadows");
    lab.roughness=option("Roughness",.65f,.05f,1);lab.specular=option("Specular",.5f,0,1);
    lab.hdriStrength=option("HDRIStrength",.25f,0,3);lab.sunStrength=option("SunStrength",.45f,0,3);
    lab.exposure=option("Exposure",0,-4,4);lab.contrast=option("Contrast",1,.25f,2);
    lab.saturation=option("Saturation",1,0,2);lab.gamma=option("Gamma",1,.5f,2);
    lab.glowStrength=option("GlowStrength",.15f,0,2);lab.glowThreshold=option("GlowThreshold",.85f,.1f,4);
    lab.shadowStrength=option("ShadowStrength",.8f,0,1);lab.shadowDistance=option("ShadowDistance",2400,500,6000);
    lab.refraction=option("Refraction",.012f,0,.08f);lab.bodyOpacity=option("BodyOpacity",.7f,0,1);
    lab.pearlStrength=option("Pearlescence",.7f,0,2);lab.clearcoat=option("Clearcoat",.5f,0,1);
    lab.eye[0]=gPlayState->view.eye.x;lab.eye[1]=gPlayState->view.eye.y;lab.eye[2]=gPlayState->view.eye.z;
    auto* player=GET_PLAYER(gPlayState);
    // Deterministic isolated camera fixtures must not inherit live controller movement.
    if(lab.nativeTest&&enabled("NativeFreeze",false)) {
        player->actor.world.pos={option("NativeX",0,-30000,30000),option("NativeY",0,-30000,30000),option("NativeZ",2100,-30000,30000)};
        player->actor.prevPos=player->actor.world.pos;player->actor.speedXZ=0;player->actor.velocity={0,0,0};
    }
    lab.center[0]=player->actor.world.pos.x;lab.center[1]=player->actor.world.pos.y;lab.center[2]=player->actor.world.pos.z;
    lab.nearPlane=gPlayState->view.zNear;lab.farPlane=gPlayState->view.zFar;lab.time=float(gPlayState->gameplayFrames)/20;
    float azimuth=option("SunAzimuth",-28,-180,180)*.0174532925f,elevation=option("SunElevation",42,5,85)*.0174532925f;
    lab.sun[0]=std::sin(azimuth)*std::cos(elevation);lab.sun[1]=std::sin(elevation);lab.sun[2]=std::cos(azimuth)*std::cos(elevation);
    lab.sunColor[0]=1;lab.sunColor[1]=.91f;lab.sunColor[2]=.75f;
    lab.profile=0;lab.environment=lab.environmentNext=0;lab.environmentBlend=0;lab.environmentRotation=0;
    const float baseHDRI=lab.hdriStrength,baseSun=lab.sunStrength;
    if(enabled("SceneProfiles")&&(CVarGetInteger("gEnhancements.LightingLab.NativeTest",0)!=3||(testFrame+1)/90>=2)) {
        const int scene=gPlayState->sceneNum;
        const bool forest=scene==SCENE_KOKIRI_FOREST||scene==SCENE_LOST_WOODS||scene==SCENE_SACRED_FOREST_MEADOW;
        const bool outdoor=(scene>=SCENE_HYRULE_FIELD&&scene<=SCENE_OUTSIDE_GANONS_CASTLE&&scene!=SCENE_ZORAS_DOMAIN&&scene!=SCENE_GORON_CITY)
            ||(scene>=SCENE_MARKET_ENTRANCE_DAY&&scene<=SCENE_TEMPLE_OF_TIME_EXTERIOR_RUINS)
            ||scene==SCENE_CASTLE_COURTYARD_GUARDS_DAY||scene==SCENE_CASTLE_COURTYARD_GUARDS_NIGHT
            ||scene==SCENE_CASTLE_COURTYARD_ZELDA||scene==SCENE_GANON_BOSS||scene==SCENE_GANONS_TOWER_COLLAPSE_EXTERIOR;
        if(outdoor&&enabled("OutdoorNishita")) {
            lab.profile=1;lab.environment=1;lab.environmentNext=2;
            if(enabled("FollowTime")) {
                float x=gPlayState->envCtx.sunPos.x,y=gPlayState->envCtx.sunPos.y,z=gPlayState->envCtx.sunPos.z;
                float len=std::max(std::sqrt(x*x+y*y+z*z),1.0f);
                lab.sun[0]=x/len;lab.sun[1]=y/len;lab.sun[2]=z/len;
            }
            const float height=lab.sun[1];
            const float daylight=std::clamp(height/.18f,0.0f,1.0f);
            lab.environmentBlend=std::clamp((.6f-height)/.46f,0.0f,1.0f);
            // Baked panoramas use Y-up, with their low-sun bright region along +X.
            lab.environmentRotation=std::atan2(lab.sun[2],lab.sun[0]);
            if(height<.14f){lab.environment=2;lab.environmentNext=3;lab.environmentBlend=std::clamp((.14f-height)/.24f,0.0f,1.0f);}
            lab.hdriStrength*=.48f*(.18f+.82f*daylight);lab.sunStrength*=daylight;
            lab.sunColor[0]=1;lab.sunColor[1]=.65f+.30f*daylight;lab.sunColor[2]=.40f+.50f*daylight;
            if(height<=0)lab.shadows=false;
        }
        if(forest&&enabled("ForestHDRI")) {
            lab.profile=2;lab.environment=lab.environmentNext=4;lab.environmentBlend=0;lab.environmentRotation=0;
            if(enabled("FollowTime")) {
                float x=gPlayState->envCtx.sunPos.x,y=gPlayState->envCtx.sunPos.y,z=gPlayState->envCtx.sunPos.z;
                float len=std::max(std::sqrt(x*x+y*y+z*z),1.0f);lab.sun[0]=x/len;lab.sun[1]=y/len;lab.sun[2]=z/len;
            }
            float daylight=std::clamp(lab.sun[1]/.18f,0.0f,1.0f);
            lab.hdriStrength=baseHDRI*.3456f*(.18f+.82f*daylight);lab.sunStrength=baseSun*.45f*daylight;
            lab.sunColor[0]=1;lab.sunColor[1]=.65f+.30f*daylight;lab.sunColor[2]=.40f+.50f*daylight;
            if(lab.sun[1]<=0){lab.hdriStrength*=.25f;lab.sunStrength=0;lab.shadows=false;}
        }
        if(!outdoor&&enabled("InteriorLighting")) {
            lab.profile=3;lab.environment=lab.environmentNext=5;lab.hdriStrength*=.48f;
            lab.sunStrength=0;lab.shadows=false;lab.environmentBlend=0;
            lab.sunColor[0]=lab.sunColor[1]=lab.sunColor[2]=1;
        }
    }
    // Native lighting comparisons explicitly show Navi; ordinary play uses her actual visibility.
    // The visibility fixture separately exercises tucked-away and hidden-flag states.
    if((fairyTest||shadowTest||visibilityTest||nativeMode==8||nativeMode==9)&&!(shadowTest&&lab.phase==6)) {
        for(int category=0;category<ACTORCAT_MAX;category++)for(Actor* actor=gPlayState->actorCtx.actorLists[category].head;actor;actor=actor->next) {
            if(actor->id!=ACTOR_EN_ELF||actor->params!=FAIRY_NAVI||!actor->update)continue;
            auto* navi=reinterpret_cast<EnElf*>(actor);
            navi->unk_2A8=visibilityTest&&lab.phase==1?8:0;
            if(visibilityTest&&lab.phase==2)navi->fairyFlags|=8;else navi->fairyFlags&=~8;
            Actor_SetScale(actor,visibilityTest&&lab.phase==1?0.0f:.008f);
            actor->world.pos=player->actor.world.pos;actor->world.pos.x-=40;actor->world.pos.y+=65;
            if(auto* door=FixtureDoor()) {
                float angle=door->shape.rot.y*0.0000958737992f;
                actor->world.pos={door->world.pos.x+std::sin(angle)*55-std::cos(angle)*35,
                    door->world.pos.y+65,door->world.pos.z+std::cos(angle)*55+std::sin(angle)*35};
            }
        }
    }
    if(nativeMode==9&&enabled("NativeHideNavi",false)) {
        for(int category=0;category<ACTORCAT_MAX;category++)
        for(Actor* actor=gPlayState->actorCtx.actorLists[category].head;actor;actor=actor->next)
            if(actor->id==ACTOR_EN_ELF&&actor->params==FAIRY_NAVI) {
                auto* navi=reinterpret_cast<EnElf*>(actor);navi->unk_2A8=8;navi->fairyFlags|=8;Actor_SetScale(actor,0);
            }
    }
    CollectFairyLights();
    CollectArchitecturalLights();
    if(Fast::OriginalGraphics())lab.enabled=false;
    lab.comparison=enabled("Comparison",false)?comparisonCase:-1;
    if(lab.comparison==1)lab.shadows=false;
    if(lab.comparison==2)lab.offCamera=false;
    if(lab.comparison==3)lab.pbr=false;
    if(lab.comparison==4)lab.hdri=false;
    if(lab.comparison==5)lab.grading=lab.glow=false;
    if(lab.comparison==6||lab.comparison==8)lab.enabled=false;
    static int lastComparison=-2;
    if(lastComparison!=lab.comparison) {
        lastComparison=lab.comparison;
        if(lab.comparison>=0) {
            nlohmann::json record={{"code",comparisonCodes[lab.comparison]},{"setting",comparisonNames[lab.comparison]},{"scene",gPlayState->sceneNum},{"shadows",lab.shadows},{"off_camera",lab.offCamera},{"lighting",lab.enabled},{"pbr",lab.pbr},{"hdri",lab.hdri},{"grading",lab.grading},{"glow",lab.glow},{"distance",lab.shadowDistance},{"strength",lab.shadowStrength}};
            std::ofstream("lighting-comparison-codes.jsonl",std::ios::app)<<record.dump()<<"\n";
            lab.capture=3100+lab.comparison;
        }
    }
    if(nativeMode==9) {
        lab.performanceFrame=++testFrame;
        const bool cycle=CVarGetInteger("gSettings.GraphicsModeNativeCycle",0)!=0;
        const bool controls=CVarGetInteger("gSettings.GraphicsControlsNativeTest",0)!=0;
        const bool timing=CVarGetInteger("gSettings.GraphicsTimingNativeTest",0)!=0;
        const bool townSweep=enabled("NativeTownSweep",false);
        const int endFrame=cycle?720:controls?880:townSweep?600:400;
        if(townSweep && testFrame>=80) {
            const int view=std::min((testFrame-80)/60,7);
            // Exercise the real free-look camera, including its wall collision.
            // A forced render-eye orbit can sit inside a building and cannot
            // establish whether the corresponding gameplay view is complete.
            gPlayState->manualCamera=true;
            gPlayState->camX=static_cast<s16>(view*8192);
            gPlayState->camY=0x0A00;
            if(testFrame==135+view*60) {
                lab.capture=4800+view;
                const auto& eye=gPlayState->mainCamera.eye;
                std::ofstream("performance-town-views.jsonl",std::ios::app)<<nlohmann::json({
                    {"view",view},{"scene",gPlayState->sceneNum},{"manual_camera",gPlayState->manualCamera},
                    {"camera_setting",gPlayState->mainCamera.setting},{"eye",{eye.x,eye.y,eye.z}}
                }).dump()<<"\n";
            }
        }
        using Ship::KbScancode;
        const int heldKey=cycle?KbScancode::LUS_KB_OEM_3:KbScancode::LUS_KB_SUBTRACT;
        auto tap=[](int key) { Fast::GraphicsHotkeyDown(key);Fast::GraphicsHotkeyUp(key); };
        if(controls && testFrame%80==0 && testFrame<endFrame) {
            int key=(testFrame<=320||testFrame==800)?KbScancode::LUS_KB_SUBTRACT:KbScancode::LUS_KB_ADD;
            Fast::GraphicsHotkeyDown(key);
            if(testFrame!=160)Fast::GraphicsHotkeyUp(key);
        }
        if(controls && testFrame==160)Fast::GraphicsHotkeyDown(KbScancode::LUS_KB_MULTIPLY);
        if(controls && testFrame==165)Fast::GraphicsHotkeyDown(KbScancode::LUS_KB_MULTIPLY);
        if(controls && testFrame==180)Fast::GraphicsHotkeyUp(KbScancode::LUS_KB_MULTIPLY);
        if(controls && testFrame==320)tap(KbScancode::LUS_KB_MULTIPLY);
        if(controls && (testFrame==400||testFrame==405))Fast::GraphicsHotkeyDown(Fast::graphicsVsyncHotkey);
        if(controls && testFrame==420)Fast::GraphicsHotkeyUp(Fast::graphicsVsyncHotkey);
        if(controls && testFrame==480)tap(Fast::graphicsVsyncHotkey);
        if(controls) {
            const double now=std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
            std::ofstream("performance-ticks.csv",std::ios::app)<<testFrame<<","<<std::setprecision(15)<<now<<"\n";
        }
        if(cycle && testFrame%80==0 && testFrame<endFrame) {
            Fast::GraphicsHotkeyDown(KbScancode::LUS_KB_OEM_3);
            if(testFrame!=160)Fast::GraphicsHotkeyUp(KbScancode::LUS_KB_OEM_3);
        }
        if((cycle||controls) && testFrame==165)Fast::GraphicsHotkeyDown(heldKey); // held-key repeat
        if((cycle||controls) && testFrame==180)Fast::GraphicsHotkeyUp(heldKey);
        if((cycle||controls) && testFrame%80==75) {
            auto texture=Ship::Context::GetRawInstance()->GetResourceManager()->LoadResource(
                "objects/gameplay_field_keep/gFieldDoor1Tex");
            nlohmann::json state={
                {"frame",testFrame},{"mode",Fast::GraphicsMode()},
                {"skybox",gPlayState->skyboxId},{"camera_setting",gPlayState->mainCamera.setting},
                {"camera_data",gPlayState->mainCamera.camDataIdx},
                {"target_fps",Fast::GraphicsTargetFps()},{"counter",Fast::graphicsFpsDrawn},
                {"vsync",Fast::GraphicsVsyncEnabled()},
                {"counter_text",Fast::graphicsFpsText},
                {"width",Fast::graphicsRenderWidth},{"height",Fast::graphicsRenderHeight},
                {"output_width",Fast::graphicsOutputWidth},{"output_height",Fast::graphicsOutputHeight},
                {"lighting",lab.enabled},{"off_camera",lab.offCamera},
                {"normals",Fast::gMaterialRelief.normals},{"parallax",Fast::gMaterialRelief.parallax},
                {"background_normals",Fast::gMaterialRelief.backgroundNormals},
                {"background_parallax",Fast::gMaterialRelief.backgroundParallax},
                {"alternate_graphics",Ship::gAlternateGraphicsEnabled.load()},
                {"texture",texture?texture->GetInitData()->Path:"MISSING"},
                {"audio_alternates",Ship::AllowAlternateResource("audio/sequences/test")}
            };
            std::ofstream(controls?"performance-controls.jsonl":"performance-mode-cycle.jsonl",std::ios::app)<<state.dump()<<"\n";
            if(!timing)lab.capture=4600+testFrame/80;
        }
        if(!timing&&!cycle&&!controls&&!Fast::OriginalGraphics()&&(testFrame==200||testFrame==300))lab.validateShadows=true;
        if(!timing&&testFrame==340) {
            lab.capture=4500;
            if(enabled("NativeGeometry",false))Fast::gMaterialRelief.capture=41;
            nlohmann::json actors=nlohmann::json::array();
            for(int category=0;category<ACTORCAT_MAX;category++)
            for(Actor* actor=gPlayState->actorCtx.actorLists[category].head;actor;actor=actor->next)
                actors.push_back({{"id",actor->id},{"params",actor->params},{"pos",{actor->world.pos.x,actor->world.pos.y,actor->world.pos.z}}});
            std::ofstream("performance-actors.json")<<actors.dump(2);
            nlohmann::json sources=nlohmann::json::array();
            for(int i=0;i<lab.fairyCount;i++)sources.push_back({{"navi",i==lab.naviIndex},
                {"pos",{lab.fairyPosition[i][0],lab.fairyPosition[i][1],lab.fairyPosition[i][2],lab.fairyPosition[i][3]}}});
            std::ofstream("performance-sources.json")<<sources.dump(2);
        }
        if(testFrame==endFrame) {
            std::ofstream("performance-complete.json")<<nlohmann::json({
                {"completed",true},{"scene",gPlayState->sceneNum},{"optimized",lab.optimize},
                {"graphics_mode",Fast::GraphicsMode()},{"lighting_enabled",lab.enabled},
                {"target_fps",Fast::GraphicsTargetFps()},{"counter",Fast::graphicsFpsDrawn},
                {"render_width",Fast::graphicsRenderWidth},{"render_height",Fast::graphicsRenderHeight},
                {"output_width",Fast::graphicsOutputWidth},{"output_height",Fast::graphicsOutputHeight},
                {"alternate_graphics",Ship::gAlternateGraphicsEnabled.load()},
                {"alternate_audio",Ship::AllowAlternateResource("audio/sequences/test")},
                {"navi",lab.naviIndex},{"fairies",lab.fairyCount},{"sun",lab.shadows},
                {"off_camera",lab.offCamera},{"shadow_overflow",lab.shadowOverflow},
                {"normals",enabled("BackgroundNormals")},{"parallax",enabled("BackgroundParallax")},
                {"navi_shadow_passes",lab.naviShadowPasses},{"sun_shadow_passes",lab.shadowPasses},
                {"off_camera_triangles",lab.offCameraTriangles}
            }).dump(2);
            Ship::Context::GetRawInstance()->GetWindow()->Close();
        }
        return;
    }
    if(CVarGetInteger("gEnhancements.LightingLab.NativeTest",0)==8) {
        int phase=std::min(testFrame/90,3);
        auto& relief=Fast::gMaterialRelief;
        static int warmup=0;
        if(warmup++<150) {
            relief.backgroundNormals=relief.backgroundParallax=false;
            CVarSetInteger("gEnhancements.LightingLab.BackgroundNormals",0);
            CVarSetInteger("gEnhancements.LightingLab.BackgroundParallax",0);
            return;
        }
        relief.backgroundNormals=(phase&1)!=0;relief.backgroundParallax=(phase&2)!=0;
        CVarSetInteger("gEnhancements.LightingLab.BackgroundNormals",relief.backgroundNormals);CVarSetInteger("gEnhancements.LightingLab.BackgroundParallax",relief.backgroundParallax);
        if(testFrame%90==74) {
            nlohmann::json report={{"phase",phase},{"scene",gPlayState->sceneNum},
                {"normals",relief.backgroundNormals},{"parallax",relief.backgroundParallax},
                {"draws",relief.draws[2][phase]},{"fairies",lab.fairyCount},{"navi",lab.naviIndex},
                {"shadow_passes",lab.naviShadowPasses},{"overflow",lab.shadowOverflow}};
            std::ofstream("background-relief-"+std::to_string(phase)+".json")<<report.dump(2);
            lab.capture=4400+phase;
        }
        if(++testFrame==360)Ship::Context::GetRawInstance()->GetWindow()->Close();
        return;
    }
    if(visibilityTest) {
        if(testFrame%90==74) {
            nlohmann::json state={{"phase",lab.phase},{"lights",lab.fairyCount},{"navi_index",lab.naviIndex},
                {"shadow_passes",lab.naviShadowPasses},{"night",lab.fairyNight},{"sun_passes",lab.shadowPasses},
                {"night_only",enabled("FairyNightOnly",false)}};
            for(int category=0;category<ACTORCAT_MAX;category++)for(Actor* actor=gPlayState->actorCtx.actorLists[category].head;actor;actor=actor->next) {
                if(actor->id==ACTOR_EN_ELF&&actor->params==FAIRY_NAVI&&actor->update) {
                    auto* navi=reinterpret_cast<EnElf*>(actor);
                    state["navi_state"]=navi->unk_2A8;state["hidden_flag"]=(navi->fairyFlags&8)!=0;
                    state["scale"]=actor->scale.x;
                }
            }
            std::ofstream("navi-visibility-"+std::to_string(lab.phase)+".json")<<state.dump(2);
            lab.capture=4300+lab.phase;
        }
        if(++testFrame==360) {
            std::ofstream("navi-visibility-result.json")<<nlohmann::json({{"completed",true},{"scene",gPlayState->sceneNum}}).dump(2);
            Ship::Context::GetRawInstance()->GetWindow()->Close();
        }
        return;
    }
    if(shadowTest) {
        lab.enabled=lab.phase!=8;
        if(testFrame%90==74) {
            nlohmann::json state={{"phase",lab.phase},{"scene",gPlayState->sceneNum},{"enabled",lab.enabled},
                {"navi_index",lab.naviIndex},{"night",lab.fairyNight},{"shadows",lab.naviShadows},
                {"sun_shadows",lab.shadows},{"sun_passes",lab.shadowPasses},{"shadow_passes",lab.naviShadowPasses},
                {"depth_pixels",lab.naviDepthPixels},{"shadow_triangles",lab.naviShadowTriangles},
                {"shadow_overflow",lab.shadowOverflow},{"debug",lab.naviShadowDebug},
                {"player",{lab.center[0],lab.center[1],lab.center[2]}}};
            if(lab.naviIndex>=0) {const float* source=lab.fairyPosition[lab.naviIndex];state["source"]={source[0],source[1],source[2],source[3]};}
            std::ofstream("navi-shadow-state-"+std::to_string(lab.phase)+".json")<<state.dump(2);
            lab.capture=4200+lab.phase;
        }
        if(++testFrame==810) {
            nlohmann::json result={{"passed",lab.naviShadowPasses>0&&lab.naviDepthPixels>100&&lab.shadowOverflow==0},
                {"scene",gPlayState->sceneNum},{"shadow_passes",lab.naviShadowPasses},{"shadow_triangles",lab.naviShadowTriangles},
                {"depth_pixels",lab.naviDepthPixels},{"sun_passes",lab.shadowPasses},{"shadow_overflow",lab.shadowOverflow}};
            std::ofstream("navi-shadow-result.json")<<result.dump(2);
            Ship::Context::GetRawInstance()->GetWindow()->Close();
        }
        return;
    }
    if(fairyTest) {
        lab.enabled=lab.phase!=8;
        if(testFrame%90==74) {
            nlohmann::json state={{"phase",lab.phase},{"lights",lab.fairyCount},{"night",lab.fairyNight},
                {"brightness",lab.fairyBrightness},{"range",lab.fairyRange},{"enabled",lab.enabled},
                {"day_time",gSaveContext.dayTime},{"kokiri_fade_disabled",CVarGetInteger("gEnhancements.DisableKokiriDrawDistance",0)}};
            state["sources"]=nlohmann::json::array();state["fairy_types"]=nlohmann::json::array();state["kokiri"]=nlohmann::json::array();
            for(int i=0;i<lab.fairyCount;i++)state["sources"].push_back({lab.fairyPosition[i][0],lab.fairyPosition[i][1],lab.fairyPosition[i][2],lab.fairyPosition[i][3]});
            for(int category=0;category<ACTORCAT_MAX;category++)for(Actor* actor=gPlayState->actorCtx.actorLists[category].head;actor;actor=actor->next) {
                if(actor->id==ACTOR_EN_ELF&&actor->update)state["fairy_types"].push_back(actor->params);
                if(actor->id==ACTOR_EN_KO&&actor->update) {
                    auto* ko=reinterpret_cast<EnKo*>(actor);
                    state["kokiri"].push_back({{"alpha",ko->modelAlpha},{"distance",actor->xzDistToPlayer},{"fade_distance",ko->appearDist}});
                }
            }
            std::ofstream("fairy-state-"+std::to_string(lab.phase)+".json")<<state.dump(2);
            lab.capture=4100+lab.phase;
        }
        if(++testFrame==810) {
            std::ofstream("fairy-native-result.json")<<nlohmann::json({{"passed",lab.materialDraws>0},{"scene",gPlayState->sceneNum},{"draws",lab.materialDraws}}).dump(2);
            Ship::Context::GetRawInstance()->GetWindow()->Close();
        }
        return;
    }
    if(CVarGetInteger("gEnhancements.LightingLab.NativeTest",0)==4)return;
    if(!lab.nativeTest||lab.scene<0)return;
    const bool worldTest=CVarGetInteger("gEnhancements.LightingLab.NativeTest",0)>=2;
    const int phases=worldTest?4:8;
    testFrame++;lab.phase=std::min(testFrame/90,phases-1);
    lab.enabled=lab.phase>0;lab.hdri=lab.phase>0;
    if(worldTest) {
        lab.pbr=lab.phase>0;lab.shadows=lab.shadows&&lab.phase>0;lab.offCamera=lab.phase>=2;
        lab.grading=lab.phase>=3;lab.glow=lab.phase>=3;lab.jelly=false;
    } else {
        lab.pbr=lab.phase>1;lab.shadows=lab.shadows&&lab.phase>2;
        lab.grading=lab.phase>3;lab.glow=lab.phase>4;lab.jelly=lab.phase>5;
        if(lab.phase>=2&&lab.phase<=6){lab.roughness=.12f;lab.specular=.85f;}
        if(lab.phase==4){lab.exposure=.5f;lab.contrast=1.1f;lab.saturation=1.1f;}
        if(lab.phase==5){lab.glowStrength=.6f;lab.glowThreshold=.4f;}
    }
    // Native ChuChu contact attacks are disabled; no player hurt-flash fixture is needed.
    if(testFrame%90==75) {
        nlohmann::json state={{"profile",lab.profile},{"environment",lab.environment},{"next",lab.environmentNext},{"blend",lab.environmentBlend},{"sun",{lab.sun[0],lab.sun[1],lab.sun[2]}},{"hdri_strength",lab.hdriStrength},{"sun_strength",lab.sunStrength},{"day_time",gSaveContext.dayTime},{"sun_shadows",lab.shadows}};
        std::ofstream("scene-profile-"+std::to_string(lab.phase)+".json")<<state.dump(2);
    }
    if(enabled("NativeWalkOrbit",false)&&testFrame>=180&&testFrame%10==0) {
        lab.capture=2000+testFrame;
        nlohmann::json pose={{"frame",testFrame},{"player",{player->actor.world.pos.x,player->actor.world.pos.y,player->actor.world.pos.z}},{"eye",{gPlayState->view.eye.x,gPlayState->view.eye.y,gPlayState->view.eye.z}},{"camera_yaw",gPlayState->camX}};
        std::ofstream trace("native-walk-orbit.jsonl",testFrame==180?std::ios::out:std::ios::app);trace<<pose.dump()<<"\n";
    }
    if(testFrame%90==75){Fast::gMaterialRelief.capture=lab.phase+(worldTest?21:11);lab.capture=1000+lab.phase;}
    if(testFrame==phases*90) {
        nlohmann::json result={{"scene",lab.scene},{"game_scene",gPlayState->sceneNum},{"off_camera_triangles",lab.offCameraTriangles},{"frustum_triangles",lab.frustumTriangles},{"explicit_triangles",lab.explicitTriangles},{"off_camera_depth_changes",lab.offCameraDepthChanges},{"shadow_overflow",lab.shadowOverflow},{"shadow_actor_draws",lab.shadowActors},{"shadow_room_chunks",lab.shadowRooms},{"shadow_passes",lab.shadowPasses},{"shadow_triangles",lab.shadowTriangles},
            {"native_blob_shadows_suppressed",lab.nativeBlobsSuppressed},{"shadow_nonclear_pixels",lab.shadowNonclear},{"chuchu_position",{lab.chuchuOrigin[0],lab.chuchuOrigin[1],lab.chuchuOrigin[2]}},{"post_passes",lab.postPasses},{"jelly_draws",lab.jellyDraws},{"chuchu_draw_calls",chuDraws},{"player_position",{lab.center[0],lab.center[1],lab.center[2]}},{"camera_position",{lab.eye[0],lab.eye[1],lab.eye[2]}},{"chuchu_spawned",lab.chuchuSpawned},
            {"chuchu_hits",lab.chuchuHits},{"chuchu_deaths",lab.chuchuDeaths}};
        bool passed=lab.shadowPasses>0&&lab.shadowTriangles>0&&lab.shadowNonclear>0&&lab.postPasses>0;
        for(int i=0;i<phases;i++){result["phase_draws"][i]=lab.phaseDraws[i];result["phase_jelly"][i]=lab.phaseJelly[i];if(i>0)passed&=lab.phaseDraws[i]>0;}
        passed&=lab.shadowOverflow==0;result["world_test"]=worldTest;result["passed"]=passed;std::ofstream("lighting-native-test.json")<<result.dump(2);
        Ship::Context::GetRawInstance()->GetWindow()->Close();
    }
}
void Register() {
    ProgressionPrewarm::Register();
    auto comparisonWindow=std::make_shared<ComparisonWindow>("gWindows.LightingComparison",true,"Lighting comparison");
    Ship::Context::GetRawInstance()->GetWindow()->GetGui()->AddGuiWindow(comparisonWindow);comparisonWindow->Show();
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnGameFrameUpdate>(Frame);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayDrawBegin>([] {
        UpdateLightCardGeometry();
        if(!gPlayState||!CVarGetInteger("gEnhancements.LightingLab.NativeTest",0))return;
        // Original backdrops require their native camera. Do not make an
        // original-mode regression capture use the enhanced orbit fixture.
        if(Fast::OriginalGraphics())return;
        if(enabled("NativeTownSweep",false))return;
        float yaw=CVarGetFloat("gEnhancements.LightingLab.NativeYaw",-999);
        if(auto* door=FixtureDoor()) {
            float angle=door->shape.rot.y*0.0000958737992f;
            gPlayState->view.eye={door->world.pos.x+std::sin(angle)*125+std::cos(angle)*45,
                door->world.pos.y+62,door->world.pos.z+std::cos(angle)*125-std::sin(angle)*45};
            gPlayState->view.lookAt={door->world.pos.x,door->world.pos.y+60,door->world.pos.z};
            gPlayState->view.up={0,1,0};
            Fast::gLightingLab.eye[0]=gPlayState->view.eye.x;Fast::gLightingLab.eye[1]=gPlayState->view.eye.y;Fast::gLightingLab.eye[2]=gPlayState->view.eye.z;
            return;
        }
        if(yaw<-180)return;
        auto* player=GET_PLAYER(gPlayState);float angle=yaw*.0174532925f;
        float distance=CVarGetFloat("gEnhancements.LightingLab.NativeDistance",400),height=CVarGetFloat("gEnhancements.LightingLab.NativeHeight",160);
        gPlayState->view.eye={player->actor.world.pos.x+std::sin(angle)*distance,player->actor.world.pos.y+height,player->actor.world.pos.z+std::cos(angle)*distance};
        gPlayState->view.lookAt={player->actor.world.pos.x,player->actor.world.pos.y+50,player->actor.world.pos.z};
        gPlayState->view.up={0,1,0};
        Fast::gLightingLab.eye[0]=gPlayState->view.eye.x;Fast::gLightingLab.eye[1]=gPlayState->view.eye.y;Fast::gLightingLab.eye[2]=gPlayState->view.eye.z;
    });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayDestroy>([] {testFrame=0;chuDraws=0;Fast::gLightingLab.scene=-1;replacedFairyLights.clear();templeWindowLight=forestShaftLight=false;UpdateLightCardGeometry();});
}
static RegisterShipInitFunc registerLab(Register);
}

// Input injection is confined to the explicit, save-free native fixture.
extern "C" void LightingLab_NativeInput(void* raw) {
    if(CVarGetInteger("gEnhancements.Prewarm.NativeTest",0)==2){*static_cast<Input*>(raw)={};return;}
    if(gPlayState&&(CVarGetInteger("gEnhancements.LightingLab.NativeTest",0)==6||CVarGetInteger("gEnhancements.LightingLab.NativeTest",0)==7||CVarGetInteger("gEnhancements.LightingLab.NativeTest",0)==8||CVarGetInteger("gEnhancements.LightingLab.NativeTest",0)==9)){*static_cast<Input*>(raw)={};return;}
    if(!gPlayState||!CVarGetInteger("gEnhancements.LightingLab.NativeTest",0)||!CVarGetInteger("gEnhancements.LightingLab.NativeWalkOrbit",0))return;
    Input* input=static_cast<Input*>(raw);*input={};
    auto* player=GET_PLAYER(gPlayState);Vec3f target={-200,0,900};
    s16 relative=Math_Vec3f_Yaw(&player->actor.world.pos,&target)-Camera_GetInputDirYaw(GET_ACTIVE_CAM(gPlayState));
    input->cur.stick_x=input->rel.stick_x=s8(-std::sin(relative*3.14159265359f/32768)*42);
    input->cur.stick_y=input->rel.stick_y=s8(std::cos(relative*3.14159265359f/32768)*42);
    if(CVarGetInteger("gEnhancements.MirroredWorld",0))input->cur.stick_x=input->rel.stick_x=-input->cur.stick_x;
    input->cur.right_stick_x=input->rel.right_stick_x=12;
}


