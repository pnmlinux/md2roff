# md2roff

A utility to convert markdown documents to Unix \*roff format (man, mdoc, mm and mom packages).

License: GPL v3+
Requires:
* Any C99 compiler

## Status

Alpha but it does the job.

80% : man format
60% : mdoc format
60% : mom format
--- : mm format

## Install

```shell
# git clone https://github.com/nereusx/md2roff.git
# cd md2roff
# make && make install
```

Note: Edit `Makefile` to set the destination directory.

## Usage

Please see the md2roff.md file

Example:
```
% md2roff myfile.md > myfile.man
```

That's all.



