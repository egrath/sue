#
# Makefile for TextStar (binary "ts", formerly SUE)
# Copyright 1992, Point to Point Communications, Inc.
#
#--------------------- Compiler Flags ---------------------------
CFLAGS = -O2 -std=c89 -Wpedantic -Wall -Werror
EFLAGS = -DUNIX -D_DEFAULT_SOURCE
LFLAGS = -lncurses
CC = cc
#
# Detect the operating system (ULTRIX needs special curses handling)
UNAME   = $(shell uname -s)
ifeq ($(UNAME), ULTRIX)
	CC = gcc
	CFLAGS += -DULTRIX
endif
#
# Target Directory for "make install"
# BINDIR = /usr/local/bin/
BINDIR = /lbin/
#
#--------------------- Directories ------------------------------
SRCDIR = src
BUILDDIR = build
TARGET = ${BUILDDIR}/ts

SOURCES = ${SRCDIR}/command.c ${SRCDIR}/data.c ${SRCDIR}/display.c \
	${SRCDIR}/funcs.c ${SRCDIR}/ts.c

OBJS = ${BUILDDIR}/command.o ${BUILDDIR}/data.o ${BUILDDIR}/display.o \
	${BUILDDIR}/funcs.o ${BUILDDIR}/ts.o

################# Do not modify below this line #####################

# One rule for all platforms: plain "make" builds TextStar.
# EFLAGS and LFLAGS are the override points for other targets,
# e.g. make LFLAGS="-lpdcurses" for a DOS/Windows PDCurses port.

.PHONY : ts clean install gcccheck

ts	: ${TARGET}

# Pedantic second opinion: Homebrew GCC 16 is often more pedantic
# than clang. Syntax/warning gate only - no objects, no binary.
gcccheck :
	gcc-16 -c ${CFLAGS} -Wextra ${EFLAGS} ${SRCDIR}/ts.c -o /dev/null
	gcc-16 -c ${CFLAGS} -Wextra ${EFLAGS} ${SRCDIR}/data.c -o /dev/null
	gcc-16 -c ${CFLAGS} -Wextra ${EFLAGS} ${SRCDIR}/display.c -o /dev/null
	gcc-16 -c ${CFLAGS} -Wextra ${EFLAGS} ${SRCDIR}/funcs.c -o /dev/null
	gcc-16 -c ${CFLAGS} -Wextra ${EFLAGS} ${SRCDIR}/command.c -o /dev/null

${TARGET}	: ${OBJS}
	${CC} ${CFLAGS} -o ${TARGET} ${OBJS} ${LFLAGS}

clean	:
	rm -f ${OBJS} ${TARGET}

install :
	strip ${TARGET}
	mv ${TARGET} ${BINDIR}

build	:
	mkdir -p ${BUILDDIR}

# Dependencies

${BUILDDIR}/command.o : ${SRCDIR}/command.c ${SRCDIR}/ts.h build
	${CC} -c ${CFLAGS} ${EFLAGS} $< -o $@

${BUILDDIR}/data.o : ${SRCDIR}/data.c ${SRCDIR}/ts.h build
	${CC} -c ${CFLAGS} ${EFLAGS} $< -o $@

${BUILDDIR}/display.o : ${SRCDIR}/display.c ${SRCDIR}/ts.h build
	${CC} -c ${CFLAGS} ${EFLAGS} $< -o $@

${BUILDDIR}/funcs.o : ${SRCDIR}/funcs.c ${SRCDIR}/ts.h ${SRCDIR}/version.h build
	${CC} -c ${CFLAGS} ${EFLAGS} $< -o $@

${BUILDDIR}/ts.o : ${SRCDIR}/ts.c ${SRCDIR}/ts.h ${SRCDIR}/version.h build
	${CC} -c ${CFLAGS} ${EFLAGS} $< -o $@
