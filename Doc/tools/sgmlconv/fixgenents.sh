#! /bin/sh
#
#  Script to fix general entities that got translated from the LaTeX as empty
#  elements.  Mostly pretty bogus, but works like a charm!

if [ "$1" ]; then
    exec <"$1"
    shift 1
fi

if [ "$1" ]; then
    exec >"$1"
    shift 1
fi

sed '
s|<ABC/>|\&ABC;|g
s|<ASCII/>|\&ASCII;|g
s|<C/>|\&C;|g
s|<Cpp/>|\&Cpp;|g
s|<EOF/>|\&EOF;|g
s|<NULL/>|\&NULL;|g
s|<POSIX/>|\&POSIX;|g
s|<UNIX/>|\&UNIX;|g
s|<e/>|\&bsol;|g
s|<geq/>|\&ge;|g
s|<ldots/>|\&hellip|g
s|<leq/>|\&le;|g
s|---|\&mdash;|g
' || exit $?
