# Shrines

Duration shrines last 300 seconds (7,500 game frames). Existing base bonuses remain.

| Shrine | Added bonus |
|---|---|
| Stamina | 30% attack speed, run/walk, hit recovery and block rate |
| Armor | 20% maximum life; regenerate 1% maximum life per second |
| Experience | 100% Gold Find; 50% Magic Find |
| Combat | 25% Bleed; 25% Impale |
| Skill | +2 Normal, +3 Nightmare, +4 Hell |
| Resistance | 10% corresponding maximum resistance |
| Mana Recharge | 50% cast rate |

Poison and Exploding shrines retain their original visuals/potions and deal an additional resolved 100,000 damage to hostile monsters within 24 tiles, in the same area. This blast bypasses elemental immunity and excludes players and allies. It does not guarantee a kill against enemies with more than 100,000 life. Fire shrine behavior is unchanged.

Monster shrines create five native elite leaders, promoted through the same native routine as the original monster shrine, including native minions. Placement retries when blocked and never replaces existing monsters. Pending spawns remain associated with the shrine area and activating player.

Extra buffs belong to native shrine stat lists and expire with them. Skill shrine supplements the native +2 bonus by only +1/+2 on Nightmare/Hell.

Implementation guards the D2R 3.4 shrine dispatch entries 20–22 at RVA 0x1d3f1d0; the Fire entry 19 is never patched. Native ABI and promotion RVA 0x49ac30 were checked against the existing local binary snapshot. Runtime player-facing behavior still needs in-game validation.
