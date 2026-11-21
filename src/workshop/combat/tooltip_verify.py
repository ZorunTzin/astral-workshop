"""New Paladin tooltips must render their actual numeric formulas."""
import json,sys
from pathlib import Path
from port import read
root=Path(sys.argv[1]);strings={r['Key']:r['enUS'] for r in json.loads((root/'data/local/lng/strings/skills.json').read_text(encoding='utf8'))}
for folder in ('data/global/excel','data/global/excel/base'):
 p=root/folder;_,skills=read(p/'skills.txt');_,descs=read(p/'skilldesc.txt')
 for skill in (100,105,110,103,109):
  r=next(r for r in skills if r['*Id']==str(skill));d=next(d for d in descs if d['skilldesc']==r['skilldesc'])
  for n in range(1,7):
   if not d['descline'+str(n)]:continue
   if d['descline'+str(n)]=='18':continue
   assert d['descline'+str(n)]=='36'
   key=d['desctexta'+str(n)];assert key==d['desctextb'+str(n)] and '%d' in strings[key]
   assert d['desccalca'+str(n)] and d['desccalcb'+str(n)]
  if skill==103:
   assert d['desccalca1']=='clc3' and 'Physical nova damage' in strings[d['desctexta1']]
   assert d['desccalca2']=='5' and 'Hits needed' in strings[d['desctexta2']]
   assert d['desccalca4']=='clc4' and 'Nova radius' in strings[d['desctexta4']]
   assert '(50+25*lvl)' in r['calc3'] and '(100+3*(lvl-1))' in r['calc4']
  if skill==110:
   assert d['descline1']=='36' and d['desccalca1']=='80*(100+clc1)/100'
   assert 'stat(' not in d['desccalca2'] and 'clc3' in d['desccalca2']
   assert 'clc3' in d['desccalca3']
   assert "1+lvl/10+stat('ws_pet_count'.accr)"==d['desccalca4']
  if skill==109:
   assert d['desccalca2']=='clc2' and d['desccalcb2']=='100'
   assert d['desccalca4']=='8' and d['desccalca5']=='8'
print('PASS: computed values for all five replacement skill tooltips, shared formatted captions, fractional regeneration, eight-second duration and eight-yard radius.')
