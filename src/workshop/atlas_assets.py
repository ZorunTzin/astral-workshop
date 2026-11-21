"""Compile the editable balance sheet into one shared graphical tree and the plugin definitions."""
import csv,json,math,struct,re
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
from node_text import Formatter

def export(root,positions):
    formatter=Formatter(root)
    source=Path(__file__).with_name("NODES.csv")
    with source.open(encoding="utf-8-sig",newline="") as f: rows=list(csv.DictReader(f,delimiter=";"))
    assert len(rows)==112
    tables=root/"data/global/excel"
    def ids(name,key,col):
        with (tables/(name+".txt")).open(encoding="utf-8-sig") as f:
            return {r[key]:int(r[col]) for r in csv.DictReader(f,delimiter="\t") if r[col].isdigit()}
    props=ids("properties","code","*Id");skills=ids("skills","skill","*Id");stats=ids("itemstatcost","Stat","*ID")
    families=["extra_ravens","extra_spirit_wolves","extra_dire_wolves","extra_grizzly_bears","extra_skelewarriors","extra_skelemages","extra_firegolems","extra_shadow_masters"]
    with Path(__file__).with_name("NODES-step2.csv").open(encoding="utf-8-sig",newline="") as f:
        draft={r["name"]:r for r in csv.DictReader(f,delimiter=";")}
    assert len(draft)==len(rows)
    inner_design=json.loads(Path(__file__).with_name("INNER-CLUSTERS.json").read_text(encoding="utf8"))
    assert len(inner_design)==27
    overrides=json.loads(Path(__file__).with_name("CLUSTER-OVERRIDES.json").read_text(encoding="utf8"))
    inner_effects={}
    suffix={"maximum all resistances":"res-all-max","cold skill damage":"extra-cold","fire skill damage":"extra-fire",
        "lightning skill damage":"extra-ltng","poison skill damage":"extra-pois","enhanced weapon damage":"dmg%",
        "faster hit recovery":"balance2","faster run/walk":"move2","increased attack speed":"swing2",
        "minion attack and movement speed":"ws-pet-speed","maximum weapon damage":"dmg-max","faster block rate":"block2",
        "magic find":"mag%","mana regeneration":"regen-mana","minion damage":"ws-pet-damage","maximum mana":"mana",
        "strength":"str%","dexterity":"dex%","vitality":"vit%","energy":"enr%",
        "Replenish Life":"regen","minion maximum life":"ws-pet-life","maximum life":"hp","crushing blow":"crush",
        "life after each kill":"heal-kill","all resistances":"res-all","chance to block":"block","physical damage reduction":"red-dmg%",
        "extra gold":"gold%","attack rating":"att","minimum weapon damage":"dmg-min","deadly strike":"deadly",
        **{"enemy "+k+" resistance":v for k,v in (("cold","pierce-cold"),("fire","pierce-fire"),("lightning","pierce-ltng"),("magic","pierce-mag"),("poison","pierce-pois"))}}
    clusters={};tails={}
    keys=set();tip={};coords={};source=['struct Star { const char* key; unsigned property; int param, lo, hi; };','constexpr Star stars[][16]={']
    regions=list(dict.fromkeys(row["region"] for row in rows));assert len(regions)==9
    colors=["#d9a85e","#ca866c","#a2c2d9","#dfa0db","#8fb992","#b69cdb","#92c6b0","#96a7ee","#d6c486"]
    w,h=1470,870;cx,cy=735,391
    screen_w,screen_h=3620,2020
    sx,sy=screen_w/w,screen_h/h
    artwork=Path(__file__).with_name("art")/"astral-sanctum"
    img=Image.open(artwork/"background.png").convert("RGBA").resize((w,h),Image.Resampling.LANCZOS)
    node_art={kind:Image.open(artwork/(name+".png")).convert("RGBA") for kind,name in (("minor","normal"),("finish","cluster"),("major","keystone"))}
    draw=ImageDraw.Draw(img)
    font=lambda n:ImageFont.truetype("C:/Windows/Fonts/georgia.ttf",n)
    def point(a,r):return (round(cx+655*r*math.cos(a)),round(cy+320*r*math.sin(a)))
    def widget(t,n,**fields):return dict(type=t,name=n,fields=fields)
    def loc(x,y,**kw):return dict(x=x,y=y,**kw)
    def sprite(name,im,frames=1,native=False):
        if not native:
            fw=round(im.width/frames*sx);fh=round(im.height*sy)
            im=im.resize((fw*frames,fh),Image.Resampling.LANCZOS)
        sw,sh=im.size;(root/"data/hd/global/ui/Workshop"/(name+".sprite")).write_bytes(struct.pack("<4sHH8I",b"SpA1",31,sw//frames,sw,sh,0,frames,0,0,0,0)+im.tobytes())
    guide=['# Astral tree','','Maximum three keystones. Slots unlock at character levels 30, 60 and 90. Each costs one point. Keystones add their bonus without removing learned cluster effects.','']
    for p,region in enumerate(regions):
        by={(int(v["path"])-1,v["slot"]):v for v in rows if v["region"]==region}
        count=16 if region=="Legion" else 12
        assert len(by)==count
        source.append("{");guide+=['## '+region,'']
        for i in range(count):
            path=i//3 if i<9 else i-9;slot=i%3 if i<9 else 3
            row=by[3,("Heart","Fury","Rush","Swarm")[i-12]] if i>=12 else by[path,("Inner","Outer","Side","Keystone")[slot]]
            key=row["name"].replace(" ","");assert key not in keys,key
            keys.add(key);prop=row["property"];param=row["parameter"]
            param=int(param or 0) if not param or param.isdigit() else skills[param]
            lo=int(row["minimum"]);hi=int(row["maximum"]);assert lo>=0 and hi>=0
            number=65535 if prop=="ws-minion-limit" else props.get("ws-wide-"+prop,props[prop])
            source.append(f'{{{json.dumps(key)},{number},{param},{lo},{hi}}},')
            effect=formatter.effect(prop,row['parameter'],lo,hi)
            if 9<=i<12 and overrides.get(row['name'],{}).get('keystone_extra'):
                effect+='\n'+formatter.many(overrides[row['name']]['keystone_extra'])
            if 9<=i<12:effect+='\nCost: 1 point\nKeystone slots unlock at character levels 30 / 60 / 90 (max 3)\nOther learned node bonuses remain active.'
            if row["name"]=="Alchemist":effect+="\nTreasure gold bonus is retained."
            name=row["name"];n=12*p+i if i<12 else 108+i-12
            spoke=p*3+path+(1 if p>6 else 0)
            if i>=12:spoke=21
            a=-math.pi/2+spoke*math.tau/28
            radius=(.34,.74,.54,.96)[slot] if i<12 else (.34,.54,.74,.94)[i-12]
            coords[p,i]=point(a,radius)
            if not 9<=i<12:
                plan=draft[name];assert plan['property']=='all-stats' and plan['minimum']==plan['maximum']=='3'
                effect="+3 to all attributes"
                base=[]
                for rank,col in enumerate(("Knoten 2","Knoten 3","Knoten 4","Knoten 5 mit Bonus"),1):
                    custom=overrides.get(name,{}).get("ranks",[])
                    if slot==0 or rank<=len(custom):
                        definition=inner_design[name]["ranks"][rank-1] if slot==0 else custom[rank-1]
                        assert definition["label"]==plan[col],(name,col)
                        effects=[];bonus_mask=0
                        for term in definition["effects"]:
                            codes=("str","dex","vit","enr") if term["code"]=="all-stats" else (term["code"],)
                            for code in codes:
                                prop_id=props.get("ws-wide-"+code,props[code])
                                parameter=term["parameter"]
                                if isinstance(parameter,str):parameter=skills[parameter]
                                if term["bonus"]:bonus_mask|=1<<len(effects)
                                effects.append((prop_id,parameter,term["minimum"],term["maximum"]))
                        assert len(effects)<=5
                        inner_effects[n,rank-1]=(effects,bonus_mask)
                        base.append((0,0,0))
                    else:
                        primary,*bonus=plan[col].split(" / ")
                        v=int(re.match(r"^[+](\d+)",primary)[1])
                        bp=bv=0
                        if bonus:
                            m=re.fullmatch(r".*?(\d+)%? (.+)",bonus[0]);assert m,bonus
                            code=suffix[m[2]]; bp=props.get("ws-wide-"+code,props[code]);bv=int(m[1])
                        base.append((v,bp,bv))
                    # Follow the orbit clockwise, then fold back inside the wedge.
                    off,dr=((.37,.038),(.79,-.018),(.67,-.12),(.23,-.09))[rank-1]
                    uid=112+len(tails)
                    tails[uid]=(p,i,rank,point(a+off*math.tau/28,radius+dr))
                    if slot==0 or rank<=len(custom):rank_text=formatter.many(definition['effects'])
                    else:
                        rank_text=formatter.effect(prop,row['parameter'],v,v)
                        if bonus:
                            extra=formatter.effect(code,0,bv,bv)
                            rank_text+='\n'+extra if rank_text else extra
                    tip[f"WorkshopNode{uid}"]=f"{inner_design[name]['title'] if slot==0 else name} {rank+1}/5\n"+(rank_text or 'Unlocks the next node')
                clusters[n]=base
            text=name+"\n"+effect;tip[f"WorkshopNode{n}"]=text;guide.append("- "+text)
        source.append("},");guide.append("")
    source+=["};",f'constexpr unsigned levelRequirement={props["levelreq"]}, sigilRow={ids("uniqueitems","index","*ID")["WorkshopAstralSigil"]};',f'constexpr unsigned firstMaskStat={stats["workshop_sector_0"]},firstMaskProperty={props["ws-page0"]};',"constexpr unsigned minionProperties[]={"+",".join(str(props[k]) for k in families)+"};"]
    source.append(f"constexpr unsigned pointRequirement={props['ws-required-level']}, pointRequirementStat={stats['ws_required_level']};")
    source.append(f"constexpr unsigned summonCountProperty={props['ws-pet-count']};")
    source.append(f"constexpr unsigned flatUnit={props['ws-wide-flat-unit']};")
    source.append(f"constexpr unsigned petMaskStat={stats['ws_pet_mask']},petMaskProperty={props['ws-pet-mask']};")
    # Prefer native stats within their save limits. Overflow uses the wider
    # custom stat; fixed-point vitality resources need 256 units per point.
    itemstats={r["Stat"]:r for r in csv.DictReader((tables/"itemstatcost.txt").open(),delimiter="\t")}
    properties={r["code"]:r for r in csv.DictReader((tables/"properties.txt").open(),delimiter="\t")}
    source.append('struct PropertyAudit {unsigned property,stat;}; constexpr PropertyAudit propertyAudit[]={')
    source.extend('{%d,%d},'%(int(r['*Id']),stats[r['stat1']]) for r in properties.values() if r['code'].startswith('ws-') and r['stat1'] in stats)
    source.append('};')
    source.append("struct WideStat {unsigned wide, native; int limit, scale;}; constexpr WideStat wideStats[]={")
    for k in ("mana","stam","att","light","regen","ac-miss","move2","str","dex","vit","enr","balance2"):
        st=itemstats[properties[k]["stat1"]]
        limit=(1<<int(st["Save Bits"]))-1-int(st["Save Add"] or 0)
        source.append(f"{{{props['ws-wide-'+k]},{props[k]},{limit},{1<<int(st['ValShift'] or 0)}}},")
    source.append(f"{{{props['ws-light-min-wide']},{props['ws-light-min-native']},63,1}},")
    source.append(f"{{{props['ws-poison-rate-wide']},{props['ws-poison-rate-native']},1023,1}},")
    source.append("};")
    source.append(f"constexpr unsigned lightningDamageProperty={props['dmg-ltng']},lightningMinimumWideProperty={props['ws-light-min-wide']},lightningMaximumProperty={props['ws-light-max-native']};")
    source.append(f"constexpr unsigned poisonDamageProperty={props['dmg-pois']},poisonRateWideProperty={props['ws-poison-rate-wide']},poisonDurationProperty={props['ws-poison-duration-native']};")
    source.append("struct Rank {int value;unsigned bonus;int extra;}; constexpr Rank ranks[112][4]={")
    for n in range(112):
        source.append("{"+",".join("{%d,%d,%d}"%r for r in clusters.get(n,()))+"},")
    source.append("};")
    source.append("struct InnerRank {unsigned count,bonusMask; Items::PropertySpec effects[5];}; constexpr InnerRank innerRanks[112][4]={")
    for n in range(112):
        entries=[]
        for rank in range(4):
            terms,mask=inner_effects.get((n,rank),([],0))
            entries.append("{%d,%d,{%s}}"%(len(terms),mask,",".join("{%d,%d,%d,%d}"%term for term in terms)))
        source.append("{"+",".join(entries)+"},")
    source.append("};")
    # Additive Keystone effects preserve the existing primary payload and IDs.
    source.append("constexpr InnerRank keystoneExtras[112]={")
    extras={}
    for row in rows:
        if row['slot']!='Keystone':continue
        p=regions.index(row['region']);n=12*p+9+int(row['path'])-1
        terms=[]
        for term in overrides.get(row['name'],{}).get('keystone_extra',[]):
            code=term['code'];parameter=term['parameter']
            if isinstance(parameter,str):parameter=skills[parameter]
            terms.append((props.get('ws-wide-'+code,props[code]),parameter,term['minimum'],term['maximum']))
        extras[n]=terms
    for n in range(112):
        terms=extras.get(n,[])
        assert len(terms)<=5
        source.append('{%d,0,{%s}},'%(len(terms),','.join('{%d,%d,%d,%d}'%term for term in terms)))
    source.append('};')
    source.append("constexpr unsigned retiredRoot=108, activeNodeCount=447;")
    source.append(f"constexpr unsigned strikeProperty={props['hit-skill']}, killProperty={props['kill-skill']}, attackProperty={props['att-skill']};")
    source+=["struct Extension {unsigned region,node,rank;}; constexpr Extension extensions[]={"]
    source.extend("{%d,%d,%d},"%v[:3] for v in tails.values())
    source+=["};","constexpr unsigned attributes[]={"+",".join(str(props["ws-wide-"+k]) for k in ("str","dex","vit","enr"))+"};"]
    assert len(tails)==340 and len(clusters)==85
    from tree_layout import build_layout,diameter
    positions_by_id,edges,parents,labels,stems=build_layout(tails)
    source.append("constexpr unsigned parentNodes[112]={"+",".join(map(str,parents))+"};")
    dest=root/"workshop";(dest/"stars.inc").write_text("\n".join(source));(dest/"CONSTELLATIONS.md").write_text("\n".join(guide),encoding="utf8")
    for uid,(p,i,rank,xy) in list(tails.items()):
        if uid in positions_by_id:tails[uid]=(p,i,rank,positions_by_id[uid])
    children=[]
    for start,end,region,branch in edges:
        draw.line((positions_by_id[start],positions_by_id[end]),fill=colors[region],width=3 if branch else 2)
    for p,(ax,ay) in labels.items():
        draw.text((ax,ay),regions[p],font=font(15),anchor="mm",fill=colors[p],stroke_width=2,stroke_fill="#10071a")
    img.alpha_composite(node_art["minor"].resize((48,48),Image.Resampling.LANCZOS),(cx-24,cy-24))
    draw.rounded_rectangle((3,3,w-4,h-4),radius=20,outline="#89784f",width=2)
    draw.text((30,20),"ASTRAL PASSIVES",font=font(24),fill="#e7d6ac")
    draw.text((w//2,835),"Apply creates your Astral Sigil directly in your inventory.",anchor="mm",font=font(17),fill="#d0bea0")
    sprite("unified",img);img.save(dest/"tree-preview.png")
    # Pack the high-resolution originals directly at native UI pixel size.
    # Avoid reducing them to logical pixels and upscaling again: the crescent
    # and triskelion must remain sharp on a 4K display.
    for kind,size in [("minor",14),("major",20),("finish",16)]:
        fw,fh=round(size*sx),round(size*sy)
        frame=node_art[kind].resize((fw,fh),Image.Resampling.LANCZOS)
        sheet=Image.new("RGBA",(fw*4,fh))
        for state in range(4):
            tile=frame.copy()
            # Native press/hover feedback is a separate UI outline; the
            # approved PNG itself is preserved unchanged on disk.
            if state in (1,3):
                pen=ImageDraw.Draw(tile)
                pen.ellipse((1,1,fw-2,fh-2),outline="#ffe795" if state==1 else "#e6b8ff",width=2)
            sheet.alpha_composite(tile,(state*fw,0))
        sprite(kind,sheet,4,native=True)
    halo=Image.new("RGBA",(22,22));pen=ImageDraw.Draw(halo)
    pen.ellipse((1,1,20,20),outline="#ffe795",width=2)
    sprite("halo",halo)
    for n,(x,y) in positions_by_id.items():
        key=n<108 and n%12>=9
        size=diameter(n)
        children.append(widget("ImageWidget","Ring"+str(n),rect=loc(x-11,y-11),filename="Workshop\\halo",visible=False,enabled=False))
        children.append(widget("ButtonWidget","Node"+str(n),rect=loc(x-size//2,y-size//2),filename="Workshop\\"+("major" if key else "finish" if size==16 else "minor"),pressedFrame=1,disabledFrame=2,hoveredFrame=3,tooltipString="@d2r:WorkshopNode"+str(n),))
    for key,label,x in [("Reset","Reset",360),("Inscribe","Apply",950)]:
        im=Image.new("RGBA",(640,46));b=ImageDraw.Draw(im)
        for j in range(4):
            x0=160*j;b.rounded_rectangle((x0+1,1,x0+158,44),8,fill="#352f25",outline="#c2a06c",width=2);b.text((x0+80,23),label,anchor="mm",font=font(18),fill="#f3dfaf")
        sprite("btn"+key,im,4);children.append(widget("ButtonWidget",key,rect=loc(x,766),filename="Workshop\\btn"+key,pressedFrame=1,disabledFrame=2,hoveredFrame=3,onClickMessage="PanelManager:ClosePanel:Workshop"+key))
    # Each segment has its own physical location. Even on old loader builds
    # which ignore initial visibility, several strings can never overlap.
    draw.text((733,744),"POINTS",font=font(16),anchor="mm",fill="#dac8a3")
    sprite("unified",img)
    preview=img.copy()
    pd=ImageDraw.Draw(preview)
    for uid,(x,y) in positions_by_id.items():
        size=diameter(uid)
        kind="major" if uid<108 and uid%12>=9 else "finish" if size==16 else "minor"
        preview.alpha_composite(node_art[kind].resize((size,size),Image.Resampling.LANCZOS),(x-size//2,y-size//2))
    preview.save(dest/"tree-preview.png")
    bars=[(2,0,13,3),(13,3,16,13),(13,15,16,25),(2,25,13,28),(0,15,3,25),(0,3,3,13),(2,12,13,15)]
    for digit in range(2):
        for j,(x1,y1,x2,y2) in enumerate(bars):
            name="vbar" if x2-x1<y2-y1 else "hbar"
            tile=Image.new("RGBA",(x2-x1,y2-y1),"#f1d8a5")
            sprite(name,tile)
            children.append(widget("ImageWidget",f"Digit{digit}Bar{j}",rect=loc(709+digit*26+x1,769+y1),filename="Workshop\\"+name,visible=False,enabled=False))
    children=[v for v in children if v["name"] not in ("Load",)]
    sprite("unified",preview)
    geometry=['struct VisualNode {float x,y,r; const char* tooltip;}; constexpr VisualNode visualNodes[]={']
    for uid in range(452):
        if uid not in positions_by_id:
            geometry.append('{-10000,-10000,0,""},')
            continue
        x,y=positions_by_id[uid]
        geometry.append("{%d,%d,%d,%s},"%(x,y,diameter(uid)//2+1,json.dumps(tip["WorkshopNode"+str(uid)].replace(chr(8722),"-"))))
    geometry.append("};")
    (dest/"visual.inc").write_text("\n".join(geometry),encoding="utf8")
    # Direct panel children match the widget paths accepted by older loaders.
    for element in children:
        rect=element["fields"]["rect"]
        rect["x"]=round(rect["x"]*sx)-screen_w//2
        rect["y"]=round(rect["y"]*sy)-screen_h//2
    bg=widget("ButtonWidget","Surface",rect=loc(-screen_w//2,-screen_h//2),filename="Workshop\\unified",pressedFrame=0,hoveredFrame=0,disabledFrame=0,onClickMessage="PanelManager:ClosePanel:WorkshopSurface")
    panel=widget("Panel","astral-workshop/Atlas",anchor=dict(x=.5,y=.5),priority=8500)
    panel["children"]=[bg,*children]
    literal=json.dumps(panel);(dest/"unified.inc").write_text("\n".join('R"layout('+literal[j:j+6000]+')layout"' for j in range(0,len(literal),6000)))
    file=root/"data/local/lng/strings/item-names.json";data=json.loads(file.read_text(encoding="utf-8-sig"))
    # Other feature generators also append localized skills. Allocate after all
    # string tables so node IDs cannot collide with those skill descriptions.
    last=max(v["id"] for path in file.parent.glob("*.json") for v in json.loads(path.read_text(encoding="utf-8-sig")) if isinstance(v,dict) and 'id' in v)
    added=[{"id":last+n,"Key":k,**{lang:t for lang in data[0] if lang not in ("id","Key")}} for n,(k,t) in enumerate(tip.items(),1)]
    raw=file.read_bytes().rstrip();file.write_bytes(raw[:-1].rstrip()+b",\n"+json.dumps(added,indent=2,ensure_ascii=False)[1:].encode()+b"\n")
