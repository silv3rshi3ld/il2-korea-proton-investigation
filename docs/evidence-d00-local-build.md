# D00 unmodified local-build parity

- Run: `D00-local-vkd3d-unmodified-r1`
- Source commit: `3dfc6f07d0953b1e8b41705275c2c59cc7374fc5`
- Build ID observed in the runtime log: `0x3dfc6f07d0953b1`
- Changed variable: Proton-supplied D3D12 DLLs replaced by an otherwise
  unmodified local build of the exact same VKD3D-Proton source commit
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

## Conclusion

The locally compiled, officially packaged DLLs reproduce the defect. This
rules out a mismatch introduced by the development build/install procedure and
allows source instrumentation to proceed from a validated control. It does not
attribute the underlying defect to VKD3D-Proton.
