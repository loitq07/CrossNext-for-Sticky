# Hardware navigation feedback audit — 2026-11-10

## Root causes and fixes

1. `HalBuzzer` had three function-local `Buzzer` instances for click, beep and
   sleep. They competed for the same LEDC pin. Once click attached it, beep's
   first `begin()` failed; the sleep instance could not detach the live owner.
   All three operations now use one instance. The original failure is reproduced
   against the production SDK buzzer driver and a stub of Arduino's exclusive
   PWM attachment API.
2. Settings, font menus, library filters/preferences, shelf choices and shortcut
   configuration copied the normal row style into selected/focused/active.
   Their replacement marker was often identical to the separator already under
   every row. Shared list styles now use a solid black fill and white text.
   Controls on dark header bands use an inverse focused style.
3. Custom-painted controls registered hit rectangles without consulting
   `stateFor()`/`buttonSelected()`. Navigation advanced and Confirm worked, but
   the pixels stayed identical. `RevealedInteractions::paintButtonFocus()` now
   draws the shared white-and-black focus outline for ordinary controls as well
   as coordinate targets. Existing render paths call this through
   `toybox::reportOverflow()`.
4. Several nested settings screens explicitly disabled the shared side-button
   adapter and erased focus on every render. These gates are removed from
   Chess, Draughts, Go, Minesweeper, Connect Four, Othello, Solitaire/Hearts
   setup/settings, Picross size, Wordle length, Chess Puzzle difficulty/reset
   and Flashcards/Words text settings. Habits detail/menu/delete and Pomodoro
   settings/reset now route their queued commands through the same adapter.
   Paging and gameplay keep their existing mappings.
5. Touch capability was treated as a reason to hide hardware selection in Wi-Fi
   and reading-stat book rows. Visibility now follows input modality. Shared
   list activities own that state, including single-row lists. Wi-Fi password
   review/details use the same focus/Confirm adapter as other menus.
6. Revealing focus at an unchanged programmatic index did not request a repaint.
   The adapter now tracks focus visibility as well as position, and UiAppHost
   invalidates on focus input. Touch-to-key transitions clear stale tap flashes.
7. Feedback was duplicated in individual list/catalog handlers but absent from
   held navigation repeats. Initial physical presses remain owned by the input
   pump; accepted `ButtonNavigator` repeats emit feedback once. Redundant
   per-screen clicks are removed. Key Sound, vibration and volume settings are
   still respected.

The logical list selection is retained when its visual cursor is hidden. This
lets measured `ListNav` viewport feedback finish normally rather than repeatedly
trying to bring an invisible selection into view.

## Verification

Run `host-tests/navigation-feedback/run.py --compiler <C++ compiler>`; Zig is
supported directly. `--snapshots <directory>` also writes four PBM previews.

- Production buzzer driver: mixed click/beep at all three volumes, repeated
  sleep/restart and absent buzzer hardware pass.
- Production side-button adapter: first-focus repaint at an unchanged index,
  disabled-control skipping, Confirm activation and touch/key handoff pass.
- Production ButtonNavigator: exactly one feedback per held repeat, with none
  duplicated on the initial press or release, passes.
- List styles, Wi-Fi primary-action handoff, viewport feedback and touch/key routing: **1,650
  checks, zero failures**.
- Eighteen real game/Flashcards/Habits/Pomodoro settings builders: focus movement changes the
  framebuffer, retains a valid target through rebuilds and confirms that target.
  Both themes, portrait/landscape and English/Vietnamese: **23,674 checks, zero
  failures**.
- `compare_baseline.py` runs the wider settings UI suite in an isolated HEAD
  source tree and the working tree. Both have **199 existing failures** and no
  additional failure signatures. Generated translation files and the same SDK
  are shared between the two runs. Existing layout/ellipsis expectations are
  outside this feedback fix.
- The separate legacy `host-tests/side-navigation/test.cpp` reports 17 failures
  in its raw-SDK coordinate-grid expectations. That test and all its SDK
  dependencies are unchanged; production menu routing is covered by the passing
  adapter and framebuffer tests above.

Host tests verify rendered pixels and buzzer API behavior. Actual panel refresh,
audible volume and physical key handling still require verification on a flashed
device. This change does not flash or publish firmware.

Final `gh_release_sticky` firmware build: **SUCCESS**. Packaged app and factory
binaries, SHA-256 hashes, regression logs and build log are in
`../../output/navigation-feedback/`. All modified application sources were
verified to have newer compiled objects before packaging.
