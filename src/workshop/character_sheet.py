"""Character-sheet labels and explicitly scoped damage values for custom skills.

str alt is independent of the skill-tree title. Custom runtime effects need an
honest display unit: weapon portion, pulse, initial DPS or added damage per hit.
No gameplay damage formulas or public skill IDs are changed here.
"""
import json
from pathlib import Path
from combat.port import read,write

CUSTOM=(8,17,18,28,66,71,72,76,77,81,82,86,87,91,100,103,105,109,110,128,131,133,137,140,146,148,153)
# Function 7 is native weapon damage with percent/flat inputs. -100 removes the
# weapon component when displaying a standalone periodic or triggered effect.
def install(root):
 root=Path(root);strings=root/'data/local/lng/strings/skills.json'
 labels=json.loads(strings.read_text(encoding='utf-8-sig'));by_key={r['Key']:r for r in labels}
 next_id=max(r['id'] for p in strings.parent.glob('*.json') for r in json.loads(p.read_text(encoding='utf-8-sig')) if isinstance(r,dict) and 'id' in r)+1
 for folder in ('data/global/excel','data/global/excel/base'):
  sh,skills=read(root/folder/'skills.txt');dh,descs=read(root/folder/'skilldesc.txt')
  by={int(r['*Id']):r for r in skills if r['*Id'].isdigit()};ds={r['skilldesc']:r for r in descs}
  for sid in CUSTOM:
   d=ds[by[sid]['skilldesc']];d['str alt']=d['str name']
   for key in d:
    if key in ('descdam','ddam calc1','ddam calc2','descatt') or key.startswith(('p1dm','p2dm','p3dm','descmissile')):d[key]=''
  displays={
   66:('DPS','-100',ds[by[66]['skilldesc']]['desccalca2']),
   77:('DPS','-100',ds[by[77]['skilldesc']]['desccalca2']),
   82:('fire','-100',ds[by[82]['skilldesc']]['desccalca2']),
   86:('+hit','-100','clc4'),
   103:('nova','-100','clc3'),
   137:('weapon','(50+5*(lvl-1))*(100+clc5*par8)/300-100','0'),
   140:('pulse','(50+10*lvl)*(100+clc5)/100-100','0'),
  }
  for sid,(unit,percent,flat) in displays.items():
   d=ds[by[sid]['skilldesc']];name=d['str name'];key='WorkshopCharacterSkill'+str(sid)
   if key not in by_key:
    label={lang:(text+' ('+unit+')' if lang not in ('id','Key') else text) for lang,text in by_key[name].items()}
    label.update(id=next_id,Key=key);next_id+=1;labels.append(label);by_key[key]=label
   d.update({'str alt':key,'descdam':'7','ddam calc1':percent,'ddam calc2':flat})
   if sid in (137,140):d['descatt']='1'
  # Explicit units prevent a multishot/chain display from implying total volley damage.
  amazon_units={6:'hit',7:'hit',11:'hit',12:'arrow',14:'poison/s',19:'hit',20:'javelin',22:'arrow',24:'melee/bolt',25:'poison/s',30:'hit',34:'melee/chain'}
  for sid,unit in amazon_units.items():
   d=ds[by[sid]['skilldesc']];key='WorkshopCharacterAmazon'+str(sid)
   if key not in by_key:
    label={lang:(text+' ('+unit+')' if lang not in ('id','Key') else text) for lang,text in by_key[d['str name']].items()}
    label.update(id=next_id,Key=key);next_id+=1;labels.append(label);by_key[key]=label
   d['str alt']=key
   if sid in (14,25):
    # Native periodic poison display includes its own skill synergies and poison
    # mastery. Do not mix total poison duration with a second throwing-hand value.
    d.update({'descdam':'8','ddam calc1':'1','ddam calc2':'1','descatt':''})
   elif sid==20:
    d.update({'descdam':'21','ddam calc1':'','ddam calc2':'','descatt':'3'})
   elif sid in (19,22):
    d['ddam calc1']='clc4' if sid==22 else 'clc1';d['ddam calc2']='0'
  write(root/folder/'skilldesc.txt',dh,descs)
 strings.write_text(json.dumps(labels,ensure_ascii=False,indent=2),encoding='utf8')
 print('Character sheet: custom labels, weapon/pulse/DPS units, zero stale Double Throw/Berserk displays.')
