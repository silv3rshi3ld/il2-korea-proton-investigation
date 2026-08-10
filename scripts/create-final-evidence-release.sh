#!/usr/bin/env bash
set -euo pipefail

usage() {
    cat <<'EOF'
Usage: ./scripts/create-final-evidence-release.sh --output-dir DIRECTORY

Build the deterministic, allowlisted final evidence archive from a clean Git
checkout. The output directory must not already contain the release files.
EOF
}

output_dir=""
while (($#)); do
    case "$1" in
        --output-dir)
            if (($# < 2)); then
                printf 'error: --output-dir requires a value\n' >&2
                exit 2
            fi
            output_dir=$2
            shift 2
            ;;
        --help|-h)
            usage
            exit 0
            ;;
        *)
            printf 'error: unknown argument: %s\n' "$1" >&2
            usage >&2
            exit 2
            ;;
    esac
done

if [[ -z "$output_dir" ]]; then
    printf 'error: --output-dir is required\n' >&2
    usage >&2
    exit 2
fi

for command_name in git install mktemp sha256sum tar gzip date; do
    if ! command -v "$command_name" >/dev/null 2>&1; then
        printf 'error: required command not found: %s\n' "$command_name" >&2
        exit 1
    fi
done

repo_root=$(git rev-parse --show-toplevel 2>/dev/null) || {
    printf 'error: not inside a Git checkout\n' >&2
    exit 1
}

cd "$repo_root"

if [[ -n "$(git status --porcelain --untracked-files=normal)" ]]; then
    printf 'error: checkout is dirty; commit or remove tracked/untracked changes first\n' >&2
    exit 1
fi

source_commit=$(git rev-parse HEAD)
source_epoch=$(git show -s --format=%ct HEAD)
source_date=$(date --utc --date="@$source_epoch" '+%Y-%m-%d %H:%M:%S UTC')

bundle_name="il2-korea-final-evidence-2026-08-10"
archive_name="$bundle_name.tar.gz"

if [[ "$output_dir" != /* ]]; then
    output_dir="$repo_root/$output_dir"
fi

mkdir -p "$output_dir"

archive_path="$output_dir/$archive_name"
checksums_path="$output_dir/SHA256SUMS"
if [[ -e "$archive_path" || -e "$checksums_path" ]]; then
    printf 'error: release output already exists in %s\n' "$output_dir" >&2
    exit 1
fi

mapfile -t release_files < <(
    {
        printf '%s\n' README.md PROVENANCE.md
        git ls-files -- docs patches probes scripts captures/README.md captures/curated
    } | LC_ALL=C sort -u
)

for source_path in "${release_files[@]}"; do
    if [[ ! -f "$source_path" ]]; then
        printf 'error: allowlisted release file is missing: %s\n' "$source_path" >&2
        exit 1
    fi
    if ! git ls-files --error-unmatch "$source_path" >/dev/null 2>&1; then
        printf 'error: allowlisted release file is not tracked: %s\n' "$source_path" >&2
        exit 1
    fi
done

temporary_root=$(mktemp -d)
cleanup() {
    rm -rf -- "$temporary_root"
}
trap cleanup EXIT

bundle_root="$temporary_root/$bundle_name"
mkdir -p "$bundle_root"

for source_path in "${release_files[@]}"; do
    install -D -m 0644 "$source_path" "$bundle_root/$source_path"
done

manifest_path="$bundle_root/MANIFEST.md"
{
    printf '# Final evidence release manifest\n\n'
    printf -- '- Source repository: <https://github.com/silv3rshi3ld/il2-korea-proton-investigation>\n'
    printf -- '- Source commit: `%s`\n' "$source_commit"
    printf -- '- Source commit date: %s\n' "$source_date"
    printf -- '- Investigation status date: 2026-08-10\n'
    printf -- '- Wine startup path: <https://gitlab.winehq.org/wine/wine/-/merge_requests/11604>\n'
    printf -- '- Terrain path: <https://github.com/HansKristian-Work/vkd3d-proton/pull/3202>\n'
    printf -- '- Lighting path: <https://github.com/HansKristian-Work/vkd3d-proton/pull/3207>\n\n'
    printf 'This archive contains source, documentation, patches, and reviewed\n'
    printf 'screenshots only. It contains no game files, custom Proton binary,\n'
    printf 'prebuilt replacement DLL, prefix, credentials, shader binary, raw\n'
    printf 'RenderDoc capture, or unfiltered runtime trace.\n\n'
    printf '## Included files\n\n'
    printf '| SHA-256 | Path |\n'
    printf '| --- | --- |\n'
    for source_path in "${release_files[@]}"; do
        file_hash=$(sha256sum "$bundle_root/$source_path")
        file_hash=${file_hash%% *}
        printf '| `%s` | `%s` |\n' "$file_hash" "$source_path"
    done
} >"$manifest_path"
chmod 0644 "$manifest_path"

find "$bundle_root" -type d -exec chmod 0755 {} +
find "$bundle_root" -type f -exec touch --date="@$source_epoch" {} +
find "$bundle_root" -type d -exec touch --date="@$source_epoch" {} +

LC_ALL=C tar \
    --sort=name \
    --mtime="@$source_epoch" \
    --owner=0 \
    --group=0 \
    --numeric-owner \
    --format=posix \
    --pax-option=delete=atime,delete=ctime \
    -C "$temporary_root" \
    -cf - "$bundle_name" | gzip -n >"$archive_path"

(
    cd "$output_dir"
    sha256sum "$archive_name" >SHA256SUMS
)

printf 'archive: %s\n' "$archive_path"
printf 'checksums: %s\n' "$checksums_path"
printf 'source commit: %s\n' "$source_commit"
