from pathlib import Path
from combat.port import read,write
import json

def install(root):
 root=Path(root);assets=[];ids=None
 for folder in ('data/global/excel','data/global/excel/base'):
  p=root/folder/'missiles.txt';h,rows=read(p);by={r['Missile']:r for r in rows};local=[]
  for name,base in [('workshop_trinity_hit','workshop_druid_hit'),('workshop_trinity_fire','nova'),('workshop_trinity_cold','frostnova'),('workshop_trinity_light','nova')]:
   old=by.get(name);r=by[base].copy();r['Missile']=name;r['*ID']=old['*ID'] if old else str(max(int(x['*ID']) for x in rows if x['*ID'].isdigit())+1)
   r.update(Skill='',ClientSend='0',SrcDamage='128' if name.endswith('_hit') else '0',EType='',EMin='0',EMax='0',MinDamage='0',MaxDamage='0',HitShift='8')
   if name.endswith('_hit'):
    r.update(pSrvDoFunc='1',CollideType='0',CollideKill='0',Explosion='0',Range='1')
   else:
    r.update(pSrvDoFunc='1',pSrvHitFunc='0',CollideType='0',CollideKill='0',Explosion='0',Range='20',Vel='24',MaxVel='24',HitClass='0',NextHit='0',NextDelay='0',ResultFlags='0')
   if old:old.clear();old.update(r)
   else:rows.append(r)
   local.append(int(r['*ID']))
  write(p,h,rows);assets.append(folder+'/missiles.txt');assert ids is None or ids==local;ids=local
 p=root/'data/hd/missiles/missiles.json';models=json.loads(p.read_text(encoding='utf-8-sig'))
 for name,base in [('workshop_trinity_fire','fireball'),('workshop_trinity_cold','frostnova'),('workshop_trinity_light','nova')]:models[name]=models[base]
 p.write_text(json.dumps(models,indent=4),encoding='utf-8');assets.append('data/hd/missiles/missiles.json')
 (root/'workshop/leagues/effect_bindings.h').write_text('#pragma once\nnamespace LeagueEffects { inline constexpr int NovaHit='+str(ids[0])+';inline constexpr int NovaVisuals[]={'+','.join(map(str,ids[1:]))+'}; }\n')
 return assets
