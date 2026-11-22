"""Private hostile spells; never reuse rewritten player-skill behavior."""
import csv, io, json, zipfile
from pathlib import Path
from combat.port import read, write
HERE=Path(__file__).resolve().parent
SPELLS=[('GiftedFire','Fire Ball','fireball',30,8),('GiftedLightning','Lightning','lightningbolt',30,8),('GiftedCold','Ice Bolt','icebolt',30,8),('EnchantedChain','Chain Lightning','chainlightning',15,4),('EnchantedCold','Ice Bolt','icebolt',15,4)]
def install(root):
 root=Path(root)
 repo=next(p for p in (root,*root.parents) if (p/'tools/inputs/reimagined-base.zip').exists())
 with zipfile.ZipFile(repo/'tools/inputs/reimagined-base.zip') as z:
  def source(name):return list(csv.DictReader(io.StringIO(z.read('data/global/excel/'+name+'.txt').decode('utf-8-sig')),delimiter='\t'))
  oldskills={r['skill']:r for r in source('skills')};oldmissiles={r['Missile']:r for r in source('missiles')}
 ids=None;visual={}
 for directory in ('data/global/excel','data/global/excel/base'):
  folder=root/directory;sh,skills=read(folder/'skills.txt');mh,missiles=read(folder/'missiles.txt');local={}
  sid=max(int(r['*Id']) for r in skills if r['*Id'].isdigit())+1
  mid=max(int(r['*ID']) for r in missiles if r['*ID'].isdigit())+1
  for name,sourceSkill,sourceMissile,base,growth in SPELLS:
   skillName='Workshop Monster '+name;missileName='workshop_monster_'+name.lower()
   s=oldskills[sourceSkill].copy();s.update(skill=skillName,charclass='',skilldesc='',**{'*Id':str(sid)})
   for key in ('reqskill1','reqskill2','reqskill3','srvstfunc','cltstfunc','srvmissile','cltmissile','aurastate','auratargetstate','EDmgSymPerCalc','DmgSymPerCalc'):s[key]=''
   s.update(reqlevel='1',mana='0',lvlmana='0',startmana='0',minmana='0',localdelay='0',globaldelay='0',ItemEffect='1',SrcDam='0',EMin=str(base+growth),EMax=str(base+growth),calc1='4' if name=='EnchantedChain' else '1',calc2='0',calc3='0',calc4='0',srvdofunc='26' if name=='EnchantedChain' else '8',cltdofunc='',ItemCltEffect='0',srvmissilea=missileName,cltmissilea='',anim='SC',seqtrans='SC',seqnum='',InTown='0')
   # Explicit missile publication is the sole visual path. Native item-proc
   # packets would replay the client spell at its victim as well.
   # Enchanted projectiles: half of the previous release's already reduced curve.
   s['HitShift']=str(int(s.get('HitShift') or 8)-2)
   for n in range(1,6):s['EMinLev'+str(n)]=s['EMaxLev'+str(n)]=str(growth)
   for suffix in ('b','c'):s['srvmissile'+suffix]=s['cltmissile'+suffix]=''
   m=oldmissiles[sourceMissile].copy();m.update(Missile=missileName,Skill=skillName,SrcDamage='0',ClientSend='0',**{'*ID':str(mid)})
   existing=next((r for r in skills if r['skill']==skillName),None)
   if existing is not None:s['*Id']=existing['*Id'];s['skilldesc']=existing['skilldesc'];existing.update(s)
   else:skills.append(s);sid+=1
   existing=next((r for r in missiles if r['Missile']==missileName),None)
   if existing is not None:m['*ID']=existing['*ID'];existing.update(m)
   else:missiles.append(m);mid+=1
   local[name]=int(s['*Id']);local[name+'Missile']=int(m['*ID']);visual[missileName]=sourceMissile
  write(folder/'skills.txt',sh,skills);write(folder/'missiles.txt',mh,missiles)
  if ids is not None:assert ids==local
  ids=local
 registry=root/'data/hd/missiles/missiles.json';data=json.loads(registry.read_text(encoding='utf-8-sig'))
 for name,source in visual.items():
  if source in data:data[name]=data[source]
 registry.write_text(json.dumps(data,indent=2,ensure_ascii=False),encoding='utf-8')
 strings=root/'data/local/lng/strings/monsters.json'
 if strings.exists():
  entries=json.loads(strings.read_text(encoding='utf-8-sig'))
  for entry in entries:
   if entry.get('enUS')=='Magic Resistant':
    entry['enUS']='Magically Gifted';entry['deDE']='Magiebegabt'
  strings.write_text(json.dumps(entries,indent=2,ensure_ascii=False),encoding='utf-8')
 (HERE/'bindings.h').write_text('\n'.join('inline constexpr int '+k+'='+str(v)+';' for k,v in ids.items())+'\n')
 (HERE/'data.json').write_text(json.dumps({'bindings':ids,'visuals':visual},indent=2)+'\n')
 return ids
