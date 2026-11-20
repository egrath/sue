/* #######################################################################
 * data.c - TextStar gap-buffer primitives.
 * purpose:
 *     The low-level text machinery: locate line starts/ends around the
 *     gap (start_line/next_line/new_line), translate buffer positions
 *     to screen columns and back (column/line_y), move the gap to the
 *     cursor (text_move), and move the cursor up/down (move_y/move_x).
 *     Also the undo stack (undel/undel_save) and small text utilities.
 * notes:
 *     A cursor position IS the gap: the text before the cursor ends at
 *     gap_beg, the text after the cursor starts at gap_end. Routines
 *     here set txt_ptr to a desired position and call text_move(),
 *     which shifts bytes across the gap one at a time while keeping
 *     the block markers (blk_beg/blk_end/tblk_beg/tblk_end) from being
 *     swallowed by it. txt_x/txt_y track the cursor's line/column.
 * ####################################################################### */

#include "ts.h"

/* #######################################################################
 * char * start_line (char *line)
 * purpose:
 *     Find the first character of the line that "line" points into,
 *     walking backwards over the gap.
 * returns:
 *     Pointer to the first character of that line (gap_end when the
 *     line starts in the text after the gap).
 * parameters:
 *     char *line: a pointer somewhere into the buffer (either side of
 *     the gap).
 * notes:
 *     Relies on txt_beg holding a newline as the backward scan's
 *     stopper; jumps from gap_beg to gap_end (and back) to cross the
 *     gap instead of walking through it.
 * ####################################################################### */
char * start_line (char *line)
{
	if (line == gap_end)		/* Look at previous char */
	{
		line = gap_beg;
	}
	else
	{
		line--;
	}
	while (*line != '\n')
	{
		if (line != gap_end)
		{
			line--;
		}
		else
		{
			line = gap_beg;
		}
	}
	if (line == gap_beg)
	{
		return (gap_end);
	}
	else
	{
		return (++line);
	}
}
/* #######################################################################
 * char * next_line (char *line)
 * purpose:
 *     Find the first character of the line FOLLOWING the line that
 *     "line" points into, walking forward over the gap. Mirror image
 *     of start_line().
 * returns:
 *     Pointer to the first character of the next line (gap_end when
 *     that line starts in the text after the gap, or at the end of
 *     the text).
 * parameters:
 *     char *line: a pointer somewhere into the buffer.
 * notes:
 *     Walks forward to the next newline, jumping the gap via
 *     gap_beg/gap_end as needed.
 * ####################################################################### */
char * next_line (char *line)
{
	if (line == gap_beg)		/* Look at previous char */
	{
		line = gap_end;
	}
	else
	{
		line++;
	}

	while (*line != '\n')
	{
		if (line != gap_beg)
		{
			line++;
		}
		else
		{
			line = gap_end;
		}
	}

	if (line == gap_beg)
	{
		return (gap_end);
	}
	else
	{
		return (++line);
	}
}
/* #######################################################################
 * int new_line (int ntime)
 * purpose:
 *     Search for the ntime'th start of line, counting from the cursor
 *     position (gap_end): 0 = start of the present line, +n = the
 *     n'th line forward, -n = the n'th line backward. Leaves txt_ptr
 *     on the line start found (or at txt_end when the text ran out).
 * returns:
 *     The actual number of line shifts performed - callers use this
 *     to detect hitting the top or bottom of the text.
 * parameters:
 *     int ntime: signed line count to move by.
 * notes:
 *     Used by display()/move_x() to walk lines without moving the
 *     gap; the caller decides whether to follow with text_move().
 * ####################################################################### */
