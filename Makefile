PREFIX=/usr/local
LIBS = -lc 
CFLAGS = -std=c99

all: md2roff md2roff.1

md2roff: md2roff.c
	$(CC) $(CFLAGS) md2roff.c -o md2roff $(LDFLAGS) $(LIBS)

md2roff.1: md2roff.md md2roff
	./md2roff md2roff.md > md2roff.1

install: md2roff md2roff1
	install -m 0755 -s md2roff $(PREFIX)/bin
	install -m 0644 md2roff.1 $(PREFIX)/share/man/man1

clean:
	-@rm *.o md2roff md2roff.1
