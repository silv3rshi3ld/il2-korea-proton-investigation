# Captures

This directory contains a curated public screenshot set and additional ignored
local evidence. The published images document the broken baseline, the repaired
terrain path, and historical lighting A/B observations. D50 through D52 later
isolated the lighting defect, but no D52 screenshot was captured or published.

## Published evidence

### Broken baseline

- [`E00-r1-menu-aircraft-block-artifacts.png`](curated/e00-baseline/E00-r1-menu-aircraft-block-artifacts.png): menu-aircraft blocks and shimmering;
- [`E00-r1-terrain-cockpit-missing-pages.png`](curated/e00-baseline/E00-r1-terrain-cockpit-missing-pages.png): missing terrain pages from the cockpit;
- [`E00-r2-terrain-external-missing-pages-magenta-seams.png`](curated/e00-baseline/E00-r2-terrain-external-missing-pages-magenta-seams.png): missing pages and magenta seams from the external camera.

### Gated causal fix (D07)

- [`D07-r1-terrain-repaired-forward-view-5491m.png`](curated/d07-page-copy/D07-r1-terrain-repaired-forward-view-5491m.png): repaired terrain at 5,491 m;
- [`D07-r1-terrain-repaired-right-view-5501m.png`](curated/d07-page-copy/D07-r1-terrain-repaired-right-view-5501m.png): repaired right-side terrain at 5,501 m;
- [`D07-r2-menu-aircraft-blocks-shimmering-persists.png`](curated/d07-page-copy/D07-r2-menu-aircraft-blocks-shimmering-persists.png): terrain fix does not resolve the menu-aircraft issue.

### General fix (D08)

- [`D08-r1-terrain-repaired-cockpit-4813m.png`](curated/d08-general-fix/D08-r1-terrain-repaired-cockpit-4813m.png): repaired terrain at 4,813 m;
- [`D08-r1-terrain-repaired-dive-2427m.png`](curated/d08-general-fix/D08-r1-terrain-repaired-dive-2427m.png): repaired terrain during a dive at 2,427 m;
- [`D08-r1-terrain-repaired-low-altitude-742m.png`](curated/d08-general-fix/D08-r1-terrain-repaired-low-altitude-742m.png): repaired terrain at 742 m;
- [`D08-r1-menu-aircraft-block-artifacts-shimmering-persists.png`](curated/d08-general-fix/D08-r1-menu-aircraft-block-artifacts-shimmering-persists.png): the independent menu-aircraft defect persists.

### Lighting evidence boundary

The repository's published lighting screenshots predate D52 and remain valid
historical visual evidence. They must not be described as screenshots of the
D52 VKD3D-Proton-only R32 alias. The D52 result consists of two observed clean
game runs plus runtime and shader-path validation, recorded in
[`../docs/evidence-d50-d52-r32-alias-result.md`](../docs/evidence-d50-d52-r32-alias-result.md).
Those isolated runs used the OpenMP startup workaround and excluded both the
Wine startup series and merged terrain fix.

The sanitized text evidence for the exact D52 tool, DLL hashes, runtime marker,
and captured shader identities is retained at
[`curated/d52-r32-alias/runtime-proof.txt`](curated/d52-r32-alias/runtime-proof.txt).
It contains no game binary or custom Proton binary.

## Additional local evidence

The working copy also contains the following intentionally ignored evidence:

- `curated/e00-baseline/`: six descriptively named baseline screenshots;
- `curated/e01-no-upload-hvv/`: three host-visible-upload test screenshots;
- `curated/e02-single-queue/`: three single-queue test screenshots;
- `curated/d01-sparse-trace/`: one high-altitude screenshot from the invalidated
  prefix-install attempt; the visual evidence is valid but no trace ran;
- `curated/d01b-custom-proton-trace/`: one screenshot from the valid custom-
  Proton sparse-resource trace;
- `curated/d02-ordinary-texture-trace/`: one cockpit screenshot showing the
  unchanged missing terrain pages at 1,385 m during valid ordinary-texture
  telemetry; SHA-256 is recorded in `docs/evidence-d02-ordinary-texture-trace.md`;
- `curated/e03-no-descriptor-buffer/`: menu, 4,858 m, and 1,121 m screenshots
  from the first descriptor-buffer-disabled run; hashes are recorded in
  `docs/evidence-e03-no-descriptor-buffer.md`;
- `curated/d04-upstream/`: menu and 6,400 m screenshots from the unmodified
  current-upstream control; hashes are recorded in
  `docs/evidence-d04-upstream-result.md`;
- `curated/d05-bc3-normalization/`: D05a's unchanged 1,416 m screenshot. The
  visual artifact is retained, but the causal run is invalid because the gate
  adjusted zero copies; its hash is in `docs/evidence-d05-result.md`;
- `curated/d07-page-copy/`: repaired-terrain and menu screenshots from two
  valid gated causal runs; hashes are recorded in
  `docs/evidence-d07-result.md`;
- `curated/d08-general-fix/`: one unchanged menu-aircraft capture and three
  repaired-terrain captures from the clean general `cf11ba76` build; hashes
  are recorded in `docs/evidence-d08-result.md`;
- `runs/`: exact compressed Proton logs, compact module/summary files,
  metadata, and observations;
- `comparisons/`: generated log comparisons;
- `validation/`: ignored full local test transcripts; the `cf11ba76` native
  copy-test subset and the current `64ec55e7` focused/full results and hashes
  are recorded in `docs/upstream-submission-plan.md` and
  `docs/evidence-pr-scope-refinement.md`.

Only the curated images listed above are committed. Other images and generated
logs remain ignored; their filenames and SHA-256 checksums are recorded in the
corresponding `docs/evidence-*.md` files. Review and redact any additional
artifact before publishing it upstream.

`proton.log.gz` is the retained exact log for each completed run. Redundant raw
logs and large generated `filtered.log` copies may be moved out after the gzip
has been validated against `source-log.sha256`; `compare-logs.py` reads the
exact compressed log first.
