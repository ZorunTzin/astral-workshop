"""Paladin 0.9.1: Sanctuary curve and forge-only runewords."""
from pathlib import Path
from combat.port import read, write

def install(root):
    root = Path(root)
    for folder in ('data/global/excel', 'data/global/excel/base'):
        p = root / folder
        h, skills = read(p / 'skills.txt')
        shock = next(r for r in skills if r['skill'] == 'Holy Shock')
        sanctuary = next(r for r in skills if r['skill'] == 'Sanctuary')
        assert shock['HitShift'] == '0'
        for key in ['EMin', 'EMax'] + [f'E{bound}Lev{band}' for bound in ('Min', 'Max') for band in range(1, 6)]:
            sanctuary[key] = str((int(shock[key] or 0) * 3 + 2) // 4)
        sanctuary['HitShift'] = '0'
        sanctuary['EDmgSymPerCalc'] = "stat('workshop_aura_effect'.accr)"
        sanctuary['Param8'] = ''; sanctuary['*Param8 Description'] = ''
        write(p / 'skills.txt', h, skills)
        h, descs = read(p / 'skilldesc.txt')
        desc = next(r for r in descs if r['skilldesc'] == sanctuary['skilldesc'])
        for key in desc:
            if key.startswith(('dsc3line', 'dsc3text', 'dsc3calc')): desc[key] = ''
        write(p / 'skilldesc.txt', h, descs)
        # This disables native rune-sequence matching only. Forging recipes use
        # the approved FORGING.json catalog and private property dispatch, so
        # their bonuses and individual socketed-rune effects remain available.
        h, words = read(p / 'runes.txt')
        for row in words: row['complete'] = '0'
        write(p / 'runes.txt', h, words)

if __name__ == '__main__':
    import sys
    install(sys.argv[1])
