# Makefile for the lab3

COMPILER_TYPE= gnu

CC=gcc

PROG=	lab3
SRCS=	lab3.c test3.c keyboard.c timer.c handler.S

CCFLAGS= -Wall

DPADD+=	${LIBDRIVER} ${LIBSYS}
LDADD+=	-ldriver -lsys

LDFLAGS+= -L .

MAN=

.include <bsd.prog.mk>
.include <bsd.gcc.mk>
