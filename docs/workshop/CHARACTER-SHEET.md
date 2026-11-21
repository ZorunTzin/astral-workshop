# Character-sheet display corrections (0.12.17)

Custom skill descriptions now populate `str alt`, the independent Character
Screen title, rather than retaining Taunt, Double Throw or the original aura
name. Non-damaging buffs, curses, passives and summons no longer inherit a
weapon damage handler from their original slot.

Native damage handler 7 uses percent and flat formula inputs. Displays label
their unit explicitly: Quake shows the scaled weapon portion (its extra 5–7.5
flat physical damage remains in the tooltip); Ground Stomp shows one pulse,
Retribution one nova, Plague/Infection initial poison per second, Attract its
added physical damage per incoming hit, and Elemental Marks its fire pulse.
These are not a total DPS estimate. Target resistance, poison growth,
conditional explosions and other Elemental Marks triggers stay in tooltips.
No gameplay damage formula is changed by this presentation pass.

The field roles follow the mod-author
[SkillDesc guide](https://d2r-tools.com/data/skilldesc), checked against the
loaded Reimagined descriptors. Both expansion/base outputs are audited.

Long Reach's three normal specialization nodes now grant +15 maximum damage
each and its end node +30, matching Fang's primary 15/30 values. The finale also grants +30 minimum damage; the +3-all-attributes path node is retained.
An existing Sigil must be regenerated with Apply to receive the new values.


Post-installation validation: all data checks and Sigil regressions passed,
including Long Reach's actual minted +75 primary maximum damage. Startup was
clean, and the user confirmed Quake/Ground Stomp names and damage values now
appear in the Character Screen. Other custom skills have matching descriptor
checks in both data modes; their individual live displays were not all tested.
