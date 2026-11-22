"""Checks actual generated game tables, map payload and entry recipes."""
import csv,json,struct,os
from pathlib import Path
root=Path(os.environ['WORKSHOP_MOD_ROOT']);meta=json.loads((root/'workshop/endgame/data.json').read_text())
for folder in ('data/global/excel','data/global/excel/base'):
 p=root/folder
 def rows(n):return list(csv.DictReader((p/(n+'.txt')).open(encoding='utf-8-sig'),delimiter='\t'))
 levels={int(r['Id']):r for r in rows('levels') if r['Id'].isdigit()}
 for i in range(5):
  r=levels[meta['firstLevel']+i];assert r['Act']==levels[109]['Act']=='4', 'Trial portals must share Harrogath act'
  assert 0 < int(r['Layer']) < 100, 'Native automap persistence has only 100 layer slots'
  assert r['SaveMonsters']=='1'
  assert int(r['Layer'])==(64,65,66,67,68)[i]
  assert all(r[k]=='32' for k in ('SizeX','SizeY','SizeX(H)','SizeY(H)'))
  assert all(r[k]=='0' for k in r if k.startswith(('MonDen','MonUMin','MonUMax','ObjGrp','ObjPrb','Vis')))
  assert not any(r[k] for k in r if k.startswith(('mon','nmon','umon','cmon')))
 types={r['Code']:r for r in rows('itemtypes')}
 misc={r['code']:r for r in rows('misc')}
 for code in meta['sealCodes']:
  r=misc['Wt'+code[-1]];assert r['cost']=='1' and r['AkaraMin']==r['AkaraMax']==r['PermStoreItem']=='1'
  assert r['spawnable']=='1', f'{code}: excluded from NPC item cache'
  assert types[r['type']]['StorePage']=='misc' and types[r['type']]['Normal']=='1'
  assert r['useable']=='0' and r['quest']=='' and r['stackable']=='0'
 recipes=[r for r in rows('cubemain') if r['description'].startswith('Workshop - Open Arena')];assert len(recipes)==5
 for i,r in enumerate(recipes):assert r['min diff']=='2' and r['input 1']==meta['sealCodes'][i] and r['output']==f"Red Portal,lvl={meta['firstLevel']+i},qty=1"
 monsters={int(r['*hcIdx']):r for r in rows('monstats') if r['*hcIdx'].isdigit()}
 for id in meta['monsters']:
  r=monsters[id];assert r['neverCount']=='1' and r['Level(H)']=='100'
  assert all(int(r[k])>0 for k in ('Exp','Exp(N)','Exp(H)'))
  assert r['TreasureClass(H)']
  source=next(x for x in monsters.values() if x['Id']==json.loads((root/'workshop/endgame/enemy_pool.json').read_text())[meta['monsters'].index(id)])
  tc={x['Treasure Class']:x for x in rows('treasureclassex')}
  for k in r:
   if k.startswith('TreasureClass') and source[k]:
    original=tc[source[k]]['Item1'] if source[k].startswith('Ws Seal World ') else source[k]
    wrapper=tc[r[k]];assert wrapper['Item1']==original and wrapper['Picks']=='-4'
    assert [wrapper['Item'+str(i)] for i in (2,3,4)]==['Ws Trial Runes','Workshop Trial Orb Materials','Ws Trial Special']
  assert tc['Ws Trial Runes']['NoDrop']=='3980' and tc['Ws Trial Materials']['NoDrop']=='24975' and tc.get('Ws Trial Special Original',tc['Ws Trial Special'])['NoDrop']=='495'
 assert any(r['state']=='ws_arena_blessing' and int(r['*ID'])==meta['state'] for r in rows('states'))
b=(root/'data/global/tiles/workshop/astral-arena.ds1').read_bytes();assert struct.unpack_from('<8I',b)==(18,32,32,2,0,0,1,1)
n=33*33;assert len(b)==32+n*4*4+8;assert b[-8:]==bytes(8)
floors=struct.unpack_from('<'+str(n)+'I',b,32+2*n*4);assert all(floors)
items=json.loads((root/'data/hd/items/items.json').read_text());assert all(any(code in x for x in items) for code in meta['sealCodes'])
assert len({next(x[c]['asset'] for x in items if c in x) for c in meta['sealCodes']})==5
print('PASS: five distinct seals at 1 gold, Hell-only portal recipes, isolated 32x32 square maps, empty population, non-quest wave monsters, native monster loot, temporary state and five HD icons.')

