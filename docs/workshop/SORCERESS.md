# Sorceress rework prototype â€” 0.13.2

Public skill IDs are preserved so existing learned skills and item skill references remain loadable. The tree positions change; removed skills become the new skills at those IDs. Existing characters may need to respec to redistribute their old points.

| Skill | Placement and behavior | Initial prototype values |
|---|---|---|
| Fire Ball | Fire Bolt position; level 1 | Existing Reimagined damage curve |
| Fire Wall | Inferno position; level 6 | Existing Reimagined damage curve |
| Conflagration | Old Fire Wall position; level 18 | 250â€“400 fire damage at level 1, +10 per level; an initial nova and three once-per-second ground ticks; radius 24 subtiles (16 yards); 3-second field, 10-second cooldown |
| Avatar of Fire | Old Fire Ball position; level 12 | Fire Golem form; 25+5L% maximum life, 10+2L% movement/attack/cast/recovery/block speeds, 100+10L% attack rating, 10+5L to 20+10L added fire damage; reserved mana; recast to revert |
| Enchant | Existing position | Original ally enchantment. The 50% Firestorm proc now belongs to Avatar of Fire at Avatar level. |
| Warmth | Existing position | Existing regeneration/kill recovery; 50+10L life; 3+floor(L/2)% life and mana steal |
| Frost Guardians | Frost Nova position; level 6 | 2+floor(L/10) guardians plus Astral summon bonuses; 500+100L life contribution, 2+L to 4+2L melee damage, nova on successful melee hit (every successful hit, without an additional cooldown) |
| Ice Blast | Old Ice Bolt position; level 1 | Existing Reimagined damage curve |
| Glacial Spike | Old Ice Blast position; level 6 | Requires Ice Blast; existing Reimagined damage curve |
| Frozen Queen | Old Glacial Spike position; level 18 | 30+10L% defense, 10+5L to 20+10L cold attack damage; 50% cold-storm chance on attack; reserves mana |
| Masterful Form | Telekinesis position; level 6 | 10+2L% cast speed; 1+L to 25+10L lightning attack damage; 50% lightning-storm chance on attack; reserves mana |

L denotes effective skill level. Unspecified balance values above are provisional. All three forms are indefinite toggled reservations and can coexist while the mana budget permits. There is no separate life or mana drain. Costs follow the current Paladin tiers: Avatar floor((45+9L)*150/(100+efficiency)), Frozen Queen floor((60+12L)*150/(100+efficiency)), Masterful Form floor((30+6L)*150/(100+efficiency)); minimum 1 mana. Native buff cast function 18 enters the shared reservation hook; function 65 was the incorrect entry point in the original prototype.

Elemental storm children have their own damage types and private skill links. Their base damage is 12â€“24 at level 1, +6 per level, using native elemental mastery. Cold storm currently uses native icy bursts along the winding storm path; lightning uses a native electrical effect. These are initial visuals, not newly authored art. Frost Nova damage is 8â€“16 at level 1, +4 per level with native cold mastery.

Avatar's native restricted transformation prevents ranged casting. Existing Sorceress armor/buff skills remain allowed, including the two new buffs. Passive skills remain active. The Fire Golem model and buff-casting animations still require real-game verification.

Native life/mana steal uses whole percentages and physical attack damage; the requested half-point progression therefore rounds down at odd levels. Spell and elemental damage do not leech through these stats.

## Implementation and verification

