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

echo $srcdir'/tools/newind.py >'$part'.ind'
$srcdir/tools/newind.py >$part.ind || exit $?
echo "$latex $part"
$latex $part || exit $?
if [ -f $part.idx ] ; then
    # using the index
    echo $srcdir'/tools/fix_hack '$part'.idx'
    $srcdir/tools/fix_hack $part.idx || exit $?
    echo 'makeindex -s '$srcdir'/texinputs/python.ist '$part'.idx'
    makeindex -s $srcdir/texinputs/python.ist $part.idx || exit $?
else
    # skipping the index; clean up the unused file
    rm -f $part.ind
fi
if [ "$pdf" ] ; then
    echo $srcdir'/tools/toc2bkm.py '$part
    $srcdir/tools/toc2bkm.py $part
fi
echo "$latex $part"
$latex $part || exit $?
