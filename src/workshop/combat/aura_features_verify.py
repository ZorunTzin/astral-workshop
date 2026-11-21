"""Validate replacement data, formula links and untouched legacy item auras."""
import sys,json
from pathlib import Path
from port import read
root=Path(sys.argv[1]);here=Path(__file__).resolve().parent
bindings=json.loads((here/'FEATURES.json').read_text())
for folder in ('data/global/excel','data/global/excel/base'):
 p=root/folder;_,skills=read(p/'skills.txt');_,descs=read(p/'skilldesc.txt')
 byid={int(r['*Id']):r for r in skills if r['*Id'].isdigit()}
 for skill,stat,rate in ((100,'workshop_reservation_efficiency',4),(105,'workshop_aura_effect',3)):
  r=byid[skill];assert r['passive']=='1' and r['passivestat1']==stat and r['passivecalc1']==f'{rate}*lvl'
  assert not r['srvdofunc'] and r['rightskill']=='0'
 guardian=byid[110];assert guardian['summon']=='workshop_astral_guardian' and 'ws_pet_count' in guardian['petmax']
 assert guardian['srvdofunc']=='16' and guardian['passivestat9']=='damagepercent'
 assert sum(guardian[k].count('.blvl') for k in ('calc4','calc5','calc6'))==17
 assert guardian['calc1']=="100+20*(lvl-1)+10*(clc4+clc5+clc6)+stat('ws_pet_life'.accr)"
 assert guardian['calc3']=="8*(clc4+clc5+clc6)+stat('ws_pet_damage'.accr)"
 assert max(len(guardian[k]) for k in ('calc1','calc3','calc4','calc5','calc6'))<200
 assert "skill('Resist Fire'" not in guardian['calc1'] and "skill('Resist Cold'" not in guardian['calc1']
 assert guardian['passivecalc9']=='clc3'
 for source,hidden in bindings['legacy'].items():
  r=byid[hidden];assert r['srvdofunc']=='65' and r['aura']=='1' and not r['charclass'] and r['skilldesc'].startswith('workshop_internal_')
  assert r['aurastat1'] and 'workshop_' not in r['aurastat1']
 nova=byid[bindings['nova_skill']];assert not nova['EType'] and nova['MinDam']==nova['MaxDam']=='1'
 assert 'workshop_retribution_charge' in nova['DmgSymPerCalc']
 assert nova['calc1']==nova['cltcalc1'] and '3*(lvl-1)' in nova['calc1']
 assert 'workshop_aura_effect' in nova['calc1']
 _,states=read(p/'states.txt');_,overlays=read(p/'overlay.txt')
 ground=next(r for r in states if r['state']=='workshop_sanctified')
 overlay_names={r['overlay'] for r in overlays}
 assert ground['overlay1'] in overlay_names and ground['overlay2'] in overlay_names
 _,missiles=read(p/'missiles.txt');m=next(r for r in missiles if r['*ID']==str(bindings['nova_missile']))
 assert m['Skill']==nova['skill'] and m['Missile']!= 'nova'
 freeze=byid[114];assert freeze['aurastatcalc2']==freeze['aurastatcalc3']=='ast1'
 assert freeze['aurastatcalc1'].startswith('(0-min(90,')
 for skill in (100,105,110,103,109):
  desc=next(r for r in descs if r['skilldesc']==byid[skill]['skilldesc'])
  assert desc['str name'].startswith('Workshop')
  assert not any(v for k,v in desc.items() if k.startswith(('dsc3line','dsc3text','dsc3calc')))
print('PASS: passive formulas, seventeen guardian hard-point sources, native summon route, five original item-aura copies, physical nova, nonduplicated capped slow and new tooltips.')
