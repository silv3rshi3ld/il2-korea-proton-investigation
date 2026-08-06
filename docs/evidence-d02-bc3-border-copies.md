# D02 BC3 terrain-cache border-copy finding

## Result

D02 contains one concrete API-semantics problem on the terrain path. The game
uploads narrow border regions into single-mip, 2048x2048 BC3 texture caches.
There are 432 buffer-to-image copies whose internal extent is not compatible
with BC3's 4x4 physical block size:

| Extent | Regions |
|---|---:|
| 128x1 | 118 |
| 1x128 | 112 |
| 64x1 | 110 |
| 1x64 | 92 |

All 432 target the same resource class: placed 2048x2048, one-mip,
`DXGI_FORMAT_BC3_UNORM` textures. The D02 run creates 164 members of that
class. Fourteen members receive 382 of the narrow border uploads before the
screenshot at approximately 35,526.95 seconds after boot. By 35,531.033, the
total reaches 432 regions across sixteen members; the trace cap follows at
35,532.815. The same resources also receive observed 64x64 or 128x128 interior
page uploads. These operations are therefore on the active, visibly corrupted
mission timeline rather than startup noise.

Representative converted copies are:

```text
extent=1x128x1 image_offset=1020,1536,0
extent=128x1x1 image_offset=1024,1536,0
extent=1x64x1 image_offset=252,1792,0
extent=64x1x1 image_offset=1280,1792,0
```

The offsets are 4-texel aligned, but the one-texel dimension does not end at
the 2048x2048 subresource edge. These are not the valid exception for a
complete terminal mip smaller than a compressed block.

The result is reproducible from the retained D02 log with:

```bash
./scripts/analyze-texture-trace.py \
  captures/runs/D02-r1/filtered.log \
  --output captures/runs/D02-r1/texture-trace-analysis.md
```

## Why these are terrain-cache operations

Read-only inspection of `landscape.dll` exposes the engine's own names:

```text
BakedTerrainCache
BakedTerrain
CBakedTerrain::rtTemp
m_atTextures[%i][%i]
g_tTiles
BlocksCache
Tex_BlocksLOD_%i
CDistantLOD
DetailedDistantLOD
ForceDistantLOD
renderTexture
renderNormalsTexture
stitchBorders
getChunkDiffuseCacheOnPoint
```

Together with the trace, these names support the following rendering path:

```text
map packages
  -> BlocksCache / BakedTerrain
  -> 64x64 or 128x128 BC3 page data
  -> 2048x2048 placed BC3 cache textures
  -> one-texel stitchBorders updates
  -> SRV / terrain LOD shader
```

This is ordinary application-managed texture caching. D01b already excludes
D3D12 reserved/tiled-resource APIs from the reproduced path.

## Translation and validity analysis

Current VKD3D-Proton's `vk_buffer_image_copy_from_d3d12()` uses the D3D12
source-box width and height directly for `VkBufferImageCopy2.imageExtent`.
`d3d12_command_list_copy_texture_region()` later passes that region to
`vkCmdCopyBufferToImage2()` without rounding the compressed extent. The same
code exists in both the installed source base and tested upstream commit
`84c87c83`.

Vulkan image transfers scale the queue transfer granularity by the compressed
texel-block dimensions. For BC3, an internal buffer-to-image region must use
4x4-aligned offsets and extents, except that a non-multiple extent may terminate
at the edge of the mip. The 432 regions above violate that condition and should
trigger `VUID-vkCmdCopyBufferToImage2-imageOffset-07738` under validation.

VKD3D-Proton's own `d3d12_invalid_usage` cross-test classifies non-block-sized
compressed-copy coordinates as invalid D3D12 usage and expects the native
runtime/debug validation path to reject them. This makes the current ownership
assessment:

- likely origin: invalid BC3 `CopyTextureRegion` parameters issued by the game;
- compatibility gap: Windows/native AMD execution apparently tolerates or
  expands the physical block while VKD3D emits an invalid Vulkan region;
- not a demonstrated RADV bug: RADV is not required to give defined results for
  an invalid Vulkan transfer.

Native Windows is still useful later for a D3D12 debug-layer capture, but is
not required to demonstrate that the emitted Vulkan regions violate Vulkan's
block-copy rule.

## What this does and does not explain

Confidence is **high** that this mechanism can explain the magenta lines and
corrupted borders between terrain pages. The visual seams, the one-texel
`stitchBorders` pattern, the BC3 cache type, and the invalid converted regions
all agree.

Confidence is **medium-low** that it explains every black or absent terrain
page. The 64x64 and 128x128 interior uploads are block-valid. Invalid Vulkan
commands can have undefined effects beyond the intended border, but D02 does
not prove that these commands cause the entire page cache to become invisible.
If normalizing the border copies removes only magenta seams, the broad page
loss remains a second defect in cache-page visibility, synchronization,
descriptor selection, or explicit LOD/indexing.

No corresponding terrain-cache link has been established for the menu blocks.
The menu aircraft base texture is visible beneath screen-space rectangular
artifacts, so the menu problem should be treated as a likely separate effect or
shadow/temporal-resource defect until resource correlation proves otherwise.

## One next behavioral discriminator

The next run should not test another generic VKD3D flag. Build one opt-in,
diagnostic-only VKD3D change which:

1. detects a buffer-to-BC-image copy with block-aligned offset, internal
   one-texel extent, and a source footprint containing the full physical block;
2. expands only the one-texel dimension to the 4-texel BC block;
3. logs the original and emitted regions plus a bounded counter;
4. remains disabled unless an investigation-specific environment variable is
   set.

One matched high-altitude mission run can then distinguish three outcomes:

- seams and page loss improve: the invalid border copy poisons more of the
  baked-terrain cache than the border alone;
- only seams improve: the BC3 issue is real but the missing-page defect is
  separate;
- no visual change: retain the validity finding for the game developer, but
  move broad page loss to descriptor/copy-to-sample correlation.

This is suitable as a diagnostic experiment, not yet as a permanent game
override or general upstream fix.
