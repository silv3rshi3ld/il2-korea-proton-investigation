# D05c exact reinterpret-copy diagnostic: build ready

## State

D05c is compiled for both architectures and passes its synthetic log check.
Creation of the isolated custom Proton tool stopped safely because Steam was
still running. No Proton installation or game-prefix file was modified.

- Base VKD3D-Proton: `84c87c8390d9df75ba41d911496296fe13f0e275`
- Diagnostic commit: `5391ec7f427795fe0fc151047422629d849e35be`
- Build directory: `build/vkd3d-proton-il2-d05c-bc3-5391ec7f/`
- Retained diagnostic patch:
  `patches/0006-vkd3d-Test-RGBA32-to-BC3-buffer-copy-geometry-for-IL.patch`
- Source Proton Experimental: `experimental-11.0-20260724c`
- Intended custom tool: `IL2-Korea-D05c-Reinterpret-5391ec7f`

## Single changed behavior

Only when `VKD3D_IL2_BC3_BORDER_COPY=1` is set, D05c recognizes the already
observed combination of:

- footprint-only `R32G32B32A32_UINT` source;
- `2048x2048`, one-mip `BC3_UNORM` destination;
- source shape `128x1`, `64x1`, `1x128`, or `1x64`;
- block-aligned destination offset and depth one.

For that class, it maps each 128-bit source texel to one 4x4 BC3 block and
recomputes `imageExtent`, `bufferRowLength`, and `bufferImageHeight` in Vulkan
destination-format texels. Every other copy and every run without the gate is
unchanged.

## Build identity

| File | Architecture | SHA-256 |
|---|---|---|
| `x64/d3d12.dll` | PE32+ x86-64 | `9c05de19c472684f5a1910fd7f123fc0d1b4fad31ece2dd2b6ce3d41dff147d2` |
| `x64/d3d12core.dll` | PE32+ x86-64 | `bebc057635a65bf9071afb38a5df96a0e5e88e0e71f274338c67488b773f4cc4` |
| `x86/d3d12.dll` | PE32 i386 | `0a4166157ab5576ba1711d4a9a047bcb879b32508013b69febd5c5a19b798288` |
| `x86/d3d12core.dll` | PE32 i386 | `6b2033e2939d349af998c36af006b819ffadab10619836112c41b0d718eb5943` |

## Runtime protocol after installation

Run ID: `D05c-reinterpret-r1`

Select `IL2-Korea-D05c-Reinterpret-5391ec7f` and use exactly:

```bash
PROTON_LOG=1 PROTON_LOG_DIR=/tmp/il2-D05c-reinterpret-r1 OMP_NUM_THREADS=16 KMP_AFFINITY=disabled VKD3D_IL2_BC3_BORDER_COPY=1 %command%
```

Use the same mission and capture the terrain around 1,300-1,500 m. First judge
the missing rectangular pages and magenta edges; also note the menu aircraft.
One run is enough for the first causal discriminator. A valid run must contain
the D05c mode marker, at least one adjustment, and no target-class rejection.

Rollback remains selecting Proton Experimental. Do not delete a custom tool
while Steam is running.
