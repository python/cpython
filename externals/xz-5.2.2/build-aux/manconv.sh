#!/bin/sh
#
###############################################################################
#
# Wrapper for GNU groff to convert man pages to a few formats
#
# Usage: manconv.sh FORMAT [PAPER_SIZE] < in.1 > out.suffix
#
# FORMAT can be ascii, utf8, ps, or pdf. PAPER_SIZE can be anything that
# groff accepts, e.g. a4 or letter. See groff_font(5). PAPER_SIZE defaults
# to a4 and is used only when FORMAT is ps (PostScript) or pdf.
#
# Multiple man pages can be given at once e.g. to create a single PDF file
# with continuous page numbering.
#
###############################################################################
#
# Author: Lasse Collin
#
# This file has been put into the public domain.
# You can do whatever you want with this file.
#
###############################################################################

FORMAT=$1
PAPER=${2-a4}

# Make PostScript and PDF output more readable:
#   - Use 11 pt font instead of the default 10 pt.
#   - Use larger paragraph spacing than the default 0.4v (man(7) only).
FONT=11
PD=0.8

SED_PD="
/^\\.TH /s/\$/\\
.PD $PD/
s/^\\.PD\$/.PD $PD/"

case $FORMAT in
	ascii)
		groff -t -mandoc -Tascii | col -bx
		;;
	utf8)
		groff -t -mandoc -Tutf8 | col -bx
		;;
	ps)
		sed "$SED_PD" | groff -dpaper=$PAPER -t -mandoc \
				-rC1 -rS$FONT -Tps -P-p$PAPER
		;;
	pdf)
		sed "$SED_PD" | groff -dpaper=$PAPER -t -mandoc \
				-rC1 -rS$FONT -Tps -P-p$PAPER | ps2pdf - -
		;;
	*)
		echo 'Invalid arguments' >&2
		exit 1
		;;
esac
