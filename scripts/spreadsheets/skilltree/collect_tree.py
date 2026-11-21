import csv
import json
import re
from pathlib import Path

repo = Path(__file__).resolve().parents[3]
root = repo / 'build/workshop-clusters'
w = root / 'workshop'
rows = list(csv.DictReader((w / 'NODES-step2.csv').open(encoding='utf-8-sig'), delimiter=';'))
design = json.loads((w / 'INNER-CLUSTERS.json').read_text())
strings = {r['Key']: r['enUS'] for r in json.loads((root / 'data/local/lng/strings/item-names.json').read_text(encoding='utf-8-sig'))}
source = (w / 'stars.inc').read_text()
extensions = re.findall(r'\{(\d+),(\d+),(\d+)\}', source.split('constexpr Extension extensions[]={')[1])
tails = {(int(p), int(i), int(rank)): 112 + n for n, (p, i, rank) in enumerate(extensions)}
regions = list(dict.fromkeys(r['region'] for r in rows))
records = []
for r in rows:
    if r["name"] == "Broodheart": continue
    p = regions.index(r['region'])
    path = int(r['path']) - 1
    slot = r['slot']
    if slot in ('Heart', 'Fury', 'Rush', 'Swarm'):
        i = 12 + ('Heart', 'Fury', 'Rush', 'Swarm').index(slot)
    elif slot == 'Keystone':
        i = 9 + path
    else:
        i = path * 3 + ('Inner', 'Outer', 'Side').index(slot)
    uid = 12 * p + i if i < 12 else 108 + i - 12
    text = strings[f'WorkshopNode{uid}'].split('\n')[1:]
    if slot == 'Keystone':
        text = text[:next((n for n, line in enumerate(text) if line.startswith('Cost:')), len(text))]
    effects = ['\n'.join(text)]
    if slot != 'Keystone':
        for rank in range(1, 5):
            effects.append('\n'.join(strings[f'WorkshopNode{tails[p, i, rank]}'].split('\n')[1:]))
    else:
        effects.extend([''] * 4)
    note = ''
    if slot == 'Inner':
        note = 'Cluster: ' + design[r['name']]['title']
    records.append([r['region'], {'Fury':1,'Rush':2,'Swarm':3}.get(slot,int(r['path'])), slot, r['name'], *effects, note])
assert len(records) == 111 and sum(1 if r[2] == 'Keystone' else 5 for r in records) == 447
swarm = next(r for r in records if r[3] == 'Swarm')
assert all('10% Minion damage' in x and '10% Minion maximum life' in x for x in swarm[5:8])
assert '+1 Maximum summons' in swarm[8] and '25% Minion maximum life' in swarm[8]
(repo / 'build/spreadsheets/skilltree/tree.json').write_text(json.dumps(records, ensure_ascii=False, indent=2), encoding='utf8')
print('Collected all 111 clusters/keystones and 447 current node effects. Swarm verified.')
