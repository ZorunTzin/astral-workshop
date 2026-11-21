# Reimagined Workshop

An offline modding workshop built on Reimagined 3.1.0, pinned to upstream commit `4fa2cff22ff94685d5ffaa6a4c2cb7e8d7d3b88f`. This mod uses its own save folder, ReimaginedWorkshop.

## Astral Passives

Open **F9** for the 447-node tree. Hover a star for its name and effect. Each of the 84 regular entry nodes grants +3 to all attributes. Follow its four-node side branch to earn its specialization and final bonus. The attribute entries form straight main paths ending in Keystones. Multiple keystones can be allocated together. Keystones add their effects without removing other learned node bonuses.

1. Choose your stars and click **Apply**. The finished Astral Sigil appears directly in your inventory.
2. Applying again replaces your carried sigil in one transaction. The required character level is the number of allocated points.
3. Each character level grants one Astral point. The counter shows remaining points. **Reset** clears the draft. The carried sigil is read automatically when entering the tree with a character. Keep the sigil in active charm inventory to receive its bonuses.

Edit [INNER-CLUSTERS.json](../../src/workshop/INNER-CLUSTERS.json) for the 27 inner specializations, including parameterized skills and procs. [NODES-step2.csv](../../src/workshop/NODES-step2.csv) contains their readable labels and the balance values for the remaining clusters. [NODES.csv](../../src/workshop/NODES.csv) defines the underlying properties and keystones. It is UTF-8 with a BOM and semicolon delimiters. Keep the region, path and slot identifiers stable. For an ordinary flat or percentage bonus, edit minimum and maximum to the same value. For skill effects the parameter identifies the skill, and for on-strike effects minimum is the chance in percent and maximum is the skill level. The generator reads the sheet and builds both gameplay definitions and tooltips. A rebuild and reinstall apply the edits.

## Rune forging

All 220 active Reimagined runeword variants can be forged. Transmute a runeword's **runes together with an eligible weapon or armor piece** to forge its bonuses directly. No Identify scroll or seal preparation is required. No empty sockets are needed. The input item, its original modifiers, rarity and socket contents are preserved. Each item can be forged once, and existing native runewords are excluded.

| Item quality | Forged bonuses |
| --- | --- |
| Normal / Superior (white / gray) | Every original runeword modifier and its intrinsic rune bonuses |
| Magic | Full-strength core plus up to four selected positive modifiers |
| Rare / Crafted | Full-strength core plus up to two selected positive modifiers |
| Set / Unique | Full-strength core only |

The number of included modifiers changes with rarity. Selected values, random ranges, aura levels and proc chances keep their original strength. The highest rune requirement sets a minimum required level. Existing item requirements and native skill requirements still apply.

The older seal recipes remain available as an optional alternative. Some words share the same rune ingredients. Add one more Identify scroll to the resulting seal to cycle through that ingredient group's alternatives. The seal's name and base hint identify the current choice. Direct recipes select the word by the target base. Narrower bases take precedence over broad weapon types. White takes precedence over Shadow of Doubt because both use Dol + Io on wands; use the optional seal path to select Shadow of Doubt. See [FORGING.md](FORGING.md) for all recipes and selections, and `FORGING.json` for the authoritative, editable definitions.

The plugin expands a generated dispatch property synchronously inside the native cube property calculation, bypassing the cube's five-slot limit while retaining its own property handling and rolls. This feature requires the included plugin and the verified D2R build 93847. Restart D2R after installing version 0.6.1.

## Affix strength (version 0.6.2)

Newly rolled Magic prefixes and suffixes use **2.00x** their numeric strength. Newly rolled Rare prefixes and suffixes use **1.35x**. Skill bonuses are rounded down, as are other integer fields. The number of affixes and their eligibility, IDs and level requirements are unchanged. Set, Unique and Crafted qualities keep their ordinary affix generation.

Proc chance and triggered skill level both scale, with chance capped at 100%. Fixed flags such as Cannot Be Frozen, Indestructible, Ethereal, Ignore Target Defense, Prevent Monster Heal and Knockback stay unchanged. Numeric charge-preservation chances still scale. Per-level rates, random single-skill bonuses, socket counts and charged skills use their correct native fields; skill IDs and monster IDs stay unchanged. Automatically calculated charged-skill values are resolved before scaling. Native caps still apply.

The byte-guarded native dispatcher limits this adjustment to actual MagicPrefix/MagicSuffix records. AutoMagic base properties, rune/gem socket bonuses, Astral properties and forged runeword properties are outside that scope. Tables are never mutated while an item rolls. Already saved item values remain as saved, and loading/equipping an item does not repeatedly increase its strength. New drops, purchases and rerolls receive the updated values when they generate fresh affixes. `affix_scaling.py` generates the property-specific rules and the auditable `AFFIX-SCALING.json` file from the actual Reimagined tables.

## Aura buffs and curses (version 0.8.4)

Paladin auras are independent toggle buffs. Cast an aura once to enable it and again to disable it. Switching the selected skill does not cancel a buff. Several can run together while there is enough maximum mana. Reservation follows the aura's required character level and uses its effective skill level, including equipment bonuses. The tooltip shows the resulting mana amount.

