# D28 ID-4 light-record filter: result

## Result

The square artifact returned with D28. This excludes captured light ID 4 and
its selected `t7` record as a necessary cause.

D28 preserved the real `t9` tile data, loop bounds, `t10` list fetches, and
actual IDs/records 1–3. Only ID 4 was mapped to the consumer's existing zero
sentinel. Because the visible grid remained, the D27 clean result must depend
on replacing IDs 2 and/or 3 (or on removing their interaction), not merely on
removing the highest captured ID.

## Run verification

- Run: `D28-filter-light-id4-r1`
- Compatibility tool: `IL2-Korea-D25-LightAtomicCompat-84c87c83`
- Log: `/tmp/il2-D28-filter-light-id4-r1/steam-247970.log`
- Visual result reported by the user: squares returned
- Shader/device failure: none

The completed log confirms one override for each intended consumer hash:

```text
Overriding shader hash df0bd777fd1bb89d .../d28-filter-light-id4-overrides/df0bd777fd1bb89d.spv
Overriding shader hash a2d104d5c813322e .../d28-filter-light-id4-overrides/a2d104d5c813322e.spv
```

## Next split

D29 should preserve IDs 1 and 2 while mapping IDs 3 and above to sentinel 0.
If the grid disappears, ID 3 is required. If it remains, ID 2 or its
interaction with ID 1 is required; a following ID-1-only real-list filter can
separate those cases.

D28 is a diagnostic negative, not a fix or upload candidate.
