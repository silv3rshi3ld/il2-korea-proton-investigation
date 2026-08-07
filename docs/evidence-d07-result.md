# D07 full terrain-page reinterpret result

## Result

D07 is a valid causal run and repairs the reproduced terrain corruption. The
two supplied screenshots were captured at 5,491 m and 5,501 m, where the
baseline showed its most severe loss. They show continuous distant terrain
instead of isolated rectangular pages, black or hollow regions, and magenta
page edges. The user classified the result as “Look great!”.

This run establishes the terrain root cause with high confidence. A second D07
run repeated the terrain repair at 5,483 m, while explicitly showing that the
main-menu aircraft blocks and shimmering remain. Clean general-build D08 later
validated the same repair without the game-specific diagnostic gate.

## Instrumentation validity

- VKD3D-Proton base: `84c87c8390d9df75ba41d911496296fe13f0e275`
- Diagnostic commit: `833cafa0b1bc87153b2e9d2859c6830f4553f80e`
- Gate: `VKD3D_IL2_BC3_PAGE_COPY=1`
- Enable markers: 1
- Candidates: 522
- Adjustments: 522
- Rejections: 0
- Adjustment-cap markers: 0
- Destination resources: 4

Every candidate was a footprint-only `DXGI_FORMAT_R32G32B32A32_UINT`
source copied to a `DXGI_FORMAT_BC3_UNORM` image:

| Source extent | Vulkan destination extent | Count |
|---|---|---:|
| `64x64x1` | `256x256x1` | 178 |
| `64x1x1` | `256x4x1` | 182 |
| `1x64x1` | `4x256x1` | 162 |

Both formats use one 16-byte physical element. D3D12 describes the placed
buffer footprint in source-format texels, but Vulkan's buffer-image copy
geometry is expressed in destination image texels. The ordinary
`vk_buffer_image_copy_from_d3d12()` path retained the source dimensions and
row geometry. D07 converted each 128-bit source element into one 4x4 BC3 block
and expressed `imageExtent`, `bufferRowLength`, and `bufferImageHeight` in BC3
destination texels.

## Causal controls

- The repair occurs near 5,500 m, excluding the earlier low-altitude visual
  confound.
- The successful run still contains 40,408 split `END_ONLY` warnings, so those
  warnings are not required for the terrain failure.
- The game's `tex.log` still records missing Korea summer and common-fallback
  inputs, yet the terrain renders. Those missing files are not the primary
  cause of the rectangular terrain loss.
- No Vulkan device loss, GPU reset/hang, or out-of-memory signature appears.

## Screenshots

The local evidence copies are intentionally ignored by Git until a small,
reviewed public bundle is prepared:

| File | Altitude | SHA-256 |
|---|---:|---|
| `D07-r1-terrain-repaired-forward-view-5491m.png` | 5,491 m | `657b9fd35f094441d7c5f28a266adc41d5cb4580c8b909607aeea5f3330e2202` |
| `D07-r1-terrain-repaired-right-view-5501m.png` | 5,501 m | `65b7ea14fdbaf6d3249eb42c022d487a119742d9539835a7dffc0bb6d63aaaa3` |

## Repeat run

D07-r2 loaded the same diagnostic commit and recorded 304/304 adjustments,
zero rejects, four destination resources, and no adjustment-log cap. It again
showed continuous terrain at 5,483 m without black/hollow pages. A separate
main-menu capture shows that block artifacts remain on the aircraft, and the
user reports that shimmering remains. Terrain fidelity is deliberately marked
inconclusive because FSR may be enabled.

| File | Classification | SHA-256 |
|---|---|---|
| `D07-r2-terrain-repaired-cockpit-5483m.png` | terrain fixed at 5,483 m | `07b8cc0f4940314391c6f4e2e8366fa5dade8aafe9b9c6120f99078864eb002d` |
| `D07-r2-menu-aircraft-blocks-shimmering-persists.png` | menu unchanged | `27682dee6c3eb14d367f041421e9d8d2a2f2efff79f3ce63f7bfcf6c8d9517b7` |

The D07-r2 raw log was accidentally written to the D07-r1 short path. It was
moved intact and collected under `captures/runs/D07-page-copy-r2`; the original
D07-r1 compressed log and derived evidence were already preserved.

The bounded machine-generated analysis is retained locally at
`captures/runs/D07-page-copy-r1/bc3-border-copy-analysis.md`. The raw 14 MiB
Proton log and copyrighted game data are not committed.

## Follow-up

Clean general-build D08 completed without `VKD3D_IL2_BC3_PAGE_COPY` and
repeated the terrain repair. See [`evidence-d08-result.md`](evidence-d08-result.md).
