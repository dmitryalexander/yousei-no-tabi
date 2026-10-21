# Yousei no Tabi — Lighting Engine

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
| F1 / Escape | Open the upstream menu; see Enhancements → Lighting Engine |

Tab retains the upstream optional mod toggle in this edition; it does not switch
Japanese learning modes. Simulation timing is independent of a low FPS target.

## Optional textures

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
