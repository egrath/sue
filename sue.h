/*
Project:	SUE, a Simple Unix Editor
Copyright:	SUE is copyright 1992 by Point to Point Communications.
File:		sue.h
Routines:	Global include file.
Author:		Carson Wilson
				Apt. 1D
				1359 W. Greenleaf, Chicago, IL 60626
				internet:	carson@mcs.com
Notes:		Edit with hard tab stops set to 4.
			If you are using ncurses, you may need to link 
			  /usr/include/curses.h to 
			  /usr/include/ncurses/curses.h.

Version:	@(#)$Id: sue.h,v 1.5 1996/12/17 01:01:03 carson Exp $
*/

/* Needed by GCC under ISC sysvr3.2: */

#if (defined SUEV32 & defined __STDC__ )
# define PATH_MAX 256
# define SYSV
#endif

/* ================== GLOBAL INCLUDES ================= */

#include <limits.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#ifdef ULTRIX
# include <cursesX.h>
#else
# include <curses.h>
# include <term.h>
#endif
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

#define	TABDEF	8			/* default TAB width */

/* ASCII control characters: */
#define CNTRL_@ 0x00
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

#if (defined AIX | defined ULTRIX)
# define KEY_SLEFT 0770		/* Not available, so disable these */
# define KEY_SRIGHT 0771
# define KEY_HELP 0772
# define KEY_SFIND 0773
# define KEY_BTAB 0774
# define KEY_END 0775
# define wechochar(win, ch) (waddch(win,ch), wrefresh(win))
#endif

#ifndef ACS_HLINE
# define ACS_HLINE '-'
#endif

#ifdef LINUX
# define SIGSYS 34	/* unused */
#endif

/* ================= GLOBAL VARIABLES ================= */

extern	int	tabdef;			/* Default tab width */
extern	int	scr_length;		/* Screen height */
extern	int	scr_width ;		/*   "    width */

extern	int	restrict;		/* Restricted mode */

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

extern	char	txtname[];		/* Current filename */

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

WINDOW	*editwin, *promptwin, 	/* Curses windows */
		*statuswin, *helpwin;

enum mvmode	{moveword, movechar}; /* Global flags for move_y() */
enum mvdir	{moveback, movefwd};

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

enum comdmode
{
	CKey,		/* Control-key mode		*/
	FKey		/* Function-key mode	*/
};

extern enum comdmode cmd_mode;		/* Global command mode flag */

enum helptype
{	REFRESH,	/* Flag refresh					*/
	PRIMARY,	/* Initial menu					*/
	QUICK,		/* Quick commands menu			*/
	BLOCK,		/* Block     "      "			*/
	ONSCREEN,	/* Onscreen  "      "			*/
	RBLOCK,		/* Restricted block menu		*/
	FKEYMENU,	/* Function key menu			*/
	SETUPMENU,	/*    "      "  setup menu		*/
	BLOCKMENU,	/*    "      "  block menu		*/
	RFKEYMENU,	/* Restricted function key menu	*/
	RSETUPMENU,	/*    "		setup menu			*/
	RBLOCKMENU,	/*    "     block menu			*/
	HOME		/* Function key HOME- help		*/
};

extern	enum	helptype	helpstate;

enum	directions	{ BACKWARD, FORWARD };

enum	filemodes	{ UNIX, MSDOS, MACINTOSH };
extern	int	autofilemode;

extern	enum	filemodes	filemode;

struct	undelnode {
	char				*text;
	unsigned			size;
	enum directions		direction;
	struct undelnode	*next, *previous;
};

extern	struct	undelnode	*undelptr;

#ifdef	DEBUG
extern	FILE	*debugterm;
#define	DEBUGTERM	"/dev/vt04"
#endif

extern	char	*optarg;	/* 3 for getopt() */
extern	int		optind;
extern	int		opterr;

/* ================== GLOBAL FUNCTION PROTOTYPES ================ */

extern	int		column(), move_x(), move_y(), line_y(), undelsave();
extern	int 	findfunc(), textins(), promptkey(), waitkey();
extern	int		text_move(), delright(), name(), undel(), blkerr();
extern	int		save_fname(), new_line(), load(), replace(), promptst();
extern	int		find(), save();

extern	char	invert(), abandon();
extern	char 	*start_line(), *next_line(), *strtoup();

extern	void	sabort(), rmset(), valerr();
extern	void	display(), TPAErr(), status(), quickhlp(), blockhlp();
extern	void	onscrhlp(), status_line(), infofunc(), savefunc();
extern	void	quitfunc(), exitfunc(), replfunc(), gotoline();
extern	void	keyerr(), notfound(), warning(), q_key(), k_key();
extern	void	o_key(), outchar(), newfile(), readblk(), writeblk();
extern	void	tabset(), move_ptr(), statprint(), edit_init(), edit_exit();
extern	void	dohelp(), blockhlp(), quickhlp(), onscrhlp();
extern	void 	replfunc(), quitfunc(), savefunc(), exitfunc(), infofunc();
extern	void 	CKey_scan(), FKey_scan(), debugpr();
extern	void 	togglehelp(), delword(), delline(), caseinvert(), insertline();
extern	void	repfind(), repaint(), addtext(), delleft();
extern	void	home_key(), setup(), block();
extern	void	reform(), xwprintw();
extern	void	blockfuncs(), setfilemode();
extern	void	marrel(), prompt(), doshell();

#ifdef LOOKBUSY
extern	void	lookbusy();
#endif

/* ===================== GLOBAL MACROS ==================== */

#define wp1(str1) wclrtoeol(editwin); xwprintw(editwin, str1)
#define wp2(str1, str2) wclrtoeol(editwin); xwprintw(editwin, str1, str2)
#define wp3(str1, str2, str3) wclrtoeol(editwin); \
	xwprintw(editwin, str1, str2, str3)
#define wp4(str1, str2, str3, str4) wclrtoeol(editwin); \
	xwprintw(editwin, str1, str2, str3, str4)

