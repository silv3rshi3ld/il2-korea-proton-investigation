# D34 record-2 shadow-visibility result

## Result

The user reports that the shimmering squares remain with D34. Both intended
shader overrides loaded exactly once, the game exited normally, and the log
contains no shader-module creation, device-loss, GPU-fault, page-fault, or
out-of-memory signature.

D34 is built on the D32 control: genuine ID 2 selects record 2 and every other
iteration selects safe record 1. It preserves record 2's ordinary spotlight
calculation but replaces its final sampled shadow visibility with constant
`1.0`. The artifact therefore does not require the `t8` comparison-sampling
result or its optional four-tap filter.

This result does not prove every instruction in the now-unused shadow branch
is correct. It establishes that the branch's returned visibility value is not
causal for the visible squares.

## Runtime verification

- Run: `D34-shadow-visibility-r1`
- Compatibility tool: `IL2-Korea-D25-LightAtomicCompat-84c87c83`
- Log: `/tmp/il2-D34-shadow-visibility-r1/steam-247970.log`
- Log size: 6,180,212 bytes, 74,629 lines
- Log SHA-256:
  `af8cd1a96313a041ff9709858918bb2c8243f9cee63086cc15c0ec962a66d1ad`
- Visual result: squares present

The log contains one load marker for each target:

```text
Overriding shader hash df0bd777fd1bb89d .../d34-shadow-visibility-overrides/df0bd777fd1bb89d.spv
Overriding shader hash a2d104d5c813322e .../d34-shadow-visibility-overrides/a2d104d5c813322e.spv
```

## Next causal boundary

Before bisecting more record fields, separate the per-pixel record-2
calculation from the screen-tiled list that decides where and how often record
2 is evaluated. A single controlled iteration of record 2 for every pixel can
remove all `t9` count/start and `t10` membership variation while retaining the
complete distance, cone, material, and accumulation math.

- If the square grid disappears and record-2 illumination becomes spatially
  smooth, the list membership/count discontinuity is required.
- If the grid remains even with constant one-record evaluation, the remaining
  record-2 calculation or another input to these pixel shaders is sufficient.

D34 is diagnostic evidence, not a proposed compatibility fix. Nothing has
been uploaded or posted.
