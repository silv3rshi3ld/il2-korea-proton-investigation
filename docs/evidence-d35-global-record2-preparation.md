# D35 constant one-record-2 evaluation: preparation

## Question

D34 excludes the returned shadow comparison value. D35 now separates two
remaining explanations:

1. real `t9`/`t10` tile membership or iteration multiplicity creates the
   square discontinuities; or
2. record 2's ordinary per-pixel spotlight calculation can create them even
   without the real tiled list.

## Exact construction

D35 builds on the accepted D34 modules and retains the fully-visible shadow
control. For each of the two target shaders it makes the packed tiled-list word
equivalent to start 0, count 1 and makes both outcomes of the existing D32
selection choose record 2.

Every target pixel therefore evaluates record 2 exactly once. The test removes
variation in:

- the real `t9` per-tile count and start offset;
- `t10` membership;
- repeated per-tile accumulation.

It preserves record 2's complete position/distance, spotlight direction/cone,
color, material response, ordinary attenuation, and final accumulation. The
existing `t10` fetch remains in the module, although its returned ID no longer
selects the record.

## Interpretation

- Squares disappear and record-2 illumination is smooth: real tiled-list
  membership/count/multiplicity is required; inspect the producer/culling
  contract rather than bisecting record values.
- Squares remain: record 2's ordinary calculation or another non-list input to
  these exact shaders is sufficient.
- The target rendering disappears or otherwise becomes unclassifiable: reject
  the result and refine the diagnostic.

D35 is a local VKD3D shader diagnostic. It changes no game file and is not a
proposed fix or mod. Nothing is to be uploaded or posted without explicit
confirmation.

## Build and audit

The generator requires the exact D34 input contract: the target `t9` fetch,
the D32 record-2 selection, and D34's fully-visible visibility multiply. It
then changes only the grid-fetch instruction and the false operand of the
existing selection. Raw-ID disassembly confirms no other logical difference.
File sizes and ID bounds remain unchanged.

| Shader | Bytes | SHA-256 | Vulkan 1.3 validation |
| --- | ---: | --- | --- |
| `df0bd777fd1bb89d` | 76,668 | `174a5b7b1ab9cf9acb2d1a4ca301ce3967dfc094f722c3585520a75cd5800cd3` | pass |
| `a2d104d5c813322e` | 76,672 | `b95969dff974ea3872155d1aed287f074a5195878580cc688df368e17bc3e864` | pass |

Override directory:

`/home/USER/Documents/Codespace/Protondb/il2-korea-proton-investigation/build/d35-global-record2-overrides`

## Runtime configuration

Keep `IL2-Korea-D25-LightAtomicCompat-84c87c83` selected and use:

```text
PROTON_LOG=1 PROTON_LOG_DIR=/tmp/il2-D35-global-record2-r1 VKD3D_SHADER_OVERRIDE=/home/USER/Documents/Codespace/Protondb/il2-korea-proton-investigation/build/d35-global-record2-overrides %command%
```

Check the same hangar view. The important classification is not merely
"squares/no squares": report whether the record-2 illumination looks smooth,
square-bounded, absent, or otherwise broken. Close the game before accepting
the log.

The run is complete. See
[`evidence-d35-global-record2-result.md`](evidence-d35-global-record2-result.md).
