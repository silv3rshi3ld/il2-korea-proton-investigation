# Root-cause assessment

## Current conclusion

The defect is now localized to the game's application-managed baked-terrain
page path, but not yet to one faulty operation. The installed Korea map defines
800 m texture quads and five LODs. Runtime evidence shows ordinary placed
2048x2048 BC3 cache textures rather than D3D12 sparse resources. This explains
why the corruption appears as regular rectangles and worsens with altitude.

D05c is decisive negative evidence: it corrected the exact
`R32G32B32A32_UINT`-to-BC3 reinterpret geometry for 202/202 observed border
copies, with zero rejects, while the missing pages and magenta edges remained
unchanged. That copy mismatch is real but is not the primary visible cause.

The startup/OpenMP problem remains an independent Wine/NUMA investigation.

## Ranked graphics assessment

| Rank | Mechanism | Evidence | Confidence |
|---:|---|---|---|
| 1 | Baked-cache page is not populated or not made visible before sampling | Terrain-page geometry is established; cache copies start at mission transition; single-queue testing does not repair an omitted in-queue dependency; D05c rules out the border geometry as primary | Medium |
| 2 | Wrong cache page or descriptor is selected at draw time | Strong altitude/LOD dependency; terrain mesh remains; descriptor-buffer disable is unchanged but invalid descriptor contents/indexing would affect both backends | Medium |
| 3 | Explicit shader LOD/page-index translation error | High-altitude distant-page selection is much worse; broad SRV minimum-LOD clamps and incomplete mip chains are excluded, but shader-side LOD/index arithmetic is not traced | Medium-low |
| 4 | Game texture-provider fallback contributes missing inputs | Six autumn paths fail both their requested and common-fallback lookup and default to white; those references are absent from installed packages | Low-medium as a contributor; low as the Linux-only cause |
| 5 | RADV mishandles otherwise valid Vulkan | Reproduced on two AMD generations, but no valid-command driver failure or minimal Vulkan reproduction is isolated | Low-medium |
| 6 | General already-fixed VKD3D/dxil-spirv defect | Unmodified current upstream remains unchanged | Low |

## Terrain model established by files and traces

```text
Korea map packages (800 m texture quads, 5 LODs)
  -> BlocksCache / BakedTerrain CPU work after mission transition
  -> page producer/intermediate resource
  -> placed 2048x2048 one-mip BC3 cache pool
  -> SRV/descriptor selection plus shader LOD/page index
  -> existing terrain mesh samples selected cache page
```

The loading display moving from 25% to 26% does not establish premature
completion. The engine continues cache work after entering the mission, and
the progress number can describe only the current loading phase. See
[`evidence-map-package-inspection.md`](evidence-map-package-inspection.md).

High altitude selects distant baked pages, exposing much more empty or wrong
content. Below roughly 1,500 m, local-detail pages and vegetation become
eligible, so more content appears without fixing the underlying cache path.

## Proven exclusions and weakened leads

- D3D12 reserved/tiled resources: zero relevant API calls.
- Separate compute/transfer queues: two `single_queue` runs are unchanged.
- Descriptor-buffer implementation alone: disabling it is visually unchanged.
- Placed-resource range aliasing: no overlap in the covered D03 class.
- Broad missing mip chains or SRV minimum LOD: D02 finds complete chains and
  clamp zero for the covered resources.
- Current-upstream shader translator: D04 is unchanged.
- The tested reinterpret border-copy geometry: D05c changes every encountered
  candidate and visuals remain unchanged.
- A wholesale missing map archive: every Maps1-6 file tree was extracted and
  multi-gigabyte content was present.
- Wine WIC scaler mode 3 as an abort: Wine logs the unsupported interpolation
  mode but falls back to nearest-neighbour and returns success.

## Missing files are real, but causality is limited

Static backend inspection shows that a `FAILED load: requested (fallback)`
line means both lookups failed and a default-white texture is retained. Package
inspection confirms the six tested autumn terrain paths are absent. However,
nearly the same absent-reference set exists in all Korea seasons, including
the summer configuration, while native Windows is reported to render the same
Windows build correctly. These references may be optional or stale. Without a
matched Windows `tex.log`, they cannot be promoted to the Linux root cause.

## Decision and next discriminator

Do not add an application override or run more unrelated flags. D06 should be
trace-only and limited to the 2048x2048 one-mip BC3 cache pool plus associated
producer intermediates. It must correlate:

1. resource creation and placed allocation;
2. full-page production/copy or render/UAV writes;
3. resource states, Vulkan layouts, and copy-to-sample barriers;
4. SRV creation/copy and the descriptor bound at first sampling;
5. command-list/queue submission and destruction ordering.

The key fork is whether corrupted pages are never populated, populated but not
visible, or populated correctly and sampled through the wrong descriptor/page
index. Only after that fork is observed is a VKD3D, RADV, or game patch
justified.
