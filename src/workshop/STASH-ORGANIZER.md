# Shared stash organizer

Click the **Organizer** button to the left of the page navigation in the open Shared Stash. The organizer is a native in-game panel; F9 is untouched.

- The header shows the current page name, favorite star, color and Softcore/Hardcore metadata profile.
- Enter a new name in **Page name** and click **Save name**. An empty name restores Unnamed.
- **Favorite / Unstar** and the six color swatches immediately save changes to the current page.
- Enter a page from 1 to 1000 and click **Go**. Names/colors/favorites are separate for the automatically detected character SC/HC profile and current/previous season.
- Search is case-insensitive and accepts multiple words. English base/unique/set names, runeword name IDs, quality and item codes are indexed. Delete the search text and click **Search** to remove the filter. Random rare names and affix text are not indexed.
- The visible current-season page refreshes automatically on the UI thread after navigation settles. The applied result list updates after captures; typing alone does not change the filter. Click **Search** to apply the draft. Previously visited pages remain searchable during the session. The counter identifies coverage; unvisited pages are not searched.
- **Scan / Stop** visits pages without moving items. Results appear progressively. Stop, manual navigation or selecting a result cancels scanning; it does not return to the starting page. A full 1000-page scan takes several minutes.
- Clicking a result opens its page and keeps the organizer open. Search matches get a gold outline; other items are dimmed. The page name/color remains visible below navigation. Reopen the menu with the Organizer button to edit/clear the search.
- **Previous / Next** browse ten rows at a time. **Only starred** filters pages.
- Metadata stays in `%LOCALAPPDATA%/AstralWorkshop/stash-pages.ini`. No character/shared-stash save files are changed. Item search is a session snapshot and requires loader support for shared-page identities. Previous-season item indexing is disabled.

Validation: native plugin compilation, model regression tests for input/search/navigation cancellation, generated panel/action/sprite/layout validation. Opening/input/rendering and enumeration coverage still require an in-game check. Item marking coordinates follow the shipped HD mouse/keyboard profile (16 by 13 cells); controller/legacy layouts are not implemented.

Compatibility correction (2026-11-20): results, status and colored titles now use native TextBoxWidget labels and the loader-resolved engine SetText descriptor (0x86DD50), rather than OverlayService text that reports success but is invisible in this installation. Labels update on the UI thread. A thread-local WH_GETMESSAGE guard resolves the focused organizer input for each keyboard event; it translates typing into WM_CHAR before suppressing game keybinding events, with Escape passed through. The hook is removed on plugin unload. Native text/focus descriptor adaptation follows the existing MIT-licensed d2r-access reference. This legacy path and native colors still require the user's in-game verification. Inventory item outlines/dimming still use OverlayService and are not confirmed visible on this loader.

2026-11-20 follow-up: removed inline color escape strings and Refresh. Page colors are native swatch images beside titles/results. Item marking uses a native 17-frame cell sprite in SharedStashTabContainer: edge masks join across multi-cell items, nonmatches use a translucent black frame. It no longer depends on OverlayService rendering. The frame setter is checked against its seven-byte reference fingerprint before use.

The three text controls now use Workshop-owned editing buffers displayed in native buttons/text labels. The window-thread message hook translates printable keys once into WM_CHAR, consumes those characters into the editor, and handles Backspace/Delete/Home/End/arrows/Ctrl+A/Ctrl+V/Tab explicitly. Escape reaches the game; clicking another control or the background exits editing. Search draft and applied query remain separate. The user performs live tests; native placement/rendering and keyboard routing have not been tested in-game by the agent.

Current controls: Make Favorite / Remove Favorite; Show Only Favorites / Show All Pages; Change Page Name. Scan and manual SC/HC controls are removed. Enter in Search applies the filter; Enter in Page executes Go. The character save-pool bit is read on the game thread using the already verified Nightmare client adapter, then metadata is loaded on the UI thread. Result navigation keeps the organizer open and refreshes the editable page name/number.

Organizer placement is right-anchored with a 20-unit right margin, leaving the shared stash visible in the shipped HD layout. Markings are visible only while the organizer is open. Selecting an item row narrows highlighting to that item (page, cell and name), including when several items match the search. Explicit close clears native sprites synchronously; Escape/native close is detected by the UI update and also clears them. Search/Clear reset the selected result.


## Sort buttons

Sort is available in the shared stash (current normal page only), inventory, and Cube.
Priority: tomes, charms, Horadric Cube, crafting materials, unique, set, rare, crafted, magic, superior, normal, inferior. Names are compared case-insensitively within each group. Items are packed into the first free position from top left. Full pages that cannot fit this order remain unchanged.

The shared stash deliberately sorts only the open page: the organizer index is not an authoritative enumeration of all 1000 pages. Tab names, favorites, and colors stay attached to their pages. Special material/rune/gem tabs are excluded.

Moves use ExistingItemTransaction on the authoritative game thread, preserve item handles and contents, use transaction-side ownership and destination validation, and refuse a held cursor item. Unsupported loader operations report a status below the button. No ground drops or save-file edits are used. Inventory is 10x8, Cube 12x8, shared page 16x13 per this mod's inventory table.

Validation: item_sort_test.cpp exercises category order, case-insensitive alphabet, collision-free packing, idempotence, a full 208-item page and failure preservation. Live game tests are performed by the user.


### Sort and affix search follow-up
Sort uses the same 240x64, Georgia/gold sprite style as Organizer. Removed the redundant cross-thread getItemInfo comparison: shared page metadata is supplied by inventory enumeration and is not a reliable comparison across read paths. Transactions remain responsible for validating moves. Rejection codes and failed operation indices are logged.

Search indexes actual nonzero layer-zero item stats with English descriptions from ItemStatCost/localized strings, using the byte-guarded legacy client accessor already used by forged item tooltips. Identified items support searches such as enhanced weapon damage and all resistances (the latter requires all four positive resistance stats). Name display remains separate; matching and highlighting both use name plus stat text. Layered skill/proc descriptions are not yet fully indexed.


### Authoritative sort handles and result scrolling
The sort planner now enumerates items and creates the full move transaction within one authoritative game-thread callback. Client UI handles are not reused for mutations: the installed loader's handle resolver checks runtime ID and item seeds against the calling thread's unit. Error 9 is StaleHandle, not a packing failure. A mock-service regression test supplies different client and server handles and verifies only server handles reach the transaction.

Mouse wheel over the result rows queues the same previous/next result-page actions as the buttons. High-resolution wheel deltas accumulate to one page per notch; outside the result area normal wheel behavior remains. Hit testing follows the right-anchored organizer at the current client size.


### One-click client synchronization and packing
After a successful move, wait 250ms across UI callbacks, then publish the same final positions once more using the transaction API. This mirrors the observed second-click repair without running the sorter twice. Re-enumerate authoritative items before publication and skip the entire refresh if the item count, runtime identity or any target position changed. No refresh follows a failed sort.

Packing now uses bounded backtracking (20,000 placements), revisiting earlier cells when a larger item cannot fit. Item priority, alphabetical iteration and unrotated dimensions remain unchanged. If the budget is exhausted or the order still does not fit, no move is submitted.

Inventory Sort is at the bottom left next to the gold area in both original and expansion HD layouts; Cube Sort is in the upper right above its grid. Both use the Organizer-style sprite.


Sort alternates row-first and column-first placement on successful clicks, independently for shared stash, inventory and Cube. The first sort after plugin load is row-first. Failed sorts retain the pending direction; the automatic client refresh does not advance it. Rarity and alphabetic priority stay the same.
