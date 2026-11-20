/* ================== GLOBAL INCLUDES ================= */

#include <limits.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <curses.h>
#include <term.h>
#include <signal.h>

/* ===================== GLOBAL DEFINES =================== */

/* Video attributes for monochrome monitors or '-b' option */

#define	STATUS_VIDEO	A_UNDERLINE	/* Status line	*/
#define	CONTROL_VIDEO	A_UNDERLINE	/* Control characters */
#define	FLAG_VIDEO		A_NORMAL	/* "<" flags	*/
#define	PROMPT_VIDEO	A_BOLD		/* Prompts	*/
#define	BLOCK_VIDEO		A_BOLD		/* Block text	*/

#define max(a,b) (((a)>(b))?(a):(b))

#define NULLCHARPTR (char *) 0

#define	TABDEF	4			/* default TAB width */

/* ASCII control characters: */
#define CNTRL_AT 0x00
#define	CNTRL_A	0x01
#define	CNTRL_B	0x02
#define	CNTRL_C	0x03
#define	CNTRL_D	0x04
#define	CNTRL_E	0x05
#define	CNTRL_F	0x06
#define	CNTRL_G	0x07
#define	CNTRL_H	0x08
#define	CNTRL_I	0x09
#define	CNTRL_J	0x0A
#define	CNTRL_K	0x0B
#define	CNTRL_L	0x0C
#define	CNTRL_M	0x0D
#define	CNTRL_N	0x0E
#define	CNTRL_O	0x0F
#define	CNTRL_P	0x10
#define	CNTRL_Q	0x11
#define	CNTRL_R	0x12
#define	CNTRL_S	0x13
#define	CNTRL_T	0x14
#define	CNTRL_U	0x15
#define	CNTRL_V	0x16
#define	CNTRL_W	0x17
#define	CNTRL_X	0x18
#define	CNTRL_Y	0x19
#define	CNTRL_Z	0x1A

#define BS	0x08
#define	CR	0x0D
#define	DEL	0x7F
#define	ESC	0x1B
#define	LF	0x0A

#define	CASE_INVERT	0x1E		/* Control-^ command key */

#ifndef ACS_HLINE
# define ACS_HLINE '-'
#endif

/* ================= GLOBAL VARIABLES ================= */

extern	int	tabdef;			/* Default tab width */
extern	int	scr_length;		/* Screen height */
extern	int	scr_width ;		/*   "    width */

						/* Screen coordinates */
extern	int scr_ybeg;	/* top row */
extern	int scr_yend;	/* bottom row */
extern	int scr_xbeg;	/* first column */
extern	int scr_xend;	/* last column */

extern	int txt_x;		/* Current cursor row */
extern	int txt_y;		/*   "       "    column */

extern	int rt_marg;	/* Current right margin */

extern	char	*gap_beg, *gap_end;	/* Gap markers */
extern	char	*txt_beg, *txt_end;	/* Text buffer limits */

extern	char	*txt_ptr;		/* General purpose pointer */

extern	char	*blk_beg, 		/* Block markers */
				*blk_end,	
				*tblk_beg,
				*tblk_end;

#define TXTNAME_SIZE (PATH_MAX + FILENAME_MAX + 1)
extern	char	txtname[TXTNAME_SIZE];		/* Current filename */

extern	char	s_str[];		/* External array for ^L */
extern	char	*sptr;			/* pointer to same */
extern	char	s_strsiz;		/* size of same */

extern	char	r_str[];		/* Replace (^QA) string buffer */
extern	char	*rptr;			/* ..and pointer	*/
extern	char	r_strsiz;		/* ..and size		*/
extern	int		repkey;			/* ..and ("*") key flag */
extern	char	repflag;		/* ..and find or replace flag */

extern	char	mod;			/* File modified flag */
extern	int		noF5;			/* F5 key present? (vt220 lacks this) */

extern	char	response[];		/* Response buffer */
extern	char	*respptr;		/* pointer to same */
extern	char	respsiz;		/* size of same */

extern	char	*bufptr;		/* General purpose pointer */

extern	char	insflag;		/* INSert mode flag */
extern	char	MRflag;			/* Margin Release flag */
extern	char	cntlflag;		/* Control char. display on/off */

extern	int		warnflag;		/* A warning is present */
extern	int		helplines;		/* Lines of help to display */
extern	int		editlines;		/* Varies with helplines */

extern	WINDOW	*editwin, *promptwin,	/* Curses windows */
		*statuswin, *helpwin;

enum mvmode	/* Global flags for move_y() */
{
	moveword, movechar
};
enum mvdir
{
	moveback, movefwd
};

extern	chtype	STATATTR;	/* Status window video attributes */
extern	chtype	FLAGATTR;	/* Flag character video attributes */
extern	chtype	PROMPTATTR;	/* Prompt video attributes */
extern	chtype	BLOCKATTR;	/* Block text attributes */
extern	chtype	CNTLATTR;	/* Control characters */

