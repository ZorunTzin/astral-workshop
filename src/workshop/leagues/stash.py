"""A virtual sixth tab backed by the native Materials registry (no save-format changes)."""
from pathlib import Path
import json,struct
from PIL import Image,ImageDraw,ImageFont

def install(root):
 from leagues.data import MECHANICS,CODES
 root=Path(root);assets=[];font=ImageFont.truetype('C:/Windows/Fonts/georgia.ttf',25)
 def sprite(name,im,frames=1):
  rel='data/hd/global/ui/Workshop/'+name+'.sprite';p=root/rel;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(struct.pack('<4sHH8I',b'SpA1',31,im.width//frames,im.width,im.height,0,frames,0,0,0,0)+im.tobytes());assets.append(rel)
 for name,text in [('leagueTab','League items'),('leagueMaterials','Materials')]:
  im=Image.new('RGBA',(240*4,64))
  for s in range(4):
   cell=Image.new('RGBA',(240,64),(48,44,34,255) if s==1 else (28,28,30,255));d=ImageDraw.Draw(cell);d.rectangle((2,2,237,61),outline=(145,124,76,255),width=2);d.text((120,32),text,font=font,anchor='mm',fill=(218,193,118,255));im.paste(cell,(240*s,0))
  sprite(name,im,4)
 im=Image.new('RGBA',(1560,1260),(20,21,24,255));d=ImageDraw.Draw(im);d.rectangle((2,2,1557,1257),outline=(120,104,66,255),width=3)
 d.text((60,40),'LEAGUE ITEMS',font=font,fill=(218,193,118,255));small=ImageFont.truetype('C:/Windows/Fonts/georgia.ttf',20)
 d.text((60,78),'Cube with equipment. Bonuses stack; maximum required level: 120.',font=small,fill=(155,155,150,255))
 # Three family rows keep full-size native icons above the Cube input region.
 # Names sit beside the icons, so native item art cannot cover the labels.
 for i,cost in enumerate((2,4,6,10)):
  d.text((270+i*300+49,115),f'Tier {i+1}  (+{cost} levels)',font=small,anchor='mm',fill=(218,193,118,255))
 for m,mechanic in enumerate(MECHANICS):
  y=135+m*120
  color=(180,180,195,255) if m==0 else tuple(max(95,int(c*255)) for c in mechanic['color'])+(255,)
  d.text((60,y+49),mechanic['name'],font=small,anchor='lm',fill=color)
  for i in range(4):
   x=270+i*300
   d.rectangle((x-2,y-2,x+100,y+100),fill=(10,11,13,255),outline=(125,105,65,255),width=2)
   d.text((x+112,y+35),mechanic['orbs'][i],font=small,anchor='lm',fill=(190,190,185,255))
   d.text((x+112,y+61),'Orb',font=small,anchor='lm',fill=(155,155,150,255))
 sprite('leagueBackground',im)
 for name,offset in [('bankexpansionlayouthd.json',0),('controller/bankexpansionlayouthd.json',212)]:
  path=root/'data/global/ui/layouts'/name;j=json.loads(path.read_text(encoding='utf-8-sig'));children=j['children'];children[:]=[c for c in children if not c.get('name','').startswith('League')]
  group=next(c for c in children if c['name']=='advancedstash_materials');group['children']=[c for c in group['children'] if not c.get('name','').startswith('LeagueOrb')]
  for m in range(3):
   for i in range(4):group['children'].append(dict(type='AdvancedStashSlotWidget',name=f'LeagueOrb{m}{i}',fields=dict(rect=dict(x=350+i*300,y=350+m*120-offset,width=98,height=98),itemCode=CODES[m][i])))
  children.insert(children.index(group),dict(type='ImageWidget',name='LeagueBackground',fields=dict(rect=dict(x=80,y=215-offset,width=1560,height=1260),filename='Workshop/leagueBackground',visible=False,enabled=False)))
  for key,label,x,w in [('LeagueTab','League',1064,240),('LeagueMaterialsBack','LeagueBack',670,196)]:
   children.append(dict(type='ButtonWidget',name=key,fields=dict(rect=dict(x=x,y=152-offset,width=w,height=64),filename='Workshop/'+('leagueTab' if label=='League' else 'leagueMaterials'),normalFrame=0,hoveredFrame=1,pressedFrame=2,disabledFrame=3,onClickMessage='PanelManager:ClosePanel:WorkshopStash'+label,visible=label=='League')))
  path.write_text(json.dumps(j,indent=2));assets.append(str(path.relative_to(root)).replace('\\','/'))
 return assets
