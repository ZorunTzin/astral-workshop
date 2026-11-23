# Astral Workshop 0.24.0

- Duration shrines now last five minutes. Stamina adds 30% attack/run-walk/hit-recovery/block speed; Armor adds 20% maximum life and 1% maximum-life regeneration per second; Experience adds 100% Gold Find and 50% Magic Find; Combat adds 25% Bleed and Impale; Mana Recharge adds 50% cast rate; resistance shrines add 10% corresponding maximum resistance.
- Skill shrines grant +2/+3/+4 skills on Normal/Nightmare/Hell. Poison and Exploding shrines add a 100,000-damage enemy-only blast. Monster shrines summon five elite leaders with native minion packs. Fire shrine is unchanged.
- Terror Zone monster-level caps: Normal 70, Nightmare 85, Hell 100.
- Improved world goblin spawning and blocked-placement retries. Odious Collectors drop crafting materials; Gem Hoarders drop custom gems; Golden Baron gold piles substantially increased, including the low-level Normal correction.
- Beyond and Astral Breach use twelve monster families each. League orb names include their tier. Conditional tier weights: Normal 80/15/4/1%, Nightmare 65/25/8/2%, Hell 50/32/15/3%. Night of the Dead's base orb drop chance is halved; other League base rates are retained.
- Ground pickups of gems, runes and Orb of Infusion use the automatic stash route again, with retries and withdrawal protection.
- Jab gains 10% Bleed chance plus 1 percentage point per additional skill level. Jab, Pounce and Fend flat damage and synergies are doubled.
- Enchant life/mana recovery now uses successful weapon attack/projectile hooks with duplicate protection. Physical damage reduction text displays its value correctly.

Validation: native build/regression suite, targeted shrine tests and generated-data checks passed. New native shrine interactions and recent combat fixes still need in-game confirmation. Restart the game after updating. Existing save profile remains ReimaginedWorkshop-v2.
