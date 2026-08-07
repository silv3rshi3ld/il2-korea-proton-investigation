# Upstream drafts (review only; do not post automatically)

> Superseded for current review by
> [`community-update-draft-2026-08-06.md`](community-update-draft-2026-08-06.md),
> which includes the invalid D05a zero-match result and paused D05b state.

The 2026-08-07 drafts immediately below reflect the successful D07 result and
validated general `cf11ba76` candidate. Older drafts remain as historical context and
must not be posted. Attach only selected, reviewed screenshots and filtered
logs; do not upload the game, prefix, credentials, or unfiltered large
artifacts.

## 2026-08-07 result update for VKD3D-Proton #3134

Review only; do not post automatically.

```text
I found a causal fix for the terrain corruption on my RX 7800 XT / RADV 26.1.6
system.

The game fills ordinary 2048x2048 BC3 baked-terrain caches from placed
R32G32B32A32_UINT upload footprints. Both physical elements are 16 bytes, so a
64x64 source footprint represents a 256x256 BC3 page. The current
vk_buffer_image_copy_from_d3d12() path keeps the 64x64 source dimensions and
source row geometry when constructing VkBufferImageCopy2. That populates only
the upper-left fraction of each terrain page.

A tightly gated diagnostic converted the geometry through physical blocks. In
one controlled run it adjusted 522/522 matching copies with zero rejects:
- 178 x 64x64 -> 256x256 interiors
- 182 x 64x1 -> 256x4 borders
- 162 x 1x64 -> 4x256 borders

At 5,491 m and 5,501 m the terrain is now continuous; the former isolated
rectangles, black gaps, and magenta page edges are gone. The same successful
run still has the game's missing-file fallback lines and 40,408 split END_ONLY
warnings, so neither is required for this terrain failure.

I reduced the diagnostic to a general VKD3D-Proton commit based on 84c87c83.
It converts placed-footprint extent and buffer layout into destination image
texels whenever source and destination physical block sizes match. There is no
game/AppID check. A focused regression test reports four failures on the old
helper and passes all 22 assertions with the fix; the existing BC copy tests
also pass.

The clean general build has now been tested without the diagnostic environment
variable. The log identifies build cf11ba76 and contains no diagnostic marker.
Terrain remains continuous and detailed at 4,813 m, 2,427 m, and 742 m. The
menu-aircraft blocks and shimmering remain unchanged and are being tracked as
a separate defect.

Proposed small attachment set: one before/after screenshot pair, the bounded
522-copy summary, environment/build hashes, and the clean patch.
```

## 2026-08-07 result update for Proton #9906

Review only; do not post automatically.

```text
Rendering update: the terrain cause has been isolated to VKD3D-Proton's
buffer-to-image copy geometry, not the loading percentage, missing-file log
entries, split-barrier warnings, descriptor buffers, or queue selection.

IL-2 supplies 64x64 R32G32B32A32_UINT placed footprints for 256x256 BC3
terrain pages. VKD3D-Proton kept the source-format dimensions instead of
converting them to destination BC3 texels, so only part of each page was
populated. A gated diagnostic corrected 522/522 observed page/border copies
with zero rejects. Terrain that was severely broken near 5,500 m then rendered
continuously with no rectangular holes or magenta seams.

A general VKD3D-Proton fix and focused regression test are now validated. The
old helper fails four assertions; the fixed helper passes all 22. A clean
general-build game run without the diagnostic gate repairs terrain from 4,813
m down to 742 m. The menu blocks/shimmering and OpenMP/NUMA startup mitigation
remain separate unresolved issues:
  OMP_NUM_THREADS=16 KMP_AFFINITY=disabled %command%
```

## VKD3D-Proton pull-request draft

