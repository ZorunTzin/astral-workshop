# Arrow publication and return endpoints — 0.15.7

0.15.6 did not solve the reported visuals. Live reads confirmed its new tables were loaded. The player clarified two separate symptoms: one click shows a non-returning wave followed by a returning wave, and the returning arrows continue behind the Amazon. Do not reinterpret these as only an overshoot.

The explicitly published private arrows now disable automatic ClientSend room synchronization, removing another possible duplicate publication route. Client skill prediction remains disabled. This is not yet an in-game confirmed solution for the extra wave or invisible Split Arrow.

Returns now use an explicit lifetime calculated from Euclidean distance to the caster position at return creation. Native creation scales the fixed velocity 24 by 75% to 18 path units/frame; 16 path units represent a world tile. Flooring the travel time stops at or just short of the destination, rather than using the native approximate-distance 0x400 lifetime. The final gap is below one movement step for ordinary return distances. The return still aims at the caster position when it starts, not a continuously moving target.

Tests cover diagonal and axis endpoints across a grid, explicit lifetime flags, single publication, generated ClientSend settings and unchanged half damage. Player confirmation remains required.
