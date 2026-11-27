"""Append the fourth split without renumbering any existing missile IDs."""
from pathlib import Path
import json
from combat.port import read,write

def install(root):
 root=Path(root);binding=None
 for folder in ('data/global/excel','data/global/excel/base'):
  p=root/folder;h,ms=read(p/'missiles.txt');dh,ds=read(p/'skilldesc.txt');sh,ss=read(p/'skills.txt')
  name='workshop_amazon_split4';r=next((r for r in ms if r['Missile']==name),None)
  if r is None:
   r=next(r for r in ms if r['Missile']=='workshop_amazon_split3').copy();r.update(Missile=name,**{'*ID':str(max(int(r['*ID']) for r in ms if r['*ID'].isdigit())+1)});ms.append(r)
  current=int(r['*ID']);assert binding is None or binding==current;binding=current
  skill=next(r for r in ss if r['skill']=='Guided Arrow');d=next(r for r in ds if r['skilldesc']==skill['skilldesc'])
  for i in range(1,6):
   if d.get('dsc2texta'+str(i))=='WorkshopAmazon22_dsc22':d['dsc2calca'+str(i)]='4'
  d['ddam calc1']='(100+clc4)/3-100'
  d['desccalca2']='(100+clc4)/3-100'
  d['desccalca3']='pnma/3';d['desccalcb3']='pxma/3'
  skill['cltdofunc']=''
  for suffix in ('','a','b','c'):skill['cltmissile'+suffix]=''
  write(p/'skills.txt',sh,ss)
  write(p/'missiles.txt',h,ms);write(p/'skilldesc.txt',dh,ds)
 p=root/'workshop/AMAZON.json';data=json.loads(p.read_text());data['bindings']['Split4']=binding;p.write_text(json.dumps(data,indent=2))
 (root/'workshop/combat/amazon_bindings.h').write_text('// Generated private Amazon missile IDs.\n'+''.join(f'inline constexpr int {k}={v};\n' for k,v in data['bindings'].items()))
 p=root/'data/hd/missiles/missiles.json';data=json.loads(p.read_text(encoding='utf-8-sig'));
 for n in range(5):data['workshop_amazon_split'+str(n)]='workshop_split_arrow'
 p.write_text(json.dumps(data,indent=4),encoding='utf8')
 p=root/'data/local/lng/strings/skills.json';rows=json.loads(p.read_text(encoding='utf-8-sig'))
 for r in rows:
  if r['Key']=='WorkshopAmazon22Desc':
   for lang in r:
    if lang not in ('id','Key'):r[lang]='Impact creates two seeking arrows.\nFour split generations:\n1 -> 2 -> 4 -> 8 -> 16.\nAll arrow damage reduced to one third.'
 p.write_text(json.dumps(rows,ensure_ascii=False,indent=2),encoding='utf8')
 print('Split Arrow: four splits, 1 -> 2 -> 4 -> 8 -> 16; appended missile',binding)
