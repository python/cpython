#! /bin/sh
#
# Drive HTML generation for a Python manual.
#
# This is probably *not* useful outside of the standard Python documentation.
#
# The first arg is required and is the designation for which manual to build;
# api, ext, lib, ref, or tut.  All other args are passed on to latex2html.

WORKDIR=`pwd`
cd `dirname $0`/..
srcdir=`pwd`
cd $WORKDIR

part=$1; shift 1

TEXINPUTS=$srcdir/$part:$TEXINPUTS
export TEXINPUTS

if [ -d $part ] ; then
    rm -f $part/*.html
fi

echo "latex2html -init_file $srcdir/perl/l2hinit.perl -dir $part" \
 "${1:+$@} $srcdir/$part/$part.tex"
latex2html \
 -init_file $srcdir/perl/l2hinit.perl \
 -dir $part \
 ${1:+$@} \
 $srcdir/$part/$part.tex

echo '(cd '$part'; '$srcdir'/tools/node2label.pl *.html)'
cd $part
$srcdir/tools/node2label.pl *.html
