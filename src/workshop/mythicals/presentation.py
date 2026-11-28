"""Elite classification, Chronicle eligibility and original belt inventory art."""
from pathlib import Path
import json,struct
from PIL import Image,ImageOps
from combat.port import read,write
HERE=Path(__file__).parent

def install(root):
 root=Path(root);out=root/'workshop/mythicals';catalog=json.loads((out/'catalog.json').read_text());codes={r['code'] for r in catalog};keys={r['key'] for r in catalog};assets=[]
 for folder in ('data/global/excel','data/global/excel/base'):
  for table in ('armor','weapons','misc'):
   p=root/folder/(table+'.txt');h,rows=read(p)
   for row in rows:
    if row['code'] in codes:
     row.update(normcode='',ubercode='',ultracode=row['code'])
     if table=='armor' and row.get('belt')=='6':
      # Retain the native belt family used by the expanded potion UI.
      # The private base remains the elite member and retains its own art/type.
      row.update(normcode='lbl',ubercode='zlb',type2=row['type'] if row['type']!='belt' else row['type2'],type='belt',belt='6',invwidth='2',invheight='1')
   write(p,h,rows);assets.append(folder+'/'+table+'.txt')
  p=root/folder/'uniqueitems.txt';h,rows=read(p)
  for row in rows:
   if row['index'] in keys:row['disableChronicle']='0' if row['spawnable']=='1' else '1'
  write(p,h,rows);assets.append(folder+'/uniqueitems.txt')
 # Native two-cell belt sprites, retaining generated transparency and aspect ratio.
 for item in catalog:
  ordinal=item['id']
  if ordinal!=9 and not 43<=ordinal<=80:continue
  image=Image.open(HERE/'original-belts'/('headhunter.png' if ordinal==9 else 'mageblood.png' if ordinal<=45 else 'screams.png')).convert('RGBA')
  image=image.crop(image.getchannel('A').getbbox())
  for suffix,w,h in [('.sprite',196,98),('.lowend.sprite',98,49)]:
   im=Image.new('RGBA',(w,h));scaled=ImageOps.contain(image,(w-4,h-4),Image.Resampling.LANCZOS);im.paste(scaled,((w-scaled.width)//2,(h-scaled.height)//2))
   rel=f'data/hd/global/ui/items/armor/belt/workshopmythical{ordinal:02d}{suffix}';p=root/rel;p.parent.mkdir(parents=True,exist_ok=True)
   p.write_bytes(struct.pack('<4sHH8I',b'SpA1',31,w,w,h,0,1,0,0,0,0)+im.tobytes());assets.append(rel)
 p=out/'assets.json';p.write_text(json.dumps(sorted(set(json.loads(p.read_text())+assets)),indent=2))
 print('Mythicals: explicit Elite bases, active Chronicle entries and original skull/bone belt icons.')
