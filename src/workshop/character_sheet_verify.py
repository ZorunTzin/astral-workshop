"""Check native character-sheet identities and damage units in both modes."""
import json
from pathlib import Path
from combat.port import read
from character_sheet import CUSTOM
root=Path(__file__).resolve().parents[1]
labels={r['Key']:r for p in (root/'data/local/lng/strings').glob('*.json') for r in json.loads(p.read_text(encoding='utf-8-sig')) if isinstance(r,dict) and 'Key' in r}
for folder in ('data/global/excel','data/global/excel/base'):
 _,skills=read(root/folder/'skills.txt');_,descs=read(root/folder/'skilldesc.txt');by={int(r['*Id']):r for r in skills if r['*Id'].isdigit()};ds={r['skilldesc']:r for r in descs}
 shown={66,77,82,86,103,137,140}
 for sid in CUSTOM:
  d=ds[by[sid]['skilldesc']];name=labels[d['str name']]['enUS'];alt=labels[d['str alt']]['enUS'];assert alt.startswith(name),(sid,name,alt)
  assert not d['descmissile1'] and not d['descmissile2']
  if sid in shown:assert d['descdam']=='7' and d['ddam calc1'] and d['ddam calc2']
  else:assert not d['descdam'] and not d['ddam calc1'] and not d['ddam calc2']
  if sid in (66,77,82):assert d['ddam calc2']==d['desccalca2']
 assert ds[by[137]['skilldesc']]['ddam calc1']=='(50+5*(lvl-1))*(100+clc5*par8)/300-100'
 assert ds[by[140]['skilldesc']]['ddam calc1']=='(50+10*lvl)*(100+clc5)/100-100'
 assert ds[by[103]['skilldesc']]['ddam calc2']=='clc3'
 assert ds[by[86]['skilldesc']]['ddam calc2']=='clc4'
 assert ds[by[133]['skilldesc']]['str alt']==ds[by[133]['skilldesc']]['str name']

 for sid in (6,7,11,12,14,19,20,22,24,25,30,34):
  d=ds[by[sid]['skilldesc']];assert d['str alt']=='WorkshopCharacterAmazon'+str(sid)
  assert labels[d['str alt']]['enUS'].startswith(labels[d['str name']]['enUS'])
 for sid in (14,25):assert ds[by[sid]['skilldesc']]['descdam']=='8' and ds[by[sid]['skilldesc']]['ddam calc1']=='1'
 assert ds[by[20]['skilldesc']]['descdam']=='21' and ds[by[20]['skilldesc']]['descatt']=='3'
 for sid in (19,22):assert ds[by[sid]['skilldesc']]['ddam calc1']==('(100+clc4)/3-100' if sid==22 else 'clc1')

print('PASS: character-sheet identities, Amazon per-hit/poison-DPS displays, throwing accuracy and non-damaging buff exclusions in both table sets.')
