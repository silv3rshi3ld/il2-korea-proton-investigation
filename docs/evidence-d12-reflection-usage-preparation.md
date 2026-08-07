# D12 named reflection-resource usage trace: preparation record

## Purpose

D11 confirms that IL-2 allocates native-resolution current and previous SSR
targets during the affected main-menu scene, but it does not prove that those
targets are used. D12 follows only the curated reflection/SSR resources after
the game names them.

D12 remains trace-only. It does not insert a barrier, clear or copy a resource,
change a state, replace a shader, disable reflections, or alter any draw or
submission. It logs the D3D12 operations the application already records.

## Build identity

- VKD3D-Proton base: `cf11ba76a1cdbee`
- D11 resource-name commit: `d3cba21d9c88749273003e03ea38a1e7df4238c7`
- D12 usage-trace commit: `e7f6e303`
- Build directory:
  `build/vkd3d-proton-il2-menu-reflection-usage-e7f6e303`
- x64 `d3d12.dll`: `b696ac14b756539c55e796929ee64bcfb41c148d9d6e3be6d1887ce4a487f115`
- x64 `d3d12core.dll`: `7068a19ff1540712fa278c10754c5636751a3fe1d8be78334df8f0bc13b99f70`
- x86 `d3d12.dll`: `87957feff65bcaa543dd64b123e1f90ad62a02b059a135c590bf77089875bdd7`
- x86 `d3d12core.dll`: `ab9ae15e30fd7e8f3434f2b4f585d107d1f4b024979689ed92f23f45d067c691`

Both architectures completed the official `package-release.sh --dev-build`
flow. File inspection confirms the expected PE32+ x86-64 and PE32 i386 DLLs.
The x64 core contains every expected `IL2MENU usage` format marker.

## Scope

After the application assigns a matching debug name, the resource is tracked
when its name belongs to the D11 reflection family: the general
`reflection_*`/`m_rtRef*` targets, `m_prtTargetReflections`, `rtSSR*`, SSR
weight targets, `rtTempSSR`, the downsampled cube-reflection target, or
reflection-shadow targets. The matcher deliberately does not treat `USSR` in
ordinary texture filenames as SSR.

The 100,000-event bounded stream records:

- legacy transition, UAV, and alias barriers, including split-barrier flags;
- render-target binds through both `OMSetRenderTargets` and `BeginRenderPass`;
- render-target clears;
- texture/resource copies where either side is tracked;
- draws to a tracked target with stable vertex/pixel shader hashes; and
- command-list execution order on the queue.

The gate remains `VKD3D_IL2_MENU_TRACE=1`. With the variable absent or `0`,
the diagnostic paths do not emit telemetry. The added resource fields and
checks do not alter any D3D12 or Vulkan command.

## Intended compatibility tool and run

The intended tool is:

```text
IL2-Korea-D12-ReflectionUsage-e7f6e303
```

It will be copied from D10, retaining the validated Wine NUMA implementation
and terrain fix, then receive only D12's four packaged D3D12 DLLs. D10 and D11
remain untouched.

After installation, prepare the menu-only run as:

```bash
./scripts/collect-proton-log.sh prepare D12-reflection-usage-r1 menu-pass-trace --no-openmp-override
```

Keep the same affected menu aircraft visible for at least 30 seconds, report
whether the squares remain, and exit without loading a mission.

## Decision rule

- Repeated render-target writes and render-to-sample transitions identify an
  active reflection resource and stable shaders for the next discriminator.
- A missing transition or unmatched split dependency is only a candidate until
  command-list submission ordering proves that no equivalent dependency exists.
- If the named targets are never bound, copied, or transitioned, G16 is
  weakened and the next trace must follow the final aircraft material/shadow
  targets instead.
- No barrier workaround or shader quirk is allowed from visual resemblance or
  allocation evidence alone.
