/*
Project:	SUE, a Simple Unix Editor
Copyright:	SUE is copyright 1992 by Point to Point Communications.
File:		data.c
Routines:	Data manipulation.
Author:		Carson Wilson
				Apt. 1D
				1359 W. Greenleaf, Chicago, IL 60626
				internet:	carson@mcs.com
Notes:		Edit with hard tab stops set to 4.
*/

static	char	rcsid[] = "@(#)$Id: data.c,v 1.3 1995/03/09 13:19:58 carson Exp $";

#include "sue.h"

/*------------------------------------------------------------
 * Return the first character in the line pointed to by line.
 * Check for gap.
 */
char	*start_line(line)
char	*line;
{
	if (line == gap_end)		/* Look at previous char */
		line = gap_beg;
	else
		line--;
	while (*line != '\n')
	{	if (line != gap_end) line--;
		else line = gap_beg;
	}
	if (line == gap_beg)
		return(gap_end);
	else
		return(++line);
}
/* -----------------------------------------------------------------
 * Return the first character in the next line pointed to by line.
 * Check for gap.
 */
char	*next_line(line)
char	*line;
{
	if (line == gap_beg)		/* Look at previous char */
		line = gap_end;
	else
		line++;

	while(*line != '\n')
	{	if (line != gap_beg)
			line++;
		else
			line = gap_end;
	}

	if (line == gap_beg)
		return(gap_end);
	else
		return(++line);
}
/*------------------------------------------------------------
 * Search for the ntime'th start of line starting at the cursor 
 *   position [gap_end].
 * 0 = start of present line
 * +n = ---"---  n'th forward line
 * -n = ---"---  n'th backward line
 *
 * Return actual number of shifts performed.
 * The beginning of the line is in txt_ptr.
 * Counts from the line indicated by gap_end.
 * No check for gap.
 */
int new_line(ntime)
int ntime;
{
	int i;

	if (ntime <= 0)
	{	i = 1;
		txt_ptr = gap_beg;
		if (*txt_ptr == '\n')
		{	i = 0;
			if ((ntime == 0) || (txt_ptr <= txt_beg))
			{	txt_ptr = gap_end;
				return(0);
			}
			txt_ptr--;
		}
		for( ; i > ntime; i--, txt_ptr--)
		{	while(*txt_ptr != '\n')
				txt_ptr--;
			if (txt_ptr <= txt_beg)
			{	txt_ptr = txt_beg + 1;
				return(i - 1);
			}
		}
		txt_ptr += 2;
		return(i);
	}
	else
	{	/* ntime > 0 */
		txt_ptr = gap_end;
		for( i = 0 ; i < ntime ; i++, txt_ptr++ )
		{	while(*txt_ptr != '\n')
			{	if (txt_ptr >= txt_end)
				{	txt_ptr = txt_end;
					return(i);
				}
				txt_ptr++;
			}
		}
		return(i);
	}		
}
/*------------------------------------------------------------*/
/* Return column count to the pointer position, skipping cursor
 * gap.
 */
int column(line)
char	*line;
{
	int i;

	txt_ptr = start_line(line);
	i = 1;
	while(txt_ptr < line)
	{	if (*txt_ptr == '\t')
			i = i - (i-1) % tabdef + tabdef;
		else if (isprint(*txt_ptr))
			i++;
		else if (cntlflag)
			i++;
		if ((txt_ptr >= gap_beg) && (txt_ptr < gap_end))
			txt_ptr= gap_end;
		else
			txt_ptr++;
	}
	return(i);
}
/*------------------------------------------------------------
 * Move the text in one block move while checking for over-running 
 * memory limits.  'txt_ptr' is set by other routines such as
 * new_line(), column(), line_y(), and move_y().
 *
 * Returns +/- change in line location.
 */
int text_move()
{
	int delx;
	delx = 0;

	/* Forward move */
	while((gap_end < txt_ptr) && (gap_end < txt_end))
	{	if (blk_end == gap_end)		/* Jump gap if necessary */
			blk_end = gap_beg + 1;
		if (blk_beg == gap_end)
			blk_beg = gap_beg + 1;
		if (tblk_end == gap_end)
			tblk_end = gap_beg + 1;
		if (tblk_beg == gap_end)
			tblk_beg = gap_beg + 1;
		*++gap_beg  = *gap_end++;
		if (*gap_beg == '\n')
			delx++;
	}

	/* Backward move */
	while((gap_beg >= txt_ptr) && (gap_beg > txt_beg))
	{	if (blk_end == gap_beg)		/* Jump gap if necessary */
			blk_end = gap_end - 1;
		if (blk_beg == gap_beg)
			blk_beg = gap_end-1;
		*--gap_end = *gap_beg--;
		if (*gap_end == '\n')
			delx--;
	}
	return(delx);	/* Return change to line location */
}
/* ------------------------------------------------------------
 * Do a memory move based on screen location, keeping note
 * of TABS and not exceeding \n's.
 */
int line_y(col)
int col;		/* Column # to move cursor to */
{
	char	ii;
	int	i;

	col = max(col, 1);
	if (col < txt_y)
	{	txt_ptr = start_line(gap_end);
		if (txt_ptr == gap_end)
			return(1);
		i = 1;
	}
	else if (col > txt_y)
	{	txt_ptr = gap_end;
		i = txt_y;
	}
	else
		return(txt_y);

	/* Do column count from line beginning */
	if (txt_ptr < txt_end )
	{	for( ; (i < col) && (*txt_ptr != '\n') ; txt_ptr++ )
		{	if (*txt_ptr == '\t')
			{	ii = tabdef - (i-1) % tabdef;
				i  += ii;
			}
			else if (isprint(*txt_ptr))
				i++;
			else if (cntlflag)
				i++;
			if (*txt_ptr != '\t')
				ii = 0;
		}
		if (i > col )
		{	col = i- ii;	/* Correct for TAB going over */
			txt_ptr--;
		}
		else col = i;
	}
	else
	{	col = 1;
		txt_ptr = txt_end;
	}
	text_move();
	return(col);
}
/* ------------------------------------------------------------
 * Move cursor to line_no, positioning to the old cursor 
 * position or nearest tab or end of line, whichever applies. 
 * Return # of line it was possible to move to.
 */
