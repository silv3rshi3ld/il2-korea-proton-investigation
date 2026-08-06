# E03 descriptor-buffer disable: run 1

- Run: `E03-r1`
- Game build: `24596901`
- Changed runtime behavior: `VK_EXT_descriptor_buffer` disabled
- Visual result: **unchanged**
- Matrix status: **inconclusive** pending a stock-Proton confirmation

## Visual result

All established symptoms remain:

- the main-menu aircraft shows the rectangular block pattern;
- at 4,858 m, most terrain is absent and only isolated rectangular pages with
  magenta edges are visible; and
- at 1,121 m, more terrain fragments and trees appear, but large continuous
  ground regions remain dark or absent.

This independently reproduces the altitude dependency. Disabling descriptor
buffers does not materially improve the defect in this run.

| Capture | SHA-256 | Observation |
|---|---|---|
| `E03-r1-menu-aircraft-blocks.png` | `61d5d0adb03aad05cfd72e73de79845f0bc3a4ac575a22a81eb1b9b179326754` | Aircraft block pattern remains |
| `E03-r1-terrain-missing-pages-4858m.png` | `2dc6a8bfda2e7121fab402a47a54ee2dfd4d429c990ca0f5e84c174f474179f7` | Severe high-altitude page loss |
| `E03-r1-terrain-missing-pages-1121m.png` | `5514ffe23e035f7eb6a4b3699eff25cab15578789c964a6f5e10062f1f859d40` | More low-altitude fragments, core loss unchanged |

The local images are retained under ignored
`captures/curated/e03-no-descriptor-buffer/`.

## Runtime validity

The log confirms the intended backend change. It reports
`Extension "VK_EXT_descriptor_buffer" is disabled` and then enables the
`VK_EXT_mutable_descriptor_type` fallback. It loads D3D12/D3D12Core and DXVK
DXGI without D3D11. No device loss, GPU reset/hang, out-of-memory, upload-
exhaustion, or invalid-copy signature appears.

The exact compressed log has SHA-256:

```text
c631db4aab98506f234229c703af0286624f8415253e886c49747f98f9c0161a
```

The 228 extension-disabled warnings are repeated capability-query
confirmations at initialization, not 228 runtime failures. Split `END_ONLY`
warnings total 54,074; their higher count than E00-r2 reflects a longer run and
does not indicate increased visual severity.

## Provenance caveat

Steam remained on `IL2-Korea-D03-Alias-Trace-cfca234e` rather than Proton
Experimental. The log and prefix hashes identify diagnostic build `cfca234e`.
Neither `VKD3D_IL2_TEXTURE_TRACE` nor `VKD3D_IL2_ALIAS_TRACE` was enabled, and
no corresponding marker or event occurs. The diagnostic additions were
therefore compiled but inactive, and they have no intended rendering behavior.

This makes E03-r1 useful evidence against descriptor buffers being a complete
fix, but it is not the requested stock-Proton run. A single confirmation with
Proton Experimental and the same extension disable is required before the E03
matrix row can be closed. No application override is justified by this result.
