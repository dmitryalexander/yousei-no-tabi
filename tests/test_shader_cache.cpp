#define NOMINMAX
#include <windows.h>
#include <d3dcompiler.h>
#include "fast/D3DShaderCache.h"
#include <cassert>
#include <iostream>

int main() {
    auto module=LoadLibraryW(L"D3DCompiler_47.dll");assert(module);
    auto compiler=reinterpret_cast<decltype(&D3DCompile)>(GetProcAddress(module,"D3DCompile"));assert(compiler);
    Fast::DialoguePerformance::enabled=true;
    const std::string source="float4 PSMain():SV_Target{return float4(1,0,0,1);}";
    auto compile=[&](const std::string& text,UINT flags=0) {
        ID3DBlob* code=nullptr;ID3DBlob* errors=nullptr;
        const auto hr=Fast::D3DShaderCache::Compile(compiler,module,text.data(),text.size(),
            "PSMain","ps_4_0",flags,&code,&errors);
        if(errors)errors->Release();assert(SUCCEEDED(hr)&&code);
        const auto* data=static_cast<const char*>(code->GetBufferPointer());
        std::vector<char> bytes(data,data+code->GetBufferSize());code->Release();return bytes;
    };
    const auto original=compile(source);
    const auto file=std::filesystem::directory_iterator("cache/d3d11-v1")->path();
    assert(compile(source)==original);
    { std::fstream out(file,std::ios::binary|std::ios::in|std::ios::out);
      out.seekg(-1,std::ios::end);char byte=0;out.get(byte);
      out.seekp(-1,std::ios::end);out.put(byte^0x5a); }
    assert(compile(source)==original);
    { std::fstream out(file,std::ios::binary|std::ios::in|std::ios::out);
      out.seekp(0);out.put('\0'); }
    assert(compile(source)==original);
    { std::ofstream out(file,std::ios::binary|std::ios::trunc);out<<"truncated"; }
    assert(compile(source)==original);
    { std::fstream out(file,std::ios::binary|std::ios::in|std::ios::out);
      uint64_t oversized=UINT64_MAX;out.seekp(2*sizeof(uint64_t));
      out.write(reinterpret_cast<const char*>(&oversized),sizeof(oversized)); }
    assert(compile(source)==original);
    assert(!compile(source+"\n// source changed").empty());
    assert(!compile(source,D3DCOMPILE_OPTIMIZATION_LEVEL2).empty());
    int hits=0,compiles=0;
    for(const auto& event:Fast::DialoguePerformance::events) {
        if(std::string(event.stage)=="shader_cache_hit")++hits;
        if(std::string(event.stage)=="shader_compile_native")++compiles;
    }
    assert(hits==1 && compiles==7);
    std::cout<<"{\"passed\":true,\"cache_hits\":"<<hits<<",\"compiles\":"<<compiles
        <<",\"checks\":[\"identical bytecode\",\"corrupt cache fallback\",\"truncated cache fallback\","
          "\"oversized cache rejection\",\"source invalidation\",\"flags invalidation\"]}\n";
}
