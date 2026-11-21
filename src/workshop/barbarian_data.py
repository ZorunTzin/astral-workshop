"""Barbarian prototype. Preserve public IDs; change presentation, not save IDs.

Bleeding/Impale state lives in the combat plugin, not additional save stats:
D2R's nine-bit item-stat namespace is already full in this Workshop.
"""
import json
from pathlib import Path
from combat.port import read,write
HERE=Path(__file__).resolve().parent
PASSIVES=(127,128,129,135,145,148,153)

def install(root):
 root=Path(root);bindings=None;labels={}
 for folder in ('data/global/excel','data/global/excel/base'):
  p=root/folder;eh,equipment=read(p/'monequip.txt');xh,extras=read(p/'monstats2.txt');extra_raw=(p/'monstats2.txt').read_bytes();sh,skills=read(p/'skills.txt');dh,descs=read(p/'skilldesc.txt');mh,missiles=read(p/'missiles.txt');sth,states=read(p/'states.txt');ph,types=read(p/'pettype.txt');monh,monsters=read(p/'monstats.txt')
  by={int(r['*Id']):r for r in skills if r['*Id'].isdigit()};ds={r['skilldesc']:r for r in descs}
  def newskill(source,name):
   r=by[source].copy();i=max(int(x['*Id']) for x in skills if x['*Id'].isdigit())+1;r.update(skill=name,charclass='',skilldesc='',**{'*Id':str(i),'reqlevel':'1','reqskill1':'','reqskill2':'','reqskill3':''});skills.append(r);by[i]=r;return r
  def mana_formula(row):
   return f"max({int(row['minmana'] or 0)},({int(row['mana'] or 0)}+{int(row['lvlmana'] or 0)}*(lvl-1))*{1<<int(row['manashift'] or 8)}/256)"
  def label(row,name,description,lines):
   d=ds[row['skilldesc']];key='WorkshopBarbarian'+row['*Id'];labels[key+'Name']=name;labels[key+'Desc']=description
   d['str name']=key+'Name';d['str short']=d['str long']=key+'Desc'
   for k in d:
    if k.startswith(('descline','desctext','desccalc','dsc2','dsc3')):d[k]=''
   for n,(caption,formula,kind) in enumerate(lines,1):
    text=key+'Value'+str(n);labels[text]=caption;d['descline'+str(n)]=str(kind);d['desctexta'+str(n)]=text;d['desccalca'+str(n)]=formula
  original_throw=by[140].copy()
  ancient_position={k:ds[by[152]['skilldesc']][k] for k in ('SkillPage','SkillRow','SkillColumn','ListRow')}
  rocks_position={k:ds[by[147]['skilldesc']][k] for k in ('SkillPage','SkillRow','SkillColumn','ListRow')}
  legacy_swing_icon=ds[by[133]['skilldesc']]['IconCel']
  # Passive replacements retain their original names internally for old items.
  vigor=by[148]
  for n,stat in enumerate(('velocitypercent','attackrate','item_fastercastrate','item_fastergethitrate','item_fasterblockrate'),1):vigor['passivestat'+str(n)]=stat;vigor['passivecalc'+str(n)]='10+2*lvl'
  label(vigor,'Vigor of the Ancients','Increases movement, attack, casting, recovery and blocking speeds.', [('All speeds: +%d%%','10+2*lvl',36)])
  by[145]['passivestat2']='damageresist';by[145]['passivecalc2']='lvl'
  d=ds[by[145]['skilldesc']];d['descline3']='36';d['desctexta3']='WorkshopBarbarianDR';d['desccalca3']='lvl';labels['WorkshopBarbarianDR']='Physical damage reduction: %d%%'
  for n in range(1,5):by[153]['passivecalc'+str(n)]='dm12+lvl/5'
  for n,stat in enumerate(('maxfireresist','maxlightresist','maxcoldresist','maxpoisonresist'),5):by[153]['passivestat'+str(n)]=stat;by[153]['passivecalc'+str(n)]='1+(lvl-1)/5'
  label(by[153],'Natural Resistance','Resists elemental and poison damage. Maximum resistances increase every five levels.', [('All resistances: +%d%%','dm12+lvl/5',36),('All maximum resistances: +%d%%','1+(lvl-1)/5',36)])
  carnage=by[128];carnage['passiveitype']=''
  for n in range(1,15):carnage['passivestat'+str(n)]='';carnage['passivecalc'+str(n)]=''
  carnage['passivestat1']='item_tohit_percent';carnage['passivecalc1']='0'
  label(carnage,'Rage','More damage against wounded enemies. Kills grant attack speed for 10 seconds.', [('More damage per 1% missing enemy life: %d%%','20+2*lvl',36),('Attack speed on kill: +%d%%','20+lvl',36),('Melee splash radius: +%d%%','5*lvl',36)])
  ds[carnage['skilldesc']]['desccalcb1']='10'  # Decimal percentage, matching runtime permille precision.
  for sid,name,effect in ((127,'Melee Mastery','Bleeding'),(135,'Throwing Mastery','Impale')):
   d=ds[by[sid]['skilldesc']];prefix,slot=('dsc2',1) if sid==135 else ('desc',4);d[prefix+'line'+str(slot)]='36';d[prefix+'texta'+str(slot)]='WorkshopBarbarian'+effect;d[prefix+'calca'+str(slot)]='min(100,10+2*lvl)';labels['WorkshopBarbarian'+effect]=effect+' chance: %d%%'
  for sid,explanation in ((127,'Melee attacks can cause physical Bleeding: 70% of the hit per second for 5 seconds; triple while the target moves.'),(135,'Thrown attacks can Impale: stores 10% physical hit damage for the next 5 hits, lasting up to 8 seconds.')):
   key='WorkshopBarbarianMasteryDesc'+str(sid);labels[key]=explanation;ds[by[sid]['skilldesc']]['str long']=key
  labels['WorkshopBarbarianBattlePull']='Affected enemies rush toward you. AI-immune foes resist this.'
  d=ds[by[146]['skilldesc']];d['dsc2line1']='18';d['dsc2texta1']='WorkshopBarbarianBattlePull'
  d['dsc2line2']='36';d['dsc2texta2']='WorkshopBarbarianBattleMana';d['dsc2calca2']=mana_formula(by[146]);labels['WorkshopBarbarianBattleMana']='Mana cost: %d'
  # Move live skills by their skilldesc coordinates; old saved IDs stay valid.
  for sid,old in ((147,133),(152,140)):
   target,source=ds[by[sid]['skilldesc']],ds[by[old]['skilldesc']]
   for k in ('SkillPage','SkillRow','SkillColumn','ListRow'):target[k]=source[k]
  cap=int(by[133]['maxlvl'] or 20)
  by[147].update(reqlevel='1',reqskill1='',reqskill2='',reqskill3='',calc1=f'ln12+{cap}*par8',calc4=str(cap),DmgSymPerCalc=f'{cap}*par8')
  by[152].update(reqlevel='12',reqskill1='Frenzy',reqskill2='',reqskill3='')
  for k in ds[by[147]['skilldesc']]:
   if k.startswith('dsc3'):ds[by[147]['skilldesc']][k]=''
  for sid in (133,140):
   by[sid]['charclass']=''
   by[sid].update(leftskill='0',rightskill='0',srvstfunc='',srvdofunc='0',cltstfunc='',cltdofunc='0',ItemEffect='0',ItemCltEffect='0')
   for k in ('SkillPage','SkillRow','SkillColumn','ListRow'):ds[by[sid]['skilldesc']][k]='0'
  for r in skills:
   for k in ('reqskill1','reqskill2','reqskill3'):
    if r[k] in ('Double Swing','Double Throw'):r[k]='Frenzy'
  # Quake: a narrow physical, weapon-based shock-wave fan using native Teeth.
  old=by[137].copy();quake=by[67].copy()
  quake.update(skill=old['skill'],charclass='bar',skilldesc=old['skilldesc'],**{'*Id':'137','reqlevel':old['reqlevel'],'reqskill1':'Howl','reqskill2':'','reqskill3':'','EType':'','HitShift':'8','SrcDam':'128','MinDam':'0','MaxDam':'0','DmgSymPerCalc':'','EDmgSymPerCalc':'','calc1':'min(64,4+lvl)','calc2':'0','calc3':'0','restrict':'0','mana':'9','lvlmana':'1','manashift':'7','minmana':'0','ToHit':'110','LevToHit':'10','ToHitCalc':'100+10*lvl','InTown':'0'})
  for b in ('Min','Max'):
   for n in range(1,6):quake[b+'LevDam'+str(n)]='0'
  for key in quake:
   if key.startswith(('EMin','EMax','ELen')):quake[key]=''
  qi=max(int(r['*ID']) for r in missiles if r['*ID'].isdigit())+1
  missile=next(r for r in missiles if r['Missile']=='teeth').copy();missile.update(Missile='workshop_quake',Skill=quake['skill'],**{'*ID':str(qi),'CelFile':next(r['CelFile'] for r in missiles if r['Missile']=='shockwave'),'SrcDamage':'128','Vel':'20','MaxVel':'20','Range':'18','ResultFlags':'5','NextDelay':'8','CollideKill':'1','LastCollide':'1','EType':'','Pierce':'1','ToHit':'1','HitShift':'8','ExplosionMissile':''});missiles.append(missile)
  # Native multi-shot selects the B variant for non-bow weapons. Bind both.
  for key in ('srvmissilea','srvmissileb','cltmissilea','cltmissileb'):quake[key]=missile['Missile']
  quake['cltstfunc']='';quake['monanim']='SC'
  missile['AnimLen']='7';missile['ClientSend']='1'
  quake['cltmissilec']='';skills[skills.index(by[137])]=quake;by[137]=quake
  label(quake,'Quake','Physical shock waves carrying weapon damage and hit effects. Stop on impact unless Pierce succeeds.', [('Weapon damage: %d%%','(50+5*(lvl-1))*(100+clc5*par8)/300',36),('Minimum added damage: %d','(1+lvl)*(100+clc5*par8)/100',36),('Maximum added damage: %d','(2+lvl)*(100+clc5*par8)/100',36),('Projectiles: %d','clc1',36),('Mana cost: %d',mana_formula(quake),36)])
  # Match War Cry's existing six-warcry hard-point synergy and its rate.
  quake['calc5']=by[154]['calc5'];quake['Param8']='5';quake['DmgSymPerCalc']=''
  qd=ds[quake['skilldesc']];wd=ds[by[154]['skilldesc']]
  for key in wd:
   if key.startswith('dsc3'):qd[key]=wd[key]
  qd['dsc3textb1']='WorkshopBarbarian137Name';qd['desccalcb3']='1';qd['descline6']='36';qd['desctexta6']='WorkshopQuakeAttack';qd['desccalca6']='100+10*lvl';labels['WorkshopQuakeAttack']='Attack rating: +%d%%';qd['desccalca5']='8+lvl';qd['desccalcb5']='2'
  # Keep the level-scaled flat physical component separate from the weapon
  # snapshot; runtime applies the same hard-point synergy to both components.
  # Feast uses native corpse validation/consumption; custom HP/buff are applied
  # once per successful consumption by the bounded combat handler.
  feast=by[131];feast.update(srvstfunc='',srvdofunc='18',cltstfunc='',cltdofunc='',srvmissilea='',TargetCorpse='',TargetPet='',range='none',anim='SC',monanim='SC',calc1='100',calc2='0',calc3='0',calc4='(3+lvl/5)*100',calc5='20+3*lvl',calc6='(8+lvl/5)*25',aurafilter='4354',mana='0',lvlmana='0',minmana='0',startmana='0',localdelay='0',InTown='1',perdelay='50',leftskill='0',rightskill='1')
  for field in ('aura','periodic','periodicClearAura','repeat','immediate','TargetableOnly','TgtPlaceCheck','SearchEnemyXY','SearchOpenXY'):feast[field]=''
  label(feast,'Feast of War','Toggle: nearby corpses restore life and grant damage. Reserves mana while active.', [('Life restored per corpse: %d%%','clc4/100',36),('Physical weapon damage: +%d%%','clc5',36),('Base buff duration: %d seconds','clc6/25',36),('Mana reserved: %d',"max(1,(15+3*lvl)*100/(100+stat('workshop_reservation_efficiency'.accr)))",36),('Corpse pulse interval: %d seconds','2',36)])
  # Reuse the removed Double Swing ID rather than growing/renumbering the
  # native class-skill save order. Only its meaning changes, as requested.
  # Public Hydra handler already creates three consecutive monster classes.
  # Keep their original Ancient BaseId/AI so Talic/Madawc/Korlic retain skills.
  call=by[62].copy();call.update(skill='Call of the Ancients',**{'*Id':'133'});skills[skills.index(by[133])]=call;by[133]=call
  by[140]['charclass']='bar'  # Preserve the original 30-entry class/save ordering; this slot is hidden and disabled.
  call.update(charclass='bar',skilldesc='workshop_call_ancients',reqlevel='30',reqskill1='Concentrate',reqskill2='',reqskill3='',pettype='workshop_ancients',petmax='3',summode='NU',mana='30',lvlmana='0',localdelay='1500',Param1='775',Param2='25',calc1='0',calc2='0',calc3='0',srvstfunc='14',srvdofunc='144',cltstfunc='17',cltdofunc='',ItemEffect='0',InTown='0',HitShift='8',EType='')
  # Blank the original Hydra damage/synergy; explicit pet stats are below.
  for k in call:
   if k.startswith(('EMin','EMax','ELen','sumskill','sumsk','passivestat','passivecalc','aurastat')):call[k]=''
  for k in ('DmgSymPerCalc','EDmgSymPerCalc','aurastate','auratargetstate'):call[k]=''
  desc=ds[by[152]['skilldesc']].copy();desc['skilldesc']=call['skilldesc'];desc.update(ancient_position);desc['IconCel']=legacy_swing_icon;descs.append(desc);ds[desc['skilldesc']]=desc
  # The former Berserk position is captured explicitly before any art changes.
  # IDs remain at the end; only class/coordinates expose the new skill.
  typ=next(r for r in types if r['pet type']=='valkyrie').copy();typ['pet type']='workshop_ancients';typ['name']='WorkshopAncientsGroup';typ['icontype']='2';typ['drawhp']='0';types.append(typ);labels['WorkshopAncientsGroup']='Ancients'
  first=max(int(r['*hcIdx']) for r in monsters if r['*hcIdx'].isdigit())+1
  # Private summons must not inherit an Ancient boss BaseId. Native spawn
  # initialization indexes a fixed equipment table with (class - bossBase);
  # IDs 855+ would overrun it, then dereference an invalid Hell item record.
  for index,base in enumerate(('ancientbarb1','ancientbarb2','ancientbarb3')):
   mon=next(r for r in monsters if r['Id']==base).copy();mon.update(Id='workshop_'+base,BaseId='workshop_'+base,NextInClass='',MonProp='',NameStr='WorkshopAncientName'+str(index),**{'*hcIdx':str(first+index),'Align':'1','neverCount':'1','killable':'0','isSpawn':'0','boss':'0','primeevil':'0','unique':'0','noRatio':'1','flying':'0','aidel':'5','aidel(N)':'5','aidel(H)':'5','TreasureClass1':'','TreasureClass1(N)':'','TreasureClass1(H)':''})
   for suffix in ('','(N)','(H)'):
    for field in ('minHP','maxHP','MinHP','MaxHP'):
     k=field+suffix
     if k in mon:mon[k]='100'
   template=next(r for r in monsters if r['Id']=='valkyrie')
   mon['AI']='GenericPet';mon['switchai']='1';mon['inventory']='1'
   for k in mon:
    if k.startswith(('aip','aidist')):mon[k]=template.get(k,'')
    if k.startswith(('Skill','Sk')) and (k.startswith('Skill') or 'mode' in k or 'lvl' in k):mon[k]=''
   for k in list(mon):
    if k not in monh:
     actual=next((h for h in monh if h.casefold()==k.casefold()),None)
     if actual:mon[actual]=mon[k]
     del mon[k]
   for k in mon:
    if k.startswith(('TreasureClass','Exp')):mon[k]='' if k.startswith('TreasureClass') else '0'
    if k.startswith(('A1MinD','A1MaxD','A2MinD','A2MaxD','S1MinD','S1MaxD')):mon[k]='0'
   # Private animation/selection records make only these pets untargetable.
   extra=next(r for r in extras if r['Id']==base).copy();extra['Id']='workshop_'+base;extra['*hcIdx']=str(max(int(r['*hcIdx']) for r in extras if r['*hcIdx'].isdigit())+1)
   extra['isSel']='0';extra['isAtt']='0';extras.append(extra);mon['MonStatsEx']=extra['Id']
   monsters.append(mon);labels['WorkshopAncientName'+str(index)]=('Talic','Madawc','Korlic')[index]
  call['summon']='workshop_ancientbarb1'
  petstats=[('maxhp',"(700+120*lvl)*(100+stat('ws_pet_life'.accr))/100*256"),('mindamage','80+20*lvl'),('maxdamage','120+25*lvl'),('damagepercent',"((skill('Melee Mastery'.lvl)>0)?skill('Melee Mastery'.ln34):0)+stat('ws_pet_damage'.accr)+5*(skill('Concentrate'.blvl)+skill('Stun'.blvl))"),('item_tohit_percent',"(skill('Melee Mastery'.lvl)>0)?skill('Melee Mastery'.ln12):0"),('item_armor_percent',"(skill('Iron Skin'.lvl)>0)?(10+10*skill('Iron Skin'.blvl)+min((skill('Iron Skin'.lvl)-skill('Iron Skin'.blvl))*3,225)):0"),('damageresist',"skill('Iron Skin'.lvl)"),*[(s,"skill('Natural Resistance'.dm12)+skill('Natural Resistance'.lvl)/5") for s in ('fireresist','coldresist','lightresist','poisonresist')],('velocitypercent',"(skill('Increased Speed'.lvl)>0)?(10+2*skill('Increased Speed'.lvl)+stat('ws_pet_speed'.accr)):stat('ws_pet_speed'.accr)"),('attackrate',"(skill('Increased Speed'.lvl)>0)?(10+2*skill('Increased Speed'.lvl)+stat('ws_pet_speed'.accr)):stat('ws_pet_speed'.accr)"),('item_fastergethitrate',"(skill('Increased Speed'.lvl)>0)?(10+2*skill('Increased Speed'.lvl)):0"),('item_deadlystrike',"(skill('Melee Mastery'.lvl)>0)?(10+skill('Melee Mastery'.blvl)):0")]
  assert len(petstats)==15
  # Skills.txt has only fourteen passive columns in this build; remaining stats
  # are applied through the same temporary native pet-state list at runtime.
  for n,(stat,formula) in enumerate(petstats[:14],1):call['passivestat'+str(n)]=stat;call['passivecalc'+str(n)]=formula
  call['aurastat1']='';call['aurastatcalc1']=''  # Runtime applies the correct mastery critical chance once.
  label(call,'Call of the Ancients','Summons invulnerable, untargetable Talic, Madawc and Korlic. They inherit Barbarian passives. Cooldown: 60 seconds.', [('Duration: %d seconds','30+lvl',36),('Base minimum damage: %d','80+20*lvl',36),('Base maximum damage: %d','120+25*lvl',36)])
  call_desc=ds[call['skilldesc']]
  for n,name in enumerate(('Carnage','Tectonic Slam'),1):
   key='WorkshopAncientSynergy'+str(n);labels[key]=name+': +%d%% damage per hard point'
   call_desc['dsc3line'+str(n)]='36';call_desc['dsc3texta'+str(n)]=key;call_desc['dsc3calca'+str(n)]='5'
  # Friendly AI movement with three independently learned native combat skills.
  # The original Ancient AI hardcodes boss class IDs and summit quest state.
  # Reuse the allocated private IDs; NPC copies leave player skills intact.
  talic=newskill(144,'Workshop Talic Carnage');talic.update(ItemEffect='1',ItemCltEffect='1',mana='0',lvlmana='0',localdelay='0',monanim='xx')
  korlic=newskill(139,'Workshop Korlic Tectonic Slam');korlic.update(ItemEffect='1',ItemCltEffect='1',mana='0',lvlmana='0',localdelay='0',monanim='xx')
  madawc=newskill(36,'Workshop Madawc Double Throw');mid=madawc['*Id'];madawc.clear();madawc.update(original_throw)
  madawc.update(skill='Workshop Madawc Double Throw',charclass='',skilldesc='',**{'*Id':mid,'reqlevel':'1','reqskill1':'','reqskill2':'','reqskill3':'','srvdofunc':'74','monanim':'xx','mana':'0','lvlmana':'0','minmana':'0','localdelay':'0','ItemEffect':'1','ItemCltEffect':'1','calc1':'ln12','DmgSymPerCalc':''})
  ai_missile=max(int(r['*ID']) for r in missiles if r['*ID'].isdigit())+1
  axe=next(r for r in missiles if r['Missile']=='ancient throwing axe').copy();axe.update(Missile='workshop_madawc_axe',Skill=madawc['skill'],**{'*ID':str(ai_missile),'SrcDamage':'128','EType':'','ResultFlags':'5','HitFlags':'0','Pierce':'1'})
  for key in axe:
   if key.startswith(('EMin','EMax','ELen','MinDamage','MaxDamage','MinLevDam','MaxLevDam')):axe[key]=''
  missiles.append(axe)
  for key in ('srvmissile','cltmissile','srvmissilea','cltmissilea','cltmissileb','srvmissileb','srvmissilec','cltmissilec'):madawc[key]=''
  # Give Talic the actual original Whirlwind ID, as the enemy Ancient uses it.
  # Keep the old private row allocated so helper/save bindings do not shift.
  attacks=[int(r['*Id']) for r in (talic,madawc,korlic)]
  for n,r in enumerate((talic,madawc,korlic),1):call['sumskill'+str(n)]=r['skill'];call['sumsk'+str(n)+'calc']='lvl'
  # Native monster sequences resolve their animation through these skill slots.
  for index,name in enumerate(('ancientbarb1','ancientbarb2','ancientbarb3')):
   mon=next(r for r in monsters if r['Id']=='workshop_'+name);mon['Skill1']=(talic,madawc,korlic)[index]['skill'];mon['Sk1mode']=('seq_workshop_carnage','seq_workshop_doublethrow','seq_workshop_tectonic')[index];mon['Sk1lvl']='1'
  # Original weapon skills require actual equipment even on private NPCs.
  # oninit is required for native spawn-time equipment. Both throwing axes
  # belong to the same equipment bundle, not competing single-weapon rows.
  for name,item,second in (('workshop_ancientbarb1','crs',''),('workshop_ancientbarb2','tax','tax')):
   equip={k:'' for k in eh};equip.update(monster=name,oninit='1',level='0',item1=item,loc1='rarm',mod1='2',**{'*eol':'0'})
   if second:equip.update(item2=second,loc2='larm',mod2='2')
   equipment.append(equip)
  seq_source=HERE/'combat/upstream/data/global/excel/monseq.txt'
  sequence=seq_source.read_bytes()
  sequence+=(''.join('seq_workshop_doublethrow\tA1\t'+str(i)+'\t\t'+('1' if i in (3,7) else '')+'\t0\r\n' for i in range(8))).encode()
  # Each real attack sequence delivers one native skill animation event.
  for name,mode in (('seq_workshop_carnage','A1'),('seq_workshop_tectonic','A2')):
   sequence+=(''.join(name+'\t'+mode+'\t'+str(i)+'\t\t'+('1' if i==3 else '')+'\t0\r\n' for i in range(8))).encode()
  (p/'monseq.txt').write_bytes(sequence)
  # A real melee attack supplies native accuracy, attack timing and mana use.
  # Successful hits emit a counted fan of falling rocks through the runtime hook.
  rockdesc=by[140]['skilldesc'];rocks=by[126].copy()
  rocks.update(skill='Shower of Rocks',charclass='bar',skilldesc=rockdesc,**{'*Id':'140','reqlevel':'24','reqskill1':'Frenzy','reqskill2':'','reqskill3':'','localdelay':'0','mana':'20','lvlmana':'0','startmana':'0','InTown':'0','calc1':'0','calc2':'0','calc3':'0','calc4':'0','EType':'','SrcDam':'128','MinDam':'0','MaxDam':'0','DmgSymPerCalc':'','EDmgSymPerCalc':'','ResultFlags':'0','HitFlags':'0','ToHitCalc':by[152]['ToHitCalc'],'ToHit':by[152]['ToHit'],'LevToHit':by[152]['LevToHit']})
  for k in rocks:
   if k.startswith(('EMin','EMax','ELen','MinLevDam','MaxLevDam')):rocks[k]=''
  skills[skills.index(by[140])]=rocks;by[140]=rocks;ds[rockdesc].update(rocks_position)
  # Restore the previously working Blizzard spell as a hidden on-striking
  # action; the public skill remains a real melee attack.
  rain=newskill(59,'Workshop Rock Rain');rain.update(ItemEffect='28',ItemCltEffect='28',srvdofunc='28',calc1='6',calc2='5',EType='',SrcDam='128',MinDam='0',MaxDam='0',DmgSymPerCalc='',EDmgSymPerCalc='',mana='0',lvlmana='0',minmana='0',startmana='0',localdelay='0',castoverlay='',ItemCastOverlay='',stsound='',ItemCastSound='')
  for key in rain:
   if key.startswith(('EMin','EMax','ELen','MinLevDam','MaxLevDam')):rain[key]=''
  rock_sources=('blizzardcenter','blizzard1','blizzard2','blizzard3','blizzard4','blizzardexplode1','blizzardexplode2','blizzardexplode3')
  rock_names={name:'workshop_rocks_'+name for name in rock_sources};rock_variants=[];rock_control=0
  for name in rock_sources:
   r=next(r for r in missiles if r['Missile']==name).copy()
   ident=max(int(x['*ID']) for x in missiles if x['*ID'].isdigit())+1
   r.update(Missile=rock_names[name],**{'*ID':str(ident)})
   for key,value in list(r.items()):
    if value in rock_names:r[key]=rock_names[value]
   r['TravelSound']='';r['HitSound']=''
   if name=='blizzardcenter':rock_control=ident;r['Range']='25';r['Light']='0'
   elif name.startswith('blizzardexplode'):r['CelFile']='ExpArrowExplode';r['AnimLen']='16'
   else:
    rock_variants.append(ident);r.update(Skill=rain['skill'],EType='',SrcDamage='128',CelFile='expansion\\armageddon_rock',AnimLen='16',ELen='0',HitFlags='2',ClientSend='1')
   missiles.append(r)
  for key in ('srvmissilea','srvmissileb','srvmissilec','cltmissilea'):rain[key]=rock_names['blizzardcenter']
  label(rocks,'Shower of Rocks','A melee hit calls down rocks. Each rock deals 20% weapon damage. No cooldown.', [('Rocks per hit: %d','5+lvl/5',36),('Weapon damage per rock: %d%%','20',36),('Attack rating: +%d%%','toht',36),('Mana cost: %d',mana_formula(rocks),36)])
  # Retire rock rain without deleting its allocated IDs. The replacement is
  # a native shout animation plus five exact, individually applied hit pulses.
  stomp=by[126].copy();stomp.update(skill='Ground Stomp',charclass='bar',skilldesc=rockdesc,**{'*Id':'140','reqlevel':'24','reqskill1':'Frenzy','reqskill2':'','reqskill3':'','localdelay':'125','mana':'12','lvlmana':'0','startmana':'0','minmana':'1','manashift':'8','InTown':'0','SrcDam':'128','MinDam':'0','MaxDam':'0','DmgSymPerCalc':'','EDmgSymPerCalc':'','EType':'','calc1':'0','calc2':'0','calc3':'0','calc4':'min(25,10+lvl/5)','calc5':"5*(skill('Melee Mastery'.blvl)+skill('Throwing Mastery'.blvl))",'UseAttackRate':'1','ItemEffect':'1','monanim':'A1'})
  for key in stomp:
   if key.startswith(('EMin','EMax','ELen','aurastat','auraevent','passivestat','passivecalc')):stomp[key]=''
  for n in range(1,6):stomp['MinLevDam'+str(n)]=stomp['MaxLevDam'+str(n)]='0'
  stomp['aurastate']=stomp['auratargetstate']=stomp['auralencalc']=''
  for key in ('srvmissilea','srvmissileb','srvmissilec','cltmissilea'):stomp[key]=''
  skills[skills.index(by[140])]=stomp;by[140]=stomp
  # The existing private rain skill becomes a zero-damage visual shout. Calling
  # the native shout function directly avoids ambiguous item-proc handlers.
  visual=by[154].copy();visual.update(skill='Workshop Ground Stomp Visual',charclass='',skilldesc='',**{'*Id':rain['*Id'],'reqlevel':'1','reqskill1':'','reqskill2':'','reqskill3':'','SrcDam':'0','MinDam':'0','MaxDam':'0','EType':'','DmgSymPerCalc':'','EDmgSymPerCalc':'','calc1':'0','calc2':'0','calc3':'0','calc4':'0','calc5':'0','mana':'0','lvlmana':'0','startmana':'0','minmana':'0','aurastate':'','auratargetstate':'','auralencalc':''})
  for key in visual:
   if key.startswith(('EMin','EMax','ELen','MinLevDam','MaxLevDam','aurastat','auraevent','passivestat','passivecalc')):visual[key]=''
  for key in ('srvmissilea','srvmissileb','srvmissilec','cltmissilea'):visual[key]='workshop_stomp_ring'
  skills[skills.index(rain)]=visual;by[int(visual['*Id'])]=visual;rain=visual
  for retired in missiles:
   if retired.get('Skill')=='Workshop Rock Rain':retired['Skill']=visual['skill']
  ring=next(r for r in missiles if int(r['*ID'])==rock_control)
  ringid=ring['*ID'];ring.clear();ring.update(next(r for r in missiles if r['Missile']=='warcry'))
  ring.update(Missile='workshop_stomp_ring',Skill=visual['skill'],**{'*ID':ringid,'CollideType':'0','CollideKill':'0','ClientCol':'0','pSrvHitFunc':'','pSrvDmgFunc':'','pCltHitFunc':'','SrcDamage':'0','SrcMissDmg':'0','ResultFlags':'0','HitFlags':'0','NextHit':'','NextDelay':'','Vel':'24','MaxVel':'24','Range':'20','ClientSend':'1','TravelSound':'','HitSound':'','ExplosionMissile':'','HitSubMissile1':'','CltSubMissile1':'','CltHitSubMissile1':''})
  label(stomp,'Ground Stomp','A successful melee hit releases physical shockwaves. Each pulse can trigger hit effects and briefly stun enemies. Cooldown: 5 seconds.', [('Pulses per hit: %d','5+lvl/10',36),('Weapon damage per pulse: %d%%','(50+10*lvl)*(100+clc5)/100',36),('Radius: %d yards','40',36),('Mana cost: %d',mana_formula(stomp),36)])
  for field in ('ToHit','LevToHit','ToHitCalc'):stomp[field]=by[147][field]
  d=ds[rockdesc];d['desccalcb3']='3'
  d['descline5']='36';d['desctexta5']='WorkshopStompAttackRating';d['desccalca5']=by[147]['ToHitCalc'] or f"{by[147]['ToHit']}+{by[147]['LevToHit']}*(lvl-1)";labels['WorkshopStompAttackRating']='Attack rating: +%d%%'
  d['dsc2line1']='36';d['dsc2texta1']='WorkshopStompStun';d['dsc2calca1']='clc4';d['dsc2calcb1']='25';labels['WorkshopStompStun']='Stun duration: %d seconds'
  d['dsc2line2']='36';d['dsc2texta2']='WorkshopStompCooldown';d['dsc2calca2']='5';labels['WorkshopStompCooldown']='Cooldown: %d seconds'
  for n,name in enumerate(('Melee Mastery','Throwing Mastery'),1):
   key='WorkshopStompSynergy'+str(n);labels[key]=name+': +%d%% damage per hard point'
   d['dsc3line'+str(n)]='36';d['dsc3texta'+str(n)]=key;d['dsc3calca'+str(n)]='5'
  # Private level helpers avoid allocating more native item-stat IDs.
  helpers={}
  for sid in (*PASSIVES,131,140,int(call['*Id'])):
   h=newskill(540,'Workshop Barbarian Level '+by[sid]['skill']);h['calc4']=f"skill('{by[sid]['skill']}'.lvl)";h['calc5']=f"skill('{by[sid]['skill']}'.blvl)";h['calc6']=f"skill('{by[sid]['skill']}'.dm56)";h['calc7']=f"skill('{by[sid]['skill']}'.dm78)";helpers[sid]=int(h['*Id'])
  pull=newskill(137,'Workshop Battle Cry Pull')
  # Restore the native Taunt template captured before Quake replaced its row.
  keep_id=pull['*Id'];pull.clear();pull.update(old);pull.update(skill='Workshop Battle Cry Pull',charclass='',skilldesc='',**{'*Id':keep_id,'reqlevel':'1','reqskill1':'','auratargetstate':'taunt','auralencalc':'25','aurastat1':'velocitypercent','aurastatcalc1':'200','aurastat2':'','aurastatcalc2':'','ItemEffect':'1','mana':'0'})
  # A direct physical tick uses the engine's complete missile damage path.
  tick=newskill(565,'Workshop Bleeding Tick');tick.update(srvstfunc='',srvdofunc='10',srvmissile='',srvmissilea='',srvmissileb='',srvmissilec='',cltdofunc='',cltstfunc='',MinDam='1',MaxDam='1',HitShift='8',EType='',DmgSymPerCalc="max(stat('workshop_retribution_charge'.accr)*100-100,0)",EDmgSymPerCalc='',mana='0')
  ti=max(int(r['*ID']) for r in missiles if r['*ID'].isdigit())+1;tm=next(r for r in missiles if r['Missile']=='firebolt').copy();tm.update(Missile='workshop_bleeding_tick',Skill=tick['skill'],**{'*ID':str(ti),'EType':'','HitShift':'8','SrcDamage':'0','Vel':'1','MaxVel':'1','Range':'1','ToHit':'0','ResultFlags':'1','HitFlags':'0','pSrvDmgFunc':'','pSrvHitFunc':'','CollideKill':'1','NextHit':'','NextDelay':''});missiles.append(tm)
  stateids={}
  for name in ('carnage','feast','feast_aura','battle_pull','bleeding','impale','ancient_passives'):
   i=max(int(r['*ID']) for r in states if r['*ID'].isdigit())+1;r={k:'' for k in sth};r.update(state='workshop_'+name,**{'*ID':str(i),'aura':'1','*eol':'0'});states.append(r);stateids[name]=i
  rage=newskill(138,'Workshop Rage Buff');rage.update(aurastate='workshop_carnage',auratargetstate='',auralencalc='250',aurastat1='attackrate',aurastatcalc1='20+lvl',ItemEffect='1')
  next(r for r in states if r['state']=='workshop_carnage')['stat']='attackrate'
  feast['aurastate']='workshop_feast_aura';feast['auralencalc']='51'
  visuals=[]
  for index,base in enumerate(('ancientbarb1','ancientbarb2','ancientbarb3')):
   state=max(int(r['*ID']) for r in states if r['*ID'].isdigit())+1
   visual={k:'' for k in sth};visual.update(state='workshop_ancient_visual_'+str(index),**{'*ID':str(state),'aura':'1','transform':'1','gfxtype':'1','gfxclass':next(r['*hcIdx'] for r in monsters if r['Id']==base),'*eol':'0'});states.append(visual);visuals.append(state)
  # Expand only the splash missiles' native skill-derived radius. No extra hit.
  splash=[]
  for r in skills:
   if not r['skill'].startswith('proc_SplashDamage'):continue
   missile=next(x for x in missiles if x['Missile']==r['srvmissilea']);radius=int(missile['sHitPar1'] or 6);r['aurarangecalc']=f"{radius}*(100+5*skill('Shield Mastery'.lvl))/100";missile['sHitPar1']='0';splash.append(int(r['*Id']))
  for r in skills:
   for field,value in list(r.items()):
    if isinstance(value,str) and "skill('Double Swing'" in value:r[field]=value.replace("skill('Double Swing'","skill('Frenzy'")
  # Shared missile variants must use their actual source skill at cast time.
  # Native proc creation stores that ID, so the missile Skill column stays blank.
  rockhit=next(r for r in missiles if r['Missile']=='firebolt').copy();rockhit.update(Missile='workshop_rock_hit',Skill='Shower of Rocks',**{'*ID':str(max(int(r['*ID']) for r in missiles if r['*ID'].isdigit())+1),'pSrvHitFunc':'','pSrvDmgFunc':'','pCltHitFunc':'','SrcDamage':'128','EType':'','Range':'1','Vel':'1','MaxVel':'1','CelFile':'null','ResultFlags':'5','HitFlags':'2','ToHit':'0','ClientSend':'0'})
  rockhit.update(Missile='workshop_stomp_hit',Skill='Ground Stomp',SrcDamage='128',HitFlags='0',pSrvDmgFunc='7',dParam1='0',NextHit='',NextDelay='',ExplosionMissile='',HitSubMissile1='',CltHitSubMissile1='',TravelSound='',HitSound='')
  missiles.append(rockhit)
  record={'quake_missile':qi,'rain_skill':int(rain['*Id']),'rock_variants':rock_variants,'rage_skill':int(rage['*Id']),'rock_hit':int(rockhit['*ID']),'rock_control':rock_control,'call':int(call['*Id']),'ancient_classes':list(range(first,first+3)),'helpers':helpers,'states':stateids,'pull':int(keep_id),'tick_skill':int(tick['*Id']),'tick_missile':ti,'splash':splash,'ancient_attacks':attacks,'ancient_visuals':visuals,'redeemed_state':next(int(r['*ID']) for r in states if r['state']=='redeemed')}
  if bindings is not None:assert record==bindings
  bindings=record
  # Preserve all quoted vanilla component fields; append only our three rows.
  appended=extras[-3:];tail=''.join('\t'.join(r.get(k,'') for k in xh)+'\r\n' for r in appended)
  (p/'monstats2.txt').write_bytes(extra_raw+(b'' if extra_raw.endswith(b'\n') else b'\r\n')+tail.encode('utf8'))
  for table,header,rows in [('monequip',eh,equipment),('skills',sh,skills),('skilldesc',dh,descs),('missiles',mh,missiles),('states',sth,states),('pettype',ph,types),('monstats',monh,monsters)]:write(p/(table+'.txt'),header,rows)
 strings=root/'data/local/lng/strings/skills.json';rows=json.loads(strings.read_text(encoding='utf8'));known={r['Key']:r for r in rows};n=max(int(r['id']) for f in strings.parent.glob('*.json') for r in json.loads(f.read_text(encoding='utf-8-sig')) if isinstance(r,dict) and 'id' in r)+1
 for key,value in labels.items():
  if key in known:
   for lang in known[key]:
    if lang not in ('id','Key'):known[key][lang]=value
  else:
   r={k:value for k in rows[0] if k not in ('id','Key')};r.update(id=n,Key=key);rows.append(r);n+=1
 strings.write_text(json.dumps(rows,ensure_ascii=False,indent=2),encoding='utf8')
 (HERE/'BARBARIAN.json').write_text(json.dumps(bindings,indent=2),encoding='utf8')
 out='// Generated by barbarian_data.py.\n'
 out+=f"inline constexpr int RageSkill={bindings['rage_skill']},RockHit={bindings['rock_hit']};\n"
 out+=f"inline constexpr int RainSkill={bindings['rain_skill']};\n"
 out+=f"inline constexpr int QuakeMissile={bindings['quake_missile']};\n"
 out+='inline constexpr int RockVariants[]={'+','.join(map(str,bindings['rock_variants']))+'};\n'
 out+=f"inline constexpr int RockControl={bindings['rock_control']};\n"
 out+=f"inline constexpr int CallSkill={bindings['call']},PullSkill={bindings['pull']},TickSkill={bindings['tick_skill']},TickMissile={bindings['tick_missile']};\n"
 for key,value in bindings['states'].items():out+='inline constexpr int '+''.join(w.title() for w in key.split('_'))+'State='+str(value)+';\n'
 out+='inline constexpr int RedeemedState='+str(bindings['redeemed_state'])+';\n'
 out+='inline constexpr int AncientVisuals[]={'+','.join(map(str,bindings['ancient_visuals']))+'};\n'
 out+='inline constexpr int AncientAttacks[]={'+','.join(map(str,bindings['ancient_attacks']))+'};\n'
 out+='inline constexpr unsigned AncientClasses[]={'+','.join(map(str,bindings['ancient_classes']))+'};\n'
 out+='inline constexpr int Helpers[][2]={'+','.join('{'+str(k)+','+str(v)+'}' for k,v in bindings['helpers'].items())+'};\n'
 (HERE/'combat/barbarian_bindings.h').write_text(out,encoding='utf8')
 aura=HERE/'combat/aura_bindings.h';text=aura.read_text(encoding='utf8')
 assert '{131,' not in text
 text=text.replace('\n};',',\n{131,65,'+str(bindings['states']['feast_aura'])+','+str(bindings['helpers'][131])+',15,3}\n};',1)
 aura.write_text(text,encoding='utf8')
 registry=root/'data/hd/missiles/missiles.json';mapping=json.loads(registry.read_text(encoding='utf-8-sig'))
 mapping['workshop_quake']=mapping['shockwave']
 for source in rock_sources:
  mapping[rock_names[source]]='null' if source=='blizzardcenter' else mapping['shockwave'] if source.startswith('blizzardexplode') else 'workshop_rock_fall'
 mapping['workshop_rock_hit']='null'
 mapping['workshop_stomp_hit']='null'
 mapping['workshop_stomp_ring']=mapping['warcry']
 registry.write_text(json.dumps(mapping,ensure_ascii=False,indent=4),encoding='utf8')
 registry=root/'data/hd/character/monsters.json';mapping=json.loads(registry.read_text(encoding='utf-8-sig'))
 for name in ('ancientbarb1','ancientbarb2','ancientbarb3'):mapping['workshop_'+name]=mapping[name]
 registry.write_text(json.dumps(mapping,ensure_ascii=False,indent=4),encoding='utf8')
 manifest=HERE/'combat/assets.json';assets=json.loads(manifest.read_text(encoding='utf8'));assets.extend(('data/hd/character/monsters.json','data/global/excel/monseq.txt','data/global/excel/base/monseq.txt'));manifest.write_text(json.dumps(list(dict.fromkeys(assets)),indent=2),encoding='utf8')
 print('Prepared Barbarian skill moves, eight reworks, three Ancients, Quake and runtime bindings.')
