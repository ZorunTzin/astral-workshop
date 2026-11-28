"""Idempotent release balance refresh for existing stable skill slots."""
from pathlib import Path
import json
from combat.port import read,write

def install(root):
 root=Path(root)
 for folder in ('data/global/excel','data/global/excel/base'):
  d=root/folder;h,skills=read(d/'skills.txt');by={int(r['*Id']):r for r in skills if r['*Id'].isdigit()}
  for i in range(1,7):
   if by[18].get('aurastat'+str(i)):by[18]['aurastatcalc'+str(i)]='10+lvl-1'
   stat=by[28].get('aurastat'+str(i),'')
   if stat:by[28]['aurastatcalc'+str(i)]='1+lvl-1' if stat=='item_crushingblow' else '10+lvl-1'
  # Raven AI looks up its owner's skill through MonStats.Skill1. Keep the
  # Alpha summons private so item-granted ordinary Ravens retain their source.
  mh,monsters=read(d/'monstats.txt');mb={r['Id']:r for r in monsters}
  raven=mb.get('workshopalpharaven')
  if raven is None:
   raven=mb['druidhawk'].copy();raven.update(Id='workshopalpharaven',**{'*hcIdx':str(max(int(r['*hcIdx']) for r in monsters if r['*hcIdx'].isdigit())+1),'NextInClass':''});monsters.append(raven)
  raven['Skill1']=by[224]['skill']
  by[224]['Param5']=by[221]['Param5'];by[224]['Param6']=by[221]['Param6']
  for r in skills:
   if r['skill']=='Workshop Wild Raven':r['summon']='workshopalpharaven'
  write(d/'monstats.txt',mh,monsters)
  write(d/'skills.txt',h,skills)
  h,desc=read(d/'skilldesc.txt');ds={r['skilldesc']:r for r in desc};avatar=ds[by[221]['skilldesc']]
  # Put Avatar into the elemental tree without occupying another skill's cell.
  occupied={(r['SkillRow'],r['SkillColumn']) for r in desc if r['SkillPage']=='3' and r is not avatar and r['skilldesc'] in {s['skilldesc'] for s in skills if s['charclass']=='dru'}}
  cell=next((str(row),str(col)) for row in (4,5,6,3,2,1) for col in (2,1,3) if (str(row),str(col)) not in occupied)
  avatar.update(SkillPage='3',SkillRow=cell[0],SkillColumn=cell[1])
  alpha=ds[by[224]['skilldesc']]
  # Transfer the two Raven lines from Avatar to Alpha, including their formulas.
  raven_lines=[]
  for prefix,count in (('desc',6),('dsc2',6),('dsc3',7)):
   for n in range(1,count+1):
    if avatar.get(prefix+'texta'+str(n)) in ('WorkshopDruid221Line5','WorkshopDruid221Line6'):
     fields=('line','texta','textb','calca','calcb')
     raven_lines.append({f:avatar.get(prefix+f+str(n),'') for f in fields})
     for f in fields:avatar[prefix+f+str(n)]=''
  for line in raven_lines:
   for n in range(1,7):
    if not alpha.get('dsc2line'+str(n)):
     for f,v in line.items():alpha['dsc2'+f+str(n)]=v
     break
  write(d/'skilldesc.txt',h,desc)
  # No vendor may sell Trial orbs, including inherited magic stock columns.
  from endgame.seal_data import CODES
  h,rows=read(d/'misc.txt')
  for r in rows:
   if r['code'] in CODES:
    for k in r:
     if k.startswith('Akara'):r[k]='0'
    r['PermStoreItem']='0'
  write(d/'misc.txt',h,rows)
 p=root/'data/hd/character/monsters.json';models=json.loads(p.read_text(encoding='utf-8-sig'));models['workshopalpharaven']=models['druidhawk'];p.write_text(json.dumps(models,ensure_ascii=False,indent=2),encoding='utf8')
 p=root/'data/local/lng/strings/skills.json';rows=json.loads(p.read_text(encoding='utf-8-sig'))
 for r in rows:
  for lang,value in list(r.items()):
   if lang in ('id','Key') or not isinstance(value,str):continue
   if r.get('Key')=='WorkshopDruid221Desc':r[lang]='Transform into a mighty Golem, granting various bonuses.'
   if 'Automatically summons one Raven every 5 seconds.' in value:r[lang]=value.replace(' Automatically summons one Raven every 5 seconds.','')
   if 'Reserve mana to strengthen all your summons' in value and 'Raven' not in value:r[lang]=value+' Summons one Raven every 5 seconds at this skill level.'
 p.write_text(json.dumps(rows,ensure_ascii=False,indent=2),encoding='utf8')
 print('Release balance: Covenant curves, elemental Avatar, Alpha Ravens and no Akara Trial orbs.')
