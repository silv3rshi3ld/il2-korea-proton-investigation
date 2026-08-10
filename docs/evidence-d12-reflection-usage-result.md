# D12 named reflection-resource usage trace: result

## Result

D12 confirms that the named reflection and screen-space-reflection resources
are actively rendered every frame. It does **not** identify a broken transition
on those resources, and it does not prove that reflection is the source of the
visible square-grid artifact.

The user reports the artifact unchanged. Two screenshots from the same game
run additionally show it in the cockpit and outside around a burning aircraft.
This establishes that the symptom is not restricted to the main-menu aircraft.

## Run identity

- Run: `D12-reflection-usage-r1`
- Game build: `24615759`
- Proton: `1786112352 experimental-11.0-20260724c-wine-mr11604-d10`
- D12 VKD3D-Proton commit: `e7f6e303`
- Terrain-fix base: `cf11ba76a1cdbee`
- GPU/driver: Radeon RX 7800 XT, RADV 26.1.6
- Launch options:
  `PROTON_LOG=1 PROTON_LOG_DIR=/tmp/il2-D12-reflection-usage-r1 VKD3D_IL2_MENU_TRACE=1 %command%`
- Full source log: 78,380,806 bytes
- D3D12 and DXGI loaded; no D3D11 module was loaded
- No device-loss, OOM, or GPU-reset signature was observed

The collected source-log hash is recorded in the ignored run directory. The
uncompressed source log is not intended for publication.

## Trace coverage and its limit

The bounded trace recorded exactly 100,000 named-resource usage events and
20,000 general resource-name events:

| Event | Count |
|---|---:|
| legacy transition barriers | 35,888 |
| render-target binds | 28,050 |
| indexed draws | 22,032 |
| render-target clears | 12,025 |
| command-list executions | 2,005 |
| tracked copies, UAV barriers, or alias barriers | 0 |

The usage trace began at Wine monotonic timestamp `44330.864` and reached its
cap at `44443.276`. With the host boot time of `08:52:06`, that corresponds to
approximately 21:10:57 through 21:12:49 local time. The resource-name stream
reached its separate cap around 21:18:12.

The screenshots were written at 21:35:31 and 21:35:54. D12 therefore covers
the initial/menu interval but **not** the later cockpit and fire frames. Those
screenshots are valid visual evidence from the same run, but they cannot be
presented as frames whose D3D12 operations appear in the bounded usage stream.

## Reflection operations observed

The trace contains about 2,003 repeated rendering cycles. Representative
stable shader identities are:

| Target | Vertex shader | Pixel shader(s) |
|---|---|---|
| `m_prtTargetReflections` | `7ab2e6bc4f546a86` | `df0bd777fd1bb89d`, `a2d104d5c813322e` |
| `rtTempSSR` | `2775bc332a8391cd` | `18a65244f839fb57`, `b3adbbc775c18d95`, `5a57a8411ec9e504`, `82b8811d9a9541b1` |
| `rtSSR22`, `m_rtSSRW` | `2775bc332a8391cd` | `109016f7fb78fe37`, `a277c289d70fa0b0` |
| `rtCubeRefDownsampledBlur` | `bf88d6277d5f9331` | `bd054c53239b3b97` |

The named targets repeatedly move through coherent, explicit legacy states:

- reflection and temporary targets alternate between render target (`0x4`)
  and pixel/non-pixel shader resource (`0xc0`);
- current SSR color/weight targets additionally become resolve sources
  (`0x2000`);
- previous SSR color/weight targets become resolve destinations (`0x1000`)
  and then shader resources; and
- the reflection-shadow depth target alternates between depth write (`0x10`)
  and depth read/shader read (`0xa0`).

Every barrier logged for these named resources has flags `0`. The large global
`END_ONLY` warning count therefore comes from other resources; D12 neither
attributes those warnings to the reflection family nor rules out a separate
split-barrier problem elsewhere.

## New light-grid lead

The general resource-name stream contains several generations of:

```text
rtLightRefs20
rtLightRefs25
rtLightRefs27
rtLightRefs29
```

Each is a 3D `80 x 34 x 2`, one-mip, `DXGI_FORMAT_R32_UINT` resource with
render-target and unordered-access flags. At the run's 2560x1080 resolution,
`80 = 2560 / 32` and `34 = ceil(1080 / 32)`. This is the exact geometry of a
screen divided into approximately 32x32-pixel light tiles.

Read-only game-binary strings independently connect this resource to a tiled
lighting path:

- `enviro.dll`: `rtLightRefs`, `computeLightsList`, `rtSelfLight`;
- `renderers.dll`: `g_tLightRefsRW`, `g_tLightsListRW`,
  `TechCollectLightsList`, and `TechDrawLightListVol`; and
- `MissionManager.dll`: `getTiledGroupsInRect` for mission lights.

The cockpit and fire observations make this lead stronger: the artifact is
visible on dark/cockpit surfaces and around a source of many dynamic light
contributions. This is an evidence-backed correlation, not yet proof that
`rtLightRefs` contains the wrong values.

## Decision

G16, a simple missing transition on the named SSR targets, is weakened. D13
must follow `rtLightRefs`, `rtSelfLight`, and the light-volume resources through
their UAV/render-target writes, clears, nearby dispatch/draw shader hashes,
read transitions, and submission. It remains trace-only; no light, reflection,
or barrier behavior should be changed before a specific failing resource and
producer are identified.
