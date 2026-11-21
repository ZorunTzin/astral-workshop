"""Native TC wrappers: preserve base drops and add weighted Trial loot."""
from pathlib import Path
import json
from combat.port import read,write
def install(root):
 root=Path(root);assets=[]
 for folder in ('data/global/excel','data/global/excel/base'):
  p=root/folder;h,rows=read(p/'treasureclassex.txt');existing={r['Treasure Class'] for r in rows}
  def add(name,picks,nodrop,items):
   previous=next((x for x in rows if x['Treasure Class']==name),None)
   if previous is not None:rows.remove(previous)
   r={k:'' for k in h};r.update({'Treasure Class':name,'Picks':str(picks),'NoDrop':str(nodrop),'*eol':'0'})
   for i,(item,weight) in enumerate(items,1):r['Item'+str(i)]=item;r['Prob'+str(i)]=str(weight)
   rows.append(r);existing.add(name)
  add('Ws Trial Runes',1,3980,[('Runes 17',20)])
  add('Ws Trial Materials',1,24975,[('Lab T1 Materials',10),('Lab T2 Materials',6),('Lab T3 Materials',4),('Lab T4 Materials',3),('Lab T5 Materials',2)])
  add('Ws Trial Special',1,495,[('Lab Jewelry',5)])
  mh,mon=read(p/'monstats.txt');mapping={}
  for r in mon:
   if not r['Id'].startswith('wsarenamon'):continue
   for k,v in r.items():
    if not k.startswith('TreasureClass') or not v or v.startswith('Ws Trial Base '):continue
    if v not in mapping:
     name='Ws Trial Base '+v;mapping[v]=name;add(name,-4,0,[(v,1),('Ws Trial Runes',1),('Ws Trial Materials',1),('Ws Trial Special',1)])
    r[k]=mapping[v]
  write(p/'treasureclassex.txt',h,rows);write(p/'monstats.txt',mh,mon)
  assets.extend([folder+'/treasureclassex.txt',folder+'/monstats.txt'])
 return assets
