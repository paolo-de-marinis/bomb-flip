# Bomb Flip code overview

This document describes the maintained implementation in `src/`. It is a procedural C program: functions operate on explicit global state, and no object-oriented model is used.

## Program structure

`src/bombflip.c` contains the application entry point and all game-specific logic. `src/seqt.h` provides the sequenced-audio helper used for `songs/gameplay.rivcard`. Cartridge metadata and the cover are stored beside the source so `src/Makefile` can package them.

The file is organized around three main types:

- `Tile` stores a card value, whether it is revealed and its flip-animation frame.
- `LevelConfig` stores counts of x2 cards, x3 cards and bombs for one level. Its final two reward fields are preserved original data and intentionally unused.
- `GameState` owns the grid, clues, score, timer, animation counters, scanner state and `GameEndState`.

Small title-screen values such as `transitionFrame`, `titleScreenTimer` and `nuclearExplosionFrame` remain outside `GameState` because they describe the application shell rather than an active board.

## Execution flow

Execution starts in `main()`:

1. `configureConsole()` sets the 256 x 256 display, 60 FPS target and tracked controls.
2. `initializeApplication()` creates level 1 and writes the initial outcard.
3. `loadBackgroundMusic()` initializes `seqt` and loads the packaged track.
4. `while (riv_present())` advances the RIVES frame.
5. `drawCurrentFrame()` selects the title, title transition, active game or ending screen.
6. `pollBackgroundMusic()` advances the sequenced track when it is active.
7. After the loop, `seqt_destroy_source()` releases the music source.

During an active run, `drawCurrentFrame()` calls `updateGame()`, checks the end state, renders with `drawGame()` or `drawEndScreen()`, and refreshes the outcard.

## Main data structures

`game.grid[GRID_SIZE][GRID_SIZE]` has a fixed 6 x 6 capacity. `getCurrentGridSize()` exposes only 5 x 5 for levels 1-8 and 6 x 6 for levels 9-12. Card values are `0` for bombs and `1`, `2` or `3` for coins.

The clue arrays `rowTotals`, `columnTotals`, `rowBombs` and `columnBombs` are recalculated by `calculateClues()`. `totalCoins` survives between levels; `levelCoins` is the current level's provisional reward. The ending is represented by `GameEndState`, while `gameOverTriggered` selects the ending screen at the application level.

Scanner coordinates and reveal flags are parallel arrays of capacity two. `scannerCount` records how many entries are valid. Animation fields use integer frame counters so their duration is deterministic at the fixed target rate.

## Input handling

RIVES exposes buttons through `riv->keys[...].press`, which is true on the press transition:

- `moveSelection()` wraps the selected row or column at the board edges.
- `handleBoardInput()` handles movement, A1 reveal and the SELECT fold dialog.
- `handleScannerInput()` sends the selected coordinate to `useScanner()` when a scanner is available.
- `drawCurrentFrame()` handles START on the title screen.

`handleFoldInput()` contains its own `riv_present()` loop because it draws and resolves a modal confirmation. SELECT accepts; START returns to the board.

## Game logic

`initializeLevel()` reads `LEVEL_CONFIGS`, resets per-level state, fills the active grid with x1 cards, randomly places bombs and high-value cards, calculates clues, assigns scanner cards and initializes the timer.

`revealSelectedCard()` is the central interaction:

- already revealed cards are ignored;
- scanner-card state is updated;
- a bomb starts the bomb reveal and explosion sequence;
- a coin adds `value * 100` and `value * TIME_BONUS_PER_CARD` seconds;
- revealing every x2 and x3 card starts the level-clear sequence.

`updateTimer()` subtracts one sixtieth of a second per frame, switches from music to warning ticks at ten seconds and ends the run at zero. `updateLevelClearing()`, `updateClearedLevel()` and `startNextLevel()` reveal the remaining grid, calculate the time bonus and move to the next level. `finishGame()` handles complete, bomb, fold, timeout and title-easter-egg endings before updating the outcard.

