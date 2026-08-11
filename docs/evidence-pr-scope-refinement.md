# PR scope refinement

## Result

The upstream candidate was narrowed before formal review to reduce its effect
on unrelated copies. At that stage, VKD3D-Proton PR
[#3202](https://github.com/HansKristian-Work/vkd3d-proton/pull/3202) pointed to
commit `64ec55e7ab3d34012a74e5cbe8f096d4a199e272`. The reviewed final revision
later merged as `731c4aae5991b33f2ddab45d3cb1b4779159bf4b`.

The conversion runs only when both conditions are true:

1. source and destination physical elements have the same byte size; and
2. source and destination block width or block height differs.

Formats whose block geometry already matches remain on the original
`vk_buffer_image_copy_from_d3d12()` path. Formats with unequal physical element
sizes also retain the original path. This removes the unnecessary possibility
of rounding same-geometry compressed extents through block counts.

The IL-2 case still selects the conversion: `R32G32B32A32_UINT` uses 16-byte
1x1 elements and BC3 uses 16-byte 4x4 blocks. The converted row length, image
height, and extent are identical to the D08-tested predecessor `cf11ba76` for
this path.

## Validation

- Native test build: successful.
- MinGW x64 build, including `d3d12.exe` and `d3d12core.dll`: successful.
- `VKD3D_TEST_MATCH=test_copy_buffer_texture_bc_rgba`: 22 tests executed,
  zero failures.
- `VKD3D_TEST_FILTER=copy`: 6,429,713 tests executed, zero failures, 14
  successful todo, one skipped, eight todo, zero bugs.
- `git show --check`: clean.
- Exported patch applies in reverse to the candidate worktree, confirming it
  exactly represents the amended commit.

Ignored local transcripts:

| File | SHA-256 |
|---|---|
| `captures/validation/64ec55e7-focused-copy-test.log` | `7e22cddd79fc12e5a3c03d86ba40c304fb531c07cf209bf64a078f995cb70b19` |
| `captures/validation/64ec55e7-copy-tests.log` | `697c3cf46dde73e55260c47ce85ebe984a9b6515df2acfd9df29235d708a391f` |

The archived candidate patch is
`patches/0009-vkd3d-Convert-buffer-image-copies-between-block-formats.patch`, SHA-256
`ca20fb05e712f2ae8216e65843990720a67d49c81b506245a17bb82fc0b58d2a`.

## Runtime evidence boundary

D08 loaded and tested predecessor `cf11ba76` in game. That historical build
identity and its DLL hashes remain unchanged in the D08 records. No claim is
made that `64ec55e7` or final merge `731c4aae` was a separately packaged game
run in this specific record. Instead, the narrowing adds a predicate while
leaving the conversion selected by IL-2 and all values computed inside that
branch unchanged. The focused GPU regression and complete copy-test subset
were rerun against `64ec55e7`.
