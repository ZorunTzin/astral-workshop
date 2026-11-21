"""Private subdued set-glow variants. Compiled particle logic stays unchanged."""
from pathlib import Path
import json,struct,hashlib,zlib
from combat.port import read,write
HERE=Path(__file__).resolve().parent
COLORS=[(1,.72,.30),(1,.91,.78),(1,.68,.14),(1,.48,.24),(.28,1,.16),(.88,.45,.18),(.16,.40,1),(.16,.40,1),(.16,.40,1)]
STRENGTH=.45
GRAD='data/hd/vfx/textures/gradient/T_FX_FadedYellow_Gradient.texture'
WHITE='data/hd/vfx/textures/default/White.texture'
def padded(stem,old):
 suffix='.texture';assert len(stem+suffix)<=len(old)
 return stem+'_'*(len(old)-len(stem+suffix))+suffix

def install(root):
 root=Path(root);source=HERE/'glow_source';manifest=json.loads((source/'manifest.json').read_text())
 for name,m in manifest.items():assert hashlib.sha256((source/name).read_bytes()).hexdigest()==m['sha256'],name
 particle=(source/'aura_fanatic.particles').read_bytes();gradient=(source/Path(GRAD).name).read_bytes();template=json.loads((source/'aura_fanatic.json').read_text());assets=[]
 assert particle.count(GRAD.encode())==particle.count(WHITE.encode())==1
 def save(rel,b):
  p=root/rel;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(b);assets.append(rel)
 # Dim the neutral highlight as well; retain its own opacity envelope.
 dim=padded('data/hd/vfx/goblins/dim',WHITE)
 header=struct.pack('<IHH7I',0x2845443c,31,258,4,4,1,0,0,1,4)+struct.pack('<II',64,4)
 save(dim,header+bytes([115,115,115,150])*16)
 for i,color in enumerate(COLORS):
  grad=padded('data/hd/vfx/goblins/glow'+str(i),GRAD);raw=bytearray(gradient)
  magic,fmt,info,w,h,depth,layout,reserved,mips,channels=struct.unpack_from('<IHH7I',raw);assert fmt==31 and mips==1 and channels==4
  size,rel=struct.unpack_from('<II',raw,36);start=40+rel;assert size==w*h*4 and start+size==len(raw)
  for p in range(start,len(raw),4):
   value=max(raw[p:p+3])*STRENGTH
   for c in range(3):raw[p+c]=round(value*color[c])
  save(grad,raw)
  path='data/hd/vfx/particles/workshop/goblin_glow'+str(i)+'.particles'
  variant=particle.replace(GRAD.encode(),grad.encode()).replace(WHITE.encode(),dim.encode())
  assert variant.replace(grad.encode(),GRAD.encode()).replace(dim.encode(),WHITE.encode())==particle
  save(path,variant)
  j=json.loads(json.dumps(template));j['name']='wsgoblinglow'+str(i);j['dependencies']['particles']=[{'path':path}];j['dependencies']['textures'] += [{'path':grad},{'path':dim}]
  for e in j['entities']:
   for c in e['components']:
    if c['type']=='VfxDefinitionComponent':c['filename']=path;c['hardKillOnDestroy']=True
   e['components'].append({'type':'TransformDefinitionComponent','name':'goblin_glow_transform','position':{'x':0,'y':0,'z':0},'orientation':{'x':0,'y':0,'z':0,'w':1},'scale':{'x':.8,'y':.8,'z':.8},'inheritOnlyPosition':True})
  save('data/hd/overlays/common/'+j['name']+'.json',json.dumps(j,indent=2).encode())
  # Native overlay initialization also resolves Filename to its legacy DCC.
  save('data/global/overlays/'+j['name']+'.dcc',(source/'aurafanatic.dcc').read_bytes())
  # Bind directly to the rendered monster, like native willowisp/skmage VFX.
  # A custom server state is no longer needed to instantiate the effect.
  family=json.loads((HERE/'definitions.json').read_text())['families'][i]
  model_rel='data/hd/character/enemy/workshop_goblin_'+family+'.json'
  model=json.loads((root/model_rel).read_text())
  for kind in ('particles','models','textures'):
   for dependency in j['dependencies'][kind]:
    if dependency not in model['dependencies'][kind]:model['dependencies'][kind].append(dependency)
  entity=json.loads(json.dumps(j['entities'][0]));entity['name']='entity_workshop_goblin_glow'
  entity['id']=zlib.crc32(('workshop_goblin_glow_'+family).encode())
  model['entities']=[e for e in model['entities'] if e['name']!=entity['name']]+[entity]
  save(model_rel,json.dumps(model,indent=2).encode())

 bindings=[]
 for folder in ('data/global/excel','data/global/excel/base'):
  p=root/folder;sh,states=read(p/'states.txt');oh,overlays=read(p/'overlay.txt');ids=[]
  for i in range(len(COLORS)):
   name='wsgoblinglow'+str(i);state=next((r for r in states if r['state']==name),None)
   if state is None:
    state={k:'' for k in sh};state.update(state=name,**{'*ID':str(max(int(r['*ID']) for r in states if r['*ID'].isdigit())+1)});states.append(state)
   state.update(aura='1',overlay1=name,**{'*eol':'0'});ids.append(int(state['*ID']))
   overlay=next((r for r in overlays if r['overlay']==name),None)
   if overlay is None:
    overlay=next(r for r in overlays if r['overlay']=='aura_fanatic').copy();overlay.update(overlay=name,Filename=name,Character='common',**{'*ID':str(max(int(r['*ID']) for r in overlays if r['*ID'].isdigit())+1)});overlays.append(overlay)
  write(p/'states.txt',sh,states);write(p/'overlay.txt',oh,overlays);assets += [folder+'/states.txt',folder+'/overlay.txt'];bindings.append(ids)
 assert bindings[0]==bindings[1]
 return assets,bindings[0]
