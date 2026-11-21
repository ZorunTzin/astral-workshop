"""Add shared stash capacity and mouse navigation after the stash generator."""
import json,struct
from PIL import Image,ImageDraw,ImageFont
from pathlib import Path

def navigation_sprites(root):
    # Four native button states, drawn at the actual 64px widget size.
    # A double chevron is one sprite/button, not two overlapping widgets.
    folder=root/'data/hd/global/ui/Workshop'
    folder.mkdir(parents=True,exist_ok=True)
    for forward in (False,True):
        for fast in (False,True):
            name=('stashNext' if forward else 'stashPrevious')+('Ten' if fast else '')
            sheet=Image.new('RGBA',(256,64))
            for state in range(4):
                im=Image.new('RGBA',(64,64));d=ImageDraw.Draw(im)
                active=state!=3
                bg=(37,36,34,255) if state!=1 else (61,55,40,255)
                edge=(132,114,72,255) if active else (73,73,70,255)
                gold=(218,193,118,255) if active else (100,100,94,255)
                d.rectangle((1,1,62,62),fill=(14,14,14,255),outline=edge,width=2)
                d.rectangle((5,5,58,58),fill=bg,outline=(85,80,67,255),width=1)
                d.line((7,56,56,56,56,7),fill=(13,13,12,255),width=2)
                d.line((7,55,7,7,55,7),fill=(103,97,79,255),width=2)
                offset=2 if state==2 else 0
                for center in ([24,40] if fast else [32]):
                    points=[(center-6,20),(center+6,32),(center-6,44)]
                    if not forward:points=[(2*center-x,y) for x,y in points]
                    d.line([(x+offset+1,y+offset+1) for x,y in points],fill=(0,0,0,255),width=5)
                    d.line([(x+offset,y+offset) for x,y in points],fill=gold,width=4)
                sheet.paste(im,(state*64,0))
            (folder/(name+'.sprite')).write_bytes(struct.pack('<4sHH8I',b'SpA1',31,64,256,64,0,4,0,0,0,0)+sheet.tobytes())


    sheet=Image.new('RGBA',(960,64))
    font=ImageFont.truetype('C:/Windows/Fonts/georgia.ttf',26)
    for state in range(4):
        im=Image.new('RGBA',(240,64));d=ImageDraw.Draw(im)
        d.rectangle((1,1,238,62),fill=(61,55,40,255) if state==1 else (37,36,34,255),outline=(132,114,72,255),width=2)
        d.rectangle((5,5,234,58),outline=(85,80,67,255),width=1)
        offset=2 if state==2 else 0
        d.text((120+offset,32+offset),'Organizer',font=font,anchor='mm',fill=(218,193,118,255) if state!=3 else (100,100,94,255))
        sheet.paste(im,(state*240,0))
    (folder/'stashOrganizer.sprite').write_bytes(struct.pack('<4sHH8I',b'SpA1',31,240,960,64,0,4,0,0,0,0)+sheet.tobytes())

    # Native in-game organizer surfaces, with normal/hover/pressed/disabled states.
    panel=Image.new('RGBA',(1200,1160),(20,21,24,255));d=ImageDraw.Draw(panel)
    d.rectangle((2,2,1197,1157),outline=(158,134,80,255),width=4)
    d.rectangle((10,10,1189,1149),outline=(76,68,48,255),width=2)
    (folder/'organizerPanel.sprite').write_bytes(struct.pack('<4sHH8I',b'SpA1',31,1200,1200,1160,0,1,0,0,0,0)+panel.tobytes())
    for w in (80,130,230,350,500,590,1120):
        sheet=Image.new('RGBA',(w*4,48))
        for state in range(4):
            im=Image.new('RGBA',(w,48),(48,44,34,255) if state==1 else (28,28,30,255));d=ImageDraw.Draw(im)
            d.rectangle((1,1,w-2,46),outline=(125,105,65,255),width=2)
            sheet.paste(im,(state*w,0))
        (folder/('organizer'+str(w)+'.sprite')).write_bytes(struct.pack('<4sHH8I',b'SpA1',31,w,w*4,48,0,4,0,0,0,0)+sheet.tobytes())
    for i,color in enumerate([(210,210,210),(234,185,76),(102,190,126),(100,165,230),(194,134,230),(236,130,130)]):
        sheet=Image.new('RGBA',(320,48))
        for state in range(4):
            im=Image.new('RGBA',(80,48),(28,28,30,255));d=ImageDraw.Draw(im)
            d.rectangle((1,1,78,46),outline=(220,200,150,255) if state==1 else (125,105,65,255),width=2)
            d.rectangle((15,10,64,37),fill=(*color,255));sheet.paste(im,(state*80,0))
        (folder/('organizerColor'+str(i)+'.sprite')).write_bytes(struct.pack('<4sHH8I',b'SpA1',31,80,320,48,0,4,0,0,0,0)+sheet.tobytes())
    # Per-cell edge masks form one continuous border around multi-cell items.
    sheet=Image.new('RGBA',(98*17,98))
    for mask in range(17):
        im=Image.new('RGBA',(98,98));d=ImageDraw.Draw(im)
        if mask==16:d.rectangle((0,0,97,97),fill=(0,0,0,140))
        else:
            if mask&1:d.rectangle((1,1,4,96),fill=(255,215,80,255))
            if mask&2:d.rectangle((93,1,96,96),fill=(255,215,80,255))
            if mask&4:d.rectangle((1,1,96,4),fill=(255,215,80,255))
            if mask&8:d.rectangle((1,93,96,96),fill=(255,215,80,255))
        sheet.paste(im,(mask*98,0))
    (folder/'stashMatch.sprite').write_bytes(struct.pack('<4sHH8I',b'SpA1',31,98,98*17,98,0,17,0,0,0,0)+sheet.tobytes())
    # Transparent hit target over the native, dynamically updated page text.
    im=Image.new('RGBA',(360*4,64))
    (folder/'stashPageHit.sprite').write_bytes(struct.pack('<4sHH8I',b'SpA1',31,360,1440,64,0,4,0,0,0,0)+im.tobytes())

