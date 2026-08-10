# E05 menu shimmer: fragment-shading-rate control

## Question

Does the moving square pattern on the menu aircraft depend on the D3D12
variable-rate-shading (VRS) path exposed through
`VK_KHR_fragment_shading_rate`?

E05 is a one-variable capability test. It is not a proposed launch workaround,
application override, or fix.

## Why this is the first discriminator

The retained menu captures show the base aircraft texture beneath moving,
rectangular light/dark regions, especially on reflective metal. Read-only
inspection of the installed game finds these exports in `dxBackend12.dll`:

- `DXRenderer12::enableVRS`
- `DXRenderer12::setVRSRate`
- `DXRenderer12::setVRSCombiners`
- `DXRenderer12::getVRSFeatures`
- `SCLContext::GetVRSHash` and `SetVRSHash`

VKD3D-Proton implements D3D12 VRS with `VK_KHR_fragment_shading_rate`.
Its generic `VKD3D_DISABLE_EXTENSIONS` parser can remove that extension before
device capability initialization. The resulting D3D12 options report VRS as
unsupported, so a correctly written game selects its non-VRS path.

This establishes a useful A/B control. It does not establish that the game
actually enables VRS in the menu or that VKD3D/RADV handles it incorrectly.

The leading alternative is a screen-space or temporal reflection pass. The
game binaries name `rtSSR`, `rtSSRPrev[0/1]`, `g_tPrevReflections`, and separate
screen, overlay, cockpit, and accumulated-reflection draws. That path should be
tested or traced only if E05 is unchanged.

## Fixed environment

- Compatibility tool: `IL2-Korea-D10-WineMR11604-Proton11`
- Game build: `24596901`
- Steam launch workarounds: no `OMP_NUM_THREADS`, `KMP_AFFINITY`, or
  `WINE_CPU_TOPOLOGY`
- Graphics settings: leave the existing `startup.cfg` unchanged
- Scene: same main-menu aircraft and camera used for the retained captures
- Observation: at least 30 seconds; video preferred because the symptom moves

D10 is required because it retains the D08 terrain fix and the exact Wine MR
!11604 startup validation. E05 changes only the advertised Vulkan fragment
shading-rate capability relative to a normal D10 launch.

## Prepare run 1

With D10 selected for AppID 247970:

```bash
./scripts/collect-proton-log.sh prepare E05-no-vrs-r1 no-fragment-shading-rate --no-openmp-override
```

Paste the printed line into Steam Launch Options, start the game, wait 30
seconds at the unchanged menu view, record the visual result, then exit. Do not
load a mission for the first menu-only discriminator unless the menu result is
already recorded.

Collect after every IL-2/Proton process exits:

```bash
./scripts/collect-proton-log.sh collect E05-no-vrs-r1
```

## Validity gates

Run 1 is valid only if all of these hold:

1. The game reaches the same interactive menu.
2. The Proton log contains an extension-disable message naming
   `VK_KHR_fragment_shading_rate`.
3. The launch-options record contains no OpenMP or topology override.
4. D10 remains the selected compatibility tool and its patched Wine modules
   are mapped.
5. No graphics setting, resolution, aircraft, or menu camera changes.

If the game refuses to start or the menu takes a materially different path,
classify the run as inconclusive rather than calling the shimmer fixed.

## Decision rule

- **Unchanged:** repeat once as `E05-no-vrs-r2`. If both runs are unchanged,
  deprioritize VRS and instrument the current/previous reflection resources and
  their synchronization.
- **Fixed or improved:** repeat once before drawing any conclusion. If the
  result repeats, add bounded tracing for `RSSetShadingRate`,
  `RSSetShadingRateImage`, the rate-image resource cookie, and affected draws.
  Do not ship the extension disable as the remedy.
- **Regressed or failed startup:** preserve the log and classify E05 as
  inconclusive; determine whether the game lacks a valid no-VRS fallback.

Only a traced API/resource defect or a minimal reproducer can justify an
upstream VKD3D-Proton or Mesa change.
