#! /bin/bash

RPM_TOPDIR=/usr/src/redhat

PY_VERSION=`perl -ne 'print "$1\n" if (/PY_VERSION\s*\"(.*)\"/o);' ../../Include/patchlevel.h`
export PY_VERSION

cp beopen-python.spec $RPM_TOPDIR/SPECS/beopen-python-$PY_VERSION.spec
cp BeOpen-Python-Setup.patch $RPM_TOPDIR/SOURCES/BeOpen-Python-$PY_VERSION-Setup.patch

perl -pi -e "s/(%define version).*/\$1 $PY_VERSION/;" $RPM_TOPDIR/SPECS/beopen-python-$PY_VERSION.spec
