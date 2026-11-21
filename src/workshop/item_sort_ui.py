from pathlib import Path
import json,struct
from PIL import Image,ImageDraw,ImageFont

def install(root):
 root=Path(root)
 folder=root/'data/hd/global/ui/Workshop';folder.mkdir(parents=True,exist_ok=True)
 sheet=Image.new('RGBA',(960,64));font=ImageFont.truetype('C:/Windows/Fonts/georgia.ttf',26)
 for state in range(4):
  im=Image.new('RGBA',(240,64));d=ImageDraw.Draw(im)
  d.rectangle((1,1,238,62),fill=(61,55,40,255) if state==1 else (37,36,34,255),outline=(132,114,72,255),width=2)
  d.rectangle((5,5,234,58),outline=(85,80,67,255),width=1)
  offset=2 if state==2 else 0
  d.text((120+offset,32+offset),'Sort',font=font,anchor='mm',fill=(218,193,118,255) if state!=3 else (100,100,94,255))
  sheet.paste(im,(state*240,0))
 (folder/'stashSort.sprite').write_bytes(struct.pack('<4sHH8I',b'SpA1',31,240,960,64,0,4,0,0,0,0)+sheet.tobytes())
 repo=Path(__file__).resolve().parents[2]
 for filename,command,x,y in [('bankexpansionlayouthd.json','Stash',1280,1519),('playerinventoryexpansionlayouthd.json','Inventory',100,1640),('playerinventoryoriginallayouthd.json','Inventory',100,1640),('horadriccubelayouthd.json','Cube',1060,140)]:
  path=root/'data/global/ui/layouts'/filename
  source=path if path.exists() else repo/'data/global/ui/layouts'/filename
  doc=json.loads(source.read_text(encoding='utf-8-sig'))
  children=doc.setdefault('children',[])
  children[:]=[c for c in children if not c.get('name','').startswith('WorkshopSort')]
  children.append(dict(type='ButtonWidget',name='WorkshopSort',fields=dict(rect=dict(x=x,y=y,width=240,height=64),filename='Workshop\\stashSort',normalFrame=0,hoveredFrame=1,pressedFrame=2,disabledFrame=3,onClickMessage='PanelManager:ClosePanel:WorkshopStashSort'+command,tooltipString='Sort current page' if command=='Stash' else 'Sort items')))
  sx,sy,sw=(1120,1590,550) if command=='Stash' else (100,1710,900) if command=='Inventory' else (200,1140,1050)
  children.append(dict(type='TextBoxWidget',name='WorkshopSortStatus',fields=dict(rect=dict(x=sx,y=sy,width=sw,height=38),text='',enabled=False,style=dict(pointSize=18))))
  path.parent.mkdir(parents=True,exist_ok=True);path.write_text(json.dumps(doc,indent=4)+'\n',encoding='utf8')
if __name__=='__main__':
 import sys
 install(sys.argv[1])
