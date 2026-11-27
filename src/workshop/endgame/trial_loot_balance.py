"""Trial-only Uber materials: equal category odds, preserving native Lab tables."""
from pathlib import Path
from combat.port import read,write

def install(root):
 for folder in ('data/global/excel','data/global/excel/base'):
  p=Path(root)/folder/'treasureclassex.txt';h,rows=read(p);by={r['Treasure Class']:r for r in rows}
  def set_tc(name,nodrop,items):
   row={k:'' for k in h};row.update({'Treasure Class':name,'Picks':'1','NoDrop':str(nodrop),'*eol':'0'})
   for i,(item,weight) in enumerate(items,1):row['Item'+str(i)]=item;row['Prob'+str(i)]=str(weight)
   if name in by:by[name].clear();by[name].update(row)
   else:rows.append(row);by[name]=row
  set_tc('Ws Trial Shards',0,[(f'xa{i}',w) for i,w in enumerate((10,6,4,3,2),1)])
  set_tc('Ws Trial Keys',0,[(f'pk{i}',1) for i in range(1,4)])
  set_tc('Ws Trial Statues',0,[(f'ua{i}',1) for i in range(1,6)])
  # Each group: 1/500 at players 1, before Trial bonus rolls.
  # Ten times the former chance for each material category.
  set_tc('Ws Trial Materials',497,[(name,1) for name in ('Ws Trial Shards','Ws Trial Keys','Ws Trial Statues')])
  ordered=[];done=set();visiting=set()
  def emit(name):
   if name in done:return
   assert name not in visiting,('TC cycle',name)
   visiting.add(name);row=by[name]
   for i in range(1,11):
    child=row['Item'+str(i)]
    if child in by and child.startswith(('Ws ','Workshop ')):emit(child)
   visiting.remove(name);done.add(name);ordered.append(row)
  for row in rows:emit(row['Treasure Class'])
  write(p,h,ordered)
 print('Trials: Worldstone Shards, Uber keys and Ancient statues each have equal rare group odds.')
