#! /bin/sh
#
#  Script to fix general entities that got translated from the LaTeX as empty
#  elements.  Mostly pretty bogus, but works like a charm!
#
#  Removes the leading XML PI that identifies the XML version, since most of
#  the XML files are not used as top-level documents.

if [ "$1" ]; then
    exec <"$1"
    shift 1
fi

if [ "$1" ]; then
    exec >"$1"
    shift 1
fi

sed '
s|<geq/>|\&ge;|g
s|<leq/>|\&le;|g
s|<geq>|\&ge;|g
s|<leq>|\&le;|g
' || exit $?
