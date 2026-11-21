# AGENTS.md

Notes for AI agents (and humans) working on TextStar. Keep this current: whenever a
change introduces a new convention, decision, or known issue, record it here.

## What this is

TextStar — a curses text editor with WordStar-style
keybindings, originally written 1992–1995 by Point to Point Communications as
SUE ("Simple Unix Editor") for
old Unices (ULTRIX, AIX, SVR3/4). This fork modernizes it to build warning-free
with modern compilers (macOS clang, Linux gcc) while keeping behavior identical.

Already removed vs. the original: F-key mode, novice mode, environment-variable
configuration. Added: line/column indicator in the status line.

## Source map

| File | Contents |
|---|---|
| `src/ts.c` | Global variable definitions, `main()`, `do_alloc()` (buffer mgmt), `edit_init()`, `edit_exit()` |
| `src/data.c` | Text-buffer primitives over the gap: `start_line`, `next_line`, `new_line`, `column`, `text_move`, `line_y`, `move_x`, `move_y`, `move_ptr`, `strtoup`, `invert`, `undel`, `undel_save` |
| `src/display.c` | All curses output: status/prompt windows, `dsp_line`/`disp_page`/`display()`, help screens (`do_help`), `xw_printw`, `prompt_st`/`prompt_key` input |
| `src/funcs.c` | Editor commands: find/replace, reform, save/load (`save`/`load`), block ops (`block_funcs`), `add_text`, deletions, `do_shell`, `ts_abort` (signal handler) |
| `src/command.c` | Input dispatch: `key_scan` and the ^Q/^K/^O menus (`q_key`/`k_key`/`o_key`), SIGALRM trampolines for the timed help screens |
| `src/ts.h` | Shared types, globals (`extern`), full C89 prototypes |
| `src/version.h` | Version string |
| `Makefile.wcc` | OpenWatcom build for DOS (DOS/4GW + PDCurses) |
| `docs/reference.txt` | User-facing command reference (all keys and menus). **Keep this current**: every new or changed command goes in here |
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
The final newline of the text is protected: `del_right()`/`reform()` never
delete or join it away — the `[EOF]` marker must stay on its own line. Every
forward cursor scan is bounded by `txt_end` (`move_y()`, `line_y()`), so the
cursor clamps at the marker start and nothing can be written behind it.

## Build

- **UNIX/macOS/Linux**: plain `make` builds `build/ts`.
- **DOS** (OpenWatcom, DOS/4GW 32-bit flat, PDCurses):
  `source ~/opt/openwatcom/env.txt && wmake -f Makefile.wcc`
  builds `build.dos/ts.exe` (`wmake -f Makefile.wcc clean` cleans).
- Requirements: a C89 compiler and curses (ncurses on UNIX, PDCurses on DOS —
  `~/opt/pdcurses`, library `dos/pdcurses.lib`).
- Flags: `-O2 -std=c89 -Wpedantic -Wall -Werror` — **zero warnings is a hard
  requirement**; do not weaken the flags to hide problems.
- DOS CFLAGS: `-bt=dos4g -mf -dDOS -wx -zq -oneatx` (plus the PDCurses/src
  include paths) — the optimizer set for 386+ systems. Note: `-3 -s` were
  tried on top (`-3` explicit 386 codegen, `-s` no stack checks) and
  **reverted** — the binary built with them hung at startup on DOSBox and a
  real 386; faster DOS startup flags remain a pending idea. PDCurses' own
  library is built with the same `-oneatx`.
- `EFLAGS` (currently `-DUNIX -D_DEFAULT_SOURCE`) and `LFLAGS` (`-lncurses`)
  are the override points for other targets.
- `make clean` empties `build/` but keeps the directory; `make install`
  strips + moves the binary to `BINDIR`.
- Smoke tests: `sh tests/smoke.sh` (expect + xxd) — runs six pty-driven
  round-trip tests against `build/ts`; run after every change.
