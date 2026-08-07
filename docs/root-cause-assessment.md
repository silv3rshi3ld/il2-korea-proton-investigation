# Root-cause assessment

## Current conclusion

The terrain defect is caused by a block-unit conversion missing from
VKD3D-Proton's buffer-to-image `CopyTextureRegion` path. The game supplies
placed `DXGI_FORMAT_R32G32B32A32_UINT` footprints and copies them into ordinary
placed `DXGI_FORMAT_BC3_UNORM` baked-terrain caches. Both physical elements are
16 bytes, so each source texel represents one 4x4 BC3 block. D3D12's buffer
layout is described in source-format texels, while Vulkan requires the buffer
layout and image extent in destination image texels.

The old `vk_buffer_image_copy_from_d3d12()` path carried the source dimensions
through unchanged. D07 converted all 522 observed page interiors and borders:
178 `64x64` interiors became `256x256`, 182 horizontal borders became
`256x4`, and 162 vertical borders became `4x256`, with zero rejects. Terrain at
approximately 5,500 m changed from mostly absent rectangular pages with
magenta seams to continuous detailed terrain. This is a causal result, not a
visual analogy.

The public D3D12 documentation does not clearly list this BC3/RGBA32_UINT
reinterpret pair among its compatible format groups. Native Windows and
VKD3D-Proton's existing image-to-image path nevertheless support the same
physical-block interpretation. The safest ownership statement is therefore a
VKD3D-Proton native-compatibility gap, not a demonstrated RADV defect. The
general `cf11ba76` predecessor passes its regression tests and D08 validates it
in game without any IL-2-specific filter or diagnostic gate. Current PR
candidate `64ec55e7` further restricts activation to equal-sized physical
elements with different block dimensions, leaving same-geometry copies on the
original path while selecting unchanged conversion arithmetic for IL-2.

The startup/OpenMP problem remains an independent Wine/NUMA investigation.

## Ranked graphics assessment

| Rank | Mechanism | Evidence | Confidence |
|---:|---|---|---|
| 1 | Missing block-unit conversion on buffer-to-BC3 terrain-page copies | D07 adjusted 522/522 exact-class copies in run 1 and 304/304 in run 2, with zero rejects, and repaired high-altitude terrain both times; clean general-build D08 repeats the repair; the focused synthetic test fails four assertions on the old path and passes 22/22 with the narrowed general fix | High; causal for terrain and addressed by current PR candidate `64ec55e7` |
| 2 | Separate menu effect, shadow, or temporal-resource defect | D07-r2 and clean D08 preserve the terrain repair while aircraft blocks and shimmering remain | Confirmed separate; cause open |
| 3 | Game texture-provider fallback contributes secondary missing inputs | The successful D07 run still logs missing summer/common inputs, proving they are not required for the rectangular terrain failure | Low as a remaining contributor |
| 4 | RADV mishandles otherwise valid Vulkan | The same driver renders correctly when VKD3D emits converted copy geometry; no driver change was required | Very low for the terrain defect |

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
  encountered border candidate and visuals remain unchanged. D07 proves that
  full interiors plus borders are required.
- Split `END_ONLY` barriers: 40,408 warnings remain in the successful D07 run.
- The logged missing Korea terrain inputs as the primary cause: the same
  fallbacks remain in the successful D07 run.
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

Do not add an application override or run more unrelated terrain flags. D08
validates the general `vk_buffer_image_copy_from_d3d12()` conversion at
predecessor `cf11ba76` without the IL-2 resource/shape filter or
`VKD3D_IL2_BC3_PAGE_COPY`. Current PR commit `64ec55e7` preserves that IL-2
branch while returning all same-block-geometry copies to the original path;
its focused and full copy tests pass. The next graphics work should instrument
the menu aircraft and motion-only flicker as a separate defect.
