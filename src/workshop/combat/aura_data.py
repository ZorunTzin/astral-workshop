"""Convert Paladin auras to castable toggles while preserving their effects."""
import json
from pathlib import Path
try:
    from .port import read,write
except ImportError:
    from port import read,write

AURAS=(98,99,100,102,103,104,105,108,109,110,113,114,115,118,119,120,122,123,124,125)

def install(root):
    try:
        from .aura_damage import install as bake_damage
    except ImportError:
        from aura_damage import install as bake_damage
    bake_damage(root)
    here=Path(__file__).resolve().parent
    definitions=[]
    for directory in (root/'data/global/excel',root/'data/global/excel/base'):
        header,states=read(directory/'states.txt')
        state_id=max(int(r['*ID']) for r in states if r['*ID'].isdigit())+1
        state={k:'' for k in header}
        state.update(state='workshop_mana_reserved',**{'*ID':str(state_id),'aura':'1','stat':'maxmana','*eol':'0'})
        states.append(state);write(directory/'states.txt',header,states)
        state_ids={r['state']:int(r['*ID']) for r in states if r['*ID'].isdigit()}
        header,skills=read(directory/'skills.txt')
        next_id=max(int(r['*Id']) for r in skills if r['*Id'].isdigit())+1
        local=[]
        descriptions=set()
        for row in skills.copy():
            if not row['*Id'].isdigit() or int(row['*Id']) not in AURAS:continue
            skill=int(row['*Id']);old_func=int(row['srvdofunc'])
            assert old_func in (65,66,81,82)
            helper={k:'' for k in header}
            helper.update(skill='Workshop Aura Level '+row['skill'],**{'*Id':str(next_id),
                'reqlevel':'1','maxlvl':'1','calc4':"skill('"+row['skill']+"'.lvl)",'*eol':'0'})
            skills.append(helper)
            local.append((skill,old_func,state_ids[row['aurastate']],next_id))
            next_id+=1;descriptions.add(row['skilldesc'])
            for field in ('aura','periodic','periodicClearAura','repeat','srvstfunc','cltstfunc','cltdofunc'):
                row[field]=''
            row.update(srvdofunc='18',anim='SC',monanim='SC',immediate='',range='none',startmana='0',mana='0',lvlmana='0',minmana='0',InTown='1')
            for field in ('TargetableOnly','TargetCorpse','TargetPet','TgtPlaceCheck','SearchEnemyXY','SearchOpenXY'):
                if field in row:row[field]=''
        assert len(local)==20
        write(directory/'skills.txt',header,skills)
        header,descs=read(directory/'skilldesc.txt')
        for row in descs:
            if row['skilldesc'] not in descriptions:continue
            # A regular formula line supplies current/next-level reservation.
            slots=[(f'descline{i}',f'desctexta{i}',f'desctextb{i}',f'desccalca{i}',f'desccalcb{i}') for i in range(1,7)]
            slots += [(f'dsc2line{i}',f'dsc2texta{i}',f'dsc2textb{i}',f'dsc2calca{i}',f'dsc2calcb{i}') for i in range(1,5)]
            slot=next((s for s in slots if row.get(s[0])=='1'),None)
            if slot is None:slot=next(s for s in slots if not row.get(s[0]))
            for key,value in zip(slot,('3','WorkshopAuraReserved','WorkshopAuraEmpty','10+2*lvl','')):
                row[key]=value
        write(directory/'skilldesc.txt',header,descs)
        if definitions:assert definitions==local
        definitions=local
    out='// Generated bindings for Reimagined Workshop.\n'
    out+='struct AuraDefinition { int skill, function, state, levelHelper; };\n'
    out+='inline constexpr AuraDefinition Definitions[]={\n'+',\n'.join('{'+','.join(map(str,r))+'}' for r in definitions)+'\n};\n'
    out+=f'inline constexpr int ReservationState={state_id};\n'
    (here/'aura_bindings.h').write_text(out,encoding='utf8')
    strings=root/'data/local/lng/strings/skills.json'
    rows=json.loads(strings.read_text(encoding='utf-8-sig'))
    last=max(r['id'] for p in strings.parent.glob('*.json') for r in json.loads(p.read_text(encoding='utf-8-sig')) if isinstance(r,dict) and 'id' in r)
    locales=[k for k in rows[0] if k not in ('id','Key')]
    for key,text in [('WorkshopAuraReserved','Mana reserved: '),('WorkshopAuraEmpty','')]:
        last+=1;rows.append({'id':last,'Key':key,**dict.fromkeys(locales,text)})
    strings.write_text(json.dumps(rows,ensure_ascii=False,indent=4),encoding='utf8')
    try:
        from .aura_reservation import install as configure_reservation
    except ImportError:
        from aura_reservation import install as configure_reservation
    configure_reservation(root)

if __name__=='__main__':
    import sys
    install(Path(sys.argv[1]))
