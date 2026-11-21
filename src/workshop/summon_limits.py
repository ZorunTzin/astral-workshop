"""Connect the shared Astral summon-count stat after every skill replacement."""
from pathlib import Path
from combat.port import read, write

def install(root):
    term = "stat('ws_pet_count'.accr)"
    for folder in ('data/global/excel', 'data/global/excel/base'):
        p = Path(root) / folder / 'skills.txt'
        header, rows = read(p)
        for row in rows:
            if row['charclass'] and row['pettype'] and row['petmax'] and term not in row['petmax']:
                row['petmax'] = '(' + row['petmax'] + ')+' + term
        write(p, header, rows)

if __name__ == '__main__':
    import sys
    install(sys.argv[1])
