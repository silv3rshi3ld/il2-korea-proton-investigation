#!/usr/bin/env bash
set -euo pipefail

app_id="247970"
mode=${1:-}
if [[ -n "$mode" ]]; then
    shift
fi
build_dir=""
backup_dir=""
backup_root_override=""
compat_dir_override=""
confirmed=0

usage() {
    printf '%s\n' \
        "Usage:" \
        "  $0 install --build-dir PATH [--compat-dir PATH] [--backup-root PATH] --yes" \
        "  $0 restore --backup-dir PATH [--compat-dir PATH] [--backup-root PATH] --yes" \
        "" \
        "Installs only x64/x86 d3d12.dll and d3d12core.dll into the AppID $app_id prefix." \
        "The Proton installation is never modified. Official package-release output is expected." \
        "WARNING: Stock Proton replaces these prefix DLLs during launch. Use this for backup/restore" \
        "or direct-Wine diagnostics; use create-custom-proton.sh for Steam runtime testing."
}

while (($#)); do
    case "$1" in
        --build-dir)
            build_dir="${2:?--build-dir requires a path}"
            shift 2
            ;;
        --backup-dir)
            backup_dir="${2:?--backup-dir requires a path}"
            shift 2
            ;;
        --backup-root)
            backup_root_override="${2:?--backup-root requires a path}"
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

if [[ "$mode" == "-h" || "$mode" == "--help" || "$mode" == "help" ]]; then
    usage
    exit 0
fi
if [[ "$mode" != "install" && "$mode" != "restore" ]]; then
    usage >&2
    exit 2
fi
if [[ $confirmed -ne 1 ]]; then
    printf 'This operation changes prefix DLLs; re-run with --yes after reviewing the paths.\n' >&2
    exit 2
fi

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

if ! compat_dir=$(find_compat_dir); then
    printf 'Could not locate compatdata/%s; pass --compat-dir.\n' "$app_id" >&2
    exit 1
fi
compat_parent=$(dirname -- "$compat_dir")
if [[ "$(basename -- "$compat_dir")" != "$app_id" || "$(basename -- "$compat_parent")" != "compatdata" ]]; then
    printf 'Refusing unexpected compatibility directory: %s\n' "$compat_dir" >&2
    exit 1
fi

system32="$compat_dir/pfx/drive_c/windows/system32"
syswow64="$compat_dir/pfx/drive_c/windows/syswow64"
if [[ ! -d "$system32" || ! -d "$syswow64" ]]; then
    printf 'Prefix system DLL directories are missing under %s\n' "$compat_dir" >&2
    exit 1
fi

game_processes=$(pgrep -af 'IL2Series\.exe|IL2Series/Launcher\.exe|SteamLaunch AppId=247970' 2>/dev/null || true)
if [[ -n "$game_processes" ]]; then
    printf 'Refusing to change DLLs while IL-2/Proton is active:\n%s\n' "$game_processes" >&2
    exit 1
fi
steam_processes=$(pgrep -ax steam 2>/dev/null || true)
if [[ -n "$steam_processes" ]]; then
    printf 'Refusing to change DLLs while Steam is running; exit Steam completely first:\n%s\n' "$steam_processes" >&2
    exit 1
fi

if [[ -n "$backup_root_override" ]]; then
    backup_root=$backup_root_override
else
    state_root=${XDG_STATE_HOME:-${HOME:?HOME is required when --backup-root is omitted}/.local/state}
    backup_root="$state_root/il2-korea-proton-investigation/vkd3d-dll-backups"
fi
mkdir -p -- "$backup_root"
backup_root=$(readlink -f -- "$backup_root")

validate_architecture() {
    local path=$1
    local expected=$2
    local description
    if ! command -v file >/dev/null 2>&1; then
        return
    fi
    description=$(file -b -- "$path")
    case "$expected" in
        x64)
            if [[ "$description" != *PE32+* || "$description" != *x86-64* ]]; then
                printf 'Expected x64 PE DLL, got %s: %s\n' "$description" "$path" >&2
                exit 1
            fi
            ;;
        x86)
            if [[ "$description" != *PE32* || "$description" == *PE32+* || \
                  ( "$description" != *80386* && "$description" != *"Intel i386"* ) ]]; then
                printf 'Expected x86 PE DLL, got %s: %s\n' "$description" "$path" >&2
                exit 1
            fi
            ;;
    esac
}

backup_current() {
    local label=$1
    local destination="$backup_root/$label-$(date -u +%Y%m%dT%H%M%SZ)"
    local dll
    if [[ -e "$destination" ]]; then
        printf 'Backup destination already exists: %s\n' "$destination" >&2
        exit 1
    fi
    mkdir -p -- "$destination/system32" "$destination/syswow64"
    for dll in d3d12.dll d3d12core.dll; do
        if [[ ! -f "$system32/$dll" || ! -f "$syswow64/$dll" ]]; then
            printf 'Required prefix DLL is missing: %s\n' "$dll" >&2
            exit 1
        fi
        cp -a -- "$system32/$dll" "$destination/system32/$dll"
        cp -a -- "$syswow64/$dll" "$destination/syswow64/$dll"
    done
    (
        cd -- "$destination"
        sha256sum system32/d3d12.dll system32/d3d12core.dll \
            syswow64/d3d12.dll syswow64/d3d12core.dll >originals.sha256
    )
    {
        printf 'created_utc=%s\n' "$(date -u --iso-8601=seconds)"
        printf 'app_id=%s\n' "$app_id"
        printf 'prefix=%s\n' "$compat_dir"
        printf 'purpose=%s\n' "$label"
    } >"$destination/metadata.txt"
    printf '%s\n' "$destination"
}

