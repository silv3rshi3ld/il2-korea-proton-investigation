# D05b footprint-aware BC3 diagnostic: paused preparation

## State

D05b is compiled and retained, but is **not installed and has not been run**.
The investigation is paused before another runtime test.

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
the revised analyzer. No custom Proton tool was created for D05b, so selecting
Proton Experimental remains the normal rollback.
