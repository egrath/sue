/* #######################################################################
 * display.c - TextStar curses output.
 * purpose:
 *     Everything the user sees: the status line (file name, cursor
 *     position, toggles), the prompt/warning line, the text repaint
 *     machinery (dsp_line/disp_page/display driven by the disp_mode
 *     ordinal), the help screens and the small user-feedback helpers
 *     (beep-and-message errors).
 * notes:
 *     Commands never paint text themselves; they set disp_mode (an
 *     ORDERED enum - higher values mean bigger repaints) and the main
 *     loop calls display(), which decides between one-line updates,
 *     line inserts/deletes, scrolls and full pages.
 * ####################################################################### */

#include "ts.h"
#include <stdarg.h>

/* #######################################################################
 * void val_err (void)
 * purpose:
 *     Feedback for an invalid numeric value typed by the user
 *     (goto line, margin, tab width).
 * returns:
 *     Nothing.
 * parameters:
 *     None.
 * ####################################################################### */
void val_err (void)
{
	beep ();
	prompt_key ("Value error ");
}

/* #######################################################################
 * void not_found (void)
 * purpose:
 *     Feedback for a failed string search.
 * returns:
 *     Nothing.
 * parameters:
 *     None.
 * ####################################################################### */
void not_found (void)
{
	beep ();
	prompt_key ("Text not found ");
}
/* #######################################################################
 * int blk_err (char level)
 * purpose:
 *     Check that a block operation may run: either no block is marked
 *     (markers outside the text) or - for operations that act at the
 *     cursor (level > 1) - the cursor sits inside the marked block.
 *     Beeps and reports "Block error" when the operation must not run.
 * returns:
 *     1 = error, refuse the operation; 0 = ok.
 * parameters:
 *     char level: 1 = only check the markers' validity; 2+ = also
 *     reject when the cursor is inside the block.
 * ####################################################################### */
int blk_err (char level)
{
	if (((level > 1) &&
	     (gap_end > blk_beg) &&
	     (gap_end <= blk_end))	|| 					/* In block */
	    (blk_beg < txt_beg) || (blk_end > txt_end))	/* No block */
	{
		beep ();
		prompt_key ("Block error ");
		return (1);
	}
	else
	{
		return (0);	/* no errors */
	}
}
/* #######################################################################
 * void key_err (void)
 * purpose:
 *     Feedback for an unrecognized command key.
 * returns:
 *     Nothing.
 * parameters:
 *     None.
 * ####################################################################### */
void key_err (void)
{
	beep ();
	prompt_key ("Unknown command ");
}
/* #######################################################################
 * void tpa_err (void)
 * purpose:
 *     Feedback for out-of-memory conditions (buffer allocation, text
 *     insertion with no gap space left).
 * returns:
 *     Nothing.
 * parameters:
 *     None.
 * ####################################################################### */
void tpa_err (void)
{
	beep ();
	prompt_key ("Out of memory! ");
}
/* #######################################################################
 * char abandon (void)
 * purpose:
 *     Ask the user whether modified text may be abandoned (quit, new
 *     file). Prompts "File modified; Abandon (Y/N)?", defaulting to N.
 * returns:
 *     'Y' when the user confirmed (uppercased), any other character
 *     (typically N) otherwise.
 * parameters:
 *     None.
 * notes:
 *     Callers compare the result against 'Y'; echoing the confirmed
 *     Y keeps the prompt line honest.
 * ####################################################################### */
char abandon (void)
{
	int	c;

	c = toupper (prompt_key ("File modified; Abandon (Y/N)? N\b"));
	if (c == 'Y')
	{
		wechochar (promptwin, c);
	}
	return (c);
}
/* #######################################################################
 * int wait_key (void)
 * purpose:
 *     Await one keystroke, reading from the prompt window.
 * returns:
 *     The key code as curses reports it (ASCII or extended KEY_*).
 * parameters:
 *     None.
 * notes:
 *     The two-key menus (^Q/^K/^O) take their second key here so the
 *     help-window repaint does not steal focus.
 * ####################################################################### */
int wait_key (void)
{
	return (wgetch (promptwin));
}
/* #######################################################################
 * void out_char (char a)
 * purpose:
 *     Output one character to the edit window, using the block
 *     attribute when the position falls inside the marked block.
 * returns:
 *     Nothing.
 * parameters:
 *     char a: the character to display.
 * notes:
 *     The block test uses gap_beg (the cursor side of the gap) - the
 *     character being painted is the one at the cursor.
 * ####################################################################### */
