"""Align documentation IDs with real linker row numbers without reordering saves."""
from combat.port import read,write

def normalize(directory):
 for table,key,idcol in [('properties','code','*Id'),('itemstatcost','Stat','*ID')]:
  path=directory/(table+'.txt');h,rows=read(path);index=0
  for row in rows:
   if row[key]=='Expansion':continue
   row[idcol]=str(index);index+=1
  write(path,h,rows)
