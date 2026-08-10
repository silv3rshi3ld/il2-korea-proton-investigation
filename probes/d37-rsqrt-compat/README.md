# D37 finite-`rsqrt` producer compatibility control

D37 applies VKD3D-Proton's existing `FIXUP_RSQRT_INF_NAN` shader quirk to
the two exact IL-2 tiled-light producer hashes:

- `651194bd0a21772e` (`ComputeLightsCount`);
- `11e32439a86036ba` (`ComputeLightsIndices`).

It does not replace the shaders, hard-code a light ID, disable shadows, or
change the consumer loop. The quirk clamps the result of a fast FP32
reciprocal square root to the largest finite FP32 value. That prevents a
degenerate normalization from turning a following `0 * infinity` into NaN.

The retained light records and earlier controls justify this boundary:

- records 1, 2, and 3 all use the producer's type-3 cone/spotlight branch;
- D30 reproduces the blocks with only genuinely tile-masked record 1;
- D32 reproduces them with tile-masked record 2;
- D27 and D35 are clean only after those sparse tile boundaries are removed;
- D36 globalizes record 2 alone, but leaves records 1 and 3 tile-masked and is
  therefore still defective;
- record 4 uses the separate type-2 branch and D28 proves it is not necessary
  for the covered artifact.

Both producer shaders contain 12 reciprocal-square-root operations in their
geometric culling calculations. Static translation with shader quirk index 10
adds exactly 12 `NMin` finite clamps to each module. The resulting modules
validate for Vulkan 1.3. Run the reproducible static check with:

```sh
make -C probes/d37-rsqrt-compat
```

For the visual control, keep the D25 compatibility tool selected so the
separate invalid `R16_UINT` allocator atomic is translated through its valid
storage-buffer path. Do not set `VKD3D_SHADER_OVERRIDE`. Point
`VKD3D_SHADER_QUIRKS` at `il2-rsqrt-quirks.conf` instead. A runtime shader dump
is useful to prove that the quirk was selected for exactly these two hashes.

D37 remains a local diagnostic until its runtime result is known. Nothing in
this directory is to be posted or uploaded without the user's explicit
confirmation.
