#pragma once
#include <atomic>
#include <chrono>
#include <vector>
#include <tuple>
#include <string>
#include <map>
#include <set>
#include <list>
#include <memory>
#include <functional>
#include <mutex>
#include <cstdint>

namespace Fast::Prewarm {
// Only CPU bytecode/maps are built off-thread. Device/context work stays on the renderer.
inline std::function<bool(uint64_t,uint64_t,bool,bool)> compile;
inline thread_local std::string rejectedReason;
using SourceKey=std::tuple<uint64_t,uint64_t,bool,bool>;
inline SourceKey Key(uint64_t a,uint64_t b,bool filter,bool srgb) {
    // The interpreter sign-extends its default shader ID. Features only consume
    // these low 33 bits, so static predictions and real render IDs share a key.
    return {a,b&0x1ffffffffull,filter,srgb};
}
struct PreparedSource {std::string text;size_t floats;};
struct SourceEntry {std::shared_ptr<const PreparedSource> value;std::list<SourceKey>::iterator age;};
inline std::mutex sourceMutex;
inline std::map<SourceKey,SourceEntry> sources;
inline std::list<SourceKey> sourceAge;
inline size_t sourceBytes=0;
inline std::atomic<uint64_t> sourceEpoch{0};
inline std::shared_ptr<const PreparedSource> FindSource(const SourceKey& key) {
    std::lock_guard lock(sourceMutex);auto it=sources.find(key);
    return it==sources.end()?nullptr:it->second.value;
}
inline void Forget(std::map<SourceKey,SourceEntry>::iterator it) {
    sourceBytes-=it->second.value->text.size();sourceAge.erase(it->second.age);sources.erase(it);
}
inline void RememberSource(const SourceKey& key,std::string text,size_t floats,uint64_t epoch) {
    std::lock_guard lock(sourceMutex);
    if(epoch!=sourceEpoch||sources.contains(key)||text.size()>16*1024*1024)return;
    sourceAge.push_back(key);sourceBytes+=text.size();
    sources.emplace(key,SourceEntry{std::make_shared<const PreparedSource>(PreparedSource{std::move(text),floats}),std::prev(sourceAge.end())});
    while(sourceBytes>16*1024*1024&&!sourceAge.empty())Forget(sources.find(sourceAge.front()));
}
inline void TrimSources(const std::set<SourceKey>& needed) {
    std::lock_guard lock(sourceMutex);
    for(auto it=sources.begin();it!=sources.end();) {
        if(!needed.contains(it->first)){auto old=it++;Forget(old);}else ++it;
    }
}
inline void ClearSources() {
    std::lock_guard lock(sourceMutex);++sourceEpoch;sources.clear();sourceAge.clear();sourceBytes=0;
}
inline std::function<void(const uint8_t*,uint32_t,uint32_t,const std::string&)> relief;
inline std::function<void()> shutdown;
inline std::atomic<bool> threePoint{false}, srgb{false};
inline std::atomic<int> scene{-1};
inline std::atomic<double> frameBudgetMs{16.67};
inline std::atomic<int64_t> yieldUntil{0};
inline int64_t Clock() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}
inline std::mutex observationMutex;
inline std::vector<std::tuple<int,uint64_t,uint64_t>> observations;
inline void Observe(uint64_t a,uint64_t b) {
    std::lock_guard lock(observationMutex);
    if(observations.size()<2048)observations.emplace_back(scene.load(),a,b);
}
inline void Stop() { if(shutdown)shutdown(); }
}
