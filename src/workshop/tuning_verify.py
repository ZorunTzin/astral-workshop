"""Assert the requested balance against pristine Reimagined input tables."""
from pathlib import Path
import csv,io,json,zipfile
from combat.port import read
root=Path(__file__).resolve().parent.parent
with zipfile.ZipFile(root.parent/'workshop-tree-v2/base.zip') as z:
 def base(name,folder='data/global/excel'):
  return list(csv.DictReader(io.StringIO(z.read(folder+'/'+name+'.txt').decode('utf-8-sig')),delimiter='\t'))
 for folder in ('data/global/excel','data/global/excel/base'):
  p=root/folder
  _,skills=read(p/'skills.txt');s={r['skill']:r for r in skills}
  assert s['Terror']['reqlevel']=='1' and not s['Terror']['reqskill1']
  assert s['Amplify Damage']['reqlevel']=='6' and s['Weaken']['reqskill1']=='Terror'
  for name in ('Amplify Damage','Terror','FMMP Virus Infection'):assert s[name]['Param7']=='15'
  assert s['Amplify Damage']['HitShift']=='6' and '/ 10000' in s['Terror']['EDmgSymPerCalc']
  assert s['Attract']['calc4'].endswith('/200')
  for name,shift in [('FMMP Element Ground','4'),('FMMP Element Nova','6'),('FMMP Element Chain','5')]:assert s[name]['HitShift']==shift
  _,descs=read(p/'skilldesc.txt');d={r['skilldesc']:r for r in descs}
  assert (d['terror']['SkillRow'],d['terror']['SkillColumn'])==('1','2')
  assert (d['amplify damage']['SkillRow'],d['amplify damage']['SkillColumn'])==('3','3')
  _,chars=read(p/'charstats.txt')
  for r in chars:
   if r['WalkVelocity']:assert (r['WalkVelocity'],r['RunVelocity'])==('9','14')
  _,monsters=read(p/'monstats.txt');old={r['Id']:r for r in base('monstats',folder)}
  for r in monsters:
   if r['Id'] not in old:continue
   a=old[r['Id']];hostile=r['enabled']=='1' and r['killable']=='1' and r['Align'] in ('','0')
   for suffix,factor in (('',200),('(N)',175),('(H)',150)):
    for prefix in ('minHP','maxHP') if not suffix else ('MinHP','MaxHP'):
     key=prefix+suffix
     if a[key]:assert int(r[key])==(int(a[key])*factor+50)//100 if hostile else r[key]==a[key],(r['Id'],key)
# The formerly omitted unique registry must resolve a real Reimagined asset.
registry={k:v for r in json.loads((root/'data/hd/items/uniques.json').read_text(encoding='utf-8-sig')) for k,v in r.items()}
assert registry['workshop_astral_sigil']['normal']=='uniquecharm/workshop_astral_sigil'
_,uniques=read(root/'data/global/excel/uniqueitems.txt')
assert next(r for r in uniques if r['index']=='WorkshopAstralSigil')['invfile']=='workshop_astral_sigil'
import struct
for suffix in ('.sprite','.lowend.sprite'):
 raw=(root/('data/hd/global/ui/items/misc/uniquecharm/workshop_astral_sigil'+suffix)).read_bytes()
 _,fw,w,h,_,n=struct.unpack_from('<HH4I',raw,4)
 assert fw==w==h and n==1 and len(raw)==40+w*h*4
print('PASS: moved unlock/prerequisite graph, half curse damage, 15% synergies, class speed, hostile HP tiers, friendly HP preserved and Sigil visual registry.')