```text
Title: vkd3d: Convert buffer-image copies between block formats

D3D12 placed footprints describe buffer geometry in footprint-format texels.
VkBufferImageCopy2 instead expresses bufferRowLength, bufferImageHeight, and
imageExtent in image-format texels. The existing buffer-to-image helper used
the placed-footprint geometry unchanged when the footprint and destination
formats had different block dimensions.

Convert the source extent through physical block counts and express the buffer
layout in destination image texels when both formats have the same physical
block size. Preserve the existing path for unequal block sizes.

This fixes R32G32B32A32_UINT footprints copied to BC3 images. A 64x64 source
contains 64x64 16-byte elements and therefore covers a 256x256 BC3 image, not a
64x64 BC3 region.

The new regression test fails four assertions before the change and passes 22
after it. Existing texture BC/RGBA and block-compressed copy tests pass. An
IL-2 Korea (247970) runtime diagnostic adjusted 522 matching terrain-page
copies and repaired the high-altitude terrain; final general-build validation
is recorded separately.
```

## Historical pre-D07 focused update — superseded

Use this as a review draft only. Do not post automatically.

```text
I have isolated a concrete compressed-copy problem on the active terrain-cache
path. This is from a separate RX 7800 XT / RADV 26.1.6 reproduction on game
build 24596901.

A verified custom VKD3D trace recorded 39,978 CopyTextureRegion calls while the
rectangular missing terrain and magenta page edges were visible. The game does
not use D3D12 reserved/tiled-resource APIs. Read-only inspection of its terrain
module instead exposes an application-managed path named BlocksCache,
BakedTerrainCache, BakedTerrain, g_tTiles, CDistantLOD, and stitchBorders.

The run creates 164 placed 2048x2048, one-mip DXGI_FORMAT_BC3_UNORM textures.
Before the screenshot, fourteen members receive 382 invalid border regions; by
the trace cap, the total is 432 across sixteen members. The affected members
also receive observed 64x64 or 128x128 interior page uploads. The same active
pool's border copies have these extents:

- 118 x 128x1
- 112 x 1x128
- 110 x 64x1
- 92 x 1x64

Their image offsets are multiples of four, but the one-texel dimension does not
reach the 2048x2048 mip edge. Representative emitted regions are
extent=1x128 at offset=1020,1536 and extent=128x1 at offset=1024,1536.

Current vk_buffer_image_copy_from_d3d12() copies the D3D12 source-box extent
directly into VkBufferImageCopy2.imageExtent, and the buffer-to-image path then
submits it to vkCmdCopyBufferToImage2. The same code remains in current upstream
commit 84c87c83, which I also tested unmodified with no visual improvement.

For BC3, Vulkan transfer granularity is scaled by the 4x4 compressed texel
block. These internal one-texel regions therefore violate
VUID-vkCmdCopyBufferToImage2-imageOffset-07738. VKD3D-Proton's own
d3d12_invalid_usage cross-test also classifies non-block-sized compressed-copy
coordinates as invalid D3D12 usage. My current assessment is invalid game usage
tolerated by native Windows plus a VKD3D compatibility gap, not evidence of a
RADV bug.

Confidence is high that this explains the magenta seams because the resource
class, border-only shapes, engine stitchBorders name, and visual result all
agree. It is not yet proven to explain the large black/absent pages: the
interior 64/128-page uploads are block-valid. The menu block effect is also not
yet correlated and may be separate.

The next controlled test will be an opt-in diagnostic build which expands only
these aligned internal one-texel BC3 regions to the complete four-texel physical
block and logs each adjustment. I am not proposing an application override or
permanent patch before that behavioral result.

Proposed attachments:
- compact D02 alignment analysis and exact counts
- representative filtered copy lines and resource-create records
- D02 screenshot at 1,385 m
- source line references for the converted VkBufferImageCopy2 path
- full environment and DLL hashes
```

## Historical Proton #9906 update — superseded

