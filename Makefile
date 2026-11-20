#
# Makefile for Simple Unix Editor (SUE)
# Copyright 1992, Point to Point Communications, Inc.
#
#--------------------- Debug flags ------------------------------
# DFLAGS = -g -DDEBUG
# DFLAGS = -g
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
TARGET = ${BUILDDIR}/sue

SOURCES = ${SRCDIR}/command.c ${SRCDIR}/data.c ${SRCDIR}/display.c \
	${SRCDIR}/funcs.c ${SRCDIR}/sue.c

OBJS = ${BUILDDIR}/command.o ${BUILDDIR}/data.o ${BUILDDIR}/display.o \
	${BUILDDIR}/funcs.o ${BUILDDIR}/sue.o

################# Do not modify below this line #####################

# One rule for all platforms: plain "make" builds SUE.
# EFLAGS and LFLAGS are the override points for other targets,
# e.g. make LFLAGS="-lpdcurses" for a DOS/Windows PDCurses port.

.PHONY : sue clean install

sue	: ${TARGET}

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

${BUILDDIR}/command.o : ${SRCDIR}/command.c ${SRCDIR}/sue.h build
	${CC} -c ${CFLAGS} ${DFLAGS} ${EFLAGS} $< -o $@

${BUILDDIR}/data.o : ${SRCDIR}/data.c ${SRCDIR}/sue.h build
	${CC} -c ${CFLAGS} ${DFLAGS} ${EFLAGS} $< -o $@

${BUILDDIR}/display.o : ${SRCDIR}/display.c ${SRCDIR}/sue.h build
	${CC} -c ${CFLAGS} ${DFLAGS} ${EFLAGS} $< -o $@

${BUILDDIR}/funcs.o : ${SRCDIR}/funcs.c ${SRCDIR}/sue.h ${SRCDIR}/version.h build
	${CC} -c ${CFLAGS} ${DFLAGS} ${EFLAGS} $< -o $@

${BUILDDIR}/sue.o : ${SRCDIR}/sue.c ${SRCDIR}/sue.h ${SRCDIR}/version.h build
	${CC} -c ${CFLAGS} ${DFLAGS} ${EFLAGS} $< -o $@
