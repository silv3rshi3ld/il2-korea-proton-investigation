# D26 tiled-light consumer causal gate: result

## Result

D26 completely removes the visible square/grid artifact while preserving the
main-menu aircraft, ordinary material appearance, shadows, reflections,
background, and all three pixel-shader render-target writes. Both intended
overrides are confirmed in the runtime log. This is the first causal visual
result for the shimmering track.

The artifact therefore requires the tiled dynamic-light loop in
`PixOutLight_msp` and `PixOutLight_mss`. It is not generated solely by a later
reflection-target write, blend, or composition pass. D26 is still diagnostic,
not a final fix: it takes the shaders' existing zero-light path and consequently
removes the affected dynamic-light contribution rather than correcting it.

## Run identity and activation

- Run: `D26-zero-tiled-light-r1`
- Compatibility tool: `IL2-Korea-D25-LightAtomicCompat-84c87c83`
- VKD3D-Proton build: `84c87c8390d9df7+`
- Override directory:
  `build/d26-zero-tiled-light-overrides`
- Proton log: `/tmp/il2-D26-zero-tiled-light-r1/steam-247970.log`
- Proton log size at inspection: 9,251,010 bytes

The log identifies `IL2Series.exe` and records both required substitutions:

```text
Overriding shader hash df0bd777fd1bb89d ...
Overriding shader hash a2d104d5c813322e ...
```

There is no shader-module creation failure, Vulkan device loss, validation
error, or GPU-fault signature in the inspected run. The game had exited cleanly
when the result was recorded.

## Visual evidence

- Screenshot:
  `/home/USER/Pictures/Screenshots/Screenshot_20260808_123914.png`
- Dimensions: 2560x1080
- SHA-256:
  `a72a042155d4dafb206b97eae0419f84f841c9eba12215056f06e6ef7fb25391`
- User classification: “Seems completely resolved”

Compared with the D25 image, the translucent blue/gray tile-sized blocks are
absent from the aircraft wings, fuselage-adjacent floor, and broad lit floor
regions. This is not explained by the entire target draw disappearing: the
aircraft and its normal shading remain clearly visible.

## Exact diagnostic boundary

For each target shader, D26 changes only the packed `t9` start/count fetch from
an `OpImageFetch` to `uint4(0)`. Each binary differs from its exact D25 input by
five bytes and passes `spirv-val --target-env vulkan1.3`.

The low ten bits of the fetched value are the per-tile light count. Returning
zero makes existing shader control flow:

1. skip the `t10` light-index loop;
2. leave the accumulated tiled dynamic-light RGB at zero;
3. continue all later material/reflection computations;
4. write the normal three render targets.

This causally selects something reached only when the dynamic-light count is
non-zero. It does **not** yet distinguish among:

- incorrect packed `t9` count/start values;
- incorrect `t10` light IDs;
- incorrect per-light data fetched after resolving each ID;
- a DXIL-to-SPIR-V semantic error inside the per-light calculations;
- a narrower valid-SPIR-V driver/compiler execution issue.

D25 is important to the boundary: correcting the earlier allocation atomic did
not change the visual artifact. That malformed allocation is real but is not a
sufficient explanation for D26's result.

## Next discriminator

D27 must preserve entry into the per-light loop and split list traversal from
per-light evaluation. The best first bisection is to keep the original `t9`
count/start calculation but replace only the `t10` light-ID fetch result with a
controlled value or skip only the per-light contribution after the ID is read.
The comparison must remain visually classifiable and should avoid treating a
wholly disabled light loop as the eventual fix.

No D26 shader binary or application bytecode may be uploaded. No upstream post
or patch is justified until a lighting-preserving compatibility correction is
identified and validated.
