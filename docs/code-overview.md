# Bomb Flip code overview

The code is easier to read after the mathematical state has been identified. Bomb Flip does not
have one monolithic "game object" whose methods hide the rules. It keeps explicit C state and
applies a small number of procedural transitions to it.

The mathematical document writes the gameplay projection as

~~~math
\sigma=(\ell,A,R,G,B,S,t,u,\varphi).
~~~

The implementation distributes these components across `GameState`:

- $\ell$: `level`;
- $A$: `grid[y][x].value` on the active board;
- $R$: the `revealed` flags;
- $G$: `scannerX[]`, `scannerY[]` and `scannerCount`;
- $B$: `totalCoins`;
- $S$: `levelCoins`;
- $t$: `timeRemaining`;
- $u$: `scannerUses`;
- $\varphi$: `phase`.

Animation counters, selection, explosion state and presentation fields complete the concrete C
state but are not needed in every mathematical rule. The derivation is in
[Mathematics of the Bomb Flip state machine](mathematics.md).

## 1. Application state and gameplay state are different layers

`bombflip.c` owns the outer `Application`:

~~~c
typedef struct {
    AppMode mode;
    GameState game;
    TitleState title;
    AudioState audio;
} Application;
~~~

`AppMode` distinguishes title, transition, active game and ending presentation. `GamePhase`,
inside `GameState`, distinguishes states *within* a run such as active play, bomb reveal,
timeout chain and level clearing.

The two enums therefore answer different questions:

~~~math
\text{AppMode}:\ \text{which application screen owns the frame?}
~~~

~~~math
\text{GamePhase}:\ \text{which gameplay transition is currently allowed?}
~~~

This separation matters because a bomb reveal or timeout animation is still part of the game
screen even though ordinary board actions are no longer accepted.

## 2. `state.h` is the concrete state space

`state.h` defines the fixed constants, `Tile`, `GameEndState`, `GamePhase`, `GameState` and
`AppMode`.

A tile stores three quantities:

~~~c
typedef struct {
    int value;
    bool revealed;
    int flipFrame;
} Tile;
~~~

The first two have gameplay meaning; `flipFrame` is presentation state. The same pattern appears
throughout `GameState`: score and timer coexist with animation counters because the program keeps
one inspectable procedural state rather than splitting every concern into objects.

Two score fields must remain distinct:

~~~math
B=\texttt{totalCoins},
\qquad
S=\texttt{levelCoins}.
~~~

$B$ is already banked. $S$ is still exposed to the current level outcome. Bomb, Fold, Timeout
and completion differ mainly in how they transform this pair.

## 3. `board.c` constructs the static part of one level

For a fixed level, `board_initialize()` applies

~~~math
\text{clear}
\longrightarrow
\text{place values}
\longrightarrow
\text{compute clues}
\longrightarrow
\text{assign scanner metadata}.
~~~

### 3.1 Active board size

`board_grid_size()` returns 5 on levels 1-8 and 6 on levels 9-12. The physical array remains
6 x 6; inactive cells are set to `-1`.

### 3.2 Fixed-count value generation

`LEVEL_CONFIGS` stores x2, x3 and bomb counts. `board_clear()` initializes active cells as x1,
and `place_level_cards()` replaces random free cells until the configured counts have been
placed.

The clue arrays are then computed directly from the resulting values. No deduction engine is
involved: row/column sums and bomb counts are deterministic aggregates of the hidden board.

### 3.3 Scanner metadata is not a tile value

`board_assign_scanner_tiles()` runs after values and clues exist. A scanner reward is represented
by coordinates stored separately from `Tile.value`.

The progression is:

~~~math
0\text{ rewards on }1\!:\!3,
\qquad
1\text{ on }4\!:\!8,
\qquad
2\text{ on }9\!:\!12.
~~~

A reward coordinate must point to a safe cell and two reward coordinates must be distinct. The
ordinary path uses random search; after one hundred unsuccessful attempts the function uses a
row-major fallback.

This is why "safe card" and "scanner card" are not synonymous. Scanner status is hidden
metadata attached to selected safe cells.

## 4. Level initialization resets only level-local state

`initialize_level()` first calls `reset_level_state()`, which clears

- `levelCoins`;
- phase and end-state presentation counters;
- scanner uses and scanner coordinates;
- level-local animation state.

It then generates a new board and initializes

~~~math
t_0(\ell)=45+5(\ell-1).
~~~

`totalCoins` is not reset here. It is banked run state and survives progression between completed
levels.

A new run is different: `game_begin_run()` resets the level to 1, `totalCoins` to zero and the
run-wide card count before initializing level 1.

## 5. `game_update()` is a phase-dispatched transition system

`game_update()` first inspects `GamePhase`. Non-active phases advance their own animation or
transition and return without executing ordinary board input.

The active path then has an order that is part of the rules:

1. decrement the timer;
2. process directional movement, reveal or Fold input;
3. advance flip animations;
4. process the optional level-completion cheat;
5. process scanner input.

Each stage can return early when it changes the phase. This prevents later actions in the same
frame from running after a terminal or clearing transition.

For example, if Reveal selects a bomb, `game_reveal_selected()` enters
`GAME_PHASE_BOMB_REVEAL`; Fold and scanner handling are not then allowed to reinterpret that
same frame.

## 6. Reveal operator

`game_reveal_selected()` is the main board-state transition.

For an unrevealed selected tile it first:

