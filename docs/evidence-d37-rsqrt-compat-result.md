# D37 finite-`rsqrt` tiled-light producer control: result

## Result

D37 is a valid negative. The user reports that the original large artifacts
returned while the exact two tiled-light producers used
`FIXUP_RSQRT_INF_NAN`.

This excludes reciprocal-square-root infinity/NaN propagation as a sufficient
explanation for the covered square blocks. It does not weaken the established
tiled-membership boundary: D27, D30, D32, D35, and D36 still show that sparse
per-tile placement of genuine local lights is required. D37 only moves the
remaining cause past the producer's normalization operations.

## Runtime verification

- Run: `D37-rsqrt-compat-r1`
- Compatibility tool: `IL2-Korea-D25-LightAtomicCompat-84c87c83`
- Log: `/tmp/il2-D37-rsqrt-compat-r1/steam-247970.log`
- Log size: 7,312,835 bytes
- Log SHA-256:
  `8f2b1e076417399c66741604529079c5842aaf7741e2c9ba900fc1b1ed635ba3`
- Visual result reported by the user: original artifacts returned
- Shader overrides loaded: zero
- Shader/device failure: none

The log identifies `IL2Series.exe`, selects the D25 build, parses only the two
requested quirk entries, and applies quirk mask `0x40000000` to exactly:

- `651194bd0a21772e` (`ComputeLightsCount`);
- `11e32439a86036ba` (`ComputeLightsIndices`).

The runtime-dumped SPIR-V modules have these identities:

| Hash | Bytes | SHA-256 |
| --- | ---: | --- |
| `651194bd0a21772e` | 38,100 | `227f79cae0e950ad3f3845c377a51c2f08f6c433c9605e424a17f9852ba47400` |
| `11e32439a86036ba` | 38,732 | `3a1b65c92431d7d8a8e62ec79653d0e643c67318d0774223d6b8421630edda9a` |

Each contains 12 `InverseSqrt` operations and 27 `NMin` operations: the
baseline 15 plus exactly 12 finite clamps. Both validate for Vulkan 1.3.

## Next causal boundary

The producer first computes a geometric near/far interval for the light volume
against the screen tile. It then performs a common, later rejection using the
tile's packed depth mask and min/max depth. D37 leaves that final gate intact.

The next useful control is not another light-ID filter or floating-point quirk.
It should preserve the geometric X/Y light-volume culling and replace only the
final depth-gated membership with the already-computed valid geometric
interval predicate. This is conservative, applies to all genuine local-light
classes, and is substantially narrower than globally assigning every light to
every tile.

Nothing from D37 is to be posted or uploaded without explicit user
confirmation.
