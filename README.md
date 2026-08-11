# Bomb Flip

Bomb Flip is a timed strategy puzzle for the RIVES fantasy console. Each hidden card contains a x1, x2 or x3 coin, or a bomb; row and column clues show the value sum and bomb count.

## Original RIVES project

- [Play the original Bomb Flip cartridge on RIVES](https://app.rives.io/cartridges/5932d82f5827)
- [Cartesi Ecosystem Recap #14 — Bomb Flip featured as Paolo's RIVES cartridge](https://cartesi.io/blog/ecosystem-recap-202410/)
- Original publication: 5 October 2024, under the RIVES profile **Paolo**

## About this repository

This repository contains a cleaned-up and documented version of the original RIVES project. The gameplay and core implementation derive from the original cartridge; the source was subsequently reorganized and documented to improve readability and make the implementation easier to study. Only the maintained source and the assets required to build it are included here; working archives and extracted copies of the original project are intentionally excluded.

## Development

Bomb Flip was developed in C with the RIVES library (`riv.h`) through a Cursor-assisted workflow. I conceived the mechanics and gameplay, directly wrote and modified parts of the code, and handled integration, testing, debugging and refinement.

## Gameplay

Reveal every x2 and x3 card without selecting a bomb. Revealed coin cards add score and time. Scanner cards grant limited previews, while folding ends the run and keeps half of the current level's coins. The board grows from 5 x 5 to 6 x 6 across twelve levels.

| Action | Keyboard | RIVES gamepad |
|---|---|---|
| Start | E | START |
| Move selection | Arrow keys | D-pad |
| Reveal card | Z | A1 |
| Use scanner | X | A2 |
| Fold / confirm fold | W | SELECT |
| Cancel fold | E | START |

## Technical overview

`main()` configures the RIVES console, initializes the first level and audio, then runs one update/render cycle per `riv_present()`. A `GameState` value stores the board, score, timer, animation counters, scanner state and ending condition. Input is read from `riv->keys`; update helpers handle selection, card reveals, the timer, level transitions, scanner previews and ending animations. Rendering uses RIVES primitive drawing functions, while `seqt.h` and `riv_waveform()` provide music and sound effects. The current outcard is written with `riv_snprintf()`.

The implementation is procedural C, not object-oriented code. See [the code overview](docs/code-overview.md) for the full frame flow, data structures, interactions and RIVES calls.

## Code structure

- `src/bombflip.c` — game state, input, rules, animation, rendering, audio control and entry point.
- `src/seqt.h` — small sequenced-audio helper used by the background track.
- `src/songs/` — RIVES music asset.
- `src/info.json` and `src/cover.png` — cartridge metadata and cover.
- `docs/` — technical architecture and validation notes.

## Debug mode

`DEBUG_MODE` at the top of `src/bombflip.c` is the compile-time switch. It is `0` by default; set it to `1` and rebuild to enable diagnostic logging and the R1 level-completion helper. With debug disabled, R1 is not tracked and the helper is not compiled. The scanner, timer and normal gameplay remain available in either mode.

## Technical documentation

- [Code overview](docs/code-overview.md)
- [Validation](docs/validation.md)

## Building / running

The verified setup uses `rivemu` 0.3.0 and the RIV OS SDK. By default the Makefile reads both from `~/.riv`; override `RIVEMU` or `RIVEMU_SDK` if they are installed elsewhere.

```sh
make -C src clean all
make -C src run
```

Checks:

```sh
make -C src smoke
```

`smoke` runs the cartridge headlessly for 180 frames.

## Repository history

This repository starts from the reviewed, publication-ready source. The published 2024 cartridge remains available through the RIVES link above, while local working archives and cleanup history are intentionally not part of this repository. The later cleanup does not establish historical authorship of individual parts of the original game.

## License

No license file was included with the original project, and this repository does not add one. No permission to copy, modify or redistribute the source is granted beyond rights provided by applicable law.
