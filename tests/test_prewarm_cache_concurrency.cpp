#define NOMINMAX
#include <windows.h>
#include <d3dcompiler.h>
#include "fast/ReliefCache.h"
#include <future>
#include <cassert>
#include <iostream>
int main() {
    auto module=LoadLibraryW(L"D3DCompiler_47.dll");assert(module);
    auto compiler=reinterpret_cast<decltype(&D3DCompile)>(GetProcAddress(module,"D3DCompile"));
    const std::string source="float4 PSMain():SV_Target{return float4(0,1,0,1);}";
    std::vector<uint8_t> pixels(32*32*4,255);for(size_t i=0;i<pixels.size();i+=4)pixels[i]=uint8_t(i);
    const std::string name="alt/scenes/shared/spot04_scene/publish-test";
    auto expected=Fast::BuildReliefPixels(pixels.data(),32,32,name);
    std::vector<std::future<std::vector<char>>> tasks;
    for(int i=0;i<12;++i)tasks.push_back(std::async(std::launch::async,[&] {
        std::vector<char> result;
        for(int pass=0;pass<10;++pass) {
            ID3DBlob *code=nullptr,*errors=nullptr;
            auto hr=Fast::D3DShaderCache::Compile(compiler,module,source.data(),source.size(),"PSMain","ps_4_0",0,&code,&errors);
            if(errors)errors->Release();assert(SUCCEEDED(hr)&&code);
            auto* data=static_cast<const char*>(code->GetBufferPointer());result.assign(data,data+code->GetBufferSize());code->Release();
            auto map=Fast::ReliefCache::Build(pixels.data(),32,32,name,"concurrency-v1");
            assert(map.rgba==expected.rgba&&map.width==expected.width&&map.height==expected.height);
        }return result;
    }));
    const auto bytecode=tasks.front().get();for(size_t i=1;i<tasks.size();++i)assert(tasks[i].get()==bytecode);
    for(auto& file:std::filesystem::recursive_directory_iterator("cache"))assert(file.path().string().find(".tmp.")==std::string::npos);
    std::cout<<"{\"passed\":true,\"threads\":12,\"shader_and_material_roundtrips\":120,\"byte_identical\":true,\"temporary_files_remaining\":0}\n";
}
