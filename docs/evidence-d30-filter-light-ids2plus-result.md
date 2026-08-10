# D30 IDs-2-and-above record filter: result

## Result

The square artifact remained with D30 even though only genuine ID-1 entries
could evaluate a light record. IDs 2–4 all took the existing sentinel/skip
branch. This rejects every record above 1 as individually necessary.

Together, the consumer controls now show:

- D26: no loop evaluations, clean;
- D27: every iteration evaluates safe record 1, clean;
- D30: a mixture of genuine record-1 evaluations and skipped iterations,
  squares present.

The important discriminator is therefore mixed evaluation/sentinel handling
or per-tile membership, not the contents of one higher-numbered light record.

## Run verification

- Run: `D30-filter-light-ids2plus-r1`
- Compatibility tool: `IL2-Korea-D25-LightAtomicCompat-84c87c83`
- Log: `/tmp/il2-D30-filter-light-ids2plus-r1/steam-247970.log`
- Visual result reported by the user: squares still present
- Shader/device failure: none

The completed log confirms both intended overrides exactly once with no
graphics error invalidating the comparison.

## Next split

D31 should preserve every real nonzero ID/record and replace only original
sentinel ID 0 with safe ID 1. This prevents the skip branch without discarding
the actual lights. A clean result selects zero/sentinel handling. Persistence
would show that nonzero record diversity is independently sufficient.

D30 is diagnostic evidence, not a fix or upload candidate.
