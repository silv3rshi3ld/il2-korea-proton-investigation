# Root-cause assessment

## Current conclusion

The terrain defect is caused by a block-unit conversion missing from
VKD3D-Proton's buffer-to-image `CopyTextureRegion` path. The game supplies
placed `DXGI_FORMAT_R32G32B32A32_UINT` footprints and copies them into ordinary
placed `DXGI_FORMAT_BC3_UNORM` baked-terrain caches. Both physical elements are
16 bytes, so each source texel represents one 4x4 BC3 block. D3D12's buffer
layout is described in source-format texels, while Vulkan requires the buffer
layout and image extent in destination image texels.

The old `vk_buffer_image_copy_from_d3d12()` path carried the source dimensions
through unchanged. D07 converted all 522 observed page interiors and borders:
178 `64x64` interiors became `256x256`, 182 horizontal borders became
`256x4`, and 162 vertical borders became `4x256`, with zero rejects. Terrain at
approximately 5,500 m changed from mostly absent rectangular pages with
magenta seams to continuous detailed terrain. This is a causal result, not a
visual analogy.

The public D3D12 documentation does not clearly list this BC3/RGBA32_UINT
reinterpret pair among its compatible format groups. Native Windows and
VKD3D-Proton's existing image-to-image path nevertheless support the same
physical-block interpretation. The safest ownership statement is therefore a
VKD3D-Proton native-compatibility gap, not a demonstrated RADV defect. The
general `cf11ba76` predecessor passes its regression tests and D08 validates it
in game without any IL-2-specific filter or diagnostic gate. Current PR
candidate `64ec55e7` further restricts activation to equal-sized physical
elements with different block dimensions, leaving same-geometry copies on the
original path while selecting unchanged conversion arithmetic for IL-2.

The startup/OpenMP problem remains an independent Wine/NUMA investigation.
The menu/cockpit/fire-lit squares are also independent. D20 found overlapping
tiled-light index ranges. D21-D24 traced that data pattern to the game's
32-bit atomic through a live `R16_UINT` typed UAV, but D25 only changed the
shader access class and left it mapped to the typed descriptor binding. D26 then
suppressed only the two pixel shaders' tiled dynamic-light loop and completely
removed the grid while preserving the scene. D27 retained the real tile counts
and complete loop but substituted valid light ID 1; the verified run was also
clean. D28–D30 remained defective while progressively replacing higher records
with the zero sentinel. D31 then preserved every real nonzero record but
replaced only zero with safe record 1; the squares still remained. D32 then
kept genuine record 2 and mapped every other entry to safe record 1; the
squares remained without skipped iterations. D33 validates record 2's live
view, bounds, and finite data; D34 excludes the returned shadow visibility;
D35 evaluates record 2 exactly once without the real tile list and removes the
original squares. The remaining boundary is the real tile-dependent
membership/evaluation mask. D36 retains the original consumers, shadows, and
all real lights while both producers include record 2 in every valid tile;
the blocks return. Record 2's boundary alone is therefore insufficient. The
remaining boundary was a broader nontrivial light class or interaction between
genuine records. D37 then excluded non-finite reciprocal-square-root results.
D38 preserved the real producer geometry, IDs, consumer loops, per-light math,
and shadows but bypassed the shared packed-mask and scalar tile-depth rejection;
the large blocks disappeared. D39 retained the scalar depth overlap and D40
retained the packed mask overlap; each independently restored the blocks.
This made the shared tile-depth optimization look causal. D44 then supplied
the missing temporal evidence: three consecutive affected D42 frames have
bit-identical packed depth and tile metadata, while all 50 workgroups still
reuse offsets 0–320 for 12,126 requested light IDs. Between adjacent frames,
69–107 of those overwritten IDs change. The capture also proves why D25 was
not a valid allocator negative: its SPIR-V uses `StorageBuffer`, but it still
addresses the typed descriptor set rather than VKD3D-Proton's emitted raw SSBO
sibling. The malformed global list is therefore the first demonstrated
unstable boundary and explains the temporal instability. D45 completes that
binding selection, retains the exact D38 depth-gate behavior, and is clean on
two independent starts. D46 appeared to retain only the allocator correction,
but later source review proves its `IL2Series.exe` mapping was absent and the
quirk never activated. D46 is invalid for minimality. Correctly wired D47 is
clean with the original depth predicates, proving the allocator correction is
sufficient. The fine sandy/film-grain lighting is also present on Windows and
is normal game rendering, not a Proton defect. D50 through D52 later isolate
the remaining cause more precisely: the decisive variable is the texel-buffer
view and its out-of-bounds behavior, not SSBO lowering.

