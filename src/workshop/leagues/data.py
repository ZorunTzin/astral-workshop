"""League content. Private IDs, native material storage, idempotent staging."""
from pathlib import Path
import json,sys,struct,copy
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from combat.port import read,write
from item_visuals import add_mappings
from PIL import Image
HERE=Path(__file__).resolve().parent
CONFIG=json.loads((HERE/'definitions.json').read_text())
MECHANICS=CONFIG['mechanics']
CODES=[[f'L{chr(110+m)}{i+1}' for i in range(4)] for m in range(3)]

def install(root):
 root=Path(root);assets=[];labels={};bindings=[]
 def save(rel,data):
  p=root/rel;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(data);assets.append(rel)
 def jsave(rel,obj):save(rel,json.dumps(obj,ensure_ascii=False,indent=4 if rel=='data/hd/missiles/missiles.json' else 2).replace('\n','\r\n').encode())
 def newrow(rows,h,key,value):
  r=next((r for r in rows if r[key]==value),None)
  if r is None:r={k:'' for k in h};r[key]=value;rows.append(r)
  return r
 for folder in ('data/global/excel','data/global/excel/base'):
  p=root/folder
  h,rows=read(p/'misc.txt');template=next(r for r in rows if r['code']=='Woa')
  for m,mechanic in enumerate(MECHANICS):
   for i,name in enumerate(mechanic['orbs']):
    code=CODES[m][i];r=newrow(rows,h,'code',code);r.update(template)
    for k in r:
     if k.endswith(('Min','Max','MagicLvl')):r[k]='0'
    r.update(code=code,normcode=code,ubercode=code,ultracode=code,name=name+' Orb',namestr=code,level='1',levelreq='1',cost='1',spawnable='0',PermStoreItem='0',multibuy='0',NightmareUpgrade=code,HellUpgrade=code,UICatOverride='wleague',AdvancedStashStackable='1',useable='0',pSpell='-1',compactsave='1',stackable='0',spelldesc='2',spelldescstr='League'+str(m),spelldescstr2='LeagueOrbHelp',invwidth='1',invheight='1')
    labels[code]='\u00ffc'+['5','2','1'][m]+name+' Orb\u00ffc0';labels['League'+str(m)]=mechanic['name']+' currency'
  labels['LeagueOrbHelp']='Automatically stored in League items.\nNo effect yet.';labels['wleague']='League items'
  write(p/'misc.txt',h,rows)
  h,rows=read(p/'itemuicategories.txt');r=newrow(rows,h,'Name','wleague');r.update(dict(zip(h,['wleague','0','misc','0','3'])));write(p/'itemuicategories.txt',h,rows)
  h,tcs=read(p/'treasureclassex.txt')
  for m in range(3):
   for rank in range(3):
    name=f'Ws League {m} {rank}';r=newrow(tcs,h,'Treasure Class',name);r.update({k:'' for k in h});r.update({'Treasure Class':name,'Picks':'1','NoDrop':str(4*[900,233,0][rank]+291),'*eol':'0'})
    for i,(code,weight) in enumerate(zip(CODES[m],[60,25,12,12]),1):r['Item'+str(i)]=code;r['Prob'+str(i)]=str(weight)
  write(p/'treasureclassex.txt',h,tcs)
  h,rows=read(p/'monstats.txt');by={r['Id']:r for r in rows};ids=[]
  nextid=max(int(r['*hcIdx']) for r in rows if r['*hcIdx'].isdigit())+1
  for m,mechanic in enumerate(MECHANICS):
   mid=[]
   for rank in range(3):
    rid=[]
    for variant,base in enumerate(mechanic['monsters']):
     key=f'wsleague{m}{rank}{variant}';old=by.get(key);r=by[base].copy();r.update(Id=key,BaseId=base,NextInClass='',NameStr=key,neverCount='1',primeevil='',quest='',minion1='',minion2='',PartyMin='0',PartyMax='0',MinGrp='1',MaxGrp='1',MonProp='',MonProp_N='',MonProp_H='',**{'*hcIdx':old['*hcIdx'] if old else str(nextid)})
     if not old:nextid+=1
     r['Velocity']=str(max(5,int(r.get('Velocity') or 5)));r['Run']=str(max(7,int(r.get('Run') or 7)))
     r['boss']='1' if rank==2 else '';r['isSpawn']='1';r['enabled']='1';r['Rarity']='0'
     for k in r:
      if k.startswith('TreasureClass'):r[k]='' if 'Quest' in k else f'Ws League {m} {rank}'
      if k in ('minHP','maxHP','MinHP(N)','MaxHP(N)','MinHP(H)','MaxHP(H)'):r[k]=str(max(1,int(r[k] or 100)*[100,250,800][rank]//100))
     r={k:v for k,v in r.items() if k in h}
     if old:old.update(r)
     else:rows.append(r)
     rid.append(int(r['*hcIdx']));labels[key]=['Night','Astral','Beyond'][m]+' '+['Shambler','Revenant','Horror'][variant]+['',' Champion',' Overlord'][rank]
    mid.append(rid)
   ids.append(mid)
  write(p/'monstats.txt',h,rows)
  # D2R has no Torch handler (slot 11 is null). SecretDoor (18) opens once without loot or quest effects.
  h,rows=read(p/'objects.txt');template=next(r for r in rows if r['*ID']=='80').copy();objects=[]
  for m in range(3):
   key='WsLeaguePillar'+str(m);old=next((r for r in rows if r['Name']==key),None);r=template.copy()
   r.update(Name=key,**{'*ID':old['*ID'] if old else str(max(int(x['*ID']) for x in rows if x['*ID'].isdigit())+1),'*Description':MECHANICS[m]['name'],'OperateFn':'18','InitFn':'0','PopulateFn':'0','Selectable0':'1','Selectable1':'0','Selectable2':'0','Restore':'0','RestoreVirgins':'0','ShrineFunction':'0','AutoMap':'0'})
   if old:old.update(r)
   else:rows.append(r)
   objects.append(int(r['*ID']));labels[key]=MECHANICS[m]['name']+'\n'+['Activate: survive the dead in darkness.','Defeat the defenders to strengthen the rifts.','Activate: killing enemies close together attracts monsters from beyond.'][m]
  write(p/'objects.txt',h,rows)
  h,rows=read(p/'states.txt');r=newrow(rows,h,'state','wsleaguenight')
  if not r.get('*ID'):r['*ID']=str(max(int(x['*ID']) for x in rows if x['*ID'].isdigit())+1)
  r.update(aura='1',remhit='',**{'*eol':'0'});dark=int(r['*ID']);write(p/'states.txt',h,rows)
  h,rows=read(p/'missiles.txt');old=next((r for r in rows if r['Missile']=='wsleague_rift'),None);r=next(r for r in rows if r['Missile']=='workshop_goblin_escape').copy()
  r.update(Missile='wsleague_rift',Range='4500',CelFile='wsleague_rift',**{'*ID':old['*ID'] if old else str(max(int(x['*ID']) for x in rows if x['*ID'].isdigit())+1)})
  if old:old.update(r)
  else:rows.append(r)
  rift=int(r['*ID']);write(p/'missiles.txt',h,rows);assets.append(folder+'/missiles.txt')
  bindings.append((ids,objects,dark,rift))
  assets += [folder+'/'+n+'.txt' for n in ('misc','itemuicategories','treasureclassex','monstats','objects','states')]
 assert bindings[0]==bindings[1]
 # Private model/variant definitions isolate colors from ordinary act monsters.
 registry=root/'data/hd/character/monsters.json';models=json.loads(registry.read_text(encoding='utf-8-sig'))
 for m,mechanic in enumerate(MECHANICS):
  for base in set(mechanic['monsters']):
   original=models[base];model=json.loads((HERE/'upstream'/(original+'.json')).read_text());variant=json.loads((HERE/'upstream'/(original+'_variant.json')).read_text())
   for entry in variant['entries']:
    if entry['name'].startswith('level'):
     for t in entry['transforms']:
      t['colorTint']=dict(zip(('x','y','z','w'),[*mechanic['color'],.90]));t['colorAdjustment']={'x':0,'y':-.5 if m==0 else .2,'z':-.35 if m==0 else 0,'w':1}
   rel=f'data/hd/character/enemy/wsleague{m}_{original}_variant.json';jsave(rel,variant)
   oldpath=f'data/hd/character/enemy/{original}/{original}_variant.json';model=json.loads(json.dumps(model).replace(oldpath,rel))
   model['name']=f'wsleague{m}_{original}';jsave('data/hd/character/enemy/'+model['name']+'.json',model)
   for rank in range(3):
    for v,b in enumerate(mechanic['monsters']):
     if b==base:models[f'wsleague{m}{rank}{v}']=model['name']
 jsave('data/hd/character/monsters.json',models)
 registry=json.loads((HERE/'upstream/objects.json').read_text())
 # Object registry keys are Class, not numeric IDs.
 source=registry.get('80',registry.get('Obelisk'))
 if source is None:
  source=registry['obelisk_1']
 for m in range(3):
  key='WsLeaguePillar'+str(m);registry[key]=source
  model=json.loads((HERE/'upstream/obelisk_1.json').read_text());model['name']=key
  jsave('data/hd/objects/'+source['asset_path']+'/'+key+'.json',model)
 jsave('data/hd/objects/objects.json',registry)
 visual=json.loads((HERE.parent/'goblins/escape_visual.json').read_text());green=json.loads((root/'data/hd/overlays/common/wsgoblinglow4.json').read_text())
 visual['name']='wsleague_rift';visual['dependencies']=green['dependencies'];visual['entities']=visual['entities'][:1]+green['entities'];visual['entities'][1]['name']='entity_rift'
 for comp in visual['entities'][1]['components']:
  if comp['type']=='TransformDefinitionComponent':comp['scale']={'x':2.4,'y':.4,'z':2.4}
 jsave('data/hd/missiles/wsleague_rift.json',visual)
 p=root/'data/hd/missiles/missiles.json';j=json.loads(p.read_text(encoding='utf-8-sig'));j['wsleague_rift']='wsleague_rift';jsave('data/hd/missiles/missiles.json',j)
 # Localization belongs in one shared string file; object/monster lookup uses same key registry.
 p=root/'data/local/lng/strings/item-names.json';strings=json.loads(p.read_text(encoding='utf-8-sig'));nextid=max(r['id'] for f in p.parent.glob('*.json') for r in json.loads(f.read_text(encoding='utf-8-sig')) if 'id' in r)+1
 for key,value in labels.items():
  r=next((r for r in strings if r['Key']==key),None)
  if r is None:r={'id':nextid,'Key':key};nextid+=1;strings.append(r)
  for lang in strings[0]:
   if lang not in ('id','Key'):r[lang]=value
 jsave('data/local/lng/strings/item-names.json',strings)
 from leagues.visuals import apply as orb_light,install as orb_lights
 orb_lights(root,save)
 mappings={}
 for m,mechanic in enumerate(MECHANICS):
  for i,code in enumerate(CODES[m]):
   name='workshop_league_'+mechanic['id']+str(i);im=Image.open(HERE/'art'/(mechanic['id']+str(i)+'.png')).convert('RGBA')
   for suffix,size in [('.sprite',98),('.lowend.sprite',49)]:
    icon=im.resize((size,size),Image.Resampling.LANCZOS);save('data/hd/global/ui/items/misc/uniquecharm/'+name+suffix,struct.pack('<4sHH8I',b'SpA1',31,size,size,size,0,1,0,0,0,0)+icon.tobytes())
   ground=orb_light(root,json.loads((HERE.parent/'integration/upstream/sigil/relic_of_the_ancients.json').read_text()),m);jsave('data/hd/items/misc/uniquecharm/'+name+'.json',ground);mappings[code]={'asset':'uniquecharm/'+name}
 add_mappings(root/'data/hd/items/items.json',mappings);assets.append('data/hd/items/items.json')
 from leagues.effects_data import install as effects
 assets+=effects(root)
 from leagues.crafting import install as crafting
 assets+=crafting(root)
 from leagues.stash import install as stash
 assets+=stash(root)
 here=root/'workshop/leagues';here.mkdir(parents=True,exist_ok=True)
 ids,objects,dark,rift=bindings[0]
 cpp=lambda x:json.dumps(x).replace('[','{').replace(']','}')
 (here/'bindings.h').write_text('#pragma once\nnamespace League { inline constexpr unsigned AreaChance='+str(CONFIG['areaChancePercent'])+',MaxLiving='+str(CONFIG['maxLiving'])+'; inline constexpr unsigned Durations[3]='+cpp([CONFIG['nightSeconds'],CONFIG['breachSeconds'],CONFIG['beyondSeconds']])+'; inline constexpr int Classes[3][3][3]='+cpp(ids)+'; inline constexpr int Pillars[3]='+cpp(objects)+'; inline constexpr int DarknessState='+str(dark)+'; inline constexpr int RiftMissile='+str(rift)+'; }\n')
 (here/'assets.json').write_text(json.dumps(sorted(set(assets)),indent=2))
 from leagues.balance import install as balance
 balance(root)
 from integration.prepare import install as pickup
 pickup(root,pickup_only=True)
 print('Leagues: 3 encounters, 27 private monster variants, 3 pillars, 12 stored currencies.')
if __name__=='__main__':install(sys.argv[1])

