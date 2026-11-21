import csv,json,sys
from pathlib import Path
root=Path(sys.argv[1]);meta=json.loads((root/'workshop/monster_modifiers/effects_data.json').read_text());sets=[]
for folder in ('data/global/excel','data/global/excel/base'):
 mons=list(csv.DictReader((root/folder/'monstats.txt').open(),delimiter='\t'));by={int(r['*hcIdx']):r for r in mons if r['*hcIdx'].isdigit()};ids=[r['Id'] for r in mons];assert len(ids)==len(set(ids))
 for old,new in meta['cloneClasses']:
  r=by[new];assert r['neverCount']=='1' and r['BaseId']==r['Id'];assert r['Align'] not in ('1','2');assert not any(v for k,v in r.items() if k.startswith(('TreasureClass','Exp')))
 t=by[meta['bindings']['TotemClass']];assert t['killable']=='1' and t['AI']=='Idle' and t['Velocity']=='0' and t['neverCount']=='1'
 rune=by[meta['bindings']['RuneClass']];assert rune['killable']=='1' and rune['AI']=='Idle' and rune['Velocity']=='0' and rune['neverCount']=='1'
 assert rune['NameStr']=='WorkshopRitualStone' and not any(v for k,v in rune.items() if k.startswith(('TreasureClass','Exp')))
 sets.append(mons)
assert sets[0]==sets[1]
registry=json.loads((root/'data/hd/character/monsters.json').read_text(encoding='utf-8-sig'))
assert all(by[b]['Id'] in registry for a,b in meta['cloneClasses'])
assert 'workshop_modifier_totem' in registry and 'workshop_modifier_runestone' in registry
print('PASS: matching monster tables, unique IDs, no quest counts/XP/loot from helpers, isolated BaseIds, all private HD mappings, killable stationary totem.')

for folder in ('data/global/excel','data/global/excel/base'):
 rows=list(csv.DictReader((root/folder/'missiles.txt').open(),delimiter='\t'));missiles={r['Missile']:r for r in rows}
 hit=missiles['workshop_modifier_hit']
 assert hit['ToHit']=='0' and hit['HitFlags']=='0' and not hit['pSrvHitFunc'] and not hit['pSrvDmgFunc']
 assert hit['SrcDamage']=='0' and hit['HitShift']=='8' and hit['ResultFlags']=='4'
 poison=missiles['workshop_modifier_poisonfield']
 assert poison['CelFile']=='PoisonSparks' and poison['Range']=='75' and poison['CollideType']=='0'
 registry=json.loads((root/'data/hd/missiles/missiles.json').read_text(encoding='utf-8-sig'))
 assert registry['workshop_modifier_poisonfield']==registry['poisonexplosioncloud']
print('PASS: area hazards have no weapon accuracy gate or hit override; real poison cloud graphics in both table sets.')

for folder in ('data/global/excel','data/global/excel/base'):
 rows={r['Missile']:r for r in csv.DictReader((root/folder/'missiles.txt').open(),delimiter='\t')}
 assert rows['workshop_modifier_hit']['Explosion']=='0', 'Native explosion bit suppresses default impact damage'
 arena=rows['workshop_modifier_arenamarker']
 assert arena['pCltDoFunc']=='1' and arena['LoopAnim']=='1' and arena['Range']=='40'
 assert arena['CollideType']=='0' and arena['SrcDamage']=='0' and arena['ClientSend']=='1'
 assert registry['workshop_modifier_arenamarker']==registry['firewall']
print('PASS: default hazard impact damage enabled; independent looping arena marker with no damage/collision.')

for folder in ('data/global/excel','data/global/excel/base'):
 skills={r['skill']:r for r in csv.DictReader((root/folder/'skills.txt').open(encoding='utf-8-sig'),delimiter='\t')}
 for name,base,growth,oldshift in [('GiftedFire',30,8,7),('GiftedLightning',30,8,8),('GiftedCold',30,8,7),('EnchantedChain',15,4,8),('EnchantedCold',15,4,7)]:
  r=skills['Workshop Monster '+name]
  assert not r['cltdofunc'] and not r['cltmissilea'] and r['ItemCltEffect']=='0'
  reduction=2 if name.startswith('Enchanted') else 1
  assert int(r['HitShift'])==oldshift-reduction
  for level in (1,10,85,99):
   assert (int(r['EMin'])+int(r['EMinLev1'])*(level-1))*2**int(r['HitShift']) == (base+growth*level)*2**oldshift//(2**reduction)
print('PASS: Enchanted curves halved again; Gifted curves unchanged; fractional damage preserved.')
