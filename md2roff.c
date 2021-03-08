/*
 *	md2roff.c
 *	A utility to convert markdown documents to troff.
 *
 *	Copyright (C) 2017, Nicholas Christopoulos (mailto:nereus@freemail.gr)
 *
 *	License GPL3+
 *	CC: std C99
 * 	URL: http://github.com/nereusx/md2roff
 * 
 *	history:
 *		20017-05-08, created
 *		20019-02-10, cleanup
 *		20021-02-13, SYNTAX and a few improvements for man-pages
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License.
 *	See LICENSE for details.
 */

#include <stdbool.h>
#include <time.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>

// options
typedef enum { mp_mm, mp_man, mp_mdoc, mp_mom } macropackage_t;
macropackage_t	mpack = mp_man;
int	man_ofc = 0, write_lock = 0, std_q = 1;
typedef struct { const char *wrong, *correct; } dict_line_t;
dict_line_t mdic[] = {
{ "bitmask", "bit mask" },
{ "builtin", "build-in" },
{ "epoch", "Epoch" },
{ "file name", "filename" },
{ "file system", "filesystem" },
{ "host name", "hostname" },
{ "i-node", "inode" },
{ "i-nodes", "inodes" },
{ "lower case", "lowercase" },
{ "lower-case", "lowercase" },
{ "upper case", "uppercase" },
{ "upper-case", "uppercase" },
{ "path name", "pathname" },
{ "pseudo-terminal", "pseudoterminal" },
{ "real time", "real-time" },
{ "realtime", "real-time" },
{ "runtime", "run time" },
{ "super user", "superuser" },
{ "super-user", "superuser" },
{ "super block", "superblock" },
{ "super-block", "superblock" },
{ "time stamp", "timestamp" },
{ "time zone", "timezone" },
{ "userspace", "user space" },
{ "user name", "username" },
{ "x86_64", "x86-64" },
{ "zeroes", "zeros" }, // ?!
{ "32bit", "32-bit" },
{ "Unices", "Unix systems" },
{ "Unixes", "Unix systems" },
{ NULL, NULL } };

/*
 * if 'when' is true, print error message and quit
 */
void panicif(int when, const char *fmt, ...)
{
	char	msg[1024];
	va_list	ap;
	
	va_start(ap, fmt);
	if ( when ) {
		vsnprintf(msg, 1024, fmt, ap);
		fprintf(stderr, "%s [%s]\n", msg, strerror(errno));
		exit(EXIT_FAILURE);
		}
	va_end(ap);
}

/*
 * clone string
 */
char *strdup(const char *s)
{
	char *d = (char *) malloc(strlen(s) + 1); 
    if ( d )
		strcpy(d, s);
    return d;
}

/*
 *	squeeze (& strdup)
 */
char *sqzdup(const char *source)
{
	char *rp, *p, *d;
	int lc = 0;

	rp = malloc(strlen(source) + 1);
	p = (char *) source;
	d = rp;

	while ( isspace(*p) ) p ++;

	while ( *p ) {
		if ( isspace(*p) ) {
			if ( !lc ) {
				lc = 1;
				if ( p > source ) {
					if ( isalnum(*(p-1)) || strchr(",;.)}]", *(p-1)) )
						*d ++ = ' ';
					else {
						char *nc = p;
						while ( isspace(*nc) )
							nc ++;
						if ( isalnum(*nc) )
							*d ++ = ' ';
						}
					}
				}
			}
		else {
			lc = 0;
			*d ++ = *p;
			}
		p ++;
		}

	*d = '\0';
	if ( d > rp ) {
		if ( isspace(*(d - 1)) ) 
			*(d - 1) = '\0';
		}
	
	return rp;
}

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
		panicif((fp = fopen(filename, "r")) == NULL, "Unable to open '%s'", filename);
		panicif((fseek(fp, 0L, SEEK_END) == -1), "fseek failed");
		panicif((len = ftell(fp)) == -1, "ftell failed");
		panicif((fseek(fp, 0L, SEEK_SET) == -1), "fseek failed");
		buf = (char *) malloc(len+1);
		panicif((fread(buf, len, 1, fp) == -1), "fread failed");
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
		if ( !write_lock )
			putchar(*p);
		p ++;
		if ( *(p-1) == '\n' )
			break;
		}
	return p;
}

