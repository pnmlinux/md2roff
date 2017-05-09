PREFIX=/usr/local
LIBS = -lc 
CFLAGS = -std=c99

all: md2roff

md2roff: md2roff.c
	$(CC) $(CFLAGS) md2roff.c -o md2roff $(LDFLAGS) $(LIBS)

install: md2roff
	install -m 0755 -s md2roff $(PREFIX)/bin

clean:
	-@rm *.o md2roff
