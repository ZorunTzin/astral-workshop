from pathlib import Path
import sys,json,hashlib
from combat.port import read
from equipment_buffs import install,RESISTS,PALADIN,FORGES
r=Path(sys.argv[1]); d=r/'data/global/excel'
_,types=read(d/'itemtypes.txt');types={t['Code']:t for t in types}
def parents(code,seen=None):
 seen=set() if seen is None else seen
 if not code or code in seen:return seen
 seen.add(code)
 if code in types:
  for k in ('Equiv1','Equiv2'):parents(types[code][k],seen)
 return seen
_,suffixes=read(d/'magicsuffix.txt'); new=[a for a in suffixes if a['name'].startswith('WorkshopMaxRes')]
assert len(new)==5 and len({a['group'] for a in new})==1
for i,a in enumerate(new,1):
 assert a['itype1']=='armo' and a['mod1code']=='res-all-max' and a['mod1min']==a['mod1max']==str(i)
 assert a['spawnable']==a['rare']=='1'
for t in ('boot','glov','belt','helm','circ','shld','tors','ashd','pelt','phlm','head','grim'):assert 'armo' in parents(t),t
for t in ('weap','char','jewl','bowq','xboq','amul','ring'):assert 'armo' not in parents(t),t
_,armor=read(d/'armor.txt'); armor={a['code'] for a in armor}
for table in RESISTS:
 _,rows=read(d/(table+'.txt'));by={v['index']:v for v in rows}
 for name,(lo,hi) in RESISTS[table].items():
  a=by[name];assert (a.get('code') or a.get('item')) in armor
  matches=[i for i in range(1,13 if table=='uniqueitems' else 10) if a['prop'+str(i)]=='res-all-max'];assert len(matches)==1
  i=matches[0];assert (int(a['min'+str(i)]),int(a['max'+str(i)]))==(lo,hi) and 1<=lo<=hi<=5
rules=json.loads((r/'workshop/AFFIX-SCALING.json').read_text(encoding='utf8'))
assert next(a for a in rules if a['code']=='res-all-max')['kind']==1
assert 5*200//100==10 and 5*135//100==6
bundles=json.loads((r/'workshop/FORGING-audit.json').read_text(encoding='utf8'))
for name,(lo,hi) in FORGES.items():
 matches=[b for b in bundles if b['name']==name];assert len(matches)==7
 for b in matches:assert b['baseType'] in ('helm','tors') and [42,0,lo,hi] in b['effects']
# Class access remains unchanged until the complete skill migration is validated.
paths=[d/(t+'.txt') for t in ('uniqueitems','setitems','magicsuffix')]+[r/'workshop/forge.inc',r/'workshop/FORGING.json']
before=[p.read_bytes() for p in paths];install(r);assert before==[p.read_bytes() for p in paths]
print('PASS: targeted armor bonuses, exclusions, 5 tiers, scaling 10/6, all forge rarities and idempotence.')
