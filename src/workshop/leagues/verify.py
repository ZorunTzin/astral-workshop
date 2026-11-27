"""Check generated League tables, assets, native slots and pickup identities."""
from pathlib import Path
import csv, json, sys, hashlib, struct
root=Path(sys.argv[1]); here=Path(__file__).resolve().parent
cfg=json.loads((here/'definitions.json').read_text())
codes=[f'L{chr(110+m)}{i+1}' for m in range(3) for i in range(4)]
def table(name,base=''):
 return list(csv.DictReader((root/f'data/global/excel/{base}{name}.txt').open(encoding='utf-8-sig'),delimiter='\t'))
for base in ('','base/'):
 misc=table('misc',base); items={r['code']:r for r in misc}
 for code in codes:
  assert sum(r['code']==code for r in misc)==1,code
  r=items[code]
  assert r['AdvancedStashStackable']=='1' and r['UICatOverride']=='wleague'
  assert r['useable']=='0' and r['pSpell']=='-1' and r['spawnable']=='0'
  assert all(not r[x] for x in ('state','cstate1','cstate2','stat1','stat2','stat3'))
  assert r['normcode']==r['ubercode']==r['ultracode']==code
 tcs={r['Treasure Class']:r for r in table('treasureclassex',base)}
 mons=table('monstats',base)
 for m in range(3):
  for rank in range(3):
   for diff,suffix in enumerate(('',' (N)',' (H)')):
    tc=tcs[f'Ws League {m} {rank}'+suffix];pool=tcs[tc['Item1']]
    assert tc['Picks']=='1' and tc['Prob1']=='109'
    assert int(tc['NoDrop'])==(4*[900,233,0][rank]+400)*(2 if m==0 else 1)-109
    assert [pool[f'Item{i+1}'] for i in range(4)]==codes[m*4:m*4+4]
    assert [int(pool[f'Prob{i+1}']) for i in range(4)]==cfg['orbTierWeights'][diff]
   for v in range(len(cfg['mechanics'][m]['monsters'])):
    r=next(r for r in mons if r['Id']==f'wsleague{m}{rank}{v}')
    assert r['enabled']=='1' and r['neverCount']=='1'
    assert r['BaseId']==cfg['mechanics'][m]['monsters'][v]
    assert all(not value or value==f'Ws League {m} {rank}'+(' (H)' if key.endswith('(H)') else ' (N)' if key.endswith('(N)') else '') for key,value in r.items() if key.startswith('TreasureClass'))
  obj=next(r for r in table('objects',base) if r['Name']==f'WsLeaguePillar{m}')
  assert obj['OperateFn']=='18' and obj['InitFn']=='0' and obj['Selectable0']=='1' and obj['Selectable1']=='0'
 for name,key in [('monstats','*hcIdx'),('objects','*ID'),('states','*ID'),('missiles','*ID')]:
  rows=table(name,base); ids=[r[key] for r in rows if r[key].isdigit()];assert len(ids)==len(set(ids)),name
for mode in ('','controller/'):
 j=json.loads((root/f'data/global/ui/layouts/{mode}bankexpansionlayouthd.json').read_text())
 children=j['children'];tabs=next(c for c in children if c['name']=='BankTabs')
 assert tabs['fields']['tabCount']==5
 slots=[c for c in next(c for c in children if c['name']=='advancedstash_materials')['children'] if c['name'].startswith('LeagueOrb')]
 assert len(slots)==12 and sorted(c['fields']['itemCode'] for c in slots)==sorted(codes)
 assert all(c['type']=='AdvancedStashSlotWidget' and c['fields'].get('visible',True)==True for c in slots)
 rects=[c['fields']['rect'] for c in slots]
 assert all(r['y']+r['height'] < 708-(212 if mode else 0) for r in rects), 'League hit areas overlap the native Cube grid'
 for a in rects:
  for b in rects:
   if a is b:continue
   assert a['x']+a['width']<=b['x'] or b['x']+b['width']<=a['x'] or a['y']+a['height']<=b['y'] or b['y']+b['height']<=a['y']
 assert len([c for c in children if c['name']=='LeagueTab'])==1