## Historical D49 implementation

D47 and candidate `9b6e15be` remain causal and runtime proof that correcting
the allocator access removes the blocks and broad flicker. They no longer
describe the proposed upstream implementation. D49 in turn superseded the
direct VKD3D-Proton quirk as an experiment, but D50 through D52 show that D49
is not the required final architecture.

The D49 design assigns generic translation to dxil-spirv. For an eligible
scalar 32-bit `I32` or `U32` atomic on a typed UAV, the compiler temporarily
presents the binding to the existing resource remapper as a raw buffer. It
enables raw lowering only when that remap succeeds with an SSBO descriptor. If
the remap fails or returns another descriptor class, the compiler restores the
original typed kind, alignment, and range and retries the normal typed path.
No public C callback structure grows.

The lowering excludes 64-bit atomics, sparse operations, non-atomic typed
UAVs, and the SM 6.6 heap path. VKD3D-Proton remains responsible for policy:
it selects exact executable `IL2Series.exe` and exact shader hash
`0x7cefa1bc80bb4c70`, and requests the compiler quirk only when `RAW_SSBO` is
available without the mutable single-descriptor `MUTABLE_TYPE_RAW_SSBO`
layout.

D49 uses VKD3D-Proton base
`731c4aae5991b33f2ddab45d3cb1b4779159bf4b`, dxil-spirv base
`edd8fdf702c3445eb659f2652d04436ed86e4206`, and retained tool
`IL2-Korea-D49-CompilerAware-ABISafe-731c4aae`. The dxil-spirv resources suite
passes cleanly. The only full-suite failure is the baseline-reproduced
`control-flow/switch-continue.frag` validator failure. Candidate, fallback, and
baseline outputs for the exact captured shader validate. VKD3D-Proton x86-64,
x86, and package builds pass. The exact remapper harness emits
`StorageBuffer` plus `OpAtomicIAdd` with capability ON, while capability OFF is
byte-identical to the typed baseline.

Runtime provenance verifies that D49 loaded. One reporting-host run covering
the menu, a short flight, and the map retained correct terrain, real lighting,
and shadows while the blocks and broad flicker were absent. This is not a
cross-hardware validation claim. This successful result established a useful
control, but did not prove that dxil-spirv or SSBO lowering was necessary.

## D50 through D52 root-cause refinement

D50 removed the D49 confounder by keeping the same shader operation, buffer,
87,040-byte range, dispatch, and coordinate-zero 32-bit atomic while switching
only the storage texel-buffer view in the order `R32_UINT`, `R16_UINT`,
`R32_UINT`. The first and second R32 runs were globally correct. The intervening
R16 run reproduced the per-workgroup restart and corrupt allocation. The
repeat rules out run ordering as the explanation.

D51 used the exact captured shader and an 87,040-byte R32 alias. It passed on
both the mutable descriptor-set and descriptor-buffer paths without
out-of-range writes. D52 then applied that discriminator in VKD3D-Proton while
leaving dxil-spirv at unmodified gitlink
`cc75a0c98d34d7bcc03560527c799b52e48b4d1f`. The DXIL remained byte-identical,
and the translated shader retained `R32ui`, `OpImageTexelPointer`, and
`OpAtomicIAdd`. Only the selected sibling binding changed. A runtime marker
confirmed creation of the exact R32 alias, and two D52-r2 game runs remained
free of the square blocks.

The causal conclusion is therefore narrower than the D49 hypothesis. IL-2
performs a 32-bit atomic through an `R16_UINT` UAV, and the observed behavior
depends on how the driver handles the resulting texel-buffer access beyond the
16-bit element. An SSBO and a dxil-spirv change are not inherently required.
D52 is an exact diagnostic quirk, not the preferred upstream implementation.

