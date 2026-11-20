/* #######################################################################
 * command.c - TextStar key dispatch.
 * purpose:
 *     Everything between the keyboard and the commands: key_scan() is
 *     the main input routine (one call = one keystroke handled), with
 *     the WordStar two-key menus ^Q (quick/curor commands), ^K (block
 *     and file commands) and ^O (onscreen toggles) plus the alarm()
 *     trampolines that time out the help screens on UNIX.
 * notes:
 *     key_scan() sets disp_mode = dnothing by default; every command
 *     case is responsible for requesting the repaint it needs. Typing
 *     falls through to add_text() in the default case.
 * ####################################################################### */

#include "ts.h"

/* #######################################################################
 * void key_scan (void)
 * purpose:
 *     The main user input routine, called in a loop from main(): read
 *     one keystroke and dispatch it - cursor movement, scrolling,
 *     reformatting, help, the two-key menus, toggles, deletions, undo,
 *     and (default case) plain text entry. Also routes the newline
 *     into the buffer and handles KEY_RESIZE repaints.
 * returns:
 *     Nothing; the dispatched handler performs the action and leaves
 *     the wanted repaint in disp_mode.
 * parameters:
 *     None.
 * notes:
 *     While warnflag is set (a warning on screen) the first keystroke
 *     is consumed just to repaint the text - RETURN/SPACE clear the
 *     warning instead of acting. Alphabetics in the menus are masked
 *     to their control codes so upper/lowercase both work.
 * ####################################################################### */
