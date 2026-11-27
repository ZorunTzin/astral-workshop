"""PoE-inspired Mythicals. Append-only identities, audited unused saved flags."""
from pathlib import Path
import json,struct,copy
from PIL import Image
from combat.port import read,write
HERE=Path(__file__).parent
POWERS=[(267,'item_absorb_thrust_percent','sin','All your damage becomes magic\nNearby enemies magic resistance is zero'),(304,'item_find_gems_bytime','svalinn','Chance to Block is Lucky (rolls twice)\n-10% to Maximum Block Chance'),(309,'item_damage_vs_monster','dream','1% increased Physical Attack Damage\nper 100 Defense'),(310,'item_damage_percent_vs_monster','rathpith','3% increased Spell Damage per 100 Maximum Life\nSpells cost 10% of current life'),(311,'item_tohit_vs_monster','aegis','Blocking refreshes a 5-second Damage Barrier\nBarrier absorbs Damage equal to 2% of Defense'),(312,'item_tohit_percent_vs_monster','nimis','Shoot additional projectiles in a random direction\nwhich return to you'),(313,'item_ac_vs_monster','prototype','Nearby enemies have this element\'s Resistance\nequal to yours')]
def eff(p,lo,hi=None,par=''):return [p,str(par),str(lo),str(lo if hi is None else hi)]
ITEMS=[
 ('Original Sin','original-sin','misc','rin','ring','sin',1,[eff('allskills',1),eff('ws-myth-speeds',10),eff('res-all',20,25),eff('all-stats',10,15)]),
 ('Svalinn','svalinn','armor','uit','shield','svalinn',1,[eff('ac',300,350),eff('hp',150,180),eff('block2',30),eff('res-all',25,35),eff('allskills',2)]),
 ('Replica Dreamfeather','replica-dreamfeather','weapons','7cr','sword','dream',1,[eff('dmg-norm',200,350),eff('swing2',15,20),eff('ac',400,500),eff('att',280,350),eff('res-all',15,25),eff('all-stats',15,25)]),
 ('Rathpith Globe','rathpith-globe','armor','ush','shield','rathpith',1,[eff('ac%',120,160),eff('hp%',15,20),eff('cast2',25,35),eff('block',10,15),eff('allskills',2)]),
 ('Aegis Aurora','aegis-aurora','armor','uow','shield','aegis',1,[eff('ac',400,450),eff('res-all',25,35),eff('res-all-max',1,3),eff('block',15,20),eff('allskills',2)]),
 ('Nimis','nimis','misc','rin','ring','nimis',1,[eff('dmg%',35,50),eff('dex',80,100),eff('res-all',15,25),eff('lifesteal',3,5),eff('manasteal',3,5),eff('att%',25,35)]),
 *[("Doryani's Prototype - "+e,'doryani-'+e.lower(),'armor','uth','armor','prototype',i,[eff('ac%',150,200),eff('hp',60,90),eff('allskills',3),eff({'Fire':'extra-fire','Cold':'extra-cold','Lightning':'extra-ltng','Poison':'extra-pois'}[e],15,25),eff('cast2',15,25),eff('all-stats',35,50)]) for i,e in enumerate(('Fire','Cold','Lightning','Poison'),1)]]
