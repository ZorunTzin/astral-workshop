# Astral Workshop 0.26.0

- Trials start at monster level 105 and gain one level per wave. Tier 1–5 life, damage, XP and base loot-roll factors: 2/3/4/5/6. All-speed bonus: 20% per tier plus 3% per later wave.
- Ctrl+T hides/shows the Trial choice panel. League invasion cards name the selected league. Hell elites receive 3–5 distinct modifiers. Summoned helpers receive missing speed and resistance bonuses while retaining their inherited life fraction.
- Trial materials: ten times the former category chances. Trial Orb chance: one third; removed from Akara. All 64 active Mythical variants are reachable from Trial drops. Blood Raven/Corpsefire Mythical test drops removed; Corpsefire’s splash charm restored.
- Elemental Covenant: 10% at level 1, +1 percentage point per additional level. Titan’s Covenant physical damage: the same; Crushing Blow chance: 1% at level 1, +1 per additional level.
- Crushing Blow: 5% instead of 25% against ordinary melee monster targets. Native boss/ranged/player-count/resistance reductions remain.
- Alpha’s Call summons Ravens; Avatar of the Wilds is in the Elemental tree. Goblins use area level, with natural encounter chance divided by three.
- Magically Gifted lightning: 40 + 2 × monster level before other modifiers (50 at level 5).
- Correct gold renderer markers for Doryani and Runeforged labels. Doryani has Fire/Cold/Lightning/Poison variants with element-specific resistance text. Updated Original Sin, Svalinn, Rathpith, Gull and Nimis descriptions. Nimis supports Multiple Shot. Gull kill buffs last 60 seconds and coexist with normal shrines.

Validation: complete native combat regression suite, Astral/Chronicle tests, generated-data checks, and website route/filter/asset checks passed. Targeted tests cover Alpha-only Ravens, Trial helper bonuses, four Doryani auras, Nimis projectiles, gold tooltip bytes, Gull duration and native Crushing Blow divisors. Browser preview checked. In-game rendering and balance still need gameplay confirmation.

Restart the game after updating. The save profile remains ReimaginedWorkshop-v2. Existing rolled items retain their properties. Local installation preserves configured monster density.
