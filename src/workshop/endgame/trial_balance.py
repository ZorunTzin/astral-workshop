"""Refresh private Trial monster awareness without changing world monsters."""
from pathlib import Path
from combat.port import read,write
import re,json

def install(root):
 root=Path(root)
 # Append only: existing character/monster IDs must remain stable.
 casters=('willowisp3','vampire5','fetishshaman5')
 binding=root/'workshop/endgame/bindings.h'
 binding_text=binding.read_text();extra_ids=[]
 models_path=root/'data/hd/character/monsters.json'
 models=json.loads(models_path.read_text(encoding='utf-8-sig'))
 for folder in ('data/global/excel','data/global/excel/base'):
  p=root/folder/'monstats.txt';h,rows=read(p);by={r['Id']:r for r in rows}
  template=by['wsarenamon2'];current=[]
  for base in casters:
   name='wsarenamon_caster_'+base
   row=by.get(name)
   if row is None:
    row=by[base].copy()
    row.update(Id=name,**{'*hcIdx':str(max(int(r['*hcIdx']) for r in rows if r['*hcIdx'].isdigit())+1)})
    for key in ('NextInClass','boss','primeevil','npc','minion1','minion2'):row[key]=''
    for key in ('PartyMin','PartyMax'):row[key]='0'
    for key in ('MinGrp','MaxGrp','neverCount'):row[key]='1'
    # Use the existing caster's Trial loot routes, including mythical rolls.
    for key in row:
     if key.startswith(('TreasureClass','Exp')) or key in ('Level','Level(N)','Level(H)'):row[key]=template[key]
    rows.append(row)
   current.append(int(row['*hcIdx']))
   models[name]=models[base]
  assert not extra_ids or extra_ids==current
  extra_ids=current;write(p,h,rows)
 ids_text=re.search(r'MonsterClasses\[\]\s*=\s*\{([^}]+)',binding_text).group(1)
 pool=list(map(int,re.findall(r'\d+',ids_text)))
 pool.extend(i for i in extra_ids if i not in pool)
 binding.write_text(binding_text.replace(ids_text,','.join(map(str,pool))))
 models_path.write_text(json.dumps(models,ensure_ascii=False,indent=2),encoding='utf8')
 ids=set()
 for filename in ('bindings.h','boss_bindings.h'):
  text=(root/'workshop/endgame'/filename).read_text()
  for group in re.findall(r'(?:MonsterClasses|BossClasses)\[\]\s*=\s*\{([^}]+)',text):
   ids.update(map(int,re.findall(r'\d+',group)))
 assert ids
 for folder in ('data/global/excel','data/global/excel/base'):
  p=root/folder/'monstats.txt';h,rows=read(p)
  found=0
  for r in rows:
   if r.get('*hcIdx','').isdigit() and int(r['*hcIdx']) in ids:
    found+=1
    # Succubus AI's ranged branch reads Skill5, not Skill2/Skill3 (curse slots).
    if r.get('AI')=='Succubus':
     r.update(Skill5='SuccubusBolt',Sk5mode='S2',Sk5lvl='1')
     for suffix in ('','(N)','(H)'):r['aip8'+suffix]='80'
    for k in ('aidist','aidist(N)','aidist(H)'):
     if k in r:r[k]='255'
  assert found==len(ids),(found,len(ids))
  write(p,h,rows)
 print('Trial private monsters: arena-wide awareness.')
