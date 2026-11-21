from pathlib import Path
from item_auras_data import read
root=Path(__file__).resolve().parents[2]
for folder in ('data/global/excel','data/global/excel/base'):
 _,rows=read(root/folder/'properties.txt')
 aura=next(r for r in rows if r['code']=='aura');skill=next(r for r in rows if r['code']=='oskill')
 for key in skill:
  if key not in ('code','*Id','*Enabled','*Notes'):assert aura[key]==skill[key],key
 assert aura['*Id']=='125'
print('PASS: both item aura properties grant full-level ordinary all-class skills.')
