# U00 updated-game baseline

- Run: `U00-game-24596901-baseline-r1`
- Previous game build: `24577563`
- Tested game build: `24596901`
- Proton: `experimental-11.0-20260724c`
- VKD3D-Proton: `3dfc6f07d0953b1e8b41705275c2c59cc7374fc5`
- D3D12 DLL state: Proton-supplied hashes
- Visual result: unchanged; the user confirms the same menu and terrain
  corruption

Steam downloaded 886,044,096 bytes and staged 5,418,273,475 bytes for the
update. Proton Experimental did not update. The game's install step restored
the prefix's four D3D12 DLLs to hashes matching Proton Experimental, so U00 was
a clean Proton-supplied control rather than an instrumented run.

## Runtime evidence

| Signal | U00 |
|---|---:|
| Raw log bytes | 7,852,737 |
| Compressed-log SHA-256 | `8ce6acba9bd08352dd1f22f2b32550093f873b6d29aeeb23dfae4644c2955638` |
| Split `END_ONLY` warnings | 11,882 |
| VKD3D warning matches | 13,773 |
| Wine error matches | 269 |
| D3D11 module lines | 0 |
| Device-loss/hang matches | 0 |
| Out-of-memory matches | 0 |
| `IL2TRACE` enabled markers | 0 |

The warning counts differ from D00 with run duration, but the frequent
signatures and active D3D12 paths are the same. No update-specific failure
class appeared.

## Conclusion

Game build `24596901` neither fixes nor materially changes the investigated
rendering corruption. Source-level D01 telemetry can resume without repeating
the earlier launch-option matrix.
