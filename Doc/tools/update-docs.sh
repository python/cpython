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

UPDATES="$HOME/tmp/$1"

cd /home/groups/python/htdocs || exit $?
rm -rf devel-docs || exit $?
mkdir devel-docs || exit $?
cd devel-docs || exit $?
(bzip2 -dc "$UPDATES" | tar xf -) || exit $?
rm "$UPDATES" || exit $?
