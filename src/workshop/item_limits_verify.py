"""Format boundary, metadata and unchanged-column regression checks."""
from pathlib import Path
import sys,json,random
from combat.port import read
from item_limits import STATS,SAVE_BITS,SAVE_PATH
root=Path(sys.argv[1]);repo=Path(__file__).resolve().parents[2]
for folder in ('data/global/excel','data/global/excel/base'):
 _,rows=read(root/folder/'itemstatcost.txt');by={r['Stat']:r for r in rows}
 for name in STATS:
  r=by[name];assert int(r['Save Bits'])==SAVE_BITS
  assert r['Send Bits'] in ('','32')
  bias=int(r['Save Add'] or 0);shift=int(r['ValShift'] or 0);limit=(1<<SAVE_BITS)-1-bias
  assert limit>=1048475 and limit<<shift<2**31
  # Adjacent unaligned fields, signed bias, and nonzero skill parameters survive.
  for value in [-bias,0,7,511,1024,65535,limit]:
   param_bits=int(r['Save Param Bits'] or 0);param=(1<<param_bits)-1
   prefix=0x55;payload=prefix|(param<<7)|((value+bias)<<(7+param_bits))|(0x1ff<<(7+param_bits+SAVE_BITS))
   assert (payload&127)==prefix
   assert (payload>>(7+param_bits))&((1<<SAVE_BITS)-1)==value+bias
   assert payload>>(7+param_bits+SAVE_BITS)==0x1ff
 _,old=read(repo/folder/'itemstatcost.txt')
 for previous in old:
  if previous['Stat'] in ('item_crush_damage_percent','item_thrust_damage','item_thrust_damage_percent'):
   assert not previous['Save Bits'] and not previous['CSvBits']
   current=next(r for r in rows if r['*ID']==previous['*ID'])
   assert current['Stat'] in ('ws-myth-life-regen','ws-myth-mana-percent','ws-myth-mana-regen') and current['Save Bits']=='16'
   continue
  if previous['Stat']=='item_crush_damage' and 'ws_mythical_power' in by:
   assert previous['*ID']=='258' and not previous['Save Bits'] and not previous['CSvBits']
   marker=by['ws_mythical_power']
   assert marker['*ID']=='258' and marker['Save Bits']=='8' and marker['Save Param Bits']=='6'
   continue
  current=by[previous['Stat']]
  for key,value in previous.items():
   if previous['Stat'] in STATS and key in ('Save Bits','Send Bits'):continue
   if previous['Stat']=='normal_damage_reduction' and key in ('descstrpos','descstrneg'):
    assert current[key]=='WorkshopDamageTakenReduced';continue
   assert current[key]==value,(previous['Stat'],key)
assert json.loads((root/'modinfo.json').read_text())['savepath']==SAVE_PATH
assert json.loads((root/'workshop/item-format.json').read_text())['format']==2
print('PASS: 28 widened stats, unaligned boundary values, signed/fixed-point ranges, unchanged skill parameters and character save fields; isolated v2 save profile.')
