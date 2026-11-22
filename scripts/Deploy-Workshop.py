from pathlib import Path
from shutil import copy2
from hashlib import sha256
from datetime import datetime
import os
repo=Path(__file__).resolve().parent.parent
src=repo/"build/workshop-clusters"
mod=Path(r"B:\D2R\Diablo II Resurrected\Mods\ReimaginedWorkshop")
game=mod/"ReimaginedWorkshop.mpq"
assets=["data/hd/global/ui/Workshop/"+s+".sprite" for s in ("unified","major","minor","finish","halo")]
assets+=["data/global/excel/"+p+s+".txt" for p in ("","base/") for s in ("properties","itemstatcost","misc","cubemain","uniqueitems","runes","setitems","magicsuffix")]
assets+=["data/local/lng/strings/item-names.json","data/local/lng/strings/monsters.json"]
assets+=["data/hd/items/items.json","data/hd/items/uniques.json"]
plugin="d2rloader/plugins/d2rl-astral-workshop.dll"
plugins=[plugin,"d2rloader/plugins/d2rl-workshop-combat.dll"]
assets += ["data/global/excel/"+p+s+".txt" for p in ("","base/") for s in ("skills","skilldesc","states","missiles","monstats","monequip","monstats2","charstats","pettype")]
import json
plugins += ["d2rloader/plugins/"+n for n in ("d2rl-celestialrayone-whirlwind-follow-cursor.dll","fmmp-auto-collectibles.dll","d2rl-ruffneckk-mapsense.dll","RuffnecKkMapSenseMapgen.exe","d2rl-buffhud.dll","d2rl-buffhud.mpq","fmmp-monster-display.dll")]
assets += ["data/global/ui/layouts/"+p for p in ("pauselayouthd.json","pauselayoutgardenhd.json","controller/pauselayoutgardenhd.json")]
modfiles=["d2rloader/config/d2rloader.toml","d2rloader/config/celestialrayone.loot-filter.toml","bank.txt","d2rloader/config/celestialrayone.whirlwind.toml","d2rloader/config/ruffneckk-mapsense.toml","d2rloader/THIRD-PARTY-NOTICES-workshop.txt"]
modfiles += [str(p.relative_to(src)).replace("\\","/") for p in (src/"d2rloader/licenses").glob("*") if p.is_file()]
assets += ["data/global/excel/"+p+"monprop.txt" for p in ("","base/")]
assets += ["data/local/lng/strings/ui.json","data/local/lng/strings/item-runes.json"]
assets += json.loads((src/"workshop/combat/assets.json").read_text(encoding="utf8"))
assets += json.loads((src/'workshop/endgame/assets.json').read_text(encoding='utf8'))

docs=["workshop/"+p for p in ("atlas_assets.py","tree_layout.py","node_text.py","summon_limits.py","buff_hud.py","tree_update_verify.py","NODES.csv","table_ids.py","table_audit.inc","paladin_update.py","BARBARIAN.md","MONSTER-MODIFIERS.md","barbarian_data.py","barbarian_verify.py","BARBARIAN.json","internal_skill_desc.py","paladin_rebalance.py","paladin_rebalance_verify.py","PALADIN-REBALANCE.json","set_reforge.py","set_catalog.inc","set_reforge_test.cpp","balance093_verify.py","CLUSTER-OVERRIDES.json","INNER-CLUSTERS.json","NODES-step2.csv","atlas_test.cpp","generate.py","passives.cpp","stars.inc","unified.inc","visual.inc","tree-preview.png","README.md","CONSTELLATIONS.md")]
docs += ["workshop/"+p for p in ("weapon_ailments.py","weapon_ailments_verify.py","WEAPON-AILMENTS.json","sorceress_data.py","sorceress_verify.py","SORCERESS.json","assassin_data.py","assassin_verify.py","ASSASSIN.json","amazon_data.py","amazon_verify.py","AMAZON.json","druid_data.py","druid_verify.py","DRUID.json","DRUID.md","assassin_preview/engine.h","assassin_preview/engine_test.cpp")]

docs += ["workshop/monster_modifiers/"+p for p in ("caption_text.h","caption.h","caption.asm","caption_test.cpp","caption_test.asm","engine.h","engine_test.cpp","balance.json","native_base.h","native_test.cpp","spells.h","bindings.h","data.py","data.json","effects_data.py","effects_bindings.h","effects_data.json","runtime.h","runtime_native.inc","runtime_test.cpp","verify.py")]

