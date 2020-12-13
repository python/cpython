# intlmacosx.m4 serial 5 (gettext-0.18.2)
dnl Copyright (C) 2004-2014, 2016 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.
dnl
dnl This file can be used in projects which are not available under
dnl the GNU General Public License or the GNU Library General Public
dnl License but which still want to provide support for the GNU gettext
dnl functionality.
dnl Please note that the actual code of the GNU gettext library is covered
dnl by the GNU Library General Public License, and the rest of the GNU
dnl gettext package is covered by the GNU General Public License.
dnl They are *not* in the public domain.

dnl Checks for special options needed on Mac OS X.
dnl Defines INTL_MACOSX_LIBS.
AC_DEFUN([gt_INTL_MACOSX],
[
  dnl Check for API introduced in Mac OS X 10.2.
  AC_CACHE_CHECK([for CFPreferencesCopyAppValue],
    [gt_cv_func_CFPreferencesCopyAppValue],
    [gt_save_LIBS="$LIBS"
     LIBS="$LIBS -Wl,-framework -Wl,CoreFoundation"
     AC_LINK_IFELSE(
       [AC_LANG_PROGRAM(
          [[#include <CoreFoundation/CFPreferences.h>]],
          [[CFPreferencesCopyAppValue(NULL, NULL)]])],
       [gt_cv_func_CFPreferencesCopyAppValue=yes],
       [gt_cv_func_CFPreferencesCopyAppValue=no])
     LIBS="$gt_save_LIBS"])
  if test $gt_cv_func_CFPreferencesCopyAppValue = yes; then
    AC_DEFINE([HAVE_CFPREFERENCESCOPYAPPVALUE], [1],
      [Define to 1 if you have the Mac OS X function CFPreferencesCopyAppValue in the CoreFoundation framework.])
  fi
  dnl Check for API introduced in Mac OS X 10.3.
  AC_CACHE_CHECK([for CFLocaleCopyCurrent], [gt_cv_func_CFLocaleCopyCurrent],
    [gt_save_LIBS="$LIBS"
     LIBS="$LIBS -Wl,-framework -Wl,CoreFoundation"
     AC_LINK_IFELSE(
       [AC_LANG_PROGRAM(
          [[#include <CoreFoundation/CFLocale.h>]],
          [[CFLocaleCopyCurrent();]])],
       [gt_cv_func_CFLocaleCopyCurrent=yes],
       [gt_cv_func_CFLocaleCopyCurrent=no])
     LIBS="$gt_save_LIBS"])
  if test $gt_cv_func_CFLocaleCopyCurrent = yes; then
    AC_DEFINE([HAVE_CFLOCALECOPYCURRENT], [1],
      [Define to 1 if you have the Mac OS X function CFLocaleCopyCurrent in the CoreFoundation framework.])
  fi
  INTL_MACOSX_LIBS=
  if test $gt_cv_func_CFPreferencesCopyAppValue = yes || test $gt_cv_func_CFLocaleCopyCurrent = yes; then
    INTL_MACOSX_LIBS="-Wl,-framework -Wl,CoreFoundation"
  fi
  AC_SUBST([INTL_MACOSX_LIBS])
])
