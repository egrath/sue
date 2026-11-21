/* #######################################################################
 * funcs.c - TextStar editor commands.
 * purpose:
 *     Everything a keystroke can DO: the find/replace machinery
 *     (find_func/repl_func/find/replace/rep_find), reformatting
 *     (reform), file I/O with line-ending translation (save/load,
 *     writeblk/readblk, save_fname/name), block operations
 *     (block_funcs), deletions and their undo saves (del_right/
 *     del_left/del_line/del_word/del_line/text_ins), text entry with
 *     word wrap (add_text), the crash-save signal handler (ts_abort),
 *     the info screen and the small help helpers.
 * notes:
 *     Commands are invoked from command.c's dispatch. They leave the
 *     wanted repaint in disp_mode (see display.c) and set mod = TRUE
 *     when the buffer changed. All file I/O is binary; line endings
 *     are translated according to filemode.
 * ####################################################################### */

#include "ts.h"
#include "version.h"

/* #######################################################################
 * void block_hlp (void)
 * purpose:
 *     Show the ^K block/file command help: in the persistent help
 *     window when help mode is on, otherwise as a timed overlay in
 *     the edit window ("strike any key").
 * returns:
 *     Nothing.
 * parameters:
 *     None.
 * ####################################################################### */
void block_hlp (void)
{
	enum	helptype	htype;

	htype = BLOCK;

	if (helplines)
	{
		do_help (htype, helpwin);		/* Show it in separate window */
	}
	else
	{
		do_help (htype, editwin);		/* Show in edit window */
		wattrset (editwin, PROMPTATTR);
		wp1 ("Strike any key: ");
		wattrset (editwin, A_NORMAL);
		wrefresh (editwin);
		wait_key ();
	}
}
/* #######################################################################
 * void quick_hlp (void)
 * purpose:
 *     Show the ^Q quick-command help (persistent window or timed
 *     overlay, like block_hlp).
 * returns:
 *     Nothing.
 * parameters:
 *     None.
 * ####################################################################### */
void quick_hlp (void)
{
	if (helplines)
	{
		do_help (QUICK, helpwin);
	}
	else
	{
		do_help (QUICK, editwin);		/* Show in edit window */
		wattrset (editwin, PROMPTATTR);
		wp1 ("Strike any key: ");
		wattrset (editwin, A_NORMAL);
		wrefresh (editwin);
		wait_key ();
	}
}
/* #######################################################################
 * void onscr_hlp (void)
 * purpose:
 *     Show the ^O onscreen-command help (persistent window or timed
 *     overlay, like block_hlp).
 * returns:
 *     Nothing.
 * parameters:
 *     None.
 * ####################################################################### */
void onscr_hlp (void)
{
	if (helplines)
	{
		do_help (ONSCREEN, helpwin);
	}
	else
	{
		do_help (ONSCREEN, editwin);		/* Show in edit window */
		wattrset (editwin, PROMPTATTR);
		wp1 ("Strike any key: ");
		wattrset (editwin, A_NORMAL);
		wrefresh (editwin);
		wait_key ();
	}
}
/* #######################################################################
 * int find_func (void)
 * purpose:
 *     The ^Q F "find text" command: prompt for a search string (the
 *     previous one is kept so ^L can repeat it), then run find()
 *     forward from the cursor.
 * returns:
 *     1 when the string was found, 0 on empty input or not found
 *     (with "Text not found" feedback).
 * parameters:
 *     None; works on the global search string s_str/sptr.
 * notes:
 *     Case-independent; repflag = FALSE keeps ^L in "find" mode.
 * ####################################################################### */
int find_func (void)
{
	char	tfindbuf[60];

	strcpy (tfindbuf, sptr);
	if (prompt_st ("Find text: ", "", sptr, s_strsiz) > 0)
	{
		repflag = FALSE;
		txt_ptr = gap_end;
		if (find () == 0)
		{
			not_found ();
			return (0);
		}
		return (1);			/* Found */
	}
	else
	{
		/* Restore previous string, if any */
		strcpy (sptr, tfindbuf);
	}

	return (0);				/* Null response */
}
/* #######################################################################
 * void repl_func (void)
 * purpose:
 *     The ^Q A "change text" command: ask what to find (find_func),
 *     then what to change it into, and loop find()/replace() forward
 *     from the cursor. The replace() Y/N/"*" prompt drives each step.
 * returns:
 *     Nothing.
 * parameters:
 *     None; works on the global search (sptr) and replace (rptr)
 *     strings.
 * notes:
 *     Ends when the string is no longer found, the user aborts, or
 *     the gap runs out of space.
 * ####################################################################### */
void repl_func (void)
{
	if (find_func ())
	{
		repflag = TRUE;
		repkey = 0;
		if (prompt_st ("Change to: ", "", rptr, r_strsiz) < 0)
		{
			return;
		}
		while (find ())
		{
			if (replace () == 0)
			{
				return;
			}
		}
	}
}
/* #######################################################################
 * void quit_func (void)
 * purpose:
 *     The ^K Q quit command: when the file is modified, ask "abandon?"
 *     first; then exit via edit_exit(0) without saving.
 * returns:
 *     Nothing (returns to caller when the user declines).
 * parameters:
 *     None.
 * ####################################################################### */
void quit_func (void)
{
	if (mod)					/* Ask first if file changed */
	{
		if (abandon () != 'Y')
		{
			return;
		}
	}
	edit_exit (0);
}
/* #######################################################################
 * void save_func (void)
 * purpose:
 *     The ^K S "save and resume" command: write the text (prompting
 *     for a name when there is none) and clear the modified flag.
 * returns:
 *     Nothing.
 * parameters:
 *     None.
 * notes:
 *     mod is only cleared when the save succeeded, so a failed save
 *     keeps protecting the text.
 * ####################################################################### */
void save_func (void)
{
	if (save_fname ())
	{
		mod = FALSE;
	}
}
/* #######################################################################
 * void exit_func (void)
 * purpose:
 *     The ^K X "save and exit" command: save (prompting for a name
 *     when needed) and leave the editor; a declined/failed save keeps
 *     the session open.
 * returns:
 *     Nothing (does not return on a successful save - edit_exit).
 * parameters:
 *     None.
 * ####################################################################### */
void exit_func (void)
{
	if (save_fname ())
	{
		edit_exit (0);
	}
}
/* #######################################################################
 * void info_func (void)
 * purpose:
 *     The ^K I info screen: version/copyright, edited file with cursor
 *     position, modified flag, file mode, margins, tab width, control-
 *     character display, memory usage (text bytes vs free gap), and
 *     terminal type. Waits for any key and repaints.
 * returns:
 *     Nothing.
 * parameters:
 *     None.
 * ####################################################################### */
void info_func (void)
{
	char	modestr[20];

	switch (filemode)
	{
	case MODE_UNIX:
		strcpy (modestr, "Unix");
		break;
	case MODE_MSDOS:
		strcpy (modestr, "MSDOS");
		break;
	case MODE_MACINTOSH:
		strcpy (modestr, "Macintosh");
		break;
	default :
		strcpy (modestr, "??????");
		break;
	}
	status (0, "TextStar, version %s", tsrev, NULL);
	wmove (editwin, 0, 0);
	wp4 ("           Editing: `%s` at line `%u` column `%u`\n", txtname, txt_x, txt_y);		/* show full name */
	wp2 ("          Modified: `%s`\n", mod ? "Yes" : "No");
	wp2 ("         File mode: `%s`\n", modestr);
	wp2 ("      Right margin: `%d`\n", rt_marg);
	wp2 ("    Hard tab width: `%d`\n", tabdef);
	wp2 ("Control characters: `%s`\n", cntlflag ? "On" : "Off");
	wp3 ("      Memory usage: `%u` bytes `%u` free\n",
	     gap_beg - txt_beg + txt_end - gap_end, gap_end - gap_beg - 1);
	wp2 ("     Terminal type: `%s`\n", longname ());
	wattrset (editwin, PROMPTATTR);
	wp1 ("    Strike any key: ");
	wattrset (editwin, A_NORMAL);
	wrefresh (editwin);
	wait_key ();					/* Get key */
	status_line ();
	disp_mode = dpage;			/* Repaint text */
}
/* #######################################################################
 * int del_right (void)
 * purpose:
 *     Delete the character UNDER the cursor (the ^G target): grow the
 *     gap by one byte at gap_end, jumping any block marker out of the
 *     way. Refuses to delete the newline that terminates the text - the
 *     [EOF] marker must stay on its own line.
 * returns:
 *     1 when a character was deleted, 0 at the end of the text or when
 *     the final newline was the target.
 * parameters:
 *     None; the cursor is gap_end.
 * notes:
 *     Sets disp_mode: dtoeos when a newline was deleted (rest of the
 *     screen moves up), dtoeolhc otherwise (rest of the line).
 * ####################################################################### */