assets=json.loads((root/'workshop/leagues/assets.json').read_text())
assert len(assets)==len(set(assets))
for rel in assets:
 p=root/rel;assert p.is_file(),rel
 if p.suffix=='.json':json.loads(p.read_text(encoding='utf-8-sig'))
 if p.suffix=='.sprite':
  raw=p.read_bytes();magic,version,w,texw,h,*_=struct.unpack_from('<4sHH8I',raw)
  assert magic==b'SpA1' and version==31 and len(raw)==40+texw*h*4,rel
  if 'workshop_league_' in rel:assert 0 in raw[43::4] and max(raw[43::4])==255,rel
models=json.loads((root/'data/hd/character/monsters.json').read_text())
for m in range(3):
 for rank in range(3):
  for v in range(len(cfg['mechanics'][m]['monsters'])):
   model=json.loads((root/f'data/hd/character/enemy/{models[f"wsleague{m}{rank}{v}"]}.json').read_text())
   var=model['dependencies']['variantdata'][0]['path'];assert var.startswith(f'data/hd/character/enemy/wsleague{m}_')
   variants=json.loads((root/var).read_text())
   for entry in variants['entries']:
    if entry['name'].startswith('level'):
     for t in entry['transforms']:assert [t['colorTint'][k] for k in ('x','y','z')]==cfg['mechanics'][m]['color']
manifest=(root/'bank.txt').read_bytes();bank=json.loads(manifest)
for name,digest in bank['tables'].items():assert hashlib.sha256((root/f'data/global/excel/{name}.txt').read_bytes()).hexdigest()==digest
assert (root/'d2rloader/plugins/fmmp-auto-collectibles.dll').read_bytes().count(hashlib.sha256(manifest).hexdigest().encode())==3
print(f'PASS: 12 crafting currencies, 81 colored monster variants, pillar operations, loot, two 3x4 layouts, {len(assets)} assets and pickup manifest.')

# Orb names are mechanic-colored, every ground model carries both beam and colored aura.
strings={r['Key']:r['enUS'] for r in json.loads((root/'data/local/lng/strings/item-names.json').read_text(encoding='utf-8-sig'))}
for m,mechanic in enumerate(cfg['mechanics']):
 for i in range(4):
  assert strings[codes[m*4+i]].startswith('\u00ffc'+['5','2','1'][m])
  assert f' Orb (t{i+1})' in strings[codes[m*4+i]]
  model=json.loads((root/f"data/hd/items/misc/uniquecharm/workshop_league_{mechanic['id']}{i}.json").read_text())
  effects=[c for e in model['entities'] for c in e['components'] if c['type']=='VfxDefinitionComponent']
  assert len(effects)>=2 and all(c['hardKillOnDestroy'] for c in effects)
  assert sum(c['filename']=='data/hd/vfx/particles/overlays/object/horadric_light/fx_horadric_light.particles' for c in effects)==1
  assert not any('/league_beam' in c['filename'] for c in effects)
  assert any('Fanaticism.model' in d['path'] for d in model['dependencies']['models'])
  assert any(c['filename']==f'data/hd/vfx/particles/workshop/league_orb{m}.particles' for c in effects)
print('PASS: 12 colored orb labels, jewel beams and mechanic-colored ground glows.')


# Exact per-monster chance, and conditional tier distribution, independently checked.
from fractions import Fraction
for m in range(3):
 for rank in range(3):
  for diff,suffix in enumerate(('',' (N)',' (H)')):
   tc=tcs[f'Ws League {m} {rank}'+suffix];prob=int(tc['Prob1']);total=int(tc['NoDrop'])+prob
   assert Fraction(prob,total)==Fraction(109,4*[900,233,0][rank]+400)*(Fraction(1,2) if m==0 else 1)
   pool=tcs[tc['Item1']];assert sum(int(pool[f'Prob{i}']) for i in range(1,5))==100
