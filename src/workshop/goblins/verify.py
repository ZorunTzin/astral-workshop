"""Structural/data regression checks. Does not start the game."""
from pathlib import Path
import sys,json,hashlib,collections
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from combat.port import read
root=Path(sys.argv[1]);here=Path(__file__).resolve().parent
up=json.loads((here/'UPSTREAM.json').read_text());families=json.loads((here/'definitions.json').read_text())['families']
for rel,digest in up['files'].items():assert hashlib.sha256((root/rel).read_bytes()).hexdigest()==digest,rel
all_ids=collections.defaultdict(list)
for p in (root/'data/local/lng/strings').glob('*.json'):
 for x in json.loads(p.read_text(encoding='utf-8-sig')):all_ids[x['id']].append(x['Key'])
for keys in all_ids.values():
 if any(k.startswith('WsGoblin') for k in keys):assert len(keys)==1,keys
for folder in ('data/global/excel','data/global/excel/base'):
 p=root/folder;_,mon=read(p/'monstats.txt');_,ex=read(p/'monstats2.txt');_,tcs=read(p/'treasureclassex.txt');tc={r['Treasure Class']:r for r in tcs};codes=set()
 for name in ('misc','weapons','armor'):
  _,rows=read(p/(name+'.txt'));codes.update(r['code'] for r in rows)
 added=[r for r in mon if r['Id'].startswith('ws_goblin_')];assert len(added)==9
 ids=[int(r['*hcIdx']) for r in mon if r['*hcIdx'].isdigit()];assert len(ids)==len(set(ids))
 for r in added:
  assert r['Velocity']=='11' and r['Run']=='11'
  assert r['AI']=='Idle' and r['neverCount']=='1' and r['killable']=='1'
  assert r['MonStatsEx']=='ws_goblin_runner' and any(x['Id']==r['MonStatsEx'] for x in ex)
  assert int(r['ResFi(H)'])<100
  for key,value in r.items():
   if key.startswith('TreasureClass'):assert value in tc,(key,value)
 for r in tcs:
  if not r['Treasure Class'].startswith('Ws Goblin '):continue
  expected=1 if r['Treasure Class'] in ('Ws Goblin Trial Orbs','Ws Goblin Crafting Materials') else 0 if any(x in r['Treasure Class'] for x in ('gelatinous_sire','gelatinous_medium')) else 2 if 'gelatinous_small' in r['Treasure Class'] else 4
  assert r['Picks']==str(expected) and r['NoDrop']=='0'
  if expected==0:assert all(not r.get('Item'+str(i)) for i in range(1,11))
  for i in range(1,11):
   item=r.get('Item'+str(i),'')
   if item:assert item in tc or item in codes,item
 _,levels=read(p/'levels.txt')
 assert not any('ws_goblin_' in str(r) for r in levels),'Only controlled runtime spawns allowed'
 # Gem loot must be genuine gems, not the armor code glv (gloves).
 _,misc=read(p/'misc.txt');gemcodes={x['code'] for x in misc if x['type'].startswith('gem')}
 for suffix in ('',' (N)',' (H)'):
  gems=tc['Ws Goblin gem_hoarder'+suffix]
  assert [gems.get('Item'+str(i),'') for i in range(1,9)]==['gmm','gmt','gms','gme','gmr','gmd','gmk','gmo']
 for suffix in ('',' (N)',' (H)'):
  collector=tc['Ws Goblin odious_collector'+suffix]
  assert collector['Item1']=='Ws Goblin Crafting Materials' and not collector['Item2'] and collector['Prob1']=='1'
 materials=tc['Ws Goblin Crafting Materials'];bycode={r['code']:r for r in misc}
 for i in range(1,8):
  item=bycode[materials['Item'+str(i)]];assert item['spawnable']=='1' and int(item['level'])<=1
 pool=tc['Ws Goblin Trial Orbs']
 assert [pool['Item'+str(i)] for i in range(1,8)]==['ooi','oos','ooc','ooa','ooe','ka3','oor']
 # Existing definitions and TC rows remain byte-for-field identical to the installed baseline.
 repo=here.parents[2]
 for name,key in [('missiles','Missile'),('monstats','Id'),('monstats2','Id'),('treasureclassex','Treasure Class')]:
  _,before=read(repo/folder/(name+'.txt'));_,after=read(p/(name+'.txt'));after={x[key]:x for x in after}
  for r in before:
   if name=='treasureclassex' and r[key]=='Ws Trial Special' and 'Ws Trial Special Original' in after and r['Item1']!='Ws Trial Special Original':
    expected=dict(r);expected[key]='Ws Trial Special Original';assert after['Ws Trial Special Original']==expected;continue
   # Mythical pool composition has its own exhaustive reachability verifier.
   if name=='treasureclassex' and r[key].startswith('Ws Mythical Pool'):continue
   if name=='monstats' and r[key].startswith('wsarena'):
    assert all(after[r[key]][k]==v for k,v in r.items() if k not in ('Level','Level(N)','Level(H)','Exp','Exp(N)','Exp(H)'));continue
   if r[key] and not r[key].startswith(('ws_goblin_','Ws Goblin ','workshop_goblin_','wsleague','Ws League ')):assert after[r[key]]==r,(name,r[key])
