# D46 allocator-only minimality control: preparation

## Purpose

D45 resolves the square blocks and broad light flicker on two independent game
starts. It still inherits D38's diagnostic bypass of two tiled-light depth
predicates. D46 removes those depth changes and retains only the corrected
allocator compatibility behavior.

This is a minimality control, not a new attempt to find the cause. If D46 is
also clean, the complete graphics fix is one narrowly scoped translation of
IL-2's invalid 32-bit atomic through an `R16_UINT` typed UAV. If the blocks or
broad flicker return, the allocator and depth behavior must be treated as two
required fixes.

## Implementation

The remaining behavior is scoped to:

- executable: `IL2Series.exe`;
- shader: `7cefa1bc80bb4c70` (`ComputeLightsFirstRef`);
- action: emit the typed-UAV atomic as a storage-buffer access and select the
  raw SSBO descriptor sibling already emitted by VKD3D-Proton.

No depth-culling shader is changed. Relative to common source commit
`0c45e3d2`, D46 contains 27 insertions in four files and no deletions.

Local source identity:

- branch: `il2-d46-allocator-only`;
- commit: `1d5049e4`;
- package: `build/vkd3d-proton-il2-d46-allocator-only-1d5049e4`.

Both MinGW architectures build successfully. Packaged DLL hashes are:

| Architecture | File | SHA-256 |
| --- | --- | --- |
| x86-64 | `d3d12.dll` | `7332401f09d2e040c56f75e9672246e3d0e174db0b55ee1f6dcf7cfac1a0edf6` |
| x86-64 | `d3d12core.dll` | `0158b0366a355eae4ff168e578a319c0ee284f262c00e4c1c7ca7a0575ba9593` |
| x86 | `d3d12.dll` | `42f4c985390bc343b301ed2548af953835ae58cbf32582e37d32e93e7aa6e1b6` |
| x86 | `d3d12core.dll` | `e510b501338a46aed6f70d26a4215cadff8652650931b7620065075190e48208` |

The isolated custom tool `IL2-Korea-D46-AllocatorOnly-1d5049e4` was created
from `IL2-Korea-D42-Complete-2d9a7467` while Steam was fully stopped. All four
installed DLL hashes match the package table. The source tool, D45 tool, game
prefix, official Proton installation, and launch options were not modified.

## Runtime gate

Run the isolated custom Proton tool with empty Steam launch options and no
RenderDoc or shader override. It uses the same NUMA-capable Wine base as D45.

Pass requires:

- game starts normally;
- no large square blocks in menu, cockpit, or fire-lit scenes;
- no broad light flicker;
- real lighting and shadows remain.

Fine sandy or film-grain lighting is normal on Windows and is not a failure.

The runtime result is recorded in
[`evidence-d46-allocator-only-result.md`](evidence-d46-allocator-only-result.md).
