# Validation

Bomb Flip uses several kinds of evidence, and they should not be conflated.

A compiler check can establish that the selected translation units satisfy a specific warning
and analyzer configuration. A host test can establish a rule for the exercised inputs. A RIVES
smoke run can establish that the packaged cartridge starts and survives a short runtime path. A
human replay can observe presentation behavior that none of those checks proves exhaustively.

The validation structure is therefore

~~~math
\boxed{
\text{static compilation}
\longrightarrow
\text{targeted host invariants}
\longrightarrow
\text{RIVES runtime check}
\longrightarrow
\text{human observation}.
}
~~~

These layers provide different evidence about the same implementation. They are not successive
steps toward a formal proof of the whole game.

## 1. Host compilation

Run

~~~sh
make -C src strict
~~~

The production modules are compiled separately in three configurations:

- normal release;
- `DEBUG_MODE=1`;
- `CHEATS_ENABLED=1`.

The check uses

~~~text
-std=c11 -Wall -Wextra -Wpedantic -Wshadow -Werror -fanalyzer
~~~

and obtains `riv.h` from the installed official RIV SDK. The repository does not vendor that
header; the host checks copy it only into a temporary build directory.

Passing `strict` means that the tested source is warning-free under this invocation. It does not
establish runtime correctness, strategic correctness or absence of every possible undefined
behavior.

## 2. Targeted board invariants

Run

~~~sh
make -C src test
~~~

`test_board.c` exercises the board generator and clue model for every level using the production
`board_grid_size()` and configuration functions.

It checks:

- all twelve fixed level compositions on their active grid sizes;
- row and column value sums;
- row and column bomb counts;
- the x2/x3 completion predicate;
- zero scanner rewards on levels 1-3;
- one reward on levels 4-8;
- two rewards on levels 9-12;
- scanner cells are safe;
- two scanner cells are distinct;
- repeatability under the deterministic test entropy path;
- row-major scanner fallback after repeated invalid random selections.

These tests support the code-derived objects used in the mathematical document:

~~~math
A,
\qquad
\Gamma(A),
\qquad
M(A)\subseteq R,
\qquad
G\subseteq\{x:a_x>0\}.
~~~

They do not enumerate every possible board, independently prove the grid-size function for all
inputs outside the valid level range or certify that every runtime entropy source is uniform.

## 3. Targeted state-transition invariants

`test_game.c` exercises selected transitions of `GameState` with a deterministic RIVES test
double.

It verifies that:

- Fold adds half of the current `levelCoins` to already banked `totalCoins`, including integer
  truncation for an odd current score;
- a bomb does not bank the current level score;
- timeout sets banked score, current level score and remaining time to zero;
- timeout explosions progress through the non-active phase path;
- scanner availability is not tied to the first scanner slot: a level-9 state with the second
  scanner marked revealed and uses available remains scanner-usable;
- bomb reveal freezes the ordinary countdown path;
- simultaneous Reveal and Fold input cannot reopen a decision after a bomb reveal changes phase;
- the same phase protection applies when the reveal completes the board;
- both Fold shortcuts open/cancel the dialog while Z/A1 confirms it;
- scanner preview turns the selected card face down again and does not mark it revealed;
- scanner preview adds no timer decrement during its nested modal frames;
- Fold confirmation and cancellation add no timer decrement or outer music poll during their
  nested modal frames;
- the cheat completion helper does not score an already revealed safe card twice.

These checks are evidence for specific transitions such as

~~~math
(B,S)\xrightarrow{\mathrm{Fold}}
B+\left\lfloor\frac S2\right\rfloor
~~~

and

~~~math
(B,S,t)\xrightarrow{\mathrm{Timeout}}(0,0,0).
~~~

They do not prove every reachable `GameState` transition or every simultaneous-input ordering.

Expected host output is

~~~text
board invariants: ok
game outcomes: ok
~~~

## 4. Modal timing is checked as control flow

Scanner preview and Fold confirmation contain nested `riv_present()` loops. The mathematical
document describes their timer behavior as a consequence of execution order.

On the ordinary active frame that opens one of these interfaces, `game_update()` has already
executed

~~~math
t\leftarrow t-\frac1{60}
~~~

before input handling. The nested modal presentation frames then run without calling
`game_update()` again.

The tests therefore check the more precise statement:

~~~math
\text{time after modal action}
=
\text{initial time}-\frac1{60},
~~~

not "zero time passes on the opening frame."

The scanner test also observes 59 nested presentation calls for its exercised setup and verifies
that no outer `audio_poll()` occurs during them. Fold confirmation and cancellation similarly
exercise one and two nested presentation calls in their configured test paths.

Those presentation counts are implementation-level observations, not mathematical constants of
the underlying rules.

## 5. Entropy-path behavior

The maintained application generates the first board only when the player starts a run. An
earlier cleaned version created and discarded a board during initialization. Removing that
unused generation changed the random-call sequence for a fixed replay seed without changing the
fixed-count board construction.

Scanner placement uses random search followed, after one hundred unsuccessful attempts for a
reward, by a deterministic row-major fallback.

The documentation therefore separates two claims:

- **code-derived:** the active board has the fixed configured counts and is produced by repeated
  random free-cell selection;
- **assumption for probability formulas:** `riv_rand_uint()` is uniform on the requested range.

The tests check deterministic behavior for their supplied entropy path; they do not statistically
certify the external RNG distribution.

## 6. RIVES build and smoke run

Run

~~~sh
make -C src clean all
make -C src smoke
~~~

The latest recorded build on 14 August 2026 used RIVEMU/libriv 0.3.0 and RIV OS SDK 0.3.0. The
production modules were compiled with `riv-opt-flags -Ospeed`, linked, stripped and packaged.

`bombflip.sqfs` completed a 180-frame headless run at the official 96 MB runtime limit and
measured 69,632 bytes, below the recorded 262,144-byte upload limit.

This establishes a concrete build/package/runtime observation for that toolchain and path. It
does not replace a complete twelve-level playthrough.

## 7. Recorded visual replay

The same cartridge was opened in the official web emulator. The recorded checks included:

- title rendering;
- Z starting a run;
- selection movement;
- one bomb-reveal path;
- absence of browser-console errors in that run.

The README animation comes from the normal build with debug and cheats disabled. Its input log is
stored in `docs/media/bomb-flip-gameplay.rivlog` and uses the recorded entropy

~~~text
5f2e81b4c393d12ae54ef21fc97aa3bf5723e319743a19c48fd93df3d625cb07
~~~

The replay covers title, run start, navigation, Fold open/close and two safe-card reveals on
level 1. Levels 1-3 contain no scanner reward, so this tape does not demonstrate scanner
acquisition or preview.

The 256 x 256 frames were enlarged with nearest-neighbour sampling; the game pixels were not
redrawn.

## 8. What is derived from code but not isolated by a dedicated test

Some statements in the mathematical documentation follow directly from the current source even
though no test names them as a standalone invariant. Examples include:

- level start time
  
  ~~~math
  t_0(\ell)=45+5(\ell-1);
  ~~~
- safe-card update
  
  ~~~math
  S\leftarrow S+100v,
  \qquad
  t\leftarrow\min(150,t+3v);
  ~~~
- time bonus
  
  ~~~math
  \lfloor10t\rfloor;
  ~~~
- scanner reward adding the revealed reward-card value to `scannerUses`;
- the phase ordering that separates completion detection from time-bonus application;
- the exact fixed level-start times beyond the initial level;
- the fact that level clearing reveals remaining cards for presentation without returning to
  active input.

Scanner preview leaving `revealed == false` is stronger than a merely code-derived statement: the
current host test checks it directly for the exercised preview path.

These distinctions matter because a formula being visible in source is not the same thing as
having an independent regression test for that formula.

## 9. What is not established

The current repository does not establish:

- an optimal strategy for Bomb Flip;
- the conditional bomb probability of every visible position;
- the Bellman value of Reveal, Scan or Fold;
- a formal proof that the level table has a particular difficulty ordering;
- statistical uniformity of the external RIVES entropy source;
- exhaustive correctness over every reachable simultaneous-input combination;
- a complete visual/audio regression across all twelve levels;
- the final ending and every scanner-acquisition order in one recorded real-runtime playthrough.

The probability and Bellman sections in `mathematics.md` are therefore labeled as external
mathematical curiosities, not implementation claims.

A longer deterministic tape or human playthrough would still be useful for complete progression,
scanner acquisition from level 4, both scanner-reward orders on levels 9-12, the final ending,
audio timing and screenshot comparison across all transitions.