param(
    [ValidateSet('Prepare','Generate','Build','Test','Check','CheckDeploy','Deploy','ExportTable')]
    [string]$Action = 'Check'
)
$ErrorActionPreference = 'Stop'
$workshopRepository = Split-Path $PSScriptRoot
$workshopStage = Join-Path $workshopRepository 'build\workshop-clusters'
$workshopSource = Join-Path $workshopRepository 'src\workshop'
$workshopPython = (Get-Command python -ErrorAction Stop).Source
$previousOutput = $env:WORKSHOP_MOD_ROOT

function Invoke-WorkshopCommand([string]$Program, [string[]]$Arguments) {
    & $Program @Arguments
    if ($LASTEXITCODE -ne 0) { throw "Workshop command failed ($LASTEXITCODE): $Program $Arguments" }
}
function Copy-WorkshopSources {
    New-Item -ItemType Directory -Path (Join-Path $workshopStage 'workshop') -Force | Out-Null
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'build\compile.cmd'),(Join-Path $PSScriptRoot 'build\combat-build.cmd'),(Join-Path $PSScriptRoot 'build\test.cmd'),(Join-Path $PSScriptRoot 'build\test-093.cmd') -Destination $workshopStage -Force
    & robocopy $workshopSource (Join-Path $workshopStage 'workshop') /E /COPY:DAT /DCOPY:DAT /XD __pycache__ node_modules /XF *.obj *.res *.exp *.pdb /R:1 /W:1 /NFL /NDL /NJH /NJS /NP
    if ($LASTEXITCODE -ge 8) { throw 'Source staging failed.' }
    if (Test-Path (Join-Path $workshopStage 'data/global/excel/misc.txt')) {
        Invoke-WorkshopCommand $workshopPython @((Join-Path $workshopSource 'stash_organizer_catalog.py'),$workshopStage,(Join-Path $workshopStage 'workshop/stash_organizer_catalog.h'))
    }
    Invoke-WorkshopCommand $workshopPython @((Join-Path $workshopSource 'item_sort_ui.py'),$workshopStage)
    Invoke-WorkshopCommand $workshopPython @((Join-Path $workshopSource 'integration/mercenary.py'),$workshopStage)
    Invoke-WorkshopCommand $workshopPython @('-c',"import sys;sys.path.insert(0,r'$workshopSource');from monster_modifiers.data import install;install(r'$workshopStage')")
    Invoke-WorkshopCommand $workshopPython @((Join-Path $workshopSource 'goblins/data.py'),$workshopStage)
    Copy-Item -LiteralPath (Join-Path $workshopRepository 'modinfo.json') -Destination $workshopStage -Force
    Invoke-WorkshopCommand $workshopPython @((Join-Path $workshopSource 'item_limits.py'),$workshopStage)
    Invoke-WorkshopCommand $workshopPython @((Join-Path $workshopSource 'mythicals/data.py'),$workshopStage)
    Invoke-WorkshopCommand $workshopPython @((Join-Path $workshopSource 'progression/data.py'),$workshopStage)
    Invoke-WorkshopCommand $workshopPython @((Join-Path $workshopSource 'leagues/data.py'),$workshopStage)
    Invoke-WorkshopCommand $workshopPython @((Join-Path $workshopSource 'mercenary_skills.py'),$workshopStage)
    Invoke-WorkshopCommand $workshopPython @((Join-Path $workshopSource 'buff_hud.py'),$workshopStage)
    Invoke-WorkshopCommand $workshopPython @((Join-Path $workshopSource 'stash_organizer_catalog.py'),$workshopStage,(Join-Path $workshopStage 'workshop/stash_organizer_catalog.h'))
    Invoke-WorkshopCommand $workshopPython @((Join-Path $workshopSource 'endgame/refresh_panel.py'),$workshopStage)
    & robocopy (Join-Path $workshopRepository 'docs\workshop') (Join-Path $workshopStage 'workshop') *.md /S /COPY:DAT /R:1 /W:1 /NFL /NDL /NJH /NJS /NP
    if ($LASTEXITCODE -ge 8) { throw 'Documentation staging failed.' }
}
Push-Location $workshopRepository
try {
    $env:WORKSHOP_MOD_ROOT = $workshopStage
    switch ($Action) {
        'Prepare' { Copy-WorkshopSources }
        'Generate' {
            Copy-WorkshopSources
            Invoke-WorkshopCommand $workshopPython @('-m','zipfile','-e',(Join-Path $workshopRepository 'tools\inputs\reimagined-base.zip'),$workshopStage)
            Invoke-WorkshopCommand $workshopPython @('-X','utf8',(Join-Path $workshopStage 'workshop\generate.py'),$workshopStage)
        }
        'Build' {
            Invoke-WorkshopCommand 'cmd.exe' @('/c',(Join-Path $workshopStage 'compile.cmd'))
            Invoke-WorkshopCommand 'cmd.exe' @('/c',(Join-Path $workshopStage 'combat-build.cmd'))
            Invoke-WorkshopCommand 'cmd.exe' @('/c',(Join-Path $workshopStage 'test.cmd'))
            Invoke-WorkshopCommand 'cmd.exe' @('/c',(Join-Path $workshopStage 'test-093.cmd'))
        }
        'Test' {
            Invoke-WorkshopCommand 'cmd.exe' @('/c',(Join-Path $workshopStage 'test.cmd'))
            Invoke-WorkshopCommand 'cmd.exe' @('/c',(Join-Path $workshopStage 'test-093.cmd'))
        }
        'Check' {
            Invoke-WorkshopCommand $workshopPython @('-X','utf8',(Join-Path $workshopSource 'player_qol_verify.py'),$workshopStage)
            Invoke-WorkshopCommand $workshopPython @('-X','utf8',(Join-Path $workshopSource 'mercenary_skills_verify.py'))
            Invoke-WorkshopCommand $workshopPython @('-X','utf8',(Join-Path $workshopSource 'leagues/verify.py'),$workshopStage)
            Invoke-WorkshopCommand $workshopPython @('-X','utf8',(Join-Path $workshopSource 'progression/verify.py'),$workshopStage)
            Invoke-WorkshopCommand $workshopPython @('-X','utf8',(Join-Path $workshopSource 'mythicals/verify.py'),$workshopStage)
            Invoke-WorkshopCommand $workshopPython @('-X','utf8',(Join-Path $workshopSource 'paladin_affixes_verify.py'),$workshopStage)
            Invoke-WorkshopCommand $workshopPython @('-X','utf8',(Join-Path $workshopStage 'workshop\endgame\verify.py'))
            Invoke-WorkshopCommand $workshopPython @('-X','utf8',(Join-Path $workshopSource 'monster_modifiers\verify.py'),$workshopStage)
            foreach ($name in @('verify.py','tree_update_verify.py','balance093_verify.py','paladin_verify.py','paladin_rebalance_verify.py','barbarian_verify.py','sorceress_verify.py','assassin_verify.py','amazon_verify.py','character_sheet_verify.py','druid_verify.py','weapon_ailments_verify.py')) {
                Invoke-WorkshopCommand $workshopPython @('-X','utf8',(Join-Path $workshopStage ('workshop\'+$name)))
            }
            foreach ($name in @('tooltip_verify.py','guardian_verify.py','aura_features_verify.py','item_auras_verify.py','ground_visual_verify.py')) {
                Invoke-WorkshopCommand $workshopPython @('-X','utf8',(Join-Path $workshopStage ('workshop\combat\'+$name)),$workshopStage)
            }
        }
        'CheckDeploy' {
            Invoke-WorkshopCommand $workshopPython @((Join-Path $PSScriptRoot 'Deploy-Workshop.py'),'--check')
        }
        'Deploy' {
            if (@(Get-Process D2R,D2RLoader -ErrorAction SilentlyContinue).Count) { throw 'Save and close D2R before deploying.' }
            Invoke-WorkshopCommand $workshopPython @((Join-Path $PSScriptRoot 'Deploy-Workshop.py'))
        }
        'ExportTable' {
            $sheetTools = Join-Path $PSScriptRoot 'spreadsheets\skilltree'
            Invoke-WorkshopCommand $workshopPython @((Join-Path $sheetTools 'collect_tree.py'))
            $node = Join-Path $env:USERPROFILE '.cache\codex-runtimes\codex-primary-runtime\dependencies\node\bin\node.exe'
            if (-not (Test-Path -LiteralPath $node)) { throw 'Bundled spreadsheet runtime not found.' }
            Invoke-WorkshopCommand $node @((Join-Path $sheetTools 'build.mjs'))
        }
    }
}
finally {
    $env:WORKSHOP_MOD_ROOT = $previousOutput
    Pop-Location
}
