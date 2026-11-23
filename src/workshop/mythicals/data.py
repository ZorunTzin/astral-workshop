"""Trial-exclusive ESR adaptations. No source installation is required after import."""
from pathlib import Path
import json,re,sys,shutil
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from combat.port import read,write
HERE=Path(__file__).resolve().parent
SOURCE=json.loads((HERE/'source.json').read_text(encoding='utf8'))
# Keep retired IDs valid for existing saves, but omit them from future drops.
SCREAMS_KEEP={59,62,64,67,69,74,79}
ACTIVE=[r for r in SOURCE if r['_ordinal']<46 or r['_ordinal'] in SCREAMS_KEEP]
# Engine-compatible base classes; imported art is independent of the base animation.
BASE={'m01':'6l7','m02':'6hx','m03':'7gm','m04':'7gw','m05':'7s8','m06':'utp','m07':'obf','m08':'7bk','m09':'7cr','m10':'ulc','m11':'6ws','m12':'7ws','m13':'7qr','m14':'7dg','m15':'drf','m16':'7sb','m17':'7sc','m20':'ci3','mam':'amu','mrn':'rin','m29':'6cs','m30':'6lw','m31':'amf','m32':'7ts','m33':'7ja','m34':'6bs','m35':'7gd','m36':'6bs','m37':'7fb','m38':'7b8','m39':'7tk','m60':'utp','m61':'7cr'}
ALIASES={'strpercent':'str%','dexpercent':'dex%','vitpercent':'vit%','enepercent':'enr%','all-stats-percent':'stats%','extra-mag2':'extra-mag','extra-summ':'ws-pet-damage','passive-summ-life':'ws-pet-life','curse-res':'res-curse-len','velocitypercent':'move2','dodge':'dae','crit-dmg':'deadly','extra-wind':'dmg2%','pierce-res-all':'pierce-elem','pierce-ele-res':'pierce-elem','spell-elemental-damage':'extra-elem','res-elem':'res-all','abs-elem%':'abs-all%','life-on-striking':'healperhit','mana-on-striking':'mana-kill','item-skill-cooldown':'cast2','proj-speed':'cast2','passive-summ-phys-resist':'ws-pet-life','passive-summ-elemental-resist':'ws-pet-life'}
SKILLS={'Cloak of Shadows':'Workshop Mythical Shadow Veil','Fire Enhanced Arrow':'Exploding Arrow','Cold Enhanced Arrow':'Freezing Arrow','Lightning Enhanced Arrow':'Lightning Bolt','Blizzard Cannon':'Glacial Spike','Cold Flash':'Frost Nova','Judgement':'Fist of the Heavens','Mjolnir Chaos Lightning':'Chain Lightning','Hydra Baal\'s Vile Whisper':'Hydra','Monkey King Leap':'Leap','Monkey King Slam':'Ground Stomp','Blades of Ice Discharge':'Glacial Spike','Fists of Fire Discharge':'Fire Ball','Claws of Thunder Discharge':'Nova','Execute Discharge':'Royal Strike','Blink Strike':'Dragon Flight','Triumvirate Orb':'Kings Trinity','Lightning Flash':'Nova','Rain of Stars':'Starfall','Rupture of Stars':'Comet','Blood Empowerment':'Workshop Mythical Blood Empowerment','Bloody Spear':'Bone Spear','Celestial Empowerment':'Workshop Mythical Celestial Empowerment','Flameburst':'Fire Ball','Frozen Blast':'Glacial Spike','Time Lapse':'Workshop Mythical Time Ward','Time Stop':'Frost Nova','Holy Bolt Weapon Damage':'Holy Bolt','Blessed Hammer Weapon Damage':'Blessed Hammer','Fist of the Heavens Weapon Damage':'Fist of the Heavens','Travel to the Underworld':'Poison Nova','Underworld Empowerment':'Workshop Mythical Underworld Empowerment','Shadow Descent':'Fade','Dragon Blade':'Dragon Claw','Spectral Maim':'Amplify Damage','Axe Proc':'Workshop Madawc Double Throw','Magic Enhanced Axe':'Bone Spear','Mythical Decrepify':'Decrepify','Ice Fangs':'Blizzard','Cold Blast':'Glacial Spike','Cold Surge':'Frost Nova','Hypothermia':'Hurricane','Energy Shield Aura':'Energy Shield','Elemental Fury':'Vengeance','Elemental Nova':'Nova','Scosglen Resonance':'Conviction','Reap Souls':'Corpse Explosion'}
TROPHIES=[('Speed',[('velocitypercent',35),('attackrate',35),('item_fastercastrate',35)]),('Physical Damage',[('damagepercent',100)]),('Spell Damage',[('passive_fire_mastery',50),('passive_ltng_mastery',50),('passive_cold_mastery',50),('passive_pois_mastery',50),('passive_mag_mastery',50)]),('Sustenance',[('hpregen',100),('manarecoverybonus',100)]),('Resistances',[('fireresist',50),('coldresist',50),('lightresist',50),('poisonresist',50)]),('Stats',[('strength',75),('dexterity',75),('vitality',75),('energy',75)]),('All Skills',[('item_allskills',3)])]
for old,(name,_) in zip(['Speed','Physical Damage','Elemental Damage','Health','Resistances','Stats','Skills'],TROPHIES):SKILLS['Headhunter '+old]='Workshop Mythical Trophy of '+name
MARKERS={0:'Prismatic Edge',18:'Highlord Power',20:'Orion Power',27:'Tsukuyomi Power',42:'Voidforge Power',**{n:'Bond '+str(n-28) for n in range(28,35)}}
# Additional real powers replace unavailable ESR framework mechanics.
EXTRAS={3:[('extra_skelemages','',5,8)],4:[('oskill','Revive',12,18),('ws-pet-damage','',60,90)],5:[('gethit-skill','Cyclone Armor',5,40)],6:[('gethit-skill','Thunder Storm',25,40)],7:[('extra-elem','',40,60)],8:[('oskill','Hydra',12,18)],10:[('openwounds','',100,100)],11:[('ws-aura-effect','',25,35)],12:[],13:[('extra_skelewarriors','',3,5)],14:[('extra_spirit_wolves','',1,2),('extra_dire_wolves','',1,2)],15:[('openwounds','',100,100)],17:[('gethit-skill','Fire Ball',15,40)],19:[('oskill','Frozen Orb',10,20)],21:[('oskill','Lightning Fury',10,15)],23:[('dmg-demon','',200,300)],24:[],25:[('dmg-undead','',200,300)],26:[('extra-pois','',40,60)],37:[('extra-mag','',40,60)],38:[('dmg2%','',40,60)],35:[('pierce-mag','',20,30)],36:[('extra-cold','',20,35)],39:[('gethit-skill','Frost Nova',15,40)],40:[('gethit-skill','Nova',15,40)],41:[('res-pois','',80,100),('res-pois-len','',75,75)]}
def key(i):return 'WorkshopMythical%02d'%i
def code(i):return 'Y%02d'%i

