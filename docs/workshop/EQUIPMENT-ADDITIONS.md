# Equipment additions

Selected balance buffs, not measured usage rankings:
- Aura Effect: Steelclash 3-5%, Twilight Presence 6-10%, Ancients' Epiphany 10-15%, Milabrega's Rod 3-5%, Greyhawk's Deflector 4-7%.
- Reservation Efficiency: Defender of Innocence 6-10%, Milabrega's Orb 3-5%, Prayer of Brother Laz 6-10%.
- All Maximum Resistances: Coif of Glory, Duskdeep, Civerb's Ward, Isenhart's Case, Infernal Cranium 1-2%; Venom Ward, Hawkmail, Milabrega's Robe 2-3%; The Ward 3-5%.
- Runeforging, retained at every rarity: Nadir 1-2%, Radiance 2-3%, Myth 3-5% all maximum resistances.
- Armor suffix tiers: +1 / +2 / +3 / +4 / +5 at affix levels 10 / 25 / 40 / 60 / 80 (required levels 7 / 18 / 30 / 45 / 60). Uses `armo`; excludes weapons, charms, jewels, arrows, bolts, rings and amulets. Magic scaling x2 reaches 10; Rare x1.35 with floor reaches 6. Aura/Efficiency remain exact-range Paladin-item affixes.

New item rolls/crafts receive fixed-table additions; previously saved items are not retroactively buffed.

## Pending class equipment request
Prepared `universal_class_items.py/.h/.inc` and conditional native property conversion, but disabled (`universalClassItemsEnabled=false`). Existing class restrictions remain active. The ordinary-property prototype maps class skill bonuses to allskills and explicit single-skill properties to oskills only on class equipment. Native staffmods and random-skill properties, plus existing saved-item migration, still require implementation and validation. Do not enable the partial implementation or call its generator from the release build until those paths are covered. No completed cross-class equip/gameplay test exists.
