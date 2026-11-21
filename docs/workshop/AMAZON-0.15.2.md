# Amazon projectile corrections — 0.15.2

- Multiple Shot schedules one half-damage return at the native flight-end event, including collision ends. It no longer depends on a successful pierce or an item-table lookup.
- Split Arrow child casts use native Guided Arrow initialization; all progressive missile slots and client slots reference the private projectiles.
- Plague Javelin supplies both client fan missiles again.
- Charged Strike supplies all three progressive missile slots and removes the native fan initializer after setting radial server directions. Its private impact nova is also assigned on the client.
- Pounce uses the established teleport helper, followed by one physical landing payload and a native replicated visual ring. The Amazon no longer attempts the Barbarian leap sequence.
- Toxic Whirl preserves exact server travel scaling of 5% per skill level. Client velocity uses the closest integer native VelLev value (6/8 velocity units per level); the display approximation is slightly below the server range. Count and damage scaling are unchanged.
- Bounded diagnostics separately record each projectile class, so testing one skill cannot consume all logs for the others.

Validation: Combat regression executable suite and generated table checks. Gameplay/visual confirmation is pending. The previous external memory recording was not a valid complete projectile capture; it is not counted as validation.

Native behavior reference: https://github.com/ThePhrozenKeep/D2MOO/blob/master/source/D2Game/src/SKILLS/SkillAma.cpp and MISSILES/Missiles.cpp. D2R addresses and callbacks are those already verified by the Workshop runtime.
