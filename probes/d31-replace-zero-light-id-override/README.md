# D31 replace only sentinel light ID 0

D31 preserves every real nonzero light ID and its selected `t7` record. It
changes only original ID 0 to safe ID 1:

```text
filtered_id = light_id < 1 ? 1 : light_id
```

This keeps the actual lights and list traversal but prevents the consumer's
zero/sentinel skip branch. It is the direct comparison to D30's mixed
evaluate/skip result and is much less destructive than D27's all-ID-1 control.

Interpretation:

- artifact gone: original zero/sentinel handling or mixed loop control flow is
  required;
- artifact remains: nonzero record diversity is independently sufficient;
- materially broken lighting: do not classify the visual result.

The Makefile reuses the audited range/select generator and validates both
modules for Vulkan 1.3. This remains a diagnostic, not the final fix.
