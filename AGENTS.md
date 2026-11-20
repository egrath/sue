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
| `src/sue.c` | Global variable definitions, `main()`, `do_alloc()` (buffer mgmt), `edit_init()`, `edit_exit()` |
| `src/data.c` | Text-buffer primitives over the gap: `start_line`, `next_line`, `new_line`, `column`, `text_move`, `line_y`, `move_x`, `move_y`, `move_ptr`, `strtoup`, `invert`, `undel`, `undel_save` |
| `src/display.c` | All curses output: status/prompt windows, `dsp_line`/`disp_page`/`display()`, help screens (`do_help`), `xw_printw`, `prompt_st`/`prompt_key` input |
| `src/funcs.c` | Editor commands: find/replace, reform, save/load (`save`/`load`), block ops (`block_funcs`), `add_text`, deletions, `do_shell`, `sabort` (signal handler) |
| `src/command.c` | Input dispatch: `key_scan` and the ^Q/^K/^O menus (`q_key`/`k_key`/`o_key`), SIGALRM trampolines for the timed help screens |
| `src/sue.h` | Shared types, globals (`extern`), full C89 prototypes |
| `src/version.h` | Version string |
| `Makefile.wcc` | OpenWatcom build for DOS (DOS/4GW + PDCurses) |
| `_original/sue07i.tgz` | Pristine 0.7i sources, reference only |

Object files and the executable are built into `build/` (UNIX) and
`build.dos/` (DOS).

### Core data structure: gap buffer

Text lives in one malloc'd area as two segments with the **gap** between them
(`gap_beg`/`gap_end`); the cursor sits in the gap. Inserts shrink the gap,
deletions grow it, `text_move()` relocates it. Block markers (`blk_beg`/`blk_end`
and the `t*` temporaries) must "jump the gap" when the gap moves past them.
`txt_beg`/`txt_end` bound the buffer; the buffer always ends with an `[EOF]`
marker and trailing newlines so all of the text can be treated as a string.

## Build

- **UNIX/macOS/Linux**: plain `make` builds `build/sue`.
- **DOS** (OpenWatcom, DOS/4GW 32-bit flat, PDCurses):
  `source ~/opt/openwatcom/env.txt && wmake -f Makefile.wcc`
  builds `build.dos/sue.exe` (`wmake -f Makefile.wcc clean` cleans).
- Requirements: a C89 compiler and curses (ncurses on UNIX, PDCurses on DOS —
  `~/opt/pdcurses`, library `dos/pdcurses.lib`).
- Flags: `-O2 -std=c89 -Wpedantic -Wall -Werror` — **zero warnings is a hard
  requirement**; do not weaken the flags to hide problems.
- DOS CFLAGS: `-bt=dos4g -mf -3 -dDOS -wx -zq -oneatx -s` — maximum speed
  optimization for 386+ systems (`-oneatx` incl. aliasing relaxation, `-s`
  no stack checks, `-3` 386 instruction set). PDCurses' own library is built
  with the same `-oneatx`.
- `EFLAGS` (currently `-DUNIX -D_DEFAULT_SOURCE`) and `LFLAGS` (`-lncurses`)
  are the override points for other targets.
- `make clean` empties `build/` but keeps the directory; `make install`
  strips + moves the binary to `BINDIR`.
- Smoke tests: `sh tests/smoke.sh` (expect + xxd) — runs five pty-driven
  round-trip tests against `build/sue`; run after every change.
- **Future targets: Windows, Macintosh, Linux, DOS** — and Solaris should
  "come free" if it compiles cleanly on the others. Add no per-platform
  `#ifdef`s beyond the platform pair (`UNIX`/`DOS`) and feature flags
  (`DEBUG`, `TRACE`, `LOOKBUSY`).
- **New files are saved with the platform's line endings** (DOS: CRLF,
  UNIX: LF); `-d`/`-m`/`-u` override, and loaded files keep their detected
  endings (the file is never rewritten to a different style than it had).
- The UNIX build is verified on macOS; the DOS build compiles and links on
  the Watcom host but has not been run on real DOS/DOSBox yet.

## Conventions / hard rules

- **DOS compatibility is mandatory for every change.** POSIX-only functions
  (`alarm`, `kill`, `popen`, `setuid`/`setgid`, `SIGHUP`/`SIGQUIT`/`SIGSYS`/
  `SIGBUS`) must be guarded with `#ifdef UNIX`; DOS-specific code (COMSPEC,
  TEMP/TMP save dir) with `#ifdef DOS`. Both builds (`make` and
  `wmake -f Makefile.wcc`) must stay warning-free. Also mind the macro
  namespaces: `<term.h>`/PDCurses define terminfo capability and `KEY_*`
  macros, and the platform defines themselves (`-DUNIX`/`-dDOS`) — which is
  why the `enum filemodes` members are `MODE_UNIX`/`MODE_MSDOS`/
  `MODE_MACINTOSH`.
- **Strict C89, no GNU extensions.** Function definitions in prototype form;
  all declarations at the top of a block.
- Single-file helpers are `static` and kept out of `sue.h` (currently:
  `test`, `dsp_line`, `disp_page`, `status_linecol`, `do_alloc`).