- **Future targets: Windows, Macintosh, Linux** — and Solaris should
  "come free" if it compiles cleanly on the others (UNIX, DOS and ULTRIX
  are done). Add no per-platform
  `#ifdef`s beyond the platform trio (`UNIX`/`DOS`/`ULTRIX`) and the
  `KEY_RESIZE` capability check — feature flags (`DEBUG`, `TRACE`,
  `LOOKBUSY`) were removed entirely in the cleanup; do not reintroduce
  them.
- **New files are saved with the platform's line endings** (DOS: CRLF,
  UNIX: LF); `-d`/`-m`/`-u` override, and loaded files keep their detected
  endings (the file is never rewritten to a different style than it had).
- **ULTRIX**: `make` detects the OS via `uname -s` and adds `-DULTRIX` —
  the build then uses DEC's extended curses (`cursesX.h`), emulates the
  extended key codes ULTRIX curses lacks (those switch cases stay dead),
  runs monochrome (ULTRIX curses has no color API), and carries a full
  block of curses prototypes in `ts.h` — ULTRIX cursesX.h declares
  almost nothing (only `initscr`/`newwin`/`subwin`/`newpad`,
  `longname`/`erasechar`/`killchar`, `wgetch`/`newterm`, plus macros).
  Sets `CC = gcc`. Needs GNU make on ULTRIX. Build and runtime verified
  by the maintainer on real hardware. (ULTRIX unistd.h also does not
  declare `getopt` or its variables — irrelevant now: TextStar has no
  `getopt` dependency at all, see the decisions log.)
- **Runtime verification**: the UNIX build is smoke-tested on macOS; the
  DOS build (`build.dos/ts.exe`) is confirmed working on real DOS by the
  maintainer; the ULTRIX build is confirmed compiling and running on real
  ULTRIX hardware. Linux compiles clean but is untested.

## Conventions / hard rules

- **Always push after committing.** Every `git commit` is immediately
  followed by `git push` to the upstream remote — a commit left unpushed
  is considered unfinished work.
- **DOS compatibility is mandatory for every change.** POSIX-only functions
  (`alarm`, `kill`, `popen`, `setuid`/`setgid`, `SIGHUP`/`SIGQUIT`/`SIGSYS`/
  `SIGBUS`) must be guarded with `#ifdef UNIX`; DOS-specific code (COMSPEC,
  TEMP/TMP save dir) with `#ifdef DOS`. Both builds (`make` and
  `wmake -f Makefile.wcc`) must stay warning-free. Also mind the macro
  namespaces: `<term.h>`/PDCurses define terminfo capability and `KEY_*`
  macros, and the platform defines themselves (`-DUNIX`/`-dDOS`) — which is
  why the `enum filemodes` members are `MODE_UNIX`/`MODE_MSDOS`/
  `MODE_MACINTOSH`.
- **No `getopt()` dependency.** Command-line parsing is the built-in parser
  in `main()` (cluster flags b/d/m/u, `-s`/`-t` with attached values). Do not
  reintroduce `getopt()`/`optarg`/`opterr`/`optind` anywhere: the Watcom clib
  getopt hangs with an empty command line on DOS, and ULTRIX's minimal
  unistd.h does not declare it.
- **Strict C89, no GNU extensions.** Function definitions in prototype form;
  all declarations at the top of a block.