void out_char (char a)
{
	if ((blk_beg <= gap_beg) && (blk_end > gap_beg))
	{
		/* Must highlight if within block */
		/*	wattron(editwin, BLOCKATTR);*/
		waddch (editwin, (chtype) a | BLOCKATTR);
	}
	else
	{
		/*	wattroff(editwin, BLOCKATTR);*/
		waddch (editwin, a);
	}
}
/* #######################################################################
 * void stat_print (int col, char *st1, char *st2, char *st3)
 * purpose:
 *     Print a fragment into the status line at column "col" without
 *     erasing the rest of it, then refresh.
 * returns:
 *     Nothing.
 * parameters:
 *     int col: 0-based status-line column to start at.
 *     char *st1: the text (a printf format, usually plain text).
 *     char *st2, char *st3: optional printf arguments - pass NULL
 *     when unused (the trailing-NULL convention of this codebase).
 * ####################################################################### */
void stat_print (int col, char *st1, char *st2, char *st3)
{
	mvwprintw (statuswin, 0, col, st1, st2, st3);
	wrefresh (statuswin);
}

/* #######################################################################
 * static void status_linecol (void)
 * purpose:
 *     Refresh the "L n C n RM n" fragment of the status line (at
 *     column COLS - 56): cursor line, cursor column and the right
 *     margin. No margin set (rt_marg = 0) shows as "RM -".
 * returns:
 *     Nothing.
 * parameters:
 *     None; reads txt_x/txt_y.
 * notes:
 *     The label buffer holds files with 100000+ lines (sized
 *     accordingly - an older fixed-width version overflowed).
 * ####################################################################### */
static void status_linecol (void)
{
	char label_pos[40];
	char rm_label[8];

	if (rt_marg > 0)
	{
		sprintf (rm_label, "%d", rt_marg);
	}
	else
	{
		strcpy (rm_label, "-");	/* No right margin set */
	}
	sprintf (label_pos, "L %-5d C %-5d RM %-6s", txt_x, txt_y, rm_label);
	stat_print (COLS - 56, label_pos, NULL, NULL);
}

/* #######################################################################
 * void status (int col, char *st1, char *st2, char *st3)
 * purpose:
 *     Clear and redraw the WHOLE status line, then print the given
 *     text at column "col". Used for transient messages like "SUE
 *     loading..." / "TextStar writing ...".
 * returns:
 *     Nothing.
 * parameters:
 *     int col: column for the message.
 *     char *st1: printf-style format (the message).
 *     char *st2, char *st3: printf arguments or NULL.
 * notes:
 *     The line is erased by painting COLS blanks (older curses have
 *     no reliable wclrtoeol on a cleared window), restoring STATATTR.
 * ####################################################################### */
void status (int col, char *st1, char *st2, char *st3)
{
	int colcnt;

	werase (statuswin);
	/* Restore video attributes: */
	for (colcnt = COLS; (colcnt) ; colcnt--)
	{
		waddch (statuswin, ' ');
	}
	stat_print (col, st1, st2, st3);
}
/* #######################################################################
 * void status_line (void)
 * purpose:
 *     Rebuild the standard status line: current file name (shortened
 *     to its basename), "L n C n" cursor position, the MR/INS toggle
 *     indicators when set, and the "^J for help" hint (suppressed
 *     while a help window is open).
 * returns:
 *     Nothing.
 * parameters:
 *     None; reads txtname, txt_x/txt_y, MRflag, insflag, helplines.
 * notes:
 *     Called after every command that changes the cursor or state.
 * ####################################################################### */
void status_line (void)
{
	char	*nameptr;

	/* Show the file name only - strip any directory part (on DOS both
	 * '/' and '\' separate directories). */
	nameptr = strrchr (txtname, '/');
#ifdef DOS
	{
		char	*dosptr = strrchr (txtname, '\\');

		if (dosptr != NULLCHARPTR && dosptr > nameptr)
		{
			nameptr = dosptr;
		}
	}
#endif
	if (nameptr != NULLCHARPTR)
	{
		nameptr++;
	}
	else
	{
		nameptr = txtname;
	}
	status (0, "%s", nameptr, NULL);

	/* Show cursor position */
	status_linecol ();

	/* Show toggle status */
	if (MRflag)
	{
		stat_print (COLS - 27, "MR", NULL, NULL);			/* Margin Release */
	}
	if (insflag)
	{
		stat_print (COLS - 24, "INS", NULL, NULL);		/* INSert status */
	}

	if (helplines)		/* Don't show redundant msg */
	{
		return;
	}
	else
	{
		stat_print (COLS - 11, "^J for help", NULL, NULL);
	}
}
/* #######################################################################
 * void prompt (int col, char *st1, ...)
 * purpose:
 *     Show a prompt on the prompt line (the same row the edit window
 *     overlaps): copy the edit-window background under it, print the
 *     formatted text in PROMPTATTR and tell curses to repaint the edit
 *     window on the next refresh so the prompt is only an overlay.
 * returns:
 *     Nothing.
 * parameters:
 *     int col: 0-based column for the prompt.
 *     char *st1: printf-style format string.
 *     ...: printf arguments for st1.
 * notes:
 *     Variadic on purpose: several call sites pass int/long printf
 *     arguments. The formatting goes through vsprintf into a local
 *     buffer - keep the format bounded.
 * ####################################################################### */
