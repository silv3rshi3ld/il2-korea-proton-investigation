# D26 tiled-light consumer override generator

This local diagnostic generates two VKD3D-Proton shader overrides without
modifying IL-2 files. It patches the exact `rtLightRefs25` fetch in:

- `df0bd777fd1bb89d` (`PixOutLight_msp`);
- `a2d104d5c813322e` (`PixOutLight_mss`).

The fetch result becomes `uint4(0)`. Both shaders already interpret a low-ten-
bit count of zero as “no tiled dynamic lights,” so their existing control flow
skips the light-index loop while the rest of each draw and its three render
targets remain intact. This distinguishes the tiled-light consumer chain from
a later reflection/write/blend/composition problem. It is not a final fix,
game mod, or proposed upstream shader replacement.

The generator patches the original SPIR-V binary in place at the instruction
word level. It verifies the complete expected instruction signature and
requires exactly one match, preserving every other word, numeric ID, header,
and generator field. Both outputs must pass `spirv-val`.

The default inputs are the local D25 shader dumps and are not tracked because
they contain application bytecode. Generate and validate the ignored outputs:

```text
make -C probes/d26-tiled-light-override
```

Override `DUMP_DIR=/path/to/shaders` when using another verified dump of the
same two hashes. The outputs are written to
`build/d26-zero-tiled-light-overrides/` and are loaded only when that directory
is explicitly supplied through VKD3D-Proton's developer-only
`VKD3D_SHADER_OVERRIDE` environment variable.
