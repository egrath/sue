/*
Project:	SUE, a Simple Unix Editor
Copyright:	SUE is copyright 1992 by Point to Point Communications.
File:		fcommand.c
Routines:	Function key mode user commands.
Author:		Carson Wilson
				Apt. 1D
				1359 W. Greenleaf, Chicago, IL 60626
				internet:	carson@mcs.com
Notes:		Edit with hard tab stops set to 4.
*/

static	char	rcsid[] = "@(#)$Id: fcommand.c,v 1.3 1995/03/09 13:19:58 carson Exp $";

#include "sue.h"

/* -------------------------------------------------
 * Main user input routine
 */
void	FKey_scan()
{
	int	old_x;
	int		ch;
	int		warnflag2;

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
	/*case CNTRL_E:*/					/* up */
	case KEY_UP:
		txt_x = move_x(txt_x - 1);
		break;

	/*case CNTRL_X:*/					/* down */
	case KEY_DOWN:
		txt_x = move_x(txt_x + 1);
		break;

	/*case CNTRL_S:*/					/* left char */
	case KEY_LEFT:
		txt_y = move_y(moveback, movechar);
		break;

	/*case CNTRL_D:*/					/* right char */
	case KEY_RIGHT:
		txt_y = move_y(movefwd, movechar);
		break;

	/*case CNTRL_A:*/					/* back word */
	case KEY_SLEFT:
		txt_y = move_y(moveback, moveword);
		break;

	/*case CNTRL_F:*/					/* forward word */
	case KEY_SRIGHT:
		txt_y = move_y(movefwd, moveword);
		break;

	case KEY_END:				/* EOL */
		txt_y = line_y(9999);
		break;

	/* --------- SCROLLING --------- */

	/*case CNTRL_C:*/			/* Forward page */
	case KEY_NPAGE:
		old_x = txt_x;		/* old cursor line */
		txt_x = move_x(txt_x + editlines - 1); /* calc new cursor ln */
		scr_xbeg += txt_x - old_x;	/* add lines to screen */
		disp_mode = dpage;
		break;

	/*case CNTRL_R:*/			/* Back page */
	case KEY_PPAGE:
		old_x = txt_x;
		txt_x = move_x(txt_x - (editlines - 1));
		old_x -= txt_x;
		scr_xbeg = (scr_xbeg <= old_x ? 1 : scr_xbeg - old_x);
		disp_mode = dpage;
		break;


	/* ----------- FUNCTION KEYS ------------ */

	case KEY_F(1):				/* Undel */
		undel();
		break;

	case KEY_F(2):				/* Find */
		findfunc();
		break;

	case KEY_HELP:
	case KEY_F(3):				/* Function key mode help screen */
		wmove(editwin, 0, 0);
		wp1("[`F1`] Cancel command, or undelete     [`F6`] Search for and replace text\n");
		wp1("[`F2`] Search for string of text       [`F7`] Exit SUE, optionally saving text\n");
		if (restrict)
		{	wp1("[`F3`] This help");
		}
		else
		{	wp1("[`F3`] This help                       [`F8`] Retrieve a new file from disk\n");
		}
		wp1("[`F4`] Block function menu             [`F9`] Reformat text to end of paragraph\n");
		if (noF5)
		{	wp1("[`F12`] Setup menu (change defaults)  [`F10`] Save text to file and continue\n");
		}
		else
		{	wp1("[`F5`] Setup menu (change defaults)   [`F10`] Save text to file and continue\n");
		}
		wp1("`PgUp/PgDn`, `arrow keys` - scroll text  `Insert` - toggle insert/overwrite\n");     
		wp1("`Home-Up/Down` - to top, end of file   `Home-Left/Right` - left, right end of line\n");
		wp1("`Home-B` - word back                   `Home-F` - word forward\n");
		wp1("`Home-T` - erase word                  `Home-Y` - erase line\n");
		wp1("`Home-N` - insert line after cursor    `Home-L` - repeat last find or replace\n");
		wp1("`Home-C` - toggle case of character    `Home-I` - go to line number\n");
		wp1("`Home-O` - refresh screen              `Home-P` - enter control character\n");
		if (restrict != 0)
			wp1("`Home-!` - shell escape\n");
		wp1("`Backspace` - delete backwards         `Delete` - delete forwards\n");
		wattrset(editwin, PROMPTATTR);
		wp1("Strike any key, or F3 to toggle help: ");
		wattrset(editwin, A_NORMAL);
		wrefresh(editwin);
		disp_mode = dpage;		/* Repaint text */
		ch = waitkey();			/* Get key */
		if (ch == KEY_F(3))
			togglehelp();
		break;

	case KEY_F(4):				/* Block */
		block();
		break;

	case KEY_F(5):
		setup();
		break;

	case KEY_F(6):				/* Replace */
		replfunc();
		break;

	case KEY_F(7):				/* Exit */
		quitfunc();
		break;

	case KEY_F(8):				/* Retrieve */
		if (restrict)
			keyerr();
		else
			newfile();
		break;

	case KEY_F(9):				/* Info */
		reform();
		break;

	case KEY_F(10):				/* Save */
		savefunc();
		break;

	case KEY_F(12):
		if (noF5)
			setup();
		else
			keyerr();
		break;

	case KEY_F(11):			/* These could be "short cuts" to menu commands */
/*
	case KEY_F(13):
	case KEY_F(14):
	case KEY_F(15):
	case KEY_F(16):
	case KEY_F(17):
	case KEY_F(18):
	case KEY_F(19):
	case KEY_F(20):
	case KEY_F(21):
	case KEY_F(22):
*/

		keyerr();			/* undefined */
		break;

	/* --------- EXTENSIONS --------- */

	case KEY_HOME:
		home_key();
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

	case KEY_F(13):
	case KEY_BTAB:			/* Margin release */
		MRflag = !MRflag;
		if (MRflag)
			statprint(COLS-27, "MR");
		else
			statprint(COLS-27, "  ");
		break;

	/* --------- TEXT DELETION --------- */

	case KEY_BACKSPACE:
	case BS:				/* Destructive BS */
		delleft();
		break;

	case KEY_DC:
	/*case CNTRL_G:*/			/* Delete character under cursor */
	case DEL:				/* delete key */
		if (!undelsave(gap_end, 1, FORWARD))
			break;
		delright();
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
		addtext(ch);
		break;
	}
	warnflag2 = 0;
	return;
}
/* -------------------------------------------------
 * Home key help
 */
