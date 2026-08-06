# Community update drafts — review only

Nothing below has been posted. The two versions target the open reports in
[ValveSoftware/Proton #9906](https://github.com/ValveSoftware/Proton/issues/9906)
and
[VKD3D-Proton #3134](https://github.com/HansKristian-Work/vkd3d-proton/issues/3134).

## Draft for ValveSoftware/Proton #9906

```text
Small progress update from a second AMD/RADV system (RX 7800 XT, Mesa 26.1.6).

The OpenMP launch options are still required here. The rendering problem is
unchanged with single_queue, descriptor buffers disabled, and an unmodified
current VKD3D-Proton build. no_upload_hvv was inconclusive because that run was
not captured at a comparable altitude.

The clearest lead so far is the terrain cache. A trace found 432 very narrow
BC3 texture uploads (64x1, 128x1, 1x64 and 1x128). These appear to be border
stitching operations, but the one-texel dimension is not valid for Vulkan's
4x4 BC3 copy granularity. This is a plausible explanation for the magenta
terrain seams. It has not yet been proven to cause the missing terrain pages,
and the menu artifacts may be separate.

I have a revised diagnostic build ready, but it has not been run yet. I also
have a small sanitized handoff bundle with the useful logs, trace excerpt,
results and diagnostic patches if somebody wants to reproduce or continue the
investigation.
```

## Draft for VKD3D-Proton #3134

```text
I reproduced this on an RX 7800 XT with Mesa/RADV 26.1.6. single_queue,
disabling descriptor buffers, and an unmodified current VKD3D-Proton build do
not change the corruption. The traced mission path uses no reserved-resource
or tile-mapping calls, and the placed resources checked so far do not overlap.

The strongest lead is an invalid-copy pattern on the active terrain cache. The
game creates 164 placed 2048x2048 one-mip BC3 textures. A trace recorded 432
narrow uploads to this pool:

- 118 x 128x1
- 112 x 1x128
- 110 x 64x1
- 92 x 1x64

The offsets are block-aligned, but the one-texel dimension does not reach the
mip edge. VKD3D forwards it to VkBufferImageCopy2, where it violates Vulkan's
4x4 BC3 copy granularity. Strings in landscape.dll such as BakedTerrainCache
and stitchBorders support the interpretation that these are terrain-border
copies. This likely explains the magenta seams, but it does not yet explain the
missing pages or menu corruption.

An initial opt-in normalization test matched zero copies because its matcher
was too strict, so that visual result was inconclusive. A footprint-aware
revision is compiled but has not yet been tested. I am not proposing an app
override at this point.

I have a sanitized handoff bundle with the focused trace, analysis, logs,
hashes and diagnostic patches available. Guidance on whether VKD3D should
tolerate this invalid D3D12 copy pattern would be useful before proceeding
further.
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