- Generator: `src/workshop/sorceress_data.py`; runtime: `src/workshop/combat/sorceress.h`.
- Enemy skill references and old projectile damage links use private legacy copies; Uber Izual retains his original Frost Nova.
- New IDs are appended; existing item-stat save widths are unchanged.
- Form activation explicitly applies and renews a native state list and registers attack events once. Fire Golem appearance uses the native transform state plus graphics invalidation; disabling removes it.
- Conflagration uses native fire-ring projectiles and bounded ground patches. Its explicit burn hit uses ordinary Fire Bolt collision/type settings, not a special hit handler with collision disabled. Base damage is multiplied by ten; per-level growth remains +10.
- Five original generated icons are packed into normal/dim HD and low-end atlas frames; unrelated frames are verified unchanged. Source artwork and prompts are in combat/icons/sorceress/.
- Character-sheet form values are explicitly labeled added elemental damage; Frost Guardians display nova damage.
- Proc recursion is blocked. Conflagration's many native nova projectiles create one bounded ground zone; particle count does not depend on enemy count. Native missile hits retain resistance, ownership and death handling.
- Frost Guardians use a private friendly, ground-bound class with `neverCount=1`; their own pet type and cap do not consume the Paladin guardian pool.
- Tests cover unique tree positions, native references, proc damage chains, reservation renewal, repeated same-frame minion hits, probability, recursion and zone deduplication.
- Combat and appearance checks are deferred at the user's request. Compilation/startup tests do not establish combat correctness.


## 0.13.2 corrections

Player elemental attack stats are stored in whole damage points, as in native Enchant. The former extra factor of 256 is removed from all three forms. At level 20, Avatar/Frozen Queen add 110-220 damage before mastery; Masterful Form adds 21-225. Missile damage storage is separate and retains native fractional units.

Avatar now delegates initial transformation and reversal to native skill function 116, which manages the transformation removal callback and learned skill mode. Graphics state changes mark the state dirty on both activation and removal.

Storm procs create three native emitters directly instead of asking the item spell dispatcher to cast from a disguised player. Their lifetime/path range is 48 frames, doubled from 24, with matching skill-level and child-damage links. Recursion remains blocked.

Conflagration emits four expanding visual rings to the 24-subtile damage boundary and bounded, randomly distributed ground flames until the 75-frame zone expires. The native nova remains damaging but invisible; stationary flame geometry is rendered explicitly. Damage ticks remain at one, two and three seconds. There are at most four new random patches every five frames, independent of enemy count.

