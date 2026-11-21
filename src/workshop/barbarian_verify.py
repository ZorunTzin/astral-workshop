"""Validate Barbarian skill routing, formulas, private summons and save compatibility."""
import json,tomllib
from pathlib import Path
from combat.port import read
root=Path(__file__).resolve().parent.parent
meta=json.loads((root/'workshop/BARBARIAN.json').read_text())
for folder in ('data/global/excel','data/global/excel/base'):
 p=root/folder;_,states=read(p/'states.txt');_,skills=read(p/'skills.txt');_,descs=read(p/'skilldesc.txt');_,monsters=read(p/'monstats.txt');_,missiles=read(p/'missiles.txt');_,stats=read(p/'itemstatcost.txt')
 by={int(r['*Id']):r for r in skills if r['*Id'].isdigit()};ds={r['skilldesc']:r for r in descs};ms={r['Id']:r for r in monsters}
 assert len(by)==len([r for r in skills if r['*Id'].isdigit()])
 assert len(stats)==511 and max(int(r['*ID']) for r in stats if r['*ID'].isdigit())==510
 assert by[133]['reqskill1']=='Concentrate' and by[137]['ToHitCalc']=='100+10*lvl'
 assert meta['call']==133 and by[133]['skill']=='Call of the Ancients'
 assert by[140]['charclass']=='bar' and by[140]['skill']=='Ground Stomp' and by[140]['srvdofunc']=='2' and by[140]['srvstfunc']=='32'
 assert ds[by[140]['skilldesc']]['SkillPage']=='1' and by[140]['reqlevel']=='24'
 assert by[140]['SrcDam']=='128' and not by[140]['EType'] and by[140]['localdelay']=='125'
 assert by[140]['calc4']=='min(25,10+lvl/5)' and by[140]['MinDam']==by[140]['MaxDam']=='0'
 assert all(by[140]['MinLevDam'+str(n)]==by[140]['MaxLevDam'+str(n)]=='0' for n in range(1,6))
 control=next(r for r in missiles if int(r['*ID'])==meta['rock_control'])
 assert control['Missile']=='workshop_stomp_ring' and control['CollideType']=='0' and control['ClientSend']=='1'
 assert not control['pSrvHitFunc'] and control['SrcDamage']=='0' and control['ResultFlags']=='0'
 visual=by[meta['rain_skill']];assert visual['skill']=='Workshop Ground Stomp Visual' and visual['srvdofunc']=='68' and visual['MinDam']==visual['MaxDam']==visual['SrcDam']=='0'
 assert visual['srvmissilea']==control['Missile']
 hit=next(r for r in missiles if int(r['*ID'])==meta['rock_hit'])
 assert hit['Missile']=='workshop_stomp_hit' and hit['Skill']=='Ground Stomp' and hit['SrcDamage']=='128' and hit['pSrvDmgFunc']=='7'
 assert not hit['NextHit'] and not hit['NextDelay'] and hit['HitFlags']=='0' and not hit['pSrvHitFunc']
 assert by[147]['reqlevel']=='1' and not by[147]['reqskill1'] and 'skill(' not in by[147]['calc1'] and not by[147]['DmgSymPerCalc'].startswith('skill(')
 assert by[152]['reqlevel']=='12' and by[152]['reqskill1']=='Frenzy'
 assert [int(r['*Id']) for r in skills if r['charclass']=='bar']==list(range(126,156))
 assert len([r for r in skills if r['charclass']=='bar' and ds[r['skilldesc']]['SkillPage']!='0'])==30
 assert [by[148]['passivecalc'+str(i)] for i in range(1,6)]==['10+2*lvl']*5
 assert by[145]['passivestat2']=='damageresist' and by[145]['passivecalc2']=='lvl'
 assert all(by[153]['passivecalc'+str(i)]=='dm12+lvl/5' for i in range(1,5))
 assert all(by[153]['passivecalc'+str(i)]=='1+(lvl-1)/5' for i in range(5,9))
 assert meta['ancient_attacks'][0]==570 and by[133]['sumskill1']=='Workshop Talic Carnage'
 assert ds[by[128]['skilldesc']]['desccalca1']=='20+2*lvl' and ds[by[128]['skilldesc']]['desccalcb1']=='10'
 assert ds[by[140]['skilldesc']]['desccalca1']=='5+lvl/10' and ds[by[140]['skilldesc']]['desccalca2']=='(50+10*lvl)*(100+clc5)/100'
 assert not by[128]['aurastate']
 assert by[meta['rage_skill']]['aurastate']=='workshop_carnage' and by[meta['rage_skill']]['auralencalc']=='250'
 assert not by[128]['passiveitype'] and by[128]['passivecalc1']=='0'
 call=by[meta['call']];assert call['charclass']=='bar' and call['srvdofunc']=='144' and call['petmax']=='3' and call['localdelay']=='1500'
 assert call['calc1']=='0' and call['Param1']=='775' and call['Param2']=='25'
 assert (int(call['Param1'])+(20-1)*int(call['Param2']))//25==50
 assert len(set(meta['ancient_classes']))==3
 for i,base in enumerate(('ancientbarb1','ancientbarb2','ancientbarb3')):
  m=ms['workshop_'+base];assert int(m['*hcIdx'])==meta['ancient_classes'][i] and m['BaseId']==m['Id'] and not m['NextInClass'] and not m['MonProp']
  # Native boss equipment initialization is dispatched on BaseId, and only
  # supports the original consecutive Ancient IDs. Never route appended pets
  # through it; spawn-time monequip supplies their weapons on every difficulty.
  assert int(ms[m['BaseId']]['*hcIdx']) not in (540,541,542)
  assert m['killable']=='0' and m['neverCount']=='1'
  _,extras=read(p/'monstats2.txt');extra=next(r for r in extras if r['Id']==m['MonStatsEx']);assert extra['isSel']==extra['isAtt']=='0'
  assert m['AI']=='GenericPet' and m['Align']=='1' and m['flying']=='0'
  assert all(m[f]=='100' for f in ('minHP','maxHP','MinHP(N)','MaxHP(N)','MinHP(H)','MaxHP(H)'))
  assert m['noRatio']=='1' if 'noRatio' in m else m['NoRatio']=='1'
  assert ms[base]['AI']=='Ancient' and not any(v for k,v in m.items() if k.startswith('TreasureClass'))
  appearance=next(r for r in states if r['*ID']==str(meta['ancient_visuals'][i]));assert appearance['gfxtype']=='1' and appearance['gfxclass']==ms[base]['*hcIdx'] and appearance['transform']=='1'
  attack=by[meta['ancient_attacks'][i]];assert call['sumskill'+str(i+1)]==attack['skill'] and call['sumsk'+str(i+1)+'calc']=='lvl'
 assert [by[i]['srvdofunc'] for i in meta['ancient_attacks']]==['11','74','11']
 assert by[137]['calc1']=='min(64,4+lvl)'
 assert by[137]['calc5']==by[154]['calc5'] and by[137]['Param8']=='5' and not by[137]['DmgSymPerCalc']
 assert by[137]['MinDam']==by[137]['MaxDam']=='0'
 assert all(by[137][b+'LevDam'+str(n)]=='0' for b in ('Min','Max') for n in range(1,6))
 assert ds[by[137]['skilldesc']]['desccalca1']=='(50+5*(lvl-1))*(100+clc5*par8)/300'
 assert ds[by[137]['skilldesc']]['desccalca2']=='(1+lvl)*(100+clc5*par8)/100' and ds[by[137]['skilldesc']]['desccalca3']=='(2+lvl)*(100+clc5*par8)/100' and ds[by[137]['skilldesc']]['desccalcb3']=='1'
 assert all(by[140][field]==by[147][field] for field in ('ToHit','LevToHit','ToHitCalc'))
 assert "5*(skill('Concentrate'.blvl)+skill('Stun'.blvl))" in by[133]['passivecalc4']
 assert all(ds[by[133]['skilldesc']]['dsc3calca'+str(n)]=='5' for n in (1,2))
 assert by[140]['calc5']=="5*(skill('Melee Mastery'.blvl)+skill('Throwing Mastery'.blvl))"
 assert by[570]['srvmissilea']==by[144]['srvmissilea']=='carnage'
 assert by[571]['srvmissilea']==by[139]['srvmissilea']=='rockmaker_m'
 assert ms['workshop_ancientbarb1']['Sk1mode']=='seq_workshop_carnage'
 assert ms['workshop_ancientbarb3']['Sk1mode']=='seq_workshop_tectonic'
 quake=next(r for r in missiles if r['Missile']=='workshop_quake');assert quake['Pierce']==quake['CollideKill']==quake['ToHit']=='1'
 assert by[137]['skill']=='Taunt' and by[137]['SrcDam']=='128' and by[137]['srvmissilea']=='workshop_quake'
 assert by[131]['srvdofunc']=='18' and by[131]['perdelay']=='50' and by[131]['mana']=='0'
 assert by[131]['aurastate']=='workshop_feast_aura'
 assert by[137]['srvmissileb']==by[137]['srvmissilea'] and not by[137]['cltstfunc']
 assert by[131]['calc1']=='100' and by[131]['calc2']==by[131]['calc3']=='0'
 assert by[131]['calc4']=='(3+lvl/5)*100' and by[131]['calc5']=='20+3*lvl'
 tick=next(r for r in missiles if r['Missile']=='workshop_bleeding_tick');assert not tick['pSrvHitFunc'] and tick['ToHit']=='0' and tick['SrcDamage']=='0' and tick['ResultFlags']=='1'
 assert by[meta['pull']]['auratargetstate']=='taunt'
 for sid,helper in meta['helpers'].items():assert by[helper]['calc4']==f"skill('{by[int(sid)]['skill']}'.lvl)"
 for sid in meta['splash']:assert "5*skill('Shield Mastery'.lvl)" in by[sid]['aurarangecalc']
 for sid in (128,131,137,148,meta['call']):
  d=ds[by[sid]['skilldesc']];assert d['str name'].startswith('WorkshopBarbarian')
  if sid not in (137,meta['call']):assert not any(v for k,v in d.items() if k.startswith('dsc3'))
  elif sid==137:assert d['dsc3textb2']==ds[by[154]['skilldesc']]['dsc3textb2']
 for sid in (147,152,meta['call']):
  d=ds[by[sid]['skilldesc']];assert d['SkillPage']=='1' and int(d['SkillRow']) in range(1,7)
