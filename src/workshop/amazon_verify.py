"""Validate both generated Amazon tables and private projectile isolation."""
import json,os
from pathlib import Path
from combat.port import read
root=Path(os.environ.get('WORKSHOP_MOD_ROOT',Path(__file__).resolve().parents[2]));w=Path(__file__).parent
b=json.loads((w/'AMAZON.json').read_text())
visuals=json.loads((root/'data/hd/missiles/missiles.json').read_text(encoding='utf-8-sig'))
for folder in ('data/global/excel','data/global/excel/base'):
 _,rows=read(root/folder/'skills.txt');s={int(r['*Id']):r for r in rows if r['*Id'].isdigit()}
 _,rows=read(root/folder/'skilldesc.txt');d={r['skilldesc']:r for r in rows}
 _,rows=read(root/folder/'missiles.txt');m={int(r['*ID']):r for r in rows if r['*ID'].isdigit()}
 slots=[tuple(d[r['skilldesc']][k] for k in ('SkillPage','SkillRow','SkillColumn')) for r in s.values() if r['charclass']=='ama']
 assert len(slots)==len(set(slots))==30
 for sid,fn,state,helper,base,growth in b['auras']:
  r=s[sid];assert r['srvdofunc']=='18' and r['auralencalc']=='50'
  assert r['aurastate']=='workshop_amazon_'+str(sid)
  assert base==15 and growth==3 and s[helper]['calc4']=="skill('"+r['skill']+"'.lvl)"
  assert not any(v for k,v in r.items() if k.startswith(('passivecalc','passivestat')))
  if sid in (18,28):assert not any(r['aurastat'+str(i)] in ('velocitypercent','attackrate','maxfireresist','maxcoldresist','maxlightresist','maxpoisonresist') for i in range(1,7))
 assert s[12]['ToHitCalc']=='100+5*lvl'
 assert s[19]['srvdofunc']=='18' and s[19]['calc4']=='100+clc1'
 assert not s[19]['TargetableOnly'] and not s[19]['SearchEnemyXY']
 assert s[20]['calc1']=="1+skill('Lightning Fury'.blvl)/5+skill('Charged Strike'.blvl)/5"
 assert s[25]['calc1']=='1+lvl/5' and s[25]['globaldelay']==s[25]['localdelay']=='0'
 assert not s[30]['aurastat3'] and not s[30]['aurastatcalc3']
 assert s[24]['calc1']=='3+lvl/5' and s[14]['calc1']=='8+lvl'
 ids=b['bindings']
 # Skill IDs and missile IDs are independent namespaces.
 assert s[ids['PounceVisual']]['cltdofunc']=='25'
 assert m[ids['PounceRing']]['Range']=='10'
 for sid in (24,25,14):
  for suffix in ('a','b'):
   assert s[sid]['srvmissile'+suffix]==s[sid]['cltmissile'+suffix], (sid,suffix)
 for sid in (24,14):assert s[sid]['srvmissilec']==s[sid]['srvmissilea']
 for sid in (12,):
  assert not s[sid]['cltdofunc']
  assert not any(s[sid]['cltmissile'+suffix] for suffix in ('a','b','c'))
  assert s[sid]['srvmissilea']
 assert m[ids['Toxic']]['VelLev']=='6'

 for n in range(4):
  r=m[ids['Split'+str(n)]];assert r['Pierce']=='0' and r['pSrvDoFunc']=='1' and r['pCltDoFunc']=='1'
  assert r['CelFile']=='Arrow' and r['Trans'] in ('','0')
  assert visuals[r['Missile']]=='arrow'
 for key in ('MultiArrow','MultiBolt','Return','Split0','Split1','Split2','Split3'):
  assert m[ids[key]]['ClientSend']=='0',key
 assert m[ids['Return']]['Vel']=='24' and m[ids['Return']]['VelLev'] in ('','0')
 assert m[ids['Return']]['Pierce']=='0'
 for sid in (6,7,11):assert m[ids['Arrow'+str(sid)]]['HitSubMissile1']==m[ids['Blast'+str(sid)]]['Missile']
 seen=set()
 def visit(name,path):
  assert name not in path,(name,path)
  r=next((x for x in s.values() if x['skill']==name),None)
  if not r or name in seen:return
  for k in ('reqskill1','reqskill2','reqskill3'):
   if r[k]:visit(r[k],path+[name])
  seen.add(name)
 for r in s.values():
  if r['charclass']=='ama':visit(r['skill'],[])
print('PASS: Amazon stable slots, prerequisites, four independent buffs, covenant penalties removed, private arrows, Pounce and javelin counts in both table sets.')

art=json.loads((root/'data/hd/missiles/workshop_split_arrow.json').read_text())
components=[c for e in art['entities'] for c in e['components']]
assert any(c['type']=='ModelDefinitionComponent' and c['filename']=='data/hd/missiles/arrow/arrow.model' and c['visibleLayers']==1 for c in components)
assert all(c['type'] in ('UnitRootComponent','ModelDefinitionComponent','TransformDefinitionComponent') for c in components)
print('PASS: native arrow HD mapping for all split generations; previous experimental asset is unused.')

assert s[22]['srvdofunc']=='8' and s[22]['cltdofunc']=='17'
assert s[22]['calc1']=='1' and s[22]['calc4']
assert s[22]['cltmissilea']==s[22]['srvmissilea']=='workshop_amazon_split0'
