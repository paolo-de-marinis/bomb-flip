# Bomb Flip code overview

This document describes the maintained implementation of the Bomb Flip RIVES cartridge. The program follows a procedural C design: state is represented by small structures, rules are expressed by named functions, and each source file has one principal responsibility.

The organization is deliberately didactic. A reader can study the matrix and its invariants without first reading the renderer, or follow the state transitions without entering the audio implementation. There is no object-oriented layer and no dynamic allocation in the game-specific code.

## Source map

| File | Responsibility |
|---|---|
| `bombflip.c` | application lifetime, RIVES console configuration and top-level screen dispatch |
| `state.h` | shared constants, `Tile`, `GameState`, `GamePhase` and `AppMode` |
| `controls.h` | canonical gameplay-button mapping and RIVEMU keyboard aliases |
| `board.c`, `board.h` | level configurations, grid generation, clues, scanner placement and board predicates |
| `game.c`, `game.h` | input, timer, reveals, score, fold, scanner use and outcome transitions |
| `render.c`, `render.h` | board, clues, status, dialogs, animations and ending panels |
| `title.c`, `title.h` | title screen, transition and delayed easter-egg sequence |
| `audio.c`, `audio.h` | background sequencer ownership and generated sound effects |
| `riv.h` | preserved RIVES API declarations used by production modules and host checks |
| `seqt.h` | reusable RIVES sequenced-audio helper; its implementation is instantiated only by `audio.c` |

The division follows dependencies rather than file size alone. `board.c` knows nothing about input or drawing. `render.c` reads game state but does not change rules. `game.c` owns rule transitions and calls the renderer only for the two intentionally modal interactions described below.

