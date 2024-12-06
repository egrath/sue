/*
Project:	SUE, a Simple Unix Editor
Copyright:	SUE is copyright 1992 by Point to Point Communications.
File:		display.c
Routines:	Display.
Author:		Carson Wilson
				Apt. 1D
				1359 W. Greenleaf, Chicago, IL 60626
				internet:	carson@mcs.com
Notes:		Edit with hard tab stops set to 4.
*/

static	char	rcsid[] = "@(#)$Id: display.c,v 1.8 1995/03/19 17:01:58 carson Exp $";

#include "sue.h"

/* -----------------------------------------------
 * Tell user he entered bad value
 */
void valerr()
{
	beep();
	promptkey("Value error ");
}

void notfound()		/* user's string search fails */
{
	beep();
	promptkey("Text not found ");
}
/* -----------------------------------------------
 * Check for and report block errors
 */
int blkerr(level)
char	level;
{
	if (((level > 1) &&
	     (gap_end > blk_beg) &&
	     (gap_end <= blk_end))	|| 					/* In block */
	    (blk_beg < txt_beg) || (blk_end > txt_end))	/* No block */
	{	beep();
		promptkey("Block error ");
		return (1);
	}
	else
		return (0);	/* no errors */
}	
/* ----------------------------------------------
 * User entered unknown command
 */
void keyerr()
{
	beep();
	promptkey("Unknown command ");
}	
/* -------------------------------------------------
 */
void TPAErr()
{
	beep();
	promptkey("Out of memory! ");
}	
/* -----------------------------------------------
 * Confirm before abandoning text
 */
char abandon()
{
	int	c;

	c = toupper(promptkey("File modified; Abandon (Y/N)? N\b"));
	if (c == 'Y')
		wechochar(promptwin,c);
	return (c);
}
/* ---------------------------------------------------
 * Await keystroke from prompt window
 */
int	waitkey()
{
	return (wgetch(promptwin));
}	
/* ----------------------------------------------------
 *	Output a single character to the screen.
 */
void outchar(a)
char a;
{
	if ((blk_beg <= gap_beg) && (blk_end > gap_beg))
		/* Must highlight if within block */
	/*	wattron(editwin, BLOCKATTR);*/
		waddch(editwin, (chtype) a | BLOCKATTR);
	else
	/*	wattroff(editwin, BLOCKATTR);*/
		waddch(editwin, a);
}
/* --------------------------------------------
 * Print to status line
 */
/* VARARGS 2 */
void statprint(col, st1, st2, st3)
int col;
char *st1, *st2, *st3;
{
	mvwprintw(statuswin, 0, col, st1, st2, st3);
	wrefresh(statuswin);
}
/* ---------------------------------------------
 * Redisplay status line
 */
/* VARARGS 2 */
void status(col, st1, st2, st3)
int col;
char *st1, *st2, *st3;
{
	char colcnt;

	werase(statuswin);
	/* Restore video attributes: */
	for (colcnt = COLS; (colcnt) ; colcnt--)
		waddch(statuswin, ' ');
	statprint(col, st1, st2, st3);
}
/* ---------------------------------------------------
 * Restore status line
 */
void status_line()
{
	char	*nameptr;

	/* Find last '/' */
	if ((nameptr = strrchr(txtname, '/')) != NULLCHARPTR)
		status(0, "...\"%s\"", nameptr + 1);
	else
		status(0, "\"%s\"",txtname);

	/* Show toggle status */
	if (MRflag)
		statprint(COLS - 27, "MR");			/* Margin Release */
	if (insflag)
		statprint(COLS - 24, "INS");		/* INSert status */

	if (helplines)		/* Don't show redundant msg */
		return;
	else
	{	if (cmd_mode == CKey)
			statprint(COLS - 11, "^J for help");
		else
			statprint(COLS - 11, "F3 for help");
	}
}
/* -----------------------------------------------------
 * Prompt user for string
 */
/* VARARGS 2 */
void prompt(col, st1, st2, st3, st4)
int col;
char *st1, *st2, *st3, *st4;
{
	wattrset(promptwin, A_NORMAL);
	/* For AIX; this affects NEXT overwrite only. */
	overwrite(editwin, promptwin);
	wattrset(promptwin, PROMPTATTR);
	mvwprintw(promptwin, 0, col, st1, st2, st3, st4);
	wclrtoeol(promptwin);
	wrefresh(promptwin);
	/* touchwin() instead of touchline() is needed for DEC */
	touchwin(editwin);			/* Tell curses to redraw */
								/* ..on next 'refresh' call */
}
/* -------------------------------------------------------
 * Show warning message at right edge
 */