docs += ["workshop/"+p for p in ("forging.py","forge_native.inc","forge_tooltip.inc","forge.inc","FORGING.json","FORGING-audit.json","FORGING-seals.json","FORGING.md","tuning.py","tuning_verify.py","paladin_balance.py","paladin_verify.py","verify.py","item_visuals.py","sigil_visuals.py","affix_scaling.py","affix_scaling.inc","affix_rules.inc","AFFIX-SCALING.json")]
docs += [str(p.relative_to(src)).replace("\\","/") for p in (src/"workshop/art").rglob("*") if p.is_file()]

docs += [str(p.relative_to(src)).replace('\\','/') for p in (src/'workshop/endgame').glob('*') if p.is_file() and p.suffix in ('.py','.h','.cpp','.json','.md')]

combat=src/"workshop/combat"
for folder in (combat,combat/"src",combat/"original"):
    docs += [str(p.relative_to(src)).replace("\\","/") for p in folder.iterdir() if p.suffix in (".h",".cpp",".py",".cmd",".json",".md",".rc")]
for table in ("skills","skilldesc","states","itemstatcost","missiles","monseq"):
    docs.append("workshop/combat/upstream/data/global/excel/"+table+".txt")
docs += [str(p.relative_to(src)).replace("\\","/") for p in (combat/"upstream/data/hd").rglob("*") if p.is_file()]
docs.append("workshop/combat/upstream/data/local/lng/strings/skills.json")
docs += ["workshop/"+p for p in ("proc_safety.py","forge_chronicle_ids.inc","forge_chronicle_engine.h","forge_chronicle_native.inc","forge_chronicle_data.py","forge_chronicle_test.cpp")]
docs += [str(p.relative_to(src)).replace("\\","/") for p in (combat/"icons").rglob("*") if p.is_file()]

docs += [str(p.relative_to(src)).replace("\\","/") for p in (src/"workshop/integration").rglob("*") if p.is_file() and "__pycache__" not in p.parts]

docs += ['workshop/paladin_affixes.py','workshop/paladin_affixes_verify.py','workshop/PALADIN-AFFIXES.json']

docs += ['workshop/equipment_buffs.py','workshop/equipment_buffs_verify.py','workshop/EQUIPMENT-BUFFS.json']

assets += ['data/global/excel/itemtypes.txt','data/global/excel/base/itemtypes.txt']
docs += ['workshop/universal_class_items.py','workshop/universal_class_items.h','workshop/universal_class_items.inc','workshop/UNIVERSAL-CLASS-ITEMS.json']

docs += ['workshop/'+p for p in ('stash_navigation.inc','stash_navigation.h','stash_navigation.py','stash-navigation-MIT.txt','stash_organizer.inc','stash_organizer_model.h','stash_organizer_catalog.h','stash_organizer_catalog.py','stash_organizer_test.cpp','stash_organizer_ui.py','STASH-ORGANIZER.md')]

assets += ['data/hd/global/ui/Workshop/'+name+'.sprite' for name in ('stashNext','stashPrevious','stashNextTen','stashPreviousTen','stashOrganizer','stashPageHit','organizerPanel','organizer130','organizer350','organizer500','organizer590','stashMatch','organizer80','organizer230','organizer1120','organizerColor0','organizerColor1','organizerColor2','organizerColor3','organizerColor4','organizerColor5')]

docs += ['workshop/nightmare_skip.inc']
docs += [str(p.relative_to(src)).replace('\\','/') for p in (src/'workshop/nightmare_skip').glob('*') if p.is_file() and p.suffix in ('.h','.cpp','.py','.md','.png')]

assets += ['data/hd/global/ui/items/misc/uniquecharm/workshop_nightmare_legacy.sprite','data/hd/global/ui/items/misc/uniquecharm/workshop_nightmare_legacy.lowend.sprite','data/hd/items/misc/uniquecharm/workshop_nightmare_legacy.json']

