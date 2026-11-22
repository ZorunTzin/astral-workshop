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
