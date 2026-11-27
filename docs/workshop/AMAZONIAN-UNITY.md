# Amazonian Unity

Level-30 bow/crossbow skill below Strafe, prerequisite Strafe. Six untargetable, invulnerable Amazon actors stand in a hexagon around the player at cast time for five seconds. Each acquires its nearest living hostile within twenty yards before firing. They fire in ring order 1–6, one arrow every 2–3 frames at 25 Hz, ten arrows per actor over five seconds. Cooldown is ten seconds from a successful cast.

Base balance: 25 mana; each arrow carries 10% weapon damage at level 1, plus one percentage point per additional skill level. Payload is snapshotted from the first arrow. Actors cannot walk, block, receive attacks, drop items or grant XP. Failed placement rolls back the complete formation without spending mana/cooldown. Death, disconnect and area changes remove the actors. Dedicated spectral-archer icon uses normal/dim frames 66/67 in all Amazon atlases. Tooltip states timing, total arrows, mana and per-level damage. Character sheet identifies damage per arrow and uses native weapon-damage display function 7 with clc4-100 percent.

Validation: combat DLL build and existing native suite; Unity runtime regression for timing, fixed aim, damage, cooldown, spawn rollback and cleanup; full Workshop data checks, including private monster flags and stable bindings. In-game visual/AI verification remains outstanding.

## In-game report corrections (2026-11-24)

- Description has explicit line breaks; every Unity English tooltip line is at most 40 characters.
- Bow and armor occupy separate `monequip` rows. The native equipment routine randomly selects one alternative per row, so the previous combined row produced bowless actors. Reference: https://github.com/ThePhrozenKeep/D2MOO/blob/master/source/D2Game/src/SKILLS/SkillAss.cpp (`SKILLS_EquipMonsterFromTable`).
- Each client volley now receives its specific spirit as the origin unit. Player remains damage owner; server absolute coordinates and fixed center are unchanged. The captured current-build publisher at RVA 0x4640f0 reads argument four as a unit at 0x464389 and serializes its identity when coordinates match. Regression checks distinct origin units for all six arrows in each of ten volleys.
- Private arrows no longer roll attack rating (`ToHit=0`); the character sheet therefore omits attack rating. `NextHit=0`/`NextDelay=0` are verified, so arrows are not suppressed by a shared next-hit state. This does not bypass walls, blocking or immunity and is not proof that every arrow hits a moving target.
- Full data checks and native regression suite are required. Screenshots established the original defects; corrected visuals and actual target damage still require an in-game check.


## Projectile-origin follow-up

The user confirmed bows but reported unchanged player-origin arrows. Passing the spirit only as publisher argument four was insufficient. `AmazonUnity::Volley` now publishes each arrow with the spirit as argument one (packet owner) AND argument four (optional origin), while missile creation retains the player as server owner. Absolute start coordinates come from the saved center plus the hexagon offset, independent of animation/path state.

Captured native evidence: publisher RVA 0x4640f0 copies argument one to RDI; 0x4641cb–0x4641f4 serializes its type/ID into the packet header. 0x48fde0 only returns a private client for type 0 (player); monsters return null and therefore use room-client delivery, including the player's client. No new native address binding is introduced.

Regression asserts distinct visual owners for all six arrows, separately from the shared player damage owner, over all ten volleys. First-volley logging records requested and actual server spawn positions, center, missile ID and both owners. Native tests alone do not establish client rendering; live confirmation is still required.


## Physical scaling and splash

Each arrow gains a 0.5–1 base flat physical bonus, with the Strafe level increments. Multiple Shot, Split Arrow (internal Guided Arrow), and Strafe each grant 8% physical damage per hard point. Native pnma/pxma supply the synergy-adjusted flat amount; the runtime preserves that complete flat addition while scaling the weapon component. Missile physical stats are in 8-bit fixed point. The tooltip lists flat damage, all three synergies, and 50% splash within four yards. Splash excludes the primary victim, requires a successful impact, and cannot recursively splash.

## Blood Moor training target

One private zombie-model Target Dummy per game is placed on a valid Blood Moor tile near the player's first transition from Rogue Encampment. It has idle AI, no movement/attack/hit-recovery/knockback animation, no XP or loot, zero resistances, and exactly 1,000,000 HP regardless of difficulty/player count. Every 250 frames it fully heals; if killed, it respawns at its fixed anchor at the next reset. New-game state is cleared and revisiting does not create duplicates. Placement, exact health, cadence, death respawn and duplicate protection have native mocked-runtime coverage. Live placement/rendering remains to be tested in D2R.


Latest balance: the complete flat physical curve is halved through HitShift 7 (base 0.5–1, all level increments halved). Runtime correction uses fixed-point calc1/calc2 so half points remain real damage; native integer tooltips can round them down. Weapon damage remains 10% + one percentage point per additional level, and synergies remain 8% each.

The formation stays where it was summoned when the player moves. Targets are reacquired independently before each shot; dead/friendly/other-area enemies are excluded. No target consumes the scheduled shot opportunity without firing; at most 60 arrows are released.

Placement reliability: each ideal hexagon vertex searches up to six nearby subtiles and accepts the native free-position result. Slots remain distinct; the formation stores actual positions for arrow origins. Fully blocked placement still rolls back without mana or runtime cooldown.

Self-cast targeting uses LineOfSight 0: the old mouse/cursor-target line-of-sight gate no longer rejects summons around the player.
