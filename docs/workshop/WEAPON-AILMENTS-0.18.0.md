# Weapon ailments - 0.18.0

Seven native alvl tiers per suffix: 2-5, 4-8, 7-12, 10-18, 15-25, 20-32, 25-40 percent. Exact values bypass magic/rare multipliers. Caster rod family excluded; orbs included.

Only new rolls/items acquire the changes. All player classes use the existing physical-hit ailment engine; mastery and item chances add to a 100% cap. Existing effects and saved stat layouts preserved. Two previously unsaved and unreferenced obsolete slash IDs (256/257) now hold item chance stats; no existing stat IDs are shifted.

| Item | Type | Effect | Roll |
|---|---|---|---|
| Rixot's Keen | uniqueitems | bleed | 2-5% |
| Skewer of Krintiz | uniqueitems | bleed | 2-5% |
| Gleamscythe | uniqueitems | bleed | 4-8% |
| Bladebone | uniqueitems | bleed | 4-8% |
| Goreshovel | uniqueitems | bleed | 4-8% |
| Brainhew | uniqueitems | bleed | 7-12% |
| Soulflay | uniqueitems | bleed | 4-8% |
| Blacktongue | uniqueitems | bleed | 7-12% |
| Ripsaw | uniqueitems | bleed | 4-8% |
| The Patriarch | uniqueitems | bleed | 4-8% |
| The Patriarch | uniqueitems | impale | 2-5% |
| The Dragon Chang | uniqueitems | bleed | 2-5% |
| Razortine | uniqueitems | bleed | 4-8% |
| Dimoak's Hew | uniqueitems | bleed | 2-5% |
| Soul Harvest | uniqueitems | bleed | 4-8% |
| Woestave | uniqueitems | bleed | 7-12% |
| Bloodmoon | uniqueitems | bleed | 15-25% |
| Flamebellow | uniqueitems | bleed | 20-32% |
| Pluckeye | uniqueitems | impale | 2-5% |
| Witherstring | uniqueitems | impale | 4-8% |
| Leadcrow | uniqueitems | impale | 2-5% |
| Ichorsting | uniqueitems | impale | 4-8% |
| Hellcast | uniqueitems | impale | 7-12% |
| Endlesshail | uniqueitems | impale | 7-12% |
| Cliffkiller | uniqueitems | impale | 10-18% |
| Magewrath | uniqueitems | impale | 10-18% |
| Langer Briser | uniqueitems | impale | 7-12% |
| Deathbit | uniqueitems | impale | 10-18% |
| Berserker's Hatchet | setitems | bleed | 2-5% |
| Death's Touch | setitems | bleed | 2-5% |
| Isenhart's Lightbrand | setitems | bleed | 2-5% |
| Angelic Sickle | setitems | bleed | 4-8% |
| Tancred's Crowbill | setitems | bleed | 4-8% |
| Arctic Horn | setitems | impale | 2-5% |
| Vidala's Barb | setitems | impale | 4-8% |
| Corgina's Orb | setitems | bleed | 4-8% |
| Warlord's Pike | setitems | bleed | 4-8% |
| Lancer's Reach | setitems | bleed | 7-12% |
| Kingpin's Blade | setitems | bleed | 7-12% |
| Cryptic Claws | setitems | bleed | 20-32% |
| Trent's Caster | setitems | impale | 10-18% |
| Alyssa's Leafblighter | setitems | impale | 10-18% |
| Holy Fury | setitems | impale | 15-25% |

Validation: all original item fields preserved; 509 unrelated stat records identical; exact tiers, eligibility, class-independent runtime effects, block/zero damage and chance cap covered. No in-game drop/tooltip/save-reload confirmation yet.