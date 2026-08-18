# Sources and provenance

The main Bomb Flip documentation distinguishes three kinds of statements:

1. rules and transitions derived from the current Bomb Flip source;
2. historical comparison with Voltorb Flip;
3. external mathematical interpretations that the cartridge does not implement.

This page keeps the sources for the second and third categories separate from the code-derived
model in [Mathematics of the Bomb Flip state machine](mathematics.md).

## Bomb Flip implementation and provenance

- [Original Bomb Flip cartridge on RIVES](https://app.rives.io/cartridges/5932d82f5827) — original
  publication and playable cartridge.
- [`src/board.c`](../src/board.c) — current fixed level configurations, board generation, clues,
  completion predicate and scanner-reward placement.
- [`src/game.c`](../src/game.c) — current timer, reveal, Fold, scanner, completion and terminal
  transitions.
- [`src/state.h`](../src/state.h) — concrete state variables, constants, `GamePhase` and
  `GameEndState`.

The maintained repository is not an archival copy of the October 2024 publication snapshot. The
original publication remains the provenance reference for the cartridge; the repository records
later maintenance, tests, documentation and refactoring.

## Voltorb Flip comparison

- [pret/pokeheartgold: game-state reconstruction](https://github.com/pret/pokeheartgold/blob/90e85d4e027f5e04800e7e015b3207094061402c/src/voltorb_flip/voltorb_flip_game.c)
  — reconstructed original board configurations, clue logic, generator and progression data.
- [pret/pokeheartgold: application reconstruction](https://github.com/pret/pokeheartgold/blob/90e85d4e027f5e04800e7e015b3207094061402c/src/voltorb_flip/voltorb_flip.c)
  — reconstructed round/application flow, including systems that Bomb Flip does not reproduce.
- [Bulbapedia: Voltorb Flip](https://bulbapedia.bulbagarden.net/wiki/Voltorb_Flip) — secondary
  reference for rules, board compositions and common clue deductions.
- [The Cave of Dragonflies: Voltorb Flip guide](https://www.dragonflycave.com/johto/voltorb-flip/)
  — worked clue deductions.
- [GameFAQs: Voltorb Flip guide](https://gamefaqs.gamespot.com/ds/960100-pokemon-soulsilver-version/faqs/59308)
  — secondary reference for the classic five-cell dead-line rule.

The current Bomb Flip documentation uses those sources only to identify inherited structure and
differences. Bomb Flip's additive score, timer, scanner rewards, Fold rule, twelve-level table and
6 x 6 extension are read from Bomb Flip's own source.

## External solver and decision-theory references

- [Marcus Pasell: algorithmic solution](https://understandable.dev/deep-dives/voltorb-flip/) —
  compatible-board enumeration and dependence between row and column constraints.
- [Gimmy Tomas: solver explanation](https://gimmytomas.github.io/voltorb-flip/algorithm.html) —
  Bayesian treatment of original Voltorb Flip board types.

These sources support the optional inference perspective, not implementation claims about Bomb
Flip. The cartridge does not enumerate compatible boards, compute posterior probabilities or
solve a Bellman equation.

## Development boundary

Paolo De Marinis designed Bomb Flip's timer, additive score, scanner rewards, Fold rule,
twelve-level progression, later level tuples and 6 x 6 extension, and wrote most of the original
gameplay code. Cursor was used mainly for animation work and implementation support. Paolo
integrated, tested, debugged and refined the complete cartridge.

Since 2026, OpenAI Codex has assisted with repository maintenance, including refactoring, tests
and documentation. Paolo reviewed, integrated and validated those changes.

The clue alphabet, clue equations, completion concept and the identified original level-1
composition are credited to the Voltorb Flip source of inspiration rather than presented as
original Bomb Flip inventions.