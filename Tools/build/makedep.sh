#!/usr/bin/bash

obj=$1

src=$(echo $obj | sed -e 's/[.]o$/.S/')
if [ -r $src ] ; then
    :
else
    src=$(echo $obj | sed -e 's/[.]o$/.c/')
fi

$CC $CPPFLAGS -MM -MT $obj $src
