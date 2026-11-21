"""Give runtime-only helper skills valid, non-tree UI descriptions.

The hireling UI may query even mercprop and triggered skills. Empty links
produce native CmnAccess/HirelingInventoryPanel assertions. No skill function,
level, prerequisite, class, state or numeric binding is changed here.
"""
import json
from pathlib import Path
from combat.port import read,write

def install(root):
 root=Path(root);labels={}
 for folder in ('data/global/excel','data/global/excel/base'):
  p=root/folder;sh,skills=read(p/'skills.txt');dh,descs=read(p/'skilldesc.txt')
  attack=next(r for r in descs if r['skilldesc']=='attack')
  for row in skills:
   if not row['*Id'].isdigit() or int(row['*Id'])<532 or row['skilldesc']:continue
   skill=row['*Id'];key='WorkshopInternalSkill'+skill;name='workshop_internal_'+skill
   desc={k:'' for k in dh};desc.update(skilldesc=name,**{'str name':key,'str short':key,'str long':key,'str alt':key,'IconCel':attack['IconCel'],'SkillPage':'0','SkillRow':'0','SkillColumn':'0','ListRow':'0'})
   # Header spelling differs between source generations; keep only real fields.
   desc={k:desc.get(k,'') for k in dh};descs.append(desc);row['skilldesc']=name
   labels[key]=row['skill'].replace('Workshop Guardian ','Astral Guardian: ').replace('Workshop Legacy ','Legacy ').replace('Workshop Aura Level ','Aura level: ')
  write(p/'skills.txt',sh,skills);write(p/'skilldesc.txt',dh,descs)
 strings=root/'data/local/lng/strings/skills.json';rows=json.loads(strings.read_text(encoding='utf8'));known={r['Key'] for r in rows};nextid=max(int(r['id']) for f in strings.parent.glob('*.json') for r in json.loads(f.read_text(encoding='utf-8-sig')) if isinstance(r,dict) and 'id' in r)+1
 for key,label in labels.items():
  if key in known:continue
  row={k:label for k in rows[0] if k not in ('id','Key')};row.update(id=nextid,Key=key);rows.append(row);nextid+=1
 strings.write_text(json.dumps(rows,ensure_ascii=False,indent=2),encoding='utf8')
 for folder in ('data/global/excel','data/global/excel/base'):
  p=root/folder;_,skills=read(p/'skills.txt');_,descs=read(p/'skilldesc.txt');names={r['skilldesc'] for r in descs}
  assert all(r['skilldesc'] in names for r in skills if r['*Id'].isdigit() and int(r['*Id'])>=532)
