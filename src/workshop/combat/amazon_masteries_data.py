"""Idempotent Amazon stances; IDs are appended once and never renumbered."""
from pathlib import Path
import json,struct
from combat.port import read,write
NAMES=('Spear Mastery','Bow Mastery','Javelin Mastery')
# stat, base, per skill level, cap, tooltip (same values drive the native runtime).
BONUSES=[[(76,15,5,200,'Maximum Life: +%d%%'),(16,20,10,500,'Defense: +%d%%'),(36,2,1,20,'Physical Damage Reduction: %d%%'),(39,5,1,30,'All Resistances: +%d%%'),(93,5,1,30,'Attack Speed: +%d%%')],[(96,10,1,40,'Movement Speed: +%d%%'),(-1,3,1,25,'Impale Chance: %d%%'),(48,1,2,500,'Fire / Cold / Lightning Minimum Damage: +%d'),(49,3,3,750,'Fire / Cold / Lightning Maximum Damage: +%d')],[(20,3,1,25,'Block Chance: +%d%%'),(76,10,4,150,'Maximum Life: +%d%%'),(99,5,2,60,'Faster Hit Recovery: +%d%%'),(102,5,2,60,'Faster Block Rate: +%d%%')]]
def install(root):
 root=Path(root);labels={};bindings=None
 for folder in ('data/global/excel','data/global/excel/base'):
  p=root/folder;sh,skills=read(p/'skills.txt');dh,descs=read(p/'skilldesc.txt');th,states=read(p/'states.txt');ih,stats=read(p/'itemstatcost.txt')
  impale=next(int(r['*ID']) for r in stats if r['Stat']=='workshop_impale_chance')
  by={r['skill']:r for r in skills};ds={r['skilldesc']:r for r in descs};ss={r['state']:r for r in states};local=[]
  for i,name in enumerate(NAMES):
   key='WorkshopAmazonMastery'+str(i);state='workshop_amazon_mastery_'+str(i)
   if name not in by:
    r=by['Inner Sight'].copy();r['*Id']=str(max(int(x['*Id']) for x in skills if x['*Id'].isdigit())+1);skills.append(r);by[name]=r
   r=by[name];r.update(SrcDam='0',skill=name,charclass='ama',skilldesc=key,reqlevel='1',reqskill1='',reqskill2='',reqskill3='',maxlvl='20',srvstfunc='',srvdofunc='18',cltstfunc='',cltdofunc='',aurastate=state,auratargetstate='',auralencalc='50',passive='',passivestate='',periodic='',aura='',mana='0',lvlmana='0',startmana='0',minmana='0',leftskill='0',rightskill='1',InTown='1',restrict='1',itypea1='',itypea2='',anim='SC',seqtrans='SC',seqnum='',ItemEffect='0')
   for k in r:
    if k!='aurastate' and k.startswith(('aurastat','passivestat','passivecalc')):r[k]=''
   if state not in ss:
    st=ss['enchant'].copy();st['*ID']=str(max(int(x['*ID']) for x in states if x['*ID'].isdigit())+1);states.append(st);ss[state]=st
   ss[state].update(state=state,skill=name,group='',stat='',setfunc='',itemtype='',itemtrans='',castoverlay='',overlay1='',aura='1',curse='',remhit='')
   helper=name+' Level'
   if helper not in by:
    h=by['Attack'].copy();h['*Id']=str(max(int(x['*Id']) for x in skills if x['*Id'].isdigit())+1);skills.append(h);by[helper]=h
   by[helper].update(skill=helper,charclass='',skilldesc='',calc4="skill('"+name+"'.lvl)")
   if key not in ds:ds[key]={k:'' for k in dh};descs.append(ds[key])
   d=ds[key];d.update(skilldesc=key,SkillPage='3',SkillRow='2' if i==2 else '1',SkillColumn='1' if i==2 else str(i+2),ListRow='0',IconCel=str((8,0,18)[i]),**{'str name':key+'Name','str short':key+'Desc','str long':key+'Desc','str alt':key+'Name','*eol':'0'})
   labels[key+'Name']=name;labels[key+'Desc']='Toggle. Only one Mastery can be active. No mana cost. '+('Spears only. Cannot use bow or javelin skills.' if i==0 else 'Bows and crossbows only.' if i==1 else 'Javelins only.')+' Bonuses are suspended with another weapon.'
   for j,(stat,base,step,cap,text) in enumerate(BONUSES[i],1):
    k=key+'Value'+str(j);labels[k]=text;d.update({f'descline{j}':'36',f'desctexta{j}':k,f'desccalca{j}':f'min({cap},{base}+{step}*lvl)'})
   local.append((int(r['*Id']),int(ss[state]['*ID']),int(by[helper]['*Id'])))
  for name,h,rows in [('skills',sh,skills),('skilldesc',dh,descs),('states',th,states)]:write(p/(name+'.txt'),h,rows)
  if bindings is not None:assert bindings==local
  bindings=local
 _,types=read(root/'data/global/excel/itemtypes.txt');types={r['Code']:r for r in types}
 def istype(t,wanted,seen=None):
  seen=set() if seen is None else seen
  if t in wanted:return True
  if not t or t in seen or t not in types:return False
  seen.add(t);return any(istype(types[t].get(k,''),wanted,seen) for k in ('Equiv1','Equiv2'))
 _,weapons=read(root/'data/global/excel/weapons.txt');codes=[]
 for w in weapons:
  t=w['type'];kind=2 if istype(t,{'jave'}) else 1 if istype(t,{'bow','xbow'}) else 0 if istype(t,{'spea'}) else -1
  if kind>=0:codes.append((int.from_bytes(w['code'].encode().ljust(4,b' '),'little'),kind))
 out=root/'workshop/combat';out.mkdir(parents=True,exist_ok=True)
 lines=['// Generated Amazon stance data.','struct Definition {int skill,state,helper;};','inline constexpr Definition Definitions[]={'+','.join('{'+','.join(map(str,x))+'}' for x in bindings)+'};','struct Weapon {unsigned code;int kind;};','inline constexpr Weapon Weapons[]={'+','.join('{'+str(c)+'u,'+str(k)+'}' for c,k in codes)+'};','struct Bonus {int kind,stat,base,step,cap;};','inline constexpr Bonus Bonuses[]={']
 for i,bonus in enumerate(BONUSES):
  for stat,base,step,cap,_ in bonus:
   targets=[39,41,43,45] if stat==39 else [48,50,54] if stat==48 else [49,51,55] if stat==49 else [impale if stat==-1 else stat]
   lines += ['{'+','.join(map(str,(i,s,base,step,cap)))+'},' for s in targets]
 lines+=['};','inline constexpr int Attacks[]={'+','.join(r['*Id'] for r in skills if r['*Id'].isdigit() and (int(r.get('SrcDam') or 0)>0 or r['skill'] in ('Attack','Throw','Kick') or (r['charclass']=='ama' and any(r.get(k) for k in ('itypea1','itypea2')))))+'};']
 (out/'amazon_masteries_bindings.h').write_text('\n'.join(lines),encoding='utf8')
 p=root/'data/local/lng/strings/skills.json';rows=json.loads(p.read_text(encoding='utf-8-sig'));last=max(r['id'] for f in p.parent.glob('*.json') for r in json.loads(f.read_text(encoding='utf-8-sig')) if 'id' in r)
 for key,value in labels.items():
  r=next((r for r in rows if r['Key']==key),None)
  if r is None:last+=1;r={'id':last,'Key':key};rows.append(r)
  for lang in rows[0]:
   if lang not in ('id','Key'):r[lang]=value
 p.write_text(json.dumps(rows,ensure_ascii=False,indent=2),encoding='utf8')
 from internal_skill_desc import install as install_internal
 install_internal(root)
 print('Amazon: three exclusive level-one Masteries, icons and exact bonus tooltips.')
if __name__=='__main__':
 import sys
 install(sys.argv[1])
