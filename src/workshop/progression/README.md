# Level-120 progression

Player cap: 120 for all eight classes. Existing seven-bit saved character level and 32-bit saved experience remain unchanged; no save-format migration is needed.

The frozen baseline is the published 0.21.0 `experience.txt` and `monlvl.txt`. Do not replace it with generated output.

## Player XP

- Levels 1–79 and the threshold for reaching 80 are unchanged.
- Penalty begins at 80. Interpolate the former 79–98 penalty transition across new levels 79–119 (47.5% of the former per-level slope, rounded to native 1/1024 units).
- Level 120 requires exactly 3,520,485,254 cumulative XP: the former level-99 threshold.
- Level thresholds are calibrated so `sum(XP needed for level / XP multiplier)` from 1 to 120 matches the previous 1-to-99 total within integer rounding. Both the saved-XP target and theoretical pre-penalty XP budget match.
- This is a constant comparable-enemy/buff reference, not a guarantee of equal hours. Monster-level differences, XP bonuses, party/players settings and the newly enabled Trial XP change actual leveling speed.
- Existing saves keep their experience. The new thresholds can therefore correspond to a higher level; normal game XP/level-up processing handles the changed thresholds. This needs in-game verification for existing high-level characters.
- No XP loss on death in Normal, Nightmare or Hell. Trial score/run failure rules are unchanged.

## Trial XP and monster levels

Trial areas and private monster templates start at 100. Each spawned unit receives `99 + wave` as its level after native champion/elite initialization. Existing tier/wave HP and damage multipliers remain in use; they are not applied twice through this XP feature.

Restore each regular monster/boss type's native XP percentage. Scale its actual native XP stat by the ratio of the smooth level-100+ Hell expansion XP curve to its original native spawn-level XP value. Thus player-count/native rank multipliers are preserved. The original special native level-110 row is preserved globally but cannot cause a sudden double-XP jump in wave 11. XP grows by the ordinary 100–109 per-level increment instead.

All initial and missing-unit replacement spawns, milestone bosses, random Trial goblins, goblin event waves and split children take this path exactly once. Zero-XP gelatinous parent/intermediate stages stay zero. Ordinary act goblins remain unchanged.

Native monster-level lookup rows are extended to 511, the nine-bit transmitted level limit. Monster level safely caps there at wave 412; XP arithmetic uses 64-bit intermediates and a signed-stat safety ceiling. This prevents wraparound in unusually long runs.

## Checks

`python src/workshop/progression/verify.py build/workshop-clusters`

Also run the Trial and goblin native runtime tests and rebuild the combat DLL. Tests cover thresholds, effort calibration, both table copies, save-width bounds, no death loss, nonzero native Trial XP, monotonic wave XP, boss/recovery/goblin paths and idempotence. In-game XP awards and high-level save transitions remain player tests.