void prompt (int col, char *st1, ...)
{
	va_list	args;
	char	pbuf[256];

	va_start (args, st1);
	vsprintf (pbuf, st1, args);
	va_end (args);

	wattrset (promptwin, A_NORMAL);
	/* Copy editwin background under the prompt; affects the next overwrite only. */
	overwrite (editwin, promptwin);
	wattrset (promptwin, PROMPTATTR);
	mvwprintw (promptwin, 0, col, "%s", pbuf);
	wclrtoeol (promptwin);
	wrefresh (promptwin);
	/* touchwin() instead of touchline() is needed for DEC */
	touchwin (editwin);			/* Tell curses to redraw */
								/* ..on next 'refresh' call */
}
/* #######################################################################
 * void warning (char *strptr)
 * purpose:
 *     Show a short warning/prompt right-aligned on the prompt line
 *     ("New file", "^K-", "Block error", ...).
 * returns:
 *     Nothing.
 * parameters:
 *     char *strptr: the message.
 * notes:
 *     Sets warnflag state via the caller (main's loop suppresses the
 *     next repaint so the warning stays readable until any key).
 * ####################################################################### */
void warning (char *strptr)
{
	prompt (COLS - strlen (strptr) - 3, "  %s", strptr, NULL, NULL);
}
/* #######################################################################
 * int prompt_st (char *st1, char *st2, char *inputst, int inlen)
 * purpose:
 *     The full-screen string input line: prompt, then edit a string
 *     with backspace delete, ^P to enter literal control codes and ^U
 *     (or ESC) to abort. Control characters are displayed as
 *     underlined "@letter".
 * returns:
 *     The number of characters stored, or -1 when the user aborted.
 * parameters:
 *     char *st1: prompt format string (printed via prompt()).
 *     char *st2: its printf argument (or NULL).
 *     char *inputst: the caller's buffer to fill (NUL-terminated).
 *     int inlen: capacity of inputst.
 * notes:
 *     A NUL byte cannot be entered (rejected with a beep); keys above
 *     255 (F-keys etc.) are ignored. Restores the status line and the
 *     prompt attributes before returning.
 * ####################################################################### */
int prompt_st (char *st1, char *st2, char *inputst, int inlen)
{
	int		scresult;
	int		abortflag;
	int		keyin;
	int		cury, curx;
	char	editing;

	wattron (statuswin, A_BOLD);
	status (0, "BS or DEL erase, ^P enters control codes", NULL, NULL);
	stat_print (COLS - 12, "^U to abort", NULL, NULL);
	wattrset (statuswin, A_NORMAL);	/* The status line is plain */

	prompt (0, st1, st2, NULL, NULL);			/* Prompt user */
	wattrset (promptwin, 0);			/* Don't hilite user text */
	editing   = 1;
	scresult  = 0;
	abortflag = 0;

	while (editing)
	{
		keyin = wait_key ();
		switch (keyin)
		{
		case KEY_ENTER:
		case '\r':
			editing = 0;
			break;

		case KEY_DC:
		case KEY_BACKSPACE:
		case BS:
		case DEL:
			if (scresult)
			{
				scresult--;
				getyx (promptwin, cury, curx);
				curx--;
				mvwdelch (promptwin, cury, curx);
			}
			break;

		case CNTRL_U:
		case ESC:
			editing = 0;
			scresult = 0;
			abortflag = 1;
			break;

		case CNTRL_P:			/* Allows ESC, ^M, ^U, ^P */
			if (scresult >= inlen)
			{
				beep ();		/* Buffer overflow error */
				sleep (1);  	/* For slow terminals */
				break;
			}
			keyin = (wait_key () & 0x1F);		/* Xlate to ctl-code */
			/* FALLTHRU */
		default:
			if (keyin > 255)	/* Unhandled extended key (F-Keys etc.) */
			{
				beep ();
				break;
			}
			if (scresult >= inlen)
			{
				beep ();
				sleep (1);
				break;
			}
			if (keyin == 0)	/* Not compatible with */
			{
				beep ();			/* standard C functions */
				break;
			}
			if (iscntrl (keyin))
			{
				wattron (promptwin, A_UNDERLINE);
				waddch (promptwin, (chtype) keyin | '@');
				wattroff (promptwin, A_UNDERLINE);
			}
			else
			{
				waddch (promptwin, (chtype) keyin);
			}
			*(inputst + scresult++) = keyin;
			break;
		}
		/* Needed for earlier curses releases: */
		wrefresh (promptwin);
	}
	*(inputst + scresult) = '\0';	/* Terminator */
	wattrset (promptwin, A_BOLD);	/* Restore hilite */
	status_line ();					/* Restore status line */
	if (abortflag)
	{
		return (-1);
	}
	else
	{
		return scresult;				/* No. chars stored */
	}
}
/* #######################################################################
 * int prompt_key (char *strptr)
 * purpose:
 *     Show a warning-style prompt and wait for ONE keystroke. Used by
 *     all the beep-and-message error helpers and the Y/N questions.
 * returns:
 *     The keystroke (0 for unhandled extended keys, with a beep).
 * parameters:
 *     char *strptr: the prompt text (shown via warning()).
 * ####################################################################### */