/*
*	types of elements
*/
enum { none,
		par_end, ln_brk,
		cblock_end, cblock_open,
		li_open, li_end,
		ol_open, ul_open, lst_close,
		man_ref, ol, ul,
		bq_open, bq_close,
		box_open, box_close,
		url_mark,
		new_sh, new_ss, new_s4 };

/*
*	list (enumeration/itemize) stack
*/
#define	MAX_LIST_SIZE	32
int		stk_list[MAX_LIST_SIZE];	// type of list
int		stk_count[MAX_LIST_SIZE];	// counter of item
int		stk_list_p = 0;				// top pointer, always points to first free

/*
*	write the roff code of 'type'
*/
void roff(int type, ...)
{
	va_list	ap;
	char	*title, *link;
	char	punc;

	if ( write_lock ) {
		va_start(ap, type);
		va_end(ap);
		return;
		}
	va_start(ap, type);
	switch ( type ) {

	// new paragraph
	case par_end:
		switch ( mpack ) {
		case mp_mdoc:	puts(".Pp"); break;
		default:		puts(".PP");
			}
		break;

	// line break
	case ln_brk:
		switch ( mpack ) {
		case mp_mom:	puts(".BR"); break;	// or .br or .EL or .LINEBREAK ????
		default:		puts(".br");
			}
		break;

	// link
	case url_mark:
		title = va_arg(ap, char *);
		link = va_arg(ap, char *);
		punc = va_arg(ap, int);
		switch ( mpack ) {
		case mp_man:
			if ( strchr(link, '@') ) {
				if ( strlen(title) && strcmp(title, link) != 0 )
					printf(".MT %s\n%s\n", link, title);
				else
					printf(".MT %s\n", link);
				if ( punc )
					printf(".ME %c\n", punc);
				else
					printf(".ME\n");
				}
			else {
				if ( strlen(title) && strcmp(title, link) != 0 )
					printf(".UR %s\n%s\n", link, title);
				else
					printf(".UR %s\n", link);
				if ( punc )
					printf(".UE %c\n", punc);
				else
					printf(".UE\n");
				}
			break;
		case mp_mdoc:
			if ( strchr(link, '@') )
				printf(".An %s Aq Mt %s\n", title, link);
			else
				printf(".Lk %s \"%s\"\n", link, title);
			break;
		case mp_mm: // there is no such thing...
			printf("%s <%s>\n", title, link);
			break;
		case mp_mom:
			printf("%s \\*[UL]%s\\*[ULX]\n", title, link);
			}
		break;
		
	// cartouche top
	case box_open:
		switch ( mpack ) {
		case mp_mom: puts(".DRH"); break;
		case mp_man: puts(".B"); break;
		default: puts(".FT B");
			}
		break;

	// cartouche bottom
	case box_close:
		switch ( mpack ) {
		case mp_mom: puts(".DRH"); break;
		default: puts(".FT P"); 
			}
		break;

	// code block - begin
	case cblock_open:
		switch ( mpack ) {
		case mp_mom:  printf(".CODE\n"); break;
		case mp_mdoc: printf(".Bd -literal -offset indent\n"); break;
		default: printf(".in +4n\n.EX\n");
			}
		break;

	// code block - end
	case cblock_end:
		switch ( mpack ) {
		case mp_mom:  printf(".CODE OFF\n"); break;
		case mp_mdoc: printf(".Ed\n"); break;
		default: printf(".EE\n.in\n");
			}
		break;

	// ordered list (1..2..3..)
	case ol_open:
		stk_list[stk_list_p] = ol;
		stk_count[stk_list_p] = 1;
		stk_list_p ++;
		switch ( mpack ) {
		case mp_mom:
			switch ( stk_list_p ) {
			case 1: puts(".LIST DIGIT"); break;
			case 2: puts(".LIST ALPHA"); break;
			case 3: puts(".LIST DIGIT"); break;
			case 4:	puts(".LIST alpha"); break;
			default:
				puts(".LIST DIGIT");
				};
			break;
		case mp_mdoc: puts(".Bl -enum -offset indent"); break;
		case mp_mm: puts(".AL");
			}
		break;

	// unordered list (bullets)
	case ul_open:
		stk_list[stk_list_p] = ul;
		stk_count[stk_list_p] = 1;
		stk_list_p ++;
		switch ( mpack ) {
		case mp_mom:
			printf(".LIST %s", ((stk_list_p % 2) ? "BULLET" : "DASH"));
			break;
		case mp_mdoc:
			printf(".Bl -%s -offset indent", ((stk_list_p % 2) ? "bullet" : "dash"));
			break;
		case mp_mm:	puts(".BL");
			}
		break;

	// close list
	case lst_close:
		switch ( mpack ) {
		case mp_mom:  puts(".LIST OFF"); break;
		case mp_mdoc: puts(".El");
			}
		break;

	// list item - begin
	case li_open:
		switch ( mpack ) {
		case mp_mom:  puts(".ITEM"); break;
		case mp_mdoc: puts(".It"); break;
		case mp_man:
			if ( stk_list_p ) {
				if ( stk_list[stk_list_p-1] == ul )
					puts(".IP \\(bu 4");
				else {
					printf(".IP %d. 4\n", stk_count[stk_list_p-1]);
					stk_count[stk_list_p-1] ++;
					}
				}
			break;
		default: puts(".LI");
			}
		break;
		
	// list item - end
	case li_end:
		if ( mpack == mp_mm ) puts(".LE");
		break;

	// new big header/section
	case new_sh:
		switch ( mpack ) {
		case mp_mom:  printf(".HEADING 1 \""); break;
		case mp_mdoc: printf(".Sh "); break;
		default: printf(".SH ");
			}
		break;

	// new medium header/secrtion
	case new_ss:
		switch ( mpack ) {
		case mp_mom:  printf(".HEADING 2 \""); break;
		case mp_mdoc: printf(".Ss "); break;
		default: printf(".SS ");
			}
		break;

	// new small header/secrtion
	case new_s4:
		switch ( mpack ) {
		case mp_mom:  printf(".HEADING 3 \""); break;
		case mp_mdoc: printf(".Ss "); break;
		default: printf(".SS ");
			}
		break;

	// reference to man page
	case man_ref:
		link = va_arg(ap, char *);
		punc = va_arg(ap, int);
		switch ( mpack ) {
		case mp_mdoc: printf(".Xr %s\n", link); break;
		case mp_man: {
			char *tmp = strdup(link);
			char *p = strchr(tmp, ' ');
			if ( p ) {
				*p = '\0';
				printf(".BR %s (%s)", tmp, p+1);
				}
			else
				printf(".BR %s", link);
			free(tmp);
			if ( punc )
				printf("%c\n", punc);
			else
				printf("\n");
			}
			break;
		default: printf("%s\n", link);
			}
		break;
		}
	
	va_end(ap);
}

