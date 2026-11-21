"""Paladin shield affixes; reuse previously runtime-only aura stats without moving IDs."""
import json
from pathlib import Path
from combat.port import read, write
from table_ids import normalize

TIERS = [(1, 1, 1, 3), (20, 15, 4, 6), (40, 30, 7, 9), (60, 45, 10, 12), (80, 60, 13, 15)]
EFFECTS = [
    ('workshop_aura_effect', 'ws-aura-effect', 'WorkshopItemAuraEffect', 'Increased Aura Effect', 'of Radiance'),
    ('workshop_reservation_efficiency', 'ws-reservation-efficiency', 'WorkshopItemReservationEfficiency', 'Reservation Efficiency', 'of Devotion'),
]

def install(root):
    root = Path(root)
    labels = {}
    audit = []
    for folder in ('data/global/excel', 'data/global/excel/base'):
        directory = root / folder
        header, stats = read(directory / 'itemstatcost.txt')
        ph, props = read(directory / 'properties.txt')
        ah, affixes = read(directory / 'magicsuffix.txt')
        next_group = max(int(r['group'] or 0) for r in affixes) + 1
        for offset, (stat, code, key, text, suffix) in enumerate(EFFECTS):
            row = next(r for r in stats if r['Stat'] == stat)
            assert row['Save Bits'] in ('', '16') and not row['Save Param Bits']
            # These IDs previously occurred only in runtime skill stats, never
            # in serialized item properties. No existing saved field is resized.
            if not row['Save Bits']:
                assert not any(p.get('stat'+str(i)) == stat for p in props for i in range(1,8))
            row.update({'Save Bits': '16', 'Save Add': '0', 'descpriority': str(88+offset),
                        'descfunc': '2', 'descval': '1', 'descstrpos': key, 'descstrneg': key, 'advdisplay': '2'})
            labels[key] = text
            existing = next((p for p in props if p['code'] == code), None)
            if existing is None:
                p = {k: '' for k in ph}
                p.update(code=code, func1='1', stat1=stat, **{'*eol': '0'})
                props.append(p)
            else:
                assert existing['func1'] == '1' and existing['stat1'] == stat
            old = [a for a in affixes if a['mod1code'] == code]
            group = old[0]['group'] if old else str(next_group+offset)
            affixes = [a for a in affixes if a['mod1code'] != code]
            for tier, (level, required, lo, hi) in enumerate(TIERS, 1):
                name = 'WorkshopPaladin' + ('Aura' if offset == 0 else 'Efficiency') + str(tier)
                labels[name] = suffix
                a = {k: '' for k in ah}
                a.update(name=name, version='100', spawnable='1', rare='1', level=str(level),
                         levelreq=str(required), frequency='8', group=group, mod1code=code,
                         mod1min=str(lo), mod1max=str(hi), itype1='pala', multiply='0', add='0')
                affixes.append(a)
                if folder.endswith('/excel'):
                    audit.append(dict(name=name, effect=text, tier=tier, affixLevel=level,
                                      requiredLevel=required, minimum=lo, maximum=hi, itemType='pala'))
        write(directory/'itemstatcost.txt', header, stats)
        write(directory/'properties.txt', ph, props)
        write(directory/'magicsuffix.txt', ah, affixes)
        normalize(directory)
    path = root/'data/local/lng/strings/item-names.json'
    strings = json.loads(path.read_text(encoding='utf-8-sig'))
    highest = max(r['id'] for f in path.parent.glob('*.json')
                  for r in json.loads(f.read_text(encoding='utf-8-sig')) if 'id' in r)
    for key, text in labels.items():
        existing = next((s for s in strings if s['Key'] == key), None)
        if existing is None:
            highest += 1
            strings.append({'id': highest, 'Key': key,
                            **{lang: text for lang in strings[0] if lang not in ('id','Key')}})
        else:
            assert existing['enUS'] == text
    path.write_text(json.dumps(strings, ensure_ascii=False, indent=2), encoding='utf8')
    (root/'workshop/PALADIN-AFFIXES.json').write_text(json.dumps(audit, indent=2), encoding='utf8')
    print('Paladin affixes: 10 suffixes, exact 1-15% ranges, existing stat IDs retained.')

if __name__ == '__main__':
    import sys
    install(Path(sys.argv[1]))
