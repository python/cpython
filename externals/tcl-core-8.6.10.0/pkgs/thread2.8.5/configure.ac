#!/bin/bash -norc
dnl	This file is an input file used by the GNU "autoconf" program to
dnl	generate the file "configure", which is run during Tcl installation
dnl	to configure the system for the local environment.

#-----------------------------------------------------------------------
# Sample configure.ac for Tcl Extensions.  The only places you should
# need to modify this file are marked by the string __CHANGE__
#-----------------------------------------------------------------------

#-----------------------------------------------------------------------
# __CHANGE__
# Set your package name and version numbers here.
#
# This initializes the environment with PACKAGE_NAME and PACKAGE_VERSION
# set as provided.  These will also be added as -D defs in your Makefile
# so you can encode the package version directly into the source files.
#-----------------------------------------------------------------------

AC_INIT([thread], [2.8.5])

#--------------------------------------------------------------------
# Call TEA_INIT as the first TEA_ macro to set up initial vars.
# This will define a ${TEA_PLATFORM} variable == "unix" or "windows"
# as well as PKG_LIB_FILE and PKG_STUB_LIB_FILE.
#--------------------------------------------------------------------

TEA_INIT()

AC_CONFIG_AUX_DIR(tclconfig)

#--------------------------------------------------------------------
# Load the tclConfig.sh file
#--------------------------------------------------------------------

TEA_PATH_TCLCONFIG
TEA_LOAD_TCLCONFIG

if test "${TCL_MAJOR_VERSION}" -ne 8 ; then
    AC_MSG_ERROR([${PACKAGE_NAME} ${PACKAGE_VERSION} requires Tcl 8.4+
Found config for Tcl ${TCL_VERSION}])
fi
if test "${TCL_MINOR_VERSION}" -lt 4 ; then
    AC_MSG_ERROR([${PACKAGE_NAME} ${PACKAGE_VERSION} requires Tcl 8.4+
Found config for Tcl ${TCL_VERSION}])
fi

#--------------------------------------------------------------------
# Load the tkConfig.sh file if necessary (Tk extension)
#--------------------------------------------------------------------

#TEA_PATH_TKCONFIG
#TEA_LOAD_TKCONFIG

#-----------------------------------------------------------------------
# Handle the --prefix=... option by defaulting to what Tcl gave.
# Must be called after TEA_LOAD_TCLCONFIG and before TEA_SETUP_COMPILER.
#-----------------------------------------------------------------------

TEA_PREFIX

#-----------------------------------------------------------------------
# Standard compiler checks.
# This sets up CC by using the CC env var, or looks for gcc otherwise.
# This also calls AC_PROG_CC and a few others to create the basic setup
# necessary to compile executables.
#-----------------------------------------------------------------------

TEA_SETUP_COMPILER

#--------------------------------------------------------------------
# Check if building with optional Gdbm package. This will declare
# GDBM_CFLAGS and GDBM_LIBS variables.
#--------------------------------------------------------------------

TCLTHREAD_WITH_GDBM

#--------------------------------------------------------------------
# Check if building with optional lmdb package. This will declare
# LMDB_CFLAGS and LMDB_LIBS variables.
#--------------------------------------------------------------------

TCLTHREAD_WITH_LMDB

#--------------------------------------------------------------------
# Locate the NaviServer/AOLserver dir for compilation as NaviServer/AOLserver module.
# This will declare NS_INCLUDES, NS_LIBS and define NS_AOLSERVER.
#--------------------------------------------------------------------

NS_PATH_AOLSERVER

#-----------------------------------------------------------------------
# __CHANGE__
# Specify the C source files to compile in TEA_ADD_SOURCES,
# public headers that need to be installed in TEA_ADD_HEADERS,
# stub library C source files to compile in TEA_ADD_STUB_SOURCES,
# and runtime Tcl library files in TEA_ADD_TCL_SOURCES.
# This defines PKG(_STUB)_SOURCES, PKG(_STUB)_OBJECTS, PKG_HEADERS
# and PKG_TCL_SOURCES.
#-----------------------------------------------------------------------

TEA_ADD_SOURCES([generic/threadNs.c           \
                 generic/threadCmd.c          \
                 generic/threadSvCmd.c        \
                 generic/threadSpCmd.c        \
                 generic/threadPoolCmd.c      \
                 generic/psGdbm.c             \
                 generic/psLmdb.c             \
                 generic/threadSvListCmd.c    \
                 generic/threadSvKeylistCmd.c \
                 generic/tclXkeylist.c        \
])

