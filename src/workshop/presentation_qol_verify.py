"""Regressions for static tooltip format strings, names and new Mastery tables."""
from pathlib import Path
import sys,json,re
from combat.port import read
from leagues.crafting import describe
r=Path(sys.argv[1]);strings={x['Key']:x['enUS'] for x in json.loads((r/'data/local/lng/strings/item-names.json').read_text(encoding='utf8'))}
for m in range(3):
 for t in range(4):
  shown=strings[f'LeagueOrbHelp{m}{t}']%()
  for g in range(3):assert describe(m,g,t) in shown
for folder in ('data/global/excel','data/global/excel/base'):
 _,rows=read(r/folder/'charstats.txt');assert all('bag' not in [x.get('item'+str(i)) for i in range(1,11)] for x in rows)
 _,rows=read(r/folder/'skills.txt');masteries=[x for x in rows if x['skill'] in ('Spear Mastery','Bow Mastery','Javelin Mastery')]
 assert len(masteries)==3 and all(x['reqlevel']=='1' and x['SrcDam']=='0' and not x['reqskill1'] for x in masteries)
 assert len({x['aurastate'] for x in masteries})==3
c=(r/'workshop/stash_organizer_catalog.h').read_text();assert '\\u00ffc' not in c and '*c9' not in c
for p in (r/'data/local/lng/strings').glob('*.json'):
 for row in json.loads(p.read_text(encoding='utf-8-sig')):
  assert not any(isinstance(v,str) and re.search('reimagined',v,re.I) for k,v in row.items() if k not in ('Key','id')),p
print('PASS: all 12 orb descriptions format safely, bonuses retained, clean catalog/branding, no starter bags and three level-one Masteries.')
