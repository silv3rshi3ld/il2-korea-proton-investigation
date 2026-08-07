# D11 main-menu resource and pass trace: preparation record

## Purpose

D11 is a read-only telemetry build for the remaining main-menu shimmering
squares. It does not disable reflections, alter synchronization, replace a
shader, or change any rendering command. Its only purpose is to ask whether
the game supplies useful D3D12 debug names or PIX labels for the affected menu
resources and passes.

This is the next discriminator after E05 showed that the artifact remains with
fragment shading rate unavailable. Static game strings name `rtSSR`,
`rtSSRPrev`, current/previous reflections, and several aircraft/cockpit
reflection passes, but static strings do not prove that those paths run in the
menu.

## Build identity

- VKD3D-Proton base: `cf11ba76a1cdbee`
- Diagnostic commit: `d3cba21d9c88749273003e03ea38a1e7df4238c7`
- Source branch: `il2-menu-pass-trace`
- Patch artifact:
  [`0010-vkd3d-Add-focused-menu-resource-and-pass-telemetry.patch`](../patches/0010-vkd3d-Add-focused-menu-resource-and-pass-telemetry.patch)
- Build directory: `build/vkd3d-proton-il2-menu-pass-trace-d3cba21d-r2`
- x64 `d3d12.dll`: `b114f84c1fbd841ad802527a7aba4342e084e54e4d736b1beaa7394402c4a702`
- x64 `d3d12core.dll`: `f7f35208e2822122097859b95244bc85010d7a99a680e9024b27a2f1b004927a`
- x86 `d3d12.dll`: `04baf0efbfd13033506a570a73d6f207ba03564fa6573f119ee131bac3702ef8`
- x86 `d3d12core.dll`: `10ca6f22285506e9dd204251533fee8b6a991158d1a7f98c24bf382c9f5de09c`

Both architectures completed the official `package-release.sh --dev-build`
flow. File inspection confirms PE32+ x86-64 and PE32 i386 DLLs, and the x64
core contains the `VKD3D_IL2_MENU_TRACE` and `IL2MENU` markers.

## Telemetry

`VKD3D_IL2_MENU_TRACE=1` enables two bounded streams:

1. D3D12 resource names supplied through the ANSI or wide debug-object-name
   GUID, recorded with the VKD3D resource cookie and immutable resource
   description.
2. Decodable PIX `SetMarker` and `BeginEvent` labels on command lists and
   queues.

Resource-name output stops after 20,000 events and PIX output after 50,000.
The gate defaults off. The added calls only decode and log application-supplied
metadata; all original D3D12/Vulkan behavior continues unchanged.

## Compatibility tool

The intended tool is:

```text
IL2-Korea-D11-MenuPassTrace-d3cba21d
```

It is copied from `IL2-Korea-D10-WineMR11604-Proton11`, so it retains the
validated Wine NUMA implementation and D08 terrain conversion. Only the four
packaged x64/x86 VKD3D-Proton D3D12 DLLs are replaced. D10 and the game prefix
remain untouched.

Steam must be fully stopped before the helper creates the tool. The first
creation attempt correctly refused to proceed because Steam was active; no
partial D11 directory was installed. After Steam exited, the tool was created
at `2026-08-07T18:53:58Z`. Its recorded source version is
`experimental-11.0-20260724c-wine-mr11604-d10`, and all four installed DLL
hashes exactly match the build identities above.

## Runtime protocol

Restart Steam and select
`IL2-Korea-D11-MenuPassTrace-d3cba21d` for AppID 247970. Prepare the run with:

```bash
./scripts/collect-proton-log.sh prepare D11-menu-pass-trace-r1 menu-pass-trace --no-openmp-override
```

The prepared run produced these exact launch options:

```text
PROTON_LOG=1 PROTON_LOG_DIR=/tmp/il2-D11-menu-pass-trace-r1 VKD3D_IL2_MENU_TRACE=1 %command%
```

Use exactly those Steam launch options. Reach the same main-menu
aircraft, leave the view visible for at least 30 seconds, confirm whether the
moving squares remain, and exit without loading a mission. Then collect with:

```bash
./scripts/collect-proton-log.sh collect D11-menu-pass-trace-r1
```

## Decision rule

- If reflection/SSR resource names or pass labels appear, use their cookies
  and ordering to design a narrowly scoped binding, barrier, and draw/dispatch
  trace. A name is correlation evidence, not yet proof of a synchronization
  defect.
- If names exist but none identify the affected path, correlate dimensions,
  format, lifetime, and descriptor bindings before changing rendering.
- If the game supplies no useful names or PIX labels, proceed to bounded
  resource-creation and descriptor/render-target binding telemetry. Do not
  infer that reflections are absent merely because release code omitted debug
  labels.

No application override or upstream fix is proposed at this stage.
