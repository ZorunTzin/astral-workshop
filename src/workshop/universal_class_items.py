"""Open class equipment and bind conditional native property conversion."""
from pathlib import Path
import json
from combat.port import read,write

def install(root):
 root=Path(root)
 for folder in ('data/global/excel','data/global/excel/base'):
  d=root/folder;h,types=read(d/'itemtypes.txt');by={r['Code']:r for r in types}
  def ancestors(code,seen=None):
   seen=set() if seen is None else seen
   if not code or code in seen:return seen
   seen.add(code)
   if code in by:
    for k in ('Equiv1','Equiv2'):ancestors(by[code][k],seen)
   return seen
  codes=[]
  for table in ('armor','weapons'):
   _,rows=read(d/(table+'.txt'))
   for r in rows:
    if any('clas' in ancestors(r.get(k,'')) for k in ('type','type2')):codes.append(r['code'])
  for r in types:
   if 'clas' in ancestors(r['Code']):r['Class']=''
  write(d/'itemtypes.txt',h,types)
  _,props=read(d/'properties.txt');lookup={r['code']:r for r in props}
  mappings=[]
  for r in props:
   if r['stat1']=='item_addclassskills' and r['func1']=='21':mappings.append((int(r['*Id']),int(lookup['allskills']['*Id'])))
   if r['stat1']=='item_singleskill' and r['func1']=='22':
    assert r['func1']=='22',r
    mappings.append((int(r['*Id']),int(lookup['oskill']['*Id'])))
  if folder.endswith('/excel'):
   out='// Generated class-equipment property bindings.\n'
   out+='constexpr unsigned universalClassItems[]={'+','.join(str(int.from_bytes(c.ljust(4).encode('ascii'),'little')) for c in sorted(set(codes)))+'};\n'
   out+='constexpr unsigned universalClassProperties[][2]={'+','.join('{'+str(a)+','+str(b)+'}' for a,b in mappings)+'};\n'
   (root/'workshop/universal_class_items.inc').write_text(out,encoding='utf8')
   (root/'workshop/UNIVERSAL-CLASS-ITEMS.json').write_text(json.dumps(dict(itemCodes=sorted(set(codes)),propertyMappings=mappings,staffmods='Separate native generation path; not covered by property dispatch'),indent=2),encoding='utf8')
 print('Class equipment unlocked; class skills / single-skill properties mapped conditionally.')
if __name__=='__main__':
 import sys
 install(Path(sys.argv[1]))
