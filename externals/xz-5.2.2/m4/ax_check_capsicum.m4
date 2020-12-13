# -*- Autoconf -*-

# SYNOPSIS
#
#   AX_CHECK_CAPSICUM([ACTION-IF-FOUND[, ACTION-IF-NOT-FOUND]])
#
# DESCRIPTION
#
#   This macro searches for an installed Capsicum header and library,
#   and if found:
#     - AC_DEFINE([HAVE_CAPSICUM]) is called.
#     - AC_DEFINE([HAVE_SYS_CAPSICUM_H]) is called if <sys/capsicum.h>
#       is present (otherwise <sys/capability.h> must be used).
#     - CAPSICUM_LIB is set to the -l option needed to link Capsicum support,
#       and AC_SUBST([CAPSICUM_LIB]) is called.
#     - The shell commands in ACTION-IF-FOUND are run. The default
#       ACTION-IF-FOUND prepends ${CAPSICUM_LIB} into LIBS. If you don't
#       want to modify LIBS and don't need to run any other commands either,
#       use a colon as ACTION-IF-FOUND.
#
#   If Capsicum support isn't found:
#     - The shell commands in ACTION-IF-NOT-FOUND are run. The default
#       ACTION-IF-NOT-FOUND calls AC_MSG_WARN to print a warning that
#       Capsicum support wasn't found.
#
#   You should use autoheader to include a definition for the symbols above
#   in a config.h file.
#
#   Sample usage in a C/C++ source is as follows:
#
#     #ifdef HAVE_CAPSICUM
#     # ifdef HAVE_SYS_CAPSICUM_H
#     #  include <sys/capsicum.h>
#     # else
#     #  include <sys/capability.h>
#     # endif
#     #endif /* HAVE_CAPSICUM */
#
# LICENSE
#
#   Copyright (c) 2014 Google Inc.
#   Copyright (c) 2015 Lasse Collin <lasse.collin@tukaani.org>
#
#   Copying and distribution of this file, with or without modification,
#   are permitted in any medium without royalty provided the copyright
#   notice and this notice are preserved.  This file is offered as-is,
#   without any warranty.

#serial 2

AC_DEFUN([AX_CHECK_CAPSICUM], [
# On FreeBSD >= 11.x and Linux, Capsicum is uses <sys/capsicum.h>.
# If this header is found, it is assumed to be the right one.
capsicum_header_found=no
AC_CHECK_HEADERS([sys/capsicum.h], [capsicum_header_found=yes])
if test "$capsicum_header_found" = no ; then
    # On FreeBSD 10.x Capsicum uses <sys/capability.h>. Such a header exists
    # on Linux too but it describes POSIX.1e capabilities. Look for the
    # declaration of cap_rights_limit to check if <sys/capability.h> is
    # a Capsicum header.
    AC_CHECK_DECL([cap_rights_limit], [capsicum_header_found=yes], [],
                  [#include <sys/capability.h>])
fi

capsicum_lib_found=no
CAPSICUM_LIB=
if test "$capsicum_header_found" = yes ; then
    AC_LANG_PUSH([C])
    # FreeBSD >= 10.x has Capsicum functions in libc.
    AC_LINK_IFELSE([AC_LANG_CALL([], [cap_rights_limit])],
                   [capsicum_lib_found=yes], [])
    # Linux has Capsicum functions in libcaprights.
    AC_CHECK_LIB([caprights], [cap_rights_limit],
                 [CAPSICUM_LIB=-lcaprights
                  capsicum_lib_found=yes], [])
    AC_LANG_POP([C])
fi
AC_SUBST([CAPSICUM_LIB])

if test "$capsicum_lib_found" = yes ; then
    AC_DEFINE([HAVE_CAPSICUM], [1], [Define to 1 if Capsicum is available.])
    m4_default([$1], [LIBS="${CAPSICUM_LIB} $LIBS"])
else
    m4_default([$2], [AC_MSG_WARN([Capsicum support not found])])
fi])
