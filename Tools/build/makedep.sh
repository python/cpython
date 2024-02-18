#!/usr?/bin/bash -x

obj=$1

src=$(echo $obj | sed -e 's/[.]o$/[.]S/')
if [ -r $src ] ; then
    :
else
    src=$(echo $obj | sed -e 's/[.]o$/[.]c/')
fi

echo $CC $CPPFLAGS -MM -MG -MT $obj $src 1>&2
$CC $CPPFLAGS -MM -MG -MT $obj $src
