#! /bin/sh

#  Script to push docs from my development area to SourceForge, where the
#  update-docs.sh script unpacks them into their final destination.

START="`pwd`"
MYDIR="`dirname $0`"
cd "$MYDIR"
MYDIR="`pwd`"
HTMLDIR="${HTMLDIR:-html}"

cd "../$HTMLDIR"
make --no-print-directory || exit $?
RELEASE=`grep '^RELEASE=' Makefile | sed 's|RELEASE=||'`
make --no-print-directory HTMLDIR="$HTMLDIR" bziphtml
scp "html-$RELEASE.tar.bz2" python.sourceforge.net:/home/users/fdrake/python-docs-update.tar.bz2
