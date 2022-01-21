#
#	GNU make
#

prefix  ?= /usr/local
bindir  ?= $(prefix)/bin
mandir  ?= $(prefix)/share/man
man1dir ?= $(mandir)/man1

LIBS   = -lc 
CFLAGS = -std=c99

all: md2roff md2roff.1.gz

md2roff: md2roff.c
	$(CC) $(CFLAGS) md2roff.c -o md2roff $(LDFLAGS) $(LIBS)

md2roff.1.gz: md2roff.md md2roff
	./md2roff --synopsis-style=1 md2roff.md > md2roff.1
	-groff md2roff.1 -Tpdf -man -P -e > md2roff.1.pdf
	gzip -f md2roff.1

install: md2roff md2roff.1.gz
	mkdir -p -m 0755 $(DESTDIR)$(bindir) $(DESTDIR)$(man1dir)
	install -m 0755 -s md2roff $(DESTDIR)$(bindir)
	install -m 0644 md2roff.1.gz $(DESTDIR)$(man1dir)

uninstall:
	-@rm $(DESTDIR)$(bindir)/md2roff
	-@rm $(DESTDIR)$(man1dir)/md2roff.1.gz

clean:
	-@rm *.o md2roff md2roff.1*

