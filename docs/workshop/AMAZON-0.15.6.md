# Amazon projectile synchronization — 0.15.6

The player clarified that a single Multiple Shot attack produces two visual waves, only one of which returns. Split Arrow remains invisible in 0.15.5. The earlier visibility confirmation was therefore not a complete validation.

Disable the native client skill prediction for Multiple Shot and Split Arrow. Publish the fully initialized damage-bearing server missile once instead, using the native volley packet. Both the generic update and a frame fallback can request publication; a per-flight guard prevents duplicates. The fallback also covers Guided Arrow's separate movement handler. Split children retain their post-initialization publication. Return damage remains half the outgoing snapshot.

Child origin overrides now explicitly clear the relative-coordinate flag before assigning absolute coordinates.

Automated checks cover generated client/server skill separation and exactly-once publication. They do not prove visual correctness or return hits. In-game confirmation remains required for one outgoing wave per click, its return damage, and visible initial/child Split Arrows.
