# Yousei no Tabi â€” Lighting Engine

A Ship of Harkinian fork focused on dynamic lighting, real shadows, material
effects, and preparation of textures and shaders before they are needed.

This is the **lighting-only edition**. It uses the game's normal text and audio.
The Japanese learning edition is planned as `toki-no-fue`; the combined edition
is `yousei-no-tabi-to-toki-no-fue`.

## Features

- Sun, fairy and window lighting with shadow casters outside the camera view.
- HDR environment lighting, material reflections, normal/parallax effects,
  color grading and glow.
- Progression-aware background CPU preparation, persistent shader/material
  caches, and validated cache invalidation. GPU work stays on the game thread.
- Eight original/enhanced graphics presets with independent FPS controls.

The enhanced renderer currently targets **Windows x64 / DirectX 11**.
Other upstream renderers are retained but are not validated for these effects.
Preparation reduces some loading work; it does not guarantee hitch-free play
or a fixed frame rate on every system.

## Setup

### Windows quick start

1. Open [Preview 3](https://github.com/dmitryalexander/yousei-no-tabi/releases/tag/v0.1.0-preview.3)
   and download **yousei-no-tabi-Windows-x64-v0.1.0-preview.3.zip**. Choose the Windows
   ZIP under Assets; GitHub's Source code ZIP is for building the program yourself.
2. Extract the **entire ZIP** into a new folder. Run `Play Yousei no Tabi.cmd` or
   `yousei-no-tabi.exe`, then follow the first-run prompt to extract your supported ROM.
3. For the recommended HD look, press **F1 / Escape**, then open
   **Enhancements â†’ Lighting Engine â†’ Set up recommended visuals...**.
4. The setup window opens separately. **Save and close the game**, leaving setup open.
5. Click **Download and set up**. Allow about **1.3 GB of downloads** and **12 GiB of
   free disk space**. Wait for the successful installation message, then click **Start game**.

The setup automatically downloads Reloaded HD and selected Djipi files from their
official sources, builds the town/shop repairs locally, and sets the load order and
lighting/material profile. Keep the included `lighting/`, `assets/`, `visual-setup/`
folders and both executables together. Existing mods are backed up; saves and controls
are preserved. **Undo last setup** restores the previous mods and visual settings.

The download, install and undo flows were tested, along with native Kokiri Forest
and Castle Town scenes. Testing has not covered a whole-game walkthrough. This
preset retains Link and the original music; private character/equipment cosmetics
are separate. See [troubleshooting and manual import](docs/VISUAL_SETUP.md#troubleshooting).

Provide your own legally obtained, supported Ocarina of Time ROM. No ROM,
extracted game archive, HD pack, character mod, replacement music, or Japanese
dialogue/voice package is included. Follow the extraction prompt on first run.
Supported hashes and extraction recipes come from the pinned upstream source.

For Windows packages, extract the whole archive into its own directory and run
`Play Yousei no Tabi.cmd` or `yousei-no-tabi.exe`. Keep the included `lighting/`
and `assets/` directories beside the executable. Settings and saves are separate
from an existing Ship of Harkinian installation.

## Controls

| Key | Action |
| --- | --- |
| Tilde / backtick | Cycle 320p, 540p, 720p, 1080p original/enhanced presets |
| Numpad + / - | Adjust FPS target: 7, 15, 30, 60, 120 |
| Numpad * | Toggle the small measured-FPS/target counter |
| Numpad . | Toggle VSync |
| F1 / Escape | Open the upstream menu; see Enhancements â†’ Lighting Engine |

Tab retains the upstream optional mod toggle in this edition; it does not switch
Japanese learning modes. Simulation timing is independent of a low FPS target.

## Optional textures

On Windows, open **Enhancements â†’ Lighting Engine â†’ Set up recommended visuals...**.
Save and close the game, then choose **Download and set up** in the companion window.
It downloads the pinned Reloaded HD and selected Djipi packs from their official
sources, generates the town/shop repairs locally, and applies the reference lighting
and material settings. Downloads total about 1.3 GB; allow 12 GiB of free space.
The existing mods directory is backed up to avoid conflicting load orders. Saves,
controls, audio settings and the independent FPS target are preserved.
**Undo last setup** restores the previous mods and visual settings.

The companion can also be opened directly as `yousei-visual-setup.exe`.
**Use downloaded packs...** imports verified original ZIP/7z files (or the extracted
Reloaded O2R) when a manual download is needed. Keep the `visual-setup/` folder beside
the companion. Never publish its player-created `.visual-setup/` cache/backups or
generated mod files. See [visual setup and permissions](docs/VISUAL_SETUP.md).

Lighting and shadows do not require a bundled texture pack. Material effects
use decoded textures supplied by the player; some profiles target HD replacement
names and the appearance varies with the installed pack. Obtain optional packs
from their authors. [OoT Reloaded](https://evilgames.eu/texture-packs/oot-reloaded.htm)
is one such optional input. Full 3D replacements for fixed backgrounds are not
included or enabled by default.

See [building](docs/BUILDING_LIGHTING.md), [distribution audit](docs/RELEASE_AUDIT.md),
[validation](docs/VALIDATION.md), [asset provenance](docs/ASSET_PROVENANCE.md),
and [upstream revisions](docs/UPSTREAM.md).
This is an unofficial fork, independent of Nintendo and Harbour Masters.
Existing component licenses and copyright notices are retained; no blanket
license is claimed over the entire upstream game code or game assets.
