# Experiment matrix

All graphics experiments retain `OMP_NUM_THREADS=16` and
`KMP_AFFINITY=disabled`. Each row requires two runs with the same scene and
settings. Until both runs exist, the classification remains **inconclusive**.

Allowed final values: **fixed**, **improved**, **unchanged**, **regressed**,
**inconclusive**.

## Graphics controls

| ID | Single changed variable | Run 1 | Run 2 | Starts | Menu aircraft | Flickering squares | Terrain loading | Distant terrain | Performance | Classification | Notes/evidence |
|---|---|---|---|---|---|---|---|---|---|---|---|
| E00 | Baseline; no VKD3D diagnostic option | [completed](evidence-e00-baseline.md) | [completed](evidence-e00-baseline.md) | yes | blocks/artifacts present both runs | present both runs (user-observed) | severe tiled failure both runs | black/absent both runs | no material difference observed; stills only | unchanged | Repeatable baseline; no device loss/OOM; counts differ with run duration |
| E01 | `VKD3D_CONFIG=no_upload_hvv` | [completed](evidence-e01-no-upload-hvv.md) | [completed](evidence-e01-no-upload-hvv.md) | yes | inconclusive | inconclusive | more vegetation at low altitude; control effect unproven | still severely broken | no material change reported; stills only | inconclusive | Upload change confirmed twice, but altitude confounds visual comparison; matched-altitude A/B required |
| E02 | `VKD3D_CONFIG=single_queue` | [completed](evidence-e02-single-queue.md) | [completed](evidence-e02-single-queue.md) | yes | unchanged | unchanged | unchanged | unchanged | no material change reported; stills only | unchanged | Async compute/transfer queues disabled; both runs unchanged; no device loss/OOM |
| E03 | `VKD3D_DISABLE_EXTENSIONS=VK_EXT_descriptor_buffer` | not run | not run | — | — | — | — | — | — | inconclusive | Runtime testing ended after E02; no result is claimed |
| E04 | `VKD3D_CONFIG=no_upload_hvv,single_queue` | not run | not run | — | — | — | — | — | — | inconclusive | Runtime testing ended after E02; combination was not evaluated |

## Development controls

| ID | Single changed variable | Status | Required observation | Classification |
|---|---|---|---|---|
| D00 | Prefix copy of locally compiled, otherwise unmodified VKD3D-Proton at `3dfc6f07…` | [invalid](evidence-d00-local-build.md) | Stock Proton recopies packaged DLLs at launch; runtime local-build parity was not established | inconclusive |
| U00 | Game build `24596901`; Proton-supplied VKD3D at `3dfc6f07…` | [completed](evidence-u00-game-update.md) | Same menu and terrain corruption; update did not change the defect | unchanged |
| D01a | Prefix copy plus `VKD3D_IL2_RESOURCE_TRACE=1`; trace-only build at `d0b4421f…` | [invalid](evidence-d01-invalid-prefix-install.md) | No trace marker; post-run hashes exactly match packaged Proton DLLs | inconclusive |
| D01b | Dedicated custom Proton plus the same trace build | [completed](evidence-d01b-sparse-trace.md) | Valid marker and diagnostic hashes; zero reserved-resource or tile-mapping calls | unchanged visually; sparse path excluded |
| D02 | Ordinary texture, mip-view, upload-copy, and lifetime census at local commit `54797ad3…` | [completed](evidence-d02-ordinary-texture-trace.md) | Valid marker/hashes; 2,355 complete compressed mip chains, zero partial, and 433 SRV-bearing BC3 textures without a logged incoming upload/copy | unchanged visually; resource class narrowed |
| D03 | Placed-resource heap ranges, alias barriers, and descriptor-use correlation | planned | Determine whether D02's 433 no-upload SRV resources overlap buffers, are intentionally aliased, and reach a shader-visible descriptor | inconclusive |

For each completed cell, link a run directory or curated evidence file and
record new warning/error fingerprints from `compare-logs.py`.

See [`altitude-observation.md`](altitude-observation.md). If testing resumes,
E03 and later runs should use matched captures below 1,500 m and near 5,000 m
within each run.

## Startup / NUMA controls

These are a separate batch. Do not interleave them with graphics comparison
runs because a startup failure cannot evaluate rendering.

| ID | Launch environment | Prior observation | Repeatability | Classification | Evidence needed |
|---|---|---|---|---|---|
| N00 | No OpenMP override | Loading failure near 60%; `GetNumaNodeProcessorMaskEx` implicated | User-reported; not repeated in this round | inconclusive | Clean log plus caller/arguments/return |
| N01 | `KMP_AFFINITY=disabled` only | Not yet isolated | Not run | inconclusive | Establish whether affinity bypass alone starts |
| N02 | `OMP_NUM_THREADS=16` only | Not yet isolated | Not run | inconclusive | Establish whether thread count alone starts |
| N03 | Both current variables | Starts successfully | Confirmed throughout six graphics runs | improved | Isolate the two variables before attributing the result |

Do not repeatedly trigger N00 if it risks corrupting state. Back up the prefix
first, and stop after enough evidence exists to identify the calling module.

## Run record template

Copy this block into `captures/runs/<run-id>/observations.md` (ignored by Git)
or into a curated small evidence file:

```text
Run ID:
UTC start/end:
Game build ID:
Proton version:
VKD3D-Proton commit:
DXVK commit:
Mesa/RADV version:
Kernel:
Launch options (exact):
Prefix backup ID:
Graphics settings hash/description:
Mission/camera protocol:
Started: yes/no
Menu aircraft: fixed/improved/unchanged/regressed/inconclusive
Flickering squares: fixed/improved/unchanged/regressed/inconclusive
Terrain loading: fixed/improved/unchanged/regressed/inconclusive
Distant terrain: fixed/improved/unchanged/regressed/inconclusive
Performance delta:
GPU hang/reset: yes/no
New warnings/errors:
Screenshot/video filenames and SHA-256:
Notes:
```
