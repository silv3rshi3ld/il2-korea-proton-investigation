#!/usr/bin/env bash
set -euo pipefail

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)
project_dir=$(cd -- "$script_dir/.." && pwd -P)
output_dir="$project_dir/captures/community-handoff-2026-08-06"
archive="$project_dir/captures/il2-korea-community-handoff-2026-08-06.tar.gz"
d02="$project_dir/captures/runs/D02-r1"
d05="$project_dir/captures/runs/D05-bc3-r1"

if [[ -e "$output_dir" || -e "$archive" ]]; then
    printf 'Refusing to overwrite an existing handoff directory or archive.\n' >&2
    exit 1
fi

for required in \
        "$d02/proton.log.gz" "$d02/filtered.log" "$d02/texture-trace-analysis.md" \
        "$d02/summary.txt" "$d02/system-info.txt" \
        "$d05/proton.log.gz" "$d05/bc3-border-copy-analysis.md" \
        "$d05/summary.txt" "$d05/system-info.txt" \
        "$project_dir/patches/0004-vkd3d-Add-gated-BC3-border-copy-diagnostic-for-IL-2-.patch" \
        "$project_dir/patches/0005-vkd3d-Handle-footprint-only-IL-2-BC3-border-copies.patch" \
        "$project_dir/captures/curated/d05-bc3-normalization/D05a-r1-terrain-unchanged-invalid-gate-1416m.png"; do
    if [[ ! -f "$required" ]]; then
        printf 'Missing required handoff input: %s\n' "$required" >&2
        exit 1
    fi
done

mkdir -p -- "$output_dir/logs" "$output_dir/analysis" "$output_dir/patches" "$output_dir/screenshots"

sanitize() {
    sed -E \
        -e 's#/home/silv3rshi3ld#/home/USER#gI' \
        -e 's/silv3rshi3ld/USER/gI' \
        -e 's/DreamHQ/HOST/g' \
        -e 's/[0-9]{17}/STEAM_ID/g'
}

gzip -cd -- "$d02/proton.log.gz" | sanitize | gzip -n -9 >"$output_dir/logs/D02-proton-redacted.log.gz"
gzip -cd -- "$d05/proton.log.gz" | sanitize | gzip -n -9 >"$output_dir/logs/D05a-proton-redacted.log.gz"
sanitize <"$d05/system-info.txt" >"$output_dir/analysis/system-info-redacted.txt"
sanitize <"$d02/summary.txt" >"$output_dir/analysis/D02-summary-redacted.txt"
sanitize <"$d05/summary.txt" >"$output_dir/analysis/D05a-summary-redacted.txt"

rg --no-filename --text \
    'IL2TEX enabled:|IL2TEX create .*dimension=3 width=2048 height=2048 depth_or_layers=1 mips=1 format=0x4d|IL2TEX copy .*extent=(1x64x1|64x1x1|1x128x1|128x1x1)' \
    "$d02/filtered.log" | sanitize >"$output_dir/logs/D02-bc3-cache-excerpt.log"

cp -- "$d02/texture-trace-analysis.md" "$output_dir/analysis/D02-texture-trace-analysis.md"
cp -- "$d05/bc3-border-copy-analysis.md" "$output_dir/analysis/D05a-bc3-zero-match-analysis.md"
sanitize <"$project_dir/patches/0004-vkd3d-Add-gated-BC3-border-copy-diagnostic-for-IL-2-.patch" \
    >"$output_dir/patches/0004-vkd3d-Add-gated-BC3-border-copy-diagnostic-for-IL-2-.patch"
sanitize <"$project_dir/patches/0005-vkd3d-Handle-footprint-only-IL-2-BC3-border-copies.patch" \
    >"$output_dir/patches/0005-vkd3d-Handle-footprint-only-IL-2-BC3-border-copies.patch"
cp -- "$project_dir/captures/curated/d05-bc3-normalization/D05a-r1-terrain-unchanged-invalid-gate-1416m.png" \
    "$output_dir/screenshots/"

if rg -i --text \
        'authorization:|bearer [a-z0-9._-]+|password[=:]|passwd[=:]|access[_-]?token[=:]|refresh[_-]?token[=:]' \
        "$output_dir"; then
    printf 'Potential credential-like text found; refusing to create archive. Review the handoff directory.\n' >&2
    exit 1
fi

{
    printf '%s\n' \
        '# IL-2 Korea Proton investigation handoff' \
        '' \
        'This is a redacted, review-only support bundle for Proton #9906 and VKD3D-Proton #3134.' \
        'D02 is the valid texture trace containing the BC3 cache evidence. D05a loaded the intended' \
        'diagnostic DLL but adjusted zero copies, so its unchanged screenshot is not a failed-fix result.' \
        'Patch 0005 is the compiled but unrun footprint-aware revision.' \
        '' \
        'Original retained log hashes (not the redacted files):' \
        '- D02 proton.log.gz: b60f9b3316d4e175b70e5f26b7c663cf5843f7b688fa60e7e2e00750ce2e2aa4' \
        '- D05a proton.log.gz: 43c005339456055cfb115e95facae871c4229496a788d24b74b55a6cafee0e75' \
        '' \
        'Shared-file SHA-256:'
    (
        cd -- "$output_dir"
        find . -type f ! -name SHA256SUMS -print0 | sort -z | xargs -0 sha256sum
    )
} >"$output_dir/README.md"

(
    cd -- "$output_dir"
    find . -type f ! -name SHA256SUMS -print0 | sort -z | xargs -0 sha256sum >SHA256SUMS
)

tar --sort=name --mtime='UTC 2026-08-06' --owner=0 --group=0 --numeric-owner \
    -czf "$archive" -C "$(dirname -- "$output_dir")" "$(basename -- "$output_dir")"

printf 'Created review-only handoff archive:\n  %s\n' "$archive"
sha256sum -- "$archive"
