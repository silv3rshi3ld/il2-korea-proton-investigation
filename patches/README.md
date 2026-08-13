# Patch status

The separate Wine startup work is documented in
[`wine/README.md`](wine/README.md). The local one-patch draft is superseded for
testing/submission by upstream Wine MR !11604, whose six-commit behavior was
validated through Proton. This NUMA work is unrelated to the VKD3D-Proton
graphics series below. Wine merged MR !11604 on 2026-08-10 at final rebased
head `663fd7cc`. The investigation validated the earlier `e8319c0e` MR head and
Valve's equivalent series, not the final rebased head.

D07 demonstrates the terrain root cause: its complete page-family conversion
adjusted 522/522 copies with zero rejects and repaired terrain near 5,500 m.
`0009-vkd3d-Convert-buffer-image-copies-between-block-formats.patch` is the
original general upstream submission. It contains a focused regression test
and no IL-2-specific override. D08 loaded the clean general package without the
diagnostic gate and repaired the terrain while leaving the separate menu
artifact unchanged. The reviewed form was merged through VKD3D-Proton PR
#3202 as upstream commit `731c4aae`.

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
Its reviewed successor was merged upstream through
[VKD3D-Proton PR #3202](https://github.com/HansKristian-Work/vkd3d-proton/pull/3202).

`0010-vkd3d-Add-focused-menu-resource-and-pass-telemetry.patch` through
`0014-vkd3d-Trace-IL-2-tiled-light-descriptors.patch` are a cumulative,
gated diagnostic series on terrain-fix predecessor `cf11ba76`. D11 names menu
resources and nearby passes; D12 follows reflection-target use; D13 follows the
tiled-light grid and self-light target; D15 extends that trace through the two
final light-list compute stages and records barriers for unnamed buffers as
well as named images. D16 follows descriptor writes and copies in a CPU sidecar
and resolves fixed SRV slots `t9`/`t10` at only the affected pixel shaders while
leaving the normal descriptor implementation active. They change no rendering
unless their private telemetry environment variables are enabled and are not
proposed as a compatibility fix. The series is retained only for
reproducibility and possible later review.

`0015-vkd3d-Trace-IL-2-t7-and-t8-descriptor-contracts.patch` is the final
trace-only increment used to resolve the allocator shader's descriptor
contract. It is diagnostic evidence, not a compatibility candidate.

`0016-vkd3d-shader-Work-around-IL-2-tiled-light-allocator.patch` is the
historical first lighting candidate. It is commit `9b6e15be` on local branch
`fix-il2-tiled-light-allocator`, based directly on upstream `84c87c83`. The
29-line change applies only to `IL2Series.exe` and exact shader
`7cefa1bc80bb4c70`: it lowers the shader's typed-UAV access as an SSBO and
selects VKD3D-Proton's raw SSBO descriptor sibling. It contains no depth-gate
bypass, producer-shader overrides, launch option, processor value, or game
modification. Allocator-only D47 validates the required runtime behavior with
the original lighting and depth predicates: the blocks and broad flicker are
gone while real lighting and shadows remain. The patch SHA-256 is
`4d43ac526b47d07b9694633de42cacc284e961d9fc84050df5d166c650a7216a`.
It was published in
[VKD3D-Proton PR #3207](https://github.com/HansKristian-Work/vkd3d-proton/pull/3207),
which was later closed unmerged as superseded.
Maintainer review correctly identified that dxil-spirv cannot generally treat
a texel buffer as an SSBO solely because VKD3D-Proton selects a different
descriptor. The patch remains valid causal and single-system runtime evidence,
but it is not a portable upstream implementation. D49 superseded its
architecture temporarily, and D50 through D52 later removed the need for the
alternate compiler lowering entirely.

The clean package builds successfully for x86-64 and x86. A fresh matched A/B
on 2026-08-10 used the following exact candidate binaries; see
[`../docs/evidence-u01-upstream-candidate-ab.md`](../docs/evidence-u01-upstream-candidate-ab.md):

| Architecture | File | SHA-256 |
| --- | --- | --- |
| x86-64 | `d3d12.dll` | `effc65c16745831c276d5fdf2a50c26ad8b51e355356eb62c5b7ede940721a65` |
| x86-64 | `d3d12core.dll` | `164847d8ad795d308fa076f91567a3a9320b8c6eb24b1bad5b2f92527d90e72b` |
| x86 | `d3d12.dll` | `17de6a419afe8c1dd90e8af25bb9e6d95a58ddbf2edfb0ed935bec9ea23c6e72` |
| x86 | `d3d12core.dll` | `73a77a5c27bc73c584a8a8ec7b226558d6528bef1f0387468eb074439b2beeca` |

## Historical D49 compiler-aware implementation

D49 moved the generic legalization into dxil-spirv and left only policy and
backend safety in VKD3D-Proton. For eligible scalar 32-bit signed or unsigned
typed-UAV atomics, dxil-spirv temporarily presents the resource to the binding
remapper as a raw buffer. Lowering is enabled only if the remapper returns an
SSBO. On rejection or a non-SSBO result, the original typed resource is
restored and remapped again through the normal typed path. The public C
callback structures are unchanged.

VKD3D-Proton enables that compiler quirk only for `IL2Series.exe`, shader
`7cefa1bc80bb4c70`, and layouts where a raw SSBO descriptor is available and
`VKD3D_BINDLESS_MUTABLE_TYPE_RAW_SSBO` is not active. Unsupported layouts keep
the existing typed fallback. The dxil-spirv side excludes 64-bit atomics,
sparse resources, non-atomic resources, and SM 6.6 heap resources.

The exact local bases are dxil-spirv
`edd8fdf702c3445eb659f2652d04436ed86e4206` and VKD3D-Proton
`731c4aae5991b33f2ddab45d3cb1b4779159bf4b`. Clean shader tests, exact shader
fallback checks, x86-64 and x86 package builds, and capability-on/off harnesses
pass. The only full-suite validator failure reproduces unchanged on the exact
dxil-spirv base. A verified D49 Steam run with empty launch options showed a
clean menu and short flight, retained the terrain repair, and showed neither
the large blocks nor broad flicker. This validation is currently limited to
the investigation system and is not a cross-vendor claim.

The historical local dxil-spirv candidate commit is
`afff4dfb3e51ab81a4d541011bcf7ec2f65e2ffa`. The compiler direction was
published for review through
[dxil-spirv PR #296](https://github.com/HansKristian-Work/dxil-spirv/pull/296),
with its VKD3D-Proton integration discussed through PR #3207. Both PRs were
closed unmerged as superseded after D50 through D52 showed that this compiler
dependency is unnecessary.

| Architecture | File | D49 SHA-256 |
| --- | --- | --- |
| x86-64 | `d3d12.dll` | `ae0436e5a8c8b9bb597288ac9846de3c01214f06a10f745918ac41ce4770e84a` |
| x86-64 | `d3d12core.dll` | `8d31d58966183707b7f73d05e763cb79fa27707f1eee3965be72a87a6a2a01af` |
| x86 | `d3d12.dll` | `1285974667c4b974baf82aea0a903d8bc7eeba8992a41a4bfc6c36d07f2d7993` |
| x86 | `d3d12core.dll` | `9cdd2eb9d326eea278dc0449d069cf2442001210874bb72a2c3c98a5aeef1024` |

No D49 patch export is included. Its private compiler gitlink was intentionally
never recorded in this repository.

## Current D50 through D52 disposition

D50 changed only the Vulkan view format around the same 87,040-byte buffer,
range, shader, and coordinate-zero 32-bit atomic. The sequence `R32_UINT`,
`R16_UINT`, `R32_UINT` produced pass, fail, pass. D51 then exercised the exact
captured shader through an 87,040-byte R32 alias and passed through both the
mutable descriptor-set and descriptor-buffer paths. Together they isolate the
view-format boundary without changing dxil-spirv lowering.

D52 used VKD3D-Proton only. It retained stock dxil-spirv commit `cc75a0c9`,
the natural R32ui texel-buffer `OpImageTexelPointer` and `OpAtomicIAdd`, and the
ordinary R16 descriptor. For the exact IL-2 executable, shader, resource, and
UAV description, it additionally selected an R32 alias. Two game runs were
free of the square blocks. Because this was an isolated lighting package based
before the other changes, it required the OpenMP startup workaround and did not
contain the merged terrain fix. No D52 screenshot was captured.

D52 is a discriminator, not an upstream patch candidate. Hans' proposed
[Mesa MR !43672](https://gitlab.freedesktop.org/mesa/mesa/-/merge_requests/43672)
is the cleaner and more general direction: it aligns RADV's out-of-bounds
component selection with native AMD D3D12 and pre-GFX10 behavior. NVIDIA
already passes the relevant descriptor-heap test. This investigation agrees
with resolving the behavior there rather than carrying a per-game alias or the
earlier dxil-spirv lowering. Mesa MR !43672 remains open, and the investigation
did not perform an otherwise unmodified-stack game test of that change.

`0017-vkd3d-proton-Add-diagnostic-R32-texel-alias-for-IL2.patch` preserves the
reviewed D52 source diff for reproducibility. It is explicitly a diagnostic
artifact, not an upstream candidate. No D52 binary is published. See
[`../docs/evidence-d50-d52-r32-alias-result.md`](../docs/evidence-d50-d52-r32-alias-result.md)
for the canonical evidence and scope limits.

The same source change was later forward-ported to personal fork branch
`diagnostic-il2-r32-alias-d52` as commit
[`8cd28e8f`](https://github.com/silv3rshi3ld/vkd3d-proton/commit/8cd28e8f98751afe3b85c3b08519464907aa5143).
That forward-port passed the x86-64 and x86 builds but was not rerun in the
game. Its
[follow-up link on closed PR #3207](https://github.com/HansKristian-Work/vkd3d-proton/pull/3207#issuecomment-5276514345)
made the source available for inspection without proposing that the PR be
reopened or the diagnostic merged.

The patch series is now archival. `0009` records historical terrain candidate
`64ec55e7`; the reviewed successor merged as `731c4aae`. `0016`, the D49 build
details, and `0017` record the lighting investigation's successive diagnostic
steps. They are preserved to explain the result, not as a combined Proton
patch set or as a recommendation to reopen the closed lighting PRs.

## Why the terrain candidate has no application override

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
