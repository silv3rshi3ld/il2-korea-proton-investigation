# Community package bootstrap: standalone non-Steam shortcut

Date: 2026-08-13

Status: external, incomplete; the game process was not created

## Scope and source

This note records the first community-supplied log from installing the
published three-fix Proton package against a standalone IL-2 installation
added to Steam as a non-Steam shortcut. The community member supplied a Proton
log excerpt in a support conversation; no raw log file is redistributed here.
The home-directory component is normalized to `/home/USER`.

This is installation and bootstrap evidence. It is not an independent runtime
confirmation of startup, terrain, map, or lighting behavior in
`IL2Series.exe`.

## Reported environment

- Compatibility tool:
  `IL2-Korea-Proton-Three-Fixes-20260813`
- Proton base:
  `experimental-11.0-20260724c-wine-mr11604-d10`
- Steam Linux Runtime: `steamrt4`, depot `4.0.20260805.254769`
- Kernel: CachyOS `7.1.8-1-cachyos`
- GPU: AMD Radeon RX 9070 XT, RADV GFX1201
- Mesa/RADV version reported by Vulkan: `26.1.6`
- Shortcut command: `/home/USER/Games/IL2Series/Launcher.exe`
- SteamGameId: generated non-Steam shortcut identifier, not AppID `247970`

## Bootstrap observations

The final log shows that:

- Steam selected the intended custom compatibility-tool directory;
- Steam Linux Runtime 4.0 and the custom Proton build started;
- ntsync initialized;
- the Proton accessibility helper `xalia.exe` loaded DXVK `v3.0.2`;
- Vulkan enumerated the RX 9070 XT through RADV; and
- Wine's built-in Steam launcher reached the attempt to create the configured
  shortcut target.

The DXVK and Vulkan observations belong to `xalia.exe`, not `IL2Series.exe`.
They therefore demonstrate a functioning package/runtime bootstrap and GPU
enumeration, but do not establish that the game's D3D12 path initialized.

## Decisive failure

The relevant terminal error was:

```text
err:steam:run_process Failed to create process L"\"Z:\\home\\USER\\Games\\IL2Series\\Launcher.exe\"": 2
```

Windows error `2` is file not found. The configured shortcut target did not
exist at that exact path, was named with different capitalization, or was
otherwise not addressable at the configured location. The failure occurred
before the launcher or game process began.

The preceding line:

```text
err:steamclient:steamclient_init_registry Failed to connect to Steam
```

does not supersede the concrete create-process failure. It is not classified
as universally harmless, but in this run it cannot explain an in-game failure
because no game process existed. Steam integration can be evaluated after the
shortcut target is corrected and launches.

## Installation-recovery observations

The support session also exposed two packaging-documentation traps before the
final log was collected:

1. An incomplete extraction contained the compatibility-tool manifest but not
   all required entry-point files.
2. Renaming that incomplete directory while leaving it under
   `compatibilitytools.d` created a second discoverable manifest. Steam could
   still enumerate and cache the wrong copy.

The reliable recovery is to move all incomplete or backup copies completely
outside `compatibilitytools.d`, retain exactly one complete tool directory,
restart Steam, and verify `proton`, `compatibilitytool.vdf`,
`toolmanifest.vdf`, and `version` before testing. Validation instructions
should use complete literal paths or redefine variables in the current shell;
a convenience variable from an earlier Fish session has no meaning to Steam.

## Evidence accounting and next step

Record this attempt as:

> Package bootstrap passed on RX 9070 XT/RADV; standalone shortcut blocked
> before process creation by an invalid `Launcher.exe` target.

Do not count it as cross-hardware validation of the three compatibility fixes.
A qualifying follow-up must correct the shortcut target, show
`IL2Series.exe` starting, and separately report the menu, terrain, map,
lighting blocks, and ordinary shadows.
