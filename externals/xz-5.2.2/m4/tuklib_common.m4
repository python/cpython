#
# SYNOPSIS
#
#   TUKLIB_COMMON
#
# DESCRIPTION
#
#   Common checks for tuklib.
#
# COPYING
#
#   Author: Lasse Collin
#
#   This file has been put into the public domain.
#   You can do whatever you want with this file.
#

AC_DEFUN_ONCE([TUKLIB_COMMON], [
AC_REQUIRE([AC_CANONICAL_HOST])
AC_REQUIRE([AC_PROG_CC_C99])
AC_REQUIRE([AC_USE_SYSTEM_EXTENSIONS])
])dnl
