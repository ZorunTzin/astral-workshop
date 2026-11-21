"""Five Paladin replacements. Run after aura toggles and reservation bindings.

Canonical skill names and IDs remain stable for Reimagined item/formula links.
Only their localized names and gameplay definitions change. Hidden legacy copies
preserve the old five auras when equipment grants them, without reserving mana.
"""
import json,re
from pathlib import Path
try:
    from .port import read,write
except ImportError:
    from port import read,write
HERE=Path(__file__).resolve().parent
REPLACEMENTS={100:'Reservation Efficiency',105:'Aura Mastery',110:'Astral Guardian',103:'Retribution',109:'Sanctified Ground'}
AURAS=(98,99,102,103,104,108,109,113,114,115,118,119,120,122,123,124,125)
EFFECT="stat('workshop_aura_effect'.accr)"
STRINGS={
 'WorkshopEfficiencyName':'Reservation Efficiency',
 'WorkshopEfficiencyDesc':'Passive. Reduces mana reserved by all aura buffs.',
 'WorkshopEfficiencyValue':'Reservation efficiency: +%d%%',
 'WorkshopMasteryName':'Aura Mastery',
 'WorkshopMasteryDesc':'Passive. Strengthens aura effects. Does not amplify either passive.',
 'WorkshopMasteryValue':'Increased aura effect: +%d%%',
 'WorkshopGuardianName':'Astral Guardian',
 'WorkshopGuardianDesc':'Summons guardians. Aura points improve them. Live values update after summoning.',
 'WorkshopGuardianBaseLife':'Base life (before gear): %d',
 'WorkshopGuardianCurrentLife':'Active guardian life: %d',
 'WorkshopGuardianMinDamage':'Holy Bolt damage (min): %d',
 'WorkshopGuardianMaxDamage':'Holy Bolt damage (max): %d',
 'WorkshopGuardianCount':'Maximum guardians: %d',
 'WorkshopGuardianOutput':'Damage after bonuses and scaling: %d%%',
 'WorkshopGuardianLife':'Guardian bonus life: +%d%%',
 'WorkshopGuardianDamage':'Aura-point damage bonus: +%d%%',
 'WorkshopRetributionName':'Retribution',
 'WorkshopRetributionDesc':'Toggle. Every fifth damaging hit against you releases a physical nova.',
 'WorkshopRetributionReturn':'Physical nova damage: %d',
 'WorkshopRetributionTime':'Skill level and Aura Mastery increase nova damage and radius.',
 'WorkshopGroundName':'Sanctified Ground',
 'WorkshopGroundDesc':'Toggle. Stand still for 1 second to consecrate the ground. Grants an 8-second buff to allies.',
 'WorkshopGroundDamage':'All damage: +%d%%',
 'WorkshopGroundRegen':'Maximum life regenerated/sec: %d%%',
 'WorkshopGroundRadius':'Ground radius: 8 yards. Buff refreshes while standing on the ground.',
 'WorkshopAuraReservedValue':'Mana reserved: %d',
 'WorkshopGroundDurationValue':'Buff duration: %d seconds',
 'WorkshopGroundRadiusValue':'Ground radius: %d yards',
 'WorkshopRetributionIntervalValue':'Hits needed: %d',
 'WorkshopRetributionCapacityValue':'Nova radius: %d%% of base',
 'WorkshopGuardianPointsValue':'Invested aura points: %d',
 'WorkshopGuardianZealValue':'Zeal hits: %d',
 'WorkshopGuardianCombatInfo':'Holy Bolt (6), Hammer/Zeal (18), H. Fist (30).',
 'WorkshopPercent':'%',
}

