#pragma once
#include <bcrypt.h>
#include <array>
#include "fast/MaterialRelief.h"
#include "fast/D3DShaderCache.h"
#pragma comment(lib,"bcrypt.lib")

namespace Fast {
namespace ReliefCache {
// Refresh with Tools/yousei-lighting/update_relief_revision.py after algorithm edits.
inline constexpr char AlgorithmRevision[]="relief-v2-87f00696ae86f461acfed82764263465e4ebf04249c80aedd4bca072a2811694";
inline std::string Key(const uint8_t* pixels,size_t size,const std::string& metadata) {
    struct Provider {
        BCRYPT_ALG_HANDLE handle=nullptr;
        Provider() { if(BCryptOpenAlgorithmProvider(&handle,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)handle=nullptr; }
        ~Provider() { if(handle)BCryptCloseAlgorithmProvider(handle,0); }
    };
    static Provider provider;
    if(!provider.handle || size>ULONG_MAX)return {};
    BCRYPT_HASH_HANDLE hash=nullptr;
    if(BCryptCreateHash(provider.handle,&hash,nullptr,0,nullptr,0,0)<0)return {};
    std::array<unsigned char,32> digest={};
    const bool ok=BCryptHashData(hash,reinterpret_cast<PUCHAR>(const_cast<char*>(metadata.data())),
        ULONG(metadata.size()),0)>=0 &&
        BCryptHashData(hash,const_cast<PUCHAR>(pixels),ULONG(size),0)>=0 &&
        BCryptFinishHash(hash,digest.data(),ULONG(digest.size()),0)>=0;
    BCryptDestroyHash(hash);
    if(!ok)return {};
    std::ostringstream out;out<<std::hex<<std::setfill('0');
    for(auto byte:digest)out<<std::setw(2)<<unsigned(byte);
    return out.str();
}
inline ReliefPixels Build(const uint8_t* source,uint32_t width,uint32_t height,
                         std::string_view name,const char* revision=AlgorithmRevision) {
    if(!source || width<4 || height<4 || uint64_t(width)*height*4>ULONG_MAX)
        return BuildReliefPixels(source,width,height,name);
    // The algorithm revision is stable across unrelated builds. Exact decoded
    // pixels and the active material profile invalidate texture/profile changes.
    std::string metadata(name);metadata.push_back('\0');metadata+=revision;
    metadata.append(reinterpret_cast<const char*>(&width),sizeof(width));
    metadata.append(reinterpret_cast<const char*>(&height),sizeof(height));
    if(const auto* material=BackgroundRelief(name)) {
        metadata.append(reinterpret_cast<const char*>(&material->normal),sizeof(material->normal));
        metadata.append(reinterpret_cast<const char*>(&material->parallax),sizeof(material->parallax));
        metadata.append(reinterpret_cast<const char*>(&material->invertHeight),sizeof(material->invertHeight));
        metadata.append(reinterpret_cast<const char*>(&material->heightMode),sizeof(material->heightMode));
    }
    const auto key=Key(source,size_t(width)*height*4,metadata);
    if(key.empty())return BuildReliefPixels(source,width,height,name);
    const auto folder=std::filesystem::path("cache")/"relief-v1";
    const auto path=folder/(key+".bin");
    constexpr uint64_t magic=0x314650414d464c52ull;
    {
        DialoguePerformance::Scope timing("relief_cache_read");
        std::ifstream file(path,std::ios::binary);
        uint64_t header[6]={};std::string storedKey(64,'\0');
        if(file.read(reinterpret_cast<char*>(header),sizeof(header)) && header[0]==magic &&
            header[1]>=4 && header[1]<=512 && header[2]>=4 && header[2]<=512 && header[3]<=1 &&
            header[4]==header[1]*header[2]*4 && file.read(storedKey.data(),storedKey.size()) && storedKey==key) {
            ReliefPixels result;result.width=uint32_t(header[1]);result.height=uint32_t(header[2]);
            result.translucent=header[3]!=0;result.rgba.resize(size_t(header[4]));
            if(file.read(reinterpret_cast<char*>(result.rgba.data()),result.rgba.size()) &&
                file.peek()==std::char_traits<char>::eof() &&
                D3DShaderCache::Hash(result.rgba.data(),result.rgba.size())==header[5]) {
                if(DialoguePerformance::enabled)DialoguePerformance::Record("relief_cache_hit",DialoguePerformance::Now(),0);
                return result;
            }
        }
    }
    ReliefPixels result;
    { DialoguePerformance::Scope timing(D3DShaderCache::background?"prewarm_relief_generate":"relief_generate");result=BuildReliefPixels(source,width,height,name); }
    if(!result.rgba.empty()) {
        std::error_code error;std::filesystem::create_directories(folder,error);
        if(!error) {
            const uint64_t header[]={magic,result.width,result.height,uint64_t(result.translucent),
                uint64_t(result.rgba.size()),D3DShaderCache::Hash(result.rgba.data(),result.rgba.size())};
            D3DShaderCache::Write(path,[&](std::ofstream& file) {
            file.write(reinterpret_cast<const char*>(header),sizeof(header));file.write(key.data(),key.size());
            file.write(reinterpret_cast<const char*>(result.rgba.data()),result.rgba.size());
            });
        }
    }
    return result;
}
}
}