| Aura required level | Mana reserved |
| --- | --- |
| 1 | 10 + 2 per skill level |
| 6 | 20 + 4 per skill level |
| 12 | 30 + 6 per skill level |
| 18 | 40 + 8 per skill level |
| 24 | 50 + 10 per skill level |
| 30 | 60 + 12 per skill level |

 Reservation lowers usable maximum mana and is released when disabled. Equipment-granted auras remain free. If equipment or skill levels change, reservation is recalculated; the most recently enabled buffs switch off first when the budget is exceeded. Death clears toggles. Enable them again in a new game.

### Paladin replacements

| Original skill | New skill | Initial balance |
| --- | --- | --- |
| Resist Fire | Reservation Efficiency | Passive: +4% reservation efficiency per effective skill level. Cost = normal reservation / (1 + efficiency / 100), rounded down to at least one mana. |
| Resist Cold | Aura Mastery | Passive: +3% aura effect per effective skill level. Affects aura strength, not reservation, range, duration, either passive or the guardian. |
| Resist Lightning | Astral Guardian | Summons one Paladin guardian with generated sword, shield, helmet and armor. Each hard point in any of the 17 aura buffs adds +10% life and +8% damage. Recast after changing points. Summoning costs 25 mana plus 1 per additional skill level; no reservation. |
| Thorns | Retribution | Toggle: stores (100 + 10 per skill level)% of actual incoming hit damage. Releases a physical Nova three seconds after charging begins. Capacity is four times maximum life, capped at 100,000 damage. Death or switching off clears the charge. |
| Cleansing | Sanctified Ground | Toggle: stand still for one second to create ground with an 8-yard radius. Allies receive +20% +5% per skill level damage and regenerate 3% +0.2% per skill level maximum life per second. The buff lasts eight seconds and is refreshed on the ground. |

The Astral Guardian also casts **Holy Bolt**, **Blessed Hammer** and **Fist of the Heavens** autonomously. They unlock at owner character levels 6, 18 and 30, respectively, and use the guardian's effective skill level. The guardian uses bolts at range, hammers nearby, and periodic targeted heavenly strikes at level 30+. Its aura-point damage bonus also increases these spells. At close range it also uses Zeal from owner character level 18, alternating with its other combat choices. Zeal performs two to five native weapon strikes, five server frames apart, with the guardian skill level controlling hit count, attack rating and damage. Each queued strike rechecks ownership, life, room and actual native melee reach; the sequence cancels when the target dies or leaves reach. It keeps its ordinary spear attacks and does not cast auras. Charge and additional shield skills are excluded from the current combat whitelist. Summon it again after updating: existing Valkyrie-type summons are not retroactively converted.

Sanctified Ground affects players and player-owned minions. Physical damage is increased after resistance; native fire, cold, lightning, poison and magic mastery handles the other damage types. The strongest ground effect wins without stacking. A field remains for eight seconds after moving; creating a new field replaces the old one. The recipient buff persists for its own eight seconds. Its regeneration and damage, and Retribution's stored-damage percentage, scale with Aura Mastery. Holy Freeze's slow is capped at 90% and its dependent calculations are amplified only once.

The five original aura definitions are retained as hidden skills for equipment-granted auras. Existing runewords keep their original resistance, Thorns or Cleansing effects. Existing skill IDs, internal names and tree positions remain stable; localized display names identify the new skills.

Holy Fire, Holy Freeze and Holy Shock include the damage formerly supplied by twenty hard points in each of their two damage synergies. Their base damage and all five growth bands use 5.8x, 5x and 4.2x respectively. Damage synergies and their tooltip rows are removed; pulse and added attack damage scale intrinsically. The multiplier applies throughout their level curve, including levels above 20. Native fixed-point precision keeps fractional growth accurate. This reference uses 20 points per synergy, rather than Reimagined's possible 25-point cap.

The Necromancer curse overhaul replaces the vanilla curse tree: Plague, Infection, Confuse with melee damage return, Attract with flat extra physical damage and a death explosion, growing Decrepify, Lower Resist affecting physical/magic as well as elemental resistance, Elemental Marks, and Soul Harvest. Curse Mastery and Plague Lord are passive. The previous colored overhead effects, curse icons and synergies are included. Skeleton warriors can be summoned without corpses. The port preserves Reimagined's other skills and the Astral minion bonuses.

Soul Harvest restores life and mana and grants independently expiring 15-second maximum-life/mana stacks. Recovery is `(1 + 0.3 * skill level)%`, multiplied by the relative 3%-per-hard-point Confuse/Attract synergy. Each stack adds `(0.5 + 0.08 * skill level)%` maximum life/mana. Only players collect souls. The previously invisible soul-orb graphic remains a known visual limitation; collection and stack logic are retained.

See [combat/README.md](combat/README.md) for implementation and verification details. These native hooks target D2R build 93847 and D2RCore 1.3.1-beta. Restart the game after updating. This is an offline modification.

## Build and install

The native x64 DLL is included. Run **Play Workshop.cmd** from the repository after installing. For another computer, run `scripts/Install-Workshop.ps1 -GameRoot 'D:\Games\Diablo II Resurrected'` in PowerShell, then launch D2RLoader with `-mod ReimaginedWorkshop -txt` from the game directory. The installer creates a separate mod folder and refuses to overwrite an existing one. The tree panel requires D2RLoader and HD graphics.