int new_line (int ntime)
{
	int i;

	if (ntime <= 0)
	{
		i = 1;
		txt_ptr = gap_beg;
		if (*txt_ptr == '\n')
		{
			i = 0;
			if ((ntime == 0) || (txt_ptr <= txt_beg))
			{
				txt_ptr = gap_end;
				return (0);
			}
			txt_ptr--;
		}
		for ( ; i > ntime; i--, txt_ptr--)
		{
			while (*txt_ptr != '\n')
			{
				txt_ptr--;
			}
			if (txt_ptr <= txt_beg)
			{
				txt_ptr = txt_beg + 1;
				return (i - 1);
			}
		}
		txt_ptr += 2;
		return (i);
	}
	else
	{
		/* ntime > 0 */
		txt_ptr = gap_end;
		for (i = 0 ; i < ntime ; i++, txt_ptr++)
		{
			while (*txt_ptr != '\n')
			{
				if (txt_ptr >= txt_end)
				{
					txt_ptr = txt_end;
					return (i);
				}
				txt_ptr++;
			}
		}
		return (i);
	}
}
/* #######################################################################
 * int column (char *line)
 * purpose:
 *     Translate a buffer position into a screen column number,
 *     counting from the start of that position's line: TABs advance
 *     to the next tab stop (tabdef wide), printable characters count
 *     one, and control characters count one only when cntlflag is set
 *     (they are invisible otherwise).
 * returns:
 *     The screen column (1-based) of position "line" on its line.
 * parameters:
 *     char *line: a buffer position (either side of the gap).
 * notes:
 *     Leaves txt_ptr at "line". The gap is skipped transparently
 *     while counting.
 * ####################################################################### */
int column (char *line)
{
	int i;

	txt_ptr = start_line (line);
	i = 1;
	while (txt_ptr < line)
	{
		if (*txt_ptr == '\t')
		{
			i = i - (i-1) % tabdef + tabdef;
		}
		else if (isprint (*txt_ptr))
		{
			i++;
		}
		else if (cntlflag)
		{
			i++;
		}
		if ((txt_ptr >= gap_beg) && (txt_ptr < gap_end))
		{
			txt_ptr = gap_end;
		}
		else
		{
			txt_ptr++;
		}
	}
	return (i);
}
/* #######################################################################
 * int text_move (void)
 * purpose:
 *     Relocate the gap to txt_ptr: shift bytes one at a time from the
 *     far side of the gap to the near side, so that everything from
 *     txt_ptr on sits after the gap. This is how the cursor "moves"
 *     in the gap buffer. While shifting, any block marker the gap
 *     would swallow (blk_beg/blk_end/tblk_beg/tblk_end) is jumped to
 *     the other side of the gap.
 * returns:
 *     The change in line number caused by the move: +1 for each
 *     newline shifted into the text before the gap, -1 for each
 *     shifted back. Callers add it to txt_x.
 * parameters:
 *     None; txt_ptr (set by the caller) selects the destination.
 * notes:
 *     Bounded by txt_beg/txt_end - the gap can never be moved onto
 *     the [EOF] marker or before the start of the text.
 * ####################################################################### */
int text_move (void)
{
	int delx;
	delx = 0;

	/* Forward move */
	while ((gap_end < txt_ptr) && (gap_end < txt_end))
	{
		if (blk_end == gap_end)		/* Jump gap if necessary */
		{
			blk_end = gap_beg + 1;
		}
		if (blk_beg == gap_end)
		{
			blk_beg = gap_beg + 1;
		}
		if (tblk_end == gap_end)
		{
			tblk_end = gap_beg + 1;
		}
		if (tblk_beg == gap_end)
		{
			tblk_beg = gap_beg + 1;
		}
		*++gap_beg  = *gap_end++;
		if (*gap_beg == '\n')
		{
			delx++;
		}
	}

	/* Backward move */
	while ((gap_beg >= txt_ptr) && (gap_beg > txt_beg))
	{
		if (blk_end == gap_beg)		/* Jump gap if necessary */
		{
			blk_end = gap_end - 1;
		}
		if (blk_beg == gap_beg)
		{
			blk_beg = gap_end - 1;
		}
		*--gap_end = *gap_beg--;
		if (*gap_end == '\n')
		{
			delx--;
		}
	}
	return (delx);	/* Return change to line location */
}
/* #######################################################################
 * int line_y (int col)
 * purpose:
 *     Move the cursor horizontally within the current line to screen
 *     column "col" (^Q S/^Q D/arrows all land here): scan the line
 *     from the nearer end, counting TABs to their tab stops and
 *     skipping over a TAB that would land mid-stop. Stops at the end
 *     of the line and - crucially - at txt_end, so the cursor can
 *     never walk behind the [EOF] marker.
 * returns:
 *     The column actually reached (may be less than requested at a
 *     TAB or the line end).
 * parameters:
 *     int col: the desired screen column (1-based; values < 1 count
 *     as 1, so huge values mean "end of line").
 * notes:
 *     If the target column is below the cursor the scan starts at the
 *     line start (start_line), otherwise at the cursor (gap_end).
 *     Moves the gap with text_move() as the last step.
 * ####################################################################### */
