# IL-2 Korea community test Proton

This directory is the repository home for the optional, unofficial
**IL-2 Korea: Three-Fix Proton (2026-08-13)** compatibility tool.

## Download

The distributable is named:

`IL2-Korea-Proton-Three-Fixes-20260813.tar.zst`

At 417 MB it exceeds GitHub's ordinary 100 MB Git file limit, so the binary is
not committed to the repository. It is published as a prerelease on the
[`community-proton-three-fixes-2026-08-13` release page](https://github.com/silv3rshi3ld/il2-korea-proton-investigation/releases/tag/community-proton-three-fixes-2026-08-13).
The locally verified build directory remains under the ignored `dist/`
directory.

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

4. Keep exactly one copy of this tool under `compatibilitytools.d`. Move old,
   incomplete, or backup copies completely outside that directory. Merely
   renaming a copy in place does not hide its manifest from Steam.
5. For native Steam, verify that all four entry-point files exist. These
   commands work in Bash and Fish:

   ```console
   ls -l \
     "$HOME/.local/share/Steam/compatibilitytools.d/IL2-Korea-Proton-Three-Fixes-20260813/proton" \
     "$HOME/.local/share/Steam/compatibilitytools.d/IL2-Korea-Proton-Three-Fixes-20260813/compatibilitytool.vdf" \
     "$HOME/.local/share/Steam/compatibilitytools.d/IL2-Korea-Proton-Three-Fixes-20260813/toolmanifest.vdf" \
     "$HOME/.local/share/Steam/compatibilitytools.d/IL2-Korea-Proton-Three-Fixes-20260813/version"
   ```

   Flatpak users should substitute the Flatpak directory shown above. Do not
   rely on a temporary shell variable from an earlier terminal session when
   performing this check.
6. Restart Steam. The tool manifest requires **Steam Linux Runtime 4.0**
   (AppID `4183110`), which Steam normally installs or updates automatically.
7. Open the game or shortcut's **Properties**, then **Compatibility**.
8. Force the compatibility tool named
   **IL-2 Korea: Three-Fix Proton (2026-08-13)**.
9. Leave Steam launch options empty and start the game.

No compilation, DLL copying, game-file modification, or game reinstall is
required. The archive contains a more detailed `README-FIRST.md`, component
checksums, exact provenance, source links, packaged patches, and rollback
instructions.

The published archive and checksum remain unchanged. This online page contains
post-release installation and troubleshooting clarifications learned from the
first external standalone-install attempt.

## Official Steam and standalone installations

The controlled investigation and successful combined-package runs used the
official Steam entry, AppID `247970`.

A website-purchased or otherwise standalone installation can also be added to
Steam as a non-Steam shortcut for testing. Before selecting the compatibility
tool, locate the actual executable rather than copying an example path:

```console
find "$HOME/Games/IL2Series" -maxdepth 6 -type f \
  \( -iname 'Launcher.exe' -o -iname 'IL2Series.exe' \) -print
```

Set the shortcut's **Target** to the exact absolute path, including the real
capitalization, and set **Start In** to that executable's parent directory.
Linux paths are case-sensitive. Prefer the installation's real launcher when
the standalone edition depends on it for authentication or updates; do not
invent a `Launcher.exe` path or bypass it without knowing that the direct game
executable is supported.

Steam assigns a generated 64-bit `SteamGameId` to a non-Steam shortcut. That
number is expected and does not need to equal `247970`. Similarly, AppID
`4183110` in this tool's manifest identifies Steam Linux Runtime 4.0, not the
game. The Wine startup work has no game-AppID check, the terrain change is
general, and the packaged lighting workaround selects `IL2Series.exe` plus the
exact affected shader. Even so, the standalone launcher path has not yet
completed an in-game package validation, so it remains a provisional testing
route rather than a confirmed equivalent of the official Steam entry.

## Troubleshooting

- `err:steam:run_process Failed to create process ...: 2` means the shortcut's
  target file was not found. Check that the file exists, its capitalization is
  exact, and **Target** and **Start In** point to the correct locations. This
  occurs before the game starts and is not evidence of a Wine, DXVK, terrain,
  or lighting failure.
- `err:steamclient:steamclient_init_registry Failed to connect to Steam` is
  not, by itself, enough to identify the blocker. In the first standalone
  attempt it appeared immediately before the decisive file-not-found error;
  the game process was never created. Investigate Steam integration separately
  only after the target launches.
- A log path naming an old or backup copy of this tool indicates that Steam is
  still enumerating that copy. Exit Steam and move the duplicate completely
  outside `compatibilitytools.d`.
- Missing entry-point files or a tool-version error usually means the archive
  was only partially extracted. Re-extract it and repeat the four-file check
  above.
- `PROTON_LOG=1 %command%` is a temporary diagnostic launch option, not part of
  the fix. Remove it after collecting the resulting `steam-<id>.log`.

## Reporting a result

Please distinguish package setup from actual game validation. Include:

- GPU model, Mesa version, and Vulkan driver;
- CPU model and logical processor count;
- native or Flatpak Steam;
- official Steam edition or website/standalone edition;
- normal Steam entry or non-Steam shortcut;
- selected compatibility-tool directory and generated `SteamGameId`, if any;
- whether Steam launch options were empty;
- the highest stage reached: tool discovery, runtime start, launcher start,
  game start, menu, mission, or map;
- the first decisive `err:` line if startup stopped; and
- only after `IL2Series.exe` ran: whether terrain, map, lighting blocks, and
  ordinary shadows worked.

The first external standalone attempt is recorded as
[`docs/evidence-community-package-bootstrap-2026-08-13.md`](../docs/evidence-community-package-bootstrap-2026-08-13.md).
It confirms package and runtime bootstrap on an RX 9070 XT but is not an
independent confirmation of any in-game fix.

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
