"""Presentation cleanup and retirement of new Gem Bags; existing saves stay readable."""
from pathlib import Path
import json,re
from combat.port import read,write

def install(root):
 root=Path(root);assets=[]
 for folder in ('data/global/excel','data/global/excel/base'):
  p=root/folder;h,rows=read(p/'charstats.txt')
  for r in rows:
   for i in range(1,11):
    if r.get('item'+str(i))=='bag':
     for suffix in ('','loc','count','quality'):r['item'+str(i)+suffix]=''
  write(p/'charstats.txt',h,rows);assets.append(folder+'/charstats.txt')
  h,rows=read(p/'misc.txt')
  for r in rows:
   if r.get('code')=='bag':
    r.update(spawnable='0',rarity='0')
    for k in h:
     if k.endswith(('Min','Max','MagicMin','MagicMax')):r[k]='0'
  write(p/'misc.txt',h,rows);assets.append(folder+'/misc.txt')
  h,rows=read(p/'cubemain.txt')
  for r in rows:
   if r.get('output','').split(',')[0]=='bag':r['enabled']='0'
  write(p/'cubemain.txt',h,rows);assets.append(folder+'/cubemain.txt')
 for p in (root/'data/local/lng/strings').glob('*.json'):
  rows=json.loads(p.read_text(encoding='utf-8-sig'));changed=False
  for r in rows:
   for lang,value in r.items():
    if lang not in ('Key','id') and isinstance(value,str) and re.search('reimagined',value,re.I):
     r[lang]=re.sub(r'reimagined(?:[ -]?workshop)?','Astral Workshop',value,flags=re.I);changed=True
  if changed:p.write_text(json.dumps(rows,ensure_ascii=False,indent=2),encoding='utf8');assets.append(p.relative_to(root).as_posix())
 from integration.prepare import install as refresh_manifest
 refresh_manifest(root,pickup_only=True)
 manifest=root/'workshop/combat/assets.json';a=json.loads(manifest.read_text());manifest.write_text(json.dumps(list(dict.fromkeys(a+assets)),indent=2))
 print('Presentation: Astral Workshop strings; no new Gem Bags; legacy withdrawal retained.')
if __name__=='__main__':
 import sys
 install(sys.argv[1])
