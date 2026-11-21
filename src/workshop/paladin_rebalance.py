"""Apply the requested aura-effect nerf independently of mana reservation.

Run after earlier Paladin additions so the later 1%-based effects can be
protected by explicit skill/slot identity rather than heuristic coefficients.
"""
import json,re
from pathlib import Path
from combat.port import read,write
from combat.aura_reservation import TIERS

TARGETS=(98,102,104,108,109,113,114,115,118,119,120,122,123,124,125)
KEEP={98:{2},104:{2},108:{2},123:{5},125:{4,5,6}}
DAMAGE=(102,114,118,119)

def install(root):
 root=Path(root);audit=[]
 for folder in ('data/global/excel','data/global/excel/base'):
  p=root/folder;h,skills=read(p/'skills.txt');dh,descs=read(p/'skilldesc.txt')
  by={int(r['*Id']):r for r in skills if r['*Id'].isdigit()};ds={r['skilldesc']:r for r in descs}
  for skill in TARGETS:
   row=by[skill];before=row.copy();desc=ds[row['skilldesc']]
   for n in range(1,7):
    field='aurastatcalc'+str(n);old=row[field]
    if old and n not in KEEP.get(skill,set()) and not re.fullmatch(r'ast[1-6]',old):row[field]='('+old+')/2'
   if skill in DAMAGE:
    for field in ('EMin','EMax')+tuple('E'+b+'Lev'+str(n) for b in ('Min','Max') for n in range(1,6)):
     if row[field]:row[field]=str(int(row[field])//2)
   # Native passive effects outside the damage tables must be reduced too.
   for n in range(1,15):
    field='passivecalc'+str(n)
    if not row.get(field):continue
    if skill==119 and n in (1,2) or skill==122 and n==1 or skill==118 and n==1:
     row[field]='('+row[field]+')/2'
   if skill==124:
    for n in (1,2,3):row['calc'+str(n)]='('+row['calc'+str(n)]+')/2'
   if skill==109:
    for n in (1,2):row['calc'+str(n)]='('+row['calc'+str(n)]+')/2'
   # Stat/passive/calculator references automatically read reduced effects.
   # Direct native ln/dm/tohit tooltip formulas still need the same factor.
   for prefix,slots in (('desc',6),('dsc2',4),('dsc3',8)):
    for n in range(1,slots+1):
     field=prefix+'calca'+str(n);label=desc.get(prefix+'texta'+str(n),'');formula=desc.get(field,'')
     if label.startswith('Workshop') or desc.get(prefix+'line'+str(n))!='74' or not formula:continue
     if not re.search(r'\b(?:ast[1-6]|pst[1-9]|clc[1-6])\b',formula):desc[field]='('+formula+')/2'
   for n in KEEP.get(skill,set()):assert row['aurastatcalc'+str(n)]==before['aurastatcalc'+str(n)]
   audit.append({'table':folder,'skill':row['skill'],'id':skill,'before':{k:v for k,v in before.items() if k.startswith(('aurastatcalc','EMin','EMax','passivecalc','calc')) and v},'after':{k:v for k,v in row.items() if k.startswith(('aurastatcalc','EMin','EMax','passivecalc','calc')) and v}})
  # Reservation changes include Prayer and Retribution, while the effect nerf
  # excludes them. Both runtime and tooltip use the exact 150/(100+eff) ratio.
  for skill in (98,99,102,103,104,108,109,113,114,115,118,119,120,122,123,124,125):
   row=by[skill];desc=ds[row['skilldesc']];base,growth=TIERS[int(row['reqlevel'])]
   for key,value in list(desc.items()):
    if value=='WorkshopAuraReservedValue' and 'texta' in key:
     desc[key.replace('texta','calca')]=f"max(1,({base}+{growth}*lvl)*150/(100+stat('workshop_reservation_efficiency'.accr)))"
  write(p/'skills.txt',h,skills);write(p/'skilldesc.txt',dh,descs)
 (root/'workshop/PALADIN-REBALANCE.json').write_text(json.dumps(audit,indent=2),encoding='utf8')
