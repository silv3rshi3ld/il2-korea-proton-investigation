# D39 scalar-depth-only tiled-light membership: result

## Result

D39 is a valid negative. The user reports that the original blocks and
artifacts remain when the packed logarithmic mask is removed but valid
geometry and the direct scalar near/far overlap are retained.

This proves that the scalar tile min/max comparison is sufficient to restore
the covered block defect. Removing only the packed mask is therefore not a
complete fix.

## Runtime verification

- Run: `D39-scalar-depth-r1`
- Compatibility tool: `IL2-Korea-D25-LightAtomicCompat-84c87c83`
- Log: `/tmp/il2-D39-scalar-depth-r1/steam-247970.log`
- Log size: 10,231,345 bytes, 104,077 lines
- Log SHA-256:
  `ab20598496ccf193e124e5aaaf585eb658a3df91a4a04a7007f9b3fddd87e5c1`
- Visual result reported by the user: blocks and artifacts present
- Intended producer overrides: exactly two
- Shader/device failure: zero

## Remaining Boolean split

The relevant predicates are now:

```text
original = geometry && packed_mask && scalar_overlap  # blocks
D38     = geometry                                    # no blocks, minor flicker
D39     = geometry && scalar_overlap                  # blocks
```

Only one complementary split remains useful:

```text
D40 = geometry && packed_mask
```

If D40 is clean, the final compatibility behavior can remove only the scalar
overlap while retaining coarse depth rejection. If D40 is defective, both
depth terms can independently expose a bad tile boundary and D38 is the
minimum conservative fix.
