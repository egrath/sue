/*
Project:	SUE, a Simple Unix Editor
File:		sue.c
Copyright:	SUE is copyright 1992 by Point to Point Communications.
Routines:	Main, initialization, and exit routines.
Author:		Carson Wilson
				Apt. 1D
				1359 W. Greenleaf, Chicago, IL 60626
				internet:	carson@mcs.com
Notes:		Edit with hard tab stops set to 4.
*/

static	char	rcsid[] = "(#)$Id: sue.c,v 1.4 1995/03/19 17:01:58 carson Exp $";

#include "version.h"
#include "sue.h"

#define EXTRAMEM 100000L		/* Extra memory to allocate at	*/
								/*  startup time		*/

/* ========= GLOBAL VARIABLES =========== */

int	repkey;						/* Response to Replace query */

int	tabdef = TABDEF;			/* Default tab spacing (^OT) */

char	insflag = TRUE;			/* INSert toggle */
char	MRflag	= FALSE;		/* Margin Release toggle */
char	cntlflag = FALSE;		/* Control char. display toggle */
char	repflag = FALSE;		/* Replace flag for ^L */

int scr_ybeg, scr_yend;			/* Horizontal screen limits */
int scr_xbeg, scr_xend;			/* Vertical      "     "    */
int txt_x, txt_y;				/* Text line and col number */

int rt_marg = 0;				/* Right margin */

/*	Text is stored in the editor as 2 sequential areas with a
 *	GAP between.  It is into this GAP that inserts are made (thereby
 *	decreasing the size of the GAP), conversely deletions enlarge the 
 *	GAP.  The GAP is defined as follows:
 *
 *		gap_beg = memory location of character to the
 *				LEFT of the cursor
 *		gap_end = memory location of character the 
 *				cursor is ON
 *		txt_beg = memory location BEFORE first legitimate
 *				text character
 *		txt_end = memory location of character AFTER last 
 *				legitimate character
 */
char	*gap_beg, *gap_end;		/* Gap definition */
char	*txt_beg, *txt_end;		/* Text definition in memory */

char	*txt_ptr;				/* Temporary text pointer */

char	*blk_beg, *blk_end;		/* Block markers */
char	*tblk_beg, *tblk_end;	/* Block temporary markers */

char	txtname[PATH_MAX+FILENAME_MAX+1];		/* Edit full file name */

char	response[60];		/* General purpose response buffer */
char	*respptr;			/* ..and pointer	*/
char	respsiz;			/* ..and size		*/

char	s_str[60];			/* Search string buffer */
char	*sptr;				/* ..and pointer	*/
char	s_strsiz;			/* ..and size		*/

char	r_str[60];			/* Replace string buffer */
char	*rptr;				/* ..and pointer	*/
char	r_strsiz;			/* ..and size		*/

char	mod;				/* Signifies modifications made */

int		helplines;			/* Lines of help to display */
int		editlines;			/* Lines in edit window; varies with helplines */

char	*bufptr;			/* Buffer ptr. for malloc */

int		opt;				/* Results from getopt() */
int		optresult;

enum
  comdmode cmd_mode;		/* Command mode */
int		restrict;			/* Secure mode flag */
int		noF5 = 0;			/* F5 key present? */
int		bwopt;				/* Force black & white display */
int		warnflag = 0;		/* A warning is present */

enum dispmode disp_mode;	/* Global screen refresh flag */

enum helptype helpstate;	/* Global help screen state flag */

enum filemodes filemode;	/* UNIX/MSDOS/MACINTOSH format */
int	autofilemode;			/* ENV or command line options override 
								file mode autodetection */

chtype	STATATTR;			/* Status window video attributes */
chtype	FLAGATTR;			/* Flag character attributes */
chtype	PROMPTATTR;			/* Prompt window attributes */
chtype	CNTLATTR;			/* Control characters */
chtype	BLOCKATTR;			/* Block text */

struct	undelnode	*undelptr;

#ifdef AIX					/* Terminal control buffers */
struct termio tbuf, tbufsave;
#endif

#ifdef DEBUG
FILE	*debugterm;
#endif

/*----------------------------------------
 * Exit SUE
 */
void edit_exit(exparm)
int exparm;
{
#ifdef AIX
	if (ioctl(0, TCSETA, &tbufsave) == -1)	/* Restore tty modes */
		perror("SUE:ioctl");
#endif
	endwin();
	printf("\n");
	exit(exparm);
}
/* ---------------------------------------------
 * Allocate memory (allocsize + EXTRAMEM)
 * Return 1 if OK, 0 if overflow.
 */
