"""Item format v2: widened vanilla item values, isolated from legacy saves."""
from pathlib import Path
import json,sys
from combat.port import read,write
FORMAT=2
SAVE_BITS=20
SAVE_PATH='ReimaginedWorkshop-v2/'
STATS='item_maxdamage_percent item_mindamage_percent item_armor_percent item_fasterattackrate item_fastercastrate item_fastergethitrate item_fasterblockrate item_fastermovevelocity strength dexterity vitality energy maxhp maxmana hpregen item_allskills item_addclassskills item_addskill_tab firemindam firemaxdam coldmindam coldmaxdam lightmindam lightmaxdam item_magicbonus item_goldbonus normal_damage_reduction magic_damage_reduction'.split()
def install(root):
 root=Path(root)
 for folder in ('data/global/excel','data/global/excel/base'):
  p=root/folder/'itemstatcost.txt';header,rows=read(p);by={r['Stat']:r for r in rows}
  for name in STATS:
   row=by[name]
   # Preserve signed bias, fixed-point scale, skill parameters and character-save widths.
   row['Save Bits']=str(SAVE_BITS)
   if row['Send Bits']:row['Send Bits']='32'
  write(p,header,rows)
 p=root/'modinfo.json';info=json.loads(p.read_text());info['savepath']=SAVE_PATH;p.write_text(json.dumps(info,indent=2)+'\n')
 report={'format':FORMAT,'savepath':SAVE_PATH,'save_bits':SAVE_BITS,'stats':STATS,'legacy_savepath':'ReimaginedWorkshop/','migration':'New profile; legacy saves are left untouched and must not be copied into v2.'}
 (root/'workshop/item-format.json').write_text(json.dumps(report,indent=2)+'\n')
if __name__=='__main__':install(sys.argv[1])
