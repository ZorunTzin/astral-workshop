from pathlib import Path
import os,json,hashlib
repo=Path(r'B:\Github\reimagined-workshop');stage=repo/'build/workshop-clusters'
ledger=json.loads((repo/'build/reorganization-moves.json').read_text())
def move(a,b):
 s=repo/a;d=repo/b
 assert os.path.commonpath([str(repo),os.path.abspath(s)])==str(repo)
 assert os.path.commonpath([str(repo),os.path.abspath(d)])==str(repo)
 assert not d.exists() and not d.is_symlink(),str(d)
 d.parent.mkdir(parents=True,exist_ok=True);os.rename(s,d);ledger.append({'from':a,'to':b})
for name in ['compile.cmd','combat-build.cmd','test.cmd','test-093.cmd']:
 target=repo/'scripts/build'/name;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes((stage/name).read_bytes())
for p in list(stage.glob('*.py')):move(str(p.relative_to(repo)),'archive/local/diagnostics/'+p.name)
for name in ['analysis-libs','guardian-check.cmd','item-check.cmd']:
 move('build/workshop-clusters/'+name,'archive/local/diagnostics/'+name)
for p in list(stage.glob('*.reference'))+list(stage.glob('*reference.cpp'))+[stage/'native-aura-snapshot.bin',stage/'suite-source']:
 if p.exists():move(str(p.relative_to(repo)),'archive/local/research/'+p.name)
for name in ['buff-hud','buff-hud.zip']:
 move('build/workshop-clusters/'+name,'archive/local/downloads/'+name)
for name in ['curse-icons-reference.jpg','curse-style.png','sigil-reference.png','paladin-icon-inputs.json','preview-small.png']:
 move('build/workshop-clusters/'+name,'archive/local/art-reference/'+name)
if (stage/'__pycache__').exists():move('build/workshop-clusters/__pycache__','build/cache/previous-stage')
if (stage/'smoke093.pid').exists():move('build/workshop-clusters/smoke093.pid','archive/local/diagnostics/smoke093.pid')
move('build/inputs/reimagined-base.zip','tools/inputs/reimagined-base.zip')
(repo/'build/inputs').rmdir()
base=repo/'tools/inputs/reimagined-base.zip'
(repo/'tools/inputs/manifest.json').write_text(json.dumps({'file':base.name,'sha256':hashlib.sha256(base.read_bytes()).hexdigest(),'purpose':'Clean Reimagined baseline for reproducible Workshop generation','upstream_commit':'4fa2cff22ff94685d5ffaa6a4c2cb7e8d7d3b88f'},indent=2)+'\n')
(repo/'tools/inputs/README.md').write_text('# Generator baseline\n\n`reimagined-base.zip` is the clean local Reimagined input used by `scripts/Workshop.ps1 -Action Generate`. It is not used by the game and is ignored by Git because it is a local source archive. Keep it when clearing `build/`. The adjacent manifest records its SHA-256 and upstream revision.\n')
p=repo/'scripts/Workshop.ps1';s=p.read_text().replace("    New-Item -ItemType Directory -Path (Join-Path $workshopStage 'workshop') -Force | Out-Null", "    New-Item -ItemType Directory -Path (Join-Path $workshopStage 'workshop') -Force | Out-Null\n    Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'build\\compile.cmd'),(Join-Path $PSScriptRoot 'build\\combat-build.cmd'),(Join-Path $PSScriptRoot 'build\\test.cmd'),(Join-Path $PSScriptRoot 'build\\test-093.cmd') -Destination $workshopStage -Force")
s=s.replace('build\\inputs\\reimagined-base.zip','tools\\inputs\\reimagined-base.zip');p.write_text(s)
p=repo/'.gitignore';s=p.read_text();s+='\n# Local clean generator input\n/tools/inputs/*.zip\n';p.write_text(s)
for rel in ['README.md','docs/FOLDER-STRUCTURE.md']:
 p=repo/rel;s=p.read_text().replace('reports and clean input archive','reports').replace('build/inputs/reimagined-base.zip','tools/inputs/reimagined-base.zip');p.write_text(s)
p=repo/'docs/FOLDER-STRUCTURE.md';s=p.read_text().replace('`Build` compiles the staging plugins and runs native tests.','`Build` compiles the existing generated staging plugins and runs native tests. Run `Generate` after source or balance changes so generated bindings stay synchronized. Build launcher templates are versioned under `scripts/build/` and are restored by `Prepare`/`Generate`.');p.write_text(s)
p=repo/'docs/UPSTREAM-README.md';s=p.read_text();s=s[s.index('# d2r-reimagined-mod'):].split('## Local development workspace')[0].rstrip()+'\n';p.write_text(s)
move('scripts/reorganize_workspace.py','archive/migrations/reorganize-20261110.py')
(repo/'build/reorganization-moves.json').write_text(json.dumps(ledger,indent=2))
print('Separated diagnostics, research, artwork references and build launcher templates. Clean input moved outside disposable build directory.')
