"""Amazonian Unity: appended stable IDs, private stationary Amazon actors."""
from pathlib import Path
import json
from combat.port import read,write
NAME='Amazonian Unity'

def install(root):
 root=Path(root);bindings=None
 for folder in ('data/global/excel','data/global/excel/base'):
  p=root/folder;tables={n:read(p/(n+'.txt')) for n in ('skills','skilldesc','monstats','monstats2','monequip','states','missiles')}
  def rows(n):return tables[n][1]
  def upsert(table,key,name,source,idkey=None):
   rs=rows(table);r=next((r for r in rs if r[key]==name),None)
   if r is None:
    r=source.copy()
    if idkey:r[idkey]=str(max(int(x[idkey]) for x in rs if x[idkey].isdigit())+1)
    rs.append(r)
   r[key]=name;return r
  by={r['skill']:r for r in rows('skills')};blank={k:'' for k in tables['skills'][0]}
  skill=upsert('skills','skill',NAME,blank,'*Id')
  skill.update(charclass='ama',skilldesc='workshop_amazonian_unity',srvdofunc='18',anim='SC',seqtrans='SC',monanim='SC',range='none',LineOfSight='0',reqlevel='30',maxlvl='25',reqskill1='Strafe',leftskill='0',rightskill='1',InTown='0',InGame='1',enhanceable='1',restrict='1',itypea1='bow',itypea2='xbow',mana='25',lvlmana='0',startmana='0',minmana='1',manashift='8',localdelay='250',globaldelay='0',SrcDam='128',HitShift='7',calc4='9+lvl',stsound='amazon_valkyrie_cast',**{'cost mult':'1','cost add':'1','*eol':'0'})
  synergy="(skill('Multiple Shot'.blvl)+skill('Guided Arrow'.blvl)+skill('Strafe'.blvl))*8"
  for k in ['MinDam','MaxDam']+[p+str(i) for p in ('MinLevDam','MaxLevDam') for i in range(1,6)]:
   skill[k]=str(int(by['Strafe'][k] or 0)//2 if k in ('MinDam','MaxDam') else 2*int(by['Strafe'][k] or 0))
  # Preserve half-point flat damage in the runtime snapshot correction.
  # Match native tier increments and synergy rounding before HitShift.
  def fixed_flat(prefix):
   terms=[skill[prefix+'Dam']]
   for i,(start,count) in enumerate(((1,7),(8,8),(16,6),(22,6),(28,None)),1):
    levels=f'max(lvl-{start},0)'
    if count is not None:levels=f'min({levels},{count})'
    terms.append(skill[prefix+'LevDam'+str(i)]+'*'+levels)
   return '(('+ '+'.join(terms)+')*(100+'+synergy+')/100)*128'
  skill.update(calc1=fixed_flat('Min'),calc2=fixed_flat('Max'),calc4='(9+lvl)*(100+'+synergy+')/100',DmgSymPerCalc=synergy)
  ds={r['skilldesc']:r for r in rows('skilldesc')}
  d=upsert('skilldesc','skilldesc',skill['skilldesc'],{k:'' for k in tables['skilldesc'][0]})
  # Dedicated normal/dim frames 66/67; prior skill artwork remains intact.
  d.update(SkillPage='1',SkillRow='6',SkillColumn='2',ListRow='1',IconCel='66',**{'str name':'WorkshopUnityName','str short':'WorkshopUnityDesc','str long':'WorkshopUnityDesc','str alt':'WorkshopUnityCharacter','*eol':'0'})
  for i,(key,formula) in enumerate([('WorkshopUnityDamage','clc4'),('WorkshopUnityCount','6'),('WorkshopUnityDuration','5'),('WorkshopUnityShots','10'),('WorkshopUnityCooldown','10'),('WorkshopUnityMana','25')],1):
   d.update({f'descline{i}':'36',f'desctexta{i}':key,f'desccalca{i}':formula,f'desccalcb{i}':''})
  d.update(dsc2line1='18',dsc2texta1='WorkshopUnityCadence',dsc2line2='18',dsc2texta2='WorkshopUnityTotal',dsc2line3='18',dsc2texta3='WorkshopUnityScaling',**{'descdam':'7','ddam calc1':'clc4-100','ddam calc2':'0','descatt':''})
  d.update(dsc2line4='75',dsc2texta4='StrSkillAddedFlatPhys',dsc2calca4='pnma',dsc2calcb4='pxma',dsc2line5='36',dsc2texta5='WorkshopUnitySplash',dsc2calca5='50')
  d.update(dsc3line1='40',dsc3texta1='Sksyn',dsc3textb1=d['str name'],dsc3calca1='2')
  for i,partner in enumerate(('Multiple Shot','Guided Arrow','Strafe'),2):
   d.update({f'dsc3line{i}':'76',f'dsc3texta{i}':'Damplev',f'dsc3textb{i}':ds[by[partner]['skilldesc']]['str name'],f'dsc3calca{i}':'8'})
  monsters={r['Id']:r for r in rows('monstats')};extras={r['Id']:r for r in rows('monstats2')}
  mon=upsert('monstats','Id','workshop_unity_amazon',monsters['valkyrie'],'*hcIdx')
  mon.update(BaseId=mon['Id'],NextInClass='',MonStatsEx=mon['Id'],NameStr='WorkshopUnityName',AI='Idle',Velocity='0',Run='0',Align='1',killable='0',neverCount='1',isSpawn='0',boss='0',primeevil='0',noRatio='1',inventory='1',switchai='0',MonProp='',MinGrp='1',MaxGrp='1')
  for k in mon:
   if k.startswith(('TreasureClass','Skill','Sk','aip','aidist')):mon[k]=''
   if k.startswith(('Exp','A1MinD','A1MaxD','A2MinD','A2MaxD','S1MinD','S1MaxD')):mon[k]='0'
  extra=upsert('monstats2','Id',mon['Id'],extras['valkyrie'],'*hcIdx')
  extra.update(BaseW='bow',SizeX='0',SizeY='0',soft='1',isSel='0',alSel='0',noSel='1',shiftSel='0',isAtt='0',corpseSel='0',objCol='0',deadCol='0',mWL='0',mRN='0',mA1='1',dA1='8',mNU='1',dNU='8',noMap='1')
  rows('monequip')[:]=[r for r in rows('monequip') if r['monster']!=mon['Id']]
  # Entries within a row are random alternatives, not simultaneous equipment.
  for item,slot in (('lbw','rarm'),('lea','tors')):
   eq={k:'' for k in tables['monequip'][0]};eq.update(monster=mon['Id'],oninit='1',level='0',item1=item,loc1=slot,mod1='2',**{'*eol':'0'});rows('monequip').append(eq)
  state=upsert('states','state','workshop_unity_amazon',next(r for r in rows('states') if r['state']=='valkyrie'),'*ID')
  state.update(gfxtype='2',gfxclass='0',overlay1='',castoverlay='',skill=NAME)
  missile=upsert('missiles','Missile','workshop_unity_arrow',next(r for r in rows('missiles') if r['Missile']=='arrow'),'*ID')
  missile.update(Skill=NAME,SrcDamage='128',ClientSend='0',ToHit='0',Pierce='0',NextHit='0',NextDelay='0',CollideKill='1',Range='16',Vel='24',MaxVel='24',VelLev='0',pSrvHitFunc='',pSrvDmgFunc='',pCltHitFunc='',EType='')
  current={'Skill':int(skill['*Id']),'Class':int(mon['*hcIdx']),'State':int(state['*ID']),'Arrow':int(missile['*ID'])}
  assert bindings is None or bindings==current;bindings=current
  for name,(header,rs) in tables.items():write(p/(name+'.txt'),header,rs)
 out=root/'workshop/combat';out.mkdir(parents=True,exist_ok=True)
 (out/'amazon_unity_bindings.h').write_text(''.join(f'inline constexpr int {k}={v};\n' for k,v in bindings.items()))
 for path,key,value in [('data/hd/character/monsters.json','workshop_unity_amazon','valkyrie'),('data/hd/missiles/missiles.json','workshop_unity_arrow','arrow')]:
  p=root/path;data=json.loads(p.read_text(encoding='utf-8-sig'));data[key]=value;p.write_text(json.dumps(data,ensure_ascii=False,indent=4),encoding='utf8')
 p=root/'data/local/lng/strings/skills.json';rs=json.loads(p.read_text(encoding='utf-8-sig'));last=max(r['id'] for f in p.parent.glob('*.json') for r in json.loads(f.read_text(encoding='utf-8-sig')) if 'id' in r)
 labels={'WorkshopUnitySplash':'Splash: %d%% (4 yards)','WorkshopUnityName':NAME,'WorkshopUnityCharacter':NAME+' (per arrow)','WorkshopUnityMana':'Mana Cost: %d','WorkshopUnityTotal':'60 arrows per cast.','WorkshopUnityScaling':'+1% weapon damage per level.','WorkshopUnityDesc':'Summon six spirits around you.\nEach attacks its nearest enemy.\nUntargetable and invulnerable.\nCannot move or block paths.\nRequires a bow or crossbow.','WorkshopUnityDamage':'Damage per arrow: %d%%','WorkshopUnityCount':'Amazons: %d','WorkshopUnityDuration':'Duration: %d seconds','WorkshopUnityShots':'Arrows per Amazon: %d','WorkshopUnityCooldown':'Cooldown: %d seconds','WorkshopUnityCadence':'Spirits fire in turn around the ring.'}
 for key,text in labels.items():
  r=next((r for r in rs if r['Key']==key),None)
  if r is None:last+=1;r={'id':last,'Key':key};rs.append(r)
  for lang in rs[0]:
   if lang not in ('id','Key'):r[lang]=text
 p.write_text(json.dumps(rs,ensure_ascii=False,indent=2),encoding='utf8')
 print('Amazonian Unity:',bindings,'level 30, six fixed archers, 60 arrows / five seconds, ten-second cooldown.')