models=json.loads((root/'data/hd/character/monsters.json').read_text(encoding='utf-8-sig'))
for f in families:
 assert models['ws_goblin_'+f]=='workshop_goblin_'+f
 assert (root/('data/hd/character/enemy/goblin_'+f+'.json')).is_file()
 assert (root/('data/hd/character/enemy/goblin_'+f+'/animation/combined.animations')).is_file()
assert not (root/'d2rloader/plugins/fmmp-goblins.dll').exists()
assert not (root/'d2rloader/config/workshop-goblins-test.txt').exists(),'Test mode must never enter the release staging payload'
print('PASS: seven encounter families/nine stages, collector crafting materials, terminal-only split loot, visual hashes, global text IDs, native baseline preservation and release/test isolation.')

portal_ids=[]
for folder in ('data/global/excel','data/global/excel/base'):
 _,rows=read(root/folder/'missiles.txt');matches=[r for r in rows if r['Missile']=='workshop_goblin_escape'];assert len(matches)==1
 r=matches[0];portal_ids.append(r['*ID'])
 for k,v in {'pSrvDoFunc':'1','pCltDoFunc':'1','Vel':'0','CollideType':'0','SrcDamage':'0','MinDamage':'0','MaxDamage':'0','EMin':'0','EMax':'0'}.items():assert r[k]==v,(k,r[k])
 assert all(not r[k] for k in ('Skill','pSrvHitFunc','pSrvDmgFunc','SubMissile1','CltSubMissile1'))
assert portal_ids[0]==portal_ids[1]
registry=json.loads((root/'data/hd/missiles/missiles.json').read_text());assert registry['workshop_goblin_escape']=='workshop_goblin_escape'
visual=json.loads((root/'data/hd/missiles/workshop_goblin_escape.json').read_text())
assert any(c.get('hardKillOnDestroy') for e in visual['entities'] for c in e['components'])
assert (root/'data/hd/vfx/particles/fmmp/goblin_hoard.particles').is_file()
print('PASS: private inert portal missile, matching IDs, HD mapping, hard visual cleanup and unchanged existing missiles (including Quake).')

from goblins.mapsense import marker_code,RVA
from integration.prepare import rva_offset
for folder in ('data/global/excel','data/global/excel/base'):
 _,rows=read(root/folder/'monstats2.txt');r=next(r for r in rows if r['Id']=='ws_goblin_runner')
 assert not r['mRN'] and r['mWL']=='1' and r['isSel']=='1' and r['isAtt']=='1'
