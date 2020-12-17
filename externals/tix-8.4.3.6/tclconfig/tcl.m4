# tcl.m4 --
#
#	This file provides a set of autoconf macros to help TEA-enable
#	a Tcl extension.
#
# Copyright (c) 1999-2000 Ajuba Solutions.
# Copyright (c) 2002-2005 ActiveState Corporation.
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#
# RCS: @(#) $Id: tcl.m4,v 1.6 2007/02/21 22:09:05 hobbs Exp $

AC_PREREQ(2.57)

dnl TEA extensions pass us the version of TEA they think they
dnl are compatible with (must be set in TEA_INIT below)
dnl TEA_VERSION="3.6"

# Possible values for key variables defined:
#
# TEA_WINDOWINGSYSTEM - win32 aqua x11 (mirrors 'tk windowingsystem')
# TEA_PLATFORM        - windows unix
#

#------------------------------------------------------------------------
# TEA_PATH_TCLCONFIG --
#
#	Locate the tclConfig.sh file and perform a sanity check on
#	the Tcl compile flags
#
# Arguments:
#	none
#
# Results:
#
#	Adds the following arguments to configure:
#		--with-tcl=...
#
#	Defines the following vars:
#		TCL_BIN_DIR	Full path to the directory containing
#				the tclConfig.sh file
#------------------------------------------------------------------------

AC_DEFUN([TEA_PATH_TCLCONFIG], [
    dnl Make sure we are initialized
    AC_REQUIRE([TEA_INIT])
    #
    # Ok, lets find the tcl configuration
    # First, look for one uninstalled.
    # the alternative search directory is invoked by --with-tcl
    #

    if test x"${no_tcl}" = x ; then
	# we reset no_tcl in case something fails here
	no_tcl=true
	AC_ARG_WITH(tcl,
	    AC_HELP_STRING([--with-tcl],
		[directory containing tcl configuration (tclConfig.sh)]),
	    with_tclconfig=${withval})
	AC_MSG_CHECKING([for Tcl configuration])
	AC_CACHE_VAL(ac_cv_c_tclconfig,[

	    # First check to see if --with-tcl was specified.
	    if test x"${with_tclconfig}" != x ; then
		case ${with_tclconfig} in
		    */tclConfig.sh )
			if test -f ${with_tclconfig}; then
			    AC_MSG_WARN([--with-tcl argument should refer to directory containing tclConfig.sh, not to tclConfig.sh itself])
			    with_tclconfig=`echo ${with_tclconfig} | sed 's!/tclConfig\.sh$!!'`
			fi ;;
		esac
		if test -f "${with_tclconfig}/tclConfig.sh" ; then
		    ac_cv_c_tclconfig=`(cd ${with_tclconfig}; pwd)`
		else
		    AC_MSG_ERROR([${with_tclconfig} directory doesn't contain tclConfig.sh])
		fi
	    fi

	    # then check for a private Tcl installation
	    if test x"${ac_cv_c_tclconfig}" = x ; then
		for i in \
			../tcl \
			`ls -dr ../tcl[[8-9]].[[0-9]].[[0-9]]* 2>/dev/null` \
			`ls -dr ../tcl[[8-9]].[[0-9]] 2>/dev/null` \
			`ls -dr ../tcl[[8-9]].[[0-9]]* 2>/dev/null` \
			../../tcl \
			`ls -dr ../../tcl[[8-9]].[[0-9]].[[0-9]]* 2>/dev/null` \
			`ls -dr ../../tcl[[8-9]].[[0-9]] 2>/dev/null` \
			`ls -dr ../../tcl[[8-9]].[[0-9]]* 2>/dev/null` \
			../../../tcl \
			`ls -dr ../../../tcl[[8-9]].[[0-9]].[[0-9]]* 2>/dev/null` \
			`ls -dr ../../../tcl[[8-9]].[[0-9]] 2>/dev/null` \
			`ls -dr ../../../tcl[[8-9]].[[0-9]]* 2>/dev/null` ; do
		    if test -f "$i/unix/tclConfig.sh" ; then
			ac_cv_c_tclconfig=`(cd $i/unix; pwd)`
			break
		    fi
		done
	    fi

	    # on Darwin, check in Framework installation locations
	    if test "`uname -s`" = "Darwin" -a x"${ac_cv_c_tclconfig}" = x ; then
		for i in `ls -d ~/Library/Frameworks 2>/dev/null` \
			`ls -d /Library/Frameworks 2>/dev/null` \
			`ls -d /Network/Library/Frameworks 2>/dev/null` \
			`ls -d /System/Library/Frameworks 2>/dev/null` \
			; do
		    if test -f "$i/Tcl.framework/tclConfig.sh" ; then
			ac_cv_c_tclconfig=`(cd $i/Tcl.framework; pwd)`
			break
		    fi
		done
	    fi

	    # on Windows, check in common installation locations
	    if test "${TEA_PLATFORM}" = "windows" \
		-a x"${ac_cv_c_tclconfig}" = x ; then
		for i in `ls -d C:/Tcl/lib 2>/dev/null` \
			`ls -d C:/Progra~1/Tcl/lib 2>/dev/null` \
			; do
		    if test -f "$i/tclConfig.sh" ; then
			ac_cv_c_tclconfig=`(cd $i; pwd)`
			break
		    fi
		done
	    fi

	    # check in a few common install locations
	    if test x"${ac_cv_c_tclconfig}" = x ; then
		for i in `ls -d ${libdir} 2>/dev/null` \
			`ls -d ${exec_prefix}/lib 2>/dev/null` \
			`ls -d ${prefix}/lib 2>/dev/null` \
			`ls -d /usr/local/lib 2>/dev/null` \
			`ls -d /usr/contrib/lib 2>/dev/null` \
			`ls -d /usr/lib 2>/dev/null` \
			; do
		    if test -f "$i/tclConfig.sh" ; then
			ac_cv_c_tclconfig=`(cd $i; pwd)`
			break
		    fi
		done
	    fi

	    # check in a few other private locations
	    if test x"${ac_cv_c_tclconfig}" = x ; then
		for i in \
			${srcdir}/../tcl \
			`ls -dr ${srcdir}/../tcl[[8-9]].[[0-9]].[[0-9]]* 2>/dev/null` \
			`ls -dr ${srcdir}/../tcl[[8-9]].[[0-9]] 2>/dev/null` \
			`ls -dr ${srcdir}/../tcl[[8-9]].[[0-9]]* 2>/dev/null` ; do
		    if test -f "$i/unix/tclConfig.sh" ; then
		    ac_cv_c_tclconfig=`(cd $i/unix; pwd)`
		    break
		fi
		done
	    fi
	])

	if test x"${ac_cv_c_tclconfig}" = x ; then
	    TCL_BIN_DIR="# no Tcl configs found"
	    AC_MSG_WARN([Can't find Tcl configuration definitions])
	    exit 0
	else
	    no_tcl=
	    TCL_BIN_DIR=${ac_cv_c_tclconfig}
	    AC_MSG_RESULT([found ${TCL_BIN_DIR}/tclConfig.sh])
	fi
    fi
])

#------------------------------------------------------------------------
# TEA_PATH_TKCONFIG --
#
#	Locate the tkConfig.sh file
#
# Arguments:
#	none
#
# Results:
#
#	Adds the following arguments to configure:
#		--with-tk=...
#
#	Defines the following vars:
#		TK_BIN_DIR	Full path to the directory containing
#				the tkConfig.sh file
#------------------------------------------------------------------------

AC_DEFUN([TEA_PATH_TKCONFIG], [
    #
    # Ok, lets find the tk configuration
    # First, look for one uninstalled.
    # the alternative search directory is invoked by --with-tk
    #

    if test x"${no_tk}" = x ; then
	# we reset no_tk in case something fails here
	no_tk=true
	AC_ARG_WITH(tk,
	    AC_HELP_STRING([--with-tk],
		[directory containing tk configuration (tkConfig.sh)]),
	    with_tkconfig=${withval})
	AC_MSG_CHECKING([for Tk configuration])
	AC_CACHE_VAL(ac_cv_c_tkconfig,[

	    # First check to see if --with-tkconfig was specified.
	    if test x"${with_tkconfig}" != x ; then
		case ${with_tkconfig} in
		    */tkConfig.sh )
			if test -f ${with_tkconfig}; then
			    AC_MSG_WARN([--with-tk argument should refer to directory containing tkConfig.sh, not to tkConfig.sh itself])
			    with_tkconfig=`echo ${with_tkconfig} | sed 's!/tkConfig\.sh$!!'`
			fi ;;
		esac
		if test -f "${with_tkconfig}/tkConfig.sh" ; then
		    ac_cv_c_tkconfig=`(cd ${with_tkconfig}; pwd)`
		else
		    AC_MSG_ERROR([${with_tkconfig} directory doesn't contain tkConfig.sh])
		fi
	    fi

	    # then check for a private Tk library
	    if test x"${ac_cv_c_tkconfig}" = x ; then
		for i in \
			../tk \
			`ls -dr ../tk[[8-9]].[[0-9]].[[0-9]]* 2>/dev/null` \
			`ls -dr ../tk[[8-9]].[[0-9]] 2>/dev/null` \
			`ls -dr ../tk[[8-9]].[[0-9]]* 2>/dev/null` \
			../../tk \
			`ls -dr ../../tk[[8-9]].[[0-9]].[[0-9]]* 2>/dev/null` \
			`ls -dr ../../tk[[8-9]].[[0-9]] 2>/dev/null` \
			`ls -dr ../../tk[[8-9]].[[0-9]]* 2>/dev/null` \
			../../../tk \
			`ls -dr ../../../tk[[8-9]].[[0-9]].[[0-9]]* 2>/dev/null` \
			`ls -dr ../../../tk[[8-9]].[[0-9]] 2>/dev/null` \
			`ls -dr ../../../tk[[8-9]].[[0-9]]* 2>/dev/null` ; do
		    if test -f "$i/unix/tkConfig.sh" ; then
			ac_cv_c_tkconfig=`(cd $i/unix; pwd)`
			break
		    fi
		done
	    fi

	    # on Darwin, check in Framework installation locations
	    if test "`uname -s`" = "Darwin" -a x"${ac_cv_c_tkconfig}" = x ; then
		for i in `ls -d ~/Library/Frameworks 2>/dev/null` \
			`ls -d /Library/Frameworks 2>/dev/null` \
			`ls -d /Network/Library/Frameworks 2>/dev/null` \
			`ls -d /System/Library/Frameworks 2>/dev/null` \
			; do
		    if test -f "$i/Tk.framework/tkConfig.sh" ; then
			ac_cv_c_tkconfig=`(cd $i/Tk.framework; pwd)`
			break
		    fi
		done
	    fi

	    # check in a few common install locations
	    if test x"${ac_cv_c_tkconfig}" = x ; then
		for i in `ls -d ${libdir} 2>/dev/null` \
			`ls -d ${exec_prefix}/lib 2>/dev/null` \
			`ls -d ${prefix}/lib 2>/dev/null` \
			`ls -d /usr/local/lib 2>/dev/null` \
			`ls -d /usr/contrib/lib 2>/dev/null` \
			`ls -d /usr/lib 2>/dev/null` \
			; do
		    if test -f "$i/tkConfig.sh" ; then
			ac_cv_c_tkconfig=`(cd $i; pwd)`
			break
		    fi
		done
	    fi

	    # on Windows, check in common installation locations
	    if test "${TEA_PLATFORM}" = "windows" \
		-a x"${ac_cv_c_tkconfig}" = x ; then
		for i in `ls -d C:/Tcl/lib 2>/dev/null` \
			`ls -d C:/Progra~1/Tcl/lib 2>/dev/null` \
			; do
		    if test -f "$i/tkConfig.sh" ; then
			ac_cv_c_tkconfig=`(cd $i; pwd)`
			break
		    fi
		done
	    fi

	    # check in a few other private locations
	    if test x"${ac_cv_c_tkconfig}" = x ; then
		for i in \
			${srcdir}/../tk \
			`ls -dr ${srcdir}/../tk[[8-9]].[[0-9]].[[0-9]]* 2>/dev/null` \
			`ls -dr ${srcdir}/../tk[[8-9]].[[0-9]] 2>/dev/null` \
			`ls -dr ${srcdir}/../tk[[8-9]].[[0-9]]* 2>/dev/null` ; do
		    if test -f "$i/unix/tkConfig.sh" ; then
			ac_cv_c_tkconfig=`(cd $i/unix; pwd)`
			break
		    fi
		done
	    fi
	])

	if test x"${ac_cv_c_tkconfig}" = x ; then
	    TK_BIN_DIR="# no Tk configs found"
	    AC_MSG_WARN([Can't find Tk configuration definitions])
	    exit 0
	else
	    no_tk=
	    TK_BIN_DIR=${ac_cv_c_tkconfig}
	    AC_MSG_RESULT([found ${TK_BIN_DIR}/tkConfig.sh])
	fi
    fi
])

#------------------------------------------------------------------------
# TEA_LOAD_TCLCONFIG --
#
#	Load the tclConfig.sh file
#
# Arguments:
#	
#	Requires the following vars to be set:
#		TCL_BIN_DIR
#
# Results:
#
#	Subst the following vars:
#		TCL_BIN_DIR
#		TCL_SRC_DIR
#		TCL_LIB_FILE
#
#------------------------------------------------------------------------

AC_DEFUN([TEA_LOAD_TCLCONFIG], [
    AC_MSG_CHECKING([for existence of ${TCL_BIN_DIR}/tclConfig.sh])

    if test -f "${TCL_BIN_DIR}/tclConfig.sh" ; then
        AC_MSG_RESULT([loading])
	. ${TCL_BIN_DIR}/tclConfig.sh
    else
        AC_MSG_RESULT([could not find ${TCL_BIN_DIR}/tclConfig.sh])
    fi

    # eval is required to do the TCL_DBGX substitution
    eval "TCL_LIB_FILE=\"${TCL_LIB_FILE}\""
    eval "TCL_STUB_LIB_FILE=\"${TCL_STUB_LIB_FILE}\""

    # If the TCL_BIN_DIR is the build directory (not the install directory),
    # then set the common variable name to the value of the build variables.
    # For example, the variable TCL_LIB_SPEC will be set to the value
    # of TCL_BUILD_LIB_SPEC. An extension should make use of TCL_LIB_SPEC
    # instead of TCL_BUILD_LIB_SPEC since it will work with both an
    # installed and uninstalled version of Tcl.
    if test -f ${TCL_BIN_DIR}/Makefile ; then
        TCL_LIB_SPEC=${TCL_BUILD_LIB_SPEC}
        TCL_STUB_LIB_SPEC=${TCL_BUILD_STUB_LIB_SPEC}
        TCL_STUB_LIB_PATH=${TCL_BUILD_STUB_LIB_PATH}
    elif test "`uname -s`" = "Darwin"; then
	# If Tcl was built as a framework, attempt to use the libraries
	# from the framework at the given location so that linking works
	# against Tcl.framework installed in an arbitary location.
	case ${TCL_DEFS} in
	    *TCL_FRAMEWORK*)
		if test -f ${TCL_BIN_DIR}/${TCL_LIB_FILE}; then
		    for i in "`cd ${TCL_BIN_DIR}; pwd`" \
			     "`cd ${TCL_BIN_DIR}/../..; pwd`"; do
			if test "`basename "$i"`" = "${TCL_LIB_FILE}.framework"; then
			    TCL_LIB_SPEC="-F`dirname "$i"` -framework ${TCL_LIB_FILE}"
			    break
			fi
		    done
		fi
		if test -f ${TCL_BIN_DIR}/${TCL_STUB_LIB_FILE}; then
		    TCL_STUB_LIB_SPEC="-L${TCL_BIN_DIR} ${TCL_STUB_LIB_FLAG}"
		    TCL_STUB_LIB_PATH="${TCL_BIN_DIR}/${TCL_STUB_LIB_FILE}"
		fi
		;;
	esac
    fi

    # eval is required to do the TCL_DBGX substitution
    eval "TCL_LIB_FLAG=\"${TCL_LIB_FLAG}\""
    eval "TCL_LIB_SPEC=\"${TCL_LIB_SPEC}\""
    eval "TCL_STUB_LIB_FLAG=\"${TCL_STUB_LIB_FLAG}\""
    eval "TCL_STUB_LIB_SPEC=\"${TCL_STUB_LIB_SPEC}\""

    AC_SUBST(TCL_VERSION)
    AC_SUBST(TCL_BIN_DIR)
    AC_SUBST(TCL_SRC_DIR)

    AC_SUBST(TCL_LIB_FILE)
    AC_SUBST(TCL_LIB_FLAG)
    AC_SUBST(TCL_LIB_SPEC)

    AC_SUBST(TCL_STUB_LIB_FILE)
    AC_SUBST(TCL_STUB_LIB_FLAG)
    AC_SUBST(TCL_STUB_LIB_SPEC)

    AC_SUBST(TCL_LIBS)
    AC_SUBST(TCL_DEFS)
    AC_SUBST(TCL_EXTRA_CFLAGS)
    AC_SUBST(TCL_LD_FLAGS)
    AC_SUBST(TCL_SHLIB_LD_LIBS)
])

#------------------------------------------------------------------------
# TEA_LOAD_TKCONFIG --
#
#	Load the tkConfig.sh file
#
# Arguments:
#	
#	Requires the following vars to be set:
#		TK_BIN_DIR
#
# Results:
#
#	Sets the following vars that should be in tkConfig.sh:
#		TK_BIN_DIR
#------------------------------------------------------------------------

AC_DEFUN([TEA_LOAD_TKCONFIG], [
    AC_MSG_CHECKING([for existence of ${TK_BIN_DIR}/tkConfig.sh])

    if test -f "${TK_BIN_DIR}/tkConfig.sh" ; then
        AC_MSG_RESULT([loading])
	. ${TK_BIN_DIR}/tkConfig.sh
    else
        AC_MSG_RESULT([could not find ${TK_BIN_DIR}/tkConfig.sh])
    fi

    # eval is required to do the TK_DBGX substitution
    eval "TK_LIB_FILE=\"${TK_LIB_FILE}\""
    eval "TK_STUB_LIB_FILE=\"${TK_STUB_LIB_FILE}\""

    # If the TK_BIN_DIR is the build directory (not the install directory),
    # then set the common variable name to the value of the build variables.
    # For example, the variable TK_LIB_SPEC will be set to the value
    # of TK_BUILD_LIB_SPEC. An extension should make use of TK_LIB_SPEC
    # instead of TK_BUILD_LIB_SPEC since it will work with both an
    # installed and uninstalled version of Tcl.
    if test -f ${TK_BIN_DIR}/Makefile ; then
        TK_LIB_SPEC=${TK_BUILD_LIB_SPEC}
        TK_STUB_LIB_SPEC=${TK_BUILD_STUB_LIB_SPEC}
        TK_STUB_LIB_PATH=${TK_BUILD_STUB_LIB_PATH}
    elif test "`uname -s`" = "Darwin"; then
	# If Tk was built as a framework, attempt to use the libraries
	# from the framework at the given location so that linking works
	# against Tk.framework installed in an arbitary location.
	case ${TK_DEFS} in
	    *TK_FRAMEWORK*)
		if test -f ${TK_BIN_DIR}/${TK_LIB_FILE}; then
		    for i in "`cd ${TK_BIN_DIR}; pwd`" \
			     "`cd ${TK_BIN_DIR}/../..; pwd`"; do
			if test "`basename "$i"`" = "${TK_LIB_FILE}.framework"; then
			    TK_LIB_SPEC="-F`dirname "$i"` -framework ${TK_LIB_FILE}"
			    break
			fi
		    done
		fi
		if test -f ${TK_BIN_DIR}/${TK_STUB_LIB_FILE}; then
		    TK_STUB_LIB_SPEC="-L${TK_BIN_DIR} ${TK_STUB_LIB_FLAG}"
		    TK_STUB_LIB_PATH="${TK_BIN_DIR}/${TK_STUB_LIB_FILE}"
		fi
		;;
	esac
    fi

    # eval is required to do the TK_DBGX substitution
    eval "TK_LIB_FLAG=\"${TK_LIB_FLAG}\""
    eval "TK_LIB_SPEC=\"${TK_LIB_SPEC}\""
    eval "TK_STUB_LIB_FLAG=\"${TK_STUB_LIB_FLAG}\""
    eval "TK_STUB_LIB_SPEC=\"${TK_STUB_LIB_SPEC}\""

    # Ensure windowingsystem is defined
    if test "${TEA_PLATFORM}" = "unix" ; then
	case ${TK_DEFS} in
	    *MAC_OSX_TK*)
		AC_DEFINE(MAC_OSX_TK, 1, [Are we building against Mac OS X TkAqua?])
		TEA_WINDOWINGSYSTEM="aqua"
		;;
	    *)
		TEA_WINDOWINGSYSTEM="x11"
		;;
	esac
    elif test "${TEA_PLATFORM}" = "windows" ; then
	TEA_WINDOWINGSYSTEM="win32"
    fi

    AC_SUBST(TK_VERSION)
    AC_SUBST(TK_BIN_DIR)
    AC_SUBST(TK_SRC_DIR)

    AC_SUBST(TK_LIB_FILE)
    AC_SUBST(TK_LIB_FLAG)
    AC_SUBST(TK_LIB_SPEC)

    AC_SUBST(TK_STUB_LIB_FILE)
    AC_SUBST(TK_STUB_LIB_FLAG)
    AC_SUBST(TK_STUB_LIB_SPEC)

    AC_SUBST(TK_LIBS)
    AC_SUBST(TK_XINCLUDES)
])

#------------------------------------------------------------------------
# TEA_ENABLE_SHARED --
#
#	Allows the building of shared libraries
#
# Arguments:
#	none
#	
# Results:
#
#	Adds the following arguments to configure:
#		--enable-shared=yes|no
#
#	Defines the following vars:
#		STATIC_BUILD	Used for building import/export libraries
#				on Windows.
#
#	Sets the following vars:
#		SHARED_BUILD	Value of 1 or 0
#------------------------------------------------------------------------

AC_DEFUN([TEA_ENABLE_SHARED], [
    AC_MSG_CHECKING([how to build libraries])
    AC_ARG_ENABLE(shared,
	AC_HELP_STRING([--enable-shared],
	    [build and link with shared libraries (default: on)]),
	[tcl_ok=$enableval], [tcl_ok=yes])

    if test "${enable_shared+set}" = set; then
	enableval="$enable_shared"
	tcl_ok=$enableval
    else
	tcl_ok=yes
    fi

    if test "$tcl_ok" = "yes" ; then
	AC_MSG_RESULT([shared])
	SHARED_BUILD=1
    else
	AC_MSG_RESULT([static])
	SHARED_BUILD=0
	AC_DEFINE(STATIC_BUILD, 1, [Is this a static build?])
    fi
    AC_SUBST(SHARED_BUILD)
])

#------------------------------------------------------------------------
# TEA_ENABLE_THREADS --
#
#	Specify if thread support should be enabled.  If "yes" is specified
#	as an arg (optional), threads are enabled by default, "no" means
#	threads are disabled.  "yes" is the default.
#
#	TCL_THREADS is checked so that if you are compiling an extension
#	against a threaded core, your extension must be compiled threaded
#	as well.
#
#	Note that it is legal to have a thread enabled extension run in a
#	threaded or non-threaded Tcl core, but a non-threaded extension may
#	only run in a non-threaded Tcl core.
#
# Arguments:
#	none
#	
# Results:
#
#	Adds the following arguments to configure:
#		--enable-threads
#
#	Sets the following vars:
#		THREADS_LIBS	Thread library(s)
#
#	Defines the following vars:
#		TCL_THREADS
#		_REENTRANT
#		_THREAD_SAFE
#
#------------------------------------------------------------------------

AC_DEFUN([TEA_ENABLE_THREADS], [
    AC_ARG_ENABLE(threads,
	AC_HELP_STRING([--enable-threads],
	    [build with threads]),
	[tcl_ok=$enableval], [tcl_ok=yes])

    if test "${enable_threads+set}" = set; then
	enableval="$enable_threads"
	tcl_ok=$enableval
    else
	tcl_ok=yes
    fi

    if test "$tcl_ok" = "yes" -o "${TCL_THREADS}" = 1; then
	TCL_THREADS=1

	if test "${TEA_PLATFORM}" != "windows" ; then
	    # We are always OK on Windows, so check what this platform wants:
    
	    # USE_THREAD_ALLOC tells us to try the special thread-based
	    # allocator that significantly reduces lock contention
	    AC_DEFINE(USE_THREAD_ALLOC, 1,
		[Do we want to use the threaded memory allocator?])
	    AC_DEFINE(_REENTRANT, 1, [Do we want the reentrant OS API?])
	    if test "`uname -s`" = "SunOS" ; then
		AC_DEFINE(_POSIX_PTHREAD_SEMANTICS, 1,
			[Do we really want to follow the standard? Yes we do!])
	    fi
	    AC_DEFINE(_THREAD_SAFE, 1, [Do we want the thread-safe OS API?])
	    AC_CHECK_LIB(pthread,pthread_mutex_init,tcl_ok=yes,tcl_ok=no)
	    if test "$tcl_ok" = "no"; then
		# Check a little harder for __pthread_mutex_init in the same
		# library, as some systems hide it there until pthread.h is
		# defined.  We could alternatively do an AC_TRY_COMPILE with
		# pthread.h, but that will work with libpthread really doesn't
		# exist, like AIX 4.2.  [Bug: 4359]
		AC_CHECK_LIB(pthread, __pthread_mutex_init,
		    tcl_ok=yes, tcl_ok=no)
	    fi

	    if test "$tcl_ok" = "yes"; then
		# The space is needed
		THREADS_LIBS=" -lpthread"
	    else
		AC_CHECK_LIB(pthreads, pthread_mutex_init,
		    tcl_ok=yes, tcl_ok=no)
		if test "$tcl_ok" = "yes"; then
		    # The space is needed
		    THREADS_LIBS=" -lpthreads"
		else
		    AC_CHECK_LIB(c, pthread_mutex_init,
			tcl_ok=yes, tcl_ok=no)
		    if test "$tcl_ok" = "no"; then
			AC_CHECK_LIB(c_r, pthread_mutex_init,
			    tcl_ok=yes, tcl_ok=no)
			if test "$tcl_ok" = "yes"; then
			    # The space is needed
			    THREADS_LIBS=" -pthread"
			else
			    TCL_THREADS=0
			    AC_MSG_WARN([Do not know how to find pthread lib on your system - thread support disabled])
			fi
		    fi
		fi
	    fi
	fi
    else
	TCL_THREADS=0
    fi
    # Do checking message here to not mess up interleaved configure output
    AC_MSG_CHECKING([for building with threads])
    if test "${TCL_THREADS}" = 1; then
	AC_DEFINE(TCL_THREADS, 1, [Are we building with threads enabled?])
	AC_MSG_RESULT([yes (default)])
    else
	AC_MSG_RESULT([no])
    fi
    # TCL_THREADS sanity checking.  See if our request for building with
    # threads is the same as the way Tcl was built.  If not, warn the user.
    case ${TCL_DEFS} in
	*THREADS=1*)
	    if test "${TCL_THREADS}" = "0"; then
		AC_MSG_WARN([
    Building ${PACKAGE_NAME} without threads enabled, but building against Tcl
    that IS thread-enabled.  It is recommended to use --enable-threads.])
	    fi
	    ;;
	*)
	    if test "${TCL_THREADS}" = "1"; then
		AC_MSG_WARN([
    --enable-threads requested, but building against a Tcl that is NOT
    thread-enabled.  This is an OK configuration that will also run in
    a thread-enabled core.])
	    fi
	    ;;
    esac
    AC_SUBST(TCL_THREADS)
])

