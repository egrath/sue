/* #######################################################################
 * ts.c - TextStar: program startup and teardown.
 * purpose:
 *     Defines every global variable of the editor and holds the program
 *     entry point: command-line parsing, curses setup, text-buffer
 *     allocation (do_alloc), file loading (edit_init) and the main edit
 *     loop, which alternates between display() and key_scan() forever.
 * notes:
 *     The other files: data.c holds the gap-buffer primitives, display.c
 *     all curses output, funcs.c the editor commands and command.c the
 *     key dispatch. See ts.h for the gap-buffer layout.
 * ####################################################################### */

#include "version.h"
#include "ts.h"

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

char	txtname[TXTNAME_SIZE];		/* Edit full file name */

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

int	optresult;				/* Usage-error counter: any bad command */
					/* line option makes main() print usage */

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

WINDOW	*editwin, *promptwin,	/* Curses windows */
		*statuswin, *helpwin;

struct undelnode *undelptr;

/* #######################################################################
 * void edit_exit (int exparm)
 * purpose:
 *     Shut the editor down: leave curses screen mode and exit the
 *     process. Used by the normal quit paths and by the error and
 *     signal handlers.
 * returns:
 *     Does not return.
 * parameters:
 *     int exparm: the process exit status (0 = ok, 1 = error,
 *     3 = terminal too small).
 * ###################################################################### */
void edit_exit (int exparm)
{
	endwin ();
	printf ("\n");
	exit (exparm);
}
/* #######################################################################
 * static int do_alloc (long allocsize)
 * purpose:
 *     (Re-)allocate the text buffer with room for allocsize bytes of
 *     file plus EXTRAMEM of working space, and lay out its fixed
 *     furniture: from the top of the buffer, the terminating NUL,
 *     LINES-1 padding newlines and the "[EOF]" marker; txt_beg gets
 *     an opening newline. The gap initially spans the whole free
 *     area, so the cursor starts on the "[EOF]" marker.
 * returns:
 *     1 on success. 0 if the size does not fit an unsigned (80286
 *     truncation guard) or malloc fails - in both cases the old
 *     buffer stays valid and the editor can continue.
 * parameters:
 *     long allocsize: file size in bytes to make room for (0 for an
 *     empty buffer).
 * notes:
 *     The new buffer is allocated BEFORE the old one is freed, so an
 *     out-of-memory failure never leaves dangling gap pointers.
 *     txt_end ends up pointing at the '[' of "[EOF]" - the marker,
 *     the padding newlines and the NUL all live above txt_end and are
 *     never part of the user's text.
 * ###################################################################### */
