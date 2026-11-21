"""Check generated combat records before installation."""
import csv,json,re,sys
from pathlib import Path
root=Path(sys.argv[1]); here=Path(__file__).resolve().parent
bindings=json.loads((here/'bindings.json').read_text())
for folder in ('data/global/excel','data/global/excel/base'):
 p=root/folder
 def rows(name):
  return list(csv.DictReader((p/(name+'.txt')).open(encoding='utf-8-sig'),delimiter='\t'))
 skills=rows('skills');states=rows('states');missiles=rows('missiles');stats=rows('itemstatcost')
 names={r['skill'] for r in skills};state_names={r['state'] for r in states};missile_names={r['Missile'] for r in missiles}
 assert max(int(r['*Id']) for r in skills if r['*Id'].isdigit())==569
 assert max(int(r['*ID']) for r in states if r['*ID'].isdigit())==299
 assert max(int(r['*ID']) for r in stats if r['*ID'].isdigit())==502
 changed={66,70,71,72,76,77,81,82,86,87,91,*range(533,570)}
 for r in skills:
  if not r['*Id'].isdigit() or int(r['*Id']) not in changed:continue
  for k,v in r.items():
   if not v:continue
   if k.lower().endswith('state'):assert v in state_names,(r['skill'],k,v)
   if 'missile' in k.lower():assert v in missile_names,(r['skill'],k,v)
   for name in re.findall(r"skill\('([^']+)'",v):assert name in names,(r['skill'],k,name)
 for table,key,idkey in [('skills','skill','*Id'),('states','state','*ID'),('missiles','Missile','*ID'),('itemstatcost','Stat','*ID')]:
  data=rows(table); ids=[r[idkey] for r in data if r[idkey].isdigit()]
  assert len(ids)==len(set(ids)),table
  for value in bindings[table].values():assert str(value) in ids,(table,value)
 for skill in (98,99,100,102,103,104,105,108,109,110,113,114,115,118,119,120,122,123,124,125):
  row=next(r for r in skills if r['*Id']==str(skill))
  if skill in (100,105):
   assert row['passive']=='1' and not row['srvdofunc']
   continue
  if skill==110:
   assert row['srvdofunc']=='16' and row['summon']=='workshop_astral_guardian'
   continue
  assert row['srvdofunc']=='18' and not row['aura'] and not row['periodic']
  tier={1:(15,3),6:(30,6),12:(45,9),18:(60,12),24:(75,15),30:(90,18)}[int(row['reqlevel'])]
  desc=next(r for r in rows('skilldesc') if r['skilldesc']==row['skilldesc'])
  field=next(k.replace('texta','calca') for k,v in desc.items() if v=='WorkshopAuraReservedValue')
  assert desc[field]==f"max(1,({tier[0]}+{tier[1]}*lvl)*100/(100+stat('workshop_reservation_efficiency'.accr)))",(row['skill'],desc[field])
for asset in json.loads((here/'assets.json').read_text()):assert (root/asset).is_file(),asset
print('PASS: combat IDs, curse state/missile/formula references, seventeen aura toggles, five replacements and all visual assets in both table sets.')
