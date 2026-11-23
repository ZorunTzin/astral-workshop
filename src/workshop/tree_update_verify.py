"""Audit the repaired summon wiring, fixed tooltips, Wind and HUD layout."""
from pathlib import Path
import json,csv
from combat.port import read

root=Path(__file__).resolve().parent.parent
for folder in ('data/global/excel','data/global/excel/base'):
    p=root/folder;_,skills=read(p/'skills.txt');by={r['skill']:r for r in skills}
    for row in skills:
        if row['charclass'] and row['pettype'] and row['petmax']:
            if row['skill']=='Call of the Ancients':
                assert row['petmax']=='3';continue
            assert row['petmax'].count("stat('ws_pet_count'.accr)")==1,row['skill']
    assert "ws_pet_life" in by['Resist Lightning']['calc1']
    assert "ws_pet_damage" in by['Resist Lightning']['calc3']
    _,monsters=read(p/'monstats.txt');m=next(r for r in monsters if r['Id']=='workshop_astral_guardian')
    original=next(r for r in monsters if r['Id']=='valkyrie')
    for k in ('minHP','maxHP','MinHP(N)','MaxHP(N)','MinHP(H)','MaxHP(H)'):
        assert int(m[k])==(int(original[k])//3)//2
    _,descs=read(p/'skilldesc.txt');d=next(r for r in descs if r['skilldesc']==by['Resist Lightning']['skilldesc'])
    assert d['descline1']=='36' and d['desccalca1']=='80*(100+clc1)/100'
    for line in (2,3):
        formula=d['desccalca'+str(line)]
        assert 'stat(' not in formula and 'clc3' in formula and formula.endswith('/1000')
strings={r['Key']:r['enUS'] for r in json.loads((root/'data/local/lng/strings/item-names.json').read_text(encoding='utf-8-sig'))}
tips=[v for k,v in strings.items() if k.startswith('WorkshopNode')]
assert len(tips)==452
blood=next(t for t in tips if t.startswith('Berserker\n'))
assert '+1 to Rage (All Classes)' in blood and 'Life on kill for' not in blood
assert 'Cost: 1 point' in blood and '30 / 60 / 90' in blood
nodes=list(csv.DictReader((root/'workshop/NODES.csv').open(encoding='utf-8-sig'),delimiter=';'))
assert next(r for r in nodes if r['name']=='Wind')['property']=='balance2'
assert all('Stamina' not in t for t in tips if t.startswith('Wind'))
layout=json.loads((root/'data/global/ui/layouts/buffhud/BuffHudhd.json').read_text())
grid=next(c for c in layout['children'] if c['name']=='BuffGrid')
assert layout['fields']['anchor']['x']==1.0 and grid['fields']['rect']['x']==-700 and sum(c['name'].startswith('BuffSlot') for c in grid['children'])==21
_,buffs=read(root/'data/global/excel/d2rloader/buffhud/buff-hud.txt')
assert any(r['skill_id']=='103' for r in buffs) and any(r['skill_id']=='109' for r in buffs)
print('PASS: all data-driven summon limits, guardian minion stats and one-sixth HP, fixed pre-summon tooltip formulas, 452 item-style node descriptions, Wind FHR, one-point Keystone level rules and bottom-right buff catalog.')
