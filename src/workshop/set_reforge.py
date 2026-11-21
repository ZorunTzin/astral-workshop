"""Direct native set-to-set recipes using one uniform hidden selector per set size."""
from pathlib import Path
from collections import defaultdict
import json
from combat.port import read,write

def install(root):
 root=Path(root);catalog=None;bindings=None
 for folder in ('data/global/excel','data/global/excel/base'):
  p=root/folder;_,sets=read(p/'setitems.txt');mh,misc=read(p/'misc.txt');ch,recipes=read(p/'cubemain.txt');sh,stats=read(p/'itemstatcost.txt')
  groups=defaultdict(list)
  for row in sets:
   if row['*ID'].isdigit() and row['set'] and row['item'] and row['disabled']!='1' and row['spawnable']!='0':groups[row['set']].append(row)
  sizes=sorted({len(v) for v in groups.values()});start=len(stats);local={size:start+i for i,size in enumerate(sizes)}
  assert max(local.values())<511
  for size,statid in local.items():
   row={k:'' for k in sh};row.update(Stat=f'workshop_set_selector_{size}',**{'*ID':str(statid),'*eol':'0'});stats.append(row)
  items=[];spans=[];names={};new=[]
  for n,(name,members) in enumerate(groups.items()):
   token=f'Z{n:03X}';assert not any(r['code']==token for r in misc)
   # Keep the previous receipt codes in their original order so old saved
   # receipts can be recovered in one native transmute. New recipes never emit one.
   receipt=next(r for r in misc if r['code']=='isc').copy()
   receipt.update(code=token,name=token,namestr=token,normcode=token,ubercode='',ultracode='',type='ques',type2='',spawnable='0',useable='0',compactsave='1',cost='0',quest='',multibuy='',PermStoreItem='',pSpell='',spelldesc='',spelldescstr='',spelldescstr2='',spelldesccalc='')
   misc.append(receipt);names[token]='Legacy Set Reforge: '+name+' (cube once to recover)'
   spans.append((token,len(items),len(members)))
   for member in members:items.append((member['item'],int(member['*ID'])))
   for input_name in [r['index'] for r in members]+[token]:
    for choice,member in enumerate(members):
     row={k:'' for k in ch};row.update(description='Workshop - Direct set reforge '+input_name+' -> '+member['index'],enabled='1',version='100',numinputs='1',op='6',param=str(local[len(members)]),value=str(choice),**{'input 1':input_name,'output':member['index'],'ilvl':'100','*eol':'0'});new.append(row)
  out=(items,spans,names)
  assert catalog is None or catalog==out;catalog=out
  assert bindings is None or bindings==local;bindings=local
  write(p/'misc.txt',mh,misc);write(p/'cubemain.txt',ch,new+recipes);write(p/'itemstatcost.txt',sh,stats)
 items,spans,names=catalog
 out='// Generated set-only catalog for validation and legacy receipt identity.\nstruct SetPiece {unsigned code,row;};\nconstexpr SetPiece setPieces[]={\n'+',\n'.join('{Items::MakeItemCode("'+c+'"),'+str(i)+'}' for c,i in items)+'\n};\nstruct SetGroup {unsigned token,start,count;};\nconstexpr SetGroup setGroups[]={\n'+',\n'.join('{Items::MakeItemCode("'+c+'"),'+str(i)+','+str(k)+'}' for c,i,k in spans)+'\n};\n'
 (root/'workshop/set_catalog.inc').write_text(out)
 (root/'workshop/combat/set_selector_bindings.h').write_text('// Generated unsaved, server-only native Cube selectors.\ninline constexpr int Selectors[][2]={'+','.join('{'+str(size)+','+str(stat)+'}' for size,stat in bindings.items())+'};\n')
 path=root/'data/local/lng/strings/item-names.json';rows=json.loads(path.read_text(encoding='utf-8-sig'));last=max(r['id'] for p in path.parent.glob('*.json') for r in json.loads(p.read_text(encoding='utf-8-sig')) if 'id' in r)
 for key,text in names.items():last+=1;rows.append({'id':last,'Key':key,**{k:text for k in rows[0] if k not in ('id','Key')}})
 path.write_text(json.dumps(rows,ensure_ascii=False,indent=4),encoding='utf8')
 print(f'Direct native set reroll: {len(items)} pieces / {len(spans)} sets; {len(bindings)} unsaved selectors; no receipt output.')
