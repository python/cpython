#! /bin/sh
#
#  Build one of the simple documents.

WORKDIR=`pwd`
cd `dirname $0`/..
srcdir=`pwd`
cd $WORKDIR

latex=latex
if [ "$1" = "--pdf" ] ; then
    pdf=true
    latex=pdflatex
    shift 1
fi

part=$1; shift 1

TEXINPUTS=$srcdir/$part:$TEXINPUTS
export TEXINPUTS

set -x
$srcdir/tools/newind.py >$part.ind || exit $?
$latex $part || exit $?
if [ -f $part.idx ] ; then
    # using the index
    $srcdir/tools/fix_hack $part.idx || exit $?
    makeindex -s $srcdir/texinputs/myindex.ist $part.idx || exit $?
else
    # skipping the index; clean up the unused file
    rm -f $part.ind
fi
if [ "$pdf" ] ; then
    $srcdir/tools/toc2bkm.py $part
fi
$latex $part || exit $?
