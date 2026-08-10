# D32 isolate genuine light ID 2

D32 keeps the original per-tile list length and executes every loop iteration.
It selects record 2 only when the extracted list ID is exactly 2; every other
entry selects known-safe record 1:

```text
filtered_id = light_id == 2 ? 2 : 1
```

There are no zero IDs and no skipped iterations. Therefore:

- artifact remains: record 2 is sufficient;
- artifact disappears: the required genuine record is ID 3 or 4;
- materially broken lighting: do not classify the visual result.

The generator verifies the exact extraction, zero test, and record-index shift,
then inserts one `OpIEqual` and one `OpSelect`. Both modules are validated for
Vulkan 1.3. This is a local diagnostic, not the final compatibility fix.