To regenerate and rebuild the plugins, run `scripts/Workshop.ps1 -Action Generate` followed by `scripts/Workshop.ps1 -Action Build` from PowerShell at the repository root. The SDK headers are pinned to D2RLoader/PluginSDK commit `03a5af9e0feb3f95f95e148e330e01f6635950df`. The generator restores the clean baseline from `tools/inputs/reimagined-base.zip` into the isolated build workspace (Python 3.10+ and Pillow on Windows). Generated data is committed; regeneration is unnecessary for play.

Upstream sources: [Reimagined](https://github.com/D2R-Reimagined/d2r-reimagined-mod) and [PluginSDK](https://github.com/D2RLoader/PluginSDK).

The tree uses native node tooltips, selection images, and counter segments.
F9 opens and closes the tree. Selection and counter updates resolve direct
native children on the UI thread through the layout verified for D2R build
93847. A byte guard prevents this path on unsupported builds. The overlay
callback handles cursor hit testing only. The panel stays open while its
existing marks and digit segments change.

For build 93847, native drawing checks widget byte 0x51. The updater verifies the corresponding native setter before changing this field. Inter-region shortcut lines and their allocation shortcuts have been removed.

Astral Sigils store their required character level separately from native skill requirements. The requirement is the number of allocated Astral points, with a minimum of level 1. A scoped, byte-guarded hook applies this value only to items with the Workshop requirement stat. Existing sigils must be regenerated with Apply to receive the corrected stat.

Version 0.5.0 replaces all 27 inner specializations with 11 attack/resource clusters and two four-rank skill clusters per class, including Warlock. Entry nodes grant +3 to all attributes. Class clusters each grant +1 to each of the three class trees and +1 to all skills of that class. Proc property rolls store chance first and skill level second. Corpsebreaker grants +15 maximum weapon damage per normal specialization node and +30 on its final node, with a 25% chance to cast level 1 Bash on attack. Deathfire grants a 5% chance to cast level 1 Corpse Explosion on kill. Poison infusion deals its listed attack damage over 2 seconds. Regenerate a carried sigil with Apply to receive the new specialization bonuses.

The Build action also compiles the combat plugin and runs its native regression tests. Run `scripts/Workshop.ps1 -Action Check` after data generation to validate the staging tables.

Version 0.8.3 fixes the Sanctified Ground activation crash: custom missile creation now supplies the entire native 0x78-byte parameter block, including zero-initialized optional fields and initialization-callback metadata. This also protects the other custom curse/ground projectiles. Gameplay balance is unchanged.

Version 0.8.4 gives the guardian a private Paladin player-form state and full character animation support. Its own MonEquip entries generate visible sword/shield/armor/helmet gear instead of leaving the private monster class unarmed. Native triggered skills temporarily select the guardian's actual learned skill node and restore its previous AI context afterward; real-game diagnosis found the learned nodes present but the used-skill slot empty.

All five new Paladin skill tooltips show calculated values using the established fractional formatter: efficiency, aura effect, guardian life/damage/aura-point totals and Zeal hit count, Retribution storage/cap/interval, and Sanctified Ground regeneration, damage, duration, radius and reservation. The floor/minimum matches runtime reservation. Sanctified Ground was verified in live server stat data: its physical bonus is applied after resistance and is not reflected in the character sheet's ordinary weapon-damage number. Regeneration becomes visible when life is missing.


## Local update 0.9.0

- Retribution physical Nova damage is now `500 + 250 * effective level`, then multiplied by aura effect. Third-hit triggering and radius scaling are unchanged.
- The Astral Sigil now has an explicit unique `invfile` and Misc `uniqueinvfile`, a dedicated HD registry key, a ground model alias, and normal/low-end inventory sprites fitted into its existing 1x1 footprint. Its artwork reuses the Reimagined Relic of the Ancients with preserved proportions and alpha. The earlier unique invfile was empty, so the name-only registry entry was insufficient.
- Infection and Plague swap skill-tab positions, required levels and prerequisite roles. Skill IDs and saved allocations retain their identities. Infection is the early level-1 skill; Plague takes its former later slot. Their base poison damage/scaling is halved and each hard-point synergy becomes 15%. Fixed-point shifts and the Infection rate formula preserve small fractional increments.
- Attract's additional physical damage is halved, including its fallback calculation. Death explosion damage is unchanged. Elemental Marks' fire, cold and lightning base damage and growth are halved through their private damage definitions, and their numerical tooltips match.
- The Astral panel starts with hidden digit segments, then updates the live widget tree over its first four UI frames. This covers deferred creation/cached roots without requiring Reset. Once ready, initial polling stops. Reset behavior and earned point budgets are unchanged.
- Magic and Rare ordinary affix multipliers exclude every Charm/Jewel type and subtype from the actual catalog. Runtime classification uses a guarded native item-code accessor rather than assuming TXT row numbers (expansion separator rows are not reliable class IDs). Existing saved rolls are not re-rolled.
- All eight classes walk at 9 and run at 14 instead of 6/9. The native tables have integer velocities, so 13.5 run speed rounds to 14. This is about +50% base movement, independent of item FRW.
- Hostile monster base HP is multiplied by 2 / 1.75 / 1.5 on Normal / Nightmare / Hell. Allied summon rows are unchanged. Resolved hostile monster damage receives the same difficulty factor in the existing post-resistance hit callback, including the native elemental components; players and player-owned summons are excluded. No additional scan is performed per hit.
- Infection remembers successfully applied marks by stable monster/player identities and expiry. If a tracked enemy dies without its native death proc, the server-frame fallback routes the same spread/explosion sequence. Native and fallback callbacks claim the same corpse, preventing duplicate explosions. Living targets which lose the curse stop being tracked. The fallback is regression-tested alongside nested chains; in-game reliability still needs combat validation.
- Mod-local MapSense and first-pickup materials deposit are adapted from the already-installed FMMP plugins. Full native signature and loader/core checks remain active. The first-pickup plugin's packed-mod identity is replaced with a hashed Workshop routing manifest, because this mod uses loose files. Unsupported/full/unavailable native bank routes remain ordinary inventory items. Withdrawn and re-dropped materials retain the upstream behavior.

Plugin adaptation inputs, hashes, exact edits and original third-party notices are under `integration/`. No FMMP encounters, rewards or collectible relic libraries are imported. MapSense includes its original external map helper. Start tests accepted both integrations; save/stash backups were taken before installation. The latest balance, UI and combat tests pass. Actual first-pickup deposits, map reveal and initial panel presentation should be checked in a live game.


## Paladin balance update 0.9.1

Retribution now deals `150 + 75 * effective level` physical Nova damage before aura effect (30% of the previous `500 + 250 * level`) and releases after every fifth damaging hit. Existing radius scaling, blocked-hit exclusions and queue safeguards remain unchanged.

Sanctuary retains magic damage and its existing targeting/utility. Its full base and five-band growth curve is 75% of the already-intrinsic Holy Shock pulse curve, quantized only to native 1/256 precision. The former Cleansing damage synergy and its tooltip rows are removed. Aura Mastery remains an independent multiplier.

All toggle reservation tiers rise by 50% before Reservation Efficiency: level requirements 1/6/12/18/24/30 now use `15+3L`, `30+6L`, `45+9L`, `60+12L`, `75+15L`, `90+18L`. The efficiency denominator is unchanged. Tooltips and native bindings share the same tier table. Direct summon casting costs are not reservations.

Every native Runes.txt sequence has `complete=0` in both profiles, preventing new socket-activated runewords. Individual rune socket bonuses and all approved forge recipes remain intact. Existing saved items are not deliberately stripped or rerolled.

Astral Guardians deal 20% of their previous resolved outgoing damage, covering weapon hits, spells, equipment and aura damage. Native summon life and aura-point stat bonuses are unchanged. Their private pet-type pool allows `1 + floor(effective level/10)` guardians: one at 1-9, two at 10-19, three at 20-29. Ordinary Valkyries keep their original pool.

The guardian tooltip now displays live maximum life and scaled weapon base min/max damage from an active guardian, plus the current summon cap and the 20% outgoing multiplier. Weapon base values explicitly precede strength/skill bonuses and target resistance; they are not spell damage or a promise of identical damage on every target. If several guardians are active, the displayed guardian is the one with the highest maximum life. Live values refresh during the existing one-second pet scan, remain available during attack cooldowns and clear when no valid guardian remains. Before summoning these live values are zero.

Checks cover all six reservation tiers and mana-boundary toggling, fifth-hit/ten-hit Nova sequences, total guardian physical/magic damage division, live tooltip publication/cleanup, Sanctuary's 75% curve at levels 1-99, separate summon limits, disabled native sequences and unchanged forging dispatch. In-game summon-count, live-tooltip and combat validation remains a user test.


## Guardian, tree and buff display repairs (0.9.2)

Guardian life now comes from one-third of its previous private MonStats base HP in every difficulty. Its tooltip uses native summon-life display line 13 and a fixed Holy Bolt base-damage calculation across the five native growth bands. These values are available before summoning; they no longer depend on transient server-to-client tooltip snapshot stats. Holy Bolt values include the guardian's aura-point/minion damage bonus and its existing 1/5 outgoing multiplier, before temporary buffs and enemy resistance. The old live snapshot publication code is removed.

`summon_limits.py` runs after all curse/Paladin skill replacement steps. Every player summon with a regular PetType/PetMax formula now reads the shared `ws_pet_count` stat exactly once. This repairs Astral Guardian, which was created after the old eight-family patch, and includes other data-driven summon families. Ravenlord grants +1 maximum summon; Swarm grants +1 at its final cluster node. Thus a level-20 Guardian cap becomes five with both completed count bonuses. Intermediate Swarm nodes retain their previously rounded zero count values and explicitly say they unlock the next node. Guardian life/damage/speed also read the established Astral minion stats.

Wind no longer grants stamina. Its three intermediate nodes grant 5% Faster Hit Recovery each; the final node grants 10% Faster Hit Recovery and 15% Faster Run/Walk. Re-Apply an existing Sigil to replace stored stamina rolls. Existing stat save widths are preserved, avoiding an incompatible saved-item format change.

All 452 hover descriptions are generated in item-style English from the actual property payloads. Proc chances and skill levels are distinguished, renamed curses/aura replacements use their current names, and Blood Drinker reads `6% Life stolen per hit`. Keystones state that they replace primary inner/outer path bonuses.

The final Keystone rule is one point each, at most three. Character levels 30, 60 and 90 unlock the first, second and third slots. Slots can be spent on any reachable Keystone. The resulting Sigil also requires at least level 30/60/90 for one/two/three Keystones, preventing transfer to a lower-level character from bypassing the unlock. Selection and Apply both enforce the rule; removing one refunds its point and frees a slot. Older allocations can still be read to allow manual adjustment, but invalid over-limit allocations cannot be applied. The proposed five-point cost was superseded before installation.

Buff HUD 1.3.1+rev.3 is installed mod-locally with its required companion MPQ and original license. A loose layout override anchors its 21 slots at bottom right, above the action bar. A mod-aware catalog includes ordinary temporary buffs, resource shields, shrines, Workshop aura states and Sanctified Ground. Retribution publishes an empty timed aura indicator so it can appear without restoring native Thorns damage. The rest of its combat logic is unchanged. Source/release: https://github.com/Pyre-Flies/d2rl-buff-hud . The fixed tooltip, summon formulas, Wind effects, node labels, Keystone level boundaries and HUD layout have regression coverage; visual placement and native summon-life rendering still need an in-game check.


## Paladin tuning, set reforging and tree fixes (0.9.3)

Astral Guardian now shows a numerical normal-difficulty base-life value (the upper native base roll, before equipment and difficulty adjustments), using a regular calculated tooltip line. Its private Holy Bolt, Blessed Hammer and Fist of the Heavens base damage curves are five times stronger; private Zeal has an additional 400% baseline skill damage and five times its previous level-based damage bonus. The global guardian output multiplier remains 1/5. Pet scans run every 10 frames; spell cooldowns are 20 frames (Holy Bolt/Hammer), 35 frames (FoH), and 25 frames between Zeal bursts. Original player skills and ordinary Valkyries are unchanged. Re-summon existing guardians to update their learned private skills.

Retribution damage is now `50 + 25 * skill level` before Aura Mastery, with the same five-attempt counter. Counting moved from the post-resistance leech callback to the earlier native damage-event entry: blocked attacks, armor misses and zero-damage attempts are counted too. Resolved physical damage amplification remains in the late hit callback, so it is not duplicated. Reflections and self-hits are excluded.

Prayer heals twice as much. Defiance adds 1% physical damage reduction per level; Might adds 1% Crushing Blow per level; Blessed Aim adds 1% Deadly Strike per level; Conviction also reduces physical resistance by 1% per level. Salvation adds 1% maximum fire/cold/lightning resistance for each completed ten levels. These new aura values use the established Aura Mastery multiplier once and keep the engine's ordinary caps. New numerical tooltip lines expose these bonuses.

Redemption grants +1% maximum life per completed ten skill levels for every corpse actually consumed by native Redemption, not every nearby death or failed chance roll. Each corpse's stack expires independently after 15 seconds. The bonus uses a separate timed native max-life-percentage stat list and never changes the character's saved base life or grants mana. It coexists with Soul Harvest. The successful redemption handler is guarded against the exact build-93847 function signature.

A single set item in the Cube now rerolls into one uniformly chosen piece from the same set, including possibly the same piece. The catalog contains 455 active pieces in 107 sets. Native Cube matching uses each exact set item name, so base items shared between different sets cannot cross sets. All modifiers, sockets and previous forging/corruption are rerolled away. The native recipe produces a private saveable per-set receipt, then the next authoritative game update atomically exchanges it for the randomly selected exact SetItems row. Output appears in the Cube. If placement fails, the receipt stays; clear enough Cube space and cube it again. Retry selection is seed-stable. Receipts never spawn as loot. Normal rune forging dispatch remains separate.

Ravenlord now uses an explicit `ws-pet-count` property constant instead of the former Swarm stem's property. That stale reference had become Energy, producing a zero Energy write that could overwrite the aggregated attribute total. The regression test combines Ravenlord and Grave Lore and checks both +1 maximum summon and the complete Energy total. The final nodes of Titan, Finesse, Fortitude, Grave Lore and Deep Mind give +20 of their attribute plus +10% of that attribute. Existing Sigils require Apply to regenerate their saved properties.

Buff HUD starts 120 UI units farther right. Its 21 slots wrap in six columns/four rows to keep every icon on screen; the first row retains its former vertical position. The local Frankenstein monster-display plugin is retargeted to Workshop with its original native guards preserved and input SHA-256 pinned. It provides health and resistance information for the hovered enemy.

Validation covers every set group/member, exact quality row and base code, queued Cube dispatch, rollback and stable retry, normal-item forwarding, independent Redemption expiry and successful-only awards, block/miss counting, all aura formulas/stat names, private spell curves, numerical tooltips, attribute/count aggregation and on-screen HUD bounds. Visual placement, live combat balance and the final native Cube exchange still need the user's in-game check.


## Native binding and direct Cube fixes (0.9.4)

The earlier minion fix still used incorrect documentation IDs: generated Properties and ItemStatCost rows had been appended in a different order from their `*Id` annotations. D2's linkers use physical row order (and skip the Properties Expansion separator), so the apparent summon-count property actually targeted Energy. `table_ids.py` now updates the annotations to the real linker IDs before generating C++ bindings. No existing row is reordered. All 503 existing stat names, Save Bits, Save Add, Save Param Bits and ValShift definitions are unchanged; this preserves existing saved-item decoding. A new native data-load audit checks every custom property against its actual compiled stat reference, and reports PASS in the startup log.

The mixed-Clarity Mana overflow had a second independent cause. Properties already apply ItemStatCost.ValShift. Multiplying a wide Mana property by 256 before submitting it converted it twice. Wide properties now receive ordinary logical amounts; their native property handler performs the single conversion. Their network Send Bits are 32, while the existing 16-bit saved encoding stays unchanged. The regression reproduces full Deep Well + Clarity 3/5 + full Siphon and verifies exactly 446 Mana in the generated property, rather than 114176. Ordinary low item-save limits are bypassed through the established wider fields; native integer/save-format limits still exist.

Swarm nodes 2, 3, 4 and 5 now each grant +1 maximum summon; the final node retains its +25% minion-life bonus. A fully allocated Swarm therefore grants four additional summons, independently of Ravenlord's one. Re-Apply existing Sigils and re-summon pets.

The additional Might/Blessed Aim/Defiance/Conviction/Salvation/Redemption tooltip lines now use normal `descline` numeric formatters. The secondary `dsc2` formatter does not support the same numeric line function and had silently omitted them. Salvation and Redemption start with 1% at level 1 and gain another percentage point each ten skill levels before Aura Mastery. Salvation grants all four elemental maximum resistances, including poison; poison is added to the same native timed aura list after its pulse because Skills.txt exposes only six aura-stat slots. Redemption's independent 15-second corpse stacks otherwise retain their existing mechanics.

Set reroll is now entirely native Cube output. Five hidden, unsaved, server-only stats select a uniform member index for set sizes 2-6; a bounded existing frame callback refreshes them for living players. Native op-6 recipes match that selector and create an exact named SetItems output. There is one matching recipe for every valid selector value, and no temporary receipt, queued inventory scan or ItemService transaction. All 455 active pieces and 107 sets are covered. Legacy receipt item codes retain their old row order and have direct recovery recipes: cube one once. The old asynchronous bridge is retired.

Checks cover mixed-Mana aggregation, all 340 cluster allocations, minion-count plus Energy aggregation, every possible direct set recipe and selector, bounded/unbiased selector generation, actual writes to living players, existing saved-stat encoding, numeric aura tooltips, independent Redemption expiry, and the compiled-property native startup audit. Combat/visual checks remain user-run.

## Swarm middle nodes (0.9.5)

Swarm nodes 2-4 each grant 10% minion damage and 10% minion maximum life. Only node 5 grants +1 maximum summon and retains its 25% minion-life bonus. The complete specialty gives 30% minion damage, 55% minion life and +1 summon. CLUSTER-OVERRIDES.json contains the multi-effect middle-node definitions. Existing Sigils need Apply to regenerate their saved bonuses.



## Excel balance import (version 0.9.6)

Imported 67 edited effect cells from `tables/Astral-Skilltree-0.9.5.xlsx` without rewriting the workbook. Leading Excel text apostrophes are ignored per line. All 452 displayed effects were compared with the edited workbook. The carried Sigil retains its saved rolls until **Apply** replaces it with the current balance.

The import includes the revised Keystone proc chances, +2 maximum summons from Ravenlord, +3 Energy Shield from Soul Font, Rimeheart's freeze immunity plus 200% faster hit recovery, revised attribute and resource clusters, physical resistance penetration in Night Sight, and flat Life on hit in Reaping. Poison attack damage now lasts one second. Native melee and missile hit events handle Life on hit.

Rimeheart exceeds the original faster-hit-recovery item field. An appended 16-bit storage stat contributes the full value through the existing flat-stat unit, preserving every earlier saved-stat record and ID. Existing Sigils remain readable. Multi-effect Keystone definitions use `keystone_extra` in `CLUSTER-OVERRIDES.json`; non-inner rank overrides use its `ranks` list. `INNER-CLUSTERS.json` remains authoritative for inner ranks.

The reusable importer is `scripts/spreadsheets/skilltree/import_values.py`. Run it without arguments to validate and report changes, or with `--apply` to update the source definitions. `verify_import.py` audits the generated node text and unchanged save-field layout. Both scripts read the workbook without saving it.


## Ring layout (version 0.9.7)

The 452-node tree now uses two staggered inner cluster rings and one outer cluster ring, with Keystones at the perimeter. Clusters follow equal distances along the elliptical orbit rather than equal angles. Each specialization forms a five-node branch with consistent spacing. The extra Swarm branch sits inside the Legion sector. Larger node and selection sprites use the same coordinates as native buttons and hover detection. Long connecting routes are muted so the specialization branches remain clearer. Region labels sit in the central space.

The exporter rejects overlapping node outlines/hover circles and nodes outside the usable tree area. Gameplay payloads, allocation IDs, prerequisites, point costs and saved Sigils are unchanged.


## Inward entries and Legion cleanup (version 0.9.8)

Each attribute entry is now the innermost node of its own cluster. The four specialization nodes form a compact box on its clockwise side, leaving the main path clear. Long connectors are generated around other node outlines and entire specialization boxes. Asset-generation checks reject intersected nodes and overlapping hover circles. Routing runs only during generation and adds no runtime search cost.

Broodheart's five nodes are removed from the active tree. The extra Legion spoke now uses Fury as its independent inner entry, branching to Rush on the staggered middle ring and Swarm on the outer ring. This leaves 84 ordinary clusters and 27 Keystones, or 447 active nodes. Every remaining node keeps its original ID and bonuses.

Older carried Sigils remain readable. Their Broodheart allocation is stripped from the draft, returning its spent points. Press **Apply** once to rebuild the Sigil with the current allocation and remove its old Broodheart bonuses. The old source record and five numeric slots are reserved rather than renumbering existing save data; they have no visible widgets, hover targets or allocatable nodes. Fresh table exports omit the retired record. The previously edited Excel file is retained as the original input snapshot.


## Astral Sanctum artwork (version 0.9.9)

Approved direction C supplies the text-free plum-purple cosmic background and three round lilac node emblems: crescent/spark for ordinary nodes, bronze triskelion for cluster endpoints, and the layered crescent/orb seal for Keystones. The concept-board title and labels are absent from all source artwork. Functional tree labels and buttons remain part of the UI.

Original PNGs and generation notes are kept in `src/workshop/art/astral-sanctum/`. The icons have transparent exteriors and opaque dark interiors. They are packed directly at native UI resolution to avoid reducing detail before the final upscale. Press and hover outlines are generated separately; allocated nodes retain their visible gold selection halo. Only the rendered graphics change; node positions, IDs, payloads, costs and save data are unchanged.


## Straight Keystone branches and Treasure (version 0.10.0)

The tree has 27 separate branches and no connections between branches. A normal main path follows Inner -> Side -> Outer -> Keystone. Every attribute entry has exactly one four-node specialization chain extending clockwise and outward. The three additional Legion clusters are integrated into the three Legion stems before their Keystones: Fury in the first, Rush in the second, Swarm in the third. These three stems have four attribute entries. Broodheart remains retired. All 447 active nodes retain their existing IDs and effect values.

`tree_layout.py` generates both the straight lines and the native parent table, so learned prerequisites match the visible paths. Generation checks all node pairs, every line against foreign node outlines, and every pair of unrelated lines. The previous orbit connectors and route-search paths are removed completely. Clearing a main-path node returns all now-unreachable descendants. When older Sigils are read, allocations incompatible with the new serial paths are pruned and their points become available; **Apply** rebuilds the carried Sigil with the current allocation.

Treasure's +50/+50/+50/+100 percent Gold Find was previously removed by Alchemist's primary-bonus exchange. Treasure's learned effects are now retained in that exchange. A full specialization grants +250% extra gold both before and after learning Alchemist, plus its existing final Magic Find bonus. The Keystone tooltip explicitly notes that Treasure gold remains. Other Keystone exchanges are unchanged. Regression tests verify the minted Sigil properties in both cases.


## Sigil property repair (version 0.10.1)

The creation API's ordinary minimum/maximum validation rejected several proc properties because their fields represent chance and triggered skill level, for example 25% chance at level 8. Sigil creation now submits one ordinary selector and expands its scoped payload synchronously through the already guarded native cube-property hook. The selector never remains on the item. Only this plugin's active Sigil transaction on a Wsig item can consume it; ordinary rune forging keeps its separate static selectors. Creation logs confirm whether the native payload executed and list requested modifiers when the API rejects an operation.

Keystones are now fully additive. The old primary Inner/Outer exchange removed class training, flat elemental damage, mastery, minimum damage and movement bonuses when selecting their branch Keystone. That exchange is removed for every Keystone. Learned tabs, class bonuses and other specializations remain present together, including Pursuit's +61% run/walk and +10% hit recovery with Wind's +25% hit recovery and +15% run/walk (combined +76% / +35%).

The existing Lightning minimum item field stores at most 63; the Poison rate fields store at most 1023. Two appended op-2 stats preserve complete Lightning minimum damage and equal Poison minimum/maximum rates without changing any previous item-stat row, width or ID. Separate direct properties keep Lightning maximum damage and Poison duration. Both Lightning specializations can retain their full 150 minimum/maximum values; Poison Infusion retains its one-second duration and full raw rate.

Tests now enforce the actual API min/max restriction on submitted properties before simulating native dispatch. They cover all 336 specialization ranks and 27 Keystones, all 24 class tabs with Keystones, descending chance/level tuples, native selector isolation, retained elemental/physical bonuses, Lightning/Poison overflow, Pursuit plus Wind and the existing rune-forge/affix regressions. Press **Apply** after updating to rebuild an existing Sigil with the corrected properties. Combat proc behaviour still needs an in-game check.


## Elemental spell damage balance (version 0.10.2)

Tempest (Lightning), Inferno (Fire), Glacier (Cold) and Virulence (Poison) now grant +7% skill damage on each of their three ordinary specialization nodes and +15% on the endpoint. This uses the requested whole-number fallback because the native mastery stats store integer percentages. A complete cluster grants +36%, with the existing -5% enemy resistance endpoint bonus retained. Flat attack-damage Infusions and every other node keep their current values. Apply again to update a carried Sigil.

## Version 0.11.0 â€” Guardian and aura balance

Astral Guardians use counted pet icons, hard ground collision and skeleton movement speeds/AI delay. They no longer fly or inherit the Valkyrie movement-speed bonus. Life and all resolved damage are halved relative to 0.10.2; tooltips follow the new values. Resummon existing Guardians after updating. Friendly owner collision uses the native summon handling.

Original effects and level growth of the reservable auras are halved, except Prayer and Retribution. Later added Crushing Blow, Deadly Strike, physical damage reduction, physical resistance reduction, maximum resistance and Redemption maximum-life bonuses are retained. Reservation Efficiency and Aura Mastery are unchanged. Reservation is increased by 50% before efficiency and rounded down once to whole mana: `(tierBase + tierGrowth * skillLevel) * 150 / (100 + efficiency)`. Sanctified Ground runtime effects match its reduced tooltips.

Pack Fury grants 15% minion damage on each of its three small nodes and 30% on its endpoint; the endpoint still grants 10% minion speed. Apply the tree again to update an existing Sigil.

## Version 0.11.1 — Runeforged word labels and crash investigation

Newly forged equipment shows `Runeforged (Stealth)` (or its actual word). Identity is saved directly in the existing two-bit `workshop_forged` and seven-bit `ws_forge_level` fields; their widths and IDs are unchanged. Marker 1 remains the legacy level-floor format. Markers 2/3 identify a word in the stable 220-entry forging catalog, whose rune-level floor is restored by the native requirement hook. Old equipment retains all bonuses and requirements, but its original word was not saved and cannot be reliably reconstructed; it retains the generic `Runeforged` line. Do not reorder the established forging catalog entries.

The read-only item tooltip contribution consumes a short-lived copy of native requirement metadata, matched to the public item runtime ID. No borrowed native pointer is retained and no item mutation occurs while hovering.

The reported Stony Field crash occurred at native RVA `0x3FA811`, a room-grid constructor called from `0x3F965F`, with an invalid destination pointer. This does not establish a Skill Shrine as the cause. Native automatic map reveal is temporarily disabled as a reversible diagnostic mitigation; MapSense overlay and monster markers remain enabled. A repeat in-game test is needed to establish whether it resolves the crash. No low-level exception is suppressed or invalid-pointer write skipped.

Separate loader assertions reported missing `skilldesc` links (including `mercprop`, skill 532). Runtime-only helper skills now have valid off-tree descriptions; their functions, skill IDs, classes, effects and prerequisites are unchanged.

Automatic map reveal was re-enabled on 2026-11-11 at the user’s request. The temporary reveal-disable override has been removed from the generator.

## Version 0.12.0 — Barbarian overhaul

The Barbarian prototype includes the full requested skill changes, Carnage, Feast of War, Quake, Bleeding/Impale, three timed Ancients and ESR-style held-button Whirlwind steering. See [BARBARIAN.md](BARBARIAN.md) for formulas, slot changes, implementation boundaries and test details.

## Version 0.12.1 — Barbarian corrections

Fixed globally colliding Barbarian/internal text IDs, renamed Carnage to Rage, corrected Quake's variants/client startup/HD visual, and converted Feast of War into an automatic corpse-consuming mana-reservation toggle (`15 + 3 × skill level`, before efficiency). Added a native physical Shower of Rocks prototype in the old Frenzy position. See BARBARIAN.md for formulas and testing boundaries.

## Version 0.12.2 — Barbarian combat and visuals

Natural Resistance now raises maximum resistances every five skill levels. Shower of Rocks is a no-cooldown melee attack with Berserk accuracy, counted on-hit rocks and 20% weapon damage per rock. Quake gains level-scaled projectile count, native hit/Pierce behavior and a mana tooltip; Battle Cry also shows mana cost. Rage has explicit timed buff metadata. Ancients gain private HD registrations, untargetable/invulnerable records and expiry through the guarded native pet manager.

## Version 0.12.3 — Barbarian lifecycle fixes

Fixed Ancient skill sequences and one-shot final world removal, removed their life tooltip, started Natural Resistance maximum resistance at skill level 1, tracked Rage for ten seconds, and split rockfall visuals from delayed native weapon hits. Corrected a hit-result filter that rejected successful hits carrying an additional result bit.

## Version 0.12.4 — native context and presentation corrections

Fixed Ancient skill selection after native mode initialization, corrected Madawc's Cast mode, and queued the actual client room-deletion packet on despawn. Restored the working Blizzard action as Shower of Rocks' on-striking spell with the current weapon fraction/count and original Armageddon rock art. Rage uses its public name/icon; logs distinguish it from the one-second Shock Wave proc.

## Version 0.12.11 — original Madawc skill and kill timing

Madawc uses original Double Throw with equipped throwing axes and two attack events. Talic receives melee equipment and the original animated-path policy. Rage confirms kills after native HP subtraction, and rock collisions cannot recursively trigger their own on-striking rain.
