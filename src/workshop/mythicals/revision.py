"""Mythical revision: actionable procs, readable regeneration and private ground effects."""
import copy,json,re
from pathlib import Path
from combat.port import read,write

def revise_effects(i,effects,changes):
 special={'Starfall','Comet','Kings Trinity'}
 for e in effects:
  if e[0] in ('oskill','hit-skill','kill-skill','gethit-skill','att-skill'):
   if not e[1].startswith('Workshop Mythical ') and e[1] not in special:
    if e[0]=='oskill':e[2]=e[3]='60'
    else:e[3]='60'
   if i==15:
    e[3]='25'
    if e[0]=='oskill':e[2]='25'
    else:e[0]='kill-skill';e[2]='5'
  if i==8:
   if e[0]=='pierce-ltng':e[0]='pierce-fire'
   if e[0]=='hit-skill' and e[1]=='Hydra':e[2]='15';e[3]='60'
 if i==6:effects.append(['oskill','Thunder Storm','60','60'])
 if i==42:
  effects=[e for e in effects if e[0] not in ('ws-mythical-power','oskill')]
  effects += [['dmg-fire','','1000','2000'],['dmg-cold','50','1000','2000'],['dmg-ltng','','1000','2000']]
 for e in list(effects):
  if e[:2]==['ws-mythical-power','43']:
   effects += [['ws-myth-life-regen','','5','5'],['ws-myth-mana-regen','','400','400']]
  if e[:2]==['ws-mythical-power','44']:
   effects += [['ws-myth-life-regen','','10','10'],['ws-myth-mana-percent','','15','15']]
 return effects

def display_stats(d,rows,header,props,ph,text):
 for ident,old,name,label in [(259,'item_crush_damage_percent','ws-myth-life-regen','Life Regenerated per Second (% of Maximum)'),(260,'item_thrust_damage','ws-myth-mana-percent','Mana Regenerated per Second (% of Maximum)'),(261,'item_thrust_damage_percent','ws-myth-mana-regen','Mana Regeneration (%)')]:
  r=next(x for x in rows if x['*ID']==str(ident))
  if r['Stat']!=name:
   assert r['Stat']==old and not r['Save Bits'] and not r['CSvBits']
   for f in d.glob('*.txt'):
    if f.name!='itemstatcost.txt':assert not re.search(r'\b'+old+r'\b',f.read_text(encoding='utf-8-sig')),f
  r.clear();r.update({k:'' for k in header});r.update(Stat=name,**{'*ID':str(ident),'Send Bits':'16','Save Bits':'16','Save Add':'0','descfunc':'1','descval':'1','descpriority':'185','descstrpos':name,'descstrneg':name,'*eol':'0'})
  text(name,label)
  prop=next((p for p in props if p['code']==name),None)
  if prop is None:
   prop={k:'' for k in ph};prop['*Id']=str(max(int(x['*Id']) for x in props if x['*Id'].isdigit())+1);props.append(prop)
  prop.update(code=name,func1='1',stat1=name,**{'*Enabled':'1','*eol':'0'})