int move_x(line_no)
int line_no;
{
	int old_y;

	if (line_no == txt_x)
		return(txt_x);
	line_no = txt_x + new_line((int) line_no - txt_x);
	old_y = txt_y;
	txt_y = 1;
	text_move();		/* Move text, preserving blocks, etc. */
	txt_y = line_y(old_y);

	return(line_no);
}
/* ------------------------------------------------------------
 *	Logical test for y cursor movement
 */
char	test(n, mode)
char	n;
enum mvmode mode;
{
	char	*prevptr;		/* Pointer to prev. char */

	if (mode == movechar)	/* Character mode */
		return(n < 1);
	else					/* Word mode */
	{	prevptr = txt_ptr - 1;
		if ((ispunct(*prevptr) && !ispunct(*txt_ptr) && !isspace(*txt_ptr)) ||
			(isspace(*prevptr) && !isspace(*txt_ptr)))
			return (0);
		else
			return (1);
	}
}
/* ------------------------------------------------------------
 * Move in memory up or down depending on the mode
 *   direction = moveback or movefwd to indicate direction of move.
 *   mode  = movechar ->  Move 1 Character
 *           moveword ->  Move 1 Word
 *
 * A word is defined as an alphanumeric preceeded by a 
 *   non-alphanumeric.
 * Keep track of new lines as they occur.
 */
int move_y(direction, mode)
enum mvdir direction;
enum mvmode mode;
{
	char	n;

	if ((direction == moveback) && gap_beg > txt_beg)
	{	n = 1;
		txt_ptr = gap_beg;
		if (*txt_ptr == '\n')
			txt_x--;
		while((txt_ptr > txt_beg+1) && test(n, mode))
		{	txt_ptr--;
			if (*txt_ptr == '\n')
				txt_x--;
		}
	}
	else if (direction == movefwd)
	{	n = 0;
		txt_ptr = gap_end;
		if (mode == moveword)	/* only look for word above gap_end */
		{	if (*txt_ptr == '\n')
				txt_x++;
			txt_ptr++;
		}
		while((txt_ptr < txt_end) && test(n, mode))
		{	if (*txt_ptr == '\n')
				txt_x++;
			txt_ptr++;
			n = 1;
		}
	}
	if (txt_ptr <= txt_beg)
	{	if (txt_beg == gap_beg)
			txt_ptr = gap_end;
		else
			txt_ptr = txt_beg+ 1;
	}
	else if (txt_ptr > txt_end)
		txt_ptr = txt_end;
	text_move();
	return(column(gap_end));
}
/* ------------------------------------------------------------
 * Move text indicated by pointer, adjusting x,y cursor positions.
 */
void	move_ptr()
{
	if (txt_ptr > txt_beg && txt_ptr <= txt_end)
	{	txt_x += text_move();
		txt_y = column(gap_end);
	}
}
/*---------------------------------------------------------------------
 * Convert a string to UPPER case given the pointer to
 *   the string [ str ]. 
 * Return pointer to string
 */
char	*strtoup(str)
char	*str;
{
	char	*s;

	s = str;
	while (*s != NULL )
	{	*s = toupper(*s);
		s++;
	}
	return str;
}
/* ------------------------------------------------
 * Invert case of ASCII character
 */
char invert(c)
char c;
{
	if (isupper(c))
		return tolower(c);
	if (islower(c))
		return toupper(c);
	return c;
}
/*-------------------------------------------------
 * Undelete text
 */
int	undel()
{
	struct	undelnode	*nodeptr;

	if (undelptr != NULL)
	{	textins(undelptr->text, undelptr->text + undelptr->size, undelptr->direction);
		/* Free previously allocated memory */
		nodeptr	= undelptr;
		undelptr = undelptr->previous;
		free(nodeptr->text);
		free(nodeptr);
		return(1);
	}
	else
		return (0);
}
/*------------------------------------------------------------
 * Save text to undelete buffer (before deleting)
 */
int	undelsave(txt_start, txt_size, undel_direction)
char		*txt_start;
/*int			txt_size;*/
unsigned	txt_size;
enum		directions	undel_direction;
{
	struct	undelnode	*nodeptr;
	char	*txtptr;
	int	c;

	if (txt_size < 1)
		return (1);

	if (((nodeptr = (struct undelnode *) 
			malloc(sizeof(struct undelnode))) != NULL) &&
		((txtptr = malloc(txt_size)) != NULL))
	{	nodeptr->text = txtptr;
		strncpy(nodeptr->text, txt_start, txt_size);
		nodeptr->size		= txt_size;
		nodeptr->direction	= undel_direction;
		nodeptr->next		= NULL;		/* End of 2x linked list */
		if (undelptr == NULL)			/* Start of 2x linked list */
			nodeptr->previous	= NULL;
		else
		{	undelptr->next		= nodeptr;
			nodeptr->previous	= undelptr;
		}
		undelptr = nodeptr;
		return (1);
	}
	else
	{	beep();
		c = toupper(promptkey("Can't save text; delete anyway (Y/N)? N\b"));
		if (c == 'Y')
		{	wechochar(promptwin,c);
			return (1);
		}
		else
			return (0);
	}
}