TEA_ADD_HEADERS([generic/tclThread.h])
TEA_ADD_INCLUDES([${NS_INCLUDES}])
TEA_ADD_LIBS([${GDBM_LIBS} ${LMDB_LIBS} ${NS_LIBS}])
TEA_ADD_CFLAGS([${GDBM_CFLAGS} ${LMDB_CFLAGS}])
TEA_ADD_STUB_SOURCES([])
TEA_ADD_TCL_SOURCES([lib/ttrace.tcl])

#--------------------------------------------------------------------
# __CHANGE__
# A few miscellaneous platform-specific items:
#
# Define a special symbol for Windows (BUILD_sample in this case) so
# that we create the export library with the dll.
#
# Windows creates a few extra files that need to be cleaned up.
# You can add more files to clean if your extension creates any extra
# files.
#
# TEA_ADD_* any platform specific compiler/build info here.
#--------------------------------------------------------------------

if test "${TEA_PLATFORM}" = "windows" ; then
    TEA_ADD_INCLUDES([-I\"$(${CYGPATH} ${srcdir}/win)\"])
else
    TEA_ADD_SOURCES([unix/threadUnix.c])
fi

#--------------------------------------------------------------------
# __CHANGE__
# Choose which headers you need.  Extension authors should try very
# hard to only rely on the Tcl public header files.  Internal headers
# contain private data structures and are subject to change without
# notice.
# This MUST be called after TEA_LOAD_TCLCONFIG / TEA_LOAD_TKCONFIG
#--------------------------------------------------------------------

TEA_PUBLIC_TCL_HEADERS
#TEA_PRIVATE_TCL_HEADERS

#TEA_PUBLIC_TK_HEADERS
#TEA_PRIVATE_TK_HEADERS
#TEA_PATH_X

#--------------------------------------------------------------------
# Check whether --enable-threads or --disable-threads was given.
# This auto-enables if Tcl was compiled threaded.
#--------------------------------------------------------------------

TEA_ENABLE_THREADS

#--------------------------------------------------------------------
# The statement below defines a collection of symbols related to
# building as a shared library instead of a static library.
#--------------------------------------------------------------------

TEA_ENABLE_SHARED

#--------------------------------------------------------------------
# This macro figures out what flags to use with the compiler/linker
# when building shared/static debug/optimized objects.  This information
# can be taken from the tclConfig.sh file, but this figures it all out.
#--------------------------------------------------------------------

TEA_CONFIG_CFLAGS

#--------------------------------------------------------------------
# Set the default compiler switches based on the --enable-symbols option.
#--------------------------------------------------------------------

TEA_ENABLE_SYMBOLS

#--------------------------------------------------------------------
# Everyone should be linking against the Tcl stub library.  If you
# can't for some reason, remove this definition.  If you aren't using
# stubs, you also need to modify the SHLIB_LD_LIBS setting below to
# link against the non-stubbed Tcl library.  Add Tk too if necessary.
#--------------------------------------------------------------------

AC_DEFINE(USE_TCL_STUBS, 1, [Use Tcl stubs])

#--------------------------------------------------------------------
# Enable compile-time support for TIP #143 and TIP #285.  When using
# a pre-Tcl 8.5 or 8.6 core, respectively, the actual functionality
# will not be available at runtime.
#--------------------------------------------------------------------

AC_DEFINE(TCL_TIP143, 1, [Enable TIP #143 support])
AC_DEFINE(TCL_TIP285, 1, [Enable TIP #285 support])

#--------------------------------------------------------------------
# This macro generates a line to use when building a library.  It
# depends on values set by the TEA_ENABLE_SHARED, TEA_ENABLE_SYMBOLS,
# and TEA_LOAD_TCLCONFIG macros above.
#--------------------------------------------------------------------

TEA_MAKE_LIB

#--------------------------------------------------------------------
# Determine the name of the tclsh and/or wish executables in the
# Tcl and Tk build directories or the location they were installed
# into. These paths are used to support running test cases only,
# the Makefile should not be making use of these paths to generate
# a pkgIndex.tcl file or anything else at extension build time.
#--------------------------------------------------------------------

TEA_PROG_TCLSH
#TEA_PROG_WISH

#--------------------------------------------------------------------
# Finally, substitute all of the various values into the Makefile.
# You may alternatively have a special pkgIndex.tcl.in or other files
# which require substituting th AC variables in.  Include these here.
#--------------------------------------------------------------------

AC_OUTPUT([Makefile pkgIndex.tcl])
