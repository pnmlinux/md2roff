/*
*	md2roff.c
*	A utility to convert markdown documents to troff.
*
*	Nicholas Christopoulos (mailto:nereus@freemail.gr)
*
*	License GPL3+
*	CC: std C99
*	history: 20017-05-08, created
*
*	TROFF Manual
*	http://www.troff.org
*/

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>

/*
 * Loads the `filename` file into memory and return a pointer to its contents.
 * The pointer must freed by the user.
 */
char *loadfile(const char *filename)
{
	int len = -1, c, alloc;
	FILE *fp;
	char *buf;

	if ( filename == NULL ) {
		len = 0;
		alloc = 1024;
		buf = (char *) malloc(alloc);
		while ( (c = fgetc(stdin)) != EOF ) {
			buf[len] = c;
			len ++;
			if ( len >= alloc ) {
				alloc += 1024;
				buf = (char *) realloc(buf, alloc);
				}
			}
		}
	else {
		fp = fopen(filename, "r");
		if ( !fp ) {
			fprintf(stderr, "Unable to open '%s': %s\n", filename, strerror(errno));
			exit(EXIT_FAILURE);
			}
		if ( fseek(fp, 0L, SEEK_END) == -1 ) {
			fprintf(stderr, "fseek failed: %s\n", strerror(errno));
			exit(EXIT_FAILURE);
			}
		if ( (len = ftell(fp)) == -1 ) {
			fprintf(stderr, "ftell failed: %s\n", strerror(errno));
			exit(EXIT_FAILURE);
			}
		if ( fseek(fp, 0L, SEEK_SET) == -1 ) {
			fprintf(stderr, "fseek failed: %s\n", strerror(errno));
			exit(EXIT_FAILURE);
			}
		buf = (char *) malloc(len+1);
		if ( fread(buf, len, 1, fp) == -1 ) {
			fprintf(stderr, "fread failed: %s\n", strerror(errno));
			exit(EXIT_FAILURE);
			}
		buf[len] = '\0';
		fclose(fp);
		}

	return buf;
}

/*
 * adds the string 'str' to buffer 'buf' and returns
 * a pointer to new position in buf.
 */
char *stradd(char *buf, const char *str)
{
	const char *p = str;
	char *d = buf;

	while ( *p )
		*d ++ = *p ++;
	return d;
}

/*
 * prints the whole line of 'src' and returns pointer
 * to the next character (the first of the next line).
 */
const char *println(const char *src)
{
	const char *p = src;

	while ( *p ) {
		putchar(*p);
		p ++;
		if ( *(p-1) == '\n' )
			break;
		}
	return p;
}

/*
 * converts a document to groff_man(7)
 */