Native reference: [Enchant](https://github.com/ThePhrozenKeep/D2MOO/blob/master/source/D2Game/src/SKILLS/SkillSor.cpp), [transformation lifecycle](https://github.com/ThePhrozenKeep/D2MOO/blob/master/source/D2Game/src/SKILLS/SkillDruid.cpp), [Diablo storm](https://github.com/ThePhrozenKeep/D2MOO/blob/master/source/D2Game/src/SKILLS/SkillMonst.cpp). These explain the algorithm; supported D2R bindings are taken from the already verified skill function table.


## 0.13.3 visual and timing corrections

The visual implementation described under 0.13.2 is superseded. Storm procs use the native item-skill dispatcher again, restoring the earlier cold/lightning path. Fire also falls back to native server skill function 102 if that dispatcher rejects the cast. This preserves the native Diablo-wall missile initializer. Bounded diagnostic logs record dispatch outcomes; visible combat effects still require an in-game test.

Conflagration now gives its moving native nova visible fireball artwork. Four random ground patches per batch are emitted only during the first 20 frames, and each lasts until the same 75-frame zone deadline (normally 55-70 frames), rather than disappearing after 8-15 frames. Particle count remains independent of enemy count.

Burn ticks are scheduled at frames 25, 50 and 75. A five-frame expiry grace lets the polling loop deliver the final scheduled tick without creating a fourth tick. Regression tests cover all five polling offsets and require exactly three burn ticks for each.


## 0.13.4 Elemental Eruption and ground-flame replication

Lightning (public ID 49) is replaced by Elemental Eruption, a level-12 melee attack. The strike uses 10% weapon damage and +100% +10% per effective skill level attack rating, with a provisional fixed cost of 8 mana. It remains usable in Avatar of Fire. Old enemy Lightning references and original Lightning projectiles use a private legacy copy.

A successful strike creates a stationary, three-second tornado at the victim's position. Its radius is 6 subtiles (4 yards); pulses occur at 1, 2 and 3 seconds. Each pulse retains 10% of native weapon damage, plus 5-10 fire, cold and lightning damage per element at level 1, increasing both bounds by 5 per subsequent level. Elemental mastery applies to the added element. Native missile hits retain item on-striking metadata. A local dispatch guard prevents tornado hits from creating further tornadoes without suppressing ordinary native procs. A bounded pool supports up to 64 concurrent tornadoes. Runtime records store IDs and positions, never persistent unit pointers.

The character screen shows the initiating melee strike; the tooltip separately states each tornado element and timing. Native on-striking behavior, appearance, and animation must still be verified in combat.

Conflagration ground patches now explicitly synchronize their server-created missiles to clients, using the existing stationary-effect replication path, and use the native firewall HD graphic. The confirmed working moving ring and storm dispatch remain unchanged.


## 0.13.5 native client trails and Eruption attack events

Conflagration's moving ring now uses native client do-function 16 with the private ground-flame child. The client itself leaves flames along the moving projectiles. The ineffective server-side random patches have been removed; burn damage still uses the existing three zone ticks. This uses native path-step emission, not a JavaScript millisecond timer.

Eruption's visual now uses a private Tornado skill through the native item-skill dispatcher, rather than only synchronizing a stationary server-created missile. The private projectile has no collision or damage; explicit pulses remain authoritative. A bounded start log records the native visual dispatch result.

Eruption pulses now pass both native target and origin in a complete missile parameter structure. If the instant hit does not dispatch an owner's missile-attack event, each active elemental form receives one explicit attack-event attempt. A counter prevents this fallback from duplicating events already delivered natively. Item on-striking proc behavior still needs gameplay confirmation; it is not emulated by unconditional item-spell casts.

Radius is 1 yard (1.5 subtiles, squared-distance comparison without truncating the radius). Mana is 8 + effective skill level: table base 9 plus 1 per subsequent level. The three-second lifetime, pulse interval and damage are unchanged. Appearance and gameplay procs remain subject to user testing.


## 0.13.6 Storm amplification guard

All three forms now have a 10% Storm chance. Storm missile attack events are no longer eligible to create more Storms. Ordinary melee hits remain eligible. Each tornado pulse gets one explicit roll per active form, regardless of the number of victims; native events during the pulse cannot duplicate these rolls.

Only one tornado per player may be active, so rapid repeated attacks cannot stack their pulse rates. The radius remains one yard. Pulses are separated by at least 25 frames, with no multi-hit catch-up after delayed updates. Damage, three-second duration and 8+level mana are unchanged. Regression tests cover rapid repeated starts, delayed missile events, probability, polling phases and delayed-update pulse spacing. The exact reported crash cause has not been proven from a dump.


## 0.13.7 consecutive-hit empowerment

Successful real Elemental Eruption melee hits build a per-player three-step combo: 1/2/3 yards radius and x1/x2/x3 tornado pulse damage. Both the weapon portion and added elemental damage scale. Further hits refresh a 75-frame inactivity timer; at exactly three seconds without another qualifying hit, empowerment resets. Misses, other skills, tornado hits and Storms cannot build the combo. Death and session changes clear it.

Hits empower an already active tornado without spawning another or restarting its pulse timer. Each tornado still has its original three-second lifetime. The combo may carry into the next tornado if attacks continue. Pulse spacing, single-tornado limit and one 10% Storm roll per form per pulse remain unchanged. Tests cover the three stages, cap, exact expiry, new streak after expiry, unchanged pulse timing and triple damage components.


## 0.13.8 form balance

All three Storm proc chances are 25%. Avatar of Fire grants 25 + 3 per effective level percent maximum life and 10 + 1 per level percent to all five animation/movement speeds. Existing base bonuses are retained. Masterful Form cast speed is unchanged. At level 20 Avatar grants 85% maximum life and 30% all speeds. The tornado combo, pulse interval and anti-recursion guards are unchanged.

Frost Guardians is explicitly permitted in Avatar of Fire; ranged offensive spell restrictions remain unchanged.


## 0.13.9 Frost Guardian nova interval

Each Frost Guardian may trigger Frost Nova at most once every 25 frames (one second), on a successful melee hit. Cooldowns are independent per guardian and owner, claimed before dispatch to prevent repeated same-frame or recursive casts, and cleared on session/time reset. Expired entries can be reused; active cooldowns are never evicted. A failed native cast still consumes the interval. Tests cover same-frame repeated hits, two independent guardians, frame 24 rejection and frame 25 eligibility. Summoning in Avatar of Fire remains allowed.


## 0.13.10 form base damage and reservation

Only the three Sorceress forms have their reservation multiplier halved from 150 to 75, before final integer rounding and efficiency. Paladin auras and Feast of War are unchanged. Level-one added damage is doubled: Avatar/Frozen Queen 30-60, Masterful Form 4-70. Per-level growth remains +5/+10 and +1/+10 respectively. At level 20 the forms add 125-250 fire/cold or 23-260 lightning before mastery; combined reservation without efficiency is 505 mana (168+225+112).

Storms now deal 24-48 at level 1, +6 minimum and +12 maximum per subsequent level, before mastery/resistance (138-276 at level 20). This is a single-hit range, not total damage per proc. Storm chance remains 25%.


## Coming Storm presentation update

Elemental Eruption is now displayed as Coming Storm (stable public skill ID 49). A new original fire/cold/lightning tornado icon is packed into its normal and dim HD/low-end atlas frames. Internal skill identifiers and gameplay are unchanged. Artwork was generated with built-in image_gen; its exact prompt is recorded in combat/icons/sorceress/PROMPTS.json.


## 0.13.11 unified reservations, recasts and Conflagration

All three Sorceress forms now use Frozen Queen's reservation: floor((60+12L)*75/(100+efficiency)), minimum 1. At level 20 each reserves 225 mana without efficiency (675 combined).

Each successful Coming Storm strike recasts the visual, moves the sole damage zone to the latest victim and renews its three-second duration. It preserves the next pulse deadline, preventing both rapid-hit stacking and starvation from repeated attacks. Old cosmetic tornadoes may finish their native animation, but only the newest zone applies damage. The three-stage combo and per-pulse Storm restrictions remain intact.

Conflagration nova and burn now have 750-1200 level-one damage and +30 to both bounds per subsequent level (exactly three times the previous curve). Both receive +5% damage per hard point in Fire Wall and Blaze. Native elemental calculations include these synergies in the displayed range, as well as mastery. Tests cover relocation, one damage zone, preserved pulse deadline, renewed expiry, matched reservations and both damage curves/synergies.

The Battle Cry Keystone now grants +1 Battle Orders and +1 Battle Command to all classes instead of +5 Shout. Existing Sigils require Apply to rebuild their payload.

The three forms grant each other +5% added elemental and Storm damage per hard point. These additive synergies do not affect reservation, life, speed or defense.

## 0.13.12

Synergies use the lower tooltip section. Form level-one damage is divided by three (rounded down), with unchanged growth and Storm damage. Frost Guardians: 200+100L life bonus, 1+floor(L/10) summons, nova 8+8L to 16+16L, melee half that before mastery. Ice Blast and Frozen Orb each grant 8% damage per hard point; their invested levels are copied to the pet for native nova calculation. One nova per guardian per second remains.

## 0.13.13

Retired/dead Frost Guardians use the verified native pet detach, client delete queue and world removal path, after collecting unit IDs outside iteration. Enchant restores 5+level life and mana on direct melee attack events, capped at current maxima (including reservation); missile/proc damage cannot restore resources.
