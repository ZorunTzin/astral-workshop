# Astral Workshop Launcher

Repository and releases: https://github.com/ZorunTzin/astral-workshop

## Players

1. Install `AstralWorkshopSetup.exe` (Windows x64). The .NET runtime is included.
2. Open Settings > Preferences and select your Diablo II: Resurrected folder.
3. D2R.exe and D2RLoader.exe must already be installed there. The base game and loader runtime are not distributed by this installer.
4. Press Play. The launcher checks the latest published GitHub release, validates the mod files, installs an update when required, then starts `D2RLoader.exe -mod ReimaginedWorkshop -txt`.

A failed/cancelled update prevents game startup. There is currently no offline bypass. Settings > Preferences > Check and repair performs the same verification without starting the game.

Existing `d2rloader/config` files and unmanaged mod files are preserved. Character saves are outside the update target. Every update retains the previous complete mod folder as `Mods/.astral-backup-<id>` for recovery; these backups can be removed manually once the new version is verified. Allow space for the download, staged installation and previous installation.

The launcher installs per user in `%LOCALAPPDATA%/Programs/AstralWorkshop`. Its preferences are under `%LOCALAPPDATA%/AstralWorkshop`. The Start menu shortcut and optional desktop shortcut open the launcher. Windows Installed Apps provides the uninstaller; removing the launcher retains the game, mod and saves.

## Release maintainers

Run `scripts/Build-Launcher.ps1` with .NET SDK 10 on Windows. It runs the isolated updater tests and produces the self-contained launcher ZIP and setup executable under `build/launcher/`.

Run `python scripts/Package-LauncherRelease.py <version>` to build the mod ZIP, manifest and corresponding source archive. Before releasing, run the existing Workshop build/check/deploy checks and inspect the payload. The packaging script omits disabled plugins and intermediate build files.

Publish a GitHub Release in ZorunTzin/astral-workshop with these assets:

- `workshop-manifest.json`
- `workshop-mod.zip`
- `workshop-source.zip` (corresponding source and notices)
- `AstralWorkshopSetup.exe`
- `AstralWorkshop.zip`

Publish all assets together; mark the tested release as latest only after uploads finish. The updater uses HTTPS release assets and SHA-256 verification. It does not use a separately signed manifest. Access to the GitHub release account is therefore the trust boundary.

Launcher updates currently use the new setup executable; automatic updates on Play apply to the mod. The installer and executables are currently unsigned.

## Validation

`dotnet run --project launcher-tests/Launcher.Tests.csproj -c Release`

Tests cover traversal, duplicate paths, corrupt/missing payload entries, configuration preservation, unmanaged files, cancellation and removal of obsolete managed files. Staging does not modify the live installation. The final directory swap restores the old installation if moving the new directory fails.

The launcher uses the project's approved astral background and its own WPF implementation; no third-party launcher artwork is included. Original project and dependency licenses remain applicable.
