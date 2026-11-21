"""Install the Buff HUD companion and a mod-aware catalog at bottom right."""
from pathlib import Path
from shutil import copy2
import json, re, hashlib
from combat.port import read

def install(root):
    root=Path(root); source=Path(__file__).resolve().parent/'integration/upstream/buff-hud'
    for name,digest in json.loads((source/'INPUTS.json').read_text()).items():
        assert hashlib.sha256((source/name).read_bytes()).hexdigest()==digest
    for name in ('d2rl-buffhud.dll','d2rl-buffhud.mpq'):copy2(source/name,root/'d2rloader/plugins'/name)
    license=root/'d2rloader/licenses/Buff-HUD-LICENSE.txt';license.parent.mkdir(exist_ok=True)
    copy2(source/'LICENSE.txt',license)
    layout=json.loads((source/'BuffHudhd.json').read_text(encoding='utf-8-sig'))
    layout['fields']['anchor']['x']=1.0
    grid=next(c for c in layout['children'] if c['name']=='BuffGrid')
    grid['fields']['rect'].update(x=-700,y=-770,width=660,height=411)
    for child in grid['children']:
        if child['name'].startswith('BuffSlot'):
            index=int(child['name'][-2:]);child['fields']['rect'].update(x=(index%6)*112,y=(3-index//6)*105)
    assert sum(c['name'].startswith('BuffSlot') for c in grid['children'])==21
    relative='data/global/ui/layouts/buffhud/BuffHudhd.json'
    target=root/relative;target.parent.mkdir(parents=True,exist_ok=True)
    target.write_text(json.dumps(layout,indent=2),encoding='utf8')

    _,states=read(root/'data/global/excel/states.txt');states={r['state']:r for r in states}
    _,skills=read(root/'data/global/excel/skills.txt')
    _,stats=read(root/'data/global/excel/itemstatcost.txt');stats={r['Stat']:int(r['*ID']) for r in stats if r['*ID'].isdigit()}
    entries={}
    def add(state,skill=0,kind='timer',value=0,maximum=0,shift=0):
        row=states.get(state)
        if not row or row.get('curse')=='1':return
        entries[int(row['*ID'])]=(state,int(row['*ID']),kind,value,maximum,skill,shift,1)
    for row in skills:
        if not row['*Id'].isdigit():continue
        state=row['aurastate']
        if not state:continue
        first,second=row['aurastat1'],row['aurastat2']
        if first and second==first+'max' and first in stats and second in stats:
            add(state,int(row['*Id']),'resource',stats[first],stats[second],8)
        elif row['auralencalc'] and (row['charclass'] or row['ItemEffect']):add(state,int(row['*Id']))
    for state in states:
        if state.startswith('shrine_') or state in ('wolf','bear','delerium','cloak_of_shadows','antidote','thawing','staminapot'):add(state)
    bindings=(root/'workshop/combat/aura_bindings.h').read_text()
    names={int(r['*Id']):r['skill'] for r in skills if r['*Id'].isdigit()}
    byid={int(r['*ID']):name for name,r in states.items() if r['*ID'].isdigit()}
    for skill,_,state,_,_,_ in re.findall(r'\{(\d+),(\d+),(\d+),(\d+),(\d+),(\d+)\}',bindings):
        if int(skill) not in (100,105,109,110):add(byid[int(state)],int(skill))
    add('workshop_carnage',128)
    add('workshop_feast',131)
    add('workshop_sanctified',109)
    add('fmmp_soul_harvest',76)
    add('workshop_redemption_life',124)
    relative_table='data/global/excel/d2rloader/buffhud/buff-hud.txt'
    target=root/relative_table;target.parent.mkdir(parents=True,exist_ok=True)
    header='name\tstate_id\tdisplay_type\tvalue_stat\tmax_stat\tskill_id\tvalue_shift\tenabled\n'
    target.write_text(header+'\n'.join('\t'.join(map(str,entries[k])) for k in sorted(entries))+'\n',encoding='utf8')
    manifest=root/'workshop/combat/assets.json';assets=json.loads(manifest.read_text())
    assets=[a for a in assets if a not in ('data/global/ui/layouts/buff-panel/BuffHudhd.json','data/global/excel/d2rloader/buff-panel/buff-hud.txt')]
    manifest.write_text(json.dumps(list(dict.fromkeys(assets+[relative,relative_table])),indent=2))
    print(f'Buff HUD: 21 slots at bottom right, {len(entries)} mod-aware buff entries.')

if __name__=='__main__':
    import sys
    install(sys.argv[1])
