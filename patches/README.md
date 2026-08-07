# Patch status

D07 demonstrates the terrain root cause: its complete page-family conversion
adjusted 522/522 copies with zero rejects and repaired terrain near 5,500 m.
`0009-vkd3d-Convert-buffer-image-copies-between-block-formats.patch` is the
general upstream candidate. It contains a focused regression test and no
IL-2-specific override. D08 loaded the clean general package without the
diagnostic gate and repaired the terrain while leaving the separate menu
artifact unchanged.

`0001-il2-korea-sparse-resource-diagnostics.patch` is a temporary, gated
instrumentation patch, not a candidate fix. It records the D3D12 reserved and
tiled-resource API path only when `VKD3D_IL2_RESOURCE_TRACE=1` is set. It is
retained so D01 is exactly reproducible and must not be proposed upstream as a
remedy.

`0002-Add-bounded-ordinary-texture-telemetry-for-IL-2.patch` is the D02
diagnostic patch, also not a candidate fix. Behind
`VKD3D_IL2_TEXTURE_TRACE=1`, it records a bounded census of ordinary texture
creation/destruction, normalized SRV mip ranges, and texture copies by stable
cookie. It is retained to make the diagnostic build reproducible and must not
be presented upstream as a rendering remedy.

`0003-Add-focused-IL-2-placed-resource-alias-telemetry.patch` is the D03
diagnostic patch. Behind `VKD3D_IL2_ALIAS_TRACE=1`, it records bounded placed-
resource heap ranges, lifetimes, and explicit alias barriers using the same
resource cookies as D02. It changes no D3D12/Vulkan behavior and must not be
presented upstream as a remedy.

`0004-vkd3d-Add-gated-BC3-border-copy-diagnostic-for-IL-2-.patch` is the D05
causal diagnostic. It is inert unless `VKD3D_IL2_BC3_BORDER_COPY=1` is set and
then recognizes only the observed 2048x2048 single-mip BC3 resource class and
`1x64`, `64x1`, `1x128`, or `128x1` copy shapes. It expands only the one-texel
dimension to a full four-texel physical block and emits bounded `IL2BCCOPY`
records. Its runtime result determines whether any permanent compatibility
behavior is justified.

`0005-vkd3d-Handle-footprint-only-IL-2-BC3-border-copies.patch` is the D05b
follow-up. D05a emitted one enable marker but zero adjustments, so it never
tested the behavior. D05b accepts a footprint-only source, permits a physically
compatible BC3 source format, and logs target candidates and explicit safety-
rejection masks before changing the extent. It is compiled but unrun and is
still diagnostic-only.

`0006-vkd3d-Test-RGBA32-to-BC3-buffer-copy-geometry-for-IL.patch` is the D05c
behavioral revision. It proves that every observed thin source is a footprint-
only `R32G32B32A32_UINT` region and performs the physical 1:4 block mapping.
Its unchanged visual result excludes borders alone.

`0007-vkd3d-Add-focused-IL-2-baked-cache-telemetry.patch` is the trace-only
D06 patch. It revealed `64x64` interiors placed on a 256-texel grid and changed
no commands.

`0008-vkd3d-Test-full-BC3-terrain-page-reinterpret-copies.patch` is the D07
increment on top of D05c. It uses a new opt-in gate and adds only the observed
`64x64`/`128x128` interiors to the already bounded conversion. It is a causal
diagnostic, not the proposed permanent fix. Its valid runtime result adjusted
522/522 copies and repaired the terrain.

`0009-vkd3d-Convert-buffer-image-copies-between-block-formats.patch` is clean
commit `64ec55e7` based on upstream `84c87c83`. It corrects
`vk_buffer_image_copy_from_d3d12()` by converting source-footprint geometry
through physical blocks into destination image texels only when the two
formats have equal-sized physical elements and different block dimensions.
Same-block-geometry copies remain on the original path. The included test
fails four assertions on the old helper and passes all 22 with the fix. The
complete native copy-test subset passes 6,429,713 checks with zero failures.
The patch SHA-256 is
`ca20fb05e712f2ae8216e65843990720a67d49c81b506245a17bb82fc0b58d2a`.

## Why there is no application override

- The repeatable E00 baseline and successful D07 causal run identify a format-
  unit conversion, not a game configuration flag.
- `VKD3D_CONFIG=single_queue` is unchanged across two runs.
- `VKD3D_CONFIG=no_upload_hvv` was enabled successfully, but its apparent
  vegetation improvement is confounded by altitude and map location.
- Descriptor-buffer disabling is the prepared E03 control; the combined
  upload/single-queue control has not been run.
- D07 repairs the ground pages and magenta borders by fixing the complete copy
  family, without changing a VKD3D configuration flag.

Adding `.NO_UPLOAD_HVV`, `.NO_STAGGERED_SUBMIT`, or another executable override
for `IL2Series.exe` would not address the demonstrated copy-unit defect. The
general helper fix is narrower in mechanism and applies only when physical
element sizes match while block dimensions differ.

## Why the candidate belongs in VKD3D-Proton

Valid D01b telemetry excludes D3D12 reserved/tiled resources. D02 supplies
ordinary texture identifiers, mip ranges, upload copies, and lifetime order:
2,355 compressed resources have complete geometric mip uploads, zero partial
resources were found, and all SRV minimum-LOD clamps are zero. It also finds
405 pre-cap placed BC3 textures with an SRV but no logged incoming upload/copy.
D03 matches all 585 same-run pre-cap candidates and finds no overlap with any
traced placed buffer/texture range and zero explicit legacy alias barriers.
This excludes placed-resource memory aliasing for the covered class. D06 then
showed that VKD3D emits 64x64 page interiors at destination offsets spaced by
256 texels, alongside borders at the corresponding page ends. D07 changed only
that geometry and repaired the image on the same game, RADV, queue, descriptor,
and upload paths.

D3D12 placed footprints describe the buffer in footprint-format texels.
Vulkan buffer-image copies describe the layout and extent in image-format
texels. The conversion therefore belongs at the D3D12-to-Vulkan translation
boundary. A Mesa workaround would merely hide incorrect Vulkan geometry, and a
game override would duplicate a general format-unit rule.

The prefix-only installation attempts did not load local DLLs because stock
Proton restored its packaged copies during launch. D08 therefore uses a
dedicated custom Proton tool, as did the valid diagnostic runs.
