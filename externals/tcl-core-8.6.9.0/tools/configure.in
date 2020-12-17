dnl	This file is an input file used by the GNU "autoconf" program to
dnl	generate the file "configure", which is run to configure the
dnl	Makefile in this directory.
AC_INIT(man2tcl.c)
AC_PREREQ(2.59)

# Recover information that Tcl computed with its configure script.

#--------------------------------------------------------------------
#       See if there was a command-line option for where Tcl is;  if
#       not, assume that its top-level directory is a sibling of ours.
#--------------------------------------------------------------------

DEF_VER=8.6

AC_ARG_WITH(tcl, [  --with-tcl=DIR          use Tcl $DEF_VER binaries from DIR], TCL_BIN_DIR=$withval, TCL_BIN_DIR=`cd ../../tcl$DEF_VER$TCL_PATCH_LEVEL/unix; pwd`)
if test ! -d $TCL_BIN_DIR; then
    AC_MSG_ERROR(Tcl directory $TCL_BIN_DIR doesn't exist)
fi
if test ! -f $TCL_BIN_DIR/tclConfig.sh; then
    AC_MSG_ERROR(There's no tclConfig.sh in $TCL_BIN_DIR;  perhaps you didn't specify the Tcl *build* directory (not the toplevel Tcl directory) or you forgot to configure Tcl?)
fi

. $TCL_BIN_DIR/tclConfig.sh

TCL_WIN_VERSION=$TCL_MAJOR_VERSION$TCL_MINOR_VERSION
AC_SUBST(TCL_WIN_VERSION)
CC=$TCL_CC
AC_SUBST(CC)
AC_SUBST(TCL_VERSION)
AC_SUBST(TCL_PATCH_LEVEL)
AC_SUBST(TCL_SRC_DIR)
AC_SUBST(TCL_BIN_DIR)

AC_OUTPUT(Makefile tcl.hpj)
