/* #######################################################################
 * ts.h - TextStar, global definitions and declarations.
 * purpose:
 *     The one shared header of the editor: system includes, the curses
 *     setup (with prototype gap-fills for ULTRIX cursesX.h), the
 *     monochrome video attributes, all global variables, the shared
 *     enums and structs, and the prototypes of every function.
 * notes:
 *     TextStar keeps the entire text in one malloc'd GAP BUFFER:
 *
 *         txt_beg          gap_beg    gap_end        txt_end
 *           v                 v          v              v
 *           [text before cursor][.. free ..][text after cursor][EOF]...
 *
 *     The cursor lives in the gap. Inserting shrinks the gap, deleting
 *     grows it, and text_move() relocates it to the cursor position -
 *     so a cursor position is simply the gap position. txt_end points
 *     at the "[EOF]" marker (followed by padding newlines and a NUL)
 *     that terminates the text so it can be treated as a string.
 *     Block markers (blk_beg/blk_end) must jump the gap whenever the
 *     gap moves past them.
 * ####################################################################### */

/* ================== GLOBAL INCLUDES ================= */

#include <limits.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#ifdef ULTRIX
#include <cursesX.h>
/* ULTRIX cursesX.h declares almost nothing: initscr/newwin/subwin/newpad,
 * longname/erasechar/killchar, wgetch/newterm - plus macros. Everything
 * else TextStar calls is prototyped here. */
extern int baudrate (void);
extern int beep (void);
extern int delwin (WINDOW *);
extern int endwin (void);
extern int idlok (WINDOW *, int);
extern int keypad (WINDOW *, int);
extern int mvwprintw (WINDOW *, int, int, ...);
extern int noecho (void);
extern int nonl (void);
extern int overwrite (WINDOW *, WINDOW *);
extern int raw (void);
extern int touchwin (WINDOW *);
extern int waddch (WINDOW *, chtype);
extern int waddstr (WINDOW *, char *);
extern int wattroff (WINDOW *, int);
extern int wattron (WINDOW *, int);
extern int wattrset (WINDOW *, int);
extern int wclear (WINDOW *);
extern int wclrtoeol (WINDOW *);
extern int wdelch (WINDOW *);
extern int wdeleteln (WINDOW *);
extern int werase (WINDOW *);
extern int winsertln (WINDOW *);
extern int wmove (WINDOW *, int, int);
extern int wrefresh (WINDOW *);
#else
#include <curses.h>
#endif
#include <signal.h>

/* ===================== GLOBAL DEFINES =================== */

/* Video attributes (monochrome) */

#define	STATUS_VIDEO	A_UNDERLINE	/* Status line	*/
#define	CONTROL_VIDEO	A_UNDERLINE	/* Control characters */
#define	FLAG_VIDEO		A_NORMAL	/* "<" flags	*/
#define	PROMPT_VIDEO	A_BOLD		/* Prompts	*/
#define	BLOCK_VIDEO		A_BOLD		/* Block text	*/

#ifndef max
#define max(a,b) (((a)>(b))?(a):(b))
#endif

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

#ifndef PATH_MAX
#define PATH_MAX 260
#endif

#ifndef ACS_HLINE
# define ACS_HLINE '-'
#endif

/* ULTRIX curses lacks the extended key codes and wechochar():
 * the emulated codes are never delivered by the terminal, so the
 * affected switch cases simply stay dead on that platform. */
#ifdef ULTRIX
# define KEY_SLEFT 0770		/* Not available, so disable these */
# define KEY_SRIGHT 0771
# define KEY_SUSPEND 0776
# define KEY_HELP 0772
# define KEY_SFIND 0773
# define wechochar(win, ch) (waddch (win, ch), wrefresh (win))
#endif

/* ================= GLOBAL VARIABLES ================= */

extern	int	tabdef;			/* Default tab width */
extern	int	scr_length;		/* Screen height */
extern	int	scr_width ;		/*   "    width */

						/* Screen coordinates */
