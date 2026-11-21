# Monster modifiers â€” 0.16.8

Champion variants remain unchanged. Random unique affix counts: Normal 1–2, Nightmare 2–4, Hell 3–5. Named superuniques retain fixed affixes and receive extra rolls to reach the selected count; a fixed set can exceed the normal range. No new combination exclusions are added. Every distinct public modifier adds +10% damage and +50% life, additively (three modifiers: +30% damage, +150% life). Raid Boss expands to all 23 distinct modifiers: +230% damage and +1150% life. Hidden native initialization flags do not count. Life is scaled once without healing wounded monsters. Custom affixes consume normal slots and cannot repeat. Raid Boss expands to every effect once, including modifiers already rolled.

| Modifier | Behavior |
|---|---|
| Extra Strong / Cursed / Mana Burn / Aura Enchanted | Original behavior. |
| Extra Fast | +200% movement and attack speed replace the former Extra Fast speed bonuses, including inherited Extra Fast on minions. |
| Magically Gifted | Half the original elemental resistance contribution. Random Fire Ball, Lightning or Ice Bolt every second, damage 30 + 8 Ã— monster level. |
| Fire Enchanted | Original fire damage/resistance. Direct successful hits leave a 3-second burning field. Death leaves a fire orb for 3 seconds, then a 6-yard explosion dealing 100 + 30 Ã— monster level fire damage. |
| Lightning Enchanted | Original effects plus Chain Lightning every second, damage 15 + 4 Ã— monster level. |
| Cold Enchanted | Original effects plus Ice Bolt every second, damage 15 + 4 Ã— monster level. |
| Teleportation | Every 4 seconds, approach a nearby player via native collision-checked relocation, attack and recover 5% maximum life on successful arrival. |
| Spectral Hit | Original elemental attacks plus an elemental burst on every fourth successful direct hit. Bursts cycle fire, cold, lightning, magic and poison; 3-yard radius. |
| Stone Skin | 75% less resolved damage of all damage channels. No added physical resistance. |
| Multiple Shots | Native primary missile plus two pairs: five suitable projectiles. Native missile exclusions/reentrancy protection remain. |
| Plaguebearer | A 2-yard poison pool every 2 seconds; three on death. Pools last 3 seconds. |
| Gravity Core | 1-second warning every 5 seconds, then pull nearby opponents, slow by 30% for 3 seconds and burst. Pull radius 8 yards. |
| Mirror Image | First copies after 1 second; up to two private copies every 8 seconds thereafter, lasting 6 seconds; 25% parent life and 35% damage. |
| Pack Leader | First reinforcement after 1 second; one reinforcement every 8 seconds thereafter, maximum four; nearby minions/reinforcements gain 30% resolved damage. |
| Glittering | Two independent normal treasure rolls. |
| Raid Boss | Every modifier once, five treasure rolls total; does not multiply Glittering's two rolls again. |
| Totemic | At most one hostile stationary totem every 3 seconds, maximum three, 10-second lifetime. Totems have 2% parent life. Nearby enemies take 10% damage and heal 10% maximum life each second. Totems do not protect each other. |

Damage reduction stacks multiplicatively: Stone Skin plus a protective totem results in 2.5% incoming resolved damage. Multiple totems do not multiply protection or regeneration.

Summoned copies and totems use private monster records, neverCount, no XP and no treasure classes. They cannot recursively roll modifiers. All state is reset for a new game. Timers never catch up by emitting dozens of effects after a pause.

## Runtime corrections in 0.16.3

Manual missile damage now uses native 8-bit fixed point. Monster corpse modes trigger the delayed fire explosion even when the engine retains positive HP. Visuals and private spell projectiles use native client missile publication. Poison and Gravity activate after one second initially; their repeat intervals remain unchanged.

## Modifier display and verification

Portable engine/native-bridge tests cover modifier deduplication, Raid expansion, timer cadence, session isolation, exactly-once delayed explosions, 2x/5x drop forwarding and output-buffer safety. Existing combat/data regressions also pass. All combat combinations still need gameplay testing.

The native monster panel now appends Plaguebearer, Gravity Core, Mirror Image, Pack Leader, Glittering, Raid Boss Totemic, Duelist, Ritualist and Corpse Eater when present, two names per line. Existing health, resistance and native-affix text is preserved. Raid Boss exposes all ten custom names. The renderer reads a locked ID/class snapshot of authoritative rolls and never touches server units. Session resets clear the cache.

The real x64 continuation test verifies stack alignment, the panel return register, text preservation and ID/class/session isolation. The final in-game layout still needs visual confirmation.

## Area hazard correction in 0.16.5

Fire detonation and poison pools now use a dedicated native spell-hit projectile with no attack-rating roll and no secondary explosion callback. The hostile owner is also the missile origin; the victim is only its target. Native resistance and damage processing remain in place. Plaguebearer uses the actual poisonexplosioncloud asset rather than a missing-name fallback. Targeted logs record hit outcome, life before/after and poison stats. Code/data tests passed; actual damage still needs an in-game confirmation.

## New encounter mechanics in 0.16.6

| Modifier | Behavior |
|---|---|
| Duelist | Visible flame boundary with an 8-yard radius centered on the monster. Damage from an attacker outside the arena is rejected before native damage processing. For player-owned summons, the player's position is checked. The boundary follows the monster and refreshes every 0.8 seconds with a 1.6-second marker lifetime. Boundary flames themselves do not deal damage. Already applied damage-over-time effects retain their native duration. |
| Ritualist | Once on engagement, creates three hostile, destructible Ritual Stones with 5% of its maximum life each. After exactly 10 seconds, remaining stones determine a private copy's strength: one = 100% parent life / 125% damage; two = 200% / 200%; three = 400% / 300%. Destroying all three prevents the summon. Killing the Ritualist cancels its ritual. The result lives for 60 seconds, grants no loot/XP/quest credit, and cannot roll modifiers. Stones are removed when the ritual resolves. |
| Corpse Eater | While wounded, checks within 8 yards every 3 seconds. Successfully consuming one eligible monster corpse restores full life. Consumed corpses are no longer available to corpse skills. Player-owned minions and private modifier helpers are excluded; failed consumption grants no heal. |

All three consume ordinary unique modifier slots and appear in the monster caption. They can combine with all existing modifiers. Raid Boss includes all three exactly once. The core tests cover the arena boundary, ritual deadline (also when the player leaves), all four stone outcomes, death cancellation and corpse-consumption success/failure. In-game behavior still needs player confirmation.

## Area-transition crash fix in 0.16.7

Both reported Dark Forest crashes stopped at native RVA 0x49e5cf with a null fourth argument from the custom modifier roll. The superunique top-up supplied nullptr where the native picker requires a readable weight-map header. It now supplies an empty header when no overrides exist, while preserving real native override maps. Difficulty counts, fixed boss affixes and the three new mechanics remain unchanged. Regression tests cover the Hell superunique top-up, exactly-once initialization, external-map forwarding and reentrant calls.

## Hazard damage and arena correction in 0.16.8

The private hazard hit inherited Explosion=2 from fireexplosion. The native hit handler checks this flag at record+0x24 and disables its default damage path; returning 2 did not prove that damage occurred. Clearing this flag enables native impact damage for the delayed fire explosion, burning/poison fields and other shared area hits. Resistance processing stays native. The arena now uses a dedicated generic, looping client marker, renewed every 20 frames for a 40-frame lifetime, independently of whether the monster currently has an AI target. Corpse Eater (modifier 22) appeared in the user's roll logs; its healing still needs a gameplay check.
