# AGENTS.md

Notes for AI agents (and humans) working on SUE. Keep this current: whenever a
change introduces a new convention, decision, or known issue, record it here.

## What this is

SUE ("Simple Unix Editor") — a curses text editor with WordStar-style
keybindings, originally written 1992–1995 by Point to Point Communications for
old Unices (ULTRIX, AIX, SVR3/4). This fork modernizes it to build warning-free
with modern compilers (macOS clang, Linux gcc) while keeping behavior identical.

Already removed vs. the original: F-key mode, novice mode, environment-variable
configuration. Added: line/column indicator in the status line.

## Source map

| File | Contents |
|---|---|
| `src/sue.c` | Global variable definitions, `main()`, `doalloc()` (buffer mgmt), `edit_init()`, `edit_exit()` |
| `src/data.c` | Text-buffer primitives over the gap: `start_line`, `next_line`, `new_line`, `column`, `text_move`, `line_y`, `move_x`, `move_y`, `move_ptr`, `strtoup`, `invert`, `undel`, `undelsave` |
| `src/display.c` | All curses output: status/prompt windows, `dsp_lne`/`disp_page`/`display()`, help screens (`dohelp`), `xwprintw`, `promptst`/`promptkey` input |
| `src/funcs.c` | Editor commands: find/replace, reform, save/load (`save`/`load`), block ops (`blockfuncs`), `addtext`, deletions, `doshell`, `sabort` (signal handler) |
| `src/command.c` | Input dispatch: `Key_scan` and the ^Q/^K/^O menus (`q_key`/`k_key`/`o_key`), SIGALRM trampolines for the timed help screens |
| `src/sue.h` | Shared types, globals (`extern`), full C89 prototypes |
| `src/version.h` | Version string |
| `_original/sue07i.tgz` | Pristine 0.7i sources, reference only |

Object files and the executable are built into `build/`.

### Core data structure: gap buffer

Text lives in one malloc'd area as two segments with the **gap** between them
(`gap_beg`/`gap_end`); the cursor sits in the gap. Inserts shrink the gap,
deletions grow it, `text_move()` relocates it. Block markers (`blk_beg`/`blk_end`
and the `t*` temporaries) must "jump the gap" when the gap moves past them.
`txt_beg`/`txt_end` bound the buffer; the buffer always ends with an `[EOF]`
marker and trailing newlines so all of the text can be treated as a string.

## Build

- **One rule for all platforms**: plain `make` builds `build/sue`. There are
  no per-platform targets or defines anymore.
- Requirements: a C89 compiler and curses/ncurses (available on every target
  platform).
- Flags: `-O2 -std=c89 -Wpedantic -Wall -Werror` — **zero warnings is a hard
  requirement**; do not weaken the flags to hide problems.
- `EFLAGS` (currently `-D_DEFAULT_SOURCE`, needed by glibc under strict C89
  for `getopt`/`popen`; ignored elsewhere) and `LFLAGS` (`-lncurses`) are the
  override points for other targets, e.g. `make LFLAGS="-lpdcurses"` for a
  future DOS/Windows PDCurses port or `make LFLAGS="-lncurses -static"` for a
  static Linux build.
- `make clean` empties `build/` but keeps the directory; `make install`
  strips + moves the binary to `BINDIR`.
- **Future targets: Windows, Macintosh, Linux, DOS** — and Solaris should
  "come free" if it compiles cleanly on the others. Add no per-platform
  `#ifdef`s; feature flags only (`DEBUG`, `TRACE`, `LOOKBUSY`).
- The unified build is verified on macOS; Linux is expected to work unchanged
  but has not been verified on this machine.

## Conventions / hard rules

- **Strict C89, no GNU extensions.** Function definitions in prototype form;
  all declarations at the top of a block.
- Single-file helpers are `static` and kept out of `sue.h` (currently:
  `test`, `dsp_lne`, `disp_page`, `status_linecol`, `doalloc`).
