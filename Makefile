#
# Makefile for Simple Unix Editor (SUE)
# Copyright 1992, Point to Point Communications, Inc.
#
# @(#)$Id: Makefile,v 1.7 1996/12/16 00:54:46 carson Exp $
#
#--------------------- Debug flags ------------------------------
# DFLAGS = -g -DDEBUG
# DFLAGS = -g
#
#--------------------- Compiler Flags ---------------------------
CFLAGS = -O2 -std=c89
#
# Uncomment the following lines for the standard C compiler:
CC = cc
#
# Uncomment the following lines for GNU C compiler
# CC = gcc
#
# Target Directory for "make install"
# BINDIR = /usr/local/bin/
BINDIR = /lbin/
#

################# Do not modify below this line #####################

make	:
	@echo ''
	@echo 'Make what?  You must tell which system to make SUE for:'
	@echo ' make aix    - IBM AIX for RS/6000'
	@echo ' make linux  - Linux with ncurses package'
	@echo ' make svr32  - Generic ATT System V Release 3.2'
	@echo ' make svr4   - Generic USL System V Release 4.0.x'
	@echo ' make ultrix - DEC ULTRIX for DECStation'
	@echo ''

SOURCES = ccommand.c data.c display.c fcommand.c funcs.c sue.c

OBJS = ccommand.o data.o display.o fcommand.o funcs.o sue.o

sue		: ${OBJS}
	${CC} ${CFLAGS} -o sue ${OBJS} ${LFLAGS}

clean	:
	rm ${OBJS}

install :
# Warning - this destroys SCCS id's:
#	mcs -d sue
	strip sue
	mv sue ${BINDIR}

lint	:
	lint -DSUEV32 -DDOCOLOR -u "${SOURCES}"

svr32	:
	make sue "EFLAGS = -DSUEV32 -DDOCOLOR" "LFLAGS = -lcurses"

svr4	:
	make sue "EFLAGS = -DDOCOLOR" "LFLAGS = -lcurses"

aix		:
	make sue "EFLAGS = -DAIX" "LFLAGS = -lcurses"

ultrix	:
	make sue "EFLAGS = -DULTRIX" "LFLAGS = -lcursesX"

linux	:
	make sue "EFLAGS = -DLINUX -DDOCOLOR -I/usr/include/ncurses" "LFLAGS = -lncurses"

# More portable Linux version (no shared libraries):
linuxstatic	:
	make sue "EFLAGS = -DLINUX -DDOCOLOR -I/usr/include/ncurses" "LFLAGS = -lncurses -static"

linuxd	:
	make sue "EFLAGS = -DLINUX -DDOCOLOR -I/local/home/ncurses/ncurses-1.9/src" "LFLAGS = -L/local/home/ncurses/ncurses-1.9/src -lncurses"

linuxdt	:
	make sue "EFLAGS = -DLINUX -DDOCOLOR -DTRACE -I/local/home/ncurses/ncurses-1.9/src" "LFLAGS = -L/local/home/ncurses/ncurses-1.9/src -ldcurses"

.c.o	:
	${CC} -c ${CFLAGS} ${DFLAGS} ${EFLAGS} $<

# Dependencies

ccommand.o : ccommand.c sue.h
data.o     : data.c sue.h
display.o  : display.c sue.h
fcommand.o : fcommand.c sue.h
funcs.o    : funcs.c sue.h version.h
sue.o	   : sue.c sue.h version.h