/*
 *  write buffer and reset
 */
char *flushln(char *d, char *bf)
{
	if ( d > bf ) {
		*d = '\0';
		d = bf;
		while ( isspace(*d) )
			d ++;
		if ( *d ) {
			char *z = sqzdup(d);
			if ( !write_lock ) puts(z);
			free(z);
			}
		}
	return bf;
}

/*
 *	this converts the file 'docname',
 *	that is loaded in 'source', to *-roff.
 */
#define KEY_BSDSYN "SYNTAX:"
static char *month[] = {
"Jan", "Feb", "Mar", "Apr", "May", "Jun",
"Jul", "Aug", "Sep", "Oct", "Nov", "Dec",
NULL };

//
const char *get_man_header(const char *source, char *name, char *section, char *date)
{
	const char *p = source;
	char *d;
	
	while ( isblank(*p) ) p ++;
	d = name;
	while ( *p ) {
		if ( isspace(*p) ) break;
		*d ++ = toupper(*p ++);
		}
	*d = '\0';
	
	while ( isblank(*p) ) p ++;
	d = section;
	while ( *p ) {
		if ( isspace(*p) ) break;
		*d ++ = *p ++;
		}
	*d = '\0';
	
	while ( isblank(*p) ) p ++;
	if ( *p != '\n' ) {
		d = date;
		while ( *p ) {
			if ( isspace(*p) ) break;
			*d ++ = *p ++;
			}
		*d = '\0';
		}
	else {
		time_t tt = time(0);   // get time now
		struct tm *t = localtime(&tt);
		sprintf(date, "%s %d %d", month[t->tm_mon], t->tm_mday,  t->tm_year+1900);
		}
	
	return p;
}

