# Building the lighting-only edition

The source includes the modified libultraship and pinned Torch trees. No Git
submodule initialization is needed for those components. Additional dependencies
are fetched by CMake unless supplied locally.

On Windows use Visual Studio 2022 C++ build tools, Windows SDK, CMake 3.26+,
Git and Python 3. Run from the repository root:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --target soh GenerateSohOtr --parallel 2
```

The executable is named `yousei-no-tabi.exe`. `GenerateSohOtr` builds the port's
custom asset archive from this source. Do not reuse the private Toki no Fue
executable or archive, which contain an excluded test actor and textures.

For preinstalled dependencies, configure with `-DUSE_AUTO_VCPKG=OFF` and provide
your toolchain/package paths. CMake's standard `FETCHCONTENT_SOURCE_DIR_<NAME>`
options support local dependency sources. `STB_IMAGE_HEADER` accepts a local
copy of the pinned stb_image header. Private machine paths are not part of the
public project configuration.

Package explicit clean-build inputs, replacing the example paths with the
actual build outputs. Add `--dll path/to/library.dll` for each required shared
runtime dependency from that same build:

```powershell
python scripts/package_windows.py --exe build/bin/RELEASE/yousei-no-tabi.exe --port-archive build/soh/soh.o2r --visual-setup build/bin/RELEASE/yousei-visual-setup.exe --output ../packages/yousei-no-tabi-Windows-x64.zip
python scripts/audit_release.py ../packages/yousei-no-tabi-Windows-x64.zip --package
```

The packager includes default settings, lighting assets, extraction recipes and
notices. It does not copy any player runtime, mods, saves or game archives.
For direct development runs, copy the contents of `runtime/` beside the built
executable and keep the build's generated `assets/` extraction recipes there.
Never overwrite a player's existing config with the defaults during an update.

The Windows build also compiles the .NET Framework visual setup companion from
`tools/VisualSetup.cs`. It requires the Windows .NET Framework 4.x compiler.
To build the helper separately, run `scripts/build_visual_setup.ps1 -Output path/to/yousei-visual-setup.exe`.
It uses Windows Forms and obtains a pinned isolated Python runtime only when the
player starts setup; no system Python or pip installation is needed.
Installer regression checks: `python tests/test_visual_setup.py`.

Before publishing source:

```powershell
python scripts/audit_release.py .
```

The content audit is a packaging guard; it does not replace build or gameplay
validation. Its file hashes can be saved with `--output audit.json` outside the
source tree. Upstream build documentation remains in `BUILDING.md` for reference.

The fairy icon source is `soh/yousei-fairy.svg`. Regenerate its multiresolution
Windows ICO with `powershell -File scripts/build_fairy_icon.ps1`. The generator
uses the reference app family palette and 16-unit rounded stroke on a 256-unit
grid, with 16/20/24/32/40/48/64/128/256-pixel frames. Both the game and visual setup
companion embed the same ICO.
