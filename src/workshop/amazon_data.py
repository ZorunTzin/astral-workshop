"""Amazon rework; stable public IDs, private missiles, shared reservation system."""
import json
from pathlib import Path
from combat.port import read,write
HERE=Path(__file__).resolve().parent
IDS=(6,7,8,11,12,14,17,18,19,20,22,24,25,28,30,34)
BUFFS=(8,17,18,28)
def install(root):
 root=Path(root);labels={};binding=None
 for directory in ('data/global/excel','data/global/excel/base'):
  p=root/directory;sh,skills=read(p/'skills.txt');dh,descs=read(p/'skilldesc.txt');mh,missiles=read(p/'missiles.txt');th,states=read(p/'states.txt')
  by={int(r['*Id']):r for r in skills if r['*Id'].isdigit()};old={i:r.copy() for i,r in by.items()};ds={r['skilldesc']:r for r in descs};mm={r['Missile']:r for r in missiles};local={};visual={};auras=[]
  def skill(source,name):
   r=source.copy();r.update(skill=name,charclass='',skilldesc='',**{'*Id':str(max(by)+1),'reqlevel':'1','reqskill1':'','reqskill2':'','reqskill3':'','mana':'0','lvlmana':'0','startmana':'0','minmana':'0','localdelay':'0','globaldelay':'0','ItemEffect':'1'});skills.append(r);by[int(r['*Id'])]=r;return r
  def missile(source,name,**kw):
   r=mm[source].copy();r.update(Missile=name,**{'*ID':str(max(int(x['*ID']) for x in missiles if x['*ID'].isdigit())+1)});r.update(kw);missiles.append(r);mm[name]=r;visual[name]=source;return r
  def line(sid,text,formula,section='dsc2'):
   d=ds[by[sid]['skilldesc']];n=next(i for i in range(1,5 if section=='dsc2' else 8) if not d.get(section+'line'+str(i)) or d[section+'line'+str(i)]=='0');key=f'WorkshopAmazon{sid}_{section}{n}';labels[key]=text;d.update({section+'line'+str(n):'36',section+'texta'+str(n):key,section+'calca'+str(n):formula,section+'calcb'+str(n):''})
  def description(sid,name,text):
   d=ds[by[sid]['skilldesc']];key='WorkshopAmazon'+str(sid);labels[key+'Name']=name;labels[key+'Desc']=text;d.update({'str name':key+'Name','str alt':key+'Name','str short':key+'Desc','str long':key+'Desc'})
  for sid in BUFFS:
   r=by[sid];st=next(x for x in states if x['state']=='enchant').copy();st.update(state='workshop_amazon_'+str(sid),**{'*ID':str(max(int(x['*ID']) for x in states if x['*ID'].isdigit())+1),'group':'','skill':r['skill'],'stat':'','setfunc':'','itemtype':'','itemtrans':'','castoverlay':'','overlay1':'','aura':'1','curse':''});states.append(st)
   helper=skill(old[0],'Workshop Amazon Level '+str(sid));helper['calc4']="skill('"+r['skill']+"'.lvl)"
   # The offensive debuffs retain their native radius/defense/slow calculations.
   # Covenants move their positive passive stats into a temporary self aura.
   for i in range(1,7):
    if sid in (18,28):r['aurastat'+str(i)]='';r['aurastatcalc'+str(i)]=''
   if sid in (18,28):
    for i in range(1,5):
     r['aurastat'+str(i)]=old[sid].get('passivestat'+str(i),'');r['aurastatcalc'+str(i)]=old[sid].get('passivecalc'+str(i),'').replace('pst1','ast1')
   for k in r:
    if k.startswith(('passivestat','passivecalc')):r[k]=''
   r.update(srvstfunc='',srvdofunc='18',cltstfunc='',cltdofunc='',passivestate='',passive='',aurastate=st['state'],auralencalc='50',periodic='',periodicClearAura='',aura='',anim='SC',seqtrans='SC',seqnum='',mana='0',lvlmana='0',startmana='0',minmana='0',localdelay='0',globaldelay='0')
   auras.append((sid,6 if sid in (8,17) else 65,int(st['*ID']),int(helper['*Id']),15,3))
   d=ds[r['skilldesc']]
   for k in d:
    if k.startswith(('descline','desctext','desccalc','dsc2','dsc3')):d[k]=''
   description(sid,old[sid]['skill'],'Reserved buff. Toggle on or off. '+('Continuously affects nearby enemies.' if sid in (8,17) else 'Positive covenant effects only; no penalties.'))
   lines={8:[('Enemy defense: %d','ast1'),('Enemy physical resistance: %d%%','ast2')],17:[('Enemy missile speed: %d%%','ast1'),('Enemy missile damage: %d%%','ast2')],18:[('Enemy elemental resistance: -%d%%','ast1')],28:[('Physical damage: +%d%%','ast1'),('Crushing blow: %d%%','ast3')]}
   for n,(text,formula) in enumerate(lines[sid]+[('Mana reserved: %d',"max(1,(15+3*lvl)*100/(100+stat('workshop_reservation_efficiency'.accr)))")],1):
    key=f'WorkshopAmazon{sid}Value{n}';labels[key]=text;d.update({'descline'+str(n):'36','desctexta'+str(n):key,'desccalca'+str(n):formula})
   if sid in (8,17):line(sid,'Radius: %d yards','18')
  # Explosions reuse the native impact model without altering shared missiles.
  for sid,source,etype in ((6,'magicarrow','mag'),(7,'firearrow','fire'),(11,'coldarrow','cold')):
   blast=missile('explodingarrowexp2',f'workshop_amazon_blast_{sid}',Skill=by[sid]['skill'],sHitPar1='5',EType=etype,SrcDamage='128',ClientSend='1')
   m=missile(source,f'workshop_amazon_arrow_{sid}',pSrvHitFunc='4',sHitPar1='1',HitSubMissile1=blast['Missile'],Pierce='0',pCltHitFunc='1',cHitPar1='5',cHitPar2='2',CltHitSubMissile1='fireexplosion2' if sid==7 else 'iceexplode' if sid==11 else 'teethexplode',ClientSend='1')
   by[sid].update(srvmissilea=m['Missile'],srvmissileb=m['Missile']);line(sid,'Impact radius: %d yards','3')
   local['Blast'+str(sid)]=int(blast['*ID']);local['Arrow'+str(sid)]=int(m['*ID'])
  by[12]['ToHitCalc']='100+5*lvl';line(12,'Attack rating: +%d%%','toht');line(12,'Returning arrow damage: %d%%','50')
  # Explicit native volley publication owns these arrows. Room resync must
  # not publish the same unit again with a second visual start.
  for key,source in (('MultiArrow','multipleshotarrow'),('MultiBolt','multipleshotbolt')):
   m=missile(source,'workshop_amazon_'+key.lower(),ClientSend='0');local[key]=int(m['*ID']);by[12]['srvmissilea' if key=='MultiArrow' else 'srvmissileb']=m['Missile'];by[12]['cltmissilea' if key=='MultiArrow' else 'cltmissileb']=m['Missile']
  m=missile('multipleshotarrow','workshop_amazon_return',ClientSend='0',Pierce='0',CollideKill='1');local['Return']=int(m['*ID'])
  for generation in range(4):
   m=missile('arrow','workshop_amazon_split'+str(generation),Pierce='0',AlwaysExplode='0',pSrvHitFunc='',pCltHitFunc='',ClientSend='0',Range='80',NextHit='0',NextDelay='0');local['Split'+str(generation)]=int(m['*ID'])
   # Keep Guided Arrow steering, but use the proven ordinary-arrow art path
   # for the initial shot and every child (HD and legacy rendering).
   for field in ('CelFile','Trans','LoopAnim','AnimLen','AnimSpeed','animrate','NumDirections'):
    m[field]=mm['multipleshotarrow'][field]
   visual[m['Missile']]='arrow'
  # Native bow attack owns the first arrow and its predicted HD visual.
  # Keep the former Guided Arrow damage formula separately from fan count.
  by[22].update(srvstfunc='4',srvdofunc='8',cltstfunc='11',cltdofunc='17',
   calc4=old[22]['calc1'],calc1='1',calc2='1',calc3='1',
   srvmissilea='workshop_amazon_split0',srvmissileb='workshop_amazon_split0',srvmissilec='',
   cltmissilea='workshop_amazon_split0',cltmissileb='workshop_amazon_split0',cltmissilec='')
  ds[by[22]['skilldesc']]['desccalca2']='clc4'
  # These arrows are published from their initialized server units. Running
  # the native client prediction as well creates a separate outgoing wave.
  # Keep the attack animation, but use one authoritative missile visual path.
  for sid in (12,):
   by[sid]['cltdofunc']=''
   for suffix in ('a','b','c'):by[sid]['cltmissile'+suffix]=''
  helper=skill(by[22],'Workshop Split Arrow Child');helper['calc1']='0';local['SplitHelper']=int(helper['*Id']) # Retain ID compatibility; children use direct arrow creation.
  description(22,'Split Arrow','Impact creates two seeking arrows. Three split generations: 1 -> 2 -> 4 -> 8.');line(22,'Split generations: %d','3')
  # The Amazon has no Barbarian leap sequence. Use the verified teleport
  # helper, then resolve the physical landing payload in the native bridge.
  r=by[19];r.update(srvstfunc='',srvdofunc='18',cltstfunc='',cltdofunc='',anim='SC',seqtrans='SC',seqnum='',range='none',TargetableOnly='',SearchEnemyXY='',srvmissile='',cltmissile='',auratargetstate='',aurastate='',auralencalc='',calc2='0',calc3='0')
  for k in r:
   if k.startswith(('aurastat','aurastatcalc')):r[k]=''
  r['calc4']='100+clc1'
  helper=skill(old[0],'Workshop Pounce Level');helper['calc4']="skill('Impale'.lvl)";local['PounceLevel']=int(helper['*Id'])
  description(19,'Pounce','Teleport onto an enemy and release a physical weapon-damage nova on landing. Ground targeting searches for the nearest enemy within 12 yards.');line(19,'Nova radius: %d yards','6')
  hit=missile('workshop_assassin_hit','workshop_pounce_hit',Skill=r['skill'],SrcDamage='128',EType='',ClientSend='0');local['PounceHit']=int(hit['*ID'])
  ring=missile('warcry','workshop_pounce_ring',Skill='',SrcDamage='0',EType='',CollideType='0',pSrvHitFunc='',pSrvDmgFunc='',ClientSend='1',Range='10');local['PounceRing']=int(ring['*ID'])
  ring.update(pCltHitFunc='',CltHitSubMissile1='',CltSubMissile1='',HitSubMissile1='',ExplosionMissile='',ClientCol='0')
  effect=skill(old[48],'Workshop Pounce Visual')
  effect.update(srvmissilea=ring['Missile'],srvmissileb=ring['Missile'],srvmissilec=ring['Missile'],cltmissilea=ring['Missile'],cltmissileb=ring['Missile'],cltmissilec=ring['Missile'],EType='',EMin='0',EMax='0',SrcDam='0',restrict='1',InTown='1')
  local['PounceVisual']=int(effect['*Id'])
  ds[by[19]['skilldesc']]['descline1']=''
  ds[by[20]['skilldesc']].update(dsc3calcb5='5',dsc3calcb6='5')
  labels['skillname19']='Pounce';labels['skillan19']='Pounce';labels['skillname22']='Split Arrow';labels['skillan22']='Split Arrow'
  by[20].update(calc1="1+skill('Lightning Fury'.blvl)/5+skill('Charged Strike'.blvl)/5",Param2='5')
  by[25].update(srvdofunc='8',cltdofunc='17',srvmissile='',cltmissile='',srvmissilea='plaguejavelin',srvmissileb='plaguejavelin',cltmissilea='plaguejavelin',cltmissileb='plaguejavelin',calc1='1+lvl/5',calc2='1',calc3='1',localdelay='0',globaldelay='0');line(25,'Javelins: %d','clc1')
  ds[by[25]['skilldesc']]['dsc2line1']=''
  by[30].update(aurastat3='',aurastatcalc3='')
  by[24]['calc1']='3+lvl/5'
  bolt=missile('chargedstrikebolt','workshop_charged_bolt',ClientSend='1',pSrvHitFunc='5',sHitPar1='16',HitSubMissile1='workshop_charged_nova',pCltHitFunc='4',cHitPar1='16',CltHitSubMissile1='workshop_charged_nova')
  nova=missile('nova','workshop_charged_nova',Skill=by[24]['skill'],SrcDamage='0',ClientSend='1',Range='7',NextHit='1',NextDelay='4');local['ChargedBolt']=int(bolt['*ID'])
  by[24].update(srvmissilea=bolt['Missile'],srvmissileb=bolt['Missile'],srvmissilec=bolt['Missile'],cltmissilea=bolt['Missile'],cltmissileb=bolt['Missile'],cltmissilec=bolt['Missile']);line(24,'Bolts in a circle: %d','clc1')
  line(34,'Chain lightning releases: %d','1+lvl/10');local['LightningStrike']=int(mm['lightningstrike']['*ID'])
  by[14].update(calc1='8+lvl',srvmissilea='workshop_toxic_prime',srvmissileb='workshop_toxic_prime',srvmissilec='workshop_toxic_prime',cltmissilea='workshop_toxic_prime',cltmissileb='workshop_toxic_prime',cltmissilec='workshop_toxic_prime')
  toxic=missile('toxic_whirl_prime','workshop_toxic_prime',ClientSend='1',Range='16',VelLev='6',MaxVel='255');local['Toxic']=int(toxic['*ID']);line(14,'Poison projectiles: %d','8+lvl');line(14,'Projectile speed: +%d%%','5*lvl')
  description(12,'Multiple Shot','Fires a spread of arrows. Arrows return toward you at the end of their flight for half damage.')
  description(24,'Charged Strike','Melee strike releases charged bolts in a circle. Each bolt explodes in a lightning nova on impact.')
  description(30,'Fend','Strikes surrounding enemies without disabling block.')
  for name,h,rows in [('skills',sh,skills),('skilldesc',dh,descs),('missiles',mh,missiles),('states',th,states)]:write(p/(name+'.txt'),h,rows)
  if binding:assert binding==local
  binding=local
 # A standalone mesh follows the actual missile transform. Native arrow HD
 # definitions can depend on launch/tracking state absent on private homing
 # missiles, although their legacy sprite and gameplay remain valid.
 (root/'data/hd/missiles/workshop_split_arrow.json').write_text((HERE/'combat/split_arrow_hd.json').read_text(encoding='utf-8'),encoding='utf-8')
 assets_file=HERE/'combat/assets.json'
 assets=json.loads(assets_file.read_text())
 asset='data/hd/missiles/workshop_split_arrow.json'
 if asset not in assets:assets.append(asset)
 assets_file.write_text(json.dumps(assets,indent=2))
 # Reuse native HD art for each private missile.
 p=root/'data/hd/missiles/missiles.json';mapping=json.loads(p.read_text(encoding='utf-8-sig'))
 for name,source in visual.items():mapping[name]=source if source=='workshop_split_arrow' else mapping.get(source,mapping.get(source.replace(' ','_'),'null'))
 p.write_text(json.dumps(mapping,indent=2),encoding='utf8')
 p=root/'data/local/lng/strings/skills.json';rows=json.loads(p.read_text(encoding='utf-8-sig'));lookup={r['Key']:r for r in rows};last=max(r['id'] for f in p.parent.glob('*.json') for r in json.loads(f.read_text(encoding='utf-8-sig')) if 'id' in r)
 for key,value in labels.items():
  if key not in lookup:last+=1;lookup[key]={'id':last,'Key':key,**{k:value for k in rows[0] if k not in ('id','Key')}};rows.append(lookup[key])
  else:
   for lang in lookup[key]:
    if lang not in ('id','Key'):lookup[key][lang]=value
 p.write_text(json.dumps(rows,ensure_ascii=False,indent=2),encoding='utf8')
 p=HERE/'combat/aura_bindings.h';p.write_text(p.read_text().replace('\n};',',\n'+',\n'.join('{'+','.join(map(str,a))+'}' for a in auras)+'\n};',1))
 (HERE/'combat/amazon_bindings.h').write_text('// Generated private Amazon missile IDs.\n'+''.join(f'inline constexpr int {k}={v};\n' for k,v in binding.items()))
 (HERE/'AMAZON.json').write_text(json.dumps({'bindings':binding,'auras':auras},indent=2))
 print('Amazon: four reservations, penalty-free covenants, private arrows, Pounce and elemental javelins generated.')
