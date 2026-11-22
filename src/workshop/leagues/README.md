# League encounters — first implementation

Local development implementation, 2026-11-22. Native builds, mock runtime tests and generated-data checks are automated. Actual visuals, pillar interaction and pickup/save/reload still require an in-game test.

## Encounters

Three independent 12% rolls on first entry into each eligible act area per game; multiple mechanics may coexist. No town, cow-level or Trial spawning. Failed placement is retried every two seconds without rolling the encounter chance again (up to twenty attempts in normal mode; until success in Blood Moor test mode). Waiting pillars persist; active encounters are removed after the area is empty for ten seconds. Each encounter ends after its timer or completion; its remaining units and rifts are cleaned up, including unloaded units when they become available again.

- **Night of the Dead:** click the pillar. For 90 seconds, waves of six zombies are queued every three seconds, using three black-tinted variants. Up to 48 live event monsters; up to 180 spawned. Champions have 2.5x template HP; rare late-event overlords have 8x HP. Event monsters continually target a player in the area and distant zombies are given pursuit movement. The native light-radius stat receives -100 while the event is active in the player's area; it is removed on exit/end. Outdoor ambient lighting may limit how dark this appears and must be checked in game.
- **Astral Breach:** three green rifts joined by effect trails. Nine initial defenders, then two waves of twelve stronger monsters, then three overlords. Clearing the current defenders advances the wave. Three-minute timeout.
- **Beyond:** click the pillar; for three minutes, five kills (including Night and Breach enemies) within five seconds and ten game tiles trigger a group of four red enemies. Every fourth trigger gives an overlord instead. Maximum twelve groups; Beyond monsters do not feed this counter. Later groups use champion variants.

All colored models are private copies referring to retail model/texture assets; normal monsters retain their original appearance. Strong variants are separate monstats rows, not a change to ordinary act monsters.

## Currencies and storage

| Mechanic | Row 1 | Row 2 | Row 3 | Row 4 |
|---|---|---|---|---|
| Night of the Dead | Dusk Orb | Grave Orb | Soul Orb | Lich Orb |
| Astral Breach | Fissure Orb | Fracture Orb | Vortex Orb | Convergence Orb |
| Beyond | Blood Orb | Hunger Orb | Ruin Orb | Dominion Orb |

Native first-pickup routing and `AdvancedStashStackable` are used. The new visible **League items** button selects a separate 3x4 view backed by the native Materials bank. It does not increase the engine's fixed five-category array or introduce an independent save file. The Materials button returns to the original view. Items are Cube crafting materials; combine one orb with one equipment item. Test pickup and save/reload before relying on it for a long playthrough.

Current native weights are 60/25/12/12, with NoDrop 3891/1223/291 for normal/champion/overlord. T1–3 base probabilities are one quarter of their former values; T4 is unchanged. These are native TreasureClass weights, so native multiplayer NoDrop adjustment applies. Event cleanup does not award drops.

The compact grid occupies the upper portion of the stash with room for additional columns and rows. Definitions, icons, placement and generated binding IDs live in this directory; native slot item codes must remain stable for saved currencies. Adding mechanics also requires extending the runtime kind/behavior and fixed binding dimensions.

## Testing

Default mode uses the random area roll. For deterministic local testing, set `d2rloader/config/workshop-leagues-test.txt` beside the installed mod's other config files to `1`, restart D2R and enter Blood Moor in a new game. All three encounters are attempted around the player's first position in Blood Moor; Night and Beyond require clicking their pillars. Set the file to `0` or remove it to return to random-only spawns. This flag is deliberately not enabled or packaged by default.

Suggested gameplay checks: click each pillar, kill Breach waves, kill normal enemies close together for Beyond, collect each orb, open League items/Materials repeatedly, save/reload and check counts. Verify black/green/red models, darkness recovery in town, and rift cleanup after completion or timeout. Existing goblin/Mythical local test settings remain unchanged.

`Workshop.ps1 -Action Prepare` generates content. `leagues/verify.py <stage>` checks data/assets/pickup identity. The regular combat build includes `runtime_test.cpp` (mock native boundary): activation, expiry, darkness removal, escalation, spatial/time kill threshold, no recursive Beyond, monster cap, unloaded cleanup, reused IDs and new-game reset. Native entry points are byte-guarded; unsupported object API disables encounters without disabling the rest of combat.

## Asset provenance

`art/*.png`: twelve original icons generated using the built-in image generation tool, transparent background; exact prompt set in `art/prompts.json`. The resulting game sprites are 98px/49px. `upstream/*.json`: retail D2R definitions extracted from the locally installed game and used as model/animation references. They retain their original dependencies; no upstream ownership is claimed.

## Placement correction (2026-11-22)

