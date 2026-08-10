# D39 scalar-depth-only tiled-light membership: preparation

## Goal

D38 proves that the original large blocks require the common depth gate, but
it bypasses both the packed logarithmic mask and the direct scalar near/far
overlap. D39 restores the direct scalar overlap and removes only the packed
mask.

The intended predicate is:

```text
valid_geometry
&& ((light_near >= tile_min && light_near < tile_max)
    || (tile_min >= light_near && tile_min < light_far))
```

This is narrower than D38 and should reduce unnecessary light evaluation. It
is the best-supported route to retain the block fix while addressing D38's
minor residual flicker.

## Static contract

The patcher requires the exact original final-membership phi, its adjacent
structured selection, and the exact merged light/tile interval operands. It
inserts only ordered comparisons and boolean operations, then redirects the
final membership branch. Both producers must validate for Vulkan 1.3, contain
the reconstructed scalar predicate, and be the only two modules in the output
directory.

The completed static outputs are:

| Hash | Bytes | SHA-256 |
| --- | ---: | --- |
| `651194bd0a21772e` | 37,928 | `6cd0330561d36149e69ea5b4981ae147eff9a533d4bb745dee1095ca4b7dae9b` |
| `11e32439a86036ba` | 38,560 | `15c6f95d2a1021cdf3b3a38fd735aa4c057825f72bbae58cbfadeef572d42bf5` |

Both validate for Vulkan 1.3. Their raw disassembly diffs contain only nine
ordered comparison/boolean instructions, the SPIR-V bound increase, and the
final branch redirection. A wrong-membership negative contract is rejected.

## Runtime decision

- No blocks and no extra flicker: D39 is the compatibility behavior to
  integrate into a no-parameter local Proton/VKD3D build.
- No blocks but flicker remains: the flicker is not caused by D38's broader
  depth bypass; compare it with native temporal lighting before expanding the
  quirk.
- Blocks return: the packed logarithmic mask specifically is required for the
  defect and cannot simply be removed while retaining scalar overlap; inspect
  its quantization/conversion semantics directly.


## Runtime control

Keep `IL2-Korea-D25-LightAtomicCompat-84c87c83` selected and replace D38's
override path with:

```text
PROTON_LOG=1 PROTON_LOG_DIR=/tmp/il2-D39-scalar-depth-r1 VKD3D_SHADER_OVERRIDE=/home/USER/Documents/Codespace/Protondb/il2-korea-proton-investigation/build/d39-scalar-depth-overlap-overrides VKD3D_SHADER_DUMP_PATH=/tmp/il2-D39-scalar-depth-r1/shaders %command%
```
