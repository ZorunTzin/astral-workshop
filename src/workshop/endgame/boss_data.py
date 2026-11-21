"""Private act-boss copies: no quest TCs, shared native death/loot tracking."""
from pathlib import Path
import json
from combat.port import read,write
BOSSES=['andariel','duriel','mephisto','diablo','baalcrab']
def install(root):
 root=Path(root);assets=[];ids=None
 for folder in ('data/global/excel','data/global/excel/base'):
  p=root/folder;h,rows=read(p/'monstats.txt');by={r['Id']:r for r in rows};current=[]
  nextid=max(int(r['*hcIdx']) for r in rows if r['*hcIdx'].isdigit())+1
  for i,base in enumerate(BOSSES):
   key='wsarenaboss'+str(i);old=by.get(key);r=by[base].copy()
   r.update(Id=key,NextInClass='',neverCount='1',primeevil='',minion1='',minion2='',PartyMin='0',PartyMax='0',MinGrp='1',MaxGrp='1',**{'*hcIdx':old['*hcIdx'] if old else str(nextid),'Level':'85','Level(N)':'85','Level(H)':'85'})
   if not old:nextid+=1
   # Private ids avoid native act-boss quest hooks; use the existing safe arena loot branch.
   template=by['wsarenamon0']
   for k in r:
    if k.startswith('TreasureClass'):r[k]='' if 'Quest' in k else template.get(k,'')
    if k.startswith('Exp'):r[k]='0'
   if old:old.update(r)
   else:rows.append(r)
   current.append(int(r['*hcIdx']))
  if ids is not None:assert ids==current
  ids=current;write(p/'monstats.txt',h,rows);assets.append(folder+'/monstats.txt')
 p=root/'data/hd/character/monsters.json';models=json.loads(p.read_text(encoding='utf-8-sig'))
 for i,base in enumerate(BOSSES):models['wsarenaboss'+str(i)]=models[base]
 p.write_text(json.dumps(models,ensure_ascii=False,indent=2),encoding='utf8');assets.append('data/hd/character/monsters.json')
 (root/'workshop/endgame/boss_bindings.h').write_text('#pragma once\nnamespace Arena { constexpr int BossClasses[]={'+','.join(map(str,ids))+'}; }\n')
 return assets
