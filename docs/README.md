# Evidence index

This directory preserves both the final proof and the chronological path used
to reach it. Start with the final material below. Preparation notes, failed
experiments, and superseded interpretations remain available because they
document control quality and prevent accidental repetition.

## Archive state

The active investigation is concluded. Wine MR !11604 merged on 2026-08-10,
and the terrain correction merged through VKD3D-Proton PR #3202. The
dxil-spirv #296 and VKD3D-Proton #3207 lighting PRs were closed unmerged after
D50 through D52 isolated the behavior at the RADV descriptor/view boundary.
Mesa MR !43672 is the preferred general direction and remains open, but this
archive does not contain a local game test of that Mesa change.

For provenance, the D52 source was later forward-ported to the personal
VKD3D-Proton fork as
[`8cd28e8f`](https://github.com/silv3rshi3ld/vkd3d-proton/commit/8cd28e8f98751afe3b85c3b08519464907aa5143)
and linked from a
[follow-up on closed PR #3207](https://github.com/HansKristian-Work/vkd3d-proton/pull/3207#issuecomment-5276514345).
That forward-port passed both architecture builds but was not rerun in the
game and was not submitted as a new merge proposal.

The Wine evidence covers the earlier MR head `e8319c0e` and Valve's equivalent
series. The final rebased Wine head `663fd7cc` was not rerun here. These scope
distinctions are intentional and apply throughout the archive.

## Final conclusions

- [`final-report.md`](final-report.md): final technical report for all three
  compatibility tracks
- [`final-release-notes.md`](final-release-notes.md): reviewed description for
  the concluded evidence archive
- [`evidence-n05-wine-mr-11604.md`](evidence-n05-wine-mr-11604.md): exact Wine
  NUMA series and empty-launch-options Steam validation of the tested pre-merge
  head
- [`evidence-pr-scope-refinement.md`](evidence-pr-scope-refinement.md): final
  terrain PR scope, regression results, and evidence boundary
- [`evidence-d50-d52-r32-alias-result.md`](evidence-d50-d52-r32-alias-result.md):
  current R16-versus-R32 descriptor-boundary proof, VKD3D-Proton-only D52
  discriminator, and disposition in favor of the still-open Mesa MR !43672
- [`evidence-d49-compiler-aware-result.md`](evidence-d49-compiler-aware-result.md):
  superseded paired dxil-spirv and VKD3D-Proton lighting experiment
- [`evidence-u01-upstream-candidate-ab.md`](evidence-u01-upstream-candidate-ab.md):
  historical matched lighting A/B for the superseded first implementation
- [`../patches/README.md`](../patches/README.md): historical
  upstream-submission and diagnostic patch identities

## Startup and processor topology

- [`startup-numa-assessment.md`](startup-numa-assessment.md): diagnosis of the
  OpenMP failure and why a fixed thread count is not a solution
- [`evidence-n04-preparation.md`](evidence-n04-preparation.md): local NUMA
  candidate preparation
- [`evidence-n05-wine-mr-11604.md`](evidence-n05-wine-mr-11604.md): validation
  of the existing general Wine implementation
- [`../probes/numa-openmp-probe.c`](../probes/numa-openmp-probe.c): focused
  topology and OpenMP probe source

## Terrain corruption

- [`evidence-e00-baseline.md`](evidence-e00-baseline.md): controlled baseline
  reproducing missing terrain pages and menu blocks
- [`evidence-d02-bc3-border-copies.md`](evidence-d02-bc3-border-copies.md):
  first concrete compressed-copy lead
- [`evidence-d05-result.md`](evidence-d05-result.md): invalid zero-match run and
  why it was not counted as a negative
- [`evidence-d05c-result.md`](evidence-d05c-result.md): border-only conversion
  result
- [`evidence-d06-result.md`](evidence-d06-result.md): complete-page geometry
  and coverage calculation
- [`evidence-d07-result.md`](evidence-d07-result.md): causal terrain repair
- [`evidence-d08-result.md`](evidence-d08-result.md): clean general-fix runtime
  validation
- [`evidence-pr-scope-refinement.md`](evidence-pr-scope-refinement.md): final
  candidate `64ec55e7` and complete regression suite

## Tiled-light blocks and flicker

- [`evidence-d13-light-grid-trace-result.md`](evidence-d13-light-grid-trace-result.md):
  tiled-light and reflection-pass correlation
- [`evidence-d14-shader-dump-result.md`](evidence-d14-shader-dump-result.md):
  exact tiled-light shader sequence
- [`evidence-d15-light-list-sync-result.md`](evidence-d15-light-list-sync-result.md):
  synchronization hypothesis control
- [`evidence-d16-descriptor-trace-result.md`](evidence-d16-descriptor-trace-result.md):
  descriptor resolution for the affected shaders
- [`evidence-d21-d25-atomic-result.md`](evidence-d21-d25-atomic-result.md):
  standalone atomic reproduction and the incomplete D25 in-game translation
- [`evidence-d33-t7-t8-contract-result.md`](evidence-d33-t7-t8-contract-result.md):
  final allocator descriptor contract
- [`evidence-d44-consecutive-capture-result.md`](evidence-d44-consecutive-capture-result.md):
  adjacent-frame overwrite proof
- [`evidence-d45-correct-ssbo-binding-result.md`](evidence-d45-correct-ssbo-binding-result.md):
  complete SSBO operation and raw descriptor pairing
- [`evidence-d46-allocator-only-result.md`](evidence-d46-allocator-only-result.md):
  invalid inactive-quirk result
- [`evidence-d47-allocator-only-wired-result.md`](evidence-d47-allocator-only-wired-result.md):
  clean allocator-only in-game result
- [`evidence-d49-compiler-aware-result.md`](evidence-d49-compiler-aware-result.md):
  historical compiler-aware result with backend capability gating and typed
  fallback, superseded after D50 through D52 removed the need for alternate
  compiler lowering
- [`evidence-d50-d52-r32-alias-result.md`](evidence-d50-d52-r32-alias-result.md):
  D50 pass/fail/pass view-format control, D51 exact-shader coverage, two clean
  D52 game runs with stock dxil-spirv, and the cleaner Mesa upstream direction
- [`evidence-u01-upstream-candidate-ab.md`](evidence-u01-upstream-candidate-ab.md):
  historically valid one-commit runtime/mechanism A/B, superseded as an
  upstream architecture first by D49 and finally by the D50 through D52
  descriptor-boundary result and Mesa direction

> [!NOTE]
> D25 originally appeared to reject the allocator as the visual cause. Later
> descriptor evidence proved that D25 emitted an SSBO operation but still
> selected the typed descriptor. D45 and D47 completed both required halves and
> supersede that interim causal interpretation. The D25 record is retained as
> an accurate account of what was known at that point.

## Reproduction, environment, and controls

- [`environment.md`](environment.md): sanitized test environment
- [`reproduction.md`](reproduction.md): fixed visual procedure, capture rules,
  and rollback preparation
- [`experiment-matrix.md`](experiment-matrix.md): complete experiment order and
  status
- [`findings.md`](findings.md): append-only chronological findings ledger
- [`development-build.md`](development-build.md): custom Proton and
  VKD3D-Proton build provenance
- [`external-evidence.md`](external-evidence.md): independent issue evidence
  and checksums without redistributing the original artifacts
- [`game-binary-inspection.md`](game-binary-inspection.md): read-only import and
  diagnostic-string evidence

## Negative controls and closed leads

- [`evidence-e01-no-upload-hvv.md`](evidence-e01-no-upload-hvv.md):
  altitude-confounded host-visible-upload control
- [`evidence-e02-single-queue.md`](evidence-e02-single-queue.md): unchanged
  single-queue result
- [`evidence-e03-no-descriptor-buffer.md`](evidence-e03-no-descriptor-buffer.md):
  unchanged mutable-descriptor fallback result
- [`evidence-e05-no-vrs-result.md`](evidence-e05-no-vrs-result.md): unchanged VRS
  control
- [`prior-art-msfs.md`](prior-art-msfs.md): reviewed MSFS precedents and why
  they were not drop-in IL-2 fixes
- [`evidence-map-package-inspection.md`](evidence-map-package-inspection.md):
  missing game texture references excluded as the necessary terrain cause

## Historical planning records

The following files are retained as dated project records, not as current work
instructions:

- [`community-update-draft-2026-08-06.md`](community-update-draft-2026-08-06.md):
  text posted during the early diagnostic handoff
- [`upstream-submission-plan.md`](upstream-submission-plan.md): terrain
  submission plan completed by PR #3202
- [`shimmering-ownership-plan.md`](shimmering-ownership-plan.md): lighting
  ownership and initial publication plan; PR #3207 later received architectural
  feedback, D49 explored a paired implementation, and D50 through D52 isolated
  the driver boundary more precisely. The experimental PRs were subsequently
  closed unmerged

All other `evidence-*-preparation.md` and `evidence-*-result.md` files form the
full dated experiment trail. Their exact order and validity classification are
recorded in [`experiment-matrix.md`](experiment-matrix.md).
