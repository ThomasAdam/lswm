.SUFFIXES: .c .o
.PHONY: clean

VERSION= 0.1

DEBUG= 1

CC?= cc
CFLAGS+= -Wno-format-nonliteral -D_GNU_SOURCE -DBUILD="\"$(VERSION)\"" -DNO_STRTONUM -DNO_STRLCPY -DNO_FGETLN
#LDFLAGS+= -L/usr/local/lib
LIBS+= -lm -lxcb -lxcb-icccm -lxcb-ewmh -lxcb-randr -lX11

ifdef DEBUG
CFLAGS+= -g -ggdb -DDEBUG
CFLAGS+= -Wno-long-long -Wall -W -Wnested-externs -Wformat=2
CFLAGS+= -Wmissing-prototypes -Wstrict-prototypes -Wmissing-declarations
CFLAGS+= -Wwrite-strings -Wshadow -Wpointer-arith -Wsign-compare
CFLAGS+= -Wundef -Wbad-function-cast -Winline -Wcast-align
endif

CPPFLAGS:= -iquote. -I/usr/local/include -Icompat ${CPPFLAGS}
ifdef DEBUG
CFLAGS+= -Wno-pointer-sign
endif

PREFIX?= /usr/local
INSTALL?= install
INSTALLDIR= ${INSTALL} -d
INSTALLBIN= ${INSTALL} -m 555
INSTALLMAN= ${INSTALL} -m 444

SRCS= arguments.c cmd.c cmd-list.c cmd-resize.c cmd-bind.c cmd-queue.c cmd-string.c cfg.c client.c ewmh.c keys.c desktop.c randr.c log.c wrapper-lib.c lswm.c config.h array.h lswm.h compat/fgetln.c compat/queue.h compat/tree.h compat/strlcpy.c compat/strtonum.c
OBJS= $(patsubst %.c,%.o,$(SRCS))
.c.o:
	${CC} ${CPPFLAGS} ${CFLAGS} -c -o $@ $<

all:	lswm

lswm:	${OBJS}
	${CC} ${LDFLAGS} -o lswm ${OBJS} ${LIBS}

clean:
	rm -f *.o compat/*.o *.log core lswm
