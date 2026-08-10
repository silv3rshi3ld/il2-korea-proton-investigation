# D32 isolate genuine light ID 2: preparation

## Question

D31 excludes zero/sentinel entries and their skip branch as necessary. D32 asks
whether genuine record 2 alone is sufficient for the square artifact.

The original per-tile list length and every loop iteration remain intact. The
extracted ID is replaced by:

```c
filtered_id = light_id == 2 ? 2 : 1;
```

Thus only genuine ID-2 entries select record 2. All other entries select the
known-safe record 1; no entry is skipped and no invalid record is accessed.

## Override construction and audit

The generator verifies the exact `t10` scalar extraction, existing zero test,
and record-index shift. It inserts one `OpIEqual` and one `OpSelect`, then
rewires only the zero test and record shift to the selected ID.

Exact disassembly comparison found only the module-bound increment, the two
inserted instructions, and those two rewired operands in each shader. The
constants were verified as unsigned ID values 2 and 1. Both modules pass
`spirv-val --target-env vulkan1.3`.

| Shader | D32 bytes | D32 SHA-256 |
|---|---:|---|
| `a2d104d5c813322e.spv` | 76,672 | `b3a343dc0e5cbf457670a9d8d577aa3666c2511cc24d2c6c2ff125434fa51d30` |
| `df0bd777fd1bb89d.spv` | 76,668 | `eb02174c091651d16b68432317ae0e6f1fec74fbe629f7564b4cad2a80f9aecb` |

Override directory:

`/home/USER/Documents/Codespace/Protondb/il2-korea-proton-investigation/build/d32-isolate-light-id2-overrides`

## Runtime configuration

```text
PROTON_LOG=1 PROTON_LOG_DIR=/tmp/il2-D32-isolate-light-id2-r1 VKD3D_SHADER_OVERRIDE=/home/USER/Documents/Codespace/Protondb/il2-korea-proton-investigation/build/d32-isolate-light-id2-overrides %command%
```

Use the same D25 compatibility-tool baseline and reproduce the same hangar
view. Report whether the squares are gone or still present, then close the game
before log inspection.

## Interpretation gate

- Squares remain: genuine record 2 is sufficient; inspect and bisect its `t7`
  fields and per-light math.
- Squares disappear: record 2 is safe in this controlled mixture; isolate
  records 3 and 4 next.
- Lighting is too incomplete to classify: refine the control rather than infer
  a fix.

D32 is a local diagnostic override, not an upload candidate or final fix.
