.SUFFIXES: .c .o
.PHONY: clean

PROG= lswm
DEBUG= -g -ggdb

CFLAGS+= -I${X11BASE}/include
LDADD+= -L${X11BASE}/lib -lm -lX11 -lX11-xcb -lxcb-icccm -lxcb-randr \
		-lxcb-ewmh -lxcb-keysyms

.if DEBUG
CFLAGS+= -DDEBUG
CFLAGS+= -Wno-long-long -Wall -W -Wnested-externs -Wformat=2
CFLAGS+= -Wmissing-prototypes -Wstrict-prototypes -Wmissing-declarations
CFLAGS+= -Wwrite-strings -Wshadow -Wpointer-arith -Wsign-compare
CFLAGS+= -Wundef -Wbad-function-cast -Winline -Wcast-align
CFLAGS+= -Wno-pointer-sign
.endif

SRCS=	arguments.c \
		array.h \
		cfg.c \
		client.c \
		cmd-bind.c \
		cmd-list.c \
		cmd-queue.c \
		cmd-resize.c \
		cmd-string.c \
		cmd.c \
		config.h \
		desktop.c \
		ewmh.c \
		log.c \
		lswm.c \
		lswm.h \
		randr.c \
		wrapper-lib.c

DPADD = ${LIBUTIL}

.include <bsd.prog.mk>
.include <bsd.xconf.mk>

