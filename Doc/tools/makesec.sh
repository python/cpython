#!/bin/sh

# Simple little checker for individual libref manual sections
#
# usage: makesec.sh section
#

# This script builds the minimal file necessary to run a single section
# through latex, does so, then converts the resulting dvi file to ps or pdf
# and feeds the result into a viewer.  It's by no means foolproof, but seems
# to work okay for me (knock wood).  It sure beats manually commenting out
# most of the man lib.tex file and running everything manually.

# It attempts to locate an appropriate dvi converter and viewer for the
# selected output format.  It understands the following environment
# variables:
#
#	PYSRC - refers to the root of your build tree (dir containing Doc)
#	DVICVT - refers to a dvi converter like dvips or dvipdf
#	VIEWER - refers to an appropriate viewer for the ps/pdf file
#
# Of the three, only PYSRC is currently required.  The other two can be set
# to specify unusual tools which perform those tasks.

# Known issues:
#   - It would be nice if the script could determine PYSRC on its own.
#   - Something about \seealso{}s blows the latex stack, so they need
#     to be commented out for now.

if [ x$PYSRC = x ] ; then
    echo "PYSRC must refer to the Python source tree" 1>&2
    exit 1
fi

if [ ! -d $PYSRC/Doc ] ; then
    echo "Can't find a Doc subdirectory in $PYSRC" 1>&2
    exit 1
fi

if [ "$#" -ne 1 ] ; then
    echo "Must specify a single libref manual section on cmd line" 1>&2
    exit 1
fi

# settle on a dvi converter
if [ x$DVICVT != x ] ; then
    converter=$DVICVT
    ext=`echo $DVICVT | sed -e 's/^dvi//'`
    result=lib.$ext
elif [ x`which dvipdf` != x ] ; then
    converter=`which dvipdf`
    ext=.pdf
elif [ x`which dvips` != x ] ; then
    converter=`which dvips`
    ext=.ps
else
    echo "Can't find a reasonable dvi converter" 1>&2
    echo "Set DVICVT to refer to one" 1>&2
    exit 1
fi

# how about a viewer?
if [ x$VIEWER != x ] ; then
    viewer=$VIEWER
elif [ $ext = ".ps" -a x`which gv` != x ] ; then
    viewer=gv
elif [ $ext = ".ps" -a x`which gs` != x ] ; then
    viewer=gs
elif [ $ext = ".pdf" -a x`which acroread` != x ] ; then
    viewer=acroread
elif [ $ext = ".pdf" -a "`uname`" = "Darwin" -a x`which open` != x ] ; then
    viewer=open
elif [ $ext = ".pdf" -a x`which acroread` != x ] ; then
    viewer=acroread
else
    echo "Can't find a reasonable viewer" 1>&2
    echo "Set VIEWER to refer to one" 1>&2
    exit 1
fi

# make sure necessary links are in place
for f in howto.cls pypaper.sty ; do
    rm -f $f
    ln -s $PYSRC/Doc/$f
done

export TEXINPUTS=.:$PYSRC/Doc/texinputs:

# strip extension in case they gave full filename
inp=`basename $1 .tex`

# create the minimal framework necessary to run section through latex
tmpf=lib.tex
cat > $tmpf <<EOF
\documentclass{manual}

% NOTE: this file controls which chapters/sections of the library
% manual are actually printed.  It is easy to customize your manual
% by commenting out sections that you are not interested in.

\title{Python Library Reference}

\input{boilerplate}

\makeindex                      % tell \index to actually write the
                                % .idx file
\makemodindex                   % ... and the module index as well.


\begin{document}

\maketitle

\ifhtml
\chapter*{Front Matter\label{front}}
\fi

\input{$inp}
\end{document}
EOF

latex $tmpf

$converter lib

$viewer lib.pdf

rm -f $tmpf howto.cls pypaper.sty *.idx *.syn
rm -f lib.aux lib.log