void warning(strptr)
char *strptr;
{
	prompt(COLS - strlen(strptr) - 3, "  %s", strptr);
}
/* -------------------------------------------------------
 * Prompt for and return string
 * Returns length of input string, or -1 if aborted. (12/15/92-crw)
 */
int promptst(st1, st2, inputst, inlen)
char *st1, *st2, *inputst;
int inlen;
{
	int		scresult;
	int		retcode;
	int		abortflag;
	int		keyin;
	int		cury, curx;
	char	editing;

	wattron(statuswin, A_BOLD);
	status(0, "BS or DEL erase, ^P enters control codes");
	statprint(COLS-18, "F1 or ^U to abort");
	wattrset(statuswin, STATATTR);

	prompt(0, st1, st2);			/* Prompt user */
	wattrset(promptwin, 0);			/* Don't hilite user text */
	editing   = 1;
	scresult  = 0;
	abortflag = 0;

	while (editing)
	{	keyin = waitkey();
		switch (keyin) {
		case KEY_ENTER:
		case '\r':
			editing = 0;
			break;

		case KEY_DC:
		case KEY_BACKSPACE:
		case BS:
		case DEL:
			if (scresult)
			{	scresult--;
				getyx(promptwin, cury, curx);
				curx--;
				mvwdelch(promptwin, cury, curx);
			}
			break;

		case KEY_F(1):			/* Cancel */
		case CNTRL_U:
		case ESC:
			editing = 0;
			scresult = 0;
			abortflag = 1;
			break;

		case CNTRL_P:			/* Allows ESC, ^M, ^U, ^P */
			if (scresult >= inlen)
			{	beep();		/* Buffer overflow error */
				sleep(1);  	/* For slow terminals */
				break;
			}
			keyin = (waitkey() & 0x1F);		/* Xlate to ctl-code */
			/* FALLTHRU */
		default:
			if (scresult >= inlen)
			{	beep();
				sleep(1);
				break;
			}
			if (keyin == NULL)	/* Not compatible with */
			{	beep();			/* standard C functions */
				break;
			}
			if (iscntrl(keyin))
			{	wattron(promptwin, A_UNDERLINE);
				waddch(promptwin, (chtype) keyin | '@');
				wattroff(promptwin, A_UNDERLINE);
			}
			else
				waddch(promptwin, (chtype) keyin);
			*(inputst + scresult++) = keyin;
			break;
		}
		/* Needed for earlier curses releases: */
		wrefresh(promptwin);
	}
	*(inputst + scresult) = '\0';	/* Terminator */
	wattrset(promptwin, A_BOLD);	/* Restore hilite */
	status_line();					/* Restore status line */
	if (abortflag)
		return (-1);
	else
		return scresult;				/* No. chars stored */
}
/* -----------------------------------------------------------
 * Prompt for and return keystroke
 */
int	promptkey(strptr)
char *strptr;
{
	warning(strptr);	/* Show prompt at right edge */
	return (waitkey());
}

/* ---------------------------------------------------------------
 * Display a line from present pointer up to \n and
 *   return a pointer to the char AFTER the \n.
 * The line is displayed on the jTH line of the screen.
 */
