# Terrain rendering-path assessment

## Current working model

The game implements terrain tiling above the D3D12 API rather than with D3D12
reserved/sparse resources. The most likely data flow is:

```text
map archives / BlocksCache threads
        -> CPU loading or decompression
        -> UpdateSubresource / CopyTextureRegion
        -> ordinary upload buffer
        -> placed BC3 texture blocks with mip chains and 2048x2048 baked caches
        -> 64x64 RGBA32_UINT physical-block pages plus thin borders
        -> buffer-to-image reinterpret copy into 256x256 BC3 pages
        -> SRV in a shader-visible descriptor heap
        -> terrain shader chooses a block and mip/LOD from camera distance
        -> the existing terrain mesh samples that surface page
```

The compiled class and method names support this model, and runtime traces
directly verify the D3D12 operations from `CopyTextureRegion` onward. A later
read-only package inspection confirms five LODs and 800 m texture quads. The
CPU decoder and shader selection formula remain unknown; no extracted game
asset is retained in this repository.

## What the captures show

Terrain geometry continues to exist: mountain silhouettes, depth relationships,
clouds, UI, and some vegetation render while large surface regions are dark or
absent. Isolated rectangular textured regions and magenta page edges follow the
visual shape expected from the engine's terrain blocks, not arbitrary missing
triangles.

At high altitude the distant terrain-page set dominates and almost all surface
content disappears. Below roughly 1,500 m, a different local-detail set becomes
eligible and additional pages and trees appear. The low-altitude scene is still
wrong, so altitude changes resource selection and severity rather than fixing
the mechanism.

In D07, converting the complete interior/border copy family produces
continuous terrain at approximately 5,500 m. The former altitude dependency
therefore reflected how much of the under-populated baked cache was visible,
not a separate mip-residency failure.

The menu aircraft shows rectangular corruption in both D07-r2 and clean D08
while the terrain remains repaired. The user also confirmed that menu
shimmering persisted. This established a separate symptom, not another
manifestation fixed by the baked-terrain copy conversion. D50 through D52
later isolated that separate track at the texel-buffer view boundary.

## Proven exclusions and weakened explanations

| Mechanism | Evidence | Assessment |
|---|---|---|
| D3D12 reserved/sparse resources | D01b records zero reserved-resource, tiling-query, tile-map, or tile-copy calls | Excluded for the reproduced path |
| Broad incomplete mip uploads | D02 finds 2,355 geometrically complete multi-mip compressed uploads and zero partial resources | Weakened; bytes and visibility are not proven |
| Non-zero SRV minimum-LOD clamp | All 4,185 D02 SRVs use clamp zero | Excluded in D02 |
| Simple use after resource destruction | No logged SRV/copy follows destruction | Weakened; descriptor lifetime still untested |
| Placed-resource memory aliasing | D03 matches all 585 pre-cap candidates; zero range overlap and zero explicit legacy alias barriers | Excluded for the covered class |
| Ordinary async queue selection | Two `single_queue` runs are visually unchanged | Unlikely as the primary trigger |
| `VK_EXT_descriptor_buffer` backend alone | E03-r1 definitely disables it and uses the mutable-descriptor fallback, with unchanged corruption | Weakened; stock-Proton confirmation remains |
| Device loss, OOM, or global upload exhaustion | No corresponding runtime signature | Not supported |

The split `END_ONLY` warnings remain observations. Their count follows run
duration, and no warning has been tied to a failing texture, barrier state, or
draw. They are not yet a root-cause explanation.

## Demonstrated terrain mechanism

D07 proves the first prior hypothesis. All 522 matching copies use
`R32G32B32A32_UINT` placed footprints and BC3 destinations with equal 16-byte
physical elements. VKD3D-Proton expressed their Vulkan geometry in source
texels. Converting through physical blocks changed:

- `64x64` interiors to `256x256` BC3 texels;
- `64x1` borders to `256x4` BC3 texels; and
- `1x64` borders to `4x256` BC3 texels.

The same mission then renders continuous high-altitude terrain. The driver,
queues, descriptor-buffer path, missing-file fallbacks, and split-barrier
warnings were otherwise unchanged. The defect is therefore in copy geometry,
not sparse residency, page selection, or RADV for this terrain path.

The D02 no-incoming-copy SRV class is relevant but not yet identified as the
visible terrain. An SRV creation event does not prove that a draw bound or
sampled it, and a later copy after telemetry suppression cannot be excluded.

## Historical next discriminator

D08 validates the general block-compatible buffer-to-image conversion at
predecessor commit `cf11ba76` without the D07 gate. Historical review commit
`64ec55e7` narrowed activation while preserving the selected IL-2 conversion;
the final revision merged through PR #3202 as `731c4aae`. The terrain track no
longer needs more configuration-flag testing. The subsequent lighting work is
preserved in [`evidence-d50-d52-r32-alias-result.md`](evidence-d50-d52-r32-alias-result.md),
which supersedes the earlier open-ended discriminator described here.

No game-specific application override is justified because the failing
behavior is in the general buffer-image conversion helper.

See [`prior-art-msfs.md`](prior-art-msfs.md) for the exact upstream cases and
why the MSFS host-import fallback is not selected for IL-2. See
[`evidence-d04-upstream-result.md`](evidence-d04-upstream-result.md) for the
closed upstream control and texture-provider evidence. See
[`evidence-d05c-result.md`](evidence-d05c-result.md),
[`evidence-d06-result.md`](evidence-d06-result.md), and
[`evidence-map-package-inspection.md`](evidence-map-package-inspection.md) for
the copy evidence and verified map geometry.
