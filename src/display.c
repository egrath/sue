#include "sue.h"
#include <stdarg.h>

/* -----------------------------------------------
 * Tell user he entered bad value
 */
void val_err (void)
{
	beep ();
	prompt_key ("Value error ");
}

void not_found (void)		/* user's string search fails */
{
	beep ();
	prompt_key ("Text not found ");
}
/* -----------------------------------------------
 * Check for and report block errors
 */
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
/* ----------------------------------------------
 * User entered unknown command
 */
void key_err (void)
{
	beep ();
	prompt_key ("Unknown command ");
}
/* -------------------------------------------------
 */
void tpa_err (void)
{
	beep ();
	prompt_key ("Out of memory! ");
}
/* -----------------------------------------------
 * Confirm before abandoning text
 */
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
/* ---------------------------------------------------
 * Await keystroke from prompt window
 */
int wait_key (void)
{
	return (wgetch (promptwin));
}
/* ----------------------------------------------------
 *	Output a single character to the screen.
 */
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
/* --------------------------------------------
 * Print to status line
 */
void stat_print (int col, char *st1, char *st2, char *st3)
{
	mvwprintw (statuswin, 0, col, st1, st2, st3);
	wrefresh (statuswin);
}

/* ----------------------------------------------
 * print line number and column in the status line
 */
static void status_linecol (void)
{
	char label_pos[32];

	sprintf (label_pos, "L %-5d C %-5d", txt_x, txt_y);
	stat_print (COLS - 49, label_pos, NULL, NULL);
}

/* ---------------------------------------------
 * Redisplay status line
 */
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
/* ---------------------------------------------------
 * Restore status line
 */
void status_line (void)
{
	char	*nameptr;

	/* Find last '/' */
	if ((nameptr = strrchr (txtname, '/')) != NULLCHARPTR)
	{
		status (0, "...\"%s\"", nameptr + 1, NULL);
	}
	else
	{
		status (0, "\"%s\"", txtname, NULL);
	}

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
/* -----------------------------------------------------
 * Prompt user for string
 */
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
/* -------------------------------------------------------
 * Show warning message at right edge
 */
void warning (char *strptr)
{
	prompt (COLS - strlen (strptr) - 3, "  %s", strptr, NULL, NULL);
}
/* -------------------------------------------------------
 * Prompt for and return string
 * Returns length of input string, or -1 if aborted. (12/15/92-crw)
 */
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
	wattrset (statuswin, STATATTR);

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
/* -----------------------------------------------------------
 * Prompt for and return keystroke
 */
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

/* ---------------------------------------------------------------
 * Display a line from present pointer up to \n and
 *   return a pointer to the char AFTER the \n.
 * The line is displayed on the jTH line of the screen.
 */
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
/* ---------------------------------------------------
 * Rewrite screen from xtop to xbot.
 */
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
/* -----------------------------------------------------
 * Global screen update routine.
 * Redisplays the screen per global disp_mode variable.
 */
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
#define CP6 "        [ MENU KEYS: `^K or ESC`=block/file, `^Q or HOME`=quick, `^O`=onscreen ]    "

/* Quick help screen */
#define CQ0 "     QUICK MOVEMENT            SEARCHING                 DELETING             "
#define CQ1 " `^Q-R`=go to top of file        `^Q-F`=find         `^Q-Y`=erase to end of line    "
#define CQ2 " `^Q-C`=go to end of file        `^Q-A`=replace                                   "
#define CQ3 " `^Q-S`=go to start of line                                                     "
#define CQ4 " `^Q-D`=go to end of line                                                       "
#define CQ5 " `^Q-I`=go to line number                                                       "
#define CQ6 "        [ `HOME` key may be substituted for `control-Q` ]                         "

/* Block help screen */
#define CB0 "      SAVE/EXIT                 BLOCKS                    FILES               "
#define CB1 " `^K-S`=save & resume       `^K-B`=begin  `^K-K`=end      `^K-F`=change file mode     "
#define CB2 " `^K-X`=save & exit         `^K-C`=copy   `^K-V`=move     `^K-L`=load new file        "
#define CB3 " `^K-Q`=quit                `^K-Y`=erase  `^K-H`=hide     `^K-N`=change file name     "
#define CB4 " `^K-D`=save & load new     `^K-W`=write to file        `^K-I`=file information   "
#define CB5 " `^K-!`=shell escape        `^K-R`=read in a file                                 "
#define CB6 "        [ `ESC` key may be substituted for `control-K` ]                          "

/* Onscreen help screen */
#define CO0 "              MARGINS & TABS                          SCREEN DISPLAY          "
#define CO1 "           `^O-X`=margin release                  `^O-D`=control code display     "
#define CO2 "           `^O-R`=set right margin                `^O-L`=refresh screen.          "
#define CO3 "           `^O-T`=set hard tab spacing                                            "
#define CO4 "                                                                              "
#define CO5 "                                                                              "
#define CO6 "                                                                              "            "           "

/* -----------------------------------------------------------------
 *	Add strings to window, highlighting sections enclosed in `reverse quotes`
 *  Uses FLAGATTR | A_BOLD to highlight.
 *  Side-effect: always sets attributes for designated window to A_NORMAL.
 */
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
/* -----------------------------------------------------------------
 *	Display help in help window.
 */
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

	wmove (hwindow, 0, 0);
	wclrtoeol (hwindow);
	wmove (hwindow, 0, HWIDTH < COLS ? (COLS - HWIDTH) / 2 : 0);
	xw_printw (hwindow, hline0, NULL, NULL, NULL);
	wmove (hwindow, 1, 0);
	wclrtoeol (hwindow);
	wmove (hwindow, 1, HWIDTH < COLS ? (COLS - HWIDTH) / 2 : 0);
	xw_printw (hwindow, hline1, NULL, NULL, NULL);
	if (hline2 != NULL)
	{
		wmove (hwindow, 2, 0);
		wclrtoeol (hwindow);
		wmove (hwindow, 2, HWIDTH < COLS ? (COLS - HWIDTH) / 2 : 0);
		xw_printw (hwindow, hline2, NULL, NULL, NULL);
		wmove (hwindow, 3, 0);
		wclrtoeol (hwindow);
		wmove (hwindow, 3, HWIDTH < COLS ? (COLS - HWIDTH) / 2 : 0);
		xw_printw (hwindow, hline3, NULL, NULL, NULL);
		wmove (hwindow, 4, 0);
		wclrtoeol (hwindow);
		wmove (hwindow, 4, HWIDTH < COLS ? (COLS - HWIDTH) / 2 : 0);
		xw_printw (hwindow, hline4, NULL, NULL, NULL);
		wmove (hwindow, 5, 0);
		wclrtoeol (hwindow);
		wmove (hwindow, 5, HWIDTH < COLS ? (COLS - HWIDTH) / 2 : 0);
		xw_printw (hwindow, hline5, NULL, NULL, NULL);
		wmove (hwindow, 6, 0);
		wclrtoeol (hwindow);
		wmove (hwindow, 6, HWIDTH < COLS ? (COLS - HWIDTH) / 2 : 0);
		xw_printw (hwindow, hline6, NULL, NULL, NULL);
	}

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
/* ================= DEBUG ROUTINE ================== */

void debug_pr (char *str1, char *str2, char *str3)
{
#ifdef DEBUG
	fprintf (debugterm, str1, str2, str3);
#else
	(void) str1;	/* Unused without DEBUG */
	(void) str2;
	(void) str3;
#endif
}
