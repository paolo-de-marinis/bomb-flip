# Bomb Flip

![Bomb Flip: timed strategy puzzle gameplay](docs/media/bomb-flip-hero.png)

Bomb Flip is a timed strategy-puzzle RIVES cartridge inspired by Voltorb Flip from *Pokémon HeartGold* and *SoulSilver*. Each hidden card contains a ×1, ×2 or ×3 coin, or a bomb. Every row and column reports its value sum and bomb count.

The clues create a finite deduction problem, but they do not describe the whole
game. Bomb Flip couples that hidden-board problem to a run state: the player must
decide whether to reveal a card, spend a scanner use or Fold before a bomb or the
countdown ends the attempt. Board information and run management are therefore
separate parts of the same decision.

- [Play the original Bomb Flip cartridge on RIVES](https://app.rives.io/cartridges/5932d82f5827)
- [Cartesi Ecosystem Recap #14](https://cartesi.io/blog/ecosystem-recap-202410/)
- [Paolo's RIVES profile](https://app.rives.io/profile/0x2e092f91bc25ebd12b8b0e4df87d9d0424d6460c)

The original cartridge was published on 5 October 2024.

## Relation to Voltorb Flip

Bomb Flip keeps the central clue system, card values and completion rule of Voltorb Flip: reveal every ×2 and ×3 while avoiding the zero-value hazards. Its first level also uses one of the original game's level-1 compositions.

The rest of the progression was redesigned for this cartridge:

- safe cards give an additive score and extra time;
- the run has a countdown;
- scanners temporarily preview hidden cards;
- Fold ends the run and banks half of the current level score;
- there are twelve sequential levels;
- levels 9–12 use a 6 × 6 grid.

Voltorb Flip instead uses multiplicative payouts, a memo pad, eight 5 × 5 levels, multiple board types and history-dependent progression. The shared model and the differences are documented with sources in [How Bomb Flip turns clues into a timed decision problem](docs/mathematics.md).

Bomb Flip is an independent, unaffiliated cartridge. Pokémon and related names belong to their respective rights holders.

## What is in this repository

This is the maintained version of the source, not an archival copy of the cartridge published in 2024. The exact publication snapshot is no longer retained. The present code contains later fixes, tests, documentation and refactoring; the published cartridge remains available at the RIVES link above.

Paolo De Marinis designed the Bomb Flip rules, scoring, timer, scanners, folding system and level progression, and wrote most of the original gameplay code. Cursor was used mainly for animation work and as implementation support. Paolo integrated, tested, debugged and refined the complete cartridge. Since 2026, OpenAI Codex has assisted with repository maintenance, including refactoring, tests and documentation. Paolo reviewed, integrated and validated these changes.

## Gameplay and controls

Reveal every ×2 and ×3 without selecting a bomb. Safe cards add coins and time. Scanner cards grant a limited number of previews. Fold ends the run and keeps half of the coins earned in the current level.

![Bomb Flip gameplay: title, board navigation, Fold decision and Scanner preview](docs/media/bomb-flip-gameplay.gif)

| Action | Keyboard | RIVES gamepad |
| --- | --- | --- |
| Start | Z or E | A1 or Start |
| Move selection | Arrow keys | D-pad |
| Reveal card | Z | A1 |
| Use scanner | X | A2 |
| Open / close Fold | W or F | Select or R2 |
| Confirm Fold | Z | A1 |

E/Start begins a run but does not confirm Fold. Scanner preview and Fold confirmation are modal: the countdown and sequenced background music pause while they are open, so using an information or stopping action does not cost game time.

## How a run evolves

A new run generates the level-1 board only when the player leaves the title
screen. Each level keeps two score quantities: `totalCoins` contains the score
already banked from completed levels, while `levelCoins` contains the still
exposed score earned on the current board. Write $S$ for this second amount.

During active play, the countdown advances before ordinary input is handled. A
safe card of value $v$ adds $100v$ to `levelCoins` and $3v$ seconds, up to the
150-second cap. One safe card is also marked as a scanner reward on levels 1–8;
two are marked on levels 9–12. Revealing one of those cards grants $v$ scanner
uses. A use temporarily shows the selected hidden card without marking it as
revealed.

The run then branches according to the event that occurs:

| Event | Effect on the current level | Final or subsequent state |
| --- | --- | --- |
| All ×2 and ×3 cards revealed | banks all `levelCoins`, then adds $\lfloor10t\rfloor$ from the remaining time $t$ | next level, or completion after level 12 |
| Fold confirmed | banks $\lfloor S/2\rfloor$ | run ends with earlier completed levels preserved |
| Bomb revealed | banks none of the current level | run ends with earlier completed levels preserved |
| Countdown reaches zero | discards both current and previously banked score | run ends at zero |

These are not four presentations of one generic game-over flag. They are
distinct transitions implemented by `GamePhase`, `GameEndState`,
`totalCoins` and `levelCoins`. The complete derivation, including a numerical
example, is in [How Bomb Flip turns clues into a timed decision problem](docs/mathematics.md).

## Reading the code

Start with these files:

1. `src/bombflip.c` owns the application loop and switches between title, transition, game and ending.
2. `src/state.h` defines the complete game and application state.
3. `src/game.c` contains the timer, input, scanner, Fold and outcome rules.
4. `src/board.c` contains the twelve level compositions, generator and clues.
5. `src/render.c` draws the board, interface and dialogs.

`src/title.c` contains the title and transition animations. `src/audio.c` owns the sequencer and sound effects. Debug logging and the level-completion cheat are controlled independently by `DEBUG_MODE` and `CHEATS_ENABLED`, both disabled by default.

The program is procedural C. The game phase is explicit, and `game_update()` shows the order in which timer, input, animations and terminal states are handled.

## Board mathematics

Represent the board by a matrix $A=(a_{ij})$ with entries in $\{0,1,2,3\}$. For row $i$,

~~~math
s_i=\sum_j a_{ij},\qquad
b_i=\sum_j [a_{ij}=0],
~~~

and the column clues are defined in the same way. Because every non-bomb card is at least 1, a line of length $n$ is certainly free of ×2 and ×3 cards when

~~~math
s_i+b_i=n.
~~~

For Bomb Flip, $n=5$ in levels 1–8 and $n=6$ in levels 9–12. The equations are inherited from the source of inspiration; the additive score, time economy, scanner, Fold rule and extended level table are Bomb Flip-specific choices. The mathematical note derives how the clue constraints and those run-specific rules interact.

![Worked Bomb Flip clue model for a 6 × 6 board](docs/media/bomb-flip-clue-model.svg)

The diagram is a mathematical example compatible with level 9, not a screenshot of a fully revealed run.

## Building

Running an existing cartridge requires [RIVEMU](https://rives.io/docs/riv/getting-started/). Compiling the C source also requires the [RIV SDK](https://rives.io/docs/riv/developing-cartridges/). The Makefile uses these default paths:

~~~text
~/.riv/rivemu
~/.riv/rivos-sdk.ext2
~~~

The repository deliberately does not vendor `riv.h`; builds use the API header supplied by the installed RIV SDK. Host-side `strict` and `test` checks copy that SDK header only into a temporary build directory.

Build and run:

~~~sh
make -C src clean all
make -C src run
~~~

For different locations:

~~~sh
make -C src \
  RIVEMU=/path/to/rivemu \
  RIVEMU_SDK=/path/to/rivos-sdk.ext2 \
  clean all
~~~

Local checks:

~~~sh
make -C src strict test
make -C src smoke
~~~

`strict` compiles all production modules as warning-free C11 in release, debug and cheats configurations. `test` checks board invariants and game outcomes with a deterministic host test double. `smoke` starts the packaged cartridge headlessly for 180 frames. The latest recorded build and 96 MB runtime check used RIVEMU/libriv and RIV OS SDK 0.3.0; see [Validation](docs/validation.md).

## Documentation

- [How Bomb Flip turns clues into a timed decision problem](docs/mathematics.md)
- [Code overview](docs/code-overview.md)
- [Validation](docs/validation.md)
- [Third-party notices](THIRD_PARTY_NOTICES.md)
- [SEQT GPLv3 §7 exception](SEQT_EXCEPTION.md)

Related project: [Slither Slide source](https://github.com/paolo-de-marinis/slither-slide) · [original Slither Slide cartridge](https://app.rives.io/cartridges/7654435bf067)

## License

Except for the third-party material listed in [`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md), the source, documentation and original assets are Copyright © 2024–2026 Paolo De Marinis and licensed under the [GNU General Public License, version 3 or later](LICENSE), with the narrow [SEQT additional permission](SEQT_EXCEPTION.md).
