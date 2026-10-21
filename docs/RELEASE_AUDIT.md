# Lighting-only distribution audit

This copy comes from Toki no Fue's lighting renderer and prewarming implementation.
It is the lighting-only edition, with the normal game dialogue/cutscene flow.

## Excluded from the source repository and Windows package

- ROMs and extracted game archives (`oot.o2r`, `oot-mq.o2r`, and equivalents).
- Every bundled mod: character replacements, Skull Kid, Hero's Shade, ZANT,
  Dark Salem, NPC/3DS packs, equipment, rupees, font replacements and enhanced music.
- OoT Reloaded and Djipi texture/background packs, including modified extracts.
- Local architecture/backdrop patches and fixes containing borrowed resources.
- Japanese dialogue banks, translations/gloss payloads, recordings and dictionaries.
- The Japanese learning implementation, its menu and Tab learning-mode controls.
- Private MM ChuChu mesh, textures, test actor and its menu controls.
- Player saves, settings, controller mappings, caches, logs, captures and fixtures.
- Private build artifacts, Git history and upstream release-publishing workflows.

## Included

- Lighting, sun/fairy/window shadows, material generation and postprocessing code.
- Texture/shader prewarming, cache validation and worker shutdown fixes.
- Tilde/backtick graphics presets and numpad FPS/counter/VSync controls.
- Vendored modified libultraship and Torch source, with component notices retained.
- Lighting shader sources, procedural lookup/noise data and environment maps;
  see `ASSET_PROVENANCE.md` for their origins.
- Upstream custom port assets and extraction recipes needed for local setup.

HD packs are optional, separately obtained inputs. Some material profiles target
replacement resource names and some effects were tuned against HD textures;
do not promise identical material detail with original textures. Full 3D
replacements for pre-rendered scenes are not bundled or enabled by default.

The port archive `soh.o2r` is generated from the checked-in custom port assets;
it must be rebuilt from this clean source. The private donor executable and its
port archive are not release inputs, because they include the private MM test.

This is a content/provenance audit and technical packaging policy, not a legal
determination about the upstream decompilation project. No new blanket license
is asserted for upstream game code or third-party assets.
