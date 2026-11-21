"""Assassin slots and native helpers; public IDs remain stable for saved characters."""
import json
from pathlib import Path
from combat.port import read,write
HERE=Path(__file__).resolve().parent
NAMES={263:'Afterimage',278:'Death Mark',256:'Gravity Trap',276:'Overclock',265:'Expose Weakness',257:'Spiked Ground',266:'ATMG Sentry',277:'Queen of Blades',275:'Omnislash',260:'Dragon Flight'}
BUFFS=(263,278,276,257,277)
OMNISLASH_DAMAGE="(200+25*lvl)*(100+3*skill('Cobra Strike'.blvl))/100"
def install(root):
 root=Path(root);labels={};bindings=None;auras=[];visuals={}
 for folder in ('data/global/excel','data/global/excel/base'):
  p=root/folder
  sh,skills=read(p/'skills.txt');dh,descs=read(p/'skilldesc.txt');mh,missiles=read(p/'missiles.txt');th,states=read(p/'states.txt');nh,monsters=read(p/'monstats.txt');ph,pets=read(p/'pettype.txt')
  by={int(r['*Id']):r for r in skills if r['*Id'].isdigit()};old={i:r.copy() for i,r in by.items()};ds={r['skilldesc']:r for r in descs}
  def clone(source,name):
   r=source.copy();r.update(skill=name,charclass='',skilldesc='',**{'*Id':str(max(by)+1),'reqskill1':'','reqskill2':'','reqskill3':'','reqlevel':'1','mana':'0','lvlmana':'0','startmana':'0','minmana':'0','localdelay':'0','globaldelay':'0','ItemEffect':'1'})
   skills.append(r);by[int(r['*Id'])]=r;return r
  def replace(sid,template):
   r=by[sid];identity={k:r[k] for k in ('skill','*Id','skilldesc','maxlvl','reqlevel')};r.clear();r.update(template);r.update(identity,charclass='ass',reqskill1='',reqskill2='',reqskill3='');return r
  def state(name):
   r=next(x for x in states if x['state']=='enchant').copy();r.update(state=name,**{'*ID':str(max(int(x['*ID']) for x in states if x['*ID'].isdigit())+1),'group':'','skill':'','stat':'','setfunc':'','itemtype':'','itemtrans':'','castoverlay':'','overlay1':'','aura':'1','curse':''});states.append(r);return r
  def missile(source,name,**kw):
   r=next(x for x in missiles if x['Missile']==source).copy();r.update(Missile=name,**{'*ID':str(max(int(x['*ID']) for x in missiles if x['*ID'].isdigit())+1)});r.update(kw);missiles.append(r);visuals[name]=source;return r
  def label(sid,description,lines):
   d=ds[by[sid]['skilldesc']];key='WorkshopAssassin'+str(sid);labels[key+'Name']=NAMES[sid];labels[key+'Desc']=description
   d.update({'str name':key+'Name','str alt':key+'Name','str short':key+'Desc','str long':key+'Desc'})
   for k in d:
    if k.startswith(('descline','desctext','desccalc','dsc2','dsc3','descmissile','p1dm','p2dm','p3dm')) or k in ('descdam','ddam calc1','ddam calc2','descatt'):d[k]=''
   for i,(caption,formula) in enumerate(lines,1):
    prefix='desc' if i<=6 else 'dsc2';n=i if i<=6 else i-6;keyline=key+'Line'+str(i);labels[keyline]=caption;d[prefix+'line'+str(n)]='36';d[prefix+'texta'+str(n)]=keyline;d[prefix+'calca'+str(n)]=formula
  legacy={}
  for sid in NAMES:
   r=clone(old[sid],'Workshop Assassin Legacy '+old[sid]['skill']);legacy[old[sid]['skill']]=r['skill']
  # Preserve monster and old projectile references before changing player rows.
  for r in monsters:
   for k,v in list(r.items()):
    if k.startswith('Skill') and v in legacy:r[k]=legacy[v]
  for r in missiles:
   if r['Skill'] in legacy:r['Skill']=legacy[r['Skill']]
  flight=replace(260,old[275]);flight.update(reqlevel=old[260]['reqlevel'])
  label(260,'Finishing move. Teleport to an enemy and release charged attacks.', [('Attack rating: +%d%%','toht'),('Damage: +%d%%','clc1'),('Mana cost: %d','7')])
  ds[flight['skilldesc']].update(descdam='7',**{'ddam calc1':'clc1','ddam calc2':'0','descatt':'1'})
  # A private native Dragon Flight retains srvstfunc 12 / srvdofunc 52 and charge release.
  blink=clone(old[54],'Workshop Omnislash Blink')
  finisher=clone(old[260],'Workshop Omnislash Finisher');finisher.update(srvstfunc='',cltstfunc='',cltdofunc='',itypea1='',itypea2='',itypea3='',seqnum='',calc1='100+25*lvl',ToHitCalc='200+20*lvl',finishing='1',prgchargestocast='0',prgchargesconsumed='1',SrcDam='128')
  levels={};stateids={};local_auras=[]
  for sid in NAMES:
   h=clone(old[0],'Workshop Assassin Level '+str(sid));h['calc4']="skill('"+by[sid]['skill']+"'.lvl)";levels[sid]=int(h['*Id'])
  for sid in BUFFS:
   st=state('workshop_assassin_'+str(sid));stateids[sid]=int(st['*ID']);r=replace(sid,old[278])
   for k in r:
    if k.startswith(('aurastat','auraevent','passivestat','passivecalc','prg')):r[k]=''
   r.update(srvstfunc='',srvdofunc='18',cltstfunc='',cltdofunc='',aurastate=st['state'],auratargetstate='',auralencalc='50',aura='',periodic='',periodicClearAura='',passivestate='',progressive='',finishing='',anim='SC',seqtrans='SC',range='none',TargetableOnly='',TargetCorpse='',TargetPet='',TgtPlaceCheck='',SearchEnemyXY='',SearchOpenXY='',startmana='0',mana='0',lvlmana='0',minmana='0',localdelay='0',globaldelay='0',InTown='1')
   st['skill']=r['skill'];local_auras.append((sid,65,stateids[sid],levels[sid],15,3))
  expose=replace(265,old[263]);expose.update(passivestate='weaponblock',passivestat1='',passivecalc1='',passive='',srvstfunc='',srvdofunc='',cltstfunc='',cltdofunc='')
  # Cast-location marker uses the native missile caster, avoiding guessed path offsets.
  for sid in (256,266,275):
   r=replace(sid,old[47] if sid!=275 else old[275])
   r.update(srvstfunc='',srvdofunc='18' if sid==275 else '1',cltstfunc='',cltdofunc='',srvmissile='',srvmissilea='',cltmissile='',cltmissilea='',anim='SC',seqtrans='SC',seqnum='',range='none',TargetableOnly='',TgtPlaceCheck='',SearchEnemyXY='',SearchOpenXY='',aurastate='',auratargetstate='',progressive='',finishing='1' if sid==275 else '',mana='15' if sid==275 else '8',lvlmana='0',minmana='1',startmana='0',localdelay='250' if sid==275 else '0',globaldelay='0',SrcDam='0')
   if sid==275:r.update(prgchargestocast='0',prgchargesconsumed='1')
  spawnids={}
  for sid,source in ((256,'shock field in air'),(266,'blade fury')):
   # Native cast creates a harmless target marker; runtime places the sentry there.
   m=missile('firebolt','workshop_assassin_place_'+str(sid),Skill='',pSrvDoFunc='1',pSrvHitFunc='',pCltDoFunc='1',pCltHitFunc='',Range='1',Vel='0',MaxVel='0',CollideType='0',ClientCol='0',SrcDamage='0',EType='',MinDamage='0',MaxDamage='0',EMin='0',EMax='0',CelFile='null',ClientSend='0')
   by[sid].update(srvmissile=m['Missile'],cltmissile=m['Missile']);spawnids[sid]=int(m['*ID']);visuals[m['Missile']]='null'
  strike=clone(old[0],'Workshop Assassin Weapon Hit');strike.update(SrcDam='128',MinDam='0',MaxDam='0',EType='',calc1='0')
  hit=missile('workshop_eruption_hit','workshop_assassin_hit',Skill=strike['skill'],SrcDamage='128',ToHit='1')
  visualids={}; visualskills={}
  for name,source in (('Queen','throwingstar'),('ATMG','throwingstar'),('Spikes','sentryspikeonground'),('Gravity','tornado'),('Afterimage','curseamplifydamage'),('DeathMark','corpseexplosion')):
   if not any(x['Missile']==source for x in missiles):source='nova'
   m=missile(source,'workshop_assassin_visual_'+name.lower(),Skill='',pSrvDoFunc='1',pSrvHitFunc='',pSrvDmgFunc='',pCltDoFunc='1',pCltHitFunc='',Vel='0',MaxVel='0',Accel='0',Range='12',CollideType='0',ClientCol='0',SrcDamage='0',EType='',MinDamage='0',MaxDamage='0',EMin='0',EMax='0',ClientSend='1')
   for k in m:
    if k.startswith(('MinLevDam','MaxLevDam','MinELev','MaxELev','SubMissile','CltSubMissile','HitSubMissile','CltHitSubMissile')):m[k]=''
   if name in ('Queen','ATMG'):m.update(Vel='22',MaxVel='22',Range='18',LoopAnim='1')
   if name in ('Queen','ATMG'):m.update(CollideType='3',ClientCol='1',CollideKill='1',Pierce='1')
   if name=='Spikes':m.update(Range='75',LoopAnim='1')
   visualids[name]=int(m['*ID'])
   # Native triggered casts send the client skill event. Direct server missile
   # creation alone did not render these effects in the player's HD client.
   effect=clone(old[36],'Workshop Assassin '+name+' Visual Cast')
   effect.update(srvmissile=m['Missile'],cltmissile=m['Missile'],srvmissilea='',cltmissilea='',srvstfunc='',srvdofunc='1',cltstfunc='',cltdofunc='1',SrcDam='0',EType='',MinDam='0',MaxDam='0',restrict='1',InTown='1')
   for k in effect:
    if k.startswith(('MinLevDam','MaxLevDam','EMin','EMax')):effect[k]='0'
   visualskills[name+'VisualSkill']=int(effect['*Id'])
  # Native sentry summons participate in the same assassintrap FIFO as all
  # existing sentries. Runtime owns only their custom attacks and lifetime.
  dummy=clone(old[0],'Workshop Sentry Idle');dummy.update(srvdofunc='',srvstfunc='',cltdofunc='',calc4='100000',anim='S1')
  classes={}
  for sid,name,source in ((256,'workshop_gravity_trap','chargeboltsentry'),(266,'workshop_atmg_sentry','lightningsentry')):
   mon=next(x for x in monsters if x['Id']==source).copy();cid=max(int(x['*hcIdx']) for x in monsters if x['*hcIdx'].isdigit())+1
   mon.update(Id=name,BaseId=name,**{'*hcIdx':str(cid),'NameStr':'WorkshopAssassin'+str(sid)+'Name','neverCount':'1','AI':'','Velocity':'0','Run':'0'})
   for k in mon:
    if k.startswith(('Skill','Sk')):mon[k]=''
   monsters.append(mon);classes[sid]=cid
   row=replace(sid,old[271]);row.update(summon=name,pettype='assassintrap',petmax=old[271]['petmax'],summode='NU',sumskill1=dummy['skill'],sumsk1calc='lvl',mana='8',lvlmana='0',minmana='1',localdelay='0',globaldelay='0')
   for n in range(2,6):row['sumskill'+str(n)]='';row['sumsk'+str(n)+'calc']=''
  echo_pet=next(x for x in pets if x['pet type']=='shadowwarrior').copy();echo_pet.update({'pet type':'workshop_afterimage','name':'WorkshopAssassin263Name','group':'','baseicon':''});pets.append(echo_pet)
  echo_mon=next(x for x in monsters if x['Id']=='shadowwarrior').copy();echo_cid=max(int(x['*hcIdx']) for x in monsters if x['*hcIdx'].isdigit())+1
  echo_mon.update(Id='workshop_afterimage',BaseId='workshop_afterimage',AI='',**{'*hcIdx':str(echo_cid),'NameStr':'WorkshopAssassin263Name','neverCount':'1','Velocity':'0','Run':'0'})
  for k in echo_mon:
   if k.startswith(('Skill','Sk')):echo_mon[k]=''
  monsters.append(echo_mon)
  echo_summon=clone(old[268],'Workshop Afterimage Clone');echo_summon.update(summon='workshop_afterimage',pettype='workshop_afterimage',petmax='16',summode='NU')
  for k in echo_summon:
   if k.startswith(('sumskill','sumsk')):echo_summon[k]=''
  speed_state=state('workshop_sentry_overclock')
  slow=state('workshop_gravity_slow');slow.update(aura='',curse='1')
  reserve="max(1,(15+3*lvl)*100/(100+stat('workshop_reservation_efficiency'.accr)))"
  definitions={
   263:('Reserved buff. Successful attacks create a delayed weapon echo.', [('Echo weapon damage: %d%%','30+3*lvl')]),
   278:('Reserved buff. Hits mark enemies for 5 seconds. Stored damage explodes on expiry or death.', [('Stored damage released: %d%%','100'),('Mark duration: %d seconds','5'),('Explosion radius: %d yards','6')]),
   276:('Reserved buff. Sentries fire faster and consume their shots faster.', [('Sentry speed: +%d%%','25+2*lvl')]),
   257:('Reserved buff. Continuously leaves stacking spikes, even while standing still, for 3 seconds. Each patch hits once per second.', [('Weapon damage per pulse: %d%%','25+5*lvl'),('Attack rating: +%d%%','100+10*lvl'),('Patch radius: %d yards','2')]),
   277:('Reserved buff. Fires blades at nearby enemies once per second. Hits can trigger effects.', [('Weapon damage per blade: %d%%','30+5*lvl'),('Attack rating: +%d%%','100+10*lvl'),('Targets per volley: %d','min(8,3+lvl/10)'),('Radius: %d yards','8')]),
   265:('Passive. Successful hits critically strike enemies at full life.', [('Critical damage bonus: +%d%%','10+lvl')]),
   256:('Every 3 seconds, pulls nearby enemies inward and curses them with slow. Boss movement may resist the pull.', [('Slow: %d%%','min(60,20+lvl)'),('Radius: %d yards','6'),('Duration: %d seconds','8'),('Maximum traps: %d','5'),('Mana cost: %d','8')]),
   266:('Sentry fires eight times per second. Successful shots can trigger on-hit effects.', [('Weapon damage per shot: %d%%','10+lvl'),('Attack rating: +%d%%','100+10*lvl'),('Shots per sentry: %d','80'),('Maximum sentries: %d','5'),('Mana cost: %d','8')]),
   275:('Finishing move. Short-lived shadows strike enemies with 2-yard splash. Shadows may strike the same enemy repeatedly.', [('Weapon damage per hit: %d%%','clc4'),('Attack rating: +%d%%','200+20*lvl'),('Number of shadow strikes: %d','10+lvl/5'),('Cooldown: %d seconds','10'),('Mana cost: %d','15')])}
  for sid,(description,lines) in definitions.items():
   if sid in BUFFS:lines=lines+[('Mana reserved: %d',reserve)]
   label(sid,description,lines)
   if sid in (263,257,277,266,275):
    formula={263:'30+3*lvl',257:'25+5*lvl',277:'30+5*lvl',266:'10+lvl',275:'200+25*lvl'}[sid]
    ds[by[sid]['skilldesc']].update(descdam='7',**{'ddam calc1':'('+formula+')-100','ddam calc2':'0','descatt':'1'});by[sid]['ToHitCalc']='200+20*lvl' if sid==275 else '100+10*lvl'
  # The native strike scales the entire weapon packet, after ordinary weapon
  # bonuses. Display 25 applies the same final percentage to both endpoints;
  # display 7 would add skill ED instead and omit scaling elemental damage.
  by[275].update(SrcDam='128',calc1='0',calc4=OMNISLASH_DAMAGE,MinDam='0',MaxDam='0',EType='',EMin='0',EMax='0')
  for k in by[275]:
   if k.startswith(('MinLevDam','MaxLevDam','EMinLev','EMaxLev')):by[275][k]='0'
  omni=ds[by[275]['skilldesc']]
  labels['WorkshopOmnislashHit']='Omnislash (per hit)'
  labels['WorkshopOmnislashSynergy']='Expose Weakness: +%d%% damage per base level'
  omni.update({'str alt':'WorkshopOmnislashHit','descdam':'25','ddam calc1':'clc4-100','ddam calc2':'clc4-100','dsc3line1':'36','dsc3texta1':'WorkshopOmnislashSynergy','dsc3calca1':'3'})
  # Prerequisites follow the original POSITION, matching the native tree arrows.
  for sid in NAMES:
   for field in ('reqskill1','reqskill2','reqskill3'):by[sid][field]=old[sid][field]
  local={'ATMGClass':classes[266],'GravityClass':classes[256],'EchoClass':echo_cid,'EchoSummon':int(echo_summon['*Id']),'SpeedState':int(speed_state['*ID']),'Blink':int(blink['*Id']),'Finisher':int(finisher['*Id']),'HitSkill':int(strike['*Id']),'HitMissile':int(hit['*ID']),'GravitySlow':int(slow['*ID']),'GravityPlace':spawnids[256],'ATMGPlace':spawnids[266],**{k+'Visual':v for k,v in visualids.items()},**visualskills}
  if bindings:assert bindings==local and auras==local_auras
  bindings=local;auras=local_auras
  for name,h,rows in [('skills',sh,skills),('skilldesc',dh,descs),('missiles',mh,missiles),('states',th,states),('monstats',nh,monsters),('pettype',ph,pets)]:write(p/(name+'.txt'),h,rows)
 strings=root/'data/local/lng/strings/skills.json';rows=json.loads(strings.read_text(encoding='utf-8-sig'));known={r['Key']:r for r in rows};last=max(r['id'] for f in strings.parent.glob('*.json') for r in json.loads(f.read_text(encoding='utf-8-sig')) if 'id' in r)
 for key,text in labels.items():
  if key in known:
   for lang in known[key]:
    if lang not in ('id','Key'):known[key][lang]=text
  else:last+=1;rows.append({'id':last,'Key':key,**{lang:text for lang in rows[0] if lang not in ('id','Key')}})
 strings.write_text(json.dumps(rows,ensure_ascii=False,indent=2),encoding='utf8')
 registry=root/'data/hd/missiles/missiles.json';mapping=json.loads(registry.read_text(encoding='utf-8-sig'))
 for name,source in visuals.items():mapping[name]='null' if source=='null' else mapping.get(source.replace(' ','_'),mapping.get(source,mapping['nova']))
 mapping['workshop_assassin_hit']='null';registry.write_text(json.dumps(mapping,indent=4),encoding='utf8')
 registry=root/'data/hd/character/monsters.json';mapping=json.loads(registry.read_text(encoding='utf-8-sig'))
 for name,source in (('workshop_gravity_trap','chargeboltsentry'),('workshop_atmg_sentry','lightningsentry'),('workshop_afterimage','shadowwarrior')):mapping[name]=mapping[source]
 registry.write_text(json.dumps(mapping,indent=4),encoding='utf8')
 aura_file=HERE/'combat/aura_bindings.h';s=aura_file.read_text();s=s.replace('\n};',',\n'+',\n'.join('{'+','.join(map(str,d))+'}' for d in auras)+'\n};',1);aura_file.write_text(s)
 header='// Generated Assassin native IDs; do not hand-edit.\n'+''.join(f'inline constexpr int {k}={v};\n' for k,v in bindings.items())
 header+='inline constexpr unsigned NativeSentryClasses[]={'+','.join(x['*hcIdx'] for x in monsters if x.get('AI') in ('AssassinSentry','DeathSentry') and x['Id'] not in ('deathsentry',))+'};\n'
 header+='inline constexpr int Levels[][2]={'+','.join('{'+str(k)+','+str(v)+'}' for k,v in levels.items())+'};\n'
 (HERE/'combat/assassin_bindings.h').write_text(header);(HERE/'ASSASSIN.json').write_text(json.dumps({'bindings':bindings,'levels':levels,'states':stateids,'skills':NAMES},indent=2))
 print('Assassin slots, five reservations, native Finisher, tooltip formulas and private helpers generated.')