int prompt_key (char *strptr)
{
	int keyin;

	warning (strptr);	/* Show prompt at right edge */
	keyin = wait_key ();
	if (keyin > 255)	/* Unhandled extended key (F-Keys etc.) */
	{
		beep ();
		return (0);
	}
	return (keyin);
}

/* #######################################################################
 * static char * dsp_line (char *line, int row)
 * purpose:
 *     Paint ONE text line on screen row "row" starting from buffer
 *     position "line": expand TABs to tab stops, render control
 *     characters as underlined "@letter" when cntlflag is set, apply
 *     the BLOCKATTR highlight inside the marked block, stop at the
 *     newline or the right margin, and mark hard-carriage lines with
 *     a '<' flag at the right edge.
 * returns:
 *     Pointer to the first character AFTER the newline (the start of
 *     the next line to paint).
 * parameters:
 *     char *line: first character of the line to display.
 *     int row: 1-based screen row to paint on.
 * notes:
 *     Skips the gap transparently while walking. The hard-CR flag is
 *     only shown when cntlflag is set and disp_mode requests it
 *     (disp_mode > dtoeol). The goto dsp_exit handles the right-margin
 *     early stop; the scan stops at the newline reached.
 * ####################################################################### */
static char * dsp_line (char *line, int row)
/* line: Pointer to first character to display; row: Screen X position */
{
	int		i;			/* Column counter */
	int		ii;			/* Tab column counter */
	int		tmp;
	int		j;
	chtype	attributes = 0;
	char	hardflag;	/* Hard CR flag */

	i = column (line);	/* Present column no */
	wmove (editwin, row - 1, i > scr_ybeg ? i - scr_ybeg : 0);
	wclrtoeol (editwin);

	/* Start highlight at start of each line */
	if ((line >= blk_beg) && (line <= blk_end))
	{
		attributes = BLOCKATTR;
		/*wattron(editwin, A_BOLD);*/
	}

	ii = 0;
	hardflag = FALSE;
	while (*line != '\n')
	{
		i += ii;		/* Need to look at previous char */
		switch (*line)
		{
		case '\t':
			tmp = ' ';
			ii = tabdef - (i-1) % tabdef;
			break;
		default:
			tmp = *line;
			if (isprint (*line) || (cntlflag))
			{
				ii = 1;
			}
			else
			{
				ii = 0;
			}
			break;
		}
		if (line == blk_beg)
		{
			/*wattron(editwin, A_BOLD);*/
			attributes = BLOCKATTR;
		}
		for (j = i; j < i+ii; j++)	/* Display char. or TAB */
		{
			if (j >= scr_ybeg)		/* if not scrolled right */
			{
				if (isprint (tmp))
				{
					waddch (editwin, (chtype) tmp | attributes);
				}
				else if (cntlflag)	/* User toggle */
				/* Display control chars, DEL as underlined CAPS */
				{
					/*wattron(editwin, A_UNDERLINE);*/
					waddch (editwin, (chtype) tmp | '@' | CNTLATTR);
					/*wattroff(editwin, A_UNDERLINE);*/
				}
				if (j >= scr_yend)
				{
					/*wattroff(editwin, A_BOLD);*/
					attributes = 0;
					line = (next_line (line)); /* Quit @ RM */
					goto dsp_exit;
				}
			}
		}
		if (line == blk_end)
		{
			attributes = 0;
			/*wattroff(editwin, A_BOLD);*/
		}
		if ((line >= gap_beg) && (line < gap_end))
		{
			line = gap_end;
		}
		else
		{
			line++;
		}
	}
	/* Newline reached */ 

	/*wattroff(editwin, A_BOLD);*/ 		/* End highlight at end of line */

	if ((line >= gap_beg) && (line < gap_end))
	{
		line = gap_end;
	}
	else
	{
		line++;
	}

	if (line == txt_beg)
	{
		hardflag = TRUE;
	}
	else if (line == gap_end)
	{
		hardflag = (*(gap_beg - 1) != ' ');
	}
	else if (line == gap_end + 1)
	{
		hardflag = (*gap_beg != ' ');
	}
	else
	{
		hardflag = (*(line - 2) != ' ');
	}

	dsp_exit:
	if ((hardflag) && (disp_mode > dtoeol) && (cntlflag))
	{
		/* If displaying ^chars */
		/*wattron(editwin, FLAGATTR);*/
		/* Hard CR indicator */
		mvwaddch (editwin, row - 1, COLS - 1, (chtype) '<' | FLAGATTR);
		/*wattroff(editwin, FLAGATTR);*/
	}
	return (line);
}
/* #######################################################################
 * static void disp_page (int x_top, int x_bot)
 * purpose:
 *     Repaint the text lines from row x_top through row x_bot: locate
 *     the buffer position of x_top with new_line() (skipping the
 *     "[EOF]" marker if the scan lands on it), then walk dsp_line()
 *     down the screen.
 * returns:
 *     Nothing.
 * parameters:
 *     int x_top, int x_bot: first and last absolute text rows
 *     (1-based) to repaint.
 * notes:
 *     Refreshes the edit window once at the end.
 * ####################################################################### */