int del_right (void)
{
	if (gap_end < txt_end)		/* Can't delete past end of file */
	{
		/* Never delete the newline that terminates the text:
		 * the [EOF] marker must stay on its own line. */
		if (gap_end + 1 == txt_end && *gap_end == '\n')
		{
			return (0);
		}
		if (*gap_end == '\n')
		{
			disp_mode = dtoeos;
		}
		else
		{
			disp_mode = dtoeolhc;
		}
		if (blk_beg == gap_end)
		{
			blk_beg++;
		}
		if (blk_end == gap_end)
		{
			blk_end++;
		}
		gap_end++;
		mod = TRUE;
		return (1);
	}
	else
	{
		return (0);
	}
}
/* #######################################################################
 * int name (void)
 * purpose:
 *     Prompt for a file name and make it the editor's file name
 *     (^K N). Only the first whitespace-separated token is taken.
 * returns:
 *     1 when a name was entered and set, 0 on empty input.
 * parameters:
 *     None; reads the response buffer, writes txtname.
 * notes:
 *     Refreshes the status line so the new name shows immediately.
 * ####################################################################### */
int name (void)
{
	char *tokptr;

	/* Prompt and take first token: */
	if ((prompt_st ("Name file: ", "", respptr, respsiz) > 0) &&
		((tokptr = strtok (respptr, " \t")) != NULLCHARPTR))
	{
		strcpy (txtname, tokptr);
		status_line ();
		return (1);
	}
	else
	{
		return (0);	/* Null response */
	}
}
/* #######################################################################
 * int save_fname (void)
 * purpose:
 *     Save the WHOLE text to txtname (prompting for a name first when
 *     there is none). Works by aiming the block markers at the full
 *     text and handing the job to save(), restoring the user's block
 *     markers afterwards - so all of save()'s line-ending logic is
 *     reused.
 * returns:
 *     1 when the file was written, 0 on failure (no name, I/O error).
 * parameters:
 *     None.
 * ####################################################################### */
int save_fname (void)
{
	int ioresult;
	char *old_beg, *old_end;

	if (strlen (txtname) == 0)		/* Have filename? */
	{
		if (!name ())
		{
			return (0);	/* Must have name! */
		}
	}

	/* Set up phony "block" for save() function */

	old_beg = blk_beg;				/* Save block pointers */
	old_end = blk_end;

	if (gap_beg != txt_beg)
	{
		blk_beg = txt_beg + 1;
	}
	else
	{
		blk_beg = gap_end;
	}

	if (gap_end != txt_end)
	{
		blk_end = txt_end - 1;
	}
	else
	{
		blk_end = gap_beg;
	}

	ioresult = save (txtname, "wb");	/* Save "block" */

	blk_beg = old_beg;				/* Restore block pointers */
	blk_end = old_end;

	return ioresult;
}
/* #######################################################################
 * int find (void)
 * purpose:
 *     Case-independent forward search for the global search string
 *     (sptr) from txt_ptr: for each candidate position, copy as many
 *     bytes as the search string is long into a work buffer, uppercase
 *     both and compare. On a match the cursor (gap) is moved there.
 * returns:
 *     1 when found (cursor on the match, disp_mode = dpages to center
 *     it), 0 when the end of the text is reached.
 * parameters:
 *     None; txt_ptr is the start position and the search string comes
 *     from sptr.
 * notes:
 *     The copy-and-compare keeps the user's text untouched (an older
 *     version uppercased the search string in place). Works across the
 *     gap since txt_ptr walks byte-wise.
 * ####################################################################### */
int find (void)
{
	char fbuf[70], *fbufptr;
	char sup[60];
	int	s_strlen;

	s_strlen = strlen (sptr);
	strcpy (sup, sptr);
	strtoup (sup);
	fbufptr = &fbuf[0];

	while (txt_ptr <= txt_end)
	{
		if (toupper (*txt_ptr) == toupper (*sptr)) /* 1st chars match */
		{
			/* Copy user's text to work buffer,		*/
			/*  length of search string or null,	*/
			/*  whichever comes first: 				*/
			strncpy (fbufptr, txt_ptr, s_strlen);
			/* Add our own terminating null: */
			fbuf[s_strlen] = '\0';
			/* Case-insensitive search: */
			strtoup (fbufptr);

			if (strcmp (fbufptr, sup) == 0)
			/* match */
			{
				move_ptr ();
				disp_mode = dpages;	/* Scroll to midscreen */
				return (1);			/* Report success */
			}
		}
		txt_ptr++;
	}
	return (0);			/* report failure */
}
/* #######################################################################
 * int replace (void)
 * purpose:
 *     Replace the found text at the cursor with the replace string
 *     (rptr): delete the old text with del_right() (block-safe), write
 *     the new text into the gap, and adjust txt_x/txt_y for any
 *     newlines the replacement contains. Asks "Change (Y N *)?" per
 *     instance unless the user answered * (change all).
 * returns:
 *     1 when handled (replaced or skipped), 0 when the user aborted
 *     (^U/ESC) or the gap had no room.
 * parameters:
 *     None; uses the global search string (sptr), replace string
 *     (rptr) and repkey.
 * notes:
 *     repkey = '*' skips all further prompts (the ^L global replace).
 *     'N'/RETURN just skips past this instance.
 * ####################################################################### */
int replace (void)
{
	int strdif, counter;
	char *tmpptr;

	strdif = strlen (rptr) - strlen (sptr);
	if (gap_beg + strdif >= gap_end)
	{
		tpa_err ();
		return (0);
	}
	getrepkey:
	if (repkey != '*')
	{
		wattron (statuswin, A_BOLD);
		status (0, "Change (Y/N/*)? ", NULL, NULL);
		stat_print (COLS - 12, "^U to abort", NULL, NULL);
		wattroff (statuswin, A_BOLD);
		disp_mode = dpages;
		display ();
		txt_ptr = gap_end;	/* restore txt_ptr */
		wrefresh (editwin);	/* show cursor location */
	}
	if (repkey != '*')
	{
		repkey = toupper (wait_key ());
		status_line ();
	}
	switch (repkey)
	{
	case 'N':				/* Skip forwards past instance */
	case '\r':
		for (counter = strlen (sptr) ;
		     (counter > 0) && (txt_ptr < txt_end) ; counter--)
		{
			txt_ptr++;
		}
		move_ptr ();			/* Restore cursor location & block markers */
		break;

	case '*':				/* Perform replace operation */
	case 'Y':
		mod = TRUE;
		for (counter = strlen (sptr) ;
		     counter > 0 ; counter--)	/* Remove old string, */
		{
			del_right ();	  		/* Preserve block markers */
		}
		disp_mode = dpage;				/* Restore from del_right() */
		gap_beg++;
		strcpy (gap_beg, rptr);
		gap_beg += strlen (rptr) - 1;
		txt_ptr = gap_end;
		/* Keep track of line numbers: */
		tmpptr = rptr;
		while ((tmpptr = (char *) strchr (tmpptr, '\n')))	/* Count newlines */
		{
			tmpptr++;
			txt_x++;
		}
		txt_y = column (gap_end);	/* Set cursor column */
		break;

	case ESC:			/* Abort replace operation */
	case CNTRL_U:
		return (0);

	default:			/* Complain if user hits invalid key */
		beep ();
		goto getrepkey;
	}
	return (1);			/* Report success */
}
/* #######################################################################
 * void goto_line (void)
 * purpose:
 *     The ^Q I "go to line" command: prompt for a line number
 *     (pre-filled suggestion = current line) and jump there.
 * returns:
 *     Nothing.
 * parameters:
 *     None.
 * notes:
 *     A non-numeric answer beeps with "Value error"; the cursor lands
 *     on the closest reachable line and the screen centers on it
 *     (dpages).
 * ####################################################################### */
