"""Idempotent hireling skill compatibility and fire-tree ordering."""
from pathlib import Path
import json
from combat.port import read,write
HERE=Path(__file__).resolve().parent

def install(root):
 root=Path(root)
 legacy=json.loads((HERE/'integration/upstream/mercenary/inner-sight-original.json').read_text())
 for folder in ('data/global/excel','data/global/excel/base'):
  p=root/folder
  sh,skills=read(p/'skills.txt');dh,descs=read(p/'skilldesc.txt');hh,hirelings=read(p/'hireling.txt')
  by={r['skill']:r for r in skills};ds={r['skilldesc']:r for r in descs}
  name='Workshop Mercenary Inner Sight';descname='workshop mercenary inner sight'
  row=by.get(name)
  if row is None:
   row={k:'' for k in sh};row.update(legacy);row['*Id']=str(max(int(r['*Id']) for r in skills if r['*Id'].isdigit())+1);skills.append(row)
  row.update(skill=name,charclass='',skilldesc=descname,reqskill1='',reqskill2='',reqskill3='',monanim='SC')
  if descname not in ds:
   desc=ds['inner sight'].copy();desc.update(skilldesc=descname,SkillPage='0',SkillRow='0',SkillColumn='0');descs.append(desc)
  else:desc=ds[descname]
  for key in desc:
   if key.startswith(('descline','desctext','desccalc','dsc2','dsc3')):desc[key]=''
  desc.update({'str name':'WorkshopMercInnerSightName','str alt':'WorkshopMercInnerSightName','str short':'WorkshopMercInnerSightDesc','str long':'WorkshopMercInnerSightDesc'})
  for i,(key,calc) in enumerate((('Defense','edmn'),('Resistance','5+lvl'),('Duration','ln34/25'),('Radius','ln56*2/3')),1):
   desc.update({f'descline{i}':'36',f'desctexta{i}':'WorkshopMercInnerSight'+key,f'desccalca{i}':calc})
  for h in hirelings:
   for i in range(1,7):
    key='Skill'+str(i)
    if h.get('Act')=='1' and h.get(key)=='Inner Sight':h[key]=name
    if h.get('Act')=='2' and h.get(key)=='Impale':h[key]='Fend';h['Mode'+str(i)]='4'
  for name,level,rownum,prereq in [('Blaze',6,2,''),('Fire Wall',12,3,'Blaze'),('Inferno',18,4,'Fire Wall')]:
   r=by[name];r.update(reqlevel=str(level),reqskill1=prereq,reqskill2='',reqskill3='')
   ds[r['skilldesc']].update(SkillPage='1',SkillRow=str(rownum),SkillColumn='1')
  write(p/'skills.txt',sh,skills);write(p/'skilldesc.txt',dh,descs);write(p/'hireling.txt',hh,hirelings)
 path=root/'data/local/lng/strings/skills.json';strings=json.loads(path.read_text(encoding='utf-8-sig'))
 texts={'WorkshopMercInnerSightName':'Inner Sight','WorkshopMercInnerSightDesc':'Illuminates nearby enemies and temporarily reduces their defense and physical resistance.',
  'WorkshopMercInnerSightDefense':'Enemy defense: -%d','WorkshopMercInnerSightResistance':'Enemy physical resistance: -%d%%',
  'WorkshopMercInnerSightDuration':'Duration: %d seconds','WorkshopMercInnerSightRadius':'Radius: %d yards'}
 german={'WorkshopMercInnerSightName':'Innere Sicht','WorkshopMercInnerSightDesc':'Erhellt Gegner in der Nähe und senkt vorübergehend ihre Verteidigung und ihren physischen Widerstand.',
  'WorkshopMercInnerSightDefense':'Gegnerische Verteidigung: -%d','WorkshopMercInnerSightResistance':'Physischer Gegnerwiderstand: -%d%%',
  'WorkshopMercInnerSightDuration':'Dauer: %d Sekunden','WorkshopMercInnerSightRadius':'Radius: %d Yards'}
 # Localization IDs are global across ALL string files, not local to skills.json.
 used=set()
 for other in path.parent.glob('*.json'):
  for entry in json.loads(other.read_text(encoding='utf-8-sig')):
   if other!=path or entry.get('Key') not in texts:used.add(entry['id'])
 next_id=max(used)+1
 for key,text in texts.items():
  row=next((r for r in strings if r.get('Key')==key),None)
  if row is None:
   row={'Key':key};strings.append(row)
  if row.get('id') is None or row['id'] in used:
   while next_id in used:next_id+=1
   row['id']=next_id;next_id+=1
  used.add(row['id'])
  for lang in ('enUS','zhTW','deDE','esES','frFR','itIT','koKR','plPL','esMX','jaJP','ptBR','ruRU','zhCN'):row[lang]=text
  row['deDE']=german[key]
 path.write_text(json.dumps(strings,ensure_ascii=False,indent=2),encoding='utf-8')
 print('Mercenary legacy Inner Sight, native Fend, and Blaze -> Fire Wall -> Conflagration installed.')

if __name__=='__main__':
 import sys
 install(sys.argv[1])
