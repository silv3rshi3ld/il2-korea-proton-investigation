# D44 consecutive tiled-light capture: result

> [!NOTE]
> Historical capture interpretation. D44 correctly identifies the malformed
> allocator and why D25 did not test a usable descriptor. D50-D52 later showed
> that a full-size R32 texel-buffer alias also repairs the allocator without
> SSBO lowering. Mesa MR !43672 is the current upstream direction.

## Result

D44 captures the square blocks and broad light flicker in three consecutive
frames. The first unstable value is not packed depth or tile metadata. It is
the overwritten light-ID payload produced by the still-malformed global list
allocation.

This also corrects the interpretation of D25 and D42. D25 changed the allocator
shader to a `StorageBuffer`, but it did not select VKD3D-Proton's raw SSBO
descriptor sibling. The translated shader therefore consumed the typed
descriptor binding with the wrong Vulkan access class. D42 inherited that
incomplete change. A visually favourable D42 frame was not a complete fix.

## Captures

The three local-only RDCs are frames 2654, 2655, and 2656 from one
`TriggerMultiFrameCapture(3)` request. The user confirmed both the square grid
and broad lighting flicker were visible during the capture. The captures may
contain game resources and are not upload artifacts.

## Stable values

All three captures contain bit-identical active resources for:

- `m_rtDepthRange26`, `80x34 R32G32_UINT`, SHA-256
  `7fad8d245716b8c0491f9a000f14ef9ecb584d30cf8cdc0b969cc50ac5b542a2`;
- `rtLightRefs25`, `80x34x2 R32_UINT`, SHA-256
  `5055f2dee8bf0efc46640fe7adc4138fcafbcf7aaab645768a80e2b643233bc2`.

The packed depth resource has zero near-greater-than-far intervals. The light
grid records the same count, start, and end for every tile in all three frames.

## Malformed allocation

The grid requests 12,126 light-index entries across 2,720 tiles, with two to
five lights per tile. A correct global allocation would form one non-overlapping
range spanning those entries. Instead:

- maximum start offset: 315;
- maximum end offset: 320;
- all 50 `8x8` workgroups form their own gap-free interval partition beginning
  at zero;
- only the first 320 entries of the 43,520-element `R16_UINT` index buffer are
  used.

This exactly reproduces D20's workgroup-local allocation pattern under D44.

## Temporal discriminator

The grid metadata is identical, but the first 320 light IDs differ:

| Frames | Metadata tile differences | Light-ID differences |
| --- | ---: | ---: |
| 2654 / 2655 | 0 | 69 |
| 2654 / 2656 | 0 | 107 |
| 2655 / 2656 | 0 | 70 |

Each workgroup races to overwrite the same small prefix with its own light
membership. Which workgroup wins changes between frames. Stable tile intervals
can therefore show stable square boundaries while changing IDs produce broad
flicker or an occasionally favourable-looking frame.

The reproducible decoder is
`probes/d44-consecutive-light-capture/analyze_light_membership.py`.

## Why D25 did not repair it

The captured D44 allocator module `7cefa1bc80bb4c70` proves the existing quirk
activated: it contains `OpAtomicIAdd` with device scope through a
`StorageBuffer`. However, both its typed output and forced SSBO variable map to
descriptor set 1, binding 1.

VKD3D-Proton's AMD descriptor-buffer layout emits typed descriptors in that
binding and a storage-buffer sibling in the raw descriptor set. For a normal
typed buffer, `dxil_resource_flags_from_kind()` does not request
`VKD3D_SHADER_BINDING_FLAG_RAW_SSBO`. D25 only changed the emitted access type;
it did not select the raw sibling. The standalone D24 test explicitly supplied
a real storage-buffer descriptor and therefore did not cover this integration
condition.

## Next gate

D45 adds the missing raw-SSBO binding selection under the same exact
`IL2Series.exe` and shader-hash quirk. Its first runtime test must use empty
Steam launch options and judge both the square grid and broad flicker. Fine
sandy or film-grain lighting remains accepted native Windows behaviour.

## D50-D52 refinement

D44 established that changing operation class without changing descriptor
selection was incomplete. It did not prove that the completed selection had to
be an SSBO. D50 later changed only the view format on the same 87,040-byte
buffer and reproduced the restart only with `R16_UINT`. D51 passed the exact
shader through a full-size `R32_UINT` view, and D52 changed only that shader's
descriptor set/binding from `1/1` to `2/0` while retaining `R32ui`,
`OpImageTexelPointer`, and `OpAtomicIAdd`. Two D52 game runs were clean.

The refined conclusion is that the allocator failure is causal and descriptor
view/OOB behavior is the decisive boundary. The raw SSBO sibling was one
successful diagnostic route, not a required final implementation.
