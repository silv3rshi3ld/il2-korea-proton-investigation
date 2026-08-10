# D30 IDs-2-and-above light-record filter

This final record discriminator preserves genuine light ID 1 entries from the
real tiled-light list and maps every ID 2 or above to sentinel 0:

```text
filtered_id = light_id < 2 ? light_id : 0
```

Unlike D27, it does not replace every list entry with ID 1. It therefore keeps
the original list positions and processes only entries that were genuinely ID
1. D28 and D29 already show IDs 4 and 3 are not necessary.

Interpretation:

- artifact gone: ID/record 2 is required;
- artifact remains: real-list distribution/traversal, rather than record 2 by
  itself, distinguishes D30 from the clean D27 control;
- materially broken lighting: do not classify the visual result.

The Makefile reuses D29's audited range-filter generator and validates both
outputs for Vulkan 1.3. This is diagnostic work, not the final fix.
