"""Druid rework: append private records; preserve every public and existing private ID."""
import json,re
from pathlib import Path
from combat.port import read,write
HERE=Path(__file__).resolve().parent

def install(root):
 root=Path(root);labels={};bindings=None;auras=None;visuals={}
 for folder in ('data/global/excel','data/global/excel/base'):
  p=root/folder;tables={n:read(p/(n+'.txt')) for n in ('skills','skilldesc','missiles','states','monstats','pettype','itemstatcost')}
  skills=tables['skills'][1];ds={r['skilldesc']:r for r in tables['skilldesc'][1]};missiles=tables['missiles'][1];states=tables['states'][1];monsters=tables['monstats'][1]
  by={int(r['*Id']):r for r in skills if r['*Id'].isdigit()};old={i:r.copy() for i,r in by.items()};local={};la=[]
  # The nine-bit item-stat table is full (511 terminates saved/network lists).
  # Reuse the unsaved missile-only tag on PLAYER aura lists. Its existing code
  # reads it only from fire missiles; player and missile stat lists are isolated.
  # This avoids adding an unrepresentable stat or altering any saved item format.
  physical=next(r for r in tables['itemstatcost'][1] if r['Stat']=='fmmp_native_blaze_tag')
  local['PhysicalSpellStat']=int(physical['*ID'])
  def clone(sid,name):
   r=old[sid].copy();r.update(skill=name,charclass='',skilldesc='',**{'*Id':str(max(by)+1),'reqskill1':'','reqskill2':'','reqskill3':'','reqlevel':'1','ItemEffect':'1'});by[int(r['*Id'])]=r;skills.append(r);return r
  def replace(sid,template=0):
   r=by[sid];ident={k:r[k] for k in ('skill','*Id','skilldesc','reqlevel','reqskill1','reqskill2','reqskill3','maxlvl')};r.clear();r.update(old[template]);r.update(ident,charclass='dru',restrict='1');return r
  def state(name,template='enchant',**kwargs):
   r=next(r for r in states if r['state']==template).copy();r.update(state=name,**{'*ID':str(max(int(x['*ID']) for x in states if x['*ID'].isdigit())+1),'skill':'','group':'','stat':'','setfunc':'','remfunc':'','overlay1':'','overlay2':'','castoverlay':'','itemtype':'','itemtrans':'','curse':'','aura':'1'});r.update(kwargs);states.append(r);return r
  def missile(source,name,**kwargs):
   r=next(r for r in missiles if r['Missile']==source).copy();r.update(Missile=name,**{'*ID':str(max(int(x['*ID']) for x in missiles if x['*ID'].isdigit())+1)});r.update(kwargs);missiles.append(r);visuals[name]=source;return r
  def label(sid,name,text,lines):
   d=ds[by[sid]['skilldesc']];key='WorkshopDruid'+str(sid)
   for k in ('str name','str alt'):labels[d[k]]=name
   labels[key+'Name']=name;labels[key+'Desc']=text;d.update({'str name':key+'Name','str alt':key+'Name','str short':key+'Desc','str long':key+'Desc','descdam':'','descatt':'','ddam calc1':'','ddam calc2':''})
   for k in d:
    if k.startswith(('descline','desctext','desccalc','dsc2','dsc3','descmissile','p1dm','p2dm','p3dm')):d[k]=''
   for i,(text,calc) in enumerate(lines):
    pre,n=('desc',i+1) if i<6 else ('dsc2',i-5);assert n<=6
    keyline=key+'Line'+str(i);labels[keyline]=text;d[pre+'line'+str(n)]='36';d[pre+'texta'+str(n)]=keyline;d[pre+'calca'+str(n)]=calc
  def damage(r,element,lo,hi,growth):
   r.update(EType=element,EMin=str(lo if element else 0),EMax=str(hi if element else 0),MinDam=str(0 if element else lo),MaxDam=str(0 if element else hi),SrcDam='0',HitShift='8',EDmgSymPerCalc='',DmgSymPerCalc='')
   for n in range(1,6):
    r['EMinLev'+str(n)]=r['EMaxLev'+str(n)]=str(growth if element else 0);r['MinLevDam'+str(n)]=r['MaxLevDam'+str(n)]=str(0 if element else growth)
  # Ravens retain their native pet AI, hit count and synergies on a hidden skill.
  raven=clone(221,'Workshop Wild Raven');local['Raven']=int(raven['*Id']);raven.update(restrict='1',mana='0',lvlmana='0',minmana='0',startmana='0')
  # Lycanthropy is evaluated at the level/base level of the respective animal form.
  for sid in (225,229,234,235,244,245,247,249,250):by[sid]['restrict']='1'
  for sid in (223,228):
   r=by[sid]
   for k,v in list(r.items()):
    for n in (1,2,3):v=v.replace("skill('Shape Shifting'.pst%d)"%n,'('+old[224]['passivecalc'+str(n)].replace('par3','20').replace('par4','5').replace('par5','1').replace('par6','1').replace('par7','2').replace('par8','2').replace('par9','3')+')')
    r[k]=v
   r['auralencalc']='(50+50*lvl)*25'
   lines=[('Maximum life: +%d%%','ast3'),('Duration: %d seconds','50+50*lvl')]
   if sid==223:lines += [('Attack speed: +%d%%','ast1'),('Attack rating: +%d%%','ast2'),('Movement speed: +%d%%','ast6'),('Dodge and avoidance: %d%%','ast4')]
   else:lines += [('Physical damage: +%d%%','ast1'),('Defense: +%d%%','ast2'),('Physical damage reduction: %d%%','ast5')]
   label(sid,'Werewolf' if sid==223 else 'Werebear','Animal form with the former Lycanthropy bonuses built in. Scales with this form level; no separate passive required.',lines)
  primal=state('workshop_primal_fury');alpha=state('workshop_alpha_call');avatar=state('workshop_wild_avatar','wolf',group='3',gfxclass='289',gfxtype='1',restrict='1',meleeonly='')
  for sid,st,name,base,growth,lines,desc in [
   (239,primal,'Primal Fury',30,4,[('Attack speed: +%d%%','10+lvl'),('Attack rating: +%d%%','50+10*lvl'),('Physical attack damage: +%d%%','20+5*lvl'),('Defense: +%d%%','30+10*lvl'),('All resistances: +%d%%','10+lvl'),('Fire explosion minimum: %d','30+15*lvl'),('Fire explosion maximum: %d','60+25*lvl')],'Every fifth successful melee hit releases a fire explosion. Works in both animal forms. Recast to disable.'),
   (224,alpha,"Alpha's Call",25,3,[('Minion damage: +%d%%','15+3*(lvl-1)'),('Minion attack speed: +%d%%','5+lvl-1'),('Minion movement speed: +%d%%','10+2*(lvl-1)'),('Raven summoned every %d seconds','5'),('Raven skill level: %d','lvl')],'Reserve mana to strengthen all your summons, including summons granted by items. No life bonus. Summons one Raven every 5 seconds at this skill level. Recast to disable.'),
   (221,avatar,'Avatar of the Wilds',40,5,[('Fire/cold/physical spell damage: +%d%%','15+3*lvl'),('Cast speed: +%d%%','10+lvl'),('Maximum life: +%d%%','20+2*lvl'),('Faster hit recovery: +%d%%','15+2*lvl'),('Maximum elemental resistances: +%d%%','lvl/10')],'Stone elemental form. Cast spells and summon companions. Recast to revert.')]:
   r=replace(sid);r.update(srvstfunc='',srvdofunc='18',cltstfunc='',cltdofunc='',anim='SC',seqtrans='SC',monanim='SC',range='none',aurastate=st['state'],auralencalc='50',mana='0',lvlmana='0',startmana='0',minmana='0',InTown='1',leftskill='0',rightskill='1',passive='',passivestate='',localdelay='0',globaldelay='0',restrict='1')
   st['skill']=r['skill']
   for k in r:
    if k.startswith(('passivestat','passivecalc','aurastat','auraevent','srvmissile','cltmissile')):r[k]=''
   r.update(reqskill1='',reqskill2='',reqskill3='')
   helper=clone(0,'Workshop Druid Reservation '+str(sid));helper['calc4']="skill('"+r['skill']+"'.lvl)";la.append((sid,65,int(st['*ID']),int(helper['*Id']),base,growth))
   label(sid,name,desc,lines+[('Mana reserved: %d',f"max(1,({base}+{growth}*lvl)*100/(100+stat('workshop_reservation_efficiency'.accr)))")]);local[{239:'PrimalState',224:'AlphaState',221:'AvatarState'}[sid]]=int(st['*ID'])
  by[224]['reqlevel']='6';by[221]['reqlevel']='18'
  # Swap only tree positions, preserving learned skill IDs and artwork.
  da,db=ds[by[221]['skilldesc']],ds[by[224]['skilldesc']]
  for k in ('SkillPage','SkillRow','SkillColumn','ListRow'):da[k],db[k]=db[k],da[k]
  # Summon synergies still count invested points in the public Raven/Avatar slot.
  # Keep the shared summon-point formula there; the hidden raven is not learned.
  by[221]['calc5']=old[221]['calc5']
  # Native Raven AI reads the public Raven record, even for a private summon.
  # Preserve its hit budget/damage parameters instead of letting clc3 become zero.
  for k,v in old[221].items():
   if k.startswith(('Param','MinDam','MaxDam','MinLevDam','MaxLevDam')) or k in ('summon','pettype','summode','petmax','calc1','calc2','calc3','DmgSymPerCalc','HitShift'):by[221][k]=v
  by[239].update(reqskill1='Feral Rage',reqskill2='Maul')
  # Downstream summons must not require learning the new elemental form.
  for sid in (226,227):
   for k in ('reqskill1','reqskill2','reqskill3'):
    if by[sid][k]=='Raven':by[sid][k]=''
  # Private form source keeps native morph install/removal callbacks.
  morph=clone(223,'Workshop Wild Morph');morph.update(aurastate=avatar['state'],auralencalc='75',mana='0',lvlmana='0',minmana='0')
  for k in morph:
   if k.startswith(('aurastat','aurastatcalc')):morph[k]=''
  local['Morph']=int(morph['*Id']);local['WolfState']=int(next(s for s in states if s['state']=='wolf')['*ID']);local['BearState']=int(next(s for s in states if s['state']=='bear')['*ID'])
  # Dedicated no-weapon direct hit: no explosion control bit suppressing native damage.
  hit=missile('workshop_modifier_hit','workshop_druid_hit',Explosion='0',Skill='',SrcDamage='0',ClientSend='0');local['HitMissile']=int(hit['*ID'])
  for sid,name,element,lo,hi,growth,desc in [(230,"Winter's Grasp",'cold',15,25,8,'A wide frost cone leaves icy ground. Cold damage once per second, with 35% slow.'),(240,'Storm Seeds','',20,30,10,'Fires wind seeds that explode on first impact. Physical spell damage in 3 yards; 50% more damage against slowed enemies.')]:
   r=replace(sid);r.update(srvstfunc='',srvdofunc='18',cltstfunc='',cltdofunc='',anim='SC',monanim='SC',range='none',mana='10',lvlmana='1',minmana='1',startmana='0',restrict='1',InTown='0',leftskill='1',rightskill='1');damage(r,element,lo,hi,growth)
   syn=old[245]['EDmgSymPerCalc'];r['Param5']=old[245]['Param5'];r['Param8']=old[245]['Param8']
   # Match Tornado's elemental-tree and Spirit Wolf hard-point synergies.
   total=syn if element else "(100+("+syn+"))*(100+stat('fmmp_native_blaze_tag'.accr))/100-100"
   r['EDmgSymPerCalc' if element else 'DmgSymPerCalc']=total;r['calc4']='100+('+total+')'
   r['calc1']='enma' if element else 'pnma';r['calc2']='exma' if element else 'pxma'
   label(sid,name,desc,[('Minimum damage per '+('second' if element else 'impact')+': %d',r['calc1']),('Maximum damage per '+('second' if element else 'impact')+': %d',r['calc2']),('Mana cost: %d','9+lvl'),('Radius / range: %d yards','8+lvl/5' if element else '3')]+([('Duration: %d seconds','3+lvl/10')] if element else [('Projectiles: %d','3+lvl/10')]))
   d=ds[r['skilldesc']];td=ds[old[245]['skilldesc']]
   for k,v in td.items():
    if k.startswith('dsc3'):d[k]=v
   d['dsc3textb1']=d['str name']
   labels['WorkshopDruidSynergy']='%s: +%d%% Damage per Level'
   for n in (2,3):d['dsc3texta'+str(n)]='WorkshopDruidSynergy'
   ds[r['skilldesc']].update(descdam='5');key='WorkshopDruidSheet'+str(sid);labels[key]=name+(' (per second)' if element else ' (impact)');ds[r['skilldesc']]['str alt']=key
  # Visual-only moving seeds are manually published; their bounded scheduler owns impact damage.
  for key,source,name,vel,life in [('SeedMissile','twister','workshop_storm_seed',20,25),('IceVisual','frostnova','workshop_winter_wave',20,12),('IceGround','frostnova','workshop_winter_ground',0,30),('AvatarVisual','tornado','workshop_wild_tornado',0,40),('FireVisual','fireball','workshop_primal_fire',0,10)]:
   r=missile(source,name,Skill='',pSrvDoFunc='1',pSrvHitFunc='',pSrvDmgFunc='',pCltDoFunc='1',pCltHitFunc='',Vel=str(vel),MaxVel=str(vel),VelLev='0',Range=str(life),CollideType='0',ClientCol='0',ClientSend='1',SrcDamage='0',Explosion='0',EType='',MinDamage='0',MaxDamage='0',EMin='0',EMax='0',LoopAnim='1')
   for k in r:
    if k.startswith(('MinLev','MaxLev','MinELev','MaxELev','SubMissile','CltSubMissile','HitSubMissile','CltHitSubMissile')):r[k]=''
   local[key]=int(r['*ID'])
  for key in ('CompanionState','VineState','SlowState','SolarState'):
   st=state('workshop_druid_'+key.lower(),curse='1' if key=='SlowState' else '');local[key]=int(st['*ID'])
  # Preserve native pet formulas; multiply the base life, which also multiplies growth.
  for sid,factor,key in [(226,5,'OakClass'),(236,5,'WolverineClass'),(246,5,'BarbsClass'),(222,3,'PoisonClass'),(231,3,'CarrionClass'),(241,3,'SolarClass')]:
   r=by[sid];m=next(m for m in monsters if m['Id']==r['summon']);local[key]=int(m['*hcIdx']);r['restrict']='1'
   for k in ('minHP','maxHP','minHP(N)','maxHP(N)','minHP(H)','maxHP(H)'):
    if m.get(k):m[k]=str(int(m[k])*factor)
   if factor==5:
    for n,(stat,calc) in enumerate([('fireresist','min(50+lvl,85)'),('coldresist','min(50+lvl,85)'),('lightresist','min(50+lvl,85)'),('poisonresist','min(50+lvl,85)'),('damageresist','25'),('maxfireresist','85'),('maxcoldresist','85'),('maxlightresist','85'),('maxpoisonresist','85')],1):r['passivestat'+str(n)]=stat;r['passivecalc'+str(n)]=calc
    for diff in ('','(N)','(H)'):
     for elem in ('Fi','Co','Li','Po','Dm'):m['Res'+elem+diff]='0'
   else:r['petmax']="1+lvl/10+stat('ws_pet_count'.accr)"
   helper=clone(0,'Workshop Druid Level '+str(sid));helper['calc4']="skill('"+r['skill']+"'.lvl)";local['Level'+str(sid)]=int(helper['*Id'])
   name={226:'Oak Sage',236:'Heart of Wolverine',246:'Spirit of Barbs',222:'Poison Creeper',231:'Carrion Vine',241:'Solar Creeper'}[sid]
   extra={226:'Adds life regeneration to nearby allies.',236:'Damage and attack rating aura, with attack speed.',246:'Reflects damage and grants physical damage reduction to nearby allies.',222:'Poisoned targets take 15% +1% per level more damage from your summons for 4 seconds.',231:'Consumes corpses to restore player life, heal your summons and grant them 5 seconds of regeneration.',241:'Consumes corpses to restore player mana and grant player and summons 5 seconds of attack, cast and movement speed.'}[sid]
   lines=[('Spirit HP: %d' if factor==5 else 'Creeper HP: %d','clc1'),('Maximum summons: %d','pets')]
   if factor==5:lines += [('Spirit elemental resistances: %d%%','min(50+lvl,85)'),('Spirit magic resistance: %d%%','25'),('Spirit physical damage reduction: %d%%','25')]
   if sid==226:lines += [('Aura maximum life: +%d%%','15+lvl-1'),('Aura life regeneration per second: %d','2+lvl')]
   if sid==236:lines += [('Aura physical damage: +%d%%','15+5*(lvl-1)'),('Aura attack rating: +%d%%','25+7*(lvl-1)'),('Aura attack speed: +%d%%','min(5+lvl,30)'),('Aura cast speed: +%d%%','min(5+lvl,30)')]
   if sid==246:lines += [('Aura reflected damage: %d','10+5*lvl'),('Aura physical damage reduction: %d%%','min(5+lvl/2,25)')]
   if sid==222:lines += [('Minion damage against poisoned targets: +%d%%','15+lvl'),('Poison damage minimum per second: %d','enms*25/256'),('Poison damage maximum per second: %d','exms*25/256')]
   if sid==231:lines += [('Life restored: %d%%','3+lvl/2'),('Minion regeneration per second: %d%%','2+lvl/10')]
   if sid==241:lines += [('Mana restored: %d%%','3+lvl/2'),('All speeds for 5 seconds: +%d%%','5+lvl')]
   if factor==5:lines += [('Aura radius: %d yards','(30+2*(lvl-1))*2/3')]
   lines += [('Mana cost: %d','usmc/256')]
   label(sid,name,extra,lines)
   # Native life formatter resolves monster base life, difficulty and summon scaling.
   ds[r['skilldesc']]['descline1']='13'
  # Native corpse-consuming AI is replaced by the bounded consume scheduler;
  # walking/burrowing visuals remain the normal vine AI.
  cm=next(m for m in monsters if m['Id']==by[231]['summon']);cm.update(AI='Vines',Skill1='Attack')
  # Restore native vine mana recovery identity even though Reimagined used a damage vine.
  by[298].update(aurastat2='hpregen',aurastatcalc2="(2+lvl)*256/25")
  by[296].update(aurafilter='65795',aurastat1='item_attackertakesdamage',aurastatcalc1='10+5*lvl',aurastat2='damageresist',aurastatcalc2='min(5+lvl/2,25)',aurastat3='',aurastatcalc3='',aurastat4='',aurastatcalc4='',aurastat5='',aurastatcalc5='')
  local['PrimalLevel']=int(next(x for x in by.values() if x['skill']=='Workshop Druid Reservation 239')['*Id'])
  for name,(h,rows) in tables.items():write(p/(name+'.txt'),h,rows)
  if bindings:assert local==bindings and la==auras
  bindings=local;auras=la
 strings=root/'data/local/lng/strings/skills.json';rows=json.loads(strings.read_text(encoding='utf-8-sig'));known={r['Key']:r for r in rows};last=max(r['id'] for f in strings.parent.glob('*.json') for r in json.loads(f.read_text(encoding='utf-8-sig')) if 'id' in r)
 for k,text in labels.items():
  if not k:continue
  if k not in known:last+=1;known[k]={'id':last,'Key':k,**{lang:text for lang in rows[0] if lang not in ('id','Key')}};rows.append(known[k])
  else:
   for lang in known[k]:
    if lang not in ('id','Key'):known[k][lang]=text
 strings.write_text(json.dumps(rows,ensure_ascii=False,indent=2),encoding='utf8')
 path=root/'data/hd/missiles/missiles.json';mapping=json.loads(path.read_text(encoding='utf-8-sig'))
 for name,source in visuals.items():mapping[name]=mapping.get(source,'null')
 path.write_text(json.dumps(mapping,indent=2),encoding='utf8')
 path=HERE/'combat/aura_bindings.h';path.write_text(path.read_text().replace('\n};',',\n'+',\n'.join('{'+','.join(map(str,a))+'}' for a in auras)+'\n};',1))
 (HERE/'combat/druid_bindings.h').write_text('// Generated; stable appended private IDs.\n'+''.join(f'inline constexpr int {k}={v};\n' for k,v in bindings.items()))
 (HERE/'DRUID.json').write_text(json.dumps({'bindings':bindings,'auras':auras},indent=2))
 print('Druid: five replacements, integrated Lycanthropy, stronger spirits and multiple creepers.')
