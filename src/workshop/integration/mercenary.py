"""Install the pinned local Frankenstein mercenary charm integration."""
from pathlib import Path
import hashlib,json,shutil
HERE=Path(__file__).resolve().parent
SOURCE=HERE/'upstream/mercenary'

def install(root):
 root=Path(root)
 manifest=json.loads((SOURCE/'PROVENANCE.json').read_text())
 for rel,digest in manifest['files'].items():
  assert hashlib.sha256((SOURCE/rel).read_bytes()).hexdigest()==digest, rel
 plugin=(SOURCE/'fmmp-mercenary-charms.dll').read_bytes()
 old=b'FMMP-Experimental-R67\0';new=b'ReimaginedWorkshop\0'
 assert plugin.count(old)==1
 plugin=plugin.replace(old,new+b'\0'*(len(old)-len(new)))
 path=root/'d2rloader/plugins/fmmp-mercenary-charms.dll';path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(plugin)
 path=root/'d2rloader/config/fmmp-mercenary-charms.toml';path.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(SOURCE/path.name,path)
 assets=[]
 # Private art paths avoid replacing the player's inventory or shared UI art.
 art={'panel/hireling/hirelingpanel':'Workshop/mercenary/hirelingpanel','panel/inventory/background_expanded2':'Workshop/mercenary/background_expanded2'}
 for rel in manifest['files']:
  if not rel.startswith('data/'):continue
  raw=(SOURCE/rel).read_bytes();target=rel
  if rel.endswith('.json'):
   layout=json.loads(raw.decode('utf-8-sig'))
   def walk(node):
    if isinstance(node,dict):
     for key,value in list(node.items()):
      if key=='filename' and isinstance(value,str):
       normalized=value.replace(chr(92),'/').strip('/').lower()
       normalized='/'.join(part for part in normalized.split('/') if part)
       if normalized in art:node[key]=art[normalized]
      else:walk(value)
    elif isinstance(node,list):
     for child in node:walk(child)
   walk(layout)
   raw=json.dumps(layout,indent=2).encode()
  else:
   for old_art,new_art in art.items():
    target=target.replace('data/hd/global/ui/'+old_art,'data/hd/global/ui/'+new_art)
  path=root/target;path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(raw);assets.append(target)
 path=root/'workshop/integration/mercenary-assets.json';path.parent.mkdir(parents=True,exist_ok=True);path.write_text(json.dumps(assets,indent=2))
 return assets

if __name__=='__main__':
 import sys
 install(sys.argv[1])
