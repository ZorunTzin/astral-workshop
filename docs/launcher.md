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

## Launcher 0.2.0

Only one launcher instance runs per Windows session; opening it again restores the existing window. Game launch is refused while D2R or D2RLoader is running. UI failures are reported and logged to `%LOCALAPPDATA%/AstralWorkshop/launcher.log`. Open saved characters resolves Windows Saved Games (including redirection) and the installed modinfo.json savepath. It creates the correct empty profile directory if no character has been saved yet.

Monster display is bundled and checks the shipped Workshop bank.txt; a separate Frankenstein installation is no longer required. Supported loader/core identity checks remain active.

## Launcher 0.3.0 — monster density

Settings > Game > Monster density selects x1 (default) through x5. The preference is saved, including across launcher restarts, and applied after a successful Play or Check and repair. Restart the game for a new selection.

The multiplier scales MonDen, MonDen(N) and MonDen(H) in both levels.txt variants from the release originals. Town/Trial zero densities stay zero; unique-pack counts, bosses and scripted League groups are unchanged. Population placement constraints can affect the actual monster count.

The updater retains checksum-verified originals under the mod's .astral-density directory. It accepts only the exact selected transformation during integrity checks, so density does not trigger repeat downloads. Switching factors never compounds; x1 restores the original bytes. New releases replace the baseline before applying the selected factor. Invalid settings default to x1; malformed tables and density values above 10000 are rejected before launch.

Validation: 63 updater/settings/density checks, including the current two game tables at every factor, plus Windows launcher and installer builds. Actual in-game population counts have not been manually measured.
