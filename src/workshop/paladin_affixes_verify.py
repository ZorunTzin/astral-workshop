from pathlib import Path
import sys
from combat.port import read
from paladin_affixes import EFFECTS, TIERS
root=Path(sys.argv[1])
for folder in ('data/global/excel','data/global/excel/base'):
    p=root/folder
    _,stats=read(p/'itemstatcost.txt'); _,props=read(p/'properties.txt'); _,affixes=read(p/'magicsuffix.txt')
    assert len(stats)==511
    groups=set()
    for stat,code,key,_,_ in EFFECTS:
        s=next(r for r in stats if r['Stat']==stat)
        assert s['Save Bits']=='16' and s['Send Bits']=='32' and s['descstrpos']==key
        assert int(s['*ID'])=={'workshop_aura_effect':497,'workshop_reservation_efficiency':496}[stat]
        prop=next(r for r in props if r['code']==code)
        assert prop['stat1']==stat and prop['func1']=='1'
        rows=[r for r in affixes if r['mod1code']==code]
        assert len(rows)==5 and len({r['group'] for r in rows})==1
        assert rows[0]['group'] not in groups
        groups.add(rows[0]['group'])
        for r,(level,required,lo,hi) in zip(rows,TIERS):
            assert (int(r['level']),int(r['levelreq']),int(r['mod1min']),int(r['mod1max']))==(level,required,lo,hi)
            assert r['itype1']=='pala' and r['rare']=='1' and r['spawnable']=='1'
            assert not any(r.get('itype'+str(i)) for i in range(2,8))
    assert all(sum(r['group']==g for r in affixes)==5 for g in groups)
import json
rules=json.loads((root/'workshop/AFFIX-SCALING.json').read_text(encoding='utf8'))
assert all(next(r for r in rules if r['code']==code)['kind']==0 for _,code,*_ in EFFECTS)
print('PASS: Paladin affix tiers, eligibility, groups, stat IDs, save fields and scaling exclusions.')
