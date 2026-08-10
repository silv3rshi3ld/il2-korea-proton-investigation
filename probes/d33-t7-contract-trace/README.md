# D33 `t7`/`t8` contract trace and capture analysis

D32 proves that genuine light ID 2 is sufficient for the square artifact.
D33 resolves the live contracts of the adjacent `t7` and `t8` SRVs before any
record math is changed.

The offline extractor searches a RenderDoc capture for the observed
240-byte `VK_FORMAT_R32G32B32A32_UINT` uniform-texel-buffer descriptor. It
maps the descriptor address back into the captured Vulkan allocation and
decodes 1,024 bytes beginning at the view base as `uint4` texels and their
bitwise float reinterpretations. The extra bytes are intentionally read from
the captured backing allocation, not through the declared view.

Descriptor-heap correlation in both D20 frames places this descriptor at heap
slot 13,796. The D16 table base and confirmed `t9`/`t10` locations make that
slot `t8`, not `t7`. The original SPIR-V expects the per-light float buffer at
register `t7` and a 2D float image at `t8`; capture bytes make those adjacent
slots look reversed. This is evidence for D33 to resolve, not yet proof of a
game or VKD3D defect: the root signature can contain nontrivial range offsets,
and raw descriptor-byte matching does not replace the live D3D sidecar.

Run the extractor with RenderDoc 1.45's Python host:

```text
IL2_RDC_PATH=/path/to/frame.rdc \
IL2_RD_OUTPUT=/tmp/d33-frame.txt \
QT_QPA_PLATFORM=offscreen \
build/renderdoc-1.45-local/package/usr/bin/qrenderdoc \
  --python probes/d33-t7-contract-trace/extract_t8_candidate.py
```

The extractor defaults to the original 240-byte `UINT` candidate. After D33
has established the real `t7` contract, the same CPU-only extractor can select
the 128-element `R32G32B32A32_SFLOAT` view and decode all 16 eight-texel light
records with:

```text
IL2_RD_RANGE=2048 IL2_RD_FORMAT=109 IL2_RD_DECODE_BYTES=2048
```

The runtime component is render-passive. It extends the existing D16 trace to
resolve `t7`, `t8`, `t9`, and `t10` for only pixel shader hashes
`df0bd777fd1bb89d` and `a2d104d5c813322e`. It must not use shader overrides or
change the game configuration. It also logs the original root-signature
version and every effective descriptor-range type, register span, table
offset, and D3D12 range flag so the exact mapping can be reconstructed.

Nothing in D33 is an upload candidate or final compatibility fix.