- Format-string helpers: `stat_print` and `status` have fixed argument lists
  (call sites pass `NULL` for unused trailing args); `xw_printw` and `prompt`
  are variadic (`...` + `<stdarg.h>`, C89-standard) because several call sites
  legitimately pass `int`/`long` printf-arguments (`baudrate()`, `txt_x`,
  `tabdef`, memory sizes) that a `char *` parameter cannot carry. Both format
  via `vsprintf` into local buffers. `prompt_st` keeps fixed args; its three
  int-substituting call sites pre-format the prompt string first.
- Signal handlers must have `void f(int)` signatures; the no-argument help
  routines get static trampolines (`alarm_*_hlp`) in `command.c`.
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
  - **Function names are lower-case snake_case** (`key_scan`, `tpa_err`,
    `del_line`, `set_file_mode`). Watch out: `<term.h>` pollutes the
    namespace with terminfo capability macros (`insert_line`, `delete_line`,
    `set_tab`, ...) — a new name must not collide (that is why the insert-a-
    newline function is `insert_newline`).
- `wp1`–`wp4` macros wrap `xw_printw`; keep their args in sync with
  `xw_printw`'s signature.

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
- 2026-11-20 — Added a DOS build (OpenWatcom `wcc386`/`wlink`, DOS/4GW
  32-bit flat, PDCurses) via Makefile.wcc producing `build.dos/sue.exe`.
  Platform code is guarded with `#ifdef UNIX` / `#ifdef DOS`: POSIX signals
  beyond SIGINT/SIGTERM/SIGSEGV/SIGILL, the alarm-based timed help (DOS
  shows the help screens at once instead), setuid/setgid, the popen import
  branch (on DOS ">" is DOS redirection and passes through), SHELL vs
  COMSPEC and HOME vs TEMP/TMP save dirs. PATH_MAX got an #ifndef fallback,
  the unused <term.h> include was removed, the UNIX build defines -DUNIX,
  and the enum filemodes members became MODE_UNIX/MODE_MSDOS/MODE_MACINTOSH
  because -DUNIX collided with the old UNIX enumerator.
- 2026-11-20 — New files default to the platform's file mode: DOS builds
  save with CRLF (MODE_MSDOS), UNIX builds with LF (MODE_UNIX); -d/-m/-u
  override, and loaded files keep their detected endings (load() preserves
  the style the file already had).
- 2026-11-20 — Closed out the remaining review findings: find() no longer
  uppercases the search string in place (an uppercased copy is compared),
  del_line() restores the cursor position when its undo-save fails, ^P ^@
  (NUL) is rejected with a beep, and the dinsln redraw at the top screen
  line no longer writes at row -1. Added tests/smoke.sh (expect-based
  round-trip tests) and refreshed README.md. Kept: sabort() curses-after-
  endwin messages (intentional), load() reporting lines+1 (only truthiness
  is used), the 80286 truncation loop in do_alloc() (vestigial but harmless).
- 2026-11-20 — Reformatted all sources to the maintainer's style (Allman
  braces, always-braced bodies, space between function name and paren
  everywhere, floating `*` in return types, name-glued `*` in declarations).
  Pure formatting: verified token-identical after whitespace/brace
  normalization (only exception: one pair of redundant parentheses removed in
  `load()`), zero-warning rebuild, smoke tests byte-perfect.
- 2026-11-20 — Made single-file helpers `static` and removed them from `sue.h`.
- 2026-11-20 — Renamed all functions to lower-case snake_case (key_scan,
  tpa_err, find_func, quit_func, set_file_mode, del_line, insert_newline,
  ...). Rule: underscore at word/abbreviation boundaries, abbreviations kept;
  `insertline` became `insert_newline` (not `insert_line`) because term.h
  defines terminfo capability macros like `insert_line` via <term.h>.
  Variables and macros intentionally untouched.
- 2026-11-20 — Moved the `WINDOW *editwin/promptwin/statuswin/helpwin`
  definitions from `sue.h` (tentative definitions in a header) to `sue.c`;
  header now declares them `extern`. Modern toolchains default to
  `-fno-common`, which rejects duplicate tentative definitions.
- 2026-11-20 — Removed the manual `optarg`/`optind`/`opterr` externs from
  `sue.h`; `<unistd.h>` supplies them (Linux needs `-D_DEFAULT_SOURCE`).
- 2026-11-20 — `.opencode/session.txt` is the running session log
  (gitignored); this file is the durable knowledge base (tracked).

## Known issues / notes (not yet fixed)

- `sabort()` runs after `endwin()`, so its `beep()`/`prompt_key()` calls operate
  on dead windows (original behavior, intentionally kept).
- `load()` returns lines+1 — only truthiness is used by callers, so this is
  harmless; changing it would need care with the empty-file case.
- `prompt()`/`xw_printw()` format via unbounded `sprintf`/`vsprintf` into
  local buffers — safe with the current internal callers, but new call sites
  must keep format strings and argument types bounded.
- The alarm-based timed help calls curses from signal context — technically
  non-reentrant; works in practice (original design, kept).
- Buffer arithmetic assumes 7-bit text (`c &= 0x7F` on load) — no UTF-8 support.
- The Linux target was updated for strict-C89 feature macros but is only
  build-verified on macOS.
