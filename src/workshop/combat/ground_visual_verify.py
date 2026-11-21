"""Validate the explicit HD ground marker and its harmless native missile."""
import sys,json
from pathlib import Path
from port import read
root=Path(sys.argv[1]);name='workshop_sanctified_ground'
registry=json.loads((root/'data/hd/missiles/missiles.json').read_text(encoding='utf8'))
assert registry[name]==name
prefab=json.loads((root/f'data/hd/missiles/{name}.json').read_text(encoding='utf8'))
assert prefab['type']=='UnitDefinition'
particles=[c for e in prefab['entities'] for c in e['components'] if c['type']=='VfxDefinitionComponent']
assert particles and all(c['hardKillOnDestroy'] for c in particles)
assert all('seal_death' in c['filename'] for c in particles)
for folder in ('data/global/excel','data/global/excel/base'):
 _,rows=read(root/folder/'missiles.txt');row=next(r for r in rows if r['Missile']==name)
 assert row['ClientSend']=='1' and row['CollideType']=='0' and not row['Explosion']
 assert not row.get('Skill') and not row.get('MinDamage') and not row.get('EMin')
 assert row['*ID']=='1024'
print('PASS: private HD floor prefab, registry binding, bounded lifetime, client sending and collision/damage-free marker in both table sets.')
