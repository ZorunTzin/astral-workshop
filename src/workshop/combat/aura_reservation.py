"""Keep runtime bindings and aura tooltip reservation formulas in sync."""
import re
from pathlib import Path
try:
    from .port import read,write
except ImportError:
    from port import read,write
TIERS={1:(15,3),6:(30,6),12:(45,9),18:(60,12),24:(75,15),30:(90,18)}

def install(root):
    root=Path(root);here=Path(__file__).resolve().parent
    bindings=here/'aura_bindings.h';text=bindings.read_text(encoding='utf8')
    definitions=[tuple(map(int,m)) for m in re.findall(r'\{(\d+),(\d+),(\d+),(\d+)(?:,\d+,\d+)?\}',text)]
    assert len(definitions)==20
    costs={}
    for folder in ('data/global/excel','data/global/excel/base'):
        p=root/folder;_,skills=read(p/'skills.txt');header,descs=read(p/'skilldesc.txt')
        for skill,_,_,_ in definitions:
            row=next(r for r in skills if r['*Id']==str(skill))
            base,growth=TIERS[int(row['reqlevel'])]
            if skill in costs:assert costs[skill]==(base,growth)
            costs[skill]=(base,growth)
            desc=next(r for r in descs if r['skilldesc']==row['skilldesc'])
            slots=[key for key,value in desc.items() if value=='WorkshopAuraReserved']
            assert len(slots)==1,(row['skill'],slots)
            formula=slots[0].replace('texta','calca')
            desc[formula]=f'{base}+{growth}*lvl'
        write(p/'skilldesc.txt',header,descs)
    text='// Generated bindings for Reimagined Workshop.\n'
    text+='// Reservation tier follows the aura required character level; cost uses effective skill level.\n'
    text+='struct AuraDefinition { int skill, function, state, levelHelper, reserveBase, reservePerLevel; };\n'
    text+='inline constexpr AuraDefinition Definitions[]={\n'
    text+=',\n'.join('{'+','.join(map(str,(*d,*costs[d[0]])))+'}' for d in definitions)+'\n};\n'
    _,states=read(root/'data/global/excel/states.txt')
    state=next(r['*ID'] for r in states if r['state']=='workshop_mana_reserved')
    text+=f'inline constexpr int ReservationState={state};\n'
    bindings.write_text(text,encoding='utf8')

if __name__=='__main__':
    import sys
    install(sys.argv[1])
