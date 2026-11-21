"""Validate exclusive drop reachability, references, save bounds and repeatable generation."""
from pathlib import Path
import sys,json,hashlib
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from combat.port import read
from mythicals.data import install,MARKERS,SOURCE,ACTIVE
root=Path(sys.argv[1]);d=root/'data/global/excel'
def table(n):return read(d/(n+'.txt'))[1]
catalog=json.loads((root/'workshop/mythicals/catalog.json').read_text(encoding='utf8'))
assert len(catalog)==len(SOURCE)==81 and len({x['code'] for x in catalog})==81
codes={x['code'] for x in catalog};keys={x['key'] for x in catalog}
uniques={r['index']:r for r in table('uniqueitems')};props={r['code']:r for r in table('properties')};skills={r['skill']:r for r in table('skills')};stats={r['Stat']:r for r in table('itemstatcost')};types={r['Code']:r for r in table('itemtypes')}
base={r['code']:r for n in ('weapons','armor','misc') for r in table(n)}
assert max(int(r['*ID']) for r in stats.values() if r['*ID'].isdigit())==510,'Must not cross native stat save sentinel'
for item in catalog:
 b=base[item['code']];u=uniques[item['key']]
 assert b['spawnable']=='0' and b['rarity']=='0' and b['unique']=='1'
 assert u['spawnable']==('1' if item['id'] in {r['_ordinal'] for r in ACTIVE} else '0')
 assert b['normcode']==b['ubercode']==b['ultracode']==item['code']
 assert types[b['type']]['Equiv2']=='myth' and types[b['type']]['TreasureClass']=='0'
 assert b['levelreq']=='90' and u['lvl req']=='90' and u['code']==item['code']
 assert 2<=len(item['properties'])<=12 and item['properties'][-1][0]=='droponly'
 assert len({(p[0],p[1]) for p in item['properties'] if p[0]=='oskill'})==sum(p[0]=='oskill' for p in item['properties'])
 for prop,par,lo,hi in item['properties']:
  assert prop in props,(item['name'],prop)
  if prop in ('oskill','skill','hit-skill','att-skill','kill-skill','gethit-skill'):
   assert par in skills,(item['name'],par)
   assert int(skills[par]['*Id']) < (512 if prop in ('oskill','skill') else 1024),(item['name'],par,'skill save parameter')
   if prop.endswith('-skill'):assert int(hi)<=63,(item['name'],par,'proc level')
   if prop.endswith('-skill'):assert skills[par].get('passive')!='1' and skills[par].get('srvdofunc'),(item['name'],par)
  pr=props[prop]
  for n in range(1,8):
   st=stats.get(pr.get('stat'+str(n)));fn=pr.get('func'+str(n))
   if not st or fn not in ('1','3','8','15','16','17') or not st['Save Bits']:continue
   vals=[par] if fn=='17' else [lo] if fn=='15' else [hi] if fn=='16' else [lo,hi]
   for val in vals:assert 0<=int(val or 0)+int(st['Save Add'] or 0)<(1<<int(st['Save Bits'])),(item['name'],prop,val)
 for field,val in b.items():
  if field.endswith(('Min','Max','MagicMin','MagicMax')):assert val in ('','0'),(item['name'],field)
marker=stats['ws_mythical_power']
assert marker['*ID']=='258' and marker['Save Bits']=='8' and marker['Save Param Bits']=='6'
assert props['ws-mythical-power']['func1']=='22'
for item in catalog:
 for prop,par,lo,hi in item['properties']:
  if prop=='ws-mythical-power':
   assert 0<=int(par)<64 and int(lo)==int(hi)==1
   packed=258 | (int(par)<<9) | (int(lo)<<15) | (511<<23)
   assert packed&511==258 and (packed>>9)&63==int(par) and (packed>>15)&255==1 and packed>>23==511
for r in table('gamble'):assert r.get('code') not in codes
# A TC graph proves that existing world monsters have no explicit route to a Mythical.
tcs={r['Treasure Class']:r for r in table('treasureclassex')}
def reaches(name,seen=frozenset()):
 if name in keys or name in codes:return True
 if name not in tcs or name in seen:return False
 return any(reaches(tcs[name].get('Item'+str(i),''),seen|{name}) for i in range(1,11))
allowed=[]
for mon in table('monstats'):
 if any(reaches(v) for k,v in mon.items() if k.startswith('TreasureClass') and v):
  assert mon['Id'].startswith(('wsarenamon','wsarenaboss')),mon['Id'];allowed.append(mon['Id'])
