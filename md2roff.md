# md2roff 1 2022-06-26 "NDC-Tools"

## NAME
md2roff \- converts markdown documents to roff (man, mdoc, ms, mm, mom).

## SYNOPSIS
md2roff [{-n[-z]|-d|-m|-o}] [FILE] [-]

## DESCRIPTION
**md2roff** converts the input files to **groff** (with man package) format
and prints the result to **stdout**. If the *FILE* is `-` then it reads
from **stdin**.

## OPTIONS

#### -h, --help
displays a short-help text and exits

#### -v, --version
displays the program version, copyright and license information and exists.

#### -n, --man
use man package (linux man pages, default), see [groff_man 7](man).

#### -s, --ms
use ms package, see [groff_ms 7](man).

#### -m, --mm
use mm package, see [groff_mm 7](man).

#### -d, --mdoc
use mdoc package (BSD man pages), see [groff_mdoc 7](man).

#### -o, --mom
use mom package, see [groff_mom 7](man).

### -pX, --synopsis-style=X
For man-page only.
specify the style of the SYNOPSIS. Where X, 0 = default, 1 = md2roff highlight, 2 = .SY/.OP commands, 3 = .Nm commands.

#### -z, --man-official
try to use rules of [man-pages 7](man).

## NOTES
1. If the documents starts with `# ` then creates the TH command with this;
otherwise there will be a default TH with the file-name. Actually only the
title (name) and section are required. See [man-pages 7](man).
```
... example
# title section date source manual
... sed manual page:
# SED 1 "January 2020" "sed 4.8" "User Commands"
```

2. If you write man page, use `####` for each option to automatically convert
the section to '.TP' as in GNU's manuals. Also this enables 'COMMAND' (see 4)
style for this line only and only in man-page output mode.

3. Use special link type (man) to link man pages.
Example, link to [tcsh 1](man).
```
... link to [tcsh 1](man) ...
```

4. For man modes, there is a special keyword `COMMAND:` in SYNOPSIS
   section that enables the syntax commands (command and options in bold,
   separators/syntax-symbols in no-color, parameters in italics).
   COMMAND takes one text string as parameter which is the full expression of
   the syntax. For multi-line use special character backslash at the end of the line.
   This is same to `--synopsis-style=1`

5. For man and mdoc modes, there is a special keyword `SYNTAX:` in SYNOPSIS
   section that enables the syntax commands (.SY/.OP/.YS). Each line starts with
   `-` translated to .OP otherwise to .RI, except the first which is the command
   name.
   This is same to `--synopsis-style=2`
```
## SYNOPSIS
SYNTAX:
	appname
	-x arg
	-y [arg]
```


## BUGS
A lot. Fix and send.

## EXAMPLES
View a markdown as console man page:
```
$ md2roff mytext.md | groff -Tutf8 -man | $PAGER
```

View a markdown as postscript man page:
```
$ md2roff mytext.md | groff -Tps -man | okular -
```

This document is example, its written in markdown and produce a nice man page.
You can see the man page as exported to _pdf_ (by **groff**) in this directory.

## HOMEPAGE
[https://github.com/nereusx/md2roff](https://github.com/nereusx/md2roff)

## COPYRIGHT
Copyright (C) 2017-2021 Nicholas Christopoulos [nereus@freemail.gr](nereus@freemail.gr).

License GPLv3+: GNU GPL version 3 or later [http://gnu.org/licenses/gpl.html](http://gnu.org/licenses/gpl.html).
This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.

## AUTHOR
Written by Nicholas Christopoulos.

## SEE ALSO
[groff_man 7](man), [man-pages 7](man),
[groff_man 7](man), [groff_mdoc 7](man),
[groff_mm 7](man), [groff_mom 7](man),
[groff_ms 7](man).

