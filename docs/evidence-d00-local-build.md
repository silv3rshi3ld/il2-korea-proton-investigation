# D00 invalidated local-build parity attempt

- Run: `D00-local-vkd3d-unmodified-r1`
- Source commit: `3dfc6f07d0953b1e8b41705275c2c59cc7374fc5`
- Build ID observed in the runtime log: `0x3dfc6f07d0953b1`
- Intended changed variable: Proton-supplied D3D12 DLLs replaced in the prefix
  by an otherwise unmodified local build of the same source commit
- Visual result: unchanged; user confirms the same aircraft block artifacts
  and terrain loss

## Runtime evidence

The collected raw log was 8,891,375 bytes and is retained compressed under the
ignored run directory. Its compressed SHA-256 is:

```text
56dca5317c69c412cee680ee4385652feeaaa114fc476182255a3a2db9369321
```

| Signal | D00 | E00-r2 comparison |
|---|---:|---:|
| Split `END_ONLY` warnings | 18,814 | 18,562 |
| VKD3D warning matches | 20,702 | duration-dependent, similar class |
| Wine error matches | 269 | duration-dependent, similar class |
| D3D11 module lines | 0 | 0 |

No device loss, GPU hang/reset, or out-of-memory signature was found. The
small warning-count difference is consistent with slightly different runtime
duration and does not identify a changed error fingerprint.

## Invalidation

The local and Proton-packaged DLLs share the same VKD3D source/build identifier,
so the D00 log cannot distinguish them. During D01a, which added an unambiguous
`IL2TRACE` marker, the marker was absent and all four post-run prefix hashes
exactly matched Proton Experimental instead of the installed diagnostic DLLs.
Inspection of Proton's launcher confirms that it copies its packaged D3D12 and
D3D12Core DLLs into the prefix during launch.

The same launch behavior necessarily applies to D00. Its visual result remains
a valid ordinary Proton reproduction, but it does not prove that the local
unmodified build ran. D00 is therefore **inconclusive**, and local-build parity
must be repeated through a dedicated custom Proton tool.
