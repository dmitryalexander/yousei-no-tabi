#pragma once
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include "fast/DialoguePerformance.h"

namespace Fast {
namespace D3DShaderCache {
inline thread_local bool background=false;
inline uint64_t Hash(const void* bytes, size_t length) {
    uint64_t value=14695981039346656037ull;
    const auto* data=static_cast<const unsigned char*>(bytes);
    for(size_t i=0;i<length;++i)value=(value^data[i])*1099511628211ull;
    return value;
}
// Render misses and the worker may finish the same key concurrently. Publish
// completed files atomically so neither consumes a partially overwritten cache.
template<class Writer> inline void Write(const std::filesystem::path& path,Writer writer) {
    auto temp=path;temp+=L".tmp."+std::to_wstring(GetCurrentThreadId());
    std::ofstream file(temp,std::ios::binary|std::ios::trunc);
    if(!file)return;writer(file);file.close();
    if(file)MoveFileExW(temp.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING);
    std::error_code ec;std::filesystem::remove(temp,ec);
}
// Generated source is self-contained. Verify it byte for byte, along with entry
// point, profile and flags, before using portable DXBC bytecode from disk.
template<class Compiler>
HRESULT Compile(Compiler compiler, HMODULE module, const char* source, size_t length,
                const char* entry, const char* profile, UINT flags,
                ID3DBlob** code, ID3DBlob** errors) {
    std::string key(source,length);
    key.push_back('\0');key+=entry;key.push_back('\0');key+=profile;
    key.append(reinterpret_cast<const char*>(&flags),sizeof(flags));
    std::ostringstream name;name<<std::hex<<Hash(key.data(),key.size())<<".bin";
    const auto folder=std::filesystem::path("cache")/"d3d11-v1";
    const auto path=folder/name.str();
    using CreateBlob=HRESULT(WINAPI*)(SIZE_T,ID3DBlob**);
    const auto createBlob=reinterpret_cast<CreateBlob>(GetProcAddress(module,"D3DCreateBlob"));
    {
        DialoguePerformance::Scope timing("shader_cache_read");
        std::ifstream file(path,std::ios::binary);
        uint64_t header[4]={};
        constexpr uint64_t magic=0x3145484341435344ull;
        if(createBlob && file.read(reinterpret_cast<char*>(header),sizeof(header)) &&
            header[0]==magic && header[1]==key.size() && header[2]>=32 && header[2]<=16*1024*1024) {
            std::string storedKey(size_t(header[1]),'\0');
            std::vector<char> bytecode(static_cast<size_t>(header[2]));
            if(file.read(storedKey.data(),storedKey.size()) && storedKey==key &&
                file.read(bytecode.data(),bytecode.size()) && file.peek()==std::char_traits<char>::eof() &&
                Hash(bytecode.data(),bytecode.size())==header[3] &&
                memcmp(bytecode.data(),"DXBC",4)==0 && SUCCEEDED(createBlob(bytecode.size(),code))) {
                memcpy((*code)->GetBufferPointer(),bytecode.data(),bytecode.size());
                if(DialoguePerformance::enabled)DialoguePerformance::Record("shader_cache_hit",DialoguePerformance::Now(),0);
                return S_OK;
            }
        }
    }
    HRESULT result;
    {
        DialoguePerformance::Scope timing(background?"prewarm_shader_compile_native":"shader_compile_native");
        result=compiler(source,length,nullptr,nullptr,nullptr,entry,profile,flags,0,code,errors);
    }
    if(SUCCEEDED(result) && *code) {
        std::error_code error;
        std::filesystem::create_directories(folder,error);
        if(!error) {
            const uint64_t header[]={0x3145484341435344ull,uint64_t(key.size()),
                uint64_t((*code)->GetBufferSize()),Hash((*code)->GetBufferPointer(),(*code)->GetBufferSize())};
            Write(path,[&](std::ofstream& file) {
            file.write(reinterpret_cast<const char*>(header),sizeof(header));
            file.write(key.data(),key.size());
            file.write(static_cast<const char*>((*code)->GetBufferPointer()),(*code)->GetBufferSize());
            });
        }
    }
    return result;
}
}
}