void goto_line (void)
{
	int line_no;
	char pbuf[40];

	sprintf (pbuf, "Go to line [%d]: ", txt_x);
	if (prompt_st (pbuf, "", respptr, 5) > 0)
	{
		if (sscanf (respptr, "%d", &line_no) == 1)
		{
			/* Move to line number given */
			txt_x = move_x (line_no);
			disp_mode = dpages;
		}
		else
		{
			val_err ();	/* Value error */
		}
	}
}
/* #######################################################################
 * void tab_set (void)
 * purpose:
 *     The ^O T "set tab width" command: prompt for a value and accept
 *     only 2..32 (clamped by the parser at 1..32 for char arithmetic
 *     safety). Repaints the page since every TAB expands differently.
 * returns:
 *     Nothing.
 * parameters:
 *     None; writes tabdef.
 * notes:
 *     Recomputes txt_y - the cursor's column may shift with the new
 *     tab stops.
 * ####################################################################### */
void tab_set (void)
{
	int c;
	char pbuf[40];

	sprintf (pbuf, "Tab width [%d]: ", tabdef);
	if (prompt_st (pbuf, "", respptr, 2) > 0)
	{
		if ((sscanf (respptr, "%d", &c) == 1) && (c > 1) && (c <= 32))
		{
			tabdef = c;
			txt_y = column (gap_end);
			disp_mode = dpage;
		}
		else
		{
			val_err ();
		}
	}
}
/* #######################################################################
 * void rm_set (void)
 * purpose:
 *     The ^O R "set right margin" command: prompt for a value; 0
 *     switches the margin off (no wrapping), any other value sets the
 *     column the margin applies to. The margin drives ^B reformatting
 *     and the word wrap in add_text().
 * returns:
 *     Nothing.
 * parameters:
 *     None; writes rt_marg.
 * notes:
 *     No repaint needed - the margin only affects future typing. The
 *     status line shows the setting as "RM -" when off.
 * ####################################################################### */
void rm_set (void)
{
	int c;
	char pbuf[40];

	sprintf (pbuf, "Right margin [%d]: ", rt_marg);
	if (prompt_st (pbuf, "", respptr, 3) > 0)
	{
		if ((sscanf (respptr, "%d", &c) == 1) && (c >= 0))	/* 0 = no margin */
		{
			rt_marg = c;
		}
		else
		{
			val_err ();
		}
	}
}
/* #######################################################################
 * void reform (void)
 * purpose:
 *     The ^B reformat command: re-flow the paragraph from the cursor's
 *     line to the next hard CR (a line whose last character before the
 *     break is not a space) or the end of the text. Lines are joined
 *     (their leading whitespace eaten) and re-broken at the right
 *     margin, breaking before the word that would cross it. The final
 *     newline of the text is never joined away - the [EOF] marker
 *     stays on its own line. With no right margin (rt_marg = 0) there
 *     is nothing to reformat towards and the command returns at once.
 * returns:
 *     Nothing; the cursor ends after the last reformed line.
 * parameters:
 *     None.
 * notes:
 *     Does nothing while margin release (MRflag) is on. The line break
 *     needs one free byte in the gap (gap_beg + 1 < gap_end); a
 *     line with no space before the margin cannot be broken and beeps
 *     "Can't reformat!".
 * ####################################################################### */
void reform (void)
{
	char	*tmp_ptr;
	long	col_cnt;

	if (MRflag || rt_marg == 0)	/* Margin released or no margin set:
				 * nothing to reformat towards */
	{
		return;
	}
	disp_mode = dpage;		/* Always redraw screen */
	mod = TRUE;
	txt_ptr = start_line (gap_end);
	/* Process lines until hard CR or EOF reached */
	while (txt_ptr < txt_end)
	{
		col_cnt = 1;
		txt_ptr = start_line (gap_end);
		text_move ();
		txt_ptr = gap_end;
		while ((txt_ptr < txt_end) && (col_cnt <= rt_marg))
		/* Process characters until right margin reached */
		{
			switch (*txt_ptr)
			{
			case '\n':						/* Join lines */
				text_move ();
				/* Never join away the final newline of the text:
				 * the [EOF] marker must stay on its own line. */
				if (gap_end + 1 == txt_end)
				{
					txt_y = column (gap_end);
					return;
				}
				/* If hard CR, move cursor & quit */
				if ((*gap_beg != ' ') || (*(gap_end + 1) == '\n'))
				{
					txt_ptr++;
					text_move ();
					txt_x++;
					txt_y = column (gap_end);
					return;
				}
				/* Else eat whitespace, remove newline, & continue */
				do
				{
					txt_ptr = gap_end;
					if (blk_beg == gap_end)		/* Preserve blocks */
					{
						blk_beg++;
					}
					if (blk_end == gap_end)
					{
						blk_end++;
					}
					gap_end++;					/* Remove newline */
				}
				while (((*gap_end == ' ') || (*gap_end == '\t')) &&
						(gap_end < txt_end));
				break;

			case '\t':						/* Count hard TAB */
				col_cnt = col_cnt - (col_cnt - 1) % tabdef + tabdef;
				break;

			default:
				if (isprint (*txt_ptr))		/* ^chars. don't count */
				{
					col_cnt++;
				}
			}
			txt_ptr++;
		}
		/* Right margin reached; break line */
		text_move ();					/* Move gap */
		if (txt_ptr == txt_end)			/* Stop if EOF */
		{
			txt_y = column (gap_end);
			return;
		}
		txt_ptr = gap_beg;
		while ((*txt_ptr != ' ') &&	/* Find prev. SPACE */
				(*txt_ptr != '\n') &&	/* or error 		*/
				(txt_ptr > txt_beg))	/*   conditions 	*/
		{
			txt_ptr--;
			col_cnt--;
		}
		if (*txt_ptr != ' ')			/* Error */
		{
			txt_y = column (gap_end);
			beep ();
			prompt_key ("Can't reformat! ");
			return;
		}
		/* Break line if necessary */
		else if ((col_cnt == rt_marg + 1) && /* A "perfect fit!" */
				(*gap_end == '\n'))
		{
			if (blk_beg == gap_end)		/* Preserve blocks */
			{
				blk_beg++;
			}
			if (blk_end == gap_end)
			{
				blk_end++;
			}
			*++gap_beg = *gap_end++;	/* Goto next line */
			txt_ptr = gap_end;
		}
		else							/* Break line */
		{
			if (gap_beg + 1 >= gap_end)
			{
				tpa_err ();
				txt_y = column (gap_end);
				return;
			}
			tmp_ptr = gap_beg;
			while (tmp_ptr >= txt_ptr)  /* First shift text */
			{
				*(tmp_ptr + 1) = *tmp_ptr;
				tmp_ptr--;
			}
			gap_beg++;					/* Space for newline */
			*(txt_ptr + 1) = '\n';		/* Break line */
			txt_ptr += 2;
		}
		text_move ();					/* Move gap */
		txt_x++;
		txt_y = column (gap_end);
	}
}
/* #######################################################################
 * void ts_abort (int sig)
 * purpose:
 *     The signal handler that tries to preserve the user's work when
 *     the editor dies: leave curses, pick a save directory ($HOME on
 *     UNIX, $TEMP/$TMP on DOS), build an unused "TSsaved.<pid>[.n]"
 *     name, sanity-check the gap pointers, save the whole text via
 *     save_fname() and report what happened. Fatal signals then re-
 *     raise themselves for a core image.
 * returns:
 *     Does not return (exit(1)).
 * parameters:
 *     int sig: the signal number that invoked the handler (SIGINT,
 *     SIGTERM, SIGSEGV, ... per platform).
 * notes:
 *     Re-arms itself with SIG_IGN first so a second signal cannot
 *     interrupt the save. Old SUEsaved.* files are simply not found
 *     any more - the prefix followed the rename.
 * ####################################################################### */