int line_y (int col)
{
	char	ii = 0;
	int	i;

	col = max (col, 1);
	if (col < txt_y)
	{
		txt_ptr = start_line (gap_end);
		if (txt_ptr == gap_end)
		{
			return (1);
		}
		i = 1;
	}
	else if (col > txt_y)
	{
		txt_ptr = gap_end;
		i = txt_y;
	}
	else
	{
		return (txt_y);
	}

	/* Do column count from line beginning */
	if (txt_ptr < txt_end)
	{
		/* txt_ptr < txt_end also bounds the loop: the scan must
		 * never walk into the [EOF] marker (e.g. ^Q D on the
		 * marker line) - the cursor stays at the marker start. */
		for ( ; (i < col) && (*txt_ptr != '\n') && (txt_ptr < txt_end) ; txt_ptr++)
		{
			if (*txt_ptr == '\t')
			{
				ii = tabdef - (i-1) % tabdef;
				i  += ii;
			}
			else if (isprint (*txt_ptr))
			{
				i++;
			}
			else if (cntlflag)
			{
				i++;
			}
			if (*txt_ptr != '\t')
			{
				ii = 0;
			}
		}
		if (txt_ptr > txt_end)	/* Belt & braces, like move_y() */
		{
			txt_ptr = txt_end;
		}
		if (i > col)
		{
			col = i - ii;	/* Correct for TAB going over */
			txt_ptr--;
		}
		else
		{
			col = i;
		}
	}
	else
	{
		col = 1;
		txt_ptr = txt_end;
	}
	text_move ();
	return (col);
}
/* #######################################################################
 * int move_x (int line_no)
 * purpose:
 *     Move the cursor vertically to line "line_no" while keeping the
 *     horizontal position: remember txt_y, walk to the target line
 *     with new_line(), set txt_y = 1 and move the gap there, then
 *     restore the column with line_y(old_y) - which honours TABs and
 *     short lines on the way.
 * returns:
 *     The line number actually reached (less than requested when the
 *     text ran out at the top or bottom).
 * parameters:
 *     int line_no: the desired text row (1-based).
 * notes:
 *     The column restore is why the cursor "sticks" to its column
 *     when moving through lines with TABs.
 * ####################################################################### */
int move_x (int line_no)
{
	int old_y;

	if (line_no == txt_x)
	{
		return (txt_x);
	}
	line_no = txt_x + new_line ((int) line_no - txt_x);
	old_y = txt_y;
	txt_y = 1;
	text_move ();		/* Move text, preserving blocks, etc. */
	txt_y = line_y (old_y);

	return (line_no);
}
/* #######################################################################
 * static char test (char n, enum mvmode mode)
 * purpose:
 *     The stop condition for move_y()'s scan loop. In movechar mode it
 *     simply allows one step. In moveword mode it examines the
 *     characters around txt_ptr: the scan stops when a word begins
 *     here (punctuation/space before a word character).
 * returns:
 *     1 = keep scanning, 0 = stop here.
 * parameters:
 *     char n: 0 on the first step (so a single step always happens in
 *     movechar mode), 1 afterwards.
 *     enum mvmode mode: movechar or moveword.
 * notes:
 *     Static helper of move_y(); reads *txt_ptr and *prevptr, never
 *     moves the gap itself.
 * ####################################################################### */
