# Korea map package and loading-path inspection

## Scope and safety

The installed Windows game build `24596901` was inspected read-only. Maps 1-6
were extracted only into an isolated temporary directory using a copied Proton
prefix, no network access, and read-only access to the installation. No game
asset, extracted package, or third-party executable was added to this
repository. The game installation and compatibility prefix were not modified.

The first attempt to initialize a new system-Wine prefix stalled at Wine Mono
installation. Its processes were terminated and that prefix was abandoned.
The successful extraction used a temporary copy of the already initialized
game prefix; Wine Mono was neither required nor installed into the game prefix.

Community extraction tool provenance:

- archive SHA-256: `de67cce37144c0df5bfe5b722181d5381f58951736e1aab8d49d57bdc538863d`
- executable SHA-256: `b6788b19978037f5c7be32707da4d9ab95c262e6541122bba1d8c7722e9fef34`
- 29,696-byte PE32 console program
- imports limited to Kernel32, VCRuntime, and CRT API sets; no network imports

This is inspection evidence, not a redistributable extraction workflow.

## Installed-content result

The local Steam manifest records one installed content depot (`247971`,
manifest `1620490147221162245`) and a total installed size of 40,227,845,710
bytes. The Proton prefix runs the Windows build; there is no evidence of a
separate Linux terrain-content depot being omitted.

All six map archives produced their file trees. Each extractor run ended on an
unrecognized `STRM` record, but the difference between archive bytes and the
sum of extracted file bytes was only about 200 KiB per multi-gigabyte archive.
That small footer/metadata-sized difference is not evidence that the 1.4-1.7
GiB terrain payload failed to extract.

## What the map files establish

The Korea map owns an application-level baked terrain system. Its setup states:

```text
numberOfLods=5
quadsInSector=16
quadSizeX=50
quadSizeY=50
textureStep=0.0078125
viewDistance=500
textureQuadSize=800
maxRiverLod=3
```

The package also contains ordinary map data such as `ground.mesh`,
`nature.mesh`, `rivers.mesh`, `surface.dat`, a distant-LOD data file, two
4096x4096 BC3 distant-LOD textures, and hundreds of surface textures. This
corroborates the runtime result that the game implements tiled landscape pages
above D3D12 rather than using D3D12 reserved/tiled resources. The isolated
rectangles in screenshots are consistent with the engine's 800 m texture
quads.

The loading display moving from 25% to 26% and then entering the mission is
therefore not proof that map loading aborted. Runtime cache-copy activity begins
at mission transition, and the engine is designed to populate or select baked
terrain pages after entry. The percentage can be phase-local rather than a
normalized whole-mission progress value.

## Missing texture references

The six autumn Korea paths in the D05c `tex.log` are not present in Maps1-6 or
as loose game files. Static inspection of `dxBackend12.dll` shows that
`FAILED load: requested (fallback)` is emitted only after both the requested
path and `graphics\\textures\\common\\<name>` fail; the backend then retains a
default-white texture object.

Comparing every season's `surfacetex.txt` references against all extracted map
packages found a nearly identical absent set in every Korea season. The summer
configuration has seven absent references; each other season has eight. These
include `spots_mask`, `nudeground_2`, `nudeground_3`, their normal/auxiliary
variants, and—outside summer—`ground_shad_01-32x32`. `tex.log` reports six of
these in the tested autumn mission; auxiliary maps appear to be handled as an
optional path.

This proves that the references are unresolved in the installed content. It
does not prove they are the Proton-only defect. Native Windows uses the same
Windows game build/content and is reported to render correctly, so these may be
stale or optional references whose fallback is expected. A Windows `tex.log`
from the same build would decide whether the failures are platform-specific.

## Consequence

The map packages explain the rectangular page geometry and disprove the simple
claim that the 26% transition alone means the map stopped loading. They do not
yet establish whether the corrupted pages are never generated, generated with
wrong contents, or generated correctly and later sampled through the wrong or
non-visible resource. That producer-to-SRV path is the next instrumentation
target.