void ts_abort (int sig)
{
	char *homeptr;
	char savname[TXTNAME_SIZE];
	int n;
	FILE *probe;

	endwin ();
	printf ("\n");

	if (signal (sig, SIG_IGN) == SIG_ERR)
	{
		perror ("TextStar signal");
	}

	/* Look for a directory to save the work in */
#ifdef UNIX
	if ((homeptr = getenv ("HOME")) == (char *) NULL ||
		strlen (homeptr) > TXTNAME_SIZE - 32)
	{
		homeptr = "/tmp";	/* not good, better than nothing */
	}
#endif
#ifdef DOS
	homeptr = getenv ("TEMP");
	if (homeptr == (char *) NULL)
	{
		homeptr = getenv ("TMP");
	}
	if (homeptr == (char *) NULL || strlen (homeptr) > TXTNAME_SIZE - 32)
	{
		homeptr = ".";		/* current directory */
	}
#endif

	/* Build a unique filename that is not in use (replaces tempnam()) */
	for (n = 0; n < 1000; n++)
	{
		if (n == 0)
		{
			sprintf (savname, "%s/TSsaved.%ld", homeptr, (long) getpid ());
		}
		else
		{
			sprintf (savname, "%s/TSsaved.%ld.%d", homeptr,
				(long) getpid (), n);
		}
		if ((probe = fopen (savname, "rb")) == (FILE *) NULL)
		{
			break;		/* Name not taken */
		}
		fclose (probe);
	}

	strcpy (txtname, savname);	/* Make it the new filename */

	fprintf (stderr, "TextStar: ");	/* Display signal received */
	switch (sig)
	{
#ifdef UNIX
	case SIGHUP:
		fprintf (stderr, "Hangup");
		break;
	case SIGQUIT:
		fprintf (stderr, "Quit");
		break;
	case SIGSYS:
		fprintf (stderr, "Bad system call");
		break;
	case SIGBUS:
		fprintf (stderr, "Bus error");
		break;
#endif
	case SIGINT:
		fprintf (stderr, "Interrupt");
		break;
	case SIGTERM:
		fprintf (stderr, "Terminate");
		break;
	case SIGSEGV:
		fprintf (stderr, "Segment violation");
		break;
	case SIGILL:
		fprintf (stderr, "Illegal instruction");
		break;
	}
	fprintf (stderr, " signal received, attempting to save your work...");

	/* Do a little sanity checking before attempting save */
	if ((gap_beg > txt_end) || (gap_beg < txt_beg) ||
		(gap_end > txt_end) || (gap_end < txt_beg))
	{
		gap_beg = gap_end = (txt_beg + (txt_end - txt_beg) / 2);
	}

	if (save_fname ())
	{
		fprintf (stderr, "\nTextStar: work saved to \"%s\".\n", txtname);
	}
	/* Note: 'vi' uses /usr/preserve/[username] directory & sends mail */

#ifdef UNIX
	if (sig == SIGSYS || sig == SIGILL || sig == SIGBUS ||
		sig == SIGSEGV)
	{
		kill (0, sig);	/* try for core image too */
	}
#endif
	exit (1);
}

/********************** FILE I/O ROUTINES *********************/

/* #######################################################################
 * void writeblk (void)
 * purpose:
 *     The ^K W "write block" command: prompt for a file name and write
 *     the marked block to it. An existing file is offered as
 *     Overwrite/Append/Quit.
 * returns:
 *     Nothing.
 * parameters:
 *     None.
 * notes:
 *     save() does the writing with mode "wb"/"ab"; nothing happens
 *     when no block is marked.
 * ####################################################################### */
void writeblk (void)
{
	int	c;

	if (prompt_st ("Write block to: ", "", respptr, respsiz) > 0)
	{
		if (blk_beg == (char *) NULL)	/* no block specified */
		{
			return;
		}

		if (fopen (respptr, "rb") == (FILE *) NULL)	/* Exists? */
		{
			save (respptr, "wb");						/* No */
		}
		else
		{
			c = toupper (prompt_key (
				"File exists: O(verwrite)/A(ppend)/Q(uit)? Q\b"));
			if (c == 'A' || c == 'O')
			{
				wechochar (promptwin, c);
			}
			switch (c)
			{
			case 'A':
				save (respptr, "ab"); /* Append block to file */
				break;
			case 'O':				/* Overwrite file */
				save (respptr, "wb");
			}
		}
	}
}
/* #######################################################################
 * void readblk (void)
 * purpose:
 *     The ^K R "read block" command: load a file INTO the text at the
 *     cursor (load() appends at the gap and the gap is then moved to
 *     the insertion point). "File not found"/"File too big" feedback
 *     as appropriate.
 * returns:
 *     Nothing.
 * parameters:
 *     None.
 * notes:
 *     Uses load() like a file open does - the loaded bytes become part
 *     of the edited text, not a separate block.
 * ####################################################################### */
void readblk (void)
{
	int inlines;
	char *tmp_ptr;

	if (prompt_st ("Read block from: ", "", respptr, respsiz) > 0)
	{
		tmp_ptr = gap_beg + 1;
		inlines = load (respptr);
		if (inlines == -1)
		{
			return;					/* File was too large */
		}
		if (inlines)				/* File found */
		{
			txt_ptr = tmp_ptr;
			text_move ();
			mod = TRUE;
			disp_mode = dpage;
		}
		else						/* No file */
		{
			beep ();
			prompt_key ("File not found ");
		}
	}
}
/* #######################################################################
 * void new_file (void)
 * purpose:
 *     The ^K L "load new file" command: ask about modified text
 *     (abandon), prompt for a file name and re-run the whole session
 *     setup (edit_init).
 * returns:
 *     Nothing.
 * parameters:
 *     None.
 * notes:
 *     edit_init() handles allocation, loading and the "New file"
 *     warning; this command only guards and feeds it.
 * ####################################################################### */
void new_file (void)
{
	if (mod)
	{
		if (abandon () != 'Y')
		{
			return;
		}
	}
	if (prompt_st ("Load file: ", "", respptr, respsiz) > 0)
	{
		edit_init (response);
		/*		disp_mode = dpage;*/
	}
}
/* #######################################################################
 * int save (char *s_name, char *s_type)
 * purpose:
 *     Write the marked block to a file: open it in the given mode,
 *     walk the buffer from blk_beg to blk_end (jumping the gap) and
 *     write every byte, translating newlines per filemode (LF for
 *     Unix, CRLF for MSDOS, CR for Macintosh - the buffer always holds
 *     LF internally). Checks ferror/fclose so a disk-full cannot pass
 *     as success.
 * returns:
 *     1 on success, 0 on any error (no name, open failed, write/
 *     close failed) - with feedback on the prompt line.
 * parameters:
 *     char *s_name: the destination file name.
 *     char *s_type: the fopen mode - "wb", "ab" (callers always use
 *     binary modes so DOS does not double-translate CRLF).
 * notes:
 *     All file I/O goes through here (whole-text saves aim the block
 *     markers at the full text first). Only 7-bit text: the buffer
 *     never holds bytes >= 0x80.
 * ####################################################################### */
