# D19 ACO forced-wait control: result

## Result

The square/shimmering artifact is **unchanged** with
`ACO_DEBUG=force-waitcnt`. The supplied close-up shows the same regular tile
grid across the aircraft wing and the reflected light on the hangar floor.
It is strongest where bright, specular, or reflected light contributes and is
comparatively absent in shadowed areas.

This observation does not prove that reflection itself is the cause. The two
covered pixel shaders write a reflection-related target while consuming the
tiled dynamic-light data, so reflection may be an output or visibility
condition. The tiled-light contribution remains the narrower lead.

Screenshot:

- `D19-r1-aco-waitcnt-light-grid-unchanged.png`
- SHA-256:
  `d47bcd009c5529ad39279d54f80177ff0fc5abe68d73f1c9c761f15c5c56ae35`

## Run identity

- Run: `D19-aco-force-waitcnt-r1`
- Game build: `24615759`
- Proton tool: `IL2-Korea-D16-DescriptorTrace-274f6f8e`
- VKD3D-Proton: `274f6f8e2d5b785fa871cedb0e3267e6a2af9abf`
- Mesa/RADV: `26.1.6`
- Kernel: `7.1.6-1-cachyos`
- Added compiler control: `ACO_DEBUG=force-waitcnt`
- RADV control: `RADV_DEBUG=startup`
- OpenMP/topology override: none

The log contains the expected D16 build marker, RADV startup output, and the
descriptor-sidecar enable marker. It contains no unknown debug option, Vulkan
device loss, OOM, GPU reset, page fault, or hang signature.

## Runtime coverage

The D16 descriptor gate covers approximately 75 seconds, from log timestamp
`4196.215` through `4271.226`:

- 9,404 descriptor lookups
- 9,404 successful resolutions
- zero failures
- 2,351 events for each shader/register pair

Every `t9` event resolves to cookie 4002, named `rtLightRefs25`, viewed as an
`80x34x2 R32_UINT` Texture3D. Every `t10` event resolves to cookie 4001, an
87,040-byte buffer viewed as 43,520 `R16_UINT` elements. The bindings and view
metadata are identical to D16-D18.

## Pipeline-cache validity

The VKD3D-Proton log says that its normal disk pipeline archive is available.
That does not invalidate this control. Mesa 26.1.6
[`radv_device.c`](https://gitlab.freedesktop.org/mesa/mesa/-/blob/mesa-26.1.6/src/amd/vulkan/radv_device.c#L1120-1124)
explicitly disables pipeline caching when ACO code-generation flags are active:

```c
if ((instance->debug_flags & RADV_DEBUG_NO_CACHE) ||
    (pdev->use_llvm ? 0 : aco_get_codegen_flags()))
   return true;
```

The adjacent source comment names `ACO_DEBUG` as one of the cache-disable
conditions. D19 therefore exercised freshly compiled ACO shaders with forced
wait states rather than silently reusing ordinary cached machine code.

## Decision

D19 excludes a normal ACO outstanding-operation wait-state hazard as the cause
of the reproduced artifact. Together, D15-D19 now weaken or exclude:

- missing application-supplied UAV/final-read dependencies;
- wrong `t9`/`t10` descriptor selection, type, or shape;
- ordinary cross-dispatch cache visibility;
- DCC as the sole cause or remedy; and
- an ACO wait-state scheduling omission.

No global debug option or game-specific workaround is justified. Further
broad launch-option tests are lower value. The next discriminator is a local
produced-value capture or validation of the `t9` grid and `t10` index buffer at
the final producer-to-consumer boundary.

Nothing from D19 was uploaded or posted.
