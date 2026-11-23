# Astral Workshop 0.23.0

- Gem Bags are no longer starting items, sold by vendors or created by recipes. Existing bags remain readable and can still be emptied.
- Player-facing localized Reimagined text and the automap brand now say Astral Workshop. Internal mod/save IDs remain compatible.
- Raid Boss is excluded from all random modifier rolls.
- All 12 Orb descriptions escape literal percent signs for the native formatter. Crafting effects and level costs are unchanged.
- Stash search strips native color control sequences. Organizer results have a visible, clickable and draggable scrollbar in addition to wheel/page controls.
- League pillars require a clear 11 by 11 subtile footprint and bypass area; failed placements retry later.
- Night of the Dead spawn offsets are tripled. All League elite promotions are reduced by 75 percent.
- Three level-one Amazon Masteries occupy free cells beside/below Jab. They are free toggles with existing weapon icons and Buff HUD entries. Exactly one is selected at a time. Bonuses suspend with the wrong active weapon; server-side start/do gates reject incompatible weapon attacks. Death and respec clear the stance.

| Mastery | Skill level 1 | Skill level 20 |
| --- | --- | --- |
| Spear | +20% max life, +30% defense, 3% physical DR, +6% all resists, +6% attack speed | +115% max life, +220% defense, 20% physical DR, +25% all resists, +25% attack speed |
| Bow / crossbow | +11% movement, 4% Impale, +3–6 fire/cold/lightning each | +30% movement, 23% Impale, +41–63 fire/cold/lightning each |
| Javelin | +4% block, +14% max life, +7% FHR, +7% FBR | +23% block, +90% max life, +45% FHR, +45% FBR |

Bonus caps are applied to effective skill levels, including equipment. Spear Mastery also blocks bow/javelin-only skills and throws. Shared spear/javelin melee skills retain their native weapon checks. Existing gear and saves are not rewritten.

Validation: paired-table checks, safe-format checks for all Orb tiers, native League/modifier regressions, and focused Mastery tests for switching, exact bonuses, incompatible weapons, inactive weapon slots, level changes, toggle-off, death and respec. Full interactive gameplay validation is still required.

Startup validation: all 190 tables compile without assertions; native Mastery start/do gates initialize successfully. Fixed forward references in Trial/Mythical and Goblin loot tables.
