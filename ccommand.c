/*
Project:	SUE, a Simple Unix Editor
Copyright:	SUE is copyright 1992 by Point to Point Communications.
File:		ccommand.c
Routines:	Control key mode user commands.
Author:		Carson Wilson
				Apt. 1D
				1359 W. Greenleaf, Chicago, IL 60626
				internet:	carson@mcs.com
Notes:		Edit with hard tab stops set to 4.
*/

static	char	rcsid[] = "@(#)$Id: ccommand.c,v 1.4 1996/12/17 01:01:30 carson Exp carson $";

#include "sue.h"

/* -------------------------------------------------
 * Main user input routine
 */
void	CKey_scan()
{
	int	old_x;
	int	ch;
	int warnflag2;

	disp_mode = dnothing;		/* Default to no redisplay */

	warnflag2 = warnflag;
	if (warnflag)				/* Avoid instant overwrite of warnings */
	{	ch = wgetch(promptwin);
		wrefresh(editwin);
		warnflag = 0;
	}
	else
		ch = wgetch(editwin);		/* Await keystroke */
	switch (ch) {

	/* --------- CURSOR MOVEMENT COMMANDS --------- */
	case CNTRL_E:					/* up */
	case KEY_UP:
		txt_x = move_x(txt_x - 1);
		break;

	case CNTRL_X:					/* down */
	case KEY_DOWN:
		txt_x = move_x(txt_x + 1);
		break;

	case CNTRL_S:					/* left char */
	case KEY_LEFT:
		txt_y = move_y(moveback, movechar);
		break;

	case CNTRL_D:					/* right char */
	case KEY_RIGHT:
		txt_y = move_y(movefwd, movechar);
		break;

	case CNTRL_A:					/* back word */
	case KEY_SLEFT:
		txt_y = move_y(moveback, moveword);
		break;

	case CNTRL_F:					/* forward word */
	case KEY_SRIGHT:
		txt_y = move_y(movefwd, moveword);
		break;

	/* --------- SCROLLING --------- */

	case CNTRL_Z: 			/* Forward line */
    case KEY_SUSPEND:       /* Workaround: ncurses maps ^z to this */
	case KEY_SF:
		old_x = txt_x;
		scr_xbeg++;
		disp_mode= dscrlup;
		if (txt_x < scr_xbeg)
		{	txt_x = move_x(scr_xbeg);
			if (old_x == txt_x)
			{	scr_xbeg--;
				disp_mode = dnothing;
			}
		}
		break;

	case CNTRL_W:			/* Back line */
	case KEY_SR:
		if (scr_xbeg > 1)
		{	scr_xbeg--;
			scr_xend--;
			if (txt_x > scr_xend)
				txt_x = move_x(scr_xend);
			disp_mode = dscrldn;
		}
		break;

	case CNTRL_C:			/* Forward page */
	case KEY_NPAGE:
		old_x = txt_x;		/* old cursor line */
		txt_x = move_x(txt_x + editlines - 1); /* calc new cursor ln */
		scr_xbeg += txt_x - old_x;	/* add lines to screen */
		disp_mode = dpage;
		break;

	case CNTRL_R:			/* Back page */
	case KEY_PPAGE:
		old_x = txt_x;
		txt_x = move_x(txt_x - (editlines - 1));
		old_x -= txt_x;
		scr_xbeg = (scr_xbeg <= old_x ? 1 : scr_xbeg - old_x);
		disp_mode = dpage;
		break;

	/* --------- TEXT REFORMAT --------- */

	case CNTRL_B:
		reform();
		break;

	/* --------- HELP --------- */
	case CNTRL_J:
	case KEY_HELP:
	case KEY_F(3):
		if (helplines)	/* Help already present */
		{	keyerr();
			break;
		}
		dohelp(PRIMARY, editwin);
		wattrset(editwin, PROMPTATTR);
		wp1("Strike a MENU KEY or RETURN: ");
		wattrset(editwin, A_NORMAL);
		wrefresh(editwin);
		ch = waitkey();
		if ((ch < 255) && (ch >= '@'))
			ch &= 0x1F;	/* mask alphabetics */
		switch (ch) {
		case CNTRL_Q:
		case KEY_HOME:
			quickhlp();
			break;

		case CNTRL_K:
		case ESC:
			blockhlp();
			break;

		case CNTRL_O:
			onscrhlp();
			break;
		}
		status_line();			/* Repaint status line */
		disp_mode = dpage;		/* Repaint text */
		break;

	/* --------- REPEAT FIND or REPLACE --------- */

	case CNTRL_L:
	case 0x1C:			/* "^\" in case ^L is arrow key */
	case KEY_SFIND:
		repfind();
		break;

	/* --------- EXTENSIONS --------- */

	case CNTRL_Q:				/* "Quick" commands */
	case KEY_HOME:
		if (ch == CNTRL_Q)
			warning("^Q-");
		else 
			warning("HOME-");
		q_key();
		break;

	case ESC:
	case CNTRL_K:				/* Block/Saving commands */
		if (ch == CNTRL_K)
			warning("^K-");
		else 
			warning("ESC-");
		k_key();
		break;

	case CNTRL_O:				/* Onscreen commands */
		warning("^O-");
		o_key();
		break;

	/* --------- TOGGLES --------- */

	case CNTRL_V:				/* INSert on/off */
	case KEY_IC:
	case KEY_EIC:
		insflag = !insflag;
		if (insflag)
			statprint(COLS-24, "INS");
		else
			statprint(COLS-24, "   ");
		break;

	case CASE_INVERT:		/* Change case of char. */
		caseinvert();
		break;

	/* --------- TEXT DELETION --------- */

	case KEY_BACKSPACE:
	case BS:				/* Destructive BS */
		delleft();
		break;

	case KEY_DC:
	case CNTRL_G:			/* Delete character under cursor */
	case DEL:				/* delete key */
		if (!undelsave(gap_end, 1, FORWARD))
			break;
		delright();
		break;

	case CNTRL_T:			/* Delete word right */
		delword();
		break;

	case CNTRL_Y:			/* Delete cursor line */
	case KEY_DL:
		delline();
		break;

	/* ------- UNDO COMMAND -------- */
	case CNTRL_U:
		undel();
		break;

	/* --------- TEXT ENTRY --------- */

	case KEY_ENTER:
	case '\r':			/* RETURN key */
		if (warnflag2)	/* CR removes warnings */
			break;
		ch = '\n';		/* Redefine to a newline character */
		if (gap_beg + 1 < gap_end)
		{	*++gap_beg = ch;
			txt_x++;
			txt_y = 1;
			disp_mode = dinsln;
			mod = TRUE;
		}
		else
			TPAErr();
		break;	

	case ' ':
		if (warnflag2)	/* Space removes warnings */
			break;
		/* FALLTHRU */
	default:
		addtext(ch);				/* All else is text */
		break;
	}
	warnflag2 = 0;
	return;
}

