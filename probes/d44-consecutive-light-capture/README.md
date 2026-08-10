# D44 consecutive tiled-light value capture

D44 keeps D42 rendering unchanged and enables VKD3D-Proton's existing
RenderDoc integration. It adds no shader replacement and no new rendering
behavior. The purpose is to capture three consecutive full frames while the
large blocks and broad light flicker are visible.

The local trigger library waits for a sentinel file, then asks RenderDoc for
three consecutive frame captures in one operation. This avoids timing three
manual hotkeys and lets CPU-only extraction compare consecutive prior-frame
contents for:

- `m_rtDepthRange26`, the `80x34 R32G32_UINT` packed depth range and mask;
- `rtLightRefs25`, the `80x34x2 R32_UINT` light-list grid;
- the fixed `R16_UINT` light-index buffer; and
- the finite `R32G32B32A32_FLOAT` light-record buffer.

The captures may contain game resources. They remain ignored and local-only.
No replay on the GPU is required, and nothing from D44 is an upload or fix
candidate.
