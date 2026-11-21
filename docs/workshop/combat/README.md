# Workshop combat port

`port.py` transfers selected curse records and assets from the tested Frankenstein 0.5.22 source snapshot. It allocates fresh IDs after Reimagined records and generates `src/` from `original/`; edit the originals or binding transformation when changing curse code. Never copy the entire source mod tables into Reimagined. Skill 81 retains Reimagined's canonical Corpse Field identifier so existing references remain valid; its display and behavior become Confuse.

`aura_data.py` converts the twenty Paladin auras to castable toggles and generates private bindings. `aura_buffs.h` manages reservation, pulses and cleanup using native stat lists. Reservation never edits saved base mana. Soul Harvest reads unreserved maximum mana for its percentage calculation. Equipment aura callbacks are forwarded and renewed independently of manual toggles.

All native entry points are byte-guarded. The combat plugin hooks the original frame dispatcher once, runs the aura lifecycle, then Soul Harvest and curse updates. Its hooks are distinct from the Astral/forge/affix plugin. Unsupported native builds are rejected instead of patched blindly.

Build with `scripts/Workshop.ps1 -Action Build` from PowerShell at the repository root, after running the Generate action. Seven curse regression programs test poison/infection, growth, multi-target processing, native regeneration cadence, skeleton procs, and independent Soul Harvest stacks. `aura_test.cpp` tests simultaneous buffs, reservation, toggles, pulses, skill-level and equipment changes, insufficient budget, death, unrelated skill forwarding and free equipment auras. These are mocked native tests; gameplay and graphics still require testing inside D2R.

The upstream snapshot contains only relevant records, localization and visual assets. Existing Reimagined icon atlases receive only the ten curse frames. The data verifier checks names, private IDs and base/expansion parity. Do not edit generated IDs manually.

## Paladin replacement layer

`aura_features_data.py` runs after aura conversion and reservation setup. It keeps canonical skill IDs/names for existing Reimagined formulas and items while changing five player skills. Generated `FEATURES.json` and `aura_features_bindings.h` bind four transient stats (496-499), one timed state, five original item-aura copies and the physical Nova helper. Regenerate from pristine upstream tables, not already-generated data.

`aura_features.h` uses the existing frame callback and the existing guarded post-resistance hit hook. Ground targets are scanned only every ten server frames, not each render frame or damage hit. The native player/monster hash-array offsets and unit links are checked against the supported build. Aura effect is baked into native aura calculations; dependent `ast1` expressions are not amplified twice. Both passives and summon are excluded from toggle dispatch and do not reserve mana. Equipment calls use hidden old aura definitions.

`aura_features_test.cpp` verifies efficiency recalculation, all new dispatch exclusions, the stationary threshold, friendly players/minions, eight-second buff expiry, strongest/nonstacking behavior, room separation, Aura Mastery, physical damage, Retribution charge/release and blocked-hit guards, and legacy item aura routing. `aura_features_verify.py` checks the tables, summon route, hard-point formulas and tooltips. These tests do not replace visual/combat testing inside D2R.

## Guardian combat (0.8.1)

`guardian_data.py` appends one private Valkyrie-based monster class, three private copies of Paladin combat spells and their gameplay missiles. It assigns these skills through the summon skill. GenericPet skill slots remain empty so native melee AI cannot bypass the controlled spell cadence. Native player skills, ordinary Valkyries and their AI remain untouched. `GUARDIAN.json` and `guardian_bindings.h` record the allocated IDs.

`guardian_combat.h` is called by the existing aura feature frame hook. One bounded scan per second finds guardians and hostile monsters. It uses native ownership, health, alignment, room, distance and owner-level checks, then dispatches the selected skill through the existing native triggered-skill function. Successful casts have a two-second delay (four for Fist of the Heavens); failures retry after a second. Only Holy Bolt, Blessed Hammer and Fist of the Heavens are whitelisted. Shield skills and player movement/attack sequences are not invoked on the spear-based unit. No new hooks or player-input automation are installed.

Private spell damage uses the guardian's damage-percent stat, so its existing aura-point scaling extends to spells. Shared player skills and projectiles are never mutated. The summon still uses its previous life/stat/equipment formulas and original one-pet cap.

`guardian_test.cpp` tests autonomous casting with the guardian as caster, level unlocks, target selection, cooldowns, room/alignment/death/respec guards and ordinary-Valkyrie exclusion. `guardian_verify.py` checks all private spell/missile links, summon assignments and unchanged player-skill definitions. Gameplay/visual validation remains an in-game check.

### Guardian Zeal (0.8.2)

A fourth private combat skill retains Paladin Zeal's native server start/do functions (37/13), weapon source damage, attack-rating curve and five-hit cap. It enables triggered activation and a monster-compatible A1 visual mode. Player Zeal is untouched. The runtime prioritizes Zeal at contact distance on alternating actions from owner level 18. A bounded, five-frame scheduler dispatches two to five native strikes instead of relying on a Paladin player animation loop. It stores only pet/target IDs, re-resolves ownership and health for every strike, checks native weapon reach, and cancels on invalid or dead targets. No additional world scans run during the sequence. Native reach functions are byte-guarded during installation.