assert len(allowed)==28
test_on=json.loads((Path(__file__).parent/'test-drops.json').read_text()).get('enabled',False)
for r in table('superuniques'):
 for k,v in r.items():
  if k.startswith('TC') and v:
   assert reaches(v)==(test_on and r['Superunique']=='Corpsefire'),(r.get('Superunique'),v)
assert tcs['Ws Mythical Test Corpsefire']['Picks']=='-6' and tcs['Ws Mythical Test Corpsefire']['Prob1']=='6'
chance=tcs['Ws Mythical Chance'];assert chance['Picks']=='1' and chance['NoDrop']=='4999' and chance['Prob1']=='1'
assert tcs['Ws Trial Special']['Picks']=='-2' and tcs['Ws Trial Special']['Item1']=='Ws Trial Special Original'
assert tcs['Ws Trial Special Original']['Item1']=='Lab Jewelry'
# All supplied dependency paths live in their own namespace; original assets are not replaced.
assets=json.loads((root/'workshop/mythicals/assets.json').read_text())
for rel in assets:assert (root/rel).is_file(),rel
assert len([x for x in assets if x.endswith('.sprite')])==162
for n in ['weapons','armor','misc','uniqueitems','properties','skills','states','skilldesc','itemtypes','superuniques','itemstatcost']:
 assert (d/(n+'.txt')).read_bytes()==(d/'base'/(n+'.txt')).read_bytes(),n
mirror={r['Treasure Class']:r for r in read(d/'base/treasureclassex.txt')[1]}
for k,r in tcs.items():
 if k.startswith(('Ws Mythical','Ws Trial Special')):assert r==mirror[k],k
# Revision regressions: exact identities, proc handlers, tooltip stats and active pool.
assert len(ACTIVE)==53 and sum(r['_ordinal']>=46 for r in ACTIVE)==7
for item in catalog:
 if item['id']==9 or item['id']>=43:assert types[base[item['code']]['type']]['Equiv1']=='belt'
 if item['id'] in {r['_ordinal'] for r in ACTIVE}:assert reaches(item['key'])
 for prop,par,lo,hi in item['properties']:
  if prop in ('hit-skill','kill-skill','gethit-skill','att-skill'):
   assert skills[par]['ItemEffect']==skills[par]['srvdofunc'],(item['name'],par,'proc handler')
   assert int(hi)==(25 if item['id']==15 else 1 if par.startswith('Workshop Mythical ') else 30 if par in ('Starfall','Comet','Kings Trinity') else 60)
assert ['hit-skill','Hydra','15','60'] in catalog[8]['properties']
assert ['kill-skill','Dragon Flight','5','25'] in catalog[15]['properties']
assert ['ws-poison-rate-wide','','2048','2048'] in catalog[31]['properties']
for prop in ('dmg-fire','dmg-cold','dmg-ltng'):
 assert any(p[0]==prop and p[2:]==['1000','2000'] for p in catalog[42]['properties'])
assert not any(p[0]=='ws-mythical-power' for p in catalog[42]['properties'])
for name in ('Raise Skeleton','Raise Skeletal Mage'):
 assert skills[name]['calc1']=="(100+lvl*par2)*(100+stat('ws_pet_life'.accr))/100-100"
 assert not any(skills[name]['passivestat'+str(n)]=='item_maxhp_percent' for n in range(1,15))
assert skills['Starfall']['EType']=='mag' and skills['Starfall']['ItemEffect']=='28' and skills['Starfall']['rightskill']=='1'
assert skills['Workshop Mythical Blood Empowerment']['aurastat4']=='workshop_bleed_chance'
for name in ('Blood Empowerment','Celestial Empowerment','Underworld Empowerment'):
 assert skills['Workshop Mythical '+name]['auralencalc']=='300'
for rel in assets:
 if rel.startswith('data/hd/items/') and 'workshopmythical' in rel and rel.endswith('.json'):
  j=json.loads((root/rel).read_text());fx=[e for e in j['entities'] if e['name'].startswith('workshop_mythical_glow_')]
  assert len(fx)==3 and all(c.get('hardKillOnDestroy') for e in fx for c in e['components'] if c['type']=='VfxDefinitionComponent')
# Re-running the generator must not change rows, IDs, probabilities, localizations or graphics.
def hashes():return {p:hashlib.sha256((root/p).read_bytes()).hexdigest() for p in assets}
before=hashes();install(root);assert hashes()==before,'Generator is not idempotent'
print('PASS: 81 definitions, save ranges, valid procs, 28 Trial-only monster routes, 162 inventory sprites, mirrored tables and idempotence.')


