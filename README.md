# IL-2 Korea Proton compatibility investigation

This repository contains the reproducible evidence behind three independent
compatibility findings for **Korea. IL-2 Series** (Steam AppID 247970): startup
through Wine, terrain uploads through VKD3D-Proton, and tiled-light allocation
at the D3D12-to-Vulkan driver boundary.

> [!IMPORTANT]
> This is an investigation and upstream evidence repository, not a game mod.
> No game files were modified. The Wine startup series was validated without
> launch parameters, but the isolated D52 lighting test deliberately excluded
> that Wine work and therefore still used the OpenMP startup workaround. A fix
> being proven here does not mean it is already available in standard Proton.

## Status at a glance

| Problem | Proven cause | Upstream path | Status on 2026-08-11 |
| --- | --- | --- | --- |
| Startup abort or need for OpenMP launch options | Wine's public NUMA queries did not expose the processor topology already known internally | [Wine MR !11604](https://gitlab.winehq.org/wine/wine/-/merge_requests/11604) | Exact six-commit series validated with empty Steam launch options. The same series is in [Valve's Wine fork](https://github.com/ValveSoftware/wine/compare/c3007e6f2a36914cc55301eb5efd067707bf8bb1...99166a7e25b08ccef0168217540542260eaed76f) and the [Proton Bleeding Edge source branch](https://github.com/ValveSoftware/Proton/commit/d28e7f2c40da279452db93897c5b9c2c84356fac), but the Wine MR remains open and standard Proton branches were still pinned before it at the final check |
| Missing or magenta terrain pages | Placed-buffer geometry stayed in uncompressed source texels instead of being converted to BC3 destination block geometry | [VKD3D-Proton PR #3202](https://github.com/HansKristian-Work/vkd3d-proton/pull/3202) | General three-file fix with a regression test, merged as `731c4aae`, and independently confirmed on another system |
| Flashing square lighting blocks | The game issues a 32-bit atomic through an `R16_UINT` UAV view; D50 and D51 isolate the view format as the failing boundary on the tested RADV stack | [Mesa MR !43672](https://gitlab.freedesktop.org/mesa/mesa/-/merge_requests/43672), which aligns RADV's out-of-bounds component selection with native AMD D3D12 and pre-GFX10 behavior | A VKD3D-Proton-only R32 alias removed the blocks in two D52 runs with stock dxil-spirv. That candidate is diagnostic and superseded as an upstream direction by the cleaner Mesa change |

The earlier community summary is in
[Proton issue #9906](https://github.com/ValveSoftware/Proton/issues/9906#issuecomment-5238316414).
The current lighting conclusion and D50-D52 results are recorded in the
[latest PR #3207 update](https://github.com/HansKristian-Work/vkd3d-proton/pull/3207#issuecomment-5256360847).
The complete technical conclusions, exact hashes, and evidence boundaries are
in [`docs/final-report.md`](docs/final-report.md).

## Public handoff

- [Consolidated Proton status](https://github.com/ValveSoftware/Proton/issues/9906#issuecomment-5238316414)
- [Wine/NUMA startup validation](https://github.com/ValveSoftware/Proton/issues/9906#issuecomment-5218434565)
- [VKD3D-Proton tiled-light root-cause report](https://github.com/HansKristian-Work/vkd3d-proton/issues/3134#issuecomment-5238151028)
- [Terrain PR #3202](https://github.com/HansKristian-Work/vkd3d-proton/pull/3202)
- [Initial lighting PR #3207 and maintainer review](https://github.com/HansKristian-Work/vkd3d-proton/pull/3207)
- [Proposed general RADV correction in Mesa MR !43672](https://gitlab.freedesktop.org/mesa/mesa/-/merge_requests/43672)
- [D50 through D52 descriptor-boundary result](docs/evidence-d50-d52-r32-alias-result.md)
- [Early diagnostic handoff from 2026-08-06](https://github.com/silv3rshi3ld/il2-korea-proton-investigation/releases/tag/handoff-2026-08-06)

## What the investigation proved

### Startup without hard-coded processor values

The game ships Intel's OpenMP runtime. During initialization it asks Windows
which processors belong to each NUMA node. Wine detected the host topology but
returned an unimplemented result through the queried public API. OpenMP treated
that result as fatal.

The exact six commits from Wine MR !11604 expose the runtime topology through
the missing APIs. A full Steam launch succeeded with an empty launch-options
field, no `KMP_*`, `OMP_*`, or `WINE_CPU_TOPOLOGY` setting, and affinity
controls allowing 1, 2, 4, 8, and 16 CPUs. OpenMP reported the corresponding
available count in every case. No CPU vendor, game, AppID, or thread count is
encoded in the implementation.

See [`docs/evidence-n05-wine-mr-11604.md`](docs/evidence-n05-wine-mr-11604.md).

### Terrain-page corruption

IL-2 uploads a `64x64 R32G32B32A32_UINT` placed footprint as the physical data
for a `256x256 BC3_UNORM` terrain page. Both physical elements are 16 bytes, so
one source texel represents one 4x4 BC3 block. VKD3D-Proton retained the source
geometry and populated only one sixteenth of the destination page.

The merged general fix converts equal-sized physical elements through their
block geometry. It contains no IL-2 executable or AppID check. A gated causal
test adjusted 522 of 522 observed page and border copies with zero rejects; a
clean build restored the terrain at multiple altitudes. The PR artifact was
also [confirmed by another user](https://github.com/ValveSoftware/Proton/issues/9906#issuecomment-5237490788).

The images below are representative observations from different controlled
runs and viewpoints. They are not presented as a matched frame-for-frame A/B.

| Corrupted baseline | Repaired by the general copy fix |
| --- | --- |
| ![Missing terrain pages and magenta seams](captures/curated/e00-baseline/E00-r2-terrain-external-missing-pages-magenta-seams.png) | ![Continuous terrain with the D08 general fix](docs/images/terrain-repaired-d08-742m.png) |

See [`docs/evidence-d08-result.md`](docs/evidence-d08-result.md) and
[`docs/evidence-pr-scope-refinement.md`](docs/evidence-pr-scope-refinement.md).

### Tiled-light blocks and broad flashing

The game's `ComputeLightsFirstRef` shader performs a 32-bit atomic through an
`R16_UINT` typed UAV. All 50 workgroups reused offsets 0 through 320 while the
frame requested 12,126 references. Between adjacent frames, 69 to 107
overwritten light IDs changed, explaining both the rectangular screen tiles
and their broad flicker.

D50 removed a remaining ambiguity in the earlier SSBO experiments by changing
only the Vulkan view format. With the same 87,040-byte buffer, range, shader,
and coordinate-zero atomic, the sequence `R32_UINT`, `R16_UINT`, `R32_UINT`
passed, failed, and passed. D51 then ran the exact captured game shader through
an 87,040-byte `R32_UINT` alias and passed through both the mutable
descriptor-set and descriptor-buffer paths. These tests isolate the R16 versus
R32 descriptor/view boundary without requiring different shader lowering.

D52 translated that finding into a narrowly selected VKD3D-Proton diagnostic.
It left dxil-spirv at stock commit `cc75a0c9`, preserved the shader's natural
R32ui texel-buffer `OpImageTexelPointer` and `OpAtomicIAdd`, and selected an
R32_UINT alias only for the exact executable, shader, resource, and UAV shape.
Two game runs were free of the blocks. D52 used the OpenMP launch workaround
because its isolated Proton base excluded the separate Wine fix, and it also
excluded the terrain fix. No D52 screenshot was captured.

The R32 alias is evidence, not the proposed upstream implementation. Hans'
[Mesa MR !43672](https://gitlab.freedesktop.org/mesa/mesa/-/merge_requests/43672)
provides the cleaner general direction by making RADV's out-of-bounds component
selection match native AMD D3D12 and pre-GFX10 behavior. NVIDIA already passes
the relevant descriptor-heap test. This investigation agrees with resolving
the compatibility behavior in Mesa instead of carrying either the earlier
dxil-spirv lowering or a per-game VKD3D-Proton alias quirk. Review and a final
test of the Mesa change on an otherwise unmodified stack remain outstanding.

The fine sandy or film-grain lighting visible in motion is also present on
native Windows and is not part of this defect.

The following historical matched A/B established the required runtime behavior.
Its one-commit VKD3D-Proton implementation was later superseded by the
compiler-aware D49 architecture, but the images remain valid mechanism and
visual evidence. Both U01 tools use the same NUMA-capable Wine base and differ
only in their four VKD3D-Proton DLLs.

| Before: unmodified master `84c87c83` | After: candidate `9b6e15be` |
| --- | --- |
| ![Flashing tiled-light blocks on upstream master](docs/images/lighting-before-upstream-84c87c83.png) | ![Tiled-light blocks removed by the candidate](docs/images/lighting-after-candidate-9b6e15be.png) |

The still image catches a relatively faint corrupted frame. The blocks flashed
repeatedly and were more pronounced during normal runtime.

D49 later reproduced the clean result using a paired compiler and runtime
experiment. It was useful evidence, but D50 through D52 showed that changing
compiler lowering is unnecessary and superseded D49 as the current technical
direction.

![Lighting after the D49 compiler-aware candidate](docs/images/lighting-after-d49-compiler-aware-731c4aae.png)

See [`docs/evidence-d50-d52-r32-alias-result.md`](docs/evidence-d50-d52-r32-alias-result.md),
[`docs/evidence-d49-compiler-aware-result.md`](docs/evidence-d49-compiler-aware-result.md),
[`docs/evidence-u01-upstream-candidate-ab.md`](docs/evidence-u01-upstream-candidate-ab.md),
and [`docs/evidence-d47-allocator-only-wired-result.md`](docs/evidence-d47-allocator-only-wired-result.md).

## Evidence map

- [`docs/final-report.md`](docs/final-report.md): final technical report and
  current upstream status
- [`docs/README.md`](docs/README.md): curated evidence index and archive map
- [`patches/README.md`](patches/README.md): diagnostic history, historical
  patch identities, and the D50 through D52 disposition
- [`docs/reproduction.md`](docs/reproduction.md): controlled reproduction and
  safety procedure
- [`docs/experiment-matrix.md`](docs/experiment-matrix.md): complete test
  chronology, including negative and invalid controls
- [`docs/findings.md`](docs/findings.md): chronological findings ledger
- [`PROVENANCE.md`](PROVENANCE.md): authorship, upstream provenance, and
  distribution boundaries

The repository intentionally retains failed tests and false leads. They show
which mechanisms were excluded and prevent the successful results from being
mistaken for coincidental configuration changes.

## Verified final environment

- CPU topology: 16 logical CPUs in one host NUMA node
- GPU: AMD Radeon RX 7800 XT
- Mesa/RADV: 26.1.6
- Game build: 24615759
- Lighting baseline: VKD3D-Proton `84c87c83`
- Descriptor-boundary control: D50/D51, same 87,040-byte range and shader with
  `R32_UINT`, `R16_UINT`, `R32_UINT` producing pass, fail, pass
- VKD3D-Proton-only lighting discriminator: D52 on `84c87c83`, with stock
  dxil-spirv `cc75a0c9`; two clean runs using the separate OpenMP workaround
- Terrain fix: merged in VKD3D-Proton `731c4aae`
- Wine MR head validated through Proton: `e8319c0e6bfe7f94512218b48e3158e0c286b481`

This hardware coverage proves the reported mechanisms and removes hard-coded
host assumptions. It does not replace upstream review or cross-vendor runtime
testing.

## Reproducing or resuming

Do not alter a Proton prefix until a backup exists and Steam and the game are
fully stopped. Start with [`docs/reproduction.md`](docs/reproduction.md) and
the safety-aware scripts under [`scripts/`](scripts/).

Generated captures, prefixes, game assets, credentials, shader caches, custom
Proton packages, and unfiltered large traces are not committed. The final
evidence release is generated only from an explicit allowlist of reviewed
files.
