# VKD3D-Proton development build

## Purpose

The launch-option controls did not isolate the graphics defect. Development
work therefore begins with an unmodified build of the exact VKD3D-Proton commit
inside the selected Proton Experimental build. This is a parity control, not a
candidate fix.

## Source and toolchain

- Repository: `https://github.com/HansKristian-Work/vkd3d-proton.git`
- Source commit: `3dfc6f07d0953b1e8b41705275c2c59cc7374fc5`
- Generated build identifier: `0x3dfc6f07d0953b1`
- VKD3D source version: `v3.0.1-236-g3dfc6f07`
- Meson: `1.11.2`
- Ninja: `1.13.2`
- x64 and x86 MinGW-w64 GCC: `16.1.0`
- Build type: release development build; build directories retained

The recursive source checkout is locally retained under ignored `src/`; build
products are retained under ignored `build/`.

## Reproduction command

From `src/vkd3d-proton` at the pinned commit:

```bash
./package-release.sh il2-baseline-3dfc6f07 ../../build --dev-build
```

This is VKD3D-Proton's official package-release development-build path. Both
architectures completed successfully.

## Build artifacts

Directory:
`build/vkd3d-proton-il2-baseline-3dfc6f07/`

| File | Architecture | SHA-256 |
|---|---|---|
| `x64/d3d12.dll` | PE32+ x86-64 | `79c6d14d110bf0337cdec9b4f712cc61b01ac59fc15bdc8446a2cb75bec7f481` |
| `x64/d3d12core.dll` | PE32+ x86-64 | `ba3395a3a20dc82039b8ff011c51b7d0c19921504d12969e71a863c63d191ca9` |
| `x86/d3d12.dll` | PE32 i386 | `e0f64e7b97d148c796b137c404cb8e54810cbf7114b1549f2abbb9185ca36c30` |
| `x86/d3d12core.dll` | PE32 i386 | `b55cb67ae733810ffa130b7952981820de8836706af77e3c19f13aa4f0a87152` |

Different hashes from Proton's packaged DLLs are expected because the local
compiler and link environment differ. Architecture and D3D12 export names were
checked. D00 did not establish runtime parity because stock Proton replaced the
prefix-copied DLLs during launch.

## Safety state

The full prefix backup
`il2-korea-247970-prefix-20260806T114004Z.tar.zst` exists under the user's local
state directory and passes its SHA-256 verification.

The unmodified local DLLs were installed into the AppID 247970 prefix on
2026-08-06. Immediately before installation, the installer created and
verified this DLL-only backup:

```text
/home/silv3rshi3ld/.local/state/il2-korea-proton-investigation/vkd3d-dll-backups/vkd3d-dll-backup-20260806T155429Z
```

The four DLL hashes matched the build-artifact table immediately after
installation, and no Proton installation file was modified. This state was not
durable: Proton recopied its packaged files when the game launched.

Installation was restricted to the four D3D12 DLLs in the AppID 247970 prefix:

```bash
./scripts/install-vkd3d-build.sh install \
  --build-dir "$PWD/build/vkd3d-proton-il2-baseline-3dfc6f07" \
  --yes
```

Rollback, with Steam and the game stopped:

```bash
./scripts/install-vkd3d-build.sh restore \
  --backup-dir /home/silv3rshi3ld/.local/state/il2-korea-proton-investigation/vkd3d-dll-backups/vkd3d-dll-backup-20260806T155429Z \
  --yes
```

## D00 invalidated parity attempt

D00 completed on 2026-08-06 and reproduced the same corruption. Because the
local build and packaged build use the same source identifier, the log cannot
distinguish them. D01a later proved that the prefix-copy method is overwritten
by Proton at launch. D00 must therefore be treated as another packaged-Proton
reproduction, not local-build parity. See
[`evidence-d00-local-build.md`](evidence-d00-local-build.md).

## D01 trace-only build

