"""Validate Druid generation, original ID safety, native artwork and UI bindings."""
import csv,json,sys
from pathlib import Path
root=Path(sys.argv[1]) if len(sys.argv)>1 else Path(__file__).resolve().parents[1]
def read(p):return list(csv.DictReader(p.open(encoding='utf-8-sig'),delimiter='\t'))
for folder in ('data/global/excel','data/global/excel/base'):
 p=root/folder;skills={int(r['*Id']):r for r in read(p/'skills.txt') if r['*Id'].isdigit()};ds={r['skilldesc']:r for r in read(p/'skilldesc.txt')};mon={r['Id']:r for r in read(p/'monstats.txt')};miss={r['Missile']:r for r in read(p/'missiles.txt')};states={r['state']:r for r in read(p/'states.txt')}
 for sid in (221,224,230,239,240):
  r=skills[sid];d=ds[r['skilldesc']];assert r['srvdofunc']=='18' and r['restrict']=='1';assert d['str alt'].startswith('WorkshopDruid');assert d['str name'].startswith('WorkshopDruid');assert r['skilldesc']
 for sid in (221,224,239):assert skills[sid]['mana']=='0' and skills[sid]['auralencalc']=='50'
 assert states['workshop_wild_avatar']['gfxclass']=='289'
 assert "skill('Raven'.blvl)" in skills[221]['calc5'];assert "skill('Raven'.clc5)" in str(skills[247])
 for sid in (223,228):assert "skill('Shape Shifting'" not in str(skills[sid]);assert skills[sid]['auralencalc']=='(50+50*lvl)*25'
 for sid in (222,231,241):assert skills[sid]['petmax']=="1+lvl/10+stat('ws_pet_count'.accr)";assert int(mon[skills[sid]['summon']]['minHP'])==495
 for sid in (226,236,246):
  r=skills[sid];assert r['passivecalc1']=='min(50+lvl,85)' and r['passivecalc5']=='25';assert r['passivestat6']=='maxfireresist';assert int(mon[r['summon']]['minHP']) in (680,1700)
 assert skills[230]['EMin']=='15' and skills[230]['EMax']=='25' and skills[240]['MinDam']=='20' and skills[240]['MaxDam']=='30';assert ds[skills[230]['skilldesc']]['descdam']=='5' and ds[skills[240]['skilldesc']]['descdam']=='5'
 for name in ('workshop_storm_seed','workshop_winter_wave','workshop_winter_ground','workshop_wild_tornado','workshop_primal_fire'):
  r=miss[name];assert r['ClientSend']=='1' and r['CollideType']=='0' and r['SrcDamage']=='0' and r['pCltDoFunc']=='1';assert not r['SubMissile1'] and not r['CltSubMissile1']
 assert miss['workshop_druid_hit']['Explosion']=='0'
 for r in (r for r in skills.values() if r["charclass"]=="dru"):
  for k in ('reqskill1','reqskill2','reqskill3'):
   if r[k]:assert any(v['skill']==r[k] for v in skills.values()),r[k]
 statnames={r['Stat'] for r in read(p/'itemstatcost.txt')}
 for sid in (221,224,226,236,239,246,296,298):
  for k,v in skills[sid].items():
   if k.startswith(('aurastat','passivestat')) and k[-1:].isdigit() and 'calc' not in k and v:assert v in statnames,(sid,k,v)
 mapping=json.loads((root/'data/hd/missiles/missiles.json').read_text(encoding='utf-8-sig'))
 for name in ('workshop_storm_seed','workshop_winter_wave','workshop_winter_ground','workshop_wild_tornado','workshop_primal_fire'):assert mapping[name] and mapping[name]!='null',name
hud=read(root/'data/global/excel/d2rloader/buffhud/buff-hud.txt')
assert {221,224,239} <= {int(r['skill_id']) for r in hud}
for name in ('drskillicon.sprite','drskillicon.lowend.sprite'):assert (root/'data/hd/global/ui/spells/druid'/name).stat().st_size>100000
print('PASS: Druid stable slots, damage display, morph model, reservations/HUD, native visual aliases, separate Raven, integrated Lycanthropy, spirit life/resists and creeper count/life.')

# Regression: pet life is native computed HP, never a factor-only tooltip.
for sid in (222,226,231,236,241,246):
 d=ds[skills[sid]['skilldesc']];assert d['descline1']=='13' and d['desccalca1']=='clc1'
assert skills[221]['calc3'] and skills[221]['Param5']=='5' and skills[221]['petmax']
assert ds[skills[224]['skilldesc']]['SkillPage']=='1' and ds[skills[221]['skilldesc']]['SkillPage']=='2'
assert skills[230]['EDmgSymPerCalc']==skills[245]['EDmgSymPerCalc']
assert "fmmp_native_blaze_tag" in skills[240]['DmgSymPerCalc']
assert ds[skills[230]['skilldesc']]['desccalca5']=='3+lvl/10'
assert ds[skills[240]['skilldesc']]['desccalca5']=='3+lvl/10'
print('PASS: spirit/vine HP formatter, explicit aura lines, Raven hit budget, swapped tree positions and shared spell scaling.')