Mesa MR [!43672](https://gitlab.freedesktop.org/mesa/mesa/-/merge_requests/43672)
is the current upstream direction. It makes GFX10 and later RADV prefer
`STRUCTURED` rather than `STRUCTURED_WITH_OFFSET` out-of-bounds selection for
texel-buffer descriptors. This matches AMD's native D3D12 driver and pre-GFX10
behavior, and provides a general compatibility behavior without a per-game
VKD3D-Proton quirk. D50 through D52 support its diagnosis, but the Mesa change
has not yet been tested locally and remains subject to upstream review.

Both D52 runs used `OMP_NUM_THREADS=16 KMP_AFFINITY=disabled` only for the
independent Wine startup issue. D52 intentionally excluded terrain PR #3202,
so it does not constitute a combined all-fixes build. See
[`evidence-d50-d52-r32-alias-result.md`](evidence-d50-d52-r32-alias-result.md).

## Ranked graphics assessment

| Rank | Mechanism | Evidence | Confidence |
|---:|---|---|---|
| 1 | Missing block-unit conversion on buffer-to-BC3 terrain-page copies | D07 adjusted 522/522 exact-class copies in run 1 and 304/304 in run 2, with zero rejects, and repaired high-altitude terrain both times; clean general-build D08 repeats the repair; the focused synthetic test fails four assertions on the old path and passes 22/22 with the narrowed general fix | High; causal for terrain and addressed by current PR candidate `64ec55e7` |
| 2 | IL-2 performs a 32-bit atomic through an `R16_UINT` UAV, and RADV's GFX10+ texel-buffer out-of-bounds selection exposes a different result from native AMD | D44 captures the corrupted allocation. D50 changes only R32/R16/R32 view format and fails only for R16. D51 passes the exact shader through both descriptor backends with an R32 alias. D52 keeps stock dxil-spirv and is clean twice. | High for the root cause on the reporting host; Mesa MR !43672 is the preferred upstream direction but is not yet locally validated |
| 3 | The producers' packed-mask/scalar tile-depth rejection changes presentation of the malformed light list | D38 can hide blocks while retaining lighting; D39/D40 restore them. D47 retains the original predicates but is clean once the allocator is corrected. | Diagnostic presentation factor only; rejected from the final fix |
| 4 | Game texture-provider fallback contributes secondary missing inputs | The successful D07 run still logs missing summer/common inputs, proving they are not required for the rectangular terrain failure | Low as a remaining contributor |
| 5 | RADV mishandles otherwise valid Vulkan terrain copies or legal global typed-buffer atomics | The same driver renders terrain correctly when VKD3D emits converted copy geometry; D21/D22 pass legal global atomic forms on both RADV GPUs. The remaining lighting behavior is reached only through the game's mismatched R16 view and is a native-compatibility choice, not a general failure of legal atomics. | Very low for terrain and rejected for legal atomics; Mesa MR !43672 addresses compatibility behavior for the invalid application access |

## Terrain model established by files and traces

```text
Korea map packages (800 m texture quads, 5 LODs)
  -> BlocksCache / BakedTerrain CPU work after mission transition
  -> page producer/intermediate resource
  -> placed 2048x2048 one-mip BC3 cache pool
  -> SRV/descriptor selection plus shader LOD/page index
  -> existing terrain mesh samples selected cache page
```

The loading display moving from 25% to 26% does not establish premature
completion. The engine continues cache work after entering the mission, and
the progress number can describe only the current loading phase. See
[`evidence-map-package-inspection.md`](evidence-map-package-inspection.md).

High altitude selects distant baked pages, exposing much more empty or wrong
content. Below roughly 1,500 m, local-detail pages and vegetation become
eligible, so more content appears without fixing the underlying cache path.

## Proven exclusions and weakened leads

- D3D12 reserved/tiled resources: zero relevant API calls.
- Separate compute/transfer queues: two `single_queue` runs are unchanged.
- Descriptor-buffer implementation alone: disabling it is visually unchanged.
- Placed-resource range aliasing: no overlap in the covered D03 class.
- Broad missing mip chains or SRV minimum LOD: D02 finds complete chains and
  clamp zero for the covered resources.
- Current-upstream shader translator: D04 is unchanged.
- The thin reinterpret borders as a complete explanation: D05c changes every
  encountered border candidate and visuals remain unchanged. D07 proves that
  full interiors plus borders are required.
- Split `END_ONLY` barriers: 40,408 warnings remain in the successful D07 run.
- The logged missing Korea terrain inputs as the primary cause: the same
  fallbacks remain in the successful D07 run.
- A wholesale missing map archive: every Maps1-6 file tree was extracted and
  multi-gigabyte content was present.
- Wine WIC scaler mode 3 as an abort: Wine logs the unsupported interpolation
  mode but falls back to nearest-neighbour and returns success.

## Missing files are real, but causality is limited

Static backend inspection shows that a `FAILED load: requested (fallback)`
line means both lookups failed and a default-white texture is retained. Package
inspection confirms the six tested autumn terrain paths are absent. However,
nearly the same absent-reference set exists in all Korea seasons, including
the summer configuration, while native Windows is reported to render the same
Windows build correctly. These references may be optional or stale. Without a
matched Windows `tex.log`, they cannot be promoted to the Linux root cause.

## Decision and next discriminator

Do not add an application override or run more unrelated terrain flags. D08
validates the general `vk_buffer_image_copy_from_d3d12()` conversion at
predecessor `cf11ba76` without the IL-2 resource/shape filter or
`VKD3D_IL2_BC3_PAGE_COPY`. Current PR commit `64ec55e7` preserves that IL-2
branch while returning all same-block-geometry copies to the original path;
its focused and full copy tests pass.

For the separate shimmering defect, D44 invalidates D25 as a causal negative:
its intended shader access translation is present, but the forced SSBO still
selects the typed descriptor binding. D26
proves the visual defect requires the tiled dynamic-light loop in
`df0bd777fd1bb89d` and `a2d104d5c813322e`, not merely a later composition
 stage. D27 further proves that the real `t10` IDs or records they select are
required while record 1/common loop math is safe in the control. D31 excludes
the zero/sentinel skip path as necessary. D32 keeps genuine record 2, replaces
every other entry with record 1, and remains defective, proving record 2
sufficient. D33 resolves every target `t7`-`t10` lookup, excludes the apparent
adjacent descriptor reversal and an out-of-bounds record 2, and identifies
record 2's shadow-enabled spotlight path as the next causal boundary. D34
forces the returned shadow visibility to fully lit and the squares remain,
excluding that comparison/filter result. D35 then evaluates record 2 exactly
once independently of the real tile list; the original squares disappear and
the contribution becomes smooth. The real tile-dependent membership/evaluation
mask is therefore required. D36 makes record 2 global while restoring all
other genuine records and the blocks return, rejecting record 2's boundary as
the complete explanation. D37 excludes non-finite `rsqrt` behavior. D38 then
removes the large blocks while preserving real lighting and shadows by
retaining valid producer geometry and bypassing the common tile-depth
rejection. D39 and D40 prove that retaining either depth component changes the
visible presentation. D44 then locates the earlier unstable boundary in the
overwritten light-index list. D45 selects the raw SSBO descriptor sibling
under the exact allocator quirk, retains the two exact depth-gate bypasses, and
passes twice with empty Steam launch options. D46 was meant to remove only
those depth bypasses, but source review proves its executable-to-quirk mapping
was absent. Correctly wired D47 is clean with the original depth predicates,
proving the allocator correction is the final minimal behavioral requirement.
D49 was the next successful implementation experiment, but D50 and D51 prove
that changing only the texel-buffer view to R32 is sufficient with the natural
typed-buffer shader. D52 confirms this in game with unmodified dxil-spirv and
two clean runs. The current upstream direction is Mesa MR !43672, which matches
native AMD and pre-GFX10 texel-buffer out-of-bounds selection without either
compiler lowering or an IL-2 VKD3D-Proton quirk. Accept the native
sandy/film-grain lighting and reject either large square blocks or broad
non-native flicker. The next decisive validation is an unmodified
VKD3D-Proton/dxil-spirv run on a Mesa build containing MR !43672. See
[`evidence-d45-correct-ssbo-binding-result.md`](evidence-d45-correct-ssbo-binding-result.md)
[`evidence-d47-allocator-only-wired-result.md`](evidence-d47-allocator-only-wired-result.md),
and
[`evidence-d50-d52-r32-alias-result.md`](evidence-d50-d52-r32-alias-result.md).
