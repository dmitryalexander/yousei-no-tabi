#include "fast/MaterialRelief.h"
#include "fast/GraphicsProfiles.h"
#include "soh/ShipInit.hpp"
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include <libultraship/bridge/consolevariablebridge.h>
#include <ship/Context.h>
#include <ship/window/Window.h>
#include <fstream>
#include <nlohmann/json.hpp>
extern "C" {
#include "global.h"
extern PlayState* gPlayState;
}

static void ReliefFrame() {
    auto& state=Fast::gMaterialRelief;
    state.scene=-1;
    if (Fast::OriginalGraphics()) {
        state.normals=state.parallax=state.backgroundNormals=state.backgroundParallax=false;
        return;
    }
    if (!gPlayState) return;
    if (gPlayState->sceneNum==SCENE_TEMPLE_OF_TIME) state.scene=0;
    if (gPlayState->sceneNum==SCENE_KOKIRI_FOREST) state.scene=1;
    state.normals=CVarGetInteger("gEnhancements.MaterialRelief.Normals",0)!=0;
    state.parallax=CVarGetInteger("gEnhancements.MaterialRelief.Parallax",0)!=0;
    state.normalStrength=std::clamp(CVarGetFloat("gEnhancements.MaterialRelief.NormalStrength",2.0f),0.0f,6.0f);
    state.parallaxDepth=std::clamp(CVarGetFloat("gEnhancements.MaterialRelief.ParallaxDepth",0.035f),0.0f,0.08f);
    state.backgroundNormals=CVarGetInteger("gEnhancements.LightingLab.BackgroundNormals",1)!=0;
    state.backgroundParallax=CVarGetInteger("gEnhancements.LightingLab.BackgroundParallax",1)!=0;
    state.backgroundStrength=std::clamp(CVarGetFloat("gEnhancements.LightingLab.BackgroundNormalStrength",1.0f),0.0f,3.0f);
    state.backgroundDepth=std::clamp(CVarGetFloat("gEnhancements.LightingLab.BackgroundParallaxDepth",0.008f),0.0f,0.02f);
    auto& light=gPlayState->envCtx.lightSettings;
    MtxF matrix;
    Matrix_MtxToMtxF(&gPlayState->view.viewing,&matrix);
    for (int row=0;row<4;row++) for (int column=0;column<4;column++) state.view[row*4+column]=matrix.mf[row][column];
    float first=float(light.light1Color[0]+light.light1Color[1]+light.light1Color[2]);
    float second=float(light.light2Color[0]+light.light2Color[1]+light.light2Color[2]);
    const s8* dominant=second>first?light.light2Dir:light.light1Dir;
    float dir[3]={float(dominant[0]),float(dominant[1]),float(dominant[2])};
    state.light[0]=dir[0]*matrix.mf[0][0]+dir[1]*matrix.mf[1][0]+dir[2]*matrix.mf[2][0];
    state.light[1]=dir[0]*matrix.mf[0][1]+dir[1]*matrix.mf[1][1]+dir[2]*matrix.mf[2][1];
    state.light[2]=dir[0]*matrix.mf[0][2]+dir[1]*matrix.mf[1][2]+dir[2]*matrix.mf[2][2];
    if (std::abs(state.light[0])+std::abs(state.light[1])+std::abs(state.light[2])<0.001f)
        { state.light[0]=0.4f; state.light[1]=0.8f; state.light[2]=0.3f; }
    float ambient=float(light.ambientColor[0]+light.ambientColor[1]+light.ambientColor[2]);
    float direct=std::max(first,second);
    state.light[3]=std::clamp(direct/std::max(ambient+direct,1.0f),0.15f,0.75f);

    if (!CVarGetInteger("gEnhancements.MaterialRelief.NativeTest",0) || state.scene<0) return;
    static int frames=0;
    ++frames;
    int phase=std::min(frames/90,3);
    state.normals=(phase&1)!=0;
    state.parallax=(phase&2)!=0;
    if (frames%90==75) state.capture=phase+1;
    if (frames==360) {
        nlohmann::json result;
        result["scene"]=state.scene;
        result["backend"]="DirectX 11";
        bool passed=true;
        for (int i=0;i<4;i++) { result["draws"][i]=state.draws[state.scene][i]; passed &= state.draws[state.scene][i]>0; }
        result["passed"]=passed;
        std::ofstream("material-relief-test.json")<<result.dump(2);
        Ship::Context::GetRawInstance()->GetWindow()->Close();
    }
}

static void RegisterMaterialRelief() {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnGameFrameUpdate>(ReliefFrame);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayDestroy>([] { Fast::gMaterialRelief.scene=-1; });
}
static RegisterShipInitFunc initRelief(RegisterMaterialRelief);
