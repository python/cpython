#! /bin/sh

#  Script which determines if a new development snapshot of the
#  documentation is available, and unpacks it into the "Python @
#  SourceForge" website.
#
#  A copy of this script is run periodically via cron.

if [ -z "$HOME" ] ; then
    HOME=`grep fdrake /etc/passwd | sed 's|^.*:\([^:]*\):[^:]*$|\1|'`
    export HOME
fi

UPDATES=$HOME/python-docs-update.tar.bz2
INFO=$HOME/python-docs-update.txt

if [ -f "$UPDATES" ] ; then
    cd /home/groups/python/htdocs
    rm -rf devel-docs || exit $?
    mkdir devel-docs || exit $?
    cd devel-docs || exit $?
    (bzip2 -dc "$UPDATES" | tar xf -) || exit $?
    rm "$UPDATES" || exit $?
    if [ -f "$INFO" ] ; then
        EXPLANATION="`cat $INFO`"
    else
        EXPLANATION=''
    fi
    Mail -s '[development doc updates]' \
     python-dev@python.org doc-sig@python.org \
     <<EOF
The development version of the documentation has been updated:

	http://python.sourceforge.net/devel-docs/

$EXPLANATION
EOF
    rm -f $HOME/python-docs-update.txt
fi