/* --------------------------------------
 * Quick (^Q-) commands
 */
void q_key()
{
	int	keyin;
	int	old_x;
	char	*temp_ptr;

	if (helplines)
	{	signal(SIGALRM, quickhlp);
		alarm(2);
	}
	keyin = waitkey();
	if (helplines)
		if (alarm(0) == 0)		/* Clock expired */
		{	wrefresh(helpwin);
			wrefresh(promptwin);
			keyin = waitkey();	/* ..so get input again */
		}
	if ((keyin < 255) && (keyin >= '@'))
		keyin &= 0x1F;		/* Mask alphabetics */

	switch(keyin) {
	case CNTRL_R:			/* To beginning of file */
	case KEY_PPAGE:
		txt_x = move_x(0);
		break;

	case CNTRL_C:			/* To end of file */
	case KEY_NPAGE:
		do
			old_x = txt_x;
		while ((txt_x = move_x(txt_x+ 999)) != old_x);
		break;

	case CNTRL_F:			/* Find string */
		findfunc();
		break;

	case CNTRL_A:			/* Replace string */
		replfunc();
		break;

	case CNTRL_S:			/* To start of line */
	case KEY_LEFT:
		txt_y = line_y(1);
		break;

	case CNTRL_D:			/* To end of line */
	case KEY_RIGHT:
		txt_y = line_y(9999);	/* end of line */
		break;

	case CNTRL_I:			/* Seek line # */
		gotoline();
		break;

	case CNTRL_Y:			/* Delete to end of line */
	case KEY_DC:
		temp_ptr = gap_end;
		while (*gap_end != '\n' && gap_end < txt_end)
			gap_end++;	/* leave \n there */
		if (!undelsave(temp_ptr, gap_end - temp_ptr, FORWARD))
		{	gap_end = temp_ptr;
			break;
		}
		disp_mode = dtoeol;
		mod=TRUE;
		break;

	case ' ':
		break;

	default:
		keyerr();
	}
}
/* ----------------------------------------------------------
 * Saving and Block (^K-) commands
 */
