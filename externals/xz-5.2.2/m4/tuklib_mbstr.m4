#
# SYNOPSIS
#
#   TUKLIB_MBSTR
#
# DESCRIPTION
#
#   Check if multibyte and wide character functionality is available
#   for use by tuklib_mbstr_* functions. If not enough multibyte string
#   support is available in the C library, the functions keep working
#   with the assumption that all strings are a in single-byte character
#   set without combining characters, e.g. US-ASCII or ISO-8859-*.
#
#   This .m4 file and tuklib_mbstr.h are common to all tuklib_mbstr_*
#   functions, but each function is put into a separate .c file so
#   that it is possible to pick only what is strictly needed.
#
# COPYING
#
#   Author: Lasse Collin
#
#   This file has been put into the public domain.
#   You can do whatever you want with this file.
#

AC_DEFUN_ONCE([TUKLIB_MBSTR], [
AC_REQUIRE([TUKLIB_COMMON])
AC_FUNC_MBRTOWC
AC_CHECK_FUNCS([wcwidth])
])dnl
