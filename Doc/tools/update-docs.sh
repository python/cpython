#! /bin/sh

# Script which installs a development snapshot of the documentation
# into the development website.
#
# The push-docs.sh script pushes this to the server when needed
# and removes it when done.

if [ -z "$HOME" ] ; then
    HOME=`grep "$LOGNAME" /etc/passwd | sed 's|^.*:\([^:]*\):[^:]*$|\1|'`
    export HOME
fi

DOCTYPE="$1"
UPDATES="$HOME/tmp/$2"

TMPDIR="$$-docs"

cd /ftp/ftp.python.org/pub/www.python.org/dev/doc/ || exit $?
mkdir $TMPDIR || exit $?
cd $TMPDIR || exit $?
(bzip2 -dc "$UPDATES" | tar xf -) || exit $?
cd .. || exit $?

if [ -d $DOCTYPE ] ; then
    mv $DOCTYPE $DOCTYPE-temp
fi
mv $TMPDIR/Python-Docs-* $DOCTYPE
rmdir $TMPDIR
rm -rf $DOCTYPE-temp || exit $?
mv "$UPDATES" python-docs-$DOCTYPE.tar.bz2 || exit $?