print('PASS: Night-only half drop rate; Normal/Nightmare/Hell tier distributions; unchanged Breach/Beyond total odds.')

# Saved properties, safe recipe gating and every equipment/family/tier combination.
import sys
sys.path.insert(0,str(here.parent))
from leagues.crafting import COST,RULES
for base in ('','base/'):
 recipes=[r for r in table('cubemain',base) if r['description'].startswith('Workshop League Orb ')]
 assert len(recipes)==48
 for r in recipes:
  assert r['output']=='useitem' and r['numinputs']=='2' and r['op']=='15' and r['param']=='265'
  assert int(r['value'])==COST[int(r['input 2'][-1])-1]
  assert r['input 1'] in ('weap','armo','ring','amul')
  m=ord(r['input 2'][1])-110;t=int(r['input 2'][-1])-1;g=0 if r['input 1']=='weap' else 1 if r['input 1']=='armo' else 2
  expected=[]
  for prop,values,param in RULES[m*3+g]:
   if values[t]:
    v=values[t];lo,hi=v if isinstance(v,tuple) else (v,v);expected.append((prop,param,lo,hi))
  expected.append(('ws-orb-level',0,COST[t],COST[t]))
  actual=[(r[f'mod {i}'],int(r[f'mod {i} param'] or 0),int(r[f'mod {i} min']),int(r[f'mod {i} max'])) for i in range(1,6) if r[f'mod {i}']]
  assert actual==expected,(r['description'],actual,expected)
  assert all(r[f'mod {i} chance']=='100' for i in range(1,len(expected)+1))
 for r in table('misc',base):
  if r['code'] in codes:
   text=strings[r['spelldescstr']];assert 'Weapons:' in text and 'Armor / Shields:' in text and 'Rings / Amulets:' in text and 'Required Level' in text

 stats={int(r['*ID']):r for r in table('itemstatcost',base) if r['*ID'].isdigit()}
 assert [stats[i]['Save Bits'] for i in (262,263,264,265)]==['12','10','7','']
 missiles={r['Missile']:r for r in table('missiles',base)}
 assert missiles['workshop_trinity_hit']['SrcDamage']=='128' and missiles['workshop_trinity_hit']['CollideType']=='0'
 for name in ('fire','cold','light'):
  r=missiles['workshop_trinity_'+name];assert r['SrcDamage']=='0' and r['CollideType']=='0'
assert RULES[6][0][1]==[1,4,6,16]
print('PASS: 48 gated Cube recipes, additive saved stat widths, no automatic nova missile collisions and all tier costs.')

# Exclusive trait classes preserve ordinary and existing League IDs.
registry=json.loads((root/'data/hd/character/monsters.json').read_text(encoding='utf-8-sig'))
for base in ('','base/'):
 mons={r['Id']:r for r in table('monstats',base)}
 traits=[r for r in mons.values() if r['Id'].startswith('wsleague') and '_trait' in r['Id']]
 assert len(traits)==162
 for r in traits:
  parent=mons[r['Id'].split('_trait')[0]];trait=int(r['Id'].split('_trait')[1])
  assert r['BaseId']==parent['BaseId'] and r['Rarity']=='0' and r['neverCount']=='1'
  if trait==5:assert all(not v for k,v in r.items() if k.startswith('TreasureClass')) and all(r[k]=='0' for k in ('Exp','Exp(N)','Exp(H)'))
  if trait in (1,5):
   name=registry[r['Id']];model=json.loads((root/'data/hd/character/enemy'/(name+'.json')).read_text())
   original=json.loads((root/'data/hd/character/enemy'/(registry[parent['Id']]+'.json')).read_text())
   def scale(m):
    e=next(e for e in m['entities'] if e['name']=='entity_root')
    return next((c['scale'] for c in e['components'] if c['type']=='TransformDefinitionComponent'),dict(x=1,y=1,z=1))
   assert scale(model)=={k:v*(1.65 if trait==1 else .6) for k,v in scale(original).items()}
print('PASS: 162 private League trait classes, large/small HD variants, zero child loot/XP, stable base families.')
