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
 assert len(slots)==len(set(slots))==35
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

 for n in range(5):
  r=m[ids['Split'+str(n)]];assert r['Pierce']=='0' and r['pSrvDoFunc']=='1' and r['pCltDoFunc']=='1'
  assert r['CelFile']=='Arrow' and r['Trans'] in ('','0')
  assert visuals[r['Missile']]=='workshop_split_arrow'
 for key in ('MultiArrow','MultiBolt','Return','Split0','Split1','Split2','Split3','Split4'):
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
print('PASS: dedicated standalone HD arrow model for all five split stages.')

assert s[22]['srvdofunc']=='8' and not s[22]['cltdofunc']
assert s[22]['calc1']=='1' and s[22]['calc4']
assert not s[22]['cltmissilea'] and s[22]['srvmissilea']=='workshop_amazon_split0'

# Physical bow curves match their paired melee skills at every damage breakpoint.
for name,source,shift,base,growth in [('Multiple Shot','Jab',6,8,1),('Guided Arrow','Impale',5,32,-1),('Strafe','Fend',7,11,0)]:
 bow=next(r for r in s.values() if r['skill']==name);melee=next(r for r in s.values() if r['skill']==source)
 for k in ['MinDam','MaxDam']+[p+str(i) for p in ('MinLevDam','MaxLevDam') for i in range(1,6)]:assert int(bow[k] or 0)==(int(melee[k] or 0)*256*4//5 if name=='Guided Arrow' else int(melee[k] or 0)),(name,k)
 assert int(bow['manashift'])==shift and int(bow['mana'])==base and int(bow['lvlmana'])==growth
 for level in range(1,33):
  before=(base+(level-1)*growth)*(1<<(shift+1));after=(base+(level-1)*growth)*(1<<shift)
  assert after*2==before
 partners=[n for n in ('Multiple Shot','Guided Arrow','Strafe') if n!=name]
 for partner in partners:assert "skill('%s'.blvl)"%partner in bow['DmgSymPerCalc']
 assert (bow['DmgSymPerCalc'].endswith('*8') if name=='Guided Arrow' else bow['DmgSymPerCalc'].endswith('*12'))
 tip=d[bow['skilldesc']]
 assert any(tip.get('desctexta'+str(i))=='StrSkillAddedFlatPhys' for i in range(1,7))
 for i,partner in enumerate(partners,2):assert tip['dsc3calca'+str(i)]==('8' if name=='Guided Arrow' else '12')
for key,name in [('MultiArrow','Multiple Shot'),('MultiBolt','Multiple Shot'),('Split0','Guided Arrow')]:assert m[ids[key]]['Skill']==name
assert s[26]['EDmgSymPerCalc']=="skill('Magic Arrow'.blvl)*12"
assert d[s[26]['skilldesc']]['dsc3calca4']=='12'
print('PASS: paired physical bow curves, mutual 12% synergies, preserved Magic Arrow synergy, half mana and flat-damage tooltips.')

# Unity is isolated from Valkyrie and uses no targetable/colliding actors.
for folder in ('data/global/excel','data/global/excel/base'):
 tables={name:read(root/folder/(name+'.txt'))[1] for name in ('skills','monstats','monstats2','monequip','states','missiles')}
 unity=next(r for r in tables['skills'] if r['skill']=='Amazonian Unity')
 assert unity['LineOfSight']=='0'
 assert unity['reqlevel']=='30' and unity['localdelay']=='250' and unity['mana']=='25'
 assert unity['itypea1']=='bow' and unity['itypea2']=='xbow' and unity['reqskill1']=='Strafe'
 mon=next(r for r in tables['monstats'] if r['Id']=='workshop_unity_amazon')
 assert mon['killable']=='0' and mon['AI']=='Idle' and mon['Velocity']=='0' and mon['Run']=='0'
 assert mon['Align']=='1' and not any(v for k,v in mon.items() if k.startswith('TreasureClass'))
 extra=next(r for r in tables['monstats2'] if r['Id']==mon['Id'])
 for key in ('isSel','alSel','shiftSel','isAtt','corpseSel','objCol','deadCol','SizeX','SizeY','mWL','mRN'):assert extra[key]=='0',key
 assert extra['noSel']=='1'
 state=next(r for r in tables['states'] if r['state']==mon['Id'])
 assert state['transform']=='1' and state['gfxtype']=='2' and state['gfxclass']=='0'
 eq=next(r for r in tables['monequip'] if r['monster']==mon['Id'])
 assert eq['item1']=='lbw' and eq['loc1']=='rarm' and eq['oninit']=='1'
 arrow=next(r for r in tables['missiles'] if r['Missile']=='workshop_unity_arrow')
 assert arrow['Skill']=='Amazonian Unity' and arrow['NextHit']=='0' and arrow['ClientSend']=='0'
print('PASS: Unity level 30, bow tree, ten-second cooldown, private untargetable/invulnerable nonblocking Amazon actors and arrows.')

for folder in ('data/global/excel','data/global/excel/base'):
 d=next(r for r in read(root/folder/'skilldesc.txt')[1] if r['skilldesc']=='workshop_amazonian_unity')
 assert d['IconCel']=='66'
 assert d['descdam']=='7' and d['ddam calc1']=='clc4-100' and d['ddam calc2']=='0'
 assert d['str alt']=='WorkshopUnityCharacter' and not d['descatt']
 assert [d['desccalca'+str(i)] for i in range(1,7)]==['clc4','6','5','10','10','25']
 assert d['desctexta6']=='WorkshopUnityMana' and d['dsc2texta2']=='WorkshopUnityTotal'
print('PASS: Unity dedicated icon, complete numeric tooltip and per-arrow character-sheet display.')

for folder in ('data/global/excel','data/global/excel/base'):
 eq=[r for r in read(root/folder/'monequip.txt')[1] if r['monster']=='workshop_unity_amazon']
 assert len(eq)==2 and {(r['item1'],r['loc1']) for r in eq}=={('lbw','rarm'),('lea','tors')}
 assert all(not r['item2'] and not r['item3'] for r in eq)
 arrow=next(r for r in read(root/folder/'missiles.txt')[1] if r['Missile']=='workshop_unity_arrow')
 assert arrow['ToHit']=='0' and arrow['NextHit']=='0' and arrow['NextDelay']=='0'
labels=json.loads((root/'data/local/lng/strings/skills.json').read_text(encoding='utf-8-sig'))
for r in labels:
 if r['Key'].startswith('WorkshopUnity'):
  assert max(map(len,r['enUS'].splitlines()))<=40,(r['Key'],r['enUS'])
print('PASS: Unity guaranteed bow plus armor, short tooltip lines, independent accurate arrow collisions without NextHit suppression.')

for folder in ('data/global/excel','data/global/excel/base'):
 by={r['skill']:r for r in read(root/folder/'skills.txt')[1]};u=by['Amazonian Unity']
 for k in ['MinDam','MaxDam']+[p+str(i) for p in ('MinLevDam','MaxLevDam') for i in range(1,6)]:assert int(u[k])==(int(by['Strafe'][k])//2 if k in ('MinDam','MaxDam') else 2*int(by['Strafe'][k]))
 assert u['DmgSymPerCalc']=="(skill('Multiple Shot'.blvl)+skill('Guided Arrow'.blvl)+skill('Strafe'.blvl))*8"
 assert u['calc4'].startswith('(9+lvl)*') and u['MinDam']=='1' and u['MaxDam']=='2'
 assert u['HitShift']=='7'
 # Check exact half damage, including fractional points, at every tier edge.
 import re
 for level in (1,2,8,9,16,17,22,23,28,29,40):
  for points in (0,1,20):
   for prefix,calc in (('Min','calc1'),('Max','calc2')):
    raw=int(u[prefix+'Dam'])+sum(int(u[prefix+'LevDam'+str(i)])*(min(max(level-start,0),count) if count else max(level-start,0)) for i,(start,count) in enumerate(((1,7),(8,8),(16,6),(22,6),(28,None)),1))
    expression=re.sub(r"skill\('[^']+'\.blvl\)",str(points),u[calc]).replace('lvl',str(level)).replace('/','//')
    assert eval(expression,{'__builtins__':{},'min':min,'max':max})==(raw*(100+points*24)//100)*128
 dummy=next(r for r in read(root/folder/'monstats.txt')[1] if r['Id']=='workshop_training_dummy')
 assert dummy['AI']=='Idle' and dummy['Velocity']=='0' and dummy['Run']=='0' and dummy['DamageRegen']=='0'
 assert all(not v for k,v in dummy.items() if k.startswith('TreasureClass'))
 assert all(v=='0' for k,v in dummy.items() if k.startswith('Exp'))
print('PASS: Unity halved complete flat curve with exact fractional fixed-point damage, three 8% synergies; private idle dummy without XP or drops.')

for folder in ('data/global/excel','data/global/excel/base'):
 by={r['skill']:r for r in read(root/folder/'skills.txt')[1]};shot=by['Big Shot']
 assert 'Guided Arrow' in by and shot['localdelay']=='0' and shot['reqlevel']=='30' and shot['calc4']=='clc3/3' and shot['calc3']=='(95+5*lvl)*(100+'+by['Amazonian Unity']['DmgSymPerCalc']+')/100'
 for k in ['MinDam','MaxDam','HitShift','DmgSymPerCalc','calc1','calc2']+[p+str(i) for p in ('MinLevDam','MaxLevDam') for i in range(1,6)]:assert shot[k]==by['Amazonian Unity'][k]
 assert (shot['mana'],shot['lvlmana'],shot['manashift'],shot['leftskill'])==('20','1','7','1')
 assert shot['itypea1']=='bow' and shot['itypea2']=='xbow'
 desc=next(r for r in read(root/folder/'skilldesc.txt')[1] if r['skilldesc']==shot['skilldesc'])
 assert desc['desccalca6']=='usmc' and desc['desccalcb6']=='256'
 ds={r['skilldesc']:r for r in read(root/folder/'skilldesc.txt')[1]};d=ds[shot['skilldesc']]
 assert d['IconCel']=='68'
 assert d['desccalca3']=='6' and all(d['dsc3calca'+str(i)]=='8' for i in (2,3,4))
 assert (d['SkillPage'],d['SkillRow'],d['SkillColumn'])==('1','6','3')
 assert sum((r['SkillPage'],r['SkillRow'],r['SkillColumn'])==('1','6','3') for r in ds.values() if r['skilldesc'] in {v['skilldesc'] for v in by.values() if v['charclass']=='ama'})==1
 ms={r['Missile']:r for r in read(root/folder/'missiles.txt')[1]}
 arrow=ms['workshop_big_shot'];nova=ms['workshop_big_shot_nova']
 assert arrow['Vel']=='6' and arrow['CollideType']=='6' and arrow['Skill']=='Big Shot' and arrow['Range']=='125'
 assert nova['CollideType']=='0' and nova['SrcDamage']=='0' and nova['ClientSend']=='0'
art=json.loads((root/'data/hd/missiles/workshop_big_shot.json').read_text())
assert any(c.get('scale')=={'x':32.0,'y':4.0,'z':6.0} for e in art['entities'] for c in e['components'])
print('PASS: Big Shot independent slot, fourth Split Arrow stage, no cooldown, slow barrier-stopped carrier, visual-only nova and massive arrow model.')

for folder in ('data/global/excel','data/global/excel/base'):
 ss={r['skill']:r for r in read(root/folder/'skills.txt')[1]};s=ss['Guided Arrow']
 assert not s['cltdofunc'] and not any(s['cltmissile'+suffix] for suffix in ('','a','b','c'))
 d=next(r for r in read(root/folder/'skilldesc.txt')[1] if r['skilldesc']==s['skilldesc'])
 assert any(d.get('dsc2texta'+str(i))=='WorkshopAmazon22_dsc22' and d.get('dsc2calca'+str(i))=='4' for i in range(1,6)),d
assert '8 -> 16' in next(r['enUS'] for r in labels if r['Key']=='WorkshopAmazon22Desc')
print('PASS: Split Arrow explicit server visuals, dedicated HD mesh, five stages and four-split tooltip.')

for folder in ('data/global/excel','data/global/excel/base'):
 rows=read(root/folder/'missiles.txt')[1];hit=next(r for r in rows if r['Missile']=='workshop_big_shot_hit')
 assert hit['SrcDamage']=='128' and hit['Skill']=='Big Shot' and hit['HitFlags']=='0' and hit['ResultFlags']=='4'
print('PASS: Big Shot native weapon hit metadata and procs enabled for direct and nova hits.')