- Format-string helpers: `statprint` and `status` have fixed argument lists
  (call sites pass `NULL` for unused trailing args); `xwprintw` and `prompt`
  are variadic (`...` + `<stdarg.h>`, C89-standard) because several call sites
  legitimately pass `int`/`long` printf-arguments (`baudrate()`, `txt_x`,
  `tabdef`, memory sizes) that a `char *` parameter cannot carry. Both format
  via `vsprintf` into local buffers. `promptst` keeps fixed args; its three
  int-substituting call sites pre-format the prompt string first.
- Signal handlers must have `void f(int)` signatures; the no-argument help
  routines get static trampolines (`alarm_*hlp`) in `command.c`.
- No deprecated APIs: crash-save filenames are generated with `getpid()`-based
  unique names instead of `tempnam()`.
- Color support is always compiled in (the `DOCOLOR` build flag was removed);
  runtime selection happens via `has_colors()` and the `-b` command line flag.
- **Code style** (do not mix styles):
  - Allman braces: every `{` / `}` on its own line — functions, `if`/`else`,
    `switch`, loops, `struct`/`enum`. `else` on its own line.
  - Always brace single-statement `if`/`else`/`for`/`while` bodies.
  - Floating asterisk in return types (`char * function (…)`); asterisk glued
    to the name in parameters, locals and struct members (`char *localParam3`).
  - One space between function name and `(` in **definitions, prototypes and
    every call** — including library/curses calls (`initscr ()`).
    Exceptions: keywords (`if (...)`, `while (...)`), casts (`(chtype) keyin`),
    and function-like macro **definitions** (`#define wp1(str1) ...` — a space
    there would turn it into an object-like macro and break the build).
  - Locals at the top of the function, one blank line between the declaration
    block and the first statement.
  - `case` labels one tab deeper than `switch`, statements one deeper.
  - `return (x);` with a space; parenless `return x;` kept where it predates
    the style pass. Tabs, one per level.
- `wp1`–`wp4` macros wrap `xwprintw`; keep their args in sync with
  `xwprintw`'s signature.

## Decisions log

- 2026-11-20 — Overhaul: converted the entire codebase from K&R C to strict
  C89 (prototypes everywhere) so it builds warning-free with `-Wall -Wpedantic`;
  `-Werror` added to enforce it.
- 2026-11-20 — Fixed latent bugs found during the audit: `status()` counted
  status-line blanks with a `char` (overflow on terminals wider than 127 cols,
  same class of bug in `disp_page()`); `sabort()` `strcat`'d into the `HOME`
  environment string; `tempnam()` replaced (deprecated on macOS).
- 2026-11-20 — Chose "fix call sites with NULL fillers" over `<stdarg.h>` for
  the missing-argument call sites of `statprint`/`status` — but a full clean
  build proved `xwprintw` and `prompt` *must* be variadic: call sites pass
  `int`/`long` printf-arguments (baud rate, cursor position, tab width, memory
  usage) through them, which a `char *` parameter cannot legally carry under
  prototyped C. Both now use `<stdarg.h>` (C89-standard) + `vsprintf`; the
  three `promptst` call sites that substitute an int pre-format their prompt
  string instead.
- 2026-11-20 — Verified: full clean rebuild on macOS produces **zero warnings**
  under `-std=c89 -Wpedantic -Wall -Werror`; smoke tests via `expect`/pty:
  editor starts, renders status line, accepts text, saves it (byte-perfect
  round-trip) and exits cleanly.
- 2026-11-20 — Restructured the project: all sources moved to `src/` (via
  `git mv`), object files and the executable build into `build/`. The Makefile
  uses explicit per-object rules (portable `$<`/`$@`, no GNU-isms) with the
  `build` directory as a prerequisite; `sue` is an alias target so
  `make sue`/`make install` keep working.
