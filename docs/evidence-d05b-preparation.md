# D05b footprint-aware BC3 diagnostic: completed

## State

D05b was compiled, installed in an isolated custom Proton tool, and run as
`D05b-bc3-r1`. It found 432 candidates but rejected all of them because the
source footprints are `R32G32B32A32_UINT`, not BC3-shaped. The unchanged image
therefore did not test normalization. See
[`evidence-d05b-result.md`](evidence-d05b-result.md).

- Base VKD3D-Proton: `84c87c8390d9df75ba41d911496296fe13f0e275`
- D05a commit: `35bd875cf58a555a64fa366926c04cd6b0664611`
- D05b commit: `f6416c79dafabcb76e2e095935dfcd0c428b9208`
- Build directory: `build/vkd3d-proton-il2-d05b-bc3-f6416c79/`
- Retained delta patch:
  `patches/0005-vkd3d-Handle-footprint-only-IL-2-BC3-border-copies.patch`

## Revision

D05b retains the opt-in `VKD3D_IL2_BC3_BORDER_COPY=1` gate and the exact
2048x2048, one-mip BC3 destination plus four observed thin shapes. It adds:

- support for both explicit source-box and footprint-only copies;
- compatible 4x4/16-byte compressed source formats rather than only exact
  `DXGI_FORMAT_BC3_UNORM` identity;
- a target-class candidate record before source safety filtering;
- physical `bufferRowLength` and `bufferImageHeight` capacity checks;
- a rejection bitmask when a candidate is unsafe to normalize;
- source representation, formats, footprint, offsets, and capacity in every
  adjustment record.

The revised analyzer requires every recorded candidate to be either adjusted
or explicitly rejected and validates block-complete emitted extents.

## Build identity

The official development build completed for both architectures.

| File | Architecture | SHA-256 |
|---|---|---|
| `x64/d3d12.dll` | PE32+ x86-64 | `120b90822e4331a83ef336a0610243d8221a36270134d3390e690a246f03b64e` |
| `x64/d3d12core.dll` | PE32+ x86-64 | `e8d6487d893a01f4261e0217a7876c1c2e9f801171fb46e856042baff20cda1c` |
| `x86/d3d12.dll` | PE32 i386 | `78ec489291be043bf092ff22fc9c0bd0fc453ac774d4aaca0745989dc0918719` |
| `x86/d3d12core.dll` | PE32 i386 | `1147120558d57849233c6e47382ceff16f66d7378a164e0e6237a20380fe355c` |

String inspection confirms the enable, candidate, rejection, adjustment, and
log-cap markers in the x64 core DLL. A synthetic footprint-only record passes
the revised analyzer.

## Installed custom Proton tool

Steam and the game were stopped before creating:

```text
/home/USER/.local/share/Steam/compatibilitytools.d/IL2-Korea-D05b-BC3-f6416c79
```

It was copied from Proton Experimental `experimental-11.0-20260724c`, then only
its packaged x64/x86 `d3d12.dll` and `d3d12core.dll` files were replaced. All
four installed hashes match the build-identity table above. The source Proton
tool and game prefix were not modified.

## Prepared run

Run ID: `D05b-bc3-r1`

Select `IL2-Korea-D05b-BC3-f6416c79` in Steam and use exactly:

```bash
PROTON_LOG=1 PROTON_LOG_DIR=/tmp/il2-D05b-bc3-r1 OMP_NUM_THREADS=16 KMP_AFFINITY=disabled VKD3D_IL2_BC3_BORDER_COPY=1 %command%
```

Observe the menu aircraft, then reproduce the terrain failure in the same
Singo-dong mission around 1,300-1,500 m. Keep settings and camera comparable to
D05a and capture the altitude in the screenshot. A second capture near 5,000 m
is useful but not required for the first discriminator.

After the game exits and Steam is fully stopped, collect with:

```bash
./scripts/collect-proton-log.sh collect D05b-bc3-r1
```

The result is behaviorally valid only if the log contains the D05b enable
marker and every candidate has either an adjustment or explicit rejection.
Selecting Proton Experimental remains the immediate rollback.
