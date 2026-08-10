# D30 IDs-2-and-above record filter: preparation

## Question

D29 excludes IDs/records 3 and 4 as necessary. D30 asks whether real
ID/record 2 is required, while distinguishing real list distribution from
D27's artificial replacement of every iteration with record 1.

## Exact change

D30 inserts the equivalent of:

```text
filtered_id = light_id < 2 ? light_id : 0
```

Only genuine ID-1 list entries are evaluated. Sentinel 0 remains unchanged,
and every ID 2 or above takes the existing skip branch. The real `t9`
start/count, list positions, loop iterations, `t10` fetches, ID-1 `t7` data,
calculations, accumulation, and outputs remain active.

D30 reuses D29's audited range-filter generator and changes only the threshold
constant from 3 to 2. The disassembly diff contains the expected unsigned
comparison/select, two redirected uses, and module-bound increment. Both
outputs pass Vulkan 1.3 validation.

| Shader | Original bytes | D30 bytes | D30 SHA-256 |
|---|---:|---:|---|
| `df0bd777fd1bb89d` | 76,624 | 76,668 | `7b929a5f29ccff47a6014b0f4935f0baba935189244e58841e4bb7dc40c26930` |
| `a2d104d5c813322e` | 76,628 | 76,672 | `ded6c2c6b789791422b890c342cda7106088b1ee70e761abe73326607983a0a3` |

The ignored local override directory is:

`/home/USER/Documents/Codespace/Protondb/il2-korea-proton-investigation/build/d30-filter-light-ids2plus-overrides`

## Runtime configuration

Keep `IL2-Korea-D25-LightAtomicCompat-84c87c83` selected and use:

```text
PROTON_LOG=1 PROTON_LOG_DIR=/tmp/il2-D30-filter-light-ids2plus-r1 VKD3D_SHADER_OVERRIDE=/home/USER/Documents/Codespace/Protondb/il2-korea-proton-investigation/build/d30-filter-light-ids2plus-overrides %command%
```

## Interpretation

- **Grid disappears:** ID/record 2 is required. Resolve its `t7` data and
  isolate the first divergent record-type branch or operation.
- **Grid remains:** record 2 itself is not sufficient to explain D27 versus
  D30; focus on list distribution/traversal and the effect of substituting
  sentinel/other entries with record 1.
- **Lighting is not classifiable:** refine the diagnostic instead of claiming
  a fix.

D30 preserves a real subset of lighting but still omits records. It is not a
final compatibility solution.

The completed visual/log result is recorded in
[`evidence-d30-filter-light-ids2plus-result.md`](evidence-d30-filter-light-ids2plus-result.md).