#------------------------------------------------------------------------
# TEA_ENABLE_SYMBOLS --
#
#	Specify if debugging symbols should be used.
#	Memory (TCL_MEM_DEBUG) debugging can also be enabled.
#
# Arguments:
#	none
#	
#	TEA varies from core Tcl in that C|LDFLAGS_DEFAULT receives
#	the value of C|LDFLAGS_OPTIMIZE|DEBUG already substituted.
#	Requires the following vars to be set in the Makefile:
#		CFLAGS_DEFAULT
#		LDFLAGS_DEFAULT
#	
# Results:
#
#	Adds the following arguments to configure:
#		--enable-symbols
#
#	Defines the following vars:
#		CFLAGS_DEFAULT	Sets to $(CFLAGS_DEBUG) if true
#				Sets to $(CFLAGS_OPTIMIZE) if false
#		LDFLAGS_DEFAULT	Sets to $(LDFLAGS_DEBUG) if true
#				Sets to $(LDFLAGS_OPTIMIZE) if false
#		DBGX		Formerly used as debug library extension;
#				always blank now.
#
#------------------------------------------------------------------------

AC_DEFUN([TEA_ENABLE_SYMBOLS], [
    dnl Make sure we are initialized
    AC_REQUIRE([TEA_CONFIG_CFLAGS])
    AC_MSG_CHECKING([for build with symbols])
    AC_ARG_ENABLE(symbols,
	AC_HELP_STRING([--enable-symbols],
	    [build with debugging symbols (default: off)]),
	[tcl_ok=$enableval], [tcl_ok=no])
    DBGX=""
    if test "$tcl_ok" = "no"; then
	CFLAGS_DEFAULT="${CFLAGS_OPTIMIZE}"
	LDFLAGS_DEFAULT="${LDFLAGS_OPTIMIZE}"
	AC_MSG_RESULT([no])
    else
	CFLAGS_DEFAULT="${CFLAGS_DEBUG}"
	LDFLAGS_DEFAULT="${LDFLAGS_DEBUG}"
	if test "$tcl_ok" = "yes"; then
	    AC_MSG_RESULT([yes (standard debugging)])
	fi
    fi
    if test "${TEA_PLATFORM}" != "windows" ; then
	LDFLAGS_DEFAULT="${LDFLAGS}"
    fi

    AC_SUBST(TCL_DBGX)
    AC_SUBST(CFLAGS_DEFAULT)
    AC_SUBST(LDFLAGS_DEFAULT)

    if test "$tcl_ok" = "mem" -o "$tcl_ok" = "all"; then
	AC_DEFINE(TCL_MEM_DEBUG, 1, [Is memory debugging enabled?])
    fi

    if test "$tcl_ok" != "yes" -a "$tcl_ok" != "no"; then
	if test "$tcl_ok" = "all"; then
	    AC_MSG_RESULT([enabled symbols mem debugging])
	else
	    AC_MSG_RESULT([enabled $tcl_ok debugging])
	fi
    fi
])

#------------------------------------------------------------------------
# TEA_ENABLE_LANGINFO --
#
#	Allows use of modern nl_langinfo check for better l10n.
#	This is only relevant for Unix.
#
# Arguments:
#	none
#	
# Results:
#
#	Adds the following arguments to configure:
#		--enable-langinfo=yes|no (default is yes)
#
#	Defines the following vars:
#		HAVE_LANGINFO	Triggers use of nl_langinfo if defined.
#
#------------------------------------------------------------------------

