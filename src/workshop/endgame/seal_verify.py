from pathlib import Path
import sys,json,hashlib
sys.path.insert(0,str(Path(__file__).parents[1]))
from combat.port import read
from endgame.seal_data import CODES,NAMES,VALID
from endgame.seal_entries import CODES as SEALS
root=Path(sys.argv[1]);p=root/'data/global/excel'
rows=lambda n:read(p/(n+'.txt'))[1]
misc={r['code']:r for r in rows('misc')}
assert len(rows('itemstatcost'))==511
for i,code in enumerate(CODES):
 r=misc[code];assert r['cost']=='1' and r['AkaraMin']==r['AkaraMax']=='1' and r['AdvancedStashStackable']=='1' and r['UICatOverride']=='wtor'
for i in range(1,6):assert misc['Wa'+str(i)]['compactsave']=='0'
for r in misc.values():
 if r['code']=='bag' or r['type']=='grab':assert r['PermStoreItem']=='0' and all(not int(v or 0) for k,v in r.items() if k.endswith(('Min','Max','MagicLvl')))
recipes=[r for r in rows('cubemain') if r['description'].startswith('Workshop Trial Craft ')]
assert len(recipes)==5*sum(map(len,VALID))
for r in recipes:
 i=CODES.index(r['input 2']);assert r['output']=='useitem' and r['op']=='18' and r['param']=='444' and int(r['value']) in VALID[i] and r['mod 1 param']==str(i)
for f in ('properties','misc','cubemain','itemuicategories'):assert (p/(f+'.txt')).read_bytes()==(p/'base'/(f+'.txt')).read_bytes(),f
classes={r['Treasure Class']:r for r in rows('treasureclassex')}
for n in ['Workshop Trial Orbs','Workshop Herald And Trial Orbs','Workshop Trial Orb Materials']:
 r=classes[n]
 for i in range(1,11):
  value=r['Item'+str(i)];assert not value or value in classes or value in misc,(n,value)
assert any('Workshop Trial Orb Materials'==r['Item2'] or 'Workshop Trial Orb Materials'==r['Item3'] for r in classes.values() if r['Treasure Class'].startswith('Ws Trial Base '))
for filename in ['bankexpansionlayouthd.json','controller/bankexpansionlayouthd.json']:
 layout=json.loads((root/'data/global/ui/layouts'/filename).read_text());group=next(c for c in layout['children'] if c['name']=='advancedstash_materials');slots=[c for c in group['children'] if c['type']=='AdvancedStashSlotWidget']
 assert len(slots)==len(group['children']), 'Native Materials container must contain only stash slots (login crash regression)'
 for code in CODES+SEALS:assert sum(c['fields']['itemCode']==code for c in slots)==1
 for a in slots:
  if a['name'] not in CODES+SEALS:continue
  x=a['fields']['rect']
  for b in slots:
   if a is b:continue
   y=b['fields']['rect'];assert x['x']+x['width']<=y['x'] or y['x']+y['width']<=x['x'] or x['y']+x['height']<=y['y'] or y['y']+y['height']<=x['y'],(a['name'],b['name'])
for n in NAMES:
 for suffix in ['.sprite','.lowend.sprite']:assert (root/('data/hd/global/ui/items/misc/uniquecharm/workshop_trial_'+n.lower()+suffix)).stat().st_size>1000
bank=json.loads((root/'bank.txt').read_text())
for n in ['misc','itemtypes','inventory']:assert bank['tables'][n]==hashlib.sha256((p/(n+'.txt')).read_bytes()).hexdigest()
print('PASS: 140 guarded recipes, 7 native Materials slots (mouse/controller), no overlap, saved noncompact seals, existing save schema intact, independent rare orb loot, vendor cleanup, HD artwork, auto-deposit table hashes.')

assert misc['1gc']['spawnable']=='0' and misc['1gc']['PermStoreItem']=='0'
assert not any(r.get('enabled')=='1' and any(r.get(k,'').split(',')[0]=='1gc' for k in ('output','output b','output c')) for r in rows('cubemain'))
assert not any(r['Item'+str(i)] in ('1gc','Gem Cluster') for r in classes.values() for i in range(1,11))
print('PASS: Gem Cluster unavailable from vendors, drops and production recipes; old item definition retained.')

labels={r['Key']:r for r in json.loads((root/'data/local/lng/strings/item-names.json').read_text(encoding='utf8'))}
for code,name in zip(CODES,NAMES):
 assert labels[code]['enUS'].startswith('\u00ffc;')
 ground=json.loads((root/('data/hd/items/misc/uniquecharm/workshop_trial_'+name.lower()+'.json')).read_text())
 effects=[c['filename'] for e in ground['entities'] for c in e['components'] if c['type']=='VfxDefinitionComponent']
 assert any('horadric_light' in x for x in effects) and any('valkyriestart' in x for x in effects)
 assert len({e['id'] for e in ground['entities']})==len(ground['entities'])
 for f in effects:assert {'path':f} in ground['dependencies']['particles']
for filename,offset in [('bankexpansionlayouthd.json',0),('controller/bankexpansionlayouthd.json',212)]:
 layout=json.loads((root/'data/global/ui/layouts'/filename).read_text())
 group=next(c for c in layout['children'] if c['name']=='advancedstash_materials')
 for i,code in enumerate(CODES):
  rect=next(c['fields']['rect'] for c in group['children'] if c['name']==code)
  assert (rect['x'],rect['y'])==(620+104*(i%3),266+104*(i//3)-offset)
 assert (root/('data/hd/global/ui/'+layout['fields']['backgroundFile'][3]+'.sprite')).exists()
print('PASS: Trial materials beside keys in both layouts, dedicated background frames, purple names and Jewel drop VFX.')

for i,code in enumerate(SEALS,1):
 r=misc[code];assert r['invwidth']==r['invheight']=='1' and r['AdvancedStashStackable']=='1' and r['compactsave']=='1'
 saved=misc['Wa'+str(i)];assert saved['AdvancedStashStackable']=='0' and saved['compactsave']=='0' and saved['AkaraMin']=='0'
 rs=[r for r in rows('cubemain') if r['description'].startswith('Workshop Blank Seal ') and r['input 1']==code]
 assert len(rs)==4
 assert sum(r['output']=='Wa'+str(i) and r['mod 1 param'] in ('0','1','6') for r in rs)==3
assert classes['Workshop Trial Seal Drops']['NoDrop']=='24975'
assert sum(int(classes['Workshop Trial Seal Drops']['Prob'+str(i)] or 0) for i in range(1,11))==25
for r in rows('monstats'):
 if r['Id'].startswith('wsarenaboss'):
  assert r['neverCount']=='1' and not r['primeevil'] and r['Level(H)']=='85'
  assert all(not v for k,v in r.items() if k.startswith('TreasureClassQuest'))
for name,r in classes.items():
 if name.startswith('Ws Seal World '):assert r['Picks']=='-2' and r['Item1'] in classes and r['Item2']=='Workshop Trial Seal Drops' and not r['Item1'].startswith('Ws Seal World ')
print('PASS: 5 bank-safe blank seals, individual crafted seals, 20 conversion/portal recipes, 0.1% independent seal drops, non-quest private boss models.')
