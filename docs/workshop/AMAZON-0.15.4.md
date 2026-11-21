# Amazon projectile correction — 0.15.4

The player reported that 0.15.3 did not restore the arrow visuals and that Multiple Shot no longer dealt return damage. The previous automated checks did not cover the complete native allocation layout or real client rendering.

## Changes

- Allocate and zero the full native missile parameter tail. The native creator at RVA 0x5371a0 checks the graphics descriptor at offset 0x84 (presence byte 0x8c) and may copy 16 bytes. The previous 0x78-byte allocation allowed an out-of-bounds read of adjacent stack memory. The block now reserves 0x98 bytes, preserving all earlier field offsets.
- Publish generated return and split arrows with the native volley routine at RVA 0x4640f0. Its signature and arguments were checked against native call sites. It handles the owner and room clients, replacing repeated single-missile room-sync calls through the player-unit hash table.
- Refresh outgoing arrow damage before its native update, after the native skill has finished initialization. Return arrows copy half of this final snapshot. Duration channels remain unchanged.
- Return arrows use an explicit destination coordinate rather than assigning the player as a collision/path target.
- Bounded diagnostics record the return damage payload and hostile return collisions.

## Validation

Regression tests poison adjacent parameter storage and check the complete graphics tail, verify damage changes after creation survive into the return snapshot, and check one volley publication after native Guided Arrow initialization. These checks do not establish in-game visibility or successful return hits. A player test is still required for both Multiple Shot and Split Arrow.