void key_scan (void)
{
	int	old_x;
	int	ch;
	int warnflag2;

	disp_mode = dnothing;		/* Default to no redisplay */

	warnflag2 = warnflag;
	if (warnflag)				/* Avoid instant overwrite of warnings */
	{
		ch = wgetch (promptwin);
		wrefresh (editwin);
		warnflag = 0;
	}
	else
	{
		ch = wgetch (editwin);		/* Await keystroke */
	}
	switch (ch)
	{

	/* --------- CURSOR MOVEMENT COMMANDS --------- */
	case CNTRL_E:					/* up */
	case KEY_UP:
		txt_x = move_x (txt_x - 1);
		break;

	case CNTRL_X:					/* down */
	case KEY_DOWN:
		txt_x = move_x (txt_x + 1);
		break;

	case CNTRL_S:					/* left char */
	case KEY_LEFT:
		txt_y = move_y (moveback, movechar);
		break;

	case CNTRL_D:					/* right char */
	case KEY_RIGHT:
		txt_y = move_y (movefwd, movechar);
		break;

	case CNTRL_A:					/* back word */
	case KEY_SLEFT:
		txt_y = move_y (moveback, moveword);
		break;

	case CNTRL_F:					/* forward word */
	case KEY_SRIGHT:
		txt_y = move_y (movefwd, moveword);
		break;

	/* --------- SCROLLING --------- */

	case CNTRL_Z: 			/* Forward line */
	case KEY_SUSPEND:       /* Workaround: ncurses maps ^z to this */
	case KEY_SF:
		old_x = txt_x;
		scr_xbeg++;
		disp_mode = dscrlup;
		if (txt_x < scr_xbeg)
		{
			txt_x = move_x (scr_xbeg);
			if (old_x == txt_x)
			{
				scr_xbeg--;
				disp_mode = dnothing;
			}
		}
		break;

	case CNTRL_W:			/* Back line */
	case KEY_SR:
		if (scr_xbeg > 1)
		{
			scr_xbeg--;
			scr_xend--;
			if (txt_x > scr_xend)
			{
				txt_x = move_x (scr_xend);
			}
			disp_mode = dscrldn;
		}
		break;

	case CNTRL_C:			/* Forward page */
	case KEY_NPAGE:
		old_x = txt_x;		/* old cursor line */
		txt_x = move_x (txt_x + editlines - 1); /* calc new cursor ln */
		scr_xbeg += txt_x - old_x;	/* add lines to screen */
		disp_mode = dpage;
		break;

	case CNTRL_R:			/* Back page */
	case KEY_PPAGE:
		old_x = txt_x;
		txt_x = move_x (txt_x - (editlines - 1));
		old_x -= txt_x;
		scr_xbeg = (scr_xbeg <= old_x ? 1 : scr_xbeg - old_x);
		disp_mode = dpage;
		break;

	/* --------- TEXT REFORMAT --------- */

	case CNTRL_B:
		reform ();
		break;

	/* --------- HELP --------- */
	case CNTRL_J:
	case KEY_HELP:
		if (helplines)	/* Help already present */
		{
			key_err ();
			break;
		}
		do_help (PRIMARY, editwin);
		wattrset (editwin, PROMPTATTR);
		wp1 ("Strike a MENU KEY or RETURN: ");
		wattrset (editwin, A_NORMAL);
		wrefresh (editwin);
		ch = wait_key ();
		if ((ch < 255) && (ch >= '@'))
		{
			ch &= 0x1F;	/* mask alphabetics */
		}
		switch (ch)
		{
		case CNTRL_Q:
		case KEY_HOME:
			quick_hlp ();
			break;

		case CNTRL_K:
		case ESC:
			block_hlp ();
			break;

		case CNTRL_O:
			onscr_hlp ();
			break;
		}
		status_line ();			/* Repaint status line */
		disp_mode = dpage;		/* Repaint text */
		break;

	/* --------- REPEAT FIND or REPLACE --------- */

	case CNTRL_L:
	case 0x1C:			/* "^\" in case ^L is arrow key */
	case KEY_SFIND:
		rep_find ();
		break;

	/* --------- EXTENSIONS --------- */

	case CNTRL_Q:				/* "Quick" commands */
	case KEY_HOME:
		if (ch == CNTRL_Q)
		{
			warning ("^Q-");
		}
		else
		{
			warning ("HOME-");
		}
		q_key ();
		break;

	case ESC:
	case CNTRL_K:				/* Block/Saving commands */
		if (ch == CNTRL_K)
		{
			warning ("^K-");
		}
		else
		{
			warning ("ESC-");
		}
		k_key ();
		break;

	case CNTRL_O:				/* Onscreen commands */
		warning ("^O-");
		o_key ();
		break;

	/* --------- TOGGLES --------- */

	case CNTRL_V:				/* INSert on/off */
	case KEY_IC:
	case KEY_EIC:
		insflag = !insflag;
		if (insflag)
		{
			stat_print (COLS - 24, "INS", NULL, NULL);
		}
		else
		{
			stat_print (COLS - 24, "   ", NULL, NULL);
		}
		break;

	case CASE_INVERT:		/* Change case of char. */
		case_invert ();
		break;

	/* --------- TEXT DELETION --------- */

	case KEY_BACKSPACE:
	case KEY_DC:
	case BS:				/* Destructive BS */
	case DEL:				/* delete key */
		del_left ();
		break;

	case CNTRL_G:			/* Delete character under cursor */
		if (!undel_save (gap_end, 1, FORWARD))
		{
			break;
		}
		del_right ();
		break;

	case CNTRL_T:			/* Delete word right */
		del_word ();
		break;

	case CNTRL_Y:			/* Delete cursor line */
	case KEY_DL:
		del_line ();
		break;

	/* ------- UNDO COMMAND -------- */
	case CNTRL_U:
		undel ();
		break;

	/* --------- TEXT ENTRY --------- */

	case KEY_ENTER:
	case '\r':			/* RETURN key */
		if (warnflag2)	/* CR removes warnings */
		{
			break;
		}
		ch = '\n';		/* Redefine to a newline character */
		if (gap_beg + 1 < gap_end)
		{
			*++gap_beg = ch;
			txt_x++;
			txt_y = 1;
			disp_mode = dinsln;
			mod = TRUE;
		}
		else
		{
			tpa_err ();
		}
		break;

	case ' ':
		if (warnflag2)	/* Space removes warnings */
		{
			break;
		}
		/* FALLTHRU */
	default:
#ifdef KEY_RESIZE
		if (ch == KEY_RESIZE)	/* Terminal was resized */
		{
			repaint ();
			break;
		}
#endif
		if (ch > 255)	/* Unhandled extended key code */
		{
			key_err ();
		}
		else
		{
			add_text (ch);				/* All else is text */
		}
		break;
	}
	warnflag2 = 0;
	return;
}

/* --------------------------------------
 * alarm() handlers for the timed help screens.
 * signal() requires a void (*)(int) function; the help
 * routines themselves take no arguments.
 * (Only used on UNIX - DOS shows the help screens at once.)
 */
#ifdef UNIX
/* #######################################################################
 * static void alarm_quick_hlp (int sig)
 * purpose:
 *     alarm() trampoline: refresh the ^Q help screen when the 2-second
 *     menu timeout expires.
 * returns:
 *     Nothing.
 * parameters:
 *     int sig: the signal number (unused, SIGALRM).
 * notes:
 *     signal() requires a void (*)(int) handler while quick_hlp()
 *     takes no arguments - UNIX only, DOS shows the help at once.
 * ####################################################################### */