## Rendering

`drawGame()` chooses the level palette, computes board offsets, and draws the board, clues, status, scanner overlay and current animation. `drawBoard()` visits each active tile; `drawTile()` applies the flip scale and selection frame; `drawTileContent()` draws bombs or numeric coins.

Title rendering is split between `drawStandardTitle()`, `drawGrowingTitleExplosion()`, `drawNuclearExplosion()` and `drawJungleScene()`. `drawTitleTransition()` covers the change from title to game. End panels are drawn by `drawLevelClearedPanel()` and `drawEndScreen()`.

RIVES primitive calls such as `riv_clear()`, `riv_draw_rect_fill()`, `riv_draw_circle_fill()`, `riv_draw_line()` and `riv_draw_text()` produce every visual; there is no sprite-based game renderer.

## Movement, interactions and collisions

Bomb Flip has selection movement rather than physical movement. The cursor wraps with modular arithmetic. An interaction targets `game.grid[selectedY][selectedX]`.

Bomb detection is a direct `tile->value == 0` check. The explosion is visual and state-driven rather than a geometric collision. Scanner previews temporarily animate an unrevealed card without setting its persistent `revealed` flag. Completion is tested by `allHighCardsFlipped()`, not by revealing the entire board.

## Debug system

`DEBUG_MODE` is a compile-time macro near the top of `bombflip.c` and defaults to `0`.

When set to `1`:

- initialization, reveal and scanner events are sent through `debugLog()`;
- `configureConsole()` tracks `CHEAT_WIN_LEVEL`, mapped to R1;
- pressing R1 reveals every non-bomb card and enters the ordinary level-clear sequence.

When set to `0`, `#if DEBUG_MODE` removes these blocks from the build, `debugLog()` discards its argument and R1 is not registered. The helper can modify score and level progress, so it is intentionally debug-only.

## Important functions

- `main()` — application lifetime.
- `drawCurrentFrame()` — top-level screen and state dispatch.
- `initializeLevel()` — board and timer setup.
- `updateGame()` — one active-game update.
- `revealSelectedCard()` — card interaction and rewards.
- `updateTimer()` — countdown, warning audio and timeout.
- `updateLevelClearing()` — reveal animation and time bonus.
- `useScanner()` — temporary card preview.
- `finishGame()` — ending-specific score and audio handling.
- `drawGame()` — active-game renderer.
- `updateOutcard()` — RIVES result payload.

## RIVES integration

- `riv_present()` synchronizes frames and exposes current input.
- `riv->width`, `riv->height` and `riv->target_fps` configure the console.
- `riv->tracked_keys` enables the required buttons.
- `riv_rand_uint()` places cards and scanner rewards using RIVES deterministic entropy.
- `riv_draw_*` functions render the game.
- `riv_waveform()` plays generated sound effects.
- `riv->outcard` and `riv->outcard_len` expose score, level, flip count and remaining time.
- `riv->quit_frame` schedules automatic exit after an ending.

## Non-obvious implementation decisions

Random placement preserves call order because RIVES replays depend on deterministic entropy consumption. The fixed-size grid avoids dynamic allocation and keeps all level layouts within one state object. Animation durations are frame counts because the game fixes the target at 60 FPS. Original reward fields and a small number of documented behavioral anomalies remain unchanged so the cleanup does not silently alter the cartridge rules.

## Known limitations

- Historical regression checks covered deterministic title/easter-egg rendering and outcards, but the original working archive used for that comparison is not distributed in this repository.
- Scanner availability still depends on `scannerRevealed[0]`, as in the original behavior.
- The title explosion retains an original unreachable audio branch to avoid changing established output without a dedicated regression case.
- Debug mode is selected at build time, not from an in-game settings screen.
- The end flow schedules cartridge exit; it does not offer an in-process restart.
