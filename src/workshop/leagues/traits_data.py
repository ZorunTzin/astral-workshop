"""Private League trait classes: stable appended IDs, HD size variants, no world edits."""
from pathlib import Path
import json,copy
from combat.port import read,write

def install(root):
 root=Path(root);registry=root/'data/hd/character/monsters.json';models=json.loads(registry.read_text(encoding='utf-8-sig'));assets=[];bindings=None;labels={}
 for folder in ('data/global/excel','data/global/excel/base'):
  p=root/folder;h,rows=read(p/'monstats.txt');by={r['Id']:r for r in rows};nextid=max(int(r['*hcIdx']) for r in rows if r['*hcIdx'].isdigit())+1;records=[]
  originals=[r for r in rows if r['Id'].startswith('wsleague') and r['Id'][8:].isdigit()]
  for base in originals:
   kind=int(base['Id'][8]);types={0:[(1,'Colossus',1.65),(2,'Plague',1)],1:[(3,'Rift Warden',1),(4,'Brood',1),(5,'Spawn',.6)],2:[(6,'Blood Hunter',1)]}[kind]
   for trait,label,scale in types:
    key=base['Id']+'_trait'+str(trait);old=by.get(key);r=base.copy();r.update(Id=key,NameStr=key,**{'*hcIdx':old['*hcIdx'] if old else str(nextid)})
    if not old:nextid+=1
    if trait==5:
     for k in r:
      if k.startswith('TreasureClass'):r[k]=''
      if k in ('Exp','Exp(N)','Exp(H)'):r[k]='0'
    if old:old.update(r)
    else:rows.append(r)
    records.append([int(base['*hcIdx']),int(r['*hcIdx']),kind,trait]);labels[key]=label+' '+base['Id']
    if folder.endswith('/base'):continue
    original=models[base['Id']];modelname=original
    if scale!=1:
     modelname=original+'_trait'+str(trait);model=copy.deepcopy(json.loads((root/'data/hd/character/enemy'/(original+'.json')).read_text(encoding='utf-8-sig')));model['name']=modelname
     changed=False
     for entity in model['entities']:
      if entity['name']=='entity_root':
       if not any(c['type']=='TransformDefinitionComponent' for c in entity['components']):entity['components'].append(dict(type='TransformDefinitionComponent',name='league_size',position=dict(x=0,y=0,z=0),orientation=dict(x=0,y=0,z=0,w=1),scale=dict(x=1,y=1,z=1)))
       for c in entity['components']:
        if c['type']=='TransformDefinitionComponent':c['scale']={k:v*scale for k,v in c['scale'].items()};changed=True
     assert changed,original
     rel='data/hd/character/enemy/'+modelname+'.json';(root/rel).write_text(json.dumps(model,indent=2));assets.append(rel)
    models[key]=modelname
  write(p/'monstats.txt',h,rows);assets.append(folder+'/monstats.txt')
  assert bindings is None or bindings==records;bindings=records
 registry.write_text(json.dumps(models,indent=2));assets.append('data/hd/character/monsters.json')
 p=root/'data/local/lng/strings/item-names.json';strings=json.loads(p.read_text(encoding='utf-8-sig'));by={r['Key']:r for r in strings};nid=max(r['id'] for f in p.parent.glob('*.json') for r in json.loads(f.read_text(encoding='utf-8-sig')) if 'id' in r)+1
 for key,label in labels.items():
  basekey=key.split('_trait')[0];prefix=label.rsplit(' ',1)[0];r=by.get(key)
  if r is None:r={'id':nid,'Key':key};nid+=1;strings.append(r)
  for lang in strings[0]:
   if lang not in ('id','Key'):r[lang]=prefix+' '+by[basekey][lang]
 p.write_text(json.dumps(strings,ensure_ascii=False,indent=2),encoding="utf-8");assets.append(str(p.relative_to(root)).replace('\\','/'))
 out=root/'workshop/leagues';(out/'traits_bindings.h').write_text('struct TraitClass {int base,cls,kind,trait;};\ninline constexpr TraitClass TraitClasses[]={'+','.join('{'+','.join(map(str,r))+'}' for r in bindings)+'};\n')
 (out/'traits_assets.json').write_text(json.dumps(sorted(set(assets)),indent=2))
 manifest=out/'assets.json';combined=json.loads(manifest.read_text()) if manifest.exists() else [];manifest.write_text(json.dumps(sorted(set(combined+assets)),indent=2))
 print('League trait variants:',len(bindings))