int save (char *s_name, char *s_type)
{
	FILE	*s_file;
	char	*ptr;

	/* This should never happen: */
	if (strlen (s_name) == 0)
	{
		beep ();
		prompt_key ("No name specified ");
		return (0);
	}

	status (0, "TextStar writing \"%s\"...", s_name, NULL);

	s_file = fopen (s_name, s_type);
	if (s_file == (FILE *) NULL)
	{
		beep ();
		prompt_key ("Can't write to file! ");
		status_line ();	/* restore */
		return (0);
	}
	ptr = blk_beg;
	while (ptr <= blk_end)
	{
		if (*ptr == '\n')				/* May have to xlate */
		{
			if (filemode != MODE_UNIX)
			{
				fputc ('\r', s_file);
			}
			if (filemode != MODE_MACINTOSH)	/* \r for MACINTOSH,			*/
			{
				fputc ('\n', s_file);	/* \n for Unix, \r\n for MSDOS	*/
			}
		}
		else
		{
			fputc (*ptr, s_file);
		}
		if (ptr == gap_beg)
		{
			ptr = gap_end;
		}
		else
		{
			ptr++;
		}
	}
	if (ferror (s_file) != 0 || fclose (s_file) != 0)
	{
		beep ();
		prompt_key ("Error writing file! ");
		status_line ();	/* Restore */
		return (0);
	}
	status_line ();	/* Restore */
	return (1);
}
/* #######################################################################
 * int load (char *l_name)
 * purpose:
 *     Read a file into the gap (appending at gap_beg), detecting its
 *     line-ending style on the fly: CRLF -> MODE_MSDOS, lone CR ->
 *     MODE_MACINTOSH, no CR -> MODE_UNIX. All endings are normalized
 *     to LF in the buffer; NUL bytes and high bits are stripped (the
 *     editor is 7-bit). On the first load of a session (and when no
 *     -d/-m/-u option was given) the detected mode becomes filemode.
 * returns:
 *     The number of lines loaded (j); 0 when the file could not be
 *     opened; -1 when the file does not fit the gap (nothing kept).
 * parameters:
 *     char *l_name: the file to read.
 * notes:
 *     Used by edit_init() (initial load) and readblk() (insert at
 *     cursor). The gap-shrinking insert checks the space per byte and
 *     rewinds cleanly on overflow.
 * ####################################################################### */
int load (char *l_name)
{
	FILE	*l_file;
	int	c;
	int	j;
	enum filemodes mode = MODE_UNIX;
	char *old_beg;

	if ((l_file = fopen (l_name, "rb")) == (FILE *) NULL)
	{
		return (0);
	}

	status (0, "TextStar loading \"%s\"...", l_name, NULL);
	j = 1;
	old_beg = gap_beg;
	while ((c = fgetc (l_file)) != EOF)
	{
		if (gap_beg + 1 >= gap_end)
		{
			beep ();
			prompt_key ("File too big ");
			gap_beg = old_beg;
			fclose (l_file);
			status_line ();
			return (-1);
		}
		else
		{
			c &= 0x7F;	/* strip high bits */
			if (c)		/* Remove NULLs */
			{
				*(gap_beg + 1) = c;		/* do_alloc() sets initial gap_beg to \n */
				if (*gap_beg == '\r')	/* Previous was \r */
				{
					*gap_beg = '\n';	/* Xlate \r to \n and \r\n to \n\n */
					if (c == '\n')		/* Hit a \r\n */
					{
						mode = MODE_MSDOS;
						gap_beg--;		/* Avoid \n\n */
					}
					else				/* Hit text\rtext */
					{
						mode = MODE_MACINTOSH;
					}
				}
				if (*gap_beg == '\n')
				{
					j++;
				}
				gap_beg++;
			}
		}
	}
	if (*gap_beg == '\r')
	{
		*gap_beg = '\n';
	}
	if (autofilemode)			/* Options override		*/
	{
		filemode = mode;		/* ..else autodetect	*/
	}
	else
	{
		autofilemode = 1;		/* Only masked 1st time */
	}
	fclose (l_file);
	status_line ();
	return (j);
}
/* #######################################################################
 * void set_file_mode (void)
 * purpose:
 *     The ^K F "change file mode" command: prompt U/D/M and set
 *     filemode (Unix / msDos / Macintosh line endings for save()).
 * returns:
 *     Nothing.
 * parameters:
 *     None.
 * notes:
 *     Also clears autofilemode - once the user picks a mode manually,
 *     the load-time autodetection must not override it.
 * ####################################################################### */
void set_file_mode (void)
{
	int	keyin;

	keyin = toupper (prompt_key ("File mode: Unix, msDos, or Macintosh (U/D/M)? "));
	switch (keyin)
	{
	case 'U':
		filemode = MODE_UNIX;
		autofilemode = 0;
		break;
	case 'D':
		filemode = MODE_MSDOS;
		autofilemode = 0;
		break;
	case 'M':
		filemode = MODE_MACINTOSH;
		autofilemode = 0;
		break;
	case ' ':
		break;
	default :
		beep ();
		break;
	}
}
/* #######################################################################
 * void mar_rel (void)
 * purpose:
 *     The ^O X "margin release" toggle: when on, the right margin does
 *     not word-wrap typed text and ^B refuses to reformat. Updates the
 *     MR status-line indicator.
 * returns:
 *     Nothing.
 * parameters:
 *     None; toggles MRflag.
 * ####################################################################### */
void mar_rel (void)
{
	MRflag = !MRflag;
	if (MRflag)
	{
		stat_print (COLS - 27, "MR", NULL, NULL);
	}
	else
	{
		stat_print (COLS - 27, "  ", NULL, NULL);
	}
}
/* #######################################################################
 * void block_funcs (int funcno)
 * purpose:
 *     The ^K block operations dispatcher: 1 = begin mark at cursor,
 *     2 = end mark at cursor, 3 = clear markers, 4 = move block to
 *     cursor, 5 = copy block to cursor, 6 = delete block. Move/copy/
 *     delete share one implementation: insert the marked text in
 *     front of the cursor via text_ins (FORWARD), then - for move and
 *     delete - expand the gap backwards over the original text.
 * returns:
 *     Nothing.
 * parameters:
 *     int funcno: which operation, 1..6 (see above).
 * notes:
 *     blk_err() guards the cases; block markers are preserved across
 *     the gap moves (tblk_* temporaries hold the moved block's new
 *     position). Undo: the delete path saves the block first; the
 *     cursor is restored to its pre-operation position.
 * ####################################################################### */