static void disp_page (int x_top, int x_bot)
{
	char	*line;
	int	i;

	/* Set pointer to first line */

	if (new_line (x_top - txt_x) < (x_top - txt_x))	/* at [EOF] marker? */
	{
		line = txt_ptr + 5;			/* Skip past if so */
	}
	else
	{
		line = txt_ptr;
	}

	x_top = x_top - scr_xbeg + 1;	/* Relative to top of page */
	x_bot = x_bot - scr_xbeg + 1;	/*    "      "  "  "    "  */

	for (i = x_top; i <= x_bot; i++)
	{
		line = dsp_line (line, i);
	}
	wrefresh (editwin);
}
/* #######################################################################
 * static void ruler_paint (void)
 * purpose:
 *     Paint the tab ruler on screen row 1, WordStar style: a dash in
 *     every column, '|' at the first column AFTER each tab stop and a
 *     'V' at the cursor's column. Tab stops are where a TAB lands the
 *     cursor: columns tabdef + 1, 2 * tabdef + 1, ... (so with tab 8
 *     the ruler starts with 8 dashes, then 7 between the bars - the
 *     bar can never be the first character of the ruler).
 *     Shows the VISIBLE columns (starting at scr_ybeg), so the tab
 *     stops stay at their true positions under horizontal scrolling.
 * returns:
 *     Nothing.
 * parameters:
 *     None; reads scr_ybeg, tabdef, txt_y (and COLS).
 * notes:
 *     Repaints only when one of its inputs changed since the last
 *     paint - the ruler is skipped entirely on pure vertical moves
 *     of the edit screen.
 * ####################################################################### */
static void ruler_paint (void)
{
	static int last_y = -1;
	static int last_beg = -1;
	static int last_tab = -1;
	static int last_cols = -1;
	int c;

	if (txt_y == last_y && scr_ybeg == last_beg &&
		tabdef == last_tab && COLS == last_cols)
	{
		return;
	}
	last_y = txt_y;
	last_beg = scr_ybeg;
	last_tab = tabdef;
	last_cols = COLS;

	wmove (rulerwin, 0, 0);
	wclrtoeol (rulerwin);
	for (c = 0; c < COLS; c++)
	{
		int	tc;				/* The text column shown here */
		char ch;

		tc = scr_ybeg + c;
		if (tc == txt_y)
		{
			ch = 'V';		/* The cursor position indicator */
		}
		else if (tc > 1 && (tc - 1) % tabdef == 0)
		{
			ch = '|';		/* First column after a tab stop */
		}
		else
		{
			ch = '-';
		}
		waddch (rulerwin, (chtype) ch);
	}
	wrefresh (rulerwin);
}
/* #######################################################################
 * void display (void)
 * purpose:
 *     The global screen update, called from the main loop. Reads
 *     disp_mode (the ORDERED repaint request set by the last command)
 *     and adjusts the scroll position first: re-center the window when
 *     the cursor left the visible rows/columns, possibly downgrading
 *     a full repaint to a one-line scroll. Then performs the cheapest
 *     repaint that satisfies the request: single line, end-of-line,
 *     line insert/delete with curses hardware support, scroll, or full
 *     page. Ends by updating the status line and restoring the cursor.
 * returns:
 *     Nothing.
 * parameters:
 *     None; everything comes from disp_mode/txt_x/txt_y and the
 *     scr_* window limits.
 * notes:
 *     The disp_mode enum is ORDINAL on purpose: "disp_mode > dtoeol"
 *     means "this command changed enough state to need the hard-CR
 *     flags repainted". Hardware scrolling (idlok) is enabled for the
 *     scroll modes and disabled for full pages - an old curses bug.
 * ####################################################################### */
