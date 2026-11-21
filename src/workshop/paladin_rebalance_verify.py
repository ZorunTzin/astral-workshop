"""Validate aura nerfs against the recorded pre-rebalance formulas."""
import json,re
from pathlib import Path
from combat.port import read
from paladin_rebalance import KEEP,DAMAGE,TARGETS
from combat.aura_reservation import TIERS
root=Path(__file__).resolve().parent.parent
records=json.loads((root/'workshop/PALADIN-REBALANCE.json').read_text())
assert len(records)==2*len(TARGETS)
for entry in records:
 before,after=entry['before'],entry['after'];skill=entry['id']
 for key,value in before.items():
  if key.startswith('aurastatcalc'):
   slot=int(key[12:])
   expected=value if slot in KEEP.get(skill,set()) or re.fullmatch(r'ast[1-6]',value) else '('+value+')/2'
   assert after[key]==expected,(skill,key)
  if skill in DAMAGE and key.startswith(('EMin','EMax')):assert int(after[key])==int(value)//2
 for slot in KEEP.get(skill,set()):assert before['aurastatcalc'+str(slot)]==after['aurastatcalc'+str(slot)]
for folder in ('data/global/excel','data/global/excel/base'):
 p=root/folder;_,skills=read(p/'skills.txt');_,descs=read(p/'skilldesc.txt')
 by={int(r['*Id']):r for r in skills if r['*Id'].isdigit()};ds={r['skilldesc']:r for r in descs}
 for skill in (*TARGETS,99,103):
  row=by[skill];desc=ds[row['skilldesc']];base,growth=TIERS[int(row['reqlevel'])]
  formulas=[desc[k.replace('texta','calca')] for k,v in desc.items() if v=='WorkshopAuraReservedValue' and 'texta' in k]
  assert formulas==[f"max(1,({base}+{growth}*lvl)*150/(100+stat('workshop_reservation_efficiency'.accr)))"]
 assert by[99]['aurastatcalc1']=="2*((edns)*(100+stat('workshop_aura_effect'.accr))/100)"
 assert by[103]['calc3'].startswith('(50+25*lvl)')
print('PASS: 15 aura effects halved, protected 1%-effects retained, 17 reservation tooltips match runtime factor, Prayer/Retribution preserved.')
