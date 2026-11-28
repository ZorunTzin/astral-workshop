"""Regression checks for full mythical belts and appended Trial casters."""
import sys,json,re,hashlib
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from combat.port import read
from endgame.trial_balance import install
root=Path(sys.argv[1]);casters=('willowisp3','vampire5','fetishshaman5')
for folder in ('data/global/excel','data/global/excel/base'):
 rows=read(root/folder/'armor.txt')[1]
 belts=[r for r in rows if r['code']=='Y09' or (r['code'].startswith('Y') and r['code'][1:].isdigit() and 43<=int(r['code'][1:])<=80)]
 assert len(belts)==39
 for r in belts:
  assert (r['type'],r['belt'],r['normcode'],r['ubercode'],r['ultracode'],r['invwidth'],r['invheight'])==('belt','6','lbl','zlb',r['code'],'2','1')
  assert r['type2'].startswith('ym')
 by={r['Id']:r for r in read(root/folder/'monstats.txt')[1]}
 binding=(root/'workshop/endgame/bindings.h').read_text()
 for base in casters:
  r=by['wsarenamon_caster_'+base]
  assert r['*hcIdx'] in re.findall(r'\d+',binding)
  assert r['AI']==by[base]['AI'] and r['Skill1']==by[base]['Skill1'] and r['Skill1']
  assert r['aidist(H)']=='255' and r['NameStr']==by[base]['NameStr']
  for k in r:
   if k.startswith('TreasureClass'):assert r[k]==by['wsarenamon2'][k]
 paths=[root/folder/'monstats.txt',root/'workshop/endgame/bindings.h',root/'data/hd/character/monsters.json']
before=[p.read_bytes() for p in paths];install(root);assert before==[p.read_bytes() for p in paths]
print('PASS: 39 full-size native belts; three appended casters with skills, Trial loot and runtime bindings; idempotent generation.')
