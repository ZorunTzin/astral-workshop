"""Validate exclusive drop reachability, references, save bounds and repeatable generation."""
from pathlib import Path
import sys,json,hashlib
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from combat.port import read
from mythicals.data import install,MARKERS,SOURCE,ACTIVE
root=Path(sys.argv[1]);d=root/'data/global/excel'
def table(n):return read(d/(n+'.txt'))[1]
catalog=json.loads((root/'workshop/mythicals/catalog.json').read_text(encoding='utf8'))
assert len(catalog)==len(SOURCE)+11==92 and len({x['code'] for x in catalog})==92
codes={x['code'] for x in catalog};keys={x['key'] for x in catalog}
uniques={r['index']:r for r in table('uniqueitems')};props={r['code']:r for r in table('properties')};skills={r['skill']:r for r in table('skills')};stats={r['Stat']:r for r in table('itemstatcost')};types={r['Code']:r for r in table('itemtypes')}
base={r['code']:r for n in ('weapons','armor','misc') for r in table(n)}
assert max(int(r['*ID']) for r in stats.values() if r['*ID'].isdigit())==510,'Must not cross native stat save sentinel'
for item in catalog:
 b=base[item['code']];u=uniques[item['key']]
 assert b['spawnable']=='0' and b['rarity']=='0' and b['unique']=='1'
 assert u['spawnable']==('1' if item['id'] in ({r['_ordinal'] for r in ACTIVE}|set(range(81,92))) else '0')
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
positions={name:i for i,name in enumerate(tcs)}
for name,row in tcs.items():
 if name.startswith('Ws '):
  for n in range(1,11):
   child=row.get('Item'+str(n),'')
   if child in positions:assert positions[child]<positions[name],(name,child,'forward TC reference')

def reaches(name,seen=frozenset()):
 if name in keys or name in codes:return True
 if name not in tcs or name in seen:return False
 return any(reaches(tcs[name].get('Item'+str(i),''),seen|{name}) for i in range(1,11))
test_on=json.loads((Path(__file__).parent/'test-drops.json').read_text()).get('enabled',False)
allowed=[]
for mon in table('monstats'):
 if any(reaches(v) for k,v in mon.items() if k.startswith('TreasureClass') and v):
  if test_on and mon['Id']=='bloodraven':
   assert all(v=='Ws Mythical Test Poe Extra' for k,v in mon.items() if k.startswith('TreasureClass'));continue
  assert mon['Id'].startswith(('wsarenamon','wsarenaboss')),mon['Id'];allowed.append(mon['Id'])
assert len(allowed)==28
test_on=json.loads((Path(__file__).parent/'test-drops.json').read_text()).get('enabled',False)
for r in table('superuniques'):
 for k,v in r.items():
  if k.startswith('TC') and v:
   assert reaches(v)==(test_on and r['Superunique']=='Corpsefire'),(r.get('Superunique'),v)
assert tcs['Ws Mythical Test Corpsefire']['Picks']=='-6' and tcs['Ws Mythical Test Corpsefire']['Prob1']=='1'
chance=tcs['Ws Mythical Chance'];assert chance['Picks']=='1' and chance['NoDrop']=='4999' and chance['Prob1']=='1'
assert tcs['Ws Trial Special']['Picks']=='-2' and tcs['Ws Trial Special']['Item1']=='Ws Trial Special Original'
assert tcs['Ws Trial Special Original']['Item1']=='Lab Jewelry'
# All supplied dependency paths live in their own namespace; original assets are not replaced.
assets=json.loads((root/'workshop/mythicals/assets.json').read_text())
for rel in assets:assert (root/rel).is_file(),rel
assert len([x for x in assets if x.endswith('.sprite')])==184
for n in ['weapons','armor','misc','uniqueitems','properties','skills','states','skilldesc','itemtypes','superuniques','itemstatcost']:
 assert (d/(n+'.txt')).read_bytes()==(d/'base'/(n+'.txt')).read_bytes(),n