int doalloc(allocsize)
long allocsize;
{
	long	allocwork;
	unsigned mallarg;
	int	i;

	/* Set up working memory */
	allocwork = allocsize + EXTRAMEM;	/* Add buffer space */

	mallarg = allocwork;			/* Convert to unsigned for malloc() */

	while (mallarg != allocwork)	/* Test for truncation on 80286 */
	{
		allocwork -= 100;
		mallarg = allocwork;
	}
	if (mallarg < (allocsize + 100))	/* Not enough space */
		return(0);

	free ((char *) bufptr);			/* Release prev. allocation */
	bufptr = malloc(mallarg);
	if (bufptr == (char *) NULL)	/* Allocation fails */
		return (0);

	txt_beg = bufptr;
	txt_end = txt_beg + mallarg - 1;	/* Final top memory */

	/* Set last part of memory to newlines + [EOF] label
	   and lastly a NULL so all of text is treated as a string */
	*txt_end-- = 0;
	for(i = 1; i <= LINES - 1; i++, txt_end--)
		*txt_end = '\n';
	*txt_end-- = ']';	/* Special [EOF] marker */
	*txt_end-- = 'F';
	*txt_end-- = 'O';
	*txt_end-- = 'E';
	*txt_end = '[';
	*txt_beg   = '\n';
	gap_beg = txt_beg;
	gap_end = txt_end;
	return (1);			/* Success */
}
/* ----------------------------------------------
 * Set up buffers & pointers for new file
 */
void edit_init(v_name)
char	*v_name;
{
	struct stat suestat;

	mod = FALSE;
	while (undel());		/* Free buffers and set pointer to NULL */
	blk_beg = blk_end = NULL;

	/* Initialize the display variables for screen and cursor */
	txt_x = txt_y = 1;
	scr_xbeg = 1; scr_xend = editlines;
	scr_ybeg = 1; scr_yend = COLS;

	if (doalloc(0L) == 0)				/* Pre-allocate up to EXTRAMEM */
	{	TPAErr();						/* No memory; abort */
		edit_exit(1);
	}
	strcpy(txtname,"");
	if (v_name != NULLCHARPTR)			/* Have filespec */
	{
		if (strlen(v_name) >= sizeof(txtname))
		{	beep();
			promptkey("Filename too long! ");
		}
		else
		{	strcpy(txtname, v_name);

			/* File present? */
			if (stat(txtname, &suestat) != -1)
			{	/* Yes. Have enough memory for file? */
				if (doalloc(suestat.st_size))
				{	/* Yes, attempt load */
					if (load(txtname) == -1)	/* Load fails */
						strcpy(txtname,"");
				}
				else	/* Not enough memory */
				{	TPAErr();
					strcpy(txtname,"");
				}
			}
		}
	}
	status_line();
	disp_mode = dpage;	/* Display 1st page of text */
	if (gap_beg == txt_beg)		/* Nothing loaded, NEW file */	
	{	*--gap_end = '\n';		/* Add a newline */
		display();				/* Show 1st screen */
		warning("New file ");	/* Warn user */
		warnflag = 1;			/* Flag; see input routines */
	}
	else					/* Move text to start of working area */
		while(gap_beg > txt_beg)
			*--gap_end = *gap_beg--;
}
/*
 ****************************************
 * 				MAIN PROGRAM			*
 ****************************************
 */
