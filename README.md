# TextStar text editor
## Overview

TextStar is a small text editor featuring keybindings very similar to the ones used by WordStar.

Written in ANSI-C (C89) with curses as the only dependency.

## Building
### macOS / Linux / ULTRIX (curses/ncurses)

    make

produces `build/ts`.

### DOS (OpenWatcom, DOS/4GW 32-bit, PDCurses)

    source ~/opt/openwatcom/env.txt
    wmake -f Makefile.wcc

produces `build.dos/ts.exe`. Requires PDCurses built for DOS/4GW in
`~/opt/pdcurses` (library `dos/pdcurses.lib`).
