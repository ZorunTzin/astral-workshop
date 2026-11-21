"""Arena data: private square preset, five Hell-only seals, isolated wave monsters."""
from pathlib import Path
import json,struct,collections
from combat.port import read,write

def make_map(source,target):
 # DS1 v18 layout: wall/orientation pairs, floors, shadow, objects, NPC paths.
 # Format reference: OpenDiablo2 d2common/d2fileformats/d2ds1 (MIT).
 b=source.read_bytes();v,w,h,act,sub,n=struct.unpack_from('<6I',b);assert v==18 and sub==0
 off=24
 for _ in range(n):off=b.index(0,off)+1
 nw,nf=struct.unpack_from('<2I',b,off);off+=8;layers=[];count=(w+1)*(h+1)
 for _ in range(nw*2+nf+1):
  layers.append(struct.unpack_from('<'+str(count)+'I',b,off));off+=count*4
 floor=collections.Counter(x for x in layers[2*nw] if x).most_common(1)[0][0]
 walls={o:collections.Counter(a for a,c in zip(layers[0],layers[1]) if c==o and a).most_common(1)[0][0] for o in (1,2,3)}
 size=32;count=(size+1)**2;wall=[0]*count;orient=[0]*count;ground=[floor]*count
 for y in range(size+1):
  for x in range(size+1):
   k=y*(size+1)+x
   if x in (0,size) or y in (0,size):
    o=3 if x in (0,size) and y in (0,size) else 2 if x in (0,size) else 1
    wall[k]=walls[o];orient[k]=o
 out=struct.pack('<8I',18,size,size,2,0,0,1,1)
 for layer in (wall,orient,ground,[0]*count):out+=struct.pack('<'+str(count)+'I',*layer)
 out+=struct.pack('<2I',0,0) # No preplaced monsters, scenery, or NPC paths.
 target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(out)
 return {'size':size,'floor':floor,'walls':walls}

def make_hd_map(root):
 # HD resolves presets by the DS1 basename, not by LevelType. A DS1 alone
 # produces an empty scene definition and the native biome assertion on entry.
 source=root/'workshop/endgame/template-hd.json'
 scene=json.loads(source.read_text(encoding='utf-8-sig'))
 assert scene.get('biomeFilename') and scene.get('terrain')
 # Keep the upstream terrain/material dependencies, but remove dungeon props:
 # the trial's collision layout is an open square, with no interior obstacles.
 scene['entities']=[]
 scene['perTileBiomeOverrides']=[]
 scene['specialTiles']={}
 transform=next(c for c in scene['terrain']['components'] if c['type']=='TransformDefinitionComponent')
 transform['scale']['x']=transform['scale']['z']=32/40
 target=root/'data/hd/env/preset/workshop/astral-arena.json'
 target.parent.mkdir(parents=True,exist_ok=True)
 target.write_text(json.dumps(scene,separators=(',',':')),encoding='utf8')
 return str(target.relative_to(root)).replace('\\','/')

