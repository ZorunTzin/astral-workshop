"""Import edited node effects without rewriting the user's Excel workbook.

Only changed effect cells are imported. Identity columns are checked first;
unknown wording fails before any source file is written. Excel quote prefixes
and apostrophes at the beginning of individual lines are ignored.
"""
import argparse, copy, csv, io, json, re, sys, zipfile
import xml.etree.ElementTree as ET
from pathlib import Path

REPO = Path(__file__).resolve().parents[3]
SOURCE = REPO / 'src/workshop'
sys.path.insert(0, str(SOURCE))
from node_text import FLAT, PERCENT
NS = {'s': 'http://schemas.openxmlformats.org/spreadsheetml/2006/main'}

def normalize(text):
    return '\n'.join(line.strip().lstrip("'",).strip() for line in str(text).strip().splitlines())

def read_book(path):
    with zipfile.ZipFile(path) as z:
        shared = ET.fromstring(z.read('xl/sharedStrings.xml')) if 'xl/sharedStrings.xml' in z.namelist() else []
        strings = [''.join(t.text or '' for t in n.findall('.//s:t', NS)) for n in shared]
        sheet = ET.fromstring(z.read('xl/worksheets/sheet1.xml'))
        cells = {}
        for cell in sheet.findall('.//s:sheetData/s:row/s:c', NS):
            value = cell.find('s:v', NS)
            value = value.text if value is not None else ''
            if cell.get('t') == 's': value = strings[int(value)]
            elif cell.get('t') == 'inlineStr': value = ''.join(t.text or '' for t in cell.findall('.//s:t', NS))
            if cell.find('s:f', NS) is not None:
                raise ValueError('Formula in editable workbook: ' + cell.get('r'))
            cells[cell.get('r')] = normalize(value)
        return cells

def term(code, value, parameter=0, maximum=None, bonus=False):
    return dict(code=code,parameter=parameter,minimum=value,maximum=value if maximum is None else maximum,bonus=bonus)

def parse_line(line, bonus=False):
    line = normalize(line)
    proc = re.fullmatch(r'(\d+)% chance to cast level (\d+) (.+) on (striking|kill|attack)',line,re.I)
    if proc:
        chance,level,skill,event=proc.groups()
        skill={'Elemental Marks':'Life Tap','Plague':'Amplify Damage'}.get(skill,skill)
        return term({'striking':'hit-skill','kill':'kill-skill','attack':'att-skill'}[event],int(chance),skill,int(level),bonus)
    skill=re.fullmatch(r'\+(\d+) to (.+) \(All Classes\)',line)
    if skill:
        value,name=skill.groups()
        return term('oskill',int(value),{'Mana Shield':'Energy Shield'}.get(name,name),bonus=bonus)
    poison=re.fullmatch(r'Adds (\d+) poison damage over (\d+) seconds? to attacks',line)
    if poison:
        damage,seconds=map(int,poison.groups());frames=seconds*25
        # Round fixed-point damage per frame, preserving the requested total to <0.5 damage.
        return term('dmg-pois',round(damage*256/frames),frames,bonus=bonus)
    resist=re.fullmatch(r'\+?(\d+)% to (maximum )?(All|Fire|Cold|Lightning|Poison|Magic) Resistances',line,re.I)
    if resist:
        value,maximum,element=resist.groups()
        code='res-'+{'lightning':'ltng','poison':'pois'}.get(element.lower(),element.lower())+('-max' if maximum else '')
        return term(code,int(value),bonus=bonus)
    pierce=re.fullmatch(r'-(\d+)% to enemy (Physical|Fire|Cold|Lightning|Poison|Magic) Resistance',line,re.I)
    if pierce:
        value,element=pierce.groups()
        code='pierce-'+{'physical':'dmg','lightning':'ltng','poison':'pois'}.get(element.lower(),element.lower())
        return term(code,int(value),bonus=bonus)
    if line.lower()=='cannot be frozen':return term('nofreeze',1,bonus=bonus)
    number=re.fullmatch(r'\+?(\d+)(%?) (.+)',line)
    if number:
        value,percent,label=number.groups()
        catalog=PERCENT if percent else FLAT
        aliases={'Crushing blow':'crush','Life on hit':'healperhit','Maximum mana':'mana%'}
        code=next((k for k,v in catalog.items() if v.lower()==label.lower()),None) or aliases.get(label)
        if code:return term(code,int(value),bonus=bonus)
    raise ValueError('Unsupported effect wording: '+repr(line))

