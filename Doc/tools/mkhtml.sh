#! /bin/sh
#
#  Drive HTML generation for a Python manual.
#
#  The first arg is required and is the designation for which manual to build;
#  api, ext, lib, ref, or tut.  All other args are passed on to latex2html.

WORKDIR=`pwd`
cd `dirname $0`/..
srcdir=`pwd`
cd $WORKDIR

part=$1; shift 1

TEXINPUTS=$srcdir/$part:$TEXINPUTS
export TEXINPUTS

if [ -d $part ] ; then
    (set -x; rm -f $part/*.html)
fi

set -x

latex2html -init_file $srcdir/perl/l2hinit.perl ${1:+$@} $srcdir/$part/$part

cd $part
$srcdir/tools/node2label.pl *.html