static char test (char n, enum mvmode mode)
{
	char	*prevptr;		/* Pointer to prev. char */

	if (mode == movechar)	/* Character mode */
	{
		return (n < 1);
	}
	else					/* Word mode */
	{
		prevptr = txt_ptr - 1;
		if ((ispunct (*prevptr) && !ispunct (*txt_ptr) && !isspace (*txt_ptr)) ||
			(isspace (*prevptr) && !isspace (*txt_ptr)))
		{
			return (0);
		}
		else
		{
			return (1);
		}
	}
}
/* #######################################################################
 * int move_y (enum mvdir direction, enum mvmode mode)
 * purpose:
 *     Move the cursor up or down (arrow keys, ^X/^E, word jumps):
 *     scan the text via txt_ptr in the requested direction until
 *     test() says stop, counting newlines to keep txt_x in sync. The
 *     result is clamped to the valid text area - the cursor can never
 *     go before the first line or behind the [EOF] marker - and the
 *     gap is moved to the final position.
 * returns:
 *     The new cursor column (via column(gap_end)).
 * parameters:
 *     enum mvdir direction: moveback (up) or movefwd (down).
 *     enum mvmode mode: movechar (one line) or moveword (one word).
 * notes:
 *     A word starts at a non-space/punctuation character preceded by
 *     one. The backward scan also stops at txt_beg + 1 so the cursor
 *     stays on the first text line.
 * ####################################################################### */
int move_y (enum mvdir direction, enum mvmode mode)
{
	char	n;

	if ((direction == moveback) && gap_beg > txt_beg)
	{
		n = 1;
		txt_ptr = gap_beg;
		if (*txt_ptr == '\n')
		{
			txt_x--;
		}
		while ((txt_ptr > txt_beg + 1) && test (n, mode))
		{
			txt_ptr--;
			if (*txt_ptr == '\n')
			{
				txt_x--;
			}
		}
	}
	else if (direction == movefwd)
	{
		n = 0;
		txt_ptr = gap_end;
		if (mode == moveword)	/* only look for word above gap_end */
		{
			if (*txt_ptr == '\n')
			{
				txt_x++;
			}
			txt_ptr++;
		}
		while ((txt_ptr < txt_end) && test (n, mode))
		{
			if (*txt_ptr == '\n')
			{
				txt_x++;
			}
			txt_ptr++;
			n = 1;
		}
	}
	if (txt_ptr <= txt_beg)
	{
		if (txt_beg == gap_beg)
		{
			txt_ptr = gap_end;
		}
		else
		{
			txt_ptr = txt_beg + 1;
		}
	}
	else if (txt_ptr > txt_end)
	{
		txt_ptr = txt_end;
	}
	text_move ();
	return (column (gap_end));
}
/* #######################################################################
 * void move_ptr (void)
 * purpose:
 *     Complete a cursor move that left its destination in txt_ptr:
 *     move the gap there (adding the line delta to txt_x) and compute
 *     the new screen column into txt_y. Does nothing when txt_ptr is
 *     outside the text.
 * returns:
 *     Nothing; txt_ptr/txt_x/txt_y are the outputs.
 * parameters:
 *     None; txt_ptr must have been set by the caller.
 * notes:
 *     The standard last step of positioning routines; safe to call
 *     with an unadjusted txt_ptr since out-of-range positions are
 *     ignored.
 * ####################################################################### */
void move_ptr (void)
{
	if (txt_ptr > txt_beg && txt_ptr <= txt_end)
	{
		txt_x += text_move ();
		txt_y = column (gap_end);
	}
}
/* #######################################################################
 * char * strtoup (char *str)
 * purpose:
 *     Convert a string to UPPER case in place. Used on search strings
 *     before the case-insensitive comparison.
 * returns:
 *     The same pointer, for convenience.
 * parameters:
 *     char *str: NUL-terminated string to convert.
 * ####################################################################### */
