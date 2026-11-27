# Workshop-local plugin adaptation

Inputs are the user's existing FMMP 0.22.2+Randomizer binaries. MapSense originated in the MIT-licensed RuffnecKk Suite: https://github.com/RuffDood/RuffnecKk-D2RLoader-Suite . Original binary inputs and third-party notices are retained in `upstream/`; `UPSTREAM.json` pins their SHA-256 digests. This directory does not claim authorship or relicense upstream code.

`prepare.py` changes only mod identity data and MapSense's corresponding entry name-length comparison. Native operation signatures, core/loader identity checks, bank transfer code and map-reveal/renderer implementation remain unchanged. FMMP-only monster-display filters intentionally keep their old length checks so Workshop uses ordinary MapSense markers.

The first-pickup plugin checked the original packed FMMP MPQ hash. Workshop has a loose MPQ directory, so its identity path becomes `/mods/ReimaginedWorkshop/bank.txt`. This generated manifest pins the actual Misc/ItemTypes/Inventory TXT catalogs used for native material routing, and its expected hash is embedded into the three original archive identity comparisons. Unrecognized source hashes or patch bytes abort preparation. `PORT.json` records the generated manifest hash.

Startup validation: core=1, loader=1, archive=1; first-pickup deposits ready. MapSense loaded with native seed atlas, monster markers, Direct navigation and Reveal Map ready. Its original map helper and dependency licenses are shipped. Gameplay deposit/map behavior still requires an in-game test. Only natively supported material routes are used; no FMMP custom relic library or encounter plugin is installed.

## Native menu and Runeforged Chronicle (2026-11-18)

The lower PauseTableExtra Loot Filter/Chronicle duplicates are removed; the upper Garden menu entries remain. Chronicle category 2 is Runeforged: all 220 recipe identities map to unique native Chronicle IDs. Ordinary runeword crafting stays disabled (Runes.complete=0). All recipes use private IDs 61000-61219, separate from old Runeword discoveries and from each other.

The native cube completion hook records the returned item for its player after output creation succeeds. Property application receives a null unit and is not used for player attribution. Existing forged items are also recorded on native pickup. Native Chronicle insertion suppresses duplicate recipes and retains the first timestamp. Unique/Set handling remains native. Legacy marker-1 items cannot be attributed reliably and are not guessed.

Validation: DLL build and fixtures pass (220 identities, invalid/legacy markers, success/failure cube output, repeats, packets, pickup delegation). Catalog startup audit passed for 220 distinct IDs. Hook entry signatures checked against build 93847. A real in-game forge/save/reload cycle remains to be verified.

Monster display now validates Workshop bank.txt instead of a separate FMMP archive. The same manifest refresh updates both pickup and monster-display binaries; native loader/core guards are unchanged.

The mod-scoped `workshop-keep-equipment.json` patch guards one contiguous 163-byte block of D2R 3.4.0 CreateDeadBody at RVA 0x42C240. Two jumps skip collection of body locations 1–12 (including weapon swap) and belt relocation; the cursor collection before this block, corpse creation/XP and gold handling after it remain native. The unchanged bytes inside the block are also guarded, so a different executable cannot receive a partial two-site patch. Items are never copied back from a corpse. Live death/save/reload validation is still required.

## Runeforged discovery chat names (2026-11-25)

The native client notification (RVA 0x2317e0) builds a synthetic socketed item via 0x232440. Because ordinary runeword crafting remains disabled, the name formatter can return Gemmed <base>. The packet already contains the correct recipe identity; changing its item ID does not address this path.

A scoped thread-local notification recipe selects the forge name. The existing name dispatch slot at 0x3e2acc0 is wrapped, with an exact return-address check for the notification call at 0x231962. Every other call delegates unchanged. Native localized message formatting, item links, toast, catalog, duplicate suppression and persistence remain native. Entry/call/thunk signatures are guarded against build 93847; no item flags or saves change.

Validation: all 220 recipe IDs, category/unknown-ID fallback, wrong-caller exclusion, nested-context restoration, existing discovery fixtures, snapshot byte guards and plugin build pass. Installed DLL hashes match staging and repository; in-game notification still requires verification on a newly discovered recipe.

## Trial pickup, Night pacing, Big Shot and Labyrinth (2026-11-25)

