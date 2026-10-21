# Tested Windows preview — 2026-10-21

The lighting-only executable and clean port archive were built successfully in
Release mode with Visual Studio 2022, using local dependency sources and the
installed Windows SDK. No private donor executable or port archive is shipped.

Five focused regression executables passed:

- Presentation-rate measurement and target/mode/VSync resets.
- Shader cache corruption, truncation, bounds and source/options invalidation.
- Material cache corruption, bounds, pixel and algorithm invalidation.
- Prepared texture source settings, epochs, trimming and concurrent access.
- Concurrent shader/material cache publication and byte-identical round trips.

Four save-free native checks passed with the user's existing original-game
archive, no mod archives and no Japanese learning data:

| Check | Verified result |
| --- | --- |
| Enhanced Kokiri Forest | Lighting active; sun/fairy shadow passes; no shadow overflow; nonblank capture |
| Kokiri graphics cycle | All eight original/enhanced presets, then back to the initial preset; independent FPS target preserved |
| Kokiri numpad controls | 7/15/30/60/120 FPS targets, bounds, held-key suppression, counter toggle, VSync toggle |
| Original Castle Town | Original fixed background and camera render; enhanced lighting disabled; nonblank capture |

All four processes exited successfully. No player slots were copied or loaded.
Kokiri and Castle Town captures were visually inspected. Native fixtures use
temporary debug state and are not included in the package or its defaults.

The port archive's three default shaders match the current renderer source.
The private MM test textures and mod overrides are absent. The package includes
the extraction recipes required by the upstream first-run checks.

Initial-preview tested executable SHA256:
`1240a054bef10bd01004ae027bfe948bea80c1730b874a0538a68099643ab716`

Tested port archive SHA256:
`8d63b24125d7d98ab5b8501ea5e5a6c5d1e75667ba686d35f47a01d6c8e04fad`

These checks establish representative startup/rendering/control behavior, not
a manual whole-game walkthrough, a fixed performance guarantee, or testing of
all optional HD packs. The clean online dependency bootstrap and other operating
systems were not tested in this offline validation environment.
# Recommended visual setup checks (2026-10-21)

The Windows companion and in-game menu integration compile successfully. Six
installer regressions cover settings preservation, interrupted-install recovery,
undo preserving later controls/saves, bad downloads, restricted HTTPS redirects,
and duplicate archive names. They pass with the pinned embedded Python runtime.
The setup window was rendered and visually inspected; argument quoting and the
manual-import manifest parser were exercised in a compiled companion harness.

The actual installer downloader retrieved both official packs and verified their
complete SHA-256 hashes. The companion's pinned Python download also passed.
An isolated installation produced all 14 selected Djipi archives and Reloaded HD
with the expected hashes. Locally generated town (134 entries) and shop (4 entries)
repairs match the private reference patch resources exactly; town generation was
also verified with the isolated embedded interpreter. Undo restored prior visual
settings and moved the installed mods aside, retaining the generated global
settings save. No player slot saves were copied or created.

Two additional native fixtures, enhanced Kokiri Forest and Castle Town, completed
with exit 0, sun/fairy shadow passes and zero shadow overflow. Captures were visually
inspected. Native fixtures use a temporary debug game state and do not establish
a whole-game walkthrough, steady target FPS, or identical personal character mods.


Preview 2 native-tested executable SHA-256: `230ac2487a8f429ab851d33acc04640cdb91e0ae33339dbf435ed7526e11508a`.

## Fairy icon refresh (Preview 3)

The Windows game and visual setup companion were rebuilt with the new fairy icon.
The artwork uses the reference app family's exact amber/dark colors and 16-unit
rounded stroke on a 256-unit canvas. Its 241 × 241 alpha bounds match Pinful's.
The ICO contains 16/20/24/32/40/48/64/128/256-pixel frames, all checked for valid
transparency and the expected amber. A comparison with Everywhere, Individuous
and Pinful was inspected at taskbar sizes.

Icons extracted from both finished executables show the new fairy. The companion's
managed icon resource matches the full source ICO byte-for-byte, and its window
Icon property was instantiated and inspected. This changes branding and icon
build dependencies; gameplay and installer behavior retain the preceding preview's
validation. Whole-game gameplay tests were not repeated for this artwork change.

## Preview 4 packaging refresh

The visual setup helper uses a separate fairy icon with an amber dot. Its EXE
and window icons were extracted and checked; the embedded ICO matches source.
Bundled instructions use the corrected UTF-8 menu arrows and punctuation.
This artwork/documentation refresh retains the preceding functional validation;
whole-game and installation flows were not repeated for this change.
