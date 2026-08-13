# IL-2 Korea community test Proton

This directory is the repository home for the optional, unofficial
**IL-2 Korea: Three-Fix Proton (2026-08-13)** compatibility tool.

## Download

The distributable is named:

`IL2-Korea-Proton-Three-Fixes-20260813.tar.zst`

At 417 MB it exceeds GitHub's ordinary 100 MB Git file limit, so the binary is
not committed to the repository. It belongs on the repository's
[Releases page](https://github.com/silv3rshi3ld/il2-korea-proton-investigation/releases)
as a release asset. Until that asset is explicitly published, the locally
verified copy remains under `dist/`, which is intentionally ignored by Git.

Always download the matching `.sha256` file and verify the archive before
extracting it:

```bash
sha256sum -c IL2-Korea-Proton-Three-Fixes-20260813.tar.zst.sha256
```

Expected SHA-256:

`e41f4c4a28e678fbce591f0142a48dcad97900a01415190d6511a0a46e29535b`

The tracked checksum file is
[`IL2-Korea-Proton-Three-Fixes-20260813.tar.zst.sha256`](IL2-Korea-Proton-Three-Fixes-20260813.tar.zst.sha256).

## Included compatibility changes

1. Wine's NUMA topology APIs for startup without a hard-coded processor count.
2. VKD3D-Proton's physical-block conversion for the missing terrain and map
   pages.
3. The exact IL-2 executable and shader scoped VKD3D-Proton lighting
   workaround used by the verified combined build.

The third change is a tested compatibility workaround, not the preferred
general upstream architecture. The final investigation instead points toward
the native-compatible RADV behavior discussed in
[Mesa MR !43672](https://gitlab.freedesktop.org/mesa/mesa/-/merge_requests/43672).

## Installation

1. Exit Steam completely.
2. Extract the archive without removing its top-level directory.
3. Place `IL2-Korea-Proton-Three-Fixes-20260813` under:

   - native Steam: `~/.local/share/Steam/compatibilitytools.d/`
   - Flatpak Steam:
     `~/.var/app/com.valvesoftware.Steam/data/Steam/compatibilitytools.d/`

4. Restart Steam.
5. Open the game's **Properties**, then **Compatibility**.
6. Force the compatibility tool named
   **IL-2 Korea: Three-Fix Proton (2026-08-13)**.
7. Leave Steam launch options empty and start the game.

No compilation, DLL copying, game-file modification, or game reinstall is
required. The archive contains a more detailed `README-FIRST.md`, component
checksums, exact provenance, source links, packaged patches, and rollback
instructions.

## Testing-only disclaimer

This package is provided as-is solely for community compatibility testing. It
comes without warranty, guaranteed updates, maintenance, or user support. Use
it at your own risk. Test reports are welcome as evidence but do not create an
obligation to investigate or respond.

The packager claims no additional rights over Proton, Wine, VKD3D-Proton,
Mesa, Steam, Korea. IL-2 Series, or other third-party components. All upstream
copyrights and license terms remain applicable. A blanket "no rights reserved"
statement cannot waive rights owned by those third parties.

This is not an official Valve, Wine, VKD3D-Proton, Mesa, Steam, or IL-2
release, and it is not a game mod.
