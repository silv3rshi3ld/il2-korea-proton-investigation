# Root-cause assessment

## Current conclusion

The defect is localized to the game's application-managed baked-terrain page
path, with a specific copy-unit mismatch now the leading candidate. The
installed Korea map defines 800 m texture quads and five LODs. Runtime evidence
shows ordinary placed 2048x2048 BC3 cache textures rather than D3D12 sparse
resources. This explains why the corruption appears as regular rectangles and
worsens with altitude.

D05c is decisive negative evidence only for a border-only correction: it
corrected 202/202 thin copies with zero rejects while the image remained
unchanged. D06 then revealed that the `64x64` interiors are also emitted at
source size despite being placed on a 256-texel destination grid. The same 1:4
physical-block conversion would raise observed cache coverage from 3.46-6.32%
to 54.69-100%. This closely explains both isolated terrain rectangles and
magenta seams. D07 will confirm the square footprints' DXGI format and test the
complete page family; until that run, this remains a high-confidence candidate
rather than a demonstrated fix.

The startup/OpenMP problem remains an independent Wine/NUMA investigation.

## Ranked graphics assessment

| Rank | Mechanism | Evidence | Confidence |
|---:|---|---|---|
| 1 | Missing block-unit conversion on buffer-to-BC3 terrain-page copies | D06 interiors are 64x64 but placed every 256 texels; border offsets align at page ends; projected 4x mapping fills the observed caches; D05c did not include interiors | High as a candidate; D07 causality pending |
| 2 | Baked-cache page is otherwise not populated or not made visible before sampling | Terrain-page geometry is established; cache copies start at mission transition; single-queue testing does not repair an omitted in-queue dependency | Medium-low after D06 |
| 3 | Wrong cache page or descriptor is selected at draw time | Strong altitude/LOD dependency; terrain mesh remains; descriptor-buffer disable is unchanged but invalid descriptor contents/indexing would affect both backends | Medium-low |
| 4 | Explicit shader LOD/page-index translation error | High-altitude distant-page selection is much worse; broad SRV minimum-LOD clamps and incomplete mip chains are excluded, but shader-side LOD/index arithmetic is not traced | Low-medium |
| 5 | Game texture-provider fallback contributes missing inputs | Six autumn paths fail both their requested and common-fallback lookup and default to white; those references are absent from installed packages | Low-medium as a contributor; low as the Linux-only cause |
| 6 | RADV mishandles otherwise valid Vulkan | Reproduced on two AMD generations, but no valid-command driver failure or minimal Vulkan reproduction is isolated | Low-medium |
| 7 | General already-fixed VKD3D/dxil-spirv defect | Unmodified current upstream remains unchanged | Low |

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
- The thin reinterpret borders as a complete explanation: D05c changes every
  encountered border candidate and visuals remain unchanged. It does not
  exclude the interior-page conversion found by D06.
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

Do not add an application override or run more unrelated flags. D07 is the next
single-variable discriminator. It retains the exact cache, source-format,
shape, byte-size, pitch, alignment, and bounds checks while converting both
observed page interiors and borders. A repeatable repair would establish the
copy-unit mismatch as causal and justify reducing it to a general VKD3D-Proton
fix and regression test. An unchanged result would return the investigation to
producer visibility and descriptor/page selection.
