# D33 t7/t8 descriptor-contract trace: preparation

## Why the plan changed

D32 still establishes the useful causal boundary: evaluating genuine light ID
2 is sufficient for the square artifact in the controlled ID-1/ID-2 mixture.
The initial D33 hypothesis was that a 240-byte, 15-element typed-buffer view
might be the pixel shaders' `t7` per-light record buffer, which would put ID 2
out of bounds.

Offline correlation of both retained D20 RenderDoc frames rejects that register
assignment. The 240-byte `VK_FORMAT_R32G32B32A32_UINT` descriptor appears at
heap slot 13,796 in both frames. D16 established table base 13,788, `t9` at
13,797, and `t10` at 13,798. The 240-byte view is therefore at the adjacent
`t8` position, not `t7`. It contains 15 declared `uint4` elements; the captured
backing bytes after the view are zero, but that fact must not be used as
evidence of an out-of-bounds `t7` access.

Static SPIR-V inspection adds a more useful discrepancy to resolve:

- offset/register 7 is loaded as a float texel buffer and supplies the
  eight-float4-stride per-light record fetches;
- offset/register 8 is loaded as a sampled 2D float image;
- the raw captured descriptor heap makes slot 13,795 look image-like and slot
  13,796 look buffer-like; and
- `t9` and `t10` remain aligned with their already confirmed 3D-grid and index
  buffer descriptors.

This looks like an adjacent `t7`/`t8` reversal, but it is not yet a conclusion.
Nontrivial root-signature ranges/offsets or raw mutable-descriptor encoding can
explain the apparent mismatch. The live D3D descriptor sidecar and exact root
range contract are authoritative.

## Local implementation state

D33 is based on the proven D16 diagnostic commit
`274f6f8e2d5b785fa871cedb0e3267e6a2af9abf`. Local diagnostic commit
`1800206168f9d43f6e0bc82a6b714e785bd6a9f8`:

- resolves `t7`, `t8`, `t9`, and `t10` only for pixel hashes
  `df0bd777fd1bb89d` and `a2d104d5c813322e`;
- records the original root-signature version;
- logs every effective descriptor range's type, register span, register space,
  descriptor count, D3D12 range flags, and table offset; and
- changes no shader, descriptor, resource, barrier, render command, or game
  setting.

The exact source is retained as
`patches/0015-vkd3d-Trace-IL-2-t7-and-t8-descriptor-contracts.patch` and in the
ignored worktree `src/vkd3d-proton-d33` on branch
`il2-d33-t7-t8-contract`.

Both x86-64 and x86 were cleaned and rebuilt after the local commit. Both now
embed the unique D33 build ID `0x1800206168f9d43`; the earlier parent-identified
outputs were overwritten and will not be used. The final DLL checks are:

| DLL | Architecture | SHA-256 |
| --- | --- | --- |
| `x64/d3d12.dll` | PE32+ x86-64 | `c00da7146a4c783c756f52fb2638c7225a9106d93ee06624961ada055889b418` |
| `x64/d3d12core.dll` | PE32+ x86-64 | `aa6a14715c11b337e8e967cf6afe28a4e7619f94e238b1cdcd6e6340622465e4` |
| `x86/d3d12.dll` | PE32 i386 | `3727bc6e9b58a728c9cf13ab8651298438b8d2edf11af2c3a60b2f4b11ea6048` |
| `x86/d3d12core.dll` | PE32 i386 | `618f5c02abf079631b73cca5ce59ba4a45f8a6a5a0aabf81eb0b17cc65c246eb` |

The trace markers are present in both `d3d12core.dll` architectures and the
`d3d12.dll` export check passes. Meson defines no test suite for either build;
the next meaningful validation is the passive runtime trace.

## Exact continuation point

The separately named custom tool
`IL2-Korea-D33-T7T8Contract-18002061` was created from
`IL2-Korea-D16-DescriptorTrace-274f6f8e` at
`2026-08-09T16:36:08+00:00`. Integrity comparison shows that only the four
packaged VKD3D-Proton DLLs differ from D16, aside from the new tool registration
and diagnostic metadata. The installed DLL hashes match the verified build
hashes above exactly. D16, D25, and the game prefix were not modified.

1. Restart Steam and select `IL2-Korea-D33-T7T8Contract-18002061` for AppID
   247970.
2. Run one passive menu reproduction with
   `VKD3D_IL2_DESCRIPTOR_TRACE=1`, no shader override, and no other graphics
   diagnostic.

```text
PROTON_LOG=1 PROTON_LOG_DIR=/tmp/il2-D33-t7-t8-contract-r1 VKD3D_IL2_DESCRIPTOR_TRACE=1 %command%
```

3. Accept the run only if the log proves the D33 build, contains `IL2ROOT`, and
   resolves all four registers. Then compare the live `t7`/`t8` descriptor
   types, cookies, view dimensions/formats, buffer ranges, and root-range
   offsets.

The run is complete. Its accepted evidence and interpretation are recorded in
[`evidence-d33-t7-t8-contract-result.md`](evidence-d33-t7-t8-contract-result.md).
No game/prefix content or Steam configuration was changed, and nothing was
uploaded or posted.
