# IL-2 Korea Proton investigation: concluded evidence archive

This release freezes the concluded investigation record for three independent
Korea. IL-2 Series compatibility findings. It includes successful results,
negative controls, superseded implementations, and the exact limits of every
runtime claim. The lighting root cause is isolated at the R16-versus-R32
descriptor/view boundary. A narrow runtime discriminator is locally validated,
while the cleaner general Mesa correction remains open for review and testing.
These notes describe the
[`final-evidence-2026-08-11`](https://github.com/silv3rshi3ld/il2-korea-proton-investigation/releases/tag/final-evidence-2026-08-11)
archive release.

## Upstream status at the archive date

- Startup without OpenMP launch parameters: the six-commit implementation from
  [Wine MR !11604](https://gitlab.winehq.org/wine/wine/-/merge_requests/11604)
  was validated through Proton at the earlier `e8319c0e` head, together with
  Valve's equivalent series. Wine merged the MR on 2026-08-10 at final rebased
  head `663fd7cc`. That final head was not rerun in this investigation.
- Terrain-page corruption: general block-geometry copy fix merged through
  [VKD3D-Proton PR #3202](https://github.com/HansKristian-Work/vkd3d-proton/pull/3202)
  as `731c4aae`.
- Flashing square lighting blocks: D50 and D51 isolate the failing boundary to
  a 32-bit atomic issued through the game's `R16_UINT` UAV view. An otherwise
  equivalent `R32_UINT` view passes. D52 confirmed this in-game twice with a
  narrowly selected VKD3D-Proton R32 alias and stock dxil-spirv. That alias is
  diagnostic and is superseded as an upstream direction by
  [Mesa MR !43672](https://gitlab.freedesktop.org/mesa/mesa/-/merge_requests/43672),
  which aligns RADV's out-of-bounds component selection with native AMD D3D12
  and pre-GFX10 behavior. The earlier
  [dxil-spirv PR #296](https://github.com/HansKristian-Work/dxil-spirv/pull/296)
  and [VKD3D-Proton PR #3207](https://github.com/HansKristian-Work/vkd3d-proton/pull/3207)
  were closed unmerged as superseded. Mesa MR !43672 remains open and was not
  locally game-tested for this snapshot.

These are separate tracks. D52 deliberately excluded the Wine startup and
terrain fixes, so its two clean lighting runs still used the OpenMP launch
workaround and did not validate the map. No D52 screenshot was captured. This
release does not claim that the merged changes and proposed Mesa direction are
already available together in standard Proton.

The repository is an evidence archive, not a binary distribution or game mod.
The closed experimental PRs remain linked because their review and failure
modes materially contributed to the final diagnosis.

## Archive contents

The attached tarball contains:

- the final report and curated evidence index;
- exact startup, terrain, and lighting evidence records;
- a historical terrain PR patch export and historical lighting patch exports;
- exact D49 historical build provenance, the D50 through D52 result, and
  reviewed terrain and historical lighting screenshots;
- source for the focused NUMA and Vulkan atomic probes;
- a manifest containing the source commit and per-file SHA-256 values.

`SHA256SUMS` records the checksum used to verify the compressed release
archive.

## Deliberately excluded

The release contains no custom Proton binary, replacement DLL, game file,
game shader binary, Proton prefix, credentials, raw RenderDoc capture, shader
cache, or unfiltered large runtime log. See `PROVENANCE.md` in the archive for
the complete distribution boundaries.

The earlier
[`final-evidence-2026-08-10`](https://github.com/silv3rshi3ld/il2-korea-proton-investigation/releases/tag/final-evidence-2026-08-10)
release remains available as a historical intermediate snapshot. The
`handoff-2026-08-06` release likewise remains available as the original
diagnostic handoff. This 2026-08-11 snapshot supersedes both as the current
summary without deleting either earlier record.
