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
        -> 64/128-texel page uploads plus one-texel border stitching
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

The menu aircraft also shows rectangular corruption, but the intact aircraft
texture is visible below a screen-space block pattern. The terrain now has a
specific BC3-cache operation which has not been tied to the menu. The safer
model is therefore two defects until a shared resource dependency is proven:
one in baked-terrain cache population/borders and another in a menu effect,
shadow, or temporal resource.

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

## Leading explanations

1. **Page population or copy-to-sample visibility.** The page producer may
   leave a cache page empty or issue work without the dependency needed before
   sampling. D05c corrected all encountered border reinterpret copies without
   changing the image, so this requires the full page path rather than another
   border-only change.
2. **Descriptor contents, index, or lifetime on the baked-terrain cache.** A
   shader may read an uninitialized, stale, destroyed, out-of-range, or wrong-
   type descriptor. Disabling descriptor buffers would not necessarily fix
   invalid D3D12 descriptor use because the fallback backend preserves the
   application's descriptor semantics.
3. **Translated descriptor/LOD shader calculation.** The game or translated
   shader may choose the wrong page or mip index, especially in distant-LOD
   paths. The zero SRV minimum clamp does not test explicit shader LOD or
   descriptor-index arithmetic.
4. **Texture-provider lookup or creation fallback.** The six tested autumn
   inputs really are absent from installed packages and the backend substitutes
   white. Nearly the same absent set is referenced by all seasons, so this may
   be a secondary or optional path rather than a Linux-only failure.
5. **RADV handling of otherwise valid Vulkan.** This remains possible but
   requires a driver comparison, validation finding, or minimal Vulkan
   reproduction before Mesa attribution.

The D02 no-incoming-copy SRV class is relevant but not yet identified as the
visible terrain. An SRV creation event does not prove that a draw bound or
sampled it, and a later copy after telemetry suppression cannot be excluded.

## Next discriminator

The next test is a trace-only D06 build, not another generic launch flag.
D05c's exact behavior executed on 202/202 candidates and the image remained
unchanged. D06 will therefore follow only the 2048x2048 BC3 cache pages and
associated producer intermediates through writes/copies, state and layout
transitions, descriptor creation/binding, first sampling, and destruction.

A native Windows `tex.log` and D3D12 debug-layer capture remain useful for
final ownership, but lack of current Windows access does not block this
producer-to-SRV discriminator.

No current evidence justifies a permanent game-specific application override.

See [`prior-art-msfs.md`](prior-art-msfs.md) for the exact upstream cases and
why the MSFS host-import fallback is not selected for IL-2. See
[`evidence-d04-upstream-result.md`](evidence-d04-upstream-result.md) for the
closed upstream control and texture-provider evidence. See
[`evidence-d05c-result.md`](evidence-d05c-result.md) and
[`evidence-map-package-inspection.md`](evidence-map-package-inspection.md) for
the completed causal copy test and the verified map geometry.
