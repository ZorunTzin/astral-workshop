"""Bank-safe blank Trial seals; crafted variants remain individual saved items."""
from pathlib import Path
import json,struct,hashlib
from PIL import Image
from combat.port import read,write
from item_visuals import add_mappings
CODES=['Wt'+str(i) for i in range(1,6)]
THEMES=['forest','desert','jungle','hell','frozen']
def install(root):
 root=Path(root);assets=[]
 for folder in ('data/global/excel','data/global/excel/base'):
  p=root/folder;h,rows=read(p/'misc.txt')
  for i,code in enumerate(CODES,1):
   original=next(r for r in rows if r['code']=='Wa'+str(i))
   original.update(invwidth='1',invheight='1',AdvancedStashStackable='0',AkaraMin='0',AkaraMax='0',PermStoreItem='0')
   fresh=original.copy();fresh.update(code=code,normcode=code,ubercode=code,ultracode=code,namestr='Wa'+str(i),compactsave='1',AdvancedStashStackable='1',UICatOverride='wtor',AkaraMin='1',AkaraMax='1',PermStoreItem='1',NightmareUpgrade=code,HellUpgrade=code)
   old=next((r for r in rows if r['code']==code),None)
   if old is None:rows.append(fresh)
   else:old.update(fresh)
  write(p/'misc.txt',h,rows)
  h,rows=read(p/'cubemain.txt');rows=[r for r in rows if not r['description'].startswith('Workshop Blank Seal ')]
  for i,code in enumerate(CODES,1):
   portal=next(r for r in rows if r['description']=='Workshop - Open Arena '+str(i)).copy()
   portal.update(description='Workshop Blank Seal Portal '+str(i),**{'input 1':code});rows.append(portal)
   for orb in ('Awakening','Ascension','Corruption'):
    craft=next(r for r in rows if r['description']==f'Workshop Trial Craft {orb} {i} 0').copy()
    craft.update(description=f'Workshop Blank Seal {orb} {i}',output='Wa'+str(i),**{'input 1':code});rows.append(craft)
  write(p/'cubemain.txt',h,rows)
  h,rows=read(p/'treasureclassex.txt');by={r['Treasure Class']:r for r in rows}
  def tc(name,items,picks=1,nodrop=0):
   r={k:'' for k in h};r.update({'Treasure Class':name,'Picks':str(picks),'NoDrop':str(nodrop),'*eol':'0'})
   for j,(item,weight) in enumerate(items,1):r['Item'+str(j)]=item;r['Prob'+str(j)]=str(weight)
   if name in by:by[name].update(r)
   else:rows.append(r);by[name]=r
  # Same aggregate 0.1% material chance used by Trials, independently of main loot.
  tc('Workshop Trial Seal Drops',list(zip(CODES,[10,6,4,3,2])),nodrop=24975)
  mh,monsters=read(p/'monstats.txt')
  for monster in monsters:
   if monster['Id'].startswith('wsarena'):continue
   for k,v in monster.items():
    if not k.startswith('TreasureClass') or not k.endswith('(H)') or 'Quest' in k or not v:continue
    if v.startswith('Ws Seal World '):continue
    if v not in by:continue
    wrapper='Ws Seal World '+hashlib.sha1(v.encode()).hexdigest()[:12]
    tc(wrapper,[(v,1),('Workshop Trial Seal Drops',1)],picks=-2)
    monster[k]=wrapper
  write(p/'monstats.txt',mh,monsters)
  tc('Workshop Trial Orb Materials',[('Ws Trial Materials',1),('Workshop Trial Orbs',1),('Workshop Trial Seal Drops',1)],picks=-3)
  # Native TC loading resolves names in row order. Custom child branches must precede parents.
  by={r['Treasure Class']:r for r in rows};ordered=[];done=set();visiting=set()
  def emit(name):
   if name in done:return
   if name in visiting:raise ValueError('Treasure-class cycle: '+name)
   visiting.add(name);row=by[name]
   for j in range(1,11):
    child=row['Item'+str(j)]
    if child in by and child.startswith(('Workshop ', 'Ws ')):emit(child)
   visiting.remove(name);done.add(name);ordered.append(row)
  for row in rows:emit(row['Treasure Class'])
  write(p/'treasureclassex.txt',h,ordered)
  assets.extend(folder+'/'+n+'.txt' for n in ('misc','cubemain','monstats','treasureclassex'))
 mappings={}
 for i,theme in enumerate(THEMES,1):
  name='workshop_trial_seal_'+theme
  art=Image.open(Path(__file__).parent/'seal-art'/(theme+'.png')).convert('RGBA')
  for suffix,size in [('.sprite',98),('.lowend.sprite',49)]:
   im=art.resize((size,size),Image.Resampling.LANCZOS);rel='data/hd/global/ui/items/misc/uniquecharm/'+name+suffix
   p=root/rel;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(struct.pack('<4sHH8I',b'SpA1',31,size,size,size,0,1,0,0,0,0)+im.tobytes());assets.append(rel)
  rel='data/hd/items/misc/uniquecharm/'+name+'.json';p=root/rel
  p.write_bytes((root/'data/hd/items/misc/uniquecharm/workshop_trial_awakening.json').read_bytes());assets.append(rel)
  for code in ('Wa'+str(i),'Wt'+str(i)):mappings[code]={'asset':'uniquecharm/'+name}
 target=root/'data/hd/items/items.json'
 existing=json.loads(target.read_text(encoding='utf-8-sig'))
 for entry in existing:
  for code in mappings:entry.pop(code,None)
 target.write_text(json.dumps([e for e in existing if e],ensure_ascii=False,indent=2),encoding='utf8')
 add_mappings(target,mappings)
 assets.append('data/hd/items/items.json')
 return assets
