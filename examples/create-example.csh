#!/bin/csh -f

set src = Markdown-1.0.1.md
set bas = $src:r
set fms = ( man mdoc mm mom )

foreach f ( $fms )
	../md2roff --$f  $src > ${bas}.$f
end
