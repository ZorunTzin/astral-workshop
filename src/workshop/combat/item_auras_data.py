"""Aura item properties now grant the skill to all classes instead of pulsing."""
from pathlib import Path
try:
 from .port import read,write
except ImportError:
 from port import read,write

def install(root):
 root=Path(root)
 for folder in ('data/global/excel','data/global/excel/base'):
  path=root/folder/'properties.txt';header,rows=read(path)
  aura=next(r for r in rows if r['code']=='aura');oskill=next(r for r in rows if r['code']=='oskill')
  identity={k:aura[k] for k in ('code','*Id','*Enabled')}
  aura.update(oskill);aura.update(identity)
  aura['*Notes']='Workshop: grants the corresponding skill to all classes; active buffs reserve mana.'
  write(path,header,rows)

if __name__=='__main__':
 import sys
 install(sys.argv[1])
