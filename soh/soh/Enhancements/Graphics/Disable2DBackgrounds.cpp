#include <libultraship/bridge/consolevariablebridge.h>
#include <fast/GraphicsProfiles.h>
#include <ship/resource/ResourceManager.h>

#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/ShipInit.hpp"
#include "soh/resource/type/scenecommand/SetTimeSettings.h"
#include "soh/resource/type/scenecommand/SetSkyboxSettings.h"
#include "soh/resource/type/scenecommand/SetSkyboxModifier.h"
#include "soh/resource/type/Scene.h"

bool Scene_CommandTimeSettings(PlayState* play, SOH::ISceneCommand* cmd);

extern "C" {
#include "functions.h"
#include "variables.h"
#include "z64save.h"
extern SaveContext gSaveContext;
extern PlayState* gPlayState;
void Skybox_Setup(PlayState* play, SkyboxContext* skyboxCtx, s16 skyboxId);
void func_800AEFC8(SkyboxContext* skyboxCtx, s16 skyboxId);
void func_800AF178(SkyboxContext* skyboxCtx, s32 faceCount);
}

#define CVAR_NAME CVAR_ENHANCEMENT("3DSceneRender")
#define CVAR_VALUE CVarGetInteger(CVAR_NAME, 0)

static bool Full3DBackgrounds() {
    return CVAR_VALUE && !Fast::OriginalGraphics() && CVarGetInteger(CVAR_SETTING("AltAssets"), 1);
}

// The inactive context has bounded owned storage: the original arena reserves
// only the active skybox. Repeated switches must not allocate from that arena.
static bool managedSkybox = false, active3D = false;
static s16 originalSkybox = SKYBOX_NONE;
static u8 originalSkyboxDisabled = 0, originalWeather = 0;
static bool skyboxReady[2] = {};
static SkyboxContext skyboxes[2] = {};
static Gfx alternateSkyboxLists[2][12][150];
static Vtx alternateSkyboxVertices[2][256];

static void PrepareInactiveSkybox(PlayState* play, bool mode, s16 id) {
    auto& context = skyboxes[mode];
    context = {};
    context.skyboxId = id;
    Skybox_Setup(play, &context, id);
    context.dListBuf = alternateSkyboxLists[mode];
    context.roomVtx = alternateSkyboxVertices[mode];
    if (id == SKYBOX_NONE) return;
    if (context.unk_140) func_800AEFC8(&context, id);
    else func_800AF178(&context, id == SKYBOX_CUTSCENE_MAP ? 6 : 5);
}

std::set<SceneID> fogControlList = {
    SCENE_MARKET_ENTRANCE_DAY,
    SCENE_MARKET_ENTRANCE_NIGHT,
    SCENE_MARKET_ENTRANCE_RUINS,
    SCENE_BACK_ALLEY_DAY,
    SCENE_BACK_ALLEY_NIGHT,
    SCENE_MARKET_DAY,
    SCENE_MARKET_NIGHT,
    SCENE_MARKET_RUINS,
    SCENE_TEMPLE_OF_TIME_EXTERIOR_DAY,
    SCENE_TEMPLE_OF_TIME_EXTERIOR_NIGHT,
    SCENE_TEMPLE_OF_TIME_EXTERIOR_RUINS,
};

std::set<SceneID> skyboxSceneControlList = {
    SCENE_MARKET_ENTRANCE_DAY,
    SCENE_MARKET_ENTRANCE_NIGHT,
    SCENE_MARKET_ENTRANCE_RUINS,
    SCENE_BACK_ALLEY_DAY,
    SCENE_BACK_ALLEY_NIGHT,
    SCENE_MARKET_DAY,
    SCENE_MARKET_NIGHT,
    SCENE_MARKET_RUINS,
    SCENE_CASTLE_COURTYARD_ZELDA,
    SCENE_TEMPLE_OF_TIME_EXTERIOR_DAY,
    SCENE_TEMPLE_OF_TIME_EXTERIOR_NIGHT,
    SCENE_TEMPLE_OF_TIME_EXTERIOR_RUINS,
    SCENE_FOREST_TEMPLE,
};

std::set<SkyboxId> skyboxIdControlList = {
    SKYBOX_BAZAAR,
    SKYBOX_HOUSE_LINK,
    SKYBOX_MARKET_ADULT,
    SKYBOX_MARKET_CHILD_DAY,
    SKYBOX_MARKET_CHILD_NIGHT,
    SKYBOX_HAPPY_MASK_SHOP,
    SKYBOX_HOUSE_KNOW_IT_ALL_BROTHERS,
    SKYBOX_HOUSE_OF_TWINS,
    SKYBOX_STABLES,
    SKYBOX_HOUSE_KAKARIKO,
    SKYBOX_KOKIRI_SHOP,
    SKYBOX_GORON_SHOP,
    SKYBOX_ZORA_SHOP,
    SKYBOX_POTION_SHOP_KAKARIKO,
    SKYBOX_POTION_SHOP_MARKET,
    SKYBOX_HOUSE_RICHARD,
    SKYBOX_HOUSE_IMPA,
    SKYBOX_TENT,
    SKYBOX_HOUSE_MIDO,
    SKYBOX_HOUSE_SARIA,
    SKYBOX_HOUSE_ALLEY,
};