char * strtoup (char *str)
{
	char	*s;

	s = str;
	while (*s != '\0')
	{
		*s = toupper (*s);
		s++;
	}
	return str;
}
/* #######################################################################
 * char invert (char c)
 * purpose:
 *     Invert the case of an ASCII letter (^^ command); any other
 *     character is returned unchanged.
 * returns:
 *     The case-inverted character, or c unchanged.
 * parameters:
 *     char c: the character to invert.
 * ####################################################################### */
char invert (char c)
{
	if (isupper (c))
	{
		return tolower (c);
	}
	if (islower (c))
	{
		return toupper (c);
	}
	return c;
}
/* #######################################################################
 * int undel (void)
 * purpose:
 *     Undo one deletion: re-insert the most recently saved text from
 *     the undo stack (via text_ins, so it reappears where it was
 *     deleted) and pop that stack entry, freeing its memory. Repeated
 *     calls walk further back in time - the undo stack is a doubly
 *     linked list (undelptr points at the newest entry).
 * returns:
 *     1 if text was restored, 0 when the stack is empty.
 * parameters:
 *     None.
 * notes:
 *     edit_init() drains the stack with while (undel()); before every
 *     new session.
 * ####################################################################### */
int undel (void)
{
	struct undelnode *nodeptr;

	if (undelptr != NULL)
	{
		text_ins (undelptr->text, undelptr->text + undelptr->size,
			undelptr->direction);
		/* Free previously allocated memory */
		nodeptr	= undelptr;
		undelptr = undelptr->previous;
		free (nodeptr->text);
		free (nodeptr);
		return (1);
	}
	else
	{
		return (0);
	}
}
/* #######################################################################
 * int undel_save (char *txt_start, unsigned txt_size, enum directions undel_direction)
 * purpose:
 *     Save text to the undo stack BEFORE it is deleted: copy the range
 *     [txt_start, txt_start + txt_size) into a fresh undelnode and
 *     link it at the end of the doubly linked list. Deletion routines
 *     call this first and only delete when it returns 1.
 * returns:
 *     1 when the text is safely saved (or there was nothing to save),
 *     0 when memory ran out AND the user answered N to the "delete
 *     anyway?" prompt - the caller then aborts the deletion.
 * parameters:
 *     char *txt_start: first byte of the text range to save.
 *     unsigned txt_size: number of bytes to save.
 *     enum directions undel_direction: BACKWARD or FORWARD - which
 *     side of the cursor the text came from, so undel() can put it
 *     back in the right place.
 * notes:
 *     On malloc failure the user is asked whether to delete anyway;
 *     del_line() style callers additionally restore the cursor when
 *     this returns 0.
 * ####################################################################### */
int undel_save (char *txt_start, unsigned txt_size,
	enum directions undel_direction)
{
	struct undelnode *nodeptr;
	char	*txtptr;
	int	c;

	if (txt_size < 1)
	{
		return (1);
	}

	if (((nodeptr = (struct undelnode *)
			malloc (sizeof (struct undelnode))) != NULL) &&
		((txtptr = malloc (txt_size)) != NULL))
	{
		nodeptr->text = txtptr;
		strncpy (nodeptr->text, txt_start, txt_size);
		nodeptr->size		= txt_size;
		nodeptr->direction	= undel_direction;
		nodeptr->next		= NULL;		/* End of 2x linked list */
		if (undelptr == NULL)			/* Start of 2x linked list */
		{
			nodeptr->previous	= NULL;
		}
		else
		{
			undelptr->next		= nodeptr;
			nodeptr->previous	= undelptr;
		}
		undelptr = nodeptr;
		return (1);
	}
	else
	{
		beep ();
		c = toupper (prompt_key ("Can't save text; delete anyway (Y/N)? N\b"));
		if (c == 'Y')
		{
			wechochar (promptwin, c);
			return (1);
		}
		else
		{
			return (0);
		}
	}
}
