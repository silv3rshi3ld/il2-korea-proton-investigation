# D49 compiler-aware ABI-safe result

> [!IMPORTANT]
> Historical implementation result. D49's visual result remains valid, but
> D50-D52 later proved that changing dxil-spirv's atomic lowering is not
> required. The paired D49 design is superseded by the preferred direction in
> the still-open Mesa MR !43672, which was not locally game-tested here. The
> later dxil-spirv PR #296 and VKD3D-Proton PR #3207 were closed unmerged as
> superseded. See
> [`evidence-d50-d52-r32-alias-result.md`](evidence-d50-d52-r32-alias-result.md).

## Purpose

D49 tests the tiled-light correction after replacing the first one-repository
prototype with an ABI-safe, paired dxil-spirv and VKD3D-Proton design. It asks
whether the compiler can legalize the affected typed UAV atomic while the
backend selects a raw SSBO only on descriptor layouts that can provide one.

The source bases are:

- VKD3D-Proton: `731c4aae5991b33f2ddab45d3cb1b4779159bf4b`;
- dxil-spirv: `edd8fdf702c3445eb659f2652d04436ed86e4206`.

These identify the bases used for the D49 build. At test time, the local
dxil-spirv candidate was commit
`afff4dfb3e51ab81a4d541011bcf7ec2f65e2ffa`, and the dependent VKD3D-Proton
integration deliberately had no final commit or gitlink identity. Later
revisions were published as dxil-spirv PR #296 and VKD3D-Proton PR #3207.
Both were closed unmerged after D50-D52 superseded this design.

## ABI-safe compiler and backend contract

D49 does not grow an existing public C callback structure. The additive
dxil-spirv quirk is opt-in, and the existing remapper contract is used as
follows:

1. The resource remains a semantic `TypedBuffer` in dxil-spirv's internal
   resource metadata.
2. For an eligible compatibility lowering, dxil-spirv presents a temporary
   effective `RawBuffer` kind only to the descriptor remapper.
3. The lowering is accepted only when remapping succeeds with an SSBO.
4. If the raw request is rejected or returns another descriptor type,
   dxil-spirv restores the original typed request and retries the normal path.

VKD3D-Proton passes the quirk only when raw SSBO descriptors are available and
`VKD3D_BINDLESS_MUTABLE_TYPE_RAW_SSBO` is not active. The latter is the
mutable-single-set layout where a typed descriptor cannot safely be consumed as
an SSBO. Such unsupported layouts retain the typed fallback; D49 does not claim
to repair the application there.

The VKD3D-Proton application scope remains exact:

- executable: `IL2Series.exe`;
- shader hash: `0x7cefa1bc80bb4c70` (`ComputeLightsFirstRef`).

The compiler eligibility is also narrow. D49 does not lower 64-bit atomics,
sparse resources, typed UAVs without atomic use, or the SM 6.6 heap path.

## Translator, fallback, and build validation

- The focused dxil-spirv `resources` reference suite passes with the pinned
  DXC build.
- The full translator suite has one SPIR-V validator failure at
  `control-flow/switch-continue.frag`. The same failure reproduces on an
  unmodified worktree at exact base `edd8fdf7`, so it is not a D49 regression.
- The private captured allocator shader converts and validates in candidate,
  fallback, and no-quirk modes. It is not included in this repository.
- The candidate output SHA-256 is
  `133485bad4503037bc64e6d821b0f0b824ba24896572df1ea3cc15029064b351`.
  The fallback and no-quirk outputs are byte-identical at
  `c205a659cefd4dc45cd5fc55ff975c7b8d1119391ca54ac1fe97ca54c8ba8844`.
- The exact VKD3D remapper harness emits a valid `StorageBuffer` access and
  `OpAtomicIAdd` with the capability enabled. Its output SHA-256 is
  `0328a3a3c6023aea32e5e0b06b6c97a6225c97467846c91e5c9a49e5c01de6d7`.
