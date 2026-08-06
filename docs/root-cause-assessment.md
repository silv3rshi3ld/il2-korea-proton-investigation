# Root-cause assessment

## Current conclusion

The evidence no longer supports treating all visual symptoms as one generic
Proton rendering failure. The most likely model is:

1. a demonstrated compressed-copy compatibility problem in the baked-terrain
   cache, with high confidence for the magenta page seams;
2. a possibly separate baked-terrain page visibility/selection defect for the
   large black or absent regions;
3. a separate menu effect, shadow, or temporal-resource defect producing
   screen-space blocks over an otherwise textured aircraft.

The startup/OpenMP problem remains an independent Wine/NUMA investigation.

## Ranked graphics assessment

| Rank | Mechanism | Evidence | Confidence |
|---:|---|---|---|
| 1 | Invalid one-texel BC3 border copies in the game-owned baked-terrain cache, forwarded unchanged by VKD3D as invalid Vulkan regions | 432 active internal regions; only four border shapes; 2048x2048 BC3 destinations; `BakedTerrain` and `stitchBorders` engine symbols; direct VKD3D source path | High for seam corruption; medium-low for whole-page loss |
| 2 | Wrong or non-visible baked-cache page at draw time: descriptor index/lifetime, copy-to-sample dependency, or explicit LOD/page-index translation | Terrain mesh remains; valid interior page uploads exist; failure is distance-dependent; descriptor-backend and queue controls are unchanged | Medium for the remaining black pages, low for a specific sub-mechanism |
| 3 | Invalid application synchronization tolerated by native Windows | Native rendering succeeds; single queue does not fix in-queue dependency errors; prior VKD3D fixes show this class exists | Medium-low until a resource/draw sequence is captured |
| 4 | RADV defect on valid Vulkan | Same symptom occurs on two RDNA generations, but the concrete border path already emits invalid Vulkan | Low for the border failure; unresolved for any later valid-command page failure |
| 5 | Linux-only package lookup/decode failure | Six Korea material inputs fall back to `defWhite`, but package enumeration succeeds and rectangular active caches still receive uploads | Low-medium as a secondary contributor |
| 6 | General shader-translation version bug | Unmodified current upstream remains unchanged | Low for an already-fixed general bug; individual shader behavior remains possible |

## Causal terrain model

```text
map packages
  -> BlocksCache / BakedTerrain CPU work
  -> 64x64 or 128x128 compressed page data
  -> placed 2048x2048 BC3 cache textures
  -> one-texel stitchBorders copies
  -> SRV and distant/local LOD selection
  -> existing terrain mesh samples the selected page
```

This explains the altitude effect without invoking D3D12 sparse residency.
High altitude selects predominantly distant baked pages, exposing much more
empty or wrong cache content. Below roughly 1,500 m, local-detail pages and
vegetation become eligible, so more content appears without curing the cache
defect.

## Concrete semantic defect

BC3 stores 4x4 texels per physical block. D02 records 432 internal border
uploads with one dimension equal to one texel. They do not terminate at the mip
edge. Current VKD3D calculates the Vulkan image extent from that D3D12 source
box without block normalization, then submits it through
`vkCmdCopyBufferToImage2`.

Under Vulkan, this violates compressed-image transfer granularity. Under the
VKD3D-Proton cross-test's D3D12 interpretation, the original non-block-sized
compressed copy is invalid application usage as well. The probable ownership
is therefore:

- **game:** originates invalid or at least non-portable BC3 region parameters;
- **native D3D12/driver:** tolerates the operation sufficiently for correct
  Windows rendering;
- **VKD3D-Proton:** currently exposes the invalid operation directly to Vulkan
  rather than providing that compatibility behavior;
- **RADV:** receives an invalid transfer, so its result cannot yet be called a
  driver defect.

## Why earlier leads are no longer primary

- D3D12 reserved/tiled resources: zero relevant API calls.
- Separate compute/transfer queues: two `single_queue` runs are unchanged.
- Descriptor buffers: disabling the extension is visually unchanged.
- Placed-resource overlap: no overlap in the covered D03 class.
- Broad missing mip chains or SRV minimum LOD: D02 finds complete chains and
  minimum LOD zero for the covered resources.
- Split `END_ONLY` warnings: VKD3D handles them conservatively, with no
  resource correlation.
- Current-upstream shader translator: D04 is unchanged.
- Archive failure: `packman.log` enumerates the packages without a reported
  open or decode error.

## Decision

Do not run more generic configuration flags and do not add an application
override yet. The one justified experiment is a gated VKD3D build that expands
only these observed one-texel BC3 border regions to their complete 4-texel
physical block and logs every adjustment.

That one run has real information value:

- if seams and pages improve, the invalid border transfer affects the larger
  cache;
- if only seams improve, we have one confirmed fix and can trace the remaining
  2048x2048 cache page at descriptor/copy-to-sample time;
- if nothing changes, the game still has an upstream-reportable invalid-copy
  defect, but it is not the visible root cause.

No native Windows machine is needed for this behavioral discriminator. A later
Windows D3D12 debug-layer run would strengthen the final game-versus-VKD3D
ownership decision.
