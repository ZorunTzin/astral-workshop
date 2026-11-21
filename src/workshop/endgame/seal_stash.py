"""Direct native Materials slots; frames are baked into the tab background."""
from pathlib import Path
import json,struct
from PIL import Image
from endgame.seal_data import CODES
from endgame.seal_entries import CODES as SEALS

def install(root):
 root=Path(root);assets=[];base=Path(__file__).parent/'stash-templates'
 for filename,mode,offset in [('bankexpansionlayouthd.json','mouse',0),('controller/bankexpansionlayouthd.json','controller',212)]:
  layout=json.loads((base/filename).read_text(encoding='utf-8-sig'))
  group=next(c for c in layout['children'] if c['name']=='advancedstash_materials')
  # Native bank initialization requires ONLY AdvancedStashSlotWidget children.
  coords=[(620+104*(i%3),266+104*(i//3)) for i in range(7)]
  coords += [(724,474),(828,474),(620,578),(724,578),(828,578)]
  for code,(x,y) in zip(CODES+SEALS,coords):group['children'].append({'type':'AdvancedStashSlotWidget','name':code,'fields':{'rect':{'x':x,'y':y-offset,'width':98,'height':98},'itemCode':code}})
  # Relocate the adjacent Token socket to the spare right margin.
  for child in group['children']:
   if child['fields'].get('itemCode')=='toa':child['fields']['rect'].update(x=1540,y=598-offset)
  bg='Workshop/trial-materials-'+mode
  layout['fields']['backgroundFile'][3]=bg
  rel='data/global/ui/layouts/'+filename;p=root/rel;p.parent.mkdir(parents=True,exist_ok=True);p.write_text(json.dumps(layout,indent=2),encoding='utf8');assets.append(rel)
  raw=(base/(mode+'-background.sprite')).read_bytes();w,h=struct.unpack_from('<II',raw,8)
  assert len(raw)==40+w*h*4
  im=Image.frombytes('RGBA',(w,h),raw[40:])
  # Reuse an exact existing 1x1 Materials socket including its stone/metal rim.
  cell=im.crop((286,494,384,592))
  token=im.crop((844,598,942,696))
  im.paste(im.crop((620,474,722,576)),(842,596))
  im.paste(token,(1540,598))
  for x,y in coords:im.paste(cell,(x,y))
  for suffix,picture in [('.sprite',im),('.lowend.sprite',im.resize((w//2,h//2),Image.Resampling.LANCZOS))]:
   sw,sh=picture.size;rel='data/hd/global/ui/'+bg+suffix;p=root/rel;p.parent.mkdir(parents=True,exist_ok=True)
   p.write_bytes(struct.pack('<4sHH8I',b'SpA1',31,sw,sw,sh,0,1,0,0,0,0)+picture.tobytes());assets.append(rel)
 return assets
