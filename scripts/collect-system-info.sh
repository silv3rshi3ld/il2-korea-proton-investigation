#!/usr/bin/env bash
set -euo pipefail

app_id="247970"
output_path=""
steam_root_override=""

usage() {
    printf '%s\n' \
        "Usage: $0 [--output PATH|-] [--steam-root PATH] [--app-id ID]" \
        "" \
        "Collects a read-only, credential-free system/Steam/Proton baseline." \
        "The default output is captures/system-info-<UTC timestamp>.txt."
}

while (($#)); do
    case "$1" in
        --output)
            output_path="${2:?--output requires a path}"
            shift 2
            ;;
        --steam-root)
            steam_root_override="${2:?--steam-root requires a path}"
            shift 2
            ;;
        --app-id)
            app_id="${2:?--app-id requires an ID}"
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

if [[ ! "$app_id" =~ ^[0-9]+$ ]]; then
    printf 'Invalid AppID: %s\n' "$app_id" >&2
    exit 2
fi

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)
project_dir=$(cd -- "$script_dir/.." && pwd -P)
timestamp=$(date -u +%Y%m%dT%H%M%SZ)

if [[ -z "$output_path" ]]; then
    output_path="$project_dir/captures/system-info-$timestamp.txt"
fi

if [[ "$output_path" != "-" ]]; then
    mkdir -p -- "$(dirname -- "$output_path")"
    exec 3>&1
    exec >"$output_path" 2>&1
fi

section() {
    printf '\n## %s\n' "$1"
}

have() {
    command -v "$1" >/dev/null 2>&1
}

acf_value() {
    local file=$1
    local key=$2
    awk -F'"' -v wanted="$key" '$2 == wanted { print $4; exit }' "$file"
}

find_steam_root() {
    local candidate
    if [[ -n "$steam_root_override" ]]; then
        candidate=$steam_root_override
        if [[ -d "$candidate/steamapps" ]]; then
            readlink -f -- "$candidate"
            return 0
        fi
        return 1
    fi

    for candidate in \
        "${STEAM_ROOT:-}" \
        "${HOME:-}/.local/share/Steam" \
        "${HOME:-}/.steam/root" \
        "${HOME:-}/.steam/steam"; do
        if [[ -n "$candidate" && -d "$candidate/steamapps" ]]; then
            readlink -f -- "$candidate"
            return 0
        fi
    done
    return 1
}

find_manifest() {
    local root=$1
    local manifest="$root/steamapps/appmanifest_${app_id}.acf"
    local library_vdf="$root/steamapps/libraryfolders.vdf"
    local library

    if [[ -f "$manifest" ]]; then
        printf '%s\n' "$manifest"
        return 0
    fi

    if [[ -f "$library_vdf" ]]; then
        while IFS= read -r library; do
            manifest="$library/steamapps/appmanifest_${app_id}.acf"
            if [[ -f "$manifest" ]]; then
                printf '%s\n' "$manifest"
                return 0
            fi
        done < <(awk -F'"' '$2 == "path" { gsub(/\\\\/, "/", $4); print $4 }' "$library_vdf")
    fi
    return 1
}

steam_root=""
manifest=""
library_root=""
install_dir=""
game_dir=""
compat_dir=""
proton_root=""

if steam_root=$(find_steam_root); then
    if manifest=$(find_manifest "$steam_root"); then
        library_root=${manifest%/steamapps/appmanifest_*}
        install_dir=$(acf_value "$manifest" installdir)
        game_dir="$library_root/steamapps/common/$install_dir"
        compat_dir="$library_root/steamapps/compatdata/$app_id"
        if [[ -f "$compat_dir/config_info" ]]; then
            while IFS= read -r config_line; do
                if [[ "$config_line" == */files/share/fonts/ ]]; then
                    proton_root=${config_line%/files/share/fonts/}
                    break
                fi
            done <"$compat_dir/config_info"
        fi
    fi
fi

section "Collection metadata"
printf 'UTC timestamp: %s\n' "$(date -u --iso-8601=seconds)"
printf 'Hostname: %s\n' "$(hostname 2>/dev/null || printf unknown)"
printf 'AppID: %s\n' "$app_id"
printf 'Collector: %s\n' "$0"