/*
 * Global flags for display(). *NOTE - these values are ORDINAL*:
 */
enum dispmode
{
	dnothing,	/* No redisplay 								*/
	dtoeol,		/* Redisplay to end of line 					*/
	dtoeolhc,	/*     "      "  "  "   "    with Hard CR flag	*/
	dtoeos,		/* Delete line or redisplay to end of screen	*/
	dinsln,		/* Insert line or redisplay from line above		*/
	dpage,		/* Redisplay entire screen						*/
	dpages,		/*     "        "      "   with 1/2 page scroll */
	dscrlup,	/* Scroll screen up								*/
	dscrldn		/*    "      "   down							*/
};
extern enum dispmode disp_mode;		/* Global screen refresh flag */

enum helptype
{
	REFRESH,	/* Flag refresh					*/
	PRIMARY,	/* Initial menu					*/
	QUICK,		/* Quick commands menu			*/
	BLOCK,		/* Block     "      "			*/
	ONSCREEN	/* Onscreen  "      "			*/
};

extern	enum	helptype	helpstate;

enum directions
{
	BACKWARD, FORWARD
};

enum filemodes
{
	UNIX, MSDOS, MACINTOSH
};
extern	int	autofilemode;

extern	enum	filemodes	filemode;

struct undelnode
{
	char *text;
	unsigned size;
	enum directions direction;
	struct undelnode *next, *previous;
};

extern	struct	undelnode	*undelptr;

#ifdef	DEBUG
extern	FILE	*debugterm;
#define	DEBUGTERM	"/dev/vt04"
#endif

/* getopt() and its variables come from <unistd.h> */

/* ================== GLOBAL FUNCTION PROTOTYPES ================ */

/* data.c */
extern char * start_line (char *line);
extern char * next_line (char *line);
extern int new_line (int ntime);
extern int column (char *line);
extern int text_move (void);
extern int line_y (int col);
extern int move_x (int line_no);
extern int move_y (enum mvdir direction, enum mvmode mode);
extern void move_ptr (void);
extern char * strtoup (char *str);
extern char invert (char c);
extern int undel (void);
extern int undelsave (char *txt_start, unsigned txt_size,
	enum directions undel_direction);

/* display.c */
extern int blkerr (char level);
extern char abandon (void);
extern int waitkey (void);
extern void outchar (char a);
extern void statprint (int col, char *st1, char *st2, char *st3);
extern void status (int col, char *st1, char *st2, char *st3);
extern void status_line (void);
extern void prompt (int col, char *st1, ...);
extern void warning (char *strptr);
extern int promptst (char *st1, char *st2, char *inputst, int inlen);
extern int promptkey (char *strptr);
extern void display (void);
extern void xwprintw (WINDOW *xwindow, char *string1, ...);
extern void dohelp (enum helptype helprequest, WINDOW *hwindow);
extern void valerr (void), notfound (void), keyerr (void), TPAErr (void);
extern void debugpr (char *str1, char *str2, char *str3);

/* funcs.c */
extern void blockhlp (void), quickhlp (void), onscrhlp (void);
extern int findfunc (void);
extern void replfunc (void), quitfunc (void), savefunc (void), exitfunc (void);
extern void infofunc (void);
extern int delright (void);
extern int name (void);
extern int save_fname (void);
extern int find (void);
extern int replace (void);
extern void gotoline (void), tabset (void), rmset (void);
extern void reform (void);
extern void sabort (int sig);
extern void writeblk (void), readblk (void), newfile (void);
extern int save (char *s_name, char *s_type);
extern int load (char *l_name);
extern void setfilemode (void), marrel (void);
extern void blockfuncs (int funcno);
extern void togglehelp (void);
extern void repfind (void), caseinvert (void);
extern void delword (void), delline (void), insertline (void);
extern void repaint (void), addtext (int ch), delleft (void);
extern int textins (char *ins_beg, char *ins_end,
	enum directions direction);
extern void doshell (void);

/* sue.c */
extern void edit_init (char *v_name);
extern void edit_exit (int exparm);
extern void Key_scan (void);

/* command.c */
extern void q_key (void), k_key (void), o_key (void);

#ifdef LOOKBUSY
extern void lookbusy (void);
#endif

/* ===================== GLOBAL MACROS ==================== */

#define wp1(str1) wclrtoeol (editwin); xwprintw (editwin, str1)
#define wp2(str1, str2) wclrtoeol (editwin); xwprintw (editwin, str1, str2)
#define wp3(str1, str2, str3) wclrtoeol (editwin); \
	xwprintw (editwin, str1, str2, str3)
#define wp4(str1, str2, str3, str4) wclrtoeol (editwin); \
	xwprintw (editwin, str1, str2, str3, str4)
