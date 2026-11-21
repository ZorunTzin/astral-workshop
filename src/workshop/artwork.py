import json, math, random, struct
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont

def generate(root,nodes,labels,sector=None,title="",effects=None):
    sprites=root/"data/hd/global/ui/Workshop"; sprites.mkdir(parents=True,exist_ok=True)
    def sprite(name,img,frames=1):
        w,h=img.size
        header=struct.pack("<4sHH8I",b"SpA1",31,w//frames,w,h,0,frames,0,0,0,0)
        (sprites/(name+".sprite")).write_bytes(header+img.tobytes())
    def font(n): return ImageFont.truetype("C:/Windows/Fonts/georgia.ttf",n)
    w,h=1200,690
    img=Image.new("RGBA",(w,h),(13,17,26,255));draw=ImageDraw.Draw(img)
    rng=random.Random(479)
    for i in range(1600):
        x=rng.randrange(w);y=rng.randrange(h);v=rng.randrange(32,95);draw.point((x,y),fill=(v,v,v+15))
    cx,cy=600,350
    for rad in (65,96,155,205,278,343):
        draw.ellipse((cx-rad,cy-rad,cx+rad,cy+rad),outline="#363539",width=1)
    for inset,color in [(3,"#a58d54"),(8,"#514934"),(12,"#34303a")]:
        draw.rounded_rectangle((inset,inset,w-inset,h-inset),radius=17,outline=color,width=2)
    palette=["#e39c82","#a9caff","#b6ddb9"]
    for i,(name,parent,stat,value,x,y) in enumerate(nodes):
        px,py=nodes[parent][-2:] if parent>=0 else (cx,cy)
        draw.line((px,py,x,y),fill="#4c422c",width=7)
        draw.line((px,py,x,y),fill="#c0a56e",width=2)
    draw.ellipse((cx-44,cy-44,cx+44,cy+44),fill="#222533",outline="#b79e69",width=3)
    draw.text((cx,cy),"✦",font=font(42),anchor="mm",fill="#e8d8a8")
    for i,(name,p,s,n,x,y) in enumerate(nodes):
        if i>=9: draw.regular_polygon((x,y,36),6,rotation=30,outline="#ffd982")
        draw.text((x,y+33),name,font=font(22),anchor="mt",fill=palette[i//3] if i<9 else "#ffe29a",stroke_width=2,stroke_fill="#11131a")
        draw.text((x,y+60),(f"+{n} {labels[s]}" if i<9 else (effects or ["Life for Life Tap","Mana for Meditation","Resists for Teleport"])[i-9]),font=font(15),anchor="mt",fill="#eee6cf",stroke_width=1,stroke_fill="#11131a")
    draw.text((32,22),(title or "ASTRAL PASSIVES"),font=font(23),fill="#d9c69d")
    draw.text((600,650),("Carry your Sigil and a level-stamped Astral Chart. Apply to imprint the selected stars." if sector is not None else "Choose a path, inscribe a plan, then transmute it with your Astral Sigil."),anchor="mm",font=font(17),fill="#cec4ad")
    skyname="sky" if sector is None else f"atlas{sector}"
    if sector is not None:
        from sectors import SECTORS
        regions=list(SECTORS)
        draw.text((600,620),f"Crossroads: each outer star opens the matching side star in {regions[(sector+1)%9]}.",anchor="mm",font=font(16),fill="#d4be90")
    sprite(skyname,img)
    for name,size in [("star",50),("inscribe",174),("prev",150),("next",150),("load",150),("apply",174)]:
        H=50
        atlas=Image.new("RGBA",(size*4,H),(0,0,0,0));a=ImageDraw.Draw(atlas)
        for k,col in enumerate(["#9a7944","#f5d67e","#4a4a4a","#eac27b"]):
            x=k*size
            if name=="star":
                a.ellipse((x+2,2,x+47,47),fill="#202433",outline=col,width=4)
                a.polygon([(x+25,7),(x+36,25),(x+25,43),(x+14,25)],fill=col)
            else:
                a.rounded_rectangle((x+1,2,x+size-2,47),radius=12,fill="#302b25",outline=col,width=2)
                a.text((x+size/2,25),{"inscribe":"Inscribe plan","prev":"Previous","next":"Next","load":"Read sigil","apply":"Apply"}[name],font=font(19),fill="#f9e7bd",anchor="mm")
        sprite(name,atlas,4)
    glow=Image.new("RGBA",(62,62));ImageDraw.Draw(glow).ellipse((1,1,60,60),outline="#ffe598",width=4);sprite("chosen",glow)
    def widget(t,name,**fields):return dict(type=t,name=name,fields=fields)
    def rect(x,y,**kw):return dict(x=x,y=y,**kw)
    children=[]
    for i,(_,_,_,_,x,y) in enumerate(nodes):
        children.append(widget("ImageWidget","Ring"+str(i),rect=rect(x-31,y-31),filename="Workshop\\chosen",visible=False))
        children.append(widget("ButtonWidget","Node"+str(i),rect=rect(x-25,y-25),filename="Workshop\\star",pressedFrame=1,disabledFrame=2,hoveredFrame=3,onClickMessage="PanelManager:ClosePanel:WorkshopAction"+str(i)))
    children.append(widget("ButtonWidget","Inscribe",rect=rect(513,554),filename="Workshop\\inscribe",pressedFrame=1,disabledFrame=2,hoveredFrame=3,onClickMessage="PanelManager:ClosePanel:WorkshopInscribe"))
    if sector is not None:
        children[-1]["fields"]["filename"]="Workshop\\apply"
        for name,action,x in [("prev","Prev",55),("load","Load",290),("next","Next",990)]:
            children.append(widget("ButtonWidget",action,rect=rect(x,554),filename="Workshop\\"+name,pressedFrame=1,disabledFrame=2,hoveredFrame=3,onClickMessage="PanelManager:ClosePanel:Workshop"+action))
    for points in range(83 if sector is not None else 11):
        children.append(widget("TextBoxWidget","Budget"+str(points),rect=rect(280,498,width=640,height=50),visible=(points==0),text=f"{points} passive points   /   Requires character level {points+1}",style="$StyleModalDialogDescription"))
    bg=widget("ImageWidget","Sky",rect=rect(-600,-345),filename="Workshop\\"+skyname);bg["children"]=children
    panel=widget("Panel",("astral-workshop/Tree" if sector is None else f"astral-workshop/Atlas{sector}"),anchor=dict(x=.5,y=.5),priority=8500);panel["children"]=[bg]
    out=root/"workshop"/("layout.inc" if sector is None else f"atlas{sector}.inc");out.parent.mkdir(exist_ok=True)
    out.write_text('R"layout('+json.dumps(panel,ensure_ascii=True)+')layout"')
    if sector is None or sector==0: img.save(root/"workshop"/"tree-preview.png")