def install(root):
 root=Path(root);out=root/'workshop/mythicals';assets=[];catalog=json.loads((out/'catalog.json').read_text());labels={};identities=[]
 for folder in ('data/global/excel','data/global/excel/base'):
  d=root/folder;tables={n:read(d/(n+'.txt')) for n in ('itemstatcost','properties','itemtypes','uniqueitems','armor','weapons','misc','treasureclassex','states')}
  sh,stats=tables['itemstatcost'];ph,props=tables['properties']
  for ident,old,power,label in POWERS:
   name='ws_poe_'+power;r=next(x for x in stats if x['*ID']==str(ident))
   if r['Stat']!=name:
    assert r['Stat']==old and not r['Save Bits'] and not r['CSvBits']
    for f in d.glob('*.txt'):
     if f.name!='itemstatcost.txt':assert old not in f.read_text(encoding='utf-8-sig'),f
   r.clear();r.update({k:'' for k in sh});r.update(Stat=name,**{'*ID':str(ident),'Send Bits':'3','Save Bits':'3','descfunc':'19','descval':'0','descpriority':'190','descstrpos':name,'descstrneg':name,'*eol':'0'})
   p=next((x for x in props if x['code']==name),None)
   if p is None:p={k:'' for k in ph};p['*Id']=str(max(int(x['*Id']) for x in props if x['*Id'].isdigit())+1);props.append(p)
   p.update(code=name,func1='1',stat1=name,**{'*Enabled':'1','*eol':'0'});labels[name]='\n'.join('\u00ffc4'+line.replace('%','%%')+'\u00ffc3' for line in reversed(label.split('\n')))
  # Variant-specific Prototype text is supplied by the item tooltip listener.
  next(x for x in stats if x['*ID']=='313')['descfunc']=''
  row=next(x for x in stats if x['*ID']=='314')
  assert row['Stat'] in ('item_ac_percent_vs_monster','ws_poe_spell_bonus') and not row['Save Bits']
  row.update(Stat='ws_poe_spell_bonus',**{'Send Bits':'16','descfunc':'2','descval':'1','descpriority':'90','descstrpos':'WsPoeComputedSpell','descstrneg':'WsPoeComputedSpell','advdisplay':'2'})
  labels['WsPoeComputedSpell']='Spell Damage from Rathpith Globe'
  sth,states=tables['states'];state=next((x for x in states if x['state']=='ws_poe_display'),None)
  if state is None:state={k:'' for k in sth};state.update(state='ws_poe_display',**{'*ID':str(max(int(x['*ID']) for x in states if x['*ID'].isdigit())+1),'aura':'1','*eol':'0'});states.append(state)
  if not any(x['state']=='ws_prototype_aura' for x in states):
   row={k:'' for k in sth};row.update(state='ws_prototype_aura',**{'*ID':str(max(int(x['*ID']) for x in states if x['*ID'].isdigit())+1),'aura':'1','*eol':'0'});states.append(row)
  th,types=tables['itemtypes'];uh,uniques=tables['uniqueitems']
  for ordinal,(name,slug,table,base,category,power,value,effects) in enumerate(ITEMS,82):
   key='WorkshopMythical'+str(ordinal);code='Y'+str(ordinal);h,rows=tables[table];b=dict(next(x for x in rows if x['code']==base));typename='ym'+str(ordinal)
   t=dict(next(x for x in types if x['Code']==b['type']));t.update(ItemType='Mythical '+name,Code=typename,Equiv1=b['type'],Equiv2='myth',TreasureClass='0',Rarity='0',VarInvGfx='0')
   prior=next((x for x in types if x['Code']==typename),None)
   if prior:prior.update(t)
   else:types.append(t)
   b.update(code=code,name=key+'Base',namestr=key+'Base',normcode=code,ubercode=code,ultracode=code,type=typename,spawnable='0',rarity='0',unique='1',level='90',levelreq='90',uniqueinvfile='',setinvfile='',**{'gamble cost':'0','magic lvl':'0','auto prefix':'','NightmareUpgrade':'xxx','HellUpgrade':'xxx','PermStoreItem':'0'})
   for k in b:
    if k.endswith(('Min','Max','MagicMin','MagicMax')):b[k]='0'
   if table=='weapons':b.update(mindam='40',maxdam='65',reqstr='100',reqdex='100')
   elif table=='armor':b['reqstr']='100'
   b={k:b.get(k,'') for k in h};prior=next((x for x in rows if x['code']==code),None)
   if prior:prior.update(b)
   else:rows.append(b)
   u=next((x for x in uniques if x['index']==key),None)
   if u is None:u={k:'' for k in uh};u['*ID']=str(max(int(x['*ID']) for x in uniques if x['*ID'].isdigit())+1);uniques.append(u)
   uid=u['*ID'];u.clear();u.update({k:'' for k in uh});u.update(index=key,**{'*ID':uid,'version':'100','spawnable':'1','rarity':'1','nolimit':'1','lvl':'1','lvl req':'90','code':code,'*ItemName':name,'cost mult':'12','cost add':'100000','*eol':'0'})
   effects=[eff('ws_poe_'+power,value),*effects,eff('droponly',1)]
   for n,e in enumerate(effects,1):u.update({a+str(n):v for a,v in zip(('prop','par','min','max'),e)})
   if folder.endswith('/excel'):
    catalog=[x for x in catalog if x['id']!=ordinal];catalog.append(dict(id=ordinal,uniqueId=int(uid),key=key,code=code,name=name,base=base,source='PoE adaptation',properties=effects,adaptations=[('Nearby enemies have '+name.split(' - ')[-1].lower()+' resistance equal to yours') if power=='prototype' else dict((p,t) for _,_,p,t in POWERS)[power]]))
   labels[key]='\u00ffc;'+name+'\u00ffc4';labels[key+'Base']='Mythical '+category.title()
  ch,tcs=tables['treasureclassex'];by={r['Treasure Class']:r for r in tcs}
  def tc(name,picks,entries):
   r=by.get(name)
   if r is None:r={k:'' for k in ch};tcs.append(r);by[name]=r
   r.update({k:'' for k in ch});r.update({'Treasure Class':name,'Picks':str(picks),'NoDrop':'0','Unique':'1024','*eol':'0'})
   for i,(key,weight) in enumerate(entries,1):r['Item'+str(i)]=key;r['Prob'+str(i)]=str(weight)
  from mythicals.data import ACTIVE
  active=[x['_ordinal'] for x in ACTIVE]+list(range(81,92));groups=[]
  for start in range(0,len(active),10):
   name='Ws Mythical Pool '+str(start//10);members=active[start:start+10];tc(name,1,[('WorkshopMythical%02d'%x,1) for x in members]);groups.append((name,len(members)))
  tc('Ws Mythical Pool',1,groups)
  tc('Ws Mythical Test Corpsefire',-6,[('WorkshopMythical'+str(x),1) for x in range(81,87)])
  tc('Ws Mythical Test Poe Extra',-5,[('WorkshopMythical'+str(x),1) for x in range(87,92)])
  # All pool dependencies precede the existing chance parent.
  names={n for n,_ in groups}|{'Ws Mythical Pool'};ordered=[by[n] for n,_ in groups]+[by['Ws Mythical Pool']];tcs[:]=[r for r in tcs if r['Treasure Class'] not in names];at=next(i for i,r in enumerate(tcs) if r['Treasure Class']=='Ws Mythical Chance');tcs[at:at]=ordered
  for n,(h,rows) in tables.items():write(d/(n+'.txt'),h,rows);assets.append(folder+'/'+n+'.txt')
 (out/'catalog.json').write_text(json.dumps(catalog,ensure_ascii=False,indent=2),encoding='utf8')
 p=root/'data/local/lng/strings/item-names.json';strings=json.loads(p.read_text(encoding='utf-8-sig'));last=max(r['id'] for f in p.parent.glob('*.json') for r in json.loads(f.read_text(encoding='utf-8-sig')) if 'id' in r)
 for key,text in labels.items():
  r=next((r for r in strings if r['Key']==key),None)
  if r is None:last+=1;r={'id':last,'Key':key};strings.append(r)
  for lang in strings[0]:
   if lang not in ('id','Key'):r[lang]=text
 p.write_text(json.dumps(strings,ensure_ascii=False,indent=2),encoding='utf8');assets.append(str(p.relative_to(root)).replace('\\','/'))
 mappings={f:{k:v for r in json.loads((root/('data/hd/items/'+f+'.json')).read_text(encoding='utf-8-sig')) for k,v in r.items()} for f in ('items','uniques')}
 for ordinal,(_,slug,table,base,category,*_) in enumerate(ITEMS,82):
  key='WorkshopMythical'+str(ordinal);asset=category+'/workshopmythical'+str(ordinal)
  oldasset=mappings['items'][base]['asset'];prefix='data/hd/global/ui/items/'+('weapon/' if table=='weapons' else 'armor/' if table=='armor' else 'misc/')
  art_slug='doryani-cold' if slug=='doryani-poison' else slug
  im=Image.open(HERE/'poe-art'/(art_slug+'.png')).convert('RGBA')
  bounds=im.getchannel('A').getbbox()
  if bounds:im=im.crop(bounds)
  base_row=next(x for x in read(root/'data/global/excel'/(table+'.txt'))[1] if x['code']=='Y'+str(ordinal))
  iw,ih=int(base_row['invwidth']),int(base_row['invheight'])
  for suffix,size in [('.sprite',196),('.lowend.sprite',98)]:
   rel=prefix+asset+suffix;p=root/rel;p.parent.mkdir(parents=True,exist_ok=True);w,h=iw*size//2,ih*size//2;pic=im.copy();pic.thumbnail((w-4,h-4),Image.Resampling.LANCZOS);canvas=Image.new('RGBA',(w,h));canvas.alpha_composite(pic,((w-pic.width)//2,(h-pic.height)//2));p.write_bytes(struct.pack('<4sHH8I',b'SpA1',31,w,w,h,0,1,0,0,0,0)+canvas.tobytes());assets.append(rel)
  # Reuse a supported native ground rig of the same equipment family.
  candidates=list((HERE/'art/data/hd/items').rglob('*.json'));want='weapon' if table=='weapons' else 'armor' if table=='armor' else 'misc'
  template=next((p for p in candidates if category in p.parts),None)
  if template is None:template=next(p for p in candidates if want in p.parts)
  rel='data/hd/items/'+('weapon/' if table=='weapons' else 'armor/' if table=='armor' else 'misc/')+asset+'.json';p=root/rel;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes((HERE/'poe-models'/(base+'.json')).read_bytes() if (HERE/'poe-models'/(base+'.json')).exists() else template.read_bytes());assets.append(rel)
  mappings['items']['Y'+str(ordinal)]={'asset':asset};mappings['uniques'][key.lower()]=dict.fromkeys(('normal','uber','ultra'),asset)
 for f,m in mappings.items():
  rel='data/hd/items/'+f+'.json';(root/rel).write_text(json.dumps([{k:v} for k,v in m.items()],indent=2),encoding='utf8');assets.append(rel)
 from mythicals.revision import ground_glow
 ground_glow(root,assets)
 p=out/'assets.json';p.write_text(json.dumps(sorted(set(json.loads(p.read_text())+assets)),indent=2))
 finalize(root)
 print('PoE Mythicals: ten new items, original art, 64 active variants; test drops controlled by test-drops.json.')
def finalize(root):
 root=Path(root);d=root/'data/global/excel';_,tc=read(d/'treasureclassex.txt');_,skills=read(d/'skills.txt');_,missiles=read(d/'missiles.txt')
 cfg=json.loads((HERE/'test-drops.json').read_text(encoding='utf-8-sig'))
 if True:
  for folder in ('data/global/excel','data/global/excel/base'):
   p=root/folder/'monstats.txt';h,rows=read(p);r=next(x for x in rows if x['Id']=='bloodraven')
   for key in h:
    if key.startswith('TreasureClass'):
     suffix=' (N)' if '(N)' in key else ' (H)' if '(H)' in key else ''
     if cfg.get('enabled'):r[key]='Ws Mythical Test Poe Extra'
     elif r[key]=='Ws Mythical Test Poe Extra':r[key]='Blood Raven'+suffix
   write(p,h,rows)
   if not cfg.get('enabled'):
    # Seal wrappers may retain the old test reward after the direct field changed.
    tp=root/folder/'treasureclassex.txt';th,trs=read(tp)
    for row in trs:
     if row['Treasure Class'].startswith('Ws Seal World '):
      for n in range(1,11):
       if row.get('Item'+str(n))=='Ws Mythical Test Poe Extra':row['Item'+str(n)]='Blood Raven (H)'
    write(tp,th,trs)
  p=root/'workshop/mythicals/assets.json';assets=json.loads(p.read_text());p.write_text(json.dumps(sorted(set(assets+['data/global/excel/monstats.txt','data/global/excel/base/monstats.txt'])),indent=2))
 # Exclude attacks, pets, passives and non-damaging utility spells.
 spells=[int(r['*Id']) for r in skills if r['*Id'].isdigit() and r['passive']!='1' and not r.get('summon') and not r.get('aurastate') and r.get('EType') not in ('stun','heal') and int(r.get('SrcDam') or 0)==0 and (r.get('EType') or any(r.get(k) not in ('',None,'0') for k in ('EMin','EMax','MinDam','MaxDam'))) and r.get('srvdofunc')]
 returns=[int(r['*ID']) for r in missiles if r['*ID'].isdigit() and r.get('pSrvDoFunc')=='1' and int(r.get('Vel') or 0)>0 and r.get('CollideType') in ('1','3') and r.get('Missile') not in ('wsreturnarrow',) and (not r['Missile'].startswith(('workshop_','ws')) or r['Missile'].startswith('workshop_amazon_arrow_'))]
 _,states=read(d/'states.txt')
 lines=['constexpr int DisplayState='+next(x['*ID'] for x in states if x['state']=='ws_poe_display')+';', '// Generated from finalized stage tables.','constexpr bool TestDrops='+str(bool(cfg.get('enabled'))).lower()+';']
 for name,label in [('Ws Mythical Test Corpsefire','TestTC'),('Ws Mythical Test Poe Extra','ExtraTC')]:lines.append('constexpr unsigned short '+label+'='+str(next(i for i,r in enumerate(tc) if r['Treasure Class']==name))+';')
 velocities={int(r['*ID']):(int(r.get('Vel') or 0),int(r.get('LevVel') or 0)) for r in missiles if r['*ID'].isdigit()}
 lines+=['constexpr int PrototypeState='+next(x['*ID'] for x in states if x['state']=='ws_prototype_aura')+';']
 lines+=['constexpr int ReturnVelocity[][2]={'+','.join('{%d,%d}'%velocities[i] for i in returns)+'};']
 lines+=['constexpr int SpellSkills[]={'+','.join(map(str,spells))+'};','constexpr int ReturnClasses[]={'+','.join(map(str,returns))+'};']
 (root/'workshop/mythicals/poe_bindings.h').write_text('\n'.join(lines)+'\n')
