# Item format v2
The 28 selected vanilla stats use 20-bit saved values. Save Add, ValShift,
Save Param Bits and character CSvBits remain unchanged. Positive storage maxima
range from 1,048,475 (MF/GF) to 1,048,575 (no bias); this is a serialization
limit, not a promise that extreme combat calculations or skill levels are safe.
Nonzero Send Bits are widened to 32. Drop rolls and balance are unchanged.

The native modinfo savepath is ReimaginedWorkshop-v2/. Existing characters,
shared stashes and loader sidecars remain untouched in ReimaginedWorkshop/.
Do not copy legacy d2s/d2i files into v2. To play legacy saves use the backed-up
v1 tables/plugins and old modinfo together. Future changes to widths must bump
the format/profile again or supply an explicit tested migration.