static void alarm_quick_hlp (int sig)
{
	(void) sig;		/* Unused */
	quick_hlp ();
}
/* #######################################################################
 * static void alarm_block_hlp (int sig)
 * purpose:
 *     alarm() trampoline: refresh the ^K help screen on menu timeout.
 * returns:
 *     Nothing.
 * parameters:
 *     int sig: the signal number (unused, SIGALRM).
 * notes:
 *     UNIX only; see alarm_quick_hlp().
 * ####################################################################### */
static void alarm_block_hlp (int sig)
{
	(void) sig;		/* Unused */
	block_hlp ();
}
/* #######################################################################
 * static void alarm_onscr_hlp (int sig)
 * purpose:
 *     alarm() trampoline: refresh the ^O help screen on menu timeout.
 * returns:
 *     Nothing.
 * parameters:
 *     int sig: the signal number (unused, SIGALRM).
 * notes:
 *     UNIX only; see alarm_quick_hlp().
 * ####################################################################### */
static void alarm_onscr_hlp (int sig)
{
	(void) sig;		/* Unused */
	onscr_hlp ();
}
#endif

/* #######################################################################
 * void q_key (void)
 * purpose:
 *     The ^Q "quick" command menu: wait for the second key and
 *     dispatch - file top/bottom, find/replace, line start/end, goto
 *     line, delete to end of line. Two-key command: main() prints the
 *     "^Q-" prompt before calling this.
 * returns:
 *     Nothing; the executed command updates the cursor/screen state.
 * parameters:
 *     None.
 * notes:
 *     When help windows are up, alarm(2) times the menu out (UNIX)
 *     or the help is shown at once (DOS). Alphabetics are masked to
 *     control codes so case does not matter; unknown keys beep via
 *     key_err().
 * ####################################################################### */
void q_key (void)
{
	int	keyin;
	int	old_x;
	char	*temp_ptr;

#ifdef UNIX
	if (helplines)
	{
		signal (SIGALRM, alarm_quick_hlp);
		alarm (2);
	}
#endif
	keyin = wait_key ();
	if (helplines)
	{
#ifdef UNIX
		if (alarm (0) == 0)		/* Clock expired */
		{
			wrefresh (helpwin);
			wrefresh (promptwin);
			keyin = wait_key ();	/* ..so get input again */
		}
#else
		do_help (QUICK, helpwin);	/* No alarm() on DOS: show at once */
		wrefresh (helpwin);
#endif
	}
	if ((keyin < 255) && (keyin >= '@'))
	{
		keyin &= 0x1F;		/* Mask alphabetics */
	}

	switch (keyin)
	{
	case CNTRL_R:			/* To beginning of file */
	case KEY_PPAGE:
		txt_x = move_x (0);
		break;

	case CNTRL_C:			/* To end of file */
	case KEY_NPAGE:
		do
		{
			old_x = txt_x;
		}
		while ((txt_x = move_x (txt_x + 999)) != old_x);
		break;

	case CNTRL_F:			/* Find string */
		find_func ();
		break;

	case CNTRL_A:			/* Replace string */
		repl_func ();
		break;

	case CNTRL_S:			/* To start of line */
	case KEY_LEFT:
		txt_y = line_y (1);
		break;

	case CNTRL_D:			/* To end of line */
	case KEY_RIGHT:
		txt_y = line_y (9999);	/* end of line */
		break;

	case CNTRL_I:			/* Seek line # */
		goto_line ();
		break;

	case CNTRL_Y:			/* Delete to end of line */
	case KEY_DC:
		temp_ptr = gap_end;
		while (*gap_end != '\n' && gap_end < txt_end)
		{
			gap_end++;	/* leave \n there */
		}
		if (!undel_save (temp_ptr, gap_end - temp_ptr, FORWARD))
		{
			gap_end = temp_ptr;
			break;
		}
		disp_mode = dtoeol;
		mod = TRUE;
		break;

	case ' ':
		break;

	default:
		key_err ();
	}
}
/* #######################################################################
 * void k_key (void)
 * purpose:
 *     The ^K block and file command menu: block begin/end/hide/move/
 *     copy/delete, load file, save, save-as (name change), quit, exit,
 *     read/write block, file mode, info screen and the shell escape.
 * Two-key command: main() prints the "^K-" prompt before calling
 *     this.
 * returns:
 *     Nothing.
 * parameters:
 *     None.
 * notes:
 *     Same timeout/masking scheme as q_key(). ^K W refuses to run when
 *     no block is marked (blk_err(1) beeps).
 * ####################################################################### */
