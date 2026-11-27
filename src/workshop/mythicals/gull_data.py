"""Original Mythical helmet; append-only IDs, native saved flag and private artwork."""
from pathlib import Path
import json,struct,copy
from PIL import Image
from combat.port import read,write
HERE=Path(__file__).parent
KEY='WorkshopMythical81';CODE='Y81';POWER='ws_gull_shrines'
NAMES=('Armor','Combat','Lightning','Fire','Cold','Poison','Skill','Mana','Stamina','Experience')

def install(root):
 root=Path(root);assets=[];bindings=None;uniqueid=None
 for folder in ('data/global/excel','data/global/excel/base'):
  d=root/folder
  sh,stats=read(d/'itemstatcost.txt');r=next(r for r in stats if r['*ID']=='266')
  if r['Stat']!=POWER:
   assert r['Stat']=='item_absorb_crush_percent' and not r['Save Bits'] and not r['CSvBits']
   for f in d.glob('*.txt'):
    if f.name!='itemstatcost.txt':assert 'item_absorb_crush_percent' not in f.read_text(encoding='utf-8-sig'),f
  r.clear();r.update({k:'' for k in sh});r.update(Stat=POWER,**{'*ID':'266','Send Bits':'1','Save Bits':'1','descfunc':'19','descval':'0','descpriority':'190','descstrpos':'WorkshopGullPower','descstrneg':'WorkshopGullPower','*eol':'0'})
  ph,props=read(d/'properties.txt');prop=next((r for r in props if r['code']==POWER),None)
  if prop is None:prop={k:'' for k in ph};prop['*Id']=str(max(int(r['*Id']) for r in props if r['*Id'].isdigit())+1);props.append(prop)
  prop.update(code=POWER,func1='1',stat1=POWER,**{'*Enabled':'1','*eol':'0'})
  th,types=read(d/'itemtypes.txt');typ=next((r for r in types if r['Code']=='ym81'),None)
  if typ is None:typ=dict(next(r for r in types if r['Code']=='helm'));types.append(typ)
  typ.update(ItemType='Mythical Bone Visage',Code='ym81',Equiv1='helm',Equiv2='myth',TreasureClass='0',Rarity='0',VarInvGfx='0')
  ah,armor=read(d/'armor.txt');base=dict(next(r for r in armor if r['code']=='uh9'))
  base.update(code=CODE,name=KEY+'Base',namestr=KEY+'Base',normcode=CODE,ubercode=CODE,ultracode=CODE,type='ym81',spawnable='0',rarity='0',unique='1',level='90',levelreq='90',reqstr='100',minac='125',maxac='157',uniqueinvfile='',setinvfile='',**{'gamble cost':'0','magic lvl':'0','auto prefix':'','NightmareUpgrade':'xxx','HellUpgrade':'xxx','PermStoreItem':'0'})
  for k in base:
   if k.endswith(('Min','Max','MagicMin','MagicMax')):base[k]='0'
  prior=next((r for r in armor if r['code']==CODE),None)
  if prior:prior.update(base)
  else:armor.append(base)
  uh,uniques=read(d/'uniqueitems.txt');u=next((r for r in uniques if r['index']==KEY),None)
  if u is None:u={k:'' for k in uh};u['*ID']=str(max(int(r['*ID']) for r in uniques if r['*ID'].isdigit())+1);uniques.append(u)
  uid=u['*ID'];u.clear();u.update({k:'' for k in uh});u.update(index=KEY,**{'*ID':uid,'version':'100','spawnable':'1','rarity':'1','nolimit':'1','lvl':'1','lvl req':'90','code':CODE,'*ItemName':'Mythical Bone Visage','cost mult':'12','cost add':'100000','*eol':'0'})
  effects=[[POWER,'','1','1'],['ac%','','150','200'],['hp','','80','120'],['res-all','','15','20'],['droponly','','1','1']]
  for n,e in enumerate(effects,1):u.update({a+str(n):v for a,v in zip(('prop','par','min','max'),e)})
  assert uniqueid is None or uniqueid==int(uid);uniqueid=int(uid)
  sth,states=read(d/'states.txt');ids=[]
  for i,name in enumerate((*NAMES,'Amplifier')):
   key='shrine_gull_'+name.lower() if i<10 else 'ws_gull_amplifier'
   row=next((r for r in states if r['state']==key),None)
   if row is None:
    row=dict(next(r for r in states if r['*ID']==str(128+i))) if i<10 else {k:'' for k in sth}
    row['*ID']=str(max(int(r['*ID']) for r in states if r['*ID'].isdigit())+1);states.append(row)
   row.update(state=key,setfunc='',remfunc='',group='',stat='',canstack='1',aura='1',**{'*eol':'0'})
   ids.append(int(row['*ID']))
  assert bindings is None or bindings==ids;bindings=ids
  for name,h,rows in [('itemstatcost',sh,stats),('properties',ph,props),('itemtypes',th,types),('armor',ah,armor),('uniqueitems',uh,uniques),('states',sth,states)]:
   write(d/(name+'.txt'),h,rows);assets.append(folder+'/'+name+'.txt')
  ch,tcs=read(d/'treasureclassex.txt');by={r['Treasure Class']:r for r in tcs}
  # Existing 53 entries occupy groups 0..5. This remains equiprobable with each.
  group=by['Ws Mythical Pool 5'];group['Item4']=KEY;group['Prob4']='1'
  top=by['Ws Mythical Pool'];top['Prob6']='4'
  write(d/'treasureclassex.txt',ch,tcs);assets.append(folder+'/treasureclassex.txt')
 out=root/'workshop/mythicals';(out/'gull_bindings.h').write_text('constexpr int GullStat=266;\nconstexpr int GullStates[]={'+','.join(map(str,bindings[:10]))+'};\nconstexpr int GullAmplifier='+str(bindings[10])+';\n')
 p=out/'catalog.json';catalog=json.loads(p.read_text(encoding='utf8'));catalog=[r for r in catalog if r['id']!=81];catalog.append(dict(id=81,uniqueId=uniqueid,key=KEY,code=CODE,name='The Gull',base='Bone Visage',source='Original Workshop design',properties=effects,adaptations=['Random shrine buff on elite kill; +50% shrine effect and duration.']));p.write_text(json.dumps(catalog,ensure_ascii=False,indent=2),encoding='utf8')
 p=root/'data/local/lng/strings/item-names.json';rows=json.loads(p.read_text(encoding='utf-8-sig'));last=max(r['id'] for f in p.parent.glob('*.json') for r in json.loads(f.read_text(encoding='utf-8-sig')) if 'id' in r)
 labels={KEY:'\u00ffc;The Gull\u00ffc4',KEY+'Base':'Mythical Bone Visage','WorkshopGullPower':'\u00ffc4This stacks with normal shrines\u00ffc3\n\u00ffc4Gain a random shrine buff on elite kill for 60 seconds\u00ffc3'}
 for key,text in labels.items():
  row=next((r for r in rows if r['Key']==key),None)
  if row is None:last+=1;row={'id':last,'Key':key};rows.append(row)
  for lang in rows[0]:
   if lang not in ('id','Key'):row[lang]=text
 p.write_text(json.dumps(rows,ensure_ascii=False,indent=2),encoding='utf8');assets.append(str(p.relative_to(root)).replace('\\','/'))
 asset='helmet/workshopmythical81'
 image=Image.open(HERE/'gull-art/the-gull.png').convert('RGBA')
 for suffix,size in [('.sprite',196),('.lowend.sprite',98)]:
  rel='data/hd/global/ui/items/armor/'+asset+suffix;p=root/rel;p.parent.mkdir(parents=True,exist_ok=True);im=image.resize((size,size),Image.Resampling.LANCZOS);p.write_bytes(struct.pack('<4sHH8I',b'SpA1',31,size,size,size,0,1,0,0,0,0)+im.tobytes());assets.append(rel)
 # Inventory art is original; the supported native headgear ground rig is retained.
 rel='data/hd/items/armor/'+asset+'.json';p=root/rel;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes((HERE/'art/data/hd/items/armor/circlet/workshopmythical17.json').read_bytes());assets.append(rel)
 from mythicals.revision import ground_glow
 ground_glow(root,assets)
 for file,key,value in [('items',CODE,{'asset':asset}),('uniques',KEY.lower(),dict.fromkeys(('normal','uber','ultra'),asset))]:
  rel='data/hd/items/'+file+'.json';p=root/rel;rows=json.loads(p.read_text(encoding='utf-8-sig'));mapping={k:v for r in rows for k,v in r.items()};mapping[key]=value;p.write_text(json.dumps([{k:v} for k,v in mapping.items()],indent=2),encoding='utf8');assets.append(rel)
 p=out/'assets.json';existing=json.loads(p.read_text());p.write_text(json.dumps(sorted(set(existing+assets)),indent=2))
 print('The Gull: Mythical helmet, five properties, Trial pool, original HD icon, shrine states',bindings)
