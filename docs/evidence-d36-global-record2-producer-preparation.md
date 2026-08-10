# D36 global record-2 producer membership: preparation

## Question

D35 removes the square grid when record 2 is evaluated once for every target
pixel, but it also replaces the real consumer list and disables the returned
shadow visibility. D36 asks the narrower producer-side question:

> Does the grid disappear when the original consumers, shadows, and all real
> lights remain intact, but spotlight record 2 is conservatively included in
> every valid screen tile?

## Exact construction

D36 modifies only the final membership predicate in the two producer shaders:

| Shader | Hash | D36 predicate |
| --- | --- | --- |
| `ComputeLightsCount` | `651194bd0a21772e` | `original_membership || light_id == 2` |
| `ComputeLightsIndices` | `11e32439a86036ba` | `original_membership || light_id == 2` |

The same predicate is applied to both passes. Each dispatch has one invocation
per tile/light pair, so the `OR` includes record 2 exactly once whether or not
the original predicate already selected it. All other light IDs retain their
original membership.

D36 does not replace either consumer pixel shader, alter `t9`/`t10` reads,
substitute IDs, change spotlight math, or disable shadow sampling. Keep the
D25 compatibility tool selected only so the resulting index ranges use its
validated global 32-bit allocator instead of the application's malformed
32-bit atomic through an `R16_UINT` view.

## Input provenance

The D25 temporary shader dump has since been cleaned. Its application quirk is
keyed only to `ComputeLightsFirstRef` (`7cefa1bc80bb4c70`); it cannot alter
`ComputeLightsCount` or `ComputeLightsIndices`. D36 therefore uses the retained
D14 translations of the exact producer DXIL hashes. Both inputs validate for
Vulkan 1.3:

| Shader | Input bytes | Input SHA-256 |
| --- | ---: | --- |
| `651194bd0a21772e` | 37,748 | `80eb3dd20cc14f56bb637a963e51aafaaabe236a079b9e341ae0248b160c013d` |
| `11e32439a86036ba` | 38,380 | `31f6f919137f1838b5e7f997173197220de4f47ae1dab34e43350d187e2c1cc1` |

## Static audit

The generator refuses any input that does not have the expected light-ID
load, record-2 constant, final membership phi, structured selection merge, and
branch labels. Raw-ID disassembly shows exactly two inserted boolean
instructions and one changed branch operand in each output:

```text
is_record2 = light_id == 2
inclusive_membership = original_membership || is_record2
```

| Shader | Output bytes | Output SHA-256 | Vulkan 1.3 validation |
| --- | ---: | --- | --- |
| `651194bd0a21772e` | 37,788 | `96a1391d23b46d0685d6df368a9df37e933b094b183d1562611a9bb1f3bcb8a2` | pass |
| `11e32439a86036ba` | 38,420 | `9063bd6168fd5835890293ce4778187a37e8f265423b72ae4e1d021be861eb5e` | pass |

The output directory contains only these two modules:

`/home/USER/Documents/Codespace/Protondb/il2-korea-proton-investigation/build/d36-global-record2-producer-overrides`

The captured grid contains 2,720 tiles and `t10` has 43,520 elements. D36 can
add at most one entry per tile. Even the larger D20 frame would rise from
13,349 to no more than 16,069 entries, well below the captured capacity. The
packed per-tile count has 10 bits while the live record buffer contains only
128 records, so one added membership cannot overflow that field.

## Runtime configuration

Keep `IL2-Korea-D25-LightAtomicCompat-84c87c83` selected. Replace the previous
D35 launch options with exactly:

```text
PROTON_LOG=1 PROTON_LOG_DIR=/tmp/il2-D36-global-record2-producer-r1 VKD3D_SHADER_OVERRIDE=/home/USER/Documents/Codespace/Protondb/il2-korea-proton-investigation/build/d36-global-record2-producer-overrides %command%
```

Use the same hangar view, then check one short cockpit or fire-lit scene in the
same run. Report:

- whether the original square grid is present;
- whether normal dynamic lighting and shadows remain;
- whether the fine sandy/fading dots seen under D35 remain.

Close the game after the observation so the log can be accepted. Do not mix
the D36 directory with D34 or D35 consumer overrides.

## Interpretation

- Squares disappear with normal lighting/shadows intact: record 2's producer
  culling has false-negative tiles. Audit its DXIL/SPIR-V floating-point and
  intersection semantics, then replace global inclusion with the smallest
  conservative correction.
- Squares remain: D35's result requires more than producer membership alone.
  Stop visual variations and passively capture exact record-2 membership and
  multiplicity at the original consumers.
- Lighting disappears, the game fails, or the override markers are absent:
  reject the run rather than interpreting it visually.

D36 is a local Proton/VKD3D shader diagnostic, not a game mod and not a
shipping quirk. Nothing is to be uploaded or posted without explicit
confirmation.
