# D27 fixed-light-ID loop bisection: result

## Result

D27 removed the visible square artifact while keeping the tiled dynamic-light
loop active. The two consumer pixel shaders still used the real packed `t9`
start/count, executed the original number of loop iterations, fetched the
light list, evaluated a complete light record, accumulated the results, and
wrote the normal render targets. Only the scalar light ID extracted from each
`t10` fetch was replaced with valid ID `1`.

This proves that the square artifact requires the real per-tile light IDs (or
the records selected by those IDs). It rejects the loop count by itself,
repeated accumulation by itself, and the common per-light calculation path for
record 1 as sufficient causes.

## Run verification

- Run: `D27-fixed-light-id-r1`
- Compatibility tool: `IL2-Korea-D25-LightAtomicCompat-84c87c83`
- Log: `/tmp/il2-D27-fixed-light-id-r1/steam-247970.log`
- Program: `IL2Series.exe`
- Visual result reported by the user: squares gone
- Shader/device failure: none

The completed log contains exactly the intended replacement markers:

```text
Overriding shader hash df0bd777fd1bb89d .../d27-fixed-light-id-overrides/df0bd777fd1bb89d.spv
Overriding shader hash a2d104d5c813322e .../d27-fixed-light-id-overrides/a2d104d5c813322e.spv
```

The ordinary Wine Bluetooth-driver warning and shutdown-time critical-section
messages are unrelated to the graphics comparison. There is no shader-module
failure, Vulkan device loss, GPU fault, or skipped game start.

## Narrowed boundary

The D20 prior-frame index buffers contain only IDs `0` through `4`. ID `0` is
the consumer's sentinel and is skipped. D27 shows that record `1` is safe in
the covered path even when evaluated for every original iteration. The next
test should preserve real IDs and remove one remaining record at a time,
starting with ID `4`, instead of disabling tiled lighting or replacing the
entire list.

D27 remains a diagnostic shader override, not a shippable fix. Replacing all
lights with record 1 changes lighting semantics and cannot be proposed for
Proton or VKD3D-Proton.