void display (void)
{
	scr_xend = scr_xbeg + editlines - 1;
	scr_yend = scr_ybeg + COLS  - 1;

	/* New screen limits */
	if ((disp_mode == dpages) &&	/* Force 1/2 screen scroll (for search) */
		(txt_x < (scr_xbeg) || txt_x > (scr_xend)))
	{
		scr_xbeg = txt_x <= (editlines)/2 ? 1 : txt_x - (editlines)/2;
		scr_xend = scr_xbeg + editlines - 1;
	}
	else if (txt_x < scr_xbeg)		/* Cursor past top of screen? */
	{
		if (scr_xbeg == txt_x + 1)	/* 1 line? */
		{
			disp_mode = dscrldn;	/* Scroll down */
		}
		else
		{
			disp_mode = dpage;
		}
		scr_xbeg = txt_x;
		scr_xend = scr_xbeg + editlines - 1;
	}
	else if (txt_x > scr_xend)		/* Cursor past end of screen? */
	{
		disp_mode = dpage;
		scr_xend = txt_x;
		scr_xbeg = scr_xend - editlines + 1;
	}

	if (txt_y < scr_ybeg)			/* Cursor to left of screen? */
	{
		scr_ybeg = txt_y - (txt_y-1) % (COLS/2);
		disp_mode = dpage;
	}
	else if (txt_y > scr_yend)		/* Cursor to right of screen? */
	{
		scr_ybeg = txt_y - COLS/2 - (txt_y-1) % (COLS/2);
		disp_mode = dpage;
	}
	scr_yend = scr_ybeg + COLS - 1;

	idlok (editwin, TRUE);	/* Default: allow hardware scrolling */

	switch (disp_mode)
	{
	case dtoeol:			/* Display to end of line */
	case dtoeolhc:			/* Same, but flags redisplay of "<" */
		dsp_line (gap_end, txt_x - scr_xbeg + 1);
		break;

	case dtoeos:			/* Delete line or redisplay from line down */
		wmove (editwin, txt_x - scr_xbeg + 1, 0);
		wdeleteln (editwin);
		disp_page (txt_x, scr_xend);
		break;

	case dinsln:  			/* Insert new line or redisplay from ln. above */
		wmove (editwin, txt_x - scr_xbeg, 0);
		winsertln (editwin);
		if (txt_x > scr_xbeg)	/* Nothing above to redraw at row 0 */
		{
			dsp_line (gap_beg, txt_x - scr_xbeg);
		}
		dsp_line (gap_end, txt_x - scr_xbeg + 1);
		wrefresh (editwin);
		break;

	case dpage:					/* Display new page */
	case dpages:				/* Same, but flags 1/2 screen scroll */
		idlok (editwin, FALSE);	/* Curses bug: hardware codes bad here */
		disp_page (scr_xbeg, scr_xend);
		break;

	case dscrlup:				/* Scroll up */
		wmove (editwin, 0, 0);
		wdeleteln (editwin);
		disp_page (scr_xend, scr_xend);
		break;

	case dscrldn:				/* Scroll down */
		wmove (editwin, 0, 0);
		winsertln (editwin);
		disp_page (scr_xbeg, scr_xbeg);
		break;

	case dnothing:
		break;
	}
	idlok (editwin, FALSE);		/* Disable hardware scrolling for help, etc.*/

	/* update cursor position in status line */
	status_linecol ();

	/* keep the tab ruler in sync (row 1) */
	ruler_paint ();

	/* Restore cursor */
	wmove (editwin, txt_x - scr_xbeg, txt_y - scr_ybeg);
	/* Needed for earlier curses implementations: */
	wrefresh (editwin);
}

