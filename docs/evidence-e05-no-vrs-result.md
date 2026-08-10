# E05 no-VRS result

## Result

The main-menu shimmering squares are **unchanged** with
`VK_KHR_fragment_shading_rate` disabled. The user visually confirmed that the
same moving square artifact remains on the menu aircraft.

This is a valid negative discriminator: the artifact does not require the
D3D12 variable-rate-shading path advertised by VKD3D-Proton. VRS remains an
engine capability, but it is no longer the leading explanation for this
symptom.

## Validity

- Game build: `24615759`
- Compatibility tool:
  `IL2-Korea-D10-WineMR11604-Proton11`
- Proton version: `experimental-11.0-20260724c-wine-mr11604-d10`
- Runtime VKD3D-Proton build: `cf11ba76a1cdbee`
- Exact changed variable:
  `VKD3D_DISABLE_EXTENSIONS=VK_KHR_fragment_shading_rate`
- OpenMP/topology overrides: none found
- Fragment-shading-rate disable warnings: 228
- D3D12 module lines: 2; DXGI module lines: 2; D3D11 module lines: 0
- Device-loss, OOM, GPU-reset signature: none found

The repeated disable warnings come from VKD3D checking the same optional
extension for multiple feature/capability paths. They prove that the requested
extension removal was active; they are not 228 VRS API calls.

## Game-update boundary

Steam updated the game from build `24596901` to `24615759` before E05 ran.
Because the artifact is still present in the no-VRS run, that update does not
invalidate the negative result: removing VRS failed to remove the symptom. A
new-build VRS-enabled baseline would only be required if the no-VRS run had
appeared fixed or materially improved.

## Interpretation

E05 weakens both a game-side VRS-rate-image defect and VKD3D/RADV translation
of that VRS path as the primary cause of the menu squares. It does not identify
the responsible pass.

The next lead is the temporal/reflection path. Static game binaries expose
current and previous SSR resources plus screen, overlay, cockpit, and
accumulated-reflection passes. The visible aircraft texture remains present
beneath moving bright/dark blocks, which is more consistent with a defective
effect or history resource than with a missing base texture.

The next diagnostic must correlate the menu artifact with those resources or
passes. No reflection-disabling workaround or application override is
justified from static names alone.