AC_DEFUN([TEA_ENABLE_LANGINFO], [
    AC_ARG_ENABLE(langinfo,
	AC_HELP_STRING([--enable-langinfo],
	    [use nl_langinfo if possible to determine encoding at startup, otherwise use old heuristic (default: on)]),
	[langinfo_ok=$enableval], [langinfo_ok=yes])

    HAVE_LANGINFO=0
    if test "$langinfo_ok" = "yes"; then
	AC_CHECK_HEADER(langinfo.h,[langinfo_ok=yes],[langinfo_ok=no])
    fi
    AC_MSG_CHECKING([whether to use nl_langinfo])
    if test "$langinfo_ok" = "yes"; then
	AC_CACHE_VAL(tcl_cv_langinfo_h, [
	    AC_TRY_COMPILE([#include <langinfo.h>], [nl_langinfo(CODESET);],
		    [tcl_cv_langinfo_h=yes],[tcl_cv_langinfo_h=no])])
	AC_MSG_RESULT([$tcl_cv_langinfo_h])
	if test $tcl_cv_langinfo_h = yes; then
	    AC_DEFINE(HAVE_LANGINFO, 1, [Do we have nl_langinfo()?])
	fi
    else 
	AC_MSG_RESULT([$langinfo_ok])
    fi
])

#--------------------------------------------------------------------
# TEA_CONFIG_SYSTEM
#
#	Determine what the system is (some things cannot be easily checked
#	on a feature-driven basis, alas). This can usually be done via the
#	"uname" command, but there are a few systems, like Next, where
#	this doesn't work.
#
# Arguments:
#	none
#
# Results:
#	Defines the following var:
#
#	system -	System/platform/version identification code.
#
#--------------------------------------------------------------------

AC_DEFUN([TEA_CONFIG_SYSTEM], [
    AC_CACHE_CHECK([system version], tcl_cv_sys_version, [
	if test "${TEA_PLATFORM}" = "windows" ; then
	    tcl_cv_sys_version=windows
	elif test -f /usr/lib/NextStep/software_version; then
	    tcl_cv_sys_version=NEXTSTEP-`awk '/3/,/3/' /usr/lib/NextStep/software_version`
	else
	    tcl_cv_sys_version=`uname -s`-`uname -r`
	    if test "$?" -ne 0 ; then
		AC_MSG_WARN([can't find uname command])
		tcl_cv_sys_version=unknown
	    else
		# Special check for weird MP-RAS system (uname returns weird
		# results, and the version is kept in special file).

		if test -r /etc/.relid -a "X`uname -n`" = "X`uname -s`" ; then
		    tcl_cv_sys_version=MP-RAS-`awk '{print $[3]}' /etc/.relid`
		fi
		if test "`uname -s`" = "AIX" ; then
		    tcl_cv_sys_version=AIX-`uname -v`.`uname -r`
		fi
	    fi
	fi
    ])
    system=$tcl_cv_sys_version
])

#--------------------------------------------------------------------
# TEA_CONFIG_CFLAGS
#
#	Try to determine the proper flags to pass to the compiler
#	for building shared libraries and other such nonsense.
#
# Arguments:
#	none
#
# Results:
#
#	Defines and substitutes the following vars:
#
#       DL_OBJS -       Name of the object file that implements dynamic
#                       loading for Tcl on this system.
#       DL_LIBS -       Library file(s) to include in tclsh and other base
#                       applications in order for the "load" command to work.
#       LDFLAGS -      Flags to pass to the compiler when linking object
#                       files into an executable application binary such
#                       as tclsh.
#       LD_SEARCH_FLAGS-Flags to pass to ld, such as "-R /usr/local/tcl/lib",
#                       that tell the run-time dynamic linker where to look
#                       for shared libraries such as libtcl.so.  Depends on
#                       the variable LIB_RUNTIME_DIR in the Makefile. Could
#                       be the same as CC_SEARCH_FLAGS if ${CC} is used to link.
#       CC_SEARCH_FLAGS-Flags to pass to ${CC}, such as "-Wl,-rpath,/usr/local/tcl/lib",
#                       that tell the run-time dynamic linker where to look
#                       for shared libraries such as libtcl.so.  Depends on
#                       the variable LIB_RUNTIME_DIR in the Makefile.
#       SHLIB_CFLAGS -  Flags to pass to cc when compiling the components
#                       of a shared library (may request position-independent
#                       code, among other things).
#       SHLIB_LD -      Base command to use for combining object files
#                       into a shared library.
#       SHLIB_LD_LIBS - Dependent libraries for the linker to scan when
#                       creating shared libraries.  This symbol typically
#                       goes at the end of the "ld" commands that build
#                       shared libraries. The value of the symbol is
#                       "${LIBS}" if all of the dependent libraries should
#                       be specified when creating a shared library.  If
#                       dependent libraries should not be specified (as on
#                       SunOS 4.x, where they cause the link to fail, or in
#                       general if Tcl and Tk aren't themselves shared
#                       libraries), then this symbol has an empty string
#                       as its value.
#       SHLIB_SUFFIX -  Suffix to use for the names of dynamically loadable
#                       extensions.  An empty string means we don't know how
#                       to use shared libraries on this platform.
#       LIB_SUFFIX -    Specifies everything that comes after the "libfoo"
#                       in a static or shared library name, using the $VERSION variable
#                       to put the version in the right place.  This is used
#                       by platforms that need non-standard library names.
#                       Examples:  ${VERSION}.so.1.1 on NetBSD, since it needs
#                       to have a version after the .so, and ${VERSION}.a
#                       on AIX, since a shared library needs to have
#                       a .a extension whereas shared objects for loadable
#                       extensions have a .so extension.  Defaults to
#                       ${VERSION}${SHLIB_SUFFIX}.
#       TCL_NEEDS_EXP_FILE -
#                       1 means that an export file is needed to link to a
#                       shared library.
#       TCL_EXP_FILE -  The name of the installed export / import file which
#                       should be used to link to the Tcl shared library.
#                       Empty if Tcl is unshared.
#       TCL_BUILD_EXP_FILE -
#                       The name of the built export / import file which
#                       should be used to link to the Tcl shared library.
#                       Empty if Tcl is unshared.
#	CFLAGS_DEBUG -
#			Flags used when running the compiler in debug mode
#	CFLAGS_OPTIMIZE -
#			Flags used when running the compiler in optimize mode
#	CFLAGS -	Additional CFLAGS added as necessary (usually 64-bit)
#
#--------------------------------------------------------------------

AC_DEFUN([TEA_CONFIG_CFLAGS], [
    dnl Make sure we are initialized
    AC_REQUIRE([TEA_INIT])

    # Step 0.a: Enable 64 bit support?

    AC_MSG_CHECKING([if 64bit support is requested])
    AC_ARG_ENABLE(64bit,
	AC_HELP_STRING([--enable-64bit],
	    [enable 64bit support (default: off)]),
	[do64bit=$enableval], [do64bit=no])
    AC_MSG_RESULT([$do64bit])

    # Step 0.b: Enable Solaris 64 bit VIS support?

    AC_MSG_CHECKING([if 64bit Sparc VIS support is requested])
    AC_ARG_ENABLE(64bit-vis,
	AC_HELP_STRING([--enable-64bit-vis],
	    [enable 64bit Sparc VIS support (default: off)]),
	[do64bitVIS=$enableval], [do64bitVIS=no])
    AC_MSG_RESULT([$do64bitVIS])

    if test "$do64bitVIS" = "yes"; then
	# Force 64bit on with VIS
	do64bit=yes
    fi

    # Step 0.c: Cross-compiling options for Windows/CE builds?

    if test "${TEA_PLATFORM}" = "windows" ; then
	AC_MSG_CHECKING([if Windows/CE build is requested])
	AC_ARG_ENABLE(wince,[  --enable-wince          enable Win/CE support (where applicable)], [doWince=$enableval], [doWince=no])
	AC_MSG_RESULT([$doWince])
    fi

    # Step 1: set the variable "system" to hold the name and version number
    # for the system.

    TEA_CONFIG_SYSTEM

    # Step 2: check for existence of -ldl library.  This is needed because
    # Linux can use either -ldl or -ldld for dynamic loading.

    AC_CHECK_LIB(dl, dlopen, have_dl=yes, have_dl=no)

    # Require ranlib early so we can override it in special cases below.

    AC_REQUIRE([AC_PROG_RANLIB])

    # Step 3: set configuration options based on system name and version.
    # This is similar to Tcl's unix/tcl.m4 except that we've added a
    # "windows" case.

    do64bit_ok=no
    LDFLAGS_ORIG="$LDFLAGS"
    # When ld needs options to work in 64-bit mode, put them in
    # LDFLAGS_ARCH so they eventually end up in LDFLAGS even if [load]
    # is disabled by the user. [Bug 1016796]
    LDFLAGS_ARCH=""
    TCL_EXPORT_FILE_SUFFIX=""
    UNSHARED_LIB_SUFFIX=""
    TCL_TRIM_DOTS='`echo ${PACKAGE_VERSION} | tr -d .`'
    ECHO_VERSION='`echo ${PACKAGE_VERSION}`'
    TCL_LIB_VERSIONS_OK=ok
    CFLAGS_DEBUG=-g
    CFLAGS_OPTIMIZE=-O
    if test "$GCC" = "yes" ; then
	CFLAGS_OPTIMIZE=-O2
	CFLAGS_WARNING="-Wall -Wno-implicit-int"
    else
	CFLAGS_WARNING=""
    fi
    TCL_NEEDS_EXP_FILE=0
    TCL_BUILD_EXP_FILE=""
    TCL_EXP_FILE=""
dnl FIXME: Replace AC_CHECK_PROG with AC_CHECK_TOOL once cross compiling is fixed.
dnl AC_CHECK_TOOL(AR, ar)
    AC_CHECK_PROG(AR, ar, ar)
    STLIB_LD='${AR} cr'
    LD_LIBRARY_PATH_VAR="LD_LIBRARY_PATH"
    case $system in
	windows)
	    # This is a 2-stage check to make sure we have the 64-bit SDK
	    # We have to know where the SDK is installed.
	    # This magic is based on MS Platform SDK for Win2003 SP1 - hobbs
	    # MACHINE is IX86 for LINK, but this is used by the manifest,
	    # which requires x86|amd64|ia64.
	    MACHINE="X86"
	    if test "$do64bit" != "no" ; then
		if test "x${MSSDK}x" = "xx" ; then
		    MSSDK="C:/Progra~1/Microsoft Platform SDK"
		fi
		MSSDK=`echo "$MSSDK" | sed -e  's!\\\!/!g'`
		PATH64=""
		case "$do64bit" in
		    amd64|x64|yes)
			MACHINE="AMD64" ; # default to AMD64 64-bit build
			PATH64="${MSSDK}/Bin/Win64/x86/AMD64"
			;;
		    ia64)
			MACHINE="IA64"
			PATH64="${MSSDK}/Bin/Win64"
			;;
		esac
		if test ! -d "${PATH64}" ; then
		    AC_MSG_WARN([Could not find 64-bit $MACHINE SDK to enable 64bit mode])
		    AC_MSG_WARN([Ensure latest Platform SDK is installed])
		    do64bit="no"
		else
		    AC_MSG_RESULT([   Using 64-bit $MACHINE mode])
		    do64bit_ok="yes"
		fi
	    fi

	    if test "$doWince" != "no" ; then
		if test "$do64bit" != "no" ; then
		    AC_MSG_ERROR([Windows/CE and 64-bit builds incompatible])
		fi
		if test "$GCC" = "yes" ; then
		    AC_MSG_ERROR([Windows/CE and GCC builds incompatible])
		fi
		TEA_PATH_CELIB
		# Set defaults for common evc4/PPC2003 setup
		# Currently Tcl requires 300+, possibly 420+ for sockets
		CEVERSION=420; 		# could be 211 300 301 400 420 ...
		TARGETCPU=ARMV4;	# could be ARMV4 ARM MIPS SH3 X86 ...
		ARCH=ARM;		# could be ARM MIPS X86EM ...
		PLATFORM="Pocket PC 2003"; # or "Pocket PC 2002"
		if test "$doWince" != "yes"; then
		    # If !yes then the user specified something
		    # Reset ARCH to allow user to skip specifying it
		    ARCH=
		    eval `echo $doWince | awk -F, '{ \
	    if (length([$]1)) { printf "CEVERSION=\"%s\"\n", [$]1; \
	    if ([$]1 < 400)   { printf "PLATFORM=\"Pocket PC 2002\"\n" } }; \
	    if (length([$]2)) { printf "TARGETCPU=\"%s\"\n", toupper([$]2) }; \
	    if (length([$]3)) { printf "ARCH=\"%s\"\n", toupper([$]3) }; \
	    if (length([$]4)) { printf "PLATFORM=\"%s\"\n", [$]4 }; \
		    }'`
		    if test "x${ARCH}" = "x" ; then
			ARCH=$TARGETCPU;
		    fi
		fi
		OSVERSION=WCE$CEVERSION;
	    	if test "x${WCEROOT}" = "x" ; then
			WCEROOT="C:/Program Files/Microsoft eMbedded C++ 4.0"
		    if test ! -d "${WCEROOT}" ; then
			WCEROOT="C:/Program Files/Microsoft eMbedded Tools"
		    fi
		fi
		if test "x${SDKROOT}" = "x" ; then
		    SDKROOT="C:/Program Files/Windows CE Tools"
		    if test ! -d "${SDKROOT}" ; then
			SDKROOT="C:/Windows CE Tools"
		    fi
		fi
		WCEROOT=`echo "$WCEROOT" | sed -e 's!\\\!/!g'`
		SDKROOT=`echo "$SDKROOT" | sed -e 's!\\\!/!g'`
		if test ! -d "${SDKROOT}/${OSVERSION}/${PLATFORM}/Lib/${TARGETCPU}" \
		    -o ! -d "${WCEROOT}/EVC/${OSVERSION}/bin"; then
		    AC_MSG_ERROR([could not find PocketPC SDK or target compiler to enable WinCE mode [$CEVERSION,$TARGETCPU,$ARCH,$PLATFORM]])
		    doWince="no"
		else
		    # We could PATH_NOSPACE these, but that's not important,
		    # as long as we quote them when used.
		    CEINCLUDE="${SDKROOT}/${OSVERSION}/${PLATFORM}/include"
		    if test -d "${CEINCLUDE}/${TARGETCPU}" ; then
			CEINCLUDE="${CEINCLUDE}/${TARGETCPU}"
		    fi
		    CELIBPATH="${SDKROOT}/${OSVERSION}/${PLATFORM}/Lib/${TARGETCPU}"
    		fi
	    fi

	    if test "$GCC" != "yes" ; then
	        if test "${SHARED_BUILD}" = "0" ; then
		    runtime=-MT
	        else
		    runtime=-MD
	        fi

                if test "$do64bit" != "no" ; then
		    # All this magic is necessary for the Win64 SDK RC1 - hobbs
		    CC="\"${PATH64}/cl.exe\""
		    CFLAGS="${CFLAGS} -I\"${MSSDK}/Include\" -I\"${MSSDK}/Include/crt\" -I\"${MSSDK}/Include/crt/sys\""
		    RC="\"${MSSDK}/bin/rc.exe\""
		    lflags="-nologo -MACHINE:${MACHINE} -LIBPATH:\"${MSSDK}/Lib/${MACHINE}\""
		    LINKBIN="\"${PATH64}/link.exe\""
		    CFLAGS_DEBUG="-nologo -Zi -Od -W3 ${runtime}d"
		    CFLAGS_OPTIMIZE="-nologo -O2 -W2 ${runtime}"
		    # Avoid 'unresolved external symbol __security_cookie'
		    # errors, c.f. http://support.microsoft.com/?id=894573
		    TEA_ADD_LIBS([bufferoverflowU.lib])
		elif test "$doWince" != "no" ; then
		    CEBINROOT="${WCEROOT}/EVC/${OSVERSION}/bin"
		    if test "${TARGETCPU}" = "X86"; then
			CC="\"${CEBINROOT}/cl.exe\""
		    else
			CC="\"${CEBINROOT}/cl${ARCH}.exe\""
		    fi
		    CFLAGS="$CFLAGS -I\"${CELIB_DIR}/inc\" -I\"${CEINCLUDE}\""
		    RC="\"${WCEROOT}/Common/EVC/bin/rc.exe\""
		    arch=`echo ${ARCH} | awk '{print tolower([$]0)}'`
		    defs="${ARCH} _${ARCH}_ ${arch} PALM_SIZE _MT _WINDOWS"
		    if test "${SHARED_BUILD}" = "1" ; then
			# Static CE builds require static celib as well
		    	defs="${defs} _DLL"
		    fi
		    for i in $defs ; do
			AC_DEFINE_UNQUOTED($i, 1, [WinCE def ]$i)
		    done
		    AC_DEFINE_UNQUOTED(_WIN32_WCE, $CEVERSION, [_WIN32_WCE version])
		    AC_DEFINE_UNQUOTED(UNDER_CE, $CEVERSION, [UNDER_CE version])
		    CFLAGS_DEBUG="-nologo -Zi -Od"
		    CFLAGS_OPTIMIZE="-nologo -Ox"
		    lversion=`echo ${CEVERSION} | sed -e 's/\(.\)\(..\)/\1\.\2/'`
		    lflags="-MACHINE:${ARCH} -LIBPATH:\"${CELIBPATH}\" -subsystem:windowsce,${lversion} -nologo"
		    LINKBIN="\"${CEBINROOT}/link.exe\""
		    AC_SUBST(CELIB_DIR)
		else
		    RC="rc"
		    lflags="-nologo"
    		    LINKBIN="link"
		    CFLAGS_DEBUG="-nologo -Z7 -Od -W3 -WX ${runtime}d"
		    CFLAGS_OPTIMIZE="-nologo -O2 -W2 ${runtime}"
		fi
	    fi

	    if test "$GCC" = "yes"; then
		# mingw gcc mode
		RC="windres"
		CFLAGS_DEBUG="-g"
		CFLAGS_OPTIMIZE="-O2 -fomit-frame-pointer"
		SHLIB_LD="$CC -shared"
		UNSHARED_LIB_SUFFIX='${TCL_TRIM_DOTS}.a'
		LDFLAGS_CONSOLE="-wl,--subsystem,console ${lflags}"
		LDFLAGS_WINDOW="-wl,--subsystem,windows ${lflags}"
	    else
		SHLIB_LD="${LINKBIN} -dll ${lflags}"
		# link -lib only works when -lib is the first arg
		STLIB_LD="${LINKBIN} -lib ${lflags}"
		UNSHARED_LIB_SUFFIX='${TCL_TRIM_DOTS}.lib'
		PATHTYPE=-w
		# For information on what debugtype is most useful, see:
		# http://msdn.microsoft.com/library/en-us/dnvc60/html/gendepdebug.asp
		# This essentially turns it all on.
		LDFLAGS_DEBUG="-debug:full -debugtype:both -warn:2"
		LDFLAGS_OPTIMIZE="-release"
		if test "$doWince" != "no" ; then
		    LDFLAGS_CONSOLE="-link ${lflags}"
		    LDFLAGS_WINDOW=${LDFLAGS_CONSOLE}
		else
		    LDFLAGS_CONSOLE="-link -subsystem:console ${lflags}"
		    LDFLAGS_WINDOW="-link -subsystem:windows ${lflags}"
		fi
	    fi

	    SHLIB_LD_LIBS='${LIBS}'
	    SHLIB_SUFFIX=".dll"
	    SHARED_LIB_SUFFIX='${TCL_TRIM_DOTS}.dll'

	    TCL_LIB_VERSIONS_OK=nodots
	    # Bogus to avoid getting this turned off
	    DL_OBJS="tclLoadNone.obj"
    	    ;;
	AIX-*)
	    if test "${TCL_THREADS}" = "1" -a "$GCC" != "yes" ; then
		# AIX requires the _r compiler when gcc isn't being used
		case "${CC}" in
		    *_r)
			# ok ...
			;;
		    *)
			CC=${CC}_r
			;;
		esac
		AC_MSG_RESULT([Using $CC for compiling with threads])
	    fi
	    LIBS="$LIBS -lc"
	    SHLIB_CFLAGS=""
	    SHLIB_LD_LIBS='${LIBS}'
	    SHLIB_SUFFIX=".so"

	    DL_OBJS="tclLoadDl.o"
	    LD_LIBRARY_PATH_VAR="LIBPATH"

	    # Check to enable 64-bit flags for compiler/linker on AIX 4+
	    if test "$do64bit" = "yes" -a "`uname -v`" -gt "3" ; then
		if test "$GCC" = "yes" ; then
		    AC_MSG_WARN([64bit mode not supported with GCC on $system])
		else 
		    do64bit_ok=yes
		    CFLAGS="$CFLAGS -q64"
		    LDFLAGS_ARCH="-q64"
		    RANLIB="${RANLIB} -X64"
		    AR="${AR} -X64"
		    SHLIB_LD_FLAGS="-b64"
		fi
	    fi

	    if test "`uname -m`" = "ia64" ; then
		# AIX-5 uses ELF style dynamic libraries on IA-64, but not PPC
		SHLIB_LD="/usr/ccs/bin/ld -G -z text"
		# AIX-5 has dl* in libc.so
		DL_LIBS=""
		if test "$GCC" = "yes" ; then
		    CC_SEARCH_FLAGS='-Wl,-R,${LIB_RUNTIME_DIR}'
		else
		    CC_SEARCH_FLAGS='-R${LIB_RUNTIME_DIR}'
		fi
		LD_SEARCH_FLAGS='-R ${LIB_RUNTIME_DIR}'
	    else
		if test "$GCC" = "yes" ; then
		    SHLIB_LD="gcc -shared"
		else
		    SHLIB_LD="/bin/ld -bhalt:4 -bM:SRE -bE:lib.exp -H512 -T512 -bnoentry"
		fi
		SHLIB_LD="${TCL_SRC_DIR}/unix/ldAix ${SHLIB_LD} ${SHLIB_LD_FLAGS}"
		DL_LIBS="-ldl"
		CC_SEARCH_FLAGS='-L${LIB_RUNTIME_DIR}'
		LD_SEARCH_FLAGS=${CC_SEARCH_FLAGS}
		TCL_NEEDS_EXP_FILE=1
		TCL_EXPORT_FILE_SUFFIX='${PACKAGE_VERSION}.exp'
	    fi

	    # AIX v<=4.1 has some different flags than 4.2+
	    if test "$system" = "AIX-4.1" -o "`uname -v`" -lt "4" ; then
		AC_LIBOBJ([tclLoadAix])
		DL_LIBS="-lld"
	    fi

	    # On AIX <=v4 systems, libbsd.a has to be linked in to support
	    # non-blocking file IO.  This library has to be linked in after
	    # the MATH_LIBS or it breaks the pow() function.  The way to
	    # insure proper sequencing, is to add it to the tail of MATH_LIBS.
	    # This library also supplies gettimeofday.
	    #
	    # AIX does not have a timezone field in struct tm. When the AIX
	    # bsd library is used, the timezone global and the gettimeofday
	    # methods are to be avoided for timezone deduction instead, we
	    # deduce the timezone by comparing the localtime result on a
	    # known GMT value.

	    AC_CHECK_LIB(bsd, gettimeofday, libbsd=yes, libbsd=no)
	    if test $libbsd = yes; then
	    	MATH_LIBS="$MATH_LIBS -lbsd"
	    	AC_DEFINE(USE_DELTA_FOR_TZ, 1, [Do we need a special AIX hack for timezones?])
	    fi
	    ;;
	BeOS*)
	    SHLIB_CFLAGS="-fPIC"
	    SHLIB_LD="${CC} -nostart"
	    SHLIB_LD_LIBS='${LIBS}'
	    SHLIB_SUFFIX=".so"
	    DL_OBJS="tclLoadDl.o"
	    DL_LIBS="-ldl"

	    #-----------------------------------------------------------
	    # Check for inet_ntoa in -lbind, for BeOS (which also needs
	    # -lsocket, even if the network functions are in -lnet which
	    # is always linked to, for compatibility.
	    #-----------------------------------------------------------
	    AC_CHECK_LIB(bind, inet_ntoa, [LIBS="$LIBS -lbind -lsocket"])
	    ;;
	BSD/OS-2.1*|BSD/OS-3*)
	    SHLIB_CFLAGS=""
	    SHLIB_LD="shlicc -r"
	    SHLIB_LD_LIBS='${LIBS}'
	    SHLIB_SUFFIX=".so"
	    DL_OBJS="tclLoadDl.o"
	    DL_LIBS="-ldl"
	    CC_SEARCH_FLAGS=""
	    LD_SEARCH_FLAGS=""
	    ;;
	BSD/OS-4.*)
	    SHLIB_CFLAGS="-export-dynamic -fPIC"
	    SHLIB_LD="cc -shared"
	    SHLIB_LD_LIBS='${LIBS}'
	    SHLIB_SUFFIX=".so"
	    DL_OBJS="tclLoadDl.o"
	    DL_LIBS="-ldl"
	    LDFLAGS="$LDFLAGS -export-dynamic"
	    CC_SEARCH_FLAGS=""
	    LD_SEARCH_FLAGS=""
	    ;;
	dgux*)
	    SHLIB_CFLAGS="-K PIC"
	    SHLIB_LD="cc -G"
	    SHLIB_LD_LIBS=""
	    SHLIB_SUFFIX=".so"
	    DL_OBJS="tclLoadDl.o"
	    DL_LIBS="-ldl"
	    CC_SEARCH_FLAGS=""
	    LD_SEARCH_FLAGS=""
	    ;;
	HP-UX-*.11.*)
	    # Use updated header definitions where possible
	    AC_DEFINE(_XOPEN_SOURCE_EXTENDED, 1, [Do we want to use the XOPEN network library?])
	    # Needed by Tcl, but not most extensions
	    #AC_DEFINE(_XOPEN_SOURCE, 1, [Do we want to use the XOPEN network library?])
	    #LIBS="$LIBS -lxnet"               # Use the XOPEN network library

	    if test "`uname -m`" = "ia64" ; then
		SHLIB_SUFFIX=".so"
	    else
		SHLIB_SUFFIX=".sl"
	    fi
	    AC_CHECK_LIB(dld, shl_load, tcl_ok=yes, tcl_ok=no)
	    if test "$tcl_ok" = yes; then
		SHLIB_CFLAGS="+z"
		SHLIB_LD="ld -b"
		SHLIB_LD_LIBS='${LIBS}'
		DL_OBJS="tclLoadShl.o"
		DL_LIBS="-ldld"
		LDFLAGS="$LDFLAGS -Wl,-E"
		CC_SEARCH_FLAGS='-Wl,+s,+b,${LIB_RUNTIME_DIR}:.'
		LD_SEARCH_FLAGS='+s +b ${LIB_RUNTIME_DIR}:.'
		LD_LIBRARY_PATH_VAR="SHLIB_PATH"
	    fi
	    if test "$GCC" = "yes" ; then
		SHLIB_LD="gcc -shared"
		SHLIB_LD_LIBS='${LIBS}'
		LD_SEARCH_FLAGS=${CC_SEARCH_FLAGS}
	    fi

	    # Users may want PA-RISC 1.1/2.0 portable code - needs HP cc
	    #CFLAGS="$CFLAGS +DAportable"

	    # Check to enable 64-bit flags for compiler/linker
	    if test "$do64bit" = "yes" ; then
		if test "$GCC" = "yes" ; then
		    hpux_arch=`${CC} -dumpmachine`
		    case $hpux_arch in
			hppa64*)
			    # 64-bit gcc in use.  Fix flags for GNU ld.
			    do64bit_ok=yes
			    SHLIB_LD="${CC} -shared"
			    SHLIB_LD_LIBS='${LIBS}'
			    CC_SEARCH_FLAGS='-Wl,-rpath,${LIB_RUNTIME_DIR}'
			    LD_SEARCH_FLAGS=${CC_SEARCH_FLAGS}
			    ;;
			*)
			    AC_MSG_WARN([64bit mode not supported with GCC on $system])
			    ;;
		    esac
		else
		    do64bit_ok=yes
		    CFLAGS="$CFLAGS +DD64"
		    LDFLAGS_ARCH="+DD64"
		fi
	    fi
	    ;;
	HP-UX-*.08.*|HP-UX-*.09.*|HP-UX-*.10.*)
	    SHLIB_SUFFIX=".sl"
	    AC_CHECK_LIB(dld, shl_load, tcl_ok=yes, tcl_ok=no)
	    if test "$tcl_ok" = yes; then
		SHLIB_CFLAGS="+z"
		SHLIB_LD="ld -b"
		SHLIB_LD_LIBS=""
		DL_OBJS="tclLoadShl.o"
		DL_LIBS="-ldld"
		LDFLAGS="$LDFLAGS -Wl,-E"
		CC_SEARCH_FLAGS='-Wl,+s,+b,${LIB_RUNTIME_DIR}:.'
		LD_SEARCH_FLAGS='+s +b ${LIB_RUNTIME_DIR}:.'
		LD_LIBRARY_PATH_VAR="SHLIB_PATH"
	    fi
	    ;;
	IRIX-5.*)
	    SHLIB_CFLAGS=""
	    SHLIB_LD="ld -shared -rdata_shared"
	    SHLIB_LD_LIBS='${LIBS}'
	    SHLIB_SUFFIX=".so"
	    DL_OBJS="tclLoadDl.o"
	    DL_LIBS=""
	    CC_SEARCH_FLAGS='-Wl,-rpath,${LIB_RUNTIME_DIR}'
	    LD_SEARCH_FLAGS='-rpath ${LIB_RUNTIME_DIR}'
	    ;;
	IRIX-6.*)
	    SHLIB_CFLAGS=""
	    SHLIB_LD="ld -n32 -shared -rdata_shared"
	    SHLIB_LD_LIBS='${LIBS}'
	    SHLIB_SUFFIX=".so"
	    DL_OBJS="tclLoadDl.o"
	    DL_LIBS=""
	    CC_SEARCH_FLAGS='-Wl,-rpath,${LIB_RUNTIME_DIR}'
	    LD_SEARCH_FLAGS='-rpath ${LIB_RUNTIME_DIR}'
	    if test "$GCC" = "yes" ; then
		CFLAGS="$CFLAGS -mabi=n32"
		LDFLAGS="$LDFLAGS -mabi=n32"
	    else
		case $system in
		    IRIX-6.3)
			# Use to build 6.2 compatible binaries on 6.3.
			CFLAGS="$CFLAGS -n32 -D_OLD_TERMIOS"
			;;
		    *)
			CFLAGS="$CFLAGS -n32"
			;;
		esac
		LDFLAGS="$LDFLAGS -n32"
	    fi
	    ;;
	IRIX64-6.*)
	    SHLIB_CFLAGS=""
	    SHLIB_LD="ld -n32 -shared -rdata_shared"
	    SHLIB_LD_LIBS='${LIBS}'
	    SHLIB_SUFFIX=".so"
	    DL_OBJS="tclLoadDl.o"
	    DL_LIBS=""
	    CC_SEARCH_FLAGS='-Wl,-rpath,${LIB_RUNTIME_DIR}'
	    LD_SEARCH_FLAGS='-rpath ${LIB_RUNTIME_DIR}'

	    # Check to enable 64-bit flags for compiler/linker

	    if test "$do64bit" = "yes" ; then
	        if test "$GCC" = "yes" ; then
	            AC_MSG_WARN([64bit mode not supported by gcc])
	        else
	            do64bit_ok=yes
	            SHLIB_LD="ld -64 -shared -rdata_shared"
	            CFLAGS="$CFLAGS -64"
	            LDFLAGS_ARCH="-64"
	        fi
	    fi
	    ;;
	Linux*)
	    SHLIB_CFLAGS="-fPIC"
	    SHLIB_LD_LIBS='${LIBS}'
	    SHLIB_SUFFIX=".so"

	    CFLAGS_OPTIMIZE="-O2 -fomit-frame-pointer"
	    # egcs-2.91.66 on Redhat Linux 6.0 generates lots of warnings 
	    # when you inline the string and math operations.  Turn this off to
	    # get rid of the warnings.
	    #CFLAGS_OPTIMIZE="${CFLAGS_OPTIMIZE} -D__NO_STRING_INLINES -D__NO_MATH_INLINES"

	    # TEA specific: use LDFLAGS_DEFAULT instead of LDFLAGS here:
	    SHLIB_LD='${CC} -shared ${CFLAGS} ${LDFLAGS_DEFAULT}'
	    DL_OBJS="tclLoadDl.o"
	    DL_LIBS="-ldl"
	    LDFLAGS="$LDFLAGS -Wl,--export-dynamic"
	    CC_SEARCH_FLAGS='-Wl,-rpath,${LIB_RUNTIME_DIR}'
	    LD_SEARCH_FLAGS=${CC_SEARCH_FLAGS}
	    if test "`uname -m`" = "alpha" ; then
		CFLAGS="$CFLAGS -mieee"
	    fi
	    if test $do64bit = yes; then
		AC_CACHE_CHECK([if compiler accepts -m64 flag], tcl_cv_cc_m64, [
		    hold_cflags=$CFLAGS
		    CFLAGS="$CFLAGS -m64"
		    AC_TRY_LINK(,, tcl_cv_cc_m64=yes, tcl_cv_cc_m64=no)
		    CFLAGS=$hold_cflags])
		if test $tcl_cv_cc_m64 = yes; then
		    CFLAGS="$CFLAGS -m64"
		    do64bit_ok=yes
		fi
	    fi

	    # The combo of gcc + glibc has a bug related
	    # to inlining of functions like strtod(). The
	    # -fno-builtin flag should address this problem
	    # but it does not work. The -fno-inline flag
	    # is kind of overkill but it works.
	    # Disable inlining only when one of the
	    # files in compat/*.c is being linked in.
	    if test x"${USE_COMPAT}" != x ; then
	        CFLAGS="$CFLAGS -fno-inline"
	    fi

	    ;;
	GNU*)
	    SHLIB_CFLAGS="-fPIC"
	    SHLIB_LD_LIBS='${LIBS}'
	    SHLIB_SUFFIX=".so"

	    SHLIB_LD="${CC} -shared"
	    DL_OBJS=""
	    DL_LIBS="-ldl"
	    LDFLAGS="$LDFLAGS -Wl,--export-dynamic"
	    CC_SEARCH_FLAGS=""
	    LD_SEARCH_FLAGS=""
	    if test "`uname -m`" = "alpha" ; then
		CFLAGS="$CFLAGS -mieee"
	    fi
	    ;;
	Lynx*)
	    SHLIB_CFLAGS="-fPIC"
	    SHLIB_LD_LIBS='${LIBS}'
	    SHLIB_SUFFIX=".so"
	    CFLAGS_OPTIMIZE=-02
	    SHLIB_LD="${CC} -shared "
	    DL_OBJS="tclLoadDl.o"
	    DL_LIBS="-mshared -ldl"
	    LD_FLAGS="-Wl,--export-dynamic"
	    CC_SEARCH_FLAGS='-Wl,-rpath,${LIB_RUNTIME_DIR}'
	    LD_SEARCH_FLAGS='-Wl,-rpath,${LIB_RUNTIME_DIR}'
	    ;;
	MP-RAS-02*)
	    SHLIB_CFLAGS="-K PIC"
	    SHLIB_LD="cc -G"
	    SHLIB_LD_LIBS=""
	    SHLIB_SUFFIX=".so"
	    DL_OBJS="tclLoadDl.o"
	    DL_LIBS="-ldl"
	    CC_SEARCH_FLAGS=""
	    LD_SEARCH_FLAGS=""
	    ;;
	MP-RAS-*)
	    SHLIB_CFLAGS="-K PIC"
	    SHLIB_LD="cc -G"
	    SHLIB_LD_LIBS=""
	    SHLIB_SUFFIX=".so"
	    DL_OBJS="tclLoadDl.o"
	    DL_LIBS="-ldl"
	    LDFLAGS="$LDFLAGS -Wl,-Bexport"
	    CC_SEARCH_FLAGS=""
	    LD_SEARCH_FLAGS=""
	    ;;
	NetBSD-*|FreeBSD-[[1-2]].*)
	    # NetBSD/SPARC needs -fPIC, -fpic will not do.
	    SHLIB_CFLAGS="-fPIC"
	    SHLIB_LD="ld -Bshareable -x"
	    SHLIB_LD_LIBS='${LIBS}'
	    SHLIB_SUFFIX=".so"
	    DL_OBJS="tclLoadDl.o"
	    DL_LIBS=""
	    CC_SEARCH_FLAGS='-Wl,-rpath,${LIB_RUNTIME_DIR}'
	    LD_SEARCH_FLAGS='-rpath ${LIB_RUNTIME_DIR}'
	    AC_CACHE_CHECK([for ELF], tcl_cv_ld_elf, [
		AC_EGREP_CPP(yes, [
#ifdef __ELF__
	yes
#endif
		], tcl_cv_ld_elf=yes, tcl_cv_ld_elf=no)])
	    if test $tcl_cv_ld_elf = yes; then
		SHARED_LIB_SUFFIX='${TCL_TRIM_DOTS}.so'
	    else
		SHARED_LIB_SUFFIX='${TCL_TRIM_DOTS}.so.1.0'
	    fi

	    # Ancient FreeBSD doesn't handle version numbers with dots.

	    UNSHARED_LIB_SUFFIX='${TCL_TRIM_DOTS}.a'
	    TCL_LIB_VERSIONS_OK=nodots
	    ;;
	OpenBSD-*)
	    # OpenBSD/SPARC[64] needs -fPIC, -fpic will not do.
	    case `machine` in
	    sparc|sparc64)
		SHLIB_CFLAGS="-fPIC";;
	    *)
		SHLIB_CFLAGS="-fpic";;
	    esac
	    SHLIB_LD="${CC} -shared ${SHLIB_CFLAGS}"
	    SHLIB_LD_LIBS='${LIBS}'
	    SHLIB_SUFFIX=".so"
	    DL_OBJS="tclLoadDl.o"
	    DL_LIBS=""
	    CC_SEARCH_FLAGS='-Wl,-rpath,${LIB_RUNTIME_DIR}'
	    LD_SEARCH_FLAGS=${CC_SEARCH_FLAGS}
	    SHARED_LIB_SUFFIX='${TCL_TRIM_DOTS}.so.1.0'
	    AC_CACHE_CHECK([for ELF], tcl_cv_ld_elf, [
		AC_EGREP_CPP(yes, [
#ifdef __ELF__
	yes
#endif
		], tcl_cv_ld_elf=yes, tcl_cv_ld_elf=no)])
	    if test $tcl_cv_ld_elf = yes; then
		LDFLAGS=-Wl,-export-dynamic
	    else
		LDFLAGS=""
	    fi

	    # OpenBSD doesn't do version numbers with dots.
	    UNSHARED_LIB_SUFFIX='${TCL_TRIM_DOTS}.a'
	    TCL_LIB_VERSIONS_OK=nodots
	    ;;
	FreeBSD-*)
	    # FreeBSD 3.* and greater have ELF.
	    SHLIB_CFLAGS="-fPIC"
	    SHLIB_LD="ld -Bshareable -x"
	    SHLIB_LD_LIBS='${LIBS}'
	    SHLIB_SUFFIX=".so"
	    DL_OBJS="tclLoadDl.o"
	    DL_LIBS=""
	    LDFLAGS="$LDFLAGS -export-dynamic"
	    CC_SEARCH_FLAGS='-Wl,-rpath,${LIB_RUNTIME_DIR}'
	    LD_SEARCH_FLAGS='-rpath ${LIB_RUNTIME_DIR}'
	    if test "${TCL_THREADS}" = "1" ; then
		# The -pthread needs to go in the CFLAGS, not LIBS
		LIBS=`echo $LIBS | sed s/-pthread//`
		CFLAGS="$CFLAGS -pthread"
	    	LDFLAGS="$LDFLAGS -pthread"
	    fi
	    case $system in
	    FreeBSD-3.*)
	    	# FreeBSD-3 doesn't handle version numbers with dots.
	    	UNSHARED_LIB_SUFFIX='${TCL_TRIM_DOTS}.a'
	    	SHARED_LIB_SUFFIX='${TCL_TRIM_DOTS}.so'
	    	TCL_LIB_VERSIONS_OK=nodots
		;;
	    esac
	    ;;
	Darwin-*)
	    CFLAGS_OPTIMIZE="-Os"
	    SHLIB_CFLAGS="-fno-common"
	    # To avoid discrepancies between what headers configure sees during
	    # preprocessing tests and compiling tests, move any -isysroot and
	    # -mmacosx-version-min flags from CFLAGS to CPPFLAGS:
	    CPPFLAGS="${CPPFLAGS} `echo " ${CFLAGS}" | \
		awk 'BEGIN {FS=" +-";ORS=" "}; {for (i=2;i<=NF;i++) \
		if ([$]i~/^(isysroot|mmacosx-version-min)/) print "-"[$]i}'`"
	    CFLAGS="`echo " ${CFLAGS}" | \
		awk 'BEGIN {FS=" +-";ORS=" "}; {for (i=2;i<=NF;i++) \
		if (!([$]i~/^(isysroot|mmacosx-version-min)/)) print "-"[$]i}'`"
	    if test $do64bit = yes; then
		case `arch` in
		    ppc)
			AC_CACHE_CHECK([if compiler accepts -arch ppc64 flag],
				tcl_cv_cc_arch_ppc64, [
			    hold_cflags=$CFLAGS
			    CFLAGS="$CFLAGS -arch ppc64 -mpowerpc64 -mcpu=G5"
			    AC_TRY_LINK(,, tcl_cv_cc_arch_ppc64=yes,
				    tcl_cv_cc_arch_ppc64=no)
			    CFLAGS=$hold_cflags])
			if test $tcl_cv_cc_arch_ppc64 = yes; then
			    CFLAGS="$CFLAGS -arch ppc64 -mpowerpc64 -mcpu=G5"
			    do64bit_ok=yes
			fi;;
		    i386)
			AC_CACHE_CHECK([if compiler accepts -arch x86_64 flag],
				tcl_cv_cc_arch_x86_64, [
			    hold_cflags=$CFLAGS
			    CFLAGS="$CFLAGS -arch x86_64"
			    AC_TRY_LINK(,, tcl_cv_cc_arch_x86_64=yes,
				    tcl_cv_cc_arch_x86_64=no)
			    CFLAGS=$hold_cflags])
			if test $tcl_cv_cc_arch_x86_64 = yes; then
			    CFLAGS="$CFLAGS -arch x86_64"
			    do64bit_ok=yes
			fi;;
		    *)
			AC_MSG_WARN([Don't know how enable 64-bit on architecture `arch`]);;
		esac
	    else
		# Check for combined 32-bit and 64-bit fat build
		echo "$CFLAGS " | grep -E -q -- '-arch (ppc64|x86_64) ' && \
		    echo "$CFLAGS " | grep -E -q -- '-arch (ppc|i386) ' && \
		    fat_32_64=yes
	    fi
	    # TEA specific: use LDFLAGS_DEFAULT instead of LDFLAGS here:
	    SHLIB_LD='${CC} -dynamiclib ${CFLAGS} ${LDFLAGS_DEFAULT}'
	    AC_CACHE_CHECK([if ld accepts -single_module flag], tcl_cv_ld_single_module, [
		hold_ldflags=$LDFLAGS
		LDFLAGS="$LDFLAGS -dynamiclib -Wl,-single_module"
		AC_TRY_LINK(, [int i;], tcl_cv_ld_single_module=yes, tcl_cv_ld_single_module=no)
		LDFLAGS=$hold_ldflags])
	    if test $tcl_cv_ld_single_module = yes; then
		SHLIB_LD="${SHLIB_LD} -Wl,-single_module"
	    fi
	    SHLIB_LD_LIBS='${LIBS}'
	    SHLIB_SUFFIX=".dylib"
	    DL_OBJS="tclLoadDyld.o"
	    DL_LIBS=""
	    # Don't use -prebind when building for Mac OS X 10.4 or later only:
	    test "`echo "${MACOSX_DEPLOYMENT_TARGET}" | awk -F '10\\.' '{print int([$]2)}'`" -lt 4 -a \
		"`echo "${CFLAGS}" | awk -F '-mmacosx-version-min=10\\.' '{print int([$]2)}'`" -lt 4 && \
		LDFLAGS="$LDFLAGS -prebind"
	    LDFLAGS="$LDFLAGS -headerpad_max_install_names"
	    AC_CACHE_CHECK([if ld accepts -search_paths_first flag], tcl_cv_ld_search_paths_first, [
		hold_ldflags=$LDFLAGS
		LDFLAGS="$LDFLAGS -Wl,-search_paths_first"
		AC_TRY_LINK(, [int i;], tcl_cv_ld_search_paths_first=yes, tcl_cv_ld_search_paths_first=no)
		LDFLAGS=$hold_ldflags])
	    if test $tcl_cv_ld_search_paths_first = yes; then
		LDFLAGS="$LDFLAGS -Wl,-search_paths_first"
	    fi
	    CC_SEARCH_FLAGS=""
	    LD_SEARCH_FLAGS=""
	    LD_LIBRARY_PATH_VAR="DYLD_LIBRARY_PATH"

	    # TEA specific: for Tk extensions, remove 64-bit arch flags from
	    # CFLAGS for combined 32-bit and 64-bit fat builds as neither TkAqua
	    # nor TkX11 can be built for 64-bit at present.
	    test "$fat_32_64" = yes && test -n "${TK_BIN_DIR}" && \
		CFLAGS="`echo "$CFLAGS " | sed -e 's/-arch ppc64 / /g' -e 's/-arch x86_64 / /g'`"
	    ;;
	NEXTSTEP-*)
	    SHLIB_CFLAGS=""
	    SHLIB_LD="cc -nostdlib -r"
	    SHLIB_LD_LIBS=""
	    SHLIB_SUFFIX=".so"
	    DL_OBJS="tclLoadNext.o"
	    DL_LIBS=""
	    CC_SEARCH_FLAGS=""
	    LD_SEARCH_FLAGS=""
	    ;;
	OS/390-*)
	    CFLAGS_OPTIMIZE=""		# Optimizer is buggy
	    AC_DEFINE(_OE_SOCKETS, 1,	# needed in sys/socket.h
		[Should OS/390 do the right thing with sockets?])
	    ;;      
	OSF1-1.0|OSF1-1.1|OSF1-1.2)
	    # OSF/1 1.[012] from OSF, and derivatives, including Paragon OSF/1
	    SHLIB_CFLAGS=""
	    # Hack: make package name same as library name
	    SHLIB_LD='ld -R -export $@:'
	    SHLIB_LD_LIBS=""
	    SHLIB_SUFFIX=".so"
	    DL_OBJS="tclLoadOSF.o"
	    DL_LIBS=""
	    CC_SEARCH_FLAGS=""
	    LD_SEARCH_FLAGS=""
	    ;;
	OSF1-1.*)
	    # OSF/1 1.3 from OSF using ELF, and derivatives, including AD2
	    SHLIB_CFLAGS="-fPIC"
	    if test "$SHARED_BUILD" = "1" ; then
	        SHLIB_LD="ld -shared"
	    else
	        SHLIB_LD="ld -non_shared"
	    fi
	    SHLIB_LD_LIBS=""
	    SHLIB_SUFFIX=".so"
	    DL_OBJS="tclLoadDl.o"
	    DL_LIBS=""
	    CC_SEARCH_FLAGS=""
	    LD_SEARCH_FLAGS=""
	    ;;
	OSF1-V*)
	    # Digital OSF/1
	    SHLIB_CFLAGS=""
	    if test "$SHARED_BUILD" = "1" ; then
	        SHLIB_LD='ld -shared -expect_unresolved "*"'
	    else
	        SHLIB_LD='ld -non_shared -expect_unresolved "*"'
	    fi
	    SHLIB_LD_LIBS=""
	    SHLIB_SUFFIX=".so"
	    DL_OBJS="tclLoadDl.o"
	    DL_LIBS=""
	    CC_SEARCH_FLAGS='-Wl,-rpath,${LIB_RUNTIME_DIR}'
	    LD_SEARCH_FLAGS='-rpath ${LIB_RUNTIME_DIR}'
	    if test "$GCC" = "yes" ; then
		CFLAGS="$CFLAGS -mieee"
            else
		CFLAGS="$CFLAGS -DHAVE_TZSET -std1 -ieee"
	    fi
	    # see pthread_intro(3) for pthread support on osf1, k.furukawa
	    if test "${TCL_THREADS}" = "1" ; then
		CFLAGS="$CFLAGS -DHAVE_PTHREAD_ATTR_SETSTACKSIZE"
		CFLAGS="$CFLAGS -DTCL_THREAD_STACK_MIN=PTHREAD_STACK_MIN*64"
		LIBS=`echo $LIBS | sed s/-lpthreads//`
		if test "$GCC" = "yes" ; then
		    LIBS="$LIBS -lpthread -lmach -lexc"
		else
		    CFLAGS="$CFLAGS -pthread"
		    LDFLAGS="$LDFLAGS -pthread"
		fi
	    fi

	    ;;
	QNX-6*)
	    # QNX RTP
	    # This may work for all QNX, but it was only reported for v6.
	    SHLIB_CFLAGS="-fPIC"
	    SHLIB_LD="ld -Bshareable -x"
	    SHLIB_LD_LIBS=""
	    SHLIB_SUFFIX=".so"
	    DL_OBJS="tclLoadDl.o"
	    # dlopen is in -lc on QNX
	    DL_LIBS=""
	    CC_SEARCH_FLAGS=""
	    LD_SEARCH_FLAGS=""
	    ;;
	SCO_SV-3.2*)
	    # Note, dlopen is available only on SCO 3.2.5 and greater. However,
	    # this test works, since "uname -s" was non-standard in 3.2.4 and
	    # below.
	    if test "$GCC" = "yes" ; then
	    	SHLIB_CFLAGS="-fPIC -melf"
	    	LDFLAGS="$LDFLAGS -melf -Wl,-Bexport"
	    else
	    	SHLIB_CFLAGS="-Kpic -belf"
	    	LDFLAGS="$LDFLAGS -belf -Wl,-Bexport"
	    fi
	    SHLIB_LD="ld -G"
	    SHLIB_LD_LIBS=""
	    SHLIB_SUFFIX=".so"
	    DL_OBJS="tclLoadDl.o"
	    DL_LIBS=""
	    CC_SEARCH_FLAGS=""
	    LD_SEARCH_FLAGS=""
	    ;;
	SINIX*5.4*)
	    SHLIB_CFLAGS="-K PIC"
	    SHLIB_LD="cc -G"
	    SHLIB_LD_LIBS=""
	    SHLIB_SUFFIX=".so"
	    DL_OBJS="tclLoadDl.o"
	    DL_LIBS="-ldl"
	    CC_SEARCH_FLAGS=""
	    LD_SEARCH_FLAGS=""
	    ;;
	SunOS-4*)
	    SHLIB_CFLAGS="-PIC"
	    SHLIB_LD="ld"
	    SHLIB_LD_LIBS=""
	    SHLIB_SUFFIX=".so"
	    DL_OBJS="tclLoadDl.o"
	    DL_LIBS="-ldl"
	    CC_SEARCH_FLAGS='-L${LIB_RUNTIME_DIR}'
	    LD_SEARCH_FLAGS=${CC_SEARCH_FLAGS}

	    # SunOS can't handle version numbers with dots in them in library
	    # specs, like -ltcl7.5, so use -ltcl75 instead.  Also, it
	    # requires an extra version number at the end of .so file names.
	    # So, the library has to have a name like libtcl75.so.1.0

	    SHARED_LIB_SUFFIX='${TCL_TRIM_DOTS}.so.1.0'
	    UNSHARED_LIB_SUFFIX='${TCL_TRIM_DOTS}.a'
	    TCL_LIB_VERSIONS_OK=nodots
	    ;;
	SunOS-5.[[0-6]])
	    # Careful to not let 5.10+ fall into this case

	    # Note: If _REENTRANT isn't defined, then Solaris
	    # won't define thread-safe library routines.

	    AC_DEFINE(_REENTRANT, 1, [Do we want the reentrant OS API?])
	    AC_DEFINE(_POSIX_PTHREAD_SEMANTICS, 1,
		[Do we really want to follow the standard? Yes we do!])

	    SHLIB_CFLAGS="-KPIC"

	    # Note: need the LIBS below, otherwise Tk won't find Tcl's
	    # symbols when dynamically loaded into tclsh.

	    SHLIB_LD_LIBS='${LIBS}'
	    SHLIB_SUFFIX=".so"
	    DL_OBJS="tclLoadDl.o"
	    DL_LIBS="-ldl"
	    if test "$GCC" = "yes" ; then
		SHLIB_LD="$CC -shared"
		CC_SEARCH_FLAGS='-Wl,-R,${LIB_RUNTIME_DIR}'
		LD_SEARCH_FLAGS=${CC_SEARCH_FLAGS}
	    else
		SHLIB_LD="/usr/ccs/bin/ld -G -z text"
		CC_SEARCH_FLAGS='-R ${LIB_RUNTIME_DIR}'
		LD_SEARCH_FLAGS=${CC_SEARCH_FLAGS}
	    fi
	    ;;
	SunOS-5*)
	    # Note: If _REENTRANT isn't defined, then Solaris
	    # won't define thread-safe library routines.

	    AC_DEFINE(_REENTRANT, 1, [Do we want the reentrant OS API?])
	    AC_DEFINE(_POSIX_PTHREAD_SEMANTICS, 1,
		[Do we really want to follow the standard? Yes we do!])

	    SHLIB_CFLAGS="-KPIC"

	    # Check to enable 64-bit flags for compiler/linker
	    if test "$do64bit" = "yes" ; then
		arch=`isainfo`
		if test "$arch" = "sparcv9 sparc" ; then
			if test "$GCC" = "yes" ; then
			    if test "`gcc -dumpversion | awk -F. '{print [$]1}'`" -lt "3" ; then
				AC_MSG_WARN([64bit mode not supported with GCC < 3.2 on $system])
			    else
				do64bit_ok=yes
				CFLAGS="$CFLAGS -m64 -mcpu=v9"
				LDFLAGS="$LDFLAGS -m64 -mcpu=v9"
				SHLIB_CFLAGS="-fPIC"
			    fi
			else
			    do64bit_ok=yes
			    if test "$do64bitVIS" = "yes" ; then
				CFLAGS="$CFLAGS -xarch=v9a"
			    	LDFLAGS_ARCH="-xarch=v9a"
			    else
				CFLAGS="$CFLAGS -xarch=v9"
			    	LDFLAGS_ARCH="-xarch=v9"
			    fi
			    # Solaris 64 uses this as well
			    #LD_LIBRARY_PATH_VAR="LD_LIBRARY_PATH_64"
			fi
		elif test "$arch" = "amd64 i386" ; then
		    if test "$GCC" = "yes" ; then
			AC_MSG_WARN([64bit mode not supported with GCC on $system])
		    else
			do64bit_ok=yes
			CFLAGS="$CFLAGS -xarch=amd64"
			LDFLAGS="$LDFLAGS -xarch=amd64"
		    fi
		else
		    AC_MSG_WARN([64bit mode not supported for $arch])
		fi
	    fi
	    
	    # Note: need the LIBS below, otherwise Tk won't find Tcl's
	    # symbols when dynamically loaded into tclsh.

	    SHLIB_LD_LIBS='${LIBS}'
	    SHLIB_SUFFIX=".so"
	    DL_OBJS="tclLoadDl.o"
	    DL_LIBS="-ldl"
	    if test "$GCC" = "yes" ; then
		SHLIB_LD="$CC -shared"
		CC_SEARCH_FLAGS='-Wl,-R,${LIB_RUNTIME_DIR}'
		LD_SEARCH_FLAGS=${CC_SEARCH_FLAGS}
		if test "$do64bit_ok" = "yes" ; then
		    # We need to specify -static-libgcc or we need to
		    # add the path to the sparv9 libgcc.
		    # JH: static-libgcc is necessary for core Tcl, but may
		    # not be necessary for extensions.
		    SHLIB_LD="$SHLIB_LD -m64 -mcpu=v9 -static-libgcc"
		    # for finding sparcv9 libgcc, get the regular libgcc
		    # path, remove so name and append 'sparcv9'
		    #v9gcclibdir="`gcc -print-file-name=libgcc_s.so` | ..."
		    #CC_SEARCH_FLAGS="${CC_SEARCH_FLAGS},-R,$v9gcclibdir"
		fi
	    else
		SHLIB_LD="/usr/ccs/bin/ld -G -z text"
		CC_SEARCH_FLAGS='-Wl,-R,${LIB_RUNTIME_DIR}'
		LD_SEARCH_FLAGS='-R ${LIB_RUNTIME_DIR}'
	    fi
	    ;;
	UNIX_SV* | UnixWare-5*)
	    SHLIB_CFLAGS="-KPIC"
	    SHLIB_LD="cc -G"
	    SHLIB_LD_LIBS=""
	    SHLIB_SUFFIX=".so"
	    DL_OBJS="tclLoadDl.o"
	    DL_LIBS="-ldl"
	    # Some UNIX_SV* systems (unixware 1.1.2 for example) have linkers
	    # that don't grok the -Bexport option.  Test that it does.
	    AC_CACHE_CHECK([for ld accepts -Bexport flag], tcl_cv_ld_Bexport, [
		hold_ldflags=$LDFLAGS
		LDFLAGS="$LDFLAGS -Wl,-Bexport"
		AC_TRY_LINK(, [int i;], tcl_cv_ld_Bexport=yes, tcl_cv_ld_Bexport=no)
	        LDFLAGS=$hold_ldflags])
	    if test $tcl_cv_ld_Bexport = yes; then
		LDFLAGS="$LDFLAGS -Wl,-Bexport"
	    fi
	    CC_SEARCH_FLAGS=""
	    LD_SEARCH_FLAGS=""
	    ;;
    esac

    if test "$do64bit" = "yes" -a "$do64bit_ok" = "no" ; then
	AC_MSG_WARN([64bit support being disabled -- don't know magic for this platform])
    fi

dnl # Add any CPPFLAGS set in the environment to our CFLAGS, but delay doing so
dnl # until the end of configure, as configure's compile and link tests use
dnl # both CPPFLAGS and CFLAGS (unlike our compile and link) but configure's
dnl # preprocessing tests use only CPPFLAGS.
    AC_CONFIG_COMMANDS_PRE([CFLAGS="${CFLAGS} ${CPPFLAGS}"; CPPFLAGS=""])

    # Step 4: disable dynamic loading if requested via a command-line switch.

    AC_ARG_ENABLE(load,
	AC_HELP_STRING([--enable-load],
	    [allow dynamic loading and "load" command (default: on)]),
	[tcl_ok=$enableval], [tcl_ok=yes])
    if test "$tcl_ok" = "no"; then
	DL_OBJS=""
    fi

    if test "x$DL_OBJS" != "x" ; then
	BUILD_DLTEST="\$(DLTEST_TARGETS)"
    else
	echo "Can't figure out how to do dynamic loading or shared libraries"
	echo "on this system."
	SHLIB_CFLAGS=""
	SHLIB_LD=""
	SHLIB_SUFFIX=""
	DL_OBJS="tclLoadNone.o"
	DL_LIBS=""
	LDFLAGS="$LDFLAGS_ORIG"
	CC_SEARCH_FLAGS=""
	LD_SEARCH_FLAGS=""
	BUILD_DLTEST=""
    fi
    LDFLAGS="$LDFLAGS $LDFLAGS_ARCH"

    # If we're running gcc, then change the C flags for compiling shared
    # libraries to the right flags for gcc, instead of those for the
    # standard manufacturer compiler.

    if test "$DL_OBJS" != "tclLoadNone.o" ; then
	if test "$GCC" = "yes" ; then
	    case $system in
		AIX-*)
		    ;;
		BSD/OS*)
		    ;;
		IRIX*)
		    ;;
		NetBSD-*|FreeBSD-*)
		    ;;
		Darwin-*)
		    ;;
		SCO_SV-3.2*)
		    ;;
		windows)
		    ;;
		*)
		    SHLIB_CFLAGS="-fPIC"
		    ;;
	    esac
	fi
    fi

    if test "$SHARED_LIB_SUFFIX" = "" ; then
	SHARED_LIB_SUFFIX='${PACKAGE_VERSION}${SHLIB_SUFFIX}'
    fi
    if test "$UNSHARED_LIB_SUFFIX" = "" ; then
	UNSHARED_LIB_SUFFIX='${PACKAGE_VERSION}.a'
    fi

    AC_SUBST(DL_LIBS)

    AC_SUBST(CFLAGS_DEBUG)
    AC_SUBST(CFLAGS_OPTIMIZE)
    AC_SUBST(CFLAGS_WARNING)

    AC_SUBST(STLIB_LD)
    AC_SUBST(SHLIB_LD)

    AC_SUBST(SHLIB_LD_LIBS)
    AC_SUBST(SHLIB_CFLAGS)

    AC_SUBST(LD_LIBRARY_PATH_VAR)

    # These must be called after we do the basic CFLAGS checks and
    # verify any possible 64-bit or similar switches are necessary
    TEA_TCL_EARLY_FLAGS
    TEA_TCL_64BIT_FLAGS
])

