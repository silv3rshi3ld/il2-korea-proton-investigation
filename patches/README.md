# Patch status

No permanent behavior-changing fix is currently justified. D02 justifies one
opt-in diagnostic behavior change: normalize only the observed internal one-
texel BC3 border-copy extent to a complete physical block and measure the
visual result. D05 implements that experiment; it must not be treated as an
application override or an upstream-ready fix before the runtime result.

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

## Why there is no application override

- The repeatable E00 baseline confirms the defect but does not identify a
  behavior that fixes it.
- `VKD3D_CONFIG=single_queue` is unchanged across two runs.
- `VKD3D_CONFIG=no_upload_hvv` was enabled successfully, but its apparent
  vegetation improvement is confounded by altitude and map location.
- Descriptor-buffer disabling is the prepared E03 control; the combined
  upload/single-queue control has not been run.
- Ground pages and magenta borders remain in every captured configuration.

Adding `.NO_UPLOAD_HVV`, `.NO_STAGGERED_SUBMIT`, or another executable override
for `IL2Series.exe` would therefore convert an unproven hypothesis into a
permanent game-specific behavior. That is unsuitable for upstream review.

## Why there is no general VKD3D-Proton or Mesa patch

Valid D01b telemetry excludes D3D12 reserved/tiled resources. D02 supplies
ordinary texture identifiers, mip ranges, upload copies, and lifetime order:
2,355 compressed resources have complete geometric mip uploads, zero partial
resources were found, and all SRV minimum-LOD clamps are zero. It also finds
405 pre-cap placed BC3 textures with an SRV but no logged incoming upload/copy.
D03 matches all 585 same-run pre-cap candidates and finds no overlap with any
traced placed buffer/texture range and zero explicit legacy alias barriers.
This excludes placed-resource memory aliasing for the covered class, but SRV
creation still does not prove shader use and descriptor propagation/use remains
untraced. The broad missing-page defect cannot yet be assigned to the game,
VKD3D-Proton, or RADV. Separately, D02 contains 432 one-texel internal BC3
border copies which VKD3D emits as invalid Vulkan regions. VKD3D's own
cross-test classifies such non-block-sized D3D12 compressed-copy coordinates as
invalid application usage, making the likely origin the game and the native-
Windows difference a compatibility behavior. A gated normalization run is
required before deciding whether this is only the magenta-seam cause or also
affects whole-page loss.

The prefix-only installation attempts did not load the local DLLs because
stock Proton restored its packaged copies during launch. A dedicated custom
Proton tool is required before D01 telemetry can be interpreted. Only a
demonstrated semantic defect should produce a behavior-changing patch.
