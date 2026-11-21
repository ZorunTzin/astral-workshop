"""Weapon ailment affixes and a curated, additive unique/set balance pass."""
import json
from pathlib import Path
from combat.port import read,write
from table_ids import normalize

# Native item saves have no free stat IDs below their 511 terminator.
# These two obsolete slash stats have no properties, skills or item references
# in the input tables and no former save fields. Reuse those exact IDs; never
# shift or widen any previously saved stat. The generator audits this premise.
STATS=[(256,"item_slash_damage","workshop_bleed_chance","ws-bleed","WorkshopBleedChance","Chance to Cause Bleeding on Hit"),
       (257,"item_slash_damage_percent","workshop_impale_chance","ws-impale","WorkshopImpaleChance","Chance to Impale on Hit")]
TIERS=[(1,11,1,2,5),(12,24,9,4,8),(25,39,18,7,12),(40,54,30,10,18),(55,69,42,15,25),(70,84,54,20,32),(85,0,65,25,40)]
# Selection is an explicit balance judgement from this mod's current records,
# not a claim about measured player usage. Never blanket-buff every unique.
SELECTION={
 "uniqueitems":{
  "Rixot's Keen":["bleed"],"Skewer of Krintiz":["bleed"],"Gleamscythe":["bleed"],
  "Bladebone":["bleed"],"Goreshovel":["bleed"],"Brainhew":["bleed"],
  "Soulflay":["bleed"],"Blacktongue":["bleed"],"Ripsaw":["bleed"],
  "The Patriarch":["bleed","impale"],"The Dragon Chang":["bleed"],
  "Razortine":["bleed"],"Dimoak's Hew":["bleed"],"Soul Harvest":["bleed"],
  "Woestave":["bleed"],"Bloodmoon":["bleed"],"Flamebellow":["bleed"],
  "Pluckeye":["impale"],"Witherstring":["impale"],"Leadcrow":["impale"],
  "Ichorsting":["impale"],"Hellcast":["impale"],"Endlesshail":["impale"],
  "Cliffkiller":["impale"],"Magewrath":["impale"],"Langer Briser":["impale"],"Deathbit":["impale"]},
 "setitems":{
  "Berserker's Hatchet":["bleed"],"Death's Touch":["bleed"],"Isenhart's Lightbrand":["bleed"],
  "Angelic Sickle":["bleed"],"Tancred's Crowbill":["bleed"],"Arctic Horn":["impale"],
  "Vidala's Barb":["impale"],"Corgina's Orb":["bleed"],"Warlord's Pike":["bleed"],
  "Lancer's Reach":["bleed"],"Kingpin's Blade":["bleed"],"Cryptic Claws":["bleed"],
  "Trent's Caster":["impale"],"Alyssa's Leafblighter":["impale"],"Holy Fury":["impale"]}}

def tier(level):
 return next(t for t in reversed(TIERS) if level>=t[0])

def install(root):
 root=Path(root);audit=[];affixes=[]
 for directory in ('data/global/excel','data/global/excel/base'):
  p=root/directory;h,stats=read(p/'itemstatcost.txt')
  for sid,old,name,code,key,text in STATS:
   row=next(r for r in stats if r['*ID']==str(sid))
   assert row['Stat']==old and not row['Save Bits'] and not row['Save Param Bits']
   for table in p.glob('*.txt'):
    if table.name=='itemstatcost.txt':continue
    assert old not in table.read_text(encoding='utf-8-sig'),(old,table)
   row.clear();row.update({k:'' for k in h})
   row.update(Stat=name,**{'*ID':str(sid),'Send Bits':'16','Save Bits':'9','Save Add':'0',
     'descpriority':str(86+(sid-256)),'descfunc':'2','descval':'1','descstrpos':key,'descstrneg':key,'advdisplay':'2','*eol':'0'})
  write(p/'itemstatcost.txt',h,stats)
  h,props=read(p/'properties.txt')
  for sid,old,name,code,key,text in STATS:
   existing=next((r for r in props if r['code']==code),None)
   if existing is not None:
    assert existing['func1']=='1' and existing['stat1']==name
    continue
   row={k:'' for k in h};row.update(code=code,func1='1',stat1=name,**{'*eol':'0'});props.append(row)
  write(p/'properties.txt',h,props)
  h,rows=read(p/'magicsuffix.txt');group=max(int(r['group'] or 0) for r in rows)+1
  for index,(sid,old,name,code,key,text) in enumerate(STATS):
   for n,(level,maximum,required,lo,hi) in enumerate(TIERS,1):
    row={k:'' for k in h};label=('of Bloodletting' if index==0 else 'of Impalement')+' '+str(n)
    row.update(name=label,version='100',spawnable='1',rare='1',level=str(level),maxlevel=str(maximum) if maximum else '',levelreq=str(required),frequency='45',group=str(group+index),mod1code=code,mod1min=str(lo),mod1max=str(hi),itype1='weap',etype1='rod',multiply='0',add='0')
    rows.append(row)
    if directory.endswith('/excel'):affixes.append({'name':label,'effect':text,'alvl':level,'maxAlvl':maximum or None,'requiredLevel':required,'min':lo,'max':hi})
  write(p/'magicsuffix.txt',h,rows)
  for table,selection in SELECTION.items():
   h,rows=read(p/(table+'.txt'));by={r['index']:r for r in rows}
   for name,effects in selection.items():
    row=by[name];power=max(int(row['lvl'] or 1),int(row['lvl req'] or 1));lo,hi=tier(power)[3:]
    for i,effect in enumerate(effects):
     # Only the weak, utility-heavy Patriarch gets both; secondary is modest.
     low,high=(2,5) if i else (lo,hi)
     slot=next((n for n in range(1,13 if table=='uniqueitems' else 10) if not row['prop'+str(n)]),None)
     assert slot,(table,name)
     row.update({f'prop{slot}':'ws-'+effect,f'par{slot}':'',f'min{slot}':str(low),f'max{slot}':str(high)})
     if directory.endswith('/excel'):audit.append({'table':table,'item':name,'code':row.get('code',row.get('item')),'powerLevel':power,'effect':effect,'min':low,'max':high,'slot':slot})
   write(p/(table+'.txt'),h,rows)
  normalize(p)
 # English item effect names and tier names use the normal localized tooltip.
 p=root/'data/local/lng/strings/item-names.json';labels=json.loads(p.read_text(encoding='utf-8-sig'))
 last=max(r['id'] for f in p.parent.glob('*.json') for r in json.loads(f.read_text(encoding='utf-8-sig')) if 'id' in r)
 new={key:text for _,_,_,_,key,text in STATS}
 new.update({a['name']:a['name'].rsplit(' ',1)[0] for a in affixes})
 for key,text in new.items():
  assert not any(r['Key']==key for r in labels);last+=1
  labels.append({'id':last,'Key':key,**{lang:text for lang in labels[0] if lang not in ('id','Key')}})
 p.write_text(json.dumps(labels,ensure_ascii=False,indent=2),encoding='utf-8')
 out=root/'workshop/WEAPON-AILMENTS.json';out.write_text(json.dumps({'version':'0.18.0','affixes':affixes,'items':audit,'excludedTypes':['rod'],'allowedException':'orb','affixScaling':'exact ranges; exempt from magic/rare multipliers'},indent=2),encoding='utf-8')
 print(f'Weapon ailments: {len(affixes)} suffixes; {len(audit)} additive bonuses on {sum(map(len,SELECTION.values()))} selected weapons.')
