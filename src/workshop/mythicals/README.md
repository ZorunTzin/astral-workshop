# Mythical Uniques — 0.21.0

53 active variants: 43 ESR adaptations, three Mageblood belts and seven selected Screams belts. Retired Screams rows retain their IDs for saved items but no longer drop. Sources: https://easternsunresurrected.com/unique_mythicals.htm and https://lukaszpg.github.io/TheArchivistSoE/ . Source snapshots and per-item adaptations are stored alongside this file.

## Drops and saves

Trial-only in production: one 1:5000 base roll per native Trial loot roll (Players 1). Additional Trial rolls and native NoDrop/Players scaling affect the effective chance per kill. Active variants are equally weighted. Level requirement 90. Test mode in test-drops.json replaces Corpsefire's loot with six guaranteed Mythicals; disabled in published builds. Existing v2 profile retained. New tooltip-only stats use previously unused, unsaved slots 259–261; old saved stat widths are unchanged.

Old items keep their rolled properties. Obtain new drops to test the revised skill levels and damage. Base-type, skill implementation, glow and tooltip changes also apply to existing definitions. Move previously misclassified belts out of the boots slot and re-equip as belts.

## Current corrections

- All Mythicals have the jewel-style sky beam and ground effects. Hard cleanup on pickup/destruction. Ordinary item assets are unchanged.
- Headhunter, Mageblood and Screams use actual belt types (Spiderweb Sash), not Wyrmhide Boots.
- Normal item skills/procs grant level 60. Natalya: level 25, 5% on kill. Mythical-only powers keep individual levels.
- Mephisto's Nightmare: 15% level-60 Hydra on striking, enemy fire resistance reduction in place of lightning.
- Voidforge: 1000–2000 each fire, cold and lightning damage. Removed the random physical-to-element power and Vengeance oskill.
- Ties That Bind Poison: native poison-rate payload 2048 instead of 20480 (one tenth), including its pair bonus. Other rings unchanged. Flat physical defense text now says Physical Damage Taken Reduced by.
- Native item-proc handler IDs corrected for referenced skills. Imperius additionally grants level-60 Thunder Storm so the native periodic skill has its required skill instance.
- Blood Empowerment: 12 seconds, +150% physical damage, +30 attack speed, +20 life steal and +100% Workshop bleeding chance.
- Celestial Empowerment: 12 seconds, +100% physical damage and +20% maximum life.
- Underworld Empowerment: 12 seconds, +100% poison/magic skill damage and +100% physical damage. All three have Buff HUD timer entries. Orpheus has 50% on-kill activation. Time Ward lasts 5 seconds, grants 30 physical/magic resistance.
- Starfall: private Blizzard-style falling-star chain, magic damage, 4-second rain, radius 8, 1200–1800 base damage plus 120–180 per level; level 30 gives 4680–7020 per impact before bonuses/resistance. Native skill and proc handlers enabled; selectable oskill with damage description. No fire ground damage or cold duration. Uses existing comet/holy-light visuals.
- Tal Rasha's missing Cloak proc is restored as Shadow Veil: 12-second nearby blind, -50% defense, -25 fire/cold/lightning/magic resistance, -15 physical resistance. Custom signature, level 1.
- Skeleton Warrior/Mage minion life bonus now enters native calc1, including the existing tooltip. Removed the duplicate percentage passive. Re-summon minions after changing equipment to update their summon-time stats.
- Renewal tooltips explicitly display percentage life/mana regeneration; native temporary regeneration state remains non-stacking and expires on unequip/death.

## Selected Screams

Might–Renewal, Elements–Resistance, Elements–Renewal, Arcane–Resistance, Arcane–Renewal, Precision–Renewal, Swiftness–Renewal. Other historical variants remain readable but are absent from the active pool.

## Remaining adaptations

On-cast source procs use on-kill. Equipped source auras become activatable Workshop auras. SoE AoE becomes maximum mana, skill frequency becomes cast rate, buff effect becomes aura effect, Deadly Strike multiplier/cap become physical damage/attack rating. Separate SoE curse protections become reduced curse duration. Mageblood Renewal gives native 10% maximum life and 15% maximum mana recovery per second; Screams Renewal gives 5% life/s and +400% mana regeneration. Additional native regeneration bonuses can affect effective rates. The SoE belts reuse existing belt art. Complex ESR conditional systems (corruption counts, stat-threshold summons, stacking underworld powers, Soul Engine 1 HP) remain documented adaptations, not full engine ports.

## Verification

verify.py checks property/save limits, all proc handler IDs, exact exception values, 53-item active drop pool, seven Screams, belt ancestry, native summon formulas, Starfall routing, three cleanup-enabled ground effects per model and repeatable generation. Native tests cover damage, resistance, ring pairing, regeneration scaling, renewal, unequip and death. Actual in-game proc animation, Buff HUD rendering and save/load still require gameplay verification.