docs += ['workshop/'+p for p in ('item_sort.inc','item_sort_model.h','item_sort_test.cpp','item_sort_runtime_test.cpp','item_sort_ui.py')]
assets += ['data/global/ui/layouts/'+p for p in ('playerinventoryexpansionlayouthd.json','playerinventoryoriginallayouthd.json','horadriccubelayouthd.json')]

assets += ['data/hd/global/ui/Workshop/stashSort.sprite']

assets += json.loads((src/'workshop/goblins/assets.json').read_text(encoding='utf8'))
docs += [str(p.relative_to(src)).replace('\\','/') for p in (src/'workshop/goblins').rglob('*') if p.is_file() and '__pycache__' not in p.parts]

assets += ['modinfo.json']
docs += ['workshop/item_limits.py','workshop/item_limits_verify.py','workshop/item-format.json','workshop/ITEM-FORMAT.md']

assets += json.loads((src/'workshop/mythicals/assets.json').read_text(encoding='utf8'))
docs += [str(p.relative_to(src)).replace('\\','/') for p in (src/'workshop/mythicals').rglob('*') if p.is_file() and '__pycache__' not in p.parts]

assets += json.loads((src/'workshop/progression/assets.json').read_text())
docs += [str(p.relative_to(src)).replace('\\','/') for p in (src/'workshop/progression').glob('*') if p.is_file()]

assets += json.loads((src/'workshop/leagues/assets.json').read_text())
docs += [str(p.relative_to(src)).replace(chr(92),'/') for p in (src/'workshop/leagues').rglob('*') if p.is_file() and '__pycache__' not in p.parts]

assets += json.loads((src/'workshop/integration/mercenary-assets.json').read_text())
plugins += ['d2rloader/plugins/fmmp-mercenary-charms.dll']
modfiles += ['d2rloader/config/fmmp-mercenary-charms.toml']

assets += ['data/global/excel/hireling.txt','data/global/excel/base/hireling.txt']
docs += ['workshop/mercenary_skills.py','workshop/mercenary_skills_verify.py']

def sha(p):return sha256(p.read_bytes()).hexdigest() if p.exists() else None
backup=repo/"archive/local/backups/workshop"/datetime.now().strftime("%Y%m%d-%H%M%S")
jobs=[]
for rel in assets+plugins+modfiles:
    target=(mod if rel in plugins+modfiles else game)/rel
    if rel=="modinfo.json":
        import json
        existing=json.loads(target.read_text()) if target.exists() else {}
        expected=json.loads((src/rel).read_text())
        if existing.get("name")!=expected.get("name") or existing.get("savepath") not in ("ReimaginedWorkshop/",expected["savepath"]):raise RuntimeError("Unexpected installed save profile")
    elif sha(repo/rel)!=sha(target) and sha(target)!=sha(src/rel):
        raise RuntimeError("Repo/game divergence: "+rel)
    jobs.append((target,rel,"game"))
def repository_destination(rel):
    if rel in {"workshop/BARBARIAN.md","workshop/README.md","workshop/CONSTELLATIONS.md","workshop/FORGING.md","workshop/combat/README.md","workshop/integration/README.md"}:
        return repo/"docs"/rel
    if rel.startswith("workshop/"):
        return repo/"src"/rel
    return repo/rel
jobs += [(repository_destination(rel),rel,"repo") for rel in assets+docs+plugins+modfiles]
import sys
if "--check" in sys.argv:
    missing=[rel for _,rel,_ in jobs if not (src/rel).is_file()]
    if missing:raise RuntimeError("Missing staging inputs: "+str(missing))
    print("Deploy plan checked:",len(jobs),"targets; no writes to repository or game.")
    raise SystemExit(0)
written=[]
try:
    for dest,rel,tag in jobs:
        source=src/rel
        if sha(source)==sha(dest):continue
        saved=backup/tag/rel
        if dest.exists():
            saved.parent.mkdir(parents=True,exist_ok=True);copy2(dest,saved)
        dest.parent.mkdir(parents=True,exist_ok=True)
        tmp=dest.with_name(dest.name+".workshop-new")
        copy2(source,tmp);os.replace(tmp,dest);written.append((dest,saved))
        assert sha(source)==sha(dest)
except Exception:
    for dest,saved in reversed(written):
        if saved.exists():copy2(saved,dest)
        else:dest.unlink()
    raise
print("Installed and verified",len(written),"files in repo and game.")
