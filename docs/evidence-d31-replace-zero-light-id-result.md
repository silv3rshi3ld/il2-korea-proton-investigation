# D31 zero/sentinel light-ID replacement: result

## Result

The square artifact remained when only original light ID 0 was replaced with
safe record ID 1. Every original nonzero ID and its selected light record was
preserved, while every loop iteration evaluated a valid nonzero record.

This rejects zero/sentinel entries, their skip branch, and mixed evaluate/skip
control flow as requirements for the artifact. Together with clean D27, the
remaining necessary difference is at least one genuine record above ID 1.

## Run verification

- Run: `D31-replace-zero-light-id-r1`
- Compatibility tool: `IL2-Korea-D25-LightAtomicCompat-84c87c83`
- Log: `/tmp/il2-D31-replace-zero-light-id-r1/steam-247970.log`
- Visual result reported by the user: squares still present
- Both intended shader overrides loaded exactly once
- Vulkan/device failure: none

The Wine critical-section messages occur during shutdown and do not invalidate
the rendered comparison.

## Next split

D32 should preserve genuine ID 2 but map every other list entry to safe ID 1.
This keeps the original loop count, prevents sentinel skipping, and selects
only records 1 and 2. A defective result makes record 2 sufficient; a clean
result narrows the required record set to IDs 3 and 4.

D31 is diagnostic evidence, not a fix or upload candidate.
