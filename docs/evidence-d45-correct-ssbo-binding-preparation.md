# D45 correct SSBO descriptor binding: preparation

## Purpose

D45 completes the D25 compatibility translation. The exact IL-2 allocator
shader performs a 32-bit atomic through an `R16_UINT` typed UAV. VKD3D-Proton
already emits both a texel-buffer descriptor and a raw storage-buffer sibling
for the D3D descriptor. The quirk must change both halves of the contract:

1. emit the atomic as a `StorageBuffer` access;
2. select the raw SSBO descriptor sibling rather than the typed binding.

D25 implemented only the first item. D44 value capture proves the allocator
remained workgroup-local in the real game.

## Implementation

The existing `VKD3D_SHADER_QUIRK_FORCE_TYPED_UAV_AS_SSBO` remains scoped to:

- executable: `IL2Series.exe`;
- shader: `7cefa1bc80bb4c70` (`ComputeLightsFirstRef`).

When that quirk forces a typed UAV to an SSBO, D45 additionally requests
`VKD3D_SHADER_BINDING_FLAG_RAW_SSBO`. Normal typed UAVs and every other game
retain existing behavior.

Local source identity:

- branch: `il2-d45-correct-ssbo-binding`;
- commit: `1368b538`;
- parent: D42 commit `2d9a7467`;
- package: `build/vkd3d-proton-il2-d45-correct-ssbo-binding-1368b538`.

Both MinGW architectures build successfully. Packaged DLL hashes are:

| Architecture | File | SHA-256 |
| --- | --- | --- |
| x86-64 | `d3d12.dll` | `cb9171d18a137f4d07b276048c5f748042e6ffe6c83ce5d919e17b1d9bf59f7f` |
| x86-64 | `d3d12core.dll` | `a164833c4a798c21f16e3d7690538a9ff7de31f78f7a31fd4d25d317153d4d47` |
| x86 | `d3d12.dll` | `fbe779f41b59a7f13987547dfec5cb9958c3b2ca6edaa95b84d732419928e063` |
| x86 | `d3d12core.dll` | `e489f24fd7864e2b2e71201b3e35fd3c88c0fef021d27727bd2b761c02c7a9ce` |

The isolated tool `IL2-Korea-D45-CorrectSSBO-1368b538` was then created from
`IL2-Korea-D42-Complete-2d9a7467` while Steam was fully stopped. All four
installed DLL hashes match the package table. The source tool, game prefix,
official Proton installation, and launch options were not modified.

## Runtime gate

Test the installed normal custom Proton tool with empty Steam launch options.
It uses the same NUMA-capable D42 Wine base, with no RenderDoc integration and
no shader overrides.

Pass requires:

- game starts normally;
- no large square blocks in menu, cockpit, or fire-lit scenes;
- no broad light flicker;
- real lighting and shadows remain.

Fine sandy or film-grain lighting is normal on Windows and is not a failure.

The first runtime result is recorded in
[`evidence-d45-correct-ssbo-binding-result.md`](evidence-d45-correct-ssbo-binding-result.md).