static void SyncSkyboxTimeToCurrentTime() {
    SOH::SetTimeSettings cmd;
    cmd.settings = { 0xFF, 0xFF, static_cast<uint8_t>(gPlayState->envCtx.timeIncrement) };
    Scene_CommandTimeSettings(gPlayState, &cmd);
}

void Register3DPreRenderedScenes() {
    // Runs after the scene commands have set play->skyboxId, but before Play_InitEnvironment builds the
    // skybox. Overriding the id here means Skybox_Setup loads a real sky; overriding it any later would
    // leave the display lists pointing at the pre-rendered skybox's texture slots.
    COND_HOOK(AfterSceneCommands, true, [](int16_t sceneNum) {
        skyboxReady[0] = skyboxReady[1] = false;
        originalSkybox = gPlayState->skyboxId;
        originalSkyboxDisabled = gPlayState->envCtx.skyboxDisabled;
        // Full-3D packs can replace the scene's skybox command itself. Read the
        // exact base header, bypassing alternates, rather than remembering that
        // replacement as the "original" when booting in enhanced mode.
        auto* loaded = static_cast<SOH::Scene*>(gPlayState->sceneSegment);
        auto path = loaded->GetInitData()->Path;
        if (path.starts_with("alt/")) path.erase(0, 4);
        auto base = std::dynamic_pointer_cast<SOH::Scene>(
            Ship::Context::GetRawInstance()->GetResourceManager()->LoadResource(path, true));
        if (base) {
            for (const auto& command : base->commands) {
                if (!command) continue;
                if (command->cmdId == SOH::SceneCommandID::SetSkyboxSettings)
                    originalSkybox = static_cast<SOH::SetSkyboxSettings*>(command.get())->settings.skyboxId;
                if (command->cmdId == SOH::SceneCommandID::SetSkyboxModifier)
                    originalSkyboxDisabled = static_cast<SOH::SetSkyboxModifier*>(command.get())->modifier.skyboxDisabled;
            }
        }
        managedSkybox = skyboxSceneControlList.contains(static_cast<SceneID>(sceneNum)) ||
                       skyboxIdControlList.contains(static_cast<SkyboxId>(originalSkybox));
        originalWeather = gWeatherMode;
        active3D = Full3DBackgrounds();
        if (managedSkybox) {
            const auto environment = gPlayState->envCtx;
            PrepareInactiveSkybox(gPlayState, !active3D, active3D ? originalSkybox : SKYBOX_NORMAL_SKY);
            skyboxReady[!active3D] = true;
            gPlayState->envCtx = environment;
        }
        if (!managedSkybox || !active3D) {
            return;
        }

        // Add a skybox on scenes from skyboxSceneControlList
        gPlayState->envCtx.skyboxDisabled = false;

        // Replace skybox with normal sky
        SyncSkyboxTimeToCurrentTime();
        gPlayState->skyboxId = SKYBOX_NORMAL_SKY;
        // Apply the always cloudy skybox as an adult for Temple of Time and the Market
        if (sceneNum == SCENE_TEMPLE_OF_TIME_EXTERIOR_RUINS || sceneNum == SCENE_MARKET_RUINS ||
            sceneNum == SCENE_MARKET_ENTRANCE_RUINS) {
            gWeatherMode = 3;
            gSaveContext.retainWeatherMode = 1;
        }
    });

    COND_HOOK(OnCameraState, true, [](PlayState* play) {
        if (!managedSkybox) return;
        const bool want3D = Full3DBackgrounds();
        if (want3D == active3D) return;
        skyboxes[active3D] = play->skyboxCtx;
        skyboxReady[active3D] = true;
        active3D = want3D;
        play->skyboxId = want3D ? SKYBOX_NORMAL_SKY : originalSkybox;
        play->envCtx.skyboxDisabled = want3D ? false : originalSkyboxDisabled;
        gWeatherMode = originalWeather;
        if (want3D) {
            SyncSkyboxTimeToCurrentTime();
            if (play->sceneNum == SCENE_TEMPLE_OF_TIME_EXTERIOR_RUINS ||
                play->sceneNum == SCENE_MARKET_RUINS || play->sceneNum == SCENE_MARKET_ENTRANCE_RUINS)
                gWeatherMode = 3;
        }
        play->skyboxCtx = skyboxes[want3D];
        Skybox_Setup(play, &play->skyboxCtx, play->skyboxId);
    });
    COND_HOOK(OnPlayDestroy, true, [] {
        managedSkybox = false;
        skyboxReady[0] = skyboxReady[1] = false;
    });

    COND_HOOK(OnPlayDrawBegin, CVAR_VALUE, []() {
        if (!Full3DBackgrounds() || !fogControlList.contains(static_cast<SceneID>(gPlayState->sceneNum))) {
            return;
        }

        if ((HREG(80) != 10) || (HREG(82) != 0)) {
            // Furthest possible fog and zFar
            gPlayState->view.zFar = 12800;
            gPlayState->lightCtx.fogNear = 1000;
            gPlayState->lightCtx.fogFar = 12800;
        }
    });

    COND_VB_SHOULD(VB_DRAW_2D_BACKGROUND, CVAR_VALUE, { if (Full3DBackgrounds()) *should = false; });
}

static RegisterShipInitFunc initFunc(Register3DPreRenderedScenes, { CVAR_NAME });
