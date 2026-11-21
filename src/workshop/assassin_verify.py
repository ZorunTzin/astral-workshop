"""Check live Assassin IDs, finisher dispatch, positions, reservation and art."""
import json,os,struct
from pathlib import Path
from combat.port import read
root=Path(os.environ.get('WORKSHOP_MOD_ROOT',Path(__file__).resolve().parents[2]));w=Path(__file__).parent
b=json.loads((w/'ASSASSIN.json').read_text())
for folder in ('data/global/excel','data/global/excel/base'):
 _,rows=read(root/folder/'skills.txt');s={int(r['*Id']):r for r in rows if r['*Id'].isdigit()}
 _,rows=read(root/folder/'skilldesc.txt');d={r['skilldesc']:r for r in rows}
 slots=[tuple(d[r['skilldesc']][k] for k in ('SkillPage','SkillRow','SkillColumn')) for r in s.values() if r['charclass']=='ass']
 assert len(slots)==len(set(slots))==30
 for sid,name in b['skills'].items():
  sid=int(sid);assert d[s[sid]['skilldesc']]['str name']=='WorkshopAssassin'+str(sid)+'Name'
  assert d[s[sid]['skilldesc']]['str alt']==('WorkshopOmnislashHit' if sid==275 else d[s[sid]['skilldesc']]['str name'])
  for field in ('reqskill1','reqskill2','reqskill3'):
   if s[sid][field]:assert any(r['skill']==s[sid][field] and r['charclass']=='ass' for r in s.values())
 for sid,prereq in ((257,''),(260,'Dragon Talon'),(263,'Claw Mastery'),(265,'Tiger Strike'),(266,'Blade Sentinel'),(275,'Dragon Tail'),(276,''),(277,'Blade Fury'),(278,'Fade')):
  assert s[sid]['reqskill1']==prereq,(sid,s[sid]['reqskill1'])
 for sid in (256,266):
  assert s[sid]['srvdofunc']=='45' and s[sid]['pettype']=='assassintrap'
  assert s[sid]['petmax']==s[271]['petmax']
 assert s[260]['srvdofunc']=='52' and s[260]['finishing']=='1'
 assert s[275]['finishing']=='1' and s[275]['localdelay']=='250'
 omni=d[s[275]['skilldesc']]
 assert s[275]['SrcDam']=='128' and "skill('Cobra Strike'.blvl)" in s[275]['calc4']
 assert omni['descdam']=='25' and omni['ddam calc1']==omni['ddam calc2']=='clc4-100'
 assert omni['dsc3calca1']=='3' and omni['desccalca1']=='clc4'
 assert omni['desccalca3']=='10+lvl/5'
 f=s[b['bindings']['Finisher']];assert f['srvdofunc']=='46' and f['finishing']=='1' and f['SrcDam']=='128'
 assert f['calc1']=='100+25*lvl' and f['ToHitCalc']=='200+20*lvl'
 for sid in (263,278,276,257,277):
  assert s[sid]['srvdofunc']=='18' and s[sid]['aurastate'].startswith('workshop_assassin_')
  assert s[sid]['mana']=='0' and s[sid]['auralencalc']=='50'
 # DFS rejects self/circular prerequisite dependencies.
 seen=set()
 def visit(name,path):
  assert name not in path,(name,path)
  r=next((x for x in s.values() if x['skill']==name),None)
  if not r or name in seen:return
  for k in ('reqskill1','reqskill2','reqskill3'):
   if r[k]:visit(r[k],path+[name])
  seen.add(name)
 for r in s.values():
  if r['charclass']=='ass':visit(r['skill'],[])
 for name in ('asskillicon.sprite','asskillicon.lowend.sprite'):
  p=root/'data/hd/global/ui/spells/assassin'/name;raw=p.read_bytes();assert raw[:4] in (b'SPa1',b'SpA1')
mapping=json.loads((root/'data/hd/missiles/missiles.json').read_text())
assert mapping['workshop_assassin_visual_queen']=='expansion_throwing_star_missile'
assert mapping['workshop_assassin_visual_spikes']=='expansion_fire_spikes_ground'
monsters=json.loads((root/'data/hd/character/monsters.json').read_text())
for key in ('workshop_atmg_sentry','workshop_gravity_trap','workshop_afterimage'):assert monsters[key] and monsters[key]!='null'
print('PASS: all ten slots/names, unique tree positions, native weapon finisher, five reservation states, prerequisite DAG and paired Assassin art.')