def install(root):
 root=Path(root);here=root/'workshop/endgame';assets=[];binding=None
 from endgame.generate_modifiers import generate
 generate(here)
 names=['Verdant Trial','Desert Trial','Jungle Trial','Infernal Trial','Frozen Trial']
 labels={};icons=['bks','vip','j34','dhn','bey']
 meta=make_map(here/'template.ds1',root/'data/global/tiles/workshop/astral-arena.ds1')
 assets.append('data/global/tiles/workshop/astral-arena.ds1')
 assets.append(make_hd_map(root))
 from endgame.panel_assets import install as install_panel
 assets.extend(install_panel(root))
 for directory in ('data/global/excel','data/global/excel/base'):
  p=root/directory
  h,levels=read(p/'levels.txt');first=max(int(r['Id']) for r in levels if r['Id'].isdigit())+1
  template=next(r for r in levels if r['Id']=='145')
  # Native automap persistence (93847 RVA 0xD5710) indexes 100 uint32s.
  # Layer 100 overwrites the stack cookie; additional levels must reuse
  # supported layers, as the upstream Labyrinth levels already do.
  # Distinct world offsets keep the trial rooms separate.
  arena_layers=(64,65,66,67,68)
  for i,name in enumerate(names):
   r=template.copy();r.update(Name='Workshop '+name,Id=str(first+i),Layer=str(arena_layers[i]),Act='4',Intensity='180',Red='180',Green='190',Blue='220',OffsetX=str(2000+i*100),OffsetY='2000',Portal='1',Position='0',SaveMonsters='1',NumMon='0',MonWndr='0',Waypoint='255',LevelName='WsArena'+str(i),LevelWarp='WsArena'+str(i),LevelEntry='WsArena'+str(i),PreventTownPortal='0')
   for k in r:
    if k.startswith(('SizeX','SizeY')):r[k]='32'
    if k.startswith(('MonDen','MonUMin','MonUMax','ObjGrp','ObjPrb','Vis','cpct','camt')):r[k]='0'
    if k.startswith('Warp') and k!='WarpDist':r[k]='-1'
    if k.startswith(('MonLvl','MonLvlEx')):r[k]='85'
    if k.startswith(('mon','nmon','umon','cmon')):r[k]=''
   levels.append(r);labels['WsArena'+str(i)]=name+' - Arena '+str(i+1)
  write(p/'levels.txt',h,levels)
  h,rows=read(p/'lvlprest.txt');template=next(r for r in rows if r['LevelId']=='145');idx=max(int(r['Def']) for r in rows if r['Def'].isdigit())+1
  for i,name in enumerate(names):
   r=template.copy();r.update(Name='Workshop '+name,Def=str(idx+i),LevelId=str(first+i),Populate='0',Files='1',File1='Workshop/astral-arena.ds1',SizeX='32',SizeY='32',KillEdge='1')
   for j in range(2,7):r['File'+str(j)]='0'
   rows.append(r)
  write(p/'lvlprest.txt',h,rows)
  # A dedicated non-usable misc type has a real vendor page; quest types do not.
  h,rows=read(p/'itemtypes.txt');t=next(r for r in rows if r['Code']=='stor').copy()
  t.update(ItemType='Astral Trial Seal',Code='wtrl',Equiv1='misc',Normal='1',StorePage='misc');rows.append(t);write(p/'itemtypes.txt',h,rows)
  h,rows=read(p/'misc.txt');by={r['code']:r for r in rows}
  for i,name in enumerate(names):
   r=by['isc'].copy();visual=by[icons[i]]
   for k in r:
    if k.endswith(('Min','Max','MagicLvl')):r[k]=''
   code='Wa'+str(i+1);r.update(code=code,normcode=code,name=name+' Seal',namestr=code,type='wtrl',type2='',useable='0',pSpell='',spawnable='1',cost='1',level='1',levelreq='1',PermStoreItem='1',multibuy='1',stackable='0',quest='',questdiffcheck='',AkaraMin='1',AkaraMax='1',AkaraMagicMin='0',AkaraMagicMax='0',AkaraMagicLvl='255',NightmareUpgrade=code,HellUpgrade=code)
   for k in ('invfile','flippyfile','invwidth','invheight'):r[k]=visual[k]
   r.update(spelldesc='1',spelldescstr='WsArenaHelp',spelldesccalc='')
   rows.append(r);labels[code]=name+' Seal (Tier '+str(i+1)+')'
  write(p/'misc.txt',h,rows)
  h,rows=read(p/'cubemain.txt')
  for i in range(5):
   r={k:'' for k in h};r.update(description='Workshop - Open Arena '+str(i+1),enabled='1',version='100',numinputs='1',**{'min diff':'2','input 1':'Wa'+str(i+1),'output':f'Red Portal,lvl={first+i},qty=1','*eol':'0'});rows.append(r)
  write(p/'cubemain.txt',h,rows)
  h,rows=read(p/'monstats.txt');by={r['Id']:r for r in rows};idx=max(int(r['*hcIdx']) for r in rows if r['*hcIdx'].isdigit())+1;mon=[]
  for i,base in enumerate(json.loads((root/'workshop/endgame/enemy_pool.json').read_text())):
   r=by[base].copy();r.update(Id='wsarenamon'+str(i),**{'*hcIdx':str(idx+i),'NextInClass':'','NameStr':'WsArenaMonster'+str(i),'boss':'','primeevil':'','npc':'','neverCount':'1','minion1':'','minion2':'','PartyMin':'0','PartyMax':'0','MinGrp':'1','MaxGrp':'1','Level':'85','Level(N)':'85','Level(H)':'85'})
   for k in r:
    if k.startswith('Exp'):r[k]='0'
   rows.append(r);mon.append(idx+i);labels['WsArenaMonster'+str(i)]=base.replace('_',' ').title()
  write(p/'monstats.txt',h,rows)
  h,rows=read(p/'states.txt');state=max(int(r['*ID']) for r in rows if r['*ID'].isdigit())+1
  r={k:'' for k in h};r.update(state='ws_arena_blessing',**{'*ID':str(state),'remhit':'0','nosend':'','*eol':'0'});rows.append(r);write(p/'states.txt',h,rows)
  current=(first,state,mon)
  if binding:assert binding==current
  binding=current
  assets += [directory+'/'+n+'.txt' for n in ('levels','lvlprest','misc','cubemain','monstats','states','itemtypes')]
 # HD monster lookup uses the private MonStats Id, not the copied graphics code.
 p=root/'data/hd/character/monsters.json';models=json.loads(p.read_text(encoding='utf-8-sig'))
 for i,base in enumerate(json.loads((root/'workshop/endgame/enemy_pool.json').read_text())):
  models['wsarenamon'+str(i)]=models[base]
 p.write_text(json.dumps(models,ensure_ascii=False,indent=2),encoding='utf8')
 assets.append('data/hd/character/monsters.json')
 labels['WsArenaHelp']='Hell only - Harrogath (Act V)\nCube this seal alone to open a Trial.\nOne run per tier per game.\nEnemies drop loot on death.'
 p=root/'data/local/lng/strings/item-names.json';rows=json.loads(p.read_text(encoding='utf-8-sig'));last=max(r['id'] for f in p.parent.glob('*.json') for r in json.loads(f.read_text(encoding='utf-8-sig')) if 'id' in r)
 for key,value in labels.items():
  last+=1;rows.append({'id':last,'Key':key,**{lang:value for lang in rows[0] if lang not in ('id','Key')}})
 p.write_text(json.dumps(rows,ensure_ascii=False,indent=2),encoding='utf8')
 # New item codes use existing five act-specific Reimagined summon-material art.
 p=root/'data/hd/items/items.json';items=json.loads(p.read_text(encoding='utf-8-sig'))
 for i,code in enumerate(icons):
  match=next((e for e in items if code in e),None)
  if match:items.append({'Wa'+str(i+1):match[code]})
 p.write_text(json.dumps(items,ensure_ascii=False,indent=2),encoding='utf8')
 first,state,mon=binding
 (here/'bindings.h').write_text(f'#pragma once\nnamespace Arena {{ constexpr int FirstLevel={first}, BuffState={state}; constexpr int MonsterClasses[]={{'+','.join(map(str,mon))+'}; }\n')
 from endgame.themes import install as install_themes
 assets.extend(install_themes(root))
 from endgame.loot_tables import install as install_loot
 assets.extend(install_loot(root))
 (here/'assets.json').write_text(json.dumps(assets))
 (here/'data.json').write_text(json.dumps({'version':'0.20.0','firstLevel':first,'state':state,'monsters':mon,'map':meta,'names':names,'sealCodes':['Wa'+str(i+1) for i in range(5)],'price':1,'hellOnly':True},indent=2))
 print('Arena: five Hell portals, square private map, Akara seals and expanded isolated wave monster pool.')
