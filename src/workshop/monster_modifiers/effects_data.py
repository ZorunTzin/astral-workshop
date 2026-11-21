"""Data for hostile modifier fields, clones and destructible totems."""
import json
from pathlib import Path
from combat.port import read,write
HERE=Path(__file__).resolve().parent

def install(root):
 root=Path(root);monster_models=json.loads((root/'data/hd/character/monsters.json').read_text(encoding='utf-8-sig'));bindings=None;aliases={};monster_aliases={};clone_map=None
 for directory in ('data/global/excel','data/global/excel/base'):
  folder=root/directory;mh,missiles=read(folder/'missiles.txt');nh,monsters=read(folder/'monstats.txt');local={};mapping=[]
  by={r['Missile']:r for r in missiles};next_id=max(int(r['*ID']) for r in missiles if r['*ID'].isdigit())+1
  def missile(key,source,**values):
   nonlocal next_id
   row=by[source].copy();name='workshop_modifier_'+key.lower();row.update(Missile=name,**{'*ID':str(next_id)},**values);row={k:v for k,v in row.items() if k in mh};missiles.append(row);aliases[name]=source;local[key]=next_id;next_id+=1
  # All visible fields are inert; one native damage packet per scheduled pulse
  # supplies damage. This avoids overlap causing uncontrolled native hit rates.
  for key,source,frames in [('FireOrb','fireball',75),('FireField','firewall',75),('PoisonField','poisonexplosioncloud',75),('GravityField','warcry',25),('Explosion','explosion',12)]:
   if source not in by:source='warcry'
   missile(key,source,Skill='',pSrvDoFunc='1',pSrvHitFunc='',pSrvDmgFunc='',pCltHitFunc='',CollideType='0',ClientCol='0',SrcDamage='0',EType='',EMin='0',EMax='0',MinDamage='0',MaxDamage='0',Vel='0',MaxVel='0',Range=str(frames),LevRange='0',ClientSend='1',CltSubMissile1='',CltHitSubMissile1='',SubMissile1='',HitSubMissile1='',ExplosionMissile='')
  # Area hazards are spell hits, not weapon attacks. No attack-rating gate and
  # no firebolt/explosion callback that replaces the supplied damage packet.
  # Explosion=0 is essential: native Hit tests record+0x24 bit 2 and otherwise
  # suppresses its default damage path, even with valid populated damage stats.
  missile('Hit','fireexplosion',Skill='',SrcDamage='0',EType='',EMin='0',EMax='0',MinDamage='0',MaxDamage='0',ClientSend='0',pSrvHitFunc='',pSrvDmgFunc='',CollideType='3',Range='1',LevRange='0',ToHit='0',HitFlags='0',ResultFlags='4',HitShift='8',NextHit='0',NextDelay='0',CelFile='null',TravelSound='',HitSound='',ExplosionMissile='',CollideKill='1',Explosion='0')
  # The generic client updater continuously animates stationary arena markers.
  # Firewall's specialized updater has its own fade/child lifetime behavior.
  missile('ArenaMarker','firewall',Skill='',pSrvDoFunc='1',pSrvHitFunc='',pSrvDmgFunc='',pCltDoFunc='1',pCltHitFunc='',CollideType='0',ClientCol='0',SrcDamage='0',EType='',EMin='0',EMax='0',MinDamage='0',MaxDamage='0',Vel='0',MaxVel='0',Range='40',LevRange='0',ClientSend='1',LoopAnim='1',CltSubMissile1='',CltSubMissile2='',CltSubMissile3='',CltHitSubMissile1='',CltHitSubMissile2='',CltHitSubMissile3='',SubMissile1='',SubMissile2='',SubMissile3='',HitSubMissile1='',HitSubMissile2='',HitSubMissile3='',ExplosionMissile='',Explosion='0')
  sources=list(monsters)
  # A private clone retains the base monster's appearance and native AI, but
  # cannot contribute to kill-count quests or produce duplicated treasure.
  for source in sources:
   cid=source.get('*hcIdx','')
   if not cid.isdigit() or source.get('killable')!='1' or source.get('Align') in ('1','2') or source.get('boss')=='1' or source.get('primeevil')=='1' or source['Id'].startswith('workshop_') or (source['Id'] not in monster_models and source.get('BaseId') not in monster_models):continue
   row=source.copy();newid=max(int(r['*hcIdx']) for r in monsters if r.get('*hcIdx','').isdigit())+1
   row.update(Id='workshop_modifier_clone_'+cid,**{'*hcIdx':str(newid)},NextInClass='',MonProp='',neverCount='1',isSpawn='0',boss='0',primeevil='0',unique='0',Rarity='0',MinGrp='1',MaxGrp='1',spawn='',placespawn='',DescStr='')
   for k in row:
    if k.startswith('TreasureClass') or k.startswith('Exp'):row[k]=''
   monster_aliases[row['Id']]=source['Id'] if source['Id'] in monster_models else source['BaseId']
   row['BaseId']=row['Id']
   row={k:v for k,v in row.items() if k in nh};monsters.append(row);mapping.append((int(cid),newid))
  source=next(r for r in sources if r['Id']=='labtotem1');row=source.copy();cid=max(int(r['*hcIdx']) for r in monsters if r.get('*hcIdx','').isdigit())+1
  row.update(Id='workshop_modifier_totem',**{'*hcIdx':str(cid)},BaseId='workshop_modifier_totem',NextInClass='',MonProp='',NameStr='WorkshopProtectionTotem',DescStr='',AI='Idle',neverCount='1',isSpawn='0',boss='0',primeevil='0',unique='0',Align='0',Rarity='0',Velocity='0',Run='0',DamageRegen='0',killable='1')
  for k in row:
   if k.startswith(('Skill','Sk','TreasureClass','Exp','Miss','Res')):row[k]=''
  monster_aliases[row['Id']]='labtotem1'
  row={k:v for k,v in row.items() if k in nh};monsters.append(row);local['TotemClass']=cid
  rune=row.copy();cid+=1
  rune.update(Id='workshop_modifier_runestone',BaseId='workshop_modifier_runestone',NameStr='WorkshopRitualStone',**{'*hcIdx':str(cid)})
  monsters.append(rune);local['RuneClass']=cid;monster_aliases[rune['Id']]='labtotem1'
  write(folder/'missiles.txt',mh,missiles);write(folder/'monstats.txt',nh,monsters)
  if bindings is not None:assert bindings==local and clone_map==mapping
  bindings=local;clone_map=mapping
 registry=root/'data/hd/missiles/missiles.json';data=json.loads(registry.read_text(encoding='utf-8-sig'))
 for key,source in aliases.items():
  if source in data:data[key]=data[source]
 registry.write_text(json.dumps(data,indent=2,ensure_ascii=False),encoding='utf-8')
 registry=root/'data/hd/character/monsters.json';data=json.loads(registry.read_text(encoding='utf-8-sig'))
 for key,source in monster_aliases.items():
  if source in data:data[key]=data[source]
 registry.write_text(json.dumps(data,indent=2,ensure_ascii=False),encoding='utf-8')
 strings=root/'data/local/lng/strings/monsters.json';entries=json.loads(strings.read_text(encoding='utf-8-sig'))
 max_id=max(int(e['id']) for f in (root/'data/local/lng/strings').glob('*.json') for e in json.loads(f.read_text(encoding='utf-8-sig')) if isinstance(e,dict) and 'id' in e)
 entry={'id':max_id+1,'Key':'WorkshopProtectionTotem'}
 for lang in ('enUS','zhTW','deDE','esES','frFR','itIT','koKR','plPL','esMX','jaJP','ptBR','ruRU','zhCN'):entry[lang]='Protection Totem'
 entry['deDE']='Schutztotem';entries.append(entry)
 rune={'id':max_id+2,'Key':'WorkshopRitualStone'}
 for lang in ('enUS','zhTW','deDE','esES','frFR','itIT','koKR','plPL','esMX','jaJP','ptBR','ruRU','zhCN'):rune[lang]='Ritual Stone'
 rune['deDE']='Ritualstein';entries.append(rune);strings.write_text(json.dumps(entries,indent=2,ensure_ascii=False),encoding='utf-8')
 out='\n'.join('inline constexpr int '+k+'='+str(v)+';' for k,v in bindings.items())+'\n'
 out+='inline constexpr int CloneClasses[][2]={'+','.join('{'+str(a)+','+str(b)+'}' for a,b in clone_map)+'};\n'
 (HERE/'effects_bindings.h').write_text(out)
 (HERE/'effects_data.json').write_text(json.dumps({'bindings':bindings,'cloneClasses':clone_map,'visuals':aliases},indent=2)+'\n')