/* ================= HELP SCREEN DEFINITIONS & ROUTINES ================== */

#define HWIDTH 78

/* Primary help screen */
#define CP0 " CURSOR MOVEMENT    SCROLLING      DELETING           OTHER COMMANDS          "
#define CP1 "   `^E`    `^F`=word   `^W`=back       `Del`,`Backspace`   `^U`=undo        `^V`=insert     "
#define CP2 " `^S`  `^D`     right  `^Z`=forward      =char left    `^N`=insert line `^P`=enter code "
#define CP3 "   `^X`    `^A`=word   `^R`=page back  `^G`=char right   `^L`=find or replace next      "
#define CP4 "or `arrow`    left   `^C`=page fwd   `^T`=word right   `^B`=reform paragraph          "
#define CP5 "  `keys`                           `^Y`=whole line   `^^`=change case               "
#define CP6 "[ MENU KEYS: `^K or ESC`=block/file, `^Q`=quick, `^O`=onscreen ]"

/* Quick help screen */
#define CQ0 "     QUICK MOVEMENT            SEARCHING                 DELETING             "
#define CQ1 " `^Q-R`=go to top of file        `^Q-F`=find         `^Q-Y`=erase to end of line    "
#define CQ2 " `^Q-C`=go to end of file        `^Q-A`=replace                                   "
#define CQ3 " `^Q-S`=go to start of line                                                     "
#define CQ4 " `^Q-D`=go to end of line                                                       "
#define CQ5 " `^Q-I`=go to line number                                                       "
#define CQ6 "                                                                                "

/* Block help screen */
#define CB0 "      SAVE/EXIT                 BLOCKS                    FILES               "
#define CB1 " `^K-S`=save & resume       `^K-B`=begin  `^K-K`=end      `^K-F`=change file mode     "
#define CB2 " `^K-X`=save & exit         `^K-C`=copy   `^K-V`=move     `^K-L`=load new file        "
#define CB3 " `^K-Q`=quit                `^K-Y`=erase  `^K-H`=hide     `^K-N`=change file name     "
#define CB4 " `^K-D`=save & load new     `^K-W`=write to file        `^K-I`=file information   "
#define CB5 " `^K-!`=shell escape        `^K-R`=read in a file                                 "
#define CB6 "[ `ESC` key may be substituted for `control-K` ]"

/* Onscreen help screen */
#define CO0 "              MARGINS & TABS                          SCREEN DISPLAY          "
#define CO1 "           `^O-X`=margin release                  `^O-D`=control code display     "
#define CO2 "           `^O-R`=set right margin                `^O-L`=refresh screen.          "
#define CO3 "           `^O-T`=set hard tab spacing                                            "
#define CO4 "                                                                              "
#define CO5 "                                                                              "
#define CO6 "                                                                              "            "           "

/* #######################################################################
 * void xw_printw (WINDOW *xwindow, char *string1, ...)
 * purpose:
 *     printf-style output to a curses window with inline highlighting:
 *     sections enclosed in backquotes (`like this`) are printed in
 *     FLAGATTR | A_BOLD - the help screens mark their command keys
 *     that way.
 * returns:
 *     Nothing.
 * parameters:
 *     WINDOW *xwindow: the curses window to print into.
 *     char *string1: printf-style format string.
 *     ...: printf arguments for string1.
 * notes:
 *     Variadic (help texts substitute numbers). Formats via vsprintf
 *     into a local buffer sized from TXTNAME_SIZE - keep formats
 *     bounded. Leaves the window attributes at A_NORMAL.
 * ####################################################################### */
void xw_printw (WINDOW *xwindow, char *string1, ...)
{
	va_list	args;
	int		On = 0;
	char	strbuf[TXTNAME_SIZE + 128];
	char	*xstring = strbuf;

	va_start (args, string1);
	vsprintf (xstring, string1, args);
	va_end (args);
	for ( ; *xstring ; xstring++)
	{
		if (*xstring == '`')
		{
			if (On)
			{
				wattrset (xwindow, A_NORMAL);
				On = 0;
			}
			else
			/* Note: A_BOLD needed for b/w systems: */
			{
				wattrset (xwindow, FLAGATTR | A_BOLD);
				On = 1;
			}
		}
		else
		{
			waddch (xwindow, *xstring);
		}
	}

	wattrset (xwindow, A_NORMAL);
}
/* #######################################################################
 * static void help_line (WINDOW *hwindow, int row, char *hline)
 * purpose:
 *     Paint one help-screen line: clear the row, then print the line
 *     centered - lines starting with '[' are centered within the
 *     help block (their true width), the table lines at the standard
 *     HWIDTH block offset.
 * returns:
 *     Nothing.
 * parameters:
 *     WINDOW *hwindow: the window to paint into.
 *     int row: 0-based row in that window.
 *     char *hline: the text (NULL is ignored).
 * notes:
 *     The bracket lines carry no hand padding; their real width is
 *     strlen() minus the backquotes (xw_printw consumes those as
 *     highlight toggles).
 * ####################################################################### */
