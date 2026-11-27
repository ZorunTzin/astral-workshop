"""Idempotent Trial seal crafting data; saved item layouts remain unchanged."""
from pathlib import Path
import json,struct,textwrap
from PIL import Image
from combat.port import read,write
from item_visuals import add_mappings
NAMES=['Awakening','Ascension','Renewal','Augmentation','Purification','Fortune','Corruption']
HELP=['Normal seal: add 1-2 modifiers (Magic).','Normal or Magic seal: reroll with 3-5 modifiers (Rare).','Reroll modifiers, keeping seal rarity.','Add a modifier: Magic maximum 2, Rare maximum 5.','Remove all modifiers and restore a Normal seal.','Reroll modifier tiers without changing their types.','Permanently corrupt a seal: add a Legendary modifier, +160% loot, or raise all modifier tiers. Cannot be crafted again.']
CODES=['Woa','Wob','Woc','Wod','Woe','Wof','Wog']
VALID=[[0],[0,11,12],[11,12,23,24,25],[11,23,24],[11,12,23,24,25],[11,12,23,24,25],[0,11,12,23,24,25]]
def install(root, *, refresh_ui=True):
 root=Path(root);assets=[];binding=None
 labels={'WsArenaHelp':'Hell only - Harrogath (Act V)\nCube this seal alone to open a Trial.\nOne run per tier per game.\nEnemies drop loot on death.','wtor':'Trial Orbs','WsTrialOrbHelp':'Transmute with one Trial Seal in the Horadric Cube. Corrupted seals cannot be crafted.'}
 for i,name in enumerate(('Verdant','Desert','Jungle','Infernal','Frozen'),1):labels['Wa'+str(i)]='\u00ffc;'+name+' Trial Seal (Tier '+str(i)+')\u00ffc0'
 for folder in ('data/global/excel','data/global/excel/base'):
  p=root/folder
  h,rows=read(p/'properties.txt')
  if not any(r['code']=='ws-trial-craft' for r in rows):
   r={k:'' for k in h};r.update(code='ws-trial-craft',func1='1',stat1='workshop_sector_0',**{'*Id':str(max(int(x['*Id']) for x in rows if x['*Id'].isdigit())+1),'*eol':'0'});rows.append(r)
  dispatch=int(next(r['*Id'] for r in rows if r['code']=='ws-trial-craft'))
  props=[int(next(r['*Id'] for r in rows if r['code']==f'ws-page{i}')) for i in range(8)]
  current=(dispatch,props);assert binding is None or binding==current;binding=current
  write(p/'properties.txt',h,rows)
  h,rows=read(p/'misc.txt');template=next(r for r in rows if r['code']=='oor').copy()
  for r in rows:
   if r['code'] in ('bag','1gc') or r.get('type')=='grab':
    for k in r:
     if k.endswith(('Min','Max','MagicLvl')) and k not in ('mindam','maxdam'):r[k]='0'
    r['PermStoreItem']='0'
   if r['code']=='1gc':r['spawnable']='0'
   if r['code'] in ['Wa1','Wa2','Wa3','Wa4','Wa5']:r['compactsave']='0';r['AdvancedStashStackable']='0'
  for i,n in enumerate(NAMES):
   code=CODES[i];r=next((r for r in rows if r['code']==code),None)
   if r is None:r=template.copy();rows.append(r)
   for k in r:
    if k.endswith(('Min','Max','MagicLvl')):r[k]='0'
   r.update(name='Trial Orb of '+n,namestr=code,code=code,normcode=code,type='elix',type2='',level='1',levelreq='1',cost='1',spawnable='1',useable='0',pSpell='-1',compactsave='1',stackable='0',multibuy='1',PermStoreItem='0',AkaraMin='0',AkaraMax='0',AkaraMagicLvl='0',NightmareUpgrade=code,HellUpgrade=code,UICatOverride='wtor',AdvancedStashStackable='1',invwidth='1',invheight='1',spelldesc='2',spelldescstr='WsTrialOrb'+str(i),spelldescstr2='WsTrialOrbHelp',spelldesccalc='',uniqueinvfile='')
   labels[code]='\u00ffc;Trial Orb of '+n+'\u00ffc0';labels['WsTrialOrb'+str(i)]=HELP[i]
  write(p/'misc.txt',h,rows)
  h,rows=read(p/'itemuicategories.txt');rows=[r for r in rows if r['Name']!='wtorb']
  if not any(r['Name']=='wtor' for r in rows):rows.append(dict(zip(h,['wtor','0','misc','0','3'])))
  write(p/'itemuicategories.txt',h,rows)
  h,rows=read(p/'cubemain.txt');rows=[r for r in rows if not r['description'].startswith('Workshop Trial Craft ')]
  for r in rows:
   if any(r.get(k,'').split(',')[0]=='1gc' for k in ('output','output b','output c')):r['enabled']='0'
  for orb,states in enumerate(VALID):
   for seal in range(1,6):
    for state in states:
     r={k:'' for k in h};r.update(description=f'Workshop Trial Craft {NAMES[orb]} {seal} {state}',enabled='1',version='100',op='18',param='444',value=str(state),numinputs='2',output='useitem',**{'input 1':f'Wa{seal}','input 2':CODES[orb],'mod 1':'ws-trial-craft','mod 1 chance':'100','mod 1 param':str(orb),'mod 1 min':'1','mod 1 max':'1','*eol':'0'});rows.append(r)
  write(p/'cubemain.txt',h,rows)
  # Match the aggregate chance of Herald Orbs, without replacing its existing
  # drops. The new branch has identical NoDrop and total item weight (4/394).
  h,rows=read(p/'treasureclassex.txt');by={r['Treasure Class']:r for r in rows}
  def tc(name,items,nodrop,picks=1):
   r=by.get(name)
   if r is None:r={k:'' for k in h};rows.append(r);by[name]=r
   r.update({'Treasure Class':name,'Picks':str(picks),'NoDrop':str(nodrop),'*eol':'0'})
   for j in range(1,11):r['Item'+str(j)]='';r['Prob'+str(j)]=''
   for j,(item,w) in enumerate(items,1):r['Item'+str(j)]=item;r['Prob'+str(j)]=str(w)
  tc('Workshop Trial Orbs',[(c,4) for c in CODES],8286)
  # Undo the old global wrapper on upgrades. Trial currencies are exclusive
  # to the explicit Trial-material branch; ordinary Herald drops stay intact.
  tc('Workshop Herald And Trial Orbs',[('Herald Orbs',1)],0,1)
  for r in rows:
   for j in range(1,11):
    if r['Item'+str(j)]=='Workshop Herald And Trial Orbs':r['Item'+str(j)]='Herald Orbs'
  # Trials' bonus-material branch can also produce orbs at the same rare rate.
  for r in rows:
   if r['Treasure Class']=='Workshop Trial Orb Materials':continue
   for j in range(1,11):
    if r['Item'+str(j)]=='Ws Trial Materials':r['Item'+str(j)]='Workshop Trial Orb Materials'
  trial_materials=[('Ws Trial Materials',1),('Workshop Trial Orbs',1)]
  if 'Workshop Trial Seal Drops' in by:trial_materials.append(('Workshop Trial Seal Drops',1))
  tc('Workshop Trial Orb Materials',trial_materials,0,-len(trial_materials))
  for r in rows:
   for j in range(1,11):
    if r['Item'+str(j)] in ('1gc','Gem Cluster'):r['Item'+str(j)]='';r['Prob'+str(j)]='0'
  write(p/'treasureclassex.txt',h,rows)
  assets += [folder+'/'+f+'.txt' for f in ('properties','misc','itemuicategories','cubemain','treasureclassex')]
 here=root/'workshop/endgame';here.mkdir(exist_ok=True)
 (here/'seal_bindings.h').write_text('#pragma once\nnamespace TrialCraft { inline constexpr int Dispatch='+str(binding[0])+'; inline constexpr int Properties[]={'+','.join(map(str,binding[1]))+'}; }\n')
 p=root/'data/local/lng/strings/item-names.json';rows=json.loads(p.read_text(encoding='utf-8-sig'));nextid=max(r['id'] for f in p.parent.glob('*.json') for r in json.loads(f.read_text(encoding='utf-8-sig')) if 'id' in r)+1
 for key,value in labels.items():
  row=next((r for r in rows if r['Key']==key),None)
  if row is None:row={'id':nextid,'Key':key};nextid+=1;rows.append(row)
  value='\n'.join(textwrap.fill(line,width=52) for line in value.split('\n'))
  for lang in rows[0]:
   if lang not in ('id','Key'):row[lang]=value
 p.write_text(json.dumps(rows,ensure_ascii=False,indent=2),encoding='utf8')
 mappings={}
 for name,code in zip(NAMES,CODES):
  base='workshop_trial_'+name.lower();original=Image.open(Path(__file__).parent/'orb-art'/ (name.lower()+'.png')).convert('RGBA')
  for suffix,size in [('.sprite',98),('.lowend.sprite',49)]:
   image=original.resize((size,size),Image.Resampling.LANCZOS)
   rel='data/hd/global/ui/items/misc/uniquecharm/'+base+suffix;p=root/rel;p.parent.mkdir(parents=True,exist_ok=True)
   p.write_bytes(struct.pack('<4sHH8I',b'SpA1',31,size,size,size,0,1,0,0,0,0)+image.tobytes());assets.append(rel)
  rel='data/hd/items/misc/uniquecharm/'+base+'.json';p=root/rel;p.parent.mkdir(parents=True,exist_ok=True);ground=json.loads((Path(__file__).parents[1]/'integration/upstream/sigil/relic_of_the_ancients.json').read_text());effects=json.loads((Path(__file__).parent/'stash-templates/jewel-drop-effects.json').read_text());ground['entities'].extend(effects)
  for entity in effects:
   for component in entity['components']:
    if component['type']=='VfxDefinitionComponent':
     dep={'path':component['filename']}
     if dep not in ground['dependencies']['particles']:ground['dependencies']['particles'].append(dep)
  p.write_text(json.dumps(ground,indent=2),encoding='utf8');assets.append(rel)
  mappings[code]={'asset':'uniquecharm/'+base}
 add_mappings(root/'data/hd/items/items.json',mappings)
 from endgame.seal_entries import install as install_entries
 assets.extend(install_entries(root))
 from endgame.boss_data import install as install_bosses
 assets.extend(install_bosses(root))
 from endgame.seal_stash import install as install_stash
 if refresh_ui:assets.extend(install_stash(root))
 assets+=['data/hd/items/items.json' ,'data/local/lng/strings/item-names.json']
 manifest=here/'assets.json';existing=json.loads(manifest.read_text());manifest.write_text(json.dumps(list(dict.fromkeys(existing+assets)),indent=2))
 print('Trial crafting: 7 orbs, saved seal recipes, Materials category, Trial-exclusive rare drop branch, no vendor stock and original HD art.')
if __name__=='__main__':
 import sys
 install(sys.argv[1])
