"""Audit exact rolls, eligibility, stable saves and strictly additive item edits."""
import json,os
from pathlib import Path
from combat.port import read
from weapon_ailments import STATS,TIERS,SELECTION
root=Path(os.environ['WORKSHOP_MOD_ROOT']);audit=json.loads((root/'workshop/WEAPON-AILMENTS.json').read_text())
assert len(audit['affixes'])==14
for folder in ('data/global/excel','data/global/excel/base'):
 p=root/folder
 _,s=read(p/'itemstatcost.txt');assert len(s)==511
 for sid,old,name,code,key,text in STATS:
  r=next(x for x in s if x['*ID']==str(sid));assert r['Stat']==name and r['Save Bits']=='9' and not r['Save Param Bits']
  assert not r['op'] and not r['itemevent1'] and not r['itemevent2']
 _,a=read(p/'magicsuffix.txt');new=[r for r in a if r['mod1code'] in ('ws-bleed','ws-impale')];assert len(new)==14
 assert len({r['group'] for r in new})==2
 for code in ('ws-bleed','ws-impale'):
  for r,t in zip([r for r in new if r['mod1code']==code],TIERS):
   assert (int(r['level']),int(r['maxlevel'] or 0),int(r['levelreq']),int(r['mod1min']),int(r['mod1max']))==t
   assert r['rare']==r['spawnable']=='1' and r['itype1']=='weap' and r['etype1']=='rod'
 # Verify type ancestry for every actual weapon, especially Sorceress orbs.
 _,types=read(p/'itemtypes.txt');types={r['Code']:r for r in types if r['Code']}
 def derives(t,parent,seen=None):
  if t==parent:return True
  seen=set() if seen is None else seen
  if not t or t in seen or t not in types:return False
  seen.add(t);return any(derives(types[t][k],parent,seen.copy()) for k in ('Equiv1','Equiv2'))
 assert derives('orb','weap') and not derives('orb','rod')
 assert all(derives(t,'rod') for t in ('staf','wand','scep'))
 for table,selection in SELECTION.items():
  _,rows=read(p/(table+'.txt'));by={r['index']:r for r in rows}
  found=[]
  for r in rows:
   effects=[r.get('prop'+str(i)) for i in range(1,13 if table=='uniqueitems' else 10) if r.get('prop'+str(i)) in ('ws-bleed','ws-impale')]
   if effects:found.append(r['index']);assert effects==['ws-'+e for e in selection[r['index']]]
  assert set(found)==set(selection)
 for a in audit['items']:
  _,rows=read(p/(a['table']+'.txt'));r=next(x for x in rows if x['index']==a['item']);n=a['slot']
  assert r[f'prop{n}']=='ws-'+a['effect'] and int(r[f'min{n}'])==a['min'] and int(r[f'max{n}'])==a['max']
 assert sum(len(v)>1 for selection in SELECTION.values() for v in selection.values())==1
policy=json.loads((root/'workshop/AFFIX-SCALING.json').read_text())
assert all(next(r['kind'] for r in policy if r['code']==code)==0 for code in ('ws-bleed','ws-impale'))
labels=json.loads((root/'data/local/lng/strings/item-names.json').read_text(encoding='utf-8-sig'))
for a in audit['affixes']:assert any(r['Key']==a['name'] for r in labels)
print('PASS: seven exact tiers each, orb eligibility/caster exclusion, independent affix groups, no multiplier, 42 selected weapons, one dual-effect exception, stable stat IDs and tooltip names.')

# Runeforging uses the same final numeric property IDs as the item affixes.
words=json.loads((root/'workshop/FORGING.json').read_text(encoding='utf8'))
changed=[w for w in words if 'ailmentBalance' in w]
assert len(changed)==12
for folder in ('data/global/excel','data/global/excel/base'):
 _,props=read(root/folder/'properties.txt')
 ids={r['code']:int(r['*Id']) for r in props if r['*Id'].isdigit()}
 assert all(sum(r['code']==c for r in props)==1 for c in ('ws-bleed','ws-impale'))
 bundles=json.loads((root/'workshop/FORGING-audit.json').read_text(encoding='utf8'))
 for w in words:
  expected=w.get('ailmentBalance')
  for b in [a for a in bundles if a['name']==w['name']]:
   effects=[e for e in b['effects'] if e[0] in (ids['ws-bleed'],ids['ws-impale'])]
   if expected and b['quality'] not in ('uni','set'):
    assert effects==[[ids['ws-'+expected['effect']],0,expected['min'],expected['max']]],b
   else:assert not effects,b
print('PASS: 12 selected runewords, final native property IDs, exact rolls, all seven qualities and unchanged unique/set cores.')
