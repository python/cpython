#! /bin/sh

#  Script to push docs from my development area to SourceForge, where the
#  update-docs.sh script unpacks them into their final destination.

TARGET=python.sourceforge.net:/home/users/fdrake/tmp

ADDRESSES='python-dev@python.org doc-sig@python.org python-list@python.org'

VERSION=`echo '$Revision$' | sed 's/[$]Revision: \(.*\) [$]/\1/'`
EXTRA=`echo "$VERSION" | sed 's/^[0-9][0-9]*\.[0-9][0-9]*//'`
if [ "$EXTRA" ] ; then
    DOCLABEL="maintenance"
    DOCTYPE="maint"
else
    DOCLABEL="development"
    DOCTYPE="devel"
fi

EXPLANATION=''

if [ "$1" = '-m' ] ; then
    EXPLANATION="$2"
    shift 2
elif [ "$1" ] ; then
    EXPLANATION="`cat $1`"
    shift 1
fi

START="`pwd`"
MYDIR="`dirname $0`"
cd "$MYDIR"
MYDIR="`pwd`"

cd ..

# now in .../Doc/
make --no-print-directory || exit $?
make --no-print-directory bziphtml || exit $?
RELEASE=`grep '^RELEASE=' Makefile | sed 's|RELEASE=||'`
PACKAGE="html-$RELEASE.tar.bz2"
scp "$PACKAGE" tools/update-docs.sh $TARGET/ || exit $?
ssh python.sourceforge.net tmp/update-docs.sh $DOCTYPE $PACKAGE '&&' rm tmp/update-docs.sh || exit $?

Mail -s "[$DOCLABEL doc updates]" $ADDRESSES <<EOF
The development version of the documentation has been updated:

	http://python.sourceforge.net/$DOCTYPE-docs/

$EXPLANATION
EOF
exit $?
