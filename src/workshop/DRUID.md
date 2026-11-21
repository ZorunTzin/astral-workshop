# Druid rework - 0.17.1

## Winter's Grasp
An 8 + floor(L/5) yard frost cone deals 15-25 base cold damage, +8 to both ends per additional level, once per second for 3 + floor(L/10) seconds. Slows movement and attack speed by 35%. Uses Tornado's synergies: +8% per hard point in other elemental-tree skills and +4% per hard point in Spirit Wolf. Costs 9 + L mana. Damage is shown per second.

## Storm Seeds
Fires 3 + floor(L/10) wind seeds. Each explodes at the first nearby enemy for 20-30 base physical spell damage, +10 to both ends per additional level, in a 3-yard area. Uses Tornado's +8% elemental-tree / +4% Spirit Wolf hard-point synergies. Avatar's physical spell bonus also affects the character-sheet values. Slowed enemies take 50% more damage. Costs 9 + L mana.

## Primal Fury
Reserves 30 + 4L mana before efficiency. +10+L% attack speed, +50+10L% attack rating, +20+5L% physical attack damage, +30+10L% defense and +10+L% all resistances. Every fifth successful melee hit releases a fire explosion for 30+15L to 60+25L damage. Compatible with animal forms.

## Alpha's Call
Reserves 25 + 3L mana before efficiency. All owned summons gain +15+3(L-1)% damage, +5+(L-1)% attack speed and +10+2(L-1)% movement speed. No life bonus. Nearby summons from other classes are included.

## Avatar of the Wilds
Reserves 40 + 5L mana before efficiency. Clay Golem appearance, without the cosmetic tornado. +15+3L% fire/cold/physical spell damage, +10+L% cast speed, +20+2L% maximum life, +15+2L% faster hit recovery and +floor(L/10)% maximum elemental resistances. No lightning damage or defense bonus. Summons a native Raven every five seconds at Avatar level, with its native hit budget restored. Recast to return to human form. Now occupies the former Alpha's Call position.

## Poison Creeper
Three times the original life; maximum 1 + floor(L/10) + Astral summon bonuses. Targets hit by the creeper take 15+L% more damage from the same owner's summons for four seconds.

## Carrion Vine
Three times the original life; maximum 1 + floor(L/10) + Astral summon bonuses. Consumes one eligible nearby corpse every three seconds. Restores 3+floor(L/2)% player and summon life; summons gain 2+floor(L/10)% maximum life regeneration per second for five seconds.

## Solar Creeper
Three times the original life; maximum 1 + floor(L/10) + Astral summon bonuses. Consumes one eligible nearby corpse every three seconds to recover 3+floor(L/2)% player mana. Player and nearby summons gain 5+L% attack, cast and movement speed for five seconds.

## Oak Sage
Own elemental resistances min(50+L,85)%; magic resistance 25%; physical damage reduction 25%. Aura: +15+(L-1)% maximum life and 2+L life regeneration per second. The in-game tooltip computes the Spirit's actual HP rather than showing its fivefold base-life multiplier.

## Heart of Wolverine
Own elemental resistances min(50+L,85)%; magic resistance 25%; physical damage reduction 25%. Aura: +15+5(L-1)% physical damage, +25+7(L-1)% attack rating, and min(5+L,30)% attack/cast speed. Actual Spirit HP is computed by the native in-game tooltip.

## Spirit of Barbs
Five times original base/scaling life. Own elemental resistances: min(50+L,85)%; 25% physical reduction. Friendly aura grants 10+5L reflected physical damage and min(5+floor(L/2),25)% physical damage reduction.

Lycanthropy formulas are embedded in Werewolf/Werebear at the respective form level. Public and existing private IDs remain stable. Five original generated icons are packed into HD/low-end atlas slots; all unrelated frames remain byte-identical. Runtime graphics use native aliases and explicit publication. Volcano is unchanged.

Validation: full existing suite, targeted Druid regression tests, native visual mappings and tooltip data. In-game combat/appearance confirmation remains pending.

0.17.1: Alpha and Avatar tree positions swapped; runtime buff lists carry the correct skill and level for their icons. Native life formatter (13) restores actual pet HP; personal resistances and aura bonuses are distinct. Avatar periodic tornado removed. Public Raven parameters retained for native AI. Shared synergy formulas drive spell hits and UI; physical spell bonus is an unsaved aura stat.
