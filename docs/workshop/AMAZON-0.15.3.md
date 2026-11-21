# Amazon arrow visuals — 0.15.3

The 0.15.2 gameplay logs confirm that Split Arrow children are created and hit targets through all three generations. Their client rendering was still missing.

Return arrows and Split Arrow children now explicitly synchronize their existing native missile to clients in visible rooms. This uses D2R's missile-sync routine at RVA 0x537bc0, not a second cosmetic/damage missile. Split children synchronize only after the native Guided Arrow handler has written its steering data. Return arrows synchronize after their damage snapshot has been halved.

The player-client lookup (0x48fde0), client room visibility predicate (0x537080), and sync entry point were checked against the native module snapshot. Startup verifies their instruction signatures. The existing server damage, branching limits and return timing remain unchanged.

Tests cover visibility rejection, owner-client delivery, native initialization before synchronization, unchanged copied damage, and one synchronization per created arrow. In-game visual confirmation remains pending.
