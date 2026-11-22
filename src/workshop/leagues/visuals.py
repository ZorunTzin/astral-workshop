"""Native jewel beam plus colored ground aura; no runtime helper units."""
from pathlib import Path
import json,copy,zlib,struct
HERE=Path(__file__).resolve().parent

def apply(root,ground,mechanic):
 source=json.loads((HERE.parent/'mythicals/glow-template.json').read_text(encoding='utf-8-sig'))
 glow=json.loads((root/f'data/hd/overlays/common/wsgoblinglow{[1,4,3][mechanic]}.json').read_text())
 from goblins.glow import GRAD,WHITE,padded
 for path in (padded("data/hd/vfx/leagues/orb"+str(mechanic),GRAD),padded("data/hd/vfx/leagues/w"+str(mechanic),WHITE)):
  ground["dependencies"]["textures"].append({"path":path})
 for kind in ('models','textures'):
  for dependency in glow['dependencies'][kind]:
   if dependency not in ground['dependencies'][kind]:ground['dependencies'][kind].append(dependency)
 effects=[copy.deepcopy(next(e for e in source['entities'] if any(c.get('filename','').endswith('fx_horadric_light.particles') for c in e['components'])))]
 effects+=copy.deepcopy(glow['entities'])
 for n,e in enumerate(effects):
  e['name']=f'league_orb_light_{mechanic}_{n}';e['id']=zlib.crc32((ground['name']+e['name']).encode())
  for c in e['components']:
   if c['type']=='VfxDefinitionComponent':
    c['name']=e['name']+'_vfx';c['hardKillOnDestroy']=True
    c['filename']=(f'data/hd/vfx/particles/workshop/league_orb{mechanic}.particles' if n>0 else 'data/hd/vfx/particles/overlays/object/horadric_light/fx_horadric_light.particles')
    dep={'path':c['filename']}
    if dep not in ground['dependencies']['particles']:ground['dependencies']['particles'].append(dep)
  ground['entities'].append(e)
 return ground

def install(root,save):
 from goblins.glow import GRAD,WHITE,padded
 import struct
 source=HERE.parent/'goblins/glow_source';particle=(source/'aura_fanatic.particles').read_bytes()
 for m,color in enumerate(((.42,.45,.55),(.08,1,.20),(1,.03,.04))):
  raw=bytearray((source/Path(GRAD).name).read_bytes());_,rel=struct.unpack_from('<II',raw,36)
  for i in range(40+rel,len(raw),4):
   value=max(raw[i:i+3])*.65
   for c in range(3):raw[i+c]=round(value*color[c])
  grad=padded('data/hd/vfx/leagues/orb'+str(m),GRAD);save(grad,raw)
  dim=padded('data/hd/vfx/leagues/w'+str(m),WHITE)
  header=struct.pack('<IHH7I',0x2845443c,31,258,4,4,1,0,0,1,4)+struct.pack('<II',64,4)
  save(dim,header+bytes([round(150*c) for c in color]+[180])*16)
  save(f'data/hd/vfx/particles/workshop/league_orb{m}.particles',particle.replace(GRAD.encode(),grad.encode()).replace(WHITE.encode(),dim.encode()))