void block_funcs (int funcno)
{
	char	*old_ptr;

	switch (funcno)
	{
	case 1:						/* Begin block */
		if (blk_end > gap_end)	/* Block ends at/after cursor */
		{
			blk_beg = gap_end;
		}
		else
		{
			blk_beg = blk_end = gap_end;
		}
		disp_mode = dpage;
		break;

	case 2:				/* End block */
		blk_end = gap_beg;
		if ((blk_beg == (char *) NULL) || (gap_beg <= blk_beg))
		{
			blk_beg = blk_end;
		}
		disp_mode = dpage;
		break;

	case 3:				/* Clear block markers */
		blk_beg = blk_end = NULL;
		disp_mode = dpage;
		break;

	case 4:			/* Move (copy then delete) */
	case 5:			/* Copy to in front of cursor */
		if (blk_err (2))				/* In or not marked */
		{
			break;
		}
		tblk_end = gap_end - 1;		/* Prepare to move block end (case 4) */
		if (!text_ins (blk_beg, blk_end + 1, FORWARD))
		{
			break;					/* TPA error */
		}
		tblk_beg = gap_end;			/* Prepare to move block begin (case 4) */
		mod = TRUE;
		disp_mode = dpage;
		if (funcno == 5)	/* Quit for "copy block" */
		{
			break;
		}
		/*FALLTHRU*/
	case 6:							/* Delete block */
		if (blk_err (1))				/* Not marked */
		{
			break;
		}
		if (blk_beg > gap_end)		/* Preserve cursor location */
		{
			old_ptr = gap_beg + 1;
		}
		else
		{
			old_ptr  = gap_end;
		}

		if (blk_end != gap_beg)		/* Move cursor to block end */
		{
			txt_ptr  = blk_end + 1;
			txt_x   += text_move ();
		}
		if (funcno == 6)			/* If we're erasing.. */
		{
			if (!undel_save (blk_beg, gap_beg - blk_beg + 1, FORWARD))
			{
				goto NoBlockDel;	/* Didn't save, so don't delete */
			}
		}
		/* Expand gap backward to block begin */
		for ( ; gap_beg >= blk_beg ; gap_beg--)
		{
			if (*gap_beg == '\n')
			{
				txt_x--;
			}
		}
		if (funcno == 6)
		{
			blk_beg = blk_end = NULL;
		}
		else
		{
			blk_beg = tblk_beg;		/* Restore markers */
			blk_end = tblk_end;
		}
		mod = TRUE;
		disp_mode = dpage;
		NoBlockDel:
		txt_ptr = old_ptr;			/* Restore cursor */
		txt_x  += text_move ();
		txt_y   = column (gap_end);
		break;
	}
}
/* #######################################################################
 * void toggle_help (void)
 * purpose:
 *     Toggle the 8-line persistent help window at the top of the
 *     screen: destroy and recreate the curses windows (they overlap
 *     differently), re-apply keypad/attributes and repaint.
 * returns:
 *     Nothing.
 * parameters:
 *     None.
 * notes:
 *     editlines (the edit window height) follows helplines. The
 *     prompt(1, "") dance is needed so newwin()'s auto-clear does not
 *     wipe text on the first warning().
 * ####################################################################### */
void toggle_help (void)
{
	if (helplines)
	{
		helplines = 0;
		delwin (helpwin);
	}
	else
	{
		helplines = 8;
	}
	delwin (editwin);			/* Kill current curses windows */
	delwin (promptwin);
	editlines = LINES - helplines - 2;	/* Status line + ruler on top */
	scr_xend = editlines;
	/* Set up new windows: */
	editwin   = newwin (editlines, COLS, helplines + 2, 0);	/* Next 2 overlap */
	promptwin = newwin (1, COLS, helplines + 2, 0);
	wattrset (promptwin, A_BOLD);
	keypad (editwin, TRUE);		/* Interpret extended keycodes */
	keypad (promptwin, TRUE);
	prompt (1, "", NULL, NULL, NULL);			/* Avoid autoclear */
	status_line ();				/* Clear expert help message */
	disp_mode = dpage;
}
/* #######################################################################
 * void rep_find (void)
 * purpose:
 *     The ^L "repeat find/replace" command: repeat the last operation
 *     from the cursor. In find mode (repflag = FALSE) search one more
 *     time; in replace mode find the next instance and run replace() -
 *     when the user answered * (change all) earlier, keep replacing
 *     until find() fails.
 * returns:
 *     Nothing.
 * parameters:
 *     None; reads repflag, s_str, rptr, repkey.
 * notes:
 *     In find mode the search starts AFTER the cursor (gap_end + 1)
 *     so the same match is not found again.
 * ####################################################################### */
void rep_find (void)
{
	if (strlen (s_str) > 0)
	{
		repkey = 0;
		if (repflag)			/* if we are replacing */
		{
			txt_ptr = gap_end;	/* start at cursor location */
		}
		else
		{
			txt_ptr = gap_end + 1;	/* if finding start after cursor */
		}
		if (find ())
		{
			if (repflag)		/* if replacing */
			{
				replace ();
				/* user said global? */
				if (repkey == '*')
				{
					while (find ())
					{
						replace ();
					}
				}
			}
		}
		else
		{
			not_found ();
		}
	}
}
/* #######################################################################
 * void case_invert (void)
 * purpose:
 *     The ^^ "invert case" command: invert the letter under the cursor
 *     in place (writing it back through out_char so block highlight
 *     stays correct) and advance the cursor one position.
 * returns:
 *     Nothing.
 * parameters:
 *     None.
 * notes:
 *     Only the isprint() case is touched; the in-place write means
 *     txt_ptr must be re-synced to gap_end first (the cursor side of
 *     the gap).
 * ####################################################################### */
void case_invert (void)
{
	txt_ptr = gap_end;
	if (isprint (*txt_ptr))
	{
		out_char (*txt_ptr = invert (*txt_ptr));
		mod = 1;
	}
	txt_y = move_y (movefwd, movechar);	/* Advance cursor */
}
/* #######################################################################
 * void del_word (void)
 * purpose:
 *     The ^T "delete word right" command: repeatedly del_right() up to
 *     the end of the word - stopping at a newline, sentence punctuation
 *     (. , :) or a space/tab boundary - then save the deleted range to
 *     the undo stack.
 * returns:
 *     Nothing.
 * parameters:
 *     None.
 * notes:
 *     Block markers are snapshotted and restored when the undo save
 *     fails (the user declined to delete without undo) - the gap is
 *     rewound to the start position.
 * ####################################################################### */
void del_word (void)
{
	char	*tmp_ptr, *tmp_blkbeg, *tmp_blkend;

	tmp_ptr = gap_end;
	tmp_blkbeg = blk_beg;
	tmp_blkend = blk_end;
	while
		(del_right () &&
		(*(gap_end - 1) != '\n') &&
		(*(gap_end - 1) != '.') &&
		(*(gap_end - 1) != ',') &&
		(*(gap_end - 1) != ':') &&
		((*(gap_end - 1) != ' ' && *(gap_end - 1) != '\t') ||
		 (*gap_end == ' ' || *gap_end == '\t')));
	if (!undel_save (tmp_ptr, gap_end - tmp_ptr, FORWARD))
	{
		blk_beg = tmp_blkbeg;
		blk_end = tmp_blkend;
		gap_end = tmp_ptr;
	}
}
/* #######################################################################
 * void del_line (void)
 * purpose:
 *     The ^Y "delete line" command: move to the start of the line
 *     (line_y(1)), extend the gap over the whole line up to and
 *     including its newline (clamped at txt_end), saving the line to
 *     the undo stack first.
 * returns:
 *     Nothing.
 * parameters:
 *     None.
 * notes:
 *     On undo-save failure the line AND the cursor column are restored
 *     (del_line is careful not to lose text it cannot undo). Leaves
 *     the cursor at the same column on the line that moved up
 *     (disp_mode = dtoeos).
 * ####################################################################### */
void del_line (void)
{
	char	*tmp_ptr;
	int	old_y;

	old_y = txt_y;		/* Remember cursor column */
	line_y (1);			/* Shift gap backward (left) to BOL */
	if (gap_end < txt_end)
	{
		tmp_ptr = gap_end;
		while (*gap_end++ != '\n');
		if (gap_end > txt_end)
		{
			gap_end = txt_end;
		}
		if (!undel_save (tmp_ptr, gap_end - tmp_ptr, FORWARD))
		{
			gap_end = tmp_ptr;	/* Restore the line ... */
			txt_y = 1;			/* ... and the cursor position */
			txt_y = line_y (old_y);
			return;
		}
		txt_y = 1;			/* txt_x unchanged */
		disp_mode = dtoeos;
		mod = TRUE;
	}
}
/* #######################################################################
 * int split_line (int keep_cursor)
 * purpose:
 *     Split the line at the cursor with auto-indent: the new line
 *     starts at the same column as the current one - the leading
 *     whitespace run (spaces/tabs, byte-exact) is carried over; when
 *     the cursor sits inside the run, the after-cursor remainder is
 *     trimmed so the new line starts at exactly the old line's
 *     column. Used by RETURN (key_scan) and ^N (insert_newline).
 * returns:
 *     1 when the split was done; 0 when the gap had no room for
 *     newline + indent (tpa_err()'d, nothing changed).
 * parameters:
 *     int keep_cursor: 0 = the cursor moves to the end of the new
 *     line's indent (RETURN); 1 = the cursor stays at the split point
 *     (the end of the line above, ^N).
 * notes:
 *     Sets mod = TRUE on success but does NOT set disp_mode - the
 *     callers choose (dinsln for RETURN, dpage for ^N, whose cursor
 *     position the dinsln fast path does not expect).
 * ####################################################################### */
