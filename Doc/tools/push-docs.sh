#! /bin/sh

#  Script to push docs from my development area to SourceForge, where the
#  update-docs.sh script unpacks them into their final destination.

TARGET=python.sourceforge.net:/home/users/fdrake

if [ "$1" ] ; then
    scp "$1" $TARGET/python-docs-update.txt || exit $?
fi

START="`pwd`"
MYDIR="`dirname $0`"
cd "$MYDIR"
MYDIR="`pwd`"
HTMLDIR="${HTMLDIR:-html}"

cd "../$HTMLDIR"
make --no-print-directory || exit $?
cd ..
RELEASE=`grep '^RELEASE=' Makefile | sed 's|RELEASE=||'`
make --no-print-directory HTMLDIR="$HTMLDIR" bziphtml
scp "html-$RELEASE.tar.bz2" $TARGET/python-docs-update.tar.bz2