char	*dsp_lne(line, row)
char	*line;	/* Pointer to first character to display   */
int		row;	/* Screen X position */
{
	int		i;			/* Column counter */
	int		ii;			/* Tab column counter */
	int		tmp;
	int		j;
	chtype	attributes = 0;
	char	hardflag;	/* Hard CR flag */
	
	i = column(line);	/* Present column no */
	wmove(editwin, row - 1, i > scr_ybeg ? i - scr_ybeg : 0);
	wclrtoeol(editwin);

	/* Start highlight at start of each line */
	if ((line >= blk_beg) && (line <= blk_end))
		attributes = BLOCKATTR;
		/*wattron(editwin, A_BOLD);*/

	ii = 0;
	hardflag = FALSE;
	while (*line != '\n')
	{	i += ii;		/* Need to look at previous char */
		switch(*line) {
		case '\t':
			tmp = ' ';
			ii = tabdef - (i-1) % tabdef;
			break;
		default:
			tmp = *line;
			if (isprint(*line) || (cntlflag))
				ii = 1;
			else
				ii = 0;
			break;
		}
		if (line == blk_beg)
			/*wattron(editwin, A_BOLD);*/
			attributes = BLOCKATTR;
		for (j = i; j < i+ii; j++)	/* Display char. or TAB */
		{	if (j >= scr_ybeg)		/* if not scrolled right */
			{	if (isprint(tmp))
					waddch(editwin, (chtype) tmp | attributes);
				else if (cntlflag)	/* User toggle */
				/* Display control chars, DEL as underlined CAPS */
				{	/*wattron(editwin, A_UNDERLINE);*/
					waddch(editwin, (chtype) tmp | '@' | CNTLATTR );
					/*wattroff(editwin, A_UNDERLINE);*/
				}
				if (j >= scr_yend)
				{	/*wattroff(editwin, A_BOLD);*/
					attributes = 0;
					line = (next_line(line)); /* Quit @ RM */
					goto dsp_exit;
				}
			}
		}
		if (line == blk_end)
			attributes = 0;
			/*wattroff(editwin, A_BOLD);*/
		if ((line >= gap_beg) && (line < gap_end))
			line = gap_end;
		else
			line++;
	}
	/* Newline reached */ 

	/*wattroff(editwin, A_BOLD);*/ 		/* End highlight at end of line */

	if ((line >= gap_beg) && (line < gap_end))
		line = gap_end;
	else
		line++;

	if (line == txt_beg)
		hardflag = TRUE;
	else if (line == gap_end)
		hardflag = (*(gap_beg - 1) != ' ');
	else if (line == gap_end + 1)
		hardflag = (*gap_beg != ' ');
	else
		hardflag = (*(line - 2) != ' ');

	dsp_exit:
	if ((hardflag) && (disp_mode > dtoeol) && (cntlflag))
		/* If displaying ^chars */
 	{	/*wattron(editwin, FLAGATTR);*/
		/* Hard CR indicator */
		mvwaddch(editwin, row - 1, COLS - 1, (chtype) '<' | FLAGATTR );
		/*wattroff(editwin, FLAGATTR);*/
	}
	return (line);
}
/* ---------------------------------------------------
 * Rewrite screen from xtop to xbot.
 */
void	disp_page(x_top, x_bot)
int x_top, x_bot;
{
	char	*line;
	char	i;

	/* Set pointer to first line */

	if (new_line(x_top - txt_x) < (x_top - txt_x))	/* at [EOF] marker? */
		line = txt_ptr + 5;			/* Skip past if so */
	else
		line = txt_ptr;

	x_top = x_top - scr_xbeg + 1;	/* Relative to top of page */
	x_bot = x_bot - scr_xbeg + 1;	/*    "      "  "  "    "  */

	for (i = x_top; i <= x_bot; i++)
		line = dsp_lne(line, i);
	wrefresh(editwin);
}
/* -----------------------------------------------------
 * Global screen update routine.
 * Redisplays the screen per global disp_mode variable.
 */
