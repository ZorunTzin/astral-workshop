"""Reproducible level-120 progression, calibrated against the frozen 0.21.0 curve."""
import csv,json,math,sys
from pathlib import Path
HERE=Path(__file__).resolve().parent
BASE=json.loads((HERE/'baseline.json').read_text())
CAP=120
MON_CAP=511 # level stat is transmitted in nine bits

def read(p):
 with p.open(encoding='utf-8-sig',newline='') as f:
  r=csv.DictReader(f,delimiter='\t');return r.fieldnames,list(r)
def write(p,h,rows):
 with p.open('w',encoding='utf8',newline='') as f:
  w=csv.DictWriter(f,fieldnames=h,delimiter='\t',lineterminator='\r\n');w.writeheader();w.writerows(rows)
def curve():
 old={int(r['Level']):r for r in BASE['experience'] if r['Level'].isdigit()}
 classes=[k for k in old[0] if k not in ('Level','ExpRatio')]
 assert all(len({r[k] for k in classes})==1 for r in old.values())
 xp=lambda level:int(old[level-1]['Amazon'])
 ratios={n:int(old[n]['ExpRatio']) for n in old}
 remaining=xp(99)-xp(80)
 target=sum((xp(n+1)-xp(n))*1024/ratios[n] for n in range(80,99))
 # Spread the old 79..98 transition over new 79..119: 47.5% of the old slope.
 nr={n:1024 for n in range(80)}
 weights=[]
 for n in range(80,120):
  x=79+(n-79)*19/40;lo=math.floor(x);hi=min(98,lo+1);f=x-lo
  nr[n]=round(ratios[lo]*(1-f)+ratios[hi]*f)
  weights.append((xp(lo+1)-xp(lo))*(1-f)+(xp(hi+1)-xp(hi))*f)
 def distribution(a):
  w=[v*math.exp(a*i/39) for i,v in enumerate(weights)];return [remaining*v/sum(w) for v in w]
 lower,upper=-50.,50.
 for _ in range(100):
  mid=(lower+upper)/2;cost=sum(v*1024/nr[80+i] for i,v in enumerate(distribution(mid)))
  if cost<target:lower=mid
  else:upper=mid
 d=distribution((lower+upper)/2)
 cumulative={n:xp(n) for n in range(1,81)}
 for i in range(40):cumulative[81+i]=xp(80)+round(sum(d[:i+1]))
 assert cumulative[120]==xp(99)
 actual=sum((cumulative[n+1]-cumulative[n])*1024/nr[n] for n in range(80,120))
 assert abs(actual/target-1)<1e-8
 maxrow=BASE['experience'][0].copy()
 for k in classes:maxrow[k]=str(CAP)
 result=[maxrow]
 for index in range(121):
  threshold=cumulative[index+1] if index<120 else cumulative[120]+(cumulative[120]-cumulative[119])
  assert threshold<2**32
  result.append({'Level':str(index),**{k:str(threshold) for k in classes},'ExpRatio':str(nr.get(index,1))})
 report={'cap':CAP,'total_xp':cumulative[120],'old_target_level':99,'old_effective_xp_80_to_99':target,'new_effective_xp_80_to_120':actual,'relative_error':actual/target-1,'levels':[{'level':n,'total_xp':cumulative[n],'next_xp':cumulative[n+1]-cumulative[n] if n<120 else 0,'xp_percent':nr.get(n,0)/1024*100} for n in range(1,121)]}
 return result,report

def install(root):
 root=Path(root);assets=[];experience,report=curve()
 original=BASE['monlvl'];mh=list(original[0]);mon=[r.copy() for r in original]
 # Keep existing level-110 special row intact. Extend native lookup bounds safely.
 for level in range(111,MON_CAP+1):
  r={'Level':str(level)}
  for k in mh[1:]:r[k]=str(int(original[110][k])+(level-110)*max(1,int(original[109][k])-int(original[108][k])))
  mon.append(r)
 pool=json.loads((root/'workshop/endgame/enemy_pool.json').read_text())
 for directory in ('data/global/excel','data/global/excel/base'):
  p=root/directory
  write(p/'experience.txt',list(experience[0]),experience)
  write(p/'monlvl.txt',mh,mon)
  h,rs=read(p/'difficultylevels.txt')
  for r in rs:r['DeathExpPenalty']='0'
  write(p/'difficultylevels.txt',h,rs)
  h,rs=read(p/'levels.txt')
  for r in rs:
   if r['Name'].startswith('Workshop ') and 'Trial' in r['Name']:
    for k in r:
     if k.startswith('MonLvl'):r[k]='100'
  write(p/'levels.txt',h,rs)
  h,rs=read(p/'monstats.txt');by={r['Id']:r for r in rs}
  for r in rs:
   if not r['Id'].startswith('wsarena'):continue
   source=by[pool[int(r['Id'][10:])]] if r['Id'].startswith('wsarenamon') else by[['andariel','duriel','mephisto','diablo','baalcrab'][int(r['Id'][11:])]]
   for k in ('Level','Level(N)','Level(H)'):r[k]='100'
   for k in ('Exp','Exp(N)','Exp(H)'):r[k]=str(max(1,int(source[k] or 100))) if source else '100'
  write(p/'monstats.txt',h,rs)
  assets.extend(directory+'/'+n+'.txt' for n in ('experience','monlvl','difficultylevels','levels','monstats'))
 # XP alone follows the smooth ordinary 100..109 trend: no special 110 boss spike.
 xp=[int(r['L-XP(H)']) for r in mon]
 start=xp[100];step=xp[109]-xp[108]
 header='''#pragma once
#include <algorithm>
#include <cstdint>
namespace Progression {
inline constexpr int MonsterCap=511;
inline constexpr int NativeHellXP[]={'''+','.join(map(str,xp))+'''};
inline int WaveLevel(int wave){return int(std::clamp<int64_t>(99LL+wave,100,MonsterCap));}
inline int ScaleExperience(int xp,int originalLevel,int wave){
 if(xp<=0)return 0; // Zero-XP split stages remain zero, never duplicate rewards.
 const int level=WaveLevel(wave),source=std::clamp(originalLevel,1,MonsterCap);
 const int64_t target='''+str(start)+'''LL+(level-100)*'''+str(step)+'''LL;
 return int(std::clamp<int64_t>(int64_t(xp)*target/std::max(1,NativeHellXP[source]),1,0x3fffffff));
}
}
'''
 out=root/'workshop/progression';out.mkdir(exist_ok=True)
 (out/'bindings.h').write_text(header)
 (out/'report.json').write_text(json.dumps(report,indent=2))
 (out/'assets.json').write_text(json.dumps(assets))
 print('Progression: cap 120; total XP',report['total_xp'],'; calibrated effective effort error',report['relative_error'],'; no death XP loss.')
if __name__=='__main__':install(sys.argv[1])
