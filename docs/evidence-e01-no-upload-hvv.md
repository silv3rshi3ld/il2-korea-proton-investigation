# E01: host-visible VRAM upload heaps disabled

- Configuration: `VKD3D_CONFIG=no_upload_hvv`
- Run 1: `E01-no-upload-hvv-r1`
- Run 2: `E01-no-upload-hvv-r2`
- Classification: **inconclusive**. More vegetation was visible in both runs, but
  both E01 captures were made near 1,350-1,500 m. Cross-configuration
  screenshots now show that altitude itself strongly changes vegetation/page
  visibility.

## Run 1 visual result

The user reports that the map is improved and shows more trees/vegetation. The
terrain screenshot does contain substantially more visible vegetation, roads,
and settlement geometry than the most sparse E00 screenshots. However:

- most underlying terrain remains dark or absent;
- rectangular texture pages remain isolated;
- magenta page-edge artifacts remain;
- the E01 camera position and map location are not identical to E00.

E01-r2 reproduces the denser vegetation at roughly 1,356 m near Singo-dong,
while showing the same large dark surface, isolated terrain pages, and magenta
edges. However, E00 at 1,245 m also shows substantially more vegetation than
E00/E02 near 5,000 m. The terrain change cannot be attributed to the upload
control without matched-altitude captures. Menu/aircraft temporal artifacts
also remain inconclusive because no run-2 menu observation was supplied.

## Curated screenshots

The local copies are ignored by Git and retained under
`captures/curated/e01-no-upload-hvv/`.

| File | SHA-256 | Evidence |
|---|---|---|
| `E01-r1-menu-aircraft-artifacts.png` (local evidence, not published) | `8f522d38633a520a882519ef194b9c49a19c9043796a379710c0c0d4f9c49f7f` | Menu aircraft still; temporal change not classified |
| `E01-r1-terrain-more-vegetation-missing-pages-remain.png` (local evidence, not published) | `98582afcab78b9864446c8ed4f3cc4ea0f58de088ff472c95f320463be0e3af4` | More vegetation reported; missing pages and magenta borders remain |
| `E01-r2-terrain-more-vegetation-missing-pages-remain-singo-dong.png` (local evidence, not published) | `8e8962a52073e3c8b2654e9325702622db6d51e92edb5a3a4eae73b6b0167391` | Repeat near Singo-dong; vegetation remains denser, ground corruption remains severe |

## Configuration verification

The log confirms the control took effect:

| Signal | E00-r2 | E01-r1 |
|---|---|---|
| `VKD3D_CONFIG` | empty | `no_upload_hvv` |
| Upload heap | `DEVICE_LOCAL \| HOST_COHERENT` | forced `HOST_COHERENT` |
| Descriptor buffer | enabled | enabled |
| Queue fallback lines | families 0, 1, 5 | families 0, 1, 5 |
| D3D11 module | absent | absent |
| Device loss/OOM | absent | absent |

The duration-matched E00-r2 and E01-r1 logs have nearly identical split-barrier
activity: 18,562 warnings over 116.506 seconds versus 18,882 over 115.620
seconds. E01-r2 records 21,518 over 140.682 seconds. This does not explain or
refute the visual result; it shows the warning stream did not materially change
when the upload allocation path changed.

Generated log comparison:
`captures/comparisons/E00-r2-vs-E01-r1.md`.

Repeatability comparison:
`captures/comparisons/E01-r1-vs-r2.md`.

## Interpretation and evidence gate

Disabling host-visible VRAM upload heaps may change symptom expression, but the
existing screenshots cannot distinguish that from altitude/LOD behavior. The
upload allocation/visibility path remains a justified hypothesis because the
control was confirmed active, not because improvement has been proven.

No application override is justified. The single-queue control was completed
without improvement; runtime testing then ended before the descriptor-buffer
control. If work resumes, matched-altitude captures are required before
reassessing `no_upload_hvv`.