int split_line (int keep_cursor)
{
	char	*line_beg;	/* The line's first character */
	char	*gap_old;	/* gap_beg at scan time */
	char	*p, *q;
	int	indent;
	int	i;
	int	old_y;

	line_beg = start_line (gap_end);
	gap_old = gap_beg;
	p = line_beg;
	indent = 0;
	while (p < txt_end && *p != '\n' && (*p == ' ' || *p == '\t'))
	{
		indent++;
		if (p == gap_old)
		{		/* The cursor char is next */
			p = gap_end;
		}
		else
		{
			p++;
		}
	}

	/* Newline plus indent in one all-or-nothing space check - no
	 * half-inserted split. */
	if (gap_beg + indent + 1 >= gap_end)
	{
		tpa_err ();
		return (0);
	}

	old_y = txt_y;
	*++gap_beg = '\n';

	/* Copy the indent bytes after the newline: same walk, same gap
	 * jump; the destination stays below gap_end (the space is
	 * reserved above). */
	q = line_beg;
	i = indent;
	while (i-- > 0)
	{
		*++gap_beg = *q;
		if (q == gap_old)
		{
			q = gap_end;
		}
		else
		{
			q++;
		}
	}

	/* Trim the whitespace the after-cursor text still carries (the
	 * rest of the run, already in the indent): the new line starts
	 * at exactly the old line's column. Bounded by the scan end -
	 * trailing blanks on ordinary lines are never touched. */
	while (gap_end < p && (*gap_end == ' ' || *gap_end == '\t'))
	{
		gap_end++;
	}

	txt_x++;
	txt_y = column (gap_end);
	mod = TRUE;

	if (keep_cursor)
	{
		/* ^N: the cursor returns to the split point - the end of
		 * the line above (on the inserted newline): the gap
		 * moves back over the indent + the newline (a pure gap
		 * move, the logical text unchanged). */
		i = indent + 1;
		while (i-- > 0)
		{
			*--gap_end = *gap_beg--;
		}
		txt_x--;
		txt_y = old_y;
	}
	return (1);
}
/* #######################################################################
 * void insert_newline (void)
 * purpose:
 *     Insert a line break at the cursor (^N): the text after the
 *     cursor moves to a new line, which is auto-indented like with
 *     RETURN (split_line). The cursor stays at the split point - the
 *     end of the line above. Named insert_newline (not insert_line)
 *     because <term.h> defines an insert_line curses capability
 *     macro.
 * returns:
 *     Nothing.
 * parameters:
 *     None.
 * notes:
 *     Needs room for the newline plus the indent in the gap;
 *     tpa_err() when the gap is exhausted.
 * ####################################################################### */
void insert_newline (void)
{
	if (split_line (1))
	{
		disp_mode = dpage;
	}
}
/* #######################################################################
 * void repaint (void)
 * purpose:
 *     Full screen refresh (^O L, and the KEY_RESIZE handler): clear
 *     the edit window, mark the help state stale, request a full page
 *     repaint and rebuild the status line.
 * returns:
 *     Nothing.
 * parameters:
 *     None.
 * ####################################################################### */
void repaint (void)
{
	status_line ();
	wclear (editwin);
	helpstate = REFRESH;
	disp_mode = dpage;
}
/* #######################################################################
 * void add_text (int ch)
 * purpose:
 *     The default key handler: insert printable text at the cursor
 *     (or overwrite it when insflag is off, saving the overwritten
 *     character for undo). Handles ^P (enter a literal control code),
 *     ^N (new line), TAB expansion to spaces, the word wrap at the
 *     right margin (break at the last space; without MRflag) and the
 *     control-character display rules.
 * returns:
 *     Nothing.
 * parameters:
 *     int ch: the keystroke as delivered by key_scan().
 * notes:
 *     The word wrap shares reform()'s break-at-space logic but only
 *     for the freshly typed character; when no space is found nothing
 *     wraps (nowrap). With no right margin (rt_marg = 0) the wrap
 *     never triggers. Control characters count as one column only
 *     when cntlflag is on - otherwise the cursor does not advance.
 *     A NUL (^P ^@) is rejected: it would break find().
 * ####################################################################### */
void add_text (int ch)
{
	char	*tmp_ptr;

	switch (ch)
	{
	case CNTRL_P:				/* Enter a control char. */
		warning ("^P-");
		ch = (wait_key () & 0x1F);
		if (ch == 0)			/* Reject ^@: a NUL would break find() */
		{
			beep ();
			break;
		}
		if (ch == '\t')
		{
			goto addtab;
		}
		else if (ch != '\n')
		{
			goto addcntl;		/* Add control code or fall through and.. */
		}
		/*FALLTHRU*/
	case CNTRL_N:				/* Insert a new line */
		insert_newline ();
		break;

	case '\t':					/* TAB */
		addtab:
		if (gap_beg + 1 < gap_end)
		{
			*++gap_beg = ch;
			if (!insflag && (gap_end < txt_end) && (*gap_end != '\n'))
			{
				if (!undel_save (gap_end, 1, FORWARD))
				{
					break;
				}
				gap_end++;						/* Overwrite */
			}
			ch = txt_y + tabdef - (txt_y - 1) % tabdef;
			/* Expand TABs on output */
			for ( ; txt_y < ch ; txt_y++)
			{
				out_char (' ');
			}
			disp_mode = dtoeol;
			mod = TRUE;
		}
		else
		{
			tpa_err ();
		}
		break;

	default:					/* Insert printable characters */
		if (isprint (ch))
		addcntl:					/* Sneak a cntl-char in (^P-) */
		{
			if (gap_beg + 2 < gap_end)	/* 1 extra in case of wrap */
			{
				*++gap_beg = ch;
				if (!insflag && (gap_end < txt_end) && (*gap_end != '\n'))
				{
					if (!undel_save (gap_end, 1, FORWARD))
					{
						break;
					}
					gap_end++;						/* Overwrite */
				}
				txt_y++;
				if (!MRflag && rt_marg > 0 && (txt_y > rt_marg))	/* Wordwrap? */
				{
					txt_ptr = gap_beg;
					while ((*txt_ptr != ' ') &&	/* Find 1st SP or CR */
						(*txt_ptr != '\n') &&
						(txt_ptr > txt_beg))
					{
						txt_ptr--;
					}
					if (*txt_ptr == ' ')
					{
						tmp_ptr = gap_beg;
						gap_beg++;					/* Space for NL */
						while (tmp_ptr > txt_ptr)	/* Shift text */
						{
							*(tmp_ptr + 1) = *tmp_ptr;
							tmp_ptr--;
						}
						*(txt_ptr + 1) = '\n';
						txt_x++;
						txt_y = column (gap_end);
						disp_mode = dpage;  /* Redraw screen */
					}
					else
					{
						goto nowrap; 		/* Can't reformat */
					}
				}
				else
				{
					nowrap:
					out_char (ch);
					disp_mode = dtoeol;		/* Redraw rest of line */
				}
				mod = TRUE;
				if (iscntrl (ch))			/* Control chars are special */
				{
					if (cntlflag)			/* Are we showing? */
					{
						disp_mode = dpage;	/* Yes, do page */
					}
					else
					{
						txt_y = column (gap_end);	/* Don't move cursor */
					}
				}
			}
			else
			{
				tpa_err ();	/* Out of space */
			}
		}
		else
		{
			key_err ();		/* Non-printable char. */
		}
		break;
	}
}
/* #######################################################################
 * void del_left (void)
 * purpose:
 *     Delete the character to the LEFT of the cursor (Backspace/Del
 *     keys): grow the gap backwards over it, jumping block markers,
 *     and save the character for undo first.
 * returns:
 *     Nothing.
 * parameters:
 *     None.
 * notes:
 *     Adjusts txt_x/txt_y per deleted character type (newline moves
 *     the cursor up a whole line, TAB recomputes the column, plain
 *     character decrements it) and picks dtoeos/dtoeolhc repaint.
 * ####################################################################### */
