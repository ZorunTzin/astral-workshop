"""Idempotent melee balance and Jab bleed tooltip."""
from pathlib import Path
import json
from combat.port import read,write

def install(root):
 root=Path(root);helper_id=None
 for folder in ('data/global/excel','data/global/excel/base'):
  p=root/folder;h,rows=read(p/'skills.txt');dh,ds=read(p/'skilldesc.txt');by={x['skill']:x for x in rows}
  for name,param,value,mins,maxs in [('Jab','Param6',20,[1,2,3,6,8],[1,2,3,6,8]),('Impale','Param6',16,[1,2,4,6,12],[1,2,5,6,12]),('Fend','Param8',12,[1,2,4,6,12],[1,2,5,6,12])]:
   s=by[name];s.update({param:str(value),'MinDam':'2','MaxDam':'4'})
   for prefix,values in [('MinLevDam',mins),('MaxLevDam',maxs)]:
    for i,v in enumerate(values,1):s[prefix+str(i)]=str(v*2)
  by['Jab']['calc4']='min(100,9+lvl)'
  name='Workshop Jab Bleed Level'
  if name not in by:
   s=by['Attack'].copy();s['*Id']=str(max(int(x['*Id']) for x in rows if x['*Id'].isdigit())+1);rows.append(s);by[name]=s
  by[name].update(skill=name,charclass='',skilldesc='',SrcDam='0',calc4="skill('Jab'.lvl)")
  helper_id=int(by[name]['*Id'])
  d=next(x for x in ds if x['skilldesc']==by['Jab']['skilldesc'])
  d.update(descline5='36',desctexta5='WorkshopJabBleedChance',desccalca5='clc4')
  write(p/'skills.txt',h,rows);write(p/'skilldesc.txt',dh,ds)
 out=root/'workshop/combat';out.mkdir(parents=True,exist_ok=True)
 (out/'amazon_melee_bindings.h').write_text('inline constexpr int JabLevelSkill='+str(helper_id)+';\n')
 p=root/'data/local/lng/strings/skills.json';rows=json.loads(p.read_text(encoding='utf-8-sig'))
 key='WorkshopJabBleedChance';s=next((x for x in rows if x['Key']==key),None)
 if s is None:
  last=max(x['id'] for f in p.parent.glob('*.json') for x in json.loads(f.read_text(encoding='utf-8-sig')) if 'id' in x)
  s={'id':last+1,'Key':key};rows.append(s)
 for lang in rows[0]:
  if lang not in ('id','Key'):s[lang]='Chance to Cause Bleeding on Hit: %d%%'
 p.write_text(json.dumps(rows,ensure_ascii=False,indent=2),encoding='utf8')
 from combat.amazon_bow_data import install as bows
 bows(root)
 from combat.amazon_unity_data import install as unity
 unity(root)
 from combat.training_dummy_data import install as dummy
 dummy(root)
 from combat.big_shot_data import install as big_shot
 big_shot(root)
 from combat.amazon_split_extension import install as split_extension
 split_extension(root)
 from internal_skill_desc import install as internal
 internal(root)
 print('Amazon melee: doubled flat damage and synergies; Jab bleed 10% + 1% per additional level.')