```text
Controlled test update for Korea. IL-2 Series (247970)

Environment
- CachyOS, kernel 7.1.6-1-cachyos, Wayland
- Ryzen 7 7800X3D (8 cores/16 threads, one Linux NUMA node)
- Radeon RX 7800 XT; 16 GiB PCI BAR mapped
- Mesa/RADV 26.1.6 (Mesa 26.1.6-arch3.1)
- Proton Experimental experimental-11.0-20260724c; prefix version 11.0-100
- VKD3D-Proton 3dfc6f07d0953b1e8b41705275c2c59cc7374fc5
- DXVK 1a5919b7edd111887648d1e8bf0c32733e2e00d3
- Game build ID 24596901

Runtime path
bin/game/IL2Series.exe loads the game's dxBackend12.dll, Proton's
VKD3D-Proton d3d12.dll/d3d12core.dll, and DXVK dxgi.dll. No completed
controlled log with module evidence loads d3d11.dll.

Startup
The current mitigation is:
  OMP_NUM_THREADS=16 KMP_AFFINITY=disabled %command%

The shipped libiomp5md.dll loads in these runs. Wine logs
GetNumaHighestNodeNumber as a semi-stub under the mitigation, but these tests
did not capture the original GetNumaNodeProcessorMaskEx call or isolate which
environment variable is necessary. This remains a mitigation, not a NUMA fix.

Rendering reproduction
1. Start with the mitigation above.
2. Observe the rotating aircraft in the main menu: moving square/block
   corruption appears over the aircraft and effects.
3. Start the same flight and use an external aircraft camera.
4. The terrain is mostly dark/absent, with isolated rectangular texture pages
   and magenta edges. The loss becomes more severe with altitude. Below roughly
   1,500 m, additional low-fidelity vegetation/terrain assets begin to appear;
   near 5,000 m, most ground is absent.

Controlled results (two runs each)
- E00, no VKD3D diagnostic option: reproducible defect in both runs.
- E01, VKD3D_CONFIG=no_upload_hvv: the upload allocation path changed as
  expected. More vegetation was visible, but both screenshots were near
  1,350-1,500 m; the result is altitude-confounded and inconclusive.
- E02, VKD3D_CONFIG=single_queue: menu and terrain defects unchanged in both
  runs. Ordinary async compute/transfer queue selection is unlikely to be the
  primary trigger.
- E03, descriptor buffer disabled: prepared but not yet run.
- E04, no_upload_hvv plus single_queue: not run.
- D01b, dedicated custom Proton with gated resource telemetry: the trace build
  is verified active and records zero CreateReservedResource,
  GetResourceTiling, UpdateTileMappings, or CopyTileMappings calls while the
  corruption is visible. D3D12 sparse/tiled resources are excluded from this
  failing path.
- D02, dedicated custom Proton with bounded ordinary-texture telemetry: the
  defect remains visible at 1,385 m. Of the multi-mip block-compressed
  resources, 2,355 have complete geometric buffer-upload coverage, none are
  partial, and all 4,185 SRV descriptions use ResourceMinLODClamp 0. A separate
  pre-cap class of 405 placed BC3 textures has an SRV but no logged incoming
  upload or texture copy. Another 28 textures were created only after copy
  suppression and are classified as unobservable rather than missing uploads.
  SRV creation alone does not establish shader use.
- D03, dedicated custom Proton with bounded placed-resource telemetry: visually
  unchanged. All 585 same-run candidates created and exposed by an SRV before
  the copy cap have matching placed-resource records; none overlaps any traced
  placed buffer/texture range. The full run records zero explicit legacy alias
  barriers. Another 952 broad candidates first appeared after copy suppression
  and were excluded from the correlation.

No run reported device loss, GPU hang/reset, or out-of-memory errors. Repeated
split END_ONLY barrier warnings remain uncorrelated with an affected resource
and are not treated as causal.

Assessment
The altitude dependence and rectangular pages make ordinary texture streaming,
descriptor propagation/use, or explicit LOD behavior the leading hypothesis
class. D01b excludes API-level sparse residency. D02 weakens broad incomplete-
mip and non-zero minimum-LOD explanations. D03 excludes placed-resource range
aliasing for the covered class but does not prove whether these SRVs are bound
or sampled. The next single control disables the active descriptor-buffer
extension; descriptor QA follows only if it remains unchanged. The defective
layer (game, VKD3D-Proton, or RADV) remains unisolated; no override or
behavior-changing source patch is proposed.

Attachments proposed after review
- environment.md
- experiment-matrix.md
- findings.md
- one E00 low-altitude and one E00 high-altitude screenshot
- one E02 high-altitude screenshot
- D02 1,385 m screenshot and compact generated resource analysis
- filtered log comparison and checksums
```

## Historical VKD3D-Proton #3134 update — superseded

Use this as a comment on the existing issue rather than opening a duplicate.