def install(root):
    root=Path(root)
    navigation_sprites(root)
    config=root/'d2rloader/config/d2rloader.toml'
    config.parent.mkdir(parents=True,exist_ok=True)
    # Dedicated mod override: preserve global loader/save-format preferences.
    text=config.read_text(encoding='utf8') if config.exists() else ''
    import re
    if '[d2rcore.stash]' in text:
        if re.search(r'^add_shared_tabs\s*=',text,re.M):
            text=re.sub(r'^add_shared_tabs\s*=.*$', 'add_shared_tabs = 995',text,flags=re.M)
        else:text=text.replace('[d2rcore.stash]','[d2rcore.stash]\nadd_shared_tabs = 995')
    else:text+='\n# 995 additional pages + 5 base shared pages = 1000 (RotW).\n[d2rcore.stash]\nadd_shared_tabs = 995\n'
    config.write_text(text,encoding='utf8')
    path=root/'data/global/ui/layouts/bankexpansionlayouthd.json'
    doc=json.loads(path.read_text(encoding='utf8'))
    def find(node,name):
        if node.get('name')==name:return node
        for child in node.get('children',[]):
            found=find(child,name)
            if found is not None:return found
    nav=find(doc,'StashNavigation')
    nav['type']='Widget'
    nav['fields']={'rect':{'x':127,'y':1506,'width':1096,'height':93}}
    buttons=[]
    for name,command,forward,fast,x in [
        ('StashLeftArrow','Previous',False,False,84),
        ('StashRightArrow','Next',True,False,548),
        ('WorkshopStashPreviousTen','PreviousTen',False,True,0),
        ('WorkshopStashNextTen','NextTen',True,True,632)]:
        sprite='Workshop\\'+('stashNext' if forward else 'stashPrevious')+('Ten' if fast else '')
        tooltip=('Next' if forward else 'Previous')+(' 10 pages (Ctrl + click: 100 pages)' if fast else ' page (Ctrl + click: 5 pages)')
        b={'type':'ButtonWidget','name':name,'fields':{
            'rect':{'x':x+400,'y':13,'width':64,'height':64},
            'filename':sprite,'normalFrame':0,'pressedFrame':2,'hoveredFrame':1,'disabledFrame':3,
            'onClickMessage':'PanelManager:ClosePanel:WorkshopStash'+command,'tooltipString':tooltip}}
        buttons.append(b)
    text=find(nav,'CurrentSharedStashText')
    text['fields']['rect']={'x':568,'y':13,'width':360,'height':64}
    text['fields']['style']['pointSize']=32
    nav['children']=[buttons[2],buttons[0],text,buttons[1],buttons[3]]
    text['fields'].pop('enabled',None)
    nav['children'].append({'type':'ButtonWidget','name':'WorkshopStashOrganizer','fields':{
        'rect':{'x':0,'y':13,'width':240,'height':64},
        'filename':'Workshop\\stashOrganizer','normalFrame':0,'pressedFrame':2,'hoveredFrame':1,'disabledFrame':3,
        'onClickMessage':'PanelManager:ClosePanel:WorkshopStashOrganizer',
        'tooltipString':'Page options: name, favorite, color, go to page and search'}})
    nav['children'].append({'type':'TextBoxWidget','name':'WorkshopStashPageTitle','fields':{
        'rect':{'x':90,'y':94,'width':440,'height':40},'text':'','enabled':False,
        'style':{'pointSize':22,'alignment':{'h':'left','v':'center'}}}})
    for c in range(6):
        nav['children'].append({'type':'ImageWidget','name':'WorkshopPageColor'+str(c),'fields':{
            'rect':{'x':0,'y':90,'width':80,'height':48},'filename':'Workshop\\organizerColor'+str(c),'visible':False,'enabled':False}})
    shared=find(doc,'SharedStashTabContainer')
    shared['children']=[n for n in shared.get('children',[]) if not n.get('name','').startswith('WorkshopMatch')]
    for y in range(13):
        for x in range(16):
            shared['children'].append({'type':'ImageWidget','name':f'WorkshopMatch{x}_{y}','fields':{
                'rect':{'x':90+x*98,'y':220+y*98,'width':98,'height':98},
                'filename':'Workshop\\stashMatch','visible':False,'enabled':False}})
    path.write_text(json.dumps(doc,indent=4)+'\n',encoding='utf8')
    licenses=root/'d2rloader/licenses';licenses.mkdir(parents=True,exist_ok=True)
    source=Path(__file__).with_name('stash-navigation-MIT.txt')
    (licenses/'d2r-access-stash-MIT.txt').write_bytes(source.read_bytes())

if __name__=='__main__':
    import sys
    install(sys.argv[1])
