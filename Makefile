# vim: ts=8 sw=8

CCMODE	=32
CC	=ccache gcc -m${CCMODE}
DEFS	=-D_GNU_SOURCE
CFLAGS	=-Os
LDFLAGS	=
LDLIBS	=-lreadline -ltermcap -lm

PREFIX =${HOME}/opt/$(shell uname -m)
MANDIR = $(PREFIX)/man/man1
BINDIR = $(PREFIX)/bin

all:	calc

clean:
	${RM} a.out core* *.o lint tags

tags:	$(wildcard *.h *.c)
	ctags -R .

distclean clobber: clean
	${RM} calc

install: calc calc.1
	install -d ${BINDIR}
	install -c -s calc ${BINDIR}/
	install -d ${MANDIR}
	install -c -m 0644 calc.1 ${MANDIR}/

uninstall:
	${RM} ${BINDIR}/calc
	${RM} ${MANDIR}/calc.1