void del_left (void)
{
	if (gap_beg > txt_beg)
	{
		if (!undel_save (gap_beg, 1, BACKWARD))
		{
			return;
		}
		mod = TRUE;
		if (blk_beg == gap_beg)
		{
			blk_beg--;
		}
		if (blk_end == gap_beg)
		{
			blk_end--;
		}
		gap_beg--;
		switch (*(gap_beg + 1))		/* Character being removed */
		{
		case '\n':
			txt_x--;
			txt_y = column (gap_end);
			disp_mode = dtoeos;
			break;
		case '\t':
			txt_y = column (gap_end);
			disp_mode = dtoeolhc;
			break;
		default:
			txt_y--;
			disp_mode = dtoeolhc;
			break;
		}
	}
}
/* #######################################################################
 * int text_ins (char *ins_beg, char *ins_end, enum directions direction)
 * purpose:
 *     The one insertion primitive: copy the range [ins_beg, ins_end)
 *     into the gap at the cursor. FORWARD inserts after the cursor
 *     (shrinks the gap from gap_end), BACKWARD before it (grows the
 *     text at gap_beg, used for typed text and undo). Checks the gap
 *     space first - FORWARD keeps one byte spare.
 * returns:
 *     1 on success, 0 when the gap has no room (tpa_err() beeped).
 * parameters:
 *     char *ins_beg: first byte of the text to insert.
 *     char *ins_end: one past the last byte to insert.
 *     enum directions direction: BACKWARD (before cursor) or FORWARD
 *     (after cursor).
 * notes:
 *     BACKWARD mode also updates txt_x for newlines inside the text
 *     and recomputes txt_y. Does NOT refresh the screen - the caller
 *     sets disp_mode.
 * ####################################################################### */
int text_ins (char *ins_beg, char *ins_end, enum directions direction)
{
	char		*ti_ptr;
	unsigned	ins_size;

	ins_size = ins_end - ins_beg;			/* Size to insert */

	if (direction == FORWARD)				/* Insert after cursor */
	{
		if ((long) ins_size > gap_end - gap_beg - 1)	/* Space? */
		{
			tpa_err ();
			return (0);
		}
		memcpy (gap_end - ins_size, ins_beg, ins_size);
		gap_end -= ins_size;
	}
	else									/* Insert before cursor */
	{
		if ((long) ins_size > gap_end - gap_beg - 2)	/* Space? */
		{
			tpa_err ();
			return (0);
		}
		memcpy (gap_beg + 1, ins_beg, ins_size);
		gap_beg += ins_size;
		for (ti_ptr = ins_beg ; ti_ptr < ins_end; ti_ptr++)
		{
			if (*ti_ptr == '\n')
			{
				txt_x++;			/* Adjust vertical */
			}
		}
		txt_y = column (gap_end);	/* Adjust horizontal */
	}
	disp_mode = dpage;						/* Redraw page afterwards */
	return (1);
}
/* -----------------------------------------------
 * Execute shell command(s)
 * NOTES: - Imported (">") text will terminate at main text buffer overflow.
 *        - Imported text lines will be truncated after 1023 characters each.
 */
/* #######################################################################
 * void do_shell (void)
 * purpose:
 *     The ^K ! shell escape: prompt for a command (empty = $SHELL on
 *     UNIX, COMSPEC on DOS), leave curses screen mode, run the command
 *     via system() with STDIN/STDOUT redirected through the terminal
 *     (on UNIX the "> file" import form is read back via popen and
 *     inserted at the cursor, truncated to 1023 bytes per line), then
 *     wait for ENTER and rebuild the editing session windows.
 * returns:
 *     Nothing.
 * parameters:
 *     None.
 * notes:
 *     The windows are destroyed and recreated (like toggle_help) so
 *     the shell has the whole screen. Imported text is bounded by the
 *     remaining gap space.
 * ####################################################################### */
void do_shell (void)
{
	int		result;
	int		tempsize;
	char	commands[256];
#ifdef UNIX
	char	pipebuf[1024];
	FILE	*fileptr;
#endif
	char	*usershell;

#ifdef UNIX
	usershell = getenv ("SHELL");
#else
	usershell = getenv ("COMSPEC");
#endif

	if (usershell == (char *) NULL)
	{
		usershell = "";
	}

	/* Buffer space for user command */
	tempsize = 256 - strlen (usershell) - 1;

	if (tempsize < respsiz)
	{
		prompt_key ("$SHELL variable too long ");
		return;
	}

	wattrset (editwin, STATATTR | A_BOLD);
	wmove (editwin, 1, 0);
	wclrtoeol (editwin);
#ifdef UNIX
	waddstr (editwin, "Hit ENTER for interactive shell, append \">\" for shell input");
#else
	waddstr (editwin, "Hit ENTER for interactive shell");
#endif
	wrefresh (editwin);
	wattrset (editwin, A_NORMAL);
	disp_mode = dpage;

	result = prompt_st ("Shell command: ", "", respptr, respsiz);
	if (result < 0)
	{
		return;		/* Abort */
	}

	/* If $SHELL defined, use it; */
	/*  otherwise, pass command line to "sh": */
	if (strlen (usershell) && strlen (respptr))
	{
		/* Have $SHELL & response; need \"s for shell input, below */
		sprintf (commands, "%s -c \"%s\"", usershell, respptr);
	}
	else if (strlen (usershell))
	{
		/* Have $SHELL, no response */
		strcpy (commands, usershell);
	}
	else if (strlen (respptr))
	{
		/* Have response only */
		sprintf (commands, "%s", respptr);
	}
	else
	{
		/* No $SHELL, no response */
#ifdef UNIX
		strcpy (commands, "/bin/sh");
#else
		strcpy (commands, "command.com");
#endif
	}

#ifdef UNIX
	setuid (getuid ());	/* Set to real uid, gid */
	setgid (getgid ());
#endif

	/* If last character is ">" ... */
#ifdef UNIX
	if (strlen (respptr) > 0 &&
		respptr[strlen (respptr) - 1] == '>')
	{
		commands[strlen (commands) - 2] = '"';	/* Remove ">" */
		commands[strlen (commands) - 1] = '\0';
		if ((fileptr = (FILE *) popen (commands, "r")) != NULL)
		{
			while ((fgets (pipebuf, 1023, fileptr)) != NULL)
			{
				if (text_ins (pipebuf, pipebuf + strlen (pipebuf), BACKWARD) == 0)
				{
					pclose (fileptr);
					return;
				}
				else
				{
					mod = 1;
				}
			}
		}
		else
		{
			prompt_key ("popen() error ");
		}
	}
	else
#endif
	{
		endwin ();

		fputs ("\n\n", stderr);
		if (strlen (respptr) == 0)
		{
			fputs (" Type \"exit\" to return to TextStar.\n\n", stderr);
		}

		system (commands);	/* Shell out */

#ifdef UNIX
		setuid (geteuid ());	/* Restore effective uid, gid */
		setgid (getegid ());
#endif

		if (strlen (respptr))
		{
			fputs (" Press ENTER to return to TextStar: ", stderr);
			fgets (commands, 255, stdin);
		}
	}
}
