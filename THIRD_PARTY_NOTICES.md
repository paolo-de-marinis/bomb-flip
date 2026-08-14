# Third-party notices

The GPL-3.0-or-later license in `LICENSE` applies only to material for which Paolo De Marinis owns the copyright. It does not relicense the third-party material listed below.

## SEQT

- File: `src/seqt.h`
- Upstream: [edubart/seqtoy](https://github.com/edubart/seqtoy)
- Upstream author: Eduardo Bart
- Vendored file: byte-for-byte identical to the current upstream `seqt.h` (Git blob `f32126fbf0b040d5ca8c423c6d4e836b798ea804` at the time of this notice)

The upstream repository does not state a standard open-source license. Its README nevertheless gives RIV cartridge developers an explicit usage instruction under **Using in RIV cartridges**: copy `seqt.h` into the project and use it. This repository relies only on that upstream permission for the unmodified file and for its use by this cartridge. `seqt.h` remains subject to the terms and permissions of its upstream author; Paolo De Marinis does not offer it under the GPL or claim broader rights.

[`SEQT_EXCEPTION.md`](SEQT_EXCEPTION.md) grants a GPLv3 §7 additional permission only for material whose copyright Paolo De Marinis controls, allowing that material to be combined with the SEQT implementation used by this cartridge. It does not alter or grant rights in `seqt.h`.

## RIV API and SDK

This repository does **not** redistribute `riv.h`. Production builds use the `riv.h` supplied by the installed official RIV SDK. The host-side `strict` and `test` targets copy that SDK header into a temporary build directory only for the duration of local checks.

Upstream RIV project: [rives-io/riv](https://github.com/rives-io/riv). The RIV project currently does not advertise a standard repository-wide license, so this repository deliberately avoids shipping a copy of its API header.
