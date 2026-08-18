# Bomb Flip

![Bomb Flip: timed strategy puzzle gameplay](docs/media/bomb-flip-hero.png)

Bomb Flip is a timed strategy-puzzle RIVES cartridge inspired by Voltorb Flip from
*Pokémon HeartGold* and *SoulSilver*. Its rules combine a hidden finite board with an explicit
run state: clues constrain what may be under each card, while score, time, scanner uses and Fold
determine what each reveal means for the current attempt.

The implementation can be read through three objects:

~~~math
A
\xrightarrow{\Gamma}
\text{row/column clues},
\qquad
\sigma
\xrightarrow{\text{action or phase update}}
\sigma'.
~~~

Here $A$ is the hidden board, $\Gamma$ is the deterministic clue map, and $\sigma$ is the
mutable gameplay state containing the current level, revealed cells, scanner metadata, banked
score, exposed level score, timer, scanner uses and phase. Reveal, Scan, Fold, Bomb, Timeout and
completion are different state transitions; they are not presentations of one generic outcome.

- [Play the original Bomb Flip cartridge on RIVES](https://app.rives.io/cartridges/5932d82f5827)
- [Cartesi Ecosystem Recap #14](https://cartesi.io/blog/ecosystem-recap-202410/)
- [Paolo's RIVES profile](https://app.rives.io/profile/0x2e092f91bc25ebd12b8b0e4df87d9d0424d6460c)

The original cartridge was published on 5 October 2024.

## What is in this repository

This is the maintained version of the source, not an archival copy of the cartridge published in
2024. The exact publication snapshot is no longer retained. The present repository contains
later fixes, tests, documentation and refactoring; the original cartridge remains available at
the RIVES link above.

The scanner progression in the maintained source follows the published cartridge: no scanner
reward on levels 1-3, one on levels 4-8 and two on levels 9-12.

Paolo De Marinis designed the Bomb Flip rules, scoring, timer, scanners, folding system and level
progression, and wrote most of the original gameplay code. Cursor was used mainly for animation
work and as implementation support. Paolo integrated, tested, debugged and refined the complete
cartridge. Since 2026, OpenAI Codex has assisted with repository maintenance, including
refactoring, tests and documentation. Paolo reviewed, integrated and validated these changes.

## Relation to Voltorb Flip

Bomb Flip keeps the hidden alphabet $\{0,1,2,3\}$, row and column value sums, row and column
bomb counts and the completion rule that requires every x2 and x3 to be revealed. Its first
level also uses one level-1 composition from Voltorb Flip.

The current cartridge separately defines:

- additive safe-card scoring;
- a countdown and value-dependent time rewards;
- scanner rewards and previews;
- Fold, which terminates the run with half of the current level score;
- twelve sequential levels;
- 6 x 6 boards on levels 9-12.

The exact current rules and their mathematical state transitions are derived in
[Mathematics of the Bomb Flip state machine](docs/mathematics.md). Probability conditioned on
player knowledge and a Bellman formulation are kept there only as separate appendices because
the cartridge does not implement either calculation. Historical and external sources are kept
separately in [Sources and provenance](docs/references.md).

Bomb Flip is an independent, unaffiliated cartridge. Pokémon and related names belong to their
respective rights holders.

## Gameplay and controls

Reveal every x2 and x3 without selecting a bomb. A safe card of value $v$ adds

~~~math
100v
~~~

coins to the exposed current-level score and

~~~math
3v
~~~

seconds, up to the 150-second cap.

From level 4, one or two safe cells are also designated as hidden scanner rewards. Revealing one
of those cells grants a number of scanner uses equal to its card value. A scanner use previews a
selected hidden card but does not mark it as revealed.

Fold ends the run and transfers

~~~math
\left\lfloor\frac S2\right\rfloor
~~~

from the current exposed level score $S$ into the final score. A bomb loses the current level
score but preserves earlier banked levels; timeout resets the complete run score to zero.

![Bomb Flip gameplay: title, board navigation, Fold decision and safe-card reveals](docs/media/bomb-flip-gameplay.gif)

| Action | Keyboard | RIVES gamepad |
| --- | --- | --- |
| Start | Z or E | A1 or Start |
| Move selection | Arrow keys | D-pad |
| Reveal card | Z | A1 |
| Use scanner | X | A2 |
| Open / close Fold | W or F | Select or R2 |
| Confirm Fold | Z | A1 |

E/Start begins a run but does not confirm Fold. Scanner preview and Fold confirmation use modal
presentation loops. While either is open, the ordinary game update is not executed, so the game
timer and sequenced background-music polling do not advance.

## Reading the model

The documentation is organized by responsibility rather than by source-file order.

1. [Mathematics of the Bomb Flip state machine](docs/mathematics.md) derives board generation,
   clues, completion, scanner metadata and the implemented score/time/phase transitions.
2. [Code overview](docs/code-overview.md) maps those mathematical objects and operators back to
   `state.h`, `board.c`, `game.c`, rendering and audio.
3. [Validation](docs/validation.md) separates static compilation, targeted rule checks, RIVES
   runtime evidence and properties not established by the current tests.
4. [Sources and provenance](docs/references.md) keeps historical comparison, external solver
   references and development attribution separate from implementation claims.

The main mathematical document intentionally distinguishes the rules actually executed by the
cartridge from optional external interpretations. Player-side conditional probability and
Bellman optimization appear only after the code-derived model is complete.

## Board model in one page

At level $\ell$, the active board is

~~~math
A=(a_{ij})\in\{0,1,2,3\}^{n(\ell)\times n(\ell)},
~~~

with

~~~math
n(\ell)=
\begin{cases}
5,&1\leq\ell\leq8,\\
6,&9\leq\ell\leq12.
\end{cases}
~~~

For row $i$ the cartridge displays

~~~math
s_i=\sum_j a_{ij},
\qquad
b_i=\sum_j[a_{ij}=0],
~~~

and columns use the analogous pair. If $R$ is the set of revealed cells, completion is exactly

~~~math
\{(i,j):a_{ij}\in\{2,3\}\}\subseteq R.
~~~

Scanner preview does not change $R$; this is why seeing a high card through the scanner does not
complete it.

![Worked Bomb Flip clue model for a 6 x 6 board](docs/media/bomb-flip-clue-model.svg)

The diagram is a mathematical example compatible with level 9, not a screenshot of a fully
revealed run.

## Reading the code

The shortest implementation path is:

1. `src/state.h` defines `GameState`, `GamePhase` and the fixed gameplay constants.
2. `src/board.c` generates $A$, computes the clue map and assigns hidden scanner metadata.
3. `src/game.c` applies the timer and action/phase transitions to the mutable run state.
4. `src/bombflip.c` owns the application-level title, transition, game and ending modes.
5. `src/render.c` and `src/title.c` present those states without defining the board rules.
6. `src/audio.c` owns sequenced music and effects requested by the state transitions.

The program is procedural C. State is explicit and the phase dispatch makes the temporal order
of rules visible rather than hiding it behind an object hierarchy.

## Building

Running an existing cartridge requires [RIVEMU](https://rives.io/docs/riv/getting-started/).
Compiling the C source also requires the [RIV SDK](https://rives.io/docs/riv/developing-cartridges/).
The Makefile uses these default paths:

~~~text
~/.riv/rivemu
~/.riv/rivos-sdk.ext2
~~~

The repository does not vendor `riv.h`; builds use the API header supplied by the installed RIV
SDK. Host-side `strict` and `test` checks copy that header only into a temporary build directory.

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

`strict` compiles the production modules as warning-free C11 in release, debug and cheats
configurations. `test` exercises board invariants and gameplay transitions with deterministic
RIVES doubles. `smoke` starts the packaged cartridge headlessly for 180 frames. See
[Validation](docs/validation.md) for what each check does and does not establish.

## Documentation

- [Mathematics of the Bomb Flip state machine](docs/mathematics.md)
- [Code overview](docs/code-overview.md)
- [Validation](docs/validation.md)
- [Sources and provenance](docs/references.md)
- [Third-party notices](THIRD_PARTY_NOTICES.md)
- [SEQT GPLv3 §7 exception](SEQT_EXCEPTION.md)

Related project: [Slither Slide source](https://github.com/paolo-de-marinis/slither-slide) ·
[original Slither Slide cartridge](https://app.rives.io/cartridges/7654435bf067)

## License

Except for the third-party material listed in [`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md),
the source, documentation and original assets are Copyright © 2024-2026 Paolo De Marinis and
licensed under the [GNU General Public License, version 3 or later](LICENSE), with the narrow
[SEQT additional permission](SEQT_EXCEPTION.md).