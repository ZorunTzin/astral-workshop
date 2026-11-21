"""Targeted equipment buffs and armor-only maximum-resistance suffixes."""
import json,re
from pathlib import Path
from combat.port import read,write
PALADIN={
 'uniqueitems':{"Steelclash": [('ws-aura-effect',3,5)], 'Twilight Presence':[('ws-aura-effect',6,10)], 'Defender of Innocence':[('ws-reservation-efficiency',6,10)], "Ancients' Epiphany":[('ws-aura-effect',10,15)]},
 'setitems':{"Milabrega's Orb":[('ws-reservation-efficiency',3,5)],"Milabrega's Rod":[('ws-aura-effect',3,5)],"Greyhawk's Deflector":[('ws-aura-effect',4,7)],'Prayer of Brother Laz':[('ws-reservation-efficiency',6,10)]}}
RESISTS={
 'uniqueitems':{'Coif of Glory':(1,2),'Duskdeep':(1,2),'Venom Ward':(2,3),'Hawkmail':(2,3),'The Ward':(3,5)},
 'setitems':{"Civerb's Ward":(1,2),"Isenhart's Case":(1,2),'Infernal Cranium':(1,2),"Milabrega's Robe":(2,3)}}
FORGES={'Nadir':(1,2),'Radiance':(2,3),'Myth':(3,5)}

def install(root):
 root=Path(root);audit=[]
 for folder in ('data/global/excel','data/global/excel/base'):
  d=root/folder
  for table in ('uniqueitems','setitems'):
   h,rows=read(d/(table+'.txt'));by={r['index']:r for r in rows}
   choices={k:list(v) for k,v in PALADIN[table].items()}
   for name,(lo,hi) in RESISTS[table].items():choices.setdefault(name,[]).append(('res-all-max',lo,hi))
   for name,effects in choices.items():
    row=by[name]
    for code,lo,hi in effects:
     slots=range(1,13 if table=='uniqueitems' else 10)
     n=next((i for i in slots if row['prop'+str(i)]==code),None)
     if n is None:n=next(i for i in slots if not row['prop'+str(i)])
     row.update({f'prop{n}':code,f'par{n}':'',f'min{n}':str(lo),f'max{n}':str(hi)})
     if folder.endswith('/excel'):audit.append(dict(table=table,item=name,property=code,min=lo,max=hi))
   write(d/(table+'.txt'),h,rows)
  h,rows=read(d/'magicsuffix.txt')
  previous=[r for r in rows if r['name'].startswith('WorkshopMaxRes')]
  group=previous[0]['group'] if previous else str(max(int(r['group'] or 0) for r in rows)+1)
  rows=[r for r in rows if not r['name'].startswith('WorkshopMaxRes')]
  for tier,(level,required) in enumerate([(10,7),(25,18),(40,30),(60,45),(80,60)],1):
   r={k:'' for k in h};r.update(name='WorkshopMaxRes'+str(tier),version='100',spawnable='1',rare='1',level=str(level),levelreq=str(required),frequency='4',group=group,mod1code='res-all-max',mod1min=str(tier),mod1max=str(tier),itype1='armo',multiply='0',add='0');rows.append(r)
  write(d/'magicsuffix.txt',h,rows)
 p=root/'data/local/lng/strings/item-names.json';rows=json.loads(p.read_text(encoding='utf-8-sig'))
 highest=max(r['id'] for f in p.parent.glob('*.json') for r in json.loads(f.read_text(encoding='utf-8-sig')) if 'id' in r)
 for tier in range(1,6):
  key='WorkshopMaxRes'+str(tier)
  if not any(r['Key']==key for r in rows):
   highest+=1;rows.append(dict(id=highest,Key=key,**{l:'of Warding' for l in rows[0] if l not in ('id','Key')}))
 p.write_text(json.dumps(rows,ensure_ascii=False,indent=2),encoding='utf8')
 # The forge compiler runs earlier. Update its documented terms and generated
 # bundles together, preserving word identities and every recipe selector.
 wp=root/'workshop/FORGING.json';words=json.loads(wp.read_text(encoding='utf8'))
 for w in words:
  if w['name'] not in FORGES:continue
  assert all(t in ('helm','tors') for t in w['includeTypes'])
  lo,hi=FORGES[w['name']]
  idx=next((i for i,t in enumerate(w['terms']) if t['code']=='res-all-max'),None)
  if idx is None:
   idx=len(w['terms']);w['terms'].append(dict(code='res-all-max',param='',lo=float(lo),hi=float(hi)))
   for key in ('original','magic','rare','unique','core'):w[key]+='\n+'+str(lo)+'-'+str(hi)+'% to All Maximum Resistances'
  if idx not in w['coreIndices']:w['coreIndices'].append(idx)
 wp.write_text(json.dumps(words,ensure_ascii=False,indent=2),encoding='utf8')
 _,props=read(root/'data/global/excel/properties.txt');pid=int(next(r['*Id'] for r in props if r['code']=='res-all-max'))
 ap=root/'workshop/FORGING-audit.json';bundles=json.loads(ap.read_text(encoding='utf8'))
 for b in bundles:
  if b['name'] not in FORGES:continue
  lo,hi=FORGES[b['name']]
  expected=[pid,0,lo,hi]
  matches=[e for e in b['effects'] if e[0]==pid]
  if matches:assert matches==[expected]
  else:b['effects'].insert(-2,expected)
 ap.write_text(json.dumps(bundles,ensure_ascii=False,indent=2),encoding='utf8')
 flat=[];spans=[]
 for b in sorted(bundles,key=lambda b:b['bundle']):
  spans.append([len(flat),len(b['effects'])]);flat.extend(b['effects'])
 inc=root/'workshop/forge.inc';text=inc.read_text(encoding='utf8')
 for name,values in [('forgeEffects',flat),('forgeSpans',spans)]:
  payload=',\n'.join('{'+','.join(map(str,row))+'}' for row in values)
  text,n=re.subn(r'('+name+r'\[\]=\{\n).*?(\n\};)',lambda m:m[1]+payload+m[2],text,flags=re.S);assert n==1
 inc.write_text(text,encoding='utf8')
 (root/'workshop/EQUIPMENT-BUFFS.json').write_text(json.dumps(dict(items=audit,forges=FORGES,affixBaseMaximum=5,magicMaximum=10,rareMaximum=6),indent=2),encoding='utf8')
 print('Equipment buffs: '+str(len(audit))+' item bonuses, 3 armor forges, 5 armor-only resistance suffixes.')
if __name__=='__main__':
 import sys
 install(Path(sys.argv[1]))
