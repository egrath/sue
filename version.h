/*
Project:	SUE, a Simple Unix Editor
Copyright:	SUE is copyright 1992 by Point to Point Communications.
File:		version.h
Routines:	Version number and update information.
Author:		Carson Wilson
				internet:	carson@mcs.com
Notes:		Edit with hard tab stops set to 4.
			AIX version is missing several keys (see sue.h).
			AIX curses does not implement colors.
			AIX ibmpc termcap is missing the definition for KEY_PP,
			  which is used to decode the IBMPC Page-Up key.
			  (looks like a typo in the distribution source).
			  It is also missing function key definitions.

ToDo:	^^ key does not work under Linux/ncurses.

*/
#define	suerev "0.7i"
/*
rcsid:      @(#)$Id: version.h,v 1.8 1996/12/17 01:01:11 carson Exp carson $
changes:    Fixed control-z to work with Linux ncurses 1.9.4.

#define	suerev "0.7h"
rcsid:		version.h,v 1.7 1995/03/19 17:01:58 carson Exp carson $
changes:	Removed Makefile dependencies from Makefile.
			Fixed problems in ncurses, so removed most workarounds in sue 
              (hardware scrolling now seems to work).

#define	suerev "0.7g"
rcsid:		version.h,v 1.7 1995/03/19 17:01:58 carson Exp carson $
changes:	Removed Makefile dependencies from Makefile.
			Workaround for several Linux ncurses 1.8.5 bugs in display.c.
			Implemented RCS.

#define	suerev "0.7f"
date:		Sat Dec 31 21:14:25 CST 1994
changes:	Modified Makefile for Linux.

#define	suerev "0.7e"
date:		Wed Jul 20 20:41:28 CDT 1994
changes:	First Linux version

#define	suerev "0.7d"
date:		Tue Apr 13 16:44:50 CDT 1993
changes:	Fixed "New file" messages w/^KD, etc.
notes:		Still needs ^KR !file command.

#define	suerev "0.7c"
date:		Tue Mar 30 16:11:47 CST 1993
changes:	Enter or Space as first character now erase "New File" message
			  without adding text to file.
notes:		Needs "New file" message with ^KD.
			Needs ^KR !file command.

#define	suerev "0.7b"
date:		Sun 21 Feb, 07.33 PM
changes:	First SCCS version.
			Fixed bug which occurred if $SHELL was undefined.
			No longer eats initial keystroke after "New file" warning.

/*
#define suerev "0.7a"
date:		02/14/93
changes:	Fixed bug in promptst which was causing core 
			  dumps under Dell Unix.
			Now traps more signals and attempts to save text if 
			  a core dump in imminent.
			Fixed AIX shell escape tty modes bug.
			Added -sn (set right margin) option.
			  - note - both -sn and -tn cannot be used 
			  concurrently yet - need to implement argv[] 
		 	  vector for 1st getopts call.

#define suerev "0.6b"
date:	12/29/92
changes:
		Fixed problem with ^K!, "command>" as opposed to "command >".

#define suerev "0.6a"
date: 		12/18/92
changes:
		Fine tuned for AIX.
		Implemented global _VIDEO definitions in sue.h (some
		  terminals don't display bold, etc. well; this allows
		  customization).
		Flags no longer displayed in BOLD on color terminals
		  due to bug in curses.  Note that curses REVERSE is unreliable on
		  both color and monochrome monitors.

#define suerev "0.6"
changes:
		Fixed ^M AIX bug.
		Implemented -t (tabs) flag in command line & $SUE 
		  variable.
		Implemented shell escape.  Uses $SHELL variable, if
		  found.
		Implemented F12 if no F5.
		^U/F1 now abort at "Change to:" prompt.
		Implemented shell command import if last character of 
		  shell command is ">".
		Fixed problem with BS erasing block markers.
		Previous search string now retained for ^L command when a 
		  search or replace command is aborted (was being discarded).


#define suerev "0.5b"
changes:	Quashed new wordwrap ghost bug.
		Many tweaks and fixes.
		Initial beta release.
*/
