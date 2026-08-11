# Provenance and distribution boundaries

This repository is a technical investigation record. It combines original
analysis, small reproduction tools, formatted patches against upstream
projects, and screenshots of third-party software. No single blanket license
is asserted over every category of content.

## Original investigation material

Unless a file states otherwise, the prose under `docs/`, the repository
automation under `scripts/`, and the focused source under `probes/` were
created for this investigation by the repository owner and contributors shown
in Git history.

No broad reuse license is granted merely by publication of this repository.
Anyone wishing to reuse a substantial original component should contact the
repository owner or rely on the terms explicitly present in that component.

## Wine, dxil-spirv, VKD3D-Proton, and Mesa changes

Files under `patches/` are formatted diffs against upstream source trees. Their
headers record authorship where applicable, while the surrounding source and
context remain subject to the corresponding upstream project's licensing and
contribution rules.

- Wine: <https://gitlab.winehq.org/wine/wine>
- dxil-spirv: <https://github.com/HansKristian-Work/dxil-spirv>
- VKD3D-Proton: <https://github.com/HansKristian-Work/vkd3d-proton>
- Mesa: <https://gitlab.freedesktop.org/mesa/mesa>

The local Wine candidate is retained only as diagnostic history. The intended
general startup implementation is upstream Wine MR !11604. The terrain change
was merged through VKD3D-Proton PR #3202. The first lighting implementation in
PR #3207 and the later paired dxil-spirv/VKD3D-Proton D49 experiment remain
useful causal and runtime evidence. D50 and D51 subsequently isolated the
failure to the R16-versus-R32 descriptor/view boundary. D52 confirmed the
result in-game twice using a narrow VKD3D-Proton R32 alias with stock
dxil-spirv. That alias is diagnostic, not the preferred upstream solution.

The investigation agrees with the cleaner general direction in Mesa MR !43672,
which aligns RADV's out-of-bounds component selection with native AMD D3D12
and pre-GFX10 behavior. NVIDIA already passes the relevant descriptor-heap
test. The local dxil-spirv and D52 candidates are therefore superseded pending
review and validation of the Mesa change.

- Terrain: <https://github.com/HansKristian-Work/vkd3d-proton/pull/3202>
- Lighting discussion and first implementation:
  <https://github.com/HansKristian-Work/vkd3d-proton/pull/3207>
- Proposed Mesa correction:
  <https://gitlab.freedesktop.org/mesa/mesa/-/merge_requests/43672>

The historical paired lighting implementation and the D52 diagnostic do not
grant redistribution rights beyond those of their upstream source trees. A
sanitized formatted D52 diagnostic patch is retained as reproducibility
evidence, but no custom Proton binary is included in this repository.

For reuse or redistribution of a patch, consult the license of the source tree
to which it applies and the applicable upstream contribution policy.

## Screenshots

The committed screenshots were captured by the repository owner during
controlled tests. They contain imagery rendered by Korea. IL-2 Series. The
game, artwork, names, and associated intellectual property belong to their
respective rights holders.

The screenshots are included only to document compatibility defects and test
outcomes. Publication here does not claim ownership of the depicted game
content or grant broader rights to it.

## External evidence

Some findings reference logs, screenshots, and comments posted by other users
to public issue trackers. Except for explicitly reviewed and sanitized handoff
material, those third-party artifacts are not copied into this repository.
The documentation records links, checksums, and derived technical facts so the
provenance remains auditable.

## Excluded material

The repository and final evidence release intentionally exclude:

- game executables, packages, textures, and extracted assets;
- account information, credentials, Steam configuration databases, and Proton
  prefixes;
- game shader binaries and shader caches;
- raw RenderDoc captures and unfiltered large runtime traces;
- custom Proton packages or prebuilt replacement DLLs;
- third-party artifacts whose redistribution has not been reviewed.

Only an allowlisted, sanitized subset of tracked files is eligible for the
final evidence release. Its generated manifest records the repository commit
and SHA-256 of every included file.
