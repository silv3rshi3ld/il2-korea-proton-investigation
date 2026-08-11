# Tiled-light artifact: ownership and upstream plan

Date: 2026-08-10

> [!NOTE]
> Historical completed plan. The root-cause update was posted to
> [VKD3D-Proton issue #3134](https://github.com/HansKristian-Work/vkd3d-proton/issues/3134#issuecomment-5238151028)
> and the allocator-only quirk was published as
> [VKD3D-Proton PR #3207](https://github.com/HansKristian-Work/vkd3d-proton/pull/3207).
> That direct implementation and the later D49 two-repository design are now
> historical evidence. D50-D52 isolated the texel-buffer view/OOB behavior
> without changing dxil-spirv. Mesa MR !43672 is the agreed upstream direction,
> and PR #3207 remains a non-mergeable draft. See
> [`final-report.md`](final-report.md) for the current combined status.

## Final technical conclusion

The main-menu, cockpit, and fire-lit square blocks and broad light flicker are
caused by one malformed tiled-light allocator shader. IL-2 performs a 32-bit
global atomic through an `R16_UINT` typed UAV. Translating that literally as a
typed Vulkan texel-buffer access does not provide a legal equivalent.

Three consecutive affected frames prove the visible mechanism:

- all 50 compute workgroups independently allocate from zero;
- 12,126 requested light references collapse into offsets 0–320;
- depth and tile metadata remain bit-identical;
- 69–107 overwritten light IDs change between adjacent frames.

This produces stable screen-tile boundaries with unstable light membership,
which explains both the square blocks and their broad flicker.

## Minimal compatibility fix

VKD3D-Proton already emits a raw storage-buffer descriptor sibling for the D3D
descriptor. For executable `IL2Series.exe` and exact shader hash
`7cefa1bc80bb4c70`, the fix must do both:

1. lower the typed UAV access as an SSBO operation;
2. select `VKD3D_SHADER_BINDING_FLAG_RAW_SSBO` so that operation addresses the
   raw descriptor rather than the typed texel-buffer binding.

D25 implemented only the first half and remained defective. D44 captured that
incomplete translation selecting the typed descriptor set. D45 implemented
both halves and was clean on two independent starts, but also contained an
earlier diagnostic depth-gate bypass. D46 attempted to remove that bypass but
accidentally removed the executable mapping too, making its result invalid.
D47 restored the mapping and retained only the allocator correction. D47 is
clean with the original depth predicates, lighting, and shadows.

Therefore the depth-gate bypass is not part of the final fix. It only hid how
the malformed light list was presented.

## D49 implementation refinement

D47 proves the minimal behavior that must change, but `9b6e15be` and patch
`0016` no longer represent the intended upstream implementation. D49 places
the generic typed-UAV atomic legalization in dxil-spirv:

1. Only a scalar 32-bit `I32` or `U32` atomic on a typed UAV is eligible.
2. The compiler temporarily presents the resource remapper with a raw-buffer
   kind.
3. Lowering is accepted only if the remapper returns an SSBO descriptor.
4. Failure or another descriptor class restores the original typed kind,
   alignment, and range, then retries the normal typed path.

No public C callback structure is extended. The path excludes 64-bit atomics,
sparse operations, non-atomic typed UAVs, and the SM 6.6 heap path.

VKD3D-Proton retains the compatibility policy and hardware-layout check. It
selects exact executable `IL2Series.exe` and shader hash
`0x7cefa1bc80bb4c70`, and enables the dxil-spirv quirk only when `RAW_SSBO` is
available without `MUTABLE_TYPE_RAW_SSBO`.

## Ownership

Principally, the game should not perform a 32-bit atomic through a 16-bit typed
view. Correct API use remains preferable and the upstream report should say so
plainly.

Practically, compatibility layers routinely emulate narrowly demonstrated
native-driver allowances for shipped games. This case meets that bar:

- native Windows renders correctly;
- legal Vulkan atomic variants pass on both tested RADV GPUs;
- the live `R16_UINT` variant reproduces the malformed allocation;
- an SSBO control repairs the standalone allocation;
- the correctly wired game integration repairs the actual pixels;
- the behavior is scoped to one executable and one shader hash.

The practical compatibility policy remains owned by VKD3D-Proton as a
surgical application and shader selection. The reusable lowering mechanism
belongs in dxil-spirv. This is not a Mesa workaround, Proton launch parameter,
game mod, or custom lighting engine.

That paragraph records the D49 ownership decision before maintainer
reproduction. D50-D52 and Mesa MR !43672 supersede it. The exact shader works
through the existing texel-buffer lowering when supplied an R32 view, so no
generic dxil-spirv legalization is required. RADV's GFX10+ texel-buffer OOB
selection is the remaining compatibility difference from native AMD D3D12 and
pre-GFX10 behavior. The preferred ownership is therefore Mesa/RADV, with no
per-game VKD3D-Proton quirk if MR !43672 is accepted.

## Separate IL-2 tracks

| Track | Mechanism | Upstream path |
| --- | --- | --- |
| Startup without parameters | Wine lacked `GetNumaNodeProcessorMaskEx`; the shipped Intel OpenMP runtime aborts | Existing Wine MR !11604; validated locally without a hard-coded thread count |
| Distant terrain pages | Buffer-to-BC3 copy geometry used source texel units instead of destination block geometry | Existing VKD3D-Proton PR #3202 |
| Square blocks and broad light flicker | Game uses a 32-bit atomic through an `R16_UINT` view; RADV GFX10+ OOB selection exposes a malformed tiled-light allocation | Mesa MR !43672; retain VKD3D-Proton PR #3207 only as draft investigation evidence |

The fine sandy or film-grain lighting is present on native Windows and is not a
Proton defect.

## Publication sequence

1. Keep terrain PR #3202 separate and allow its review to proceed.
2. Add one concise follow-up to existing VKD3D-Proton issue #3134, whose
   original report already mentions the menu squares, and note that #3202 fixes
   only terrain.
3. Include compact evidence and hashes; do not upload RenderDoc captures,
   shader binaries, game assets, prefixes, or giant unfiltered logs.
4. Allow a reasonable interval for maintainer feedback.
5. Rebase the one-commit allocator fix on then-current VKD3D-Proton master,
   rebuild, and open one PR referencing #3134.
6. After the PR exists, prepare short updates for Proton #9906, the original
   affected users, and this investigation repository.

## Current two-repository publication sequence

The preceding sequence records how the D47 result reached PR #3207. D49 now
requires this order:

1. Submit the generic compiler mechanism and its shader/reference tests to
   dxil-spirv as a draft PR.
2. Convert VKD3D-Proton PR #3207 to a dependent draft and preserve its causal
   and before/after evidence.
3. Do not point VKD3D-Proton at a dxil-spirv commit available only from a
   personal fork. Wait for an upstream-reachable merged or maintainer-landed
   commit.
4. Rebase the VKD3D-Proton integration, update its dxil-spirv gitlink if
   needed, and rerun x86-64, x86, package, exact-shader, remapper, and game
   checks.
5. Mark PR #3207 ready only after that dependency and exact-head validation
   are complete.

## Historical D47 upstream scope gate

The proposed code may contain only:

- one new typed-UAV-as-SSBO shader quirk;
- raw descriptor-sibling selection under that quirk;
- the `IL2Series.exe` entry and allocator shader hash.

It must not contain:

- either depth-producer hash or the D38 SPIR-V rewrite;
- a processor/thread-count constant;
- a launch option, environment-variable requirement, game-file patch, or Mesa
  workaround;
- captured shaders or RenderDoc files.

The issue-comment and PR drafts live locally for review.

## D49 upstream scope gate

The dxil-spirv change may contain the additive quirk, generic typed-atomic
analysis and lowering, safe remapper fallback, and focused positive and
negative reference tests. It must not contain an IL-2 executable name, AppID,
captured game shader, or game-specific hash.

The VKD3D-Proton change may contain the interface capability bit, the
`RAW_SSBO && !MUTABLE_TYPE_RAW_SSBO` gate, capability-gated compiler option,
exact `IL2Series.exe` entry, exact allocator hash, and an upstream-reachable
dxil-spirv gitlink. It must not contain callback-structure growth, either depth
producer rewrite, a broad shader override, or unrelated terrain and Wine
changes.

D49 tool `IL2-Korea-D49-CompilerAware-ABISafe-731c4aae` passed the clean
dxil-spirv resources suite, exact-shader and remapper checks, VKD3D-Proton
x86-64 and x86 builds, packaging, and one verified-loaded reporting-host run.
The only full translator-suite failure reproduces on the base at
`control-flow/switch-continue.frag`. No cross-hardware D49 result is claimed.

## Current D50-D52 resolution

D50 closes the buffer-size control gap with one 87,040-byte allocation and an
R32, R16, R32 view sequence. Only R16 fails on both tested RADV devices. D51
passes the exact captured game shader and full-size R32 alias through both
descriptor backends on both devices. D52 then selects an R32 texel-buffer
sibling for only the exact IL-2 shape and shader while leaving dxil-spirv at
`cc75a0c9`. Its DXIL is unchanged, its SPIR-V retains the natural texel-buffer
atomic, and two game runs are visually clean.

D52 is diagnostic and not merge-ready. It makes sibling-layout assumptions
and encodes an application-specific descriptor policy. Maintainer testing
instead produced Mesa MR !43672, which switches RADV's GFX10+ texel-buffer OOB
selection from `STRUCTURED_WITH_OFFSET` to `STRUCTURED`. This matches native
AMD D3D12 and pre-GFX10 behavior; NVIDIA also passes the maintainer's test with
a descriptor heap.

The current sequence is therefore:

1. Keep dxil-spirv PR #296 and VKD3D-Proton PR #3207 as drafts or close them as
   superseded after maintainer guidance. Do not present either as merge-ready.
2. Follow Mesa MR !43672 review and test its accepted revision locally with
   unmodified dxil-spirv and VKD3D-Proton.
3. If that clean control passes, update the existing reports with the result.
   Do not open another VKD3D-Proton graphics PR.

The detour caused no lasting upstream change because neither draft was merged.
It supplied the A/B evidence that isolated the descriptor-format boundary and
helped connect the game symptom to the driver behavior.