void k_key()
{
	int	keyin;

	if (helplines)
	{	signal(SIGALRM, blockhlp);
		alarm(2);
	}
	keyin = waitkey();
	if (helplines)
		if (alarm(0) == 0)		/* Clock expired */
		{	wrefresh(helpwin);
			wrefresh(promptwin);
			keyin = waitkey();	/* ..so get input again */
		}
	if ((keyin < 255) && (keyin >= '@'))
		keyin &= 0x1F;	/* Mask alphabetics */
	switch (keyin)
	{
	case CNTRL_B:		/* Begin block */
		blockfuncs(1);
		break;
	case CNTRL_K:		/* End block */
		blockfuncs(2);
		break;
	case CNTRL_H:		/* Clear block markers */
		blockfuncs(3);
		break;
	case CNTRL_V:	/* Move */
		blockfuncs(4);
		break;
	case CNTRL_C:	/* Copy */
		blockfuncs(5);
		break;
	case CNTRL_Y:	/* Delete block */
		blockfuncs(6);
		break;

	case CNTRL_L:			/* Load new file */
		if (restrict)
			keyerr();
		else
			newfile();
		break;

	case CNTRL_Q:			/* Quit */
		quitfunc();
		break;

	case CNTRL_R:			/* read in */
		if (restrict)
			keyerr();
		else
			readblk();
		break;

	case CNTRL_S:			/* Save */
		savefunc();
		break;

	case CNTRL_D:			/* Done */
		if (restrict)
			keyerr();
		else
			if (save_fname())
			{
				mod = FALSE;
				newfile();
			}
		break;

	case CNTRL_N:			/* Specify or change fileName */
		if (restrict)
			keyerr();
		else
			name();
		break;

	case CNTRL_W:			/* Write block */
		if (restrict)
			keyerr();
		else
		{	if (blkerr(1))
				break;
			else
				writeblk();
		}
		break;

	case CNTRL_X:			/* eXit */
		exitfunc();
		break;

	case CNTRL_I:			/* Status */
		infofunc();
		break;

	case CNTRL_F:			/* Change file mode */
		if (restrict)
			keyerr();
		else
			setfilemode();
		break;

	case '!':				/* Shell escape */
		if (restrict)
			keyerr();
		else
			doshell();
		break;

	case ' ':
		break;

	default:
		keyerr();
	}
}
/*----------------------------------------------------------
 * Onscreen (^O-) commands
 */
void o_key()
{
	int	keyin;

	if (helplines)
	{	signal(SIGALRM, onscrhlp);
		alarm(2);
	}
	keyin = waitkey();
	if (helplines)
		if (alarm(0) == 0)		/* Clock expired */
		{	wrefresh(helpwin);
			wrefresh(promptwin);
			keyin = waitkey();	/* ..so get input again */
		}
	if ((keyin < 255) && (keyin >= '@'))
		keyin &= 0x1F;				/* Mask alphabetics */
	switch (keyin) {
	case CNTRL_D:					/* Toggle cntl. code display */
		cntlflag = !cntlflag;
		txt_y = column(gap_end);
		disp_mode = dpage;			/* Repaint screen */
		break;

	case CNTRL_X:					/* MR toggle */
		marrel();
		break;

	case CNTRL_R:					/* Set right margin */
		rmset();
		break;

	case CNTRL_T:					/* set Tabs */
		tabset();
		break;

#ifdef LOOKBUSY
	case CNTRL_B:					/* Don't ask */
		lookbusy();
		break;
#endif

	case CNTRL_L:					/* Repaint */
		repaint();
		break;

	case CNTRL_J:					/* Toggle help display */
		togglehelp();
		break;

	case ' ':
		break;
	default:
		keyerr();
	}
}
