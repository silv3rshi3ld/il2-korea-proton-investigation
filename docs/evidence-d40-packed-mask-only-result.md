# D40 packed-mask-only tiled-light membership: result

## Result

D40 is a valid negative. The user reports that the original large square
artifacts remain when valid light geometry and the packed logarithmic
depth-mask overlap are retained while the direct scalar tile min/max overlap
is bypassed.

Together with D39, this closes the Boolean split around D38:

```text
original = geometry && packed_mask && scalar_overlap  # blocks
D38      = geometry                                    # no blocks
D39      = geometry && scalar_overlap                  # blocks
D40      = geometry && packed_mask                     # blocks
```

Either depth term can independently expose the visible defect in the covered
scene. D38's geometric-membership predicate is therefore the minimum tested
compatibility behavior; retaining either depth optimization is not sufficient.

## Runtime verification

- Run: `D40-packed-mask-r1`
- Compatibility tool: `IL2-Korea-D25-LightAtomicCompat-84c87c83`
- Log: `/tmp/il2-D40-packed-mask-r1/steam-247970.log`
- Log size: 7,205,932 bytes, 82,220 lines
- Log SHA-256:
  `9de510edb4a10baddc79a8710cb67b06aa83233952b25af9c682400d9c685f13`
- Visual result reported by the user: blocks and artifacts present
- Intended producer overrides: exactly two
- Shader/device failure: zero

The log loads only the D40 overrides for `ComputeLightsCount`
`651194bd0a21772e` and `ComputeLightsIndices` `11e32439a86036ba`.
There is no stale D38 override or unrelated consumer override.

## Acceptance criterion

The fine sandy or film-grain-like lighting is not part of this defect. The
user obtained confirmation that it is also present in the native Windows
renderer. The compatibility target is removal of the large blocky squares
while retaining genuine lighting and shadows, as observed in D38.
