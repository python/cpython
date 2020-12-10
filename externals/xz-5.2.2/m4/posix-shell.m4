# Find a POSIX-conforming shell.

# Copyright (C) 2007-2008 Free Software Foundation, Inc.

# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# Written by Paul Eggert.

# If a POSIX-conforming shell can be found, set POSIX_SHELL and
# PREFERABLY_POSIX_SHELL to it.  If not, set POSIX_SHELL to the
# empty string and PREFERABLY_POSIX_SHELL to '/bin/sh'.

AC_DEFUN([gl_POSIX_SHELL],
[
  AC_CACHE_CHECK([for a shell that conforms to POSIX], [gl_cv_posix_shell],
    [gl_test_posix_shell_script='
       func_return () {
	 (exit [$]1)
       }
       func_success () {
	 func_return 0
       }
       func_failure () {
	 func_return 1
       }
       func_ret_success () {
	 return 0
       }
       func_ret_failure () {
	 return 1
       }
       subshell_umask_sanity () {
	 (umask 22; (umask 0); test $(umask) -eq 22)
       }
       test "[$](echo foo)" = foo &&
       func_success &&
       ! func_failure &&
       func_ret_success &&
       ! func_ret_failure &&
       (set x && func_ret_success y && test x = "[$]1") &&
       subshell_umask_sanity
     '
     for gl_cv_posix_shell in \
	 "$CONFIG_SHELL" "$SHELL" /bin/sh /bin/bash /bin/ksh /bin/sh5 no; do
       case $gl_cv_posix_shell in
         /*)
	   "$gl_cv_posix_shell" -c "$gl_test_posix_shell_script" 2>/dev/null \
	     && break;;
       esac
     done])

  if test "$gl_cv_posix_shell" != no; then
    POSIX_SHELL=$gl_cv_posix_shell
    PREFERABLY_POSIX_SHELL=$POSIX_SHELL
  else
    POSIX_SHELL=
    PREFERABLY_POSIX_SHELL=/bin/sh
  fi
  AC_SUBST([POSIX_SHELL])
  AC_SUBST([PREFERABLY_POSIX_SHELL])
])