_,rows=read(root/'data/global/excel/monstats.txt');ids=[int(r['*hcIdx']) for r in rows if r['Id'].startswith('ws_goblin_')]
raw=(root/'d2rloader/plugins/d2rl-ruffneckk-mapsense.dll').read_bytes();off=rva_offset(raw,RVA)
assert raw[off:off+19]==marker_code(ids)
assert 19 not in ids and len(ids)==9
print('PASS: selectable WL movement at fleeing speed; MapSense excludes Fallen and includes all nine goblin forms.')

# Execute the exact guarded classifier bytes in isolation, replacing only its
# draw/false destinations with boolean returns. No game process is involved.
if sys.platform=='win32':
 import ctypes
 code=bytearray(b'\xcc'*1024);code[:19]=raw[off:off+19]
 code[19:25]=bytes.fromhex('b801000000c3')
 target=0x73cc2-RVA;code[target:target+3]=bytes.fromhex('31c0c3')
 k=ctypes.WinDLL('kernel32',use_last_error=True)
 k.VirtualAlloc.argtypes=[ctypes.c_void_p,ctypes.c_size_t,ctypes.c_ulong,ctypes.c_ulong];k.VirtualAlloc.restype=ctypes.c_void_p
 k.VirtualProtect.argtypes=[ctypes.c_void_p,ctypes.c_size_t,ctypes.c_ulong,ctypes.POINTER(ctypes.c_ulong)]
 k.VirtualFree.argtypes=[ctypes.c_void_p,ctypes.c_size_t,ctypes.c_ulong]
 address=k.VirtualAlloc(None,len(code),0x3000,0x04);assert address
 try:
  ctypes.memmove(address,bytes(code),len(code));old=ctypes.c_ulong()
  assert k.VirtualProtect(address,len(code),0x20,ctypes.byref(old))
  classify=ctypes.WINFUNCTYPE(ctypes.c_uint32,ctypes.c_void_p,ctypes.c_uint32)(address)
  for monster in list(range(2000))+[0xffffffff,0x80000000]:assert bool(classify(None,monster))==(monster in ids),monster
 finally:k.VirtualFree(address,0,0x8000)
 print('PASS: native MapSense classifier accepts exactly nine goblin IDs; rejects Fallen, old FMMP IDs and out-of-range values.')

for f in families:
 original=json.loads((here/'upstream/data/hd/character/enemy'/('goblin_'+f+'.json')).read_text())
 scaled=json.loads((root/'data/hd/character/enemy'/('workshop_goblin_'+f+'.json')).read_text())
 def scale(j):return next(c['scale'] for e in j['entities'] if e['name']=='entity_root' for c in e['components'] if c['type']=='TransformDefinitionComponent')
 assert scale(scaled)=={k:v*1.5 for k,v in scale(original).items()}
for folder in ('data/global/excel','data/global/excel/base'):
 p=root/folder;_,states=read(p/'states.txt');state=next(r for r in states if r['state']=='ws_goblin_channel');assert state['overlay1']=='workshopgoblinescape' and not state['pgsv'] and not state['remhit']
 _,overlays=read(p/'overlay.txt');assert any(r['overlay']=='workshopgoblinescape' for r in overlays)
 _,sounds=read(p/'sounds.txt');soundmap={r['Sound']:r for r in sounds};ids=[r['*Index'] for r in sounds];assert len(ids)==len(set(ids))
 for r in sounds:
  if r['Sound'].startswith(('ws_goblin_taunt_','ws_goblin_voice_')):
   assert not r['Redirect'] or r['Redirect'] in soundmap
   assert (root/'data/local/sfx'/r['FileName']).read_bytes()[:4]==b'fLaC'
 _,mon=read(p/'monstats.txt');assert all(r['MonSound']=='ws_goblin_voice' for r in mon if r['Id'].startswith('ws_goblin_'))
visual=json.loads((root/'data/hd/overlays/common/workshopgoblinescape.json').read_text());assert visual['type']=='OverlayDefinition'
assert all(c['hardKillOnDestroy'] for e in visual['entities'] for c in e['components'] if c['type']=='VfxDefinitionComponent')
print('PASS: 1.5x models preserve child proportions; original FLAC sound routing; portal is a nonpersistent monster state overlay.')

