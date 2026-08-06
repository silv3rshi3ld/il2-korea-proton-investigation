#!/usr/bin/env bash
set -euo pipefail

app_id="247970"
compat_dir_override=""
backup_root_override=""

usage() {
    printf '%s\n' \
        "Usage: $0 [--compat-dir PATH] [--output-dir PATH]" \
        "" \
        "Creates a compressed, checksummed backup of compatdata/$app_id." \
        "Steam and the game must be stopped. Existing backups are never overwritten."
}

while (($#)); do
    case "$1" in
        --compat-dir)
            compat_dir_override="${2:?--compat-dir requires a path}"
            shift 2
            ;;
        --output-dir)
            backup_root_override="${2:?--output-dir requires a path}"
            shift 2
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            printf 'Unknown argument: %s\n' "$1" >&2
            usage >&2
            exit 2
            ;;
    esac
done

find_compat_dir() {
    local candidate
    if [[ -n "$compat_dir_override" ]]; then
        readlink -f -- "$compat_dir_override"
        return
    fi
    for candidate in \
        "${STEAM_COMPAT_DATA_PATH:-}" \
        "${HOME:-}/.local/share/Steam/steamapps/compatdata/$app_id" \
        "${HOME:-}/.steam/root/steamapps/compatdata/$app_id"; do
        if [[ -n "$candidate" && -d "$candidate/pfx" ]]; then
            readlink -f -- "$candidate"
            return
        fi
    done
    return 1
}

game_processes=$(pgrep -af 'IL2Series\.exe|IL2Series/Launcher\.exe|SteamLaunch AppId=247970' 2>/dev/null || true)
if [[ -n "$game_processes" ]]; then
    printf 'Refusing to back up a live prefix. Active processes:\n%s\n' "$game_processes" >&2
    exit 1
fi
steam_processes=$(pgrep -ax steam 2>/dev/null || true)
if [[ -n "$steam_processes" ]]; then
    printf 'Refusing to back up while Steam is running; exit Steam completely first:\n%s\n' "$steam_processes" >&2
    exit 1
fi

if ! compat_dir=$(find_compat_dir); then
    printf 'Could not locate compatdata/%s; pass --compat-dir.\n' "$app_id" >&2
    exit 1
fi
if [[ ! -d "$compat_dir/pfx" || "$(basename -- "$compat_dir")" != "$app_id" ]]; then
    printf 'Refusing unexpected compatibility directory: %s\n' "$compat_dir" >&2
    exit 1
fi

compat_parent=$(dirname -- "$compat_dir")
if [[ "$(basename -- "$compat_parent")" != "compatdata" ]]; then
    printf 'Compatibility directory parent is not named compatdata: %s\n' "$compat_parent" >&2
    exit 1
fi

if [[ -n "$backup_root_override" ]]; then
    backup_root=$backup_root_override
else
    state_root=${XDG_STATE_HOME:-${HOME:?HOME is required when --output-dir is omitted}/.local/state}
    backup_root="$state_root/il2-korea-proton-investigation/prefix-backups"
fi
mkdir -p -- "$backup_root"
backup_root=$(readlink -f -- "$backup_root")

timestamp=$(date -u +%Y%m%dT%H%M%SZ)
base_name="il2-korea-${app_id}-prefix-$timestamp"
if command -v zstd >/dev/null 2>&1; then
    extension="tar.zst"
else
    extension="tar.gz"
fi
archive="$backup_root/$base_name.$extension"
partial="$archive.partial.$$"
metadata="$archive.metadata.txt"
checksum="$archive.sha256"

for path in "$archive" "$metadata" "$checksum"; do
    if [[ -e "$path" ]]; then
        printf 'Refusing to overwrite existing backup artifact: %s\n' "$path" >&2
        exit 1
    fi
done

cleanup() {
    if [[ -e "$partial" ]]; then
        rm -f -- "$partial"
    fi
}
trap cleanup EXIT INT TERM

source_bytes=$(du -sb -- "$compat_dir" | awk '{ print $1 }')
printf 'Backing up %s (%s bytes)\n' "$compat_dir" "$source_bytes"
printf 'Destination: %s\n' "$archive"

if [[ "$extension" == "tar.zst" ]]; then
    tar --xattrs --acls --numeric-owner -C "$compat_parent" -cf - "$app_id" \
        | zstd -T0 -10 -q -o "$partial"
else
    tar --xattrs --acls --numeric-owner -C "$compat_parent" -czf "$partial" "$app_id"
fi
mv -- "$partial" "$archive"

archive_name=$(basename -- "$archive")
archive_sha=$(sha256sum -- "$archive" | awk '{ print $1 }')
printf '%s  %s\n' "$archive_sha" "$archive_name" >"$checksum"

{
    printf 'created_utc=%s\n' "$(date -u --iso-8601=seconds)"
    printf 'app_id=%s\n' "$app_id"
    printf 'source=%s\n' "$compat_dir"
    printf 'source_bytes=%s\n' "$source_bytes"
    printf 'archive=%s\n' "$archive"
    printf 'archive_bytes=%s\n' "$(stat -c %s -- "$archive")"
    printf 'sha256=%s\n' "$archive_sha"
    if [[ -f "$compat_dir/version" ]]; then
        printf 'prefix_version=%s\n' "$(sed -n '1p' "$compat_dir/version")"
    fi
} >"$metadata"

(
    cd -- "$backup_root"
    sha256sum -c -- "$(basename -- "$checksum")"
)

printf '\nBackup complete and verified.\n'
printf 'Archive:  %s\n' "$archive"
printf 'Checksum: %s\n' "$checksum"
printf 'Metadata: %s\n' "$metadata"
printf 'Restore with:\n  %q --archive %q --yes\n' "$(dirname -- "$0")/restore-prefix.sh" "$archive"
