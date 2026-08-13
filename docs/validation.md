# Validation

## Verification layers

Bomb Flip separates checks that can run on an ordinary C compiler from checks that require the RIVES SDK. The host suite tests rule and matrix invariants directly; the RIVES build remains the integration gate for packaging, audio and frame rendering.

## Strict host compilation

```sh
make -C src strict
```

This compiles every production translation unit to an object file as C11 in the default, `DEBUG_MODE=1` and `CHEATS_ENABLED=1` configurations with:

```text
-Wall -Wextra -Wpedantic -Wshadow -Werror -fanalyzer
```

The object builds are kept separate under `.strict-build/debug0` and `.strict-build/debug1`, so warnings that appear only during real compilation, such as an unused static function, cannot be hidden by a syntax-only pass. The modular source completes both configurations without diagnostics.

## Host-side rule tests

```sh
make -C src test
```

The test executables use a deterministic RIVES test double and cover the following properties.

### Board and mathematics

- all twelve configured tuples contain exactly the documented numbers of bombs, ×1, ×2 and ×3 cards;
- row and column value sums agree with `N - B + X2 + 2 X3`;
- both row and column bomb totals equal the configured bomb count;
- the completion predicate changes only after every ×2 and ×3 is revealed;
- scanner cells are safe, distinct and present in the correct number;
- the same entropy seed gives the same board and scanner positions;
- scanner allocation still succeeds through its deterministic fallback when random selection repeatedly proposes an invalid cell.

### Outcomes and corrected edge cases

- fold banks `floor(levelCoins / 2)` in addition to earlier levels;
- bomb loss preserves previously banked coins;
- timeout resets both coin totals and clamps remaining time to exactly zero;
- the timeout explosion chain advances without an internal `riv_present()` loop;
- scanner 2 can unlock uses before scanner 1 on levels 9–12;
- a selected bomb freezes the timer and therefore cannot turn into a timeout during its animation;
- simultaneous A1 and either Fold-menu shortcut cannot fold after a bomb or the last required card;
- W/SELECT and F/R2 both open Fold, Z/A1 confirms it, and either opener closes it;
- START/E is ignored by the Fold dialog and remains reserved for starting the run;
- the scanner preview performs its 59 modal presentation frames without consuming additional countdown time and returns the card face down;
- fold confirmation and cancellation consume only the ordinary outer-frame timer tick and do not poll sequenced music; cancellation leaves the outcard unchanged, while confirmation writes the terminal result;
- the cheat completion helper does not score an already revealed safe card twice.

Expected output:

```text
board invariants: ok
game outcomes: ok
```

## Deliberately modal timing

Scanner preview and fold confirmation retain their internal `riv_present()` loops by design. While either is open:

- `game_update()` is not called, so the countdown is frozen;
- `audio_poll()` is not called, so sequenced background music is frozen;
- the main-loop outcard update is not called.

The scanner host test verifies its 59 modal presentation frames, no additional modal countdown and final face-down state. The fold test scripts confirmation and cancellation and verifies no additional modal countdown, sequencer poll or cancel-path outcard update. A full input tape remains the appropriate integration test for rendering and sound.

## Intentional entropy-path change

The application formerly generated a complete level during initialization and discarded it when the run began. The maintained version generates the first board only when Z/A1 or E/START starts the run. This removes unnecessary random calls and means that a fixed replay entropy stream can produce a different first board than the earlier cleaned build.

The probability law is unchanged: card positions still use the same fixed-count rejection sampling. The new same-seed host test defines determinism for the maintained path. Scanner selection preserves its ordinary random call order and adds a non-random fallback only after one hundred failed attempts.

## RIVES build and smoke test

With RIVEMU and the RIV SDK installed as described in the README:

```sh
make -C src clean all \
  RIVEMU="$HOME/.riv/rivemu -mem=1024"

"$HOME/.riv/rivemu" \
  -mem=96 \
  -quiet \
  -no-window \
  -no-audio \
  -stop-frame=180 \
  -cartridge=src/bombflip.sqfs

stat -c '%n: %s bytes' src/bombflip.sqfs
```

The 1,024 MB override applies only to the SDK processes that compile, link and package the cartridge. Runtime compatibility is checked separately at the official 96 MB limit.

The final compatibility pass on 13 August 2026 used RIVEMU and `libriv` 0.3.0. The six modules compiled with `riv-opt-flags -Ospeed`; the linked executable was processed by `riv-strip` before packaging. The resulting `bombflip.sqfs` ran headlessly for 180 frames with `-mem=96` and measured 69,632 bytes. This is 454,656 bytes below the 524,288-byte cartridge limit.

The same cartridge was loaded in the [official RIVES web emulator](https://emulator.rives.io/). The title screen rendered, Z started a run, keyboard movement changed selection, reveal input produced the expected bomb animation, and the browser console reported no errors.

The earlier single-file baseline was built with RIVEMU 0.3.0 and RIV OS SDK `v0.3-rc16`. A later comparison audit built and launched commit `4d53231` with RIVEMU/libriv 0.3.0 by compiling the six modules separately, linking them and packaging the cartridge. The maintained Makefile now uses that same per-module build structure instead of one monolithic compiler invocation.

The basic web-emulator check is not deterministic. The short documentation replay is deterministic, but it is not a full run. The interaction paths below remain appropriate candidates for longer recorded input tapes.

## Documentation-media verification

The mathematical diagrams are self-contained SVG files and add nothing to the packaged cartridge. Their XML and raster previews were checked with:

```sh
xmllint --noout \
  docs/media/bomb-flip-clue-model.svg \
  docs/media/bomb-flip-level-progression.svg

magick -background none \
  docs/media/bomb-flip-clue-model.svg \
  /tmp/bomb-flip-clue-model.png

magick -background none \
  docs/media/bomb-flip-level-progression.svg \
  /tmp/bomb-flip-level-progression.png
```

The clue diagram uses the exact level-9 totals `B=13`, `X1=13`, `X2=7`, `X3=3`; its row and column clue totals both give value sum 36 and bomb count 13. The progression bars reproduce the twelve tuples tested by the host board suite.

The README gameplay media comes from the normal cartridge build with `DEBUG_MODE=0` and `CHEATS_ENABLED=0`. `docs/media/bomb-flip-gameplay.rivlog` fixes the input sequence and the cartridge was replayed with entropy `5f2e81b4c393d12ae54ef21fc97aa3bf5723e319743a19c48fd93df3d625cb07` at the official 96 MB runtime limit. The replay covers the title, run start, board navigation, Fold open/close, a revealed Scanner and its preview. The tile-reveal section samples every third game frame, preserving each 15-frame flip at 20 fps. The native 256 × 256 captures were enlarged to 768 × 768 with nearest-neighbour sampling; the game pixels were not redrawn.

Media dimensions and animation frames were checked with:

```sh
identify docs/media/bomb-flip-gameplay.png \
  docs/media/bomb-flip-fold.png \
  docs/media/bomb-flip-gameplay.gif
```

## Interactive checks still required

The following paths benefit from a RIVES input tape because they combine timing, input transitions, rendering and audio:

- normal reveal and level completion;
- bomb selection during the last visible second;
- fold confirmation and cancellation;
- scanner 1 and scanner 2 acquired in either order;
- timeout chain animation;
- transition from level 8 to the 6 × 6 level 9;
- final completion and title easter egg.

These checks should compare both screenshots and outcards. Historical screenshot hashes from the pre-refactor title sequence are not used as acceptance criteria because the formerly unreachable periodic explosion sound was intentionally corrected and the first-board entropy path changed.
