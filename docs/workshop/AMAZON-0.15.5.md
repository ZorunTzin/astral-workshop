# Split Arrow visual follow-up — 0.15.5

The player confirmed visible Multiple Shot returns in 0.15.4. Return damage was not explicitly confirmed. Split Arrow remained invisible, including its first arrow. Combat logs show children spawning and hitting through all three generations.

All four private Split Arrow generations now use the ordinary arrow HD mapping and legacy sprite settings that are visible for Multiple Shot, instead of the shared SafeArrow graphics. Native client/server Guided Arrow steering remains in place. The child volley packet also receives its actual homing target, while its origin remains the impact location. Multiple Shot is unchanged.

Checks cover all four HD/legacy mappings, preserved homing handlers, and the child packet target after native initialization. In-game Split Arrow visibility still requires confirmation.