def revise_tables(root,d,skills,descs,props,statrows,stath,ph,text):
 assets=[];by={r['skill']:r for r in skills};ds={r['skilldesc']:r for r in descs}
 # Native item proc fields are handler IDs. Inherited placeholder 1 is a no-op.
 for r in skills:
  if r['skill'].startswith('Workshop Mythical '):
   r['ItemEffect']=r['srvdofunc'];r['restrict']='0';r['reqskill1']='';r['auralencalc']='300' if 'Empowerment' in r['skill'] else r['auralencalc']
 from mythicals.data import SOURCE,SKILLS
 referenced=set()
 for src in SOURCE:
  for n in range(1,13):
   if src.get('prop'+str(n)) in ('hit-skill','att-skill','cast-skill','kill-skill','gethit-skill','takedmg-skill','oskill','skill'):
    name=src.get('par'+str(n));referenced.add(SKILLS.get(name,name))
 referenced|={'Thunder Storm','Hydra','Cyclone Armor','Workshop Sorceress Legacy Frost Nova'}
 for name in referenced:
  r=by.get(name)
  if not r or not r.get('srvdofunc') or r.get('passive')=='1':continue
  r['ItemEffect']=r['srvdofunc'];r['ItemCltEffect']=r['cltdofunc'] or '0'
  if name in ('Starfall','Comet','Kings Trinity'):r['rightskill']='1'
 # Rebuild the formerly meteor-wired Starfall using the proven Blizzard rain chain.
 mh,missiles=read(d/'missiles.txt');originals=[dict(r) for r in missiles if r['Missile'].startswith('blizzard')];rename={r['Missile']:'wsmyth_'+r['Missile'] for r in originals}
 for old in originals:
  name=rename[old['Missile']];prior=next((r for r in missiles if r['Missile']==name),None);r=copy.deepcopy(old)
  r['*ID']=prior['*ID'] if prior else str(max(int(x['*ID']) for x in missiles if x['*ID'].isdigit())+1)
  r['Missile']=name
  for k,v in list(r.items()):
   if v in rename and k!='Missile':r[k]=rename[v]
  if r.get('Skill'):r['Skill']='Starfall'
  r.update(EType='mag',ELen='0',Red='200',Green='170',Blue='255')
  if prior:prior.clear();prior.update(r)
  else:missiles.append(r)
 r=by['Starfall'];sid=r['*Id'];r.clear();r.update(by['Blizzard']);r.update(skill='Starfall',**{'*Id':sid,'charclass':'','skilldesc':'starfall','reqskill1':'','reqskill2':'','reqskill3':'','reqlevel':'1','rightskill':'1','EType':'mag','EMin':'1200','EMax':'1800','EDmgSymPerCalc':'','ELen':'0','ItemEffect':'28','ItemCltEffect':'28','calc1':'8','calc2':'4','localdelay':'25','mana':'35','lvlmana':'0'})
 for n in range(1,6):r['EMinLev'+str(n)]='120';r['EMaxLev'+str(n)]='180'
 for k in ('srvmissilea','srvmissileb','srvmissilec','cltmissilea'):r[k]=rename['blizzardcenter']
 desc=ds['starfall'];desc.update(ds['blizzard']);desc.update(skilldesc='starfall',SkillPage='0',SkillRow='0',SkillColumn='0',ListRow='0',**{'str name':'Starfall','str short':'WorkshopStarfallText','str long':'WorkshopStarfallText','str alt':'WorkshopStarfallText','descdam':'0'})
 for k in list(desc):
  if k.startswith(('dsc3','dsc2')):desc[k]=''
 desc.update(descline1='75',desctexta1='StrSkill4',desccalca1='enma',desccalcb1='exma')
 text('WorkshopStarfallText','Rains falling stars on an area for 4 seconds. Each impact deals magic damage. Mythical exclusive.')
 # Skeleton handlers consume calc1 as native life%, so apply the bonus there,
 # instead of relying on a later item_maxhp_percent passive being recalculated.
 for name in ('Raise Skeleton','Raise Skeletal Mage'):
  r=by[name];r['calc1']="(100+lvl*par2)*(100+stat('ws_pet_life'.accr))/100-100"
  for n in range(1,15):
   if r['passivestat'+str(n)]=='item_maxhp_percent' and 'ws_pet_life' in r['passivecalc'+str(n)]:r['passivestat'+str(n)]='';r['passivecalc'+str(n)]=''
  # Existing life tooltip already evaluates clc1, including difficulty/mastery HP.
 # Make the flat physical defense wording unambiguous, without changing mechanics.
 stat=next(x for x in statrows if x['Stat']=='normal_damage_reduction')
 stat.update(descstrpos='WorkshopDamageTakenReduced',descstrneg='WorkshopDamageTakenReduced')
 text('WorkshopDamageTakenReduced','Physical Damage Taken Reduced by %d')
 display_stats(d,statrows,stath,props,ph,text)
 write(d/'itemstatcost.txt',stath,statrows);write(d/'missiles.txt',mh,missiles)
 assets += [str((d/f).relative_to(root)).replace('\\','/') for f in ('itemstatcost.txt','missiles.txt')]
 reg=root/'data/hd/missiles/missiles.json';j=json.loads(reg.read_text(encoding='utf-8-sig'))
 for old,new in rename.items():j[new]='null' if old=='blizzardcenter' else 'holy_bolt_missile' if 'explode' in old else 'comet'
 reg.write_text(json.dumps(j,indent=4),encoding='utf8');assets.append('data/hd/missiles/missiles.json')
 return assets

def ground_glow(root,assets):
 source=json.loads((Path(__file__).parent/'glow-template.json').read_text(encoding='utf-8-sig'))
 effects=[e for e in source['entities'] if any(c['type']=='VfxDefinitionComponent' for c in e['components'])]
 for rel in list(assets):
  if not rel.startswith('data/hd/items/') or not rel.endswith('.json') or 'workshopmythical' not in rel:continue
  p=root/rel;j=json.loads(p.read_text(encoding='utf8'))
  j['entities']=[e for e in j['entities'] if not e.get('name','').startswith('workshop_mythical_glow_')]
  for n,entry in enumerate(effects):
   e=copy.deepcopy(entry);e['name']='workshop_mythical_glow_'+str(n);e['id']=3900000000+n
   for c in e['components']:
    if c['type']=='VfxDefinitionComponent':
     c['hardKillOnDestroy']=True
     dep={'path':c['filename']}
     if dep not in j['dependencies']['particles']:j['dependencies']['particles'].append(dep)
   j['entities'].append(e)
  p.write_text(json.dumps(j,indent=2),encoding='utf8')

def prepare_skills(skills,descs,text):
 template=json.loads((Path(__file__).parent/'cloak-template.json').read_text(encoding='utf8'))
 name='Workshop Mythical Shadow Veil';old=next((r for r in skills if r['skill']==name),None)
 sid=old['*Id'] if old else str(max(int(r['*Id']) for r in skills if r['*Id'].isdigit())+1)
 r={k:template.get(k,'') for k in skills[0]};r.update(skill=name,**{'*Id':sid,'charclass':'','skilldesc':name,'srvdofunc':'47','ItemEffect':'47','ItemCltEffect':'0','auralencalc':'300','aurarangecalc':'16','reqskill1':'','reqskill2':'','reqskill3':'','mana':'0','minmana':'0','lvlmana':'0','localdelay':'0','maxlvl':'1','restrict':'0'})
 for n,(stat,calc) in enumerate([('armor_override_percent','-50'),('magicresist','-25'),('damageresist','-15'),('fireresist','-25'),('coldresist','-25'),('lightresist','-25')],1):r['aurastat'+str(n)]=stat;r['aurastatcalc'+str(n)]=calc
 if old:old.clear();old.update(r)
 else:skills.append(r)
 dr={k:'' for k in descs[0]};dr.update(skilldesc=name,**{'str name':name,'str short':name,'str long':name,'str alt':name,'*eol':'0'})
 old=next((r for r in descs if r['skilldesc']==name),None)
 if old:old.update(dr)
 else:descs.append(dr)
 text(name,'Shadow Veil: blinds nearby enemies, lowers defense and resistances for 12 seconds')
