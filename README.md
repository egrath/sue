# SUE text editor

SUE b the "Simple Unix Editor" b is a small full-screen text editor with
WordStar-style keybindings, originally written 1992b1995 by Point to Point
Communications for old Unices (ULTRIX, AIX, SVR3/4). This fork modernizes the
code (strict C89, warning-free) and brings it to modern systems b and back
to DOS.

## Features

- WordStar-style keybindings b `^J` opens the built-in help screens
- Gap-buffer editing core, block operations, search & replace, paragraph
  reform, shell escape
- UNIX/DOS/Macintosh line-ending handling: auto-detected when loading, new
  files use the platform default (LF on UNIX, CRLF on DOS)

## Removed from the original

- The F-key mode (WordStar keys only; F-keys are ignored)
- The novice mode
- Configuration via environment variables

## Building

### UNIX / macOS / Linux (ncurses)

    make

produces `build/sue`.

### DOS (OpenWatcom, DOS/4GW 32-bit, PDCurses)

    source ~/opt/openwatcom/env.txt
    wmake -f Makefile.wcc

produces `build.dos/sue.exe`. Requires PDCurses built for DOS/4GW in
`~/opt/pdcurses` (library `dos/pdcurses.lib`).

## Usage

    sue [-bdmstu] [-sn] [-tn] [file]

- `-b` black & white mode
- `-d` / `-m` / `-u` force DOS/Macintosh/Unix line endings
- `-sn` set the right margin to n
- `-tn` set the tab width to n (1..32)

New files are saved with the platform's line endings; loaded files keep the
endings detected from their content.