void	hkeyhlp()
{
	dohelp(HOME, helpwin);
}
/* -------------------------------------------------
 * Home key routines
 */
void	home_key()
{
	int		keyin;
	int		old_x;

	warning("HOME-");

	if (helplines)
	{	signal(SIGALRM, (void (*)()) hkeyhlp);
		alarm(2);
	}
	keyin = waitkey();
	if (helplines)
		if (alarm(0) == 0)		/* Clock expired */
		{	wrefresh(helpwin);
			wrefresh(promptwin);/* (restore cursor) */
			keyin = waitkey();	/* ..so get input again */
		}

	rehome:
	if ((keyin < 255) && (keyin >= '@'))
		keyin &= 0x1F;			/* Mask alphabetics */

	switch (keyin) {
	case KEY_HOME :
		keyin = waitkey();
		goto rehome;
		/*NOTREACHED*/
		break;

	case KEY_UP :			/* TOF */
		txt_x = move_x(0);
		break;

	case KEY_DOWN :			/* EOF */
		do
			old_x = txt_x;
		while ((txt_x = move_x(txt_x + 999)) != old_x);
		break;

	case KEY_LEFT :				/* SOL */
		txt_y = line_y(1);
		break;

	case KEY_RIGHT :				/* EOL */
		txt_y = line_y(9999);
		break;

	case CNTRL_I :				/* Seek line by # */
		gotoline();
		break;

	case CNTRL_T :
		delword();
		break;

	case CNTRL_Y :
		delline();
		break;

	case CNTRL_C :				/* Change case of char. */
		caseinvert();
		break;

	case CNTRL_N:				/* Insert a new line */
		insertline();
		break;

	case CNTRL_L:
	case 0x1C:			/* "^\" in case ^L is arrow key */
		repfind();
		break;

	case CNTRL_O:
		repaint();
		break;

	case CNTRL_P:
		addtext(keyin);
		break;

	case CNTRL_B:
		txt_y = move_y(moveback, moveword);
		break;

	case CNTRL_F:
		txt_y = move_y(movefwd, moveword);
		break;

	case '!':			/* Shell escape */
		if (restrict)
			keyerr();
		else
			doshell();
		break;

	case KEY_F(1):
	case ' ' :
	case KEY_ENTER:
	case '\r':			/* RETURN key */
		break;

	default :
		keyerr();
	}
}
/* -------------------------------------------------
 * Setup menu
 */
