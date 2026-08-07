# D04 unmodified current-upstream result

## Result

D04 is **unchanged** in its single planned run. Current unmodified
VKD3D-Proton does not fix the menu or terrain corruption:

- the menu aircraft still has translucent rectangular blocks;
- at 6,400 m, the terrain surface is mostly absent;
- isolated rectangular texture pages, white strips, and magenta edges remain;
- no GPU hang or reset was reported.

This closes the “already fixed in current upstream” control. No repeat or
MSFS-derived application workaround is justified.

| Capture | SHA-256 | Observation |
|---|---|---|
| `D04-r1-menu-aircraft-blocks.png` | `f05d4ff6667eebffafd7ab47f3dd31598d9c7c966f518e31cdd1237eed240d38` | Menu artifact unchanged |
| `D04-r1-terrain-missing-pages-6400m.png` | `6267c1e35821ae98b855fb4c698aeebd525e038b16cf007ac3f6c4637e377990` | Severe page loss at 6,400 m |

The local images are retained under ignored
`captures/curated/d04-upstream/`.

## Runtime provenance

All four post-run prefix hashes exactly match the D04 custom tool:

| Prefix DLL | SHA-256 |
|---|---|
| `system32/d3d12.dll` | `b0b1fe2eb239d38b7d51bac6b12e2309fff5d57c4f77bd3323f2175857a705e8` |
| `system32/d3d12core.dll` | `f716b80f4e0460e3f7290994ca0f4e31943aac2022a7e6d228a17158bd8c9b5b` |
| `syswow64/d3d12.dll` | `9d35a3077e73518caec83f833a41b6e920bdda2bb6f504064c4999ec1773ee22` |
| `syswow64/d3d12core.dll` | `d9fe83fa531082bbfcc364057cbc847e49b89ac195ff2c08838bab8a690e1811` |

Their prefix mtimes precede both captures. This proves that unmodified
VKD3D-Proton `84c87c8390d9df75ba41d911496296fe13f0e275` with `dxil-spirv`
`cc75a0c98d34d7bcc03560527c799b52e48b4d1f` ran.

No Proton log was created in the prepared `PROTON_LOG_DIR`, so D04 makes no
warning-count or module-log comparison. A repeat solely to obtain that log is
not warranted because the DLL provenance and visual discriminator are valid.

## Game texture-provider evidence

The game wrote `data/tex.log` during D04. It contains 18 real failed texture
loads, including these six Korea winter terrain inputs:

```text
GRAPHICS\LANDSCAPE_KOREA_WI\SURFACETEX\ARF3\GROUND_SHAD_01-32X32.DDS
GRAPHICS\LANDSCAPE_KOREA_WI\SURFACETEX\ARF3\SPOTS_MASK.DDS
GRAPHICS\LANDSCAPE_KOREA_WI\LANDTEXTURESQUALITY\NUDEGROUND_2.DDS
GRAPHICS\LANDSCAPE_KOREA_WI\LANDTEXTURESQUALITY\NUDEGROUND_2_NM.DDS
GRAPHICS\LANDSCAPE_KOREA_WI\LANDTEXTURESQUALITY\NUDEGROUND_3.DDS
GRAPHICS\LANDSCAPE_KOREA_WI\LANDTEXTURESQUALITY\NUDEGROUND_3_NM.DDS
```

It also records six failures for
`GRAPHICS\SCENE\SCENEOBJECTS\TEXTURES\GLASS_MAIN.DDS`, one tree-shadow
texture, one vehicle texture, and four particle-path failures.

The `tex.log` SHA-256 is
`8e25cba0892975a598b3d30d871576bd28a394611fc62bdd6a9a81e3caaf0431`.
Its final mtime is 15 seconds before the corrupted 6,400 m screenshot.

This is not merely a list of attempted lookups. In the current
`dxBackend12.dll`, `FAILED load: %s (%s)` is emitted after the provider call
returns failure. The failure path then requests
`graphics\textures\defWhite.bmp`. The relevant static call sites are around
virtual addresses `0x18008bd81` and `0x180090fac`; the fallback request occurs
at `0x180090fff`. The game is therefore substituting its white default for
these failed textures.

`Maps6.gtp` contains configuration strings referencing the exact Korea winter
paths above. `packman.log` shows that all six map packages were enumerated, a
45,100-node global package tree was created, and 11,575 files were opened from
packages. It reports no package-open or package-decode error. This narrows the
failure to a texture lookup/decode/create stage, but does not yet distinguish
among them.

The game logs are retained under ignored
`captures/runs/D04-upstream-r1/game-logs/`; `collect-game-logs.sh` reproduces
the bounded collection without changing the game installation.

## Loading-progress observation

The user observed the mission display move from 25% to 26% and then enter the
mission. Proton logs do not expose that UI percentage, and the D04 Proton log
was absent. The transition is therefore recorded but cannot yet be called a
premature load completion. It may be a phase-local or non-normalized progress
value.

The decisive discriminator is a current native-Windows run of the same Korea
winter free-flight scenario:

- if Windows also enters at 26% and records the same `tex.log` failures, both
  observations are probably normal or unrelated;
- if Windows reaches the usual final progress and does not record the six
  terrain failures, the investigation should move above VKD3D resource
  translation to the game's texture lookup/decode/create path under Wine;
- if Windows records the failures but renders correctly, the failed material
  inputs are non-causal and descriptor/resource visibility remains the leading
  graphics path.

This was true for the original D04 analysis. A later read-only package
inspection established the map's page geometry and the installed-content
status of these references; see
[`evidence-map-package-inspection.md`](evidence-map-package-inspection.md).
