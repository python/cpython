#! /bin/sh
#
#  Build one of the simple documents.

WORKDIR=`pwd`
cd `dirname $0`/..
srcdir=`pwd`
cd $WORKDIR

PART=$1

TEXINPUTS=$srcdir/$PART:$TEXINPUTS
export TEXINPUTS

set -x
$srcdir/tools/newind.py >$PART.ind || exit $?
latex $PART || exit $?
if [ -f $PART.idx ] ; then
    # using the index
    $srcdir/tools/fix_hack $*.idx || exit $?
    makeindex -s $srcdir/texinputs/myindex.ist $*.idx || exit $?
else
    # skipping the index; clean up the unused file
    rm -f $PART.ind
fi
latex $PART || exit $?