void	display()
{
	scr_xend = scr_xbeg + editlines - 1;	
	scr_yend = scr_ybeg + COLS  - 1;

	/* New screen limits */
	if ((disp_mode == dpages) &&	/* Force 1/2 screen scroll (for search) */
		(txt_x < (scr_xbeg) || txt_x > (scr_xend)))
	{	scr_xbeg = txt_x <= (editlines)/2 ? 1 : txt_x - (editlines)/2;
		scr_xend = scr_xbeg + editlines - 1;	
	}
	else if (txt_x < scr_xbeg)		/* Cursor past top of screen? */
	{	if (scr_xbeg == txt_x + 1)	/* 1 line? */
			disp_mode = dscrldn;	/* Scroll down */
		else
			disp_mode = dpage;
		scr_xbeg = txt_x;
		scr_xend = scr_xbeg + editlines - 1;
	}
	else if (txt_x > scr_xend)		/* Cursor past end of screen? */
	{	disp_mode = dpage;
		scr_xend = txt_x;
		scr_xbeg = scr_xend - editlines + 1;
	}

	if (txt_y < scr_ybeg )			/* Cursor to left of screen? */
	{	scr_ybeg = txt_y - (txt_y-1) % (COLS/2);
		disp_mode = dpage;
	}
	else if (txt_y > scr_yend)		/* Cursor to right of screen? */
	{	scr_ybeg = txt_y - COLS/2 - (txt_y-1) % (COLS/2);
		disp_mode = dpage;
	}
	scr_yend = scr_ybeg + COLS - 1;

	idlok(editwin, TRUE);	/* Default: allow hardware scrolling */

#ifndef TRACE
#ifdef xLINUX		/* New bug starting with ncurses 1.8.5 :-( */
	switch(disp_mode) {
	case dinsln:
	case dscrldn:
	case dtoeos:
		disp_mode = dpage;
		break;
	}
#endif
#endif

	switch(disp_mode) {
	case dtoeol:			/* Display to end of line */
	case dtoeolhc:			/* Same, but flags redisplay of "<" */
		dsp_lne(gap_end, txt_x - scr_xbeg+1);
		break;

	case dtoeos:			/* Delete line or redisplay from line down */
		wmove(editwin, txt_x - scr_xbeg + 1, 0);
		wdeleteln(editwin);
		disp_page(txt_x, scr_xend);
		break;

	case dinsln:  			/* Insert new line or redisplay from ln. above */
		wmove(editwin, txt_x - scr_xbeg, 0);
		winsertln(editwin);
		dsp_lne(gap_beg, txt_x - scr_xbeg);
		dsp_lne(gap_end, txt_x - scr_xbeg + 1);
		wrefresh(editwin);
		break;

	case dpage:					/* Display new page */
	case dpages:				/* Same, but flags 1/2 screen scroll */
		idlok(editwin, FALSE);	/* Curses bug: hardware codes bad here */
		disp_page(scr_xbeg, scr_xend);
		break;

	case dscrlup:				/* Scroll up */
		wmove(editwin, 0, 0);
		wdeleteln(editwin);
		disp_page(scr_xend, scr_xend);
		break;

	case dscrldn:				/* Scroll down */
		wmove(editwin, 0, 0);
		winsertln(editwin);
		disp_page(scr_xbeg, scr_xbeg);
		break;
	}
	idlok(editwin, FALSE);		/* Disable hardware scrolling for help, etc.*/

	/* Restore novice mode user help */
	if (helplines)
	{	if ((cmd_mode == CKey) && (helpstate != PRIMARY))
			dohelp(PRIMARY, helpwin);
		else if ((cmd_mode == FKey) &&
				 (helpstate != FKEYMENU) &&
				 (helpstate != RFKEYMENU))
			if (restrict)
				dohelp(RFKEYMENU, helpwin);
			else
				dohelp(FKEYMENU, helpwin);
	}
	/* Restore cursor */
	wmove(editwin, txt_x - scr_xbeg, txt_y - scr_ybeg);
	/* Needed for earlier curses implementations: */
	wrefresh(editwin);
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
/* Same, restricted mode version */
#define RCB0 "      SAVE/EXIT                 BLOCKS                    FILES               "
#define RCB1 " `^K-S`=save & resume       `^K-B`=begin  `^K-K`=end                                "
#define RCB2 " `^K-X`=save & exit         `^K-C`=copy   `^K-V`=move                               "
#define RCB3 " `^K-Q`=quit                `^K-Y`=erase  `^K-H`=hide                               "
#define RCB4 "                                                    `^K-I`=file information   "
#define RCB5 "                                                                              "
#define RCB6 "        [ `ESC` key may be substituted for `control-K` ]                          "

/* Onscreen help screen */
#define CO0 "              MARGINS & TABS                          SCREEN DISPLAY          "
#define CO1 "           `^O-X`=margin release                  `^O-D`=control code display     "
#define CO2 "           `^O-R`=set right margin                `^O-J`=novice mode toggle       "
#define CO3 "           `^O-T`=set hard tab spacing            `^O-L`=refresh screen           "
#define CO4 "                                                                              "
#define CO5 "                                                                              "
#define CO6 "                                                                              "

/* Function key mode primary screen */
#define FP0  "[`F1`]    [`F2`]    [`F3`]    [`F4`]    [`F5`]    [`F6`]    [`F7`]    [`F8`]    [`F9`]    [`F10`] "
#define FP0F12  "[`F1`]    [`F2`]    [`F3`]    [`F4`]    [`F12`]    [`F6`]    [`F7`]    [`F8`]    [`F9`]    [`F10`] "
/*            [F1]    [F2]    [F3]    [F4]    [F5]    [F6]    [F7]    [F8]    [F9]    [F10] */
#define FP1  "Cancel  Search  Help    Block   Setup  Replace  Exit  Retrieve  Reform  Save  "
/* Same, restricted mode version */
#define RFP0 "[`F1`]    [`F2`]    [`F3`]    [`F4`]    [`F5`]    [`F6`]    [`F7`]            [`F9`]    [`F10`] "
#define RFP0F12 "[`F1`]    [`F2`]    [`F3`]    [`F4`]    [`F12`]    [`F6`]    [`F7`]            [`F9`]    [`F10`] "
#define RFP1 "Cancel  Search  Help    Block   Setup  Replace  Exit            Reform  Save  "

/* Function key mode setup menu */
#define FS0 "  `1` - reveal `C`odes   `3` - `M`argin set     `5` - file `N`ame      `7` - program `I`nfo    "
#define FS1 "  `2` - `F`ile mode      `4` - margin `R`elease `6` - `T`ab display                        "
/* Same, restricted mode version */
#define RFS0 "  `1` - reveal `C`odes   `3` - `M`argin set                        `7` - program `I`nfo    "
#define RFS1 "                     `4` - margin `R`elease `6` - `T`ab display                        "

/* Function key mode block menu */
#define FB0 "  `1` - `B`egin block    `3` - `C`opy block     `5` - `W`rite block    `7` - `D`elete block    "
#define FB1 "  `2` - `E`nd block      `4` - `M`ove block     `6` - `H`ide block     `8` - `R`ead block      "
/* Same, restricted mode version */
#define RFB0 "  `1` - `B`egin block    `3` - `C`opy block                        `7` - `D`elete block    "
#define RFB1 "  `2` - `E`nd block      `4` - `M`ove block     `6` - `H`ide block                         "

/* Function key mode home key menu */
/*
#define FH0 "`up`,`down`=begin,end file;  `b`=-word; `t`=del.word;  `n`=insert line; `p`=code; `o`=refresh"
#define FH1 "`<--`,`-->`=begin,end line;  `f`=+word; `y`=del.line;  `l`=next search; `c`=case; `i`=to line"
*/

#define  FH0 "`up`,`dn`=begin,end text; `b`=-word; `t`=del.word; `n`=ins.line; `p`=code; `c`=case; `!`=shell"
#define RFH0 "`up`,`dn`=begin,end text; `b`=-word; `t`=del.word; `n`=insert line; `p`=code;    `c`=case;   "
#define  FH1 "`<-`,`->`=begin,end line; `f`=+word; `y`=del.line; `l`=next search; `o`=refresh; `i`=to line "

/* -----------------------------------------------------------------
 *	Add strings to window, highlighting sections enclosed in `reverse quotes`
 *  Uses FLAGATTR | A_BOLD to highlight.
 *  Side-effect: always sets attributes for designated window to A_NORMAL.
 */
/* VARARGS 1 */
void	xwprintw(xwindow, string1, string2, string3, string4)
WINDOW	*xwindow;
char	*string1, *string2, *string3, *string4;
{
	int		On = 0;
	char	strbuf[2000];
	char	*xstring = strbuf;

	sprintf(xstring, string1, string2, string3, string4);
	for ( ; *xstring ; xstring++ )
		if (*xstring == '`')
		{	if (On)
			{	wattrset(xwindow, A_NORMAL);
				On = 0;
			}
			else	
			/* Note: A_BOLD needed for b/w systems: */
			{	wattrset(xwindow, FLAGATTR | A_BOLD);
				On = 1;
			}
		}
		else
			waddch(xwindow, *xstring);

	wattrset(xwindow, A_NORMAL);
}
/* -----------------------------------------------------------------
 *	Display help in help window.
 */
void	dohelp(helprequest, hwindow)
enum helptype	helprequest;
WINDOW			*hwindow;
{
	chtype	ULchar = ACS_HLINE;	/* From curses.h */
	int		i;
	char	*hline0, *hline1, *hline2, *hline3, *hline4, *hline5, *hline6;

	hline0 = hline1 = hline2 = hline3 = hline4 = hline5 = hline6 = NULL;
	
	switch (helprequest) {
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
	case RBLOCK:
		hline0 = RCB0;
		hline1 = RCB1;
		hline2 = RCB2;
		hline3 = RCB3;
		hline4 = RCB4;
		hline5 = RCB5;
		hline6 = RCB6;
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
	case FKEYMENU:
		if (noF5)
			hline0 = FP0F12;
		else
			hline0 = FP0;
		hline1 = FP1;
		break;
	case RFKEYMENU:
		if (noF5)
			hline0 = RFP0F12;
		else
			hline0 = RFP0;
		hline1 = RFP1;
		break;
	case BLOCKMENU:
		hline0 = FB0;
		hline1 = FB1;
		break;
	case RBLOCKMENU:
		hline0 = RFB0;
		hline1 = RFB1;
		break;
	case SETUPMENU:
		hline0 = FS0;
		hline1 = FS1;
		break;
	case RSETUPMENU:
		hline0 = RFS0;
		hline1 = RFS1;
		break;
	case HOME:
		if (restrict)
			hline0 = RFH0;
		else
			hline0 = FH0;
		hline1 = FH1;
		break;
	default:
		warning("Internal error!");
		return;
		/*NOTREACHED*/
		break;
	}
#ifdef LINUX
	if (hwindow == helpwin)
/*		wclear(hwindow);*/
		touchwin(hwindow);

#endif
	wmove(hwindow, 0, 0);
	wclrtoeol(hwindow);
	wmove(hwindow, 0, HWIDTH < COLS ? (COLS - HWIDTH) / 2 : 0 );
	xwprintw(hwindow, hline0);
	wmove(hwindow, 1, 0);
	wclrtoeol(hwindow);
	wmove(hwindow, 1, HWIDTH < COLS ? (COLS - HWIDTH) / 2 : 0 );
	xwprintw(hwindow, hline1);
	if (hline2 != NULL)
	{	wmove(hwindow, 2, 0);
		wclrtoeol(hwindow);
		wmove(hwindow, 2, HWIDTH < COLS ? (COLS - HWIDTH) / 2 : 0 );
		xwprintw(hwindow, hline2);
		wmove(hwindow, 3, 0);
		wclrtoeol(hwindow);
		wmove(hwindow, 3, HWIDTH < COLS ? (COLS - HWIDTH) / 2 : 0 );
		xwprintw(hwindow, hline3);
		wmove(hwindow, 4, 0);
		wclrtoeol(hwindow);
		wmove(hwindow, 4, HWIDTH < COLS ? (COLS - HWIDTH) / 2 : 0 );
		xwprintw(hwindow, hline4);
		wmove(hwindow, 5, 0);
		wclrtoeol(hwindow);
		wmove(hwindow, 5, HWIDTH < COLS ? (COLS - HWIDTH) / 2 : 0 );
		xwprintw(hwindow, hline5);
		wmove(hwindow, 6, 0);
		wclrtoeol(hwindow);
		wmove(hwindow, 6, HWIDTH < COLS ? (COLS - HWIDTH) / 2 : 0 );
		xwprintw(hwindow, hline6);
	}

	helpstate = helprequest;	/* Global flag */

	if (hwindow == helpwin)
	{	wmove(hwindow, helplines-1, 0);
		wattron(hwindow, STATATTR);
		if (STATATTR == A_UNDERLINE)
			ULchar = ' ';
		for (i = 0; i < COLS ; i++)
			waddch(hwindow, ULchar);	/* Underline screen width */
		wattroff(hwindow, STATATTR);
		wrefresh(hwindow);
	}
	else
/*Gives spurious "^M" on AIX:
		waddstr(hwindow, "\n\r");*/		/* Next line */

		waddstr(hwindow, "\n");		/* Next line */
}
/* ================= DEBUG ROUTINE ================== */

/* VARARGS 1 */
void	debugpr(str1, str2, str3)
char	*str1, *str2, *str3;
{
#ifdef DEBUG
	fprintf(debugterm, str1, str2, str3);
#endif
}