- Single-file helpers are `static` and kept out of `ts.h` (currently:
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
- **Monochrome only — no color support.** The color code (has_colors/
  start_color/init_pair, the `-b` flag) was removed: the display uses the
  fixed `*_VIDEO` attributes (A_UNDERLINE/A_BOLD) on every platform. Do not
  reintroduce curses color calls.
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
  - **Function documentation**: every function definition carries a
    `#`-bordered block directly above it, with the signature repeated
    inside and the sections `purpose:` (always), `returns:` (omit for
    void), `parameters:` (omit when none) and `notes:` (only when it
    aids understanding). Everything else — variables, constructs, file
    banners — uses plain `/* ... */` comments. Never `//`. Template:

        /* ###################################################################
         * int xyz (int x, char y)
         * purpose:
         *     What the function does; can span multiple lines.
         * returns:
         *     The result and what it means.
         * parameters:
         *     int x: description of x
         *     char y: description of y
         * notes:
         *     Anything a reader needs beyond the above.
         * ################################################################### */

    New or changed functions must adhere to this style.
- `wp1`–`wp4` macros wrap `xw_printw`; keep their args in sync with
  `xw_printw`'s signature.

## Decisions log

- 2026-11-21 — Tab ruler below the status line (branch "ruler"): the
  top bar is now two lines - the plain status line, then the ruler
  row (dots, '|' at every tabdef-th column, 'V' at the cursor's
  column) carrying the underline that separates the top bar from the
  edit window; the status line itself is plain now. The ruler shows
  the visible columns (scr_ybeg..) so tab stops stay true under
  horizontal scrolling, and repaints only when txt_y/scr_ybeg/tabdef/
  COLS changed. editlines = LINES - helplines - 2 everywhere (the
  edit window starts at row 2; the ^J help overlay - the only live
  help path - lands below the ruler). Found on the way:
  toggle_help() has NO caller and helpwin is never assigned - the
  persistent-help-window paths are dead code (kept, geometry made
  consistent; a future cleanup or key binding is a separate
  decision).
- 2026-11-21 — No right margin by default: rt_marg = 0 means "no
  margin" - the old `if (rt_marg < 2) rt_marg = COLS - 5` fallback in
  main() is gone (a programmer's editor wraps nothing unless asked).
  -s0 and ^O R 0 switch the margin off; -s1/^O R 1 are legal (honest
  value, degenerate wrapping); -s requires a digit string after the
  s (!isdigit rejected - the old value <= 1 check incidentally caught
  -sx, and the first draft of the widened check let "-sx" parse as
  "-s0" and swallow the x). reform() returns at once without a
  margin, add_text()'s word wrap never triggers (rt_marg > 0 guard).
  The status line's position fragment is now "L n C n RM n" (column
  COLS - 56, value padded %-6s), showing "-" when no margin is set.
  File names in the status line are shown unquoted, basename only
  (DOS: the last '/' OR '\\' separates).
- 2026-11-20 — Project renamed SUE -> TextStar; the binary is `ts`
  (classic Unix form: the file with main() is named after the program,
  so sue.c -> ts.c and sue.h -> ts.h). User-visible strings carry the
  new name ("TextStar - a WordStar-style text editor", loading/writing
  status, shell-return prompts); identifiers containing the old name
  swept: suerev -> tsrev, sabort -> ts_abort, suestat -> filestat,
  crash-save prefix SUEsaved. -> TSsaved. (old crash files simply go
  unfound). Historical decisions-log entries keep the old names
  verbatim. Known trade-off: `ts` is also a moreutils utility on
  Linux - BINDIR (/lbin) order decides which one wins.
- 2026-11-20 — Overhaul: converted the entire codebase from K&R C to strict
  C89 (prototypes everywhere) so it builds warning-free with `-Wall -Wpedantic`;
  `-Werror` added to enforce it.
- 2026-11-20 — Fixed latent bugs found during the audit: `status()` counted
  status-line blanks with a `char` (overflow on terminals wider than 127 cols,
  same class of bug in `disp_page()`); `sabort()` `strcat`'d into the `HOME`
  environment string; `tempnam()` replaced (deprecated on macOS).
- 2026-11-20 — Chose "fix call sites with NULL fillers" over `<stdarg.h>` for
  the missing-argument call sites of `stat_print`/`status` — but a full clean
  build proved `xw_printw` and `prompt` *must* be variadic: call sites pass
  `int`/`long` printf-arguments (baud rate, cursor position, tab width, memory
  usage) through them, which a `char *` parameter cannot legally carry under
  prototyped C. Both now use `<stdarg.h>` (C89-standard) + `vsprintf`; the
  three `prompt_st` call sites that substitute an int pre-format their prompt
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
  anymore; the `#ifdef LINUX` `touchwin` in `do_help()` became unconditional.
- 2026-11-20 — Removed the `DOCOLOR` build flag: color support is always
  compiled in; runtime selection via `has_colors()` and `-b`.
- 2026-11-20 — Reformatted all sources to the maintainer's style (Allman
  braces, always-braced bodies, space between function name and paren
  everywhere, floating `*` in return types, name-glued `*` in declarations).
  Pure formatting: verified token-identical after whitespace/brace
  normalization (only exception: one pair of redundant parentheses removed in
  `load()`), zero-warning rebuild, smoke tests byte-perfect.
