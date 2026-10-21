#define NOMINMAX
#include <windows.h>
#include <d3dcompiler.h>
#include "fast/ReliefCache.h"
#include <cassert>
#include <iostream>

int main() {
    Fast::DialoguePerformance::enabled=true;
    const char* name="alt/scenes/shared/spot04_scene/spot04_sceneTex_00FE18";
    std::vector<uint8_t> pixels(64*64*4);
    for(size_t i=0;i<pixels.size();i+=4) {
        pixels[i]=uint8_t(i%251);pixels[i+1]=uint8_t((i/17)%255);
        pixels[i+2]=123;pixels[i+3]=i%13 ? 255 : 127;
    }
    const auto reference=Fast::BuildReliefPixels(pixels.data(),64,64,name);
    auto equal=[&](const Fast::ReliefPixels& map) {
        return map.rgba==reference.rgba && map.width==reference.width &&
            map.height==reference.height && map.translucent==reference.translucent;
    };
    auto load=[&] {return Fast::ReliefCache::Build(pixels.data(),64,64,name,"test-v1");};
    assert(equal(load()));assert(equal(load()));
    const auto path=std::filesystem::directory_iterator("cache/relief-v1")->path();
    { std::fstream file(path,std::ios::binary|std::ios::in|std::ios::out);
      file.seekg(-1,std::ios::end);char byte=0;file.get(byte);
      file.seekp(-1,std::ios::end);file.put(byte^0x5a); }
    assert(equal(load()));
    { std::ofstream file(path,std::ios::binary|std::ios::trunc);file<<"truncated"; }
    assert(equal(load()));
    { std::fstream file(path,std::ios::binary|std::ios::in|std::ios::out);
      uint64_t oversized=UINT64_MAX;file.seekp(sizeof(uint64_t));
      file.write(reinterpret_cast<const char*>(&oversized),sizeof(oversized)); }
    assert(equal(load()));
    assert(equal(Fast::ReliefCache::Build(pixels.data(),64,64,name,"test-v2")));
    pixels[0]^=0x7f;
    const auto changed=Fast::BuildReliefPixels(pixels.data(),64,64,name);
    assert(Fast::ReliefCache::Build(pixels.data(),64,64,name,"test-v1").rgba==changed.rgba);
    int hits=0,generated=0;
    for(const auto& event:Fast::DialoguePerformance::events) {
        if(std::string(event.stage)=="relief_cache_hit")++hits;
        if(std::string(event.stage)=="relief_generate")++generated;
    }
    assert(hits==1 && generated==6);
    std::cout<<"{\"passed\":true,\"byte_identical_maps\":true,\"cache_hits\":"<<hits
        <<",\"generated\":"<<generated
        <<",\"checks\":[\"pixel invalidation\",\"algorithm invalidation\",\"corruption fallback\","
          "\"truncation fallback\",\"oversized dimensions rejected\"]}\n";
}
