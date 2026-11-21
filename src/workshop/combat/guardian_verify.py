"""Verify isolated guardian class, combat whitelist and native missile links."""
import json,sys
from pathlib import Path
from port import read
root=Path(sys.argv[1]);here=Path(__file__).resolve().parent
bindings=json.loads((here/'GUARDIAN.json').read_text())
for folder in ('data/global/excel','data/global/excel/base'):
 p=root/folder;_,skills=read(p/'skills.txt');_,monsters=read(p/'monstats.txt');_,missiles=read(p/'missiles.txt')
 byid={int(r['*Id']):r for r in skills if r['*Id'].isdigit()}
 guardian=byid[110];assert guardian['summon']=='workshop_astral_guardian'
 assert guardian['pettype']=='workshop_astral_guardian' and 'ws_pet_count' in guardian['petmax']
 _,types=read(p/'pettype.txt');assert any(r['pet type']=='workshop_astral_guardian' for r in types)
 pet=next(r for r in monsters if r['Id']==guardian['summon'])
 assert int(pet['*hcIdx'])==bindings['monster_class'] and pet['AI']=='GenericPet' and pet['MonStatsEx']=='workshop_guardian_paladin'
 skeleton=next(r for r in monsters if r['Id']=='necroskeleton')
 assert pet['flying']=='0' and all(pet[k]==skeleton[k] for k in ('Velocity','Run','aidel','aidel(N)','aidel(H)'))
 assert next(r for r in types if r['pet type']=='workshop_astral_guardian')['icontype']=='2'
 assert guardian['passivecalc8']=="stat('ws_pet_speed'.accr)"
 assert next(r for r in monsters if r['Id']=='valkyrie')['Skill1']==''
 assert len({r['Id'] for r in monsters})==len(monsters)
 _,equipment=read(p/'monequip.txt');_,states=read(p/'states.txt');_,extras=read(p/'monstats2.txt')
 own=[r for r in equipment if r['monster']==pet['Id']]
 assert {'rarm','larm','head','tors'}<={r['loc1'] for r in own if r['level']=='0'}
 assert [int(r['level']) for r in own]==sorted((int(r['level']) for r in own),reverse=True)
 assert all(r['item1'] in ('crs','9cr','7cr') for r in own if r['loc1']=='rarm')
 state=next(r for r in states if int(r['*ID'] or -1)==bindings['paladin_state'])
 assert state['gfxtype']=='2' and state['gfxclass']=='3' and state['transform']=='1'
 assert next(r for r in states if r['state']=='valkyrie')['gfxclass']=='0'
 extra=next(r for r in extras if r['Id']==pet['MonStatsEx'])
 assert extra['soft']=='0' and extra['SizeX']==extra['SizeY']=='2'
 assert all(extra['m'+mode]=='1' for mode in ('A1','A2','SC','BL','S1'))
 names={r['Missile'] for r in missiles}
 for slot,((private,unlock),original) in enumerate(zip(bindings['skills'],(101,112,121,106)),1):
  row=byid[private];public=byid[original]
  assert unlock==int(public['reqlevel'])
  assert public['charclass']=='pal' and not public['EDmgSymPerCalc'].startswith('stat(')
  assert not row['charclass'] and row['skilldesc'].startswith('workshop_internal_') and row['mana']=='0' and row['InTown']=='0'
  if original==106:
   assert row['srvstfunc']=='37' and row['srvdofunc']=='13' and row['ItemEffect']=='1'
   assert row['calc1']=='min((par5 + lvl -1), par6)' and row['Param6']=='5'
   assert row['calc2']=='400+5*((lvl<5)?0:((lvl-4)*par4))' and row['SrcDam']=='128'
  else:assert row['EDmgSymPerCalc']=="stat('damagepercent'.accr)"
  assert row['monanim']=='A1' 
  assert guardian['sumskill'+str(slot)]==row['skill'] and guardian[f'sumsk{slot}calc']=='lvl'
  assert pet['Skill'+str(slot)]=='' and pet[f'Sk{slot}lvl']==''
  for k,v in row.items():
   if v and 'missile' in k.lower():assert v in names,(row['skill'],k,v)
 assert guardian['sumskill5']==''
 assert not any('aura' in byid[private]['skill'].lower() for private,_ in bindings['skills'])
 for missile in missiles:
  if not missile['Missile'].startswith('workshop_guardian_'):continue
  for k,v in missile.items():
   if v and 'submissile' in k.lower():assert v in names,(missile['Missile'],k,v)
print('PASS: isolated guardian class, three Paladin spells and native Zeal, matching level unlocks, skill assignments, damage scaling, all missile references and unchanged ordinary Valkyrie/player skills.')
