# Upstream submission plan

> [!NOTE]
> Historical completed plan. The terrain branch was published as
> [VKD3D-Proton PR #3202](https://github.com/HansKristian-Work/vkd3d-proton/pull/3202).
> The separate startup and tiled-light outcomes were later published through
> Wine MR !11604 and VKD3D-Proton PR #3207. The direct lighting
> implementation was subsequently superseded by D49, then D49 was superseded
> by D50-D52 and Mesa MR !43672. PR #3207 is a draft and is not a mergeable
> final implementation. Current status is maintained in
> [`final-report.md`](final-report.md).

The user approved publication on 2026-08-07. The investigation repository is
public, VKD3D-Proton PR
[#3202](https://github.com/HansKristian-Work/vkd3d-proton/pull/3202) is open,
and updates were posted to VKD3D-Proton #3134 and Proton #9906.

## Correct repository

The code change belongs in **VKD3D-Proton**, because the demonstrated defect is
in the conversion of a D3D12 placed buffer footprint to Vulkan buffer-to-image
copy geometry. It should not be submitted as a duplicate patch to Valve
Proton. Proton consumes VKD3D-Proton as a component and will receive an accepted
fix through a later component update.

Recommended sequence:

1. Review the local commit, patch, test evidence, and selected screenshots.
2. Fork `HansKristian-Work/vkd3d-proton` under the user's GitHub account.
3. Push local branch `fix-buffer-image-block-units`, based on upstream commit
   `84c87c8390d9df75ba41d911496296fe13f0e275`, to that fork.
4. Open a VKD3D-Proton pull request referencing issue #3134 and Steam AppID
   247970.
5. After the pull request exists, add a short update to ValveSoftware/Proton
   #9906 linking to it. Do not duplicate the implementation in Proton.
6. Respond to review and retest any revised commit before it is considered
   final.

## Why propose a general fix first

VKD3D-Proton already converts compressed/uncompressed **image-to-image** copy
geometry through physical blocks and tests `R32G32B32A32_UINT` to/from BC3.
The candidate supplies the corresponding conversion for the demonstrated
placed-buffer-to-image upload direction. The defect is in a shared translation
helper, not in an IL-2 configuration flag, and the candidate contains no
executable name, AppID, resource dimensions, or environment-variable gate.

This does not mean the risk is zero. Public D3D12 documentation describes
texture-copy formats as identical or from a compatible type group; it does not
clearly define the game's `R32G32B32A32_UINT` to `BC3_UNORM` combination. Native
Windows nevertheless accepts the pattern, and VKD3D-Proton already emulates
the corresponding image-to-image reinterpretation. The pull request should
state this compatibility context without claiming that the game call is
unambiguously valid D3D12.

## Impact boundary and regression risk

Current candidate `64ec55e7` changes only placed-buffer-to-image copies for
which the source and destination physical elements have the same byte size
**and** their block width or height differs. The IL-2 case is 16 bytes on each
side: one uncompressed `R32G32B32A32_UINT` texel maps to one 4x4 BC3 block.
Same-block-geometry uploads and copies whose physical element sizes differ
stay exactly on the original path.

Other applications using the same uncommon equal-byte, different-block-
geometry reinterpretation can therefore be affected. The expected effect is
to make those copies match the physical data represented by the footprint,
but a claim of zero cross-game risk would be unjustified.

Evidence currently limiting that risk:

- focused regression: four deterministic failures with the old helper and
  22/22 assertions passing with current candidate `64ec55e7`;
- all native tests selected by `VKD3D_TEST_FILTER=copy`: 6,429,713 executed,
  zero failures, 14 successful todo, one skipped, eight todo, zero bugs;
- existing neighboring compressed-copy tests: 147/147 and 50/50 passing;
- native and MinGW x64 test builds compile, and x64/x86 packages build;
- two gated D07 runs and one clean D08 run repair the terrain;
- D08 reports no device loss, GPU reset/hang, OOM, or new fatal error.

The retained local current-candidate transcripts are
`captures/validation/64ec55e7-focused-copy-test.log` and
`captures/validation/64ec55e7-copy-tests.log`, with SHA-256 values recorded in
[`evidence-pr-scope-refinement.md`](evidence-pr-scope-refinement.md). They are
intentionally ignored by Git; the concise results above are sufficient for the
pull request.

Known limitations:

- the focused test has not yet been run on native Windows hardware;
- the runtime validation covers RADV on one RDNA3 system;
- the inverse image-to-placed-buffer helper was not changed because IL-2 does
  not demonstrate that direction;
- startup/NUMA and menu shimmering are unrelated and remain unresolved.

## If maintainers request narrower scope

Do not begin with an executable override while the general translation fix has
both a regression test and matching upstream precedent. The first narrowing—
requiring different block geometry with equal physical element size—was
implemented in `64ec55e7`. If reviewers still consider the native-Windows
behavior too application-specific, narrow in this order:

1. restrict it to the demonstrated
   `R32G32B32A32_UINT -> BC3_UNORM` upload pair;
2. use an exact `IL2Series.exe` application quirk only as the final fallback.

Each narrower revision must be rebuilt and retested. No format-pair or
application-specific fallback has been implemented in the current candidate.

## Minimal initial pull-request evidence

Keep the submission compact:

- the single clean commit and included regression test;
- one broken/fixed terrain screenshot pair;
- the focused old-versus-fixed result and the copy-suite summary;
- the D07 conversion counts and clean D08 build identity;
- a link to VKD3D-Proton #3134.

Large Proton logs, all diagnostic patches, duplicate screenshots, game files,
prefix data, and unrelated menu/NUMA investigation material should not be
attached to the initial pull request.

## D49 lighting supersession

This section supersedes only the tiled-light submission path. The terrain plan
above remains its historical record, and the startup work remains independent
in Wine.

The D47 `9b6e15be` implementation and patch `0016` remain causal and runtime
evidence, but are no longer the implementation proposed for merging. The D49
design requires two coordinated repositories:

1. **dxil-spirv:** one atomic commit containing the additive typed-UAV atomic
   quirk, compiler-owned analysis and lowering, existing-remapper fallback,
   API version update, shader sources, and generated references.
2. **VKD3D-Proton:** one dependent atomic commit containing the interface
   capability, the `RAW_SSBO && !MUTABLE_TYPE_RAW_SSBO` gate, capability-gated
   dxil-spirv option, exact `IL2Series.exe` and shader
   `0x7cefa1bc80bb4c70` selection, and the final upstream-reachable dxil-spirv
   gitlink if VKD3D-Proton master does not already contain it.

The compiler mechanism is generic. For an eligible scalar 32-bit `I32` or
`U32` atomic on a typed UAV, dxil-spirv temporarily asks the resource remapper
for a raw-buffer binding. Only an SSBO result enables lowering. Failure or
another descriptor type restores the original typed binding and retries. No
public C callback structure grows. The path excludes 64-bit atomics, sparse
operations, non-atomic resources, and the SM 6.6 heap path.

### Draft PR order and dependency

1. Clean and validate the dxil-spirv change first, then open it as a draft PR.
2. Convert existing VKD3D-Proton PR #3207 to a dependent draft. Do not open a
   replacement unless the maintainer requests one.
3. Cross-link the two PRs, but do not point the VKD3D-Proton submodule at a
   commit that exists only in a personal fork.
4. After dxil-spirv merges or the maintainer lands an equivalent, rebase
   VKD3D-Proton on current master and use that upstream-reachable commit.
5. Rerun exact-head validation and mark #3207 ready only after the dependency
   and its gitlink are final.

The initial dxil-spirv PR should explain the generic legality boundary and
test matrix. It should not contain the game executable, AppID, captured shader,
or screenshots. The VKD3D-Proton PR should explain the exact app/hash policy,
capability gate, D44-D47 causal evidence, and D49 before/after result. The
terrain and Wine work should be linked only as separate context.

### D49 readiness evidence and boundary

The retained tool is
`IL2-Korea-D49-CompilerAware-ABISafe-731c4aae`, using VKD3D-Proton base
`731c4aae5991b33f2ddab45d3cb1b4779159bf4b` and dxil-spirv base
`edd8fdf702c3445eb659f2652d04436ed86e4206`.

- The dxil-spirv resources suite passes cleanly.
- The full suite has only the baseline-reproduced
  `control-flow/switch-continue.frag` validator failure.
- Candidate, fallback, and baseline outputs for the exact captured shader
  validate.
- VKD3D-Proton x86-64, x86, and package builds pass.
- The exact remapper harness emits `StorageBuffer` plus `OpAtomicIAdd` with
  capability ON. Capability OFF is byte-identical to the typed baseline.
- D49 runtime provenance is verified. One menu, short-flight, and map run is
  clean for the square blocks and broad flicker while terrain, real lighting,
  and shadows remain correct.

The sandy or film-grain lighting also occurs on native Windows and is excluded
from acceptance criteria. D49 has not received cross-hardware runtime
validation, so neither PR should claim it.

## D50-D52 lighting resolution and revised upstream path

This section supersedes the D49 submission path above. The terrain PR and Wine
startup MR remain independent.

D50 held the minimal shader, pipeline, dispatch, and 87,040-byte buffer fixed
and changed only the view in an R32, R16, R32 sequence. Both R32 runs passed
and only R16 reproduced the workgroup restart on both tested RADV devices. D51
then passed the exact captured shader with a full-size R32 view through both
descriptor backends on both devices.

D52 applied that discriminator in VKD3D-Proton without changing dxil-spirv. It
kept `R32ui`, `OpImageTexelPointer`, and `OpAtomicIAdd`, changed only the exact
resource's descriptor set/binding from `1/1` to `2/0`, and removed the blocks
in two game runs. This proves compiler-side SSBO lowering is unnecessary, but
the D52 application-specific alias and sibling-layout assumptions are not a
merge-ready upstream design.

Maintainer reproduction produced
[Mesa MR !43672](https://gitlab.freedesktop.org/mesa/mesa/-/merge_requests/43672).
It changes RADV's GFX10+ texel-buffer OOB selection from
`STRUCTURED_WITH_OFFSET` to `STRUCTURED`, matching the native AMD D3D12 driver
and pre-GFX10 behavior. NVIDIA also passes the maintainer's descriptor-heap
test. This is the agreed upstream direction.

Revised sequence:

1. Keep dxil-spirv PR #296 and VKD3D-Proton PR #3207 in draft state while the
   maintainer decides their disposition. Do not mark either ready for review.
2. Do not publish D52 as another PR. Retain it as causal evidence only.
3. Follow Mesa MR !43672 and build its accepted revision locally.
4. Test that Mesa revision with stock dxil-spirv and unmodified VKD3D-Proton,
   using the separate OpenMP workaround only if the Wine startup MR is absent.
5. Record the runtime provenance and repeat the visual check. If it passes,
   update the existing VKD3D-Proton and Proton discussions instead of opening
   another graphics issue or PR.

The D49 and D52 work was not merged, so the change in direction caused no
lasting upstream impact. Their tests remain useful because they isolate why
the driver-level Mesa change is the cleaner general solution.
