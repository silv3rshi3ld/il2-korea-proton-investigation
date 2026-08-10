# D42-D43 integrated tiled-light results

## Results

D42 and D43 are both incomplete. Later D44 value capture additionally proves
that D42's allocator remains malformed; the initially clean-looking D42 view
was a transient presentation, not a complete block fix.

- D42 integrates the D38 geometric-membership predicate and the incomplete
  D25 typed-UAV compatibility change. The large square blocks disappeared in
  the first observed view, but broad lighting on the aircraft still flickered;
  later identical runs restored the blocks.
- D43 restores the original depth rejection and instead expands the packed
  depth interval and occupancy mask by one encoded step. The square blocks
  return and the affected lighting also flickers.

The fine sandy or film-grain-like lighting remains excluded from this result:
it was separately confirmed on native Windows. The broad aircraft-light
flicker reported in D42 and D43 is visually distinct and is not accepted as
native behavior.

## D43 runtime provenance

- Compatibility tool:
  `IL2-Korea-D43-ConservativeDepth-505ebc10`
- VKD3D-Proton source commit: `505ebc10`
- Source branch: `il2-conservative-depth-range`
- Steam launch options: empty
- Wine base: the D10 NUMA-capable Proton tool
- VKD3D-Proton base includes the validated terrain-copy correction

The prefix `config_info` names the D43 tool. Its installed x86-64 DLL hashes
match the tool exactly:

| File | SHA-256 |
| --- | --- |
| `d3d12.dll` | `62e062006955e45d05baa22cd92594c28e7c241b4487703208fc35aa0bd13da6` |
| `d3d12core.dll` | `c4c72f559ac3fc0a3f289dbd4f5df1681ca1f664f8c3801509623da62a9d37c7` |

VKD3D-Proton's shader-interface cache key hashes the compiler revision, the
complete application quirk table, each shader hash, and each quirk mask. Its
pipeline blobs also validate both the VKD3D build and this interface key.
Therefore a D42 shader cannot be silently reused as D43 through the normal
VKD3D pipeline cache.

## Interpretation

D43 rejects the hypothesis that a one-step conservative correction to the
producer's packed quantization is sufficient. It must not be proposed as a
fix or widened repeatedly without value-level evidence.

The spatial blocks and temporal flicker are now likely two presentations of
the same unstable tiled-light membership chain:

- a stable wrong decision over neighbouring pixels appears as a square;
- a decision changing between frames appears as light flicker.

D42 shows that bypassing the two final depth predicates can suppress the most
visible spatial presentation but does not repair the underlying temporal
instability. D44 completes the proposed consecutive-frame capture and finds
stable depth/grid metadata but a still-overlapping light-index allocation.
See `evidence-d44-consecutive-capture-result.md`.
