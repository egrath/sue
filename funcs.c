/*
Project:	SUE, a Simple Unix Editor
Copyright:	SUE is copyright 1992 by Point to Point Communications.
File:		funcs.c
Routines:	User functions & file i/o.
Author:		Carson Wilson
				Apt. 1D
				1359 W. Greenleaf, Chicago, IL 60626
				internet:	carson@mcs.com
Notes:		Edit with hard tab stops set to 4.
*/

static	char	rcsid[] = "@(#)$Id: funcs.c,v 1.3 1995/03/09 13:19:58 carson Exp $";

#include "sue.h"
#include "version.h"

/* -------------------------------------------
 * Saving & Block help
 */
void blockhlp()
{
	enum	helptype	htype;

	if (restrict)
		htype = RBLOCK;
	else
		htype = BLOCK;

	if (helplines)
		dohelp(htype, helpwin);		/* Show it in separate window */
	else
	{	dohelp(htype, editwin);		/* Show in edit window */
		wattrset(editwin, PROMPTATTR);
		wp1("Strike any key: ");
		wattrset(editwin, A_NORMAL);
		wrefresh(editwin);
		waitkey();
	}
}
/* -----------------------------------------
 * Quick command help
 */
void quickhlp()
{
	if (helplines)
		dohelp(QUICK, helpwin);
	else
	{	dohelp(QUICK, editwin);		/* Show in edit window */
		wattrset(editwin, PROMPTATTR);
		wp1("Strike any key: ");
		wattrset(editwin, A_NORMAL);
		wrefresh(editwin);
		waitkey();
	}
}
/* ----------------------------------------------------------
 * Onscreen command help
 */
void onscrhlp()
{
	if (helplines)
		dohelp(ONSCREEN, helpwin);
	else
	{	dohelp(ONSCREEN, editwin);		/* Show in edit window */
		wattrset(editwin, PROMPTATTR);
		wp1("Strike any key: ");
		wattrset(editwin, A_NORMAL);
		wrefresh(editwin);
		waitkey();
	}
}

#ifdef LOOKBUSY
/*--------------------------------------------------------------
 * Drivel
 */
void lookbusy()
{
	wclear(editwin);
wp1("and the annual outlook for such a plan seems reasonably promising. \n");
wp1("On the other hand, we will have to anticipate further adjustments \n");
wp1("to the entire project if current trends continue for the rest of \n");
wp1("the fiscal year.  What must be taken into account is the continual \n");
wp1("fluidity of the marketplace despite predictions of a coming period \n");
wp1("of stabilization.  While the firm has in the past dealt \n");
wp1("effectively with market forces beyond its control, in the future \n");
wp1("further attention must be paid to the possible side effects of \n");
wp1("these forces.\n\n");
wp1("Another aspect of the problem that must be taken into \n");
wp1("consideration is the continual growth in significance of nonmarket ");
	wrefresh(editwin);
	waitkey();				/* Get key */
	disp_mode = dpage;		/* Repaint text */
}
#endif
/* ----------------------------------------------------------
 * Find text command.
 * Case-independent, forward text search.
 */
int findfunc()
{
	char	tfindbuf[60];

	strcpy(tfindbuf, sptr);
	if (promptst("Find text: ", "", sptr, s_strsiz) > 0)
	{	repflag = FALSE;
		txt_ptr = gap_end;
		if (find() == NULL)
		{	notfound();
			return (0);
		}
		return (1);			/* Found */
	}
	else					/* Restore previous string, if any */
		strcpy(sptr, tfindbuf);

	return (0);				/* Null response */
}
/* -----------------------------------------------------------
 * Change text command.
 * Case-independent, forward text search.
 */
void replfunc()
{
	if (findfunc())
	{	repflag = TRUE;
		repkey = 0;
		if (promptst("Change to: ", "", rptr, r_strsiz) < 0)
			return;
		while (find())
			if (replace() == 0)
				return;
	}
}
/* --------------------------------------------------
 * Quit command
 */
void quitfunc()
{
	if (mod)					/* Ask first if file changed */
		if (abandon() != 'Y')
			return;
	edit_exit(0);
}
/* ---------------------------------------------------
 * Save file & continue command
 */
void savefunc()
{
	if (save_fname())
		mod = FALSE;
}
/* -----------------------------------------------
 *	Save/quit command
 */
void exitfunc()
{
	if (save_fname())
		edit_exit(0);
}
/* -----------------------------------------------------
 * Info command (current status of edit session)
 */
