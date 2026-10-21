# Lighting asset provenance

| Runtime asset | Origin |
| --- | --- |
| `environment.ibl` | GGX-prefiltered Poly Haven Industrial Sunset 02 by Sergej Majboroda; CC0 |
| `forest.ibl` | GGX-prefiltered Poly Haven Forest Slope by Andreas Mischok; CC0 |
| `nishita-day.ibl`, `nishita-low.ibl`, `nishita-dusk.ibl` | Locally rendered procedural Blender sky environments, sun disc disabled |
| `interior.ibl` | Locally generated neutral procedural room environment |
| `brdf.bin` | Locally generated split-sum BRDF integration lookup |
| `jelly-detail.bin` | Locally generated seamless Godot FastNoiseLite normal texture; seed 4182 |
| `shadow.hlsl`, `navi-shadow.hlsl`, `post.hlsl` | Custom lighting shader source from this renderer |
| `soh/yousei-lighting.ico` | Local Lighting Lab eye icon; replaces the private Triforce branding |

Primary asset references:

- https://polyhaven.com/a/industrial_sunset_02
- https://polyhaven.com/a/forest_slope
- https://polyhaven.com/license (CC0; redistribution permitted)

The environment files are lighting data, not extracted Zelda textures. Raw HDR/EXR
sources, duplicate mip experiments and material caches are not bundled. Upstream
port assets retain their existing provenance and component notices.
