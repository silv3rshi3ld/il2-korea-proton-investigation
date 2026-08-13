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

The local Wine candidate is retained only as diagnostic history. Wine merged
the general startup implementation through MR !11604 on 2026-08-10 at final
head `663fd7cc`. This investigation tested the earlier `e8319c0e` MR head and
Valve's equivalent series through Proton, not the final rebased head. The
terrain change was merged through VKD3D-Proton PR #3202 as `731c4aae`.

The first lighting implementation in VKD3D-Proton PR #3207 and the later
paired dxil-spirv/VKD3D-Proton D49 experiment remain useful causal and runtime
evidence. D50 and D51 subsequently isolated the failure to the R16-versus-R32
descriptor/view boundary. D52 confirmed the result in-game twice using a
narrow VKD3D-Proton R32 alias with stock dxil-spirv. That alias is diagnostic,
not the preferred upstream solution. dxil-spirv PR #296 and VKD3D-Proton PR
#3207 were closed unmerged as superseded.

The investigation agrees with the cleaner general direction in Mesa MR !43672,
which aligns RADV's out-of-bounds component selection with native AMD D3D12
and pre-GFX10 behavior. NVIDIA already passes the relevant descriptor-heap
test. The local dxil-spirv and D52 candidates are therefore superseded. Mesa
MR !43672 remains open and was not locally game-tested for this archive.

- Startup: <https://gitlab.winehq.org/wine/wine/-/merge_requests/11604>
- Terrain: <https://github.com/HansKristian-Work/vkd3d-proton/pull/3202>
- Closed dxil-spirv lighting experiment:
  <https://github.com/HansKristian-Work/dxil-spirv/pull/296>
- Closed VKD3D-Proton lighting experiment and maintainer discussion:
  <https://github.com/HansKristian-Work/vkd3d-proton/pull/3207>
- Open proposed Mesa correction:
  <https://gitlab.freedesktop.org/mesa/mesa/-/merge_requests/43672>

The historical paired lighting implementation and the D52 diagnostic do not
grant redistribution rights beyond those of their upstream source trees. A
sanitized formatted D52 diagnostic patch is retained as reproducibility
evidence. No custom Proton binary is tracked in Git; an optional testing build
may instead be distributed separately as a GitHub Release asset with upstream
licenses, source provenance, packaged modifications, and checksums.

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
- custom Proton packages or prebuilt replacement DLLs in the Git repository
  itself; optional binary release assets are documented separately;
- third-party artifacts whose redistribution has not been reviewed.

Only an allowlisted, sanitized subset of tracked files is eligible for the
final evidence release. Its generated manifest records the repository commit
and SHA-256 of every allowlisted source file copied into the archive. The
external `SHA256SUMS` file covers the complete compressed archive. The Git
history and closed upstream PRs remain part of the public technical record even
though their implementations were not merged.
