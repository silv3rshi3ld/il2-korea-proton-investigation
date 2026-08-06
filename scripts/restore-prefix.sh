#!/usr/bin/env bash
set -euo pipefail

app_id="247970"
archive=""
compat_dir_override=""
confirmed=0

usage() {
    printf '%s\n' \
        "Usage: $0 --archive PATH [--compat-dir PATH] --yes" \
        "" \
        "Validates and restores a backup made by backup-prefix.sh." \
        "The displaced prefix is retained as compatdata/${app_id}.pre-restore-*."
}

while (($#)); do
    case "$1" in
        --archive)
            archive="${2:?--archive requires a path}"
            shift 2
            ;;
        --compat-dir)
            compat_dir_override="${2:?--compat-dir requires a path}"
            shift 2
            ;;
        --yes)
            confirmed=1
            shift
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

if [[ -z "$archive" || $confirmed -ne 1 ]]; then
    usage >&2
    exit 2
fi
if [[ ! -f "$archive" ]]; then
    printf 'Archive not found: %s\n' "$archive" >&2
    exit 1
fi
archive=$(readlink -f -- "$archive")

find_compat_dir() {
    local candidate
    if [[ -n "$compat_dir_override" ]]; then
        if [[ -d "$compat_dir_override" ]]; then
            readlink -f -- "$compat_dir_override"
        else
            readlink -m -- "$compat_dir_override"
        fi
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

archive_list() {
    case "$archive" in
        *.tar.zst) tar --zstd -tf "$archive" ;;
        *.tar.gz|*.tgz) tar -tzf "$archive" ;;
        *.tar) tar -tf "$archive" ;;
        *)
            printf 'Unsupported archive suffix: %s\n' "$archive" >&2
            return 2
            ;;
    esac
}

archive_extract() {
    local destination=$1
    case "$archive" in
        *.tar.zst) tar --zstd --xattrs --acls --no-same-owner -xf "$archive" -C "$destination" ;;
        *.tar.gz|*.tgz) tar -xzf "$archive" --xattrs --acls --no-same-owner -C "$destination" ;;
        *.tar) tar --xattrs --acls --no-same-owner -xf "$archive" -C "$destination" ;;
    esac
}

if ! compat_dir=$(find_compat_dir); then
    printf 'Could not locate the target; pass --compat-dir ending in compatdata/%s.\n' "$app_id" >&2
    exit 1
fi
compat_parent=$(dirname -- "$compat_dir")
if [[ "$(basename -- "$compat_dir")" != "$app_id" || "$(basename -- "$compat_parent")" != "compatdata" ]]; then
    printf 'Refusing unexpected restore target: %s\n' "$compat_dir" >&2
    exit 1
fi
if [[ ! -d "$compat_parent" ]]; then
    printf 'Target parent does not exist: %s\n' "$compat_parent" >&2
    exit 1
fi

game_processes=$(pgrep -af 'IL2Series\.exe|IL2Series/Launcher\.exe|SteamLaunch AppId=247970' 2>/dev/null || true)
if [[ -n "$game_processes" ]]; then
    printf 'Refusing to restore a live prefix. Active processes:\n%s\n' "$game_processes" >&2
    exit 1
fi
steam_processes=$(pgrep -ax steam 2>/dev/null || true)
if [[ -n "$steam_processes" ]]; then
    printf 'Refusing to restore while Steam is running; exit Steam completely first:\n%s\n' "$steam_processes" >&2
    exit 1
fi

checksum="$archive.sha256"
if [[ -f "$checksum" ]]; then
    printf 'Verifying archive checksum...\n'
    (
        cd -- "$(dirname -- "$archive")"
        sha256sum -c -- "$(basename -- "$checksum")"
    )
else
    printf 'Warning: no adjacent checksum file found; computing SHA-256 only.\n' >&2
    sha256sum -- "$archive"
fi

printf 'Validating archive paths...\n'
while IFS= read -r entry; do
    if [[ "$entry" == /* || "$entry" == ../* || "$entry" == *'/../'* || \
          ( "$entry" != "$app_id" && "$entry" != "$app_id/" && "$entry" != "$app_id/"* ) ]]; then
        printf 'Unsafe or unexpected archive entry: %s\n' "$entry" >&2
        exit 1
    fi
done < <(archive_list)

staging=$(mktemp -d -- "$compat_parent/.il2-restore.XXXXXX")
cleanup() {
    if [[ -n "${staging:-}" && -d "$staging" ]]; then
        rm -rf -- "$staging"
    fi
}
trap cleanup EXIT INT TERM

archive_extract "$staging"
restored="$staging/$app_id"
if [[ ! -d "$restored/pfx" ]]; then
    printf 'Archive did not produce %s/pfx; target is unchanged.\n' "$restored" >&2
    exit 1
fi

timestamp=$(date -u +%Y%m%dT%H%M%SZ)
displaced="$compat_parent/${app_id}.pre-restore-$timestamp"
if [[ -e "$displaced" ]]; then
    printf 'Displaced-prefix target already exists: %s\n' "$displaced" >&2
    exit 1
fi

printf 'Restoring archive: %s\n' "$archive"
printf 'Target:            %s\n' "$compat_dir"
if [[ -e "$compat_dir" ]]; then
    printf 'Preserving current prefix at: %s\n' "$displaced"
    mv -- "$compat_dir" "$displaced"
fi

if ! mv -- "$restored" "$compat_dir"; then
    printf 'Failed to install restored prefix; attempting rollback.\n' >&2
    if [[ -d "$displaced" && ! -e "$compat_dir" ]]; then
        mv -- "$displaced" "$compat_dir"
    fi
    exit 1
fi

printf '\nRestore completed.\n'
printf 'Restored prefix: %s\n' "$compat_dir"
if [[ -d "$displaced" ]]; then
    printf 'Displaced prefix (recoverable): %s\n' "$displaced"
fi
printf 'Start Steam and let Proton perform any required prefix migration before testing.\n'