mirror={r['Treasure Class']:r for r in read(d/'base/treasureclassex.txt')[1]}
for k,r in tcs.items():
 if k.startswith(('Ws Mythical','Ws Trial Special')):assert r==mirror[k],k
# Revision regressions: exact identities, proc handlers, tooltip stats and active pool.
assert len(ACTIVE)==53 and sum(r['_ordinal']>=46 for r in ACTIVE)==7
for item in catalog:
 if item['id']==9 or 43<=item['id']<81:assert types[base[item['code']]['type']]['Equiv1']=='belt'
 if item['id'] in ({r['_ordinal'] for r in ACTIVE}|set(range(81,92))):assert reaches(item['key'])
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
  assert not any(c.get('filename','').endswith('fx_horadric_light.particles') for e in j['entities'] for c in e['components'])
  assert len(fx)==2 and all(c.get('hardKillOnDestroy') for e in fx for c in e['components'] if c['type']=='VfxDefinitionComponent')
gull=next(i for i in catalog if i['id']==81)
assert gull['name']=='The Gull' and types[base[gull['code']]['type']]['Equiv1']=='helm'
assert stats['ws_gull_shrines']['Save Bits']=='1' and stats['ws_gull_shrines']['*ID']=='266'
assert reaches(gull['key']) and tcs['Ws Mythical Pool 5']['Item4']==gull['key']
assert ['ac%','','150','200'] in gull['properties'] and ['hp','','80','120'] in gull['properties'] and ['res-all','','15','20'] in gull['properties']
# New families have stable saved identities; all ten test rewards are explicit.
expected={f'WorkshopMythical{i}' for i in range(81,92)}
reward=set()
for name,count in [('Ws Mythical Test Corpsefire',6),('Ws Mythical Test Poe Extra',5)]:
 row=tcs[name];assert row['Picks']==str(-count)
 for n in range(1,count+1):assert row['Prob'+str(n)]=='1';reward.add(row['Item'+str(n)])
assert reward==expected
for ident,power in [(267,'sin'),(304,'svalinn'),(309,'dream'),(310,'rathpith'),(311,'aegis'),(312,'nimis'),(313,'prototype')]:
 row=stats['ws_poe_'+power];assert row['*ID']==str(ident) and row['Save Bits']=='3'
for ident in range(82,92):assert uniques['WorkshopMythical'+str(ident)]['*ID']==str(1485+ident)
assert not any(p[0].endswith('-skill') for p in catalog[83]['properties']), 'Svalinn must not trigger novae'
assert all('Astral Projector' not in i['name'] and 'Inpulsa' not in i['name'] for i in catalog[81:])
assert [next(p[2] for p in catalog[i]['properties'] if p[0]=='ws_poe_prototype') for i in range(88,92)]==['1','2','3','4']
# Regression: texture geometry follows the native inventory footprint.
import struct
for i in range(81,92):
 item=catalog[i];b=base[item['code']]
 sprites=[p for p in assets if p.endswith('/workshopmythical'+str(i)+'.sprite')]
 assert len(sprites)==1,(i,sprites)
 raw=(root/sprites[0]).read_bytes();head=struct.unpack('<4sHH8I',raw[:40]);w,h=int(b['invwidth'])*98,int(b['invheight'])*98
 assert head[3:5]==(w,h) and len(raw)==40+w*h*4,(i,head,w,h)
 if i==84:assert '/items/weapon/sword/' in sprites[0]
for i in (83,85,86):
 model=next(p for p in assets if p.endswith('/workshopmythical'+str(i)+'.json') and '/hd/items/' in p)
 j=json.loads((root/model).read_text());assert all('/shield/' in x['path'] for x in j['dependencies']['models'])
