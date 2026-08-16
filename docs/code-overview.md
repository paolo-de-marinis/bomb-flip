# Bomb Flip code overview

The shortest way to understand Bomb Flip is to follow one run, not to memorize
six filenames. `bombflip.c` decides whether the application is on the title,
transition, board or ending screen. Inside the board screen, `game.c` advances
an explicit `GameState`; `board.c` supplies the hidden matrix and its clues. A
terminal event then decides whether the current level is banked, halved, lost or
used to reset the whole run.

The program is procedural C. State is explicit, and the update function reads
like a state machine rather than hiding control flow behind an object framework.
The mathematical meaning of that state is derived in
[How Bomb Flip turns clues into a timed decision problem](mathematics.md).

## Application loop

`bombflip.c` defines an `Application` with four fields:

~~~c
typedef struct {
    AppMode mode;
    GameState game;
    TitleState title;
    AudioState audio;
} Application;
~~~

`main()` configures the 256 × 256 RIVES console, initializes those fields and runs:

~~~c
while (riv_present()) {
    draw_current_frame(&app);
    audio_poll(&app.audio);
}
~~~

`draw_current_frame()` switches between title, transition, active play and game-over screens. A board is generated when the player starts the run, not during application initialization.

## Game state

`state.h` contains the constants and data types shared by the modules. `GameState` groups:

- the 6 × 6 maximum board and its clues;
- current level, banked score, exposed level score and countdown;
- selection and reveal animation;
- the explicit gameplay phase;
- scanner positions and uses;
- timeout, explosion and level-clear timers.

The fields are grouped by purpose rather than wrapped in accessor functions. This makes the complete state inspectable in one place and keeps the small modules easy to follow.

Two fields must not be conflated. `totalCoins` contains score secured by earlier
completed levels. `levelCoins` contains score earned on the current board and is
still exposed to the current outcome. Completion transfers all of it, Fold
transfers half, a bomb transfers none, and timeout clears `totalCoins` as well.

`GamePhase` separates active play from animations and terminal transitions:

- `GAME_PHASE_ACTIVE`;
- bomb reveal;
- timeout explosion chain;
- level clearing and cleared panel;
- next level;
- finished.

`game_update()` switches on this phase before reading ordinary input. Bomb, timeout and level-clear sequences therefore advance without also running the countdown or accepting board actions.

## Board generation and clues

`board.c` contains the twelve `LevelConfig` entries. Each configuration records only the number of ×2 cards, ×3 cards and bombs; the remaining cells are ×1.

`board_initialize()` performs four steps:

1. clear the fixed-capacity grid;
2. place the configured non-×1 cards at random unused positions;
3. calculate row and column clues;
4. assign one scanner tile on levels 1–8 or two on levels 9–12.

The random scanner search has a deterministic row-major fallback. Scanner tiles are always safe and distinct.

The scanner coordinates do not replace tile values. They mark safe cells that
grant a reward when revealed: a scanner card of value $v$ adds $v$ uses. Each
use can preview another selected card, but previewing does not set that tile's
`revealed` field.

`board_all_high_cards_flipped()` implements the completion rule directly by scanning for an unrevealed ×2 or ×3.

## Active-play update

The normal active frame in `game_update()` is:

1. decrement the timer;
2. process movement, reveal or Fold input;
3. advance flip animations;
4. process the optional cheat;
5. process scanner input.

Each step may return early when it changes the phase. For example, selecting a bomb enters `GAME_PHASE_BOMB_REVEAL` before any Fold input can be accepted.

`game_reveal_selected()` handles one tile:

- bombs start the reveal/explosion sequence;
- safe cards add `100 * value` coins and `3 * value` seconds;
- a scanner tile adds a number of uses equal to its value;
- revealing the last required high card banks the level score and starts the clear sequence.

The countdown is capped at 150 seconds.

## Fold and scanner timing

Fold confirmation and scanner preview use small modal `riv_present()` loops. While either loop is open, `game_update()` and `audio_poll()` are not called. This freezes the countdown and sequenced music by design.

Fold banks half of `levelCoins` using integer division. A bomb preserves previously banked levels but loses the unbanked current level. Timeout sets the entire run score to zero.

The scanner temporarily animates the selected card face up, waits for the preview interval and turns it face down again. The card is not marked as revealed.

## Rendering

`render.c` draws the board from `GameState` without changing it. The main groups are:

- tiles, coins, bombs and scanner marks;
- row and column clues;
- score, level and timer;
- bomb and timeout explosions;
- scanner overlay;
- Fold, completion and game-over panels.

`title.c` owns the standard title, transition and easter-egg animation. Keeping it outside `render.c` prevents the gameplay renderer from also becoming an application-state module.

## Audio

`audio.c` owns the SEQT player and all waveform descriptions. `audio_poll()` is called once per ordinary application frame; the gameplay module only starts or stops background music and requests named effects.

## Outcard

`game_update_outcard()` writes:

~~~json
{"score":0,"level":1,"cards_flipped":0,"time_remaining":45.00}
~~~

The outcard is refreshed during active play and again when the run ends.

## Build flags

`DEBUG_MODE` and `CHEATS_ENABLED` default independently to zero:

- debug mode adds diagnostic messages;
- cheats add the R1 level-completion helper;
- neither flag changes the ordinary control mapping when disabled.

## Tests

`tests/test_board.c` checks every level composition, clue totals, completion and scanner assignment. `tests/test_game.c` exercises Fold, bomb, timeout, scanner, modal timing, simultaneous inputs and the cheat completion path with a deterministic RIVES test double.
