# Shared stash

The mod enables 1,000 shared pages (995 extra pages in RotW).

| Control | Click | Ctrl + click |
| --- | --- | --- |
| Back / forward | 1 page | 5 pages |
| Double back / forward | 10 pages | 100 pages |

Navigation stops at the first or last available page. Shared items and save formats are managed by D2RLoader; this feature does not rewrite saves.

The installed loader requires the registered `PanelManager:ClosePanel` layout message. Workshop-specific text identifies the action, which is consumed before native panel closing. Unknown custom message targets are not usable in this loader's layout parser.

Validation: compiled DLL and exhaustive page-boundary tests passed. In-game mouse test on 2026-11-20: 1 -> 2 -> 12 -> 2 -> 1, all confirmed in the plugin log. Ctrl stride mapping (5/100) is covered by native unit tests; Ctrl-click itself was not automated. The test session was saved and exited.
