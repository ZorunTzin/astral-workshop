"""Bake twenty hard points in both former damage synergies into aura damage.

Damage table integers are shifted by HitShift into native 8-bit fixed point.
Using HitShift=0 preserves fractional damage when multiplying all five growth
bands, instead of rounding each per-level increment to a whole damage point.
This affects pulse damage and the attack damage derived from it together.
"""
from fractions import Fraction
try:
    from .port import read,write
except ImportError:
    from port import read,write

TARGETS={'Holy Fire':('Resist Fire',16,8),'Holy Freeze':('Resist Cold',14,6),'Holy Shock':('Resist Lightning',12,4)}

def tune(skills,descs):
    report=[]
    for row in skills:
        if row['skill'] not in TARGETS:continue
        resist,resist_rate,salvation_rate=TARGETS[row['skill']]
        expected="skill('"+resist+"'.blvl)*par8+skill('Salvation'.blvl)*par7"
        assert row['EDmgSymPerCalc']==expected,(row['skill'],row['EDmgSymPerCalc'])
        assert int(row['Param8'])==resist_rate and int(row['Param7'])==salvation_rate
        factor=Fraction(100+20*(resist_rate+salvation_rate),100)
        shift=int(row['HitShift'])
        fields=['EMin','EMax']+[f'E{bound}Lev{band}' for bound in ('Min','Max') for band in range(1,6)]
        before={key:row[key] for key in fields+['HitShift','EDmgSymPerCalc','Param7','Param8']}
        for key in fields:
            if row[key]:row[key]=str(round(int(row[key])*(1<<shift)*factor))
        row['HitShift']='0';row['EDmgSymPerCalc']=''
        row['Param7']='';row['Param8']=''
        row['*Param7 Description']='';row['*Param8 Description']=''
        desc=next(d for d in descs if d['skilldesc']==row['skilldesc'])
        # These three lines are solely the former synergy heading and sources.
        for key in desc:
            if key.startswith(('dsc3line','dsc3text','dsc3calc')):desc[key]=''
        report.append({'skill':row['skill'],'reference_hard_points_per_synergy':20,'damage_multiplier':float(factor),'before':before,'after':{key:row[key] for key in before}})
    assert len(report)==3
    return report

def install(root):
    import json
    root=__import__('pathlib').Path(root)
    reports=[]
    for folder in ('data/global/excel','data/global/excel/base'):
        p=root/folder;sh,skills=read(p/'skills.txt');dh,descs=read(p/'skilldesc.txt')
        report=tune(skills,descs)
        if reports:assert report==reports
        reports=report
        write(p/'skills.txt',sh,skills);write(p/'skilldesc.txt',dh,descs)
    (__import__('pathlib').Path(__file__).resolve().parent/'AURA-DAMAGE.json').write_text(json.dumps(reports,indent=2),encoding='utf8')

if __name__=='__main__':
    import sys
    install(sys.argv[1])