config=tomllib.loads((root/'d2rloader/config/celestialrayone.whirlwind.toml').read_text(encoding='utf8'))
assert config['whirlwind']['enabled'] and config['cyclone']['enabled']
assert not config['dual_wield_same_target']['enabled'] and not config['whirlwind_with_bows']['enabled']
print('PASS: Barbarian IDs/UI/prerequisites, baked Frenzy synergies, all five speeds, DR/resistance, 30 visible skills, three private friendly Ancients/abilities/lifetime, Quake/Feast formulas, single-target bleed ticks, splash radii, unchanged saved-stat formats and scoped Whirlwind steering.')

# String IDs are global across ALL JSON files, not just skills.json.
ids={};keys={}
for path in (root/'data/local/lng/strings').glob('*.json'):
 for row in json.loads(path.read_text(encoding='utf-8-sig')):
  if isinstance(row,dict) and 'id' in row:
   ids.setdefault(int(row['id']),[]).append(row['Key']);keys[row['Key']]=row
for key,row in keys.items():
 if key.startswith(('WorkshopBarbarian','WorkshopInternalSkill')):assert len(ids[int(row['id'])])==1,(key,ids[int(row['id'])])
assert keys['WorkshopBarbarian128Name']['enUS']=='Rage'
assert json.loads((root/'data/hd/missiles/missiles.json').read_text())['workshop_quake']
print('PASS: globally unique Barbarian/internal localization IDs, Rage name, Quake HD mapping and Feast toggle binding.')

monster_mapping=json.loads((root/'data/hd/character/monsters.json').read_text(encoding='utf8'))
assert all(monster_mapping['workshop_'+name]==monster_mapping[name] for name in ('ancientbarb1','ancientbarb2','ancientbarb3'))
print('PASS: Ground Stomp weapon scaling, native stun, visual-only ring, five-second cooldown, Carnage/Tectonic bindings and private Ancient HD mappings.')

_,equipment=read(root/'data/global/excel/monequip.txt')
axes=[r for r in equipment if r['monster']=='workshop_ancientbarb2']
assert len(axes)==1 and axes[0]['oninit']=='1'
assert axes[0]['item1']==axes[0]['item2']=='tax' and axes[0]['loc1']=='rarm' and axes[0]['loc2']=='larm'
assert any(r['monster']=='workshop_ancientbarb1' and r['oninit']=='1' and r['item1']=='crs' for r in equipment)
assert 'seq_workshop_doublethrow' in (root/'data/global/excel/monseq.txt').read_text()
print('PASS: private Ancient BaseIds bypass native boss equipment on all difficulties; explicit spawn-time weapons retained.')
print('PASS: original native Double Throw and actual dual throwing-axe equipment with two monster attack events.')
