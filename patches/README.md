# Patch status

No behavior-changing fix is currently justified.

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

## Why there is no application override

- The repeatable E00 baseline confirms the defect but does not identify a
  behavior that fixes it.
- `VKD3D_CONFIG=single_queue` is unchanged across two runs.
- `VKD3D_CONFIG=no_upload_hvv` was enabled successfully, but its apparent
  vegetation improvement is confounded by altitude and map location.
- Descriptor-buffer disabling and the combined upload/single-queue control
  were not run before runtime testing ended.
- Ground pages and magenta borders remain in every captured configuration.

Adding `.NO_UPLOAD_HVV`, `.NO_STAGGERED_SUBMIT`, or another executable override
for `IL2Series.exe` would therefore convert an unproven hypothesis into a
permanent game-specific behavior. That is unsuitable for upstream review.

## Why there is no general VKD3D-Proton or Mesa patch

Valid D01b telemetry excludes D3D12 reserved/tiled resources, but the current
completed logs still contain no ordinary-texture identifiers, mip ranges,
layouts, or synchronization sequence for the affected terrain. They also
contain no Vulkan validation failure, device loss, GPU hang, or cross-driver
comparison. D02 is designed to supply the next resource-level discriminator;
until it runs, the defective layer cannot be assigned to the game,
VKD3D-Proton, or RADV.

The prefix-only installation attempts did not load the local DLLs because
stock Proton restored its packaged copies during launch. A dedicated custom
Proton tool is required before D01 telemetry can be interpreted. Only a
demonstrated semantic defect should produce a behavior-changing patch.
