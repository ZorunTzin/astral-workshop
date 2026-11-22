# Mercenary charms

The existing local Frankenstein mercenary-charms plugin is ported to ReimaginedWorkshop. Open the normal mercenary inventory (O) to access its combined equipment and 10 by 4 charm grid. Charms remain saved on the plugin-owned player page; the native plugin projects eligible bonuses onto the living mercenary, not the player. Its original page identity, item restrictions, duplicate checks, temporary bonus lifecycle and native version guards are preserved.

Only the exact active-mod identity string in the pinned DLL changes. PROVENANCE.json records all source hashes. No character save is edited during installation. The mouse and controller layouts come from the same installed Frankenstein version. Private Workshop art paths prevent changes to the player's inventory background.

Prepare and Generate reinstall this integration; Deploy backs up changed files and checks installed hashes. Static checks verified the identity-only binary diff, matching D2RCore/D2RLoader hashes, the 10x4 grid and controller inheritance. Actual charm transfer, saving/reloading, bonus changes and mercenary death/revival still need an in-game check in Workshop. The upstream binary itself labels live combat verification as pending, so static integration checks do not prove every charm affix works.
