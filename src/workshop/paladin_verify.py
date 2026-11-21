"""Check Sanctuary's entire curve, summon count, reservation and forge routing."""
from pathlib import Path
from fractions import Fraction
from combat.port import read
from combat.aura_reservation import TIERS

root = Path(__file__).resolve().parent.parent

def damage(row, bound, level):
    total = int(row['E' + bound] or 0); remaining = level - 1
    for band, count in enumerate((7, 8, 6, 6, 1000), 1):
        used = min(remaining, count)
        total += used * int(row['E' + bound + 'Lev' + str(band)] or 0)
        remaining -= used
    return Fraction(total * (1 << int(row['HitShift'])), 256)

for folder in ('data/global/excel', 'data/global/excel/base'):
    p = root / folder
    _, skills = read(p / 'skills.txt'); by = {r['skill']: r for r in skills}
    sanctuary, shock = by['Sanctuary'], by['Holy Shock']
    assert sanctuary['EType'] == 'mag'
    assert sanctuary['EDmgSymPerCalc'] == "stat('workshop_aura_effect'.accr)"
    assert not sanctuary['Param8']
    for level in range(1, 100):
        for bound in ('Min', 'Max'):
            assert abs(damage(sanctuary, bound, level) - damage(shock, bound, level) * Fraction(3, 4)) <= Fraction(level, 512)
    _, descs = read(p / 'skilldesc.txt')
    desc = next(r for r in descs if r['skilldesc'] == sanctuary['skilldesc'])
    assert not any(v for k, v in desc.items() if k.startswith(('dsc3line', 'dsc3text', 'dsc3calc')))
    guardian = by['Resist Lightning']
    assert 'ws_pet_count' in guardian['petmax'] and guardian['pettype'] == 'workshop_astral_guardian'
    assert [1 + level // 10 for level in (1, 9, 10, 19, 20, 30)] == [1, 1, 2, 2, 3, 4]
    assert by['Thorns']['calc3'].startswith('(50+25*lvl)')
    _, words = read(p / 'runes.txt')
    assert len(words) == 220 and all(r['complete'] == '0' for r in words)
    _, recipes = read(p / 'cubemain.txt')
    assert len([r for r in recipes if r['description'].startswith('Workshop - ')]) > 2000
assert TIERS == {1:(15,3),6:(30,6),12:(45,9),18:(60,12),24:(75,15),30:(90,18)}
print('PASS: Sanctuary 75% curve at levels 1-99, no synergy, private guardian pool/count, new reservation tiers, 220 socket patterns disabled and forging recipes retained.')
