"""Refresh private Trial monster awareness without changing world monsters."""
from pathlib import Path
from combat.port import read,write
import re

def install(root):
 root=Path(root)
 ids=set()
 for filename in ('bindings.h','boss_bindings.h'):
  text=(root/'workshop/endgame'/filename).read_text()
  for group in re.findall(r'(?:MonsterClasses|BossClasses)\[\]\s*=\s*\{([^}]+)',text):
   ids.update(map(int,re.findall(r'\d+',group)))
 assert ids
 for folder in ('data/global/excel','data/global/excel/base'):
  p=root/folder/'monstats.txt';h,rows=read(p)
  found=0
  for r in rows:
   if r.get('*hcIdx','').isdigit() and int(r['*hcIdx']) in ids:
    found+=1
    for k in ('aidist','aidist(N)','aidist(H)'):
     if k in r:r[k]='255'
  assert found==len(ids),(found,len(ids))
  write(p,h,rows)
 print('Trial private monsters: arena-wide awareness.')