section "Operating system"
uname -a
if [[ -r /etc/os-release ]]; then
    sed -n '1,80p' /etc/os-release
fi
if have loginctl && [[ -n "${XDG_SESSION_ID:-}" ]]; then
    loginctl show-session "$XDG_SESSION_ID" -p Type -p Desktop -p Remote 2>/dev/null || true
fi
printf 'XDG_SESSION_TYPE=%s\n' "${XDG_SESSION_TYPE:-unset}"
printf 'WAYLAND_DISPLAY=%s\n' "${WAYLAND_DISPLAY:-unset}"
printf 'DISPLAY=%s\n' "${DISPLAY:-unset}"

section "CPU and NUMA"
if have lscpu; then
    lscpu
fi
if have numactl; then
    numactl --hardware || true
fi

section "Memory and kernel settings"
if have free; then
    free -h
fi
if have sysctl; then
    sysctl vm.max_map_count 2>/dev/null || true
fi

section "GPU, PCI resources, and kernel driver"
gpu_slot=""
if have lspci; then
    lspci -Dnnk
    gpu_slot=$(lspci -Dnn | awk '
        /VGA compatible controller.*(AMD|ATI).*747e/ && first == "" { first = $1 }
        END { if (first != "") print first }
    ')
    if [[ -z "$gpu_slot" ]]; then
        gpu_slot=$(lspci -Dnn | awk '
            /VGA compatible controller.*(AMD|ATI)/ && first == "" { first = $1 }
            END { if (first != "") print first }
        ')
    fi
    if [[ -n "$gpu_slot" ]]; then
        printf '\nDetailed device information for %s:\n' "$gpu_slot"
        lspci -vv -s "$gpu_slot" 2>&1 || true
    fi
fi

section "Installed graphics packages"
if have pacman; then
    pacman -Q mesa lib32-mesa vulkan-radeon lib32-vulkan-radeon \
        linux-cachyos vulkan-tools 2>&1 || true
elif have dpkg-query; then
    dpkg-query -W mesa-vulkan-drivers libvulkan1 vulkan-tools 2>&1 || true
elif have rpm; then
    rpm -qa | sort | awk 'tolower($0) ~ /(mesa|vulkan|amdgpu)/'
fi

section "Vulkan summary"
if have vulkaninfo; then
    vulkan_output=$(vulkaninfo 2>&1 || true)
    printf '%s\n' "$vulkan_output" | sed -n '1,220p'

    section "Relevant Vulkan extensions"
    printf '%s\n' "$vulkan_output" | awk '
        /^[[:space:]]*VK_(EXT_descriptor_buffer|EXT_descriptor_heap|EXT_graphics_pipeline_library|EXT_host_image_copy|EXT_image_compression_control|EXT_image_view_min_lod|EXT_memory_budget|EXT_memory_priority|EXT_pageable_device_local_memory|KHR_buffer_device_address|KHR_maintenance[0-9]+|KHR_present_id|KHR_present_wait|MESA_image_alignment_control|VALVE_mutable_descriptor_type)[[:space:]]/ { print }
    ' | sort -u

    section "First Vulkan device queue families"
    printf '%s\n' "$vulkan_output" | awk '
        /^VkQueueFamilyProperties:/ { blocks++; if (blocks == 1) printing = 1 }
        printing { print }
        printing && /^VkPhysicalDeviceMemoryProperties:/ { printing = 0 }
    '

    section "Sparse Vulkan features"
    printf '%s\n' "$vulkan_output" | awk '
        /^[[:space:]]*(sparseBinding|sparseResidency|residencyStandard|residencyNonResidentStrict|bufferImageGranularity)/ { print }
    ' | sort -u
else
    printf 'vulkaninfo not installed\n'
fi

section "Steam and game metadata"
if [[ -z "$steam_root" ]]; then
    printf 'Steam root not found.\n'
else
    printf 'Steam root: %s\n' "$steam_root"
fi
if [[ -z "$manifest" ]]; then
    printf 'Manifest for AppID %s not found.\n' "$app_id"
else
    printf 'Manifest: %s\n' "$manifest"
    printf 'Name: %s\n' "$(acf_value "$manifest" name)"
    printf 'Install directory: %s\n' "$install_dir"
    printf 'Game directory: %s\n' "$game_dir"
    printf 'Build ID: %s\n' "$(acf_value "$manifest" buildid)"
    printf 'Size on disk: %s\n' "$(acf_value "$manifest" SizeOnDisk)"
    printf 'Prefix/compatdata: %s\n' "$compat_dir"
fi

section "Selected Proton metadata"
if [[ -n "$compat_dir" && -f "$compat_dir/version" ]]; then
    printf 'Prefix version: '
    sed -n '1p' "$compat_dir/version"
fi
if [[ -n "$proton_root" && -d "$proton_root" ]]; then
    printf 'Proton root: %s\n' "$proton_root"
    if [[ -f "$proton_root/version" ]]; then
        printf 'Proton version: '
        sed -n '1p' "$proton_root/version"
    fi
    for version_file in \
        "$proton_root/files/lib/wine/vkd3d-proton/version" \
        "$proton_root/files/lib/wine/dxvk/version" \
        "$proton_root/files/lib/vkd3d/version"; do
        if [[ -f "$version_file" ]]; then
            printf '%s: ' "$version_file"
            sed -n '1p' "$version_file"
        fi
    done
else
    printf 'Selected Proton root could not be derived from config_info.\n'
fi

section "Executable and D3D backend"
game_exe="$game_dir/bin/game/IL2Series.exe"
backend_dll="$game_dir/bin/game/dxBackend12.dll"
for pe_file in "$game_exe" "$backend_dll"; do
    if [[ -f "$pe_file" ]]; then
        file "$pe_file"
        if have objdump; then
            printf 'Imports for %s:\n' "$pe_file"
            objdump -p "$pe_file" 2>/dev/null | awk '
                /DLL Name:/ || tolower($0) ~ /(d3d12|d3d11|dxgi|vulkan|nvapi)/ { print }
            ' | sed -n '1,240p'
        fi
    fi
done

printf 'Game-local graphics overrides:\n'
if [[ -d "$game_dir" ]]; then
    find "$game_dir" -type f \
        \( -iname 'd3d12*.dll' -o -iname 'd3d11.dll' -o -iname 'dxgi.dll' \
        -o -iname 'dxvk*.dll' -o -iname 'vkd3d*.dll' \) \
        -printf '%P\n' 2>/dev/null | sort
fi

section "Prefix D3D DLL provenance"
if [[ -d "$compat_dir/pfx" ]]; then
    for dll in d3d12.dll d3d12core.dll dxgi.dll d3d11.dll; do
        prefix_dll="$compat_dir/pfx/drive_c/windows/system32/$dll"
        if [[ -e "$prefix_dll" || -L "$prefix_dll" ]]; then
            file "$prefix_dll"
            sha256sum "$prefix_dll"
        fi
    done
fi

if [[ -n "$proton_root" && -d "$proton_root/files/lib/wine" ]]; then
    printf '\nMatching provider payloads:\n'
    for provider_dll in \
        "$proton_root/files/lib/wine/vkd3d-proton/x86_64-windows/d3d12.dll" \
        "$proton_root/files/lib/wine/vkd3d-proton/x86_64-windows/d3d12core.dll" \
        "$proton_root/files/lib/wine/dxvk/x86_64-windows/dxgi.dll" \
        "$proton_root/files/lib/wine/dxvk/x86_64-windows/d3d11.dll"; do
        if [[ -f "$provider_dll" ]]; then
            sha256sum "$provider_dll"
        fi
    done
fi

section "Display outputs"
if have kscreen-doctor; then
    kscreen-doctor -o 2>&1 || true
elif have xrandr; then
    xrandr --current 2>&1 || true
else
    printf 'No supported display query tool found.\n'
fi

section "Collector notes"
printf '%s\n' \
    'This report does not inspect Steam config.vdf/localconfig.vdf and therefore does not collect tokens, cloud keys, or depot decryption keys.' \
    'Vulkan queue-family availability does not prove which queues VKD3D selected for the game; use a controlled Proton log.'

if [[ "$output_path" != "-" ]]; then
    printf 'Wrote %s\n' "$output_path" >&3
    exec 3>&-
fi
