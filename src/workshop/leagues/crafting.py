"""League equipment crafting: stable saved stats, exact additive bundles."""
from pathlib import Path
import json
from combat.port import read,write
COST=[2,4,6,10]
# Each tuple is (native property, values by tier, parameter).
RULES=[
 [('dmg%',[3,8,15,24],0),('dmg-max',[0,0,0,5],0)],
 [('hp',[5,12,21,36],0),('hp%',[0,0,0,1],0)],
 [('mana',[5,12,21,36],0),('mana%',[0,0,0,1],0)],
 [('ws-orb-spell',[1,4,9,16],0),('cast2',[0,0,0,10],0)],
 [('ac%',[5,12,21,36],0),('red-dmg%',[0,0,0,1],0)],
 [('res-all',[1,4,9,16],0),('res-all-max',[0,0,0,1],0)],
 [('ws-bleed',[1,4,6,16],0),('ws-impale',[0,0,0,5],0)],
 [('mag%',[3,8,15,24],0),('gold%',[0,0,0,20],0)],
 [('dmg-fire',[(1,3),(4,8),(9,16),(16,25)],0),('dmg-ltng',[(1,3),(4,8),(9,16),(16,25)],0),('dmg-cold',[(1,3),(4,8),(9,16),(16,25)],25),('ws-orb-nova',[0,0,0,1],0)]]
STATS=[(262,'item_absorb_slash','ws_orb_spell_damage','ws-orb-spell',12,'WorkshopOrbSpell','Spell Damage',2),
 (263,'item_absorb_crush','ws_orb_trinity_chance','ws-orb-nova',10,'WorkshopOrbTrinity','Chance to cast level 60 Trinity Nova on Striking',2),
 (264,'item_absorb_thrust','ws_orb_level_cost','ws-orb-level',7,'WorkshopOrbLevel','Required Level from Orbs',1),
 (265,'item_absorb_slash_percent','ws_orb_cube_budget','',0,'','',0)]
LABELS={'dmg%':'% Enhanced Weapon Damage','dmg-max':' Maximum Damage','hp':' Life','hp%':'% Maximum Life','mana':' Mana','mana%':'% Maximum Mana','ws-orb-spell':'% Spell Damage (all types)','cast2':'% Faster Cast Rate','ac%':'% Defense','red-dmg%':'% Physical Damage Reduction','res-all':' All Resistances','res-all-max':'% All Maximum Resistances','ws-bleed':'% Bleed Chance','ws-impale':'% Impale Chance','mag%':'% Magic Find','gold%':'% Gold Find','dmg-fire':' Fire Damage','dmg-ltng':' Lightning Damage','dmg-cold':' Cold Damage','ws-orb-nova':'% Chance to cast level 60 Trinity Nova on Striking'}
def describe(family,group,tier):
 parts=[]
 for prop,values,param in RULES[family*3+group]:
  value=values[tier]
  if value:parts.append('+'+('-'.join(map(str,value)) if isinstance(value,tuple) else str(value))+LABELS[prop])
 return '; '.join(parts)

