# IL-2 Korea Proton compatibility investigation

This repository is the concluded public record of three independent
compatibility investigations for **Korea. IL-2 Series** (Steam AppID 247970):
startup through Wine, terrain uploads through VKD3D-Proton, and tiled-light
allocation at the D3D12-to-Vulkan driver boundary. It preserves the successful
results, negative controls, superseded approaches, and exact evidence limits so
the work can be audited or resumed without repeating the investigation.

> [!IMPORTANT]
> This is primarily an investigation and upstream evidence archive, not a game
> mod. An optional, unofficial community test Proton is documented separately
> under [`community-proton/`](community-proton/README.md); it is not an
> upstream or supported Proton release.
> No game files were modified. The Wine startup series was validated without
> launch parameters, but the isolated D52 lighting test deliberately excluded
> that Wine work and therefore still used the OpenMP startup workaround. A fix
> being proven here does not mean it is already available in standard Proton.

## Status at a glance

| Problem | Proven cause | Upstream path | Status on 2026-08-11 |
| --- | --- | --- | --- |
| Startup abort or need for OpenMP launch options | Wine's public NUMA queries did not expose the processor topology already known internally | [Wine MR !11604](https://gitlab.winehq.org/wine/wine/-/merge_requests/11604) | Merged on 2026-08-10 at final head `663fd7cc`. This investigation validated the earlier `e8319c0e` head and Valve's equivalent series with empty Steam launch options, not the final rebased MR head. Availability in standard Proton depends on downstream integration |
| Missing or magenta terrain pages | Placed-buffer geometry stayed in uncompressed source texels instead of being converted to BC3 destination block geometry | [VKD3D-Proton PR #3202](https://github.com/HansKristian-Work/vkd3d-proton/pull/3202) | The pre-merge PR artifact was independently confirmed on another system; its reviewed successor merged as `731c4aae`. The final merge was not separately packaged in this investigation |
| Flashing square lighting blocks | The game issues a 32-bit atomic through an `R16_UINT` UAV view; D50 and D51 isolate the view format as the failing boundary on the tested RADV stack | [Mesa MR !43672](https://gitlab.freedesktop.org/mesa/mesa/-/merge_requests/43672), which aligns RADV's out-of-bounds component selection with native AMD D3D12 and pre-GFX10 behavior | A VKD3D-Proton-only R32 alias removed the blocks in two D52 runs with stock dxil-spirv. The dxil-spirv and VKD3D-Proton PRs were closed unmerged as superseded. The Mesa MR is the preferred general direction, but it has not been locally game-tested by this investigation |

The final community-facing status is in
[Proton issue #9906](https://github.com/ValveSoftware/Proton/issues/9906#issuecomment-5257604136).
The D50-D52 reasoning and the decision to move away from a VKD3D-Proton quirk
are preserved in the
[PR #3207 discussion](https://github.com/HansKristian-Work/vkd3d-proton/pull/3207#issuecomment-5256360847).
The complete technical conclusions, exact hashes, and evidence boundaries are
in [`docs/final-report.md`](docs/final-report.md).
The sanitized archive bundle is the
[2026-08-11 final evidence release](https://github.com/silv3rshi3ld/il2-korea-proton-investigation/releases/tag/final-evidence-2026-08-11).

## Community test Proton

The separately packaged
[`IL-2 Korea Three-Fix Proton`](community-proton/README.md) combines the tested
Wine NUMA startup series, the VKD3D-Proton terrain candidate, and the narrowly
scoped VKD3D-Proton lighting workaround in one importable Steam compatibility
tool. Its page contains installation, verification, rollback, source, and
testing-limit information.

The binary archive is too large for ordinary Git tracking and belongs as a
GitHub Release asset. It is supplied strictly for testing, as-is, without
warranty, maintenance, or user support. The lighting workaround remains a
historical application-specific compatibility mechanism, not the preferred
general Mesa direction described by the final investigation.

## Upstream record

- [Final Proton community status](https://github.com/ValveSoftware/Proton/issues/9906#issuecomment-5257604136)
- [Wine/NUMA startup validation](https://github.com/ValveSoftware/Proton/issues/9906#issuecomment-5218434565)
- [VKD3D-Proton tiled-light root-cause report](https://github.com/HansKristian-Work/vkd3d-proton/issues/3134#issuecomment-5238151028)
- [Merged terrain PR #3202](https://github.com/HansKristian-Work/vkd3d-proton/pull/3202)
- [Closed dxil-spirv experiment #296](https://github.com/HansKristian-Work/dxil-spirv/pull/296)
- [Closed VKD3D-Proton lighting experiment #3207 and maintainer review](https://github.com/HansKristian-Work/vkd3d-proton/pull/3207)
- [Published D52 diagnostic source on the personal VKD3D-Proton fork](https://github.com/silv3rshi3ld/vkd3d-proton/commit/8cd28e8f98751afe3b85c3b08519464907aa5143)
- [D52 publication note on closed PR #3207](https://github.com/HansKristian-Work/vkd3d-proton/pull/3207#issuecomment-5276514345)
- [Open general RADV correction in Mesa MR !43672](https://gitlab.freedesktop.org/mesa/mesa/-/merge_requests/43672)
- [D50 through D52 descriptor-boundary result](docs/evidence-d50-d52-r32-alias-result.md)
- [Final evidence release from 2026-08-11](https://github.com/silv3rshi3ld/il2-korea-proton-investigation/releases/tag/final-evidence-2026-08-11)
- [Historical evidence snapshot from 2026-08-10](https://github.com/silv3rshi3ld/il2-korea-proton-investigation/releases/tag/final-evidence-2026-08-10)
- [Early diagnostic handoff from 2026-08-06](https://github.com/silv3rshi3ld/il2-korea-proton-investigation/releases/tag/handoff-2026-08-06)

## What the investigation proved

### Startup without hard-coded processor values

The game ships Intel's OpenMP runtime. During initialization it asks Windows
which processors belong to each NUMA node. Wine detected the host topology but
returned an unimplemented result through the queried public API. OpenMP treated
that result as fatal.

The six-commit series tested from Wine MR !11604 exposes the runtime topology
through the missing APIs. A full Steam launch succeeded with an empty
launch-options field, no `KMP_*`, `OMP_*`, or `WINE_CPU_TOPOLOGY` setting, and
affinity controls allowing 1, 2, 4, 8, and 16 CPUs. OpenMP reported the
corresponding available count in every case. No CPU vendor, game, AppID, or
thread count is encoded in the implementation.

Wine merged the MR on 2026-08-10 at rebased head `663fd7cc`. The local Proton
validation predates that rebase and covers head `e8319c0e` plus Valve's
equivalent series. It supports the merged mechanism but is not a claim that
the final rebased commit itself was rerun here.

See [`docs/evidence-n05-wine-mr-11604.md`](docs/evidence-n05-wine-mr-11604.md).

### Terrain-page corruption

IL-2 uploads a `64x64 R32G32B32A32_UINT` placed footprint as the physical data
for a `256x256 BC3_UNORM` terrain page. Both physical elements are 16 bytes, so
one source texel represents one 4x4 BC3 block. VKD3D-Proton retained the source
geometry and populated only one sixteenth of the destination page.

The general change converts equal-sized physical elements through their block
geometry. It contains no IL-2 executable or AppID check. A gated causal test
adjusted 522 of 522 observed page and border copies with zero rejects; D08
predecessor `cf11ba76` restored the terrain at multiple altitudes. The
pre-merge PR artifact was also
[confirmed by another user](https://github.com/ValveSoftware/Proton/issues/9906#issuecomment-5237490788),
and the reviewed successor merged as `731c4aae`. Neither that final merge nor
historical review candidate `64ec55e7` was separately packaged for an in-game
run in this specific evidence record.

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

The game-tested D52 source was later forward-ported as commit
[`8cd28e8f`](https://github.com/silv3rshi3ld/vkd3d-proton/commit/8cd28e8f98751afe3b85c3b08519464907aa5143)
on the personal fork branch
[`diagnostic-il2-r32-alias-d52`](https://github.com/silv3rshi3ld/vkd3d-proton/tree/diagnostic-il2-r32-alias-d52).
That forward-port passed the x86-64 and x86 builds but was not rerun in the
game. It was linked from a
[follow-up comment on closed PR #3207](https://github.com/HansKristian-Work/vkd3d-proton/pull/3207#issuecomment-5276514345)
only to make the final discriminator inspectable, not to reopen the PR or
propose the per-game alias for merging.

The R32 alias is evidence, not the proposed upstream implementation. The
experimental [dxil-spirv PR #296](https://github.com/HansKristian-Work/dxil-spirv/pull/296)
and [VKD3D-Proton PR #3207](https://github.com/HansKristian-Work/vkd3d-proton/pull/3207)
were closed unmerged once the descriptor-boundary result made those approaches
unnecessary. Hans'
[Mesa MR !43672](https://gitlab.freedesktop.org/mesa/mesa/-/merge_requests/43672)
provides the cleaner general direction by making RADV's out-of-bounds component
selection match native AMD D3D12 and pre-GFX10 behavior. NVIDIA already passes
the relevant descriptor-heap test. This investigation agrees with resolving
the compatibility behavior in Mesa instead of carrying either the earlier
dxil-spirv lowering or a per-game VKD3D-Proton alias quirk. Review and a final
test of the Mesa change on an otherwise unmodified stack remain outstanding.
No local game run in this archive used Mesa MR !43672.

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

## Verified test environment and component identities

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
- Wine MR final merged head: `663fd7cc` (not rerun after the final rebase)

This hardware coverage supports the reported mechanisms, and the tested
implementations encode no fixed host processor count. It does not replace
upstream review, physical cross-topology validation, or cross-vendor runtime
testing.

## Using this archive

Do not alter a Proton prefix until a backup exists and Steam and the game are
fully stopped. Start with [`docs/reproduction.md`](docs/reproduction.md) and
the safety-aware scripts under [`scripts/`](scripts/).

The investigation itself is concluded. Future work should begin from the
merged Wine and VKD3D-Proton changes and test Mesa MR !43672 on an otherwise
unmodified graphics stack, rather than reviving the closed compiler or
per-game alias approaches without new evidence.

Generated captures, prefixes, game assets, credentials, shader caches, custom
Proton packages, and unfiltered large traces are not committed. The final
evidence release is generated only from an explicit allowlist of reviewed
files.
