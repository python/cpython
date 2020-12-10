# getopt.m4 serial 14 (modified version)
dnl Copyright (C) 2002-2006, 2008 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

# The getopt module assume you want GNU getopt, with getopt_long etc,
# rather than vanilla POSIX getopt.  This means your code should
# always include <getopt.h> for the getopt prototypes.

AC_DEFUN([gl_GETOPT_SUBSTITUTE],
[
  AC_LIBOBJ([getopt])
  AC_LIBOBJ([getopt1])
  gl_GETOPT_SUBSTITUTE_HEADER
])

AC_DEFUN([gl_GETOPT_SUBSTITUTE_HEADER],
[
  GETOPT_H=getopt.h
  AC_DEFINE([__GETOPT_PREFIX], [[rpl_]],
    [Define to rpl_ if the getopt replacement functions and variables
     should be used.])
  AC_SUBST([GETOPT_H])
])

AC_DEFUN([gl_GETOPT_CHECK_HEADERS],
[
  if test -z "$GETOPT_H"; then
    AC_CHECK_HEADERS([getopt.h], [], [GETOPT_H=getopt.h])
  fi

  if test -z "$GETOPT_H"; then
    AC_CHECK_FUNCS([getopt_long], [], [GETOPT_H=getopt.h])
  fi

  dnl BSD getopt_long uses a way to reset option processing, that is different
  dnl from GNU and Solaris (which copied the GNU behavior). We support both
  dnl GNU and BSD style resetting of getopt_long(), so there's no need to use
  dnl GNU getopt_long() on BSD due to different resetting style.
  dnl
  dnl With getopt_long(), some BSD versions have a bug in handling optional
  dnl arguments. This bug appears only if the environment variable
  dnl POSIXLY_CORRECT has been set, so it shouldn't be too bad in most
  dnl cases; probably most don't have that variable set. But if we actually
  dnl hit this bug, it is a real problem due to our heavy use of optional
  dnl arguments.
  dnl
  dnl According to CVS logs, the bug was introduced in OpenBSD in 2003-09-22
  dnl and copied to FreeBSD in 2004-02-24. It was fixed in both in 2006-09-22,
  dnl so the affected versions shouldn't be popular anymore anyway. NetBSD
  dnl never had this bug. TODO: What about Darwin and others?
  if test -z "$GETOPT_H"; then
    AC_CHECK_DECL([optreset],
      [AC_DEFINE([HAVE_OPTRESET], 1,
        [Define to 1 if getopt.h declares extern int optreset.])],
      [], [#include <getopt.h>])
  fi

  dnl Solaris 10 getopt doesn't handle `+' as a leading character in an
  dnl option string (as of 2005-05-05). We don't use that feature, so this
  dnl is not a problem for us. Thus, the respective test was removed here.
])

AC_DEFUN([gl_GETOPT_IFELSE],
[
  AC_REQUIRE([gl_GETOPT_CHECK_HEADERS])
  AS_IF([test -n "$GETOPT_H"], [$1], [$2])
])

AC_DEFUN([gl_GETOPT], [gl_GETOPT_IFELSE([gl_GETOPT_SUBSTITUTE])])
