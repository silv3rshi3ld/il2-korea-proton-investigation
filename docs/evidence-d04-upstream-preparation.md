# D04 unmodified current-upstream control: preparation

## Purpose

D04 tests whether the rendering defect has already changed in current
VKD3D-Proton before adding more IL-2-specific instrumentation. Microsoft
Flight Simulator 2024 is relevant precedent because a grass/near-ground path
exposed a `dxil-spirv` defect which `spirv-val` did not catch. This makes the
control justified; it does not predict that D04 will fix IL-2.

## Exact source

- VKD3D-Proton: `84c87c8390d9df75ba41d911496296fe13f0e275`
- VKD3D-Proton version: `v3.0.1-259-g84c87c83`
- `dxil-spirv`: `cc75a0c98d34d7bcc03560527c799b52e48b4d1f`
- Installed comparison: VKD3D-Proton `3dfc6f07d0953b1e8b41705275c2c59cc7374fc5`
  with `dxil-spirv` `7ecda135de740f4db016c2bbdf8b021ce6b0bebd`

The source is an isolated detached worktree under ignored `src/`. All tracked
files and recursive submodules match their recorded commits. Meson created one
zero-byte untracked `subprojects/.wraplock`; it is build-system noise and is
not compiled source.

## Build

The official retained development-build method completed for x86-64 and x86:

```bash
./package-release.sh il2-upstream-84c87c83 ../../build --dev-build
```

Build directory:
`build/vkd3d-proton-il2-upstream-84c87c83/`

| File | SHA-256 |
|---|---|
| `x64/d3d12.dll` | `b0b1fe2eb239d38b7d51bac6b12e2309fff5d57c4f77bd3323f2175857a705e8` |
| `x64/d3d12core.dll` | `f716b80f4e0460e3f7290994ca0f4e31943aac2022a7e6d228a17158bd8c9b5b` |
| `x86/d3d12.dll` | `9d35a3077e73518caec83f833a41b6e920bdda2bb6f504064c4999ec1773ee22` |
| `x86/d3d12core.dll` | `d9fe83fa531082bbfcc364057cbc847e49b89ac195ff2c08838bab8a690e1811` |

The build header identifies `0x84c87c8390d9df7`. File inspection confirms
PE32+ x86-64 for the x64 pair and PE32 i386 for the x86 pair.

## Safety and remaining step

The intended custom tool is
`IL2-Korea-D04-Upstream-84c87c83`. Its first creation attempt safely refused
because Steam was running. No custom-tool file, prefix DLL, Proton
Experimental file, or earlier diagnostic tool was changed.

After Steam is fully stopped, create the isolated tool with:

```bash
./scripts/create-custom-proton.sh \
  --source-tool "/home/silv3rshi3ld/.local/share/Steam/steamapps/common/Proton - Experimental" \
  --build-dir "$PWD/build/vkd3d-proton-il2-upstream-84c87c83" \
  --destination "/home/silv3rshi3ld/.local/share/Steam/compatibilitytools.d/IL2-Korea-D04-Upstream-84c87c83" \
  --tool-name IL2-Korea-D04-Upstream-84c87c83 \
  --yes
```

Rollback is selecting Proton Experimental or any retained earlier custom tool.
If D04 is fixed, bisect the complete VKD3D-Proton range rather than attributing
the result directly to `dxil-spirv`. If unchanged, proceed to descriptor QA.