- 2026-11-20 — One unified build rule for all platforms: the `macos`, `linux`,
  `linuxstatic`, `svr32`, `svr4`, `aix`, `ultrix`, `linuxd`, `linuxdt` and
  `lint` targets were dropped, together with their code paths (AIX
  termio/ioctl blocks, ULTRIX `cursesX.h`, `SUEV32`/`SYSV`, the dead `xLINUX`
  workaround, the Linux `SIGSYS` hack). No `-DMACOS`/`-DLINUX` defines exist
  anymore; the `#ifdef LINUX` `touchwin` in `dohelp()` became unconditional.
- 2026-11-20 — Removed the `DOCOLOR` build flag: color support is always
  compiled in; runtime selection via `has_colors()` and `-b`. The remaining
  feature `#ifdef`s are `DEBUG`, `TRACE`, `LOOKBUSY` only.
- 2026-11-20 — Bugfix pass from a full manual code review:
  - textins(): per-direction gap-space bounds (the old check allowed
    overrunning the gap by up to 3 bytes, corrupting text next to the gap);
    the newline counter scans the exclusive range now
  - replace(): space check off-by-one could clobber the cursor char when the
    gap was nearly exactly full
  - status_linecol(): label buffer overflow on files with >= 100000 lines
  - xwprintw(): info-screen buffer sized from txtname (new TXTNAME_SIZE in
    sue.h); a very long filename overflowed the old fixed 2000-byte buffer
  - doalloc(): allocates the new buffer before freeing the old one - an
    out-of-memory failure no longer leaves dangling gap pointers behind
  - doshell(): guards the respptr[-1] read on empty shell input
  - caseinvert(): re-syncs txt_ptr to gap_end - after backward gap moves
    (e.g. ^Q-S) it pointed into the gap and ^^ inverted a free byte
  - Key_scan(): repaint on KEY_RESIZE; unknown key codes > 255 no longer
    reach isprint() out-of-domain or get inserted as text
  - tab width clamped to 1..32 (char arithmetic in line_y()/column())
  - reform(): col_cnt signed; the search-back loop can no longer wrap
  - save(): checks ferror()/fclose() - disk-full no longer reports success
  - usage/getopt unified: no-op -n dropped, usage shows [-bdmstu]
  Verified with an ASan/UBSan build (110000-line file, scripted sessions)
  plus the regular smoke tests.
- 2026-11-20 — Reformatted all sources to the maintainer's style (Allman
  braces, always-braced bodies, space between function name and paren
  everywhere, floating `*` in return types, name-glued `*` in declarations).
  Pure formatting: verified token-identical after whitespace/brace
  normalization (only exception: one pair of redundant parentheses removed in
  `load()`), zero-warning rebuild, smoke tests byte-perfect.
- 2026-11-20 — Made single-file helpers `static` and removed them from `sue.h`.
- 2026-11-20 — Moved the `WINDOW *editwin/promptwin/statuswin/helpwin`
  definitions from `sue.h` (tentative definitions in a header) to `sue.c`;
  header now declares them `extern`. Modern toolchains default to
  `-fno-common`, which rejects duplicate tentative definitions.
- 2026-11-20 — Removed the manual `optarg`/`optind`/`opterr` externs from
  `sue.h`; `<unistd.h>` supplies them (Linux needs `-D_DEFAULT_SOURCE`).
- 2026-11-20 — `.opencode/session.txt` is the running session log
  (gitignored); this file is the durable knowledge base (tracked).

## Known issues / notes (not yet fixed)

- `delline()` (funcs.c): on failed `undelsave` the cursor jumps to column 1
  (original author's BUG note).
- `find()` (funcs.c) uppercases the search string in place (`strtoup(sptr)`),
  destroying it as typed.
- `sabort()` runs after `endwin()`, so its `beep()`/`promptkey()` calls operate
  on dead windows (original behavior, intentionally kept).
- Buffer arithmetic assumes 7-bit text (`c &= 0x7F` on load) — no UTF-8 support.
- The Linux target was updated for strict-C89 feature macros but is only
  build-verified on macOS.