assert stats['ws_poe_spell_bonus']['advdisplay']=='2' and not stats['ws_poe_spell_bonus']['Save Bits']
state_by={x['state']:x for x in table('states')}
assert all(x['aura']=='1' for x in state_by.values() if x['state'].startswith('shrine_gull_') or x['state']=='ws_gull_amplifier')
# Revised rolls and deterministic Blood Raven test drops.
expected={82:{'allskills':('1','1'),'res-all':('20','25'),'all-stats':('10','15'),'ws-myth-speeds':('10','10')},83:{'ac':('300','350'),'hp':('150','180'),'res-all':('25','35'),'allskills':('2','2')},84:{'dmg-norm':('200','350'),'ac':('400','500'),'all-stats':('15','25'),'res-all':('15','25')},85:{'cast2':('25','35'),'hp%':('15','20'),'allskills':('2','2')},86:{'ac':('400','450'),'res-all-max':('1','3'),'res-all':('25','35'),'allskills':('2','2')}}
expected[87]={'dmg%':('35','50'),'dex':('80','100'),'res-all':('15','25'),'lifesteal':('3','5'),'manasteal':('3','5'),'att%':('25','35')}
for i,element in enumerate(('fire','cold','ltng','pois'),88):expected[i]={'allskills':('3','3'),'extra-'+element:('15','25'),'cast2':('15','25'),'all-stats':('35','50')}
for ident,wanted in expected.items():
 u=uniques['WorkshopMythical'+str(ident)];actual={u['prop'+str(i)]:(u['min'+str(i)],u['max'+str(i)]) for i in range(1,13)}
 assert all(actual.get(k)==v for k,v in wanted.items()),(ident,actual)
from mythicals.poe_data import HERE
if json.loads((HERE/'test-drops.json').read_text()).get('enabled'):
 raven=next(r for r in table('monstats') if r['Id']=='bloodraven')
 assert all(v=='Ws Mythical Test Poe Extra' for k,v in raven.items() if k.startswith('TreasureClass'))
 extra=next(r for r in table('treasureclassex') if r['Treasure Class']=='Ws Mythical Test Poe Extra')
 assert extra['Picks']=='-5' and [extra['Item'+str(i)] for i in range(1,6)]==['WorkshopMythical'+str(i) for i in range(87,92)]
# Compare actual Unicode code points with the existing working native wolf affix.
strings={r['Key']:r for f in (root/'data/local/lng/strings').glob('*.json') for r in json.loads(f.read_text(encoding='utf-8-sig'))}
reference=strings['extraspiritwolves']['enUS']
assert ord(reference[0])==0xff
for ident in (266,267,304,309,310,311,312,313):
 st=next(r for r in stats.values() if r['*ID']==str(ident))
 assert (st['descfunc'],st['descval'],st['descpriority'])==(('' if ident==313 else '19'),'0','190')
 for language,text in strings[st['descstrpos']].items():
  if language in ('id','Key'):continue
  assert '\ufffd' not in text and '*c4' not in text
  assert all(line.startswith(reference[:3]) and line.endswith(reference[-3:]) for line in text.split('\n'))
  assert '%' not in text.replace('%%','')
def visible(key):
 return '\n'.join(line[3:-3] for line in reversed(strings[key]['enUS'].split('\n')))
assert visible('WorkshopGullPower')=='Gain a random shrine buff on elite kill for 60 seconds\nThis stacks with normal shrines'
assert visible('ws_poe_nimis')=='Shoot additional projectiles in a random direction\nwhich return to you'
assert visible('ws_poe_sin')=='All your damage becomes magic\nNearby enemies magic resistance is zero'
assert 'Maximum before' not in visible('ws_poe_svalinn')
assert visible('ws_poe_rathpith').split('\n')[1]=='Spells cost 10%% of current life'
# Re-running the generator must not change rows, IDs, probabilities, localizations or graphics.
def hashes():return {p:hashlib.sha256((root/p).read_bytes()).hexdigest() for p in assets}
before=hashes();install(root);assert hashes()==before,'Generator is not idempotent'
print('PASS: 92 definitions, save ranges, valid procs, 28 Trial-only monster routes, 184 inventory sprites, mirrored tables and idempotence.')


