# Validation

## Scope

This repository contains only the maintained source, documentation and build assets. Working archives and extracted copies of the 2024 project are not distributed.

## RIVES build

Verified environment:

- `rivemu` 0.3.0;
- RIV OS SDK `v0.3-rc16`;
- RISC-V compilation with `riv-opt-flags -Ospeed`;
- SquashFS packaging with `riv-mksqfs`.

```sh
make -C src clean all
```

Result: `bombflip` and `bombflip.sqfs` were generated successfully. A strict C11 check with `-Wall -Wextra -Wpedantic -Werror` also completed without warnings.

## Smoke test

```sh
make -C src smoke
```

The cartridge completed a 180-frame headless run successfully.

## Historical regression check

Before selecting the files for this repository, a local copy of the 2024 source and the cleaned source were rebuilt with the same SDK and entropy. Their screenshots and outcards matched byte for byte at 180 and 3820 frames.

The checks covered the normal title, initialization, title timer, growing explosion, nuclear flash, randomized jungle and cloud. The nuclear-scene screenshot had this SHA-256 in both builds:

```text
0f329072186a2c211591b370549047fc79ca32a816a47b424b791497b1c8d42f
```

The original working archive and its dependent comparison script are intentionally not included.

## Debug mode

The normal build keeps `DEBUG_MODE` at `0`. For the publication audit, a temporary copy was built with `DEBUG_MODE` set to `1` and executed for 180 frames. Scanner-assignment and initialization diagnostics were observed. The temporary copy is not part of this repository.

## Limitations

There is no complete interactive tape for card reveal, bomb, fold, scanner, timeout, all twelve level transitions and endings. Scanner availability still depends on `scannerRevealed[0]`; this behavior remains unchanged until a dedicated regression case is available.
