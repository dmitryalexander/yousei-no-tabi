#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "BackgroundReliefMaterials.h"

namespace Fast {
// Experimental renderer state. Scene IDs are supplied by the game, never inferred
// from a shared tree texture: object_wood02 also appears outside Kokiri Forest.
struct MaterialReliefState {
    int scene = -1; // 0: Temple of Time, 1: Kokiri Forest, -1: ordinary rendering
    bool normals = false, parallax = false;
    float normalStrength = 2.0f, parallaxDepth = 0.035f;
    float light[4] = {0.4f,0.8f,0.3f,0.5f}; // view-space light and diffuse fraction
    float view[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
    bool backgroundNormals = true, backgroundParallax = true;
    float backgroundStrength = 1.0f, backgroundDepth = 0.008f;
    uint64_t draws[3][4] = {};
    int capture = 0;
};
inline MaterialReliefState gMaterialRelief;

inline bool KokiriGrassRelief(std::string_view path) {
    return path == "alt/scenes/shared/spot04_scene/spot04_sceneTex_00FE18" ||
           path == "alt/scenes/shared/spot04_scene/spot04_sceneTex_010618";
}

inline int ReliefTextureScene(std::string_view path) {
    if (BackgroundRelief(path)) return 2;
    if (path.find("/tokinoma_scene/") != path.npos) {
        static const char* stone[] = {"sceneTex_00D7A0", "sceneTex_00DFA0", "sceneTex_00EFA0",
            "sceneTex_00FFA0", "sceneTex_010FA0", "sceneTex_011FA0", "room_0Tex_0159D8"};
        for (auto name : stone) if (path.find(name) != path.npos) return 0;
        return -1;
    }
    if (path.find("/spot04_scene/") != path.npos ||
        path.find("/object_spot04_objects/") != path.npos ||
        path.find("/object_wood02/") != path.npos ||
        path.find("/gameplay_field_keep/") != path.npos) return 1;
    return -1;
}

struct ReliefPixels {
    uint32_t width = 0, height = 0;
    bool translucent = false;
    std::vector<uint8_t> rgba;
};

// Derive normal RGB + raised-height A from the actual decoded active texture.
// This respects the user's installed HD replacements and does not alter albedo.
inline ReliefPixels BuildReliefPixels(const uint8_t* source, uint32_t width, uint32_t height,
                                     std::string_view name) {
    ReliefPixels out;
    if (!source || width < 4 || height < 4) return out;
    const bool panelDoor=name=="alt/objects/gameplay_field_keep/gFieldDoor1Tex";
    float factor = std::min(1.0f, (panelDoor?512.0f:256.0f) / std::max(width,height));
    out.width = std::max(4u, uint32_t(width*factor));
    out.height = std::max(4u, uint32_t(height*factor));
    size_t size = size_t(out.width)*out.height;
    std::vector<float> luminance(size), smoothed(size);
    for (uint32_t y=0;y<out.height;y++) for (uint32_t x=0;x<out.width;x++) {
        size_t pixel=(size_t(y)*height/out.height*width + size_t(x)*width/out.width)*4;
        luminance[size_t(y)*out.width+x]=(source[pixel]*0.2126f+source[pixel+1]*0.7152f+source[pixel+2]*0.0722f)/255.0f;
        out.translucent |= source[pixel+3]<240;
    }
    auto at=[&](const std::vector<float>& values,int x,int y) {
        if (panelDoor) {
            x=std::clamp(x,0,int(out.width)-1);y=std::clamp(y,0,int(out.height)-1);
        } else {
            x=(x%int(out.width)+int(out.width))%int(out.width);
            y=(y%int(out.height)+int(out.height))%int(out.height);
        }
        return values[size_t(y)*out.width+x];
    };
    for (uint32_t y=0;y<out.height;y++) for (uint32_t x=0;x<out.width;x++) {
        float sum=0;
        for (int dy=-1;dy<=1;dy++) for (int dx=-1;dx<=1;dx++)
            sum+=at(luminance,x+dx,y+dy)*(dx==0?2:1)*(dy==0?2:1);
        smoothed[size_t(y)*out.width+x]=sum/16.0f;
    }
    // Suppress baked broad lighting/color gradients in new background materials.
    // Keep the old Temple/Kokiri appearance unchanged.
    const auto* backgroundMaterial=BackgroundRelief(name);
    bool background=backgroundMaterial!=nullptr;
    if (background) {
        auto local=smoothed;
        for (uint32_t y=0;y<out.height;y++) for (uint32_t x=0;x<out.width;x++) {
            float broad=0;
            for (int dy=-8;dy<=8;dy+=2) for (int dx=-8;dx<=8;dx+=2) broad+=at(local,x+dx,y+dy);
            smoothed[size_t(y)*out.width+x]=local[size_t(y)*out.width+x]-broad/81.0f;
        }
    }
    auto sorted=smoothed; std::sort(sorted.begin(),sorted.end());
    float low=sorted[size/50], high=sorted[size-size/50-1];
    float range=std::max(high-low,0.08f);
    bool checker=name.find("sceneTex_00D7A0")!=name.npos;
    for (uint32_t y=0;y<out.height;y++) for (uint32_t x=0;x<out.width;x++) {
        auto& h=smoothed[size_t(y)*out.width+x];
        if (checker) {
            // Preserve equal elevation of painted black/white floor squares.
            float u=std::fmod(float(x)/out.width*2,1.0f),v=std::fmod(float(y)/out.height*2,1.0f);
            float edge=std::min(std::min(u,1-u),std::min(v,1-v));
            float bevel=std::clamp(edge/0.07f,0.0f,1.0f);
            h=0.08f+0.84f*bevel*bevel*(3-2*bevel);
        } else if (panelDoor) {
            // This is one mirrored half of the door: x=0 is its centre seam.
            // Height follows joined timber, not the grayscale baked lighting.
            float u=float(x)/float(out.width-1),v=float(y)/float(out.height-1);
            auto smooth=[](float value,float lo,float hi) {
                float a=std::clamp((value-lo)/(hi-lo),0.0f,1.0f);return a*a*(3-2*a);
            };
            float stile=smooth(u,.59f,.70f);
            float top=1-smooth(v,.065f,.105f),bottom=smooth(v,.91f,.96f);
            float rail=smooth(v,.535f,.555f)*(1-smooth(v,.575f,.595f));
            float frame=std::max(std::max(stile,top),std::max(bottom,rail));
            float label=(1-smooth(u,.265f,.30f))*smooth(v,.195f,.21f)*(1-smooth(v,.29f,.305f));
            float labelInset=(1-smooth(u,.235f,.255f))*smooth(v,.225f,.235f)*(1-smooth(v,.268f,.278f));
            float grain=std::clamp(-h/std::max(-low,.04f),0.0f,1.0f);
            h=.40f+.34f*frame+.18f*label-.10f*labelInset;
            // Actual wood grooves contribute to BOTH normal and parallax channels.
            h-=.14f*grain;
        } else if (backgroundMaterial && backgroundMaterial->heightMode==8) {
            // Castle Town's painted pale joints are not height. Author the
            // matching tile grid first; keep stone grain only on raised faces.
            const bool smallTiles=name.find("hairal_yuka_01b_")!=name.npos;
            const float divisions=smallTiles?6.0f:3.0f;
            float u=std::fmod((float(x)+.5f)/out.width*divisions+(smallTiles?.5f:0.0f),1.0f);
            float v=std::fmod((float(y)+.5f)/out.height*divisions,1.0f);
            float edge=std::min(std::min(u,1-u),std::min(v,1-v))*128.0f/divisions;
            float bevel=std::clamp((edge-.55f)/1.8f,0.0f,1.0f);
            bevel=bevel*bevel*(3-2*bevel);
            float grain=std::clamp(-h/std::max(-low,.04f),0.0f,1.0f);
            h=.32f+bevel*(.32f-.04f*grain);
        } else if (backgroundMaterial && backgroundMaterial->heightMode==6) {
            // Shallow local turf/soil grain; broad grass/dirt color is not terrain elevation.
            h=.42f+.16f*std::clamp((h-low)/range,0.0f,1.0f);
        } else if (backgroundMaterial && backgroundMaterial->heightMode==7) {
            // Stone face stays level; dark crevices cut into it without raised halos.
            h=.62f-.26f*std::clamp(-h/std::max(-low,.04f),0.0f,1.0f);
        } else if (backgroundMaterial && backgroundMaterial->heightMode==1) {
            // Wood is a flat face with dark grain/growth rings cut into it.
            // One-sided local contrast removes the old bright raised halos.
            h=0.58f-0.20f*std::clamp(-h/std::max(-low,0.04f),0.0f,1.0f);
        } else if (backgroundMaterial && backgroundMaterial->heightMode==2) {
            // Cloth paint is color, not embroidery/geometry. Only a tiny weave.
            h=0.5f+0.02f*std::sin(float(x)*1.57079632679f)*std::sin(float(y)*1.57079632679f);
        } else if (backgroundMaterial && backgroundMaterial->heightMode==3) {
            h=0.47f+0.06f*std::clamp((h-low)/range,0.0f,1.0f);
        } else if (backgroundMaterial && backgroundMaterial->heightMode==4) h=0.5f;
        else h=std::clamp((h-low)/range,0.0f,1.0f)*(background?0.5f:0.8f)+(background?0.25f:0.1f);
    }
    // Pale grout is paint/color, not raised stone. Invert before deriving normals
    // so normal slopes and parallax agree about the recessed joints.
    if (backgroundMaterial && backgroundMaterial->heightMode==0 && backgroundMaterial->invertHeight)
        for (auto& height : smoothed) height=1.0f-height;
    out.rgba.resize(size*4);
    for (uint32_t y=0;y<out.height;y++) for (uint32_t x=0;x<out.width;x++) {
        float nx=-(at(smoothed,x+1,y)-at(smoothed,x-1,y))*2.0f;
        float ny=-(at(smoothed,x,y+1)-at(smoothed,x,y-1))*2.0f;
        if (panelDoor) { nx*=float(out.width)/128.0f;ny*=float(out.height)/256.0f; }
        float inv=1.0f/std::sqrt(nx*nx+ny*ny+1.0f);
        auto i=(size_t(y)*out.width+x)*4;
        out.rgba[i]=uint8_t((nx*inv*0.5f+0.5f)*255);
        out.rgba[i+1]=uint8_t((ny*inv*0.5f+0.5f)*255);
        out.rgba[i+2]=uint8_t((inv*0.5f+0.5f)*255);
        out.rgba[i+3]=uint8_t(smoothed[size_t(y)*out.width+x]*255);
    }
    return out;
}
} // namespace Fast
