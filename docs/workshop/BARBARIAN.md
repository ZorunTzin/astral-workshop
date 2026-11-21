# Barbarian prototype — 0.12.2

The overhaul includes the complete requested proposal, Rage, Feast of War and Quake. The original 30-entry Barbarian class/save order is preserved. Call of the Ancients repurposes removed Double Swing ID 133; Double Throw ID 140 is reused for Shower of Rocks at the former Frenzy position. Frenzy and Berserk retain their original IDs. An existing Barbarian should respec to clear the two removed double-attack allocations before testing the layout.

| Skill | Behavior |
| --- | --- |
| Vigor of the Ancients | Replaces Increased Speed; `10 + 2 × level` percent movement, attack, casting, hit recovery and block speed. |
| Iron Skin | Original armor bonus plus one percentage point physical damage reduction per level. |
| Natural Resistance | Original resistance curve plus `floor(level / 5)` percentage points to all four elemental/poison resistances, and the same additional maximum-resistance bonus. Maximum resistance therefore increases by one percentage point at levels 5, 10, 15, and so on. |
| Frenzy | Double Swing's position, required level 1. Former Double Swing/Double Throw synergies are included at their base hard-point cap. Explicit synergy requirements/tooltip lines are removed. |
| Berserk | Double Throw's position, required level 12; requires Frenzy. |
| Rage | Replaces Shield Mastery; no shield requirement. Adds `(2 + 0.2 × level)` percent more hit damage for every whole percent of missing enemy life. Kills grant `20 + level` percent attack speed for 10 seconds. Native melee-splash radius grows by five percent per level, without adding a duplicate splash hit. |
| Melee Mastery | Original bonuses retained. `min(100, 10 + 2 × level)` percent chance to cause physical Bleeding. Strongest bleed: 70% of the applying physical hit per second for five seconds, triple while the enemy moves. |
| Throwing Mastery | Original bonuses retained. Same Impale chance. Each independent application stores 10% of the pre-resistance physical hit for five subsequent hits, with an eight-second timeout. Five normal stacks; previous Impales trigger before applying a new one and are excluded from its snapshot. Allies and damaging spells can consume existing stacks. |
| Quake | Replaces Taunt. `min(64, 4 + level)` physical shock-wave projectiles with 100% weapon damage and 20–30 added damage at level 1, growing by 5–7 per level. Eight mana. Both mastery effects can apply if learned. Travel distance grows with level; native next-hit delay limits overlapping projectiles. Native arrow-style hit checks and Pierce are enabled: projectiles stop on a target unless its inherited Pierce roll succeeds. Central projectiles retain native item-modifier events. The tooltip includes projectile count and the actual mana cost. |
| Feast of War | Replaces Find Potion. Consumes up to five corpses within eight displayed yards. Each heals `3 + floor(level / 5)` percent maximum life. Grants `20 + 3 × level` percent physical weapon damage for `8 + floor(level / 5)` seconds, plus two seconds for each extra corpse consumed by that pulse; refreshes without stacking its strength. Toggle with `15 + 3 × effective skill level` mana reserved, reduced by reservation efficiency. Automatically consumes corpses every two seconds; activation works without nearby corpses. The Paladin reservation surcharge does not apply. Uses native corpse validation and consumption. |
| Battle Cry | Original weakening retained. Susceptible enemies are Taunted for one second at +200% movement speed, drawing them toward the Barbarian along valid paths. The pursuit state coexists with Battle Cry. AI-immune foes can resist it. |
| Call of the Ancients | Berserk's old position, required level 30. Talic, Madawc and Korlic for `30 + level` seconds, 60-second cooldown. Fixed group of three. Base life `800 + 120 × level`; base damage `80 + 20 × level` to `120 + 25 × level`, before passive/minion bonuses. |
| Whirlwind | Hold the skill button and steer with the cursor; release to end. Uses the locally installed ESR CelestialRayone steering plugin, unchanged and with its native byte guards. Bow Whirlwind, the dual-weapon same-target damage buff and the global item-proc change are disabled in this port. |

Rage and Feast have timed buff-HUD entries. Tooltips show the new calculated values. Original skill names may remain in old item strings because those skill IDs are intentionally stable.

