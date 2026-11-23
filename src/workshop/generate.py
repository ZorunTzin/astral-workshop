"""Generate the paired Reimagined tables and the clickable constellation panel."""
import csv, io, json, math, random, struct, sys
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

root = Path(sys.argv[1])
here = Path(__file__).resolve().parent
# Each root unlocks two independent leaves. Coordinates also drive the panel artwork.
nodes = [
    ("Ember", -1, "hp", 10, 380, 310), ("Bloodstone", 0, "hp", 25, 245, 215),
    ("Fleetfoot", 0, "move2", 5, 260, 430), ("Aether", -1, "mana", 10, 600, 290),
    ("Deep Well", 3, "mana", 25, 495, 175), ("Quickening", 3, "move2", 5, 720, 180),
    ("Ward", -1, "res-all", 3, 820, 310), ("Prism", 6, "res-all", 7, 955, 215),
    ("Wayfarer", 6, "move2", 5, 940, 430),
    ("Blood Tithe", 1, "blood", 0, 150, 82), ("Inner Choir", 4, "choir", 0, 565, 53),
    ("Rift Walker", 7, "rift", 0, 1050, 82)]
labels = {"hp":"Life","mana":"Mana","res-all":"All resistances","move2":"Run/walk %","cast2":"Cast rate %"}
valid = [m for m in range(512) if all(not m>>i&1 or p<0 or m>>p&1 for i,(_,p,*_) in enumerate(nodes))]
small_masks = valid.copy()
for key,parent in enumerate((1,4,7),9):
    valid.extend(mask | (1 << key) for mask in small_masks if mask & (1 << parent))
assert len(valid)==275
names={}
def code(m): return f"W{m:03X}"
def read(p):
    raw=p.read_bytes()
    return list(csv.DictReader(io.StringIO(raw.decode("utf-8-sig")),delimiter="\t")),raw
def append(p,rows):
    old,raw=read(p)
    buf=io.StringIO()
    writer=csv.DictWriter(buf,fieldnames=old[0].keys(),delimiter="\t",lineterminator="\r\n",extrasaction="raise")
    writer.writerows(rows)
    p.write_bytes(raw.rstrip(b"\r\n")+b"\r\n"+buf.getvalue().encode())
def rec(name,inputs,output,mods=(),**more):
    r={"description":"Workshop - "+name,"enabled":1,"version":100,"numinputs":len(inputs),"output":output,"*eol":0,**more}
    for i,v in enumerate(inputs,1): r[f"input {i}"]=v
    for i,(prop,n) in enumerate(mods,1):
        assert i<=5
        r.update({f"mod {i}":prop,f"mod {i} min":n,f"mod {i} max":n})
    return r