#define dcopy(c) { for ( const char *s = (c); *s; *d ++ = *s ++ ); }
void md2roff(const char *docname, const char *source)
{
	const char *p = source, *pnext, *pstart;
	char	*dest, *d;
	bool	bline = true, bcode = false;
	bool	bold = false, italics = false;
	bool	inside_list = false;
	bool	title_level = 0;
	char	secname[256], appname[64], appsec[64], appdate[64];

	stk_list_p = 0; // reset stack
	secname[0] = '\0';
	dest = (char *) malloc(64*1024);
	d = dest;

	puts(".\\# roff document");
	switch ( mpack ) {
	case mp_mm:
		puts(".do mso m.tmac"); // mm package, AL BL DL LI LE
		break;
	case mp_mdoc:
	case mp_man:
		if ( mpack == mp_mdoc )
			puts(".do mso mdoc.tmac"); // BSD man
		else
			puts(".do mso man.tmac"); // Linux man
		
		while ( isspace(*p) ) p ++;
		if ( p[0] == '#' && isblank(p[1]) ) {
			p = get_man_header(p+2, appname, appsec, appdate);
			if ( mpack == mp_mdoc ) {
				printf(".Dd $Mdocdate: %s $\n", appdate);
				printf(".Dt %s %s\n", appname, appsec);
				printf(".Os\n");
				while ( *p != '\n' ) p ++;
				}
			else { // linux man
				printf(".TH %s %s %s", appname, appsec, appdate);
				if ( *p != '\n' )
					p = println(p);
				else
					printf("\n");
				}
			while ( isspace(*p) ) p ++;
			}
		else { // no header specified
			time_t tt = time(0);   // get time now
			struct tm *t = localtime(&tt);
			
			strcpy(appname, docname);
			if ( mpack == mp_mdoc ) {
				printf(".Dd $Mdocdate: %s %d %d $\n",
					month[t->tm_mon], t->tm_mday,  t->tm_year+1900);
				printf(".Dt %s %s\n", appname, "7");
				printf(".Os\n");
				}
			else {
				printf(".TH %s 7 %d-%02d-%02d document\n", docname,
					t->tm_year+1900, t->tm_mon+1, t->tm_mday);
				}
			}
		break;
	case mp_mom:
		puts(".do mso mom.tmac"); // mom
		printf(".TITLE \"%s\"\n", docname);
		printf(".AUTHOR \"md2roff\"\n");
		printf(".PAPER A4\n");
		printf(".PRINTSTYLE TYPESET\n");
		printf(".START\n");
		break;
		}

	while ( *p ) {

		//////////////////////////////////
		// inside code block
		//////////////////////////////////
		if ( bcode ) {
			d = flushln(d, dest); // we dont care
			
			if ( strncmp(p, "```", 3) == 0 ) { // end of code-block
				p += 3;
				while ( *p != '\n' ) p ++;
				if ( *p == '\n' ) p ++;
				bcode = false;
				roff(cblock_end);
				d = flushln(d, dest);
				continue;
				}
			else {
				bool xchg_dot = false;
				if ( *p == '.' ) {
					if ( mpack == mp_mom )
						puts(".ESC_CHAR !");
					else
						puts(".cc !");
					xchg_dot = true;
					}
				p = println(p);
				if ( xchg_dot ) {
					if ( mpack == mp_mom )
						puts(".ESC_CHAR .");
					else
						puts("!cc .");
					}
				continue;
				}
			}

		//////////////////////////////////
		// ignore escape characters
		if ( *p == '\\' ) {
			p ++;
			switch (*p) {
			case 'n': *d ++ = '\n'; break;
			case 'r': *d ++ = '\r'; break;
			case 't': *d ++ = '\t'; break;
			case 'f': *d ++ = '\f'; break;
			case 'b': *d ++ = '\b'; break;
			case 'a': *d ++ = '\a'; break;
			case 'e': *d ++ = '\033'; break;
			default:
				*d ++ = *p;
				}
			p ++;
			bline = false;
			continue;
			}
		
		//////////////////////////////////
		// beginning of line
		//////////////////////////////////
		if ( bline ) {
			bline = false;
			
			if ( *p == '\n' ) { // empty line
				d = flushln(d, dest);
				
				if ( stk_list_p ) {
					roff(li_end);
					roff(lst_close);
					stk_list_p --;
					}
				roff(par_end);
				bline = true;
				p ++;
				continue;
				}
			else if ( *p == '#' ) { // header
				d = flushln(d, dest);
				
				pnext = strchr(p+1, '\n');
				if ( pnext ) {
					if ( *(pnext-1) != '#' ) {
						int	level = 0;
						while ( *p == '#' ) { level ++; p ++; }
						while ( *p == ' ' || *p == '\t' ) p ++;
						switch ( level ) {
						case 1: roff(new_sh); break; // TH?
						case 2: {
							const char *s;
							char *n;
							for ( s = p, n = secname; *s != '\n'; *n ++ = *s ++ );
							*n = '\0';
							if ( man_ofc ) {
								if ( strcmp(secname, "COPYRIGHT") == 0 \
										|| strcmp(secname, "AUTHOR") == 0 \
										|| strcmp(secname, "HOMEPAGE") == 0 \
										|| strcmp(secname, "REPORTING BUGS") == 0 \
										|| strcmp(secname, "AUTHORS") == 0 )
									write_lock = 1;
								else
									write_lock = 0;
								}
							if ( !write_lock ) roff(new_sh);
							}
							break;
						case 3: roff(new_ss); break;
						case 4:
						default:
							if ( mpack == mp_man ) {
								printf(".TP\n\\fB");
								p = println(p);
								printf("\\fR");
								}
							else
								roff(new_s4);
							continue;
							}
						p = println(p);
						bline = true;
						continue;
						}
					else {
						roff(box_open);
						roff(ln_brk);
						p = println(p);
						roff(ln_brk);
						roff(box_close);
						continue;
						}
					}
				}
			else if ( mpack == mp_man && strcmp(secname, "SYNOPSIS") == 0 && strncmp(p, KEY_BSDSYN, strlen(KEY_BSDSYN)) == 0 ) { // SYNTAX BLOCK (.SY/.YS)
				bool first;
				
				d = flushln(d, dest);
				p += strlen(KEY_BSDSYN);
				dcopy(".SY ");
				while ( isspace(*p) ) p ++;
				while ( *p && *p != '\n' ) *d ++ = *p ++;
				if ( *p ) *d ++ = *p ++;
				while ( *p ) {
					if ( !isblank(*p) ) {
						if ( *p == '\n' )
							break;
						else {
							int mode = 2;
							if ( *p == '-' ) mode = 1;
							if ( mode == 1 )
								dcopy(".OP \\")
							else 
								dcopy(".RI ")
							first = true;
							while ( *p && *p != '\n' ) {
								if ( *p == ' ' && first ) {
									first = false;
									if ( mode == 2 )
										dcopy("\\ ");
									}
								else if ( !first ) {
									if ( *p == ' ' ) {
										dcopy("\\fR\\");
										*d ++ = *p ++;
										dcopy("\\f");
										*d ++ = (mode == 1) ? 'I' : 'R';
										continue;
										}
									else if ( strchr("[].-{}|", *p) ) {
										dcopy("\\f");
										*d ++ = (mode == 1) ? 'B' : 'R';
										while ( strchr("[].-{}|", *p) )
											*d ++ = *p ++;
										dcopy("\\f");
										*d ++ = (mode == 1) ? 'I' : 'R';
										continue;
										}
									}
								*d ++ = *p ++;
								}
							
							if ( *p ) *d ++ = *p ++;
							continue;
							}
						}
					p ++;
					}
				dcopy(".YS");
				*d = '\0';
				puts(dest);
				d = dest;
				continue;
				}
			else if ( mpack == mp_mdoc && strcmp(secname, "SYNOPSIS") == 0 && strncmp(p, KEY_BSDSYN, strlen(KEY_BSDSYN)) == 0 ) { // SYNTAX BLOCK (.Nm)
				d = flushln(d, dest);
				p += strlen(KEY_BSDSYN);
				dcopy(".Nm ");
				while ( isspace(*p) ) p ++;
				while ( *p && *p != '\n' ) *d ++ = *p ++;
				if ( *p ) *d ++ = *p ++;
				while ( *p ) {
					if ( !isblank(*p) ) {
						if ( *p == '\n' )
							break;
						else {
							if ( p[0] == '-' || p[1] == '-' || p[2] == '-' )
								dcopy(".Op ")
							else
								dcopy(".Ar ")
							while ( *p && *p != '\n' ) {
								switch ( *p ) {
								case '-': dcopy(" Fl "); p ++; continue;
								case '[': dcopy(" Oo "); p ++; continue;
								case ']': dcopy(" Oc "); p ++; continue;
								case ' ': dcopy(" Ar "); p ++; continue;
									};
								*d ++ = *p ++;
								}
							if ( *p ) *d ++ = *p ++;
							continue;
							}
						}
					p ++;
					}
				*d = '\0';
				puts(dest);
				d = dest;
				continue;
				}
			else if ( (*(p+1) == ' ' || *(p+1) == '\t')
				&& (*p == '*' || *p == '+' || *p == '-') ) { // unordered list
				d = flushln(d, dest);
				if ( stk_list_p )
					roff(li_end);
				else
					roff(ul_open);
				roff(li_open);
				p ++;
				continue;
				}
			else if ( isdigit(*p) ) { // ordered list
				char	num[16], *n;
				const char *pstub = p;

				n = num;
				while ( isdigit(*p) )
					*n ++ = *p ++;
				*n = '\0';
				if ( *p == '.' ) {
					d = flushln(d, dest);
					if ( stk_list_p )
						roff(li_end);
					else
						roff(ol_open);
					stk_count[stk_list_p-1] = atoi(num);
					roff(li_open);
					p ++;
					while ( *p == ' ' || *p == '\t' ) p ++;
					continue;
					}
				p = pstub;
				}
			else if ( strncmp(p, "```", 3) == 0 ) { // open code-block
				bcode = true;
				p += 3;
				while ( *p != '\n' ) p ++;
				if ( *p == '\n' ) p ++;
				d = flushln(d, dest);
				roff(cblock_open);
				continue;
				}
			} // inside if ( beginning of line )

		//////////////////////////////////
		// in line
		//////////////////////////////////
		if ( *p == '\n' ) {
			if ( strncmp(p+1, "===", 3) == 0
				|| strncmp(p+1, "---", 3) == 0
				|| strncmp(p+1, "***", 3) == 0 ) {
				char rc = *(p+1);
				char	*prevln;
				p = strchr(p+1, '\n');
				if ( !p )
					return;
				if ( d == dest ) {
					p ++;
					continue;
					}

				// this is ruler or section
				*d = '\0';
				prevln = strrchr(dest, '\n');
				if ( prevln ) {
					*prevln = '\0';
					if ( prevln > dest )
						puts(dest);
					prevln ++;
					roff(new_sh);
					printf("%s\n", prevln);
					d = dest;
					}
				else {
					roff(new_sh);
					d = flushln(d, dest);
					}
				
				p ++;
				continue;
				}
			else
				*d ++ = ' ';

			bline = true;
			}
		else if (
			(std_q && (*p == '*' && *(p+1) == '*' ) || ( *p == '_' && *(p+1) == '_' ))
			||
			(!std_q && (*p == '*' && *(p+1) == '*' ))
			||
			(!std_q && (*p == '*' ))
			) { // strong
			if ( bold ) {
				bold = false;
				if ( mpack == mp_mom )
					d = stradd(d, "\\*[PREV]");
				else
					d = stradd(d, "\\fP");
				}
			else {
				char pc = (p > source) ? *(p-1) : ' ';
				if ( strchr("({[,.;`'\" \t\n\r", pc) != NULL ) {
					if ( pc == ';' || pc == ',' || pc == '.' ) *d ++ = ' ';
					bold = true;
					if ( mpack == mp_mom )
						d = stradd(d, "\\*[BD]");
					else
						d = stradd(d, "\\fB");
					}
				else {
					*d ++ = *p;
					if ( p[1] == '*' || p[1] == '_' )
						*d ++ = *(p+1);
					}
				}
			if ( p[1] == '*' || p[1] == '_' )
				p ++;
			p ++;
			continue;
			}
		else if ( // emphasis
				(std_q && (*p == '*' || *p == '_'))
				||
				(!std_q && (*p == '_' && *(p+1) == '_'))
				||
				(!std_q && (*p == '_') )
				) {
		   	// emphasis
			if ( italics ) {
				italics = false;
				if ( mpack == mp_mom )
					d = stradd(d, "\\*[PREV]");
				else
					d = stradd(d, "\\fP");
				}
			else {
				char pc = (p > source) ? *(p-1) : ' ';
				if ( strchr("({[,.;`'\" \t\n\r", pc) != NULL ) {
					if ( pc == ';' || pc == ',' || pc == '.' ) *d ++ = ' ';
					italics = true;
					if ( mpack == mp_mom )
						d = stradd(d, "\\*[IT]");
					else
						d = stradd(d, "\\fI");
					}
				else {
					*d ++ = *p;
					if ( p[1] == '*' || p[1] == '_' )
						*d ++ = *(p+1);
					}
				}
			if ( p[1] == '_' || p[1] == '*' )
				p ++;
			p ++;
			continue;
			}
		else if ( *p == '`' ) { // inline code
			p ++;
			if ( mpack == mp_mom )
				d = stradd(d, "`\\*[CODE]");
			else
				d = stradd(d, "‘\\f[CR]");
			
			while ( *p != '`' ) {
				if ( *p == '\0' ) {
					fprintf(stderr, "%s", "inline code (`) didnt closed.");
					exit(EXIT_FAILURE);
					}
				*d ++ = *p ++;
				}

			if ( mpack == mp_mom )
				d = stradd(d, "\\*[CODE OFF]'");
			else
				d = stradd(d, "\\fP’");
			}

		//
		//	Markdown link:
		//
		//	generic link syntax  [text](link)
		//	image link syntax	![text](link)
		//	man page syntax      [page section](man)
		//
		else if ( *p == '[' || (*p == '!' && *(p+1) == '[') ) { // markdown link
			const char *pfin;
			bool bimg = false;
			if ( *p == '!' ) {
				p ++;
				bimg = true;
				}
			pstart = p + 1;
			pnext = strchr(pstart, ']');
			if ( pnext
					 && ( *(pnext+1) == '(' )
						 && ((pfin = strchr(pnext+2, ')')) != NULL)
			   ) {
				char *left = strdup(pstart);
				char *rght = strdup(pnext+2);
				char *cp, punc = '\0';

				d = flushln(d, dest);
				
				cp = strchr(left, ']');	*cp = '\0';
				cp = strchr(rght, ')');	*cp = '\0';
				if ( strchr(".,)]}", cp[1]) )
					punc = cp[1];

//				if ( bimg ) // RTFM
				if ( strcmp(rght, "man") == 0 )
					roff(man_ref, left, (int) punc);
				else
					roff(url_mark, left, rght, (int) punc);
				
				// finish
				free(left);
				free(rght);
				p = pfin + 1;
				if ( punc )
					p ++;
				continue;
				}
			else {
				*d ++ = *p ++;
				continue;
				}
			}
		else {
			*d = *p;
			d ++;
			}

		p ++;
		}
	d = flushln(d, dest);

	free(dest);
}

