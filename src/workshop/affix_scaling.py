"""Bind affix scaling rules to Reimagined's actual property and skill IDs."""
import csv
import json


def export(root):
    directory = root / 'data/global/excel'
    here = root / 'workshop'
    def read(name):
        with (directory/(name+'.txt')).open(encoding='utf-8-sig',newline='') as file:
            return list(csv.DictReader(file,delimiter='\t'))
    used = {r[f'mod{i}code'] for table in ('magicprefix','magicsuffix') for r in read(table)
            for i in (1,2,3) if r[f'mod{i}code']}
    fixed = {'ws-aura-effect','ws-reservation-efficiency','ws-bleed','ws-impale','ethereal','indestruct','half-freeze','nofreeze','ignore-ac','noheal','knock'}
    policies, audit = [], []
    for row in read('properties'):
        code = row['code']
        if code not in used:
            continue
        funcs = {int(row[f'func{i}']) for i in range(1,8) if row[f'func{i}'].isdigit() and int(row[f'func{i}'])}
        # Native fields are semantic. A parameter can be a skill ID, a numeric
        # per-level coefficient, or a skill bonus; those must never be confused.
        kind = 0 if code in fixed or not funcs else 2 if funcs == {17} or 12 in funcs else 3 if 14 in funcs else 4 if 11 in funcs else 5 if 19 in funcs else 1
        cap = 100 if 11 in funcs or 24 in funcs or code=='charge-noconsume' else 255 if code=='howl' else 0
        policies.append((int(row['*Id']),kind,cap))
        audit.append({'code':code,'propertyId':int(row['*Id']),'kind':kind,'minimumCap':cap})
    skill_rows = [(int(r['*Id']),int(r['reqlevel'] or 1),int(r['maxlvl'] or 20))
                  for r in read('skills') if r['*Id'].isdigit()]
    types = {r['Code']: r for r in read('itemtypes') if r['Code']}
    def excluded(kind, seen=None):
        seen=set() if seen is None else seen
        if kind in ('char','jewl'):return True
        if not kind or kind in seen or kind not in types:return False
        seen.add(kind)
        return any(excluded(types[kind].get(k,''),seen) for k in ('Equiv1','Equiv2'))
    catalog=read('weapons')+read('armor')+read('misc')
    excluded_ids=sorted({int.from_bytes(r['code'].ljust(4).encode('ascii'),'little') for r in catalog if r.get('code') and any(excluded(r.get(k,'')) for k in ('type','type2'))})
    out = '// Generated from current Reimagined tables.\n'
    out += 'constexpr unsigned unscaledAffixCodes[]={'+','.join(map(str,excluded_ids))+'};\n'
    out += 'struct AffixRule { unsigned property, kind, cap; };\nconstexpr AffixRule affixRules[]={\n'
    out += ',\n'.join('{'+','.join(map(str,r))+'}' for r in policies)+'\n};\n'
    out += 'struct AffixSkill { int id, required, maximum; };\nconstexpr AffixSkill affixSkills[]={\n'
    out += ',\n'.join('{'+','.join(map(str,r))+'}' for r in skill_rows)+'\n};\n'
    (here/'affix_rules.inc').write_text(out,encoding='utf8')
    (here/'AFFIX-SCALING.json').write_text(json.dumps(audit,indent=2),encoding='utf8')