def tables(d):
    misc,_=read(d/"misc.txt"); uniques,_=read(d/"uniqueitems.txt"); stats,_=read(d/"itemstatcost.txt")
    props,_=read(d/"properties.txt")
    assert not any(r["code"]=="Wsig" for r in misc), "Run against a clean Reimagined checkout"
    marker=max(int(s["*ID"]) for s in stats if s["*ID"].isdigit())+1
    assert marker<511
    extra=[]; recipes=[]
    def item(k,name,template):
        r=next(x.copy() for x in misc if x["code"]==template)
        for col in r:
            if col.endswith(("Max","Min","MagicLvl")): r[col]=""
        r.update(code=k,name=name,namestr=k,normcode=k,ubercode="",ultracode="",spawnable="0",cost="0",PermStoreItem="",multibuy="")
        if template!="cm1":
            r.update(type="ques",type2="",useable="0",pSpell="",spelldesc="",spelldescstr="",spelldescstr2="",spelldesccalc="",stackable="",quest="",gemapplytype="")
        extra.append(r); names[k]=name
    item("Wsig","Astral Sigil","cm1")
    uname="WorkshopAstralSigil"; names[uname]="Astral Sigil"
    uid=max(int(x["*ID"]) for x in uniques if x["*ID"].isdigit())+1
    append(d/"uniqueitems.txt",[{"index":uname,"*ID":uid,"version":100,"spawnable":1,"rarity":1,"nolimit":1,"lvl":1,"lvl req":1,"code":"Wsig","carry1":1,"*eol":0}])
    recipes.append(rec("Blank sigil",["isc","tsc","yps"],uname))
    for m in valid:
        item(code(m),f"Constellation plan {m:03X} (level {m.bit_count()+1})","isc")
        bonuses={}
        for i,(_,_,s,n,*_) in enumerate(nodes[:9]):
            if m>>i&1: bonuses[s]=bonuses.get(s,0)+n
        special=None
        if m & (1<<9):
            special=("hit-skill","Life Tap",5,bonuses.pop("hp")//5)
        elif m & (1<<10):
            rank=bonuses.pop("mana")//10
            special=("aura","Meditation",rank,rank)
        elif m & (1<<11):
            bonuses.pop("res-all")
            special=("oskill","Teleport",1,1)
        if m: bonuses["levelreq"]=m.bit_count()
        recipes.append(rec("Attune "+code(m),["Wsig",code(m)],uname,bonuses.items(),op=3,param=12,value=m.bit_count()+1))
        if special:
            slot=len(bonuses)+1
            assert slot<=5
            prop,param,minimum,maximum=special
            recipes[-1].update({f"mod {slot}":prop,f"mod {slot} param":param,
                                f"mod {slot} min":minimum,f"mod {slot} max":maximum})
    names["WorkshopForged"]="Runeforged"
    append(d/"itemstatcost.txt",[{"Stat":"workshop_forged","*ID":marker,"Send Bits":2,"Save Bits":2,"descpriority":300,"descfunc":0,"descval":0,"descstrpos":"WorkshopForged","descstrneg":"WorkshopForged","*eol":0}])
    append(d/"properties.txt",[{"code":"ws-forged","*Id":max(int(x["*Id"]) for x in props if x["*Id"].isdigit())+1,"func1":1,"stat1":"workshop_forged","*eol":0}])
    item("Wcap","Astral Chart","isc")
    extra[-1]["compactsave"]="0"  # Persist the character-level stamp across save/load.
    recipes.append(rec("Read the stars",["isc","tsc","hp1"],"Wcap",plvl=100))
    item("Wpln","Inscribed Astral Plan","isc")
    extra[-1]["compactsave"]="0"
    # Nine compact, item-owned masks persist the allocation through save/load.
    stat_rows=[]; prop_rows=[]
    next_prop=max(int(x["*Id"]) for x in props if x["*Id"].isdigit())+2
    for sector in range(9):
        stat=f"workshop_sector_{sector}"
        stat_rows.append({"Stat":stat,"*ID":marker+1+sector,"Send Bits":12,"Save Bits":12,"*eol":0})
        prop_rows.append({"code":f"ws-page{sector}","*Id":next_prop+sector,"func1":1,"stat1":stat,"*eol":0})
    # The original item stats have narrow save fields. Use additive op-2
    # modifiers with a constant multiplier on the sigil, preserving existing saves.
    originals={s["Stat"]:s for s in stats}
    direct={p["code"]:p["stat1"] for p in props}
    widened=("move2","mana","att","stam","light","ac-miss","regen")
    unit_stat="ws_flat_unit"
    for j,code_ in enumerate(("flat-unit",)+widened+("str","dex","vit","enr")):
        name=unit_stat if j==0 else "ws_flat_"+str(j)
        offset=j if j<8 else j+5
        entry={"Stat":name,"*ID":marker+10+offset,"Send Bits":32,"Save Bits":16,"*eol":0}
        if j:
            target=originals[direct[code_]]
            entry.update({k:target[k] for k in ("ValShift","descpriority","descfunc","descval","descstrpos","descstrneg")})
            entry.update({"op":2,"op base":unit_stat,"op param":0,"op stat1":target["Stat"]})
        stat_rows.append(entry)
        prop_rows.append({"code":"ws-wide-"+code_,"*Id":next_prop+9+offset,"func1":1,"stat1":name,"*eol":0})
    for j,name in enumerate(("life","damage","speed","count","mask")):
        stat="ws_pet_"+name
        row={"Stat":stat,"*ID":marker+18+j,"Send Bits":12,"Save Bits":12,"*eol":0}
        if name!="mask":
            label="WorkshopPet"+name
            names[label]={"life":"Minion Maximum Life (%)","damage":"Minion Damage (%)","speed":"Minion Attack and Movement Speed (%)","count":"Maximum Summons"}[name]
            row.update(descpriority=350+j,descfunc=1,descval=1,descstrpos=label,descstrneg=label)
        stat_rows.append(row)
        prop_rows.append({"code":"ws-pet-"+name,"*Id":next_prop+17+j,"func1":1,"stat1":stat,"*eol":0})
    stat_rows.append({"Stat":"ws_required_level","*ID":marker+27,"Send Bits":7,"Save Bits":7,"*eol":0})
    prop_rows.append({"code":"ws-required-level","*Id":next_prop+26,"func1":1,"stat1":"ws_required_level","*eol":0})
    # Preserve all existing summon formulas and the special one-bear restriction.
    skillpath=d/"skills.txt"
    skills,raw=read(skillpath)
    families={"Raven":"extra_ravens","Raise Skeleton":"extra_skelewarriors","Raise Skeletal Mage":"extra_skelemages",
              "FireGolem":"extra_firegolems","Summon Spirit Wolf":"extra_spirit_wolves","Summon Fenris":"extra_dire_wolves",
              "Summon Grizzly":"extra_grizzly_bears","Shadow Master":"extra_shadow_masters"}
    changed=set()
    lines=raw.splitlines(keepends=True)
    for ix,skill in enumerate(skills,1):
        name=skill["skill"]
        if name not in families:continue
        term="stat('"+families[name]+"'.accr)"
        assert term in skill["petmax"]
        skill["petmax"]=skill["petmax"].replace(term,"("+term+"+stat('ws_pet_count'.accr))")
        bonuses=[("item_maxhp_percent","life"),("damagepercent","damage"),("velocitypercent","speed"),("attackrate","speed")]
        if name=="Raise Skeletal Mage":
            bonuses.extend(("passive_"+element+"_mastery","damage") for element in ("fire","ltng","cold","pois"))
            bonuses.append(("item_fastercastrate","speed"))
        elif name=="FireGolem":bonuses.append(("passive_fire_mastery","damage"))
        elif name in ("Summon Spirit Wolf","Summon Fenris"):
            for i in range(1,15):
                if skill[f"passivestat{i}"] in ("coldmindam","coldmaxdam","firemindam","firemaxdam"):
                    field=f"passivecalc{i}";skill[field]=f"({skill[field]})*(100+stat('ws_pet_damage'.accr))/100"
        for target,bonus in bonuses:
            slots=list(range(1,15))
            slot=next((i for i in slots if skill[f"passivestat{i}"]==target),None)
            if slot is None:slot=next(i for i in slots if not skill[f"passivestat{i}"])
            skill[f"passivestat{slot}"]=target
            field=f"passivecalc{slot}";old=skill[field]
            skill[field]=(f"({old})+" if old else "")+f"stat('ws_pet_{bonus}'.accr)"
        buf=io.StringIO();csv.DictWriter(buf,fieldnames=skills[0].keys(),delimiter="\t",lineterminator="\r\n").writerow(skill)
        lines[ix]=buf.getvalue().encode();changed.add(name)
    assert changed==families.keys()
    skillpath.write_bytes(b"".join(lines))
    append(d/"itemstatcost.txt",stat_rows); append(d/"properties.txt",prop_rows)
    from table_ids import normalize
    normalize(d)
    from forging import generate_forging
    generate_forging(d, here, marker, read, append, rec, item, recipes, names)
    assert not set(x["code"] for x in extra)&set(x["code"] for x in misc)
    append(d/"misc.txt",extra); append(d/"cubemain.txt",recipes)
    return recipes
excel=root/"data/global/excel"
assert tables(excel)==tables(excel/"base")
strings=root/"data/local/lng/strings"
def jsonrows(p): return json.loads(p.read_text(encoding="utf-8-sig"))
langfile=strings/"item-names.json"; rows=jsonrows(langfile)
last=max(x["id"] for f in strings.glob("*.json") for x in jsonrows(f) if isinstance(x,dict) and "id" in x)
locales=[k for k in rows[0] if k not in ("id","Key")]
for i,(k,name) in enumerate(names.items(),last+1):
    assert not any(x["Key"]==k for x in rows)
    rows.append({"id":i,"Key":k,**dict.fromkeys(locales,name)})
# Preserve the original localization formatting and append only our records.
raw=langfile.read_bytes().rstrip(); assert raw.endswith(b"]")
langfile.write_bytes(raw[:-1].rstrip()+b",\n"+json.dumps(rows[-len(names):],ensure_ascii=False,indent=4)[1:].encode()+b"\n")
(root/"modinfo.json").write_text(json.dumps(dict(name="ReimaginedWorkshop",version="0.1.0",savepath="ReimaginedWorkshop/"),indent=2))

from artwork import generate
generate(root,nodes,labels)
from atlas_assets import export

from item_visuals import install as install_item_visuals
install_item_visuals(root, valid)
from combat.port import install as install_curse_port
install_curse_port(root)
from combat.aura_data import install as install_aura_data
install_aura_data(root)
from combat.aura_features_data import install as install_aura_features
install_aura_features(root)
from combat.guardian_data import install as install_guardian
install_guardian(root)
from combat.ground_visual_data import install as install_ground_visual
install_ground_visual(root)
from combat.item_auras_data import install as install_item_auras
install_item_auras(root)
from tuning import install as install_tuning
install_tuning(root)
from paladin_balance import install as install_paladin_balance
install_paladin_balance(root)
from paladin_update import install as install_paladin_update
install_paladin_update(root)
from paladin_rebalance import install as install_paladin_rebalance
install_paladin_rebalance(root)
from summon_limits import install as install_summon_limits
install_summon_limits(root)
from combat.paladin_icons import install as install_paladin_icons
install_paladin_icons(root)
from sigil_visuals import install as install_sigil_visuals
install_sigil_visuals(root)
from affix_scaling import export as export_affix_scaling
export_affix_scaling(root)

from set_reforge import install as install_set_reforge
install_set_reforge(root)

from integration.prepare import install as install_integrations
install_integrations(root)

from buff_hud import install as install_buff_hud
install_buff_hud(root)

# Append wider FHR storage after all established records. Never insert or resize
# an existing saved stat: that would reinterpret older item save streams.
from combat.port import read as read_table, write as write_table
from table_ids import normalize
for directory in (excel,excel/'base'):
    header,stat_rows=read_table(directory/'itemstatcost.txt')
    target=next(r for r in stat_rows if r['Stat']=='item_fastergethitrate')
    entry={k:'' for k in header}
    entry.update(Stat='ws_flat_fhr',**{'Send Bits':'32','Save Bits':'16','op':'2','op base':'ws_flat_unit','op param':'0','op stat1':target['Stat'],'*eol':'0'})
    entry.update({k:target[k] for k in ('ValShift','descpriority','descfunc','descval','descstrpos','descstrneg')})
    assert len(stat_rows)==508 and not any(r['Stat']==entry['Stat'] for r in stat_rows)
    stat_rows.append(entry);write_table(directory/'itemstatcost.txt',header,stat_rows)
    header,prop_rows=read_table(directory/'properties.txt')
    entry={k:'' for k in header};entry.update(code='ws-wide-balance2',func1='1',stat1='ws_flat_fhr',**{'*eol':'0'})
    prop_rows.append(entry);write_table(directory/'properties.txt',header,prop_rows)
    # Lightning minimum damage has a six-bit native item save field (63).
    # Append a wider op-2 contribution rather than resizing any existing stat.
    header,stat_rows=read_table(directory/'itemstatcost.txt')
    target=next(r for r in stat_rows if r['Stat']=='lightmindam')
    entry={k:'' for k in header}
    entry.update(Stat='ws_flat_light_min',**{'Send Bits':'32','Save Bits':'16','op':'2','op base':'ws_flat_unit','op param':'0','op stat1':'lightmindam','*eol':'0'})
    entry.update({k:target[k] for k in ('ValShift','descpriority','descfunc','descval','descstrpos','descstrneg')})
    assert len(stat_rows)==509
    stat_rows.append(entry);write_table(directory/'itemstatcost.txt',header,stat_rows)
    header,prop_rows=read_table(directory/'properties.txt')
    for code,stat in (('ws-light-min-wide','ws_flat_light_min'),('ws-light-min-native','lightmindam'),('ws-light-max-native','lightmaxdam')):
        entry={k:'' for k in header};entry.update(code=code,func1='1',stat1=stat,**{'*eol':'0'});prop_rows.append(entry)
    write_table(directory/'properties.txt',header,prop_rows)
    # Poison Infusion's equal min/max damage rate can exceed both native
    # ten-bit fields. One appended op-2 stat contributes to both rates.
    header,stat_rows=read_table(directory/'itemstatcost.txt')
    target=next(r for r in stat_rows if r['Stat']=='poisonmindam')
    entry={k:'' for k in header}
    entry.update(Stat='ws_flat_poison_rate',**{'Send Bits':'32','Save Bits':'16','op':'2','op base':'ws_flat_unit','op param':'0','op stat1':'poisonmindam','op stat2':'poisonmaxdam','*eol':'0'})
    entry.update({k:target[k] for k in ('ValShift','descpriority','descfunc','descval','descstrpos','descstrneg')})
    assert len(stat_rows)==510
    stat_rows.append(entry);write_table(directory/'itemstatcost.txt',header,stat_rows)
    header,prop_rows=read_table(directory/'properties.txt')
    for code,stat in (('ws-poison-rate-wide','ws_flat_poison_rate'),('ws-poison-rate-native','poisonmindam'),('ws-poison-duration-native','poisonlength')):
        entry={k:'' for k in header};entry.update(code=code,func1='1',stat1=stat,**{'*eol':'0'})
        if code=='ws-poison-rate-native':entry.update(func2='1',stat2='poisonmaxdam')
        prop_rows.append(entry)
    write_table(directory/'properties.txt',header,prop_rows)
    normalize(directory)
from barbarian_data import install as install_barbarian
install_barbarian(root)
from sorceress_data import install as install_sorceress
install_sorceress(root)
from assassin_data import install as install_assassin
install_assassin(root)
from amazon_data import install as install_amazon
install_amazon(root)
from monster_modifiers.data import install as install_monster_spells
install_monster_spells(root)
from monster_modifiers.effects_data import install as install_monster_effects
install_monster_effects(root)
from druid_data import install as install_druid
install_druid(root)
install_buff_hud(root)
from internal_skill_desc import install as install_internal_skill_desc
install_internal_skill_desc(root)
# Compile tooltips/property bindings after the final linker IDs are assigned.
export(root,nodes)

from combat.barbarian_icons import install as install_barbarian_icons
install_barbarian_icons(root)
from combat.sorceress_icons import install as install_sorceress_icons
install_sorceress_icons(root)
from character_sheet import install as install_character_sheet
install_character_sheet(root)

from combat.assassin_icons import install as install_assassin_icons
install_assassin_icons(root)

from combat.amazon_icons import install as install_amazon_icons
install_amazon_icons(root)

from combat.druid_icons import install as install_druid_icons
install_druid_icons(root)

from weapon_ailments import install as install_weapon_ailments
install_weapon_ailments(root)
from paladin_affixes import install as install_paladin_affixes
install_paladin_affixes(root)
export_affix_scaling(root)

from endgame.data import install as install_arena
install_arena(root)

from endgame.seal_data import install as install_trial_crafting
install_trial_crafting(root)
install_integrations(root) # refresh material pickup table hashes after adding Trial Orbs

from proc_safety import install as install_proc_safety
install_proc_safety(root)

from equipment_buffs import install as install_equipment_buffs
install_equipment_buffs(root)
export_affix_scaling(root)

from universal_class_items import install as install_universal_class_items
# install_universal_class_items(root) # Pending complete native staffmod conversion.

from stash_navigation import install as install_stash_navigation
install_stash_navigation(root)

from nightmare_skip.data import install as install_nightmare_skip
install_nightmare_skip(root)
install_integrations(root, pickup_only=True) # refresh pickup hashes after the reward item

from goblins.data import install as install_goblins
install_goblins(root)

from item_limits import install as install_item_limits
install_item_limits(root)

from mythicals.data import install as install_mythicals
install_mythicals(root)

from progression.data import install as install_progression
install_progression(root)

from leagues.data import install as install_leagues
install_leagues(root)

# Saved mercenary charm page and combined equipment panel.
from integration.mercenary import install as install_mercenary_charms
install_mercenary_charms(root)

from mercenary_skills import install as install_mercenary_skills
install_mercenary_skills(root)

from presentation_qol import install as install_presentation_qol
install_presentation_qol(root)

from combat.amazon_masteries_data import install as install_amazon_masteries
install_amazon_masteries(root)
from combat.amazon_melee_data import install as install_amazon_melee
install_amazon_melee(root)
install_buff_hud(root)
