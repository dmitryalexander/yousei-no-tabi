# Recommended visual setup

The Windows menu button opens a separate setup window so the player can save and
close the game before installing. No download begins just from opening the window.
The player chooses Download and set up, then Start game after successful installation.

## Step-by-step setup

1. Download the **Windows x64 ZIP** from [Preview 3](https://github.com/dmitryalexander/yousei-no-tabi/releases/tag/v0.1.0-preview.3),
   extract it completely, and run `Play Yousei no Tabi.cmd`.
2. Supply your supported ROM when prompted. Finish extraction before using visual setup.
3. Press **F1 / Escape** and open **Enhancements → Lighting Engine → Set up recommended visuals...**.
4. Save and close the game. The separate setup window can stay open.
5. Click **Download and set up**. It needs about 1.3 GB of downloads and 12 GiB free
   on the game drive. No system Python installation or manual mod copying is required.
6. Wait for the installation success message, then click **Start game**.

You can instead open `yousei-visual-setup.exe` directly from the extracted game folder.
Keep its `visual-setup/` folder alongside it. Opening setup alone does not download anything.

## Troubleshooting

- **The menu button is missing:** use Preview 2 or newer. Preview 1 does not include it.
- **Setup says to close the game:** save and exit all Yousei no Tabi instances, then try again.
- **No extracted game archive:** run the game and complete ROM extraction first.
- **Download unavailable or blocked:** open the author links in the setup window and
  download the matching versions. Click **Use downloaded packs...** and select the
  original `oot-reloaded-v11.0.0-soh-o2r-hd.7z` and
  `djipi_s_3ds_experience_tot_fix.zip`, then click **Download and set up**. Verified
  cached downloads are reused. Different versions are rejected rather than installed.
- **Windows cannot extract the 7z:** extract it with a compatible archive tool, import
  `OoT_Reloaded_v11.0.0_HD.o2r` through **Use downloaded packs...**, and retry setup.
- **Checksum changed:** do not rename or bypass verification. Check for an updated
  Yousei no Tabi recipe; setup is pinned to the audited pack versions.
- **Setup was cancelled or interrupted:** reopen the companion and retry setup or
  choose **Undo last setup** before starting the game. Keep the `.visual-setup/`
  directory so the previous installation can be recovered.
- **Return to your previous setup:** close the game and choose **Undo last setup**.
  It restores previous mods and visual settings while preserving saves and unrelated settings.

## Sources checked on 2026-10-21

| Input | Local download route | Redistribution finding |
| --- | --- | --- |
| OoT Reloaded v11.0.0 HD by GhostlyDark and credited contributors | Public HD/SoH link on the author's [website](https://evilgames.eu/texture-packs/oot-reloaded.htm) | No general redistribution license found in author page, source README or local readme; pack remains outside our releases. This is an ordinary user-requested download from the official distribution, not a claimed redistribution grant. |
| Djipi's 3DS Experience 6.0.2, file 1810733 | Official file endpoint advertised by the author's [GameBanana listing](https://gamebanana.com/mods/477979) and its mod-manager integrations | API ProfilePage checklist explicitly allows downloading/installing and forbids redistribution on other sites. The listed [CC BY-NC-ND 4.0 license](https://creativecommons.org/licenses/by-nc-nd/4.0/) also requires attribution, noncommercial use and no distribution of modified material. No Djipi files or generated adaptations are shipped in our release. |
| Embedded Python 3.14.8, x64 | Official [Python Windows downloads](https://www.python.org/downloads/windows/) | Official runtime and its PSF/component notices are retained in the player's setup directory; it is not installed system-wide. |
| mpyq 0.2.5 | Small reader included as source | BSD notice retained in `runtime/visual-setup/MPYQ-LICENSE.txt`. |

These findings document author distribution routes and published terms, not a
blanket clearance of underlying third-party assets. Direct downloads go to the
player's installation; none are mirrored on the Yousei no Tabi release server.
Changes to author endpoints, contents or terms require another review.

## What matches the reference build

The recipe installs the exact Reloaded HD O2R plus 14 byte-identical selected Djipi
archives: Animals, Inventory, NPC and Enemies/Bosses model/texture pairs, Background
3DS/Textures, and optional Full 3D/Textures. Reloaded loads first, the selected
Djipi files override it, and the generated architecture/shop patches load last.

The town patch is generated from downloaded geometry and the player's own extracted
game archive. Its 134 uncompressed resource entries match the private lighting build
exactly in the audited fixture. The four shop-fix entries also match. No patch bytes
are embedded in the generator. Lighting/material values use a reviewed allowlist;
private test/debug, language, player data and unrelated settings are not imported.

The original Link model and music remain. Personal character replacements,
HugAMuffin NPC Overhaul, equipment, font, rupee and model-specific private overrides
are outside this preset. This reproduces the reference lighting/background recipe,
not every personal cosmetic choice. Whole-game visual equivalence is not claimed.

## Installation and recovery

All downloads and selected extracted archives have pinned sizes and SHA-256 hashes
in `runtime/visual-setup/manifest.json`. Unexpected versions fail before installation.
Downloaded ZIP entries are selected by exact basename, copied only to fixed
destinations, and checked again. Reloaded is streamed out of the verified 7z using
Windows tar; older Windows versions can use an extracted, verified O2R import.
HTTPS redirects are restricted to the known author distribution hosts. Python's
runtime ZIP is separately pinned and freshly extracted for each setup session.

Download progress and cancellation are available. Installation requires the game
to be closed, enough free disk space, and an already extracted supported ROM.
Existing mods and config are backed up before the generated directory is activated.
Failed downloads/builds do not replace them. A transaction journal restores an
interrupted installation on retry or undo; the game refuses to start with an
unfinished transaction. Undo changes only the owned visual settings and restores
the previous mods, retaining later controller/audio settings and player saves.

Downloaded files, generated patches and backups remain under the player's
`.visual-setup/` directory or `mods/`. They are ignored by Git and rejected by the
release content audit. Never copy a configured player installation into a release.
