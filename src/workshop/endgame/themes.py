"""Four themed HD arenas using retail terrain, with the established square collision map."""
from pathlib import Path
import json,struct,copy
from combat.port import read,write
THEMES=[('desert','desert1',(220,205,170)),('jungle','spidns',(175,205,170)),('infernal','chaos16x16_0',(220,155,120)),('frozen','snowpatchlg',(175,205,240))]
def install(root):
 root=Path(root);here=root/'workshop/endgame';assets=[];meta=[]
 for suffix,template,rgb in THEMES:
  scene=json.loads((here/('template-'+template+'.json')).read_text(encoding='utf-8-sig'))
  _,w,h=struct.unpack_from('<3I',(here/('template-'+template+'.ds1')).read_bytes())
  scene['entities']=[];scene['perTileBiomeOverrides']=[];scene['specialTiles']={}
  tr=next(c for c in scene['terrain']['components'] if c['type']=='TransformDefinitionComponent')
  tr['scale']['x']=32/w;tr['scale']['z']=32/h
  hd='data/hd/env/preset/workshop/astral-'+suffix+'.json';ds='data/global/tiles/workshop/astral-'+suffix+'.ds1'
  (root/hd).parent.mkdir(parents=True,exist_ok=True);(root/hd).write_text(json.dumps(scene,separators=(',',':')),encoding='utf-8')
  (root/ds).write_bytes((root/'data/global/tiles/workshop/astral-arena.ds1').read_bytes());assets.extend([hd,ds]);meta.append({'theme':suffix,'biome':scene['biomeFilename'],'sourceSize':[w,h],'hd':hd,'ds1':ds})
 for directory in ('data/global/excel','data/global/excel/base'):
  p=root/directory;h,rows=read(p/'lvlprest.txt')
  for i,(_,_,_) in enumerate(THEMES,1):
   row=next(r for r in rows if r['Name']=='Workshop '+['Verdant Trial','Desert Trial','Jungle Trial','Infernal Trial','Frozen Trial'][i]);row['File1']='Workshop/astral-'+THEMES[i-1][0]+'.ds1'
  write(p/'lvlprest.txt',h,rows)
  h,rows=read(p/'levels.txt')
  for i,(suffix,template,rgb) in enumerate(THEMES,1):
   row=next(r for r in rows if r['Name']=='Workshop '+['Verdant Trial','Desert Trial','Jungle Trial','Infernal Trial','Frozen Trial'][i]);row.update(Intensity='180',Red=str(rgb[0]),Green=str(rgb[1]),Blue=str(rgb[2]))
  write(p/'levels.txt',h,rows)
 (here/'themes.json').write_text(json.dumps(meta,indent=2))
 return assets
