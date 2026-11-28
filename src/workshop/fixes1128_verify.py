"""Regression checks for Labyrinth clone resistances, charm recipes and ground-only beams."""
from pathlib import Path
import sys,json
sys.path.insert(0,str(Path(__file__).resolve().parent))
from combat.port import read
from endgame.labyrinth_balance import SOURCES,RESISTS
root=Path(sys.argv[1])
for folder in ('data/global/excel','data/global/excel/base'):
 d=root/folder;rows=read(d/'monstats.txt')[1];by={r['Id']:r for r in rows};clones=0
 for target,source in SOURCES.items():
  expected=[str(int(by[source][f] or 0)+15) for f in RESISTS]
  assert [by[target][f] for f in RESISTS]==expected
  clone=by.get('workshop_modifier_clone_'+by[target]['*hcIdx'])
  if clone:assert [clone[f] for f in RESISTS]==expected;clones+=1
 assert clones>0
 recipes=[r for r in read(d/'cubemain.txt')[1] if 'UPGRADE CHARM TIER' in r['description'] and 'Splash Charm' in r['description']]
 assert len(recipes)==8
 for r in recipes:
  assert r['input 1']=='key' and not r['op'] and not r['param'] and not r['value']
  assert r['output'] and r['output']!='usetype,mod' and not r['output b']
  assert not any(v for k,v in r.items() if k.startswith('mod '))
 print('PASS:',folder,len(SOURCES),'base resist sets,',clones,'clones and eight ordinary-key charm recipes.')
assets=json.loads((root/'workshop/mythicals/assets.json').read_text());count=0
for rel in assets:
 if not rel.startswith('data/hd/items/') or not rel.endswith('.json') or 'workshopmythical' not in rel:continue
 j=json.loads((root/rel).read_text());components=[c for e in j['entities'] for c in e['components']]
 assert any(c['type']=='UnitRootComponent' and '/dropped_items/' in c.get('state_machine_filename','') for c in components)
 beams=[c for c in components if c.get('filename','').endswith('fx_horadric_light.particles')]
 assert len(beams)==1 and beams[0]['hardKillOnDestroy'];count+=1
assert count==92
for p in (root/'data/hd/character').rglob('*.json'):
 assert 'workshopmythical' not in p.read_text(encoding='utf-8-sig'),p
print('PASS: 92 dropped-item beams with immediate destruction; no equipped character asset references.')