main(argc, argv)
int		argc;
char	*argv[];
{
	char	*eargv[2];
	int	i;

#ifdef DEBUG
	debugterm = fopen(DEBUGTERM, "w");
#endif
	respptr = &response[0];		/* Initialize pointers */
	respsiz = sizeof(response) - 1;
	undelptr = NULL;

	sptr = &s_str[0];
	s_strsiz = sizeof(s_str) - 1;

	rptr = &r_str[0];
	r_strsiz = sizeof(r_str) - 1;

	opterr = 0;			/* Suppress getopt() error message */
	bwopt = optresult = helplines = restrict = 0;	/* Option defaults */
	autofilemode = 1;	/* Default to file mode autodetect */
	filemode = UNIX;	/* Default to Unix file type */
	cmd_mode = CKey;	/* Default to control-key mode */


	/* First, check for $SUE environment variable */
	if ( (txt_ptr = getenv("SUE")) != NULLCHARPTR)
	{	eargv[1] = txt_ptr;
		while ((opt = getopt(2, eargv, "bfdmnrs:t:u")) != -1)
			switch (opt) {
			case 'b' :
				bwopt = 1;
				break;
			case 'f' :
				cmd_mode = FKey;
				break;
			case 'd' :
				autofilemode = 0;
				filemode = MSDOS;
				break;
			case 'm' :
				autofilemode = 0;
				filemode = MACINTOSH;
				break;
			case 'n' :
				helplines = 8;
				break;
			case 'r' :
				restrict = 1;
				break;
			case 's' :
				if (atoi(optarg) > 1)
					rt_marg = atoi(optarg);
				else
					optresult++;
				break;
			case 't' :
				if (atoi(optarg) > 0)
					tabdef = atoi(optarg);
				else
					optresult++;
				break;
			case 'u' :
				autofilemode = 0;
				filemode = UNIX;
				break;
			default  :
				optresult++;
			}
		if (optresult)
		{	fprintf(stderr, "\n%s: option error in SUE environment variable.\n\n", argv[0]);
			exit (2);
		}
	}

	/* Process command line options (override environment options) */
	optind = 1;
	while ( (opt = getopt(argc, argv, "bcfdmnrs:t:u")) != -1)
		switch (opt) {
		case 'b' :
			bwopt = !bwopt;
			break;
		case 'c' :				/* Override ENV setting */
			cmd_mode = CKey;
			break;
		case 'f' :
			cmd_mode = FKey;
			break;
		case 'd' :
			autofilemode = 0;
			filemode = MSDOS;
			break;
		case 'm' :
			autofilemode = 0;
			filemode = MACINTOSH;
			break;
		case 'n' :
			if (helplines)			/* Toggle off */
				helplines = 0;
			else
				helplines = 8;
			break;
		case 'r' :
			restrict = !restrict;
			break;
		case 's' :
			if (atoi(optarg) > 1)
				rt_marg = atoi(optarg);
			else
				optresult++;
			break;
		case 't' :
			if (atoi(optarg) > 0)
				tabdef = atoi(optarg);
			else
				optresult++;
			break;
		case 'u' :
			autofilemode = 0;
			filemode = UNIX;
			break;
		default :
			optresult++;
		}
	if (optresult)
	{	fprintf(stderr, "\nSUE - a Simple Unix Editor, version %s", suerev);
		fprintf(stderr, "\nCopyright 1995 by Point to Point Communications\n");
		fprintf(stderr, "  Usage:\n");
		fprintf(stderr, "    %s [-bcfdmnru][-sn][-tn] [file]\n", argv[0]);
		fprintf(stderr, "  Options:\n");
		fprintf(stderr, "    -b  for black & white operation\n");
		fprintf(stderr, "    -c  control key mode\n");
		fprintf(stderr, "    -f  function key mode\n");
		fprintf(stderr, "    -d  MSDOS file mode\n");
		fprintf(stderr, "    -m  Macintosh file mode\n");
		fprintf(stderr, "    -n  novice mode\n");
		fprintf(stderr, "    -r  restricted mode\n");
		fprintf(stderr, "    -sn set right margin to n\n");
		fprintf(stderr, "    -tn set tab spacing to n\n");
		fprintf(stderr, "    -u  Unix file mode\n\n");
		exit (1);
	}

	/* Initialize 'curses' functions */
	initscr();	/* Initialize 'curses' from environment */

#ifdef TRACE
/* Ncurses tracing */
	trace(TRACE_ORDINARY);
/*	trace(TRACE_CHARPUT);	*/
/*	trace(TRACE_MAXIMUM);	*/
#endif

	if (COLS < 80)
	{	endwin();
		fprintf(stderr, "SUE: your terminal definition specifies %d columns.\n", COLS);
		fprintf(stderr, "I need at least 80 columns in order to operate properly.\n");
		fprintf(stderr, "Current terminal definition is: %s\n", longname());
		fprintf(stderr,	"Press RETURN: ");
		fgetc(stdin);
		exit(3);
	}

	if (cmd_mode == FKey)
	{	if (tgetstr("k;", NULL) == /*(char *)*/ NULL)
		{	endwin();
			fprintf(stderr, "SUE: your current terminal definition is: %s\n", longname());
			fprintf(stderr, "This terminal definition does not include enough function keys\n");
			fprintf(stderr, "for function key mode.  I need definitions for at least the first\n");
			fprintf(stderr, "10 function keys in order to operate properly.\n");
			fprintf(stderr, "Would you like to use control key mode instead (y/N)? ");
			i = fgetc(stdin);
			if (i == 'Y' || i == 'y')
			{	fflush(stdin);	/* Remove \r */
				cmd_mode = CKey;
				fprintf(stderr, "You have selected control key mode.\n");
				fprintf(stderr, "To select it from the command line, use option -c.\n");
				fprintf(stderr, "Press RETURN: ");
				fgetc(stdin);
			}
			else
			{	fprintf(stderr, "SUE aborting.\n");
				exit(4);
			}
		}
		else
		{	if (helplines)
				helplines = 3;
			/* Do we have F5 key definition (vt220 lacks) */
			noF5 = (tgetstr("k5", NULL) == NULL);
		}
	}

	noecho();	/* Do not echo characters typed at keyboard */
	nonl();		/* Pass ^M thru to SUE as-is */
	raw();
#ifdef AIX
		/* AIX needs some help... */
	if (ioctl(0, TCGETA, &tbuf) == -1)
		perror("SUE:ioctl2");
	tbufsave = tbuf;
	tbuf.c_iflag &= ~IXON;		/* Disable flow control */
	tbuf.c_lflag &= ~ISIG;		/* Disable keyboard signals */
	if (ioctl(0, TCSETAF, &tbuf) == -1)
		perror("SUE:ioctl3");
#endif

	editlines = LINES - helplines - 1;

	/* Set up 'curses' windows: */

	if (helplines)
		helpwin   = newwin(helplines, COLS, 1, 0);		/* Novice help */
	statuswin = newwin(1, COLS, 0, 0);					/* Top line */
	editwin   = newwin(editlines, COLS, helplines+1, 0);	/* Next 2 overlap */
	promptwin = newwin(1, COLS, helplines+1, 0);

#ifdef DOCOLOR
	/* Color terminals and curses implementations */
	start_color();					/* Initialize color routines */
	if (has_colors() && !bwopt)		/* Black & White option flag */
	{	init_pair(1, COLOR_GREEN, COLOR_BLACK);
		STATATTR = COLOR_PAIR(1);
		init_pair(2, COLOR_BLUE, COLOR_BLACK);
		/*FLAGATTR = COLOR_PAIR(2) | A_BOLD;*/
		/* Curses bug precludes this ^^^^ */
		FLAGATTR = COLOR_PAIR(2);
		init_pair(3, COLOR_YELLOW, COLOR_BLACK);
		PROMPTATTR = COLOR_PAIR(3) | A_BOLD;
	}
	else
#endif
	/* Monochrome terminals and/or curses implementations */
	{	STATATTR   = STATUS_VIDEO;	/* A_UNDERLINE */
		FLAGATTR   = FLAG_VIDEO;	/* A_NORMAL */
		PROMPTATTR = PROMPT_VIDEO;	/* A_BOLD */
	}
	BLOCKATTR  = BLOCK_VIDEO;		/* A_BOLD */
	CNTLATTR   = CONTROL_VIDEO;		/* A_UNDERLINE */

	wattrset(statuswin, STATATTR);
	wattrset(promptwin, PROMPTATTR);
	keypad(editwin, TRUE);		/* Interpret extended keycodes */
	keypad(promptwin, TRUE);
	if (rt_marg < 2)
		rt_marg = COLS - 5;		/* Set default right margin */

	if (helplines)				/* Initialize novice help */
	{	if (cmd_mode == CKey)
			dohelp(PRIMARY, helpwin);
		else if (cmd_mode == FKey)
		{	if (restrict)
				dohelp(RFKEYMENU, helpwin);
			else
				dohelp(FKEYMENU, helpwin);
		}
	}

	/* Trap signals to our exit routine that preserves user text */
	signal(SIGINT,  sabort);
	signal(SIGHUP,  sabort);
	signal(SIGQUIT, sabort);
	signal(SIGTERM, sabort);
	signal(SIGSEGV, sabort);
	signal(SIGBUS,  sabort);
	signal(SIGILL,  sabort);
	signal(SIGSYS,  sabort);

	bufptr = malloc(0);		/* Initialize pointer for doalloc() */

	prompt(1, "");		/* Must do this so that newwin()'s auto-clear */
						/* function doesn't clear text on first */
						/* warning() call */

	/* Load file and/or prepare to edit */
	edit_init(argv[optind]);
	/* Main edit loop - should never exit */
	for ( ; ; )
	{	if (!warnflag)
			display();		/* Don't erase warnings */
		switch (cmd_mode) {
		case CKey :			/* Control key mode */
			CKey_scan();
			break;
		case FKey :			/* Function key mode */
			FKey_scan();
			break;
		}
	}
}
