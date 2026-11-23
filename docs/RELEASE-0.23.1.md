# Astral Workshop 0.23.1

Three exclusive Amazon Masteries now have dedicated painted HD and low-end icons, with normal and inactive frames. Existing skill artwork is preserved.

All listed base values apply at skill level 1; increments start at level 2. Fractional results round down to whole native percentage points, identically in gameplay and tooltips.

| Mastery | Updated bonuses | Level 20 |
| --- | --- | --- |
| Spear | Life 20% +2%/level; attack speed 10% +3%/level; all resistances 10% +2%/level | 58% life, 67% attack speed, 48% all resistances |
| Javelin | Life 15% +1.5%/level; mana after kill 5 +1/level | 43% life, 24 mana after kill |
| Bow | Life 10% +1%/level; life stolen per hit 2% +0.5%/level | 29% life, 11% life stolen per hit |

Other existing bonuses and weapon restrictions remain unchanged.

Artwork: built-in Imagegen, three individual dark gothic square RPG icons. Prompts: silver spear and bronze shield with crimson aura; gold/wood longbow and arrow with violet aura; three barbed throwing javelins and buckler with emerald aura. No text or border, clear silhouettes for small skill icons. Original PNGs are under src/workshop/combat/icons/amazon/*-mastery.png.

Validation: full Combat build/regressions; level 1–40 Mastery scaling and cleanup tests; native tooltip rounding checks; all data checks; unchanged original sprite frames and idempotent packing. Game startup completes without table assertions and loads the Mastery weapon gates. Visual placement in the live skill tree has not been manually checked.