Native allocation argument 7 is flags, not animation mode: bit 0 attaches the object to the room. Argument 8 selects neutral animation mode 0. Free-position search mutates its second argument; the selected coordinates must be used by the allocator. Regression fixtures now model both behaviors and verify retries after blocked area entry. Startup/session/placement diagnostics are logged to workshop-curses.log.

## Activation and storage correction (2026-11-22)

Pillars use native SecretDoor operation 18 (one-shot opening), because the retail D2R operation table has no handler in Torch slot 11. A guarded native deposit retry scans only the twelve League currencies in living players' backpacks once per second. It uses the same native bank resolution and deposit operation as auto-collectibles, retaining items when the bank is unavailable. Already collected orbs are included. No separate balances or save files are written. Mock regression tests cover unavailable banks, failed deposits, all twelve codes and duplicate prevention; in-game pickup/save/reload remains a manual check.

## Zone balance and orb feedback

League spawns are normalized to the expansion area level. Native HP scales to 70% of that zone level and damage to 140% (twice the previous 70%); champion/overlord HP multipliers remain. XP is twice the average ordinary monster XP from the area's population table, retaining the native spawn's player-count multiplier. Ground orbs use a jewel beam with a colored floor aura (silver-gray Night, green Breach, red Beyond) and matching localized name colors.

Bank selection trusts the authoritative server lookup of the player's native stash node; hidden stash proxies need not carry a normal unit game back-pointer. Limited session diagnostics report candidate eligibility, missing banks and native deposit results. Actual pickup/save/reload still requires an in-game test.

## Follow-up fixes and validation

League elite tiers now set the native unique flag and invoke the existing difficulty-scaled modifier picker and initializer, including custom affixes. Waiting pillars survive leaving an area; active encounters retain bounded cleanup. Each eligible area rolls three independent 12% chances, with its complete mask logged (bits 1/2/4). At least two mechanics occur with probability 3.9744% per area; none across 40 independently rolled areas remains about 19.75% likely.

League slots initialize like Trial Materials slots, without an initial hidden field; the UI visibility controller switches their display afterward. Storage first tries the authoritative bank list and then the installed core's local-player accessor and native Advanced Stash destination. The destination ID must resolve in the authoritative server game before depositing. Missing destinations retain every item; node diagnostics support further live investigation.

Beam renderer properties at 0x20aa and 0x6ef0 previously selected the R-only texture sample, explaining white red-orb beams and nearly invisible green beams. Both now select RGB. Only the actual jewel-beam entity and one floor aura are copied, with distinct effect IDs and names. Map markers use native static-path client coordinates and native-to-overlay viewport scaling.

Mock/native-boundary tests and generated-data checks pass. Visual appearance and real save/reload deposits still require an in-game confirmation; these checks do not establish them.

## Stackable equipment crafting

Source of truth: `crafting.py`, 36 bundles, 48 recipes. Costs +2/+4/+6/+10 required levels. All bonuses stack including T4 extras. Original item retained; native matcher rejects results above 120 before consumption. Unsaved stat 265 defaults to zero, failing closed without the plugin. Saved stats 262/263/264 hold universal spell damage, Trinity chance and level cost.

Trinity triggers only on successful weapon attacks. Deferred damage uses a weapon snapshot, 100% as each fire/cold/lightning within 24 subtiles, with recursion prevention. Universal Spell Damage affects every resolved spell channel including physical/magic/poison.

`craft_test.cpp` tests level boundaries and refusal before matching. `effects_test.cpp` tests hit classification, all spell channels, Nova payload, deferred execution and session reset. In-game craft/save/reload and visual checks remain playtests. Website export uses the same crafting rules.

## Pickup, withdrawal and native recipe correction

Only League item IDs observed on the ground (or dropping) are eligible for deposit retries. Inventory-only items and stash withdrawals are ignored. Moving a pending pickup onto the cursor, into the Cube or into a stash cancels its retry; successful deposits erase the observed identity. Native item buckets are observed every server frame, using the existing verified item-table offset. The original native first-pickup integration remains enabled.

All 48 orb recipes now write up to five real native Cube modifiers directly, including +2/+4/+6/+10 saved required-level cost. Orb crafting no longer depends on the bundle-dispatch hook. The level-cap matcher remains in front of consumption. Orb help is in the displayed `spelldescstr` field, covering all three equipment groups. All four stash rows use 64px hit areas above the native Cube grid, also in the controller layout.

Validation now checks each native recipe property/parameter/min/max/chance against the approved matrix, both table copies, all visible tooltip references, non-overlapping slots, ground-only eligibility, retry preservation and no re-deposit after withdrawal. Previously miscrafted items lack their consumed orb identities and cannot be reconstructed automatically.