- 2026-11-20 — Made single-file helpers `static` and removed them from `sue.h`;
  renamed all functions to lower-case snake_case (key_scan, tpa_err, find_func,
  quit_func, set_file_mode, del_line, insert_newline, ...). Rule: underscore at
  word/abbreviation boundaries, abbreviations kept; `insertline` became
  `insert_newline` (not `insert_line`) because term.h defines terminfo
  capability macros like `insert_line` via <term.h>. Variables and macros
  intentionally untouched.
- 2026-11-20 — Closed out the remaining review findings: find() no longer
  uppercases the search string in place (an uppercased copy is compared),
  del_line() restores the cursor position when its undo-save fails, ^P ^@
  (NUL) is rejected with a beep, and the dinsln redraw at the top screen
  line no longer writes at row -1. Added tests/smoke.sh (expect-based
  round-trip tests) and refreshed README.md.
- 2026-11-20 — Bugfix pass from a full manual code review:
  - textins(): per-direction gap-space bounds (the old check allowed
    overrunning the gap by up to 3 bytes, corrupting text next to the gap);
    the newline counter scans the exclusive range now
  - replace(): space check off-by-one could clobber the cursor char when the
    gap was nearly exactly full
  - status_linecol(): label buffer overflow on files with >= 100000 lines
  - xw_printw(): info-screen buffer sized from txtname (new TXTNAME_SIZE in
    sue.h); a very long filename overflowed the old fixed 2000-byte buffer
  - do_alloc(): allocates the new buffer before freeing the old one - an
    out-of-memory failure no longer leaves dangling gap pointers behind
  - do_shell(): guards the respptr[-1] read on empty shell input
  - case_invert(): re-syncs txt_ptr to gap_end - after backward gap moves
    (e.g. ^Q-S) it pointed into the gap and ^^ inverted a free byte
  - key_scan(): repaint on KEY_RESIZE; unknown key codes > 255 no longer
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
- 2026-11-20 — DOS build: maximum speed optimization for 386 (-oneatx -s -3).
- 2026-11-20 — ULTRIX is back as a build target (build and runtime verified
  by the maintainer on real hardware): the Makefile detects it via uname,
  sets CC = gcc and defines -DULTRIX; sue.h uses DEC's extended curses
  (cursesX.h) there and emulates the extended key codes ULTRIX curses lacks
  (KEY_SLEFT/KEY_SRIGHT/KEY_SUSPEND/KEY_HELP/KEY_SFIND, wechochar). Those
  switch cases stay dead on ULTRIX, exactly like the 1995 original.
- 2026-11-20 — ULTRIX cursesX.h declares almost nothing (initscr/newwin/
  subwin/newpad, longname/erasechar/killchar, wgetch/newterm + macros), so
  sue.h carries the full prototype block for every curses routine SUE calls
  - all standard curses signatures, compatible with the old-style header
  declarations. Round 2: getopt itself is also undeclared on ULTRIX - sue.h
  prototypes it (int getopt (int, char **, char *)). The `ii` might-be-
  uninitialized warning from old gcc's conservative dataflow was fixed with
  an initializer in line_y(). ULTRIX cursesX.h has no color API, so that
  build runs monochrome (like the 1995 original, which built ULTRIX without
  DOCOLOR).
- 2026-11-20 — Removed the DEBUG/TRACE/LOOKBUSY feature flags entirely
  (never enabled, never needed - see conventions); the codebase has zero
  conditional feature code left: the only preprocessor decisions left are
  the platform guards (UNIX/DOS/ULTRIX) and the KEY_RESIZE capability
  check. A stray "sleep (1);" accidentally inserted into o_key()'s switch
  during the LOOKBUSY removal was caught by Watcom's unreachable-code
  warning and removed.
