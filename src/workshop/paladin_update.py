"""Paladin balance additions; original skill IDs and equipped aura copies stay stable."""
from pathlib import Path
import json
from combat.port import read,write

def install(root):
 root=Path(root);stateid=None
 labels={
 'WorkshopPrayerHeal':'Life healed per pulse: %d',
 'WorkshopMightCrush':'Crushing Blow: +%d%%',
 'WorkshopAimDeadly':'Deadly Strike: +%d%%',
 'WorkshopDefiancePhysical':'Physical damage reduction: %d%%',
 'WorkshopSalvationMaximum':'Maximum all resistances: +%d%%',
 'WorkshopConvictionPhysical':'Enemy physical resistance: -%d%%',
 'WorkshopRedemptionLife':'Max life per redeemed corpse (15 s): +%d%%'}
 for folder in ('data/global/excel','data/global/excel/base'):
  p=root/folder;h,skills=read(p/'skills.txt');dh,descs=read(p/'skilldesc.txt')
  by={r['skill']:r for r in skills};descby={r['skilldesc']:r for r in descs}
  def effect(skill,slot,stat,formula,label):
   r=by[skill];r['aurastat'+str(slot)]=stat;r['aurastatcalc'+str(slot)]=formula
   d=descby[r['skilldesc']]
   # Extra line is independent of vanilla display functions and reservation.
   prefix='desc';n=next(i for i in range(1,7) if not d[prefix+'line'+str(i)])
   for field,value in [('line','36'),('texta',label),('textb',label),('calca',formula),('calcb','1')]:d[prefix+field+str(n)]=value
  mastery="(100+stat('workshop_aura_effect'.accr))/100"
  by['Prayer']['aurastatcalc1']='2*('+by['Prayer']['aurastatcalc1']+')'
  # Native Prayer uses its elemental damage calculator as a healing calculator.
  # Keep the display in step with the doubled heal without touching synergy.
  d=descby[by['Prayer']['skilldesc']]
  for i in range(1,7):
   if d['descline'+str(i)]=='74':
    d['descline'+str(i)]='36';d['desctexta'+str(i)]=d['desctextb'+str(i)]='WorkshopPrayerHeal'
    d['desccalca'+str(i)]='('+by['Prayer']['aurastatcalc1']+')/256';d['desccalcb'+str(i)]='1'
  effect('Might',2,'item_crushingblow','lvl*'+mastery,'WorkshopMightCrush')
  effect('Blessed Aim',2,'item_deadlystrike','lvl*'+mastery,'WorkshopAimDeadly')
  effect('Defiance',2,'damageresist','lvl*'+mastery,'WorkshopDefiancePhysical')
  for slot,stat in enumerate(('maxfireresist','maxcoldresist','maxlightresist'),4):
   by['Salvation']['aurastat'+str(slot)]=stat;by['Salvation']['aurastatcalc'+str(slot)]='(1+lvl/10)*'+mastery
  d=descby[by['Salvation']['skilldesc']];n=next(i for i in range(1,7) if not d['descline'+str(i)])
  for f,v in [('line','36'),('texta','WorkshopSalvationMaximum'),('textb','WorkshopSalvationMaximum'),('calca','(1+lvl/10)*'+mastery),('calcb','1')]:d['desc'+f+str(n)]=v
  effect('Conviction',5,'damageresist','0-lvl*'+mastery,'WorkshopConvictionPhysical')
  # The displayed magnitude uses a positive number after the minus sign.
  d=descby[by['Conviction']['skilldesc']]
  for i in range(1,7):
   if d['desctexta'+str(i)]=='WorkshopConvictionPhysical':d['desccalca'+str(i)]='lvl*'+mastery
  d=descby[by['Redemption']['skilldesc']];n=next(i for i in range(1,7) if not d['descline'+str(i)])
  for f,v in [('line','36'),('texta','WorkshopRedemptionLife'),('textb','WorkshopRedemptionLife'),('calca','(1+lvl/10)*'+mastery),('calcb','1')]:d['desc'+f+str(n)]=v
  sth,states=read(p/'states.txt');sid=max(int(r['*ID']) for r in states if r['*ID'].isdigit())+1
  row={k:'' for k in sth};row.update(state='workshop_redemption_life',**{'*ID':str(sid),'aura':'1','*eol':'0'});states.append(row)
  assert stateid in (None,sid);stateid=sid
  for name,header,rows in [('skills',h,skills),('skilldesc',dh,descs),('states',sth,states)]:write(p/(name+'.txt'),header,rows)
 strings=root/'data/local/lng/strings/skills.json';rows=json.loads(strings.read_text(encoding='utf8'));last=max(r['id'] for p in strings.parent.glob('*.json') for r in json.loads(p.read_text(encoding='utf-8-sig')) if 'id' in r)
 for key,text in labels.items():
  last+=1;rows.append({'id':last,'Key':key,**{k:text for k in rows[0] if k not in ('id','Key')}})
 strings.write_text(json.dumps(rows,ensure_ascii=False,indent=4),encoding='utf8')
 (root/'workshop/combat/redemption_bindings.h').write_text(f'// Generated dedicated temporary redemption state.\ninline constexpr int RedemptionLifeState={stateid};\n')
"""
All new aura stats use the existing Aura Mastery multiplier once. Maximum
resistance bonuses are quantized at each completed ten skill levels before
mastery; the engine retains its own resistance and damage reduction caps.
"""
