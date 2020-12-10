#!/bin/sh

###############################################################################
#
# Author: Lasse Collin
#
# This file has been put into the public domain.
# You can do whatever you want with this file.
#
###############################################################################

# The result of using "autoreconf -fi" should be identical to using this
# script. I'm leaving this script here just in case someone finds it useful.

set -e -x

${AUTOPOINT:-autopoint} -f
${LIBTOOLIZE:-libtoolize} -c -f || glibtoolize -c -f
${ACLOCAL:-aclocal} -I m4
${AUTOCONF:-autoconf}
${AUTOHEADER:-autoheader}
${AUTOMAKE:-automake} -acf --foreign
