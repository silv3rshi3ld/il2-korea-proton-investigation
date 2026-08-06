# Altitude and distance dependence

## Observation

The user reports a visible transition below roughly 1,500 m: additional
low-fidelity vegetation and terrain assets begin loading. The corruption is
strongest at higher altitude. The existing screenshot set supports that
observation, although map locations and camera positions are not identical.

| Configuration | Capture | HUD altitude | Visible result |
|---|---|---:|---|
| E00 baseline r1 | `E00-r1-terrain-external-wide-missing-pages.png` | 1,245 m | More vegetation visible, but ground remains dark and paged |
| E00 baseline r1 | `E00-r1-terrain-external-missing-pages.png` | 2,419 m | Sparse vegetation and isolated pages |
| E00 baseline r2 | `E00-r2-terrain-external-missing-pages-magenta-seams.png` | 5,475 m | Almost all ground absent; rectangular pages dominate |
| E01 `no_upload_hvv` r1 | `E01-r1-terrain-more-vegetation-missing-pages-remain.png` | 1,481 m | Dense vegetation/roads; underlying ground still absent |
| E01 `no_upload_hvv` r2 | `E01-r2-terrain-more-vegetation-missing-pages-remain-singo-dong.png` | 1,356 m | Dense vegetation repeats; pages and dark ground remain |
| E02 `single_queue` r1 | `E02-r1-terrain-unchanged-missing-pages-magenta-seams.png` | 2,538 m | Some vegetation; severe page loss remains |
| E02 `single_queue` r2 | `E02-r2-terrain-unchanged-missing-pages-magenta-seams.png` | 5,036 m | Very little vegetation; ground almost entirely absent |
| D02 ordinary-texture trace | `D02-r1-cockpit-missing-terrain-pages-1385m.png` | 1,385 m | More local detail is visible, but many rectangular ground pages remain absent |

This pattern is present across baseline, `no_upload_hvv`, and `single_queue`.
It therefore confounds the earlier apparent E01 improvement: both E01 captures
were low-altitude, while the clearest E00/E02 failures were high-altitude.
E01 is downgraded to **inconclusive** until the same location is captured at
matched low and high altitudes under baseline and `no_upload_hvv`.

## What the current logs show

The ordinary `PROTON_LOG=1` logs do not contain camera position or altitude.
D01b excludes D3D12 sparse/reserved resources from the reproduced path. D02
adds stable ordinary-texture identifiers, SRV mip ranges, and upload copies and
finds complete geometric uploads for most compressed mip chains. It does not
identify which resources correspond to visible terrain pages or whether the
remaining no-upload SRV class reaches a shader.

The completed telemetry still does not contain:

- camera position or altitude;
- a mapping from a resource cookie to a visible terrain page;
- placed-buffer overlap or alias-barrier correlation;
- descriptor propagation and actual draw/dispatch use;
- residency or explicit shader-LOD decisions associated with visible pages.

Only E00-r1 logs two application CPU thread names, `BlocksCache`. The same
visible failure occurs without that thread-name message in the other logs, so
the name alone is not useful telemetry. Split-barrier warnings are continuous
and have no resource or subresource identifiers.

## Protocol if testing resumes

For E03 and later runs, use the same mission and approximate map landmark, then
capture both conditions before exiting:

1. external camera below 1,500 m;
2. external camera at approximately 5,000 m;
3. same heading and field of view where practical;
4. pause before each capture;
5. record whether underlying ground, vegetation, roads, page edges, and
   magenta seams change independently.

D03 found no placed-resource range overlap for its same-run pre-cap no-upload
resource class and no explicit legacy alias barriers. E03 therefore isolates
the active descriptor-buffer backend. Descriptor QA follows only if that
control is unchanged. Logging every GPU operation indiscriminately is not
justified.