## Ancients

The three summons use private monster records with native Ancient appearances. The summit's real bosses and quest behavior are unchanged. Their original AI cannot be used directly: it hardcodes boss IDs and checks the summit quest. Friendly pet AI handles movement; a bounded dispatcher invokes private native Talic Whirlwind, Madawc axe and Korlic Leap Attack skills. Explicit private-monster HD mappings select the existing Ancient models. Private animation records disable selection and attack targeting; the private monsters are unkillable and the runtime damage guard rejects damage against them. Their Hydra-initialized AI parameters are reset using a pinned native setter. Expiration calls the native pet manager used by Unsummon directly, with game, owner and pet ID. The native caller and target are guarded against the current executable. This avoids the selected-skill target parameter required by triggered Unsummon; no unit is freed by hand.

Melee mastery affects Talic/Korlic, throwing mastery affects Madawc, and Iron Skin, Natural Resistance, Vigor and Rage transfer. Madawc also inherits throwing pierce. Deadly Treasures continues through normal owner loot credit; NPC ammunition conservation does not apply to the unlimited axe projectile. Passive/minion base bonuses are captured on summon; resummon after substantial passive changes.

The native three-consecutive-monster creation pattern is documented in [D2MOO's Hydra handler](https://github.com/ThePhrozenKeep/D2MOO/blob/master/source/D2Game/src/SKILLS/SkillSor.cpp); current-build layouts/call sites are additionally validated against local native code. Native corpse consumption follows [the Redemption handler](https://github.com/ThePhrozenKeep/D2MOO/blob/master/source/D2Game/src/SKILLS/SkillPal.cpp).

## Prototype boundaries and validation

Bleeding and Impale are runtime mechanics derived from mastery levels. They do not introduce saved item-stat IDs: the Workshop's nine-bit saved-stat namespace is full. Independent Bleeding/Impale affixes, extended Impale stack counts and extra-hit modifiers are outside this prototype. Physical effects use D2's native hit/resistance/death handling, so attack/leech interactions follow the D2 implementation rather than being a complete PoE combat-system replacement. Bleed ticks are single-target native physical hits, at most once per second per target; they cannot recursively trigger these mechanics. Target tables are bounded and use session resets and unit IDs instead of retained pointers.

Automated tests cover Rage scaling and its kill buff, strongest-bleed selection and movement multiplier, one-shot tick cleanup, Impale order/non-recursion/expiry, Quake mastery effects, native corpse consumption and healing caps, normal-warcry forwarding, and Ancient duration/combat dispatch. Generated-table checks validate IDs, prerequisites, positions, passives, private monster/skill/state links, unchanged saved-stat widths, tooltips and steering configuration. In-game animation, pathing, packet visibility and practical balance still need player combat testing.

## 0.12.1 corrections

Barbarian and internal localization IDs are now allocated across every string JSON file, avoiding item-name collisions. Rage replaces Carnage's display name without changing its saved ID. Quake binds both native weapon variants, removes an inherited client startup requiring the removed bone-cast projectile, uses the seven-frame Shock Wave animation and registers its HD missile mapping. Feast uses the shared reservation-toggle handler; its active marker and timed damage bonus have separate states. Native Battle Cry no longer passes through the retired Feast shout hook.

Shower of Rocks is a local prototype at required level 24, in the original Frenzy position. It is a native melee attack without cooldown, using Berserk's attack-rating formula. A successful hit triggers `5 + floor(level / 5)` falling rocks, each with 20% weapon damage, over the struck target's area. The primary swing uses ordinary weapon damage and costs 20 mana. Physical and elemental damage components on each native weapon snapshot are divided by five; elemental durations and proc metadata are retained. This avoids the 1/128 rounding inherent in the data-only source-damage byte. Native Meteor client motion supplies the falling Armageddon rock visual; its empty server child-missile field prevents ground fire. Rock impacts are marked separately so they cannot recursively create more rocks. This is a Workshop approximation, not a port of Median XL's code or assets.

## 0.12.2 validation boundaries

The Rage attack-speed state now has an explicit source-skill aura link, ten-second duration and primary attack-speed stat for the timed HUD catalog. Both Quake and Battle Cry expose their actual mana costs. Tests cover counted rock emission only after successful melee hits, blocked/impact recursion rejection, one-fifth damage components with unchanged durations, native pet-manager expiry, private Ancient damage rejection, all maximum-resistance passives, HD registry links and native Pierce fields. Visibility, practical Pierce/proc behavior and Ancient movement still require the user's in-game check.

## 0.12.3 combat corrections

Natural Resistance maximum resistance starts at one on skill level 1 and increases every five further levels: `1 + floor((level - 1) / 5)`. Call of the Ancients no longer displays life. Talic/Korlic use real native monster sequence modes; Madawc uses a normal casting animation to launch two axes. A bounded stalled-sequence recovery prevents a pet remaining locked indefinitely. Expiration detaches the pet without killing, queues its world deletion and calls native world removal exactly once. It does not repeat the ineffective death animation of an immortal monster.

Rage uses a private buff-source skill and an owner timer lasting exactly 250 frames, refreshed on a kill. Unexpected early state loss is repaired only for the remaining time. Shower of Rocks separates visible overhead rock debris from delayed native weapon-damage hits. Each bounded queued impact applies the complete native missile hit within its small radius; no enemy life is written directly. Successful-result bit 2 is accepted rather than mistaken for a blocked hit. Block and evade guards and impact recursion guards remain.

## 0.12.4 targeted native fixes

The in-game inspection exposed that native mode initialization clears the used-skill pointer. Ancient skill nodes are now bound after initialization; Madawc uses native Cast mode 7, and monster sequence tokens remain `xx`, not the player-only `SQ` token. Final despawn queues the actual room deletion packet through the same native call site as corpse consumption before detaching and removing the server unit. The earlier similarly located routine was not that packet queue, leaving client ghosts.

Shower of Rocks now triggers the original working Blizzard-shaped action on a successful melee hit. A hidden skill and eight private native missiles preserve Blizzard scheduling, collisions and client creation. The controller lifetime follows the requested rock count, and new physical/elemental weapon snapshots are scaled to 20% on creation. The HD asset is derived from the real Armageddon rock prefab extracted read-only from the local installation, with its declared native particle/model dependencies. The manual impact queue is retired.

The HUD log distinguished a one-second Shock Wave proc (skill 243) from the actual ten-second Rage state. Rage now attributes its timed list directly to public skill 128, giving the correct Barbarian name and icon instead of an internal helper's presentation.

## 0.12.5 follow-up corrections

Madawc uses a private copy of original Double Throw server function 74 and actual normal-quality throwing axes in both hands. A private eight-frame monster sequence contains two attack events. Talic receives a real melee weapon and the native animated-path policy used by the original Ancient AI. The public removed Double Throw slot remains Shower of Rocks; NPC logic no longer depends on that slot.

Normal hit-event completion precedes HP subtraction. Rage now records recent damaged targets and confirms their death on a subsequent server tick before awarding its ten-second buff. Rain child collision callbacks carry a scoped proc context, preventing on-striking recursion while the owner retains the melee skill. Actual falling children and Madawc projectiles are synchronized explicitly using native missile packets.


## 0.12.6 hit and animation corrections

Rage now multiplies the resolved damage in the pre-HP-subtraction callback, alongside Attract. The earlier hit-reaction callback can receive a copied packet and cannot reliably alter applied damage. The hit-reaction path remains responsible for deferred kill attribution. The native caller at RVA 0x44d038 reads the final total at +0x134 and subtracts it from HP after the resolved callback returns.

Triggered Rock Rain uses ItemEffect/ItemCltEffect **28**, not 1. These fields are handler indices, not Boolean flags: handler 1 returned success without invoking the Blizzard creation action. Child creation logs report actual missile IDs and weapon snapshots. The original Chasm Break missile chain in Reimagined provides a second local reference for native falling rocks; its origin is credited to ESR/Median in [Reimagined's release notes](https://github.com/D2R-Reimagined/wiki-content/blob/main/Patch_Notes/2-1-0.md).

Read-only in-game inspection identified the shared GenericPet callback at RVA 0x5d2f70. Only the private Ancients defer this callback while a skill sequence is active, retaining native animation and movement rather than accepting new pet-follow commands mid-leap. Talic prepares the native path step count and uses a private copy of skill 151, the original Whirlwind referenced by the enemy Talic, with its synergy terms cleared. Their base walk/run speeds return to the original Ancient values. Duration and native room removal remain unchanged. Native byte guards reject mismatched executable builds; ordinary pets continue through the original callback.

Automated checks cover resolved Rage scaling, unchanged hit-reaction packets, private-only AI deferral, original-AI forwarding, path preparation, native handler indices and existing despawn/non-recursion tests. Combat appearance still requires in-game confirmation.

Spawn-time equipment now explicitly sets `oninit=1`. Madawc receives both throwing axes in one native equipment bundle; Talic receives his melee sword. The previous entries omitted spawn initialization and split the two axes into separate rules.


## 0.12.7 Rage balance and direct original Whirlwind

Rage grants `2 + 0.2 × skill level` percent more damage for every whole percent of missing enemy life. Runtime scaling retains tenths of a percentage point, and the tooltip uses the same ratio. At level 1 against a half-life target the hit multiplier is 2.10; at level 20 it is 4.00. The ten-second attack-speed buff is unchanged.

Talic now learns and invokes actual skill ID 151, exactly the Whirlwind ID referenced by the enemy Ancient, instead of a private copied skill. The old private record remains allocated to keep subsequent numeric bindings stable. His successful animation/hits still require an in-game test. Shower of Rocks is left unchanged while replacement ideas are reviewed.


## 0.12.8 Ground Stomp replaces Shower of Rocks and Talic Whirlwind

Ground Stomp occupies the same saved skill slot 140. One cast produces five separate pulses at 0, 5, 10, 15 and 20 server frames (0.8 seconds total). Each pulse deals approximately 10% weapon damage plus 5–8 physical damage at level 1, growing by 2–3 per level. Native SrcDamage uses 13/128 for the weapon fraction. The initial radius is ten displayed yards, growing gently with level to a thirty-yard cap. Native War Cry/Shock Wave stun handling uses `min(25, 10 + floor(level / 5))` frames. Boss and champion stun eligibility follows the engine. Mana cost is 12; no skill cooldown is added.

The visible native War Cry ring has no collision or damage. Each pulse enumerates nearby hostile monsters in the caster's room once and submits exactly one complete native missile hit per target, independent of ring overlap. This preserves physical resistance, weapon modifiers, native hit/proc handling and kill credit. Global proc suppression is deliberately not enabled. A dedicated emission guard prevents recursively starting another Ground Stomp from its own triggered effects; overlapping manual casts retain separate five-pulse bursts. Records store unit IDs and cancel after caster death or session changes.

Talic uses the same Ground Stomp scheduler at the Call of the Ancients skill level, with a two-second action interval while enemies are in range. His native A1 animation provides the gesture; damage no longer depends on his broken Whirlwind/melee sequence. Madawc, Korlic, timed despawn, Rage and the original player Whirlwind are unchanged. Retired private skill/missile IDs remain allocated so subsequent bindings do not shift.

Tests cover five hits rather than five damage packets in one frame, duplicate tick calls, exact target/radius bounds, caster-death cancellation, self-proc rejection, visual-only rings and Talic sharing the scheduler. Actual native proc rates and the appearance of the replacement still need player combat testing.


## 0.12.9 On-striking Stomp, level scaling and Barbarian artwork

Ground Stomp is a normal melee attack again. A successful primary hit starts `5 + floor(skill level / 10)` pulses, spaced five server frames apart. Each pulse deals `(10 + skill level)%` weapon damage with no added flat damage. The native full weapon snapshot is scaled at creation without scaling elemental durations, leech percentages or proc metadata. Tooltips show count, weapon percentage, radius, mana and stun duration, with no flat minimum/maximum lines.

Each pulse explicitly creates sixteen collision-free native War Cry shockwave visuals, mapped to the existing `warcry_shockwave` prefab, instead of calling a private unlearned visual skill through the shout handler. Native damage still applies once per eligible target and pulse. Talic's shared Stomp scheduler now runs independently of busy sequence modes; his A1 gesture is attempted only when safe. Dedicated Talic scheduling logs distinguish activation from ordinary melee damage.

Six painted icons cover Rage, Feast of War, Quake, Call of the Ancients, Vigor of the Ancients and Ground Stomp. The original Barbarian atlas is the immutable input. Only these six normal/dim pairs change in both HD and low-end files; the packer checks that all other frames are unchanged. Artwork, generation subjects and prompts are stored in `src/workshop/combat/icons/barbarian/`.


## 0.12.10 Ground Stomp radius

Halved the effective radius for both the player and Talic, including the visible War Cry wave lifetime and tooltip. Half-subtile precision avoids rounding an odd original radius down twice. Pulse count, weapon damage, stun and on-striking behavior are unchanged.


## 0.12.11 Hell summon crash: isolate private monster families

Two user crash reports on 2026-11-11 identify the same access violation at native RVA `0x4951EA`: a null item record is dereferenced at `+0x8c` while initializing Talic, before Workshop combat tracking begins. The call chain runs through Hydra summoning (`0x554343`), monster creation, its special initialization dispatcher (`0x494F45`) and the original Ancient equipment initializer (`0x495110`). That initializer indexes a fixed equipment table using the monster class minus the original Ancient base ID; appended private class 855 does not belong to the supported original class range 540–542. Nightmare/Hell then dereference the resulting invalid item record for its difficulty-specific upgrade.

Each private Ancient now has its own private BaseId, no NextInClass link into the boss family, and no inherited boss MonProp. This bypasses the boss-only initialization path. Original Ancient records remain untouched. Explicit oninit monequip still supplies Talic's sword and Madawc's dual axes, and the existing private MonStatsEx/HD registry mappings retain their models. Stats, learned skills, lifetime and minion passive transfer are unchanged.

Regression checks validate private BaseId isolation, absent boss family links/properties and explicit weapons in both expansion/base table sets. Startup testing cannot substitute for a player test of Call of the Ancients on Hell.


## 0.12.12 Allied aura reception

A read-only live capture found level-4 Might on the player (+35% damage,
4% Crushing Blow), but no Might state on any of the three nearby Ancients.
After each native friendly basic-aura pulse, a narrow bridge refreshes the
same timed aura state on the caster's own private Ancients. It copies only the
six computed aura stats, including Aura Mastery, never aggregate item stats.
Native skill radius is evaluated at the effective aura level. The fallback is
limited to the same room; ordinary native aura propagation is unchanged.
Leaving range, switching off the aura or losing the caster stops renewal;
normal aura expiry removes the bonus. Existing aura states never stack.
Hostile auras, passive replacements and unrelated pets are excluded.


## 0.12.13 Quake, Ancient attacks and Ground Stomp

Quake uses War Cry's existing six-warcry hard-point damage synergy at 15% per
point, with matching synergy text. Talic now learns a private copy of the
Reimagined Carnage attack (Concentrate), and Korlic a private Tectonic Slam
(Stun). Both use real single-event monster attack sequences; Madawc keeps
his original Double Throw. Private skill IDs, summon lifetime and Hell-safe
monster families remain unchanged. Talic no longer emits automatic Stomps.

Ground Stomp remains a successful melee-hit trigger with War Cry visuals and
5 + floor(skill level / 10) pulses. It has a five-second native cooldown and
an additional server guard against repeated proc starts. Radius is fixed at
20 subtiles (13 1/3 displayed yards). Each pulse deals (250 + 25 * level)% of
weapon damage: +150% base bonus, +25 percentage points per level, on top of
100% normal damage. Melee Mastery and Throwing Mastery each increase this
pulse damage by 5% per hard point, added together before multiplication.
The initiating ordinary melee hit, stun and on-striking behavior are retained.


## 0.12.14 Quake weapon scaling and aura boundaries

Quake now deals constant 5–7.5 added physical damage (one quarter of its old
20–30 base) plus 50% weapon damage at level 1, increasing by 5 percentage
points per additional level. The same six hard-point sources as War Cry give
5% increased weapon portion per point. The flat portion does not scale with
levels or synergies. A scoped post-creation handler scales the full native
weapon snapshot, then adds the constant physical damage, leaving cold/poison
durations and proc metadata unchanged. Native flat skill damage is zero to
avoid applying that component twice. Tooltip calculations match these rules.

Ground Stomp now deals (50 + 10 * skill level)% weapon damage per pulse,
before its two 5%-per-hard-point masteries. It inherits Frenzy's attack rating
and shows the result. Its five-second cooldown and radius 20 remain unchanged.
Call of the Ancients gains 5% increased weapon damage per hard point in both
Carnage (Concentrate) and Tectonic Slam (Stun), applied on summon and listed
in the tooltip. Resummon after changing these skills.

A further live capture found no Might state on the Ancients. The aura fallback
now uses D2RCore's supported GetLevelIdFromUnit export to allow adjacent
internal rooms within one area, still checking native aura radius. It also
explicitly enables the client's aura state and records a bounded set of
successful aura transfers for the next real-game check. Combat/visual results
must still be confirmed in game; synthetic tests verify room boundaries,
range, expiry and no stacking, but cannot prove the rendered effect.


## 0.12.15 Native formula offsets and aura diagnostics

The native compiled skill row places calc4 at +0x19c and calc5 at +0x1a0.
The Barbarian helper and Quake runtime reader used earlier columns, preventing
runtime synergy/auxiliary formulas from matching their tooltip calculations.
Both readers now use the verified offsets, with sentinel regression checks.
Might diagnostics additionally report the source state, evaluated radius,
area/ownership/distance of Ancient targets and native allocation failures.
Reports are throttled to once per five seconds and capped per process; no
runtime debug pointer writes or gameplay-input automation is used.


## 0.12.16 Positive buffs on untargetable Ancients

Live 0.12.15 diagnostics confirmed valid owner, living target, matching area
and radius 22, followed by native aura allocation failure for all three pets.
The nearby mercenary received Might normally. Read-only native inspection
identified the rejecting branch in ApplyWideTimedStatEffect: the target's
MonStats2 flags byte +5 must contain bit 0x02 (isAtt). The private summons
intentionally clear that flag, which also prevented their auxiliary passive
buff from being allocated. The field meaning agrees with the primary
[D2MOO monster-table definitions](https://github.com/ThePhrozenKeep/D2MOO/blob/master/source/D2Common/include/DataTbls/MonsterTbls.h).

Only the synchronous positive-buff allocation for classes 855–857 temporarily
sets this bit in their private compiled extra record. An RAII scope restores
the exact original byte on success or allocation failure. No permanent table
change, isSel change, targeting-flag change or shared vanilla class change is
made. The native isAtt eligibility check is passed without reconstructing the
native stat-list lifetime, removal callbacks or client replication. Damage
immunity remains continuously active. Tests check exact flag restoration and
unchanged neighboring selection flags; the native getter is signature-guarded.


Real-game validation after installing 0.12.16: the player confirmed the aura
is now visible. Native transfer logs independently recorded Might level 4 on
Talic, Madawc and Korlic (pet IDs 56/57/58), each with +35% damage and 4%
Crushing Blow at radius 22. Repeated pulses retained 35/4 rather than stacking.


## 0.12.18: Quake and summon quest counting

Quake uses one third of its previous weapon fraction: `(50 + 5*(level-1))/3` percent. The runtime divides after multiplication, retaining fractional weapon damage. Its separate physical addition is `1+level` minimum and `2+level` maximum, both increased by the same 5% per hard point in its existing War Cry synergy sources. Attack rating is `100+10*level` percent. Mana is `4+0.5*level` (native half-unit cost, including level one). The character sheet still labels its weapon-only component explicitly.

Call of the Ancients requires Carnage (internal skill name `Concentrate`). Its three private MonStats rows now have `neverCount=1`, like ordinary summoned skeletons/golems, to prevent inclusion in monster-clear quests. A previously polluted Den of Evil instance needs a new game; no quest or save flags are edited.
