# D29 IDs-3-and-above record filter: result

## Result

The square artifact remained with D29. This excludes captured light IDs 3 and
4, and their selected `t7` records, as necessary causes.

D29 preserved the real `t9` data, list traversal, loop bounds, and actual IDs
1 and 2. It mapped only IDs 3 and above to the existing zero sentinel. Since
D27 was clean when every iteration selected record 1, the remaining
record-level suspect is ID 2, with a smaller open possibility involving the
real ID-1/ID-2 distribution or interaction.

## Run verification

- Run: `D29-filter-light-ids3plus-r1`
- Compatibility tool: `IL2-Korea-D25-LightAtomicCompat-84c87c83`
- Log: `/tmp/il2-D29-filter-light-ids3plus-r1/steam-247970.log`
- Visual result reported by the user: squares still present
- Shader/device failure: none

The log confirms one replacement for each intended consumer hash and contains
no graphics failure invalidating the comparison.

## Next split

D30 should preserve only genuine ID-1 list entries and map every ID 2 or above
to sentinel 0. A clean result isolates ID/record 2. Persistence would instead
show that real list distribution/traversal differs materially from D27's
fixed-record control.

D29 is a diagnostic negative, not a fix or upload candidate.
