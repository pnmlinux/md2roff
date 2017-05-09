#TH md2roff 1 2017-05-09

## NAME
md2roff - converts markdown documents to groff_man(7)

## SYNOPSIS
md2roff [OPTION]... [FILE]...

## DESCRIPTION
Converts the input files to groff (with man package) formats and prints
the result to stdout.

### -h, --help
displays a short-help text and exits

### -v, --version
displays the program version, copyright and license information and exists.

## SPECIAL KEYWORDS

If the documents starts with `#TH` then creates the TH command with this.
```
#TH mame section date
```

## AUTOR
Written by Nicholas Christopoulos.

## BUGS
A lot.

## COPYRIGHT
Copyright © 2017 Free Software Foundation, Inc.
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>.
This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.

## SEE ALSO
groff_man(7)
