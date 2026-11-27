# Big Shot prototype

New level-30 Amazon bow/crossbow skill, bottom-right bow-tree slot, prerequisite Strafe. Split Arrow is retained. Dedicated Big Shot artwork uses frames 68/69, with normal and dim states in HD and low-end atlases.

A massive arrow travels in a fixed aimed direction at Vel 6 (one quarter of ordinary arrow speed), for up to 125 frames. Native barrier collision stops the carrier; monsters do not stop it. The HD arrow model is widened 32-fold and lengthened sixfold (eight times the prototype width, unchanged length). Swept segment checks use a four-subtile half-width and permit one direct hit per enemy per arrow, avoiding tunnelling and repeated stationary contact damage.

Physical shockwave novae originate at the carrier's current position about every half second (ten possible pulses over five seconds). Each nova affects hostile monsters within nine subtiles/six yards, once per pulse. Visual ring missiles carry no collisions; dedicated native hit missiles carry the actual damage, normal resistance handling and player kill credit.

Initial balance: 25 mana, no cooldown, 33 1/3% weapon damage plus 1 2/3 percentage points per additional skill level; each nova receives 50% of the arrow payload. Direction is fixed at cast time. Failed creation consumes no mana. Carrier expiry, wall collision, owner death, disconnect and area changes stop future pulses.

Validation: native mocked tests cover direction, failed spawn rollback, immediate concurrent casts and exact one-third weapon damage, swept width, direct-hit deduplication, ten repeated novae, half damage, enemy/area filters, wall removal and death cleanup. Data checks cover the independent skill slot, preserved Split Arrow, private projectile tables, HD model scale and labels. Actual HD appearance and remote nova rendering still require in-game testing.

Big Shot now uses the identical flat damage curve and fixed-point scaling of Amazonian Unity, with 8% physical damage per hard point in Multiple Shot, Split Arrow and Strafe. Flat damage is retained in full after scaling the weapon component, and the nova inherits half of the finalized arrow payload.

Split Arrow now appends a fourth split (1 -> 2 -> 4 -> 8 -> 16), preserving all previous IDs. All five stages use the standalone workshop_split_arrow mesh. Client prediction is disabled; server missiles publish explicitly once after initialization, with split children anchored at their own position. Actual visibility still requires an in-game check.

Client expiry fix: the captured native 3.3.93847 handler table at RVA 0x2358430 assigns client function 1 to 0x1b5380. It decrements remaining time, but at 0x1b56f4 calls 0x34f8d0 and skips removal unless unit+0x124 bit 21 (ISASYNC) is set. A byte-guarded wrapper enables this flag only for the Big Shot carrier on the client. The original handler owns expiry/deletion; the wrapper never touches the potentially freed unit after forwarding. Server 125-frame cutoff, ownership, damage and ordinary missiles are unchanged. In-game disappearance remains to be verified.
