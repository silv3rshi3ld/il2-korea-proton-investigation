# D44 consecutive tiled-light capture: preparation

## Purpose

D42 removes the large spatial blocks but leaves broad aircraft-light flicker.
D43 restores both the blocks and flicker, and rejects blind one-step widening
of the packed depth range. D44 therefore changes no lighting decision. It
captures three consecutive D42 frames so the first changing value in the
remaining membership chain can be located before another candidate is made.

CPU-only extraction of the retained D20 frames resolved the exact producer
resource:

- active resource: `m_rtDepthRange26`;
- Vulkan format and extent: `80x34 R32G32_UINT`;
- layout: 2,720 `(packed near/far, logarithmic occupancy mask)` texels;
- both retained frames contain complete, ordered near/far intervals and
  nonzero masks;
- the companion `m_rtDepthRange21` is entirely zero in both frames.

The retained frames are hundreds of frames apart. They prove that the active
resource is coherent and identify its exact representation, but cannot show
the temporal transition responsible for flicker.

## Passive capture build

D44 starts from the exact D42 VKD3D-Proton commit and changes only the Meson
default for existing RenderDoc support:

- source branch: `il2-d44-consecutive-light-capture`;
- source commit: `74fc6aa2`;
- D42 parent: `2d9a7467`;
- package output:
  `build/vkd3d-proton-il2-d44-consecutive-capture-74fc6aa2`;
- `enable_renderdoc=true` in both 64-bit and 32-bit Meson builds.

The packaged DLL hashes are:

| Architecture | File | SHA-256 |
| --- | --- | --- |
| x86-64 | `d3d12.dll` | `88b7fa1423652553e3efb9f950ceb52e06cab55b81c8459dbd09ba6210cea4b5` |
| x86-64 | `d3d12core.dll` | `7ab53a66ac69b6e061aa2b8b09e1362490d8dd5fbcb211afbd7c4f158e51c98e` |
| x86 | `d3d12.dll` | `715281090c944416a9099677c7827513e1815f1720b4c3697d67067080d9a8aa` |
| x86 | `d3d12core.dll` | `229dff52c2fbe192ed28e6ea1d23ef04086dfd89545333cc17e29a1e1bddb39a` |

The installed local tool is
`IL2-Korea-D44-ConsecutiveCapture`. It was cloned from
`IL2-Korea-D42-Complete-2d9a7467`, so its Wine/Proton base and both accepted
compatibility behaviors are unchanged. All four installed DLLs match the
package output.

## Consecutive trigger

`probes/d44-consecutive-light-capture/trigger_capture.c` waits until it is in
the `IL2Series.exe` process and until a local sentinel appears. It then calls
RenderDoc's `TriggerMultiFrameCapture(3)`. A single trigger therefore captures
three adjacent frames without manual hotkey timing.

Run only while Steam is fully stopped:

```text
./scripts/launch-d44-consecutive-renderdoc.sh
```

Select `IL2-Korea-D44-ConsecutiveCapture` for AppID 247970 and start the game.
When the affected menu view is visible and the broad flicker can be judged,
create the sentinel printed by the launcher. On this host it is:

```text
touch /tmp/il2-d44-trigger-$USER
```

After all three captures finish, close the game and Steam normally. Analysis
will remain CPU-only. The captures are expected to expose consecutive initial
contents for `m_rtDepthRange26`, the now-fixed `rtLightRefs25` grid/index
pair, and the finite light-record buffer.

The `.rdc` files may contain game resources. They remain ignored and local.
Nothing in D44 is a compatibility fix or upload candidate. The completed
analysis is recorded
in [`evidence-d44-consecutive-capture-result.md`](evidence-d44-consecutive-capture-result.md).
