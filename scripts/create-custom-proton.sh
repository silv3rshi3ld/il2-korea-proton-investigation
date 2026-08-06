#!/usr/bin/env bash
set -euo pipefail

source_tool=""
build_dir=""
destination=""
tool_name=""
confirmed=0

usage() {
    printf '%s\n' \
        "Usage:" \
        "  $0 --source-tool PATH --build-dir PATH --destination PATH --tool-name NAME --yes" \
        "" \
        "Creates a copy-on-write custom Proton tool and replaces only its packaged" \
        "x64/x86 d3d12.dll and d3d12core.dll files. The source tool and game prefix" \
        "are not modified. Steam must be fully stopped."
}

while (($#)); do
    case "$1" in
        --source-tool)
            source_tool="${2:?--source-tool requires a path}"
            shift 2
            ;;
        --build-dir)
            build_dir="${2:?--build-dir requires a path}"
            shift 2
            ;;
        --destination)
            destination="${2:?--destination requires a path}"
            shift 2
            ;;
        --tool-name)
            tool_name="${2:?--tool-name requires a value}"
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

if [[ $confirmed -ne 1 ]]; then
    printf 'Re-run with --yes after reviewing the source and destination paths.\n' >&2
    exit 2
fi
if [[ ! "$tool_name" =~ ^[A-Za-z0-9][A-Za-z0-9._-]*$ ]]; then
    printf 'Invalid tool name %q; use letters, digits, dot, underscore, or hyphen.\n' "$tool_name" >&2
    exit 2
fi
if [[ -z "$source_tool" || ! -d "$source_tool" ]]; then
    printf 'A valid --source-tool directory is required.\n' >&2
    exit 2
fi
if [[ -z "$build_dir" || ! -d "$build_dir" ]]; then
    printf 'A valid --build-dir is required.\n' >&2
    exit 2
fi
if [[ -z "$destination" ]]; then
    printf -- '--destination is required.\n' >&2
    exit 2
fi

source_tool=$(readlink -f -- "$source_tool")
build_dir=$(readlink -f -- "$build_dir")
destination_parent=$(dirname -- "$destination")
mkdir -p -- "$destination_parent"
destination_parent=$(readlink -f -- "$destination_parent")
destination="$destination_parent/$(basename -- "$destination")"

expected_parent=$(readlink -f -- "${HOME:?HOME is required}/.local/share/Steam/compatibilitytools.d")
if [[ "$destination_parent" != "$expected_parent" ]]; then
    printf 'Refusing destination outside the Steam custom-tools directory: %s\n' "$destination" >&2
    exit 1
fi
if [[ "$(basename -- "$destination")" != "$tool_name" ]]; then
    printf 'Destination basename must match tool name: %s != %s\n' \
        "$(basename -- "$destination")" "$tool_name" >&2
    exit 1
fi
if [[ -e "$destination" ]]; then
    printf 'Destination already exists; refusing to overwrite it: %s\n' "$destination" >&2
    exit 1
fi

if pgrep -ax steam >/dev/null 2>&1; then
    printf 'Exit Steam completely before creating a custom compatibility tool.\n' >&2
    exit 1
fi
if pgrep -af 'IL2Series\.exe|SteamLaunch AppId=247970|proton.*247970' >/dev/null 2>&1; then
    printf 'Exit IL-2 and its Proton processes before continuing.\n' >&2
    exit 1
fi

for required in proton toolmanifest.vdf \
        files/lib/wine/vkd3d-proton/x86_64-windows/d3d12.dll \
        files/lib/wine/vkd3d-proton/x86_64-windows/d3d12core.dll \
        files/lib/wine/vkd3d-proton/i386-windows/d3d12.dll \
        files/lib/wine/vkd3d-proton/i386-windows/d3d12core.dll; do
    if [[ ! -f "$source_tool/$required" ]]; then
        printf 'Required source-tool file is missing: %s\n' "$source_tool/$required" >&2
        exit 1
    fi