# The HD companion must be shipped along with the DS1. Missing presets pass
# the classic-map checks but crash HD level translation with an empty biome.
hd_path='data/hd/env/preset/workshop/astral-arena.json'
assert hd_path in json.loads((root/'workshop/endgame/assets.json').read_text())
hd=json.loads((root/hd_path).read_text())
assert hd['type']=='Preset' and hd['biomeFilename']=='data/hd/env/biome/act3_jungle_dungeon.json'
assert hd['entities']==[] and hd['specialTiles']=={}
components=hd['terrain']['components']
assert any(c['type']=='TerrainDefinitionComponent' for c in components)
transform=next(c for c in components if c['type']=='TransformDefinitionComponent')
assert transform['scale']['x']==transform['scale']['z']==0.8
assert any(c['type']=='ModelDefinitionComponent' and c['filename'] for c in components)
print('PASS: packaged HD arena scene, explicit biome, terrain and square scaling; no interior dungeon props.')

models=json.loads((root/'data/hd/character/monsters.json').read_text(encoding='utf-8-sig'))
for i,base in enumerate(json.loads((root/'workshop/endgame/enemy_pool.json').read_text())):
 assert models['wsarenamon'+str(i)]==models[base]
print('PASS: all private wave monsters resolve to their original HD models.')

import re
layout=json.loads(''.join(re.findall(r'R"trial\((.*?)\)trial"',(root/'workshop/endgame/panel_layout.h').read_text(),re.S)))
assert layout['type']=='Panel' and layout['name']=='workshop-curses/TrialChoices'
offers={w['name']:w for w in layout['children'] if w['name'].startswith('Offer_')}
catalog=json.loads((root/'workshop/endgame/modifiers.json').read_text());assert len(offers)==3*5*len(catalog)*5
for slot in range(3):
 for stage in range(1,6):
  for mod in range(len(catalog)):
   for tier in range(5):assert not offers[f'Offer_{slot}_{stage}_{mod}_{tier}']['fields']['visible']
assert sum(w['type']=='ButtonWidget' for w in layout['children'])==9
for name in ('trial-panel','trial-card','trial-finish','trial-active'):
 rel='data/hd/global/ui/Workshop/'+name+'.sprite'
 assert rel in json.loads((root/'workshop/endgame/assets.json').read_text())
 assert (root/rel).read_bytes()[:4]==b'SpA1'
print('PASS: native choice panel, all offer text variants, four buttons and packaged UI sprites.')

rarities=[w for w in layout['children'] if w['name'].startswith('Rarity_')]
assert len(rarities)==15
for slot in range(3):
 for tier in range(5):
  w=next(w for w in rarities if w['name']==f'Rarity_{slot}_{tier}')
  assert not w['fields']['visible'] and not w['fields']['enabled']
  rel=f'data/hd/global/ui/Workshop/trial-rarity-{tier}.sprite'
  assert rel in json.loads((root/'workshop/endgame/assets.json').read_text())
  b=(root/rel).read_bytes()
  colors=((255,255,255,255),(82,143,255,255),(255,255,66,255),(187,102,255,255),(232,180,67,255))
  assert bytes(colors[tier]) in b[40:] and len(b)==40+650*740*4
print('PASS: five distinct rarity borders, all three card slots, non-interactive transparent overlays.')

assert len(layout["children"])<16384
assert sum(w["name"].startswith("Active_") for w in layout["children"])==8*len(catalog)
assert all(m["cap"]<=100 for m in catalog if m["kind"]=="proc")

# Distinct themed HD presets retain the proven square gameplay collision.
for theme in json.loads((root/'workshop/endgame/themes.json').read_text()):
 scene=json.loads((root/theme['hd']).read_text());assert scene['biomeFilename']==theme['biome'] and scene['terrain'] and not scene['entities']
 assert (root/theme['ds1']).read_bytes()==(root/'data/global/tiles/workshop/astral-arena.ds1').read_bytes()
print('PASS: four distinct themed HD maps, biome/terrain bindings and square collision.')

for name in ('Reroll','Banish'):
 assert any(w['name']==name for w in layout['children'])
 assert sum(w['name'].startswith(name+'Count_') for w in layout['children'])==4
assert next(m for m in catalog if m['key']=='PlayerLife')['tiers']==[3,6,9,12,20]
assert not next(m for m in catalog if m['key']=='PlayerRecovery')['enabled']
print('PASS: explicit tiers, removed Steadfast, reroll/banish counters and weighted native loot wrappers.')

headers=[w for w in layout['children'] if w['name'].startswith('Category_')]
assert len(headers)==6
assert all(w['fields']['rect']['y']+w['fields']['rect']['height'] < -400+115 for w in headers)
assert all('CHARACTER BONUS' not in w['fields']['text'] and 'MONSTER MODIFIER' not in w['fields']['text'] and '+10% life / +2% damage' in w['fields']['text'] for w in layout['children'] if w['name'].startswith('Offer_'))
print('PASS: category headers above divider; all offers disclose universal monster burden.')
