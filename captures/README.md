# Local captures

This directory contains ignored local evidence:

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
- `runs/`: exact compressed Proton logs, compact module/summary files,
  metadata, and observations;
- `comparisons/`: generated log comparisons.

Images and generated logs are deliberately not committed. Their filenames and
SHA-256 checksums are recorded in the corresponding `docs/evidence-*.md` files.
Review and redact any selected artifact before attaching it upstream.

`proton.log.gz` is the retained exact log for each completed run. Redundant raw
logs and large generated `filtered.log` copies may be moved out after the gzip
has been validated against `source-log.sha256`; `compare-logs.py` reads the
exact compressed log first.
