#pragma once
#include <algorithm>
#include <cstdint>
namespace Fast {
struct LightingLabState {
    bool enabled=false,hdri=true,pbr=true,shadows=true,grading=true,glow=true,jelly=true;
    bool spawnChuchu=false,nativeTest=false,dx11=false,offCamera=true;
    // Reference rendering is accessible only to the isolated native performance fixture.
    bool optimize=true,performanceTest=false,validateShadows=false;
    int performanceFrame=0;
    uint64_t constantUploads=0,shadowDraws=0,shadowVertexUploads=0;
    static constexpr int MaxFairyLights=32;
    int fairyCount=0,naviIndex=-1;
    bool naviShadows=true,naviShadowDebug=false;
    uint64_t naviShadowPasses=0,naviShadowTriangles=0,naviDepthPixels=0;
    float fairyNight=0,fairyBrightness=1.8f,fairyRange=450;
    float fairyPosition[MaxFairyLights][4]={}; // world position, radius
    uintptr_t fairyKey[MaxFairyLights]={};
    float fairyCone[MaxFairyLights][4]={}; // direction, outer cosine; zero = point
    float fairyColor[MaxFairyLights][4]={}; // linear blue-white energy
    int comparison=-1; // transient coded A/B mode; persistent settings are unchanged
    uint8_t shadowOnly=0;
    uint64_t offCameraTriangles=0,frustumTriangles=0,explicitTriangles=0,shadowOverflow=0,offCameraDepthChanges=0;
    uint64_t shadowActors=0,shadowRooms=0,nativeBlobsSuppressed=0;
    int scene=-1,capture=0,phase=0,environment=0,environmentNext=0,profile=0;
    float environmentBlend=0,environmentRotation=0;
    float roughness=.65f,specular=.5f,hdriStrength=.25f,sunStrength=.45f;
    float exposure=0,contrast=1,saturation=1,gamma=1,glowStrength=.15f,glowThreshold=.85f;
    float shadowStrength=.8f,shadowDistance=2400,shadowBias=.00025f;
    float refraction=.012f,bodyOpacity=.7f,pearlStrength=.7f,clearcoat=.5f;
    float sun[3]={.4f,.8f,.3f},sunColor[3]={1,.91f,.75f};
    float eye[3]={},center[3]={},time=0,nearPlane=10,farPlane=12800;
    float chuchuOrigin[4]={0,0,0,60};
    uint64_t shadowNonclear=0;
    uint64_t materialDraws=0,jellyDraws=0,shadowTriangles=0,shadowPasses=0,postPasses=0;
    uint64_t phaseDraws[8]={},phaseJelly[8]={};
    int chuchuSpawned=0,chuchuHits=0,chuchuDeaths=0;
};
inline LightingLabState gLightingLab;
inline bool LightingLabActive() {return gLightingLab.enabled&&gLightingLab.scene>=0;}
}
