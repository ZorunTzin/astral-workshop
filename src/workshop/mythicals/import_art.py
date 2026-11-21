"""One-time import from a user-installed ESR; builds use the checked-in art package."""
from pathlib import Path
import json,re,importlib.util,ctypes
ESR=Path('B:/D2R/Diablo II Resurrected/Mods/Eastern_Sun_Resurrected/Eastern_Sun_Resurrected.mpq')
ROOT=Path(__file__).resolve().parent
spec=importlib.util.spec_from_file_location('casc','B:/D2R/Diablo II Resurrected/Mods/FMMP-Experimental-R67/Randomizer-Setup/Curse-Overhaul/Overlays/casc_extract.py');casc=importlib.util.module_from_spec(spec);spec.loader.exec_module(casc)
h=casc.W.HANDLE();assert casc.d.CascOpenStorage(b'B:/D2R/Diablo II Resurrected',0xffffffff,ctypes.byref(h))
rows=json.loads((ROOT/'source.json').read_text(encoding='utf8'));dest=ROOT/'art';seen=set();fallbacks=[]
base={k:v for x in json.loads((ESR/'data/hd/items/items.json').read_text(encoding='utf-8-sig')) for k,v in x.items()}
pattern=re.compile(rb'data[/\\][A-Za-z0-9_ /\\.\-]+?\.(?:json|texture|model|skeleton|animation|particle|sprite|material|physics|dc6|dcc)(?![A-Za-z])',re.I)
def copy(rel,target=None,raw=None):
 rel=rel.replace('\\','/');target=target or rel.replace('data/hd/','data/wm/',1)
 if target in seen:return
 seen.add(target)
 if raw is None:raw=(ESR/rel).read_bytes()
 for match in set(pattern.findall(raw)):
  ref=match.decode('ascii').replace('\\','/')
  if (ESR/ref).is_file():
   new=ref.replace('data/hd/','data/wm/',1);copy(ref,new)
   replacement=new.replace('/', '\\') if b'\\' in match else new
   assert len(replacement.encode())==len(match);raw=raw.replace(match,replacement.encode())
 out=dest/target;out.parent.mkdir(parents=True,exist_ok=True);out.write_bytes(raw)
try:
 for r in rows:
  i=r['_ordinal'];visual=r.get('_original_visual',r['_visual']);asset=visual['normal'];kind='weapon' if r['_base']['table']=='weapons' else 'armor' if r['_base']['table']=='armor' else 'misc'
  folder,leaf=asset.rsplit('/',1);private=folder+'/workshopmythical%02d'%i
  path='data/hd/items/'+kind+'/'+asset+'.json';raw=None
  if not (ESR/path).is_file():
   fallback='data/hd/items/'+kind+'/'+base[r['code']]['asset']+'.json'
   raw=(ESR/fallback).read_bytes() if (ESR/fallback).is_file() else casc.read(h,'data:'+fallback)
   fallbacks.append(dict(id=i,model=fallback))
  copy(path,'data/hd/items/'+kind+'/'+private+'.json',raw)
  for suffix in ['.sprite','.lowend.sprite']:copy('data/hd/global/ui/items/'+kind+'/'+asset+suffix,'data/hd/global/ui/items/'+kind+'/'+private+suffix)
  r['_original_visual']=visual;r['_visual']=dict.fromkeys(['normal','uber','ultra'],private)
finally:casc.d.CascCloseStorage(h)
(ROOT/'source.json').write_text(json.dumps(rows,ensure_ascii=False,indent=2),encoding='utf8')
(ROOT/'art-import.json').write_text(json.dumps(dict(source='Eastern Sun Resurrected, locally installed',url='https://easternsunresurrected.com/unique_mythicals.htm',files=len(seen),bytes=sum(p.stat().st_size for p in dest.rglob('*') if p.is_file()),modelFallbacks=fallbacks,namespace='data/wm; fixed-width dependency paths'),indent=2),encoding='utf8')
print('Imported',len(seen),'files; model fallbacks',fallbacks)
