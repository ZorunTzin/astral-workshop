# Monster balance and HD Split Arrow — 0.17.4

Each distinct modifier adds 5% damage and 25% life, additively. Raid expansion still counts each of its 23 effects once. Gravity pulls within 24 subtiles (16 yards), moving two thirds toward the source through native collision-checked relocation; cadence, slowing and burst damage remain unchanged.

Private hostile spells disable their inherited client proc spell replay and automatic ClientSend. The existing explicit server missile publication owns their visuals, avoiding an extra cast at the victim. Native lightning-on-hit bolts and cold death nova remain unchanged.

The player's live test confirmed Split Arrow visible in legacy graphics but invisible in HD. Read-only samples recorded the initial projectile and all three child generations. All four retain their existing client/server homing handlers and use a dedicated HD unit with an explicit visible arrow mesh and a fixed elevated transform, without launch-dependent tracking or particles. This is a rendering correction, not an additional damage projectile.

Automated checks validate balance, non-compounding life updates, generated spell flags, split mesh/mappings and unchanged guided handlers. In-game HD visibility and hostile spell origins still require confirmation after installation.
