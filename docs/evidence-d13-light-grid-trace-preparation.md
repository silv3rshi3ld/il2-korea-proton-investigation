# D13 tiled-light usage trace: preparation record

## Purpose

D12 shows a runtime `rtLightRefs` resource whose `80 x 34` footprint maps a
2560x1080 frame into approximately 32x32-pixel tiles. The same square artifact
is visible in the menu, cockpit, and around a burning aircraft. D13 tests
whether that tiled light-reference path has a missing clear, transition,
synchronization operation, or stable suspect producer shader.

D13 is telemetry only. It does not alter a resource state, insert a barrier,
replace a shader, clear a target, change a descriptor, or apply an application
override.

## Build identity

- VKD3D-Proton terrain base: `cf11ba76a1cdbee`
- D11 resource-name base: `d3cba21d9c88749273003e03ea38a1e7df4238c7`
- D12 reflection-usage base: `e7f6e30381aece36ad2d8ed86b18b011af191a17`
- D13 light-usage commit: `395d9747`
- Build directory: `build/vkd3d-proton-il2-light-usage-395d9747`
- x64 `d3d12.dll`: `1642a053e5d9dbbf17adef2c383303a11062479f65e467bfff75ff1916adf369`
- x64 `d3d12core.dll`: `8b6720ce34068690fe018e844e833f672235dbbc3566ca7ec8fc0d8cb5d6d48d`
- x86 `d3d12.dll`: `471d9050a29f66154ffdf75f1ec2d07c9f03b0307da08216d17c67091ed8e4fc`
- x86 `d3d12core.dll`: `48794b8c69971f557a005f76170ec55ce58342fa781e014fbc0cd313c9129c10`

The official `package-release.sh --dev-build` flow completed for PE32+ x86-64
and PE32 i386. Both architectures compile cleanly. The packaged x64 core
contains all 19 expected `IL2LIGHT` telemetry format markers.

## Scope

The independent gate is:

```text
VKD3D_IL2_LIGHT_TRACE=1
```

It does not enable D11/D12's broad `IL2MENU` name stream. Only application
resource names beginning with `rtLightRefs`, `rtSelfLight`, or `m_rtLightVol`
are selected. The bounded trace records:

- matching resource dimensions, format, flags, and stable cookie;
- legacy and enhanced texture barriers, including split flags/access/layouts;
- UAV and alias barriers;
- UAV and render-target clears;
- render-target binds and exact writer draws with vertex/pixel shader hashes;
- a bounded set of compute dispatches after a tracked UAV transition, with
  compute shader hash and group dimensions;
- the first two draw/dispatch candidates after a tracked shader-read
  transition; and
- copies and command-list submission.

The write/read association is deliberately described as a candidate unless an
exact RTV/UAV operation identifies the resource. A nearby operation after a
state transition is useful for narrowing shaders but does not by itself prove
descriptor binding.

## Intended compatibility tool and run

The proposed tool name is:

```text
IL2-Korea-D13-LightUsage-395d9747
```

It has not yet been installed. Installation should clone D10, retaining the
validated Wine NUMA implementation and terrain fix, then replace only the four
packaged D3D12/D3D12Core DLLs.

After installation, prepare the run with:

```bash
./scripts/collect-proton-log.sh prepare D13-light-usage-r1 light-trace --no-openmp-override
```

The desired visual protocol is short so the bounded trace covers every scene:

1. Leave the affected main-menu aircraft visible for about 10 seconds.
2. Enter the cockpit and hold the same view for about 10 seconds.
3. If a reproducible burning-aircraft scene is immediately available, show it
   for about 10 seconds; otherwise stop after the cockpit rather than idling.
4. Exit the game, then collect the log.

## Decision rule

- A clear/write/read cycle with stable shaders identifies the exact producer
  and consumer family for a narrower synchronization or shader test.
- A transition to UAV followed by dispatch without an adequate UAV/global
  dependency before read supports a specific barrier experiment.
- Correct, stable cycles with no symptom-correlated difference weaken the
  light-grid hypothesis and move the next trace to final lighting composition.
- Visual resemblance alone does not justify a Proton game quirk.