- Source base: `3dfc6f07d0953b1e8b41705275c2c59cc7374fc5`
- Local diagnostic commit: `d0b4421f` (`il2-korea-resource-trace`)
- Build directory: `build/vkd3d-proton-il2-resource-trace-3dfc6f07/`
- Gate: `VKD3D_IL2_RESOURCE_TRACE=1`

The instrumentation records stable resource cookies and the parameters of
`CreateReservedResource`, `GetResourceTiling`, `UpdateTileMappings`, and
`CopyTileMappings`. Logging is disabled unless the gate is set. It does not
alter synchronization, image layouts, descriptors, allocations, queues, or
resource lifetime.

| File | Architecture | SHA-256 |
|---|---|---|
| `x64/d3d12.dll` | PE32+ x86-64 | `37a302c0768f5755f47dca7c26724cdfc1ccd291825b3b397ccd64f5260d8942` |
| `x64/d3d12core.dll` | PE32+ x86-64 | `e8b4de0df971fbf6e4e4b267210815b8bad1f38479c00d6bc8c7e8ebd484ac19` |
| `x86/d3d12.dll` | PE32 i386 | `7b867e13c54908dac7adf044c01a8a9985c59d538af4377871c52c6091962807` |
| `x86/d3d12core.dll` | PE32 i386 | `3260a153211725afa7e84364d3bc931be7e81b4df841e78a8acd4610d35c5d04` |

Installation displaced the unmodified local DLLs into the verified rollback
directory:

```text
/home/silv3rshi3ld/.local/state/il2-korea-proton-investigation/vkd3d-dll-backups/vkd3d-dll-backup-20260806T160657Z
```

Before D01 launched, Steam auto-updated the game to build `24596901`. Its
install step restored all four prefix D3D12 DLLs to hashes matching Proton
Experimental, so the trace marker cannot be active in U00. The diagnostic DLLs
remain available in the retained build directory. U00 subsequently confirmed
the same corruption, so the trace build is cleared for reinstallation before
D01.

The trace build was copied into the prefix again after U00 on 2026-08-06. The
clean Proton-DLL rollback point was:

```text
/home/silv3rshi3ld/.local/state/il2-korea-proton-investigation/vkd3d-dll-backups/vkd3d-dll-backup-20260806T165614Z
```

D01a contained no `IL2TRACE` marker. Post-run prefix hashes exactly matched the
packaged Proton DLLs, proving that Proton replaced the diagnostic files before
VKD3D initialized. Zero reserved/tiled-resource calls in that log are therefore
not evidence about the game. See
[`evidence-d01-invalid-prefix-install.md`](evidence-d01-invalid-prefix-install.md).

## Corrected custom-Proton method

`scripts/create-custom-proton.sh` creates a Btrfs copy-on-write clone of Proton
Experimental under Steam's `compatibilitytools.d`, replaces only the clone's
four packaged VKD3D-Proton DLLs, adds a unique compatibility-tool manifest, and
records hashes and source versions. It does not modify Proton Experimental or
the game prefix. When the clone is selected, Proton's normal DLL-copy step
installs the diagnostic files rather than overwriting them.

After Steam is stopped, create the tool with:

```bash
./scripts/create-custom-proton.sh \
  --source-tool "/home/silv3rshi3ld/.local/share/Steam/steamapps/common/Proton - Experimental" \
  --build-dir "$PWD/build/vkd3d-proton-il2-resource-trace-3dfc6f07" \
  --destination "/home/silv3rshi3ld/.local/share/Steam/compatibilitytools.d/IL2-Korea-Diagnostic-3dfc6f07" \
  --tool-name IL2-Korea-Diagnostic-3dfc6f07 \
  --yes
```

Restart Steam and select `IL2-Korea-Diagnostic-3dfc6f07` for AppID 247970.
The next log must contain `IL2TRACE enabled` before any API counts are
interpreted. Rollback is selecting Proton Experimental again; the custom tool
must not be removed while Steam is running.