def parse(text):
    # Rimeheart's two effects share a cell separated with a comma.
    lines=normalize(text).replace('Cannot be frozen, ','Cannot be frozen\n').splitlines()
    return [parse_line(line,bonus=i>0) for i,line in enumerate(lines)]

def csv_text(rows):
    out=io.StringIO();writer=csv.DictWriter(out,fieldnames=rows[0].keys(),delimiter=';',lineterminator='\n')
    writer.writeheader();writer.writerows(rows);return out.getvalue()

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--apply',action='store_true');args=parser.parse_args()
    book=REPO/'tables/Astral-Skilltree-0.9.5.xlsx'
    cells=read_book(book)
    baseline=json.loads((REPO/'build/spreadsheets/skilltree/tree.json').read_text(encoding='utf8'))
    nodes=list(csv.DictReader((SOURCE/'NODES.csv').open(encoding='utf-8-sig'),delimiter=';'))
    draft=list(csv.DictReader((SOURCE/'NODES-step2.csv').open(encoding='utf-8-sig'),delimiter=';'))
    inner=json.loads((SOURCE/'INNER-CLUSTERS.json').read_text(encoding='utf8'))
    overrides=json.loads((SOURCE/'CLUSTER-OVERRIDES.json').read_text(encoding='utf8'))
    by_node={r['name']:r for r in nodes};by_draft={r['name']:r for r in draft}
    changes=[];expected={}
    for row,old in enumerate(baseline,8):
        assert all(cells.get(f'{col}{row}','')==str(old[i]) for i,col in enumerate('ABCD')),('Changed identity',row)
        values=[cells.get(f'{col}{row}','') for col in 'EFGHI']
        expected[old[3]]=values
        edited=[i for i in range(5) if values[i]!=normalize(old[4+i])]
        if not edited:continue
        name=old[3];node=by_node[name];plan=by_draft[name]
        for i in edited:changes.append(dict(cell=f'{"EFGHI"[i]}{row}',name=name,before=old[4+i],after=values[i],effects=parse(values[i])))
        if node['slot']=='Keystone':
            effects=parse(values[0]);first=effects[0]
            node.update(property='ws-minion-limit' if first['code']=='ws-pet-count' else first['code'],parameter=str(first['parameter']),minimum=str(first['minimum']),maximum=str(first['maximum']))
            for field in ('property','parameter','minimum','maximum'):plan[field]=node[field]
            if len(effects)>1:overrides.setdefault(name,{})['keystone_extra']=effects[1:]
        else:
            assert 0 not in edited,('Root node changes require an explicit root payload',name)
            definitions=[dict(label=' / '.join(value.splitlines()),effects=parse(value)) for value in values[1:]]
            for col,definition in zip(('Knoten 2','Knoten 3','Knoten 4','Knoten 5 mit Bonus'),definitions):plan[col]=definition['label']
            if node['slot']=='Inner':inner[name]['ranks']=definitions
            else:overrides.setdefault(name,{})['ranks']=definitions
    output=REPO/'build/reports';output.mkdir(parents=True,exist_ok=True)
    (output/'skilltree-excel-import.json').write_text(json.dumps(dict(workbook=str(book),changes=changes,expected=expected),ensure_ascii=False,indent=2),encoding='utf8')
    print(f'Validated {len(baseline)} rows and {len(changes)} changed effect cells. Workbook unchanged.')
    if args.apply:
        for name,rows in (('NODES.csv',nodes),('NODES-step2.csv',draft)):(SOURCE/name).write_text(csv_text(rows),encoding='utf-8-sig')
        for name,data in (('INNER-CLUSTERS.json',inner),('CLUSTER-OVERRIDES.json',overrides)):(SOURCE/name).write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf8')
        print('Updated canonical node payloads and labels.')

if __name__=='__main__':main()