```text
I reproduced the issue on a separate AMD generation and completed six
controlled runs.

Environment: RX 7800 XT (Navi 32), Mesa/RADV 26.1.6, Proton Experimental
experimental-11.0-20260724c, VKD3D-Proton commit
3dfc6f07d0953b1e8b41705275c2c59cc7374fc5, game build 24596901.

The runtime path is IL2Series.exe -> dxBackend12.dll -> VKD3D-Proton
D3D12/D3D12Core plus DXVK DXGI. d3d11.dll is not loaded in any controlled run.
Startup still requires OMP_NUM_THREADS=16 KMP_AFFINITY=disabled on this host.

Results, two runs per completed configuration:
- Baseline: repeated menu block artifacts and severe rectangular terrain-page
  loss with magenta edges.
- VKD3D_CONFIG=no_upload_hvv: allocation-path change confirmed, but visual
  comparison is inconclusive because both captures were at lower altitude.
- VKD3D_CONFIG=single_queue: unchanged twice.
- Descriptor-buffer disable and the combined option were not run.
- A valid dedicated-custom-Proton trace recorded zero D3D12 reserved-resource,
  tiling-query, tile-update, or tile-copy calls during reproduction. This
  excludes API-level sparse/tiled resources from the failing path.
- A second valid diagnostic run traced ordinary textures. The same missing
  pages remain at 1,385 m. It found 2,355 complete block-compressed mip uploads,
  zero partial resources, zero non-zero SRV minimum-LOD clamps, and no logged
  operation after resource destruction. Cap-aware analysis found 405 pre-cap
  placed BC3 textures with SRVs but no logged incoming upload/copy; actual
  binding/use is not yet known.
- A third valid diagnostic run added placed-resource range and legacy alias-
  barrier telemetry. All 585 same-run pre-cap candidates have matching resource
  records, none overlaps any traced placed buffer/texture range, and no explicit
  legacy alias barrier occurs in the full run. This excludes placed-resource
  memory aliasing for the covered class, but not descriptor-heap type reuse.

There is a strong altitude/distance dependency across configurations. The user
observes some low-fidelity assets loading below roughly 1,500 m, while captures
near 5,000 m show almost entirely absent ground. D02 now supplies stable
ordinary-resource IDs and mip ranges, but it does not yet correlate the
suspicious no-upload class with descriptor propagation or actual shader use.
The next one-variable test disables `VK_EXT_descriptor_buffer`. This still does
not distinguish application, VKD3D-Proton, or driver behavior.

No device loss, GPU reset/hang, or OOM was found. Split END_ONLY barrier
warnings are frequent but have no resource correlation, and the current
VKD3D-Proton code already handles END_ONLY as a conservative full transition.

Current conclusion: no game override or general patch is justified. D03 rules
out a speculative placed-resource alias workaround. E03 now tests the active
descriptor-buffer backend in isolation; only a repeatable visual change or a
descriptor-QA finding should select a descriptor workaround or source fix.
```

## Wine or Proton startup report outline

Do not file this yet. The original failing call, caller, arguments, return
value, and Windows comparison have not been captured. A useful report needs:

- a clean failure log without the OpenMP mitigation;
- module attribution for `GetNumaNodeProcessorMaskEx`;
- the returned Wine processor-group/NUMA topology;
- isolated tests of `KMP_AFFINITY=disabled` and `OMP_NUM_THREADS=16`;
- comparison with the same `libiomp5md.dll` behavior on Windows.

## Mesa/RADV report gate

No Mesa report is justified from the current evidence. Prepare one only if a
minimal Vulkan reproduction or driver comparison shows that VKD3D-Proton emits
valid Vulkan and RADV produces the wrong result. Include exact Mesa commit,
kernel and firmware, validation classification, trace/reproducer checksum,
alternative-driver result, and good/bad commit range where available.

## Pull-request gate

A PR draft and validated general candidate now exist above. Do not submit them
automatically; the user will decide when and where to publish. The final
handoff should include the focused regression, one compact before/after pair,
the bounded copy summary, build hashes, and a note that menu artifacts and the
NUMA startup issue are separate. No `IL2Series.exe` override is proposed.
