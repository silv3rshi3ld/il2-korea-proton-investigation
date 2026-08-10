# Evidence index

This directory preserves both the final proof and the chronological path used
to reach it. Start with the final material below. Preparation notes, failed
experiments, and superseded interpretations remain available because they
document control quality and prevent accidental repetition.

## Final conclusions

- [`final-report.md`](final-report.md): final technical report for all three
  compatibility tracks
- [`final-release-notes.md`](final-release-notes.md): reviewed description for
  the final evidence snapshot
- [`evidence-n05-wine-mr-11604.md`](evidence-n05-wine-mr-11604.md): exact Wine
  NUMA series and empty-launch-options Steam validation
- [`evidence-pr-scope-refinement.md`](evidence-pr-scope-refinement.md): final
  terrain PR scope, regression results, and evidence boundary
- [`evidence-u01-upstream-candidate-ab.md`](evidence-u01-upstream-candidate-ab.md):
  clean matched lighting A/B against upstream master
- [`../patches/README.md`](../patches/README.md): final and diagnostic patch
  identities

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
- [`evidence-u01-upstream-candidate-ab.md`](evidence-u01-upstream-candidate-ab.md):
  final one-commit upstream candidate and matched A/B

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
  ownership and publication plan completed by issue update and PR #3207

All other `evidence-*-preparation.md` and `evidence-*-result.md` files form the
full dated experiment trail. Their exact order and validity classification are
recorded in [`experiment-matrix.md`](experiment-matrix.md).
