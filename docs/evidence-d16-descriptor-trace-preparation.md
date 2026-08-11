# D16 tiled-light descriptor resolution: preparation record

## Purpose

D15 proves that the final tiled-light grid and index buffer receive the
application-supplied UAV dependencies and shader-read transitions. D14 fixes
the two corresponding pixel-shader inputs at SRV `t9` (`g_tLightsList`) and
`t10` (`g_bufLightsIndices`). D16 resolves those exact table slots when either
affected pixel shader draws.

The trace does not replace a shader, descriptor, or resource; insert a barrier;
change a Vulkan command; or enable VKD3D descriptor QA. It keeps a CPU-side
record of application descriptor creation and copy operations, follows the
root-signature table mapping at the draw, and reports the resource/view
metadata associated with the final heap offsets. The diagnostic sidecar and
mutex add CPU work and memory use, so D16 is render-passive but not intended as
a performance measurement.

## Source and build identity

- VKD3D-Proton diagnostic commit:
  `274f6f8e2d5b785fa871cedb0e3267e6a2af9abf`
- Parent D15 telemetry commit: `9c6a4338a2eff9f`
- Terrain-fix base: `cf11ba76a1cdbee`
- Wine base: exact MR !11604 D10 package
- Custom Proton tool: `IL2-Korea-D16-DescriptorTrace-274f6f8e`
- Trace gate: `VKD3D_IL2_DESCRIPTOR_TRACE=1`
- OpenMP/topology overrides: none
- Full descriptor-QA mode: disabled

Both architectures compile cleanly and embed VKD3D build identity
`0x274f6f8e2d5b785`. Meson reports no configured test targets for these retained
release-build directories.

| Packaged file | SHA-256 |
|---|---|
| x64 `d3d12.dll` | `1642a053e5d9dbbf17adef2c383303a11062479f65e467bfff75ff1916adf369` |
| x64 `d3d12core.dll` | `3744cc303c2f5f95a97c9b4ca26df0973aba3719eeb62498b27d1ad81d08403a` |
| x86 `d3d12.dll` | `471d9050a29f66154ffdf75f1ec2d07c9f03b0307da08216d17c67091ed8e4fc` |
| x86 `d3d12core.dll` | `baae63a75f7df51cd64b99ba827ae44fa338623d79db2a5230be225720c22c52` |

The installed custom-tool hashes match the build outputs exactly. The source
D10 tool and the game prefix were not modified.

## Trace boundary

D16 records descriptor writes and copies for CBV/SRV/UAV heaps in a private
sidecar. At direct or indexed draws using pixel shader
`df0bd777fd1bb89d` (`PixOutLight_msp`) or `a2d104d5c813322e`
(`PixOutLight_mss`), it:

1. finds the pixel-visible SRV binding range containing register 9 or 10;
2. maps its dense descriptor-table index back to the D3D12 root parameter;
3. combines the live table base, range offset, and register offset;
4. bounds-checks the resulting index against the currently bound resource
   heap; and
5. reports the sidecar's descriptor type, resource cookie and shape, view type
   and format, and buffer range fields.

The path remains the normal RDNA3 descriptor implementation. This matters
because `VKD3D_CONFIG=descriptor_qa` would deliberately select validation
structures and shader instrumentation, changing the descriptor path under
investigation.

The expected image mapping and initial size-only buffer interpretation were:

- `t9`: an SRV of the `80x34x2 R32_UINT` 3D `rtLightRefs*` resource;
- `t10`: an SRV of the 87,040-byte buffer. D16 later resolved its actual view
  as 43,520 `R16_UINT` elements, equivalent to
  `80 * 34 * 16 * sizeof(uint16_t)`, rather than the initial eight-`uint32_t`
  size interpretation.

The trace also emits the selected `rtLightRefs*` resource name and cookie, so
the image descriptor can be correlated without relying on its raw Vulkan
descriptor bytes.

## Prepared run

- Run ID: `D16-descriptor-trace-r1`
- Launch options:

```text
PROTON_LOG=1 PROTON_LOG_DIR=/tmp/il2-D16-descriptor-trace-r1 VKD3D_IL2_DESCRIPTOR_TRACE=1 %command%
```

Protocol:

1. Start Steam after the new compatibility tool was created while Steam was
   fully stopped.
2. Select `IL2-Korea-D16-DescriptorTrace-274f6f8e` for AppID 247970.
3. Use the exact launch options above.
4. Start the game, leave the affected main-menu aircraft visible for about ten
   seconds, and note whether the squares remain.
5. Exit the game normally. A mission and screenshot are not required for this
   descriptor discriminator.

## Decision rule

- Stable `t9`/`t10` mappings to the expected SRVs exclude wrong descriptor
  selection/type/shape for these two inputs and select produced-value capture
  or a subtler shader-translation/driver check next.
- A wrong type, wrong resource shape/cookie, out-of-range table, or unstable
  mapping selects the relevant descriptor write/copy history for narrower
  source instrumentation.
- Missing sidecar data invalidates the descriptor conclusion; it does not prove
  an application or VKD3D defect.
- Visual persistence is expected because D16 changes no rendering command.

Nothing from D16 is a game override or proposed upstream fix. The diagnostic
commit and compatibility tool were kept local and are not distributed in this
archive.
