.SUFFIXES: .c .o
.PHONY: clean

VERSION= 0.1

DEBUG= 1

CC?= cc
CFLAGS+= -D_GNU_SOURCE -DBUILD="\"$(VERSION)\""
#LDFLAGS+= -L/usr/local/lib
LIBS+= -lm -lxcb -lxcb-icccm -lxcb-ewmh -lxcb-randr

ifdef DEBUG
CFLAGS+= -g -ggdb -DDEBUG
CFLAGS+= -Wno-long-long -Wall -W -Wnested-externs -Wformat=2
CFLAGS+= -Wmissing-prototypes -Wstrict-prototypes -Wmissing-declarations
CFLAGS+= -Wwrite-strings -Wshadow -Wpointer-arith -Wsign-compare
CFLAGS+= -Wundef -Wbad-function-cast -Winline -Wcast-align
endif

CPPFLAGS:= -iquote. -I/usr/local/include ${CPPFLAGS}
ifdef DEBUG
CFLAGS+= -Wno-pointer-sign
endif

PREFIX?= /usr/local
INSTALL?= install
INSTALLDIR= ${INSTALL} -d
INSTALLBIN= ${INSTALL} -m 555
INSTALLMAN= ${INSTALL} -m 444

SRCS= log.c wrapper-lib.c lswm.c lswm.h 
OBJS= log.o wrapper-lib.o lswm.o
.c.o:
	${CC} ${CPPFLAGS} ${CFLAGS} -c -o $@ $<

all:	lswm

lswm:	${OBJS}
	${CC} ${LDFLAGS} -o lswm ${OBJS} ${LIBS}

clean:
	rm -f *.o *.log core lswm