/*
 * --- main() ---
 */
static char *usage ="\
usage: md2roff [options] [file1 .. [fileN]]\n\
\t-n, --man\n\t\tuse man package (default)\n\
\t-d, --mdoc\n\t\tuse mdoc package (BSD man-pages)\n\
\t-m, --mm\n\t\tuse mm package\n\
\t-o, --mom\n\t\tuse mom package\n\
\t-z, --man-official\n\t\ttry to be as official as man-pages(7)\n\
\t-q, --non-std-q\n\t\tnon-standard emphasis/strong quotation\n\
\t-h, --help\n\t\tprint this screen\n\
\t-v, --version\n\t\tprint version information\n\
";

static char *version ="\
md2roff, version 1.3\n\
Copyright (C) 2017-2021 Nicholas Christopoulos <mailto:nereus@freemail.gr>.\n\
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>.\n\
This is free software: you are free to change and redistribute it.\n\
There is NO WARRANTY, to the extent permitted by law.\n\
";

int main(int argc, char *argv[])
{
	int files[64];
	int fc = 0;
	
	for ( int i = 1; i < argc; i ++ ) {
		if ( argv[i][0] == '-' ) {
			if ( argv[i][1] == '\0' ) { // read from stdin
				char *buf = loadfile(NULL);
				md2roff("stdin", buf);
				free(buf);
				}
			else if ( strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0 )
				printf("%s", usage);
			else if ( strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0 )
				printf("%s", version);
			else if ( strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--man") == 0 )
				mpack = mp_man;
			else if ( strcmp(argv[i], "-m") == 0 || strcmp(argv[i], "--mm") == 0 )
				mpack = mp_mm;
			else if ( strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--mdoc") == 0 )
				mpack = mp_mdoc;
			else if ( strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--mom") == 0 )
				mpack = mp_mom;
			else if ( strcmp(argv[i], "-z") == 0 || strcmp(argv[i], "--man-official") == 0 )
				man_ofc = 1;
			else if ( strcmp(argv[i], "-q") == 0 || strcmp(argv[i], "--non-std-q") == 0 )
				std_q = 0;
			else
				fprintf(stderr, "unknown option: [%s]\n", argv[i]);
			}
		else {
			files[fc] = i;
			fc ++;
			}
		}
		
	for ( int i = 0; i < fc; i ++ ) {
		char *buf = loadfile(argv[files[i]]);
		md2roff(argv[files[i]], buf);
		free(buf);
		}

	return EXIT_SUCCESS;
}