static int do_alloc (long allocsize)
{
	long	allocwork;
	unsigned mallarg;
	int	i;
	char *newbuf;

	/* Set up working memory */
	allocwork = allocsize + EXTRAMEM;	/* Add buffer space */

	mallarg = allocwork;			/* Convert to unsigned for malloc() */

	while (mallarg != allocwork)	/* Test for truncation on 80286 */
	{
		allocwork -= 100;
		mallarg = allocwork;
	}
	if (mallarg < (allocsize + 100))	/* Not enough space */
	{
		return (0);
	}

	newbuf = malloc (mallarg);		/* Allocate new buffer first */
	if (newbuf == NULL)				/* Allocation fails; old buffer stays valid */
	{
		return (0);
	}
	free ((char *) bufptr);			/* Release prev. allocation */
	bufptr = newbuf;

	txt_beg = bufptr;
	txt_end = txt_beg + mallarg - 1;	/* Final top memory */

	/* Set last part of memory to newlines + [EOF] label
	   and lastly a NULL so all of text is treated as a string */
	*txt_end-- = 0;
	for (i = 1; i <= LINES - 1; i++, txt_end--)
	{
		*txt_end = '\n';
	}
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
/* #######################################################################
 * void edit_init (char *v_name)
 * purpose:
 *     Prepare a fresh editing session: reset the undo list, block
 *     markers and cursor, allocate the empty buffer, and - if a file
 *     name was given - stat the file and load it into a right-sized
 *     buffer. A new file gets one newline created and a "New file"
 *     warning; a loaded file has its text moved to the start of the
 *     working area.
 * parameters:
 *     char *v_name: the file to edit, or NULLCHARPTR for an unnamed
 *     new buffer.
 * notes:
 *     On load failure the file name is dropped and the session
 *     continues as a new file. Leaves disp_mode = dpage so the first
 *     display() in main() repaints the whole screen.
 * ###################################################################### */
void edit_init (char *v_name)
{
	struct stat filestat;

	mod = FALSE;
	while (undel());		/* Free buffers and set pointer to NULL */
	blk_beg = blk_end = NULL;

	/* Initialize the display variables for screen and cursor */
	txt_x = txt_y = 1;
	scr_xbeg = 1; scr_xend = editlines;
	scr_ybeg = 1; scr_yend = COLS;

	if (do_alloc (0L) == 0)				/* Pre-allocate up to EXTRAMEM */
	{
		tpa_err ();						/* No memory; abort */
		edit_exit (1);
	}
	strcpy (txtname, "");
	if (v_name != NULLCHARPTR)			/* Have filespec */
	{
		if (strlen (v_name) >= sizeof (txtname))
		{
			beep ();
			prompt_key ("Filename too long! ");
		}
		else
		{
			strcpy (txtname, v_name);

			/* File present? */
			if (stat (txtname, &filestat) != -1)
			{
				/* Yes. Have enough memory for file? */
				if (do_alloc (filestat.st_size))
				{
					/* Yes, attempt load */
					if (load (txtname) == -1)	/* Load fails */
					{
						strcpy (txtname, "");
					}
				}
				else	/* Not enough memory */
				{
					tpa_err ();
					strcpy (txtname, "");
				}
			}
		}
	}
	status_line ();
	disp_mode = dpage;	/* Display 1st page of text */
	if (gap_beg == txt_beg)		/* Nothing loaded, NEW file */
	{
		*--gap_end = '\n';		/* Add a newline */
		display ();				/* Show 1st screen */
		warning ("New file ");	/* Warn user */
		warnflag = 1;			/* Flag; see input routines */
	}
	else					/* Move text to start of working area */
	{
		while (gap_beg > txt_beg)
		{
			*--gap_end = *gap_beg--;
		}
	}
}
/* #######################################################################
 * int main (int argc, char *argv[])
 * purpose:
 *     Program entry point. Initializes the global pointer/size pairs,
 *     parses the command line with the built-in parser, sets up curses
 *     (windows, monochrome attributes, extended keys, signal traps),
 *     calls edit_init() and runs the main edit loop: display() +
 *     key_scan() forever - key_scan dispatches every keystroke to the
 *     command handlers; the loop is only left via edit_exit() from
 *     inside a command.
 * returns:
 *     Never returns normally. exit(1) on a usage error, exit(3) on a
 *     terminal below 80 columns.
 * parameters:
 *     int argc, char *argv[]: as usual. Options may be clustered and
 *     take attached values only: -d/-m/-u file mode, -s78 right
 *     margin (0 = none, the default), -t8 tab width; the first
 *     non-option argument is the file to edit.
 * notes:
 *     The option parser is built in deliberately: the Watcom clib
 *     getopt() hangs on an empty command line under DOS, and ULTRIX's
 *     minimal unistd.h does not declare getopt at all.
 * ###################################################################### */
int main (int argc, char *argv[])
{
	int i;

	respptr = &response[0];		/* Initialize pointers */
	respsiz = sizeof (response) - 1;
	undelptr = NULL;

	sptr = &s_str[0];
	s_strsiz = sizeof (s_str) - 1;

	rptr = &r_str[0];
	r_strsiz = sizeof (r_str) - 1;

	optresult = helplines = 0;	/* Option defaults */
	autofilemode = 1;	/* Default to file mode autodetect */
#ifdef DOS
	filemode = MODE_MSDOS;	/* Default to Dos file type */
#else
	filemode = MODE_UNIX;	/* Default to Unix file type */
#endif


	/* Parse the command line options (built-in parser - no getopt()
	 * dependency: the Watcom clib getopt hangs with an empty command
	 * line on DOS). */
	i = 1;
	while (i < argc && argv[i][0] == '-' && argv[i][1] != '\0')
	{
		char	*arg = argv[i] + 1;

		while (*arg != '\0')
		{
			switch (*arg)
			{
			case 'd':
				autofilemode = 0;
				filemode = MODE_MSDOS;
				break;
			case 'm':
				autofilemode = 0;
				filemode = MODE_MACINTOSH;
				break;
			case 's':
				{
					int value = atoi (arg + 1);

					/* A digit string must follow the 's'
					 * (rejection of -s, -sx); 0 = no margin */
					if (!isdigit (arg[1]) || value < 0)
					{
						optresult++;
					}
					else
					{
						rt_marg = value;
					}
					arg += strlen (arg + 1);
				}
				break;
			case 't':
				{
					int value = atoi (arg + 1);

					if (arg[1] == '\0' || value < 1 || value > 32)
					{
						optresult++;
					}
					else
					{
						tabdef = value;
					}
					arg += strlen (arg + 1);
				}
				break;
			case 'u':
				autofilemode = 0;
				filemode = MODE_UNIX;
				break;
			default:
				optresult++;
			}
			arg++;
		}
		i++;
	}
	if (optresult)
	{
		fprintf (stderr, "\nTextStar - a WordStar-style text editor, version %s", tsrev);
		fprintf (stderr, "\nCopyright 1995 by Point to Point Communications\n");
		fprintf (stderr, "  Usage:\n");
		fprintf (stderr, "    %s [-dmstu][-sn][-tn] [file]\n", argv[0]);
		fprintf (stderr, "  Options:\n");
		fprintf (stderr, "    -d  MSDOS file mode\n");
		fprintf (stderr, "    -m  Macintosh file mode\n");
		fprintf (stderr, "    -sn set right margin to n (0 = none)\n");
		fprintf (stderr, "    -tn set tab spacing to n\n");
		fprintf (stderr, "    -u  Unix file mode\n\n");
		exit (1);
	}

	/* Initialize 'curses' functions */
	initscr ();	/* Initialize 'curses' from environment */

	if (COLS < 80)
	{
		endwin ();
		fprintf (stderr, "TextStar: your terminal definition specifies %d columns.\n", COLS);
		fprintf (stderr, "I need at least 80 columns in order to operate properly.\n");
		fprintf (stderr, "Current terminal definition is: %s\n", longname ());
		fprintf (stderr,	"Press RETURN: ");
		fgetc (stdin);
		exit (3);
	}

	noecho ();	/* Do not echo characters typed at keyboard */
	nonl ();		/* Pass ^M thru to TextStar as-is */
	raw ();

	editlines = LINES - helplines - 1;

	/* Set up 'curses' windows: */
	statuswin = newwin (1, COLS, 0, 0);					/* Top line */
	editwin   = newwin (editlines, COLS, helplines + 1, 0);	/* Next 2 overlap */
	promptwin = newwin (1, COLS, helplines + 1, 0);

	/* Monochrome video attributes for all platforms */
	STATATTR   = STATUS_VIDEO;	/* A_UNDERLINE */
	FLAGATTR   = FLAG_VIDEO;	/* A_NORMAL */
	PROMPTATTR = PROMPT_VIDEO;	/* A_BOLD */
	BLOCKATTR  = BLOCK_VIDEO;		/* A_BOLD */
	CNTLATTR   = CONTROL_VIDEO;		/* A_UNDERLINE */

	wattrset (statuswin, STATATTR);
	wattrset (promptwin, PROMPTATTR);
	keypad (editwin, TRUE);		/* Interpret extended keycodes */
	keypad (promptwin, TRUE);
	/* rt_marg stays 0 (no margin) unless -sn was given */

	/* Trap signals to our exit routine that preserves user text */
	signal (SIGINT,  ts_abort);
#ifdef UNIX
	signal (SIGHUP,  ts_abort);
	signal (SIGQUIT, ts_abort);
	signal (SIGSYS,  ts_abort);
	signal (SIGBUS,  ts_abort);
#endif
	signal (SIGTERM, ts_abort);
	signal (SIGSEGV, ts_abort);
	signal (SIGILL,  ts_abort);

	bufptr = malloc (0);		/* Initialize pointer for do_alloc() */

	prompt (1, "", NULL, NULL, NULL);	/* Must do this so that newwin()'s auto-clear */
						/* function doesn't clear text on first */
						/* warning() call */

	/* Load file and/or prepare to edit */
	edit_init (argv[i]);	/* argv[i] = the file name (if any) */

	/* Main edit loop - should never exit */
	for ( ;; )
	{
		if (!warnflag)
		{
			display ();		/* Don't erase warnings */
		}

		key_scan ();
	}
}
