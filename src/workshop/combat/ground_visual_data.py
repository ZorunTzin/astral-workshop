"""Add an explicit HD floor-effect prefab for Sanctified Ground.

Uses an existing native Warlock sigil particle, with no damage or collision.
The custom registry entry avoids depending on the old Sanctuary center prefab.
"""
import json
from pathlib import Path
try:
 from .port import read,write
except ImportError:
 from port import read,write
HERE=Path(__file__).resolve().parent
NAME='workshop_sanctified_ground'

def install(root):
 root=Path(root);allocated=None
 for folder in ('data/global/excel','data/global/excel/base'):
  p=root/folder/'missiles.txt';header,rows=read(p)
  ident=max(int(r['*ID']) for r in rows if r['*ID'].isdigit())+1
  if allocated is not None:assert allocated==ident
  allocated=ident
  row=next(r for r in rows if r['Missile']=='sanctuarycenter').copy()
  row.update(Missile=NAME,**{'*ID':str(ident),'CelFile':'Expansion\\ShockField','Range':'100','ClientSend':'1','Explosion':'','AlwaysExplode':'','CollideType':'0','CollideKill':'','Red':'255','Green':'215','Blue':'90','Light':'3'})
  rows.append(row);write(p,header,rows)
 registry=root/'data/hd/missiles/missiles.json';mapping=json.loads(registry.read_text(encoding='utf-8-sig'))
 assert NAME not in mapping;mapping[NAME]=NAME
 registry.write_text(json.dumps(mapping,ensure_ascii=False,indent=4),encoding='utf8')
 template=json.loads((root/'data/hd/missiles/seal_rancor.json').read_text(encoding='utf-8-sig'))
 template['name']=NAME
 for entity in template['entities']:
  for component in entity['components']:
   if component['type']=='VfxDefinitionComponent':component['hardKillOnDestroy']=True
   elif component['type']=='TransformDefinitionComponent':
    component['scale']={'x':1.6,'y':1.0,'z':1.6};component['position']={'x':0.0,'y':0.03,'z':0.0};component['inheritOnlyPosition']=True
 asset=root/('data/hd/missiles/'+NAME+'.json')
 asset.write_text(json.dumps(template,ensure_ascii=False,indent=4),encoding='utf8')
 assets=HERE/'assets.json';paths=json.loads(assets.read_text(encoding='utf8'))
 paths+=['data/hd/missiles/missiles.json','data/hd/missiles/'+NAME+'.json']
 assets.write_text(json.dumps(list(dict.fromkeys(paths)),indent=2),encoding='utf8')
 (HERE/'ground_bindings.h').write_text(f'// Generated visual-only projectile ID.\ninline constexpr int GroundVisualMissile={allocated};\n',encoding='utf8')

if __name__=='__main__':
 import sys
 install(sys.argv[1])
