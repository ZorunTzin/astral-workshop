"""Import selected Frankenstein visuals/monster definitions; Workshop owns spawns and loot."""
from pathlib import Path
import json, hashlib, shutil, sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from combat.port import read,write
HERE=Path(__file__).resolve().parent
NAMES=[('Treasure Goblin','Schatzgoblin'),('Gem Hoarder','Edelsteinhorter'),('Gilded Baron','Goldener Baron'),('Insufferable Miscreant','Runenräuber'),('Odious Collector','Materialsammler'),('Malevolent Tormentor','Bösartiger Peiniger'),('Gelatinous Sire','Gallertiger Stammvater'),('Gelatinous Goblin','Gallertiger Goblin'),('Small Gelatinous Goblin','Kleiner gallertiger Goblin')]
def install(root):
 root=Path(root);up=json.loads((HERE/'definitions.json').read_text());families=up['families'];assets=[]
 manifest=json.loads((HERE/'UPSTREAM.json').read_text())
 for rel,digest in manifest['files'].items():
  source=HERE/'upstream'/rel
  assert hashlib.sha256(source.read_bytes()).hexdigest()==digest,rel
  target=root/rel;target.parent.mkdir(parents=True,exist_ok=True)
  if target.exists():assert target.read_bytes()==source.read_bytes(),('Asset conflict',rel)
  shutil.copy2(source,target);assets.append(rel)
 bindings=[];portal_ids=[];state_ids=[]
 for folder in ('data/global/excel','data/global/excel/base'):
  p=root/folder
  ah,sounds=read(p/'sounds.txt');audio=json.loads((HERE/'audio_definitions.json').read_text())
  for source in audio:
   name='ws_'+source['Sound'];r=next((r for r in sounds if r['Sound']==name),None)
   if r is None:
    r={k:source.get(k,'') for k in ah};r['*Index']=str(max(int(x['*Index']) for x in sounds if x['*Index'].isdigit())+1);sounds.append(r)
   index=r['*Index'];r.update({k:source.get(k,'') for k in ah});r['*Index']=index
   r['Sound']=name;r['Redirect']='ws_'+source['Redirect'] if source['Redirect'] else '';r['FileName']='workshop/goblins/'+source['FileName'].replace(chr(92),'/').rsplit('/',1)[-1]
  write(p/'sounds.txt',ah,sounds);assets.append(folder+'/sounds.txt')
  if not (p/'monsounds.txt').exists():shutil.copy2(HERE/'monsounds-base.txt',p/'monsounds.txt')
  nh,profiles=read(p/'monsounds.txt');profile=next((r for r in profiles if r['Id']=='ws_goblin_voice'),None)
  if profile is None:profile={k:'' for k in nh};profiles.append(profile)
  profile.update(Id='ws_goblin_voice',HitSound='ws_goblin_voice_sd1',HitDelay='0',Footstep='ws_goblin_voice_sd1',FsCnt='5',FsOff='0',FsPrb='25',Neutral='ws_goblin_voice_sd1',NeuTime='150',Init='ws_goblin_voice_sd1',Taunt='ws_goblin_voice_sd1',Flee='ws_goblin_voice_sd1',EOL='0')
  write(p/'monsounds.txt',nh,profiles);assets.append(folder+'/monsounds.txt')
  mh,missiles=read(p/'missiles.txt');name='workshop_goblin_escape'
  portal=next((r for r in missiles if r['Missile']==name),None)
  if portal is None:
   portal={k:'' for k in mh};portal['*ID']=str(max(int(r['*ID']) for r in missiles if r['*ID'].isdigit())+1);missiles.append(portal)
  portal.update(Missile=name,pSrvDoFunc='1',pCltDoFunc='1',Range='85',LevRange='0',Vel='0',MaxVel='0',CollideType='0',ClientCol='0',ClientSend='1',SrcDamage='0',MinDamage='0',MaxDamage='0',EMin='0',EMax='0',LoopAnim='1',CelFile='groundFireBig',Trans='1',animrate='1024',AnimLen='37',AnimSpeed='16',NumDirections='1',**{'*eol':'0'})
  for key in list(portal):
   if key not in mh:del portal[key]
  portal_ids.append(int(portal['*ID']));write(p/'missiles.txt',mh,missiles);assets.append(folder+'/missiles.txt')
  sh,states=read(p/'states.txt');state=next((r for r in states if r['state']=='ws_goblin_channel'),None)
  if state is None:
   state={k:'' for k in sh};state.update(state='ws_goblin_channel',**{'*ID':str(max(int(r['*ID']) for r in states if r['*ID'].isdigit())+1)});states.append(state)
  state.update(aura='1',overlay1='workshopgoblinescape',**{'*eol':'0'});state_ids.append(int(state['*ID']));write(p/'states.txt',sh,states);assets.append(folder+'/states.txt')
  oh,overlays=read(p/'overlay.txt');overlay=next((r for r in overlays if r['overlay']=='workshopgoblinescape'),None)
  if overlay is None:
   overlay=next(r for r in overlays if r['overlay']=='battleorders').copy();overlay.update(overlay='workshopgoblinescape',Filename='workshopgoblinescape',**{'*ID':str(max(int(r['*ID']) for r in overlays if r['*ID'].isdigit())+1)});overlays.append(overlay)
  write(p/'overlay.txt',oh,overlays);assets.append(folder+'/overlay.txt')
  h,rows=read(p/'monstats2.txt')
  ex=next((r for r in rows if r['Id']=='ws_goblin_runner'),None)
  if ex is None:
   ex={k:up['monstats2'][0].get(k,'') for k in h};ex.update(Id='ws_goblin_runner',**{'*hcIdx':str(max(int(r['*hcIdx']) for r in rows if r['*hcIdx'].isdigit())+1),'revive':'0','corpseSel':'0'});rows.append(ex)
  ex.update(mRN='',isSel='1',isAtt='1')
  write(p/'monstats2.txt',h,rows)
  h,tcs=read(p/'treasureclassex.txt');existing={r['Treasure Class'] for r in tcs}
  def tc(name,items,rare=False,picks=4):
   r=next((r for r in tcs if r['Treasure Class']==name),None)
   if r is None:r={k:'' for k in h};tcs.append(r)
   r.update({k:'' for k in h});existing.add(name)
   r.update({'Treasure Class':name,'Picks':str(picks),'NoDrop':'0','Unique':'800' if rare else '0','Set':'700' if rare else '0','Rare':'900' if rare else '0','*eol':'0'})
   for i,(item,prob) in enumerate(items,1):
    assert item in existing or item in codes,(name,item)
    r['Item'+str(i)]=item;r['Prob'+str(i)]=str(prob)
  codes=set()
  for name in ('misc','weapons','armor'):
   _,items=read(p/(name+'.txt'));codes.update(r['code'] for r in items)
  tc('Ws Goblin Trial Orbs',[(c,1) for c in ('Woa','Wob','Woc','Wod','Woe','Wof','Wog')],picks=1)
  for diff,suffix in enumerate(('',' (N)',' (H)')):
   equip='Act 1'+suffix+' Equip A'
   gems=[('g'+grade+color,1) for color in ('v','y','b','g','r','w') for grade in [('s','l','p')[diff]]]
   gems=[("gzv" if code=="glv" else code,weight) for code,weight in gems]
   gems.append((("sku","skl","skz")[diff],1))
   rewards=[[(equip,5),('Jewelry A',1) ],gems,[('gld',1)],[('Runes '+str((5,10,15)[diff]),1)],[('Lab T'+str(diff+1)+' Materials',3),('Ws Goblin Trial Orbs',1)],[(equip,3),('Jewelry A',2)],[],[],[(equip,5),('Jewelry A',1)]]
   for i,f in enumerate(families):tc('Ws Goblin '+f+suffix,rewards[i],i in (0,5,8),picks=0 if i in (6,7) else 2 if i==8 else 4)
  write(p/'treasureclassex.txt',h,tcs)
  h,rows=read(p/'monstats.txt');ids=[]
  for i,f in enumerate(families):
   name='ws_goblin_'+f;r=next((r for r in rows if r['Id']==name),None)
   source=next(r for r in up['monstats'] if r['Id']=='fmmp_goblin_'+f)
   if r is None:
    r={k:source.get(k,'') for k in h};r['*hcIdx']=str(max(int(r['*hcIdx']) for r in rows if r['*hcIdx'].isdigit())+1);rows.append(r)
   r.update(Id=name,BaseId=name,NameStr='WsGoblin'+str(i),MonStatsEx='ws_goblin_runner',AI='Idle',MonSound='ws_goblin_voice',UMonSound='ws_goblin_voice',Velocity='11',Run='11',neverCount='1',ResFi='0',**{'ResFi(N)':'20','ResFi(H)':'40','Exp':'100','Exp(N)':'100','Exp(H)':'100','Skill1':'','Sk1mode':'','Sk1lvl':''})
   if i in (6,7):
    for k in ('Exp','Exp(N)','Exp(H)'):r[k]='0'
   if i in (7,8):
    for k in ('minHP','maxHP','MinHP(N)','MaxHP(N)','MinHP(H)','MaxHP(H)'):r[k]=str(max(1,int(source[k])*(60 if i==7 else 35)//100))
   # Keep native area/difficulty life scaling and ordinary death loot; no quest/pet flags.
   for diff,suffix in enumerate(('',' (N)',' (H)')):
    key=('','(N)','(H)')[diff]
    for field in h:
     if field.startswith('TreasureClass') and (field.endswith(key) if key else not field.endswith(('(N)','(H)'))):r[field]='Ws Goblin '+f+suffix
   ids.append(int(r['*hcIdx']))
  write(p/'monstats.txt',h,rows);bindings.append(ids)
  assets.extend(folder+'/'+n+'.txt' for n in ('monstats','monstats2','treasureclassex'))
 assert bindings[0]==bindings[1] and portal_ids[0]==portal_ids[1] and state_ids[0]==state_ids[1]
 registry=root/'data/hd/missiles/missiles.json';j=json.loads(registry.read_text(encoding='utf-8-sig'));j['workshop_goblin_escape']='workshop_goblin_escape';registry.write_text(json.dumps(j,indent=2))
 assets.append('data/hd/missiles/missiles.json')
 visual='data/hd/missiles/workshop_goblin_escape.json';shutil.copy2(HERE/'escape_visual.json',root/visual);assets.append(visual)
 p=root/'data/hd/character/monsters.json';j=json.loads(p.read_text(encoding='utf-8-sig'))
 for f in families:
  j['ws_goblin_'+f]='workshop_goblin_'+f
  model=json.loads((HERE/'upstream/data/hd/character/enemy'/('goblin_'+f+'.json')).read_text())
  for entity in model['entities']:
   if entity['name']=='entity_root':
    for component in entity['components']:
     if component['type']=='TransformDefinitionComponent':component['scale']={axis:value*1.5 for axis,value in component['scale'].items()}
  rel='data/hd/character/enemy/workshop_goblin_'+f+'.json';(root/rel).write_text(json.dumps(model,indent=2));assets.append(rel)
 p.write_text(json.dumps(j,indent=2),encoding='utf8');assets.append(str(p.relative_to(root)).replace('\\','/'))
 p=root/'data/local/lng/strings/monsters.json';j=json.loads(p.read_text(encoding='utf-8-sig'));locales=[k for k in j[0] if k not in ('id','Key')]
 allstrings=[x for file in p.parent.glob('*.json') for x in json.loads(file.read_text(encoding='utf-8-sig'))]
 used={x['id'] for x in allstrings if not x.get('Key','').startswith('WsGoblin')}
 nid=max(x['id'] for x in allstrings)+1
 for i,(en,de) in enumerate(NAMES):
  key='WsGoblin'+str(i);r=next((r for r in j if r['Key']==key),None)
  if r is None:r={'id':nid,'Key':key};nid+=1;j.append(r)
  if r['id'] in used:r['id']=nid;nid+=1
  used.add(r['id'])
  r.update({loc:de if loc=='deDE' else en for loc in locales})
 p.write_text(json.dumps(j,ensure_ascii=False,indent=2),encoding='utf8');assets.append('data/local/lng/strings/monsters.json')
 out=root/'workshop/goblins';out.mkdir(parents=True,exist_ok=True)
 overlay=json.loads((HERE/'escape_visual.json').read_text());overlay['type']='OverlayDefinition';overlay['name']='workshopgoblinescape';overlay['entities']=[e for e in overlay['entities'] if e['name']=='entity_vfx1'];overlay['entities'][0]['name']='entity_root'
 rel='data/hd/overlays/common/workshopgoblinescape.json';(root/rel).parent.mkdir(parents=True,exist_ok=True);(root/rel).write_text(json.dumps(overlay,indent=2));assets.append(rel)
 from goblins.glow import install as install_glow
 glow_assets,glow_ids=install_glow(root);assets.extend(glow_assets)
 (out/'bindings.h').write_text('#pragma once\nnamespace Goblins { inline constexpr int NaturalCount=7; inline constexpr int Classes[]={'+','.join(map(str,bindings[0]))+'}; inline constexpr int EscapePortal='+str(portal_ids[0])+'; inline constexpr int EscapeState='+str(state_ids[0])+'; inline constexpr int GlowStates[]={'+','.join(map(str,glow_ids))+'}; }\n')
 from goblins.mapsense import install as install_markers
 install_markers(root,bindings[0])
 (out/'assets.json').write_text(json.dumps(sorted(set(assets)),indent=2))
 print('Goblins: 7 encounter families,',len(families),'monster stages;',len(assets),'assets; classes',bindings[0])
if __name__=='__main__':install(sys.argv[1])
