# D29 IDs-3-and-above light-record filter

This local diagnostic preserves the real tiled-light list and real light IDs
0, 1, and 2. It inserts the equivalent of:

```text
filtered_id = light_id < 3 ? light_id : 0
```

The existing consumer branch then skips captured IDs 3 and 4 while leaving IDs
1 and 2 and their `t7` records unchanged. The patcher inserts one unsigned
comparison and one select, rewires exactly one sentinel comparison and one
record address, and rejects unexpected input signatures. Both outputs must
pass Vulkan 1.3 validation.

Interpretation:

- artifact gone: ID/record 3 is required, because D28 already excluded ID 4;
- artifact remains: ID/record 2 or its interaction with ID 1 is required;
- materially broken lighting: do not classify the visual result.

This remains a diagnostic shader override, not the eventual compatibility fix.
