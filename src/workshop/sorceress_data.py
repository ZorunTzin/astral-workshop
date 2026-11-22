"""Sorceress 0.13 prototype: stable public IDs, private missiles and pet rows."""
import json
from pathlib import Path
from combat.port import read,write
HERE=Path(__file__).resolve().parent

def install(root):
 root=Path(root);labels={};bindings=None;visuals={};aura_definitions=[]
 for folder in ('data/global/excel','data/global/excel/base'):
  p=root/folder
  sh,skills=read(p/'skills.txt');dh,descs=read(p/'skilldesc.txt');mh,missiles=read(p/'missiles.txt');sth,states=read(p/'states.txt');ph,pets=read(p/'pettype.txt');nh,monsters=read(p/'monstats.txt')
  original_missiles={r['Missile'] for r in missiles}
  by={int(r['*Id']):r for r in skills if r['*Id'].isdigit()};ds={r['skilldesc']:r for r in descs};original={i:by[i].copy() for i in by}
  positions={i:{k:ds[by[i]['skilldesc']][k] for k in ('SkillPage','SkillRow','SkillColumn','ListRow')} for i in (36,39,41,43,44,45,47,51,55)}
  def clone_skill(source,name):
   row=source.copy();row.update(skill=name,charclass='',skilldesc='',**{'*Id':str(max(by)+1),'reqskill1':'','reqskill2':'','reqskill3':'','reqlevel':'1','mana':'0','lvlmana':'0','minmana':'0','startmana':'0','localdelay':'0','globaldelay':'0','ItemEffect':'1'})
   skills.append(row);by[int(row['*Id'])]=row;return row
  def replace(sid,source):
   old=by[sid];ident={k:old[k] for k in ('skill','*Id','skilldesc','maxlvl')};old.clear();old.update(source);old.update(ident,charclass='sor',reqskill1='',reqskill2='',reqskill3='');return old
  def state(name,template='enchant',**kw):
   r=next(r for r in states if r['state']==template).copy();r.update(state=name,**{'*ID':str(max(int(x['*ID']) for x in states if x['*ID'].isdigit())+1),'group':'','skill':'','stat':'','setfunc':'','itemtype':'','itemtrans':'','castoverlay':'','overlay1':''});r.update(kw);states.append(r);return r
  def missile(source,name,**kw):
   r=next(r for r in missiles if r['Missile']==source).copy();r.update(Missile=name,**{'*ID':str(max(int(x['*ID']) for x in missiles if x['*ID'].isdigit())+1)});r.update(kw);missiles.append(r);visuals[name]=source;return r
  def label(sid,name,description,lines):
   r=by[sid];d=ds[r['skilldesc']];key='WorkshopSorceress'+str(sid);labels[key+'Name']=name;labels[key+'Desc']=description
   if d.get('str name'):labels[d['str name']]=name
   d.update({'str name':key+'Name','str alt':key+'Name','str short':key+'Desc','str long':key+'Desc'})
   for k in d:
    if k.startswith(('descline','desctext','desccalc','dsc2','dsc3','descmissile','p1dm','p2dm','p3dm')):d[k]=''
   regular=0;synergy=0
   for i,(text,calc) in enumerate(lines,1):
    if 'per base level' in text:
     synergy+=1;prefix='dsc3';n=synergy
    else:
     regular+=1;prefix='desc' if regular<=6 else 'dsc2';n=regular if regular<=6 else regular-6
    keyline=key+'Line'+str(i);labels[keyline]=text;d[prefix+'line'+str(n)]='36';d[prefix+'texta'+str(n)]=keyline;d[prefix+'calca'+str(n)]=calc
   d['descdam']='';d['ddam calc1']=d['ddam calc2']=''
  def damage(r,element,minimum,maximum,growth,shift=8):
   r.update(EType=element,EMin=str(minimum),EMax=str(maximum),HitShift=str(shift),EDmgSymPerCalc='',SrcDam='0',MinDam='0',MaxDam='0',DmgSymPerCalc='')
   for b in range(1,6):r['EMinLev'+str(b)]=r['EMaxLev'+str(b)]=str(growth);r['MinLevDam'+str(b)]=r['MaxLevDam'+str(b)]='0'
  # Existing learned Fire Ball, Fire Wall, Ice Blast and Glacial Spike IDs survive.
  for sid,slot,req,prereq in ((47,36,1,''),(51,41,6,''),(45,39,1,''),(55,45,6,'Ice Blast')):
   ds[by[sid]['skilldesc']].update(positions[slot]);by[sid].update(reqlevel=str(req),reqskill1=prereq,reqskill2='',reqskill3='')
  # Fire Bolt's obsolete public slot becomes Avatar; Inferno becomes Conflagration.
  avatar_state=state('workshop_avatar_fire','wolf',group='3',setfunc='14',gfxclass='292',gfxtype='1',restrict='1',meleeonly='1')
  avatar=replace(36,original[223]);avatar.update(reqlevel='12',reqskill1='Fire Ball',aurastate=avatar_state['state'],auralencalc='3000',mana='20',lvlmana='0',globaldelay='0',localdelay='25',cltoverlaya='',cltoverlayb='')
  ds[avatar['skilldesc']].update(positions[47])
  stats=[('attackrate','10+lvl'),('item_tohit_percent','100+10*lvl'),('item_maxhp_percent','25+3*lvl'),('velocitypercent','10+lvl'),('firemindam','(25+5*lvl)'),('firemaxdam','(50+10*lvl)')]
  for i,(stat,calc) in enumerate(stats,1):avatar['aurastat'+str(i)]=stat;avatar['aurastatcalc'+str(i)]=calc
  # Additional animation speeds are supplied to this native transform list at runtime.
  label(36,'Avatar of Fire','Fire elemental form. Melee attacks only; buffs and passives remain usable. Cast again to revert.', [('Maximum life: +%d%%','25+3*lvl'),('All speeds: +%d%%','10+lvl'),('Attack rating: +%d%%','100+10*lvl'),('Added fire damage: %d minimum','25+5*lvl'),('Added fire damage: %d maximum','50+10*lvl'),('Duration: %d seconds','120'),('Mana cost: %d','20')])
  # Native nova plus a bounded runtime zone; no wall emitter per victim.
  con=replace(41,original[48]);con.update(reqlevel='18',reqskill1='Fire Wall',mana='20',lvlmana='1',localdelay='250',calc1='0',stsound='sorceress_cast_fire')
  ds[con['skilldesc']].update(positions[51]);damage(con,'fire',750,1200,30)
  wave=missile('nova','workshop_conflagration_nova',pCltDoFunc='16',CltSubMissile1='workshop_conflagration_ground',Skill=con['skill'],Range='24',Vel='32',MaxVel='32',Accel='0',Red='255',Green='80',Blue='15',ProgOverlay='',ClientSend='1',TravelSound='sorceress_cast_fire',pCltHitFunc='')
  visuals[wave['Missile']]='frostnova' # Remap below to native fire nova artwork.
  for k in ('srvmissilea','srvmissileb','srvmissilec','cltmissilea'):con[k]=wave['Missile']
  dot=clone_skill(con,'Workshop Conflagration Burn');dot.update(srvdofunc='0',cltdofunc='0',srvmissilea='',srvmissileb='',srvmissilec='',cltmissilea='');damage(dot,'fire',750,1200,30)
  tick=missile('firebolt','workshop_conflagration_tick',Skill=dot['skill'],pSrvDoFunc='1',pSrvHitFunc='',pCltDoFunc='1',pCltHitFunc='',Range='1',Vel='0',MaxVel='0',CollideType='3',ClientCol='0',NextHit='',NextDelay='',LastCollide='0',HitSubMissile1='',CltHitSubMissile1='',AlwaysExplode='0',SrcDamage='0')
  ground=missile('diabwall','workshop_conflagration_ground',Skill='',pSrvDoFunc='1',pSrvHitFunc='',Range='75',CollideType='0',ClientCol='0',EType='',EMin='0',EMax='0',MinDamage='0',MaxDamage='0')
  for k in ground:
   if k.startswith(('MinLevDam','MaxLevDam','MinELev','MaxELev')):ground[k]='0'
  con['EDmgSymPerCalc']=dot['EDmgSymPerCalc']="(skill('Fire Wall'.blvl)+skill('Blaze'.blvl))*5"
  label(41,'Conflagration','A fire nova ignites nearby ground for 3 seconds. Fire mastery increases both effects.', [('Nova / burn minimum per second: %d','enma'),('Nova / burn maximum per second: %d','exma'),('Radius: %d yards','16'),('Burn duration: %d seconds','3'),('Cooldown: %d seconds','10'),('Mana cost: %d','19+lvl'),('Fire Wall: +%d%% damage per base level','5'),('Blaze: +%d%% damage per base level','5')])
  ds[con['skilldesc']]['descdam']='5'  # Native elemental nova range in character sheet.
  # Warmth keeps existing regeneration and kill recovery.
  warm=by[37]
  for i,(stat,calc) in enumerate((('maxhp','(50+10*lvl)*256'),('lifedrainmindam','3+lvl/2'),('manadrainmindam','3+lvl/2')),3):warm['passivestat'+str(i)]=stat;warm['passivecalc'+str(i)]=calc
  label(37,'Warmth','Improves mana recovery and life. Life/mana steal applies to physical attacks, with native whole-percent rounding.', [('Mana recovery: +%d%%','ln12'),('Life: +%d','50+10*lvl'),('Life and mana stolen per hit: %d%%','3+lvl/2'),('Life after kill: +%d','blvl')])
  # Private Diablo/Torch-style winding flame streams; their children own damage.
  procids=[];makerids=[]
  for element,sid in (('fire',36),('cold',39),('ltng',43)):
   proc=clone_skill(original[197],'Workshop '+element+' Storm');proc.update(anim='SC',seqtrans='SC',monanim='A1',calc1='3',restrict='1');damage(proc,element,24,48,6)
   for tier in range(1,6):proc['EMaxLev'+str(tier)]='12'
   child=missile('diabwall','workshop_'+element+'_storm_fire',Skill=proc['skill'],HitShift='8',MinDamage='0',MaxDamage='0',EType=element,Range='25',NextHit='1',NextDelay='5')
   for k in child:
    if k.startswith(('MinLevDam','MaxLevDam','MinELev','MaxELev')):child[k]='0'
   child['EMin']=child['EMax']='0'
   maker=missile('diabwallmaker','workshop_'+element+'_storm_maker',Range='48',Skill=proc['skill'],SubMissile1=child['Missile'],CltSubMissile1=child['Missile'])
   proc['srvmissilea']=proc['cltmissilea']=maker['Missile'];procids.append(int(proc['*Id']));makerids.append(int(maker['*ID']))
   if element=='cold':proc.update(ELen='75',ELevLen1='0',ELevLen2='0',ELevLen3='0');child.update(Red='30',Green='130',Blue='255',ProgOverlay='ice_explode');visuals[child['Missile']]='frostnova'
   if element=='ltng':child.update(Red='255',Green='240',Blue='60',ProgOverlay='lightning');visuals[child['Missile']]='shock_field_on_ground'
   for k in ('auraevent1','auraeventfunc1'):by[sid][k]='domeleeattack' if k=='auraevent1' else '33'
  # Enchant is again the ordinary ally enchantment; Firestorm belongs to Avatar.
  enchant=by[52]
  enchant.update(auraevent1='domeleeattack',auraeventfunc1='33')
  ed=ds[enchant['skilldesc']]
  for n,(key,text) in enumerate((('WorkshopEnchantLifeAttack','Life on attack: +%d'),('WorkshopEnchantManaAttack','Mana on attack: +%d')),1):
   labels[key]=text;ed['dsc2line'+str(n)]='36';ed['dsc2texta'+str(n)]=key;ed['dsc2calca'+str(n)]='5+lvl'
  queen_state=state('workshop_frozen_queen',overlay1='frozenarmor',armblue='1')
  master_state=state('workshop_masterful_form',overlay1='energyshield')
  for sid,st,slot,name,desc,stats in [
   (39,queen_state,55,'Frozen Queen','Cold battle form. 25% chance on attack for a cold storm; drains life. Recast to switch off.', [('skill_armor_percent','30+10*lvl'),('coldmindam','(25+5*lvl)'),('coldmaxdam','(50+10*lvl)'),('coldlength','50')]),
   (43,master_state,43,'Masterful Form','Lightning battle form. 25% chance on attack for a lightning storm; drains mana. Recast to switch off.', [('item_fastercastrate','10+2*lvl'),('lightmindam','(3+lvl)'),('lightmaxdam','(60+10*lvl)')])]:
   row=replace(sid,original[267]);row.update(srvdofunc='65',aurastate=st['state'],auralencalc='3000',reqlevel='18' if sid==39 else '6',mana='10',lvlmana='0',auraevent1='domeleeattack',auraeventfunc1='33',passivestate='',passivestat1='',passivecalc1='',stsound='sorceress_cast_cold' if sid==39 else 'sorceress_cast_lightning')
   for i in range(1,7):row['aurastat'+str(i)]=row['aurastatcalc'+str(i)]=''
   for i,(stat,calc) in enumerate(stats,1):row['aurastat'+str(i)]=stat;row['aurastatcalc'+str(i)]=calc
   ds[row['skilldesc']].update(positions[slot]);label(sid,name,desc,[(('Defense: +%d%%' if sid==39 else 'Cast speed: +%d%%'),('30+10*lvl' if sid==39 else '10+2*lvl')),('Minimum added damage: %d','25+5*lvl' if sid==39 else '3+lvl'),('Maximum added damage: %d','50+10*lvl' if sid==39 else '60+10*lvl'),('Storm chance on attack: %d%%','25'),('Life drained per second: %d%%' if sid==39 else 'Mana drained per second: %d','2' if sid==39 else '2+lvl'),('Duration: %d seconds','120'),('Mana cost: %d','10')])
  # Native restriction masks reject offensive spells during the fire form.
  for row in skills:
   if row['charclass']=='sor' and not row['passivestate']:
    row['restrict']='1' if int(row['*Id']) in (36,39,40,43,46,50,52,57,58,60) else '0'
  # Frost Guardians use ground-bound NecroPet AI and their own cap.
  pet=next(r for r in pets if r['pet type']=='grizzly').copy();pet.update({'pet type':'workshop_frost_guardian','name':'WorkshopFrostGuardian','baseicon':'earthgolumicon'});pets.append(pet)
  mon=next(r for r in monsters if r['Id']=='claygolem').copy();cid=max(int(r['*hcIdx']) for r in monsters if r['*hcIdx'].isdigit())+1
  mon.update(Id='workshop_frost_guardian',BaseId='workshop_frost_guardian',MonStatsEx='frozenhorror1',Code='f0',NameStr='WorkshopFrostGuardian',flying='0',neverCount='1',MonProp='',**{'*hcIdx':str(cid),'Velocity':'10','Run':'10'})
  for k in mon:
   if k.startswith(('Skill','Sk','TreasureClass')):mon[k]=''
  for suffix in ('','(N)','(H)'):
   mon['ResCo'+suffix]='75';mon[('minHP' if not suffix else 'MinHP'+suffix)]='500';mon[('maxHP' if not suffix else 'MaxHP'+suffix)]='500'
  monsters.append(mon)
  nova=clone_skill(original[44],'Workshop Frost Guardian Nova');damage(nova,'cold',16,32,8);nova.update(ELen='75',ELevLen1='0',ELevLen2='0',ELevLen3='0',restrict='1')
  guardian_synergy="(skill('Ice Blast'.blvl)+skill('Frozen Orb'.blvl))*8"
  nova['EDmgSymPerCalc']=guardian_synergy
  for tier in range(1,6):nova['EMaxLev'+str(tier)]='16'
  nm=missile('frostnova','workshop_frost_guardian_nova',Skill=nova['skill'],Range='16')
  for k in ('srvmissilea','srvmissileb','srvmissilec','cltmissilea'):nova[k]=nm['Missile']
  guardian=replace(44,original[247]);guardian.update(reqlevel='6',summon=mon['Id'],pettype=pet['pet type'],petmax="1+lvl/10+stat('ws_pet_count'.accr)",summode='NU',calc1='0',calc2='ulvl',mana='20',lvlmana='1',restrict='0')
  for k in guardian:
   if k.startswith(('aurastat','auraevent','passivestat','passivecalc','sumskill','sumsk')):guardian[k]=''
  # Copy only invested synergy levels to the pet for its native nova calculation.
  guardian.update(sumskill1=nova['skill'],sumsk1calc='lvl',sumskill2='Ice Blast',sumsk2calc="skill('Ice Blast'.blvl)",sumskill3='Frozen Orb',sumsk3calc="skill('Frozen Orb'.blvl)")
  melee_min='(4+4*lvl)*(100+'+guardian_synergy+')/100'
  melee_max='(8+8*lvl)*(100+'+guardian_synergy+')/100'
  for i,(stat,calc) in enumerate([('maxhp',"(200+100*lvl)*(100+stat('ws_pet_life'.accr))/100*256"),('mindamage',melee_min),('maxdamage',melee_max),('damagepercent',"stat('ws_pet_damage'.accr)"),('tohit','150+30*lvl'),('velocitypercent',"stat('ws_pet_speed'.accr)"),('attackrate',"stat('ws_pet_speed'.accr)"),('passive_cold_mastery',"stat('passive_cold_mastery'.accr)")],1):guardian['passivestat'+str(i)]=stat;guardian['passivecalc'+str(i)]=calc
  label(44,'Frost Guardians','Successful melee hits release Frost Nova. Maximum one nova per guardian per second.', [('Maximum guardians: %d',guardian['petmax']),('Life bonus: %d',"(200+100*lvl)*(100+stat('ws_pet_life'.accr))/100"),('Melee minimum: %d',melee_min),('Melee maximum: %d',melee_max),('Nova minimum damage: %d','enma'),('Nova maximum damage: %d','exma'),('Mana cost: %d','19+lvl'),('Ice Blast: +%d%% damage per base level','8'),('Frozen Orb: +%d%% damage per base level','8')])
  labels['WorkshopFrostGuardian']='Frost Guardian'
  # Helper formulas query levels without guessing native learned-skill offsets.
  helper=clone_skill(original[0],'Workshop Sorceress Levels');helper.update(calc1="skill('Fire Bolt'.lvl)",calc2="skill('Frost Nova'.lvl)",calc3="skill('Enchant'.lvl)",calc4="skill('Warmth'.lvl)")
  # Enemy scripts and original missile damage links must not turn into the
  # new player forms/summons (Uber Izual explicitly casts Frost Nova).
  legacy={}
  for sid in (36,39,41,43,44):
   old=original[sid];copy=clone_skill(old,'Workshop Sorceress Legacy '+old['skill']);legacy[old['skill']]=copy['skill']
  for monster in monsters:
   for k,value in list(monster.items()):
    if k.startswith('Skill') and value in legacy:monster[k]=legacy[value]
  for projectile in missiles:
   if projectile['Missile'] in original_missiles and projectile['Skill'] in legacy:projectile['Skill']=legacy[projectile['Skill']]
  # Use the same intercepted native buff cast as Paladin reservations. The old
  # Fade dofunc (65) never entered our toggle hook (18), so its state vanished.
  form_names={36:'Avatar of Fire',39:'Frozen Queen',43:'Masterful Form'}
  # Hard points only; share damage bonuses without multiplying reservation or utility.
  form_synergy={sid:'('+ '+'.join("skill('"+by[other]['skill']+"'.blvl)" for other in form_names if other!=sid)+')*5' for sid in form_names}
  local_auras=[]
  for sid,st,base,growth in ((36,avatar_state,60,12),(39,queen_state,60,12),(43,master_state,60,12)):
   row=by[sid]
   for field in ('aura','periodic','periodicClearAura','repeat','srvstfunc','cltstfunc','cltdofunc','TargetableOnly','TargetCorpse','TargetPet','TgtPlaceCheck','SearchEnemyXY','SearchOpenXY','passivestate'):
    row[field]=''
   row.update(srvdofunc='18',anim='SC',monanim='SC',immediate='',range='none',startmana='0',mana='0',lvlmana='0',minmana='0',InTown='1',auralencalc='50',localdelay='0',globaldelay='0',auraevent1='domeleeattack',auraeventfunc1='33',auraevent2='domissileattack',auraeventfunc2='33')
   st.update(skill=row['skill'],aura='1',curse='',setfunc='')
   reserve=f"max(1,({base}+{growth}*lvl)*75/(100+stat('workshop_reservation_efficiency'.accr)))"
   if sid==36:
    name='Avatar of Fire';description='Reserve mana to become a Fire Golem. Melee attacks and buffs only. 25% Firestorm on attack.'
    lines=[('Maximum life: +%d%%','25+3*lvl'),('All speeds: +%d%%','10+lvl'),('Attack rating: +%d%%','100+10*lvl'),('Added fire minimum: %d','25+5*lvl'),('Added fire maximum: %d','50+10*lvl')]
   elif sid==39:
    name='Frozen Queen';description='Reserve mana for cold damage and defense. 25% cold storm on attack.'
    lines=[('Defense: +%d%%','30+10*lvl'),('Added cold minimum: %d','25+5*lvl'),('Added cold maximum: %d','50+10*lvl')]
   else:
    name='Masterful Form';description='Reserve mana for lightning damage and cast speed. 25% lightning storm on attack.'
    lines=[('Cast speed: +%d%%','10+2*lvl'),('Added lightning minimum: %d','3+lvl'),('Added lightning maximum: %d','60+10*lvl')]
   label(sid,name,description+' Cast again to disable.',lines+[('Mana reserved: %d',reserve)]+[(form_names[other]+': +%d%% damage per base level','5') for other in form_names if other!=sid])
   helper_aura=clone_skill(original[0],'Workshop Sorceress Reservation '+str(sid))
   helper_aura['calc4']="skill('"+row['skill']+"'.lvl)"
   local_auras.append((sid,65,int(st['*ID']),int(helper_aura['*Id']),base,growth))
  if aura_definitions:assert aura_definitions==local_auras
  aura_definitions=local_auras
  # Summoning Frost Guardians is permitted in the elemental transformation.
  by[44]['restrict']='1'
  # Character-sheet values explicitly show the form's added attack damage.
  for sid,element,lo,hi in ((36,'fire','25+5*lvl','50+10*lvl'),(39,'cold','25+5*lvl','50+10*lvl'),(43,'ltng','3+lvl','60+10*lvl'),(44,'cold','8+4*(lvl-1)','16+4*(lvl-1)')):
   desc=ds[by[sid]['skilldesc']];key='WorkshopSorceressSheet'+str(sid)
   labels[key]=('Avatar of Fire (+fire)' if sid==36 else 'Frozen Queen (+cold)' if sid==39 else 'Masterful Form (+lightning)' if sid==43 else 'Frost Guardians (nova)')
   # Native elemental range uses these skill formulas; runtime stats match them.
   damage(by[sid],element,10 if sid in (36,39) else 1 if sid==43 else 16,20 if sid in (36,39) else 23 if sid==43 else 32,5 if sid in (36,39) else 1 if sid==43 else 8)
   for tier in range(1,6):by[sid]['EMaxLev'+str(tier)]=str(10 if sid in (36,39,43) else 16)
   desc.update({'str alt':key,'descdam':'5'})
   if sid==44:by[sid]['EDmgSymPerCalc']=guardian_synergy
   if sid in (36,39,43):
    by[sid].update(calc1='enma',calc2='exma',EDmgSymPerCalc=form_synergy[sid])
    by[procids[(36,39,43).index(sid)]]['EDmgSymPerCalc']=form_synergy[sid]
    for i in range(1,7):
     stat=by[sid]['aurastat'+str(i)]
     if stat in ('firemindam','coldmindam','lightmindam'):by[sid]['aurastatcalc'+str(i)]='enma'
     if stat in ('firemaxdam','coldmaxdam','lightmaxdam'):by[sid]['aurastatcalc'+str(i)]='exma'
    for i in range(1,7):
     caption=labels.get(desc['desctexta'+str(i)],'')
     if 'minimum:' in caption:desc['desccalca'+str(i)]='enma'
     if 'maximum:' in caption:desc['desccalca'+str(i)]='exma'
  # Lightning's public ID becomes a melee attack; keep enemy/projectile spells private.
  old_lightning=clone_skill(original[49],'Workshop Sorceress Legacy Lightning')
  for monster in monsters:
   for k,v in list(monster.items()):
    if k.startswith('Skill') and v=='Lightning':monster[k]=old_lightning['skill']
  for projectile in missiles:
   if projectile['Missile'] in original_missiles and projectile['Skill']=='Lightning':projectile['Skill']=old_lightning['skill']
  eruption=replace(49,original[126])
  eruption.update(reqlevel='12',reqskill1='',calc1='-90',calc2='0',calc3='0',calc4='0',EType='',SrcDam='128',MinDam='0',MaxDam='0',DmgSymPerCalc='',EDmgSymPerCalc='',ResultFlags='0',HitFlags='0',ToHitCalc='100+10*lvl',ToHit='0',LevToHit='0',mana='9',lvlmana='1',minmana='1',startmana='0',localdelay='0',globaldelay='0',restrict='1',leftskill='1',rightskill='1',InTown='0')
  for k in eruption:
   if k.startswith(('EMin','EMax','ELen','MinLevDam','MaxLevDam')):eruption[k]='0'
  label(49,'Coming Storm','Melee hits empower one tornado: 1/2/3 yards and x1/x2/x3 damage. Resets after 3 seconds without a hit. One pulse per second.', [('Attack rating: +%d%%','100+10*lvl'),('Weapon damage per hit / pulse: %d%%','10'),('Each element minimum per pulse: %d','5+5*(lvl-1)'),('Each element maximum per pulse: %d','10+5*(lvl-1)'),('Tornado duration: %d seconds','3'),('Maximum combo radius: %d yards','3'),('Mana cost: %d','8+lvl')])
  ds[eruption['skilldesc']].update({'descdam':'7','ddam calc1':'-90','ddam calc2':'0','descatt':'1'})
  helper['calc3']="skill('Lightning'.lvl)"
  vortex=missile('tornado','workshop_elemental_eruption',Skill='',pSrvDoFunc='1',pSrvHitFunc='',pSrvDmgFunc='',pCltDoFunc='1',pCltHitFunc='',Vel='0',MaxVel='0',Accel='0',Range='75',CollideType='0',ClientCol='0',ClientSend='1',SrcDamage='0',EType='',MinDamage='0',MaxDamage='0',EMin='0',EMax='0',NextHit='',NextDelay='')
  vortex_spell=clone_skill(original[245],'Workshop Eruption Tornado Visual')
  vortex_spell.update(srvmissilea=vortex['Missile'],cltmissilea=vortex['Missile'],restrict='1',SrcDam='0',MinDam='0',MaxDam='0',DmgSymPerCalc='',EType='',ItemEffect='1')
  for k in vortex_spell:
   if k.startswith(('MinLevDam','MaxLevDam','EMin','EMax')):vortex_spell[k]='0'
  eruption_hit=missile('firebolt','workshop_eruption_hit',Skill=eruption['skill'],SrcDamage='128',EType='',NextHit='',NextDelay='',pSrvHitFunc='',pSrvDmgFunc='',pCltHitFunc='',Range='1',Vel='1',MaxVel='1',CelFile='null',ResultFlags='5',HitFlags='2',ToHit='0',ClientSend='0')
  local={'AvatarState':int(avatar_state['*ID']),'QueenState':int(queen_state['*ID']),'MasterState':int(master_state['*ID']),'FrostClass':cid,'FrostNova':int(nova['*Id']),'LevelHelper':int(helper['*Id']),'ConflagrationMissile':int(wave['*ID']),'BurnSkill':int(dot['*Id']),'BurnMissile':int(tick['*ID']),'GroundMissile':int(ground['*ID']),'FireProc':procids[0],'ColdProc':procids[1],'LightningProc':procids[2],'FireMaker':makerids[0],'ColdMaker':makerids[1],'LightningMaker':makerids[2]}
  local.update(EruptionVisual=int(vortex['*ID']),EruptionVisualSkill=int(vortex_spell['*Id']),EruptionHit=int(eruption_hit['*ID']))
  if bindings:assert bindings==local
  bindings=local
  ch,chars=read(p/'charstats.txt')
  next(r for r in chars if r['class']=='Sorceress')['StartSkill']='Fire Ball'
  write(p/'charstats.txt',ch,chars)
  for name,h,rows in [('skills',sh,skills),('skilldesc',dh,descs),('missiles',mh,missiles),('states',sth,states),('monstats',nh,monsters),('pettype',ph,pets)]:write(p/(name+'.txt'),h,rows)
 strings=root/'data/local/lng/strings/skills.json';rows=json.loads(strings.read_text(encoding='utf-8-sig'));known={r['Key']:r for r in rows};last=max(r['id'] for f in strings.parent.glob('*.json') for r in json.loads(f.read_text(encoding='utf-8-sig')) if 'id' in r)
 for key,text in labels.items():
  if key in known:
   for lang in known[key]:
    if lang not in ('id','Key'):known[key][lang]=text
  else:last+=1;rows.append({'id':last,'Key':key,**{lang:text for lang in rows[0] if lang not in ('id','Key')}})
 strings.write_text(json.dumps(rows,ensure_ascii=False,indent=2),encoding='utf8')
 registry=root/'data/hd/missiles/missiles.json';mapping=json.loads(registry.read_text(encoding='utf-8-sig'))
 for name,source in visuals.items():mapping[name]=mapping.get(source,mapping['nova'])
 mapping['workshop_conflagration_tick']='null'
 mapping['workshop_eruption_hit']='null'
 mapping['workshop_conflagration_ground']=mapping['firewall']
 mapping['workshop_conflagration_nova']=mapping['fireball'] # Moving native fire projectiles form the expanding ring.
 for name in ('workshop_cold_storm_maker','workshop_ltng_storm_maker'):mapping[name]='null'
 registry.write_text(json.dumps(mapping,indent=4),encoding='utf8')
 registry=root/'data/hd/character/monsters.json';mapping=json.loads(registry.read_text(encoding='utf-8-sig'));mapping['workshop_frost_guardian']=mapping['frozenhorror1'];registry.write_text(json.dumps(mapping,indent=4),encoding='utf8')
 aura_file=HERE/'combat/aura_bindings.h'
 aura_text=aura_file.read_text(encoding='utf8')
 aura_text=aura_text.replace('\n};',',\n'+',\n'.join('{'+','.join(map(str,d))+'}' for d in aura_definitions)+'\n};',1)
 aura_file.write_text(aura_text,encoding='utf8')
 (HERE/'combat/sorceress_bindings.h').write_text('// Generated private IDs.\n'+''.join(f'inline constexpr int {k}={v};\n' for k,v in bindings.items()),encoding='utf8')
 (HERE/'SORCERESS.json').write_text(json.dumps(bindings,indent=2),encoding='utf8')
 print('Sorceress rework: stable IDs, three elemental forms, Conflagration and Frost Guardians.')