def install(root):
 root=Path(root);assets=[];bundles=[]
 for folder in ('data/global/excel','data/global/excel/base'):
  p=root/folder;h,stats=read(p/'itemstatcost.txt')
  for sid,old,name,prop,bits,key,text,fmt in STATS:
   r=next(x for x in stats if x['*ID']==str(sid))
   if r['Stat']==old:
    assert not r['Save Bits'] and not r['Save Param Bits']
    for f in p.glob('*.txt'):
     if f.name!='itemstatcost.txt':assert old not in f.read_text(encoding='utf-8-sig'),(old,f)
   else:assert r['Stat']==name
   r.clear();r.update({k:'' for k in h});r.update(Stat=name,**{'*ID':str(sid),'Send Bits':'16','Save Bits':str(bits) if bits else '', 'Save Add':'0' if bits else '', 'descpriority':'85' if bits else '', 'descfunc':str(fmt) if fmt else '', 'descval':'1' if fmt else '', 'descstrpos':key,'descstrneg':key,'*eol':'0'})
  write(p/'itemstatcost.txt',h,stats)
  h,props=read(p/'properties.txt')
  def prop(code,stat):
   r=next((x for x in props if x['code']==code),None)
   if r is None:r={k:'' for k in h};r.update(code=code,**{'*Id':str(len(props)),'*Enabled':'1','*eol':'0'});props.append(r)
   r.update(func1='1',stat1=stat)
  for sid,old,name,code,bits,key,text,fmt in STATS:
   if code:prop(code,name)
  prop('ws-orb-craft','ws_orb_level_cost');write(p/'properties.txt',h,props)
  ids={r['code']:int(r['*Id']) for r in props if r.get('*Id','').isdigit()}
  h,rows=read(p/'cubemain.txt');rows=[r for r in rows if not r['description'].startswith('Workshop League Orb ')]
  local=[]
  for m in range(3):
   for group,types in enumerate([['weap'],['armo'],['ring','amul']]):
    for tier in range(4):
     code=f'L{chr(110+m)}{tier+1}';payload=[]
     for prop,values,param in RULES[m*3+group]:
      value=values[tier]
      if value:
       lo,hi=value if isinstance(value,tuple) else (value,value)
       payload.append((ids[prop],param,lo,hi))
     payload.append((ids['ws-orb-level'],0,COST[tier],COST[tier]));index=len(local);local.append(payload)
     for typ in types:
      r={k:'' for k in h};r.update(description=f'Workshop League Orb {code} {typ}',enabled='1',version='100',numinputs='2',op='15',param='265',value=str(COST[tier]),output='useitem',**{'input 1':typ,'input 2':code,'*eol':'0'})
      names={v:k for k,v in ids.items()}
      assert len(payload)<=5
      for slot,(pid,param,lo,hi) in enumerate(payload,1):
       r.update({f'mod {slot}':names[pid],f'mod {slot} chance':'100',f'mod {slot} param':str(param),f'mod {slot} min':str(lo),f'mod {slot} max':str(hi)})
      rows.append(r)
  write(p/'cubemain.txt',h,rows);bundles=local
  for name in ('itemstatcost','properties','cubemain'):assets.append(folder+'/'+name+'.txt')
 # Bind classifications to the actual final skill/missile rows; zero SrcDamage denotes spells.
 _,skills=read(root/'data/global/excel/skills.txt');_,missiles=read(root/'data/global/excel/missiles.txt')
 weaponSkills=[int(r['*Id']) for r in skills if r['*Id'].isdigit() and int(r.get('SrcDam') or 0)>0]
 weaponMissiles=[int(r['*ID']) for r in missiles if r['*ID'].isdigit() and int(r.get('SrcDamage') or 0)>0]
 out=root/'workshop/leagues';out.mkdir(parents=True,exist_ok=True)
 flat=[];spans=[]
 for payload in bundles:spans.append((len(flat),len(payload)));flat+=payload
 (out/'craft_bindings.h').write_text('#pragma once\nnamespace LeagueCraft {\ninline constexpr int SpellStat=262,NovaStat=263,LevelStat=264,GateStat=265;\ninline constexpr unsigned Dispatch='+str(ids['ws-orb-craft'])+';\nstruct Effect {unsigned property;int param,lo,hi;};\ninline constexpr Effect Effects[]={'+','.join('{'+','.join(map(str,e))+'}' for e in flat)+'};\nstruct Span {unsigned start,count;};\ninline constexpr Span Spans[]={'+','.join('{'+','.join(map(str,e))+'}' for e in spans)+'};\ninline constexpr int Costs[]={2,4,6,10};\ninline constexpr int WeaponSkills[]={'+','.join(map(str,weaponSkills))+'};\ninline constexpr int WeaponMissiles[]={'+','.join(map(str,weaponMissiles))+'};\n}\n')
 p=root/'data/local/lng/strings/item-names.json';strings=json.loads(p.read_text(encoding='utf-8-sig'));last=max(r['id'] for f in p.parent.glob('*.json') for r in json.loads(f.read_text(encoding='utf-8-sig')) if 'id' in r)
 labels={key:text for _,_,_,_,bits,key,text,_ in STATS if bits}
 labels['LeagueOrbHelp']='Cube with equipment to add permanent bonuses.\nAll bonuses stack. Maximum required level: 120.'
 for m in range(3):
  for tier in range(4):
   labels[f'LeagueOrbHelp{m}{tier}']='\n'.join(f'{name}: {describe(m,g,tier)}' for g,name in enumerate(['Weapons','Armor / Shields','Rings / Amulets']))+f'\nCube with equipment. +{COST[tier]} Required Level.\nAll bonuses stack. Maximum required level: 120.'
 for key,text in labels.items():
  r=next((x for x in strings if x['Key']==key),None)
  if r is None:last+=1;r={'id':last,'Key':key};strings.append(r)
  for lang in strings[0]:
   if lang not in ('id','Key'):r[lang]=text
 p.write_text(json.dumps(strings,indent=2,ensure_ascii=False),encoding="utf-8");assets.append(str(p.relative_to(root)).replace('\\','/'))
 for folder in ('data/global/excel','data/global/excel/base'):
  p=root/folder/'misc.txt';h,rows=read(p)
  for m in range(3):
   for tier in range(4):next(r for r in rows if r['code']==f'L{chr(110+m)}{tier+1}')['spelldescstr']=f'LeagueOrbHelp{m}{tier}'
  write(p,h,rows)
 return assets
