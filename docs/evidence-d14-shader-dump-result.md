# D14 exact light-list shader dump: result

## Result

D14 collected the original application DXIL and VKD3D-Proton SPIR-V for every
target shader. The run was visually unchanged, as expected for passive shader
collection. Static inspection narrows the square artifact from reflection in
general to the game's tiled dynamic-light list, but it does not yet prove a
fault or provide a fix.

The translated SPIR-V preserves the original resource dimensions, `8x8` thread
groups, bounds tests, unsigned integer packing, and atomics for the relevant
stages. All inspected modules pass `spirv-val`. There is therefore no obvious
structural DXIL-to-SPIR-V mistranslation in this sequence.

## Run identity

- Run: `D14-shader-dump-r1`
- Game build: `24615759`
- Proton: `1786112352 experimental-11.0-20260724c-wine-mr11604-d10`
- VKD3D-Proton: `395d974767f56f1`
- Mesa/RADV: `26.1.6`
- Kernel: `7.1.6`
- Source log: 88,802,520 bytes, SHA-256
  `f7e81840942bee41312f95ad335fffecf3abbb57c197d8557d37c170f876c3e8`
- Captured files: 1,360, totaling 10,849,628 bytes
- Launch options:
  `PROTON_LOG=1 PROTON_LOG_DIR=/tmp/il2-D14-shader-dump-r1 VKD3D_SHADER_DUMP_PATH=/tmp/il2-D14-shader-dump-r1/shaders %command%`

The shader binaries remain ignored local application data and are not part of
the repository.

## Exact stages

DXIL names and runtime hashes identify six light-list compute stages:

| Hash | DXIL entry point | Group size | Role |
|---|---|---:|---|
| `ce5553a11c1e3c3d` | `SetLightIndex` | 8x8x1 | initializes the per-tile light-index state |
| `e41c75bf472dc42b` | `ComputeDepthRange` | 8x8x1 | computes tile depth ranges |
| `14096b77d9f7cb60` | `ClearLightRefs` | 8x8x1 | clears the two-layer light-reference grid |
| `651194bd0a21772e` | `ComputeLightsCount` | 8x8x8 | counts lights affecting each tile |
| `7cefa1bc80bb4c70` | `ComputeLightsFirstRef` | 8x8x1 | allocates each tile's range in the index buffer |
| `11e32439a86036ba` | `ComputeLightsIndices` | 8x8x8 | writes the light IDs for each tile |

D13's bounded four-dispatch window captured the first four stages but missed
the last two. Those final stages are important because `ComputeLightsFirstRef`
packs a start offset and count into the `R32_UINT` light-reference grid, while
`ComputeLightsIndices` populates a separate uint light-index buffer.

The correlated graphics stages are:

| Hash | DXIL entry point | Stage |
|---|---|---|
| `7ab2e6bc4f546a86` | `VertOut` | vertex |
| `df0bd777fd1bb89d` | `PixOutLight_msp` | pixel |
| `a2d104d5c813322e` | `PixOutLight_mss` | pixel |

Both pixel shaders statically consume a 3D uint light-reference SRV and the
separate uint light-index buffer. They derive tile coordinates from pixel
coordinates, use the low ten packed bits as the light count and the high bits
as the first index, and then read the light IDs. Incorrect data at this point
can therefore affect a complete screen tile. That mechanism fits the observed
approximately square blocks better than a generic reflection-only theory.

## Coverage and translation checks

`ClearLightRefs` is dispatched as `10x5x2`; with its `8x8x1` group size and
explicit `80x34` bounds, it covers the complete `80x34x2` grid. The count and
index stages use the same tile envelope and bound their third coordinate by
the active light count. Static comparison found no disagreement between DXIL
and SPIR-V in:

- 3D `R32_UINT` image shape and coordinates;
- group sizes and bounds checks;
- low/high-bit packing of count and start offsets;
- atomic allocation and increment operations; or
- the pixel shaders' integer unpacking and index-buffer reads.

Passing validation is not proof of correct runtime descriptors or memory
visibility. It only removes the most obvious translator-shape errors.

## Remaining synchronization boundary

D13 shows UAV barriers after the count stage and before the final transition of
`rtLightRefs` to shader-read state. However, it did not identify the two final
compute hashes or the separate light-index buffer. The `rtLightRefs` transition
can make the image writes visible, but it does not by itself prove that the
separate buffer written by `ComputeLightsIndices` has the required dependency
before the pixel shaders consume it.

D15 therefore adds gated, passive telemetry for the two post-count dispatches
and every immediately following legacy resource barrier, including unnamed
buffers. A forced barrier is not justified until that trace establishes the
actual dependency sequence.

## Decision

Reflection resources remain part of the frame graph, but the strongest lead is
now the tiled lighting data consumed by the reflection/light pixel passes. D14
does not establish a fix. The next valid discriminator is D15's exact
post-count buffer/barrier trace, followed by a narrowly scoped synchronization
A/B only if a missing dependency is demonstrated.
