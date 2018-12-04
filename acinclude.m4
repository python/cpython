# RCW - Based on autoconf c.m4
AC_DEFUN([AC_LANG(C)],
[ac_ext=c
ac_cpp='$CPP $CPPFLAGS'
ac_compile='$CC -c $CFLAGS $CPPFLAGS conftest.$ac_ext >&AS_MESSAGE_LOG_FD'
ac_link='$LD -o conftest$ac_exeext $LDFLAGS $LDFLAGS_STATIC $LDFLAGS_EXE conftest.$ac_objext $LIBS >&AS_MESSAGE_LOG_FD'
ac_compiler_gnu=no
])

AC_DEFUN([AC_LANG(C++)],
[ac_ext=cpp
ac_cpp='$CXXCPP $CPPFLAGS'
ac_compile='$CXX -c $CXXFLAGS $CPPFLAGS conftest.$ac_ext >&AS_MESSAGE_LOG_FD'
ac_link='$LD -o conftest$ac_exeext $LDFLAGS $LDFLAGS_STATIC $LDFLAGS_EXE conftest.$ac_ext $LIBS >&AS_MESSAGE_LOG_FD'
ac_compiler_gnu=$ac_cv_cxx_compiler_gnu
])

# RCW - Based on autoconf lang.m4
#
# _AC_COMPILER_EXEEXT_DEFAULT
# ---------------------------
# Check for the extension used for the default name for executables.
#
# We do this in order to find out what is the extension we must add for
# creating executables (see _AC_COMPILER_EXEEXT's comments).
#
# On OpenVMS 7.1 system, the DEC C 5.5 compiler when called through a
# GNV (gnv.sourceforge.net) cc wrapper, produces the output file named
# `a_out.exe'.
# b.out is created by i960 compilers.
#
# Start with the most likely output file names, but:
# 1) Beware the clever `test -f' on Cygwin, try the DOS-like .exe names
# before the counterparts without the extension.
# 2) The algorithm is not robust to junk in `.', hence go to wildcards
# (conftest.*) only as a last resort.
# Beware of `expr' that may return `0' or `'.  Since this macro is
# the first one in touch with the compiler, it should also check that
# it compiles properly.
#
# The IRIX 6 linker writes into existing files which may not be
# executable, retaining their permissions.  Remove them first so a
# subsequent execution test works.
#
m4_define([_AC_COMPILER_EXEEXT_DEFAULT],
[# Try to create an executable without -o first, disregard a.out.
# It will help us diagnose broken compilers, and finding out an intuition
# of exeext.
AC_MSG_CHECKING([whether the _AC_LANG compiler works])
ac_link_default=`AS_ECHO(["$ac_link"]) | sed ['s/ -o *conftest[^ ]*//']`

# The possible output files:
ac_files="a.out conftest.exe conftest a.exe a_out.exe b.out conftest.*"

ac_rmfiles=
for ac_file in $ac_files
do
  case $ac_file in
    _AC_COMPILER_EXEEXT_REJECT ) ;;
    * ) ac_rmfiles="$ac_rmfiles $ac_file";;
  esac
done
rm -f $ac_rmfiles

rm -f conftest.o conftest.obj
AS_IF([_AC_DO_VAR(ac_compile)],
[for ac_file in conftest.o conftest.obj conftest.*; do
  test -f "$ac_file" || continue;
  case $ac_file in
    _AC_COMPILER_OBJEXT_REJECT ) ;;
    *) ac_cv_objext=`expr "$ac_file" : '.*\.\(.*\)'`
       break;;
  esac
done],
      [_AC_MSG_LOG_CONFTEST
AC_MSG_FAILURE([cannot create object files: cannot compile])])
dnl RCW rm -f conftest.$ac_cv_objext conftest.$ac_ext
AC_SUBST([OBJEXT], [$ac_cv_objext])dnl
ac_objext=$OBJEXT

AS_IF([_AC_DO_VAR(ac_link_default)],
[# Autoconf-2.13 could set the ac_cv_exeext variable to `no'.
# So ignore a value of `no', otherwise this would lead to `EXEEXT = no'
# in a Makefile.  We should not override ac_cv_exeext if it was cached,
# so that the user can short-circuit this test for compilers unknown to
# Autoconf.
for ac_file in $ac_files ''
do
  test -f "$ac_file" || continue
  case $ac_file in
    _AC_COMPILER_EXEEXT_REJECT )
	;;
    [[ab]].out )
	# We found the default executable, but exeext='' is most
	# certainly right.
	break;;
    *.* )
	if test "${ac_cv_exeext+set}" = set && test "$ac_cv_exeext" != no;
	then :; else
	   ac_cv_exeext=`expr "$ac_file" : ['[^.]*\(\..*\)']`
	fi
	# We set ac_cv_exeext here because the later test for it is not
	# safe: cross compilers may not add the suffix if given an `-o'
	# argument, so we may need to know it at that point already.
	# Even if this section looks crufty: it has the advantage of
	# actually working.
	break;;
    * )
	break;;
  esac
done
test "$ac_cv_exeext" = no && ac_cv_exeext=
],
      [ac_file=''])
AS_IF([test -z "$ac_file"],
[AC_MSG_RESULT([no])
_AC_MSG_LOG_CONFTEST
AC_MSG_FAILURE([_AC_LANG compiler cannot create executables], 77)],
[AC_MSG_RESULT([yes])])
AC_MSG_CHECKING([for _AC_LANG compiler default output file name])
AC_MSG_RESULT([$ac_file])
ac_exeext=$ac_cv_exeext
])# _AC_COMPILER_EXEEXT_DEFAULT


# Based on /usr/share/autoconf/autoconf/general.m4
# _AC_LINK_IFELSE_BODY
# --------------------
# Shell function body for _AC_LINK_IFELSE.
m4_define([_AC_LINK_IFELSE_BODY],
[  AS_LINENO_PUSH([$[]1])
  rm -f conftest.$ac_object conftest.$ac_objext conftest$ac_exeext
  AS_IF([_AC_DO_STDERR($ac_compile) && {
	 test -z "$ac_[]_AC_LANG_ABBREV[]_werror_flag" ||
	 test ! -s conftest.err
       } && test -s conftest.$ac_objext &&
         _AC_DO_STDERR($ac_link) && {
	 test -z "$ac_[]_AC_LANG_ABBREV[]_werror_flag" ||
	 test ! -s conftest.err
       } && test -s conftest$ac_exeext && {
	 test "$cross_compiling" = yes ||
	 AS_TEST_X([conftest$ac_exeext])
       }],
      [ac_retval=0],
      [_AC_MSG_LOG_CONFTEST
	ac_retval=1])
  # Delete the IPA/IPO (Inter Procedural Analysis/Optimization) information
  # created by the PGI compiler (conftest_ipa8_conftest.oo), as it would
  # interfere with the next link command; also delete a directory that is
  # left behind by Apple's compiler.  We do this before executing the actions.
  rm -rf conftest.dSYM conftest_ipa8_conftest.oo
  AS_LINENO_POP
  AS_SET_STATUS([$ac_retval])
])# _AC_LINK_IFELSE_BODY
