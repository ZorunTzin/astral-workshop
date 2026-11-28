# Trial seal crafting — 0.20.0

Seven Trial Orbs, no Severance. Akara sells each for 1 gold during testing. Native direct Materials slots register each code for first-pickup deposit. Gem Bag and Grabbers removed from vendors; owned items unchanged.

Normal/Magic/Rare crafting grade lives in the seal tooltip independently of equipment quality. Non-equippable Wa1..Wa5 use existing 12-bit Workshop sector fields 444..451; the Sigil reader checks its own code. No stat IDs or save widths change. Seals are noncompact so their stats save. Native op18 recipes gate exact grade/count/corruption state. The property dispatcher applies differences through native additive saved properties.

The byte-guarded CreatePortalObject hook captures a seal from the owner's Cube before native consumption, only commits on successful portal creation, and binds game/owner/tier. First arena entry consumes that snapshot once. Runtime fixtures exercise failed portals, wrong container, exact modifiers and repeated entry.

Crafting pool: life, damage, density, movement/attack/cast speed, attack rating, defense, elemental/physical resistance, hit recovery, Last Stand, Champion/Elite population. Ordinary values match wave cards. Population tiers 5/10/15/20/30 and 2/4/6/8/12 percent; no trial-tier scaling. Existing rank precedence remains. Corruption equally rolls an extra Legendary modifier, +160% loot, or +1 tier to all modifiers (capped). Empty upgrade result adds Legendary instead.

Drop pool each4, NoDrop2730: aggregate28/2758 equals Herald Orbs4/394. Independent negative-pick wrappers preserve original Herald and Trial material rolls. Baseline per-orb4/2758, repeated by extra Trial loot rolls; native limits apply.

Art generated with ImageGen: src/workshop/endgame/orb-art. HD98/lowend49, website256 WebP. Classic retains original orb art. Tests: 10000 crafting sequences, same save-field reconstruction, native portal fixture, 140 guarded recipes, stash-slot overlap and loot dependencies. Gameplay validation pending.

Property behavior reference: https://github.com/ThePhrozenKeep/D2MOO/blob/master/source/D2Common/src/Items/ItemMods.cpp . Exact native ABI: locally governed build93847 reference and memory snapshot.

## Login crash hotfix
Crash 2026-11-18 10:50:54 UTC: native slot resolver RVA 0x2CE916, null RCX, called from bank initialization 0x23B4D9. Removed TextBoxWidget and ImageWidget direct children from advancedstash_materials in mouse/controller layouts. Only AdvancedStashSlotWidget children are valid here. seal_verify.py now rejects any non-slot child. Installed-file hashes verified; character login awaits user confirmation.

## 0.20.1: seal tooltip and legacy compact flags
Craft logs verified server-side stat application, but older seals can retain ItemData compact-save bit 21. Native serializer RVA 0x37FFD0 sets but never clears it; this bypasses extended stats in packets/saves. Trial craft dispatch now clears only bit 21 on a valid Wa1-Wa5 cube output before applying properties. Tooltip uses plain text and short lines; empty seals explicitly show no starting modifiers. Native dispatch fixture checks migration, preserved flags, Awakening/Ascension/Purification and exact stats; 10000 craft sequences also check tooltip values. In-game confirmation remains pending.

## 0.20.2
Every selected card and each starting seal modifier add 10 percentage points to MonsterLife and 2 to MonsterDamage. Additive, including positive cards and one-wave events (the baseline burden persists); rejected/stale choices and rerolls/banishes do not add burden. Active modifier totals include it. Ctrl on seals shows White-to-Legendary range at the seal tier (discrete tiers). Category labels now occupy independent widgets above the top divider. Gem Cluster new acquisition disabled; old definitions and consumption remain compatible. Runtime, seal, loot-removal and panel geometry checks pass.

## 0.20.2 visual update
Trial slots: x620/724/828, y266/370/474; controller y minus212. BackgroundFile[3] points at dedicated Materials artwork with exact existing 98x98 sockets; no non-slot children added. Seven ground model definitions include the Jewel VFX with unique entity IDs and explicit particle dependencies. Names use UTF-8 purple marker U+00FF c;. Slot overlap, native child types, dependencies, names and generated asset preview verified.

### Material tab input (2026-11-27)

Inactive League slots are disabled as well as hidden. Their positions overlap the right side of Trial orbs/seals and several key/statue/shard slots; invisible interactive slots previously intercepted those clicks. Active slots restore both properties through guarded native setters. Decorative backgrounds remain noninteractive. Ground-picked Uber keys, Ancient statues, Worldstone shards and Uber organs now use the existing automatic native deposit route; manual withdrawals are excluded.
