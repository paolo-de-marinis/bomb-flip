# Validation

Bomb Flip has two validation layers: host-side C checks for rules and state transitions, and RIVES checks for the real build, package and runtime.

## Host compilation

~~~sh
make -C src strict
~~~

All six production modules are compiled separately in three configurations:

- normal release;
- `DEBUG_MODE=1`;
- `CHEATS_ENABLED=1`.

The check uses:

~~~text
-std=c11 -Wall -Wextra -Wpedantic -Wshadow -Werror -fanalyzer
~~~

## Automated tests

~~~sh
make -C src test
~~~

`test_board.c` verifies:

- all twelve level compositions;
- row and column value sums and bomb counts;
- the ×2/×3 completion predicate;
- safe, distinct scanner placement;
- repeatability for a fixed entropy seed;
- the scanner fallback after repeated invalid random choices.

`test_game.c` verifies:

- Fold banks half of the current level in addition to earlier levels;
- a bomb preserves only previously banked coins;
- timeout sets the run score and remaining time to zero;
- timeout explosions advance through the outer frame loop;
- either scanner can be found first on 6 × 6 levels;
- bomb reveal freezes the countdown;
- simultaneous reveal and Fold inputs cannot reopen a decision after a terminal reveal;
- both Fold shortcuts open and close the dialog, while Z/A1 confirms it;
- scanner preview turns the card face down again and consumes no countdown time;
- Fold confirmation and cancellation consume no modal countdown or music polls;
- the cheat helper does not score an already revealed safe card twice.

Expected output:

~~~text
board invariants: ok
game outcomes: ok
~~~

## Modal timing

Scanner preview and Fold confirmation contain their own `riv_present()` loops. While either is open, `game_update()` and `audio_poll()` are not called. The countdown and sequenced background music therefore pause by design.

The host test counts the scanner's 59 modal presentation frames and checks both Fold confirmation and cancellation.

## Entropy-path change

The maintained application creates the first board only when the player starts the run. An earlier cleaned version created and discarded a board during initialization. Removing that unused generation changes the random-call sequence for a fixed replay seed, but not the board distribution.

Scanner placement retains its ordinary random search and adds a deterministic fallback only after one hundred unsuccessful attempts.

## RIVES build

~~~sh
make -C src clean all
make -C src smoke
~~~

The latest recorded build on 13 August 2026 used RIVEMU/libriv 0.3.0 and RIV OS SDK 0.3.0. The six modules were compiled with `riv-opt-flags -Ospeed`, linked, stripped and packaged.

`bombflip.sqfs` completed a 180-frame headless run at the official 96 MB runtime limit and measured 69,632 bytes, below the 524,288-byte cartridge limit.

The same cartridge was also opened in the official web emulator. The title rendered, Z started a run, movement changed the selection, a bomb reveal played and the browser console reported no errors.

## Documentation media

The README animation comes from the normal build with debug and cheats disabled. The input log in `docs/media/bomb-flip-gameplay.rivlog` fixes the sequence, and the replay uses entropy

~~~text
5f2e81b4c393d12ae54ef21fc97aa3bf5723e319743a19c48fd93df3d625cb07
~~~

It covers the title, run start, navigation, Fold open/close, scanner reveal and scanner preview. The 256 × 256 frames were enlarged with nearest-neighbour sampling; the game pixels were not redrawn.

## What is not covered

A longer input tape or human playthrough is still needed for the complete level progression, both scanner orders in live rendering, the final ending, audio timing and screenshot comparison across all transitions.