extern	int scr_ybeg;	/* first column */
extern	int scr_yend;	/* last column */
extern	int scr_xbeg;	/* first text row */
extern	int scr_xend;	/* last text row */

extern	int txt_x;		/* Current cursor row */
extern	int txt_y;		/*   "       "    column */

extern	int rt_marg;	/* Current right margin */

extern	char	*gap_beg, *gap_end;	/* The gap: text before the cursor ends
					 * at gap_beg, text after the cursor
					 * starts at gap_end */
extern	char	*txt_beg, *txt_end;	/* Buffer limits; txt_end points at the
					 * "[EOF]" marker */

extern	char	*txt_ptr;		/* General purpose pointer */

extern	char	*blk_beg, 		/* Marked block: begins at blk_beg, */
				*blk_end,	/* ends before blk_end. tblk_* are  */
				*tblk_beg,	/* the temporaries while the block  */
				*tblk_end;	/* is being moved/resized           */

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
	MODE_UNIX, MODE_MSDOS, MODE_MACINTOSH
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

/* ================== GLOBAL FUNCTION PROTOTYPES ================ */

/* data.c - gap-buffer primitives: cursor positioning, the gap itself,
 * undo-save. See the full documentation at each definition. */
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
extern int undel_save (char *txt_start, unsigned txt_size,
	enum directions undel_direction);

/* display.c - all curses output: status/prompt windows, screen
 * repainting, help screens. */
extern int blk_err (char level);
extern char abandon (void);
extern int wait_key (void);
extern void out_char (char a);
extern void stat_print (int col, char *st1, char *st2, char *st3);
extern void status (int col, char *st1, char *st2, char *st3);
extern void status_line (void);
extern void prompt (int col, char *st1, ...);
extern void warning (char *strptr);
extern int prompt_st (char *st1, char *st2, char *inputst, int inlen);
extern int prompt_key (char *strptr);
extern void display (void);
extern void xw_printw (WINDOW *xwindow, char *string1, ...);
extern void do_help (enum helptype helprequest, WINDOW *hwindow);
extern void val_err (void), not_found (void), key_err (void), tpa_err (void);

/* funcs.c - the editor commands: find/replace, reformatting, file
 * I/O, block operations, deletions, shell escape. */
extern void block_hlp (void), quick_hlp (void), onscr_hlp (void);
extern int find_func (void);
extern void repl_func (void), quit_func (void), save_func (void), exit_func (void);
extern void info_func (void);
extern int del_right (void);
extern int name (void);
extern int save_fname (void);
extern int find (void);
extern int replace (void);
extern void goto_line (void), tab_set (void), rm_set (void);
extern void reform (void);
extern void ts_abort (int sig);
extern void writeblk (void), readblk (void), new_file (void);
extern int save (char *s_name, char *s_type);
extern int load (char *l_name);
extern void set_file_mode (void), mar_rel (void);
extern void block_funcs (int funcno);
extern void toggle_help (void);
extern void rep_find (void), case_invert (void);
extern void del_word (void), del_line (void), insert_newline (void);
extern void repaint (void), add_text (int ch), del_left (void);
extern int text_ins (char *ins_beg, char *ins_end,
	enum directions direction);
extern void do_shell (void);

/* ts.c - program startup, buffer allocation, exit path. */
extern void edit_init (char *v_name);
extern void edit_exit (int exparm);

/* command.c - key dispatch: the main loop and the ^Q/^K/^O menus. */
extern void key_scan (void);
extern void q_key (void), k_key (void), o_key (void);

/* ===================== GLOBAL MACROS ==================== */

#define wp1(str1) wclrtoeol (editwin); xw_printw (editwin, str1)
#define wp2(str1, str2) wclrtoeol (editwin); xw_printw (editwin, str1, str2)
#define wp3(str1, str2, str3) wclrtoeol (editwin); \
	xw_printw (editwin, str1, str2, str3)
#define wp4(str1, str2, str3, str4) wclrtoeol (editwin); \
	xw_printw (editwin, str1, str2, str3, str4)
