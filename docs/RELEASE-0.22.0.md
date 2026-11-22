# Astral Workshop 0.22.0

League encounters, twelve crafting orbs and mercenary improvements.

- Night of the Dead, Astral Breach and Beyond roll independently at 12% per eligible area; multiple mechanics can coexist.
- Twelve tiered League Orbs add equipment-specific, stackable bonuses. Required level increases by 2/4/6/10; crafts exceeding level 120 are rejected. Tier 4 adds a secondary bonus.
- League ground pickups go to their dedicated stash. Withdrawn orbs stay in inventory. All four tiers can be withdrawn; the panel now uses three family rows and four tier columns.
- Orb tooltips describe equipment bonuses. Tiers 1-3 drop at one quarter of the previous chance; tier 4 is unchanged. Mythicals keep their ground glow without a sky beam. Forced test encounters/drops are disabled.
- Mercenaries gain the Frankenstein 10x4 charm inventory through the normal mercenary screen. Act 1 uses a separate native, timed Inner Sight; Act 2 uses Fend instead of Pounce.
- Inner Sight localization IDs no longer collide with Bleed/Impale text. Its tooltip includes defense reduction, physical resistance reduction, duration and radius.
- Sorceress fire prerequisites and positions: Blaze (6) -> Fire Wall (12) -> Conflagration (18).
- Includes level-120 progression and the current League combat/balance fixes.

## Validation and compatibility

Native builds and League runtime/crafting tests passed in the preceding implementation; generated-data checks and tooltip localization checks passed for this release. Mercenary charm persistence, individual charm affixes, native Fend combat and the corrected tooltip still need in-game confirmation in Workshop. Existing incorrectly crafted orbs are not retroactively reconstructed.

Uses the existing ReimaginedWorkshop-v2 save profile. Character saves are not included or modified by the update. Update through Play or Check and repair in the launcher. Existing launcher binaries are unchanged.

Compendium: https://astral-workshop-compendium.chattyactor2.chatgpt.site
