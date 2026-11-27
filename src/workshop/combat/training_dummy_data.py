"""Private stationary training target; exact HP is assigned by runtime."""
from pathlib import Path
import json
from combat.port import read,write

def install(root):
 root=Path(root);binding=None
 for folder in ('data/global/excel','data/global/excel/base'):
  p=root/folder;h,rows=read(p/'monstats.txt');eh,extras=read(p/'monstats2.txt')
  source=next(r for r in rows if r['Id']=='zombie1');name='workshop_training_dummy'
  r=next((r for r in rows if r['Id']==name),None)
  if r is None:
   r=source.copy();r['*hcIdx']=str(max(int(x['*hcIdx']) for x in rows if x['*hcIdx'].isdigit())+1);rows.append(r)
  r.update(Id=name,BaseId=name,NextInClass='',MonStatsEx=name,NameStr='WorkshopTargetDummy',AI='Idle',Velocity='0',Run='0',Align='0',killable='1',neverCount='1',isSpawn='0',boss='0',primeevil='0',noRatio='1',inventory='0',switchai='0',MonProp='',MinGrp='1',MaxGrp='1',DamageRegen='0')
  for k in r:
   if k.startswith(('TreasureClass','Skill','Sk','aip','aidist')):r[k]=''
   if k.startswith(('Exp','Res','A1MinD','A1MaxD','A2MinD','A2MaxD','S1MinD','S1MaxD')):r[k]='0'
  e=next((e for e in extras if e['Id']==name),None)
  if e is None:
   e=next(e for e in extras if e['Id']==source['MonStatsEx']).copy();extras.append(e)
  e.update(Id=name,isSel='1',alSel='1',noSel='0',shiftSel='1',isAtt='1',corpseSel='0',mWL='0',mRN='0',mA1='0',mA2='0',mSC='0',mGH='0',mKB='0',noMap='0')
  current=int(r['*hcIdx']);assert binding is None or binding==current;binding=current
  write(p/'monstats.txt',h,rows);write(p/'monstats2.txt',eh,extras)
 out=root/'workshop/combat';out.mkdir(parents=True,exist_ok=True)
 (out/'training_dummy_bindings.h').write_text('inline constexpr int Class='+str(binding)+';\n')
 p=root/'data/hd/character/monsters.json';data=json.loads(p.read_text(encoding='utf-8-sig'));data['workshop_training_dummy']=data.get('zombie1','zombie');p.write_text(json.dumps(data,ensure_ascii=False,indent=4),encoding='utf8')
 p=root/'data/local/lng/strings/skills.json';rs=json.loads(p.read_text(encoding='utf-8-sig'));r=next((r for r in rs if r['Key']=='WorkshopTargetDummy'),None)
 if r is None:
  last=max(r['id'] for f in p.parent.glob('*.json') for r in json.loads(f.read_text(encoding='utf-8-sig')) if 'id' in r);r={'id':last+1,'Key':'WorkshopTargetDummy'};rs.append(r)
 for lang in rs[0]:
  if lang not in ('id','Key'):r[lang]='Target Dummy'
 p.write_text(json.dumps(rs,ensure_ascii=False,indent=2),encoding='utf8')
 print('Training dummy:',binding,'Blood Moor, 1,000,000 HP, ten-second full heal.')
