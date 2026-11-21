"""Three paths per sector. Keystones sacrifice the bonuses of their own path."""
# Each path contains three incremental bonuses followed by one exclusive rule.
# A rule is (name, short description, property, parameter, minimum, maximum).
SECTORS = {
    "Origins": [
        (("Ember","hp",10),("Bloodstone","hp",25),("Fleetfoot","move2",5),("Blood Tithe","Life for Life Tap","hit-skill","Life Tap",5,7)),
        (("Aether","mana",10),("Deep Well","mana",25),("Quickening","move2",5),("Inner Choir","Mana for Meditation","aura","Meditation",3,3)),
        (("Ward","res-all",3),("Prism","res-all",7),("Wayfarer","move2",5),("Rift Walker","Resists for Teleport","oskill","Teleport",1,1))],
    "Arms": [
        (("Steel","dmg%",5),("Warhammer","dmg%",15),("Momentum","swing2",5),("Sunder","Damage for crushing blow","crush",0,16,16)),
        (("Focus","att",25),("Sightline","att",75),("Finesse","dex",6),("True Aim","Accuracy for ignore defense","ignore-ac",0,1,1)),
        (("Vigor","str",3),("Titan","str",8),("Long Reach","dmg-max",5),("Battle Cry","Strength for Shout","oskill","Shout",4,4))],
    "Bulwark": [
        (("Iron","ac",25),("Ironwall","ac",75),("Poise","balance2",8),("Rimeheart","Defense for freeze immunity","nofreeze",0,1,1)),
        (("Guard","block",2),("Bastion","block",5),("Parry","block2",8),("Thorn Crown","Block for Thorns aura","aura","Thorns",5,5)),
        (("Grit","red-dmg",1),("Adamant","red-dmg",3),("Fortitude","vit",6),("Faithguard","Reduction for Holy Shield","oskill","Holy Shield",3,3))],
    "Elements": [
        (("Spark","extra-ltng",2),("Tempest","extra-ltng",5),("Conductor","pierce-ltng",3),("Stormheart","Lightning bonus for Static","oskill","Static Field",3,3)),
        (("Cinder","extra-fire",2),("Inferno","extra-fire",5),("Scorch","pierce-fire",3),("Firebrand","Fire bonus for Enchant","oskill","Enchant",6,6)),
        (("Rime","extra-cold",2),("Glacier","extra-cold",5),("Fracture","pierce-cold",3),("Winter Tide","Cold bonus for Holy Freeze","aura","Holy Freeze",4,4))],
    "Hunt": [
        (("Aim","att%",4),("Eagle Eye","att%",10),("Draw","swing2",6),("Throughshot","Aim for piercing attacks","pierce",0,30,30)),
        (("Stride","move2",3),("Pursuit","move2",7),("Wind","stam",25),("Windcaller","Speed for Vigor aura","aura","Vigor",4,4)),
        (("Edge","dmg-min",1),("Fang","dmg-min",3),("Barbs","openwounds",6),("Deadeye","Min damage for deadly strike","deadly",0,15,15))],
    "Occult": [
        (("Venom","extra-pois",2),("Virulence","extra-pois",5),("Toxin","pierce-pois",3),("Blight","Poison bonus for Lower Resist","oskill","Lower Resist",2,2)),
        (("Veil","res-curse-len",5),("Shroud","res-curse-len",12),("Night Sight","light",2),("Hexblade","Curse ward for Amplify proc","hit-skill","Amplify Damage",3,8)),
        (("Bone","extra-mag",2),("Ossuary","extra-mag",5),("Grave Lore","enr",6),("Boneweaver","Magic bonus for Bone Spear bolt","extra_bonespears",0,1,1))],
    "Legion": [
        (("Sentry","ac-miss",20),("Watchtower","ac-miss",55),("Warden","res-ltng",8),("Machinist","Missile defense for +1 trap","extra_traps",0,1,1)),
        (("Pack","stam",12),("Great Hunt","stam",30),("Trail","regen-stam",15),("Ravenlord","Stamina for +2 ravens","extra_ravens",0,2,2)),
        (("Crypt","att-undead",25),("Tomb","att-undead",75),("Exorcist","dmg-undead",12),("Legionnaire","Undead aim for +2 warriors","extra_skelewarriors",0,2,2))],
    "Arcana": [
        (("Flow","regen-mana",5),("Wellspring","regen-mana",12),("Invocation","cast2",5),("Soul Font","Regen for mana after kills","mana-kill",0,8,8)),
        (("Spirit","enr",3),("Deep Mind","enr",8),("Clarity","mana",15),("Oracle","Energy for Oak Sage","oskill","Oak Sage",3,3)),
        (("Runeglass","red-mag",1),("Spellward","red-mag",3),("Anchor","res-mag",3),("Shadow Skin","Magic reduction for Fade","oskill","Fade",3,3))],
    "Harvest": [
        (("Spoils","gold%",12),("Treasure","gold%",30),("Fortune","mag%",8),("Alchemist","Gold find for magic find","mag%",0,25,25)),
        (("Reaping","heal-kill",1),("Harvest","heal-kill",3),("Renewal","regen",4),("Blood Drinker","Life on kill for life steal","lifesteal",0,6,6)),
        (("Hunger","mana-kill",1),("Feast","mana-kill",3),("Siphon","mana",15),("Mind Drinker","Mana on kill for mana steal","manasteal",0,5,5))]
}
LABELS = dict(zip(
    "hp move2 mana res-all dmg% swing2 att dex str dmg-max ac balance2 block block2 red-dmg vit extra-ltng pierce-ltng extra-fire pierce-fire extra-cold pierce-cold att% stam dmg-min openwounds extra-pois pierce-pois res-curse-len light extra-mag enr ac-miss res-ltng regen-stam att-undead dmg-undead regen-mana cast2 red-mag res-mag gold% mag% heal-kill regen mana-kill".split(),
    "Life|Run/walk %|Mana|All resists|Weapon damage %|Attack speed %|Attack rating|Dexterity|Strength|Max damage|Defense|Hit recovery %|Block %|Block rate %|Physical reduction|Vitality|Lightning damage %|Lightning pierce %|Fire damage %|Fire pierce %|Cold damage %|Cold pierce %|Attack rating %|Stamina|Min damage|Open wounds %|Poison damage %|Poison pierce %|Curse duration reduction %|Light radius|Magic damage %|Energy|Missile defense|Lightning resist|Stamina recovery %|Aim vs undead|Damage vs undead %|Mana regen %|Cast rate %|Magic reduction|Magic resist|Gold find %|Magic find %|Life after kill|Replenish life|Mana after kill".split("|"), strict=True))
