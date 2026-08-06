# D01a invalidated prefix-install trace

- Run: `D01-game-24596901-sparse-trace-r1`
- Game build: `24596901`
- Intended diagnostic commit: `d0b4421f129b72e6127e6b9abd4028e8df946ea7`
- Intended gate: `VKD3D_IL2_RESOURCE_TRACE=1`
- Visual result: unchanged; severe isolated terrain pages and magenta edges at
  6,306 m
- Instrumentation validity: **invalid**

## Decisive evidence

The launch environment included the trace gate, but the complete compressed log
contains zero `IL2TRACE` markers. After the run, all four prefix DLL hashes
exactly matched Proton Experimental:

| Prefix file | Post-run SHA-256 | Diagnostic SHA-256 | Result |
|---|---|---|---|
| x64 `d3d12.dll` | `c383f4c513aa12f9d93177c798e67f70a73758ab1d4537ecd3c47119b6e51cd2` | `37a302c0768f5755f47dca7c26724cdfc1ccd291825b3b397ccd64f5260d8942` | packaged Proton |
| x64 `d3d12core.dll` | `a9fe0f0c9741c1fd3c152883c66f2aa941eacf456f7ebfd3f6e7a61d22e7b661` | `e8b4de0df971fbf6e4e4b267210815b8bad1f38479c00d6bc8c7e8ebd484ac19` | packaged Proton |
| x86 `d3d12.dll` | `26d1495ef14eabc8c6582df27d2ab7b9e30df1fdf84e7803f2026ef2e576c3e9` | `7b867e13c54908dac7adf044c01a8a9985c59d538af4377871c52c6091962807` | packaged Proton |
| x86 `d3d12core.dll` | `04606c392425d6c6681054f8c525d8f284b41537975cf902aa7c74957cbb12a2` | `3260a153211725afa7e84364d3bc931be7e81b4df841e78a8acd4610d35c5d04` | packaged Proton |

This matches Proton's launcher implementation: it copies the packaged
VKD3D-Proton DLLs into `system32` and `syswow64` and sets native overrides on
each launch. Installing different DLLs directly into the prefix is therefore
not durable under stock Proton.

## Retained evidence

- Compressed log SHA-256:
  `428d4d061d17b5d9cf7773a7192556f067e526327a2ac7b392e5658e5520c7c6`
- Screenshot:
  `D01-r1-cockpit-right-high-altitude-isolated-terrain-pages-magenta-edges-6306m.png`
- Screenshot SHA-256:
  `40b61ad3ca23ed748d0aefb86333d013e7deffa8a2cb50081a159be5f6a2a80a`

The screenshot and ordinary Proton reproduction remain valid. The zero API
counts cannot be used to reject the sparse/tiled-resource hypothesis because
the instrumented code never loaded.

## Corrected method

Use `scripts/create-custom-proton.sh` to make a copy-on-write clone of Proton
Experimental and replace the clone's packaged VKD3D DLLs. Selecting that tool
in Steam makes Proton itself copy the diagnostic files into the prefix.
