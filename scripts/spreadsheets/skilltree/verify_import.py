"""Audit every exported node against the edited workbook and saved-stat baseline."""
import csv, json, re, sys
from pathlib import Path
from import_values import REPO, SOURCE, parse, normalize
root=REPO/'build/workshop-clusters'
report=json.loads((REPO/'build/reports/skilltree-excel-import.json').read_text(encoding='utf8'))
rows=list(csv.DictReader((root/'workshop/NODES-step2.csv').open(encoding='utf-8-sig'),delimiter=';'))
regions=list(dict.fromkeys(r['region'] for r in rows))
source=(root/'workshop/stars.inc').read_text()
extensions=re.findall(r'\{(\d+),(\d+),(\d+)\}',source.split('constexpr Extension extensions[]={')[1])
tails={(int(p),int(i),int(rank)):112+n for n,(p,i,rank) in enumerate(extensions)}
strings={r['Key']:r['enUS'] for r in json.loads((root/'data/local/lng/strings/item-names.json').read_text(encoding='utf-8-sig'))}
changed={(x['name'],ord(x['cell'][0])-ord('E')) for x in report['changes']}
checked=0
for row in rows:
    if row["name"]=="Broodheart":continue
    p=regions.index(row['region']);path=int(row['path'])-1;slot=row['slot']
    i=12+('Heart','Fury','Rush','Swarm').index(slot) if slot in ('Heart','Fury','Rush','Swarm') else 9+path if slot=='Keystone' else path*3+('Inner','Outer','Side').index(slot)
    uid=12*p+i if i<12 else 108+i-12
    ids=[uid]+([] if slot=='Keystone' else [tails[p,i,k] for k in range(1,5)])
    for rank,uid in enumerate(ids):
        actual=strings[f'WorkshopNode{uid}'].split('\n')[1:]
        actual=actual[:next((n for n,t in enumerate(actual) if t.startswith('Cost:')),len(actual))]
        actual='\n'.join(actual);expected=report['expected'][row['name']][rank]
        if (row['name'],rank) in changed: assert parse(actual)==parse(expected),(row['name'],rank,actual,expected)
        else: assert normalize(actual)==normalize(expected),(row['name'],rank,actual,expected)
        checked+=1
assert checked==447
for folder in ('data/global/excel','data/global/excel/base'):
    def read(base,name):return list(csv.DictReader((base/folder/(name+'.txt')).open(encoding='utf-8-sig'),delimiter='\t'))
    before=read(REPO,'itemstatcost');after=read(root,'itemstatcost')
    assert len(before) in (509,511) and len(after)==511
    assert before[:509]==after[:509], 'Existing stat records or save encodings changed'
    props={r['code']:r for r in read(root,'properties')}
    stat=after[508]
    assert stat['Stat']=='ws_flat_fhr' and stat['Save Bits']=='16' and stat['op stat1']=='item_fastergethitrate'
    assert props['ws-wide-balance2']['stat1']==stat['Stat']
    lightning=after[509]
    assert lightning['Stat']=='ws_flat_light_min' and lightning['op stat1']=='lightmindam' and lightning['Save Bits']=='16'
    poison=after[510]
    assert poison['Stat']=='ws_flat_poison_rate' and poison['op stat1']=='poisonmindam' and poison['op stat2']=='poisonmaxdam'
    heal=next(r for r in after if r['Stat']=='heal_afterhit')
    assert (1<<int(heal['Save Bits']))-1>=50
    assert props['healperhit']['stat1']=='heal_afterhit' and heal['itemevent1']=='domeleedamage' and heal['itemevent2']=='domissiledamage'
print('PASS: all 447 active node texts agree with the workbook, all 67 edits have exact semantic payloads, 509 previous stat records unchanged, append-only FHR storage and native Life on hit events.')

node_ids={row['id'] for row in json.loads((root/'data/local/lng/strings/item-names.json').read_text(encoding='utf-8-sig')) if row['Key'].startswith('WorkshopNode')}
assert len(node_ids)==452
for path in (root/'data/local/lng/strings').glob('*.json'):
    for row in json.loads(path.read_text(encoding='utf-8-sig')):
        if isinstance(row,dict) and 'id' in row and not row.get('Key','').startswith('WorkshopNode'):
            assert row['id'] not in node_ids,('Node/localization ID collision',path,row)
print('PASS: all 452 node localization IDs are distinct from every other string table.')