def install(root):
 root=Path(root);assets=[];catalog=[];bind={};labels={}
 def text(k,v):labels[k]=v
 for folder in ('data/global/excel','data/global/excel/base'):
  d=root/folder;sh,skills=read(d/'skills.txt');sth,states=read(d/'states.txt');dh,descs=read(d/'skilldesc.txt');ph,props=read(d/'properties.txt')
  pmap={x['code']:x for x in props};smap={x['skill']:x for x in skills}
  stath,statrows=read(d/'itemstatcost.txt');statmap={r['Stat']:r for r in statrows}
  marker=next(r for r in statrows if r['*ID']=='258')
  if marker['Stat']!='ws_mythical_power':
   assert marker['Stat']=='item_crush_damage' and not marker['Save Bits'] and not marker['CSvBits']
   for f in d.glob('*.txt'):
    if f.name!='itemstatcost.txt':assert not re.search(r'\bitem_crush_damage\b',f.read_text(encoding='utf-8-sig')),f
  marker.clear();marker.update({k:'' for k in stath});marker.update(Stat='ws_mythical_power',**{'*ID':'258','Send Bits':'8','Send Param Bits':'6','Save Bits':'8','Save Param Bits':'6','Save Add':'0','Encode':'1','descfunc':'1','descval':'0','descpriority':'180','descstrpos':'WorkshopMythicalPower','descstrneg':'WorkshopMythicalPower','*eol':'0'})
  text('WorkshopMythicalPower','Mythical Power');statmap['ws_mythical_power']=marker
  write(d/'itemstatcost.txt',stath,statrows);assets.append(folder+'/itemstatcost.txt')
  mp={k:'' for k in ph};mp.update(code='ws-mythical-power',func1='22',stat1='ws_mythical_power',**{'*Enabled':'1','*eol':'0'})
  if mp['code'] in pmap:mp['*Id']=pmap[mp['code']]['*Id'];pmap[mp['code']].update(mp)
  else:mp['*Id']=str(max(int(r['*Id']) for r in props if r['*Id'].isdigit())+1);props.append(mp);pmap[mp['code']]=mp

  from mythicals.revision import display_stats
  display_stats(d,statrows,stath,props,ph,text)
  pmap={x['code']:x for x in props};statmap={x['Stat']:x for x in statrows}

  def alloc_state(name,template='battlecommand'):
   old=next((x for x in states if x['state']==name),None)
   if old:return int(old['*ID'])
   r=dict(next(x for x in states if x['state']==template));r['state']=name;r['*ID']=str(max(int(x['*ID']) for x in states if x['*ID'].isdigit())+1)
   for k in ('overlay1','overlay2','overlay3','overlay4','stat','setfunc','remfunc','group','colorpri','colorshift'):r[k]=''
   states.append(r);return int(r['*ID'])
  def skill(name,stats=None,seconds=20):
   old=smap.get(name);sid=old['*Id'] if old else str(max(int(x['*Id']) for x in skills if x['*Id'].isdigit())+1)
   r=dict(smap['Quickness']) if stats else {k:'' for k in sh}
   r.update(skill=name,**{'*Id':sid,'charclass':'','skilldesc':name,'reqlevel':'1','maxlvl':'1','InGame':'1','InTown':'1','leftskill':'0','rightskill':'0','mana':'0','minmana':'0','manashift':'8','passive':'0','*eol':'0'})
   if stats:
    st='wsmyth'+str(sid);alloc_state(st,'quickness');r.update(aurastate=st,auratargetstate='',auralencalc=str(seconds*25),srvstfunc='',srvdofunc='18',ItemEffect='18',ItemCltEffect='0',restrict='0',reqskill1='',reqskill2='',reqskill3='')
    for n in range(1,7):r['aurastat'+str(n)]=stats[n-1][0] if n<=len(stats) else '';r['aurastatcalc'+str(n)]=str(stats[n-1][1]) if n<=len(stats) else ''
   else:r.update(passive='1',immediate='1',enhanceable='0',range='none',monanim='xx')
   if old:old.clear();old.update(r)
   else:skills.append(r);smap[name]=r
   dr=dict(next(x for x in descs if x['skilldesc']=='quickness'));dr.update(SkillPage='0',SkillRow='0',SkillColumn='0',ListRow='0');dr.update(skilldesc=name,**{'str name':name,'str short':name,'str long':name,'str alt':name,'*eol':'0'})
   found=next((x for x in descs if x['skilldesc']==name),None)
   if found:found.update(dr)
   else:descs.append(dr)
   text(name,name.replace('Workshop Mythical ',''));return int(sid)
  for name,st in TROPHIES:skill('Workshop Mythical Trophy of '+name,st)
  for name,st in [('Blood Empowerment',[('damagepercent',150),('attackrate',30),('lifedrainmindam',20),('workshop_bleed_chance',100)]),('Celestial Empowerment',[('damagepercent',100),('item_maxhp_percent',20)]),('Underworld Empowerment',[('passive_pois_mastery',100),('passive_mag_mastery',100),('damagepercent',100)]),('Time Ward',[('damageresist',30),('magicresist',30)])]:skill('Workshop Mythical '+name,st,12 if 'Empowerment' in name else 5)
  from mythicals.revision import prepare_skills
  prepare_skills(skills,descs,text)
  smap={r['skill']:r for r in skills}
  # Retire only our never-deployed high-ID marker skill drafts. Numeric item
  # skill parameters have nine save bits; powers use an audited unused stat instead.
  retired={'Workshop Mythical '+name for name in MARKERS.values()}
  skills[:]=[r for r in skills if r['skill'] not in retired]
  descs[:]=[r for r in descs if r['skilldesc'] not in retired]
  for i in MARKERS:bind[str(i)]=i
  bind['markerStat']=258
  bind['bondState']=alloc_state('wsmythicalbonds')
  bind['RenewalState']=alloc_state('wsmythicalrenewal')
  # A compound property keeps all five speed bonuses within native 12-property limit.
  name='ws-myth-speeds';r={k:'' for k in ph};r.update(code=name,**{'*Id':str(max(int(x['*Id']) for x in props if x['*Id'].isdigit())+1),'*Enabled':'1','*eol':'0'})
  for n,st in enumerate(['item_fasterattackrate','item_fastercastrate','item_fastergethitrate','item_fasterblockrate','item_fastermovevelocity'],1):r.update({f'func{n}':'1' if n==1 else '3',f'stat{n}':st})
  if name in pmap:r['*Id']=pmap[name]['*Id'];pmap[name].update(r)
  else:props.append(r);pmap[name]=r
  ih,types=read(d/'itemtypes.txt');typeMap={r['Code']:r for r in types}
  if 'myth' not in typeMap:
   tag={k:'' for k in ih};tag.update(ItemType='Mythical Unique',Code='myth',TreasureClass='0',Rarity='0',**{'*eol':'0'});types.append(tag)
  uh,uniques=read(d/'uniqueitems.txt');tables={n:read(d/(n+'.txt')) for n in ['weapons','armor','misc']}
  for src in SOURCE:
   i=src['_ordinal'];base=src['_base'];table=base['table'];h,rows=tables[table];template=next(x for x in rows if x['code']==BASE[src['code']]);b=dict(template)
   b.update(code=code(i),name=key(i)+'Base',namestr=key(i)+'Base',normcode=code(i),ubercode=code(i),ultracode=code(i),spawnable='0',rarity='0',level='90',levelreq='90',**{'gamble cost':'0','magic lvl':'0','auto prefix':'','unique':'1','NightmareUpgrade':'xxx','HellUpgrade':'xxx','PermStoreItem':'0','diablocloneweight':'0'})
   for k in b:
    if re.match(r'^(Charsi|Gheed|Akara|Fara|Lysander|Drognan|Hratli|Alkor|Ormus|Elzix|Asheara|Cain|Halbu|Jamella|Larzuk|Anya|Malah)',k):b[k]='0'
   # Preserve target's supported combat animations/types; increase base damage/armor to ESR mythical values.
   for k in ['mindam','maxdam','2handmindam','2handmaxdam','minmisdam','maxmisdam','minac','maxac','reqstr','reqdex','speed','StrBonus','DexBonus','invwidth','invheight']:
    if k in h and base['row'].get(k):b[k]=base['row'][k]
   typename='ym%02d'%i;tr=dict(typeMap[template['type']]);tr.update(ItemType='Mythical '+template['name'],Code=typename,Equiv1=template['type'],Equiv2='myth',TreasureClass='0',Rarity='0',VarInvGfx='0')
   prior=next((r for r in types if r['Code']==typename),None)
   if prior:prior.update(tr)
   else:types.append(tr)
   b['type']=typename
   b['uniqueinvfile']='';b['setinvfile']='' if 'setinvfile' in h else b.get('setinvfile','')
   b={k:b.get(k,'') for k in h}
   old=next((x for x in rows if x['code']==code(i)),None)
   if old:old.update(b)
   else:rows.append(b)
   existing=next((x for x in uniques if x['index']==key(i)),None)
   uid=existing['*ID'] if existing else str(max(int(x['*ID']) for x in uniques if x['*ID'].isdigit())+1)
   u={k:'' for k in uh};u.update(index=key(i),**{'*ID':uid,'version':'100','spawnable':'1','rarity':'1','nolimit':'1','lvl':'1','lvl req':'90','code':code(i),'*ItemName':base['row']['name'],'cost mult':'12','cost add':'100000','chrtransform':'dpur','invtransform':'dpur','*eol':'0'})
   effects=[];changes=list(src.get('_adaptations',[]))
   for n in range(1,13):
    prop,par,lo,hi=[src.get(x+str(n),'') for x in ['prop','par','min','max']]
    if not prop:continue
    if prop in ['tinkerflag2','mythicaldesc']:continue
    if prop=='all-speeds':prop='ws-myth-speeds'
    if prop=='cast-skill':prop='kill-skill';changes.append('On-cast proc adapted to on-kill proc, usable by spellcasters.')
    if prop=='takedmg-skill':prop='gethit-skill'
    if prop=='aura':prop='oskill';changes.append('Equipped aura becomes an activatable all-class skill, consistent with Workshop aura rules.')
    if prop=='dmg-bleed':prop,par,lo,hi='openwounds','','100','100';changes.append('Bleeding replaced by 100% Open Wounds.')
    if prop in ALIASES:changes.append(prop+' -> '+ALIASES[prop]);prop=ALIASES[prop]
    if prop in ('oskill','skill','att-skill','hit-skill','gethit-skill','kill-skill'):
     if par in ['Poison Mastery','Magic Mastery','Wind Mastery']:
      changes.append(par+' becomes a spell/physical damage bonus.');continue
     mapped=SKILLS.get(par,par)
     if prop.endswith('-skill') and mapped=='Frost Nova':mapped='Workshop Sorceress Legacy Frost Nova'
     if mapped not in smap:changes.append('Unavailable '+par+' replaced by item-specific Workshop powers.');continue
     if par!=mapped:changes.append(par+' -> '+mapped)
     par=mapped
     if prop=='skill':prop='oskill';changes.append('Class-only skill becomes an all-class skill.')
     if prop.endswith('-skill') and prop!='oskill':hi='1' if mapped.startswith('Workshop Mythical ') else str(min(30,int(hi or 1))) if mapped in ('Starfall','Comet','Kings Trinity') else '60'
     else:lo=hi='30' if mapped in ('Starfall','Comet','Kings Trinity') else '60'
    if prop not in pmap:changes.append('ESR-only '+prop+' replaced by item-specific Workshop powers.');continue
    if prop=='dmg-mag':lo=str(min(255,int(lo)));hi=str(min(511,int(hi)))
    if prop=='dmg-pois':
     if i==31:lo=hi=str(int(lo)//10)
     assert lo==hi
     effects.extend([['ws-poison-rate-wide','',lo,hi],['ws-poison-duration-native','',par,par],['ws-wide-flat-unit','','1','1']]);changes.append('Poison preserved through the existing wide-rate save property.');continue
    if prop in ['abs-all%','abs-fire%','abs-cold%','abs-ltng%']:hi=str(min(20,int(hi or 0)));lo=str(min(int(lo or 0),int(hi)))
    effects.append([prop,par,lo,hi])
   # Native skill tab IDs differ: Paladin offensive=10 is correct; Amazon jav=2 is correct.
   additions=[list(map(str,e)) for e in EXTRAS.get(i,[])]
   if i in MARKERS:additions.insert(0,['ws-mythical-power',str(i),'1','1'])
   # Avoid duplicate copies of a replaced ESR property. Keep useful distinct rolls.
   for e in additions:
    if e not in effects:effects.append(e)
   from mythicals.revision import revise_effects
   effects=revise_effects(i,effects,changes)
   if i==42:additions=effects[-3:]
   if i==6:additions.append(['oskill','Thunder Storm','60','60'])
   # Critical signatures must survive the 12-property engine cap; reserve drop-only.
   while len(effects)>11:
    removable=next((n for n,e in enumerate(effects) if e[0] in ['addxp','light','res-all-max','passive-summ-phys-resist','ws-pet-life','res-curse-len','att','att%','reduce-ac']),None)
    if removable is None:removable=next((n for n,e in enumerate(effects) if e not in additions),0)
    changes.append('Property-slot budget: omitted '+effects[removable][0]);effects.pop(removable)
   # Keep v2 saves stable: clamp only still-narrow legacy fields, never silently overflow.
   for e in effects:
    pr=pmap[e[0]]
    for n in range(1,8):
     st=statmap.get(pr.get('stat'+str(n)));fn=pr.get('func'+str(n))
     if not st or fn not in ('1','3','8','15','16','17') or not st['Save Bits']:continue
     maximum=(1<<int(st['Save Bits']))-1-int(st['Save Add'] or 0)
     indices=[1] if fn=='17' else [2] if fn=='15' else [3] if fn=='16' else [2,3]
     for idx in indices:
      if e[idx] and int(e[idx])>maximum:changes.append(e[0]+' capped at '+str(maximum)+' for existing v2 save field.');e[idx]=str(maximum)
   effects.append(['droponly','','1','1'])
   for n,e in enumerate(effects,1):u.update({a+str(n):v for a,v in zip(['prop','par','min','max'],e)})
   if i>=46 and i not in SCREAMS_KEEP:u['spawnable']='0'
   if existing:existing.update(u)
   else:uniques.append(u)
   variant=''
   if 28<=i<=34:variant=['Fire','Cold','Lightning','Poison','Magic','Physical','Summoning'][i-28]
   if i in [17,39,40]:variant={17:'Fire',39:'Cold',40:'Lightning'}[i]
   if i in [26,37,38]:variant={26:'Poison',37:'Magic',38:'Physical'}[i]
   display=src['_display']+(' - '+variant if variant else '')
   text(key(i),'\u00ffc;'+display+'\u00ffc4');text(key(i)+'Base','Mythical '+template['name'].replace('Mythical ',''))
   if folder.endswith('/excel'):catalog.append(dict(id=i,uniqueId=int(uid),key=key(i),code=code(i),name=display,base=template['name'],source=src['index'],properties=effects,adaptations=changes))
  write(d/'itemtypes.txt',ih,types);assets.append(folder+'/itemtypes.txt')
  for n,(h,rows) in tables.items():write(d/(n+'.txt'),h,rows);assets.append(folder+'/'+n+'.txt')
  from mythicals.revision import revise_tables
  assets.extend(revise_tables(root,d,skills,descs,props,statrows,stath,ph,text))
  for n,h,rows in [('uniqueitems',uh,uniques),('skills',sh,skills),('states',sth,states),('skilldesc',dh,descs),('properties',ph,props)]:write(d/(n+'.txt'),h,rows);assets.append(folder+'/'+n+'.txt')
  th,tcs=read(d/'treasureclassex.txt')
  def tc(name,picks,nd,items):
   r={k:'' for k in th};r.update({'Treasure Class':name,'Picks':str(picks),'NoDrop':str(nd),'*eol':'0'})
   for n,(item,w) in enumerate(items,1):r['Item'+str(n)]=item;r['Prob'+str(n)]=str(w)
   old=next((x for x in tcs if x['Treasure Class']==name),None)
   if old:old.update(r)
   else:tcs.append(r)
  groups=[]
  for start in range(0,len(ACTIVE),10):
   entries=[(key(r['_ordinal']),1) for r in ACTIVE[start:start+10]];name='Ws Mythical Pool '+str(start//10);tc(name,1,0,entries);groups.append((name,len(entries)))
  tc('Ws Mythical Pool',1,0,groups);tc('Ws Mythical Chance',1,4999,[('Ws Mythical Pool',1)])
  # Nested inside the existing Trial-special roll: leaves wrapper arity and all old rolls intact.
  old=next(x for x in tcs if x['Treasure Class']=='Ws Trial Special')
  if old['Item1']!='Ws Trial Special Original':
   prior=dict(old);prior['Treasure Class']='Ws Trial Special Original';tcs.append(prior)
  tc('Ws Trial Special',-2,0,[('Ws Trial Special Original',1),('Ws Mythical Chance',1)])
  # Native TC compilation resolves nested classes only from preceding rows.
  # Move this wrapper's new dependencies ahead of it, preserving all other rows.
  names={'Ws Trial Special Original','Ws Mythical Pool','Ws Mythical Chance'}
  names.update(name for name,_ in groups)
  dependencies=[r for r in tcs if r['Treasure Class'] in names]
  tcs[:]=[r for r in tcs if r['Treasure Class'] not in names]
  ordered=[]
  def place(row):
   if row in ordered:return
   for n in range(1,11):
    child=next((r for r in dependencies if r['Treasure Class']==row.get('Item'+str(n))),None)
    if child is not None:place(child)
   ordered.append(row)
  for row in dependencies:place(row)
  index=next(i for i,r in enumerate(tcs) if r['Treasure Class']=='Ws Trial Special')
  tcs[index:index]=ordered
  # Opt-in early-game test source. Store and restore every original difficulty TC.
  cfg=json.loads((HERE/'test-drops.json').read_text(encoding='utf8'))
  test_on=cfg.get('enabled',False)
  suh,sus=read(d/'superuniques.txt')
  corpse=next(r for r in sus if r['Superunique']=='Corpsefire')
  for field in [k for k in corpse if k.startswith('TC')]:
   saved='Ws Mythical Test Original '+field
   oldtc=next((r for r in tcs if r['Treasure Class']==saved),None)
   if corpse[field]!='Ws Mythical Test Corpsefire' and corpse[field]:
    tc(saved,1,0,[(corpse[field],1)])
    oldtc=next(r for r in tcs if r['Treasure Class']==saved)
   if test_on and oldtc:corpse[field]='Ws Mythical Test Corpsefire'
   elif oldtc:corpse[field]=oldtc['Item1']
  tc('Ws Mythical Test Corpsefire',-6,0,[('Ws Mythical Pool',6)])
  write(d/'superuniques.txt',suh,sus);assets.append(folder+'/superuniques.txt')
  write(d/'treasureclassex.txt',th,tcs);assets.append(folder+'/treasureclassex.txt')
 # IDs are unique across localization files, and stay stable on a second build.
 p=root/'data/local/lng/strings/item-names.json';strings=json.loads(p.read_text(encoding='utf-8-sig'));existing={x['Key']:x for x in strings};highest=max(x['id'] for f in p.parent.glob('*.json') for x in json.loads(f.read_text(encoding='utf-8-sig')) if 'id' in x)
 for k,v in labels.items():
  row=existing.get(k)
  if row is None:highest+=1;row=dict(id=highest,Key=k);strings.append(row)
  for lang in strings[0]:
   if lang not in ('id','Key'):row[lang]=v
 p.write_text(json.dumps(strings,ensure_ascii=False,indent=2),encoding='utf8');assets.append(str(p.relative_to(root)).replace('\\','/'))
 install_art(root,assets)
 out=root/'workshop/mythicals';out.mkdir(parents=True,exist_ok=True)
 (out/'catalog.json').write_text(json.dumps(catalog,ensure_ascii=False,indent=2),encoding='utf8')
 (out/'bindings.h').write_text('\n'.join('constexpr int '+('BondState' if k=='bondState' else 'MarkerStat' if k=='markerStat' else k if k=='RenewalState' else 'Power'+k)+'='+str(v)+';' for k,v in bind.items())+'\n',encoding='utf8')
 # Fixed ring rolls can be duplicated exactly into a short-lived native stat list.
 _,statrows=read(root/'data/global/excel/itemstatcost.txt');statmap={r['Stat']:r for r in statrows}
 bonds=[]
 for item in catalog:
  if not 28<=item['id']<=34:continue
  for prop,par,lo,hi in item['properties']:
   if prop in ('oskill','oskill_hide','ws-mythical-power','droponly','ws-wide-flat-unit','ws-poison-duration-native'):continue
   if prop=='ws-poison-rate-wide':
    for stat in ('poisonmindam','poisonmaxdam'):bonds.append([bind[str(item['id'])],int(statmap[stat]['*ID']),int(lo)])
    continue
   row=pmap[prop]
   for n in range(1,8):
    st=row.get('stat'+str(n));fn=row.get('func'+str(n))
    if not st or fn=='17':continue
    assert fn in ('1','3','8','15','16','17'),(prop,fn)
    assert fn not in ('1','3') or lo==hi,(prop,lo,hi)
    val=int(par if fn=='17' else hi if fn=='16' else lo)
    val <<= int(statmap[st]['ValShift'] or 0)
    bonds.append([bind[str(item['id'])],int(statmap[st]['*ID']),val])
 (out/'bonds.inc').write_text('constexpr int BondBonuses[][3]={\n'+',\n'.join('{'+','.join(map(str,x))+'}' for x in bonds)+'\n};\n',encoding='utf8')
 (out/'assets.json').write_text(json.dumps(sorted(set(assets)),indent=2),encoding='utf8')
 print('Mythicals:',len(ACTIVE),'variants; 1/5000 base Trial loot roll; native stats, procs and imported visuals.')

def install_art(root,assets):
 # Art was imported into a private package; no live ESR dependency at build time.
 package=HERE/'art'
 if not package.exists():raise RuntimeError('Missing imported Mythical artwork package')
 for p in package.rglob('*'):
  if p.is_file():
   rel=p.relative_to(package);target=root/rel;target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(p,target);assets.append(str(rel).replace('\\','/'))
 for f in ['items','uniques']:
  p=root/('data/hd/items/'+f+'.json');j=json.loads(p.read_text(encoding='utf-8-sig'));mapping={k:v for e in j for k,v in e.items()}
  for s in SOURCE:
   i=s['_ordinal'];asset=s['_visual']['normal'];name='workshop_mythical_'+str(i)
   if f=='items':mapping[code(i)]={'asset':asset}
   else:mapping[key(i).lower()]=dict.fromkeys(['normal','uber','ultra'],asset)
  p.write_text(json.dumps([{k:v} for k,v in mapping.items()],indent=2),encoding='utf8');assets.append(str(p.relative_to(root)).replace('\\','/'))
 from mythicals.revision import ground_glow
 ground_glow(root,assets)

if __name__=='__main__':install(sys.argv[1])
