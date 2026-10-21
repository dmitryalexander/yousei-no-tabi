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

Tested executable SHA256:
`1240a054bef10bd01004ae027bfe948bea80c1730b874a0538a68099643ab716`

Tested port archive SHA256:
`8d63b24125d7d98ab5b8501ea5e5a6c5d1e75667ba686d35f47a01d6c8e04fad`

These checks establish representative startup/rendering/control behavior, not
a manual whole-game walkthrough, a fixed performance guarantee, or testing of
all optional HD packs. The clean online dependency bootstrap and other operating
systems were not tested in this offline validation environment.