- 2026-11-20 — Runtime verification closed out: the DOS build is confirmed
  working on real DOS and the ULTRIX build compiles and works on real
  ULTRIX hardware (both maintainer-tested); the UNIX build is smoke-tested
  on macOS.
- 2026-11-20 — DOS speed flags reverted: `-3 -s` on top of `-oneatx`
  produced a binary that hung at startup on DOSBox and a real 386
  (maintainer-tested); CFLAGS are back to `-bt=dos4g -mf -dDOS -wx -zq
  -oneatx`. A MARKERS=Y stderr-startup-marker build (Makefile hook +
  #ifdef MARKERS blocks in main()) was added to isolate the hang and
  later removed again once the cause was found (see the option-parser
  entry). Faster startup flags remain a pending idea.
- 2026-11-20 — Option parsing is a built-in parser, not `getopt()`: the
  Watcom clib getopt hangs with an empty command line on DOS (reads past
  `argv[1] = NULL`), and the -s option segfaulted since the session-4
  option-string cleanup dropped a colon from the getopt string. Cluster
  options b/d/m/u; -s/-t take attached values only (`-s78`, `-t8`); the
  first non-option argument is the file. No getopt/optarg/opterr/optind
  reference remains anywhere — sue.h carries no option externs (the
  ULTRIX unistd.h gap is gone for good). MARKERS=Y stderr startup
  markers (the diagnostic build that located the DOS hang) were removed
  again after the fix.
- 2026-11-20 — [EOF] separator protection: del_right() refuses to delete
  the newline that terminates the text and reform() refuses to join it
  away (`gap_end + 1 == txt_end`), so Ctrl-G/^B can no longer merge text
  onto the [EOF] line and corrupt the buffer cascade.
- 2026-11-20 — All file I/O is binary ("rb"/"wb"/"ab"): on DOS the CRLF
  text translation no longer double-converts saves; -d/-m/-u now write
  exact CRLF/CR/LF and load()'s own line-ending detection sees the real
  bytes. Binary mode is a no-op on POSIX, so UNIX/macOS is unchanged.
- 2026-11-20 — Color support removed entirely: the display uses the fixed
  monochrome `*_VIDEO` attributes (A_UNDERLINE/A_BOLD) on every platform -
  what `-b` did is now the only behavior. The has_colors()/start_color()/
  init_pair() block and its `#ifdef ULTRIX` guard are gone (one platform
  ifdef fewer; ULTRIX ran this path already), as are the `bwopt` global and
  the `-b` option (usage is `[-dmstu]`; scripts passing -b get a usage
  error). The STATATTR/FLAGATTR/PROMPTATTR/BLOCKATTR/CNTLATTR globals stay
  as the single display-attribute abstraction (the pristine 0.7i original
  had the same mono fallback via the *_VIDEO defines).
- 2026-11-20 — line_y() clamps the cursor at the [EOF] marker: its
  forward scan loop was the one scan without a txt_end bound, so ^Q D
  (end of line) on the marker line walked the cursor through the marker
  into the sentinel padding and typing then wrote behind [EOF], splitting
  the marker on screen ("A[[EOF]" — the user-reported "[[[[[[EOF]"
  garbage). WordStar semantics kept: ^Q D on the marker line = marker
  start; text typed there appends to the last text line. Smoke test 6
  guards it (screen-stream check — the saved file alone hid the bug).

## Known issues / notes (not yet fixed)

- **ULTRIX build quirks** (compiles and links with gcc on real ULTRIX
  hardware): cursesX.h declares `wgetch`/`wmove`/etc. but NOT
  `wrefresh`/`wattrset`/`wclrtoeol` — ts.h prototypes them under
  `#ifdef ULTRIX`. It also has no color API (mono build) and no
  `KEY_SLEFT`/`KEY_SRIGHT`/`KEY_SUSPEND`/`KEY_HELP`/`KEY_SFIND` codes
  (emulated, dead cases). More missing declarations may surface — same
  one-line fix pattern.
- `ts_abort()` runs after `endwin()`, so its `beep()`/`prompt_key()` calls operate
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
