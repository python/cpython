#! /bin/sh

#  Script to push docs from my development area to SourceForge, where the
#  update-docs.sh script unpacks them into their final destination.

TARGET=python.sourceforge.net:/home/users/fdrake/tmp

ADDRESSES='python-dev@python.org doc-sig@python.org python-list@python.org'

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
ssh python.sourceforge.net tmp/update-docs.sh $PACKAGE '&&' rm tmp/update-docs.sh || exit $?

Mail -s '[development doc updates]' $ADDRESSES <<EOF
The development version of the documentation has been updated:

	http://python.sourceforge.net/devel-docs/

$EXPLANATION
EOF
exit $?
