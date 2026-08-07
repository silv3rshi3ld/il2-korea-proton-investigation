# D11 main-menu resource and pass trace result

## Result

The main-menu shimmering squares are **unchanged** on D11. The user visually
confirmed that the same artifact remains after the prepared menu observation.
The trace-only changes therefore did not mask, repair, or materially alter the
symptom.

D11 produced useful runtime evidence: the game allocates its named screen-space
reflection and reflection-history targets while constructing the affected menu
scene. This advances the reflection lead beyond static binary strings, but
allocation alone does not prove that a resource is rendered, sampled, or
responsible for the visible squares.

## Validity

- Game build: `24615759`
- Compatibility tool: `IL2-Korea-D11-MenuPassTrace-d3cba21d`
- Proton version:
  `1786112352 experimental-11.0-20260724c-wine-mr11604-d10`
- VKD3D-Proton base: `cf11ba76a1cdbee`
- Diagnostic commit: `d3cba21d9c88749273003e03ea38a1e7df4238c7`
- Launch options contained no OpenMP or topology override
- Prefix x64 `d3d12.dll`:
  `b114f84c1fbd841ad802527a7aba4342e084e54e4d736b1beaa7394402c4a702`
- Prefix x64 `d3d12core.dll`:
  `f7f35208e2822122097859b95244bc85010d7a99a680e9024b27a2f1b004927a`
- D3D12 module lines: 2; DXGI module lines: 2; D3D11 module lines: 0
- Device-loss, OOM, or GPU-reset signature: none

The Proton log names the D11 custom-tool path, both trace DLL hashes match the
validated build, and the `IL2MENU resource-name trace enabled` marker occurs
once. The uncompressed log is 11,765,013 bytes. Its compressed SHA-256 is
`0f92305252d975a28c6269c1afe1734273efb4cc4270bdb11dbcf2b2f7126358`.

## Telemetry result

- Resource-name events: 3,145
- Resource-name suppression: none
- PIX marker/begin events: zero
- PIX trace enable marker: zero, because the lazy trace initializer is reached
  only when the application calls a marker/event API

The collector's broad reflection count is 56, but that regex also matches
ordinary filenames containing `USSR`. It must not be treated as 56 reflection
render targets. The curated runtime targets are:

| Cookie | Name | Size | DXGI format value | Flags |
|---:|---|---|---:|---:|
| 166 | `reflection_color` | 1024x512 | 11 | `0x1` |
| 169 | `reflection_depth` | 1024x512 | 45 | `0x2` |
| 178 | `m_rtRefEdges` | 1024x512 | 36 | `0x1` |
| 181 | `m_rtRefTemp` | 1024x512 | 11 | `0x5` |
| 3269 | `m_prtTargetReflections` | 2560x1080 | 29 | `0x1` |
| 3273 | `rtSSR22` | 2560x1080 | 29 | `0x1` |
| 3276 | `m_rtSSRW` | 2560x1080 | 65 | `0x1` |
| 3279 | `rtSSRPrev[0]23` | 2560x1080 | 29 | `0x1` |
| 3282 | `rtSSRWPrev[0]24` | 2560x1080 | 65 | `0x1` |
| 4105 | `rtCubeRefDownsampledBlur` | 1280x540 | 28 | `0x1` |
| 4113 | `rtTempSSR` | 2560x1080 | 10 | `0x1` |

Four separately allocated 1024x1024 depth resources are also named
`m_pctReflectionShadows`. They appear during repeated menu-scene asset setup,
not as the persistent full-resolution SSR/history group.

The current and previous SSR color and weight targets are allocated together
at the native render resolution. `rtTempSSR` is allocated a few seconds later
while the menu aircraft assets are being created. This is consistent with a
temporal reflection pipeline, but D11 cannot distinguish active use from eager
allocation.

## Next discriminator

The game supplied no PIX pass labels, so the next trace follows only the
curated named resources through D3D12 transitions, render-target binds, clears,
copies, draws, shader hashes, and command-list submission. Local diagnostic
commit `e7f6e303` adds that bounded telemetry without changing any rendering
operation and compiles cleanly for x64 and x86.

No reflection disable, game modification, application override, or upstream
fix is justified by D11 alone.
