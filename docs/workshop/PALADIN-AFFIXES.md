# Paladin equipment affixes

Magic and rare Paladin shields can roll two new suffix families:
- Increased Aura Effect (`workshop_aura_effect`)
- Reservation Efficiency (`workshop_reservation_efficiency`)

| Tier | Affix level | Required level | Roll |
| --- | --- | --- | --- |
| 1 | 1 | 1 | 1-3% |
| 2 | 20 | 15 | 4-6% |
| 3 | 40 | 30 | 7-9% |
| 4 | 60 | 45 | 10-12% |
| 5 | 80 | 60 | 13-15% |

Both families may appear together; tiers within one family cannot stack as separate affixes. Lower tiers remain eligible on higher-level items. Eligibility uses the `pala` type (currently Auric Shields), not all items a Paladin can equip. Each affix has frequency 8 and is exempt from Magic/Rare affix multipliers.

The existing runtime aura stats 496/497 receive 16-bit item save fields and item tooltip labels. They previously had no item property bindings or saved payloads. No stat IDs move and no existing saved field changes size. Gear contributes to the same accumulated stats used by aura effects and reservation formulas.

Validation: `python src/workshop/paladin_affixes_verify.py build/workshop-clusters`.
