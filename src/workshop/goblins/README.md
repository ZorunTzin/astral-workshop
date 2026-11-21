# Workshop treasure goblins

Seven encounter families (nine Frankenstein monster definitions including split children) and their HD assets are imported from the locally installed FMMP pack. The DLL is not imported: Workshop owns the runtime and uses the existing guarded game-thread spawn, monster-mode and delete bindings. Visual asset hashes and archive provenance are recorded in UPSTREAM.json; monster-definition provenance is in definitions.json. Upstream redistribution permission has not been established by this local integration; this change has not been published.

## Encounters

3% once per ordinary Act I–V area per game, excluding towns and the Cow Level. Trials (166–170) roll 1% once per fighting wave, at most one successful spawn per trial per game. Rolls are shared across players and consumed even when placement fails. Special endgame areas, towns, followers and the Goblin Hoard are excluded. Goblins are not required to clear a trial wave. Their escape timer pauses while the trial modifier cards are open.

## Fleeing

Run away from the nearest living player in the area. After eight seconds, stop for a three-second escape channel. Damage interrupts the first three channels and delays the next attempt by five seconds. The fourth channel can finish through damage; lethal damage still wins. The channel displays a private, stationary, non-damaging gold portal effect using the Frankenstein particles. Interruption, death, trial pause and escape remove that effect. Escape first publishes a room removal to clients and then removes the server monster without kill loot. Normal monster death uses native treasure classes exactly once; unloaded monsters are not treated as dead.

## Rewards

Treasure Goblin: equipment/jewelry. Gem Hoarder: gems scaled by difficulty. Gilded Baron: gold piles. Insufferable Miscreant: runes with difficulty-limited tiers. Odious Collector: four rolls, each choosing Workshop lab materials (tiers 1/2/3) with weight 3 or a Trial Orb with weight 1. The dedicated zero-NoDrop Trial Orb subtable picks one of all seven orbs (Woa-Wog) with equal weight; it does not use the low-chance world orb table. Malevolent Tormentor: equipment with a stronger jewelry weighting. Gear variants use improved unique/set/rare chances; no guaranteed unique. Loot balance is Workshop-specific. Gelatinous Sire: confirmed death creates two medium goblins, each of which creates two small goblins on death. Only the four terminal goblins drop loot, two equipment/jewelry rolls each. Intermediate stages give no loot or XP. Children have 60% / 35% of the source base life factors, use the same fleeing rules, and never roll as independent encounters. Missing units and escape do not split. Corpse-removed lifecycle events are handled once; blocked placement is retried while a player is nearby in the same area. Pending children clear on game change. Followers and enterable portals are not included.

## Validation

Run data.py against the staging root, then verify.py. The combat build runs engine_test.cpp and runtime_test.cpp. In-game model rendering, pathing and drop balance require manual playtesting. No direct game tests are automated here.

## Local test mode

Create d2rloader/config/workshop-goblins-test.txt beside the installed mod with the single value 1. In a new game, entering Blood Moor spawns the seven families in order, one encounter at a time near the player. The next appears after the previous encounter (including all gelatinous children) dies or escapes. Blocked positions retry every two seconds without advancing the family. Each family appears once per game. This local file is not part of the release payload. Set it to 0 or remove it, then create a new game to restore ordinary rarity. Tests use the same combat stats and loot as natural encounters.

Targeting fix: flee using native WL mode at Velocity 11 (HD assets select their Run animation at that speed). The Fallen base has no legacy RN mode; do not advertise mRN. All nine classes enqueue death events for portal cleanup, including early corpse removal. Missile ID zero is valid. MapSense special icon classifier is hash/byte guarded and restricted to current Workshop goblin IDs; Fallen and other monsters fall back to their normal markers.

2026-11-21 follow-up: Goblin render scale is 1.5 times each original root transform, preserving splitter proportions. Six user-selected Zedge clips (AUDIO-SOURCES.json) replace the initial ESR taunt pair; native sounds/monsounds routing uses randomized six-entry SD/HD groups with reduced volume and sparse movement triggers. No external audio playback process. Escape now uses a private timed monster state/overlay instead of a missile: the normal room-removal packet excludes missiles. Cancellation expires the state on the next game frame; death/removal destroys its attached overlay. Original missile row remains inert for compatibility but is no longer spawned. Manual verification is still required for rendering and sound cadence.

Goblin visibility: private, subdued native set-glow variants match each model palette; Sire and both child stages are blue. The visual state refreshes while alive and expires on death; attached effects are destroyed on removal. Original set glow assets remain unchanged. Visual intensity requires ingame review.

Glow compatibility: every custom overlay includes its matching legacy DCC in addition to the HD definition. Successful initial visual-state application explicitly marks client graphics dirty and refreshes the unit; ordinary lifetime renewal does not restart the effect.

Glow revision 3: HD monster definitions now own the colored VFX directly, using the native monster VfxDefinitionComponent pattern. Runtime glow-state application has been removed to prevent duplicate effects. The effect is hard-killed when its owning entity is destroyed. Live visibility and corpse behavior still require ingame verification.

Blood Moor test mode disabled locally (workshop-goblins-test.txt = 0). Natural rolls remain 3% once per eligible act area and 1% per Trial wave, at most one natural encounter per Trial. The legendary loot-goblin wave is a separate one-wave event.
