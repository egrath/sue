#include "sue.h"
#include "version.h"

/* -------------------------------------------
 * Saving & Block help
 */
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
/* -----------------------------------------
 * Quick command help
 */
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
/* ----------------------------------------------------------
 * Onscreen command help
 */
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

#ifdef LOOKBUSY
/*--------------------------------------------------------------
 * Drivel
 */
void look_busy (void)
{
	wclear (editwin);
	wp1 ("and the annual outlook for such a plan seems reasonably promising. \n");
	wp1 ("On the other hand, we will have to anticipate further adjustments \n");
	wp1 ("to the entire project if current trends continue for the rest of \n");
	wp1 ("the fiscal year.  What must be taken into account is the continual \n");
	wp1 ("fluidity of the marketplace despite predictions of a coming period \n");
	wp1 ("of stabilization.  While the firm has in the past dealt \n");
	wp1 ("effectively with market forces beyond its control, in the future \n");
	wp1 ("further attention must be paid to the possible side effects of \n");
	wp1 ("these forces.\n\n");
	wp1 ("Another aspect of the problem that must be taken into \n");
	wp1 ("consideration is the continual growth in significance of nonmarket ");
	wrefresh (editwin);
	wait_key ();				/* Get key */
	disp_mode = dpage;		/* Repaint text */
}
#endif
/* ----------------------------------------------------------
 * Find text command.
 * Case-independent, forward text search.
 */
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
/* -----------------------------------------------------------
 * Change text command.
 * Case-independent, forward text search.
 */
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
/* --------------------------------------------------
 * Quit command
 */
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
/* ---------------------------------------------------
 * Save file & continue command
 */
void save_func (void)
{
	if (save_fname ())
	{
		mod = FALSE;
	}
}
/* -----------------------------------------------
 *	Save/quit command
 */
void exit_func (void)
{
	if (save_fname ())
	{
		edit_exit (0);
	}
}
/* -----------------------------------------------------
 * Info command (current status of edit session)
 */
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
	status (0, "SUE version %s Copyright 1995 Point to Point", suerev, NULL);
	stat_print (COLS - 23, "Author: carson@ppci.com", NULL, NULL);
	wmove (editwin, 0, 0);
	wp4 ("           Editing: `%s` at line `%u` column `%u`\n", txtname, txt_x, txt_y);		/* show full name */
	wp2 ("          Modified: `%s`\n", mod ? "Yes" : "No");
	wp2 ("         File mode: `%s`\n", modestr);
	wp2 ("      Right margin: `%d`\n", rt_marg);
	wp2 ("    Hard tab width: `%d`\n", tabdef);
	wp2 ("Control characters: `%s`\n", cntlflag ? "On" : "Off");
	wp3 ("      Memory usage: `%u` bytes `%u` free\n",
	     gap_beg - txt_beg + txt_end - gap_end, gap_end - gap_beg - 1);
	wp2 ("         Baud rate: `%d`\n", baudrate ());
	wp2 ("     Terminal type: `%s`\n", longname ());
	wattrset (editwin, PROMPTATTR);
	wp1 ("    Strike any key: ");
	wattrset (editwin, A_NORMAL);
	wrefresh (editwin);
	wait_key ();					/* Get key */
	status_line ();
	disp_mode = dpage;			/* Repaint text */
}
/* ------------------------------------------------
 * Erase character under cursor
 */
int del_right (void)
{
	if (gap_end < txt_end)		/* Can't delete past end of file */
	{
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
/*----------------------------------------------------
 * Select or change name of text
 */
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
/* ------------------------------------------------
 * Save text to file, prompting for name if needed
 */
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

	ioresult = save (txtname, "w");	/* Save "block" */

	blk_beg = old_beg;				/* Restore block pointers */
	blk_end = old_end;

	return ioresult;
}
/*------------------------------------------------
 * Global search for a string
 */
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
/* -----------------------------------------------------------
 * Replace the text under the cursor with the null-terminated 
 * string pointed to by the global pointer rptr.  'repkey' is 
 * a global variable which holds the user's last response to the
 * "Change (Y / N / *)?" prompt.  If repkey == '*' then the prompt 
 * is skipped and the string replaced without asking.
 */
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
/* --------------------------------------------------
 * Position cursor to arbitrary line
 */
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
/* -----------------------------------------------
 * Set hard tab display
 */
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
/* -------------------------------
 * Set right margin
 */
void rm_set (void)
{
	int c;
	char pbuf[40];

	sprintf (pbuf, "Right margin [%d]: ", rt_marg);
	if (prompt_st (pbuf, "", respptr, 3) > 0)
	{
		if ((sscanf (respptr, "%d", &c) == 1) && (c > 1))
		{
			rt_marg = c;
		}
		else
		{
			val_err ();
		}
	}
}
/* -------------------------------------------------------
 * Reform paragraph from cursor to next Hard CR or EOF.
 * Cursor gap is moved to position after last reformed line.
 */
void reform (void)
{
	char	*tmp_ptr;
	long	col_cnt;

	if (MRflag)				/* Margin released */
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
/* ------------------------------------------------
 * Exit gracefully on receipt of Unix SIGNALs
 */
void sabort (int sig)
{
	char *homeptr;
	char savname[TXTNAME_SIZE];
	int n;
	FILE *probe;

	endwin ();
	printf ("\n");

	if (signal (sig, SIG_IGN) == SIG_ERR)
	{
		perror ("SUE signal");
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
			sprintf (savname, "%s/SUEsaved.%ld", homeptr, (long) getpid ());
		}
		else
		{
			sprintf (savname, "%s/SUEsaved.%ld.%d", homeptr,
				(long) getpid (), n);
		}
		if ((probe = fopen (savname, "r")) == (FILE *) NULL)
		{
			break;		/* Name not taken */
		}
		fclose (probe);
	}

	strcpy (txtname, savname);	/* Make it the new filename */

	fprintf (stderr, "SUE: ");	/* Display signal received */
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
		fprintf (stderr, "\nSUE: work saved to \"%s\".\n", txtname);
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

/* ----------------------------------------
 * Write block to specified file
*/
void writeblk (void)
{
	int	c;

	if (prompt_st ("Write block to: ", "", respptr, respsiz) > 0)
	{
		if (blk_beg == (char *) NULL)	/* no block specified */
		{
			return;
		}

		if (fopen (respptr, "r") == (FILE *) NULL)	/* Exists? */
		{
			save (respptr, "w");						/* No */
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
				save (respptr, "a"); /* Append block to file */
				break;
			case 'O':				/* Overwrite file */
				save (respptr, "w");
			}
		}
	}
}
/* ------------------------------------------------
 *	Insert file at cursor
 */
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
/* ---------------------------------------------------
 * Edit new file.
 */
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
/* -----------------------------------------------------------
 *	Save the block defined by the block markers,
 *	  jumping the gap if necessary.
 */
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

	status (0, "SUE writing \"%s\"...", s_name, NULL);

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
/* -------------------------------------------------------
 * Load the named file into memory starting at gap_beg,
 *   checking for overflow.  Both new files and block loads may 
 *   be performed with this routine.
 * Set filemode if this is first load this session & mode option not used.
 * Return number of new lines loaded in text.
 */
int load (char *l_name)
{
	FILE	*l_file;
	int	c;
	int	j;
	enum filemodes mode = MODE_UNIX;
	char *old_beg;

	if ((l_file = fopen (l_name, "r")) == (FILE *) NULL)
	{
		return (0);
	}

	status (0, "SUE loading \"%s\"...", l_name, NULL);
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
/* -------------------------------------------------------
 * Set filemode
 */
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
/* -------------------------------------------------------
 * Toggle margin release
 */
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
/* -------------------------------------------------------
 * Related block functions
 */
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
/* -------------------------------------------------------
 * Toggle help mode
 */
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
	editlines = LINES - helplines - 1;
	scr_xend = editlines;
	/* Set up new windows: */
	editwin   = newwin (editlines, COLS, helplines + 1, 0);	/* Next 2 overlap */
	promptwin = newwin (1, COLS, helplines + 1, 0);
	wattrset (promptwin, A_BOLD);
	keypad (editwin, TRUE);		/* Interpret extended keycodes */
	keypad (promptwin, TRUE);
	prompt (1, "", NULL, NULL, NULL);			/* Avoid autoclear */
	status_line ();				/* Clear expert help message */
	disp_mode = dpage;
}
/* -------------------------------------------------------
 * Repeat last find/replace.
 */
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
/* -------------------------------------------------------
 * Invert case of character at cursor.
 */
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
/* -------------------------------------------------------
 * Delete word right
 */
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
/* -------------------------------------------------------
 * Delete cursor line.
 */
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
/* -------------------------------------------------------
 * Insert line after cursor
 */
void insert_newline (void)
{
	if (gap_beg + 1 < gap_end)
	{
		*--gap_end = '\n';
		disp_mode = dpage;
		mod = TRUE;
	}
	else
	{
		tpa_err ();
	}
}
/* -------------------------------------------------------
 * Repaint screen.
 */
void repaint (void)
{
	status_line ();
	wclear (editwin);
	helpstate = REFRESH;
	disp_mode = dpage;
}
/* -------------------------------------------------------
 * Add text at cursor.
 */
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
				if (!MRflag && (txt_y > rt_marg))	/* Need to wordwrap? */
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
/* -------------------------------------------------------
 * Delete character to left of cursor
 */
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
/* ----------------------------------------------------------
 * Insert text from ins_beg to ins_end at cursor.
 * Returns 0 if out of TPA, else 1.
 * Note: does NOT refresh screen.
 */
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
			fputs (" Type \"exit\" to return to SUE.\n\n", stderr);
		}

		system (commands);	/* Shell out */

#ifdef UNIX
		setuid (geteuid ());	/* Restore effective uid, gid */
		setgid (getegid ());
#endif

		if (strlen (respptr))
		{
			fputs (" Press ENTER to return to SUE: ", stderr);
			fgets (commands, 255, stdin);
		}
	}
}