1. sets `revealed = true`;
2. starts its flip animation;
3. increments `totalCardsFlipped`;
4. checks whether that coordinate is one of the hidden scanner rewards.

If the tile is a scanner reward of value $v$, it adds $v$ to `scannerUses`.

### Bomb

For `value == 0`, the function enters `GAME_PHASE_BOMB_REVEAL`, records the explosion cell,
stops background music and returns. It does **not** add `levelCoins` to `totalCoins`.

The later bomb phase eventually calls `game_finish(..., GAME_END_BOMB, ...)`, so the run ends
with the earlier banked score only.

### Safe card

For `value == v > 0`, the function applies

~~~math
S\leftarrow S+100v,
~~~

~~~math
t\leftarrow\min(150,t+3v).
~~~

It then evaluates `board_all_high_cards_flipped()`.

If high cards remain, the phase stays active. If none remain, the current `levelCoins` is added
to `totalCoins` immediately and the phase becomes `GAME_PHASE_LEVEL_CLEARING`.

## 7. Completion is split across two phase transitions

Completion is not one function call.

The last required high-card reveal first banks the complete current card score and enters
`GAME_PHASE_LEVEL_CLEARING`.

`update_level_clearing()` then reveals the remaining cards for presentation, waits for their
animations and the clear delay, and computes

~~~math
\texttt{timeBonus}=\lfloor10\,\texttt{timeRemaining}\rfloor.
~~~

That bonus is added to both `totalCoins` and `levelCoins`, then the phase becomes
`GAME_PHASE_LEVEL_CLEARED`.

`update_cleared_level()` waits for the cleared panel. It either enters `GAME_PHASE_NEXT_LEVEL`
or, on level 12, calls `game_finish(..., GAME_END_COMPLETE, ...)`.

The mathematical total

~~~math
B+S+\lfloor10t\rfloor
~~~

is therefore implemented by several temporally separated code steps.

## 8. Bomb, Fold and Timeout are distinct terminal maps

`game_finish()` records the end state and moves to `GAME_PHASE_FINISHED`, but its score update
depends on the outcome.

### Bomb

No score transfer occurs in `GAME_END_BOMB`:

~~~math
(B,S)\longmapsto B\text{ as final score}.
~~~

### Fold

`GAME_END_FOLD` computes

~~~math
\texttt{foldedCoins}=S/2
~~~

with integer division and applies

~~~math
B\leftarrow B+\left\lfloor\frac S2\right\rfloor.
~~~

### Timeout

`GAME_END_TIMEOUT` sets

~~~math
B=0,
\qquad
S=0,
\qquad
t=0.
~~~

The timeout explosion chain happens before this final call, but ordinary active input does not
resume during the chain.

## 9. Scanner preview is presentation without reveal

`game_scanner_is_available()` requires:

- active phase;
- scanner metadata present for the level;
- at least one available use;
- incomplete high-card objective.

`use_scanner()` refuses an already revealed tile. Otherwise it sets `scannerInUse`, animates the
selected card face up, holds the preview and animates it face down again.

It never writes

~~~c
tile->revealed = true;
~~~

and only after the preview does it decrement `scannerUses`.

Thus scanner preview changes the use count and temporary presentation state but not the revealed
set used by completion.

The code stores no permanent memo of the previewed value.

## 10. Fold and scanner loops pause the ordinary state machine

Both Fold confirmation and scanner preview contain nested `riv_present()` loops. These loops own
presentation temporarily instead of returning to the application's ordinary outer frame.

Consequently, while they are open:

- `game_update()` is not called;
- `update_timer()` is not called;
- `audio_poll()` is not called.

The timer pause is therefore a direct consequence of control flow, not a separate timer flag.

## 11. Rendering reads state; it does not define the rules

`render.c` draws `GameState`: board cells, clue panels, selection, score, timer, scanners,
explosions and terminal overlays.

`title.c` owns title, transition and easter-egg presentation. Keeping title state outside the
board renderer prevents application navigation from being folded into the gameplay transition
logic.

The code does contain animation state inside `GameState`, but the hidden board values, revealed
flags, score transfers and completion predicate are defined outside rendering.

## 12. Audio follows state transitions

`audio.c` owns the SEQT player and waveform descriptions. Gameplay code requests named effects
or starts/stops background playback at transition points.

The ordinary application loop calls `audio_poll()` once after drawing. Modal Fold/scanner loops
do not return to that outer poll, which is why sequenced music pauses together with the game
timer during those interfaces.

## 13. Outcard is a projection of run state

`game_update_outcard()` serializes

~~~json
{"score":0,"level":1,"cards_flipped":0,"time_remaining":45.00}
~~~

using `totalCoins`, `level`, `totalCardsFlipped` and `timeRemaining`.

The outcard therefore exposes a selected projection of `GameState`; it does not serialize the
hidden board, scanner coordinates or unbanked `levelCoins`.

## 14. Build flags

`DEBUG_MODE` and `CHEATS_ENABLED` default independently to zero.

- debug mode adds diagnostic messages;
- cheats add the R1 completion helper;
- neither changes the ordinary control mapping when disabled.

The cheat helper uses production state and phase transitions but is excluded from normal builds.

## 15. Tests

The host suite is intentionally smaller than the full state space.

`tests/test_board.c` checks board compositions, clues, completion and scanner placement.
`tests/test_game.c` checks selected score, timing, modal and phase-transition behaviors with a
deterministic RIVES test double.

The exact evidentiary scope of those tests is described in [Validation](validation.md).