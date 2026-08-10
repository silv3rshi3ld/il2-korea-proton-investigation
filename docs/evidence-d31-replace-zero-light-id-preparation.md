# D31 zero/sentinel light-ID replacement: preparation

## Question

D30 proves that no light record above ID 1 is individually required for the
square artifact. The remaining important difference from clean D27 is that
D30 mixes genuine record-1 evaluations with the shader's zero-ID skip path.

D31 isolates that difference while preserving the real lighting data. It
changes only an extracted light ID of zero to the known-safe ID 1:

```c
filtered_id = light_id < 1 ? 1 : light_id;
```

Every original nonzero ID and selected light record remains unchanged. The
existing comparison with zero therefore no longer takes the sentinel/skip
branch for entries read from the list.

## Override construction and audit

The generalized range-filter generator inserts an unsigned-less-than test and
an `OpSelect` immediately after the `t10` ID extraction in each correlated
pixel shader. The selected ID is then used by both the existing zero comparison
and the existing record-index shift. No other shader behavior is changed.

Exact binary-disassembly comparison found only the expected module-bound
change, the inserted comparison/select pair, and the two rewired operands in
each module. Both outputs pass `spirv-val --target-env vulkan1.3`.

| Shader | D31 bytes | D31 SHA-256 |
|---|---:|---|
| `a2d104d5c813322e.spv` | 76,672 | `b57cbd3f30e935dec2220435c332c62641aaca218521dc1263b74ed089125ff0` |
| `df0bd777fd1bb89d.spv` | 76,668 | `c65e5796efe4d30ecda8b42eaa513539a68e80d9b5432981ea070cd50a7a1e98` |

Override directory:

`/home/USER/Documents/Codespace/Protondb/il2-korea-proton-investigation/build/d31-replace-zero-light-id-overrides`

## Runtime configuration

Use the same D25 compatibility-tool baseline and replace only the shader
override directory:

```text
PROTON_LOG=1 PROTON_LOG_DIR=/tmp/il2-D31-replace-zero-light-id-r1 VKD3D_SHADER_OVERRIDE=/home/USER/Documents/Codespace/Protondb/il2-korea-proton-investigation/build/d31-replace-zero-light-id-overrides %command%
```

Reproduce the same hangar view, report whether the squares are gone or still
present, and close the game before log inspection.

## Interpretation gate

- If the squares disappear, original zero/sentinel entries or their mixed skip
  behavior are required. This is a much narrower and more feature-preserving
  result than D26 or D27, but is still diagnostic until the invalid contract is
  identified.
- If the squares remain, nonzero record diversity can independently reproduce
  them, and the next split must operate on the real nonzero records without
  suppressing the whole lighting feature.

D31 is a local diagnostic override, not an upload candidate or final fix.
