"""Append dedicated Mastery frames without changing any existing skill artwork."""
from pathlib import Path
import json,struct
from PIL import Image,ImageEnhance
HERE=Path(__file__).resolve().parent
NAMES=('spear-mastery','bow-mastery','javelin-mastery')
def install(root):
 root=Path(root);assets=[]
 for path in (root/'data/hd/global/ui/spells/amazon').glob('*.sprite'):
  original=path.read_bytes();version,fw,width,height,_,count=struct.unpack_from('<HH4I',original,4)
  assert version==31 and count in (60,66) and len(original)==40+width*height*4
  stride=width//count;newwidth=stride*66
  header=bytearray(original[:40]);struct.pack_into('<I',header,8,newwidth);struct.pack_into('<I',header,20,66)
  if struct.unpack_from('<I',header,32)[0]:struct.pack_into('<I',header,32,newwidth*height*4)
  pixels=bytearray(newwidth*height*4)
  for y in range(height):pixels[y*newwidth*4:y*newwidth*4+60*stride*4]=original[40+y*width*4:40+y*width*4+60*stride*4]
  for i,name in enumerate(NAMES):
   with Image.open(HERE/'icons/amazon'/f'{name}.png') as image:normal=image.convert('RGBA').resize((fw,height),Image.Resampling.LANCZOS)
   for offset,art in enumerate((normal,ImageEnhance.Brightness(normal).enhance(0.55))):
    raw=art.tobytes();frame=60+2*i+offset
    for y in range(height):
     start=(y*newwidth+frame*stride)*4;pixels[start:start+fw*4]=raw[y*fw*4:(y+1)*fw*4]
  for y in range(height):assert pixels[y*newwidth*4:y*newwidth*4+60*stride*4]==original[40+y*width*4:40+y*width*4+60*stride*4]
  path.write_bytes(header+pixels);assets.append(path.relative_to(root).as_posix())
 manifest=root/'workshop/combat/assets.json';previous=json.loads(manifest.read_text());manifest.write_text(json.dumps(list(dict.fromkeys(previous+assets)),indent=2))
 print('Mastery icons: three dedicated normal/dim pairs in HD and low-end; all 60 existing frames preserved.')
