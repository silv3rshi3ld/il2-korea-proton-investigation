# D33 live t7/t8 descriptor-contract result

## Result

The passive D33 run completed successfully and the user reported that the
shimmering squares remained visible. That visual result is expected because
D33 changes no rendering. Its purpose was to resolve the apparent adjacent
`t7`/`t8` reversal seen in the raw D20 capture.

The reversal is not present in the live D3D12 descriptor state. For both
target pixel shaders, every covered draw resolves the intended resources:

| Register | Live descriptor |
| --- | --- |
| `t7` | `R32G32B32A32_FLOAT` buffer SRV, 128 elements (2,048 bytes), cookie 162 |
| `t8` | `4096x2048` 2D `R16_UNORM` SRV backed by a `D16_UNORM` resource, cookie 3118 |
| `t9` | `80x34x2 R32_UINT` 3D SRV, cookie 4002 |
| `t10` | 43,520-element `R16_UINT` buffer SRV over 87,040 bytes, cookie 4001 |

Each of `t7`, `t8`, `t9`, and `t10` resolved 1,133 times for each shader,
for 9,064 successful target lookups and zero unresolved target lookups. All
use root parameter/table 0, table base 13,788, and the expected logical heap
offsets 13,795 through 13,798. The matching root signature has one SRV range
covering `t0` through `t22`, so register 7 is safely inside the declared
range. The apparent reversal in the raw Vulkan descriptor bytes was therefore
an interpretation error caused by the mutable-descriptor physical encoding,
not a live binding error.

The accepted log is:

- path: `/tmp/il2-D33-t7-t8-contract-r1/steam-247970.log`;
- size: 11,918,363 bytes, 86,015 lines;
- SHA-256: `45c19bcdfa2b526bbdf2b3e7a59307b79d3098a8b81ea11004bc0f322de2f73e`;
- embedded build: `1800206168f9d43`;
- no device-loss, out-of-memory, page-fault, or failed target-lookup signature.

## CPU-only backing-data check

After the live trace identified `t7`, the retained D20 captures were parsed
again with the correct 2,048-byte `R32G32B32A32_FLOAT` view. Each frame has one
matching candidate at the same captured address. All 128 float4 values are
finite. Records 0 through 4 are populated and records 5 through 15 are zero,
so light ID 2 is in bounds and has a complete eight-float4 record.

The useful contrast is between the D27-safe record 1 and the D32-sufficient
record 2:

- record 1 has no shadow flag and its transform fields 4 through 7 are zero;
- record 2 is a spotlight with its shadow flag encoded in field 1, has a valid
  projection transform in fields 4 through 7, and selects shadow slot 0 or 1
  in the two retained frames;
- the changing slot is encoded as the integer part of a finite field whose
  fractional component remains stable. It is not random corruption.

Static inspection of the validated target SPIR-V confirms the corresponding
control flow. Record 2 enters the shadow-projection branch, constructs
projected coordinates, and samples `t8` with comparison sampling through
logical sampler `s5`, including an optional four-tap filter. Record 1 does not
enter that branch.

This does not yet prove that comparison sampling is wrong. It narrows the next
causal test to a single operation boundary: preserve record 2's ordinary light
math but replace only its shadow visibility result. If that removes the
squares, the shadow projection/comparison path is causal; if it does not, the
remaining ordinary record-2 spotlight math is causal.

The RenderDoc captures may contain game assets and remain local-only. Nothing
from this result has been uploaded or posted.