def install(root):
 root=Path(root);result=None
 for folder in ('data/global/excel','data/global/excel/base'):
  p=root/folder;sh,skills=read(p/'skills.txt');dh,descs=read(p/'skilldesc.txt')
  th,stats=read(p/'itemstatcost.txt');sth,states=read(p/'states.txt');mh,missiles=read(p/'missiles.txt')
  sid=max(int(r['*ID']) for r in stats if r['*ID'].isdigit())+1
  statids={}
  for offset,name in enumerate(('workshop_reservation_efficiency','workshop_aura_effect','workshop_ground_physical','workshop_retribution_charge','workshop_guardian_life','workshop_guardian_min_damage','workshop_guardian_max_damage')):
   row={k:'' for k in th};row.update(Stat=name,**{'*ID':str(sid+offset),'Signed':'1','Send Bits':'32','*eol':'0'})
   stats.append(row);statids[name]=sid+offset
  assert sid+6<511
  stateid=max(int(r['*ID']) for r in states if r['*ID'].isdigit())+1
  row={k:'' for k in sth};row.update(state='workshop_sanctified',**{'*ID':str(stateid),'aura':'1','overlay1':'sanctuaryfront','overlay2':'sanctuaryback','*eol':'0'})
  states.append(row)
  nextid=max(int(r['*Id']) for r in skills if r['*Id'].isdigit())+1
  legacy={}
  for skill in REPLACEMENTS:
   src=next(r for r in skills if r['*Id']==str(skill));hidden=src.copy()
   hidden.update(skill='Workshop Legacy '+src['skill'],charclass='',skilldesc='',**{'*Id':str(nextid),'srvdofunc':'65','aura':'1','periodic':'1','passive':'','passivestate':''})
   for key in hidden:
    if key.startswith(('passivestat','passivecalc','reqskill')):hidden[key]=''
   skills.append(hidden);legacy[skill]=nextid;nextid+=1
  # A native Nova carries skill-derived physical damage; its own missile record
  # references the private helper, so it cannot alter Sorceress Nova.
  nova_id=nextid
  nova=next(r for r in skills if r['skill']=='Nova').copy()
  nova.update(skill='Workshop Retribution Nova',charclass='',skilldesc='',**{'*Id':str(nova_id),'reqlevel':'1','maxlvl':'99','mana':'0','lvlmana':'0','startmana':'0','minmana':'0','manashift':'8','EType':'','MinDam':'1','MaxDam':'1','HitShift':'8','EDmgSymPerCalc':'','DmgSymPerCalc':"max(stat('workshop_retribution_charge'.accr)*100-100,0)",'castoverlay':''})
  for key in nova:
   if key.startswith(('EMin','EMax','MinLevDam','MaxLevDam','reqskill')):nova[key]=''
  missile_id=max(int(r['*ID']) for r in missiles if r['*ID'].isdigit())+1
  missile=next(r for r in missiles if r['Missile']=='nova').copy()
  missile.update(Missile='workshop_retribution_nova',Skill=nova['skill'],**{'*ID':str(missile_id),'Red':'255','Green':'190','Blue':'70','Range':'16','TravelSound':''})
  missiles.append(missile)
  for key in ('srvmissilea','srvmissileb','srvmissilec','cltmissilea'):nova[key]=missile['Missile']
  # Native srvdo 22 adds calc1 to projectile speed; use the same client formula.
  # With a fixed lifetime, speed controls the expanding ring's travel radius.
  nova['calc1']=nova['cltcalc1']='max(0,24*(100+3*(lvl-1))*(100+'+EFFECT+')/10000-24)'
  skills.append(nova)
  # Native Valkyrie creation supplies a visible, combat-capable pet and equipment.
  guardian=next(r for r in skills if r['skill']=='Valkyrie').copy()
  target=next(r for r in skills if r['*Id']=='110')
  for key in ('skill','*Id','charclass','skilldesc','reqlevel','reqskill1','reqskill2','reqskill3','maxlvl'):guardian[key]=target[key]
  points='+'.join("skill('"+next(r['skill'] for r in skills if r['*Id']==str(i))+"'.blvl)" for i in AURAS)
  guardian.update(calc1="100+20*(lvl-1)+10*(clc4+clc5+clc6)+stat('ws_pet_life'.accr)",calc3="8*(clc4+clc5+clc6)+stat('ws_pet_damage'.accr)",ToHitCalc='40*lvl',mana='25',lvlmana='1',manashift='8',localdelay='15',petmax='1+lvl/10')
  terms=points.split('+')
  for field,group in zip(('calc4','calc5','calc6'),(terms[:6],terms[6:12],terms[12:])):guardian[field]='+'.join(group)
  guardian['passivestat9']='damagepercent';guardian['passivecalc9']='clc3'
  guardian['passivecalc8']="stat('ws_pet_speed'.accr)"
  guardian['passivestat10']='attackrate';guardian['passivecalc10']="stat('ws_pet_speed'.accr)"
  for key in guardian:
   if key.startswith(('sumskill','sumsk')):guardian[key]=''
  skills[skills.index(target)]=guardian
  for skill,name in REPLACEMENTS.items():
   row=next(r for r in skills if r['*Id']==str(skill))
   desc=next(r for r in descs if r['skilldesc']==row['skilldesc'])
   for key in desc:
    if key.startswith(('descline','desctext','desccalc','dsc2','dsc3','ddam','p1','p2')) or key=='descdam':desc[key]=''
   prefix={100:'Efficiency',105:'Mastery',110:'Guardian',103:'Retribution',109:'Ground'}[skill]
   desc['str name']='Workshop'+prefix+'Name';desc['str short']=desc['str long']='Workshop'+prefix+'Desc'
   def line(n,label,calc,percent=True):
    desc[f'descline{n}']='36';desc[f'desctexta{n}']=desc[f'desctextb{n}']=label;desc[f'desccalca{n}']=calc;desc[f'desccalcb{n}']='1'
   if skill in (100,105):
    for key in row:
     if key.startswith(('aura','passivestat','passivecalc','srvmissile','cltmissile')):row[key]=''
    row.update(srvstfunc='',srvdofunc='',cltstfunc='',cltdofunc='',passive='1',rightskill='0',leftskill='0')
    row['passivestate']='passive_resistfire' if skill==100 else 'passive_resistcold'
    row['passivestat1']='workshop_reservation_efficiency' if skill==100 else 'workshop_aura_effect'
    row['passivecalc1']='4*lvl' if skill==100 else '3*lvl'
    line(1,'Workshop'+prefix+'Value',row['passivecalc1'])
   elif skill==110:
    line(1,'WorkshopGuardianBaseLife','80*(100+clc1)/100',False)
    line(2,'WorkshopGuardianMinDamage','1',False)
    line(3,'WorkshopGuardianMaxDamage','1',False)
    line(4,'WorkshopGuardianCount',"1+lvl/10+stat('ws_pet_count'.accr)",False)
    line(5,'WorkshopGuardianOutput','(100+clc3)/10')
    line(6,'WorkshopGuardianLife','clc1')
    desc['dsc2line1']='18';desc['dsc2texta1']='WorkshopGuardianCombatInfo'
    desc['dsc2line2']='36';desc['dsc2texta2']=desc['dsc2textb2']='WorkshopGuardianDamage'
    desc['dsc2calca2']='8*(clc4+clc5+clc6)';desc['dsc2calcb2']='1'
   elif skill==103:
    for key in row:
     if key.startswith(('aurastat','auraevent','EMin','EMax')):row[key]=''
    row['calc3']='(50+25*lvl)*(100+'+EFFECT+')/100'
    row['calc4']='(100+3*(lvl-1))*(100+'+EFFECT+')/100'
    line(1,'WorkshopRetributionReturn','clc3',False)
    line(2,'WorkshopRetributionIntervalValue','5',False);line(4,'WorkshopRetributionCapacityValue','clc4',False)
   elif skill==109:
    for key in row:
     if key.startswith(('aurastat','auraevent')):row[key]=''
    row['calc1']='(20+5*lvl)*(100+'+EFFECT+')/100'
    row['calc2']='(300+20*lvl)*(100+'+EFFECT+')/100'
    line(1,'WorkshopGroundDamage','clc1')
    # Show one decimal digit through a readable 1/10 percentage calculator.
    line(2,'WorkshopGroundRegen','clc2');desc['desccalcb2']='100'
    line(4,'WorkshopGroundDurationValue','8');line(5,'WorkshopGroundRadiusValue','8')
    desc['dsc2line1']='';desc['dsc2texta1']=''
   if skill in (103,109):
    base,growth={103:(30,6),109:(45,9)}[skill]
    line(3,'WorkshopAuraReservedValue',f'max(1,({base}+{growth}*lvl)*100/(100+stat(\'workshop_reservation_efficiency\'.accr)))',False)
  # Apply Aura Mastery once, at the point each native aura calculates its effects.
  # Radius, duration, reservation and both passives are deliberately excluded.
  for row in skills:
   if row['*Id'].isdigit() and int(row['*Id']) in AURAS:
    for n in range(1,7):
     field=f'aurastatcalc{n}'
     if row[field] and not re.fullmatch(r'ast[1-6]',row[field]):
      row[field]='('+row[field]+')*(100+'+EFFECT+')/100'
      if int(row['*Id'])==114:row[field]='0-min(90,min(ln34,50)*(100+'+EFFECT+')/100)'
    if int(row['*Id']) in (102,114,118):row['EDmgSymPerCalc']=EFFECT
  # Native descriptions for remaining toggles use the same efficiency denominator.
  aura_desc={r['skilldesc'] for r in skills if r['*Id'].isdigit() and int(r['*Id']) in AURAS}
  for desc in descs:
   if desc['skilldesc'] not in aura_desc:continue
   for key,value in list(desc.items()):
    if value=='WorkshopAuraReserved':
     field=key.replace('texta','calca')
     if 'reservation_efficiency' not in desc[field]:desc[field]='('+desc[field]+')*100/(100+stat(\'workshop_reservation_efficiency\'.accr))'
     prefix,suffix=key.split('texta')
     desc[prefix+'line'+suffix]='36';desc[key]=desc[key.replace('texta','textb')]='WorkshopAuraReservedValue'
     desc[field]='max(1,'+desc[field]+')';desc[field.replace('calca','calcb')]='1'
  local={'legacy':legacy,'nova_skill':nova_id,'nova_missile':missile_id,'ground_state':stateid,'stats':statids}
  if result:assert result==local
  result=local
  for table,header,rows in [('skills',sh,skills),('skilldesc',dh,descs),('itemstatcost',th,stats),('states',sth,states),('missiles',mh,missiles)]:write(p/(table+'.txt'),header,rows)
 strings=root/'data/local/lng/strings/skills.json';rows=json.loads(strings.read_text(encoding='utf-8-sig'))
 last=max(r['id'] for p in strings.parent.glob('*.json') for r in json.loads(p.read_text(encoding='utf-8-sig')) if isinstance(r,dict) and 'id' in r)
 locales=[k for k in rows[0] if k not in ('id','Key')]
 for key,value in STRINGS.items():
  assert not any(r['Key']==key for r in rows);last+=1;rows.append({'id':last,'Key':key,**dict.fromkeys(locales,value)})
 strings.write_text(json.dumps(rows,ensure_ascii=False,indent=4),encoding='utf8')
 (HERE/'FEATURES.json').write_text(json.dumps(result,indent=2),encoding='utf8')
 header='// Generated private skill/state/stat IDs.\n'
 header+='inline constexpr int EfficiencyStat='+str(result['stats']['workshop_reservation_efficiency'])+';\n'
 header+='inline constexpr int EffectStat='+str(result['stats']['workshop_aura_effect'])+';\n'
 header+='inline constexpr int GroundPhysicalStat='+str(result['stats']['workshop_ground_physical'])+';\n'
 header+='inline constexpr int ChargeStat='+str(result['stats']['workshop_retribution_charge'])+';\n'
 for key,name in (('GuardianLifeStat','workshop_guardian_life'),('GuardianMinDamageStat','workshop_guardian_min_damage'),('GuardianMaxDamageStat','workshop_guardian_max_damage')):
  header+='inline constexpr int '+key+'='+str(result['stats'][name])+';\n'
 header+='inline constexpr int GroundState='+str(result['ground_state'])+';\n'
 header+='inline constexpr int NovaSkill='+str(result['nova_skill'])+';\n'
 header+='inline constexpr int LegacySkills[][2]={'+','.join('{'+str(k)+','+str(v)+'}' for k,v in result['legacy'].items())+'};\n'
 (HERE/'aura_features_bindings.h').write_text(header,encoding='utf8')

if __name__=='__main__':
 import sys
 install(sys.argv[1])
