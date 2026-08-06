# Local captures

This directory contains ignored local evidence:

- `curated/e00-baseline/`: six descriptively named baseline screenshots;
- `curated/e01-no-upload-hvv/`: three host-visible-upload test screenshots;
- `curated/e02-single-queue/`: three single-queue test screenshots;
- `curated/d01-sparse-trace/`: one high-altitude screenshot from the invalidated
  prefix-install attempt; the visual evidence is valid but no trace ran;
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
