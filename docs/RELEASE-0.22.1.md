# Astral Workshop 0.22.1

- New Sorceresses start with +1 Fire Ball on their starting orb instead of the obsolete Fire Bolt slot. Existing items are unchanged.
- Equipment, weapon-swap slots and belt contents stay on the character on death. The cursor, corpse creation and existing gold/experience rules retain native behavior.
- Monster health/resistance display now validates the shipped Workshop manifest instead of requiring a separate Frankenstein installation. Native runtime compatibility guards remain intact.
- Launcher 0.2.0 allows one instance per Windows session, restores its window on repeated starts and prevents repeated game starts while the loader is active. UI errors are reported and written to launcher.log.
- Open saved characters uses the Windows Saved Games location and the installed mod savepath, including redirected folders and ReimaginedWorkshop-v2.

Install the new AstralWorkshopSetup.exe for launcher fixes. Play/check-and-repair updates the mod; it does not replace the launcher executable.

Validation: mod data regression suite and 20 launcher/update/path checks passed. Native startup confirmed the guarded death patch applied and the monster display activated with the Workshop manifest. A full death/respawn/save/reload session and another player's installation still need in-game validation.