static void help_line (WINDOW *hwindow, int row, char *hline)
{
	int	off;
	int	width;
	char	*p;

	if (hline == NULL)
	{
		return;
	}
	off = HWIDTH < COLS ? (COLS - HWIDTH) / 2 : 0;
	if (hline[0] == '[')
	{
		width = strlen (hline);
		for (p = hline; *p != '\0'; p++)
		{
			if (*p == '`')
			{
				width--;
			}
		}
		off += (HWIDTH - width) / 2;
	}
	wmove (hwindow, row, 0);
	wclrtoeol (hwindow);
	wmove (hwindow, row, off);
	xw_printw (hwindow, hline, NULL, NULL, NULL);
}
/* #######################################################################
 * void do_help (enum helptype helprequest, WINDOW *hwindow)
 * purpose:
 *     Paint one of the help screens (primary menu, ^Q quick, ^K block,
 *     ^O onscreen) - the seven-line texts are #defined above, command
 *     keys highlighted through xw_printw's backquote mechanism. A
 *     separator line (underline or horizontal rule) closes the screen.
 * returns:
 *     Nothing.
 * parameters:
 *     enum helptype helprequest: which screen (PRIMARY/QUICK/BLOCK/
 *     ONSCREEN).
 *     WINDOW *hwindow: the window to paint into (helpwin for the
 *     persistent top help, editwin for the timed menus).
 * notes:
 *     Centers the HWIDTH-wide text on wider terminals. On the persistent
 *     helpwin the separator follows helplines; the alarm() timeout of
 *     the timed menus re-enters this via the alarm_*_hlp trampolines.
 * ####################################################################### */
void do_help (enum helptype helprequest, WINDOW *hwindow)
{
	chtype	ULchar = ACS_HLINE;	/* From curses.h */
	int		i;
	char	*hline0, *hline1, *hline2, *hline3, *hline4, *hline5, *hline6;

	hline0 = hline1 = hline2 = hline3 = hline4 = hline5 = hline6 = NULL;

	switch (helprequest)
	{
	case PRIMARY:
		hline0 = CP0;
		hline1 = CP1;
		hline2 = CP2;
		hline3 = CP3;
		hline4 = CP4;
		hline5 = CP5;
		hline6 = CP6;
		break;
	case QUICK:
		hline0 = CQ0;
		hline1 = CQ1;
		hline2 = CQ2;
		hline3 = CQ3;
		hline4 = CQ4;
		hline5 = CQ5;
		hline6 = CQ6;
		break;
	case BLOCK:
		hline0 = CB0;
		hline1 = CB1;
		hline2 = CB2;
		hline3 = CB3;
		hline4 = CB4;
		hline5 = CB5;
		hline6 = CB6;
		break;
	case ONSCREEN:
		hline0 = CO0;
		hline1 = CO1;
		hline2 = CO2;
		hline3 = CO3;
		hline4 = CO4;
		hline5 = CO5;
		hline6 = CO6;
		break;
	default:
		warning ("Internal error!");
		return;
		/*NOTREACHED*/
		break;
	}
	if (hwindow == helpwin)
	{
		touchwin (hwindow);
	}

	help_line (hwindow, 0, hline0);
	help_line (hwindow, 1, hline1);
	help_line (hwindow, 2, hline2);
	help_line (hwindow, 3, hline3);
	help_line (hwindow, 4, hline4);
	help_line (hwindow, 5, hline5);
	help_line (hwindow, 6, hline6);

	helpstate = helprequest;	/* Global flag */

	if (hwindow == helpwin)
	{
		wmove (hwindow, helplines - 1, 0);
		wattron (hwindow, STATATTR);
		if (STATATTR == A_UNDERLINE)
		{
			ULchar = ' ';
		}
		for (i = 0; i < COLS ; i++)
		{
			waddch (hwindow, ULchar);	/* Underline screen width */
		}
		wattroff (hwindow, STATATTR);
		wrefresh (hwindow);
	}
	else
	{
		waddstr (hwindow, "\n");		/* Next line */
	}
}
