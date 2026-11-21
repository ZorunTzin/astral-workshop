from pathlib import Path
import json,os
repo=Path(r'B:\Github\reimagined-workshop')
w=repo/'src/workshop'; stage=repo/'build/workshop-clusters'
def put(path,text):
 p=repo/path;p.parent.mkdir(parents=True,exist_ok=True);p.write_text(text,encoding='utf8')
# Runtime payloads remain at the repository root; development code is separate.
p=repo/'.gitignore';s=p.read_text();s=s.replace('workshop/__pycache__/','**/__pycache__/').replace('workshop/*.','src/workshop/*.').replace('build/*','/build/').replace('# Local Diablo build workspaces, prototypes and backups\n/local-work/','# Local archives and spreadsheet dependency junction\n/archive/local/\n/scripts/spreadsheets/node_modules/');p.write_text(s)
p=repo/'.gitattributes';p.write_text(p.read_text().replace('workshop/integration/upstream/','src/workshop/integration/upstream/'))
# Build commands accept an explicit output root and also work from the canonical tree.
p=w/'build.cmd';s=p.read_text().replace('if not exist "..\\d2rloader\\plugins" mkdir "..\\d2rloader\\plugins"','if not defined WORKSHOP_MOD_ROOT (\n if exist "%~dp0..\\d2rloader\\plugins" (for %%I in ("%~dp0..") do set "WORKSHOP_MOD_ROOT=%%~fI") else (for %%I in ("%~dp0..\\..") do set "WORKSHOP_MOD_ROOT=%%~fI")\n)\nif not exist "%WORKSHOP_MOD_ROOT%\\d2rloader\\plugins" mkdir "%WORKSHOP_MOD_ROOT%\\d2rloader\\plugins"').replace('/Fe:..\\d2rloader\\plugins\\d2rl-astral-workshop.dll','/Fe:"%WORKSHOP_MOD_ROOT%\\d2rloader\\plugins\\d2rl-astral-workshop.dll"');p.write_text(s)
p=w/'combat/build.cmd';s=p.read_text().replace('pushd "%~dp0"','pushd "%~dp0"\nif not defined WORKSHOP_MOD_ROOT (\n if exist "%~dp0..\\..\\d2rloader\\plugins" (for %%I in ("%~dp0..\\..") do set "WORKSHOP_MOD_ROOT=%%~fI") else (for %%I in ("%~dp0..\\..\\..") do set "WORKSHOP_MOD_ROOT=%%~fI")\n)',1).replace('/Fe:..\\..\\d2rloader\\plugins\\d2rl-workshop-combat.dll','/Fe:"%WORKSHOP_MOD_ROOT%\\d2rloader\\plugins\\d2rl-workshop-combat.dll"');p.write_text(s)
# Short, portable staging launchers; invoked by scripts/Workshop.ps1.
put('build/workshop-clusters/compile.cmd','@echo off\ncall "C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\BuildTools\\Common7\\Tools\\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul\ncall "%~dp0workshop\\build.cmd"\n')
put('build/workshop-clusters/combat-build.cmd','@echo off\ncall "C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\BuildTools\\Common7\\Tools\\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul\ncall "%~dp0workshop\\combat\\build.cmd"\n')
for name in ['test.cmd','test-093.cmd']:
 p=stage/name;s=p.read_text();s=s.replace('B:\\Github\\reimagined-workshop\\local-work\\workshop-clusters\\workshop','%~dp0workshop');p.write_text(s)
# Retired receipt bridge is no longer compiled or deployed.
p=w/'set_reforge.inc';os.rename(p,repo/'archive/legacy-tree/set_reforge.inc')
# Relocate the deploy helper without changing any live game files.
p=repo/'scripts/Deploy-Workshop.py';s=p.read_text().replace('src=Path(__file__).resolve().parent\nrepo=Path(r"B:\\Github\\reimagined-workshop")','repo=Path(__file__).resolve().parent.parent\nsrc=repo/"build/workshop-clusters"').replace('"set_reforge.inc",','').replace('backup=src/"backup"/datetime.now()', 'backup=repo/"archive/local/backups/workshop"/datetime.now()')
s=s.replace('jobs += [(repo/rel,rel,"repo") for rel in assets+docs+plugins+modfiles]', '''def repository_destination(rel):
    if rel in {"workshop/README.md","workshop/CONSTELLATIONS.md","workshop/FORGING.md","workshop/combat/README.md","workshop/integration/README.md"}:
        return repo/"docs"/rel
    if rel.startswith("workshop/"):
        return repo/"src"/rel
    return repo/rel
jobs += [(repository_destination(rel),rel,"repo") for rel in assets+docs+plugins+modfiles]
import sys
if "--check" in sys.argv:
    missing=[rel for _,rel,_ in jobs if not (src/rel).is_file()]
    if missing:raise RuntimeError("Missing staging inputs: "+str(missing))
    print("Deploy plan checked:",len(jobs),"targets; no writes to repository or game.")
    raise SystemExit(0)''')
p.write_text(s)
# Linter's relative data paths now traverse tools/lint back to the runtime root.
p=repo/'tools/lint/config.json';s=p.read_text().replace('..\\\\data','..\\\\..\\\\data').replace('"log": "output.txt"','"log": "../../build/reports/lint/output.txt"');p.write_text(s)
(repo/'build/reports/lint').mkdir(parents=True,exist_ok=True)
# Spreadsheet source collectors/builders work independently of the calling cwd.
p=repo/'scripts/spreadsheets/skilltree/collect_tree.py';s=p.read_text().replace("root = Path('workshop-clusters')", "repo = Path(__file__).resolve().parents[3]\nroot = repo / 'build/workshop-clusters'").replace("Path('outputs/workshop-swarm-update/tree.json')", "(repo / 'build/spreadsheets/skilltree/tree.json')");p.write_text(s)
p=repo/'scripts/spreadsheets/skilltree/build.mjs';s=p.read_text().replace("const outputDir = new URL('.', import.meta.url).pathname.replace(/^\\/([A-Za-z]:)/, '$1');", "const generatedDir = new URL('../../../build/spreadsheets/skilltree/', import.meta.url);").replace("new URL('tree.json', import.meta.url)","new URL('tree.json', generatedDir)").replace('new URL(`preview-${name}.png`,import.meta.url)','new URL(`preview-${name}.png`,generatedDir)').replace("../../../workshop/Astral-Skilltree-0.9.5.xlsx","../../../tables/Astral-Skilltree-0.9.5.xlsx");p.write_text(s)
# Documentation links point to the relocated editable source definitions.
p=repo/'docs/workshop/README.md';s=p.read_text()
for name in ['INNER-CLUSTERS.json','NODES-step2.csv','NODES.csv']:
 s=s.replace(']('+name+')','](../../src/workshop/'+name+')')
p.write_text(s)
print('Updated development paths, deploy destination mapping, linter and spreadsheet scripts.')