- With the capability disabled, the harness output is byte-identical to the
  normal typed baseline and has SHA-256
  `f99c3e226f7be3cda3ec94387dc06ee4b40f4557c2250f102adc8c856bb398a1`.
- Clean VKD3D-Proton x86-64 and x86 builds and the complete D49 package build
  pass.

## Built and installed artifacts

The isolated Proton tool is
`IL2-Korea-D49-CompilerAware-ABISafe-731c4aae`. The installed files match the
fresh package:

| Architecture | File | SHA-256 |
| --- | --- | --- |
| x86-64 | `d3d12.dll` | `ae0436e5a8c8b9bb597288ac9846de3c01214f06a10f745918ac41ce4770e84a` |
| x86-64 | `d3d12core.dll` | `8d31d58966183707b7f73d05e763cb79fa27707f1eee3965be72a87a6a2a01af` |
| x86 | `d3d12.dll` | `1285974667c4b974baf82aea0a903d8bc7eeba8992a41a4bfc6c36d07f2d7993` |
| x86 | `d3d12core.dll` | `9cdd2eb9d326eea278dc0449d069cf2442001210874bb72a2c3c98a5aeef1024` |

The live process path was checked and resolved through the D49 compatibility
tool rather than another installed Proton tool.

## Runtime result

The final test was performed on 2026-08-10 with game build `24615759`, an
RX 7800 XT, and Mesa/RADV 26.1.6. Steam launch options were empty. The user
checked the rotating menu aircraft, then played a short flight and inspected
terrain and the map. The run was not separately timed. The large square blocks
and broad lighting flicker were absent. Normal lighting and shadows were
retained, and the terrain and map rendered correctly.

The fine sandy or film-grain lighting is excluded from the acceptance result
because it has independently been confirmed on native Windows.

The reviewed 2560x1080 post-test screenshot is stored as
[`images/lighting-after-d49-compiler-aware-731c4aae.png`](images/lighting-after-d49-compiler-aware-731c4aae.png).
Its SHA-256 is
`def6b7aa9be228e8dc031d06f7565c399e0fc4651eeda174c1046b6035e5187e`.

## Conclusion and evidence boundary

D49 validates the compiler-aware, ABI-safe paired behavior on the reporting
RX 7800 XT RADV system. It confirms that the corrected architecture retains the
clean visual result previously demonstrated by D47 and U01 without changing an
existing public C callback struct.

This is one-system runtime validation, not cross-vendor proof. It does not
claim that the paired changes are merged, accepted upstream, or safe to enable
on descriptor layouts which cannot provide the raw SSBO sibling. Those layouts
deliberately keep the existing typed path.

## D50-D52 supersession

The D49 detour was useful but is no longer the proposed architecture. D50
changed only the view format on one 87,040-byte buffer in an R32, R16, R32
sequence and reproduced corruption only with R16 on both tested RADV devices.
D51 ran the exact captured shader with a full-size R32 alias and passed on both
devices through both descriptor backends. D52 then reproduced the clean game
result twice while leaving dxil-spirv unchanged at
`cc75a0c98d34d7bcc03560527c799b52e48b4d1f`.

The D52 shader retained its natural `R32ui` texel-buffer type,
`OpImageTexelPointer`, and `OpAtomicIAdd`; only its descriptor set and binding
changed. This shows that D49's SSBO lowering is sufficient but not necessary.

Maintainer reproduction led to
[Mesa MR !43672](https://gitlab.freedesktop.org/mesa/mesa/-/merge_requests/43672),
which changes RADV's GFX10+ texel-buffer OOB selection to match native AMD
D3D12 and pre-GFX10 behavior. NVIDIA also passes the maintainer's test with a
descriptor heap. That driver-level behavior is cleaner and more general than
either D49 or the per-game D52 alias. D49 remains historical validation and
should not be described as merge-ready or current. dxil-spirv PR #296 and
VKD3D-Proton PR #3207 are closed, neither was merged, and no lasting upstream
change resulted. Mesa MR !43672 remains open and is the preferred upstream
path. Its exact revision has not been locally game-tested.
