# D29 IDs-3-and-above record filter: preparation

## Question

D28 excludes light ID 4 as necessary. D29 asks whether ID 3 is required while
preserving the real tiled-light list and actual IDs/records 1 and 2.

## Exact change

D29 inserts the equivalent of:

```text
filtered_id = light_id < 3 ? light_id : 0
```

The existing zero-sentinel branch then skips IDs 3 and 4. IDs 0, 1, and 2 are
unchanged. The real `t9` start/count, loop iterations, `t10` fetches, retained
`t7` records, per-light calculations, accumulation, and outputs remain active.

For each shader the generator verifies the exact input signature, allocates
two new SPIR-V IDs, inserts one `OpULessThan` and one `OpSelect`, and rewires
exactly one sentinel comparison plus one record-address shift. The complete
disassembly diff contains only those changes and the module-bound increment.

| Shader | Original bytes | D29 bytes | D29 SHA-256 |
|---|---:|---:|---|
| `df0bd777fd1bb89d` | 76,624 | 76,668 | `81d8670b8c717599dfcdd5ff85444b19a59610ed9711eb6ed10378698c4c4523` |
| `a2d104d5c813322e` | 76,628 | 76,672 | `560ca3bb5509ee2c4acad6437ed010da41abeb354fee78d56357b8b26fc38138` |

Both outputs pass `spirv-val --target-env vulkan1.3`. The ignored local
override directory is:

`/home/USER/Documents/Codespace/Protondb/il2-korea-proton-investigation/build/d29-filter-light-ids3plus-overrides`

## Runtime configuration

Keep `IL2-Korea-D25-LightAtomicCompat-84c87c83` selected and use:

```text
PROTON_LOG=1 PROTON_LOG_DIR=/tmp/il2-D29-filter-light-ids3plus-r1 VKD3D_SHADER_OVERRIDE=/home/USER/Documents/Codespace/Protondb/il2-korea-proton-investigation/build/d29-filter-light-ids3plus-overrides %command%
```

## Interpretation

- **Grid disappears:** ID/record 3 is required, because D28 already excludes
  ID 4. Inspect that record/type and its `t7` operations.
- **Grid remains:** ID/record 2 or its interaction with ID 1 is required. A
  real-list ID-1-only filter is then the final record discriminator.
- **Lighting is not classifiable:** refine the diagnostic; do not infer a fix.

D29 is local diagnostic work, not a game modification or final workaround.

The completed visual/log result is recorded in
[`evidence-d29-filter-light-ids3plus-result.md`](evidence-d29-filter-light-ids3plus-result.md).