void	setup()
{
	int		result;

	if (helplines)
	{	if (restrict)
			dohelp(RSETUPMENU, helpwin);
		else
			dohelp(SETUPMENU, helpwin);
	}
	else
	{	wmove(editwin, 0, 0);
		if (restrict)
		{	wp1("`C`odes, `M`argin, `R`elease, `T`ab, `I`nfo");
		}
		else
		{	wp1("`C`odes, `F`ilemode, `M`argin, `R`elease, `N`ame, `T`ab, `I`nfo");	
		}
	}
	result = promptkey("Setup command: ");
	if (result < 255)
		result = toupper(result);
	switch (result) {
	case '1':
	case 'C':						/* Reveal codes */
		cntlflag = !cntlflag;
		txt_y = column(gap_end);
		disp_mode = dpage;			/* Repaint screen */
		break;

	case '2':
	case 'F':						/* File mode */
		if (restrict)
			keyerr();
		else
			setfilemode();
		break;

	case '3':
	case 'M':						/* Margin set */
		rmset();
		break;

	case '4':
	case 'R':						/* margin Release */
		marrel();
		break;

	case '5':
	case 'N':						/* file Name */
		if (restrict)
			keyerr();
		else
			name();
		break;

	case '6':
	case 'T':						/* Tab display */
		tabset();
		break;

	case '7':
	case 'I':						/* program Info */
		infofunc();
		break;

	case KEY_F(3):
	case KEY_F(5):
	case KEY_F(1):
	case KEY_ENTER:
	case '\r':			/* RETURN key */
	case ' ':
		break;

	default:
		keyerr();
	}
	disp_mode = dpage;		/* Repaint text */
}
/* -------------------------------------------------
 * Block menu
 */
void	block()
{
	int	result;

	if (helplines)
	{	if (restrict)
			dohelp(RBLOCKMENU, helpwin);
		else
			dohelp(BLOCKMENU, helpwin);
	}
	else
	{	wmove(editwin, 0, 0);
		if (restrict)
		{	wp1("`B`egin, `E`nd, `C`opy, `M`ove, `H`ide, `D`elete");	
		}
		else
		{	wp1("`B`egin, `E`nd, `C`opy, `M`ove, `W`rite, `H`ide, `D`elete, `R`ead");	
		}
	}
	result = promptkey("Block command: ");
	if (result < 255)
		result = toupper(result);
	switch (result) {
	case '1':
	case 'B':						/* Begin block */
		blockfuncs(1);
		break;

	case '2':
	case 'E':						/* End block */
		blockfuncs(2);
		break;

	case '3':
	case 'C':						/* Copy block */
		blockfuncs(5);
		break;

	case '4':
	case 'M':						/* Move block */
		blockfuncs(4);
		break;

	case '5':
	case 'W':						/* Write block */
		if (restrict)
			keyerr();
		else
		{	if (blkerr(1))			/* No block */
				break;
			else
				writeblk();
		}
		break;

	case '6':
	case 'H':						/* Hide block */
		blockfuncs(3);
		break;

	case '7':
	case 'D':						/* Delete block */
		blockfuncs(6);
		break;

	case '8':
	case 'R':						/* Read block */
		if (restrict)
			keyerr();
		else
			readblk();
		break;

	case KEY_F(3):					/* Block help */
		break;

	case KEY_F(4):
	case KEY_F(1):
	case KEY_ENTER:
	case '\r':			/* RETURN key */
	case ' ':
		break;

	default:
		keyerr();
	}
	disp_mode = dpage;		/* Repaint text */
}
