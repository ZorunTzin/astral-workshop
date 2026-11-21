from pathlib import Path
import os,json,hashlib,subprocess
repo=Path(r'B:\Github\reimagined-workshop')
assert Path.cwd().resolve()==repo
ledger=[]
tracked=subprocess.check_output(['git','ls-files','-z'],cwd=repo).decode().split('\0')
tracked=[p for p in tracked if p]
runtime={p:hashlib.sha256((repo/p).read_bytes()).hexdigest() for p in ['modinfo.json','bank.txt','Play Workshop.cmd']+[str(p.relative_to(repo)).replace('\\','/') for p in (repo/'d2rloader/plugins').glob('*.dll')]}
(repo/'build').mkdir(exist_ok=True)
def checked(rel):
 p=Path(os.path.abspath(repo/rel))
 assert os.path.commonpath([str(repo),str(p)])==str(repo) and p!=repo,p
 return p
def move(a,b):
 src,dst=checked(a),checked(b)
 assert src.exists() or src.is_symlink(),str(src)
 assert not dst.exists() and not dst.is_symlink(),str(dst)
 dst.parent.mkdir(parents=True,exist_ok=True)
 os.rename(src,dst)
 ledger.append({'from':a,'to':b})
 (repo/'build/reorganization-moves.json').write_text(json.dumps(ledger,indent=2))
move('local-work','archive/local')
move('archive/local/workshop-clusters','build/workshop-clusters')
move('archive/local/workshop-tree-v2/base.zip','build/inputs/reimagined-base.zip')
move('build/workshop-clusters/backup','archive/local/backups/workshop')
move('build/workshop-clusters/save-backups','archive/local/backups/saves')
move('workshop','src/workshop')
for name in ['README.md','CONSTELLATIONS.md','FORGING.md','combat/README.md','integration/README.md']:
 move('src/workshop/'+name,'docs/workshop/'+name)
move('src/workshop/Astral-Skilltree-0.9.5.xlsx','tables/Astral-Skilltree-0.9.5.xlsx')
move('src/workshop/INNER-CLUSTERS-draft.csv','tables/drafts/INNER-CLUSTERS-draft.csv')
move('src/workshop/NODES-step1.csv','archive/legacy-tree/NODES-step1.csv')
for name in [*[f'atlas{i}.inc' for i in range(9)],'layout.inc','sectors.py']:
 move('src/workshop/'+name,'archive/legacy-tree/'+name)
for name in ['passives.obj','passives.res']:
 if (repo/'src/workshop'/name).exists():move('src/workshop/'+name,'build/objects/previous/'+name)
if (repo/'src/workshop/__pycache__').exists():move('src/workshop/__pycache__','build/cache/previous-workshop')
move('src/workshop/install.ps1','scripts/Install-Workshop.ps1')
move('build/workshop-clusters/deploy.py','scripts/Deploy-Workshop.py')
for name in list((repo/'scripts').iterdir()):
 if name.name not in ['Install-Workshop.ps1','Deploy-Workshop.py','reorganize_workspace.py'] and name.is_file():
  move('scripts/'+name.name,'archive/upstream-tools/'+name.name)
move('D2R Data Guide.url','docs/links/D2R Data Guide.url')
move('lint','tools/lint')
for name in ['output.txt','output_base.txt']:
 if (repo/'tools/lint'/name).exists():move('tools/lint/'+name,'archive/legacy-reports/lint/'+name)
move('archive/local/outputs/runeword-forging-20261109/Runenwoerter-Schmieden-Entwurf.xlsx','tables/Runenwoerter-Schmieden-Entwurf.xlsx')
move('archive/local/outputs/runeword-forging-20261109/Runenwoerter-Schmieden-Entwurf.xlsx.inspect.ndjson','build/spreadsheets/runewords/Runenwoerter-Schmieden-Entwurf.xlsx.inspect.ndjson')
for name in ['build.mjs','collect_tree.py']:
 move('archive/local/outputs/workshop-swarm-update/'+name,'scripts/spreadsheets/skilltree/'+name)
move('archive/local/outputs/workshop-swarm-update/node_modules','scripts/spreadsheets/node_modules')
for p in list((repo/'archive/local/outputs/workshop-swarm-update').iterdir()):
 move('archive/local/outputs/workshop-swarm-update/'+p.name,'build/spreadsheets/skilltree/'+p.name)
for p in list((repo/'archive/local').iterdir()):
 name=p.name
 if name.startswith('workshop-'):move('archive/local/'+name,'archive/local/prototypes/'+name)
for name in ['.pd2-aura-buffs','.pd2-reawakening-setup']:move('archive/local/'+name,'archive/local/pd2/'+name)
for name in ['.fmmp-repository','.frankenstein-release-0522']:move('archive/local/'+name,'archive/local/frankenstein/'+name)
move('archive/local/.runeforge-editor','archive/local/tools/runeforge-editor')
move('archive/local/.workshop-sdk','archive/local/dependencies/.workshop-sdk')
move('archive/local/runeword-plan','archive/local/design/runeword-plan')
move('archive/local/runtime-library-copy','archive/local/recovery/runtime-library-copy')
move('archive/local/migration-20261110.json','archive/local/migrations/first-workspace-move.json')
move('archive/local/README.md','archive/local/migrations/previous-workspace-README.md')
for rel in ['archive/local/outputs/runeword-forging-20261109','archive/local/outputs/workshop-swarm-update','archive/local/outputs']:
 p=checked(rel)
 if p.is_dir() and not any(p.iterdir()):p.rmdir()
# Preserve the old project overview before replacing it with the new navigation.
(repo/'docs/UPSTREAM-README.md').write_bytes((repo/'README.md').read_bytes())
(repo/'build/reorganization-runtime-sha256.json').write_text(json.dumps(runtime,indent=2))
(repo/'build/reorganization-original-tracked.json').write_text(json.dumps(tracked))
for rel,digest in runtime.items():assert hashlib.sha256((repo/rel).read_bytes()).hexdigest()==digest,rel
print(f'Moved {len(ledger)} entries; runtime launcher/config/plugin hashes unchanged.')