The guardian regression additionally tests staggered hits, exact hit cap, cooldown, distant/dead-target cancellation and the melee priority. The actual monster animation and native hit sequence still need an in-game combat check.

## Sanctified Ground crash correction (0.8.3)

The November 9 crash report identified native missile creation calling an invalid initialization-function table entry: the fault at game RVA 0x3BAD6D was reached from 0x5379D8. The stack held GroundVisual's missile 81, skill 109 and range 100. The previous `Pulse::Params` stopped at offset 0x60, but the native creator reads optional fields at 0x64/0x68 and an initialization-function index/argument at 0x70/0x74. Those reads consumed adjacent stack data; the bogus function index matched the low bytes of a plugin address.

The shared parameter block is now 0x78 bytes. All custom creation sites value-initialize the entire block. Static assertions protect its size and native offsets. The pulse test uses poisoned adjacent memory and emulates the native tail reads, and verifies that forwarded native requests retain valid callback metadata. The Sanctified Ground regression explicitly runs GroundVisual through a creator stub which checks offset 0x70 and 0x74. No skill values, aura balance or graphics were removed to hide the problem.

## Guardian appearance, native skill context and tooltips (0.8.4)

Live read-only diagnosis confirmed learned private skill nodes 566-569 at level 25 but a null used-skill slot. The native triggered dispatcher may report success while the start/do path lacks this context. `SkillScope` locates the already-learned node, temporarily binds it at skill-list +0x18, and restores the previous slot with RAII. The layout is guarded against the native getter. Regression cases cover both null and non-null previous slots and missing nodes; burst tests run through the same selection path.

Private MonEquip rows now cover sword, shield, torso and head, with normal/exceptional/elite upgrades, in descending equipment-level order. A private MonStats2 row enables all needed player animation modes. A private transform state renders Paladin class 3; the native Valkyrie class-0 state is removed from this pet and replaced, without changing ordinary Amazons or Valkyries. State and inventory refreshes notify clients. Re-summon after updating.

Live server values also confirmed Sanctified Ground already contributing physical damage stat 498, elemental/magic mastery and life regeneration. Its post-resistance physical increase is deliberately not a character-sheet weapon modifier. `tooltip_verify.py` now checks numerical formatting for every new skill, including fractional regeneration and the same minimum/floor used for reservation. Labels use the proven same-caption `descline=36` format instead of plain captions without numeric placeholders. Formulas are regenerated for normal and base tables.

## Equipment aura skills and combat presentation (0.8.5)

The `aura` item property now uses the ordinary all-class skill grant functions (display stat 387 and skill stat 97). Skill levels are preserved. This applies to new drops and forged modifiers at every quality. Equip the item, select its granted skill and activate the buff; active buffs reserve mana under the normal Workshop rules. For the five replaced Paladin skills, the granted skill is the Workshop replacement. These grants follow native oskill restrictions, including the native same-class cap.

`item_auras.h` bridges old item-owned stat-151 writes to ordinary skill grants while saved items are reconstructed. Other unit types and unrelated writes are forwarded unchanged; existing same-layer skill contributions are retained. This migration is checked with mock stat lists; a previously saved aura item still needs an in-game load/equip test. Back up mod saves before first use. The hook is byte-guarded for the supported game build. No save files are directly edited.

Guardian Zeal now selects using native melee reach rather than a second center-distance cutoff, and receives a short-lived attack-rate stat list during its bounded strike sequence. Ordinary Valkyries and player Zeal remain unchanged. Sanctified Ground has its own harmless client-visible missile and HD seal prefab; neither adds damage or collision. The marker lifetime is bounded by the ground lifetime. Combat and appearance remain subject to in-game verification.

Checks: full curse/aura/guardian regression suite, old-item conversion forwarding tests, generated item-property equivalence in both table sets, ground prefab/registry validation, guardian bindings, numerical tooltips, Astral allocations, forging and affix scaling.

## Painted Paladin replacement icons

The five replacement skills use original built-in Imagegen artwork matching the custom curse icons: Reservation Efficiency (blue mana crescent), Aura Mastery (golden sun crystal), Astral Guardian (armored cyan-lit helm), Retribution (ruby shield), and Sanctified Ground (golden sacred seal). Source PNGs, the exact prompts and contact sheets live in `icons/`.

`paladin_icons.py` reads actual `IconCel` values from generated skill descriptions and replaces only their five normal/dim frame pairs in both the HD and low-end Paladin atlases. All other pixels and atlas headers are checked against pristine inputs. Regeneration runs after the skill replacements. This is an artwork-only change: no gameplay values or DLLs need rebuilding. Restart D2R to refresh cached UI art.


## Retribution: every third received hit (0.8.6)