void md2roff(const char *docname, const char *source)
{
	const char *p = source, *pnext;
	char	*dest, *d;
	bool	bline = true, bcode = false;
	bool	bold = false, italics = false;
	bool	inside_list = false;
	bool	title_level = 0;

	puts(".\\\" troff document, requires man package");
	puts(".\\\" md2roff file.md | groff -Tutf8 -man | $PAGER");

	if ( strncmp(p, "#TH ", 4) == 0 ) {
		printf(".TH ");
		p = println(p+4);
		}
	else
		printf(".TH %s 7 document %s\n", docname, docname);
	
	dest = (char *) malloc(8192);
	d = dest;
	while ( *p ) {

		// inside code block
		if ( bcode ) {
			if ( strncmp(p, "```", 3) == 0 ) {
				p += 3;
				bcode = false;
				puts(".fi");
				puts(".RE");
				puts(".ft P");
				continue;
				}
			else {
				bool xchg_dot = false;
				if ( *p == '.' ) {
					puts(".cc !");
					xchg_dot = true;
					}
				p = println(p);
				if ( xchg_dot )
					puts("!cc .");
				continue;
				}
			}

		// beginning of line
		if ( bline ) {
			bline = false;
			if ( *p == '\n' ) {
				if ( inside_list ) {
					puts(".LE");
					inside_list = false;
					}
				puts(".P");
				bline = true;
				p ++;
				continue;
				}
			else if ( *p == '#' ) {
				pnext = strchr(p, '\n');
				if ( pnext ) {
					if ( *(pnext-1) != '#' ) {
						int	level = 0;
						while ( *p == '#' ) { level ++; p ++; }
						while ( *p == ' ' || *p == '\t' ) p ++;
						switch ( level ) {
						case 1: printf(".SH "); break;
						case 2: printf(".SH "); break;
						default: printf(".SS ");
							}
						}
					else {
						puts(".br");
						p = println(p);
						puts(".br");
						continue;
						}
					}
				}
			else if ( (*(p+1) == ' ' || *(p+1) == '\t') && (*p == '*' || *p == '+' || *p == '-') ) {
				if ( inside_list ) {
					puts(".LE 1");
					}
				else {
					switch ( *p ) {
					case '-': puts(".DL"); break;
					case '*': puts(".BL"); break;
					default:  puts(".BL");
						}
					inside_list = true;
					}
				puts(".LI");
				p ++;
				continue;
				}
			else if ( isdigit(*p) ) {
				char	num[16], *n;
				const char *pstub = p;

				n = num;
				while ( isdigit(*p) )
					*n ++ = *p ++;
				*n = '\0';
				if ( *p == '.' ) {
					if ( inside_list ) {
						puts("\n.LE 1");
						}
					else {
						puts(".AL");
						inside_list = true;
						}
					printf(".LI\n%s. ", num);
					p ++;
					continue;
					}
				p = pstub;
				}
			else if ( strncmp(p, "```", 3) == 0 ) {
				bcode = true;
				p += 3;
				puts(".ft CR");
				puts(".RS 4");
				puts(".nf");
				continue;
				}
			}

		// in line
		if ( *p == '\n' ) {
			if ( strncmp(p+1, "===", 3) == 0
				|| strncmp(p+1, "---", 3) == 0
				|| strncmp(p+1, "***", 3) == 0 ) {
				p = strchr(p+1, '\n');
				if ( !p )
					return;
				if ( title_level == 0 ) {
					title_level ++;
					printf(".SH ");
					}
				else
					printf(".SH ");
				}

			// flush buffer
			*d = '\0';
			printf("%s\n", dest);
			d = dest;
			bline = true;
			}
		else if (
			( *p == '*' && *(p+1) == '*' ) ||
			( *p == '_' && *(p+1) == '_' ) ) {
			if ( bold ) {
				bold = false;
				d = stradd(d, "\\fP");
				}
			else {
				bold = true;
				d = stradd(d, "\\fB");
				}
			p += 2;
			continue;
			}
		else if ( *p == '*' ||  *p == '_' ) {
			if ( italics ) {
				italics = false;
				d = stradd(d, "\\fP");
				}
			else {
				italics = true;
				d = stradd(d, "\\fI");
				}
			p ++;
			continue;
			}
		else if ( *p == '`' ) { // inline code
			p ++;
			d = stradd(d, "\\f[CR]");
			while ( *p != '`' ) {
				if ( *p == '\0' ) {
					fprintf(stderr, "%s", "inline code (`) didnt closed.");
					exit(EXIT_FAILURE);
					}
				*d ++ = *p ++;
				}
			d = stradd(d, "\\fP");
			}
		else {
			*d = *p;
			d ++;
			}

		p ++;
		}

	free(dest);
}

/*
 */
static char *usage =
"usage: md2roff file1 .. fileN\n";

static char *version ="\
md2roff, version 1.0\n\
Copyright (C) 2017 Nicholas Christopoulos <mailto:nereus@freemail.gr>.\n\
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>.\n\
This is free software: you are free to change and redistribute it.\n\
There is NO WARRANTY, to the extent permitted by law.\n\
";

int main(int argc, char *argv[])
{
	for ( int i = 1; i < argc; i ++ ) {
		if ( argv[i][0] == '-' ) {
			if ( strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0 ) 
				printf("%s", usage);
			else if ( strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0 ) 
				printf("%s", version);
			else 
				fprintf(stderr, "unknown option: [%s]\n", argv[i]);
			}
		else {
			char *buf = loadfile(argv[i]);
			md2roff(argv[i], buf);
			free(buf);
			}
		}

	return EXIT_SUCCESS;
}
