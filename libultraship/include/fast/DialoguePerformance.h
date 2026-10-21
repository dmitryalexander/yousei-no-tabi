#pragma once
#include <atomic>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <vector>

namespace Fast {
namespace DialoguePerformance {
inline std::atomic<bool> enabled{false};
struct Event { const char* stage; double start, duration; };
inline std::mutex mutex;
inline std::vector<Event> events;
inline double Now() {
    return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now().time_since_epoch()).count();
}
inline void Record(const char* stage, double start, double duration) {
    std::lock_guard lock(mutex);
    events.push_back({stage,start,duration});
}
struct Scope {
    const char* stage; double start;
    explicit Scope(const char* name):stage(name),start(enabled ? Now() : 0) {}
    ~Scope() { if(start)Record(stage,start,Now()-start); }
};
inline void Present() {
    if(!enabled)return;
    static double previous=0;
    const double now=Now();
    Record("present",now,previous ? now-previous : 0);
    previous=now;
}
inline void Save() {
    std::lock_guard lock(mutex);
    std::ofstream out("dialogue-timings.csv");
    out<<"stage,start_ms,duration_ms\n"<<std::setprecision(15);
    for(const auto& event:events)out<<event.stage<<","<<event.start<<","<<event.duration<<"\n";
}
}
}
