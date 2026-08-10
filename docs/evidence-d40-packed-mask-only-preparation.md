# D40 packed-mask-only tiled-light membership: preparation

## Goal

D40 is the complementary half of D39. It retains valid geometry and coarse
packed depth-mask overlap, while removing only the direct scalar tile min/max
overlap which D39 proves can reproduce the blocks.

## Decision

- No blocks, reduced/no D38 flicker: remove only scalar overlap in the final
  compatibility behavior and preserve the coarse packed mask.
- Blocks remain: both depth terms can expose the defect independently, so
  D38's `valid_geometry` predicate is the minimum conservative fix.
- No blocks but flicker remains: the flicker does not come from excessive
  D38 depth inclusion; compare with native temporal lighting and keep the
  narrower D40 predicate if otherwise correct.

Both outputs must differ only by `valid_geometry`, logical negation of the
existing `mask_zero` predicate, their conjunction, and the final branch
redirection. They must validate for Vulkan 1.3 and reject a wrong-shader
contract.

## Static validation

| Hash | Bytes | SHA-256 |
| --- | ---: | --- |
| `651194bd0a21772e` | 37,804 | `ae02e6250e71452ece1c05f1b3d2adf21e737129c87bbb2ab7feea5d863d08e7` |
| `11e32439a86036ba` | 38,436 | `d274046e4c3f17d8c8db0e01e120ef9b104174973083e801f875ba56faf1ee2f` |

Both validate for Vulkan 1.3. Each raw disassembly diff contains only the
SPIR-V bound increase, `near < far`, logical negation of the original
`mask_zero` predicate, their conjunction, and the final branch redirection.
The wrong-membership negative contract is rejected.


## Runtime control

Keep `IL2-Korea-D25-LightAtomicCompat-84c87c83` selected and use:

```text
PROTON_LOG=1 PROTON_LOG_DIR=/tmp/il2-D40-packed-mask-r1 VKD3D_SHADER_OVERRIDE=/home/USER/Documents/Codespace/Protondb/il2-korea-proton-investigation/build/d40-packed-mask-only-overrides VKD3D_SHADER_DUMP_PATH=/tmp/il2-D40-packed-mask-r1/shaders %command%
```