for folder in ('data/global/excel','data/global/excel/base'):
 _,rows=read(root/folder/'sounds.txt')
 for prefix in ('ws_goblin_voice_sd','ws_goblin_voice_hd'):
  start=next(i for i,r in enumerate(rows) if r['Sound']==prefix+'1')
  assert [r['Sound'] for r in rows[start:start+6]]==[prefix+str(i) for i in range(1,7)]
  assert rows[start]['Group Size']=='6' and all(r['Group Size']=='0' for r in rows[start+1:start+6])
print('PASS: all six requested recordings have contiguous native random groups and HD redirects.')

# Private set-glow variants: particle behavior untouched, only dimmed color textures.
import struct
from goblins.glow import COLORS,STRENGTH,GRAD,WHITE,padded
source=here/'glow_source'
original=(source/'aura_fanatic.particles').read_bytes()
base=(source/Path(GRAD).name).read_bytes()
dim=padded('data/hd/vfx/goblins/dim',WHITE)
assert (root/dim).read_bytes()[44:]==bytes([115,115,115,150])*16
for i,color in enumerate(COLORS):
 name='wsgoblinglow'+str(i)
 assert (root/('data/global/overlays/'+name+'.dcc')).read_bytes()==(source/'aurafanatic.dcc').read_bytes()
 grad=padded('data/hd/vfx/goblins/glow'+str(i),GRAD)
 pixels=(root/grad).read_bytes()
 assert pixels[:44]==base[:44] and pixels[47::4]==base[47::4]
 for offset in range(44,len(base),4):
  assert list(pixels[offset:offset+3])==[round(max(base[offset:offset+3])*STRENGTH*c) for c in color]
 particle=(root/('data/hd/vfx/particles/workshop/goblin_glow'+str(i)+'.particles')).read_bytes()
 assert particle.replace(grad.encode(),GRAD.encode()).replace(dim.encode(),WHITE.encode())==original
 visual=json.loads((root/('data/hd/overlays/common/'+name+'.json')).read_text())
 assert all(c['hardKillOnDestroy'] for e in visual['entities'] for c in e['components'] if c['type']=='VfxDefinitionComponent')
 for folder in ('data/global/excel','data/global/excel/base'):
  _,states=read(root/folder/'states.txt');state=next(r for r in states if r['state']==name)
  assert state['overlay1']==name and state['aura']=='1' and not state['stat']
  assert next(r for r in states if r['state']=='fullsetgeneric')['overlay1']=='aura_fanatic'
assert all(COLORS[i][2]>COLORS[i][0] for i in (6,7,8))
print('PASS: nine private subdued glow variants, blue Sire stages, unchanged particle behavior and original set glow.')

# Every rendered family must load its own VFX directly, without custom state activation.
for i,family in enumerate(families):
 model=json.loads((root/('data/hd/character/enemy/workshop_goblin_'+family+'.json')).read_text())
 effects=[e for e in model['entities'] if e['name']=='entity_workshop_goblin_glow']
 assert len(effects)==1
 vfx=next(c for c in effects[0]['components'] if c['type']=='VfxDefinitionComponent')
 assert vfx['filename']=='data/hd/vfx/particles/workshop/goblin_glow'+str(i)+'.particles'
 assert vfx['hardKillOnDestroy'] and {'path':vfx['filename']} in model['dependencies']['particles']
 assert (root/vfx['filename']).is_file()
 assert len({e['id'] for e in model['entities']})==len(model['entities'])
 original=json.loads((here/'upstream/data/hd/character/enemy'/('goblin_'+family+'.json')).read_text())
 for entity in original['entities']:
  if entity['name']!='entity_root':assert entity in model['entities']
runtime=(root/'workshop/goblins/runtime.h').read_text()
assert 'Glow(r,unit' not in runtime and 'Glow(child,unit' not in runtime
print('PASS: all nine models directly instantiate their matching glow; original mesh components preserved; no duplicate runtime overlay.')
