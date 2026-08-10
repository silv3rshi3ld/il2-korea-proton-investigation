# IL-2 Korea Proton investigation: final evidence snapshot

This release freezes the reviewed evidence for three independent Korea. IL-2
Series compatibility findings. The root-cause investigation is complete. The
current lighting implementation is locally validated, while upstream review
and release integration remain in progress.

## Upstream status at the snapshot

- Startup without OpenMP launch parameters: exact six-commit implementation
  validated from [Wine MR !11604](https://gitlab.winehq.org/wine/wine/-/merge_requests/11604).
  The same series is present in Valve's Wine fork and the Proton Bleeding Edge
  source branch. The upstream Wine MR remains open.
- Terrain-page corruption: general block-geometry copy fix merged through
  [VKD3D-Proton PR #3202](https://github.com/HansKristian-Work/vkd3d-proton/pull/3202).
- Flashing square lighting blocks: a locally validated paired implementation.
  Generic typed-UAV atomic lowering is implemented in dxil-spirv. Exact
  executable and shader selection, plus a safe backend capability gate, are
  implemented in VKD3D-Proton. The first implementation and maintainer
  discussion remain in
  [VKD3D-Proton PR #3207](https://github.com/HansKristian-Work/vkd3d-proton/pull/3207),
  but that version is superseded and is not the current publication candidate.

These are separate changes. This release does not claim that all three are
already available in standard Proton.

## Archive contents

The attached tarball contains:

- the final report and curated evidence index;
- exact startup, terrain, and lighting evidence records;
- the merged terrain patch export and historical lighting patch export;
- exact D49 build provenance and reviewed terrain and lighting screenshots;
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
