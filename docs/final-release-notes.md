# IL-2 Korea Proton investigation: final evidence snapshot

This release freezes the reviewed evidence for three independent Korea. IL-2
Series compatibility findings. The lighting root cause is now isolated at the
R16-versus-R32 descriptor/view boundary. A narrow runtime discriminator is
locally validated, while the cleaner general Mesa correction remains under
review.

## Upstream status at the snapshot

- Startup without OpenMP launch parameters: exact six-commit implementation
  validated from [Wine MR !11604](https://gitlab.winehq.org/wine/wine/-/merge_requests/11604).
  The same series is present in Valve's Wine fork and the Proton Bleeding Edge
  source branch. The upstream Wine MR remains open.
- Terrain-page corruption: general block-geometry copy fix merged through
  [VKD3D-Proton PR #3202](https://github.com/HansKristian-Work/vkd3d-proton/pull/3202).
- Flashing square lighting blocks: D50 and D51 isolate the failing boundary to
  a 32-bit atomic issued through the game's `R16_UINT` UAV view. An otherwise
  equivalent `R32_UINT` view passes. D52 confirmed this in-game twice with a
  narrowly selected VKD3D-Proton R32 alias and stock dxil-spirv. That alias is
  diagnostic and is superseded as an upstream direction by
  [Mesa MR !43672](https://gitlab.freedesktop.org/mesa/mesa/-/merge_requests/43672),
  which aligns RADV's out-of-bounds component selection with native AMD D3D12
  and pre-GFX10 behavior. The earlier implementation and discussion remain in
  [VKD3D-Proton PR #3207](https://github.com/HansKristian-Work/vkd3d-proton/pull/3207).

These are separate changes. D52 deliberately excluded the Wine startup and
terrain fixes, so its two clean lighting runs still used the OpenMP launch
workaround and did not validate the map. No D52 screenshot was captured. This
release does not claim that all three fixes are already available in standard
Proton.

## Archive contents

The attached tarball contains:

- the final report and curated evidence index;
- exact startup, terrain, and lighting evidence records;
- the merged terrain patch export and historical lighting patch export;
- exact D49 historical build provenance, the D50 through D52 result, and
  reviewed terrain and historical lighting screenshots;
- source for the focused NUMA and Vulkan atomic probes;
- a manifest containing the source commit and per-file SHA-256 values.

`SHA256SUMS` authenticates the compressed release archive.

## Deliberately excluded

The release contains no custom Proton binary, replacement DLL, game file,
game shader binary, Proton prefix, credentials, raw RenderDoc capture, shader
cache, or unfiltered large runtime log. See `PROVENANCE.md` in the archive for
the complete distribution boundaries.

The earlier `handoff-2026-08-06` release remains available as a historical
diagnostic handoff and is not replaced or deleted by this snapshot.
