"""Generate zone baselines from the same expansion tables used by the game."""
from combat.port import read
import json

def install(root):
 p=root/'data/global/excel'
 _,levels=read(p/'levels.txt');_,mons=read(p/'monstats.txt');_,ml=read(p/'monlvl.txt')
 by={r['Id']:r for r in mons};levelby={int(r['Level']):r for r in ml if r['Level'].isdigit()}
 from leagues.data import MECHANICS
 coefficients=[[[int(by[b].get('Exp'+s) or 100) for b in m['monsters']] for m in MECHANICS] for s in ('','(N)','(H)')]
 zones=[];curves=[]
 for diff,suffix in enumerate(('', '(N)', '(H)')):
  curve=[[int(levelby.get(i,levelby[max(levelby)]).get('L-'+stat+suffix) or 1) for stat in ('HP','DM','XP')] for i in range(512)];curves.append(curve)
  z=[[1,1] for _ in range(133)]
  for r in levels:
   if not r['Id'].isdigit() or int(r['Id'])>=133:continue
   lv=max(1,int(r.get('MonLvlEx'+suffix) or 1));xp=[]
   for n in range(1,26):
    m=by.get(r.get(('mon' if diff==0 else 'nmon')+str(n)))
    if not m:continue
    nativelevel=max(1,int(m.get('Level') or lv)) if diff==0 else lv
    xp.append(curve[min(511,nativelevel)][2]*int(m.get('Exp'+suffix) or 0)//100)
   z[int(r['Id'])]=[lv,max(1,round(sum(xp)/len(xp))) if xp else curve[min(lv,511)][2]]
  zones.append(z)
 cpp=lambda a:json.dumps(a).replace('[','{').replace(']','}')
 (root/'workshop/leagues/balance_bindings.h').write_text('#pragma once\nnamespace League { inline constexpr int ZoneBaseline[3][133][2]='+cpp(zones)+'; inline constexpr int LevelCurve[3][512][3]='+cpp(curves)+'; inline constexpr int ExperienceCoefficients[3][3]['+str(max(len(m['monsters']) for m in MECHANICS))+']='+cpp(coefficients)+'; }\n')
