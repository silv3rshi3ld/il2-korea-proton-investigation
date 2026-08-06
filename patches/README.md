# Patch status

No patch is currently justified.

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

The current logs contain no resource identifiers, mip ranges, tile mappings,
residency changes, layouts, or synchronization sequence for the affected
terrain. They also contain no Vulkan validation failure, device loss, GPU hang,
or cross-driver comparison. The defective layer cannot be assigned to the
game, VKD3D-Proton, or RADV.

If work resumes, first perform matched-location low/high-altitude captures,
then use filtered instrumentation for candidate terrain resources. Only a
demonstrated semantic defect should produce a patch.
