"""Give only the private Astral Guardian Paladin combat spell variants.

Ordinary Valkyries and all player skills remain untouched. Private projectiles
retain native visuals and hit handling while binding to the pet's spell damage.
"""
import json
from pathlib import Path
try:
 from .port import read,write
except ImportError:
 from port import read,write
HERE=Path(__file__).resolve().parent
SPELLS=((101,'HolyBolt',6),(112,'Hammer',18),(121,'Heavens',30),(106,'Zeal',18))

def install(root):
 root=Path(root);result=None
 for folder in ('data/global/excel','data/global/excel/base'):
  p=root/folder;sh,skills=read(p/'skills.txt');mh,missiles=read(p/'missiles.txt');monh,monsters=read(p/'monstats.txt')
  eh,equipment=read(p/'monequip.txt');xh,extras=read(p/'monstats2.txt');sth,states=read(p/'states.txt')
  extra_raw=(p/'monstats2.txt').read_bytes()
  ph,pettypes=read(p/'pettype.txt')
  dh,descs=read(p/'skilldesc.txt')
  nextskill=max(int(r['*Id']) for r in skills if r['*Id'].isdigit())+1
  nextmissile=max(int(r['*ID']) for r in missiles if r['*ID'].isdigit())+1
  clones={};definitions=[]
  for offset,(original,key,unlock) in enumerate(SPELLS):
   src=next(r for r in skills if r['*Id']==str(original));row=src.copy();skillid=nextskill+offset
   row.update(skill='Workshop Guardian '+key,charclass='',skilldesc='',**{'*Id':str(skillid),'reqlevel':'1','maxlvl':'99','mana':'0','lvlmana':'0','startmana':'0','minmana':'0','manashift':'8','InTown':'0','monanim':'A1','EDmgSymPerCalc':"stat('damagepercent'.accr)"})
   if original!=106:
    for field in ('EMin','EMax')+tuple(f'E{bound}Lev{band}' for bound in ('Min','Max') for band in range(1,6)):
     if row.get(field):row[field]=str(int(row[field])*5)
   if original==106:
    # Trigger each native Zeal hit through the normal start/do path. The pet's
    # bounded burst scheduler supplies timing instead of a player animation loop.
    row.update(ItemEffect='1',ItemCltEffect='1',localdelay='0',EDmgSymPerCalc='',DmgSymPerCalc='',calc2='400+5*((lvl<5)?0:((lvl-4)*par4))')
   for field in row:
    if field.startswith('reqskill'):row[field]=''
   # Clone the main projectile and FoH's emitted holy bolts. A shared visual
   # explosion can remain shared because it has no gameplay damage.
   names={row[field] for field in ('srvmissile','srvmissilea','srvmissileb','srvmissilec','cltmissilea','cltmissileb','cltmissilec') if row.get(field)}
   if original==121:names.add('fistoftheheavensbolt')
   replacements={name:'workshop_guardian_'+key.lower()+'_'+name for name in sorted(names)}
   for name,newname in replacements.items():
    missile=next(r for r in missiles if r['Missile']==name).copy()
    missile['Missile']=newname;missile['*ID']=str(nextmissile);nextmissile+=1
    if missile['Skill']:missile['Skill']=row['skill']
    if missile.get('EDmgSymPerCalc'):missile['EDmgSymPerCalc']="stat('damagepercent'.accr)"
    for field,value in list(missile.items()):
     if 'missile' in field.lower() and value in replacements:missile[field]=replacements[value]
    missiles.append(missile)
   for field,value in list(row.items()):
    if 'missile' in field.lower() and value in replacements:row[field]=replacements[value]
   skills.append(row);clones[original]=row;definitions.append((skillid,unlock))
  guardian=next(r for r in skills if r['*Id']=='110')
  pettype=next(r for r in pettypes if r['pet type']=='valkyrie').copy()
  pettype['pet type']='workshop_astral_guardian';pettype['name']='WorkshopGuardianName'
  pettype['icontype']='2'  # Native counted pet icon, as used by skeletons.
  pettypes.append(pettype);guardian['pettype']=pettype['pet type']
  write(p/'pettype.txt',ph,pettypes)
  # A distinct monster class lets runtime AI identify this pet exactly, even
  # when an Amazon or an oskill user also owns an ordinary Valkyrie.
  monster=next(r for r in monsters if r['Id']=='valkyrie').copy()
  classid=max(int(r['*hcIdx']) for r in monsters if r['*hcIdx'].isdigit())+1
  monster.update(Id='workshop_astral_guardian',BaseId='workshop_astral_guardian',NameStr='WorkshopGuardianName',**{'*hcIdx':str(classid)})
  skeleton=next(r for r in monsters if r['Id']=='necroskeleton')
  monster['flying']='0'
  for field in ('Velocity','Run','aidel','aidel(N)','aidel(H)'):monster[field]=skeleton[field]
  for field in monster:
   if field.startswith(('Skill','Sk')) and ('mode' in field or 'lvl' in field or field.startswith('Skill')):monster[field]=''
  for slot,(original,_,_) in enumerate(SPELLS,1):
   spell=clones[original]
   # Keep GenericPet's own skill slots empty: the bounded runtime dispatcher
   # chooses spells; the original AI remains responsible for spear attacks.
   guardian['sumskill'+str(slot)]=spell['skill'];guardian[f'sumsk{slot}calc']='lvl'
  # Use a private full player-animation definition, never edit Amazon Valkyrie.
  extra=next(r for r in extras if r['Id']=='valkyrie').copy()
  extra['Id']='workshop_guardian_paladin'
  extra['*hcIdx']=str(max(int(r['*hcIdx']) for r in extras if r['*hcIdx'].isdigit())+1)
  extra['BaseW']='1hs'
  skeleton_extra=next(r for r in extras if r['Id']=='necroskeleton')
  for field in ('SizeX','SizeY','soft'):extra[field]=skeleton_extra[field]
  extra['soft']='0'  # Hard ground collision; friendly owner handling remains native.
  for mode in ('A2','BL','SC','S1','S2','S3','S4'):extra['m'+mode]='1'
  extras.append(extra);monster['MonStatsEx']=extra['Id']
  # The native summon equips by monster-class ID. The old private class had
  # no MonEquip rows, leaving it unarmed despite Valkyrie-derived stat scaling.
  for src in [r for r in equipment if r['monster']=='valkyrie']:
   equip=src.copy();equip['monster']=monster['Id']
   if equip['loc1']=='rarm':equip['item1']={0:'crs',8:'9cr',17:'7cr'}[int(equip['level'])]
   if equip['loc1']=='head':equip['item1']='uhm' if int(equip['level'])>=27 else 'ghm'
   equipment.append(equip)
  for level,item,loc in [(17,'uit','larm'),(8,'xit','larm'),(0,'kit','larm'),(0,'hlm','head')]:
   equip={k:'' for k in eh};equip.update(monster=monster['Id'],level=str(level),item1=item,loc1=loc,mod1='6',**{'*eol':'0'});equipment.append(equip)
  group=[r for r in equipment if r['monster']==monster['Id']]
  equipment=[r for r in equipment if r['monster']!=monster['Id']]+sorted(group,key=lambda r:int(r['level']),reverse=True)
  guardian_state=max(int(r['*ID']) for r in states if r['*ID'].isdigit())+1
  state=next(r for r in states if r['state']=='valkyrie').copy()
  state.update(state='workshop_guardian_paladin',**{'*ID':str(guardian_state),'gfxclass':'3','overlay1':'','castoverlay':''})
  states.append(state)
  zeal_state=guardian_state+1
  zeal={k:'' for k in sth};zeal.update(state='workshop_guardian_zeal',**{'*ID':str(zeal_state),'aura':'1','*eol':'0'});states.append(zeal)
  for field in ('minHP','maxHP','MinHP(N)','MaxHP(N)','MinHP(H)','MaxHP(H)'):
   monster[field]=str(max(1,(int(monster[field])//3)//2))
  monsters.append(monster);guardian['summon']=monster['Id']
  # Fixed spell values use the same five native level bands as the private
  # Holy Bolt definition, aura-point/minion scaling and final 1/10 multiplier.
  desc=next(r for r in descs if r['skilldesc']==guardian['skilldesc'])
  for line,bound in ((2,'Min'),(3,'Max')):
   spell=clones[101]
   terms=[spell['E'+bound] or '0']
   weights=('min(lvl-1,7)','min(max(lvl-8,0),8)','min(max(lvl-16,0),6)','min(max(lvl-22,0),6)','max(lvl-28,0)')
   for band,weight in enumerate(weights,1):terms.append('('+weight+')*'+(spell['E'+bound+'Lev'+str(band)] or '0'))
   desc['desccalca'+str(line)]='('+ '+'.join(terms)+')*(100+clc3)/1000'
  write(p/'skilldesc.txt',dh,descs)
  local={'monster_class':classid,'skills':definitions,'last_missile':nextmissile-1,'paladin_state':guardian_state,'zeal_state':zeal_state}
  if result:assert result==local
  result=local
  for name,header,rows in [('skills',sh,skills),('missiles',mh,missiles),('monstats',monh,monsters),('monequip',eh,equipment),('states',sth,states)]:write(p/(name+'.txt'),header,rows)
  # Preserve quoted component-variant fields in every original animation row.
  tail='\t'.join(extra.get(k,'') for k in xh)+'\r\n'
  (p/'monstats2.txt').write_bytes(extra_raw+(b'' if extra_raw.endswith(b'\n') else b'\r\n')+tail.encode('utf8'))
 strings=root/'data/local/lng/strings/skills.json';rows=json.loads(strings.read_text(encoding='utf8'))
 desc=next(r for r in rows if r['Key']=='WorkshopGuardianDesc')
 for key in desc:
  if key not in ('id','Key'):desc[key]='Summons a guardian using Paladin combat skills, including Zeal.\nAura hard points grant life and damage. Recast to update.'
 strings.write_text(json.dumps(rows,ensure_ascii=False,indent=4),encoding='utf8')
 (HERE/'GUARDIAN.json').write_text(json.dumps(result,indent=2),encoding='utf8')
 header='// Generated private guardian class and Paladin combat skill IDs.\n'
 header+=f'inline constexpr unsigned MonsterClass={result["monster_class"]};\n'
 header+=f'inline constexpr int PaladinState={result["paladin_state"]};\n'
 header+=f'inline constexpr int ZealState={result["zeal_state"]};\n'
 header+='inline constexpr int CombatSpells[][2]={'+','.join('{'+str(a)+','+str(b)+'}' for a,b in result['skills'])+'};\n'
 (HERE/'guardian_bindings.h').write_text(header,encoding='utf8')

if __name__=='__main__':
 import sys
 install(sys.argv[1])