#--------------------------------------------------------------------
# TEA_SERIAL_PORT
#
#	Determine which interface to use to talk to the serial port.
#	Note that #include lines must begin in leftmost column for
#	some compilers to recognize them as preprocessor directives,
#	and some build environments have stdin not pointing at a
#	pseudo-terminal (usually /dev/null instead.)
#
# Arguments:
#	none
#	
# Results:
#
#	Defines only one of the following vars:
#		HAVE_SYS_MODEM_H
#		USE_TERMIOS
#		USE_TERMIO
#		USE_SGTTY
#
#--------------------------------------------------------------------

AC_DEFUN([TEA_SERIAL_PORT], [
    AC_CHECK_HEADERS(sys/modem.h)
    AC_CACHE_CHECK([termios vs. termio vs. sgtty], tcl_cv_api_serial, [
    AC_TRY_RUN([
#include <termios.h>

int main() {
    struct termios t;
    if (tcgetattr(0, &t) == 0) {
	cfsetospeed(&t, 0);
	t.c_cflag |= PARENB | PARODD | CSIZE | CSTOPB;
	return 0;
    }
    return 1;
}], tcl_cv_api_serial=termios, tcl_cv_api_serial=no, tcl_cv_api_serial=no)
    if test $tcl_cv_api_serial = no ; then
	AC_TRY_RUN([
#include <termio.h>

int main() {
    struct termio t;
    if (ioctl(0, TCGETA, &t) == 0) {
	t.c_cflag |= CBAUD | PARENB | PARODD | CSIZE | CSTOPB;
	return 0;
    }
    return 1;
}], tcl_cv_api_serial=termio, tcl_cv_api_serial=no, tcl_cv_api_serial=no)
    fi
    if test $tcl_cv_api_serial = no ; then
	AC_TRY_RUN([
#include <sgtty.h>

int main() {
    struct sgttyb t;
    if (ioctl(0, TIOCGETP, &t) == 0) {
	t.sg_ospeed = 0;
	t.sg_flags |= ODDP | EVENP | RAW;
	return 0;
    }
    return 1;
}], tcl_cv_api_serial=sgtty, tcl_cv_api_serial=no, tcl_cv_api_serial=no)
    fi
    if test $tcl_cv_api_serial = no ; then
	AC_TRY_RUN([
#include <termios.h>
#include <errno.h>

int main() {
    struct termios t;
    if (tcgetattr(0, &t) == 0
	|| errno == ENOTTY || errno == ENXIO || errno == EINVAL) {
	cfsetospeed(&t, 0);
	t.c_cflag |= PARENB | PARODD | CSIZE | CSTOPB;
	return 0;
    }
    return 1;
}], tcl_cv_api_serial=termios, tcl_cv_api_serial=no, tcl_cv_api_serial=no)
    fi
    if test $tcl_cv_api_serial = no; then
	AC_TRY_RUN([
#include <termio.h>
#include <errno.h>

int main() {
    struct termio t;
    if (ioctl(0, TCGETA, &t) == 0
	|| errno == ENOTTY || errno == ENXIO || errno == EINVAL) {
	t.c_cflag |= CBAUD | PARENB | PARODD | CSIZE | CSTOPB;
	return 0;
    }
    return 1;
    }], tcl_cv_api_serial=termio, tcl_cv_api_serial=no, tcl_cv_api_serial=no)
    fi
    if test $tcl_cv_api_serial = no; then
	AC_TRY_RUN([
#include <sgtty.h>
#include <errno.h>

int main() {
    struct sgttyb t;
    if (ioctl(0, TIOCGETP, &t) == 0
	|| errno == ENOTTY || errno == ENXIO || errno == EINVAL) {
	t.sg_ospeed = 0;
	t.sg_flags |= ODDP | EVENP | RAW;
	return 0;
    }
    return 1;
}], tcl_cv_api_serial=sgtty, tcl_cv_api_serial=none, tcl_cv_api_serial=none)
    fi])
    case $tcl_cv_api_serial in
	termios) AC_DEFINE(USE_TERMIOS, 1, [Use the termios API for serial lines]);;
	termio)  AC_DEFINE(USE_TERMIO, 1, [Use the termio API for serial lines]);;
	sgtty)   AC_DEFINE(USE_SGTTY, 1, [Use the sgtty API for serial lines]);;
    esac
])