atomic_copy() {
    local source=$1
    local destination=$2
    local temporary="${destination}.il2-new.$$"
    cp -a -- "$source" "$temporary"
    mv -f -- "$temporary" "$destination"
}

if [[ "$mode" == "install" ]]; then
    if [[ -z "$build_dir" || ! -d "$build_dir" ]]; then
        printf 'install requires --build-dir pointing to package-release output.\n' >&2
        exit 2
    fi
    build_dir=$(readlink -f -- "$build_dir")
    for source in \
        "$build_dir/x64/d3d12.dll" "$build_dir/x64/d3d12core.dll" \
        "$build_dir/x86/d3d12.dll" "$build_dir/x86/d3d12core.dll"; do
        if [[ ! -f "$source" ]]; then
            printf 'Expected build artifact not found: %s\n' "$source" >&2
            exit 1
        fi
    done
    validate_architecture "$build_dir/x64/d3d12.dll" x64
    validate_architecture "$build_dir/x64/d3d12core.dll" x64
    validate_architecture "$build_dir/x86/d3d12.dll" x86
    validate_architecture "$build_dir/x86/d3d12core.dll" x86

    original_backup=$(backup_current vkd3d-dll-backup)
    printf 'Original DLL backup: %s\n' "$original_backup"
    {
        printf 'build_dir=%s\n' "$build_dir"
        if [[ -f "$build_dir/build.64/vkd3d_build.h" ]]; then
            printf 'vkd3d_build_header='
            sed -n 's/.*vkd3d_build = \([^;]*\);/\1/p' "$build_dir/build.64/vkd3d_build.h"
        fi
        sha256sum "$build_dir/x64/d3d12.dll" "$build_dir/x64/d3d12core.dll" \
            "$build_dir/x86/d3d12.dll" "$build_dir/x86/d3d12core.dll"
    } >>"$original_backup/metadata.txt"

    rollback_needed=1
    rollback_install() {
        if [[ ${rollback_needed:-0} -eq 1 ]]; then
            printf 'Install interrupted; restoring original DLLs from %s\n' "$original_backup" >&2
            for dll in d3d12.dll d3d12core.dll; do
                cp -a -- "$original_backup/system32/$dll" "$system32/$dll"
                cp -a -- "$original_backup/syswow64/$dll" "$syswow64/$dll"
            done
        fi
    }
    trap rollback_install EXIT INT TERM

    atomic_copy "$build_dir/x64/d3d12.dll" "$system32/d3d12.dll"
    atomic_copy "$build_dir/x64/d3d12core.dll" "$system32/d3d12core.dll"
    atomic_copy "$build_dir/x86/d3d12.dll" "$syswow64/d3d12.dll"
    atomic_copy "$build_dir/x86/d3d12core.dll" "$syswow64/d3d12core.dll"
    rollback_needed=0
    trap - EXIT INT TERM

    printf '\nInstalled custom VKD3D-Proton DLLs only into %s\n' "$compat_dir"
    sha256sum "$system32/d3d12.dll" "$system32/d3d12core.dll" \
        "$syswow64/d3d12.dll" "$syswow64/d3d12core.dll"
    printf 'Rollback with:\n  %q restore --backup-dir %q --yes\n' "$0" "$original_backup"
    printf 'WARNING: Stock Proton will replace these prefix DLLs during launch.\n'
    printf 'Use scripts/create-custom-proton.sh for a durable Steam runtime test.\n'
else
    if [[ -z "$backup_dir" || ! -d "$backup_dir" ]]; then
        printf 'restore requires --backup-dir from a prior install.\n' >&2
        exit 2
    fi
    backup_dir=$(readlink -f -- "$backup_dir")
    for source in \
        "$backup_dir/system32/d3d12.dll" "$backup_dir/system32/d3d12core.dll" \
        "$backup_dir/syswow64/d3d12.dll" "$backup_dir/syswow64/d3d12core.dll"; do
        if [[ ! -f "$source" ]]; then
            printf 'Backup artifact is missing: %s\n' "$source" >&2
            exit 1
        fi
    done
    if [[ -f "$backup_dir/originals.sha256" ]]; then
        (
            cd -- "$backup_dir"
            sha256sum -c originals.sha256
        )
    fi
    validate_architecture "$backup_dir/system32/d3d12.dll" x64
    validate_architecture "$backup_dir/system32/d3d12core.dll" x64
    validate_architecture "$backup_dir/syswow64/d3d12.dll" x86
    validate_architecture "$backup_dir/syswow64/d3d12core.dll" x86

    displaced_backup=$(backup_current pre-vkd3d-restore)
    atomic_copy "$backup_dir/system32/d3d12.dll" "$system32/d3d12.dll"
    atomic_copy "$backup_dir/system32/d3d12core.dll" "$system32/d3d12core.dll"
    atomic_copy "$backup_dir/syswow64/d3d12.dll" "$syswow64/d3d12.dll"
    atomic_copy "$backup_dir/syswow64/d3d12core.dll" "$syswow64/d3d12core.dll"

    printf '\nRestored VKD3D-Proton DLLs from %s\n' "$backup_dir"
    printf 'The displaced DLLs remain recoverable at %s\n' "$displaced_backup"
fi
