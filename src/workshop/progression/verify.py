"""Progression table and calibrated effort regression checks (no live game)."""
import json,sys,hashlib
from pathlib import Path
from data import BASE,curve,read,install
root=Path(sys.argv[1]);expected,report=curve()
for folder in ('data/global/excel','data/global/excel/base'):
 p=root/folder;_,rows=read(p/'experience.txt');assert rows==expected
 classes=[k for k in rows[0] if k not in ('Level','ExpRatio')]
 assert all(rows[0][c]=='120' for c in classes)
 old={int(r['Level']):r for r in BASE['experience'] if r['Level'].isdigit()}
 new={int(r['Level']):r for r in rows if r['Level'].isdigit()}
 assert all(new[n]==old[n] for n in range(80))
 assert int(new[119]['Amazon'])==int(old[98]['Amazon'])==3520485254
 assert 1024>int(new[80]['ExpRatio'])>int(old[80]['ExpRatio'])
 assert int(new[119]['ExpRatio'])==int(old[98]['ExpRatio'])==16
 assert all(int(new[n]['Amazon'])<int(new[n+1]['Amazon'])<2**32 for n in range(120))
 assert all(int(new[n]['ExpRatio'])>=int(new[n+1]['ExpRatio'])>0 for n in range(120))
 # Compute work independently from the emitted thresholds and penalties.
 cost=lambda t,start,end:sum((int(t[n]['Amazon'])-int(t[n-1]['Amazon']))*1024/int(t[n]['ExpRatio']) for n in range(start,end))
 assert abs(cost(new,1,120)/cost(old,1,99)-1)<1e-8
 _,diff=read(p/'difficultylevels.txt');assert all(r['DeathExpPenalty']=='0' for r in diff)
 _,stats=read(p/'itemstatcost.txt');level=next(r for r in stats if r['Stat']=='level')
 assert 120<2**int(level['CSvBits']) and 511<2**int(level['Send Bits'])
 _,mon=read(p/'monlvl.txt');assert len(mon)==512 and [r['Level'] for r in mon]==list(map(str,range(512)))
 assert mon[:111]==BASE['monlvl'],'Existing world monster curves must not change'
 _,rs=read(p/'monstats.txt');by={r['Id']:r for r in rs}
 pool=json.loads((root/'workshop/endgame/enemy_pool.json').read_text())
 for i,source in enumerate(pool):
  r=by['wsarenamon'+str(i)]
  for k in ('Exp','Exp(N)','Exp(H)'):assert int(r[k])==max(1,int(by[source][k] or 100))
  assert r['Level']==r['Level(N)']==r['Level(H)']=='100'
 for i in range(5):assert by['wsarenaboss'+str(i)]['Level(H)']=='100' and int(by['wsarenaboss'+str(i)]['Exp(H)'])>0
 _,levels=read(p/'levels.txt')
 for r in levels:
  if r['Name'].startswith('Workshop ') and 'Trial' in r['Name']:assert all(r[k]=='100' for k in r if k.startswith('MonLvl'))
assets=json.loads((root/'workshop/progression/assets.json').read_text())
before={a:hashlib.sha256((root/a).read_bytes()).hexdigest() for a in assets};install(root)
assert before=={a:hashlib.sha256((root/a).read_bytes()).hexdigest() for a in assets}
print('PASS: cap/save ranges, unchanged 1–79, exact old-99 total, calibrated effective XP, zero death loss, native XP on Trial monsters/bosses, lookup bounds and idempotence.')
