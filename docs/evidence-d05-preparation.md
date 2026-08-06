# D05 gated BC3 border normalization: preparation

## Question

D02 demonstrates that the game's baked-terrain cache issues 432 one-texel BC3
border uploads which VKD3D-Proton forwards as non-block-complete Vulkan copy
regions. D05 asks one causal question: does making only those regions valid at
the Vulkan boundary change the visible terrain corruption?

This is a diagnostic compatibility behavior, not a permanent fix. It remains
disabled unless `VKD3D_IL2_BC3_BORDER_COPY=1` is present.

## Exact source and change

- Unmodified D04 base: `84c87c8390d9df75ba41d911496296fe13f0e275`
- D05 commit: `35bd875cf58a555a64fa366926c04cd6b0664611`
- Retained patch:
  `patches/0004-vkd3d-Add-gated-BC3-border-copy-diagnostic-for-IL-2-.patch`
- Source branch: `il2-bc3-border-normalization-diagnostic`
- Gate: `VKD3D_IL2_BC3_BORDER_COPY=1`

The gate changes only buffer-to-image copies satisfying all of these observed
conditions:

- destination is a 2048x2048, one-layer, one-mip `DXGI_FORMAT_BC3_UNORM`
  `TEXTURE2D`;
- source and destination have matching 4x4 BC3 block geometry;
- source and destination offsets are four-texel aligned;
- depth is one and the adjusted region remains inside the destination mip;
- the original extent is exactly `1x64`, `64x1`, `1x128`, or `128x1`.

For a vertical border it emits `4x64` or `4x128`; for a horizontal border it
emits `64x4` or `128x4`. Each physical BC3 block occupies 16 bytes. The runtime
log includes the source footprint width, height, depth, row pitch, source box,
destination offset, original extent, and emitted extent. The analyzer rejects
a record whose footprint row cannot contain the required physical BC3 data.

The patch deliberately does not detect the executable or enable itself for
AppID 247970. Requiring an explicit environment gate prevents accidental use
as a game override before the behavior is validated.

## Build and binary identity

The official development-build method completed without compiler errors for
x86-64 and x86:

```bash
./package-release.sh il2-d05-bc3-35bd875c ../../build --dev-build
```

Build directory: `build/vkd3d-proton-il2-d05-bc3-35bd875c/`

| File | Architecture | SHA-256 |
|---|---|---|
| `x64/d3d12.dll` | PE32+ x86-64 | `4a5bb2ad5349c0c79728e1c70f73ff4bf9d8c675d7ce0af64ea8b6aae273bdb9` |
| `x64/d3d12core.dll` | PE32+ x86-64 | `9af9a0726ea901de60e7d712a67e65190d733feef9a1aa08ebd6e334023df7a6` |
| `x86/d3d12.dll` | PE32 i386 | `f85db40755d2f465bc057e91b35565a2a932682633400dde054ad141b293bb01` |
| `x86/d3d12core.dll` | PE32 i386 | `f2d5b77a0b4b71decb162fc26071c4d4a2070f4dbfa5a696208588c1115fe8c2` |

String inspection of the x64 core DLL confirms the environment gate and the
`IL2BCCOPY enabled`, `IL2BCCOPY adjust`, and bounded-log marker formats.

## Isolated custom Proton tool

Steam and the game were stopped before creating:

```text
/home/silv3rshi3ld/.local/share/Steam/compatibilitytools.d/IL2-Korea-D05-BC3-35bd875c
```

It was copied from Proton Experimental
`experimental-11.0-20260724c`, then only its packaged x64/x86 `d3d12.dll` and
`d3d12core.dll` pairs were replaced. Recursive comparison against the source
tool reports exactly those four DLL differences plus
`compatibilitytool.vdf` and `il2-korea-diagnostic-metadata.txt`. Installed and
build hashes match. Proton Experimental, the game prefix, D04, and earlier
diagnostic tools were not modified.

Rollback is selecting Proton Experimental or the retained D04 tool. Do not
delete a custom compatibility tool while Steam is running.

## Prepared run

Run ID: `D05-bc3-r1`

1. Start Steam and open the game's **Properties → Compatibility** page.
2. Enable **Force the use of a specific Steam Play compatibility tool** and
   select `IL2-Korea-D05-BC3-35bd875c`.
3. In **Properties → General → Launch Options**, paste exactly:

```bash
PROTON_LOG=1 PROTON_LOG_DIR=/tmp/il2-D05-bc3-r1 OMP_NUM_THREADS=16 KMP_AFFINITY=disabled VKD3D_IL2_BC3_BORDER_COPY=1 %command%
```

4. Start the game. In the menu, observe whether the rectangular aircraft
   blocks change; D05 is not expected to target that resource class, but the
   observation guards against an unexpected regression.
5. Load the same mission used for D04. Reach approximately 5,000-6,400 m and
   reproduce the broad missing/black terrain pages. Look especially at the
   magenta page edges. Keep the run long enough for the affected terrain cache
   to populate, then take one screenshot with the altitude visible.
6. Exit the game normally if possible. `Alt+F4` is acceptable if the game does
   not provide a reliable exit path. Then fully exit Steam.
7. Report the screenshot path and classify these separately: magenta seams,
   missing/black pages, menu blocks, and any GPU hang or crash. The log will be
   collected and analyzed automatically.

The collector will run `scripts/analyze-bc3-border-copy.py`. A valid run needs
one enable marker, at least one adjustment, contiguous bounded sequence
numbers, only the four expected original/emitted shape pairs, aligned offsets,
complete source BC3 rows, and no destination overrun. The D02 count of 432 is
a reference, not a hard validity requirement because cache demand and run
duration can change it.

## Decision rule

- **Seams and pages improve:** the invalid border upload poisons more of the
  baked-terrain cache than the intended edge; repeat once before designing a
  narrowly scoped compatibility remedy.
- **Only seams improve:** the invalid-copy defect is visually confirmed, but
  the page-visibility defect is separate. Retain a narrow seam remedy and move
  the next trace to the 2048x2048 cache descriptor/copy-to-sample path.
- **No visual change with valid adjustments:** preserve the invalid API usage
  finding for the game developer, but do not present normalization as the root
  cause of the visible failure.
- **No adjustments or invalid analyzer result:** do not interpret the visual
  result; first correct the gate or the source-footprint assumption.

No application override will be implemented from a single D05 run.