void infofunc()
{
	char	modestr[20];

	switch (filemode) {
	case UNIX:
		strcpy(modestr, "Unix");
		break;
	case MSDOS:
		strcpy(modestr, "MSDOS");
		break;
	case MACINTOSH:
		strcpy(modestr, "Macintosh");
		break;
	default :
		strcpy(modestr, "??????");
		break;
	}
	status(0, "SUE version %s Copyright 1995 Point to Point", suerev);
	statprint(COLS - 23, "Author: carson@ppci.com");
	wmove(editwin, 0,0);
	wp4("           Editing: `%s` at line `%u` column `%u`\n", txtname, txt_x, txt_y);		/* show full name */
	wp2("          Modified: `%s`\n", mod ? "Yes" : "No");
	wp2("            Access: `%s`\n",
	      (restrict) ? "Restricted" : "Unrestricted");
	wp2("         File mode: `%s`\n", modestr);
	wp2("      Right margin: `%d`\n", rt_marg);
	wp2("    Hard tab width: `%d`\n", tabdef);
	wp2("Control characters: `%s`\n", cntlflag ? "On" : "Off");
	wp3("      Memory usage: `%u` bytes `%u` free\n", 
	     gap_beg - txt_beg + txt_end - gap_end, gap_end - gap_beg - 1);
	wp2("         Baud rate: `%d`\n", baudrate());
	wp2("     Terminal type: `%s`\n", longname());
	wattrset(editwin, PROMPTATTR);
	wp1("    Strike any key: ");
	wattrset(editwin, A_NORMAL);
	wrefresh(editwin);
	waitkey();					/* Get key */
	status_line();
	disp_mode = dpage;			/* Repaint text */
}
/* ------------------------------------------------
 * Erase character under cursor
 */
int delright()
{
	if (gap_end < txt_end)		/* Can't delete past end of file */
	{	if (*gap_end == '\n')
			disp_mode = dtoeos;
		else
			disp_mode = dtoeolhc;
		if (blk_beg == gap_end)
			blk_beg++;
		if (blk_end == gap_end)
			blk_end++;
		gap_end++;
		mod = TRUE;
		return (1);
	}
	else return (0);
}
/*----------------------------------------------------
 * Select or change name of text
 */
int name()
{
	char *tokptr;

	/* Prompt and take first token: */
	if ((promptst("Name file: ", "", respptr, respsiz) > 0) &&
		((tokptr = strtok(respptr, " \t")) != NULLCHARPTR))
		{	strcpy(txtname, tokptr);
			status_line();
			return (1);
		}
	else return (0);	/* Null response */
}
/* ------------------------------------------------
 * Save text to file, prompting for name if needed
 */
int save_fname()
{
	int ioresult;
	char *old_beg, *old_end;

	if (strlen(txtname) == 0)		/* Have filename? */
		if (!name()) return (0);	/* Must have name! */

	/* Set up phony "block" for save() function */

	old_beg = blk_beg;				/* Save block pointers */
	old_end = blk_end;

	if (gap_beg != txt_beg)
		blk_beg = txt_beg + 1;
	else
		blk_beg = gap_end;

	if (gap_end != txt_end)
		blk_end = txt_end - 1;
	else
		blk_end = gap_beg;

	ioresult = save(txtname, "w");	/* Save "block" */

	blk_beg = old_beg;				/* Restore block pointers */
	blk_end = old_end;

	return ioresult;
}
/*------------------------------------------------
 * Global search for a string
 */
