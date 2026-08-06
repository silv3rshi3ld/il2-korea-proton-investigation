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
checked; runtime parity remains the relevant control.

## Safety state

The full prefix backup
`il2-korea-247970-prefix-20260806T114004Z.tar.zst` exists under the user's local
state directory and passes its SHA-256 verification. The active prefix DLLs
still match the original Proton payload as of this build step. No custom DLL
has yet been installed.

Installation is restricted to the four D3D12 DLLs in the AppID 247970 prefix:

```bash
./scripts/install-vkd3d-build.sh install \
  --build-dir "$PWD/build/vkd3d-proton-il2-baseline-3dfc6f07" \
  --yes
```

The script creates another DLL-only backup and prints the exact rollback
command. Steam and the game must be stopped.

## D00 parity procedure

1. Install the unmodified local DLLs with Steam stopped.
2. Prepare `D00-local-vkd3d-unmodified-r1` with variant
   `local-vkd3d-baseline`.
3. Check the menu aircraft for the established moving block artifacts.
4. Enter the established Singo-dong flight and capture once below 1,500 m and
   once near 5,000 m.
5. Exit and collect the log.
6. Confirm the logged build identifier and compare with E00.

Only after D00 reproduces the defect should a diagnostic build add filtered
telemetry. The first instrumentation gate is whether the game actually calls
`CreateReservedResource`, `GetResourceTiling`, `UpdateTileMappings`, or
`CopyTileMappings`. If it does not, investigation shifts to the game's own
texture atlas, mip-range SRVs, upload copies, descriptors, and resource
lifetime rather than Vulkan sparse residency.
