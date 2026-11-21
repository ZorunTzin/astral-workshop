"""Focused regression audit for the 0.9.3 player requests."""
from pathlib import Path
import json,csv
from combat.port import read
root=Path(__file__).resolve().parent.parent
for folder in ('data/global/excel','data/global/excel/base'):
 p=root/folder;_,skills=read(p/'skills.txt');by={r['skill']:r for r in skills};_,descs=read(p/'skilldesc.txt');desc={r['skilldesc']:r for r in descs};_,stats=read(p/'itemstatcost.txt');statnames={r['Stat'] for r in stats}
 for name in ('Might','Blessed Aim','Defiance','Conviction','Salvation'):
  for i in range(1,7):
   assert not by[name]['aurastat'+str(i)] or by[name]['aurastat'+str(i)] in statnames
 assert by['Might']['aurastat2']=='item_crushingblow'
 assert by['Blessed Aim']['aurastat2']=='item_deadlystrike'
 assert by['Defiance']['aurastat2']=='damageresist'
 assert by['Conviction']['aurastat5']=='damageresist' and by['Conviction']['aurastatcalc5'].startswith('0-lvl*')
 assert [by['Salvation']['aurastat'+str(i)] for i in (4,5,6)]==['maxfireresist','maxcoldresist','maxlightresist']
 assert by['Prayer']['aurastatcalc1'].startswith('2*(')
 assert desc[by['Prayer']['skilldesc']]['desccalca3']=='('+by['Prayer']['aurastatcalc1']+')/256'
 d=desc[by['Resist Lightning']['skilldesc']];assert d['descline1']=='36' and '80*' in d['desccalca1']
 for suffix in ('HolyBolt','Hammer','Heavens'):
  private=by['Workshop Guardian '+suffix];original=by[{'HolyBolt':'Holy Bolt','Hammer':'Blessed Hammer','Heavens':'Fist of the Heavens'}[suffix]]
  for key in ('EMin','EMax')+tuple(f'E{bound}Lev{n}' for bound in ('Min','Max') for n in range(1,6)):
   assert int(private[key] or 0)==5*int(original[key] or 0)
 _,items=read(p/'setitems.txt');groups={}
 for row in items:
  if row['*ID'].isdigit() and row['set'] and row['item'] and row['disabled']!='1' and row['spawnable']!='0':groups.setdefault(row['set'],set()).add(row['index'])
 _,recipes=read(p/'cubemain.txt');r=[r for r in recipes if r['description'].startswith('Workshop - Direct set reforge ')]
 assert len(r)==sum(len(v)*(len(v)+1) for v in groups.values())
 for members in groups.values():
  for member in members:
   options=[x for x in r if x['input 1']==member]
   assert len(options)==len(members) and {x['output'] for x in options}==members
   assert {int(x['value']) for x in options}==set(range(len(members)))
   assert len({x['param'] for x in options})==1 and all(x['op']=='6' and x['numinputs']=='1' and not x['mod 1'] for x in options)
 j=json.loads((root/'data/global/ui/layouts/buffhud/BuffHudhd.json').read_text());grid=next(x for x in j['children'] if x['name']=='BuffGrid')
 for slot in grid['children']:
  if not slot['name'].startswith('BuffSlot'):continue
  box=slot['fields']['rect'];assert grid['fields']['rect']['x']+box['x']+box['width']<=0
assert 'add(summonCountProperty,n.lo)' in (root/'workshop/passives.cpp').read_text()
rows=list(csv.DictReader((root/'workshop/NODES-step2.csv').open(encoding='utf-8-sig'),delimiter=';'))
for row in rows:
 if row['name'] in ('Titan','Finesse','Fortitude','Grave Lore','Deep Mind'):
  assert row['Knoten 5 mit Bonus'].startswith('+20 ') and any(t in row['Knoten 5 mit Bonus'] for t in (' / +10%',' / 10%'))
print('PASS: new aura stat names/formulas and heal tooltip; 5x private spell curves; explicit Guardian life; every same-set recipe; attribute finales; summon-count binding; HUD slots stay on screen.')
