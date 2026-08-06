# D05a result: valid DLL, zero matching adjustments

## Classification

**Inconclusive / invalid causal test.** The intended D05a DLL loaded, but its
normalization path adjusted zero copies. The visually unchanged terrain must
not be represented as evidence that BC3 normalization failed.

## Runtime validity

- Run: `D05-bc3-r1`
- Diagnostic commit: `35bd875cf58a555a64fa366926c04cd6b0664611`
- Base: unmodified upstream `84c87c8390d9df75ba41d911496296fe13f0e275`
- Enable markers: 1
- Adjustment records: 0
- Vulkan device loss or reported GPU reset: none
- Post-run prefix `d3d12.dll` SHA-256:
  `4a5bb2ad5349c0c79728e1c70f73ff4bf9d8c675d7ce0af64ea8b6aae273bdb9`
- Post-run prefix `d3d12core.dll` SHA-256:
  `9af9a0726ea901de60e7d712a67e65190d733feef9a1aa08ebd6e334023df7a6`

Both prefix hashes match the D05a build. The marker
`IL2BCCOPY enabled mode=normalize-observed-bc3-borders` appears exactly once,
proving that the gate was enabled in the intended binary. Its zero adjustment
count proves that the code never changed a Vulkan copy.

## Visual observation

At approximately 1,416 m, the terrain remained mostly dark/absent with isolated
rectangular pages and magenta edges. The screenshot is retained as:

```text
captures/curated/d05-bc3-normalization/
  D05a-r1-terrain-unchanged-invalid-gate-1416m.png
```

SHA-256:
`912641ebacce26ea495404dd5ac3aeb9c0b542d845f6f8f757b7fbab4d83b812`

This is a valid reproduction image, but not a before/after result because the
experimental behavior did not execute.

## Why the matcher missed

D05a required a non-null `D3D12_BOX`, an exact BC3 source format, and the final
resource/shape checks before emitting any per-copy record. D02 logged the
converted Vulkan extent, destination resource class, offsets, and formats of
created destinations, but it did not record whether the original D3D12 call
used a source box or the placed footprint directly. A footprint-only call is
the leading explanation; a compatible typeless source footprint is another
source-side possibility.

D05b removes that ambiguity. It recognizes the already-demonstrated
destination/shape class first, records every candidate, handles both source
representations, validates physical buffer capacity, and logs an explicit
rejection mask before changing anything. D05b is built and retained but has not
been installed or run.

## Retained logs

| Artifact | SHA-256 |
|---|---|
| Exact raw Proton log | `84754d72b060620d1bce05b72de463d7070a8629919216303f648e6b0cf5ba3f` |
| Deterministic compressed Proton log | `43c005339456055cfb115e95facae871c4229496a788d24b74b55a6cafee0e75` |

The compact `summary.txt`, `bc3-border-copy-analysis.md`, module list, and
system information remain under `captures/runs/D05-bc3-r1/`. Generated logs
are intentionally ignored by Git.