void k_key (void)
{
	int	keyin;

#ifdef UNIX
	if (helplines)
	{
		signal (SIGALRM, alarm_block_hlp);
		alarm (2);
	}
#endif
	keyin = wait_key ();
	if (helplines)
	{
#ifdef UNIX
		if (alarm (0) == 0)		/* Clock expired */
		{
			wrefresh (helpwin);
			wrefresh (promptwin);
			keyin = wait_key ();	/* ..so get input again */
		}
#else
		do_help (BLOCK, helpwin);	/* No alarm() on DOS: show at once */
		wrefresh (helpwin);
#endif
	}
	if ((keyin < 255) && (keyin >= '@'))
	{
		keyin &= 0x1F;	/* Mask alphabetics */
	}
	switch (keyin)
	{
	case CNTRL_B:		/* Begin block */
		block_funcs (1);
		break;
	case CNTRL_K:		/* End block */
		block_funcs (2);
		break;
	case CNTRL_H:		/* Clear block markers */
		block_funcs (3);
		break;
	case CNTRL_V:	/* Move */
		block_funcs (4);
		break;
	case CNTRL_C:	/* Copy */
		block_funcs (5);
		break;
	case CNTRL_Y:	/* Delete block */
		block_funcs (6);
		break;

	case CNTRL_L:			/* Load new file */
		new_file ();
		break;

	case CNTRL_Q:			/* Quit */
		quit_func ();
		break;

	case CNTRL_R:			/* read in */
		readblk ();
		break;

	case CNTRL_S:			/* Save */
		save_func ();
		break;

	case CNTRL_D:			/* Done */
		if (save_fname ())
		{
			mod = FALSE;
			new_file ();
		}
		break;

	case CNTRL_N:			/* Specify or change fileName */
		name ();
		break;

	case CNTRL_W:			/* Write block */
		if (blk_err (1))
		{
			break;
		}
		else
		{
			writeblk ();
		}
		break;

	case CNTRL_X:			/* eXit */
		exit_func ();
		break;

	case CNTRL_I:			/* Status */
		info_func ();
		break;

	case CNTRL_F:			/* Change file mode */
		set_file_mode ();
		break;

	case '!':				/* Shell escape */
		do_shell ();
		break;

	case ' ':
		break;

	default:
		key_err ();
	}
}
/* #######################################################################
 * void o_key (void)
 * purpose:
 *     The ^O "onscreen" command menu: toggle control-character display
 *     (^O D), margin release (^O X), right margin (^O R), tab width
 *     (^O T) and full repaint (^O L).
 * returns:
 *     Nothing.
 * parameters:
 *     None.
 * notes:
 *     Same timeout/masking scheme as q_key().
 * ####################################################################### */
void o_key (void)
{
	int	keyin;

#ifdef UNIX
	if (helplines)
	{
		signal (SIGALRM, alarm_onscr_hlp);
		alarm (2);
	}
#endif
	keyin = wait_key ();
	if (helplines)
	{
#ifdef UNIX
		if (alarm (0) == 0)		/* Clock expired */
		{
			wrefresh (helpwin);
			wrefresh (promptwin);
			keyin = wait_key ();	/* ..so get input again */
		}
#else
		do_help (ONSCREEN, helpwin);	/* No alarm() on DOS: show at once */
		wrefresh (helpwin);
#endif
	}
	if ((keyin < 255) && (keyin >= '@'))
	{
		keyin &= 0x1F;				/* Mask alphabetics */
	}
	switch (keyin)
	{
	case CNTRL_D:					/* Toggle cntl. code display */
		cntlflag = !cntlflag;
		txt_y = column (gap_end);
		disp_mode = dpage;			/* Repaint screen */
		break;

	case CNTRL_X:					/* MR toggle */
		mar_rel ();
		break;

	case CNTRL_R:					/* Set right margin */
		rm_set ();
		break;

	case CNTRL_T:					/* set Tabs */
		tab_set ();
		break;

	case CNTRL_L:					/* Repaint */
		repaint ();
		break;

	case ' ':
		break;
	default:
		key_err ();
	}
}
