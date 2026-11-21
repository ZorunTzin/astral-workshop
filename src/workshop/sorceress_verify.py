"""Validate public skill identity, tree placement, proc chains and prototype math."""
import os,json
from pathlib import Path
from combat.port import read
root=Path(os.environ.get('WORKSHOP_MOD_ROOT',Path(__file__).resolve().parents[2]))
b=json.loads((Path(__file__).parent/'SORCERESS.json').read_text())
for folder in ('data/global/excel','data/global/excel/base'):
 p=root/folder;_,skills=read(p/'skills.txt');_,descs=read(p/'skilldesc.txt');_,missiles=read(p/'missiles.txt');_,monsters=read(p/'monstats.txt');_,states=read(p/'states.txt')
 s={int(r['*Id']):r for r in skills if r['*Id'].isdigit()};d={r['skilldesc']:r for r in descs};m={r['Missile']:r for r in missiles};st={r['state']:r for r in states}
 expected={47:1,51:6,45:1,55:6,36:12,41:18,39:18,43:6,44:6}
 for sid,level in expected.items():assert int(s[sid]['reqlevel'])==level,(sid,s[sid]['reqlevel'])
 positions=[tuple(d[r['skilldesc']][k] for k in ('SkillPage','SkillRow','SkillColumn')) for r in s.values() if r['charclass']=='sor']
 assert len(positions)==len(set(positions))==30
 assert s[36]['srvdofunc']=='18' and st[s[36]['aurastate']]['gfxclass']=='292'
 assert d[s[41]['skilldesc']]['descdam']=='5' and not d[s[41]['skilldesc']]['descmissile1']
 assert s[41]['localdelay']=='250' and m['workshop_conflagration_ground']['Range']=='75'
 assert s[39]['srvdofunc']==s[43]['srvdofunc']=='18'
 for sid in (36,39,40,43,46,50,52,57,58,60):assert s[sid]['restrict']=='1'
 for sid in (41,45,47,51,53,54,55,56,59,62,64):assert s[sid]['restrict']=='0'
 assert s[37]['passivecalc3']=='(50+10*lvl)*256'
 assert s[37]['passivecalc4']==s[37]['passivecalc5']=='3+lvl/2'
 assert s[52]['auraevent1']=='domeleeattack' and s[52]['auraeventfunc1']=='33'
 for sid in (36,39,43):
  assert s[sid]['mana']=='0' and s[sid]['auralencalc']=='50'
  assert not s[sid]['periodic'] and not s[sid]['cltstfunc']
  assert d[s[sid]['skilldesc']]['descdam']=='5'
  assert s[sid]['auraevent1']=='domeleeattack'
  for i in range(1,7):
   if s[sid]['aurastat'+str(i)] in ('firemindam','firemaxdam','coldmindam','coldmaxdam','lightmindam','lightmaxdam'):assert '*256' not in s[sid]['aurastatcalc'+str(i)]
 assert s[41]['EMin']=='750' and s[41]['EMax']=='1200'
 assert m['workshop_conflagration_tick']['CollideType']=='3' and not m['workshop_conflagration_tick']['pSrvHitFunc']
 for key,element in [('FireProc','fire'),('ColdProc','cold'),('LightningProc','ltng')]:
  proc=s[b[key]];assert proc['EType']==element and proc['calc1']=='3'
  maker=m[proc['srvmissilea']];child=m[maker['SubMissile1']]
  assert maker['Range']=='48' and maker['Skill']==proc['skill']
  assert child['Skill']==proc['skill'] and child['EType']==element and child['MinDamage']==child['MaxDamage']=='0'
 assert next(r for r in monsters if r['Id']=='uberizual')['Skill1']=='Workshop Sorceress Legacy Frost Nova'
 assert m['frostnova']['Skill']=='Workshop Sorceress Legacy Frost Nova'
 assert m['workshop_conflagration_nova']['Skill']=='Inferno'
 pet=next(r for r in monsters if r['Id']=='workshop_frost_guardian');assert pet['neverCount']=='1' and pet['flying']=='0' and pet['Align']=='1'
 assert s[44]['sumskill1']==s[b['FrostNova']]['skill']
 assert s[44]['petmax']=="1+lvl/10+stat('ws_pet_count'.accr)"
 names={r['skill'] for r in skills}
 for r in skills:
  if r['charclass']=='sor':
   for field in ('reqskill1','reqskill2','reqskill3'):assert not r[field] or r[field] in names
 # Proc IDs and append-only states cannot collide with existing classes' IDs.
 assert len({b[k] for k in ('AvatarState','QueenState','MasterState')})==3
 assert all(b[k]>313-3 for k in ('AvatarState','QueenState','MasterState'))
print('PASS: Sorceress slots, states, mastery-scaled elemental proc chains, reservation and pet ownership data.')

