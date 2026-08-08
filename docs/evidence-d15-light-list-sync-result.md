# D15 final light-list synchronization trace: result

## Result

D15 is visually unchanged and conclusively closes the proposed missing
dependency after the final tiled-light writer. The separate light-index buffer
receives the same explicit per-stage UAV dependencies and final
unordered-access-to-shader-read transition as the 3D light-reference grid.
Adding a forced VKD3D-Proton barrier would duplicate synchronization already
requested by the application and is not a justified fix.

The trace also confirms D14's static stage identification at runtime:
`ComputeLightsFirstRef` is followed by `ComputeLightsIndices` in every covered
cycle.

## Run identity

- Run: `D15-light-list-sync-r1`
- Game build: `24615759`
- Proton: `1786112352 experimental-11.0-20260724c-wine-mr11604-d10`
- VKD3D-Proton: `9c6a4338a2eff9f`
- GPU/driver: Radeon RX 7800 XT, RADV 26.1.6
- Kernel: `7.1.6-1-cachyos`
- Source log: 57,758,478 bytes, SHA-256
  `f2f7e943a9ec5b3384c2820288a7b70adc967a7ad7752ef9852d6a878deddb20`
- No OpenMP/topology override
- D3D12 and DXGI loaded; no D3D11 module loaded
- No reported device loss, OOM, GPU reset, or hang

The source log remains ignored and local.

## Visual classification

The user started the game, observed the affected menu, exited normally, and
explicitly confirmed that the artifacts/shimmering remained. D15 is passive,
so visual persistence is expected; its significance is that the trace and
symptom occurred in the same valid run.

## Trace coverage

The bounded trace recorded 200,000 events from Wine monotonic timestamp
`150.203` through `179.583`, approximately 29.4 seconds. It includes 1,594
command-list executions and 1,593 occurrences of each final stage:

| Stage | Hash | Dispatch | Count |
|---|---|---:|---:|
| `ComputeLightsFirstRef` | `7cefa1bc80bb4c70` | 10x5x1 | 1,593 |
| `ComputeLightsIndices` | `11e32439a86036ba` | 10x5x1 | 1,593 |

The two alternating graphics command lists show the same ordering.

## Exact resource sequence

The stable post-count sequence is:

1. UAV barrier on `rtLightRefs25`, cookie 4002, a `80x34x2 R32_UINT`
   texture;
2. UAV barrier on cookie 4001, an 87,040-byte UAV buffer;
3. `ComputeLightsFirstRef`;
4. UAV barrier on cookie 4002;
5. UAV barrier on cookie 4001;
6. `ComputeLightsIndices`;
7. transition of cookie 4002 from UAV state `0x8` to shader-read state `0xc0`;
8. transition of cookie 4001 from UAV state `0x8` to shader-read state `0xc0`.

Each target has exactly 3,186 post-count UAV-barrier records, two per covered
cycle. The bounded sixteen-barrier window directly records 1,580 final
transitions for each target. In the remaining thirteen cycles, a large
multi-subresource transition batch consumes that per-cycle logging window
before the two target entries; it does not indicate that the application
omitted them. The normal cycles, matching counts, and adjacent full-resource
transition batches establish the repeated dependency pattern.

The unnamed buffer size is not arbitrary. D16 later resolves its SRV as 43,520
`R16_UINT` elements, so 87,040 bytes equals
`80 * 34 * 16 * sizeof(uint16_t)`: sixteen light-index slots for every screen
tile. D14 identifies it statically as `g_bufLightsIndices`; the typed uint load
zero-extends each `R16_UINT` element for shader use.

## Atomic-scope check

The translated `ComputeLightsFirstRef` SPIR-V performs its shared allocation
counter `OpAtomicIAdd` with Vulkan scope value 1, which is Device scope, not
Workgroup scope. `ComputeLightsIndices` uses the same Device scope for its
per-tile atomic increment. The relaxed memory-semantics operand is compatible
with D3D interlocked arithmetic, which is atomic but does not itself imply a
memory fence; the explicit D3D12 UAV barriers and state transitions provide the
inter-dispatch dependencies.

This removes another plausible cross-workgroup failure mode.

## Shader binding boundary

D14 reflection metadata gives fixed bindings for the affected pixel shaders:

- `g_tLightsList`: SRV `t9`, a 3D uint texture;
- `g_bufLightsIndices`: SRV `t10`, a uint buffer.

They are fixed descriptor-table slots, not arbitrary application-selected
bindless indices. The next passive discriminator can therefore resolve those
two exact table entries at draws using pixel hashes `df0bd777fd1bb89d` and
`a2d104d5c813322e`, then map their metadata/view cookies back to D15 resources.

## Decision

The missing-synchronization form of G17 is closed for this sequence. Do not add
`VKD3D_SHADER_QUIRK_FORCE_PRE_COMPUTE_BARRIER`, a global barrier, or an IL-2
application override based on the square appearance.

The remaining focused possibilities are:

1. incorrect descriptor/view selection or metadata at SRV `t9`/`t10`;
2. incorrect values or indices produced despite correct resource ordering;
3. a subtler shader-translation or RADV compilation issue not detected by
   structural SPIR-V validation; or
4. a different tiled input in the same lighting pixel pass.

Resolve the two fixed descriptor slots first. If they map to the expected
resources and view types, capture/check the producer values rather than adding
speculative barriers.
