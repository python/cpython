#! /bin/sh
#
#  Build one of the simple documents.  This can be used to create the DVI,
#  PDF, or LaTeX "aux" files.  It can accept one of three optional parameters:
#
#  --aux	Create only the LaTeX .aux file
#  --dvi	Create the DeVice Independent output
#  --pdf	Create Adobe PDF output
#
#  If no parameter is given, DVI output is produced.
#
#  One positional parameter is required:  the "base" of the document to
#  format.  For the standard Python documentation, this will be api, ext,
#  lib, mac, ref, or tut.

WORKDIR=`pwd`
cd `dirname $0`/..
srcdir=`pwd`
cd $WORKDIR

latex=latex
aux=''
pdf=''
if [ "$1" = "--pdf" ] ; then
    pdf=true
    latex=pdflatex
    shift 1
elif [ "$1" = "--aux" ] ; then
    aux=true
    shift 1
elif [ "$1" = "--dvi" ] ; then
    shift 1
fi

part=$1; shift 1

TEXINPUTS=$srcdir/$part:$TEXINPUTS
export TEXINPUTS

echo $srcdir'/tools/newind.py >'$part'.ind'
$srcdir/tools/newind.py >$part.ind || exit $?
echo $srcdir'/tools/newind.py modindex >mod'$part'.ind'
$srcdir/tools/newind.py modindex >mod$part.ind || exit $?
echo "$latex $part"
$latex $part || exit $?
if [ ! -f mod$part.idx ] ; then
    echo "Not using module index; removing mod$part.ind"
    rm mod$part.ind || exit $?
fi
if [ "$aux" ] ; then
    # make sure the .dvi isn't interpreted as useful:
    rm $part.dvi || exit $?
else
    if [ -f $part.idx ] ; then
	# using the index
	echo $srcdir'/tools/fix_hack '$part'.idx'
	$srcdir/tools/fix_hack $part.idx || exit $?
	echo 'makeindex -s '$srcdir'/texinputs/python.ist '$part'.idx'
	makeindex -s $srcdir/texinputs/python.ist $part.idx || exit $?
	echo $srcdir'/tools/indfix.py '$part'.ind'
	$srcdir/tools/indfix.py $part.ind || exit $?
    else
	# skipping the index; clean up the unused file
	rm -f $part.ind
    fi
    if [ -f mod$part.idx ] ; then
	# using the index
	echo 'makeindex -s '$srcdir'/texinputs/python.ist mod'$part'.idx'
	makeindex -s $srcdir/texinputs/python.ist mod$part.idx || exit $?
    fi
    if [ "$pdf" ] ; then
	echo $srcdir'/tools/toc2bkm.py '$part
	$srcdir/tools/toc2bkm.py $part || exit $?
    fi
    echo "$latex $part"
    $latex $part || exit $?
fi