#--------------------------------------------------------------------
# TEA_MISSING_POSIX_HEADERS
#
#	Supply substitutes for missing POSIX header files.  Special
#	notes:
#	    - stdlib.h doesn't define strtol, strtoul, or
#	      strtod insome versions of SunOS
#	    - some versions of string.h don't declare procedures such
#	      as strstr
#
# Arguments:
#	none
#	
# Results:
#
#	Defines some of the following vars:
#		NO_DIRENT_H
#		NO_ERRNO_H
#		NO_VALUES_H
#		HAVE_LIMITS_H or NO_LIMITS_H
#		NO_STDLIB_H
#		NO_STRING_H
#		NO_SYS_WAIT_H
#		NO_DLFCN_H
#		HAVE_SYS_PARAM_H
#
#		HAVE_STRING_H ?
#
# tkUnixPort.h checks for HAVE_LIMITS_H, so do both HAVE and
# CHECK on limits.h
#--------------------------------------------------------------------

AC_DEFUN([TEA_MISSING_POSIX_HEADERS], [
    AC_CACHE_CHECK([dirent.h], tcl_cv_dirent_h, [
    AC_TRY_LINK([#include <sys/types.h>
#include <dirent.h>], [
#ifndef _POSIX_SOURCE
#   ifdef __Lynx__
	/*
	 * Generate compilation error to make the test fail:  Lynx headers
	 * are only valid if really in the POSIX environment.
	 */

	missing_procedure();
#   endif
#endif
DIR *d;
struct dirent *entryPtr;
char *p;
d = opendir("foobar");
entryPtr = readdir(d);
p = entryPtr->d_name;
closedir(d);
], tcl_cv_dirent_h=yes, tcl_cv_dirent_h=no)])

    if test $tcl_cv_dirent_h = no; then
	AC_DEFINE(NO_DIRENT_H, 1, [Do we have <dirent.h>?])
    fi

    AC_CHECK_HEADER(errno.h, , [AC_DEFINE(NO_ERRNO_H, 1, [Do we have <errno.h>?])])
    AC_CHECK_HEADER(float.h, , [AC_DEFINE(NO_FLOAT_H, 1, [Do we have <float.h>?])])
    AC_CHECK_HEADER(values.h, , [AC_DEFINE(NO_VALUES_H, 1, [Do we have <values.h>?])])
    AC_CHECK_HEADER(limits.h,
	[AC_DEFINE(HAVE_LIMITS_H, 1, [Do we have <limits.h>?])],
	[AC_DEFINE(NO_LIMITS_H, 1, [Do we have <limits.h>?])])
    AC_CHECK_HEADER(stdlib.h, tcl_ok=1, tcl_ok=0)
    AC_EGREP_HEADER(strtol, stdlib.h, , tcl_ok=0)
    AC_EGREP_HEADER(strtoul, stdlib.h, , tcl_ok=0)
    AC_EGREP_HEADER(strtod, stdlib.h, , tcl_ok=0)
    if test $tcl_ok = 0; then
	AC_DEFINE(NO_STDLIB_H, 1, [Do we have <stdlib.h>?])
    fi
    AC_CHECK_HEADER(string.h, tcl_ok=1, tcl_ok=0)
    AC_EGREP_HEADER(strstr, string.h, , tcl_ok=0)
    AC_EGREP_HEADER(strerror, string.h, , tcl_ok=0)

    # See also memmove check below for a place where NO_STRING_H can be
    # set and why.

    if test $tcl_ok = 0; then
	AC_DEFINE(NO_STRING_H, 1, [Do we have <string.h>?])
    fi

    AC_CHECK_HEADER(sys/wait.h, , [AC_DEFINE(NO_SYS_WAIT_H, 1, [Do we have <sys/wait.h>?])])
    AC_CHECK_HEADER(dlfcn.h, , [AC_DEFINE(NO_DLFCN_H, 1, [Do we have <dlfcn.h>?])])

    # OS/390 lacks sys/param.h (and doesn't need it, by chance).
    AC_HAVE_HEADERS(sys/param.h)
])

#--------------------------------------------------------------------
# TEA_PATH_X
#
#	Locate the X11 header files and the X11 library archive.  Try
#	the ac_path_x macro first, but if it doesn't find the X stuff
#	(e.g. because there's no xmkmf program) then check through
#	a list of possible directories.  Under some conditions the
#	autoconf macro will return an include directory that contains
#	no include files, so double-check its result just to be safe.
#
#	This should be called after TEA_CONFIG_CFLAGS as setting the
#	LIBS line can confuse some configure macro magic.
#
# Arguments:
#	none
#	
# Results:
#
#	Sets the following vars:
#		XINCLUDES
#		XLIBSW
#		PKG_LIBS (appends to)
#
#--------------------------------------------------------------------

AC_DEFUN([TEA_PATH_X], [
    if test "${TEA_WINDOWINGSYSTEM}" = "x11" ; then
	TEA_PATH_UNIX_X
    fi
])