int find()
{
	char fbuf[70], *fbufptr;
	int	s_strlen;

	fbufptr = &fbuf[0];

	s_strlen = strlen(sptr);

	while (txt_ptr <= txt_end)
	{	if (toupper(*txt_ptr) == toupper(*sptr)) /* 1st chars match */
		{	/* Copy user's text to work buffer,		*/
			/*  length of search string or null,	*/
			/*  whichever comes first: 				*/
			strncpy(fbufptr, txt_ptr, s_strlen);
			/* Add our own terminating null: */
			fbuf[s_strlen] = '\0';
			/* Case-insensitive search: */
			strtoup(fbufptr);
			strtoup(sptr);

			if (strcmp(fbufptr, sptr) == 0)
			/* match */
			{	move_ptr();
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
 * "Change (Y/N/*)?" prompt.  If repkey == '*' then the prompt 
 * is skipped and the string replaced without asking.
 */
int replace()
{
	int strdif, counter;
	char *tmpptr;

	strdif = strlen(rptr) - strlen(sptr);
	if (gap_beg + strdif > gap_end)
	{	TPAErr();
		return (0);
	}
	getrepkey:
	if (repkey != '*')
	{	wattron(statuswin, A_BOLD);
		status(0, "Change (Y/N/*)? ");
		statprint(COLS-18, "F1 or ^U to abort");
		wattroff(statuswin, A_BOLD);
		disp_mode = dpages;
		display();
		txt_ptr = gap_end;	/* restore txt_ptr */
		wrefresh(editwin);	/* show cursor location */
	}
	if (repkey != '*')
	{	repkey = toupper(waitkey());
		status_line();
	}
	switch (repkey)	{
	case 'N':				/* Skip forwards past instance */
	case '\r':
		for (counter = strlen(sptr) ;
		     (counter > 0) && (txt_ptr < txt_end) ; counter--)
			txt_ptr++;
		move_ptr();			/* Restore cursor location & block markers */
		break;

	case '*':				/* Perform replace operation */
	case 'Y':
		mod = TRUE;
		for (counter = strlen(sptr) ;
			counter > 0 ; counter--)	/* Remove old string, */
			delright();		  			/* Preserve block markers */
		disp_mode = dpage;				/* Restore from delright() */
		gap_beg++;
		strcpy(gap_beg, rptr);
		gap_beg += strlen(rptr) - 1;
		txt_ptr = gap_end;
		/* Keep track of line numbers: */
		tmpptr = rptr;
		while (tmpptr = (char *) strchr(tmpptr, '\n'))	/* Count newlines */
		{	tmpptr++;
			txt_x++;
		}
		txt_y = column(gap_end);	/* Set cursor column */
		break;

	case ESC:			/* Abort replace operation */
	case KEY_F(1):
	case CNTRL_U:
		return (0);

	default:			/* Complain if user hits invalid key */
		beep();
		goto getrepkey;
	}
	return (1);			/* Report success */
}
/* --------------------------------------------------
 * Position cursor to arbitrary line
 */
void gotoline()
{
	int line_no;

	if (promptst("Go to line [%d]: ", txt_x, respptr, 5) > 0)
	{	if (sscanf(respptr, "%d", &line_no) == 1)
		{	/* Move to line number given */
			txt_x = move_x(line_no);
			disp_mode = dpages;
		}
		else
			valerr();	/* Value error */
	}
}
/* -----------------------------------------------
 * Set hard tab display
 */
void tabset()
{
	int c;

	if (promptst("Tab width [%d]: ", tabdef, respptr, 2) > 0)
	{	if ((sscanf(respptr, "%d", &c) == 1) && (c > 1))
		{	tabdef = c;
			txt_y = column(gap_end);
			disp_mode = dpage;
		}
		else
			valerr();
	}
}
/* -------------------------------
 * Set right margin
 */
void rmset()
{
	int c;

	if (promptst("Right margin [%d]: ", rt_marg, respptr, 3) > 0)
	{	if ((sscanf(respptr, "%d", &c) == 1) && (c > 1))
			rt_marg = c;
		else
			valerr();
	}
}
/* -------------------------------------------------------
 * Reform paragraph from cursor to next Hard CR or EOF.
 * Cursor gap is moved to position after last reformed line.
 */
void	reform()
{
	char	*tmp_ptr;
	unsigned	col_cnt;

	if (MRflag)				/* Margin released */
		return;
	disp_mode = dpage;		/* Always redraw screen */
	mod = TRUE;
	txt_ptr = start_line(gap_end);
	/* Process lines until hard CR or EOF reached */
	while (txt_ptr < txt_end)
	{	col_cnt = 1;
		txt_ptr = start_line(gap_end);
		text_move();
		txt_ptr = gap_end;
		while ((txt_ptr < txt_end) && (col_cnt <= rt_marg))
		/* Process characters until right margin reached */
		{	switch (*txt_ptr) {
			case '\n':						/* Join lines */
				text_move();
				/* If hard CR, move cursor & quit */
				if ((*gap_beg != ' ') || (*(gap_end + 1) == '\n'))
				{	txt_ptr++;
					text_move();
					txt_x++;
					txt_y = column(gap_end);
					return;
				}
				/* Else eat whitespace, remove newline, & continue */
				do
				{	txt_ptr = gap_end;
					if (blk_beg == gap_end)		/* Preserve blocks */
						blk_beg++;
					if (blk_end == gap_end)
						blk_end++;
					gap_end++;					/* Remove newline */
				}
				while (((*gap_end == ' ') || (*gap_end == '\t')) && 
						(gap_end < txt_end));
				break;

			case '\t':						/* Count hard TAB */
				col_cnt = col_cnt - (col_cnt - 1) % tabdef + tabdef;
				break;

			default:
				if (isprint(*txt_ptr))		/* ^chars. don't count */
					col_cnt++;			
			}
			txt_ptr++;
		}
		/* Right margin reached; break line */
		text_move();					/* Move gap */
		if (txt_ptr == txt_end)			/* Stop if EOF */
		{	txt_y = column(gap_end);
			return;
		}
		txt_ptr = gap_beg;
		while  ((*txt_ptr != ' ') &&	/* Find prev. SPACE */
				(*txt_ptr != '\n') &&	/* or error 		*/
				(txt_ptr > txt_beg))	/*   conditions 	*/
		{	txt_ptr--;
			col_cnt--;
		}
		if (*txt_ptr != ' ')			/* Error */
		{	txt_y = column(gap_end);
			beep();
			promptkey("Can't reformat! ");
			return;
		}
		/* Break line if necessary */
		else if ((col_cnt == rt_marg + 1) && /* A "perfect fit!" */
				(*gap_end == '\n'))
		{	if (blk_beg == gap_end)		/* Preserve blocks */
				blk_beg++;
			if (blk_end == gap_end)
				blk_end++;
			*++gap_beg = *gap_end++;	/* Goto next line */
				txt_ptr = gap_end;
		}
		else							/* Break line */
		{	if (gap_beg + 1 >= gap_end)
			{	TPAErr();
				txt_y = column(gap_end);
				return;
			}
			tmp_ptr = gap_beg;
			while (tmp_ptr >= txt_ptr)  /* First shift text */
			{	*(tmp_ptr + 1) = *tmp_ptr;
				tmp_ptr--;
			}
			gap_beg++;					/* Space for newline */
			*(txt_ptr + 1) = '\n';		/* Break line */
			txt_ptr += 2;
		}
		text_move();					/* Move gap */
		txt_x++;
		txt_y = column(gap_end);
	}
}
/* ------------------------------------------------
 * Exit gracefully on receipt of Unix SIGNALs
 */
void sabort(sig)
int	sig;
{
	char *homeptr, *savname;

	endwin();
	printf("\n");

	if (signal(sig, SIG_IGN) == SIG_ERR)
		perror("SUE signal");		

	/* Look for user's home directory */
	if ((homeptr = getenv("HOME")) == (char *) NULL)
		homeptr = "/tmp";	/* not good, better than nothing */
	
	/* Try to build unique filename */
	if ((savname = (char *) tempnam(homeptr, "SUE")) == (char *) NULL)
		savname = strcat(homeptr, "/SUEsaved");

	strcpy(txtname, savname);	/* Make it the new filename */

	fprintf(stderr, "SUE: ");	/* Display signal received */
	switch (sig) {
	case SIGHUP:
		fprintf(stderr, "Hangup");
		break;
	case SIGINT:
		fprintf(stderr, "Interrupt");
		break;
	case SIGQUIT:
		fprintf(stderr, "Quit");
		break;
	case SIGTERM:
		fprintf(stderr, "Terminate");
		break;
	case SIGSEGV:
		fprintf(stderr, "Segment violation");
		break;
	case SIGBUS:
		fprintf(stderr, "Bus error");
		break;
	case SIGILL:
		fprintf(stderr, "Illegal instruction");
		break;
	case SIGSYS:
		fprintf(stderr, "Bad system call");
		break;
	}
	fprintf(stderr, " signal received, attempting to save your work...");

	if (sig == SIGSYS || sig == SIGILL || sig == SIGBUS ||
		sig == SIGSEGV)
	/* Do a little sanity checking before attempting save */
		if ((gap_beg > txt_end) || (gap_beg < txt_beg) ||
			(gap_end > txt_end) || (gap_end < txt_beg))
			gap_beg = gap_end = (txt_beg + (txt_end - txt_beg) / 2);

	if (save_fname())
		fprintf(stderr, "\nSUE: work saved to \"%s\".\n", txtname);
	/* Note: 'vi' uses /usr/preserve/[username] directory & sends mail */

	if (sig == SIGSYS || sig == SIGILL || sig == SIGBUS ||
		sig == SIGSEGV)
		kill(0, sig);	/* try for core image too */
	exit(1);
}

/********************** FILE I/O ROUTINES *********************/

/* ----------------------------------------
 * Write block to specified file
*/
void writeblk()
{
	int	c;

	if (promptst("Write block to: ", "", respptr, respsiz) > 0)
	{	if (blk_beg == (char *)NULL)	/* no block specified */
			return;

		if (fopen(respptr, "r") == (FILE *) NULL)	/* Exists? */
			save(respptr, "w");						/* No */
		else
		{	c = toupper(promptkey(
				"File exists: O(verwrite)/A(ppend)/Q(uit)? Q\b"));
			if (c == 'A' || c == 'O')
				wechochar(promptwin, c);
			switch (c) {
			case 'A':
				save(respptr, "a"); /* Append block to file */
				break;
			case 'O':				/* Overwrite file */
				save(respptr, "w");
			}
   		}
	}
}
/* ------------------------------------------------
 *	Insert file at cursor
 */
void readblk()
{
	int inlines;
	char *tmp_ptr;

	if (promptst("Read block from: ", "", respptr, respsiz) > 0)
	{	tmp_ptr = gap_beg + 1;
		inlines = load(respptr);
		if (inlines == -1)
			return;					/* File was too large */
		if (inlines)				/* File found */
		{
			txt_ptr = tmp_ptr;
			text_move();
			mod = TRUE;
			disp_mode = dpage;
		}
		else						/* No file */
		{	beep();
			promptkey("File not found ");
		}
	}
}
/* ---------------------------------------------------
 * Edit new file.
 */
void newfile()
{
	if (mod)
		if (abandon() != 'Y') return;
	if (promptst("Load file: ", "", respptr, respsiz) > 0)
	{	edit_init(response);
/*		disp_mode = dpage;*/
	}
}
/* -----------------------------------------------------------
 *	Save the block defined by the block markers,
 *	  jumping the gap if necessary.
 */
int	save(s_name, s_type)
char	*s_name, *s_type;
{
	FILE	*s_file;
	char	*ptr;

	/* This should never happen: */
	if (strlen(s_name) == 0)
	{	beep();
		promptkey("No name specified ");
		return(0);
	}

	status(0,"SUE writing \"%s\"...", s_name);

	s_file = fopen(s_name, s_type);
	if (s_file == (FILE *)NULL)
	{	beep();
		promptkey("Can't write to file! ");
		status_line();	/* restore */
		return(0);
	}
	ptr = blk_beg;
	while (ptr <= blk_end)
	{	if (*ptr == '\n')				/* May have to xlate */
		{	if (filemode != UNIX)
				fputc('\r', s_file);
			if (filemode != MACINTOSH)	/* \r for MACINTOSH,			*/
				fputc('\n', s_file);	/* \n for Unix, \r\n for MSDOS	*/
		}
		else
			fputc(*ptr, s_file);
		if (ptr == gap_beg)
			ptr = gap_end;
		else
			ptr++;
	}
	fclose(s_file);
	status_line();	/* Restore */
	return(1);
}
/* -------------------------------------------------------
 * Load the named file into memory starting at gap_beg,
 *   checking for overflow.  Both new files and block loads may 
 *   be performed with this routine.
 * Set filemode if this is first load this session & mode option not used.
 * Return number of new lines loaded in text.
 */
int	load(l_name)
char	*l_name;
{
	FILE	*l_file;
	int	c;
	int	j;
	enum filemodes mode = UNIX;
	char *old_beg;

	if ((l_file = fopen(l_name, "r")) == (FILE *)NULL)
		return(0);

	status(0,"SUE loading \"%s\"...", l_name);
	j = 1;
	old_beg = gap_beg;
	while ((c = fgetc(l_file)) != EOF)
		if (gap_beg + 1 >= gap_end)
		{	beep();
			promptkey("File too big ");
			gap_beg = old_beg;
			fclose(l_file);
			status_line();
			return(-1);
		}
		else
		{	c &= 0x7F;	/* strip high bits */
			if (c)		/* Remove NULLs */
			{	*(gap_beg + 1) = c;		/* doalloc() sets initial gap_beg to \n */
				if (*gap_beg == '\r')	/* Previous was \r */
				{	*gap_beg = '\n';	/* Xlate \r to \n and \r\n to \n\n */
					if (c == '\n')		/* Hit a \r\n */
					{	mode = MSDOS;
						gap_beg--;		/* Avoid \n\n */
					}
					else				/* Hit text\rtext */
						mode = MACINTOSH;
				}
				if (*gap_beg == '\n')
					j++;
				gap_beg++;
			}
		}
	if (*gap_beg == '\r')
		(*gap_beg = '\n');
	if (autofilemode)			/* Options override		*/
		filemode = mode;		/* ..else autodetect	*/
	else
		autofilemode = 1;		/* Only masked 1st time */
	fclose(l_file);
	status_line();
	return(j);
}
/* -------------------------------------------------------
 * Set filemode
 */
void	setfilemode()
{
	int	keyin;

	keyin = toupper(promptkey("File mode: Unix, msDos, or Macintosh (U/D/M)? "));
	switch (keyin) {
	case 'U' :
		filemode = UNIX;
		autofilemode = 0;
		break;
	case 'D' :
		filemode = MSDOS;
		autofilemode = 0;
		break;
	case 'M' :
		filemode = MACINTOSH;
		autofilemode = 0;
		break;
	case ' ' :
		break;
	default :
		beep();
		break;
	}
}
/* -------------------------------------------------------
 * Toggle margin release
 */
void	marrel()
{
	MRflag = !MRflag;
	if (MRflag)
		statprint(COLS-27, "MR");
	else
		statprint(COLS-27, "  ");
}
/* -------------------------------------------------------
 * Related block functions
 */
void	blockfuncs(funcno)
int		funcno;
{
	char	*old_ptr;

	switch (funcno) {
	case 1:						/* Begin block */
		if (blk_end > gap_end)	/* Block ends at/after cursor */
			blk_beg = gap_end;
		else
			blk_beg = blk_end = gap_end;
		disp_mode = dpage;
		break;

	case 2:				/* End block */
		blk_end = gap_beg;
		if ((blk_beg == (char *)NULL) || (gap_beg <= blk_beg))
			blk_beg = blk_end;
		disp_mode = dpage;
		break;

	case 3:				/* Clear block markers */
		blk_beg = blk_end = NULL;
		disp_mode = dpage;
		break;

	case 4:			/* Move (copy then delete) */
	case 5:			/* Copy to in front of cursor */
		if (blkerr(2))				/* In or not marked */
			break;
		tblk_end = gap_end - 1;		/* Prepare to move block end (case 4) */
		if (!textins(blk_beg, blk_end+1, FORWARD))
			break;					/* TPA error */
		tblk_beg = gap_end;			/* Prepare to move block begin (case 4) */
		mod = TRUE;
		disp_mode = dpage;
		if (funcno == 5)	/* Quit for "copy block" */
			break;
		/*FALLTHRU*/
	case 6:							/* Delete block */
		if (blkerr(1))				/* Not marked */
			break;
		if (blk_beg > gap_end)		/* Preserve cursor location */
			old_ptr = gap_beg+1;
		else
			old_ptr  = gap_end;

		if (blk_end != gap_beg)		/* Move cursor to block end */
		{	txt_ptr  = blk_end + 1;
			txt_x   += text_move();
		}
		if (funcno == 6)			/* If we're erasing.. */
			if (!undelsave(blk_beg, gap_beg - blk_beg + 1, FORWARD))
				goto NoBlockDel;	/* Didn't save, so don't delete */
		/* Expand gap backward to block begin */
		for ( ; gap_beg >= blk_beg ; gap_beg--)
			if (*gap_beg == '\n')
				txt_x--;
		if (funcno == 6)
			blk_beg = blk_end = NULL;
		else
		{	blk_beg = tblk_beg;		/* Restore markers */
			blk_end = tblk_end;
		}
		mod = TRUE;
		disp_mode = dpage;
		NoBlockDel:
		txt_ptr = old_ptr;			/* Restore cursor */
		txt_x  += text_move();
		txt_y   = column(gap_end);
		break;
	}
}
/* -------------------------------------------------------
 * Toggle help mode
 */
void	togglehelp()
{
	if (helplines)
	{	helplines = 0;
		delwin(helpwin);
	}
	else
	{	if (cmd_mode == CKey)	
			helplines = 8;
		else
			helplines = 3;
	}
	delwin(editwin);			/* Kill current curses windows */
	delwin(promptwin);
	editlines = LINES - helplines - 1;
	scr_xend = editlines;	
	/* Set up new windows: */
	editwin   = newwin(editlines, COLS, helplines+1, 0);	/* Next 2 overlap */
	promptwin = newwin(1, COLS, helplines+1, 0);
	wattrset(promptwin, A_BOLD);
	keypad(editwin, TRUE);		/* Interpret extended keycodes */
	keypad(promptwin, TRUE);
	if (helplines)
	{	helpwin = newwin(helplines, COLS, 1, 0); /* Novice help */
		helpstate = REFRESH;		/* Tell display() to refresh */
	}
	prompt(1,"");				/* Avoid autoclear */
	status_line();				/* Clear expert help message */
	disp_mode = dpage;
}
/* -------------------------------------------------------
 * Repeat last find/replace.
 */
void	repfind()
{
	if (strlen(s_str) != NULL)
	{	repkey = 0;
		if (repflag)			/* if we are replacing */
			txt_ptr = gap_end;	/* start at cursor location */
		else
			txt_ptr = gap_end + 1;	/* if finding start after cursor */
		if (find())
		{	if (repflag)		/* if replacing */
			{	replace();
				/* user said global? */
				if (repkey == '*')
					while (find())
						replace();
			}
		}
		else notfound();
	}
}
/* -------------------------------------------------------
 * Invert case of character at cursor.
 */
void	caseinvert()
{
	if (isprint(*txt_ptr))
	{	outchar(*txt_ptr = invert(*txt_ptr));
		mod = 1;
	}
	txt_y = move_y(movefwd, movechar);	/* Advance cursor */
}
/* -------------------------------------------------------
 * Delete word right
 */
void	delword()
{
	char	*tmp_ptr, *tmp_blkbeg, *tmp_blkend;

	tmp_ptr = gap_end;
	tmp_blkbeg = blk_beg;
	tmp_blkend = blk_end;
	while
		(delright() && 
		(*(gap_end - 1) != '\n') && 
		(*(gap_end - 1) != '.') && 
		(*(gap_end - 1) != ',') && 
		(*(gap_end - 1) != ':') && 
		((*(gap_end - 1) != ' ' && *(gap_end - 1) != '\t') ||
		 (*gap_end == ' ' || *gap_end == '\t')));
	if (!undelsave(tmp_ptr, gap_end - tmp_ptr, FORWARD))
	{	blk_beg = tmp_blkbeg;
		blk_end = tmp_blkend;
		gap_end = tmp_ptr;
	}
}
/* -------------------------------------------------------
 * Delete cursor line.
 */
void	delline()
{
	char	*tmp_ptr;

	line_y(1);			/* Shift gap backward (left) to BOL */
	if (gap_end < txt_end)
	{	tmp_ptr = gap_end;
		while (*gap_end++ != '\n');
		if (gap_end > txt_end)
			gap_end = txt_end;
		if (!undelsave(tmp_ptr, gap_end - tmp_ptr, FORWARD))
			gap_end = tmp_ptr;	/* BUG - cursor jumps to col. 1 */
		txt_y = 1;			/* txt_x unchanged */
		disp_mode = dtoeos;
		mod=TRUE;
	}
}
/* -------------------------------------------------------
 * Insert line after cursor
 */
void	insertline()
{
	if (gap_beg + 1 < gap_end)
	{	*--gap_end = '\n';
		disp_mode = dpage;
		mod = TRUE;
	}
	else
		TPAErr();
}
/* -------------------------------------------------------
 * Repaint screen.
 */
void	repaint()
{
	status_line();
	wclear(editwin);
	helpstate = REFRESH;
	disp_mode = dpage;
}
/* -------------------------------------------------------
 * Add text at cursor.
 */
void	addtext(ch)
int		ch;
{
	char	*tmp_ptr;

	switch (ch) {
	case CNTRL_P:				/* Enter a control char. */
		if (cmd_mode == CKey)
			warning("^P-");
		else
			warning("HOME-P-");
		ch = (waitkey() & 0x1F);
#if 0
		if (ch == 0)			/* Allow this? (search() won't find ^@) */
		{	beep();
			break();
		}
#endif
		if (ch == '\t')
			goto addtab;
		else if (ch != '\n')
			goto addcntl;		/* Add control code or fall through and.. */
		/*FALLTHRU*/
	case CNTRL_N:				/* Insert a new line */
		insertline();
		break;

	case '\t':					/* TAB */
		addtab:
		if (gap_beg + 1 < gap_end)
		{	*++gap_beg = ch;
			if (!insflag && (gap_end < txt_end) && (*gap_end != '\n'))
			{	if (!undelsave(gap_end, 1, FORWARD))
					break;
				gap_end++;						/* Overwrite */
			}
			ch = txt_y + tabdef - (txt_y - 1) % tabdef;
			/* Expand TABs on output */
			for ( ; txt_y < ch ; txt_y++)
				outchar(' ');
			disp_mode = dtoeol;
			mod=TRUE;
		}
		else
			TPAErr();
		break;

	default:					/* Insert printable characters */
		if (isprint(ch))
	addcntl:					/* Sneak a cntl-char in (^P-) */
		{	if (gap_beg + 2 < gap_end)	/* 1 extra in case of wrap */
			{	*++gap_beg = ch;
				if (!insflag && (gap_end < txt_end) && (*gap_end != '\n'))
				{	if (!undelsave(gap_end, 1, FORWARD))
						break;
					gap_end++;						/* Overwrite */
				}
				txt_y++;
				if (!MRflag && (txt_y > rt_marg))	/* Need to wordwrap? */
				{	txt_ptr = gap_beg;
					while  ((*txt_ptr != ' ') &&	/* Find 1st SP or CR */
						(*txt_ptr != '\n') &&
						(txt_ptr > txt_beg))
						txt_ptr--;
					if (*txt_ptr == ' ')
					{	tmp_ptr = gap_beg;
						gap_beg++;					/* Space for NL */
						while (tmp_ptr > txt_ptr)	/* Shift text */
						{	*(tmp_ptr + 1) = *tmp_ptr;
							tmp_ptr--;
						}
						*(txt_ptr + 1) = '\n';
						txt_x++;
						txt_y = column(gap_end);
						disp_mode = dpage;  /* Redraw screen */
					}
					else
						goto nowrap; 		/* Can't reformat */
				}
				else
				{	nowrap:
					outchar(ch);
					disp_mode = dtoeol;		/* Redraw rest of line */
				}
				mod = TRUE;
				if (iscntrl(ch))			/* Control chars are special */
				{	if (cntlflag)			/* Are we showing? */
						disp_mode = dpage;	/* Yes, do page */
					else
						txt_y = column(gap_end);	/* Don't move cursor */
				}
			}
			else
				TPAErr();	/* Out of space */
		}
		else
			keyerr();		/* Non-printable char. */
		break;
	}
}
/* -------------------------------------------------------
 * Delete character to left of cursor
 */
void	delleft()
{
	if (gap_beg > txt_beg)
	{	if (!undelsave(gap_beg, 1, BACKWARD))
			return;
		mod = TRUE;
		if (blk_beg == gap_beg)
			blk_beg--;
		if (blk_end == gap_beg)
			blk_end--;
		gap_beg--;
		switch (*(gap_beg+1)) {		/* Character being removed */
		case '\n':
			txt_x--;
			txt_y = column(gap_end);
			disp_mode = dtoeos;
			break;
		case '\t':
			txt_y = column(gap_end);
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
int	textins(ins_beg, ins_end, direction)
char	*ins_beg, *ins_end;
enum	directions	direction;
{
	char		*ti_ptr;
	unsigned	ins_size;

	ins_size = ins_end - ins_beg;			/* Size to insert */

	if (ins_size > (gap_end - gap_beg + 2))	/* Space? */
	{	TPAErr();
		return(0);
	}
	disp_mode = dpage;						/* Redraw page afterwards */

	if (direction == FORWARD)				/* Insert after cursor */
	{	memcpy(gap_end - ins_size, ins_beg, ins_size);
		gap_end -= ins_size;
	}
	else									/* Insert before cursor */
	{	memcpy(gap_beg + 1, ins_beg, ins_size);
		gap_beg += ins_size;
		for (ti_ptr = ins_beg ; ti_ptr <= ins_end; ti_ptr++)
			if (*ti_ptr == '\n')
				txt_x++;			/* Adjust vertical */
		txt_y = column(gap_end);	/* Adjust horizontal */
	}
	return (1);
}
/* -----------------------------------------------
 * Execute shell command(s)
 * NOTES: - Imported (">") text will terminate at main text buffer overflow.
 *        - Imported text lines will be truncated after 1023 characters each.
 */
void	doshell()
{
	int		c;
	int		result;
	int		tempsize;
	char	commands[256];
	char	pipebuf[1024];
	char	*usershell;
	FILE	*fileptr;

	usershell = getenv("SHELL");

	if (usershell == (char *) NULL)
		usershell = "";

	/* Buffer space for user command */
	tempsize = 256 - strlen(usershell) - 1;

	if (tempsize < respsiz)
	{	promptkey("$SHELL variable too long ");
		return;
	}

	wattrset(editwin, STATATTR | A_BOLD);
	wmove(editwin, 1, 0);
	wclrtoeol(editwin);
	waddstr(editwin, "Hit ENTER for interactive shell, append \">\" for shell input");
	wrefresh(editwin);
	wattrset(editwin, A_NORMAL);
	disp_mode = dpage;

	result = promptst("Shell command: ", "", respptr, respsiz);
	if (result < 0)
		return;		/* Abort */

	/* If $SHELL defined, use it; */
	/*  otherwise, pass command line to "sh": */
	if (strlen(usershell) && strlen(respptr))
		/* Have $SHELL & response; need \"s for shell input, below */
		sprintf(commands, "%s -c \"%s\"", usershell, respptr);
	else if (strlen(usershell))
		/* Have $SHELL, no response */
		strcpy(commands, usershell);
	else if (strlen(respptr))
		/* Have response only */
		sprintf(commands, "%s", respptr);
	else
		/* No $SHELL, no response */
		strcpy(commands, "/bin/sh");

	setuid(getuid());	/* Set to real uid, gid */
	setgid(getgid());

	/* If last character is ">" ... */
	if (respptr[strlen(respptr) - 1] == '>')
	{	commands[strlen(commands)-2] = '"';	/* Remove ">" */
		commands[strlen(commands)-1] = '\0';
		if ((fileptr = (FILE *) popen(commands, "r")) != NULL)
		{	while ((fgets(pipebuf, 1023, fileptr)) != NULL)
				if (textins(pipebuf, pipebuf+strlen(pipebuf), BACKWARD) == 0)
				{	pclose(fileptr);
					return;
				}
				else
					mod = 1;
		}
		else
			promptkey("popen() error ");	
	}
	else
	{
		endwin();

		fputs("\n\n", stderr);
		if (strlen(respptr) == 0)
			fputs(" Type \"exit\" to return to SUE.\n\n", stderr);

		system(commands);	/* Shell out */

		setuid(geteuid());	/* Restore effective uid, gid */
		setgid(getegid());

		if (strlen(respptr))
		{	fputs(" Press ENTER to return to SUE: ", stderr);
			fgets(commands, 255, stdin);
		}
#ifdef AIX
		display();					/* Wake curses up.. */
		ioctl(0, TCSETAF, &tbuf);	/* ..and restore tty modes */
#endif
	}
}
