# Final technical report

Status date: 2026-08-11

## Executive conclusion

Korea. IL-2 Series exposed three independent compatibility problems on the
tested Linux/Proton path. Each problem now has a demonstrated cause, a focused
solution, and its own upstream delivery path.

| Track | Final solution | Upstream state |
| --- | --- | --- |
| Startup | Implement the missing Windows NUMA topology queries in Wine | Wine MR [!11604](https://gitlab.winehq.org/wine/wine/-/merge_requests/11604) remains open. The same six commits are present in Valve's Wine fork and the Proton Bleeding Edge source branch |
| Terrain | Convert placed-buffer copy geometry through equal-sized physical blocks when source and destination block dimensions differ | Merged through VKD3D-Proton PR [#3202](https://github.com/HansKristian-Work/vkd3d-proton/pull/3202) as upstream commit `731c4aae` |
| Lighting | Match the native AMD and pre-GFX10 texel-buffer out-of-bounds selection in RADV | Mesa MR [!43672](https://gitlab.freedesktop.org/mesa/mesa/-/merge_requests/43672) is the current upstream direction. D52 independently isolates the view-format boundary without modifying dxil-spirv. The earlier dxil-spirv and VKD3D-Proton drafts are superseded experiments, not the intended final fix |

The three tracks do not depend on one another. None is a game mod. The latest
lighting discriminator changes neither the game nor dxil-spirv: it keeps the
same typed-buffer shader and supplies an exact `R32_UINT` sibling view for the
affected allocation. Two D52 runs removed the rectangular lighting blocks
while retaining real lighting and shadows. Those D52 runs used
`OMP_NUM_THREADS=16 KMP_AFFINITY=disabled` only to bypass the independent Wine
startup problem, and intentionally excluded the terrain fix. They are not a
combined, parameter-free Proton validation.

This report describes proven candidate behavior. It does not claim that all
three changes are already included in standard Proton.

## 1. Startup and Wine NUMA topology

### Symptom

The game could abort around 60 percent startup unless users supplied an
OpenMP-affinity launch workaround. Some configurations also used a fixed
`OMP_NUM_THREADS` value, which was inherently host-specific.

### Cause

The game's `libiomp5md.dll` queries Windows NUMA and processor topology during
initialization. Wine already knew the Linux topology internally, but the
queried public functions returned an unimplemented result. Intel OpenMP treated
that failure as fatal.

### Solution and validation

Wine MR !11604 implements `SystemNumaProcessorMap` and the associated
kernelbase and kernel32 NUMA APIs. The exact six-commit MR head
`e8319c0e6bfe7f94512218b48e3158e0c286b481` was backported without conflicts
to Proton 11 Wine commit `81d78e4f3ea8ce868d775021fdc9f90122dc1a6b`.

Validation used the game's exact OpenMP DLL and a full Steam launch:

- Steam launch-options field: empty;
- no `KMP_*`, `OMP_*`, or `WINE_CPU_TOPOLOGY` variable in the live game;
- OpenMP initialized successfully;
- affinity-limited runs with 1, 2, 4, 8, and 16 allowed CPUs all succeeded;
- OpenMP reported the corresponding runtime-available processor count;
- no CPU vendor, AppID, executable name, or processor count is encoded in the
  six commits.

At the final status check, the Wine MR was still open and mergeable. The same
six-patch series had since been applied to
[Valve's Wine fork](https://github.com/ValveSoftware/wine/compare/c3007e6f2a36914cc55301eb5efd067707bf8bb1...99166a7e25b08ccef0168217540542260eaed76f),
and the
[Proton Bleeding Edge source branch](https://github.com/ValveSoftware/Proton/commit/d28e7f2c40da279452db93897c5b9c2c84356fac)
pinned that Wine revision. The ordinary `experimental_11.0` and `proton_11.0`
source branches were still pinned before the series. No separate runtime claim
is made here for a Steam-distributed Bleeding Edge package.

The report from `@bwRavencl` is useful independent terrain evidence but is not
an empty-launch-options startup validation: that run retained
`KMP_AFFINITY=disabled` and only omitted `OMP_NUM_THREADS`.

Primary evidence:

- [`evidence-n05-wine-mr-11604.md`](evidence-n05-wine-mr-11604.md)
- [`startup-numa-assessment.md`](startup-numa-assessment.md)
- [`../probes/numa-openmp-probe.c`](../probes/numa-openmp-probe.c)

### Evidence boundary

The runtime test covers one 16-logical-CPU AMD host with one NUMA node. The
implementation is topology-driven rather than host-specific, but sparse node
IDs, physical multi-node systems, processor groups, and additional CPU vendors
remain appropriate upstream and cross-hardware validation cases.

## 2. Terrain-page copy geometry

### Symptom

Flight views contained large black or hollow rectangular terrain pages and
magenta page boundaries. Menu lighting blocks remained after terrain repair,
proving that the two graphics signatures were independent.

### Cause

IL-2 uses a `64x64 R32G32B32A32_UINT` placed footprint as the physical source
for a `256x256 BC3_UNORM` destination page. One uncompressed 16-byte source
texel represents one 16-byte 4x4 BC3 block. D3D12 expresses the buffer layout
in source-format texels, while Vulkan expresses `VkBufferImageCopy2` geometry
in destination-image texels.

VKD3D-Proton retained the `64x64` source geometry. Only a `64x64` BC3 region,
one sixteenth of the intended `256x256` destination area, was populated.

### Solution and validation

Candidate `64ec55e7ab3d34012a74e5cbe8f096d4a199e272` converts the row length,
image height, and extent through physical block counts only when:

1. source and destination physical elements have equal byte size; and
2. their block width or height differs.

Formats with equal block geometry or unequal physical element size stay on the
original path. The change contains no IL-2 executable, AppID, resource-size, or
environment-variable check.

Validation includes:

- a causal diagnostic adjusted 522 of 522 encountered terrain-page and border
  copies with zero rejects;
- the clean general D08 build restored terrain at 4,813 m, 2,427 m, and 742 m;
- the focused regression passes 22 of 22 assertions and fails four
  deterministically on the old helper;
- `VKD3D_TEST_FILTER=copy` executes 6,429,713 checks with zero failures;
- native and MinGW x64 builds, plus x64 and x86 packages, complete;
- no GPU device loss, reset, hang, or out-of-memory signature was observed;
- `@bwRavencl` independently confirmed that the PR artifact repairs terrain on
  another system.

The exact PR patch is
[`../patches/0009-vkd3d-Convert-buffer-image-copies-between-block-formats.patch`](../patches/0009-vkd3d-Convert-buffer-image-copies-between-block-formats.patch).

Primary evidence:

- [`evidence-d08-result.md`](evidence-d08-result.md)
- [`evidence-pr-scope-refinement.md`](evidence-pr-scope-refinement.md)
- [VKD3D-Proton PR #3202](https://github.com/HansKristian-Work/vkd3d-proton/pull/3202)

### Evidence boundary

The in-game D08 run used predecessor `cf11ba76`. Candidate `64ec55e7` adds a
narrower predicate while leaving the selected IL-2 conversion and computed
values unchanged. The focused and complete copy tests were rerun on
`64ec55e7`; it was not separately packaged for another in-game run. The public
corrupted and repaired terrain images come from different controlled runs and
viewpoints and are representative, not a frame-matched A/B.

## 3. Tiled-light allocator corruption

### Symptom

Translucent rectangular blocks and broad temporal flicker appeared in the
menu, cockpit, external view, and fire-lit scenes. The blocks followed
screen-space lighting tiles rather than terrain pages. Disabling anti-aliasing,
HDR lighting, VRS, descriptor buffers, or asynchronous queues did not repair
them.

The fine sandy or film-grain lighting visible on some aircraft surfaces is
also present on native Windows and is excluded from this defect.

### Cause

Shader `7cefa1bc80bb4c70`, `ComputeLightsFirstRef`, performs a 32-bit global
atomic through a resource bound as an `R16_UINT` typed UAV. Native D3D12 drivers
tolerate this invalid or non-portable combination. A literal typed Vulkan path
cannot legally represent the access.

Three consecutive affected captures establish the visible mechanism:

- 50 workgroups each restart allocation from zero;
- 12,126 requested light references collapse into offsets 0 through 320;
- tile and depth metadata remain identical;
- 69 to 107 overwritten light IDs change between adjacent frames.

That produces stable rectangular tile boundaries with temporally unstable
light membership.

### Why an earlier test appeared negative

D25 changed the shader operation to a StorageBuffer atomic but still selected
the typed texel-buffer descriptor. Its unchanged pixels were therefore an
incomplete translation, not evidence against the allocator mechanism. D45
fixed both halves: it emitted the SSBO access and selected the raw SSBO
descriptor sibling. D46 later appeared to regress because its executable
mapping had accidentally been removed, so the quirk never activated. D47
restored that mapping and retained only the allocator correction.

The chronological D21-D25 document intentionally preserves its then-correct
interim conclusion. D44-D47 supersede that interpretation with descriptor
selection evidence and a clean allocator-only runtime result.

### Solution and validation

The D47 candidate described immediately below is historical causal and runtime
proof. Its direct VKD3D-Proton implementation at `9b6e15be` and patch `0016`
was followed by the compiler-aware D49 experiment. D50 through D52 later
showed that neither SSBO lowering nor a dxil-spirv change is required to repair
the observed failure. The old artifacts remain valid evidence for the
identified allocator failure, but they are not the current upstream
implementation.

Candidate `9b6e15be29fc1ebb1c26796477009152cb1c760d` adds
`VKD3D_SHADER_QUIRK_FORCE_TYPED_UAV_AS_SSBO`. It applies only to:

- executable `IL2Series.exe`;
- shader hash `7cefa1bc80bb4c70`.

The quirk lowers the typed-UAV access as an SSBO and explicitly selects
VKD3D-Proton's raw storage-buffer descriptor sibling. It contains no depth
predicate bypass, producer-shader replacement, launch option, processor value,
terrain change, or game modification.

Validation includes:

- the exact live `R16_UINT` standalone case deterministically produces 7,966
  overlapping and 7,966 missing allocations on both tested RADV devices and
  with both descriptor backends;
- the raw 32-bit SSBO control completes all 8,160 allocations with zero
  overlaps, zero missing entries, and zero validation errors in all four
  GPU/backend combinations;
- D47 keeps the original depth predicates and removes the blocks and broad
  flicker while real lighting and shadows remain;
- a fresh matched A/B compares unmodified `84c87c83` with exact candidate
  `9b6e15be` on the same NUMA-capable Wine base;
- the two packages differ only in their four VKD3D-Proton DLLs;
- no RenderDoc layer, shader override, launch parameter, or game-file
  modification was active during the final A/B;
- clean x86-64 and x86 MinGW packages build.

The exact PR patch is
[`../patches/0016-vkd3d-shader-Work-around-IL-2-tiled-light-allocator.patch`](../patches/0016-vkd3d-shader-Work-around-IL-2-tiled-light-allocator.patch).

Primary evidence:

- [`evidence-d44-consecutive-capture-result.md`](evidence-d44-consecutive-capture-result.md)
- [`evidence-d45-correct-ssbo-binding-result.md`](evidence-d45-correct-ssbo-binding-result.md)
- [`evidence-d47-allocator-only-wired-result.md`](evidence-d47-allocator-only-wired-result.md)
- [`evidence-u01-upstream-candidate-ab.md`](evidence-u01-upstream-candidate-ab.md)
- [VKD3D-Proton issue #3134 update](https://github.com/HansKristian-Work/vkd3d-proton/issues/3134#issuecomment-5238151028)
- [VKD3D-Proton PR #3207](https://github.com/HansKristian-Work/vkd3d-proton/pull/3207)
- [D50-D52 conclusion posted on PR #3207](https://github.com/HansKristian-Work/vkd3d-proton/pull/3207#issuecomment-5256360847)

### D49 compiler-aware, ABI-safe experiment

D49 tested moving generic legalization into dxil-spirv while leaving
application selection and Vulkan descriptor capability in VKD3D-Proton:

- dxil-spirv recognizes only an eligible scalar 32-bit `I32` or `U32` atomic
  on a typed UAV. It temporarily presents that binding to the resource
  remapper as a raw buffer.
- Lowering is accepted only when the remapper returns an SSBO descriptor. If
  remapping fails or returns another descriptor class, dxil-spirv restores the
  original typed binding and retries the normal typed path.
- 64-bit atomics, sparse operations, resources without atomics, and the
  SM 6.6 heap path are excluded from lowering.
- No field was added to a public C callback structure. The mechanism uses the
  existing remapper contract and an additive shader-quirk value.
- VKD3D-Proton requests the compiler quirk only for exact executable
  `IL2Series.exe` and exact shader hash `0x7cefa1bc80bb4c70`, and only when
  `RAW_SSBO` is available without the mutable single-descriptor
  `MUTABLE_TYPE_RAW_SSBO` layout.

The retained test tool is
`IL2-Korea-D49-CompilerAware-ABISafe-731c4aae`. Its source bases are
VKD3D-Proton `731c4aae5991b33f2ddab45d3cb1b4779159bf4b` and dxil-spirv
`edd8fdf702c3445eb659f2652d04436ed86e4206`.
The current local dxil-spirv candidate is
`afff4dfb3e51ab81a4d541011bcf7ec2f65e2ffa`; it has not been published. The
dependent VKD3D-Proton integration has no final commit or gitlink identity yet.

Validation includes:

- a clean dxil-spirv resources reference suite;
- a full dxil-spirv suite whose only failure is the independently reproduced
  baseline validator failure in `control-flow/switch-continue.frag`;
- valid SPIR-V for the exact captured IL-2 shader in candidate, fallback, and
  no-quirk baseline modes;
- clean VKD3D-Proton x86-64 and x86 builds plus the complete package build;
- an exact VKD3D remapper harness in which capability ON emits a
  `StorageBuffer` access with `OpAtomicIAdd`, while capability OFF is
  byte-identical to the typed baseline;
- verified loading of the D49 tool and one runtime covering the menu, a short
  flight, and the map. Terrain, real lighting, and shadows rendered correctly,
  while the square blocks and broad flicker were absent.

The fine sandy or film-grain lighting remains excluded because it is also
present on native Windows. D49 runtime evidence currently covers the reporting
host only. No cross-hardware validation claim is made.

D49 was a successful diagnostic implementation, but it is superseded by the
D50 through D52 result below. Its success does not establish that dxil-spirv or
SSBO lowering is required.

### D50 through D52 view-format isolation

Rubber-ducking the D49 result exposed one remaining confounder: the SSBO
candidate changed both shader lowering and descriptor representation. D50
therefore held the buffer, 87,040-byte range, shader operation, dispatch, and
coordinate-zero 32-bit atomic constant while changing only the texel-buffer
view in the sequence `R32_UINT`, `R16_UINT`, `R32_UINT`. Both R32 runs were
globally correct. The R16 run reproduced the per-workgroup counter restart and
corrupt allocation.

D51 then ran the exact captured game shader with the same 87,040-byte
`R32_UINT` alias through both the mutable descriptor-set and descriptor-buffer
paths. Both passed without out-of-range writes. This excludes buffer size,
range, dispatch topology, and an inherent need for StorageBuffer lowering from
the observed failure.

D52 translated the original byte-identical DXIL with the stock dxil-spirv
gitlink `cc75a0c98d34d7bcc03560527c799b52e48b4d1f`. Its SPIR-V retained the
natural typed-buffer form:

- `R32ui` texel-buffer image type;
- `OpImageTexelPointer`;
- `OpAtomicIAdd`.

The only relevant SPIR-V change from D14 was selection of the R32 sibling
descriptor. A one-shot runtime marker confirmed creation of that alias for the
exact executable, shader, 87,040-byte resource, and matching UAV description.
Two D52-r2 game runs showed no square lighting blocks. The first captured the
marker and shader dump; the second used no VKD3D diagnostic environment.

D52 is deliberately narrow diagnostic code, not the preferred upstream fix.
It supports the conclusion that the failure is at the texel-buffer view and
RADV out-of-bounds-behavior boundary, not in dxil-spirv lowering. It also
intentionally excludes terrain PR #3202. Both runs needed
`OMP_NUM_THREADS=16 KMP_AFFINITY=disabled` because D52 used a Proton base
without the independent Wine NUMA fix.

Full evidence is recorded in
[`evidence-d50-d52-r32-alias-result.md`](evidence-d50-d52-r32-alias-result.md).

### Current upstream direction

Mesa MR [!43672](https://gitlab.freedesktop.org/mesa/mesa/-/merge_requests/43672)
changes RADV texel-buffer descriptors on GFX10 and later from
`STRUCTURED_WITH_OFFSET` to `STRUCTURED` out-of-bounds selection. Both choices
are valid for ordinary texel-buffer access, but the proposed choice matches
AMD's native D3D12 driver and pre-GFX10 behavior for this invalid 32-bit atomic
through an `R16_UINT` view. It is a cleaner and more general compatibility
location than either a game-specific VKD3D-Proton alias or compiler lowering.

The D50 through D52 evidence agrees with that direction, but it is not a local
runtime validation of the Mesa MR itself. The MR was still open at this status
date. Review and a clean test using unmodified VKD3D-Proton and dxil-spirv
remain necessary before calling the upstream fix complete.

### Compatibility policy

Principally, the game should bind a legal 32-bit UAV for a 32-bit atomic.
Practically, compatibility layers also reproduce narrowly demonstrated native
driver tolerance for shipped software. D52 used exact executable, shader, and
resource predicates to make the experiment safe. Mesa MR !43672 instead
proposes matching native AMD and pre-GFX10 descriptor behavior generally,
without carrying an IL-2 application quirk in VKD3D-Proton.

## Exact public artifact hashes

| Artifact | SHA-256 |
| --- | --- |
| Terrain PR patch `0009` | `ca20fb05e712f2ae8216e65843990720a67d49c81b506245a17bb82fc0b58d2a` |
| Lighting PR patch `0016` | `4d43ac526b47d07b9694633de42cacc284e961d9fc84050df5d166c650a7216a` |
| Representative corrupted terrain image | `d613c4f044d7a9dccef12cba9992aa7210909f86c7feca82ae5459cb07badacd` |
| Repaired terrain image | `2c0b1e25bd394c192ba9b33e7e387ae7d521afcf0386ee629fda5d4a7d711900` |
| Matched lighting baseline | `8f88c75baaf51f595edd94362d5663554415082390e890076ba7d2209d3682be` |
| Matched lighting candidate | `29df6e3346c79597135fe0f8dc833aed149e2e099308057bb497a18144ddc454` |

The `0016` hash identifies the historical D47 artifact and is retained for
reproducibility. It must not be presented as the D49 two-repository
implementation artifact.

## Repository role and remaining work

The causal investigation is complete. Remaining work is to test and review
Mesa MR !43672, obtain cross-hardware confirmation, and follow its eventual
Mesa and Proton delivery. The dxil-spirv and VKD3D-Proton lighting drafts are
superseded and should not be treated as mergeable final implementations. This
repository remains open so that upstream results can be recorded without
rewriting the original evidence.

For the complete chronology, negative controls, and invalid test attempts, see
[`README.md`](README.md), [`experiment-matrix.md`](experiment-matrix.md), and
[`findings.md`](findings.md).