done
for required in x64/d3d12.dll x64/d3d12core.dll x86/d3d12.dll x86/d3d12core.dll; do
    if [[ ! -f "$build_dir/$required" ]]; then
        printf 'Required diagnostic DLL is missing: %s\n' "$build_dir/$required" >&2
        exit 1
    fi
done

staging=$(mktemp -d -- "$destination_parent/.${tool_name}.new.XXXXXXXX")
cleanup() {
    if [[ -n ${staging:-} && -d "$staging" ]]; then
        rm -rf -- "$staging"
    fi
}
trap cleanup EXIT INT TERM

cp -a --reflink=auto -- "$source_tool/." "$staging/"

install -m 0644 -- "$build_dir/x64/d3d12.dll" \
    "$staging/files/lib/wine/vkd3d-proton/x86_64-windows/d3d12.dll"
install -m 0644 -- "$build_dir/x64/d3d12core.dll" \
    "$staging/files/lib/wine/vkd3d-proton/x86_64-windows/d3d12core.dll"
install -m 0644 -- "$build_dir/x86/d3d12.dll" \
    "$staging/files/lib/wine/vkd3d-proton/i386-windows/d3d12.dll"
install -m 0644 -- "$build_dir/x86/d3d12core.dll" \
    "$staging/files/lib/wine/vkd3d-proton/i386-windows/d3d12core.dll"

{
    printf '"compatibilitytools"\n'
    printf '{\n'
    printf '  "compat_tools"\n'
    printf '  {\n'
    printf '    "%s"\n' "$tool_name"
    printf '    {\n'
    printf '      "install_path" "."\n'
    printf '      "display_name" "%s"\n' "$tool_name"
    printf '      "from_oslist" "windows"\n'
    printf '      "to_oslist" "linux"\n'
    printf '    }\n'
    printf '  }\n'
    printf '}\n'
} >"$staging/compatibilitytool.vdf"

{
    printf 'created_utc=%s\n' "$(date -u --iso-8601=seconds)"
    printf 'tool_name=%s\n' "$tool_name"
    printf 'source_tool=%s\n' "$source_tool"
    printf 'source_version='; tr '\n' ' ' <"$source_tool/version"; printf '\n'
    printf 'diagnostic_build=%s\n' "$build_dir"
    printf 'diagnostic_dll_sha256:\n'
    (
        cd -- "$staging"
        sha256sum \
            files/lib/wine/vkd3d-proton/x86_64-windows/d3d12.dll \
            files/lib/wine/vkd3d-proton/x86_64-windows/d3d12core.dll \
            files/lib/wine/vkd3d-proton/i386-windows/d3d12.dll \
            files/lib/wine/vkd3d-proton/i386-windows/d3d12core.dll
    )
} >"$staging/il2-korea-diagnostic-metadata.txt"

cmp --silent "$build_dir/x64/d3d12.dll" \
    "$staging/files/lib/wine/vkd3d-proton/x86_64-windows/d3d12.dll"
cmp --silent "$build_dir/x64/d3d12core.dll" \
    "$staging/files/lib/wine/vkd3d-proton/x86_64-windows/d3d12core.dll"
cmp --silent "$build_dir/x86/d3d12.dll" \
    "$staging/files/lib/wine/vkd3d-proton/i386-windows/d3d12.dll"
cmp --silent "$build_dir/x86/d3d12core.dll" \
    "$staging/files/lib/wine/vkd3d-proton/i386-windows/d3d12core.dll"

mv -- "$staging" "$destination"
staging=""
trap - EXIT INT TERM

printf 'Created custom Proton tool without modifying the source tool or prefix:\n  %s\n' "$destination"
printf 'Restart Steam, then select %s for AppID 247970.\n' "$tool_name"
printf 'To roll back, select Proton Experimental again; do not delete the custom tool while Steam is running.\n'