While the toggle is active, every third resolved hit dealing positive damage queues one physical Nova. Incoming damage magnitude no longer determines output. Blocked, dodged, missed and zero-damage results do not count. There is no three-second timer. Multiple earned releases remain queued, with at most one per server frame to avoid recursive hit resolution. Failed native casts retain their release for retry; death, deactivation and insufficient-mana deactivation clear both partial hits and pending Novas.

Damage is `floor((500 + 250 * effective skill level) * (100 + aura effect) / 100)`. Nova radius scales from 100% at level 1 by 3 percentage points per additional effective level, then multiplies by aura effect. A level-20 Nova deals 5500 physical damage and has 157% base radius before aura effect. At +60% aura effect these become 8800 damage and approximately 251% radius. Native projectile speed is integer-valued, so travel radius has small rounding steps.

The supported build's server Nova handler (RVA 0x553CF0, calculation/call at 0x553DE6-0x553E2E) adds `calc1` to base missile velocity. The private helper uses this for radius growth at fixed lifetime, with an identical `cltcalc1` for client presentation. Original Sorceress Nova remains unchanged. Actual skill level is passed to the native triggered cast; the transient stat 499 snapshots only physical damage during missile creation. It is cleared immediately after dispatch. Both the counter and snapshot are transient, not save-file changes.

Regression checks cover hits 1/2/3, six-hit double release, tiny versus large incoming damage, no periodic release, blocked/zero/missed results, nested dispatch, queued casts, retry, death/toggle cleanup, level/effect scaling and computed tooltip formulas. These checks do not replace the user's in-game combat test.


## Local balance and Infection recovery (0.9.0)

The Workshop-level `tuning.py` runs once after the original curse port, so the upstream curse tables remain reusable. Its half-damage adjustments are fixed-point safe and include the corresponding tooltip expressions. The two poison skills now have 15% hard-point synergies. The original script's prerequisite graph is remapped with the two moved slots, avoiding a level-1 icon that secretly requires a later node or a prerequisite cycle.

`Virus::Track` records successful Infection applications from `Curses::OnApply` using IDs, class, owner, level and duration. `Virus::Poll` runs after the native frame, before stale clock targets are removed. A missing death event routes the ordinary queued native death handler. `Claim` coordinates native and fallback paths for each tracked corpse; the recovery test verifies both an absent native event and a late duplicate native event. Living targets which lose the mark or whose expiry ends are removed. Session/frame rollback clears tracking. The 4096-entry bound matches the existing curse target/chain limits.

`AuraFeatures::MonsterDamage` scales only hostile, non-player-owned monsters by difficulty. It reuses the existing guarded resolved-hit callback; no new damage hook or timer is installed. Physical, fire, burn, lightning, magic, cold, poison and total damage values are bounded to safe positive 32-bit values; durations, flags and leech fields are not modified. Health adjustments are restricted to hostile enabled/killable MonStats rows. The regression tests check all three damage factors while existing ground/guardian/curse tests remain passing.


## Paladin adjustments (0.9.1)

Retribution uses five damaging hits and `150 + 75L` damage; reservation costs are 1.5x their previous tier values. `paladin_balance.py` derives Sanctuary's magic pulse from 75% of Holy Shock's intrinsic curve and disables native socket-sequence recognition without changing forge properties. The corresponding checks run in `paladin_verify.py`.

The guardian's private outgoing-hit branch divides the resolved native damage components and total by five exactly once. It excludes every other monster class and runs before ordinary hostile-monster scaling. The dedicated pet type isolates its `1+lvl/10` count from normal Valkyries. Existing per-pet combat schedules continue to handle each guardian separately.

Three transient, non-save stats (500-502) carry the active guardian's actual maximum life and scaled weapon-base min/max into the owner tooltip. `Observe` collects values during the existing scan; `Publish` writes only changed values and clears stale owners. A separate bounded scan is not introduced. The tooltip labels base weapon values explicitly because target resistance, strength, Zeal and combat-spell damage differ. Summon to populate the live fields.


## Fixed Guardian display and shared tree limits (0.9.2)

The 0.9.1 live tooltip-stat transport did not produce visible numbers in the user's game. Guardian descriptions now use the native summon-life formatter (line 13, `clc1`) and explicit five-band Holy Bolt base-damage expressions, so they do not require an active pet or custom stat synchronization. The three old transient stat IDs remain reserved to keep bindings stable, but no longer drive the tooltip or receive per-second writes. Private Guardian MonStats HP fields are divided by three. Equipment and other native summon modifiers remain native.

Guardian formulas include Astral minion life/damage/speed, and the shared summon-limit pass is applied after private skill creation. Its default count remains `1+floor(level/10)` with the Astral count added once. Native summon and casting behavior, private pet pool, spell cadence and the outgoing 1/5 damage factor remain intact.

The optional Buff HUD companion reads a generated mod-aware catalog. Retribution's existing periodic feature callback now maintains only an empty timed native aura-state list as an indicator. Disable expires that state through the existing aura lifecycle. This adds no old reflected-damage stat and no independent timer or scanning loop.
