# Folder structure and development workflow

## Game payload

Only the existing loader-facing files stay at the root: `data/`, `d2rloader/`, `modinfo.json`, `bank.txt` and the local launcher. This reorganization does not change gameplay or overwrite the installed game mod.

## Authoritative source

`src/workshop/` contains the plugin and generator source. Python modules, generated C++ includes and their balance inputs stay together because the generators use local imports and native headers. `combat/`, `integration/` and `SDK/` preserve their internal layout and byte-pinned upstream inputs.

Important editable definitions:

- `src/workshop/NODES.csv`: stable cluster identities, underlying properties and Keystones.
- `src/workshop/NODES-step2.csv`: cluster labels and ordinary cluster values.
- `src/workshop/INNER-CLUSTERS.json`: inner cluster specializations.
- `src/workshop/CLUSTER-OVERRIDES.json`: multi-effect exceptions such as Swarm.
- `src/workshop/FORGING.json`: approved runeword bonuses by item rarity.

Human-editable workbooks are in `tables/`. The original user draft remains in `tables/drafts/` without content changes.

## Scripts

`scripts/Workshop.ps1` is the supported entry point. `Prepare` copies sources and feature documentation to `build/workshop-clusters/workshop/`. `Generate` first restores `tools/inputs/reimagined-base.zip`, then runs the generators against that staging mod. `Build` compiles the existing generated staging plugins and runs native tests. Run `Generate` after source or balance changes so generated bindings stay synchronized. Build launcher templates are versioned under `scripts/build/` and are restored by `Prepare`/`Generate`. `Check` validates the generated data.

`CheckDeploy` calls the deployment helper with `--check`: it validates source paths and the repository/game baseline without writing files. `Deploy` requires D2R to be closed and installs the tested staging files. Runtime assets keep their root paths, source outputs return to `src/workshop/`, and generated feature documents return to `docs/workshop/`.

The toolchain uses the existing Visual Studio 2019 Build Tools installation. Generated build files remain under `build/`, not among the authoritative sources.

`scripts/spreadsheets/skilltree/` contains the workbook collector and builder. Its JSON and preview images are under `build/spreadsheets/skilltree/`; its output workbook is under `tables/`. The local `node_modules` junction points to the bundled spreadsheet runtime and is ignored by Git. `ExportTable` deliberately overwrites the workbook from the current staging data, so preserve manual edits first.

## Archive

- `archive/legacy-tree/`: superseded tree layouts and earlier design files.
- `archive/upstream-tools/`: original environment-specific scripts retained for reference, including the old copy helper. They are not supported build entry points.
- `archive/legacy-reports/`: earlier linter reports.
- `archive/local/prototypes/`: historical Workshop snapshots.
- `archive/local/frankenstein/` and `archive/local/pd2/`: earlier mod work.
- `archive/local/backups/`: deployment and save backups.
- `archive/local/recovery/`: verified library recovery copy.
- `archive/local/migrations/`: previous workspace migration record.

The large `archive/local/` tree and all `build/` products are ignored by Git. Source, feature documentation, current workbooks and selected historical source files remain versioned.
