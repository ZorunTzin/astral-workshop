# Belts, Split Arrow and Trial casters — 2026-11-28

- All 39 mythical belt bases use the native belt primary type, retain their private mythical type as secondary, and retain the native Sash/exceptional family for potion UI resolution. Elite identity and 2x1 inventory size are preserved; belt index 6 provides the full 16 slots.
- Split Arrow applies one sixth instead of one third of its finalized damage, halving physical and elemental damage once before branching. All four split generations inherit this payload; cold/poison duration is unchanged. Tooltip and character damage formulas match.
- Appended private Burning Soul, Banished vampire and Stygian Doll Shaman variants to the Trial pool. Existing IDs remain stable. Native AI/skills, arena awareness, Trial scaling and existing Trial loot routes are used.
- Validation: native builds/tests, complete split-generation payload regression, mythical drop reachability, and idempotent belt/caster data checks. Potion panel behavior still needs in-game confirmation.
