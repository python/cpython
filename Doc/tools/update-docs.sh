#! /bin/sh

# Script which installs a development snapshot of the documentation
# into the "Python @ SourceForge" website.
#
# The push-docs.sh script pushes this to the SourceForge when needed
# and removes it when done.

if [ -z "$HOME" ] ; then
    HOME=`grep fdrake /etc/passwd | sed 's|^.*:\([^:]*\):[^:]*$|\1|'`
    export HOME
fi

DOCTYPE="$1"
UPDATES="$HOME/tmp/$2"

TMPDIR="$$-docs"

cd /home/groups/python/htdocs || exit $?
mkdir $TMPDIR || exit $?
cd $TMPDIR || exit $?
(bzip2 -dc "$UPDATES" | tar xf -) || exit $?
cd .. || exit $?

if [ -d $DOCTYPE-docs ] ; then
    mv $DOCTYPE-docs $DOCTYPE-temp
fi
mv $TMPDIR $DOCTYPE-docs
rm -rf $DOCTYPE-temp || exit $?
rm "$UPDATES" || exit $?