registry=json.loads((root/'data/hd/missiles/missiles.json').read_text(encoding='utf-8-sig'))
assert registry['workshop_conflagration_nova']==registry['fireball']!='null'
assert registry['workshop_conflagration_ground'] not in ('','null')
for element in ('fire','cold','ltng'):
 assert registry['workshop_'+element+'_storm_fire'] not in ('','null')
print('PASS: visible moving Conflagration ring, ground flame and three storm-child registry entries.')

for folder in ('data/global/excel','data/global/excel/base'):
 _,rows=read(root/folder/'skills.txt');eruption=next(r for r in rows if r['*Id']=='49')
 assert eruption['calc1']=='-90' and eruption['ToHitCalc']=='100+10*lvl'
 assert eruption['restrict']=='1' and eruption['range']=='h2h'
print('PASS: Elemental Eruption melee, accuracy, ten-percent weapon strike and transformation eligibility.')

for folder in ('data/global/excel','data/global/excel/base'):
 _,rows=read(root/folder/'skills.txt');eruption=next(r for r in rows if r['*Id']=='49')
 assert eruption['mana']=='9' and eruption['lvlmana']=='1'
 _,rows=read(root/folder/'missiles.txt');wave=next(r for r in rows if r['Missile']=='workshop_conflagration_nova')
 assert wave['pCltDoFunc']=='16' and wave['CltSubMissile1']=='workshop_conflagration_ground'
print('PASS: native client fire trail and 8+level mana cost.')

for folder in ('data/global/excel','data/global/excel/base'):
 _,rows=read(root/folder/'skills.txt')
 assert next(r for r in rows if r['*Id']=='44')['restrict']=='1'
print('PASS: Frost Guardians can be summoned in Avatar of Fire.')

for folder in ('data/global/excel','data/global/excel/base'):
 _,rows=read(root/folder/'skills.txt')
 for element in ('fire','cold','ltng'):
  storm=next(r for r in rows if r['skill']=='Workshop '+element+' Storm')
  assert storm['EMin']=='24' and storm['EMax']=='48'
  for tier in range(1,6):assert storm['EMinLev'+str(tier)]=='6' and storm['EMaxLev'+str(tier)]=='12'
print('PASS: all Storms use 24-48 base with +6/+12 per additional level.')

for folder in ('data/global/excel','data/global/excel/base'):
 _,rows=read(root/folder/'skills.txt')
 for row in rows:
  if row['skill'] in ('Inferno','Workshop Conflagration Burn'):
   assert row['EMin']=='750' and row['EMax']=='1200'
   assert row['EDmgSymPerCalc']=="(skill('Fire Wall'.blvl)+skill('Blaze'.blvl))*5"
   for tier in range(1,6):assert row['EMinLev'+str(tier)]==row['EMaxLev'+str(tier)]=='30'
print('PASS: Conflagration nova and burn damage tripled, with both 5% hard-point synergies.')

for folder in ('data/global/excel','data/global/excel/base'):
 _,rows=read(root/folder/'skills.txt');skills={int(r['*Id']):r for r in rows if r['*Id'].isdigit()}
 for sid,key in ((36,'FireProc'),(39,'ColdProc'),(43,'LightningProc')):
  expected='('+'+'.join("skill('"+skills[other]['skill']+"'.blvl)" for other in (36,39,43) if other!=sid)+')*5'
  assert skills[sid]['EDmgSymPerCalc']==skills[b[key]]['EDmgSymPerCalc']==expected
print('PASS: forms and their Storms receive 5% damage per hard point in each other form.')

for folder in ('data/global/excel','data/global/excel/base'):
 _,rows=read(root/folder/'skills.txt');ss={int(r['*Id']):r for r in rows if r['*Id'].isdigit()}
 _,rows=read(root/folder/'skilldesc.txt');dd={r['skilldesc']:r for r in rows}
 for sid,low,high in ((36,10,20),(39,10,20),(43,1,23)):
  assert ss[sid]['EMin']==str(low) and ss[sid]['EMax']==str(high)
  assert ss[sid]['EMaxLev1']=='10'
 for sid in (36,39,43,41,44):
  assert dd[ss[sid]['skilldesc']]['dsc3line1']=='36' and dd[ss[sid]['skilldesc']]['dsc3line2']=='36'
 for sid in (44,b['FrostNova']):
  r=ss[sid];assert (r['EMin'],r['EMax'],r['EMinLev1'],r['EMaxLev1'])==('16','32','8','16')
  assert r['EDmgSymPerCalc']=="(skill('Ice Blast'.blvl)+skill('Frozen Orb'.blvl))*8"
 assert ss[44]['sumsk2calc']=="skill('Ice Blast'.blvl)" and ss[44]['sumsk3calc']=="skill('Frozen Orb'.blvl)"
print('PASS: lower synergy sections, reduced form bases and Frost Guardian damage/cap/synergy inheritance.')
