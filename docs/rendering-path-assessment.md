# Terrain rendering-path assessment

## Current working model

The game implements terrain tiling above the D3D12 API rather than with D3D12
reserved/sparse resources. The most likely data flow is:

```text
map archives / BlocksCache threads
        -> CPU loading or decompression
        -> UpdateSubresource / CopyTextureRegion
        -> ordinary upload buffer
        -> separately ranged placed BC3 texture blocks with mip chains
        -> SRV in a shader-visible descriptor heap
        -> terrain shader chooses a block and mip/LOD from camera distance
        -> the existing terrain mesh samples that surface page
```

The compiled class and method names support this model, and runtime traces
directly verify the D3D12 operations from `CopyTextureRegion` onward. The exact
archive format, CPU decoder, and shader selection formula remain inferred; no
game archive was unpacked and no asset was copied.

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

The menu aircraft also shows rectangular corruption. That makes damaged map
files an unlikely common cause and suggests a rendering-resource problem shared
by terrain and menu effects, such as descriptor contents, resource visibility,
or translated shader access. The precise menu resource has not been identified.

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

1. **Texture-provider lookup, decode, or creation failure.** D04's game-owned
   `tex.log` names six failed Korea winter terrain inputs, and the backend
   substitutes `defWhite.bmp` after the provider returns failure. A native
   Windows `tex.log` comparison is required because these may be optional
   inputs which also fail on the correctly rendered platform.
2. **Descriptor contents, index, or lifetime common to both backends.** A
   shader may read an uninitialized, stale, destroyed, out-of-range, or wrong-
   type descriptor. Disabling descriptor buffers would not necessarily fix
   invalid D3D12 descriptor use because the fallback backend preserves the
   application's descriptor semantics.
3. **Copy-to-sample or UAV dependency.** The resource may have correct bytes
   but become visible to a draw without the D3D12 dependency needed for VKD3D
   to emit the corresponding Vulkan synchronization. A native Windows driver
   may serialize or tolerate the sequence.
4. **Translated descriptor/LOD shader calculation.** The game or translated
   shader may choose the wrong page or mip index, especially in distant-LOD
   paths. The zero SRV minimum clamp does not test explicit shader LOD or
   descriptor-index arithmetic.
5. **RADV handling of otherwise valid Vulkan.** This remains possible but
   requires a driver comparison, validation finding, or minimal Vulkan
   reproduction before Mesa attribution.

The D02 no-incoming-copy SRV class is relevant but not yet identified as the
visible terrain. An SRV creation event does not prove that a draw bound or
sampled it, and a later copy after telemetry suppression cannot be excluded.

## Next discriminator

D04 has completed unchanged, removing the broad “already fixed upstream”
possibility. Before descriptor QA, compare the current Linux `tex.log` with a
native Windows run of the same Korea winter free-flight scenario and game
build. This is narrower and cheaper than another Proton option:

- Linux-only failed terrain inputs select the game's package
  lookup/decode/backend-create boundary under Wine.
- The same failures on correctly rendered Windows make them non-causal and
  return the investigation to VKD3D-Proton's existing GPU-assisted descriptor
  QA. That QA can report heap out-of-range access, descriptor-type mismatch,
  and access to a destroyed resource with shader hash, instruction number,
  heap cookie, resource/view cookie, and descriptor index.

- A repeatable descriptor-QA fault correlated with the corrupted scene selects
  descriptor propagation, lifetime, or invalid game usage.
- A clean descriptor-QA run moves the investigation to a filtered
  copy/barrier/draw-use trace for the terrain resource class and explicit shader
  LOD/index analysis.
- Only after VKD3D's Vulkan commands are shown valid should the investigation
  attribute the result to RADV or prepare a Mesa reproducer.

No current evidence justifies a game-specific application override.

See [`prior-art-msfs.md`](prior-art-msfs.md) for the exact upstream cases and
why the MSFS host-import fallback is not selected for IL-2. See
[`evidence-d04-upstream-result.md`](evidence-d04-upstream-result.md) for the
closed upstream control and texture-provider evidence.