The preserved `riv.h` corresponds to the official [RIV API header](https://github.com/rives-io/riv/blob/main/libriv/riv.h). The RISC-V build still links the SDK's `libriv`; the local header also makes declaration checks and test doubles reproducible on the host.

`controls.h` keeps the action mapping in one place: Z/A1 starts the run, reveals and confirms Fold; E/START is an alternative start input; X/A2 activates a scanner; W/SELECT and F/R2 both open or close Fold. Reusing the ordinary action button for confirmation keeps START out of gameplay dialogs, while requiring a different button from the opener prevents an accidental double press from ending the run.

## Application and game state

`Application` in `bombflip.c` contains four explicit components:

- an `AppMode`, selecting title, transition, active play or ending;
- a `GameState`, containing all run and level data;
- a `TitleState`, containing title-only animation counters;
- an `AudioState`, owning the sequenced music source and playback identifier.

This removes the former dependence on overlapping file-scope booleans. At application level exactly one `AppMode` is active. Within a run exactly one `GamePhase` is active:

```text
ACTIVE
  ├─ bomb ───────────────> BOMB_REVEAL ──> FINISHED
  ├─ timeout ────────────> TIMEOUT_CHAIN ─> FINISHED
  ├─ all ×2/×3 revealed ─> LEVEL_CLEARING ─> LEVEL_CLEARED
  │                                            ├─ level 12 ─> FINISHED
  │                                            └─ otherwise ─> NEXT_LEVEL ─> ACTIVE
  └─ fold ───────────────> FINISHED
```

The state transition itself communicates what operations are legal. For example, input and countdown updates occur only in `GAME_PHASE_ACTIVE`. Once a bomb has been selected, `GAME_PHASE_BOMB_REVEAL` freezes the timer until the bomb ending is committed; a near-zero timer can no longer replace a bomb outcome with a timeout.

## Board representation

`GameState.grid` has fixed capacity `MAX_GRID_SIZE × MAX_GRID_SIZE`, namely 6 × 6. `board_grid_size()` selects the active prefix:

- levels 1–8 use 5 × 5;
- levels 9–12 use 6 × 6.

An active `Tile` stores a value in `{0,1,2,3}`, its persistent reveal flag and its flip-animation frame. Inactive cells have value `-1`. Fixed capacity avoids allocation and keeps indexing uniform between the two board sizes.

`board_initialize()` performs four steps:

1. fill active cells with ×1 and reset clue arrays;
2. place the configured bombs, ×2 cards and ×3 cards in distinct cells;
3. compute row and column sums and bomb counts;
4. assign one scanner cell, or two from level 9 onward.

Random placement retains the original rejection-sampling order. Scanner selection also retains that order in the ordinary path. If one hundred random attempts cannot select a valid distinct safe cell, a deterministic row-major fallback guarantees completion without consuming further entropy.

`LevelConfig` also retains two clearly named `legacy*Reward` columns from the original source table as historical metadata. They are not inputs to Bomb Flip's additive card score, time bonus or fold rule.

The first board is generated only when Z/A1 or E/START begins a run. Application initialization no longer generates and discards a hidden board. This is an intentional correction: it removes unnecessary entropy consumption, so a replay tied to the previous cleaned build can obtain a different first board even though each level distribution is unchanged.

## Frame execution

`main()` configures RIVES, initializes the application and audio, then repeats:

1. `riv_present()` synchronizes the frame and updates input;
2. `draw_current_frame()` dispatches from `AppMode`;
3. `audio_poll()` advances the sequenced background track when active.

During active play, `game_update()` dispatches from `GamePhase`. Only the active phase decrements time and accepts board input. Rendering follows the update, and `game_update_outcard()` serializes the public result.

The title renderer returns an event when its delayed easter egg completes. Game logic then records the corresponding ending; drawing code does not mutate the score or end state.

## Reveal, score and completion

`game_reveal_selected()` is the central board interaction. It:

1. rejects an already revealed cell;
2. records a scanner reward if the cell carries one;
3. enters bomb-reveal phase for value 0;
4. otherwise adds `100 × value` coins and `3 × value` seconds;
5. tests `board_all_high_cards_flipped()`.

If the final required card is revealed, the function banks the current card score, changes phase to level clearing, stops the background music and starts the fanfare. The input dispatcher observes the phase change and returns immediately. A Fold-menu input in the same frame therefore cannot convert either a bomb or a completed level into a fold.

The remaining-card animation does not award coins for cards revealed only for display. Once it finishes, the integer time bonus is sampled as `(int)(10 * timeRemaining)` and banked. The timer remains frozen during the reveal sequence and cleared-level panel.

## Scanner model

Every scanner is assigned to a distinct safe card. Revealing a scanner cell grants uses equal to that card's value. Availability depends on the accumulated use count, not on the array index of the scanner that was found. On a 6 × 6 board, finding scanner 2 before scanner 1 therefore works normally.

`use_scanner()` temporarily animates the selected unrevealed card, displays its content, then turns it face down without setting `Tile.revealed`. A scan cannot itself satisfy the completion predicate.

## Deliberately modal interactions

Fold confirmation and scanner preview contain their own modal `riv_present()` loops. The scanner sequence has a fixed duration; the fold dialog waits for confirmation, cancellation or termination of `riv_present()`. This is an intentional rule, not an accidental limitation:

- the countdown does not advance while the player evaluates a fold;
- the countdown does not advance while a scanner bonus is being displayed;
- sequenced background-music polling is suspended for the same frames;
- scanner and fold cancellation do not refresh the outcard while the modal is open; fold confirmation writes the terminal outcard through `game_finish()`.

This prevents an information bonus or confirmation dialog from consuming the resource it is meant to help manage. Generated interface sounds can still be scheduled when the modal action starts. The timeout explosion chain is different: `GAME_PHASE_TIMEOUT_CHAIN` advances through the ordinary frame loop after time has reached and been clamped to zero, so it does not introduce a third nested presentation loop.

## Outcomes

`game_finish()` is the single outcome boundary:

- completion retains every banked level and time reward;
- a bomb discards the current provisional `levelCoins` but preserves earlier `totalCoins`;
- folding adds `floor(levelCoins / 2)` to earlier banked coins;
- timeout clamps time to zero and resets both coin totals;
- the title easter egg applies its fixed negative outcard score.

The end-screen text follows these exact semantics. In particular, a bomb says that the current level's coins were lost rather than claiming that previously banked coins were erased.

## Diagnostics and cheats

`DEBUG_MODE` defaults to `0` in `state.h` and can be overridden at compilation:

```sh
make -C src clean all GAME_CPPFLAGS=-DDEBUG_MODE=1
```

The debug build logs initialization and interactions but does not change the available inputs. Cheats use a separate switch:

```sh
make -C src clean all GAME_CPPFLAGS=-DCHEATS_ENABLED=1
```

With cheats enabled, R1 reveals and scores only safe cards that are still hidden, so an earlier manual reveal is not counted twice. It then enters the ordinary level-clear sequence. `DEBUG_MODE` and `CHEATS_ENABLED` can be enabled independently or together.

## Internal linkage and dependencies

Functions used only inside one implementation file are `static`. Public headers expose the small set of operations needed across modules. Game functions receive `GameState *` and `AudioState *` explicitly; render functions receive `const GameState *` when they only observe it.

This makes coupling visible at the call site and keeps modification local. Changing the twelve level tuples belongs to `board.c`; changing a reward transition belongs to `game.c`; changing its presentation belongs to `render.c`.

## Host-side verification

The tests do not require RIVEMU. A small RIVES test double supplies deterministic entropy, input state and audio/render no-ops.

- `test_board.c` verifies all twelve compositions, clue identities, bomb totals, completion and scanner constraints. It also verifies same-seed generation and the deterministic scanner fallback.
- `test_game.c` verifies the initial outcard, fold/bomb/timeout accounting, timeout clamping, scanner-index independence, the 59-frame modal preview, fold confirmation/cancellation with no modal countdown or sequencer advance, timer freezing after a bomb, same-frame fold exclusion and cheat-score non-duplication.

Run them together with the strict compiler pass:

```sh
make -C src strict test
```

Both targets use C11 with `-Wall -Wextra -Wpedantic -Werror`; the host checks also enable GCC's static analyzer.
