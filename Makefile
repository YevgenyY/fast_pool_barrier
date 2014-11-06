#
# $FreeBSD: src/tools/tools/netmap/Makefile,v 1.1 2011/11/17 12:17:39 luigi Exp $
#
# For multiple programs using a single source file each,
# we can just define 'progs' and create custom targets.
PROGS	=	barrier_test

CLEANFILES = $(PROGS) *~ *.o *.core
NO_MAN=
CFLAGS += -g -Wall -nostdinc -I/usr/include -I/usr/src/sys -I/usr/local/include
CFLAGS += -Wextra -O0

LDFLAGS += -L /usr/local/lib -lpthread
#LDFLAGS += -L /usr/local/lib -lpthread -lc

#.include <bsd.prog.mk>
#.include <bsd.lib.mk>

all: $(PROGS)

barrier_test: main.c pool_barrier.h pool_barrier.o 
	gcc $(CFLAGS) -o barrier_test main.c pool_barrier.o $(LDFLAGS)

clean:
	rm $(CLEANFILES)
