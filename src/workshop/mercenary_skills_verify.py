"""Validate hireling tooltip localization and fire-tree prerequisites."""
import json,os
from pathlib import Path
from combat.port import read
root=Path(os.environ['WORKSHOP_MOD_ROOT'])
entries=[]
for p in (root/'data/local/lng/strings').glob('*.json'):
 entries.extend(json.loads(p.read_text(encoding='utf-8-sig')))
texts={r['Key']:r for r in entries}
for r in entries:
 if r['Key'].startswith('WorkshopMercInnerSight'):
  assert sum(e['id']==r['id'] for e in entries)==1, r['Key']
  assert r['enUS'] and r['deDE']
for folder in ('data/global/excel','data/global/excel/base'):
 p=root/folder;_,rows=read(p/'skills.txt');skills={r['skill']:r for r in rows};_,rows=read(p/'skilldesc.txt');descs={r['skilldesc']:r for r in rows}
 r=skills['Workshop Mercenary Inner Sight'];d=descs[r['skilldesc']]
 assert r['srvdofunc']=='6' and r['auralencalc']=='ln34'
 for field in ('str name','str alt','str short','str long'):assert d[field] in texts
 for i,calc in enumerate(('edmn','5+lvl','ln34/25','ln56*2/3'),1):
  assert d[f'desccalca{i}']==calc and d[f'desctexta{i}'] in texts
 for name,req,level,row in [('Blaze','',6,2),('Fire Wall','Blaze',12,3),('Inferno','Fire Wall',18,4)]:
  s=skills[name];assert s['reqskill1']==req and s['reqlevel']==str(level) and descs[s['skilldesc']]['SkillRow']==str(row)
print('PASS: globally unique mercenary tooltip IDs, all localized effect lines, native debuff and fire-tree chain.')
