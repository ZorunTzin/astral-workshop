# Astral Workshop

Offline Diablo II: Resurrected modding project based on D2R Reimagined.
The game-ready mod payload remains at the repository root. Development files are separated below.

## Current release

Version **0.26.0**: [release notes](docs/RELEASE-0.26.0.md), [Mythical mechanics](src/workshop/mythicals/README.md), [64-variant catalog](src/workshop/mythicals/CATALOG.md) and [live compendium](https://astral-workshop-compendium.chattyactor2.chatgpt.site).

Uses the separate **ReimaginedWorkshop-v2** save profile; do not copy legacy saves into it.

## Editable tables

- [Astral skilltree](tables/Astral-Skilltree-0.9.5.xlsx)
- [Runeword forging](tables/Runenwoerter-Schmieden-Entwurf.xlsx)

Edit the workbooks and return the changed file for import. Exporting the skilltree workbook again replaces it with the current generated values.

## Folder layout

| Folder | Contents |
| --- | --- |
| `data/` | Game data, localization and visual assets. |
| `d2rloader/` | Runtime plugins, plugin configuration and required licenses. |
| `docs/` | Feature documentation, development guide and upstream project information. |
| `tables/` | Editable Excel files and preserved design drafts. |
| `src/workshop/` | Native plugin source, generators, balance definitions, tests and pinned dependencies. |
| `scripts/` | Supported build, installation, deployment and spreadsheet entry points. |
| `tools/lint/` | Upstream table-validation utility. |
| `build/` | Generated staging mod, binaries, previews, reports. Ignored by Git. |
| `archive/` | Superseded tree implementations, historical tools and old workspaces. Large local archives are ignored by Git. |

`modinfo.json`, `bank.txt`, `Play Workshop.cmd`, `data/` and `d2rloader/` retain their loader paths.
Use [Play Workshop.cmd](Play%20Workshop.cmd) to start the already installed local Workshop.

## Development

Run these commands from the repository root:

```powershell
.\scripts\Workshop.ps1 -Action Generate
.\scripts\Workshop.ps1 -Action Build
.\scripts\Workshop.ps1 -Action Check
.\scripts\Workshop.ps1 -Action CheckDeploy
```

`Generate` rebuilds a clean staging mod from the canonical sources. `Build` compiles the plugins and runs native tests. `Check` validates the generated data. `CheckDeploy` validates the deployment plan without writing files.

To install a tested update, close D2R and run `-Action Deploy`. It backs up changed files under `archive/local/backups/workshop/`. A fresh installation can use [Install-Workshop.ps1](scripts/Install-Workshop.ps1).

- [Gameplay and feature notes](docs/workshop/README.md)
- [Detailed structure and workflow](docs/FOLDER-STRUCTURE.md)
- [Original Reimagined project information](docs/UPSTREAM-README.md)
