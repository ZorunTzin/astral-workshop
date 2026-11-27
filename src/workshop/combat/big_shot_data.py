"""Big Shot prototype: independent skill and private carrier/nova/hit missiles."""
from pathlib import Path
import json
from combat.port import read,write
NAME='Big Shot'

def install(root):
 root=Path(root);bindings=None
 for folder in ('data/global/excel','data/global/excel/base'):
  p=root/folder;sh,ss=read(p/'skills.txt');dh,ds=read(p/'skilldesc.txt');mh,ms=read(p/'missiles.txt')
  def put(rows,key,name,source,idkey=None):
   r=next((r for r in rows if r[key]==name),None)
   if r is None:
    r=source.copy()
    if idkey:r[idkey]=str(max(int(x[idkey]) for x in rows if x[idkey].isdigit())+1)
    rows.append(r)
   r[key]=name;return r
  skill=put(ss,'skill',NAME,{k:'' for k in sh},'*Id')
  skill.update(charclass='ama',skilldesc='workshop_big_shot',srvdofunc='18',anim='A1',seqtrans='A1',monanim='A1',range='none',LineOfSight='0',reqlevel='30',maxlvl='25',reqskill1='Strafe',leftskill='1',rightskill='1',InTown='0',InGame='1',enhanceable='1',restrict='1',itypea1='bow',itypea2='xbow',mana='20',lvlmana='1',startmana='0',minmana='1',manashift='7',localdelay='0',globaldelay='0',SrcDam='128',HitShift='8',calc4='95+5*lvl',stsound='',**{'cost mult':'1','cost add':'1','*eol':'0'})
  unity=next(r for r in ss if r['skill']=='Amazonian Unity')
  for k in ['MinDam','MaxDam','HitShift','DmgSymPerCalc','calc1','calc2']+[p+str(i) for p in ('MinLevDam','MaxLevDam') for i in range(1,6)]:skill[k]=unity[k]
  skill['calc3']='(95+5*lvl)*(100+'+unity['DmgSymPerCalc']+')/100'
  skill['calc4']='clc3/3'
  d=put(ds,'skilldesc',skill['skilldesc'],{k:'' for k in dh})
  d.update(SkillPage='1',SkillRow='6',SkillColumn='3',ListRow='1',IconCel='68',descdam='7',**{'str name':'WorkshopBigShotName','str short':'WorkshopBigShotDesc','str long':'WorkshopBigShotDesc','str alt':'WorkshopBigShotCharacter','ddam calc1':'clc4-100','ddam calc2':'0','*eol':'0'})
  for i,(label,formula) in enumerate((('Damage','clc4'),('Nova','50'),('Radius','6'),('Duration','5'),('Cooldown','0'),('Mana','usmc')),1):
   d.update({f'descline{i}':'36',f'desctexta{i}':'WorkshopBigShot'+label,f'desccalca{i}':formula})
  d.update(desccalcb6='256') # Native fixed-point mana display includes half points.
  d.update(dsc2line1='18',dsc2texta1='WorkshopBigShotCadence',dsc2line2='18',dsc2texta2='WorkshopBigShotScaling')
  d.update(dsc2line3='75',dsc2texta3='StrSkillAddedFlatPhys',dsc2calca3='pnma',dsc2calcb3='pxma',dsc3line1='40',dsc3texta1='Sksyn',dsc3textb1=d['str name'],dsc3calca1='2')
  skills={r['skill']:r for r in ss};descs={r['skilldesc']:r for r in ds}
  for i,partner in enumerate(('Multiple Shot','Guided Arrow','Strafe'),2):
   d.update({f'dsc3line{i}':'76',f'dsc3texta{i}':'Damplev',f'dsc3textb{i}':descs[skills[partner]['skilldesc']]['str name'],f'dsc3calca{i}':'8'})
  by={r['Missile']:r for r in ms}
  carrier=put(ms,'Missile','workshop_big_shot',by['arrow'],'*ID')
  carrier.update(pSrvDoFunc='1',pCltDoFunc='1',pSrvHitFunc='',pCltHitFunc='',pSrvDmgFunc='',Skill=NAME,SrcDamage='128',ClientSend='0',Vel='6',MaxVel='6',VelLev='0',Range='125',CollideType='6',CollideKill='1',ClientCol='1',ToHit='0',Size='1',NextHit='0',NextDelay='0',EType='',CanSlow='0')
  ring=put(ms,'Missile','workshop_big_shot_nova',by['workshop_stomp_ring'],'*ID')
  ring.update(Skill='',SrcDamage='0',ClientSend='0',CollideType='0',ClientCol='0',CollideKill='0',Range='8',Vel='24',MaxVel='24',VelLev='0',ToHit='0',EType='')
  hit=put(ms,'Missile','workshop_big_shot_hit',by['workshop_assassin_hit'],'*ID')
  # Native weapon-hit initialization supplies leech and item on-hit metadata.
  hit.update(Skill=NAME,SrcDamage='128',ClientSend='0',ToHit='0',NextHit='0',NextDelay='0',EType='',ResultFlags='4',HitFlags='0',HitClass='10',ReturnFire='1',GetHit='1',HitSound='',TravelSound='',ExplosionMissile='')
  current={'Skill':int(skill['*Id']),'Arrow':int(carrier['*ID']),'Nova':int(ring['*ID']),'HitMissile':int(hit['*ID'])}
  assert bindings is None or bindings==current;bindings=current
  write(p/'skills.txt',sh,ss);write(p/'skilldesc.txt',dh,ds);write(p/'missiles.txt',mh,ms)
 out=root/'workshop/combat';out.mkdir(parents=True,exist_ok=True)
 (out/'big_shot_bindings.h').write_text(''.join(f'inline constexpr int {k}={v};\n' for k,v in bindings.items()))
 p=root/'data/hd/missiles/missiles.json';data=json.loads(p.read_text(encoding='utf-8-sig'));data.update(workshop_big_shot='workshop_big_shot',workshop_big_shot_nova=data.get('workshop_stomp_ring','warcry'),workshop_big_shot_hit='null');p.write_text(json.dumps(data,ensure_ascii=False,indent=4),encoding='utf8')
 # Widen the existing arrow eightfold across X; keep its Z length and height.
 p=root/'data/hd/missiles/workshop_split_arrow.json';art=json.loads(p.read_text(encoding='utf-8-sig'));art['name']='workshop_big_shot'
 for entity in art['entities']:
  for component in entity['components']:
   if component['type']=='TransformDefinitionComponent':component['scale']={'x':32.0,'y':4.0,'z':6.0}
 (p.parent/'workshop_big_shot.json').write_text(json.dumps(art,indent=2),encoding='utf8')
 manifest=out/'assets.json';assets=json.loads(manifest.read_text());assets.append('data/hd/missiles/workshop_big_shot.json');manifest.write_text(json.dumps(list(dict.fromkeys(assets)),indent=2))
 p=root/'data/local/lng/strings/skills.json';rs=json.loads(p.read_text(encoding='utf-8-sig'));last=max(r['id'] for f in p.parent.glob('*.json') for r in json.loads(f.read_text(encoding='utf-8-sig')) if 'id' in r)
 labels={'Name':NAME,'Character':'Big Shot (arrow hit)','Desc':'Fire a massive, slow arrow.\nPierces enemies along its path.\nReleases physical damage novae.\nStops at walls. Requires a bow.','Damage':'Arrow weapon damage: %d%%','Nova':'Nova damage: %d%% of arrow','Radius':'Nova radius: %d yards','Duration':'Flight duration: %d seconds','Cooldown':'Cooldown: %d seconds','Mana':'Mana Cost: %d','Cadence':'A nova every 0.5 seconds.','Scaling':'+1.67% weapon damage per level.'}
 for suffix,text in labels.items():
  key='WorkshopBigShot'+suffix;r=next((r for r in rs if r['Key']==key),None)
  if r is None:last+=1;r={'id':last,'Key':key};rs.append(r)
  for lang in rs[0]:
   if lang not in ('id','Key'):r[lang]=text
 p.write_text(json.dumps(rs,ensure_ascii=False,indent=2),encoding='utf8')
 print('Big Shot:',bindings,'slow piercing arrow, half-second novae, no cooldown.')