Blank Trial entry seals Wt1-Wt5 now use the observed-ground-pickup deposit retry. Existing native Materials slots and stackability are retained. Crafted Wa1-Wa5 seals are intentionally not stacked: they contain individual modifiers. Withdrawn or inventory-created items never qualify for automatic redeposit.

Night of the Dead lasts 45 seconds. Six-enemy waves are scheduled at absolute 37/38-frame intervals (1.5 seconds at 25 Hz), retaining 30 planned waves / 180 enemies and existing living caps/placement retries. Breach and Beyond durations remain unchanged.

Big Shot is selectable on left click. Mana is 10 + 0.5 per additional skill level, represented in exact 1/256 units by both the skill table and server debit. Its tooltip uses native usmc/256 formatting. Failed casts do not consume mana, and maximum mana is untouched.

The 57 Labyrinth variants with blanket 200% resistances now use their named non-Labyrinth Hell counterpart plus 15 percentage points for physical, magic and all four elemental resistances. The explicit mapping in endgame/labyrinth_balance.py is idempotent and runs in Prepare and Generate. Other monster rows and other difficulties are untouched.

Validation: combat build and suite, complete Workshop Check, all five Trial entry-code pickup fixtures, 30-wave pacing, exact level-2 10.5-mana cast and failure boundary, both table banks and idempotence passed. Source/staging/installed DLL and table hashes verified; live gameplay remains unverified. The separate Reimagined Maps integration was omitted at the user's request after identifying overlapping map/trial level IDs; no Maps DLL or new map drops were installed.


## Split Arrow and Trial loot (2026-11-25)

Split Arrow divides the finalized initial physical and elemental payload by three, after skill bonuses. Split descendants inherit that payload at 100%; cold and poison durations are preserved. The first native movement/hit applies the reduction once. Tooltip and physical character-sheet formula reflect the reduction.

Trial native loot rolls multiply the existing wave/modifier roll budget by the starting-stat tier factors 1.2 / 1.5 / 1.9 / 2.4 / 3.0. Fractional rolls use a random chance, so this increases opportunities for every native branch, including high runes and mythical uniques, rather than guaranteeing a quality. Existing MF and wave/modifier bonuses remain active.

Trial materials now use independent named pools for five Worldstone shards, three Uber keys and five Ancient statues. Each category has the same 1/5000 probability per material roll before player-count NoDrop adjustment and Trial rerolls. This preserves the former aggregate shard chance; native Labyrinth tables are untouched. Prepare and Generate apply the idempotent table update.

Validation: complete data checks, native combat build/suites, 100,000 simulated loot rolls per tier, native drop/quest guard tests and full split-generation payload inheritance passed. Local installation backed up under archive/local/backups/split-trial-loot-20261125-193644; six deployed files verified by SHA256. In-game playtesting remains outstanding.


## The Gull (2026-11-25)

Original Mythical Bone Visage, level 90, strength 100: 150–200% enhanced defense, 80–120 life, 15–20 all resistances, plus a random positive shrine buff on elite kill and 50% increased shrine effect/duration. Kill buffs last 450 seconds and replace the previous helmet buff. The ten durable shrine types are supported; instant shrines are excluded. Player-owned missile/summon kills count. An observation-only lifecycle listener queues grants for the next server tick; duplicate death events and non-elite/friendly kills are rejected.

Natural shrine timers extend once, and a separate short-lived stat list applies amplification without recursively multiplying bonuses. Unequipping removes amplification; existing duration is retained. Native whole-number stats round down. Stat 266 was audited unused/unsaved before defining a one-bit equipment flag; existing saved widths remain unchanged. New states are append-only 361–371, unique ID 1566 and base code Y81. The active Trial Mythical pool now has 54 equally weighted items.

Inventory art is original AI-generated RGBA (built-in image_gen), retained at src/workshop/mythicals/gull-art/the-gull.png with its prompt. Ground/equipped visuals use an existing native headgear rig. Two native inventory sprites and Mythical glow are included.

Validation: complete Workshop Check (including Mythical idempotence/reachability/save bounds), full native combat build/suite and targeted ten-buff/450-second/equipment/expiry/death/elite attribution tests passed. 22 local game files backed up and SHA256 verified. Backup: archive/local/backups/the-gull-20261125-195709. Live in-game testing is outstanding. No GitHub or website publication.

## Release 0.26.0

The earlier Gull prototype above is superseded: 60-second kill buffs, concurrent shrine types, and 64 active Mythical variants. Current behavior and validation are described in [release notes](../../RELEASE-0.26.0.md).
