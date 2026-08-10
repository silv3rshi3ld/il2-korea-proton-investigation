# D28 real-ID record bisection: preparation

## Question

D27 proves that real light IDs or the `t7` records they select are required for
the square artifact. D28 asks whether captured light ID 4 is required while
preserving the other real light IDs and almost all of the tiled-light feature.

## Exact change

The D20 prior-frame lists contain only IDs 0–4. The consumers already treat ID
0 as a sentinel. D28 inserts `filtered_id = light_id & 3` immediately after the
real `t10` fetch and uses `filtered_id` for the existing sentinel comparison
and `t7` record address. In the observed range this mapping is exact:

| Input ID | D28 ID | Effect |
|---:|---:|---|
| 0 | 0 | existing sentinel, unchanged |
| 1 | 1 | real record preserved |
| 2 | 2 | real record preserved |
| 3 | 3 | real record preserved |
| 4 | 0 | only this record is skipped |

D28 otherwise preserves the real `t9` start/count, loop iterations, `t10`
fetches, record data, per-light calculations, accumulation, and outputs.

For each shader the generator:

1. verifies the exact light-ID `OpCompositeExtract` signature;
2. allocates one new SPIR-V ID and inserts one five-word `OpBitwiseAnd`;
3. rewires exactly one zero comparison and one record-index shift;
4. rejects any unexpected match count; and
5. validates the result for Vulkan 1.3.

The complete disassembly diff contains only the module-bound increment, the
new `OpBitwiseAnd`, and those two operand substitutions.

| Shader | Original bytes | D28 bytes | D28 SHA-256 |
|---|---:|---:|---|
| `df0bd777fd1bb89d` | 76,624 | 76,644 | `51c2a1bf29d4cee306705b7a90e4cf111a773fd708aa3341657c312315db6e5a` |
| `a2d104d5c813322e` | 76,628 | 76,648 | `e0e1dd44e204a012bebbf9a28ea0e795e0b479414359a8018a9fcdba9c41a586` |

The ignored local override directory is:

`/home/USER/Documents/Codespace/Protondb/il2-korea-proton-investigation/build/d28-filter-light-id4-overrides`

## Runtime configuration

Keep `IL2-Korea-D25-LightAtomicCompat-84c87c83` selected and use:

```text
PROTON_LOG=1 PROTON_LOG_DIR=/tmp/il2-D28-filter-light-id4-r1 VKD3D_SHADER_OVERRIDE=/home/USER/Documents/Codespace/Protondb/il2-korea-proton-investigation/build/d28-filter-light-id4-overrides %command%
```

The completed log must contain replacement markers for both hashes and no
shader-module or Vulkan-device failure.

## Interpretation

- **Grid disappears with useful lighting retained:** ID 4 or its selected
  `t7` light record/type is required. Resolve that record's runtime view and
  isolate the exact operation rather than shipping the filter.
- **Grid remains:** ID 2, ID 3, a multi-record interaction, or an earlier list
  interpretation remains causal. Continue record-preserving filters.
- **Lighting is too incomplete to classify:** refine the test; do not call it
  a fix.

D28 is a local diagnostic override, not a game modification or a candidate
Proton/VKD3D-Proton solution.

The completed visual/log result is recorded in
[`evidence-d28-filter-light-id4-result.md`](evidence-d28-filter-light-id4-result.md).
