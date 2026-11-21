"""Audit both game modes and every reachable prototype allocation."""
from pathlib import Path
import csv
import json
root=Path(__file__).resolve().parents[1]
for directory in (root/'data/global/excel',root/'data/global/excel/base'):
    def rows(name):
        with (directory/(name+'.txt')).open(encoding='utf-8-sig',newline='') as file:
            return list(csv.DictReader(file,delimiter='\t'))
    recipes=[r for r in rows('cubemain') if r['description'].startswith('Workshop - ')]
    props={r['code'] for r in rows('properties')}
    misc={r['code'] for r in rows('misc')}
    marker=next(r['*ID'] for r in rows('itemstatcost') if r['Stat']=='workshop_forged')
    audit=json.loads((root/'workshop/FORGING-audit.json').read_text(encoding='utf8'))
    assert len(recipes)>2000
    masks=set()
    for row in recipes:
        for i in range(1,6):
            prop=row[f'mod {i}']
            assert not prop or prop in props,(row['description'],prop)
        if row['input 1']=='Wsig':
            mask=int(row['input 2'][1:],16)
            masks.add(mask)
            assert row['input 2'] in misc and int(row['value'])==mask.bit_count()+1
            assert (row['op'],row['param'],row['output'])==('3','12','WorkshopAstralSigil')
            assert all(not mask>>i&1 or mask>>(i-i%3)&1 for i in range(9))
            mods={row[f'mod {i}']:(row[f'mod {i} param'],row[f'mod {i} min'],row[f'mod {i} max']) for i in range(1,6)}
            key=mask>>9
            assert key in (0,1,2,4)
            if key:
                idx=key.bit_length()-1
                assert mask & (1<<(1+3*idx))
                cost,prop,effect=[('hp','hit-skill',('Life Tap','5','7')),('mana','aura',('Meditation','3','3')),('res-all','oskill',('Teleport','1','1'))][idx]
                assert cost not in mods and mods[prop]==effect
    assert len(masks)==275 and {0,1,8,64,511}<=masks
    chart=next(r for r in recipes if r["output"]=="Wcap")
    assert chart["plvl"]=="100" and [chart[f"input {i}"] for i in (1,2,3)]==["isc","tsc","hp1"]
    assert next(r for r in rows("misc") if r["code"]=="Wcap")["compactsave"]=="0"
    with (root/'workshop/NODES.csv').open(encoding='utf-8-sig') as f:
        nodes=list(csv.DictReader(f,delimiter=';'))
    assert len(nodes)==112 and len({n['name'] for n in nodes})==112
    assert all(n['property'] in props or n['property']=='ws-minion-limit' for n in nodes)
    assert all(int(n["minimum"]) == int(n["maximum"]) for n in nodes if n["slot"] != "Keystone")
    assert next(r['compactsave'] for r in rows('misc') if r['code']=='Wpln')=='0'
    for i in range(9):
        prop=next(r for r in rows("properties") if r["code"]==f"ws-page{i}")
        stat=next(r for r in rows("itemstatcost") if r["Stat"]==prop["stat1"])
        assert stat["Save Bits"]=="12" and int(stat["*ID"])<511
    forge=[r for r in recipes if r['description'].startswith('Workshop - Forge ')]
    assert len(forge)==len(audit)
    selections=json.loads((root/'workshop/FORGING.json').read_text(encoding='utf8'))
    assert len(selections)==220
    assert {a['seal'] for a in audit}==set(json.loads((root/'workshop/FORGING-seals.json').read_text(encoding='utf8')))
    identity_by_seal={ {'Lore':'Wlor','Stealth':'Wstl'}.get(w['name'],f'F{i:03X}'):i for i,w in enumerate(selections)}
    for row,a in zip(forge,audit):
        assert row['output']=='useitem' and row['op']=='18' and row['param']==marker and row['value']=='0'
        assert row['mod 1']=='ws-forge-dispatch' and int(row['mod 1 param'])==a['bundle']
        assert ',nru' in row['input 1'] and row['input 2'] in misc
        assert 2<len(a['effects'])<=16
        identity=identity_by_seal[a['seal']]
        assert a['effects'][-1][2:]==[2+(identity>>7)]*2
        assert a['effects'][-2][2:]==[identity&127]*2
    direct=[r for r in recipes if r['description'].startswith('Workshop - Direct ') and r['mod 1']=='ws-forge-dispatch']
    assert len(direct)==len(audit)
    audit_by_bundle={a['bundle']:a for a in audit}
    for row in direct:
        a=audit_by_bundle[int(row['mod 1 param'])]
        assert row['output']=='useitem' and ',nru' in row['input 1']
        assert row['op']=='18' and row['param']==marker and row['value']=='0'
        ingredients=[row[f'input {i}'] for i in range(2,8) if row[f'input {i}']]
        assert all(x.split(',')[0].startswith('r') for x in ingredients)
        total=1+sum(int(x.split('qty=')[-1]) if 'qty=' in x else 1 for x in ingredients)
        assert total==int(row['numinputs'])<=7
    prepares=[r for r in recipes if r['description'].startswith('Workshop - Prepare ')]
    for row in prepares:
        total=0
        for i in range(1,8):
            field=row[f'input {i}']
            if field:total+=int(field.split('qty=')[-1]) if 'qty=' in field else 1
        assert total==int(row['numinputs'])<=7
    assert {a['quality'] for a in audit}=={'nor','hiq','mag','rar','crf','set','uni'}
print('PASS: Astral regression checks and all 220 runeword variants in both table sets.')

# Full-strength unique/set core and explicit thrown-weapon support.
for name,code in [('Lore','allskills'),('Authority',None)]:
 word=next(w for w in selections if w['name']==name)
 for quality in ('set','uni'):
  entry=next(a for a in audit if a['name']==name and a['quality']==quality)
  assert len(entry['effects'])==len(word['coreIndices'])+2
  if name=='Lore':assert entry['effects'][0]==[104,0,1,1]
for word in selections:
 if 'miss' in word['includeTypes']:
  assert 'thro' in word['includeTypes']
  assert all(any(a['name']==word['name'] and a['baseType']=='thro' and a['quality']==q for a in audit) for q in ('set','uni','rar','mag','nor'))
print('PASS: Lore/Authority full-strength set/unique cores and missile-word recipes for thrown weapons.')
