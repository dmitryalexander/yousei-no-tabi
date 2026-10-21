#pragma once
#include <algorithm>
#include <array>
#include <deque>
#include "PresentationRate.h"
#include <ship/Context.h>
#include <ship/config/ConsoleVariable.h>
#include <ship/controller/controldevice/controller/mapping/keyboard/KeyboardScancodes.h>

namespace Fast {
// Graphics presets never change the independent presentation target.
// -1 retains the port's custom resolution controls.
inline int GraphicsMode() {
    return std::clamp(Ship::Context::GetRawInstance()->GetConsoleVariables()->GetInteger(
        "gSettings.GraphicsMode", 3), -1, 7);
}
inline bool OriginalGraphics() { return GraphicsMode() >= 0 && GraphicsMode() % 2 == 0; }
inline int GraphicsHeight() {
    constexpr int heights[] = {320, 540, 720, 1080};
    return heights[std::max(GraphicsMode(), 0) / 2];
}
inline const char* GraphicsModeName() {
    constexpr const char* names[] = {
        "320p original", "320p enhanced", "540p original", "540p enhanced",
        "720p original", "720p enhanced", "1080p original", "1080p enhanced"
    };
    return GraphicsMode() < 0 ? "Custom graphics" : names[GraphicsMode()];
}
inline constexpr std::array<int, 5> graphicsFpsSteps = {7, 15, 30, 60, 120};
inline int GraphicsTargetFps() {
    const int value = Ship::Context::GetRawInstance()->GetConsoleVariables()->GetInteger(
        "gSettings.GraphicsTargetFPS", 60);
    return std::find(graphicsFpsSteps.begin(), graphicsFpsSteps.end(), value) != graphicsFpsSteps.end() ? value : 60;
}
inline void StepGraphicsTargetFps(int direction) {
    const auto current = std::find(graphicsFpsSteps.begin(), graphicsFpsSteps.end(), GraphicsTargetFps());
    const int index = std::clamp(int(current - graphicsFpsSteps.begin()) + direction, 0, 4);
    Ship::Context::GetRawInstance()->GetConsoleVariables()->SetInteger(
        "gSettings.GraphicsTargetFPS", graphicsFpsSteps[index]);
}
inline bool GraphicsFpsVisible() {
    return Ship::Context::GetRawInstance()->GetConsoleVariables()->GetInteger("gSettings.ShowGraphicsFPS", 1) != 0;
}
inline bool GraphicsVsync(bool configured) { return configured; }
inline bool GraphicsVsyncEnabled() {
    return Ship::Context::GetRawInstance()->GetConsoleVariables()->GetInteger("gSettings.VsyncEnabled", 1) != 0;
}
inline void CycleGraphicsMode() {
    Ship::Context::GetRawInstance()->GetConsoleVariables()->SetInteger(
        "gSettings.GraphicsMode", (GraphicsMode() + 1) % 8);
}
inline unsigned graphicsRenderWidth = 0, graphicsRenderHeight = 0;
inline float graphicsOutputWidth = 0, graphicsOutputHeight = 0;
inline char graphicsFpsText[32] = {};
inline bool graphicsFpsDrawn = false;
// Quick taps survive between game ticks; held keys do not repeat.
// Keypad decimal is set-1 83; the separate Delete key has the extended bit.
inline constexpr int graphicsVsyncHotkey = Ship::KbScancode::LUS_KB_NUMPAD_DEL;
inline constexpr std::array<int, 5> graphicsHotkeys = {
    Ship::KbScancode::LUS_KB_OEM_3, Ship::KbScancode::LUS_KB_MULTIPLY,
    Ship::KbScancode::LUS_KB_ADD, Ship::KbScancode::LUS_KB_SUBTRACT, graphicsVsyncHotkey
};
inline std::array<bool, 5> graphicsKeysHeld = {};
inline std::deque<int> graphicsKeyPresses;
inline bool IsGraphicsHotkey(int scancode) {
    return std::find(graphicsHotkeys.begin(), graphicsHotkeys.end(), scancode) != graphicsHotkeys.end();
}
inline void GraphicsHotkeyDown(int scancode) {
    auto key = std::find(graphicsHotkeys.begin(), graphicsHotkeys.end(), scancode);
    if (key == graphicsHotkeys.end()) return;
    auto& held = graphicsKeysHeld[key - graphicsHotkeys.begin()];
    if (!held) graphicsKeyPresses.push_back(scancode);
    held = true;
}
inline void GraphicsHotkeyUp(int scancode) {
    auto key = std::find(graphicsHotkeys.begin(), graphicsHotkeys.end(), scancode);
    if (key != graphicsHotkeys.end()) graphicsKeysHeld[key - graphicsHotkeys.begin()] = false;
}
inline void ResetGraphicsHotkeys() {
    graphicsKeysHeld.fill(false);
    graphicsKeyPresses.clear();
}
}
