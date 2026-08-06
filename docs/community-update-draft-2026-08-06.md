# Community update drafts — review only

Nothing below has been posted. The two versions target the open reports in
[ValveSoftware/Proton #9906](https://github.com/ValveSoftware/Proton/issues/9906)
and
[VKD3D-Proton #3134](https://github.com/HansKristian-Work/vkd3d-proton/issues/3134).

## Draft for ValveSoftware/Proton #9906

```text
Controlled investigation update from a second AMD/RADV system

Environment:
- CachyOS / Wayland, kernel 7.1.6-1-cachyos
- Ryzen 7 7800X3D, Radeon RX 7800 XT
- Mesa/RADV 26.1.6
- game build 24596901
- Proton Experimental experimental-11.0-20260724c

The game still requires:
  OMP_NUM_THREADS=16 KMP_AFFINITY=disabled %command%

I am treating that as a startup mitigation, not a Wine/NUMA fix. The graphics
problem remains separate: moving rectangular blocks affect the menu aircraft,
and missions show mostly dark/absent terrain with isolated rectangular pages
and magenta edges. The loss is much worse at altitude; below roughly 1,500 m,
more low-detail terrain and vegetation appear.

Controlled results so far:
- two baseline runs reproduce the problem;
- VKD3D_CONFIG=single_queue is unchanged across two runs;
- VKD3D_CONFIG=no_upload_hvv changed the allocation path, but its apparent
  low-altitude improvement is altitude-confounded and therefore inconclusive;
- disabling VK_EXT_descriptor_buffer is visually unchanged;
- an unmodified current VKD3D-Proton build at 84c87c83, including 36 newer
  dxil-spirv commits than the installed build, is unchanged;
- traced mission startup makes zero D3D12 reserved-resource/tile-mapping calls;
- focused placed-resource tracing found no overlapping placed ranges in the
  covered class and no explicit legacy alias barriers.

The most concrete finding is in the application's baked-terrain cache. A valid
custom trace recorded 164 placed 2048x2048, one-mip BC3_UNORM cache textures.
It also recorded 432 internal buffer-to-image border copies with these extents:

- 118 x 128x1
- 112 x 1x128
- 110 x 64x1
- 92 x 1x64

The destination image offsets are four-texel aligned, but the one-texel
dimension does not end at the mip edge. Current VKD3D-Proton forwards that
dimension directly to VkBufferImageCopy2. For BC3's 4x4 physical blocks, the
resulting Vulkan regions violate the compressed-copy granularity rule. Static
inspection of the game's terrain module also exposes BakedTerrainCache,
BlocksCache, CDistantLOD, and stitchBorders names, matching the visible
magenta page seams.

Current assessment:
- high confidence that this invalid/non-portable border-copy behavior is
  relevant to the magenta seams;
- not yet proven to cause the large missing pages;
- no resource correlation yet for the menu blocks, which may be separate;
- likely invalid game D3D12 usage tolerated by native Windows, with VKD3D
  exposing it as invalid Vulkan—not demonstrated as a RADV bug.

The first opt-in normalization build loaded correctly but matched zero copies
because its source-side matcher was too strict. Its unchanged screenshot is
therefore an invalid test, not a failed fix. A footprint-aware revision has
been compiled with candidate and rejection telemetry but has not yet been run.

The repeated split END_ONLY warnings are present, but I have not treated them
as causal because VKD3D converts that path conservatively. I can provide a
small redacted handoff bundle containing the exact BC3 trace excerpt, analyzer
reports, build hashes, diagnostic patches, and compressed Proton logs.
```

## Draft for VKD3D-Proton #3134

```text
I have reproduced this independently on an RX 7800 XT with Mesa/RADV 26.1.6
and isolated one concrete invalid-copy pattern on the active terrain-cache
path. This does not yet explain every visual symptom, so I am not proposing an
application override as a fix.

Runtime/build controls:
- game build 24596901;
- Proton Experimental experimental-11.0-20260724c;
- installed VKD3D-Proton 3dfc6f07;
- unmodified current upstream 84c87c83 with dxil-spirv cc75a0c9 is visually
  unchanged;
- single_queue is unchanged across two runs;
- no_upload_hvv is altitude-confounded;
- disabling VK_EXT_descriptor_buffer is unchanged;
- zero D3D12 reserved-resource, GetResourceTiling, UpdateTileMappings, or
  CopyTiles calls occur on the reproduced mission path;
- no placed-resource range overlap was found in the covered D03 class.

A valid custom texture trace recorded 39,978 CopyTextureRegion calls. The game
creates 164 placed 2048x2048, one-mip DXGI_FORMAT_BC3_UNORM resources used as
an application-managed cache. The same active pool receives normal 64x64 or
128x128 interior uploads and 432 narrow border uploads:

- 118 x 128x1
- 112 x 1x128
- 110 x 64x1
- 92 x 1x64

Representative converted copies are extent=1x128 at offset=1020,1536 and
extent=128x1 at offset=1024,1536. All observed offsets are four-texel aligned,
but the thin dimension does not reach the 2048x2048 mip edge.

vk_buffer_image_copy_from_d3d12() currently uses the D3D12 source-box or
placed-footprint dimensions directly as VkBufferImageCopy2.imageExtent, and
the buffer-to-image path submits that region to vkCmdCopyBufferToImage2. BC3
has 4x4 compressed blocks, so these internal one-texel regions violate
VUID-vkCmdCopyBufferToImage2-imageOffset-07738. The vkd3d-proton
d3d12_invalid_usage tests also classify non-block-sized compressed-copy
coordinates as invalid D3D12 application usage.

Read-only inspection of landscape.dll contains BakedTerrainCache,
BakedTerrain, BlocksCache, CDistantLOD, DetailedDistantLOD, stitchBorders, and
getChunkDiffuseCacheOnPoint. Together with the trace and visible magenta page
edges, this gives high confidence that the one-texel operations are terrain
border stitching. It is only medium-low confidence for the much larger
black/absent pages because their interior uploads are block-valid. The menu
blocks remain uncorrelated and may be a separate issue.

I built an opt-in diagnostic which expands only this exact destination class
and these four thin shapes to a complete four-texel block. The first version
loaded correctly but logged zero adjustments because it required a non-null
source box/exact source format before logging. Its visually unchanged run is
inconclusive. A revised build now:

- accepts either an explicit source box or a footprint-only copy;
- validates matching 4x4/16-byte physical source blocks;
- logs every target candidate before adjustment;
- records buffer row/image-height capacity;
- emits an explicit rejection bitmask for unsafe candidates.

That revision is compiled but intentionally unrun while testing is paused. I
can attach a redacted ~3 MB handoff bundle with both Proton logs, the 432-copy
trace excerpt, analyzer reports, exact hashes, and the two diagnostic patches.
Any guidance on whether native D3D12 is expected to tolerate/expand this border
copy form would be especially useful before considering a narrow compatibility
quirk.
```

## Proposed attachments

The generated community handoff archive should contain:

- sanitized, compressed D02 and D05a Proton logs;
- a compact D02 excerpt containing the 2048x2048 BC3 cache creation and 432
  thin-copy records;
- D02 texture analysis and D05a zero-match analysis;
- run summaries and sanitized system information;
- diagnostic patches 0004 and 0005;
- D05a screenshot, clearly labeled as an unchanged reproduction from an
  invalid zero-adjustment run;
- a README with exact original and shared-artifact hashes.

Do not attach raw game files, prefix contents, shader caches, credentials, or
the unreviewed 20 MB filtered trace.
