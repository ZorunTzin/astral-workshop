from pathlib import Path
import json,struct
from PIL import Image
from combat.port import read,write

def install(root):
 root=Path(root)
 for sub in ('','base'):
  p=root/'data/global/excel'/sub/'misc.txt';h,rows=read(p)
  if not any(r['code']=='Wnsk' for r in rows):
   r=next(x for x in rows if x['code']=='xyz').copy()
   r.update(name='Nightmare Legacy',code='Wnsk',namestr='Wnsk',compactsave='0',level='1',levelreq='1',spawnable='0',cost='0',useable='1',quest='',questdiffcheck='',type='elix',pSpell='0',spelldesc='1',spelldescstr='WsNightmareLegacyHelp',spelldescstr2='',stackable='0',AdvancedStashStackable='0',UsageConditionCalc='')
   for k in h:
    if k.endswith(('Min','Max','MagicLvl')) or k.startswith(('stat','calc')):r[k]=''
   rows.append(r)
  # Visual lookup uses the normal/exceptional/elite codes inherited from xyz.
  # Repair existing rows too, so already-created tokens gain the dedicated icon.
  token=next(r for r in rows if r['code']=='Wnsk')
  # Generic misc has no StorePage and is discarded by merchant stock creation.
  # Elixir supplies the Misc shop page, like the existing trial consumables.
  token.update(type='elix',alternategfx='spe',flippyfile='flpsple',invfile='invsple',normcode='Wnl',ubercode='Wnl',ultracode='Wnl')
  # Temporary local test stock; this flag also permits vendor-generated seeds.
  test_vendor='TestVendorEnabled=true' in Path(__file__).with_name('reward_item.h').read_text()
  token.update(cost='1' if test_vendor else '0',spawnable='1' if test_vendor else '0',
   PermStoreItem='1' if test_vendor else '0',AkaraMin='1' if test_vendor else '0',
   AkaraMax='1' if test_vendor else '0',AkaraMagicMin='0',AkaraMagicMax='0',AkaraMagicLvl='255',
   NightmareUpgrade='Wnsk',HellUpgrade='Wnsk')
  fresh=token.copy();fresh.update(code='Wnl',namestr='Wnsk',NightmareUpgrade='Wnl',HellUpgrade='Wnl')
  existing=next((r for r in rows if r['code']=='Wnl'),None)
  if existing is None:rows.append(fresh)
  else:existing.update(fresh)
  # Preserve old saved items, but only stock the standard three-letter version.
  token.update(AkaraMin='0',AkaraMax='0',PermStoreItem='0',spawnable='0')
  write(p,h,rows)
 p=root/'data/local/lng/strings/item-names.json';rows=json.loads(p.read_text(encoding='utf-8-sig'))
 labels={'Wnsk':'Nightmare Legacy','WsNightmareLegacyHelp':'Right-click in your inventory to consume.\n+4 Skill Points\n+5 Attribute Points\n+20 Maximum Life\n+10% to All Resistances'}
 nextid=max(x['id'] for f in p.parent.glob('*.json') for x in json.loads(f.read_text(encoding='utf-8-sig')) if 'id' in x)+1
 for key,value in labels.items():
  r=next((x for x in rows if x.get('Key')==key),None)
  if r is None:r={'id':nextid,'Key':key};rows.append(r);nextid+=1
  for loc in ('enUS','zhTW','deDE','esES','frFR','itIT','koKR','plPL','esMX','jaJP','ptBR','ruRU','zhCN'):r[loc]=value
 p.write_text(json.dumps(rows,ensure_ascii=False,indent=2),encoding='utf-8')
 p=root/'data/hd/items/items.json';data=json.loads(p.read_text(encoding='utf-8-sig'))
 # A dedicated inventory sprite; preserve transparent alpha in both HD sizes.
 asset='uniquecharm/workshop_nightmare_legacy'
 entry=next((x for x in data if 'Wnsk' in x),None)
 if entry is None:entry={};data.append(entry)
 entry['Wnsk']={'asset':asset}
 # Also cover the native three-character inventory visual lookup for this
 # four-character custom item code; preserve the full code for saved items.
 alias=next((x for x in data if 'Wns' in x),None)
 if alias is None:alias={};data.append(alias)
 alias['Wns']={'asset':asset}
 fresh=next((x for x in data if 'Wnl' in x),None)
 if fresh is None:fresh={};data.append(fresh)
 fresh['Wnl']={'asset':asset}
 p.write_text(json.dumps(data,ensure_ascii=False,indent=2),encoding='utf-8')
 art=Image.open(Path(__file__).with_name('nightmare-legacy.png')).convert('RGBA')
 for suffix,size in [('.sprite',98),('.lowend.sprite',49)]:
  image=art.resize((size,size),Image.Resampling.LANCZOS)
  p=root/('data/hd/global/ui/items/misc/'+asset+suffix);p.parent.mkdir(parents=True,exist_ok=True)
  p.write_bytes(struct.pack('<4sHH8I',b'SpA1',31,size,size,size,0,1,0,0,0,0)+image.tobytes())
 # Keep a real existing relic ground model; the new icon is inventory-specific.
 ground=Path(__file__).parents[1]/'integration/upstream/sigil/relic_of_the_ancients.json'
 p=root/('data/hd/items/misc/'+asset+'.json');p.parent.mkdir(parents=True,exist_ok=True)
 p.write_bytes(ground.read_bytes())
 print('Nightmare Legacy: temporary Akara test stock enabled.' if test_vendor else 'Nightmare Legacy: vendor test disabled.')

if __name__=='__main__':
 import sys
 install(sys.argv[1])
