"""Idempotent bow balance: paired melee curves, mutual synergies and Strafe splash."""
from pathlib import Path
import json
from combat.port import read,write

def install(root):
 root=Path(root);binding=None
 for folder in ('data/global/excel','data/global/excel/base'):
  p=root/folder;h,rows=read(p/'skills.txt');dh,ds=read(p/'skilldesc.txt');mh,ms=read(p/'missiles.txt')
  by={r['skill']:r for r in rows};desc={r['skilldesc']:r for r in ds};miss={r['Missile']:r for r in ms}
  pairs=[('Multiple Shot','Jab',6),('Guided Arrow','Impale',5),('Strafe','Fend',7)]
  for name,source,shift in pairs:
   s=by[name];d=desc[s['skilldesc']]
   for k in ['MinDam','MaxDam']+[prefix+str(i) for prefix in ('MinLevDam','MaxLevDam') for i in range(1,6)]:s[k]=by[source][k]
   others=[n for n,_,_ in pairs if n!=name]
   rate=8 if name=='Guided Arrow' else 12
   synergy='('+'+'.join("skill('%s'.blvl)"%n for n in others)+')*'+str(rate)
   s.update(manashift=str(shift),DmgSymPerCalc=synergy,HitShift=by[source]['HitShift'])
   if name=='Multiple Shot':
    s['calc5']=synergy;d['ddam calc1']='clc5';d['dsc2calca1']='75+clc5*96/128'
   elif name=='Guided Arrow':
    s['calc4']='ln34+'+synergy
    s['HitShift']='0'
    for k in ['MinDam','MaxDam']+[prefix+str(i) for prefix in ('MinLevDam','MaxLevDam') for i in range(1,6)]:s[k]=str(int(by[source][k] or 0)*256*4//5)
   else:
    s.update(calc2='ln12+'+synergy,Param8='12',Param9='12',EDmgSymPerCalc="skill('Magic Arrow'.blvl)*12")
    d['ddam calc1']=d['desccalca1']='clc2'
    d.update(descline2='75',desctexta2='StrSkill39',desccalca2='enma',desccalcb2='exma')
   # Rebuild tooltips; physical partners and the existing magic synergy grant 12%.
   for k in d:
    if k.startswith('dsc3'):d[k]=''
   d.update(dsc3line1='40',dsc3texta1='Sksyn',dsc3textb1=d['str name'],dsc3calca1='2')
   for i,partner in enumerate(others,2):
    for key,value in [('line','76'),('texta','Damplev'),('textb',desc[by[partner]['skilldesc']]['str name']),('calca',str(rate))]:d['dsc3'+key+str(i)]=value
   if name=='Strafe':d.update(dsc3line4='76',dsc3texta4='Magdplev',dsc3textb4='skillname6',dsc3calca4='12')
   slot=5 if name=='Strafe' else 3
   d.update({f'descline{slot}':'75',f'desctexta{slot}':'StrSkillAddedFlatPhys',f'desccalca{slot}':'pnma',f'desccalcb{slot}':'pxma'})
   if name=='Strafe':d.update(dsc2line2='36',dsc2texta2='WorkshopStrafeSplash',dsc2calca2='50',dsc2line3='36',dsc2texta3='WorkshopStrafeRadius',dsc2calca3='4')
  # Link only primary missiles. Split children / returns inherit finalized damage.
  for name,skill in [('workshop_amazon_multiarrow','Multiple Shot'),('workshop_amazon_multibolt','Multiple Shot'),('workshop_amazon_split0','Guided Arrow')]:miss[name]['Skill']=skill
  splash=miss.get('workshop_strafe_splash')
  if splash is None:
   splash=miss['workshop_assassin_hit'].copy();splash['*ID']=str(max(int(r['*ID']) for r in ms if r['*ID'].isdigit())+1);ms.append(splash)
  splash.update(Missile='workshop_strafe_splash',Skill='',SrcDamage='0',ClientSend='0',HitSound='',EType='')
  current={'StrafeArrow':int(miss['strafearrow']['*ID']),'StrafeBolt':int(miss['strafebolt']['*ID']),'StrafeSplash':int(splash['*ID'])}
  assert binding is None or binding==current;binding=current
  write(p/'skills.txt',h,rows);write(p/'skilldesc.txt',dh,ds);write(p/'missiles.txt',mh,ms)
 out=root/'workshop/combat';out.mkdir(parents=True,exist_ok=True)
 (out/'amazon_bow_bindings.h').write_text(''.join(f'inline constexpr int {k}={v};\n' for k,v in binding.items()))
 p=root/'data/local/lng/strings/skills.json';rows=json.loads(p.read_text(encoding='utf-8-sig'))
 last=max(r['id'] for f in p.parent.glob('*.json') for r in json.loads(f.read_text(encoding='utf-8-sig')) if 'id' in r)
 for key,text in {'WorkshopStrafeSplash':'Splash damage: %d%%','WorkshopStrafeRadius':'Splash radius: %d yards'}.items():
  r=next((r for r in rows if r['Key']==key),None)
  if r is None:last+=1;r={'id':last,'Key':key};rows.append(r)
  for lang in rows[0]:
   if lang not in ('id','Key'):r[lang]=text
 p.write_text(json.dumps(rows,ensure_ascii=False,indent=2),encoding='utf8')
 print('Bow skills: melee flat curves, mutual 12% synergies, half mana and 50% Strafe splash.')
