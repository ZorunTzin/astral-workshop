"""Requested local balance changes, applied once to pristine generated tables."""
from pathlib import Path
from combat.port import read, write

def install(root):
    for folder in ('data/global/excel', 'data/global/excel/base'):
        p = Path(root) / folder
        h, skills = read(p / 'skills.txt')
        by = {r['skill']: r for r in skills}
        plague, infection = by['Amplify Damage'], by['Terror']
        # Preserve fractional poison values by changing the fixed-point shift,
        # rather than rounding each small per-level coefficient down to zero.
        plague['HitShift'] = str(int(plague['HitShift']) - 1)
        plague['Param7'] = '15'
        for r in (infection, by['FMMP Virus Infection']):
            r['Param7'] = '15'
            r['EDmgSymPerCalc'] = r['EDmgSymPerCalc'].replace('+ 4999', '+ 9999').replace('/ 5000', '/ 10000')
        attract = by['Attract']
        attract['calc4'] = attract['calc4'].removesuffix('/100') + '/200'
        for name in ('FMMP Element Ground', 'FMMP Element Nova', 'FMMP Element Chain'):
            by[name]['HitShift'] = str(int(by[name]['HitShift']) - 1)
        # Move the early unlock and prerequisite graph with the two UI slots;
        # otherwise Infection would sit first but still require a later node.
        fields=('reqlevel','reqskill1','reqskill2','reqskill3')
        old_plague={k:plague[k] for k in fields}
        for k in fields:plague[k],infection[k]=infection[k],old_plague[k]
        for r in skills:
            if r['charclass']=='nec':
                for k in fields[1:]:
                    if r[k]=='Amplify Damage':r[k]='Terror'
                    elif r[k]=='Terror':r[k]='Amplify Damage'
        write(p / 'skills.txt', h, skills)

        h, descs = read(p / 'skilldesc.txt')
        ds = {r['skilldesc']: r for r in descs}
        a, b = ds['amplify damage'], ds['terror']
        for k in ('SkillPage', 'SkillRow', 'SkillColumn', 'ListRow'):
            a[k], b[k] = b[k], a[k]
        for r in (a, b):
            for n in (2, 3, 4):
                r[f'dsc3calca{n}'] = '15'
        b['desccalca2'] = b['desccalca2'].replace('100+10*', '100+15*').replace('+4999', '+9999').replace('/5000', '/10000')
        ds['attract']['desccalca2'] = attract['calc4']
        for n in (2, 3, 4):
            expr = ds['life tap'][f'desccalca{n}']
            # Display the same half-damage result as the private missile skills.
            ds['life tap'][f'desccalca{n}'] = '(' + expr + ')/2'
        write(p / 'skilldesc.txt', h, descs)

        h, chars = read(p / 'charstats.txt')
        for r in chars:
            for k in ('WalkVelocity', 'RunVelocity'):
                if r[k]: r[k] = str((int(r[k]) * 3 + 1) // 2)
        write(p / 'charstats.txt', h, chars)

        h, monsters = read(p / 'monstats.txt')
        for r in monsters:
            if r['enabled'] != '1' or r['killable'] != '1' or r['Align'] not in ('', '0'):
                continue
            for suffix, factor in (('', 200), ('(N)', 175), ('(H)', 150)):
                for k in h:
                    bare = k.removesuffix(suffix) if suffix else k
                    if suffix and not k.endswith(suffix): continue
                    if not suffix and k.endswith(('(N)', '(H)')): continue
                    if bare.lower() not in ('minhp', 'maxhp'): continue
                    if r[k]: r[k] = str((int(r[k]) * factor + 50) // 100)
        write(p / 'monstats.txt', h, monsters)

if __name__ == '__main__':
    import sys
    install(sys.argv[1])
