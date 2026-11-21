"""Regression: intrinsic aura damage matches the former fully synergized curve."""
import json,sys
from fractions import Fraction
from pathlib import Path
try:
    from .port import read
except ImportError:
    from port import read
here=Path(__file__).resolve().parent
reports=json.loads((here/'AURA-DAMAGE.json').read_text())
def damage(row,bound,level):
    total=int(row['E'+bound] or 0);remaining=level-1
    for band,count in enumerate((7,8,6,6,1000),1):
        used=min(remaining,count);total+=used*int(row['E'+bound+'Lev'+str(band)] or 0);remaining-=used
    return Fraction(total*(1<<int(row['HitShift'])),256)
root=Path(sys.argv[1])
for folder in ('data/global/excel','data/global/excel/base'):
    _,skills=read(root/folder/'skills.txt');_,descs=read(root/folder/'skilldesc.txt')
    for report in reports:
        row=next(r for r in skills if r['skill']==report['skill'])
        assert row['EDmgSymPerCalc']=="stat('workshop_aura_effect'.accr)" and not row['Param7'] and not row['Param8']
        desc=next(r for r in descs if r['skilldesc']==row['skilldesc'])
        assert not any(v for k,v in desc.items() if k.startswith(('dsc3line','dsc3text','dsc3calc')))
        for key,val in report['after'].items():
            if key!='EDmgSymPerCalc':assert row[key]==val,(row['skill'],key)
        factor=Fraction(str(report['damage_multiplier']))
        for level in range(1,100):
            for bound in ('Min','Max'):
                expected=damage(report['before'],bound,level)*factor
                actual=damage(row,bound,level)
                # At most half a native 1/256 unit per term is rounded.
                assert abs(actual-expected)<=Fraction(level,512),(row['skill'],level,bound,actual,expected)
        lo=damage(row,'Min',20);hi=damage(row,'Max',20)
        if folder=='data/global/excel':print(f"{row['skill']} level 20 pulse: {float(lo):.3f} - {float(hi):.3f}; multiplier {float(factor):g}")
print('PASS: all three intrinsic damage curves, all five scaling bands, levels 1-99, no old synergy formulas or tooltip rows, independent Aura Mastery effect, both table sets.')
