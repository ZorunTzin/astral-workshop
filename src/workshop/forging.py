"""Compile approved, full-strength rune-forging selections into cube recipes.

The cube preserves the input item with `useitem`. Its one dispatch property is
expanded synchronously by forge_native.inc into ordinary native properties.
No replacement item, affix reroll, socket use, or delayed inventory scan occurs.
"""
import json
from collections import Counter, defaultdict


def generate_forging(d, here, marker, read, append, rec, item, recipes, names):
    words = json.loads((here / 'FORGING.json').read_text(encoding='utf8'))
    assert len(words) == 220
    properties = {r['code']: int(r['*Id']) for r in read(d/'properties.txt')[0] if r['*Id'].isdigit()}
    # Declare ailment properties before compiling numeric forge bundles. Their
    # saved stat IDs are installed later without moving any existing save field.
    for code, stat in [('ws-bleed','workshop_bleed_chance'), ('ws-impale','workshop_impale_chance')]:
        if code not in properties:
            pid = max(properties.values()) + 1
            append(d/'properties.txt', [{'code':code, '*Id':pid, 'func1':1, 'stat1':stat, '*eol':0}])
            properties[code] = pid
    skill_ids = {r['skill'].casefold(): int(r['*Id']) for r in read(d/'skills.txt')[0] if r['*Id'].isdigit()}
    types = {r['Code']: r for r in read(d/'itemtypes.txt')[0]}
    runes = {r['code']: r for r in read(d/'misc.txt')[0]}

    dispatch = max(properties.values()) + 1
    floor_property = dispatch + 1
    # Add new saved fields after the established Astral fields. Never resize an
    # existing saved stat, which would invalidate characters' older equipment.
    floor_stat = marker + 28
    assert floor_stat < 511
    append(d/'itemstatcost.txt', [{'Stat':'ws_forge_level', '*ID':floor_stat,
        'Send Bits':7, 'Save Bits':7, '*eol':0}])
    append(d/'properties.txt', [
        {'code':'ws-forge-dispatch', '*Id':dispatch, 'func1':0, '*eol':0},
        {'code':'ws-forge-level', '*Id':floor_property, 'func1':1, 'stat1':'ws_forge_level', '*eol':0}])

    def ancestors(code, seen=None):
        seen = set() if seen is None else seen
        if not code or code in seen:
            return seen
        seen.add(code)
        for field in ('Equiv1', 'Equiv2'):
            ancestors(types.get(code, {}).get(field, ''), seen)
        return seen

    def native(e):
        parameter = e['param']
        if str(parameter).casefold() in skill_ids:
            parameter = skill_ids[str(parameter).casefold()]
        else:
            parameter = int(parameter or 0)
        lo, hi = int(e['lo']), int(e['hi'])
        assert lo == e['lo'] and hi == e['hi'], e
        return [properties[e['code']], parameter, lo, hi]

    groups = defaultdict(list)
    assert len(words) <= 256  # Two spare marker values times the existing seven-bit field.
    for index, word in enumerate(words):
        word['_identity'] = index
        word['seal'] = {'Lore':'Wlor', 'Stealth':'Wstl'}.get(word['name'], f'F{index:03X}')
        groups[tuple(sorted(word['runeCodes']))].append(word)
        label = word['name'] + ' forge seal'
        # Same-name variants get a base hint so their recipes stay distinguishable.
        if sum(w['name'] == word['name'] for w in words) > 1:
            label += ' (' + word['base'] + ')'
        item(word['seal'], label, 'isc')

    for rune_set, group in groups.items():
        counts = Counter(rune_set)
        inputs = [code + (f',qty={count}' if count > 1 else '') for code, count in counts.items()]
        row = rec('Prepare ' + group[0]['name'], [*inputs, 'isc'], group[0]['seal'])
        row['numinputs'] = len(rune_set) + 1
        recipes.append(row)
        if len(group) > 1:
            # Cube ingredients have no order. Shared rune multisets therefore
            # use a harmless, explicit scroll cycle to select the intended word.
            for i, word in enumerate(group):
                recipes.append(rec('Select forge ' + word['name'], [word['seal'], 'isc'], group[(i+1)%len(group)]['seal']))

    bundles, audit, direct = [], [], []
    for word in words:
        assert not word['excludeTypes'], 'Add explicit exclusions before enabling this word'
        floor = max(int(runes[code]['levelreq'] or 0) for code in word['runeCodes'])
        # Shields precede the broad armor type because their intrinsic rune
        # bonuses differ. The source words explicitly include shields here.
        base_types = sorted(word['includeTypes'], key=lambda t: 0 if 'shld' in ancestors(t) else 1)
        for quality, tier, extras in [('nor','white',None), ('hiq','white',None),
                ('mag','magic',4), ('rar','rare',2), ('crf','rare',2), ('set','unique',0), ('uni','unique',0)]:
            indices = range(len(word['terms'])) if extras is None else sorted(set(word['coreIndices'] + word['secondaryIndices'][:extras]))
            selected = [word['terms'][i] for i in indices]
            for base_type in base_types:
                ancestry = ancestors(base_type)
                kind = 'weapon' if 'weap' in ancestry else 'shield' if 'shld' in ancestry else 'helm'
                effects = [native(e) for e in selected]
                if extras is None:
                    effects += [native(e) for e in word['runeEffects'][kind]]
                # Preserve both established save widths. Marker 1 remains the old
                # level-floor format; markers 2/3 encode a persistent word ID.
                identity=word['_identity'];tag=2+(identity>>7);low=identity&127
                effects += [[floor_property,0,low,low], [properties['ws-forged'],0,tag,tag]]
                bundle = len(bundles) + 1
                bundles.append(effects)
                row = rec('Forge ' + word['name'] + ' ' + quality + ' ' + base_type,
                    [base_type + ',' + quality + ',nru', word['seal']], 'useitem',
                    [('ws-forge-dispatch',1)], op=18, param=marker, value=0)
                row['mod 1 param'] = bundle
                recipes.append(row)
                counts=Counter(word['runeCodes'])
                rune_inputs=[code+(f',qty={count}' if count>1 else '') for code,count in counts.items()]
                immediate=row.copy()
                immediate['description']='Workshop - Direct '+word['name']+' '+quality+' '+base_type
                for input_index in range(1,8):
                    immediate.pop(f'input {input_index}',None)
                for input_index,ingredient in enumerate([base_type+','+quality+',nru',*rune_inputs],1):
                    immediate[f'input {input_index}']=ingredient
                immediate['numinputs']=1+len(word['runeCodes'])
                # A narrow base (wand/staff) must precede a generic weapon recipe.
                # White and Shadow of Doubt truly share both runes and base;
                # prefer the familiar White recipe, leaving seal selection available.
                priority=(len(ancestry),word['name']=='White')
                direct.append((priority,immediate))
                audit.append({'name':word['name'], 'seal':word['seal'], 'quality':quality,
                    'tier':tier, 'baseType':base_type, 'bundle':bundle, 'effects':effects,
                    'runeLevel':floor, 'kind':kind})

    recipes.extend(row for _,row in sorted(direct,key=lambda pair:pair[0],reverse=True))

    flattened = []
    spans = []
    for effects in bundles:
        spans.append([len(flattened),len(effects)])
        flattened.extend(effects)
    def cpp(rows):
        return ',\n'.join('{' + ','.join(map(str,row)) + '}' for row in rows)
    include = ('// Generated from FORGING.json. Do not hand-edit numeric bindings.\n'
        f'constexpr unsigned forgeDispatchProperty={dispatch}, forgeLevelStat={floor_stat}, forgeMarkerStat={marker};\n'
        'struct ForgeWord { const char* name; int required; };\n'
        'constexpr ForgeWord forgeWords[]={\n'+',\n'.join('{'+json.dumps(w['name'])+','+str(max(int(runes[c]['levelreq'] or 0) for c in w['runeCodes']))+'}' for w in words)+'\n};\n'
        'struct ForgeSpan { unsigned start, count; };\n'
        'constexpr Items::PropertySpec forgeEffects[]={\n' + cpp(flattened) + '\n};\n'
        'constexpr ForgeSpan forgeSpans[]={\n' + cpp(spans) + '\n};\n')
    (here/'forge.inc').write_text(include, encoding='utf8')
    (here/'FORGING-audit.json').write_text(json.dumps(audit,ensure_ascii=False,indent=2),encoding='utf8')
    (here/'FORGING-seals.json').write_text(json.dumps({w['seal']:w['name'] for w in words},indent=2),encoding='utf8')

