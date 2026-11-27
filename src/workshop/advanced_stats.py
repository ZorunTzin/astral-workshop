"""Expose useful combat totals in the native Advanced Stats list (mouse/controller)."""
import json
from pathlib import Path
from combat.port import read,write
ALWAYS=('workshop_bleed_chance','workshop_impale_chance')
EXTRA=('workshop_reservation_efficiency','workshop_aura_effect','heal_afterhit','ws_pet_life','ws_pet_damage','ws_pet_speed','ws_pet_count','ws_orb_spell_damage','ws_orb_trinity_chance','maxfireresist','maxcoldresist','maxlightresist','maxpoisonresist','item_maxhp_percent','item_maxmana_percent','damagepercent','item_allskills','item_tohit_percent','armorclass_vs_missile','armorclass_vs_hth','item_strength_percent','item_dexterity_percent','item_vitality_percent','item_energy_percent','extra_traps','extra_shadow_masters','extra_ravens','extra_spirit_wolves','extra_dire_wolves','extra_firegolems','extra_skelewarriors','extra_skelemages','extra_grizzly_bears')
LABELS={'item_allskills':('WorkshopAdvancedAllSkills','to All Skills','zu allen Fertigkeiten'),'damagepercent':('WorkshopAdvancedAttackDamage','Enhanced Physical Attack Damage','Erhoehter physischer Angriffsschaden'),'passive_dodge':('WorkshopAdvancedDodge','Chance to Dodge Melee Attacks','Chance, Nahkampfangriffen auszuweichen'),'passive_avoid':('WorkshopAdvancedAvoid','Chance to Avoid Missiles','Chance, Geschossen auszuweichen'),'passive_evade':('WorkshopAdvancedEvade','Chance to Evade While Moving','Ausweichchance in Bewegung')}
def install(root):
 root=Path(root)
 for folder in ('data/global/excel','data/global/excel/base'):
  p=root/folder/'itemstatcost.txt';h,rows=read(p);by={r['Stat']:r for r in rows}
  for name in ALWAYS+EXTRA:
   r=by[name];assert r['descstrpos'] and r['descfunc'];r['advdisplay']='1' if name in ALWAYS else '2'
  for name,(key,en,de) in LABELS.items():
   by[name].update(advdisplay='2',descfunc='2',descval='1',descpriority='85',descstrpos=key,descstrneg=key)
  by['item_allskills'].update(descfunc='1',descval='1')
  write(p,h,rows)
 p=root/'data/local/lng/strings/ui.json';rows=json.loads(p.read_text(encoding='utf-8-sig'));by={r['Key']:r for r in rows}
 next_id=max(r['id'] for f in p.parent.glob('*.json') for r in json.loads(f.read_text(encoding='utf-8-sig')) if isinstance(r,dict) and 'id' in r)+1
 languages=[k for k in rows[0] if k not in ('id','Key')]
 for key,en,de in LABELS.values():
  if key not in by:r={'id':next_id,'Key':key};rows.append(r);by[key]=r;next_id+=1
  for lang in languages:by[key][lang]=de if lang=='deDE' else en
 p.write_text(json.dumps(rows,ensure_ascii=False,indent=2),encoding='utf8')
 print('Advanced Stats: always-visible bleed/impale plus',len(EXTRA)+len(LABELS),'additional combat totals; native list in mouse/controller UI.')
if __name__=='__main__':
 import sys
 install(sys.argv[1])
