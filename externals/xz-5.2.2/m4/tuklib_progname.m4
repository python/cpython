#
# SYNOPSIS
#
#   TUKLIB_PROGNAME
#
# DESCRIPTION
#
#   Put argv[0] into a global variable progname. On DOS-like systems,
#   modify it so that it looks nice (no full path or .exe suffix).
#
#   This .m4 file is needed allow this module to use glibc's
#   program_invocation_name.
#
# COPYING
#
#   Author: Lasse Collin
#
#   This file has been put into the public domain.
#   You can do whatever you want with this file.
#

AC_DEFUN_ONCE([TUKLIB_PROGNAME], [
AC_REQUIRE([TUKLIB_COMMON])
AC_CHECK_DECLS([program_invocation_name], [], [], [#include <errno.h>])
])dnl