AC_DEFUN([TEA_PATH_UNIX_X], [
    AC_PATH_X
    not_really_there=""
    if test "$no_x" = ""; then
	if test "$x_includes" = ""; then
	    AC_TRY_CPP([#include <X11/XIntrinsic.h>], , not_really_there="yes")
	else
	    if test ! -r $x_includes/X11/Intrinsic.h; then
		not_really_there="yes"
	    fi
	fi
    fi
    if test "$no_x" = "yes" -o "$not_really_there" = "yes"; then
	AC_MSG_CHECKING([for X11 header files])
	found_xincludes="no"
	AC_TRY_CPP([#include <X11/Intrinsic.h>], found_xincludes="yes", found_xincludes="no")
	if test "$found_xincludes" = "no"; then
	    dirs="/usr/unsupported/include /usr/local/include /usr/X386/include /usr/X11R6/include /usr/X11R5/include /usr/include/X11R5 /usr/include/X11R4 /usr/openwin/include /usr/X11/include /usr/sww/include"
	    for i in $dirs ; do
		if test -r $i/X11/Intrinsic.h; then
		    AC_MSG_RESULT([$i])
		    XINCLUDES=" -I$i"
		    found_xincludes="yes"
		    break
		fi
	    done
	fi
    else
	if test "$x_includes" != ""; then
	    XINCLUDES="-I$x_includes"
	    found_xincludes="yes"
	fi
    fi
    if test found_xincludes = "no"; then
	AC_MSG_RESULT([couldn't find any!])
    fi

    if test "$no_x" = yes; then
	AC_MSG_CHECKING([for X11 libraries])
	XLIBSW=nope
	dirs="/usr/unsupported/lib /usr/local/lib /usr/X386/lib /usr/X11R6/lib /usr/X11R5/lib /usr/lib/X11R5 /usr/lib/X11R4 /usr/openwin/lib /usr/X11/lib /usr/sww/X11/lib"
	for i in $dirs ; do
	    if test -r $i/libX11.a -o -r $i/libX11.so -o -r $i/libX11.sl; then
		AC_MSG_RESULT([$i])
		XLIBSW="-L$i -lX11"
		x_libraries="$i"
		break
	    fi
	done
    else
	if test "$x_libraries" = ""; then
	    XLIBSW=-lX11
	else
	    XLIBSW="-L$x_libraries -lX11"
	fi
    fi
    if test "$XLIBSW" = nope ; then
	AC_CHECK_LIB(Xwindow, XCreateWindow, XLIBSW=-lXwindow)
    fi
    if test "$XLIBSW" = nope ; then
	AC_MSG_RESULT([could not find any!  Using -lX11.])
	XLIBSW=-lX11
    fi
    if test x"${XLIBSW}" != x ; then
	PKG_LIBS="${PKG_LIBS} ${XLIBSW}"
    fi
])

#--------------------------------------------------------------------
# TEA_BLOCKING_STYLE
#
#	The statements below check for systems where POSIX-style
#	non-blocking I/O (O_NONBLOCK) doesn't work or is unimplemented. 
#	On these systems (mostly older ones), use the old BSD-style
#	FIONBIO approach instead.
#
# Arguments:
#	none
#	
# Results:
#
#	Defines some of the following vars:
#		HAVE_SYS_IOCTL_H
#		HAVE_SYS_FILIO_H
#		USE_FIONBIO
#		O_NONBLOCK
#
#--------------------------------------------------------------------

AC_DEFUN([TEA_BLOCKING_STYLE], [
    AC_CHECK_HEADERS(sys/ioctl.h)
    AC_CHECK_HEADERS(sys/filio.h)
    TEA_CONFIG_SYSTEM
    AC_MSG_CHECKING([FIONBIO vs. O_NONBLOCK for nonblocking I/O])
    case $system in
	# There used to be code here to use FIONBIO under AIX.  However, it
	# was reported that FIONBIO doesn't work under AIX 3.2.5.  Since
	# using O_NONBLOCK seems fine under AIX 4.*, I removed the FIONBIO
	# code (JO, 5/31/97).

	OSF*)
	    AC_DEFINE(USE_FIONBIO, 1, [Should we use FIONBIO?])
	    AC_MSG_RESULT([FIONBIO])
	    ;;
	SunOS-4*)
	    AC_DEFINE(USE_FIONBIO, 1, [Should we use FIONBIO?])
	    AC_MSG_RESULT([FIONBIO])
	    ;;
	*)
	    AC_MSG_RESULT([O_NONBLOCK])
	    ;;
    esac
])

#--------------------------------------------------------------------
# TEA_TIME_HANLDER
#
#	Checks how the system deals with time.h, what time structures
#	are used on the system, and what fields the structures have.
#
# Arguments:
#	none
#	
# Results:
#
#	Defines some of the following vars:
#		USE_DELTA_FOR_TZ
#		HAVE_TM_GMTOFF
#		HAVE_TM_TZADJ
#		HAVE_TIMEZONE_VAR
#
#--------------------------------------------------------------------

AC_DEFUN([TEA_TIME_HANDLER], [
    AC_CHECK_HEADERS(sys/time.h)
    AC_HEADER_TIME
    AC_STRUCT_TIMEZONE

    AC_CHECK_FUNCS(gmtime_r localtime_r)

    AC_CACHE_CHECK([tm_tzadj in struct tm], tcl_cv_member_tm_tzadj, [
	AC_TRY_COMPILE([#include <time.h>], [struct tm tm; tm.tm_tzadj;],
	    tcl_cv_member_tm_tzadj=yes, tcl_cv_member_tm_tzadj=no)])
    if test $tcl_cv_member_tm_tzadj = yes ; then
	AC_DEFINE(HAVE_TM_TZADJ, 1, [Should we use the tm_tzadj field of struct tm?])
    fi

    AC_CACHE_CHECK([tm_gmtoff in struct tm], tcl_cv_member_tm_gmtoff, [
	AC_TRY_COMPILE([#include <time.h>], [struct tm tm; tm.tm_gmtoff;],
	    tcl_cv_member_tm_gmtoff=yes, tcl_cv_member_tm_gmtoff=no)])
    if test $tcl_cv_member_tm_gmtoff = yes ; then
	AC_DEFINE(HAVE_TM_GMTOFF, 1, [Should we use the tm_gmtoff field of struct tm?])
    fi

    #
    # Its important to include time.h in this check, as some systems
    # (like convex) have timezone functions, etc.
    #
    AC_CACHE_CHECK([long timezone variable], tcl_cv_timezone_long, [
	AC_TRY_COMPILE([#include <time.h>],
	    [extern long timezone;
	    timezone += 1;
	    exit (0);],
	    tcl_cv_timezone_long=yes, tcl_cv_timezone_long=no)])
    if test $tcl_cv_timezone_long = yes ; then
	AC_DEFINE(HAVE_TIMEZONE_VAR, 1, [Should we use the global timezone variable?])
    else
	#
	# On some systems (eg IRIX 6.2), timezone is a time_t and not a long.
	#
	AC_CACHE_CHECK([time_t timezone variable], tcl_cv_timezone_time, [
	    AC_TRY_COMPILE([#include <time.h>],
		[extern time_t timezone;
		timezone += 1;
		exit (0);],
		tcl_cv_timezone_time=yes, tcl_cv_timezone_time=no)])
	if test $tcl_cv_timezone_time = yes ; then
	    AC_DEFINE(HAVE_TIMEZONE_VAR, 1, [Should we use the global timezone variable?])
	fi
    fi
])

#--------------------------------------------------------------------
# TEA_BUGGY_STRTOD
#
#	Under Solaris 2.4, strtod returns the wrong value for the
#	terminating character under some conditions.  Check for this
#	and if the problem exists use a substitute procedure
#	"fixstrtod" (provided by Tcl) that corrects the error.
#	Also, on Compaq's Tru64 Unix 5.0,
#	strtod(" ") returns 0.0 instead of a failure to convert.
#
# Arguments:
#	none
#	
# Results:
#
#	Might defines some of the following vars:
#		strtod (=fixstrtod)
#
#--------------------------------------------------------------------

AC_DEFUN([TEA_BUGGY_STRTOD], [
    AC_CHECK_FUNC(strtod, tcl_strtod=1, tcl_strtod=0)
    if test "$tcl_strtod" = 1; then
	AC_CACHE_CHECK([for Solaris2.4/Tru64 strtod bugs], tcl_cv_strtod_buggy,[
	    AC_TRY_RUN([
		extern double strtod();
		int main() {
		    char *infString="Inf", *nanString="NaN", *spaceString=" ";
		    char *term;
		    double value;
		    value = strtod(infString, &term);
		    if ((term != infString) && (term[-1] == 0)) {
			exit(1);
		    }
		    value = strtod(nanString, &term);
		    if ((term != nanString) && (term[-1] == 0)) {
			exit(1);
		    }
		    value = strtod(spaceString, &term);
		    if (term == (spaceString+1)) {
			exit(1);
		    }
		    exit(0);
		}], tcl_cv_strtod_buggy=ok, tcl_cv_strtod_buggy=buggy,
		    tcl_cv_strtod_buggy=buggy)])
	if test "$tcl_cv_strtod_buggy" = buggy; then
	    AC_LIBOBJ([fixstrtod])
	    USE_COMPAT=1
	    AC_DEFINE(strtod, fixstrtod, [Do we want to use the strtod() in compat?])
	fi
    fi
])

#--------------------------------------------------------------------
# TEA_TCL_LINK_LIBS
#
#	Search for the libraries needed to link the Tcl shell.
#	Things like the math library (-lm) and socket stuff (-lsocket vs.
#	-lnsl) are dealt with here.
#
# Arguments:
#	Requires the following vars to be set in the Makefile:
#		DL_LIBS
#		LIBS
#		MATH_LIBS
#	
# Results:
#
#	Subst's the following var:
#		TCL_LIBS
#		MATH_LIBS
#
#	Might append to the following vars:
#		LIBS
#
#	Might define the following vars:
#		HAVE_NET_ERRNO_H
#
#--------------------------------------------------------------------

AC_DEFUN([TEA_TCL_LINK_LIBS], [
    #--------------------------------------------------------------------
    # On a few very rare systems, all of the libm.a stuff is
    # already in libc.a.  Set compiler flags accordingly.
    # Also, Linux requires the "ieee" library for math to work
    # right (and it must appear before "-lm").
    #--------------------------------------------------------------------

    AC_CHECK_FUNC(sin, MATH_LIBS="", MATH_LIBS="-lm")
    AC_CHECK_LIB(ieee, main, [MATH_LIBS="-lieee $MATH_LIBS"])

    #--------------------------------------------------------------------
    # Interactive UNIX requires -linet instead of -lsocket, plus it
    # needs net/errno.h to define the socket-related error codes.
    #--------------------------------------------------------------------

    AC_CHECK_LIB(inet, main, [LIBS="$LIBS -linet"])
    AC_CHECK_HEADER(net/errno.h, [
	AC_DEFINE(HAVE_NET_ERRNO_H, 1, [Do we have <net/errno.h>?])])

    #--------------------------------------------------------------------
    #	Check for the existence of the -lsocket and -lnsl libraries.
    #	The order here is important, so that they end up in the right
    #	order in the command line generated by make.  Here are some
    #	special considerations:
    #	1. Use "connect" and "accept" to check for -lsocket, and
    #	   "gethostbyname" to check for -lnsl.
    #	2. Use each function name only once:  can't redo a check because
    #	   autoconf caches the results of the last check and won't redo it.
    #	3. Use -lnsl and -lsocket only if they supply procedures that
    #	   aren't already present in the normal libraries.  This is because
    #	   IRIX 5.2 has libraries, but they aren't needed and they're
    #	   bogus:  they goof up name resolution if used.
    #	4. On some SVR4 systems, can't use -lsocket without -lnsl too.
    #	   To get around this problem, check for both libraries together
    #	   if -lsocket doesn't work by itself.
    #--------------------------------------------------------------------

    tcl_checkBoth=0
    AC_CHECK_FUNC(connect, tcl_checkSocket=0, tcl_checkSocket=1)
    if test "$tcl_checkSocket" = 1; then
	AC_CHECK_FUNC(setsockopt, , [AC_CHECK_LIB(socket, setsockopt,
	    LIBS="$LIBS -lsocket", tcl_checkBoth=1)])
    fi
    if test "$tcl_checkBoth" = 1; then
	tk_oldLibs=$LIBS
	LIBS="$LIBS -lsocket -lnsl"
	AC_CHECK_FUNC(accept, tcl_checkNsl=0, [LIBS=$tk_oldLibs])
    fi
    AC_CHECK_FUNC(gethostbyname, , [AC_CHECK_LIB(nsl, gethostbyname,
	    [LIBS="$LIBS -lnsl"])])
    
    # Don't perform the eval of the libraries here because DL_LIBS
    # won't be set until we call TEA_CONFIG_CFLAGS

    TCL_LIBS='${DL_LIBS} ${LIBS} ${MATH_LIBS}'
    AC_SUBST(TCL_LIBS)
    AC_SUBST(MATH_LIBS)
])

#--------------------------------------------------------------------
# TEA_TCL_EARLY_FLAGS
#
#	Check for what flags are needed to be passed so the correct OS
#	features are available.
#
# Arguments:
#	None
#	
# Results:
#
#	Might define the following vars:
#		_ISOC99_SOURCE
#		_LARGEFILE64_SOURCE
#		_LARGEFILE_SOURCE64
#
#--------------------------------------------------------------------

AC_DEFUN([TEA_TCL_EARLY_FLAG],[
    AC_CACHE_VAL([tcl_cv_flag_]translit($1,[A-Z],[a-z]),
	AC_TRY_COMPILE([$2], $3, [tcl_cv_flag_]translit($1,[A-Z],[a-z])=no,
	    AC_TRY_COMPILE([[#define ]$1[ 1
]$2], $3,
		[tcl_cv_flag_]translit($1,[A-Z],[a-z])=yes,
		[tcl_cv_flag_]translit($1,[A-Z],[a-z])=no)))
    if test ["x${tcl_cv_flag_]translit($1,[A-Z],[a-z])[}" = "xyes"] ; then
	AC_DEFINE($1, 1, [Add the ]$1[ flag when building])
	tcl_flags="$tcl_flags $1"
    fi
])

AC_DEFUN([TEA_TCL_EARLY_FLAGS],[
    AC_MSG_CHECKING([for required early compiler flags])
    tcl_flags=""
    TEA_TCL_EARLY_FLAG(_ISOC99_SOURCE,[#include <stdlib.h>],
	[char *p = (char *)strtoll; char *q = (char *)strtoull;])
    TEA_TCL_EARLY_FLAG(_LARGEFILE64_SOURCE,[#include <sys/stat.h>],
	[struct stat64 buf; int i = stat64("/", &buf);])
    TEA_TCL_EARLY_FLAG(_LARGEFILE_SOURCE64,[#include <sys/stat.h>],
	[char *p = (char *)open64;])
    if test "x${tcl_flags}" = "x" ; then
	AC_MSG_RESULT([none])
    else
	AC_MSG_RESULT([${tcl_flags}])
    fi
])

#--------------------------------------------------------------------
# TEA_TCL_64BIT_FLAGS
#
#	Check for what is defined in the way of 64-bit features.
#
# Arguments:
#	None
#	
# Results:
#
#	Might define the following vars:
#		TCL_WIDE_INT_IS_LONG
#		TCL_WIDE_INT_TYPE
#		HAVE_STRUCT_DIRENT64
#		HAVE_STRUCT_STAT64
#		HAVE_TYPE_OFF64_T
#
#--------------------------------------------------------------------

AC_DEFUN([TEA_TCL_64BIT_FLAGS], [
    AC_MSG_CHECKING([for 64-bit integer type])
    AC_CACHE_VAL(tcl_cv_type_64bit,[
	tcl_cv_type_64bit=none
	# See if the compiler knows natively about __int64
	AC_TRY_COMPILE(,[__int64 value = (__int64) 0;],
	    tcl_type_64bit=__int64, tcl_type_64bit="long long")
	# See if we should use long anyway  Note that we substitute in the
	# type that is our current guess for a 64-bit type inside this check
	# program, so it should be modified only carefully...
        AC_TRY_COMPILE(,[switch (0) { 
            case 1: case (sizeof(]${tcl_type_64bit}[)==sizeof(long)): ; 
        }],tcl_cv_type_64bit=${tcl_type_64bit})])
    if test "${tcl_cv_type_64bit}" = none ; then
	AC_DEFINE(TCL_WIDE_INT_IS_LONG, 1, [Are wide integers to be implemented with C 'long's?])
	AC_MSG_RESULT([using long])
    elif test "${tcl_cv_type_64bit}" = "__int64" \
		-a "${TEA_PLATFORM}" = "windows" ; then
	# We actually want to use the default tcl.h checks in this
	# case to handle both TCL_WIDE_INT_TYPE and TCL_LL_MODIFIER*
	AC_MSG_RESULT([using Tcl header defaults])
    else
	AC_DEFINE_UNQUOTED(TCL_WIDE_INT_TYPE,${tcl_cv_type_64bit},
	    [What type should be used to define wide integers?])
	AC_MSG_RESULT([${tcl_cv_type_64bit}])

	# Now check for auxiliary declarations
	AC_CACHE_CHECK([for struct dirent64], tcl_cv_struct_dirent64,[
	    AC_TRY_COMPILE([#include <sys/types.h>
#include <sys/dirent.h>],[struct dirent64 p;],
		tcl_cv_struct_dirent64=yes,tcl_cv_struct_dirent64=no)])
	if test "x${tcl_cv_struct_dirent64}" = "xyes" ; then
	    AC_DEFINE(HAVE_STRUCT_DIRENT64, 1, [Is 'struct dirent64' in <sys/types.h>?])
	fi

	AC_CACHE_CHECK([for struct stat64], tcl_cv_struct_stat64,[
	    AC_TRY_COMPILE([#include <sys/stat.h>],[struct stat64 p;
],
		tcl_cv_struct_stat64=yes,tcl_cv_struct_stat64=no)])
	if test "x${tcl_cv_struct_stat64}" = "xyes" ; then
	    AC_DEFINE(HAVE_STRUCT_STAT64, 1, [Is 'struct stat64' in <sys/stat.h>?])
	fi

	AC_CHECK_FUNCS(open64 lseek64)
	AC_MSG_CHECKING([for off64_t])
	AC_CACHE_VAL(tcl_cv_type_off64_t,[
	    AC_TRY_COMPILE([#include <sys/types.h>],[off64_t offset;
],
		tcl_cv_type_off64_t=yes,tcl_cv_type_off64_t=no)])
	dnl Define HAVE_TYPE_OFF64_T only when the off64_t type and the
	dnl functions lseek64 and open64 are defined.
	if test "x${tcl_cv_type_off64_t}" = "xyes" && \
	        test "x${ac_cv_func_lseek64}" = "xyes" && \
	        test "x${ac_cv_func_open64}" = "xyes" ; then
	    AC_DEFINE(HAVE_TYPE_OFF64_T, 1, [Is off64_t in <sys/types.h>?])
	    AC_MSG_RESULT([yes])
	else
	    AC_MSG_RESULT([no])
	fi
    fi
])

##
## Here ends the standard Tcl configuration bits and starts the
## TEA specific functions
##

#------------------------------------------------------------------------
# TEA_INIT --
#
#	Init various Tcl Extension Architecture (TEA) variables.
#	This should be the first called TEA_* macro.
#
# Arguments:
#	none
#
# Results:
#
#	Defines and substs the following vars:
#		CYGPATH
#		EXEEXT
#	Defines only:
#		TEA_VERSION
#		TEA_INITED
#		TEA_PLATFORM (windows or unix)
#
# "cygpath" is used on windows to generate native path names for include
# files. These variables should only be used with the compiler and linker
# since they generate native path names.
#
# EXEEXT
#	Select the executable extension based on the host type.  This
#	is a lightweight replacement for AC_EXEEXT that doesn't require
#	a compiler.
#------------------------------------------------------------------------

AC_DEFUN([TEA_INIT], [
    # TEA extensions pass this us the version of TEA they think they
    # are compatible with.
    TEA_VERSION="3.6"

    AC_MSG_CHECKING([for correct TEA configuration])
    if test x"${PACKAGE_NAME}" = x ; then
	AC_MSG_ERROR([
The PACKAGE_NAME variable must be defined by your TEA configure.in])
    fi
    if test x"$1" = x ; then
	AC_MSG_ERROR([
TEA version not specified.])
    elif test "$1" != "${TEA_VERSION}" ; then
	AC_MSG_RESULT([warning: requested TEA version "$1", have "${TEA_VERSION}"])
    else
	AC_MSG_RESULT([ok (TEA ${TEA_VERSION})])
    fi
    case "`uname -s`" in
	*win32*|*WIN32*|*CYGWIN_NT*|*CYGWIN_9*|*CYGWIN_ME*|*MINGW32_*)
	    AC_CHECK_PROG(CYGPATH, cygpath, cygpath -w, echo)
	    EXEEXT=".exe"
	    TEA_PLATFORM="windows"
	    ;;
	*)
	    CYGPATH=echo
	    EXEEXT=""
	    TEA_PLATFORM="unix"
	    ;;
    esac

    # Check if exec_prefix is set. If not use fall back to prefix.
    # Note when adjusted, so that TEA_PREFIX can correct for this.
    # This is needed for recursive configures, since autoconf propagates
    # $prefix, but not $exec_prefix (doh!).
    if test x$exec_prefix = xNONE ; then
	exec_prefix_default=yes
	exec_prefix=$prefix
    fi

    AC_SUBST(EXEEXT)
    AC_SUBST(CYGPATH)

    # This package name must be replaced statically for AC_SUBST to work
    AC_SUBST(PKG_LIB_FILE)
    # Substitute STUB_LIB_FILE in case package creates a stub library too.
    AC_SUBST(PKG_STUB_LIB_FILE)

    # We AC_SUBST these here to ensure they are subst'ed,
    # in case the user doesn't call TEA_ADD_...
    AC_SUBST(PKG_STUB_SOURCES)
    AC_SUBST(PKG_STUB_OBJECTS)
    AC_SUBST(PKG_TCL_SOURCES)
    AC_SUBST(PKG_HEADERS)
    AC_SUBST(PKG_INCLUDES)
    AC_SUBST(PKG_LIBS)
    AC_SUBST(PKG_CFLAGS)
])

#------------------------------------------------------------------------
# TEA_ADD_SOURCES --
#
#	Specify one or more source files.  Users should check for
#	the right platform before adding to their list.
#	It is not important to specify the directory, as long as it is
#	in the generic, win or unix subdirectory of $(srcdir).
#
# Arguments:
#	one or more file names
#
# Results:
#
#	Defines and substs the following vars:
#		PKG_SOURCES
#		PKG_OBJECTS
#------------------------------------------------------------------------
AC_DEFUN([TEA_ADD_SOURCES], [
    vars="$@"
    for i in $vars; do
	case $i in
	    [\$]*)
		# allow $-var names
		PKG_SOURCES="$PKG_SOURCES $i"
		PKG_OBJECTS="$PKG_OBJECTS $i"
		;;
	    *)
		# check for existence - allows for generic/win/unix VPATH
		if test ! -f "${srcdir}/$i" -a ! -f "${srcdir}/generic/$i" \
		    -a ! -f "${srcdir}/win/$i" -a ! -f "${srcdir}/unix/$i" \
		    ; then
		    AC_MSG_ERROR([could not find source file '$i'])
		fi
		PKG_SOURCES="$PKG_SOURCES $i"
		# this assumes it is in a VPATH dir
		i=`basename $i`
		# handle user calling this before or after TEA_SETUP_COMPILER
		if test x"${OBJEXT}" != x ; then
		    j="`echo $i | sed -e 's/\.[[^.]]*$//'`.${OBJEXT}"
		else
		    j="`echo $i | sed -e 's/\.[[^.]]*$//'`.\${OBJEXT}"
		fi
		PKG_OBJECTS="$PKG_OBJECTS $j"
		;;
	esac
    done
    AC_SUBST(PKG_SOURCES)
    AC_SUBST(PKG_OBJECTS)
])

#------------------------------------------------------------------------
# TEA_ADD_STUB_SOURCES --
#
#	Specify one or more source files.  Users should check for
#	the right platform before adding to their list.
#	It is not important to specify the directory, as long as it is
#	in the generic, win or unix subdirectory of $(srcdir).
#
# Arguments:
#	one or more file names
#
# Results:
#
#	Defines and substs the following vars:
#		PKG_STUB_SOURCES
#		PKG_STUB_OBJECTS
#------------------------------------------------------------------------
AC_DEFUN([TEA_ADD_STUB_SOURCES], [
    vars="$@"
    for i in $vars; do
	# check for existence - allows for generic/win/unix VPATH
	if test ! -f "${srcdir}/$i" -a ! -f "${srcdir}/generic/$i" \
	    -a ! -f "${srcdir}/win/$i" -a ! -f "${srcdir}/unix/$i" \
	    ; then
	    AC_MSG_ERROR([could not find stub source file '$i'])
	fi
	PKG_STUB_SOURCES="$PKG_STUB_SOURCES $i"
	# this assumes it is in a VPATH dir
	i=`basename $i`
	# handle user calling this before or after TEA_SETUP_COMPILER
	if test x"${OBJEXT}" != x ; then
	    j="`echo $i | sed -e 's/\.[[^.]]*$//'`.${OBJEXT}"
	else
	    j="`echo $i | sed -e 's/\.[[^.]]*$//'`.\${OBJEXT}"
	fi
	PKG_STUB_OBJECTS="$PKG_STUB_OBJECTS $j"
    done
    AC_SUBST(PKG_STUB_SOURCES)
    AC_SUBST(PKG_STUB_OBJECTS)
])

#------------------------------------------------------------------------
# TEA_ADD_TCL_SOURCES --
#
#	Specify one or more Tcl source files.  These should be platform
#	independent runtime files.
#
# Arguments:
#	one or more file names
#
# Results:
#
#	Defines and substs the following vars:
#		PKG_TCL_SOURCES
#------------------------------------------------------------------------
AC_DEFUN([TEA_ADD_TCL_SOURCES], [
    vars="$@"
    for i in $vars; do
	# check for existence, be strict because it is installed
	if test ! -f "${srcdir}/$i" ; then
	    AC_MSG_ERROR([could not find tcl source file '${srcdir}/$i'])
	fi
	PKG_TCL_SOURCES="$PKG_TCL_SOURCES $i"
    done
    AC_SUBST(PKG_TCL_SOURCES)
])

#------------------------------------------------------------------------
# TEA_ADD_HEADERS --
#
#	Specify one or more source headers.  Users should check for
#	the right platform before adding to their list.
#
# Arguments:
#	one or more file names
#
# Results:
#
#	Defines and substs the following vars:
#		PKG_HEADERS
#------------------------------------------------------------------------
AC_DEFUN([TEA_ADD_HEADERS], [
    vars="$@"
    for i in $vars; do
	# check for existence, be strict because it is installed
	if test ! -f "${srcdir}/$i" ; then
	    AC_MSG_ERROR([could not find header file '${srcdir}/$i'])
	fi
	PKG_HEADERS="$PKG_HEADERS $i"
    done
    AC_SUBST(PKG_HEADERS)
])

#------------------------------------------------------------------------
# TEA_ADD_INCLUDES --
#
#	Specify one or more include dirs.  Users should check for
#	the right platform before adding to their list.
#
# Arguments:
#	one or more file names
#
# Results:
#
#	Defines and substs the following vars:
#		PKG_INCLUDES
#------------------------------------------------------------------------
AC_DEFUN([TEA_ADD_INCLUDES], [
    vars="$@"
    for i in $vars; do
	PKG_INCLUDES="$PKG_INCLUDES $i"
    done
    AC_SUBST(PKG_INCLUDES)
])

#------------------------------------------------------------------------
# TEA_ADD_LIBS --
#
#	Specify one or more libraries.  Users should check for
#	the right platform before adding to their list.  For Windows,
#	libraries provided in "foo.lib" format will be converted to
#	"-lfoo" when using GCC (mingw).
#
# Arguments:
#	one or more file names
#
# Results:
#
#	Defines and substs the following vars:
#		PKG_LIBS
#------------------------------------------------------------------------
AC_DEFUN([TEA_ADD_LIBS], [
    vars="$@"
    for i in $vars; do
	if test "${TEA_PLATFORM}" = "windows" -a "$GCC" = "yes" ; then
	    # Convert foo.lib to -lfoo for GCC.  No-op if not *.lib
	    i=`echo "$i" | sed -e 's/^\([[^-]].*\)\.lib[$]/-l\1/i'`
	fi
	PKG_LIBS="$PKG_LIBS $i"
    done
    AC_SUBST(PKG_LIBS)
])

#------------------------------------------------------------------------
# TEA_ADD_CFLAGS --
#
#	Specify one or more CFLAGS.  Users should check for
#	the right platform before adding to their list.
#
# Arguments:
#	one or more file names
#
# Results:
#
#	Defines and substs the following vars:
#		PKG_CFLAGS
#------------------------------------------------------------------------
AC_DEFUN([TEA_ADD_CFLAGS], [
    PKG_CFLAGS="$PKG_CFLAGS $@"
    AC_SUBST(PKG_CFLAGS)
])

#------------------------------------------------------------------------
# TEA_PREFIX --
#
#	Handle the --prefix=... option by defaulting to what Tcl gave
#
# Arguments:
#	none
#
# Results:
#
#	If --prefix or --exec-prefix was not specified, $prefix and
#	$exec_prefix will be set to the values given to Tcl when it was
#	configured.
#------------------------------------------------------------------------
AC_DEFUN([TEA_PREFIX], [
    if test "${prefix}" = "NONE"; then
	prefix_default=yes
	if test x"${TCL_PREFIX}" != x; then
	    AC_MSG_NOTICE([--prefix defaulting to TCL_PREFIX ${TCL_PREFIX}])
	    prefix=${TCL_PREFIX}
	else
	    AC_MSG_NOTICE([--prefix defaulting to /usr/local])
	    prefix=/usr/local
	fi
    fi
    if test "${exec_prefix}" = "NONE" -a x"${prefix_default}" = x"yes" \
	-o x"${exec_prefix_default}" = x"yes" ; then
	if test x"${TCL_EXEC_PREFIX}" != x; then
	    AC_MSG_NOTICE([--exec-prefix defaulting to TCL_EXEC_PREFIX ${TCL_EXEC_PREFIX}])
	    exec_prefix=${TCL_EXEC_PREFIX}
	else
	    AC_MSG_NOTICE([--exec-prefix defaulting to ${prefix}])
	    exec_prefix=$prefix
	fi
    fi
])

#------------------------------------------------------------------------
# TEA_SETUP_COMPILER_CC --
#
#	Do compiler checks the way we want.  This is just a replacement
#	for AC_PROG_CC in TEA configure.in files to make them cleaner.
#
# Arguments:
#	none
#
# Results:
#
#	Sets up CC var and other standard bits we need to make executables.
#------------------------------------------------------------------------
AC_DEFUN([TEA_SETUP_COMPILER_CC], [
    # Don't put any macros that use the compiler (e.g. AC_TRY_COMPILE)
    # in this macro, they need to go into TEA_SETUP_COMPILER instead.

    # If the user did not set CFLAGS, set it now to keep
    # the AC_PROG_CC macro from adding "-g -O2".
    if test "${CFLAGS+set}" != "set" ; then
	CFLAGS=""
    fi

    AC_PROG_CC
    AC_PROG_CPP

    AC_PROG_INSTALL

    #--------------------------------------------------------------------
    # Checks to see if the make program sets the $MAKE variable.
    #--------------------------------------------------------------------

    AC_PROG_MAKE_SET

    #--------------------------------------------------------------------
    # Find ranlib
    #--------------------------------------------------------------------

    AC_PROG_RANLIB

    #--------------------------------------------------------------------
    # Determines the correct binary file extension (.o, .obj, .exe etc.)
    #--------------------------------------------------------------------

    AC_OBJEXT
    AC_EXEEXT
])

#------------------------------------------------------------------------
# TEA_SETUP_COMPILER --
#
#	Do compiler checks that use the compiler.  This must go after
#	TEA_SETUP_COMPILER_CC, which does the actual compiler check.
#
# Arguments:
#	none
#
# Results:
#
#	Sets up CC var and other standard bits we need to make executables.
#------------------------------------------------------------------------
AC_DEFUN([TEA_SETUP_COMPILER], [
    # Any macros that use the compiler (e.g. AC_TRY_COMPILE) have to go here.
    AC_REQUIRE([TEA_SETUP_COMPILER_CC])

    #------------------------------------------------------------------------
    # If we're using GCC, see if the compiler understands -pipe. If so, use it.
    # It makes compiling go faster.  (This is only a performance feature.)
    #------------------------------------------------------------------------

    if test -z "$no_pipe" -a -n "$GCC"; then
	AC_CACHE_CHECK([if the compiler understands -pipe],
	    tcl_cv_cc_pipe, [
	    hold_cflags=$CFLAGS; CFLAGS="$CFLAGS -pipe"
	    AC_TRY_COMPILE(,, tcl_cv_cc_pipe=yes, tcl_cv_cc_pipe=no)
	    CFLAGS=$hold_cflags])
	if test $tcl_cv_cc_pipe = yes; then
	    CFLAGS="$CFLAGS -pipe"
	fi
    fi

    #--------------------------------------------------------------------
    # Common compiler flag setup
    #--------------------------------------------------------------------

    AC_C_BIGENDIAN
    if test "${TEA_PLATFORM}" = "unix" ; then
	TEA_TCL_LINK_LIBS
	TEA_MISSING_POSIX_HEADERS
	# Let the user call this, because if it triggers, they will
	# need a compat/strtod.c that is correct.  Users can also
	# use Tcl_GetDouble(FromObj) instead.
	#TEA_BUGGY_STRTOD
    fi
])

#------------------------------------------------------------------------
# TEA_MAKE_LIB --
#
#	Generate a line that can be used to build a shared/unshared library
#	in a platform independent manner.
#
# Arguments:
#	none
#
#	Requires:
#
# Results:
#
#	Defines the following vars:
#	CFLAGS -	Done late here to note disturb other AC macros
#       MAKE_LIB -      Command to execute to build the Tcl library;
#                       differs depending on whether or not Tcl is being
#                       compiled as a shared library.
#	MAKE_SHARED_LIB	Makefile rule for building a shared library
#	MAKE_STATIC_LIB	Makefile rule for building a static library
#	MAKE_STUB_LIB	Makefile rule for building a stub library
#------------------------------------------------------------------------

AC_DEFUN([TEA_MAKE_LIB], [
    if test "${TEA_PLATFORM}" = "windows" -a "$GCC" != "yes"; then
	MAKE_STATIC_LIB="\${STLIB_LD} -out:\[$]@ \$(PKG_OBJECTS)"
	MAKE_SHARED_LIB="\${SHLIB_LD} \${SHLIB_LD_LIBS} \${LDFLAGS_DEFAULT} -out:\[$]@ \$(PKG_OBJECTS)"
	MAKE_STUB_LIB="\${STLIB_LD} -out:\[$]@ \$(PKG_STUB_OBJECTS)"
    else
	MAKE_STATIC_LIB="\${STLIB_LD} \[$]@ \$(PKG_OBJECTS)"
	MAKE_SHARED_LIB="\${SHLIB_LD} -o \[$]@ \$(PKG_OBJECTS) \${SHLIB_LD_LIBS}"
	MAKE_STUB_LIB="\${STLIB_LD} \[$]@ \$(PKG_STUB_OBJECTS)"
    fi

    if test "${SHARED_BUILD}" = "1" ; then
	MAKE_LIB="${MAKE_SHARED_LIB} "
    else
	MAKE_LIB="${MAKE_STATIC_LIB} "
    fi

    #--------------------------------------------------------------------
    # Shared libraries and static libraries have different names.
    # Use the double eval to make sure any variables in the suffix is
    # substituted. (@@@ Might not be necessary anymore)
    #--------------------------------------------------------------------

    if test "${TEA_PLATFORM}" = "windows" ; then
	if test "${SHARED_BUILD}" = "1" ; then
	    # We force the unresolved linking of symbols that are really in
	    # the private libraries of Tcl and Tk.
	    SHLIB_LD_LIBS="${SHLIB_LD_LIBS} \"`${CYGPATH} ${TCL_BIN_DIR}/${TCL_STUB_LIB_FILE}`\""
	    if test x"${TK_BIN_DIR}" != x ; then
		SHLIB_LD_LIBS="${SHLIB_LD_LIBS} \"`${CYGPATH} ${TK_BIN_DIR}/${TK_STUB_LIB_FILE}`\""
	    fi
	    eval eval "PKG_LIB_FILE=${PACKAGE_NAME}${SHARED_LIB_SUFFIX}"
	else
	    eval eval "PKG_LIB_FILE=${PACKAGE_NAME}${UNSHARED_LIB_SUFFIX}"
	fi
	# Some packages build their own stubs libraries
	eval eval "PKG_STUB_LIB_FILE=${PACKAGE_NAME}stub${UNSHARED_LIB_SUFFIX}"
	if test "$GCC" = "yes"; then
	    PKG_STUB_LIB_FILE=lib${PKG_STUB_LIB_FILE}
	fi
	# These aren't needed on Windows (either MSVC or gcc)
	RANLIB=:
	RANLIB_STUB=:
    else
	RANLIB_STUB="${RANLIB}"
	if test "${SHARED_BUILD}" = "1" ; then
	    SHLIB_LD_LIBS="${SHLIB_LD_LIBS} ${TCL_STUB_LIB_SPEC}"
	    if test x"${TK_BIN_DIR}" != x ; then
		SHLIB_LD_LIBS="${SHLIB_LD_LIBS} ${TK_STUB_LIB_SPEC}"
	    fi
	    eval eval "PKG_LIB_FILE=lib${PACKAGE_NAME}${SHARED_LIB_SUFFIX}"
	    RANLIB=:
	else
	    eval eval "PKG_LIB_FILE=lib${PACKAGE_NAME}${UNSHARED_LIB_SUFFIX}"
	fi
	# Some packages build their own stubs libraries
	eval eval "PKG_STUB_LIB_FILE=lib${PACKAGE_NAME}stub${UNSHARED_LIB_SUFFIX}"
    fi

    # These are escaped so that only CFLAGS is picked up at configure time.
    # The other values will be substituted at make time.
    CFLAGS="${CFLAGS} \${CFLAGS_DEFAULT} \${CFLAGS_WARNING}"
    if test "${SHARED_BUILD}" = "1" ; then
	CFLAGS="${CFLAGS} \${SHLIB_CFLAGS}"
    fi

    AC_SUBST(MAKE_LIB)
    AC_SUBST(MAKE_SHARED_LIB)
    AC_SUBST(MAKE_STATIC_LIB)
    AC_SUBST(MAKE_STUB_LIB)
    AC_SUBST(RANLIB_STUB)
])

#------------------------------------------------------------------------
# TEA_LIB_SPEC --
#
#	Compute the name of an existing object library located in libdir
#	from the given base name and produce the appropriate linker flags.
#
# Arguments:
#	basename	The base name of the library without version
#			numbers, extensions, or "lib" prefixes.
#	extra_dir	Extra directory in which to search for the
#			library.  This location is used first, then
#			$prefix/$exec-prefix, then some defaults.
#
# Requires:
#	TEA_INIT and TEA_PREFIX must be called first.
#
# Results:
#
#	Defines the following vars:
#		${basename}_LIB_NAME	The computed library name.
#		${basename}_LIB_SPEC	The computed linker flags.
#------------------------------------------------------------------------

AC_DEFUN([TEA_LIB_SPEC], [
    AC_MSG_CHECKING([for $1 library])

    # Look in exec-prefix for the library (defined by TEA_PREFIX).

    tea_lib_name_dir="${exec_prefix}/lib"

    # Or in a user-specified location.

    if test x"$2" != x ; then
	tea_extra_lib_dir=$2
    else
	tea_extra_lib_dir=NONE
    fi

    for i in \
	    `ls -dr ${tea_extra_lib_dir}/$1[[0-9]]*.lib 2>/dev/null ` \
	    `ls -dr ${tea_extra_lib_dir}/lib$1[[0-9]]* 2>/dev/null ` \
	    `ls -dr ${tea_lib_name_dir}/$1[[0-9]]*.lib 2>/dev/null ` \
	    `ls -dr ${tea_lib_name_dir}/lib$1[[0-9]]* 2>/dev/null ` \
	    `ls -dr /usr/lib/$1[[0-9]]*.lib 2>/dev/null ` \
	    `ls -dr /usr/lib/lib$1[[0-9]]* 2>/dev/null ` \
	    `ls -dr /usr/local/lib/$1[[0-9]]*.lib 2>/dev/null ` \
	    `ls -dr /usr/local/lib/lib$1[[0-9]]* 2>/dev/null ` ; do
	if test -f "$i" ; then
	    tea_lib_name_dir=`dirname $i`
	    $1_LIB_NAME=`basename $i`
	    $1_LIB_PATH_NAME=$i
	    break
	fi
    done

    if test "${TEA_PLATFORM}" = "windows"; then
	$1_LIB_SPEC=\"`${CYGPATH} ${$1_LIB_PATH_NAME} 2>/dev/null`\"
    else
	# Strip off the leading "lib" and trailing ".a" or ".so"

	tea_lib_name_lib=`echo ${$1_LIB_NAME}|sed -e 's/^lib//' -e 's/\.[[^.]]*$//' -e 's/\.so.*//'`
	$1_LIB_SPEC="-L${tea_lib_name_dir} -l${tea_lib_name_lib}"
    fi

    if test "x${$1_LIB_NAME}" = x ; then
	AC_MSG_ERROR([not found])
    else
	AC_MSG_RESULT([${$1_LIB_SPEC}])
    fi
])

#------------------------------------------------------------------------
# TEA_PRIVATE_TCL_HEADERS --
#
#	Locate the private Tcl include files
#
# Arguments:
#
#	Requires:
#		TCL_SRC_DIR	Assumes that TEA_LOAD_TCLCONFIG has
#				 already been called.
#
# Results:
#
#	Substs the following vars:
#		TCL_TOP_DIR_NATIVE
#		TCL_GENERIC_DIR_NATIVE
#		TCL_UNIX_DIR_NATIVE
#		TCL_WIN_DIR_NATIVE
#		TCL_BMAP_DIR_NATIVE
#		TCL_TOOL_DIR_NATIVE
#		TCL_PLATFORM_DIR_NATIVE
#		TCL_BIN_DIR_NATIVE
#		TCL_INCLUDES
#------------------------------------------------------------------------

AC_DEFUN([TEA_PRIVATE_TCL_HEADERS], [
    AC_MSG_CHECKING([for Tcl private include files])

    TCL_SRC_DIR_NATIVE=`${CYGPATH} ${TCL_SRC_DIR}`
    TCL_TOP_DIR_NATIVE=\"${TCL_SRC_DIR_NATIVE}\"
    TCL_GENERIC_DIR_NATIVE=\"${TCL_SRC_DIR_NATIVE}/generic\"
    TCL_UNIX_DIR_NATIVE=\"${TCL_SRC_DIR_NATIVE}/unix\"
    TCL_WIN_DIR_NATIVE=\"${TCL_SRC_DIR_NATIVE}/win\"
    TCL_BMAP_DIR_NATIVE=\"${TCL_SRC_DIR_NATIVE}/bitmaps\"
    TCL_TOOL_DIR_NATIVE=\"${TCL_SRC_DIR_NATIVE}/tools\"
    TCL_COMPAT_DIR_NATIVE=\"${TCL_SRC_DIR_NATIVE}/compat\"

    if test "${TEA_PLATFORM}" = "windows"; then
	TCL_PLATFORM_DIR_NATIVE=${TCL_WIN_DIR_NATIVE}
    else
	TCL_PLATFORM_DIR_NATIVE=${TCL_UNIX_DIR_NATIVE}
    fi
    # We want to ensure these are substituted so as not to require
    # any *_NATIVE vars be defined in the Makefile
    TCL_INCLUDES="-I${TCL_GENERIC_DIR_NATIVE} -I${TCL_PLATFORM_DIR_NATIVE}"
    if test "`uname -s`" = "Darwin"; then
        # If Tcl was built as a framework, attempt to use
        # the framework's Headers and PrivateHeaders directories
        case ${TCL_DEFS} in
	    *TCL_FRAMEWORK*)
	        if test -d "${TCL_BIN_DIR}/Headers" -a -d "${TCL_BIN_DIR}/PrivateHeaders"; then
	        TCL_INCLUDES="-I\"${TCL_BIN_DIR}/Headers\" -I\"${TCL_BIN_DIR}/PrivateHeaders\" ${TCL_INCLUDES}"; else
	        TCL_INCLUDES="${TCL_INCLUDES} ${TCL_INCLUDE_SPEC} `echo "${TCL_INCLUDE_SPEC}" | sed -e 's/Headers/PrivateHeaders/'`"; fi
	        ;;
	esac
    else
	if test ! -f "${TCL_SRC_DIR}/generic/tclInt.h" ; then
	    AC_MSG_ERROR([Cannot find private header tclInt.h in ${TCL_SRC_DIR}])
	fi
    fi


    AC_SUBST(TCL_TOP_DIR_NATIVE)
    AC_SUBST(TCL_GENERIC_DIR_NATIVE)
    AC_SUBST(TCL_UNIX_DIR_NATIVE)
    AC_SUBST(TCL_WIN_DIR_NATIVE)
    AC_SUBST(TCL_BMAP_DIR_NATIVE)
    AC_SUBST(TCL_TOOL_DIR_NATIVE)
    AC_SUBST(TCL_PLATFORM_DIR_NATIVE)

    AC_SUBST(TCL_INCLUDES)
    AC_MSG_RESULT([Using srcdir found in tclConfig.sh: ${TCL_SRC_DIR}])
])

#------------------------------------------------------------------------
# TEA_PUBLIC_TCL_HEADERS --
#
#	Locate the installed public Tcl header files
#
# Arguments:
#	None.
#
# Requires:
#	CYGPATH must be set
#
# Results:
#
#	Adds a --with-tclinclude switch to configure.
#	Result is cached.
#
#	Substs the following vars:
#		TCL_INCLUDES
#------------------------------------------------------------------------

AC_DEFUN([TEA_PUBLIC_TCL_HEADERS], [
    AC_MSG_CHECKING([for Tcl public headers])

    AC_ARG_WITH(tclinclude, [  --with-tclinclude       directory containing the public Tcl header files], with_tclinclude=${withval})

    AC_CACHE_VAL(ac_cv_c_tclh, [
	# Use the value from --with-tclinclude, if it was given

	if test x"${with_tclinclude}" != x ; then
	    if test -f "${with_tclinclude}/tcl.h" ; then
		ac_cv_c_tclh=${with_tclinclude}
	    else
		AC_MSG_ERROR([${with_tclinclude} directory does not contain tcl.h])
	    fi
	else
	    if test "`uname -s`" = "Darwin"; then
		# If Tcl was built as a framework, attempt to use
		# the framework's Headers directory
		case ${TCL_DEFS} in
		    *TCL_FRAMEWORK*)
			list="`ls -d ${TCL_BIN_DIR}/Headers 2>/dev/null`"
			;;
		esac
	    fi

	    # Look in the source dir only if Tcl is not installed,
	    # and in that situation, look there before installed locations.
	    if test -f "${TCL_BIN_DIR}/Makefile" ; then
		list="$list `ls -d ${TCL_SRC_DIR}/generic 2>/dev/null`"
	    fi

	    # Check order: pkg --prefix location, Tcl's --prefix location,
	    # relative to directory of tclConfig.sh.

	    eval "temp_includedir=${includedir}"
	    list="$list \
		`ls -d ${temp_includedir}        2>/dev/null` \
		`ls -d ${TCL_PREFIX}/include     2>/dev/null` \
		`ls -d ${TCL_BIN_DIR}/../include 2>/dev/null`"
	    if test "${TEA_PLATFORM}" != "windows" -o "$GCC" = "yes"; then
		list="$list /usr/local/include /usr/include"
		if test x"${TCL_INCLUDE_SPEC}" != x ; then
		    d=`echo "${TCL_INCLUDE_SPEC}" | sed -e 's/^-I//'`
		    list="$list `ls -d ${d} 2>/dev/null`"
		fi
	    fi
	    for i in $list ; do
		if test -f "$i/tcl.h" ; then
		    ac_cv_c_tclh=$i
		    break
		fi
	    done
	fi
    ])

    # Print a message based on how we determined the include path

    if test x"${ac_cv_c_tclh}" = x ; then
	AC_MSG_ERROR([tcl.h not found.  Please specify its location with --with-tclinclude])
    else
	AC_MSG_RESULT([${ac_cv_c_tclh}])
    fi

    # Convert to a native path and substitute into the output files.

    INCLUDE_DIR_NATIVE=`${CYGPATH} ${ac_cv_c_tclh}`

    TCL_INCLUDES=-I\"${INCLUDE_DIR_NATIVE}\"

    AC_SUBST(TCL_INCLUDES)
])

#------------------------------------------------------------------------
# TEA_PRIVATE_TK_HEADERS --
#
#	Locate the private Tk include files
#
# Arguments:
#
#	Requires:
#		TK_SRC_DIR	Assumes that TEA_LOAD_TKCONFIG has
#				 already been called.
#
# Results:
#
#	Substs the following vars:
#		TK_INCLUDES
#------------------------------------------------------------------------

AC_DEFUN([TEA_PRIVATE_TK_HEADERS], [
    AC_MSG_CHECKING([for Tk private include files])

    TK_SRC_DIR_NATIVE=`${CYGPATH} ${TK_SRC_DIR}`
    TK_TOP_DIR_NATIVE=\"${TK_SRC_DIR_NATIVE}\"
    TK_UNIX_DIR_NATIVE=\"${TK_SRC_DIR_NATIVE}/unix\"
    TK_WIN_DIR_NATIVE=\"${TK_SRC_DIR_NATIVE}/win\"
    TK_GENERIC_DIR_NATIVE=\"${TK_SRC_DIR_NATIVE}/generic\"
    TK_XLIB_DIR_NATIVE=\"${TK_SRC_DIR_NATIVE}/xlib\"
    if test "${TEA_PLATFORM}" = "windows"; then
	TK_PLATFORM_DIR_NATIVE=${TK_WIN_DIR_NATIVE}
    else
	TK_PLATFORM_DIR_NATIVE=${TK_UNIX_DIR_NATIVE}
    fi
    # We want to ensure these are substituted so as not to require
    # any *_NATIVE vars be defined in the Makefile
    TK_INCLUDES="-I${TK_GENERIC_DIR_NATIVE} -I${TK_PLATFORM_DIR_NATIVE}"
    if test "${TEA_WINDOWINGSYSTEM}" = "win32" \
	-o "${TEA_WINDOWINGSYSTEM}" = "aqua"; then
	TK_INCLUDES="${TK_INCLUDES} -I${TK_XLIB_DIR_NATIVE}"
    fi
    if test "${TEA_WINDOWINGSYSTEM}" = "aqua"; then
	TK_INCLUDES="${TK_INCLUDES} -I${TK_SRC_DIR_NATIVE}/macosx"
    fi
    if test "`uname -s`" = "Darwin"; then
        # If Tk was built as a framework, attempt to use
        # the framework's Headers and PrivateHeaders directories
        case ${TK_DEFS} in
	    *TK_FRAMEWORK*)
	        if test -d "${TK_BIN_DIR}/Headers" -a -d "${TK_BIN_DIR}/PrivateHeaders"; then
	        TK_INCLUDES="-I\"${TK_BIN_DIR}/Headers\" -I\"${TK_BIN_DIR}/PrivateHeaders\" ${TK_INCLUDES}"; fi
	        ;;
	esac
    else
	if test ! -f "${TK_SRC_DIR}/generic/tkInt.h" ; then
	    AC_MSG_ERROR([Cannot find private header tkInt.h in ${TK_SRC_DIR}])
	fi
    fi

    AC_SUBST(TK_TOP_DIR_NATIVE)
    AC_SUBST(TK_UNIX_DIR_NATIVE)
    AC_SUBST(TK_WIN_DIR_NATIVE)
    AC_SUBST(TK_GENERIC_DIR_NATIVE)
    AC_SUBST(TK_XLIB_DIR_NATIVE)
    AC_SUBST(TK_PLATFORM_DIR_NATIVE)

    AC_SUBST(TK_INCLUDES)
    AC_MSG_RESULT([Using srcdir found in tkConfig.sh: ${TK_SRC_DIR}])
])

#------------------------------------------------------------------------
# TEA_PUBLIC_TK_HEADERS --
#
#	Locate the installed public Tk header files
#
# Arguments:
#	None.
#
# Requires:
#	CYGPATH must be set
#
# Results:
#
#	Adds a --with-tkinclude switch to configure.
#	Result is cached.
#
#	Substs the following vars:
#		TK_INCLUDES
#------------------------------------------------------------------------

AC_DEFUN([TEA_PUBLIC_TK_HEADERS], [
    AC_MSG_CHECKING([for Tk public headers])

    AC_ARG_WITH(tkinclude, [  --with-tkinclude      directory containing the public Tk header files.], with_tkinclude=${withval})

    AC_CACHE_VAL(ac_cv_c_tkh, [
	# Use the value from --with-tkinclude, if it was given

	if test x"${with_tkinclude}" != x ; then
	    if test -f "${with_tkinclude}/tk.h" ; then
		ac_cv_c_tkh=${with_tkinclude}
	    else
		AC_MSG_ERROR([${with_tkinclude} directory does not contain tk.h])
	    fi
	else
	    if test "`uname -s`" = "Darwin"; then
		# If Tk was built as a framework, attempt to use
		# the framework's Headers directory.
		case ${TK_DEFS} in
		    *TK_FRAMEWORK*)
			list="`ls -d ${TK_BIN_DIR}/Headers 2>/dev/null`"
			;;
		esac
	    fi

	    # Look in the source dir only if Tk is not installed,
	    # and in that situation, look there before installed locations.
	    if test -f "${TK_BIN_DIR}/Makefile" ; then
		list="$list `ls -d ${TK_SRC_DIR}/generic 2>/dev/null`"
	    fi

	    # Check order: pkg --prefix location, Tk's --prefix location,
	    # relative to directory of tkConfig.sh, Tcl's --prefix location, 
	    # relative to directory of tclConfig.sh.

	    eval "temp_includedir=${includedir}"
	    list="$list \
		`ls -d ${temp_includedir}        2>/dev/null` \
		`ls -d ${TK_PREFIX}/include      2>/dev/null` \
		`ls -d ${TK_BIN_DIR}/../include  2>/dev/null` \
		`ls -d ${TCL_PREFIX}/include     2>/dev/null` \
		`ls -d ${TCL_BIN_DIR}/../include 2>/dev/null`"
	    if test "${TEA_PLATFORM}" != "windows" -o "$GCC" = "yes"; then
		list="$list /usr/local/include /usr/include"
	    fi
	    for i in $list ; do
		if test -f "$i/tk.h" ; then
		    ac_cv_c_tkh=$i
		    break
		fi
	    done
	fi
    ])

    # Print a message based on how we determined the include path

    if test x"${ac_cv_c_tkh}" = x ; then
	AC_MSG_ERROR([tk.h not found.  Please specify its location with --with-tkinclude])
    else
	AC_MSG_RESULT([${ac_cv_c_tkh}])
    fi

    # Convert to a native path and substitute into the output files.

    INCLUDE_DIR_NATIVE=`${CYGPATH} ${ac_cv_c_tkh}`

    TK_INCLUDES=-I\"${INCLUDE_DIR_NATIVE}\"

    AC_SUBST(TK_INCLUDES)

    if test "${TEA_WINDOWINGSYSTEM}" = "win32" \
	-o "${TEA_WINDOWINGSYSTEM}" = "aqua"; then
	# On Windows and Aqua, we need the X compat headers
	AC_MSG_CHECKING([for X11 header files])
	if test ! -r "${INCLUDE_DIR_NATIVE}/X11/Xlib.h"; then
	    INCLUDE_DIR_NATIVE="`${CYGPATH} ${TK_SRC_DIR}/xlib`"
	    TK_XINCLUDES=-I\"${INCLUDE_DIR_NATIVE}\"
	    AC_SUBST(TK_XINCLUDES)
	fi
	AC_MSG_RESULT([${INCLUDE_DIR_NATIVE}])
    fi
])

#------------------------------------------------------------------------
# TEA_PROG_TCLSH
#	Determine the fully qualified path name of the tclsh executable
#	in the Tcl build directory or the tclsh installed in a bin
#	directory. This macro will correctly determine the name
#	of the tclsh executable even if tclsh has not yet been
#	built in the build directory. The tclsh found is always
#	associated with a tclConfig.sh file. This tclsh should be used
#	only for running extension test cases. It should never be
#	or generation of files (like pkgIndex.tcl) at build time.
#
# Arguments
#	none
#
# Results
#	Subst's the following values:
#		TCLSH_PROG
#------------------------------------------------------------------------

AC_DEFUN([TEA_PROG_TCLSH], [
    AC_MSG_CHECKING([for tclsh])
    if test -f "${TCL_BIN_DIR}/Makefile" ; then
        # tclConfig.sh is in Tcl build directory
        if test "${TEA_PLATFORM}" = "windows"; then
            TCLSH_PROG="${TCL_BIN_DIR}/tclsh${TCL_MAJOR_VERSION}${TCL_MINOR_VERSION}${TCL_DBGX}${EXEEXT}"
        else
            TCLSH_PROG="${TCL_BIN_DIR}/tclsh"
        fi
    else
        # tclConfig.sh is in install location
        if test "${TEA_PLATFORM}" = "windows"; then
            TCLSH_PROG="tclsh${TCL_MAJOR_VERSION}${TCL_MINOR_VERSION}${TCL_DBGX}${EXEEXT}"
        else
            TCLSH_PROG="tclsh${TCL_MAJOR_VERSION}.${TCL_MINOR_VERSION}${TCL_DBGX}"
        fi
        list="`ls -d ${TCL_BIN_DIR}/../bin 2>/dev/null` \
              `ls -d ${TCL_BIN_DIR}/..     2>/dev/null` \
              `ls -d ${TCL_PREFIX}/bin     2>/dev/null`"
        for i in $list ; do
            if test -f "$i/${TCLSH_PROG}" ; then
                REAL_TCL_BIN_DIR="`cd "$i"; pwd`"
                break
            fi
        done
        TCLSH_PROG="${REAL_TCL_BIN_DIR}/${TCLSH_PROG}"
    fi
    AC_MSG_RESULT([${TCLSH_PROG}])
    AC_SUBST(TCLSH_PROG)
])

#------------------------------------------------------------------------
# TEA_PROG_WISH
#	Determine the fully qualified path name of the wish executable
#	in the Tk build directory or the wish installed in a bin
#	directory. This macro will correctly determine the name
#	of the wish executable even if wish has not yet been
#	built in the build directory. The wish found is always
#	associated with a tkConfig.sh file. This wish should be used
#	only for running extension test cases. It should never be
#	or generation of files (like pkgIndex.tcl) at build time.
#
# Arguments
#	none
#
# Results
#	Subst's the following values:
#		WISH_PROG
#------------------------------------------------------------------------

AC_DEFUN([TEA_PROG_WISH], [
    AC_MSG_CHECKING([for wish])
    if test -f "${TK_BIN_DIR}/Makefile" ; then
        # tkConfig.sh is in Tk build directory
        if test "${TEA_PLATFORM}" = "windows"; then
            WISH_PROG="${TK_BIN_DIR}/wish${TK_MAJOR_VERSION}${TK_MINOR_VERSION}${TK_DBGX}${EXEEXT}"
        else
            WISH_PROG="${TK_BIN_DIR}/wish"
        fi
    else
        # tkConfig.sh is in install location
        if test "${TEA_PLATFORM}" = "windows"; then
            WISH_PROG="wish${TK_MAJOR_VERSION}${TK_MINOR_VERSION}${TK_DBGX}${EXEEXT}"
        else
            WISH_PROG="wish${TK_MAJOR_VERSION}.${TK_MINOR_VERSION}${TK_DBGX}"
        fi
        list="`ls -d ${TK_BIN_DIR}/../bin 2>/dev/null` \
              `ls -d ${TK_BIN_DIR}/..     2>/dev/null` \
              `ls -d ${TK_PREFIX}/bin     2>/dev/null`"
        for i in $list ; do
            if test -f "$i/${WISH_PROG}" ; then
                REAL_TK_BIN_DIR="`cd "$i"; pwd`"
                break
            fi
        done
        WISH_PROG="${REAL_TK_BIN_DIR}/${WISH_PROG}"
    fi
    AC_MSG_RESULT([${WISH_PROG}])
    AC_SUBST(WISH_PROG)
])

#------------------------------------------------------------------------
# TEA_PATH_CONFIG --
#
#	Locate the ${1}Config.sh file and perform a sanity check on
#	the ${1} compile flags.  These are used by packages like
#	[incr Tk] that load *Config.sh files from more than Tcl and Tk.
#
# Arguments:
#	none
#
# Results:
#
#	Adds the following arguments to configure:
#		--with-$1=...
#
#	Defines the following vars:
#		$1_BIN_DIR	Full path to the directory containing
#				the $1Config.sh file
#------------------------------------------------------------------------

AC_DEFUN([TEA_PATH_CONFIG], [
    #
    # Ok, lets find the $1 configuration
    # First, look for one uninstalled.
    # the alternative search directory is invoked by --with-$1
    #

    if test x"${no_$1}" = x ; then
	# we reset no_$1 in case something fails here
	no_$1=true
	AC_ARG_WITH($1, [  --with-$1              directory containing $1 configuration ($1Config.sh)], with_$1config=${withval})
	AC_MSG_CHECKING([for $1 configuration])
	AC_CACHE_VAL(ac_cv_c_$1config,[

	    # First check to see if --with-$1 was specified.
	    if test x"${with_$1config}" != x ; then
		case ${with_$1config} in
		    */$1Config.sh )
			if test -f ${with_$1config}; then
			    AC_MSG_WARN([--with-$1 argument should refer to directory containing $1Config.sh, not to $1Config.sh itself])
			    with_$1config=`echo ${with_$1config} | sed 's!/$1Config\.sh$!!'`
			fi;;
		esac
		if test -f "${with_$1config}/$1Config.sh" ; then
		    ac_cv_c_$1config=`(cd ${with_$1config}; pwd)`
		else
		    AC_MSG_ERROR([${with_$1config} directory doesn't contain $1Config.sh])
		fi
	    fi

	    # then check for a private $1 installation
	    if test x"${ac_cv_c_$1config}" = x ; then
		for i in \
			../$1 \
			`ls -dr ../$1*[[0-9]].[[0-9]]*.[[0-9]]* 2>/dev/null` \
			`ls -dr ../$1*[[0-9]].[[0-9]][[0-9]] 2>/dev/null` \
			`ls -dr ../$1*[[0-9]].[[0-9]] 2>/dev/null` \
			`ls -dr ../$1*[[0-9]].[[0-9]]* 2>/dev/null` \
			../../$1 \
			`ls -dr ../../$1*[[0-9]].[[0-9]]*.[[0-9]]* 2>/dev/null` \
			`ls -dr ../../$1*[[0-9]].[[0-9]][[0-9]] 2>/dev/null` \
			`ls -dr ../../$1*[[0-9]].[[0-9]] 2>/dev/null` \
			`ls -dr ../../$1*[[0-9]].[[0-9]]* 2>/dev/null` \
			../../../$1 \
			`ls -dr ../../../$1*[[0-9]].[[0-9]]*.[[0-9]]* 2>/dev/null` \
			`ls -dr ../../../$1*[[0-9]].[[0-9]][[0-9]] 2>/dev/null` \
			`ls -dr ../../../$1*[[0-9]].[[0-9]] 2>/dev/null` \
			`ls -dr ../../../$1*[[0-9]].[[0-9]]* 2>/dev/null` \
			${srcdir}/../$1 \
			`ls -dr ${srcdir}/../$1*[[0-9]].[[0-9]]*.[[0-9]]* 2>/dev/null` \
			`ls -dr ${srcdir}/../$1*[[0-9]].[[0-9]][[0-9]] 2>/dev/null` \
			`ls -dr ${srcdir}/../$1*[[0-9]].[[0-9]] 2>/dev/null` \
			`ls -dr ${srcdir}/../$1*[[0-9]].[[0-9]]* 2>/dev/null` \
			; do
		    if test -f "$i/$1Config.sh" ; then
			ac_cv_c_$1config=`(cd $i; pwd)`
			break
		    fi
		    if test -f "$i/unix/$1Config.sh" ; then
			ac_cv_c_$1config=`(cd $i/unix; pwd)`
			break
		    fi
		done
	    fi

	    # check in a few common install locations
	    if test x"${ac_cv_c_$1config}" = x ; then
		for i in `ls -d ${libdir} 2>/dev/null` \
			`ls -d ${exec_prefix}/lib 2>/dev/null` \
			`ls -d ${prefix}/lib 2>/dev/null` \
			`ls -d /usr/local/lib 2>/dev/null` \
			`ls -d /usr/contrib/lib 2>/dev/null` \
			`ls -d /usr/lib 2>/dev/null` \
			; do
		    if test -f "$i/$1Config.sh" ; then
			ac_cv_c_$1config=`(cd $i; pwd)`
			break
		    fi
		done
	    fi
	])

	if test x"${ac_cv_c_$1config}" = x ; then
	    $1_BIN_DIR="# no $1 configs found"
	    AC_MSG_WARN([Cannot find $1 configuration definitions])
	    exit 0
	else
	    no_$1=
	    $1_BIN_DIR=${ac_cv_c_$1config}
	    AC_MSG_RESULT([found $$1_BIN_DIR/$1Config.sh])
	fi
    fi
])

#------------------------------------------------------------------------
# TEA_LOAD_CONFIG --
#
#	Load the $1Config.sh file
#
# Arguments:
#	
#	Requires the following vars to be set:
#		$1_BIN_DIR
#
# Results:
#
#	Subst the following vars:
#		$1_SRC_DIR
#		$1_LIB_FILE
#		$1_LIB_SPEC
#
#------------------------------------------------------------------------

AC_DEFUN([TEA_LOAD_CONFIG], [
    AC_MSG_CHECKING([for existence of ${$1_BIN_DIR}/$1Config.sh])

    if test -f "${$1_BIN_DIR}/$1Config.sh" ; then
        AC_MSG_RESULT([loading])
	. ${$1_BIN_DIR}/$1Config.sh
    else
        AC_MSG_RESULT([file not found])
    fi

    #
    # If the $1_BIN_DIR is the build directory (not the install directory),
    # then set the common variable name to the value of the build variables.
    # For example, the variable $1_LIB_SPEC will be set to the value
    # of $1_BUILD_LIB_SPEC. An extension should make use of $1_LIB_SPEC
    # instead of $1_BUILD_LIB_SPEC since it will work with both an
    # installed and uninstalled version of Tcl.
    #

    if test -f ${$1_BIN_DIR}/Makefile ; then
	AC_MSG_WARN([Found Makefile - using build library specs for $1])
        $1_LIB_SPEC=${$1_BUILD_LIB_SPEC}
        $1_STUB_LIB_SPEC=${$1_BUILD_STUB_LIB_SPEC}
        $1_STUB_LIB_PATH=${$1_BUILD_STUB_LIB_PATH}
    fi

    AC_SUBST($1_VERSION)
    AC_SUBST($1_BIN_DIR)
    AC_SUBST($1_SRC_DIR)

    AC_SUBST($1_LIB_FILE)
    AC_SUBST($1_LIB_SPEC)

    AC_SUBST($1_STUB_LIB_FILE)
    AC_SUBST($1_STUB_LIB_SPEC)
    AC_SUBST($1_STUB_LIB_PATH)
])

#------------------------------------------------------------------------
# TEA_PATH_CELIB --
#
#	Locate Keuchel's celib emulation layer for targeting Win/CE
#
# Arguments:
#	none
#
# Results:
#
#	Adds the following arguments to configure:
#		--with-celib=...
#
#	Defines the following vars:
#		CELIB_DIR	Full path to the directory containing
#				the include and platform lib files
#------------------------------------------------------------------------

AC_DEFUN([TEA_PATH_CELIB], [
    # First, look for one uninstalled.
    # the alternative search directory is invoked by --with-celib

    if test x"${no_celib}" = x ; then
	# we reset no_celib in case something fails here
	no_celib=true
	AC_ARG_WITH(celib,[  --with-celib=DIR        use Windows/CE support library from DIR], with_celibconfig=${withval})
	AC_MSG_CHECKING([for Windows/CE celib directory])
	AC_CACHE_VAL(ac_cv_c_celibconfig,[
	    # First check to see if --with-celibconfig was specified.
	    if test x"${with_celibconfig}" != x ; then
		if test -d "${with_celibconfig}/inc" ; then
		    ac_cv_c_celibconfig=`(cd ${with_celibconfig}; pwd)`
		else
		    AC_MSG_ERROR([${with_celibconfig} directory doesn't contain inc directory])
		fi
	    fi

	    # then check for a celib library
	    if test x"${ac_cv_c_celibconfig}" = x ; then
		for i in \
			../celib-palm-3.0 \
			../celib \
			../../celib-palm-3.0 \
			../../celib \
			`ls -dr ../celib-*3.[[0-9]]* 2>/dev/null` \
			${srcdir}/../celib-palm-3.0 \
			${srcdir}/../celib \
			`ls -dr ${srcdir}/../celib-*3.[[0-9]]* 2>/dev/null` \
			; do
		    if test -d "$i/inc" ; then
			ac_cv_c_celibconfig=`(cd $i; pwd)`
			break
		    fi
		done
	    fi
	])
	if test x"${ac_cv_c_celibconfig}" = x ; then
	    AC_MSG_ERROR([Cannot find celib support library directory])
	else
	    no_celib=
	    CELIB_DIR=${ac_cv_c_celibconfig}
	    CELIB_DIR=`echo "$CELIB_DIR" | sed -e 's!\\\!/!g'`
	    AC_MSG_RESULT([found $CELIB_DIR])
	fi
    fi
])


# Local Variables:
# mode: autoconf
# End:
