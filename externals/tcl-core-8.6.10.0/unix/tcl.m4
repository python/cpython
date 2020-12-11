#------------------------------------------------------------------------
# SC_PATH_TCLCONFIG --
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

AC_DEFUN([SC_PATH_TCLCONFIG], [
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
	    with_tclconfig="${withval}")
	AC_MSG_CHECKING([for Tcl configuration])
	AC_CACHE_VAL(ac_cv_c_tclconfig,[

	    # First check to see if --with-tcl was specified.
	    if test x"${with_tclconfig}" != x ; then
		case "${with_tclconfig}" in
		    */tclConfig.sh )
			if test -f "${with_tclconfig}"; then
			    AC_MSG_WARN([--with-tcl argument should refer to directory containing tclConfig.sh, not to tclConfig.sh itself])
			    with_tclconfig="`echo "${with_tclconfig}" | sed 's!/tclConfig\.sh$!!'`"
			fi ;;
		esac
		if test -f "${with_tclconfig}/tclConfig.sh" ; then
		    ac_cv_c_tclconfig="`(cd "${with_tclconfig}"; pwd)`"
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
			ac_cv_c_tclconfig="`(cd $i/unix; pwd)`"
			break
		    fi
		done
	    fi

	    # on Darwin, check in Framework installation locations
	    if test "`uname -s`" = "Darwin" -a x"${ac_cv_c_tclconfig}" = x ; then
		for i in `ls -d ~/Library/Frameworks 2>/dev/null` \
			`ls -d /Library/Frameworks 2>/dev/null` \
			`ls -d /Network/Library/Frameworks 2>/dev/null` \
			; do
		    if test -f "$i/Tcl.framework/tclConfig.sh" ; then
			ac_cv_c_tclconfig="`(cd $i/Tcl.framework; pwd)`"
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
			`ls -d /usr/pkg/lib 2>/dev/null` \
			`ls -d /usr/lib/tcl8.6 2>/dev/null` \
			`ls -d /usr/lib 2>/dev/null` \
			`ls -d /usr/lib64 2>/dev/null` \
			`ls -d /usr/local/lib/tcl8.6 2>/dev/null` \
			`ls -d /usr/local/lib/tcl/tcl8.6 2>/dev/null` \
			; do
		    if test -f "$i/tclConfig.sh" ; then
			ac_cv_c_tclconfig="`(cd $i; pwd)`"
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
			ac_cv_c_tclconfig="`(cd $i/unix; pwd)`"
			break
		    fi
		done
	    fi
	])

	if test x"${ac_cv_c_tclconfig}" = x ; then
	    TCL_BIN_DIR="# no Tcl configs found"
	    AC_MSG_ERROR([Can't find Tcl configuration definitions. Use --with-tcl to specify a directory containing tclConfig.sh])
	else
	    no_tcl=
	    TCL_BIN_DIR="${ac_cv_c_tclconfig}"
	    AC_MSG_RESULT([found ${TCL_BIN_DIR}/tclConfig.sh])
	fi
    fi
])

#------------------------------------------------------------------------
# SC_PATH_TKCONFIG --
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

AC_DEFUN([SC_PATH_TKCONFIG], [
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
	    with_tkconfig="${withval}")
	AC_MSG_CHECKING([for Tk configuration])
	AC_CACHE_VAL(ac_cv_c_tkconfig,[

	    # First check to see if --with-tkconfig was specified.
	    if test x"${with_tkconfig}" != x ; then
		case "${with_tkconfig}" in
		    */tkConfig.sh )
			if test -f "${with_tkconfig}"; then
			    AC_MSG_WARN([--with-tk argument should refer to directory containing tkConfig.sh, not to tkConfig.sh itself])
			    with_tkconfig="`echo "${with_tkconfig}" | sed 's!/tkConfig\.sh$!!'`"
			fi ;;
		esac
		if test -f "${with_tkconfig}/tkConfig.sh" ; then
		    ac_cv_c_tkconfig="`(cd "${with_tkconfig}"; pwd)`"
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
			ac_cv_c_tkconfig="`(cd $i/unix; pwd)`"
			break
		    fi
		done
	    fi

	    # on Darwin, check in Framework installation locations
	    if test "`uname -s`" = "Darwin" -a x"${ac_cv_c_tkconfig}" = x ; then
		for i in `ls -d ~/Library/Frameworks 2>/dev/null` \
			`ls -d /Library/Frameworks 2>/dev/null` \
			`ls -d /Network/Library/Frameworks 2>/dev/null` \
			; do
		    if test -f "$i/Tk.framework/tkConfig.sh" ; then
			ac_cv_c_tkconfig="`(cd $i/Tk.framework; pwd)`"
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
			`ls -d /usr/pkg/lib 2>/dev/null` \
			`ls -d /usr/lib/tk8.6 2>/dev/null` \
			`ls -d /usr/lib 2>/dev/null` \
			`ls -d /usr/lib64 2>/dev/null` \
			`ls -d /usr/local/lib/tk8.6 2>/dev/null` \
			`ls -d /usr/local/lib/tcl/tk8.6 2>/dev/null` \
			; do
		    if test -f "$i/tkConfig.sh" ; then
			ac_cv_c_tkconfig="`(cd $i; pwd)`"
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
			ac_cv_c_tkconfig="`(cd $i/unix; pwd)`"
			break
		    fi
		done
	    fi
	])

	if test x"${ac_cv_c_tkconfig}" = x ; then
	    TK_BIN_DIR="# no Tk configs found"
	    AC_MSG_ERROR([Can't find Tk configuration definitions. Use --with-tk to specify a directory containing tkConfig.sh])
	else
	    no_tk=
	    TK_BIN_DIR="${ac_cv_c_tkconfig}"
	    AC_MSG_RESULT([found ${TK_BIN_DIR}/tkConfig.sh])
	fi
    fi
])

#------------------------------------------------------------------------
# SC_LOAD_TCLCONFIG --
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
#	Substitutes the following vars:
#		TCL_BIN_DIR
#		TCL_SRC_DIR
#		TCL_LIB_FILE
#------------------------------------------------------------------------

AC_DEFUN([SC_LOAD_TCLCONFIG], [
    AC_MSG_CHECKING([for existence of ${TCL_BIN_DIR}/tclConfig.sh])

    if test -f "${TCL_BIN_DIR}/tclConfig.sh" ; then
        AC_MSG_RESULT([loading])
	. "${TCL_BIN_DIR}/tclConfig.sh"
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
    if test -f "${TCL_BIN_DIR}/Makefile" ; then
        TCL_LIB_SPEC="${TCL_BUILD_LIB_SPEC}"
        TCL_STUB_LIB_SPEC="${TCL_BUILD_STUB_LIB_SPEC}"
        TCL_STUB_LIB_PATH="${TCL_BUILD_STUB_LIB_PATH}"
    elif test "`uname -s`" = "Darwin"; then
	# If Tcl was built as a framework, attempt to use the libraries
	# from the framework at the given location so that linking works
	# against Tcl.framework installed in an arbitrary location.
	case ${TCL_DEFS} in
	    *TCL_FRAMEWORK*)
		if test -f "${TCL_BIN_DIR}/${TCL_LIB_FILE}"; then
		    for i in "`cd "${TCL_BIN_DIR}"; pwd`" \
			     "`cd "${TCL_BIN_DIR}"/../..; pwd`"; do
			if test "`basename "$i"`" = "${TCL_LIB_FILE}.framework"; then
			    TCL_LIB_SPEC="-F`dirname "$i" | sed -e 's/ /\\\\ /g'` -framework ${TCL_LIB_FILE}"
			    break
			fi
		    done
		fi
		if test -f "${TCL_BIN_DIR}/${TCL_STUB_LIB_FILE}"; then
		    TCL_STUB_LIB_SPEC="-L`echo "${TCL_BIN_DIR}"  | sed -e 's/ /\\\\ /g'` ${TCL_STUB_LIB_FLAG}"
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
    AC_SUBST(TCL_PATCH_LEVEL)
    AC_SUBST(TCL_BIN_DIR)
    AC_SUBST(TCL_SRC_DIR)

    AC_SUBST(TCL_LIB_FILE)
    AC_SUBST(TCL_LIB_FLAG)
    AC_SUBST(TCL_LIB_SPEC)

    AC_SUBST(TCL_STUB_LIB_FILE)
    AC_SUBST(TCL_STUB_LIB_FLAG)
    AC_SUBST(TCL_STUB_LIB_SPEC)
])

#------------------------------------------------------------------------
# SC_LOAD_TKCONFIG --
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

AC_DEFUN([SC_LOAD_TKCONFIG], [
    AC_MSG_CHECKING([for existence of ${TK_BIN_DIR}/tkConfig.sh])

    if test -f "${TK_BIN_DIR}/tkConfig.sh" ; then
        AC_MSG_RESULT([loading])
	. "${TK_BIN_DIR}/tkConfig.sh"
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
    if test -f "${TK_BIN_DIR}/Makefile" ; then
        TK_LIB_SPEC="${TK_BUILD_LIB_SPEC}"
        TK_STUB_LIB_SPEC="${TK_BUILD_STUB_LIB_SPEC}"
        TK_STUB_LIB_PATH="${TK_BUILD_STUB_LIB_PATH}"
    elif test "`uname -s`" = "Darwin"; then
	# If Tk was built as a framework, attempt to use the libraries
	# from the framework at the given location so that linking works
	# against Tk.framework installed in an arbitrary location.
	case ${TK_DEFS} in
	    *TK_FRAMEWORK*)
		if test -f "${TK_BIN_DIR}/${TK_LIB_FILE}"; then
		    for i in "`cd "${TK_BIN_DIR}"; pwd`" \
			     "`cd "${TK_BIN_DIR}"/../..; pwd`"; do
			if test "`basename "$i"`" = "${TK_LIB_FILE}.framework"; then
			    TK_LIB_SPEC="-F`dirname "$i" | sed -e 's/ /\\\\ /g'` -framework ${TK_LIB_FILE}"
			    break
			fi
		    done
		fi
		if test -f "${TK_BIN_DIR}/${TK_STUB_LIB_FILE}"; then
		    TK_STUB_LIB_SPEC="-L` echo "${TK_BIN_DIR}"  | sed -e 's/ /\\\\ /g'` ${TK_STUB_LIB_FLAG}"
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

    AC_SUBST(TK_VERSION)
    AC_SUBST(TK_BIN_DIR)
    AC_SUBST(TK_SRC_DIR)

    AC_SUBST(TK_LIB_FILE)
    AC_SUBST(TK_LIB_FLAG)
    AC_SUBST(TK_LIB_SPEC)

    AC_SUBST(TK_STUB_LIB_FILE)
    AC_SUBST(TK_STUB_LIB_FLAG)
    AC_SUBST(TK_STUB_LIB_SPEC)
])

#------------------------------------------------------------------------
# SC_PROG_TCLSH
#	Locate a tclsh shell installed on the system path. This macro
#	will only find a Tcl shell that already exists on the system.
#	It will not find a Tcl shell in the Tcl build directory or
#	a Tcl shell that has been installed from the Tcl build directory.
#	If a Tcl shell can't be located on the PATH, then TCLSH_PROG will
#	be set to "". Extensions should take care not to create Makefile
#	rules that are run by default and depend on TCLSH_PROG. An
#	extension can't assume that an executable Tcl shell exists at
#	build time.
#
# Arguments:
#	none
#
# Results:
#	Substitutes the following vars:
#		TCLSH_PROG
#------------------------------------------------------------------------

AC_DEFUN([SC_PROG_TCLSH], [
    AC_MSG_CHECKING([for tclsh])
    AC_CACHE_VAL(ac_cv_path_tclsh, [
	search_path=`echo ${PATH} | sed -e 's/:/ /g'`
	for dir in $search_path ; do
	    for j in `ls -r $dir/tclsh[[8-9]]* 2> /dev/null` \
		    `ls -r $dir/tclsh* 2> /dev/null` ; do
		if test x"$ac_cv_path_tclsh" = x ; then
		    if test -f "$j" ; then
			ac_cv_path_tclsh=$j
			break
		    fi
		fi
	    done
	done
    ])

    if test -f "$ac_cv_path_tclsh" ; then
	TCLSH_PROG="$ac_cv_path_tclsh"
	AC_MSG_RESULT([$TCLSH_PROG])
    else
	# It is not an error if an installed version of Tcl can't be located.
	TCLSH_PROG=""
	AC_MSG_RESULT([No tclsh found on PATH])
    fi
    AC_SUBST(TCLSH_PROG)
])

#------------------------------------------------------------------------
# SC_BUILD_TCLSH
#	Determine the fully qualified path name of the tclsh executable
#	in the Tcl build directory. This macro will correctly determine
#	the name of the tclsh executable even if tclsh has not yet
#	been built in the build directory. The build tclsh must be used
#	when running tests from an extension build directory. It is not
#	correct to use the TCLSH_PROG in cases like this.
#
# Arguments:
#	none
#
# Results:
#	Substitutes the following values:
#		BUILD_TCLSH
#------------------------------------------------------------------------

AC_DEFUN([SC_BUILD_TCLSH], [
    AC_MSG_CHECKING([for tclsh in Tcl build directory])
    BUILD_TCLSH="${TCL_BIN_DIR}"/tclsh
    AC_MSG_RESULT([$BUILD_TCLSH])
    AC_SUBST(BUILD_TCLSH)
])

#------------------------------------------------------------------------
# SC_ENABLE_SHARED --
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

AC_DEFUN([SC_ENABLE_SHARED], [
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
])

#------------------------------------------------------------------------
# SC_ENABLE_FRAMEWORK --
#
#	Allows the building of shared libraries into frameworks
#
# Arguments:
#	none
#
# Results:
#
#	Adds the following arguments to configure:
#		--enable-framework=yes|no
#
#	Sets the following vars:
#		FRAMEWORK_BUILD	Value of 1 or 0
#------------------------------------------------------------------------

AC_DEFUN([SC_ENABLE_FRAMEWORK], [
    if test "`uname -s`" = "Darwin" ; then
	AC_MSG_CHECKING([how to package libraries])
	AC_ARG_ENABLE(framework,
	    AC_HELP_STRING([--enable-framework],
		[package shared libraries in MacOSX frameworks (default: off)]),
	    [enable_framework=$enableval], [enable_framework=no])
	if test $enable_framework = yes; then
	    if test $SHARED_BUILD = 0; then
		AC_MSG_WARN([Frameworks can only be built if --enable-shared is yes])
		enable_framework=no
	    fi
	    if test $tcl_corefoundation = no; then
		AC_MSG_WARN([Frameworks can only be used when CoreFoundation is available])
		enable_framework=no
	    fi
	fi
	if test $enable_framework = yes; then
	    AC_MSG_RESULT([framework])
	    FRAMEWORK_BUILD=1
	else
	    if test $SHARED_BUILD = 1; then
		AC_MSG_RESULT([shared library])
	    else
		AC_MSG_RESULT([static library])
	    fi
	    FRAMEWORK_BUILD=0
	fi
    fi
])

#------------------------------------------------------------------------
# SC_ENABLE_THREADS --
#
#	Specify if thread support should be enabled
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
#------------------------------------------------------------------------

AC_DEFUN([SC_ENABLE_THREADS], [
    AC_ARG_ENABLE(threads,
	AC_HELP_STRING([--enable-threads],
	    [build with threads (default: on)]),
	[tcl_ok=$enableval], [tcl_ok=yes])

    if test "${TCL_THREADS}" = 1; then
	tcl_threaded_core=1;
    fi

    if test "$tcl_ok" = "yes" -o "${TCL_THREADS}" = 1; then
	TCL_THREADS=1
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
			AC_MSG_WARN([Don't know how to find pthread lib on your system - you must disable thread support or edit the LIBS in the Makefile...])
		    fi
		fi
	    fi
	fi

	# Does the pthread-implementation provide
	# 'pthread_attr_setstacksize' ?

	ac_saved_libs=$LIBS
	LIBS="$LIBS $THREADS_LIBS"
	AC_CHECK_FUNCS(pthread_attr_setstacksize pthread_atfork)
	LIBS=$ac_saved_libs
    else
	TCL_THREADS=0
    fi
    # Do checking message here to not mess up interleaved configure output
    AC_MSG_CHECKING([for building with threads])
    if test "${TCL_THREADS}" = 1; then
	AC_DEFINE(TCL_THREADS, 1, [Are we building with threads enabled?])
	if test "${tcl_threaded_core}" = 1; then
	    AC_MSG_RESULT([yes (threaded core)])
	else
	    AC_MSG_RESULT([yes])
	fi
    else
	AC_MSG_RESULT([no])
    fi

    AC_SUBST(TCL_THREADS)
])

#------------------------------------------------------------------------
# SC_ENABLE_SYMBOLS --
#
#	Specify if debugging symbols should be used.
#	Memory (TCL_MEM_DEBUG) and compile (TCL_COMPILE_DEBUG) debugging
#	can also be enabled.
#
# Arguments:
#	none
#
#	Requires the following vars to be set in the Makefile:
#		CFLAGS_DEBUG
#		CFLAGS_OPTIMIZE
#		LDFLAGS_DEBUG
#		LDFLAGS_OPTIMIZE
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
#------------------------------------------------------------------------

AC_DEFUN([SC_ENABLE_SYMBOLS], [
    AC_MSG_CHECKING([for build with symbols])
    AC_ARG_ENABLE(symbols,
	AC_HELP_STRING([--enable-symbols],
	    [build with debugging symbols (default: off)]),
	[tcl_ok=$enableval], [tcl_ok=no])
# FIXME: Currently, LDFLAGS_DEFAULT is not used, it should work like CFLAGS_DEFAULT.
    DBGX=""
    if test "$tcl_ok" = "no"; then
	CFLAGS_DEFAULT='$(CFLAGS_OPTIMIZE)'
	LDFLAGS_DEFAULT='$(LDFLAGS_OPTIMIZE)'
	AC_DEFINE(NDEBUG, 1, [Is no debugging enabled?])
	AC_MSG_RESULT([no])
	AC_DEFINE(TCL_CFG_OPTIMIZED, 1, [Is this an optimized build?])
    else
	CFLAGS_DEFAULT='$(CFLAGS_DEBUG)'
	LDFLAGS_DEFAULT='$(LDFLAGS_DEBUG)'
	if test "$tcl_ok" = "yes"; then
	    AC_MSG_RESULT([yes (standard debugging)])
	fi
    fi
    AC_SUBST(CFLAGS_DEFAULT)
    AC_SUBST(LDFLAGS_DEFAULT)

    if test "$tcl_ok" = "mem" -o "$tcl_ok" = "all"; then
	AC_DEFINE(TCL_MEM_DEBUG, 1, [Is memory debugging enabled?])
    fi

    ifelse($1,bccdebug,dnl Only enable 'compile' for the Tcl core itself
	if test "$tcl_ok" = "compile" -o "$tcl_ok" = "all"; then
	    AC_DEFINE(TCL_COMPILE_DEBUG, 1, [Is bytecode debugging enabled?])
	    AC_DEFINE(TCL_COMPILE_STATS, 1, [Are bytecode statistics enabled?])
	fi)

    if test "$tcl_ok" != "yes" -a "$tcl_ok" != "no"; then
	if test "$tcl_ok" = "all"; then
	    AC_MSG_RESULT([enabled symbols mem ]ifelse($1,bccdebug,[compile ])[debugging])
	else
	    AC_MSG_RESULT([enabled $tcl_ok debugging])
	fi
    fi
])

#------------------------------------------------------------------------
# SC_ENABLE_LANGINFO --
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
#------------------------------------------------------------------------

AC_DEFUN([SC_ENABLE_LANGINFO], [
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
# SC_CONFIG_MANPAGES
#
#	Decide whether to use symlinks for linking the manpages,
#	whether to compress the manpages after installation, and
#	whether to add a package name suffix to the installed
#	manpages to avoidfile name clashes.
#	If compression is enabled also find out what file name suffix
#	the given compression program is using.
#
# Arguments:
#	none
#
# Results:
#
#	Adds the following arguments to configure:
#		--enable-man-symlinks
#		--enable-man-compression=PROG
#		--enable-man-suffix[=STRING]
#
#	Defines the following variable:
#
#	MAN_FLAGS -	The apropriate flags for installManPage
#			according to the user's selection.
#
#--------------------------------------------------------------------

AC_DEFUN([SC_CONFIG_MANPAGES], [
    AC_MSG_CHECKING([whether to use symlinks for manpages])
    AC_ARG_ENABLE(man-symlinks,
	AC_HELP_STRING([--enable-man-symlinks],
	    [use symlinks for the manpages (default: off)]),
	test "$enableval" != "no" && MAN_FLAGS="$MAN_FLAGS --symlinks",
	enableval="no")
    AC_MSG_RESULT([$enableval])

    AC_MSG_CHECKING([whether to compress the manpages])
    AC_ARG_ENABLE(man-compression,
	AC_HELP_STRING([--enable-man-compression=PROG],
	    [compress the manpages with PROG (default: off)]),
	[case $enableval in
	    yes) AC_MSG_ERROR([missing argument to --enable-man-compression]);;
	    no)  ;;
	    *)   MAN_FLAGS="$MAN_FLAGS --compress $enableval";;
	esac],
	enableval="no")
    AC_MSG_RESULT([$enableval])
    if test "$enableval" != "no"; then
	AC_MSG_CHECKING([for compressed file suffix])
	touch TeST
	$enableval TeST
	Z=`ls TeST* | sed 's/^....//'`
	rm -f TeST*
	MAN_FLAGS="$MAN_FLAGS --extension $Z"
	AC_MSG_RESULT([$Z])
    fi

    AC_MSG_CHECKING([whether to add a package name suffix for the manpages])
    AC_ARG_ENABLE(man-suffix,
	AC_HELP_STRING([--enable-man-suffix=STRING],
	    [use STRING as a suffix to manpage file names (default: no, AC_PACKAGE_NAME if enabled without specifying STRING)]),
	[case $enableval in
	    yes) enableval="AC_PACKAGE_NAME" MAN_FLAGS="$MAN_FLAGS --suffix $enableval";;
	    no)  ;;
	    *)   MAN_FLAGS="$MAN_FLAGS --suffix $enableval";;
	esac],
	enableval="no")
    AC_MSG_RESULT([$enableval])

    AC_SUBST(MAN_FLAGS)
])

#--------------------------------------------------------------------
# SC_CONFIG_SYSTEM
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

AC_DEFUN([SC_CONFIG_SYSTEM], [
    AC_CACHE_CHECK([system version], tcl_cv_sys_version, [
	if test -f /usr/lib/NextStep/software_version; then
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
		if test "`uname -s`" = "NetBSD" -a -f /etc/debian_version ; then
		    tcl_cv_sys_version=NetBSD-Debian
		fi
	    fi
	fi
    ])
    system=$tcl_cv_sys_version
])

#--------------------------------------------------------------------
# SC_CONFIG_CFLAGS
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
#       MAKE_LIB -      Command to execute to build the a library;
#                       differs when building shared or static.
#       MAKE_STUB_LIB -
#                       Command to execute to build a stub library.
#       INSTALL_LIB -   Command to execute to install a library;
#                       differs when building shared or static.
#       INSTALL_STUB_LIB -
#                       Command to execute to install a stub library.
#       STLIB_LD -      Base command to use for combining object files
#                       into a static library.
#       SHLIB_CFLAGS -  Flags to pass to cc when compiling the components
#                       of a shared library (may request position-independent
#                       code, among other things).
#       SHLIB_LD -      Base command to use for combining object files
#                       into a shared library.
#       SHLIB_LD_LIBS - Dependent libraries for the linker to scan when
#                       creating shared libraries.  This symbol typically
#                       goes at the end of the "ld" commands that build
#                       shared libraries. The value of the symbol defaults to
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
# TCL_SHLIB_LD_EXTRAS - Additional element which are added to SHLIB_LD_LIBS
#  TK_SHLIB_LD_EXTRAS   for the build of Tcl and Tk, but not recorded in the
#                       tclConfig.sh, since they are only used for the build
#                       of Tcl and Tk.
#                       Examples: MacOS X records the library version and
#                       compatibility version in the shared library.  But
#                       of course the Tcl version of this is only used for Tcl.
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
#       TCL_LIBS -
#                       Libs to use when linking Tcl shell or some other
#                       shell that includes Tcl libs.
#	CFLAGS_DEBUG -
#			Flags used when running the compiler in debug mode
#	CFLAGS_OPTIMIZE -
#			Flags used when running the compiler in optimize mode
#	CFLAGS -	Additional CFLAGS added as necessary (usually 64-bit)
#
#--------------------------------------------------------------------

AC_DEFUN([SC_CONFIG_CFLAGS], [

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
    # Force 64bit on with VIS
    AS_IF([test "$do64bitVIS" = "yes"], [do64bit=yes])

    # Step 0.c: Check if visibility support is available. Do this here so
    # that platform specific alternatives can be used below if this fails.

    AC_CACHE_CHECK([if compiler supports visibility "hidden"],
	tcl_cv_cc_visibility_hidden, [
	hold_cflags=$CFLAGS; CFLAGS="$CFLAGS -Werror"
	AC_TRY_LINK([
	    extern __attribute__((__visibility__("hidden"))) void f(void);
	    void f(void) {}], [f();], tcl_cv_cc_visibility_hidden=yes,
	    tcl_cv_cc_visibility_hidden=no)
	CFLAGS=$hold_cflags])
    AS_IF([test $tcl_cv_cc_visibility_hidden = yes], [
	AC_DEFINE(MODULE_SCOPE,
	    [extern __attribute__((__visibility__("hidden")))],
	    [Compiler support for module scope symbols])
	AC_DEFINE(HAVE_HIDDEN, [1], [Compiler support for module scope symbols])
    ])

    # Step 0.d: Disable -rpath support?

    AC_MSG_CHECKING([if rpath support is requested])
    AC_ARG_ENABLE(rpath,
	AC_HELP_STRING([--disable-rpath],
	    [disable rpath support (default: on)]),
	[doRpath=$enableval], [doRpath=yes])
    AC_MSG_RESULT([$doRpath])

    # Step 1: set the variable "system" to hold the name and version number
    # for the system.

    SC_CONFIG_SYSTEM

    # Step 2: check for existence of -ldl library.  This is needed because
    # Linux can use either -ldl or -ldld for dynamic loading.

    AC_CHECK_LIB(dl, dlopen, have_dl=yes, have_dl=no)

    # Require ranlib early so we can override it in special cases below.

    AC_REQUIRE([AC_PROG_RANLIB])

    # Step 3: set configuration options based on system name and version.

    do64bit_ok=no
    # default to '{$LIBS}' and set to "" on per-platform necessary basis
    SHLIB_LD_LIBS='${LIBS}'
    LDFLAGS_ORIG="$LDFLAGS"
    # When ld needs options to work in 64-bit mode, put them in
    # LDFLAGS_ARCH so they eventually end up in LDFLAGS even if [load]
    # is disabled by the user. [Bug 1016796]
    LDFLAGS_ARCH=""
    UNSHARED_LIB_SUFFIX=""
    TCL_TRIM_DOTS='`echo ${VERSION} | tr -d .`'
    ECHO_VERSION='`echo ${VERSION}`'
    TCL_LIB_VERSIONS_OK=ok
    CFLAGS_DEBUG=-g
    AS_IF([test "$GCC" = yes], [
	CFLAGS_OPTIMIZE=-O2
	CFLAGS_WARNING="-Wall"
    ], [
	CFLAGS_OPTIMIZE=-O
	CFLAGS_WARNING=""
    ])
    AC_CHECK_TOOL(AR, ar)
    STLIB_LD='${AR} cr'
    LD_LIBRARY_PATH_VAR="LD_LIBRARY_PATH"
    PLAT_OBJS=""
    PLAT_SRCS=""
    LDAIX_SRC=""
    AS_IF([test "x${SHLIB_VERSION}" = x],[SHLIB_VERSION=".1.0"],[SHLIB_VERSION=".${SHLIB_VERSION}"])
    case $system in
	AIX-*)
	    AS_IF([test "${TCL_THREADS}" = "1" -a "$GCC" != "yes"], [
		# AIX requires the _r compiler when gcc isn't being used
		case "${CC}" in
		    *_r|*_r\ *)
			# ok ...
			;;
		    *)
			# Make sure only first arg gets _r
		    	CC=`echo "$CC" | sed -e 's/^\([[^ ]]*\)/\1_r/'`
			;;
		esac
		AC_MSG_RESULT([Using $CC for compiling with threads])
	    ])
	    LIBS="$LIBS -lc"
	    SHLIB_CFLAGS=""
	    SHLIB_SUFFIX=".so"

	    DL_OBJS="tclLoadDl.o"
	    LD_LIBRARY_PATH_VAR="LIBPATH"

	    # ldAix No longer needed with use of -bexpall/-brtl
	    # but some extensions may still reference it
	    LDAIX_SRC='$(UNIX_DIR)/ldAix'

	    # Check to enable 64-bit flags for compiler/linker
	    AS_IF([test "$do64bit" = yes], [
		AS_IF([test "$GCC" = yes], [
		    AC_MSG_WARN([64bit mode not supported with GCC on $system])
		], [
		    do64bit_ok=yes
		    CFLAGS="$CFLAGS -q64"
		    LDFLAGS_ARCH="-q64"
		    RANLIB="${RANLIB} -X64"
		    AR="${AR} -X64"
		    SHLIB_LD_FLAGS="-b64"
		])
	    ])

	    AS_IF([test "`uname -m`" = ia64], [
		# AIX-5 uses ELF style dynamic libraries on IA-64, but not PPC
		SHLIB_LD="/usr/ccs/bin/ld -G -z text"
		# AIX-5 has dl* in libc.so
		DL_LIBS=""
		AS_IF([test "$GCC" = yes], [
		    CC_SEARCH_FLAGS='-Wl,-R,${LIB_RUNTIME_DIR}'
		], [
		    CC_SEARCH_FLAGS='-R${LIB_RUNTIME_DIR}'
		])
		LD_SEARCH_FLAGS='-R ${LIB_RUNTIME_DIR}'
	    ], [
		AS_IF([test "$GCC" = yes], [
		    SHLIB_LD='${CC} -shared -Wl,-bexpall'
		], [
		    SHLIB_LD="/bin/ld -bhalt:4 -bM:SRE -bexpall -H512 -T512 -bnoentry"
		    LDFLAGS="$LDFLAGS -brtl"
		])
		SHLIB_LD="${SHLIB_LD} ${SHLIB_LD_FLAGS}"
		DL_LIBS="-ldl"
		CC_SEARCH_FLAGS='-L${LIB_RUNTIME_DIR}'
		LD_SEARCH_FLAGS=${CC_SEARCH_FLAGS}
	    ])
	    ;;
	BeOS*)
	    SHLIB_CFLAGS="-fPIC"
	    SHLIB_LD='${CC} -nostart'
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
	    SHLIB_SUFFIX=".so"
	    DL_OBJS="tclLoadDl.o"
	    DL_LIBS="-ldl"
	    CC_SEARCH_FLAGS=""
	    LD_SEARCH_FLAGS=""
	    ;;
	BSD/OS-4.*)
	    SHLIB_CFLAGS="-export-dynamic -fPIC"
	    SHLIB_LD='${CC} -shared'
	    SHLIB_SUFFIX=".so"
	    DL_OBJS="tclLoadDl.o"
	    DL_LIBS="-ldl"
	    LDFLAGS="$LDFLAGS -export-dynamic"
	    CC_SEARCH_FLAGS=""
	    LD_SEARCH_FLAGS=""
	    ;;
	CYGWIN_*|MINGW32*)
	    SHLIB_CFLAGS=""
	    SHLIB_LD='${CC} -shared'
	    SHLIB_SUFFIX=".dll"
	    DL_OBJS="tclLoadDl.o"
	    PLAT_OBJS='${CYGWIN_OBJS}'
	    PLAT_SRCS='${CYGWIN_SRCS}'
	    DL_LIBS="-ldl"
	    CC_SEARCH_FLAGS=""
	    LD_SEARCH_FLAGS=""
	    TCL_NEEDS_EXP_FILE=1
	    TCL_EXPORT_FILE_SUFFIX='${VERSION}\$\{DBGX\}.dll.a'
	    SHLIB_LD_LIBS="${SHLIB_LD_LIBS} -Wl,--out-implib,\$[@].a"
	    AC_CACHE_CHECK(for Cygwin version of gcc,
		ac_cv_cygwin,
		AC_TRY_COMPILE([
		#ifdef __CYGWIN__
		    #error cygwin
		#endif
		], [],
		ac_cv_cygwin=no,
		ac_cv_cygwin=yes)
	    )
	    if test "$ac_cv_cygwin" = "no"; then
		AC_MSG_ERROR([${CC} is not a cygwin compiler.])
	    fi
	    if test "x${TCL_THREADS}" = "x0"; then
		AC_MSG_ERROR([CYGWIN compile is only supported with --enable-threads])
	    fi
	    do64bit_ok=yes
	    if test "x${SHARED_BUILD}" = "x1"; then
		echo "running cd ../win; ${CONFIG_SHELL-/bin/sh} ./configure $ac_configure_args"
		# The eval makes quoting arguments work.
		if cd ../win; eval ${CONFIG_SHELL-/bin/sh} ./configure $ac_configure_args; cd ../unix
		then :
		else
		    { echo "configure: error: configure failed for ../win" 1>&2; exit 1; }
		fi
	    fi
	    ;;
	dgux*)
	    SHLIB_CFLAGS="-K PIC"
	    SHLIB_LD='${CC} -G'
	    SHLIB_LD_LIBS=""
	    SHLIB_SUFFIX=".so"
	    DL_OBJS="tclLoadDl.o"
	    DL_LIBS="-ldl"
	    CC_SEARCH_FLAGS=""
	    LD_SEARCH_FLAGS=""
	    ;;
	Haiku*)
	    LDFLAGS="$LDFLAGS -Wl,--export-dynamic"
	    SHLIB_CFLAGS="-fPIC"
	    SHLIB_SUFFIX=".so"
	    SHLIB_LD='${CC} ${CFLAGS} ${LDFLAGS} -shared'
	    DL_OBJS="tclLoadDl.o"
	    DL_LIBS="-lroot"
	    AC_CHECK_LIB(network, inet_ntoa, [LIBS="$LIBS -lnetwork"])
	    ;;
	HP-UX-*.11.*)
	    # Use updated header definitions where possible
	    AC_DEFINE(_XOPEN_SOURCE_EXTENDED, 1, [Do we want to use the XOPEN network library?])
	    AC_DEFINE(_XOPEN_SOURCE, 1, [Do we want to use the XOPEN network library?])
	    LIBS="$LIBS -lxnet"               # Use the XOPEN network library

	    AS_IF([test "`uname -m`" = ia64], [
		SHLIB_SUFFIX=".so"
	    ], [
		SHLIB_SUFFIX=".sl"
	    ])
	    AC_CHECK_LIB(dld, shl_load, tcl_ok=yes, tcl_ok=no)
	    AS_IF([test "$tcl_ok" = yes], [
		SHLIB_CFLAGS="+z"
		SHLIB_LD="ld -b"
		DL_OBJS="tclLoadShl.o"
		DL_LIBS="-ldld"
		LDFLAGS="$LDFLAGS -Wl,-E"
		CC_SEARCH_FLAGS='-Wl,+s,+b,${LIB_RUNTIME_DIR}:.'
		LD_SEARCH_FLAGS='+s +b ${LIB_RUNTIME_DIR}:.'
		LD_LIBRARY_PATH_VAR="SHLIB_PATH"
	    ])
	    AS_IF([test "$GCC" = yes], [
		SHLIB_LD='${CC} -shared'
		LD_SEARCH_FLAGS=${CC_SEARCH_FLAGS}
	    ], [
		CFLAGS="$CFLAGS -z"
	    ])

	    # Users may want PA-RISC 1.1/2.0 portable code - needs HP cc
	    #CFLAGS="$CFLAGS +DAportable"

	    # Check to enable 64-bit flags for compiler/linker
	    AS_IF([test "$do64bit" = "yes"], [
		AS_IF([test "$GCC" = yes], [
		    case `${CC} -dumpmachine` in
			hppa64*)
			    # 64-bit gcc in use.  Fix flags for GNU ld.
			    do64bit_ok=yes
			    SHLIB_LD='${CC} -shared'
			    AS_IF([test $doRpath = yes], [
				CC_SEARCH_FLAGS='-Wl,-rpath,${LIB_RUNTIME_DIR}'])
			    LD_SEARCH_FLAGS=${CC_SEARCH_FLAGS}
			    ;;
			*)
			    AC_MSG_WARN([64bit mode not supported with GCC on $system])
			    ;;
		    esac
		], [
		    do64bit_ok=yes
		    CFLAGS="$CFLAGS +DD64"
		    LDFLAGS_ARCH="+DD64"
		])
	    ]) ;;
	HP-UX-*.08.*|HP-UX-*.09.*|HP-UX-*.10.*)
	    SHLIB_SUFFIX=".sl"
	    AC_CHECK_LIB(dld, shl_load, tcl_ok=yes, tcl_ok=no)
	    AS_IF([test "$tcl_ok" = yes], [
		SHLIB_CFLAGS="+z"
		SHLIB_LD="ld -b"
		SHLIB_LD_LIBS=""
		DL_OBJS="tclLoadShl.o"
		DL_LIBS="-ldld"
		LDFLAGS="$LDFLAGS -Wl,-E"
		CC_SEARCH_FLAGS='-Wl,+s,+b,${LIB_RUNTIME_DIR}:.'
		LD_SEARCH_FLAGS='+s +b ${LIB_RUNTIME_DIR}:.'
		LD_LIBRARY_PATH_VAR="SHLIB_PATH"
	    ]) ;;
	IRIX-5.*)
	    SHLIB_CFLAGS=""
	    SHLIB_LD="ld -shared -rdata_shared"
	    SHLIB_SUFFIX=".so"
	    DL_OBJS="tclLoadDl.o"
	    DL_LIBS=""
	    AC_LIBOBJ(mkstemp)
	    AS_IF([test $doRpath = yes], [
		CC_SEARCH_FLAGS='-Wl,-rpath,${LIB_RUNTIME_DIR}'
		LD_SEARCH_FLAGS='-rpath ${LIB_RUNTIME_DIR}'])
	    ;;
	IRIX-6.*)
	    SHLIB_CFLAGS=""
	    SHLIB_LD="ld -n32 -shared -rdata_shared"
	    SHLIB_SUFFIX=".so"
	    DL_OBJS="tclLoadDl.o"
	    DL_LIBS=""
	    AC_LIBOBJ(mkstemp)
	    AS_IF([test $doRpath = yes], [
		CC_SEARCH_FLAGS='-Wl,-rpath,${LIB_RUNTIME_DIR}'
		LD_SEARCH_FLAGS='-rpath ${LIB_RUNTIME_DIR}'])
	    AS_IF([test "$GCC" = yes], [
		CFLAGS="$CFLAGS -mabi=n32"
		LDFLAGS="$LDFLAGS -mabi=n32"
	    ], [
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
	    ])
	    ;;
	IRIX64-6.*)
	    SHLIB_CFLAGS=""
	    SHLIB_LD="ld -n32 -shared -rdata_shared"
	    SHLIB_SUFFIX=".so"
	    DL_OBJS="tclLoadDl.o"
	    DL_LIBS=""
	    AC_LIBOBJ(mkstemp)
	    AS_IF([test $doRpath = yes], [
		CC_SEARCH_FLAGS='-Wl,-rpath,${LIB_RUNTIME_DIR}'
		LD_SEARCH_FLAGS='-rpath ${LIB_RUNTIME_DIR}'])

	    # Check to enable 64-bit flags for compiler/linker

	    AS_IF([test "$do64bit" = yes], [
	        AS_IF([test "$GCC" = yes], [
	            AC_MSG_WARN([64bit mode not supported by gcc])
	        ], [
	            do64bit_ok=yes
	            SHLIB_LD="ld -64 -shared -rdata_shared"
	            CFLAGS="$CFLAGS -64"
	            LDFLAGS_ARCH="-64"
	        ])
	    ])
	    ;;
	Linux*|GNU*|NetBSD-Debian)
	    SHLIB_CFLAGS="-fPIC"
	    SHLIB_SUFFIX=".so"

	    CFLAGS_OPTIMIZE="-O2"
	    # egcs-2.91.66 on Redhat Linux 6.0 generates lots of warnings
	    # when you inline the string and math operations.  Turn this off to
	    # get rid of the warnings.
	    #CFLAGS_OPTIMIZE="${CFLAGS_OPTIMIZE} -D__NO_STRING_INLINES -D__NO_MATH_INLINES"

	    SHLIB_LD='${CC} ${CFLAGS} ${LDFLAGS} -shared'
	    DL_OBJS="tclLoadDl.o"
	    DL_LIBS="-ldl"
	    LDFLAGS="$LDFLAGS -Wl,--export-dynamic"
	    AS_IF([test $doRpath = yes], [
		CC_SEARCH_FLAGS='-Wl,-rpath,${LIB_RUNTIME_DIR}'])
	    LD_SEARCH_FLAGS=${CC_SEARCH_FLAGS}
	    AS_IF([test "`uname -m`" = "alpha"], [CFLAGS="$CFLAGS -mieee"])
	    AS_IF([test $do64bit = yes], [
		AC_CACHE_CHECK([if compiler accepts -m64 flag], tcl_cv_cc_m64, [
		    hold_cflags=$CFLAGS
		    CFLAGS="$CFLAGS -m64"
		    AC_TRY_LINK(,, tcl_cv_cc_m64=yes, tcl_cv_cc_m64=no)
		    CFLAGS=$hold_cflags])
		AS_IF([test $tcl_cv_cc_m64 = yes], [
		    CFLAGS="$CFLAGS -m64"
		    do64bit_ok=yes
		])
	   ])

	    # The combo of gcc + glibc has a bug related to inlining of
	    # functions like strtod(). The -fno-builtin flag should address
	    # this problem but it does not work. The -fno-inline flag is kind
	    # of overkill but it works. Disable inlining only when one of the
	    # files in compat/*.c is being linked in.

	    AS_IF([test x"${USE_COMPAT}" != x],[CFLAGS="$CFLAGS -fno-inline"])
	    ;;
	Lynx*)
	    SHLIB_CFLAGS="-fPIC"
	    SHLIB_SUFFIX=".so"
	    CFLAGS_OPTIMIZE=-02
	    SHLIB_LD='${CC} -shared'
	    DL_OBJS="tclLoadDl.o"
	    DL_LIBS="-mshared -ldl"
	    LD_FLAGS="-Wl,--export-dynamic"
	    AS_IF([test $doRpath = yes], [
		CC_SEARCH_FLAGS='-Wl,-rpath,${LIB_RUNTIME_DIR}'
		LD_SEARCH_FLAGS='-Wl,-rpath,${LIB_RUNTIME_DIR}'])
	    ;;
	MP-RAS-02*)
	    SHLIB_CFLAGS="-K PIC"
	    SHLIB_LD='${CC} -G'
	    SHLIB_LD_LIBS=""
	    SHLIB_SUFFIX=".so"
	    DL_OBJS="tclLoadDl.o"
	    DL_LIBS="-ldl"
	    CC_SEARCH_FLAGS=""
	    LD_SEARCH_FLAGS=""
	    ;;
	MP-RAS-*)
	    SHLIB_CFLAGS="-K PIC"
	    SHLIB_LD='${CC} -G'
	    SHLIB_LD_LIBS=""
	    SHLIB_SUFFIX=".so"
	    DL_OBJS="tclLoadDl.o"
	    DL_LIBS="-ldl"
	    LDFLAGS="$LDFLAGS -Wl,-Bexport"
	    CC_SEARCH_FLAGS=""
	    LD_SEARCH_FLAGS=""
	    ;;
	OpenBSD-*)
	    arch=`arch -s`
	    case "$arch" in
	    alpha|sparc64)
		SHLIB_CFLAGS="-fPIC"
		;;
	    *)
		SHLIB_CFLAGS="-fpic"
		;;
	    esac
	    SHLIB_LD='${CC} ${SHLIB_CFLAGS} -shared'
	    SHLIB_SUFFIX=".so"
	    DL_OBJS="tclLoadDl.o"
	    DL_LIBS=""
	    AS_IF([test $doRpath = yes], [
		CC_SEARCH_FLAGS='-Wl,-rpath,${LIB_RUNTIME_DIR}'])
	    LD_SEARCH_FLAGS=${CC_SEARCH_FLAGS}
	    SHARED_LIB_SUFFIX='${TCL_TRIM_DOTS}.so${SHLIB_VERSION}'
	    LDFLAGS="-Wl,-export-dynamic"
	    CFLAGS_OPTIMIZE="-O2"
	    AS_IF([test "${TCL_THREADS}" = "1"], [
		# On OpenBSD:	Compile with -pthread
		#		Don't link with -lpthread
		LIBS=`echo $LIBS | sed s/-lpthread//`
		CFLAGS="$CFLAGS -pthread"
	    ])
	    # OpenBSD doesn't do version numbers with dots.
	    UNSHARED_LIB_SUFFIX='${TCL_TRIM_DOTS}.a'
	    TCL_LIB_VERSIONS_OK=nodots
	    ;;
	NetBSD-*)
	    # NetBSD has ELF and can use 'cc -shared' to build shared libs
	    SHLIB_CFLAGS="-fPIC"
	    SHLIB_LD='${CC} ${SHLIB_CFLAGS} -shared'
	    SHLIB_SUFFIX=".so"
	    DL_OBJS="tclLoadDl.o"
	    DL_LIBS=""
	    LDFLAGS="$LDFLAGS -export-dynamic"
	    AS_IF([test $doRpath = yes], [
		CC_SEARCH_FLAGS='-Wl,-rpath,${LIB_RUNTIME_DIR}'])
	    LD_SEARCH_FLAGS=${CC_SEARCH_FLAGS}
	    AS_IF([test "${TCL_THREADS}" = "1"], [
		# The -pthread needs to go in the CFLAGS, not LIBS
		LIBS=`echo $LIBS | sed s/-pthread//`
		CFLAGS="$CFLAGS -pthread"
	    	LDFLAGS="$LDFLAGS -pthread"
	    ])
	    ;;
	DragonFly-*|FreeBSD-*)
	    # This configuration from FreeBSD Ports.
	    SHLIB_CFLAGS="-fPIC"
	    SHLIB_LD="${CC} -shared"
	    SHLIB_LD_LIBS="${SHLIB_LD_LIBS} -Wl,-soname,\$[@]"
	    SHLIB_SUFFIX=".so"
	    DL_OBJS="tclLoadDl.o"
	    DL_LIBS=""
	    LDFLAGS=""
	    AS_IF([test $doRpath = yes], [
		CC_SEARCH_FLAGS='-Wl,-rpath,${LIB_RUNTIME_DIR}'
		LD_SEARCH_FLAGS='-Wl,-rpath,${LIB_RUNTIME_DIR}'])
	    AS_IF([test "${TCL_THREADS}" = "1"], [
		# The -pthread needs to go in the LDFLAGS, not LIBS
		LIBS=`echo $LIBS | sed s/-pthread//`
		CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
		LDFLAGS="$LDFLAGS $PTHREAD_LIBS"])
	    case $system in
	    FreeBSD-3.*)
		# Version numbers are dot-stripped by system policy.
		TCL_TRIM_DOTS=`echo ${VERSION} | tr -d .`
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
	    AS_IF([test $do64bit = yes], [
		case `arch` in
		    ppc)
			AC_CACHE_CHECK([if compiler accepts -arch ppc64 flag],
				tcl_cv_cc_arch_ppc64, [
			    hold_cflags=$CFLAGS
			    CFLAGS="$CFLAGS -arch ppc64 -mpowerpc64 -mcpu=G5"
			    AC_TRY_LINK(,, tcl_cv_cc_arch_ppc64=yes,
				    tcl_cv_cc_arch_ppc64=no)
			    CFLAGS=$hold_cflags])
			AS_IF([test $tcl_cv_cc_arch_ppc64 = yes], [
			    CFLAGS="$CFLAGS -arch ppc64 -mpowerpc64 -mcpu=G5"
			    do64bit_ok=yes
			]);;
		    i386)
			AC_CACHE_CHECK([if compiler accepts -arch x86_64 flag],
				tcl_cv_cc_arch_x86_64, [
			    hold_cflags=$CFLAGS
			    CFLAGS="$CFLAGS -arch x86_64"
			    AC_TRY_LINK(,, tcl_cv_cc_arch_x86_64=yes,
				    tcl_cv_cc_arch_x86_64=no)
			    CFLAGS=$hold_cflags])
			AS_IF([test $tcl_cv_cc_arch_x86_64 = yes], [
			    CFLAGS="$CFLAGS -arch x86_64"
			    do64bit_ok=yes
			]);;
		    *)
			AC_MSG_WARN([Don't know how enable 64-bit on architecture `arch`]);;
		esac
	    ], [
		# Check for combined 32-bit and 64-bit fat build
		AS_IF([echo "$CFLAGS " |grep -E -q -- '-arch (ppc64|x86_64) ' \
		    && echo "$CFLAGS " |grep -E -q -- '-arch (ppc|i386) '], [
		    fat_32_64=yes])
	    ])
	    SHLIB_LD='${CC} -dynamiclib ${CFLAGS} ${LDFLAGS}'
	    AC_CACHE_CHECK([if ld accepts -single_module flag], tcl_cv_ld_single_module, [
		hold_ldflags=$LDFLAGS
		LDFLAGS="$LDFLAGS -dynamiclib -Wl,-single_module"
		AC_TRY_LINK(, [int i;], tcl_cv_ld_single_module=yes, tcl_cv_ld_single_module=no)
		LDFLAGS=$hold_ldflags])
	    AS_IF([test $tcl_cv_ld_single_module = yes], [
		SHLIB_LD="${SHLIB_LD} -Wl,-single_module"
	    ])
	    SHLIB_SUFFIX=".dylib"
	    DL_OBJS="tclLoadDyld.o"
	    DL_LIBS=""
	    LDFLAGS="$LDFLAGS -headerpad_max_install_names"
	    AC_CACHE_CHECK([if ld accepts -search_paths_first flag],
		    tcl_cv_ld_search_paths_first, [
		hold_ldflags=$LDFLAGS
		LDFLAGS="$LDFLAGS -Wl,-search_paths_first"
		AC_TRY_LINK(, [int i;], tcl_cv_ld_search_paths_first=yes,
			tcl_cv_ld_search_paths_first=no)
		LDFLAGS=$hold_ldflags])
	    AS_IF([test $tcl_cv_ld_search_paths_first = yes], [
		LDFLAGS="$LDFLAGS -Wl,-search_paths_first"
	    ])
	    AS_IF([test "$tcl_cv_cc_visibility_hidden" != yes], [
		AC_DEFINE(MODULE_SCOPE, [__private_extern__],
		    [Compiler support for module scope symbols])
	    ])
	    CC_SEARCH_FLAGS=""
	    LD_SEARCH_FLAGS=""
	    LD_LIBRARY_PATH_VAR="DYLD_LIBRARY_PATH"
	    AC_DEFINE(MAC_OSX_TCL, 1, [Is this a Mac I see before me?])
	    PLAT_OBJS='${MAC_OSX_OBJS}'
	    PLAT_SRCS='${MAC_OSX_SRCS}'
	    AC_MSG_CHECKING([whether to use CoreFoundation])
	    AC_ARG_ENABLE(corefoundation,
		AC_HELP_STRING([--enable-corefoundation],
		    [use CoreFoundation API on MacOSX (default: on)]),
		[tcl_corefoundation=$enableval], [tcl_corefoundation=yes])
	    AC_MSG_RESULT([$tcl_corefoundation])
	    AS_IF([test $tcl_corefoundation = yes], [
		AC_CACHE_CHECK([for CoreFoundation.framework],
			tcl_cv_lib_corefoundation, [
		    hold_libs=$LIBS
		    AS_IF([test "$fat_32_64" = yes], [
			for v in CFLAGS CPPFLAGS LDFLAGS; do
			    # On Tiger there is no 64-bit CF, so remove 64-bit
			    # archs from CFLAGS et al. while testing for
			    # presence of CF. 64-bit CF is disabled in
			    # tclUnixPort.h if necessary.
			    eval 'hold_'$v'="$'$v'";'$v'="`echo "$'$v' "|sed -e "s/-arch ppc64 / /g" -e "s/-arch x86_64 / /g"`"'
			done])
		    LIBS="$LIBS -framework CoreFoundation"
		    AC_TRY_LINK([#include <CoreFoundation/CoreFoundation.h>],
			[CFBundleRef b = CFBundleGetMainBundle();],
			tcl_cv_lib_corefoundation=yes,
			tcl_cv_lib_corefoundation=no)
		    AS_IF([test "$fat_32_64" = yes], [
			for v in CFLAGS CPPFLAGS LDFLAGS; do
			    eval $v'="$hold_'$v'"'
		        done])
		    LIBS=$hold_libs])
		AS_IF([test $tcl_cv_lib_corefoundation = yes], [
		    LIBS="$LIBS -framework CoreFoundation"
		    AC_DEFINE(HAVE_COREFOUNDATION, 1,
			[Do we have access to Darwin CoreFoundation.framework?])
		], [tcl_corefoundation=no])
		AS_IF([test "$fat_32_64" = yes -a $tcl_corefoundation = yes],[
		    AC_CACHE_CHECK([for 64-bit CoreFoundation],
			    tcl_cv_lib_corefoundation_64, [
			for v in CFLAGS CPPFLAGS LDFLAGS; do
			    eval 'hold_'$v'="$'$v'";'$v'="`echo "$'$v' "|sed -e "s/-arch ppc / /g" -e "s/-arch i386 / /g"`"'
			done
			AC_TRY_LINK([#include <CoreFoundation/CoreFoundation.h>],
			    [CFBundleRef b = CFBundleGetMainBundle();],
			    tcl_cv_lib_corefoundation_64=yes,
			    tcl_cv_lib_corefoundation_64=no)
			for v in CFLAGS CPPFLAGS LDFLAGS; do
			    eval $v'="$hold_'$v'"'
			done])
		    AS_IF([test $tcl_cv_lib_corefoundation_64 = no], [
			AC_DEFINE(NO_COREFOUNDATION_64, 1,
			    [Is Darwin CoreFoundation unavailable for 64-bit?])
                        LDFLAGS="$LDFLAGS -Wl,-no_arch_warnings"
		    ])
		])
	    ])
	    ;;
	NEXTSTEP-*)
	    SHLIB_CFLAGS=""
	    SHLIB_LD='${CC} -nostdlib -r'
	    SHLIB_LD_LIBS=""
	    SHLIB_SUFFIX=".so"
	    DL_OBJS="tclLoadNext.o"
	    DL_LIBS=""
	    CC_SEARCH_FLAGS=""
	    LD_SEARCH_FLAGS=""
	    ;;
	OS/390-*)
	    SHLIB_LD_LIBS=""
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
	    AS_IF([test "$SHARED_BUILD" = 1], [SHLIB_LD="ld -shared"], [
	        SHLIB_LD="ld -non_shared"
	    ])
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
	    AS_IF([test "$SHARED_BUILD" = 1], [
	        SHLIB_LD='ld -shared -expect_unresolved "*"'
	    ], [
	        SHLIB_LD='ld -non_shared -expect_unresolved "*"'
	    ])
	    SHLIB_SUFFIX=".so"
	    DL_OBJS="tclLoadDl.o"
	    DL_LIBS=""
	    AS_IF([test $doRpath = yes], [
		CC_SEARCH_FLAGS='-Wl,-rpath,${LIB_RUNTIME_DIR}'
		LD_SEARCH_FLAGS='-rpath ${LIB_RUNTIME_DIR}'])
	    AS_IF([test "$GCC" = yes], [CFLAGS="$CFLAGS -mieee"], [
		CFLAGS="$CFLAGS -DHAVE_TZSET -std1 -ieee"])
	    # see pthread_intro(3) for pthread support on osf1, k.furukawa
	    AS_IF([test "${TCL_THREADS}" = 1], [
		CFLAGS="$CFLAGS -DHAVE_PTHREAD_ATTR_SETSTACKSIZE"
		CFLAGS="$CFLAGS -DTCL_THREAD_STACK_MIN=PTHREAD_STACK_MIN*64"
		LIBS=`echo $LIBS | sed s/-lpthreads//`
		AS_IF([test "$GCC" = yes], [
		    LIBS="$LIBS -lpthread -lmach -lexc"
		], [
		    CFLAGS="$CFLAGS -pthread"
		    LDFLAGS="$LDFLAGS -pthread"
		])
	    ])
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
	    AS_IF([test "$GCC" = yes], [
	    	SHLIB_CFLAGS="-fPIC -melf"
	    	LDFLAGS="$LDFLAGS -melf -Wl,-Bexport"
	    ], [
	    	SHLIB_CFLAGS="-Kpic -belf"
	    	LDFLAGS="$LDFLAGS -belf -Wl,-Bexport"
	    ])
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
	    SHLIB_LD='${CC} -G'
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

	    SHARED_LIB_SUFFIX='${TCL_TRIM_DOTS}.so${SHLIB_VERSION}'
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
	    SHLIB_SUFFIX=".so"
	    DL_OBJS="tclLoadDl.o"
	    DL_LIBS="-ldl"
	    AS_IF([test "$GCC" = yes], [
		SHLIB_LD='${CC} -shared'
		CC_SEARCH_FLAGS='-Wl,-R,${LIB_RUNTIME_DIR}'
		LD_SEARCH_FLAGS=${CC_SEARCH_FLAGS}
	    ], [
		SHLIB_LD="/usr/ccs/bin/ld -G -z text"
		CC_SEARCH_FLAGS='-R ${LIB_RUNTIME_DIR}'
		LD_SEARCH_FLAGS=${CC_SEARCH_FLAGS}
	    ])
	    ;;
	SunOS-5*)
	    # Note: If _REENTRANT isn't defined, then Solaris
	    # won't define thread-safe library routines.

	    AC_DEFINE(_REENTRANT, 1, [Do we want the reentrant OS API?])
	    AC_DEFINE(_POSIX_PTHREAD_SEMANTICS, 1,
		[Do we really want to follow the standard? Yes we do!])

	    SHLIB_CFLAGS="-KPIC"

	    # Check to enable 64-bit flags for compiler/linker
	    AS_IF([test "$do64bit" = yes], [
		arch=`isainfo`
		AS_IF([test "$arch" = "sparcv9 sparc"], [
		    AS_IF([test "$GCC" = yes], [
			AS_IF([test "`${CC} -dumpversion | awk -F. '{print [$]1}'`" -lt 3], [
			    AC_MSG_WARN([64bit mode not supported with GCC < 3.2 on $system])
			], [
			    do64bit_ok=yes
			    CFLAGS="$CFLAGS -m64 -mcpu=v9"
			    LDFLAGS="$LDFLAGS -m64 -mcpu=v9"
			    SHLIB_CFLAGS="-fPIC"
			])
		    ], [
			do64bit_ok=yes
			AS_IF([test "$do64bitVIS" = yes], [
			    CFLAGS="$CFLAGS -xarch=v9a"
			    LDFLAGS_ARCH="-xarch=v9a"
			], [
			    CFLAGS="$CFLAGS -xarch=v9"
			    LDFLAGS_ARCH="-xarch=v9"
			])
			# Solaris 64 uses this as well
			#LD_LIBRARY_PATH_VAR="LD_LIBRARY_PATH_64"
		    ])
		], [AS_IF([test "$arch" = "amd64 i386"], [
		    AS_IF([test "$GCC" = yes], [
			case $system in
			    SunOS-5.1[[1-9]]*|SunOS-5.[[2-9]][[0-9]]*)
				do64bit_ok=yes
				CFLAGS="$CFLAGS -m64"
				LDFLAGS="$LDFLAGS -m64";;
			    *)
				AC_MSG_WARN([64bit mode not supported with GCC on $system]);;
			esac
		    ], [
			do64bit_ok=yes
			case $system in
			    SunOS-5.1[[1-9]]*|SunOS-5.[[2-9]][[0-9]]*)
				CFLAGS="$CFLAGS -m64"
				LDFLAGS="$LDFLAGS -m64";;
			    *)
				CFLAGS="$CFLAGS -xarch=amd64"
				LDFLAGS="$LDFLAGS -xarch=amd64";;
			esac
		    ])
		], [AC_MSG_WARN([64bit mode not supported for $arch])])])
	    ])

	    #--------------------------------------------------------------------
	    # On Solaris 5.x i386 with the sunpro compiler we need to link
	    # with sunmath to get floating point rounding control
	    #--------------------------------------------------------------------
	    AS_IF([test "$GCC" = yes],[use_sunmath=no],[
		arch=`isainfo`
		AC_MSG_CHECKING([whether to use -lsunmath for fp rounding control])
		AS_IF([test "$arch" = "amd64 i386" -o "$arch" = "i386"], [
			AC_MSG_RESULT([yes])
			MATH_LIBS="-lsunmath $MATH_LIBS"
			AC_CHECK_HEADER(sunmath.h)
			use_sunmath=yes
			], [
			AC_MSG_RESULT([no])
			use_sunmath=no
		])
	    ])
	    SHLIB_SUFFIX=".so"
	    DL_OBJS="tclLoadDl.o"
	    DL_LIBS="-ldl"
	    AS_IF([test "$GCC" = yes], [
		SHLIB_LD='${CC} -shared'
		CC_SEARCH_FLAGS='-Wl,-R,${LIB_RUNTIME_DIR}'
		LD_SEARCH_FLAGS=${CC_SEARCH_FLAGS}
		AS_IF([test "$do64bit_ok" = yes], [
		    AS_IF([test "$arch" = "sparcv9 sparc"], [
			# We need to specify -static-libgcc or we need to
			# add the path to the sparv9 libgcc.
			SHLIB_LD="$SHLIB_LD -m64 -mcpu=v9 -static-libgcc"
			# for finding sparcv9 libgcc, get the regular libgcc
			# path, remove so name and append 'sparcv9'
			#v9gcclibdir="`gcc -print-file-name=libgcc_s.so` | ..."
			#CC_SEARCH_FLAGS="${CC_SEARCH_FLAGS},-R,$v9gcclibdir"
		    ], [AS_IF([test "$arch" = "amd64 i386"], [
			SHLIB_LD="$SHLIB_LD -m64 -static-libgcc"
		    ])])
		])
	    ], [
		AS_IF([test "$use_sunmath" = yes], [textmode=textoff],[textmode=text])
		case $system in
		    SunOS-5.[[1-9]][[0-9]]*|SunOS-5.[[7-9]])
			SHLIB_LD="\${CC} -G -z $textmode \${LDFLAGS}";;
		    *)
			SHLIB_LD="/usr/ccs/bin/ld -G -z $textmode";;
		esac
		CC_SEARCH_FLAGS='-Wl,-R,${LIB_RUNTIME_DIR}'
		LD_SEARCH_FLAGS='-R ${LIB_RUNTIME_DIR}'
	    ])
	    ;;
	UNIX_SV* | UnixWare-5*)
	    SHLIB_CFLAGS="-KPIC"
	    SHLIB_LD='${CC} -G'
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
	    AS_IF([test $tcl_cv_ld_Bexport = yes], [
		LDFLAGS="$LDFLAGS -Wl,-Bexport"
	    ])
	    CC_SEARCH_FLAGS=""
	    LD_SEARCH_FLAGS=""
	    ;;
    esac

    AS_IF([test "$do64bit" = yes -a "$do64bit_ok" = no], [
	AC_MSG_WARN([64bit support being disabled -- don't know magic for this platform])
    ])

    AS_IF([test "$do64bit" = yes -a "$do64bit_ok" = yes], [
	AC_DEFINE(TCL_CFG_DO64BIT, 1, [Is this a 64-bit build?])
    ])

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
    AS_IF([test "$tcl_ok" = no], [DL_OBJS=""])

    AS_IF([test "x$DL_OBJS" != x], [BUILD_DLTEST="\$(DLTEST_TARGETS)"], [
	AC_MSG_WARN([Can't figure out how to do dynamic loading or shared libraries on this system.])
	SHLIB_CFLAGS=""
	SHLIB_LD=""
	SHLIB_SUFFIX=""
	DL_OBJS="tclLoadNone.o"
	DL_LIBS=""
	LDFLAGS="$LDFLAGS_ORIG"
	CC_SEARCH_FLAGS=""
	LD_SEARCH_FLAGS=""
	BUILD_DLTEST=""
    ])
    LDFLAGS="$LDFLAGS $LDFLAGS_ARCH"

    # If we're running gcc, then change the C flags for compiling shared
    # libraries to the right flags for gcc, instead of those for the
    # standard manufacturer compiler.

    AS_IF([test "$DL_OBJS" != "tclLoadNone.o" -a "$GCC" = yes], [
	case $system in
	    AIX-*) ;;
	    BSD/OS*) ;;
	    CYGWIN_*|MINGW32_*) ;;
	    IRIX*) ;;
	    NetBSD-*|DragonFly-*|FreeBSD-*|OpenBSD-*) ;;
	    Darwin-*) ;;
	    SCO_SV-3.2*) ;;
	    *) SHLIB_CFLAGS="-fPIC" ;;
	esac])

    AS_IF([test "$tcl_cv_cc_visibility_hidden" != yes], [
	AC_DEFINE(MODULE_SCOPE, [extern],
	    [No Compiler support for module scope symbols])
    ])

    AS_IF([test "$SHARED_LIB_SUFFIX" = ""], [
	SHARED_LIB_SUFFIX='${VERSION}${SHLIB_SUFFIX}'])
    AS_IF([test "$UNSHARED_LIB_SUFFIX" = ""], [
	UNSHARED_LIB_SUFFIX='${VERSION}.a'])
    DLL_INSTALL_DIR="\$(LIB_INSTALL_DIR)"

    AS_IF([test "${SHARED_BUILD}" = 1 -a "${SHLIB_SUFFIX}" != ""], [
        LIB_SUFFIX=${SHARED_LIB_SUFFIX}
        MAKE_LIB='${SHLIB_LD} -o [$]@ ${OBJS} ${SHLIB_LD_LIBS} ${TCL_SHLIB_LD_EXTRAS} ${TK_SHLIB_LD_EXTRAS} ${LD_SEARCH_FLAGS}'
        AS_IF([test "${SHLIB_SUFFIX}" = ".dll"], [
            INSTALL_LIB='$(INSTALL_LIBRARY) $(LIB_FILE) "$(BIN_INSTALL_DIR)/$(LIB_FILE)";if test -f $(LIB_FILE).a; then $(INSTALL_DATA) $(LIB_FILE).a "$(LIB_INSTALL_DIR)"; fi;'
            DLL_INSTALL_DIR="\$(BIN_INSTALL_DIR)"
        ], [
            INSTALL_LIB='$(INSTALL_LIBRARY) $(LIB_FILE) "$(LIB_INSTALL_DIR)/$(LIB_FILE)"'
        ])
    ], [
        LIB_SUFFIX=${UNSHARED_LIB_SUFFIX}

        AS_IF([test "$RANLIB" = ""], [
            MAKE_LIB='$(STLIB_LD) [$]@ ${OBJS}'
        ], [
            MAKE_LIB='${STLIB_LD} [$]@ ${OBJS} ; ${RANLIB} [$]@'
        ])
        INSTALL_LIB='$(INSTALL_LIBRARY) $(LIB_FILE) "$(LIB_INSTALL_DIR)/$(LIB_FILE)"'
    ])

    # Stub lib does not depend on shared/static configuration
    AS_IF([test "$RANLIB" = ""], [
        MAKE_STUB_LIB='${STLIB_LD} [$]@ ${STUB_LIB_OBJS}'
    ], [
        MAKE_STUB_LIB='${STLIB_LD} [$]@ ${STUB_LIB_OBJS} ; ${RANLIB} [$]@'
    ])
    INSTALL_STUB_LIB='$(INSTALL_LIBRARY) $(STUB_LIB_FILE) "$(LIB_INSTALL_DIR)/$(STUB_LIB_FILE)"'

    # Define TCL_LIBS now that we know what DL_LIBS is.
    # The trick here is that we don't want to change the value of TCL_LIBS if
    # it is already set when tclConfig.sh had been loaded by Tk.
    AS_IF([test "x${TCL_LIBS}" = x], [
        TCL_LIBS="${DL_LIBS} ${LIBS} ${MATH_LIBS}"])
    AC_SUBST(TCL_LIBS)

	# See if the compiler supports casting to a union type.
	# This is used to stop gcc from printing a compiler
	# warning when initializing a union member.

	AC_CACHE_CHECK(for cast to union support,
	    tcl_cv_cast_to_union,
	    AC_TRY_COMPILE([],
	    [
		  union foo { int i; double d; };
		  union foo f = (union foo) (int) 0;
	    ],
	    tcl_cv_cast_to_union=yes,
	    tcl_cv_cast_to_union=no)
	)
	if test "$tcl_cv_cast_to_union" = "yes"; then
	    AC_DEFINE(HAVE_CAST_TO_UNION, 1,
		    [Defined when compiler supports casting to union type.])
	fi

    # FIXME: This subst was left in only because the TCL_DL_LIBS
    # entry in tclConfig.sh uses it. It is not clear why someone
    # would use TCL_DL_LIBS instead of TCL_LIBS.
    AC_SUBST(DL_LIBS)

    AC_SUBST(DL_OBJS)
    AC_SUBST(PLAT_OBJS)
    AC_SUBST(PLAT_SRCS)
    AC_SUBST(LDAIX_SRC)
    AC_SUBST(CFLAGS)
    AC_SUBST(CFLAGS_DEBUG)
    AC_SUBST(CFLAGS_OPTIMIZE)
    AC_SUBST(CFLAGS_WARNING)

    AC_SUBST(LDFLAGS)
    AC_SUBST(LDFLAGS_DEBUG)
    AC_SUBST(LDFLAGS_OPTIMIZE)
    AC_SUBST(CC_SEARCH_FLAGS)
    AC_SUBST(LD_SEARCH_FLAGS)

    AC_SUBST(STLIB_LD)
    AC_SUBST(SHLIB_LD)
    AC_SUBST(TCL_SHLIB_LD_EXTRAS)
    AC_SUBST(TK_SHLIB_LD_EXTRAS)
    AC_SUBST(SHLIB_LD_LIBS)
    AC_SUBST(SHLIB_CFLAGS)
    AC_SUBST(SHLIB_SUFFIX)
    AC_DEFINE_UNQUOTED(TCL_SHLIB_EXT,"${SHLIB_SUFFIX}",
	[What is the default extension for shared libraries?])

    AC_SUBST(MAKE_LIB)
    AC_SUBST(MAKE_STUB_LIB)
    AC_SUBST(INSTALL_LIB)
    AC_SUBST(DLL_INSTALL_DIR)
    AC_SUBST(INSTALL_STUB_LIB)
    AC_SUBST(RANLIB)
])

#--------------------------------------------------------------------
# SC_MISSING_POSIX_HEADERS
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
#		NO_VALUES_H
#		NO_STDLIB_H
#		NO_STRING_H
#		NO_SYS_WAIT_H
#		NO_DLFCN_H
#		HAVE_SYS_PARAM_H
#
#		HAVE_STRING_H ?
#
#--------------------------------------------------------------------

AC_DEFUN([SC_MISSING_POSIX_HEADERS], [
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

    AC_CHECK_HEADER(float.h, , [AC_DEFINE(NO_FLOAT_H, 1, [Do we have <float.h>?])])
    AC_CHECK_HEADER(values.h, , [AC_DEFINE(NO_VALUES_H, 1, [Do we have <values.h>?])])
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
# SC_PATH_X
#
#	Locate the X11 header files and the X11 library archive.  Try
#	the ac_path_x macro first, but if it doesn't find the X stuff
#	(e.g. because there's no xmkmf program) then check through
#	a list of possible directories.  Under some conditions the
#	autoconf macro will return an include directory that contains
#	no include files, so double-check its result just to be safe.
#
# Arguments:
#	none
#
# Results:
#
#	Sets the following vars:
#		XINCLUDES
#		XLIBSW
#
#--------------------------------------------------------------------

AC_DEFUN([SC_PATH_X], [
    AC_PATH_X
    not_really_there=""
    if test "$no_x" = ""; then
	if test "$x_includes" = ""; then
	    AC_TRY_CPP([#include <X11/Xlib.h>], , not_really_there="yes")
	else
	    if test ! -r $x_includes/X11/Xlib.h; then
		not_really_there="yes"
	    fi
	fi
    fi
    if test "$no_x" = "yes" -o "$not_really_there" = "yes"; then
	AC_MSG_CHECKING([for X11 header files])
	found_xincludes="no"
	AC_TRY_CPP([#include <X11/Xlib.h>], found_xincludes="yes", found_xincludes="no")
	if test "$found_xincludes" = "no"; then
	    dirs="/usr/unsupported/include /usr/local/include /usr/X386/include /usr/X11R6/include /usr/X11R5/include /usr/include/X11R5 /usr/include/X11R4 /usr/openwin/include /usr/X11/include /usr/sww/include"
	    for i in $dirs ; do
		if test -r $i/X11/Xlib.h; then
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
    if test "$found_xincludes" = "no"; then
	AC_MSG_RESULT([couldn't find any!])
    fi

    if test "$no_x" = yes; then
	AC_MSG_CHECKING([for X11 libraries])
	XLIBSW=nope
	dirs="/usr/unsupported/lib /usr/local/lib /usr/X386/lib /usr/X11R6/lib /usr/X11R5/lib /usr/lib/X11R5 /usr/lib/X11R4 /usr/openwin/lib /usr/X11/lib /usr/sww/X11/lib"
	for i in $dirs ; do
	    if test -r $i/libX11.a -o -r $i/libX11.so -o -r $i/libX11.sl -o -r $i/libX11.dylib; then
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
])

#--------------------------------------------------------------------
# SC_BLOCKING_STYLE
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

AC_DEFUN([SC_BLOCKING_STYLE], [
    AC_CHECK_HEADERS(sys/ioctl.h)
    AC_CHECK_HEADERS(sys/filio.h)
    SC_CONFIG_SYSTEM
    AC_MSG_CHECKING([FIONBIO vs. O_NONBLOCK for nonblocking I/O])
    case $system in
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
# SC_TIME_HANLDER
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

AC_DEFUN([SC_TIME_HANDLER], [
    AC_CHECK_HEADERS(sys/time.h)
    AC_HEADER_TIME

    AC_CHECK_FUNCS(gmtime_r localtime_r mktime)

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
# SC_TCL_LINK_LIBS
#
#	Search for the libraries needed to link the Tcl shell.
#	Things like the math library (-lm) and socket stuff (-lsocket vs.
#	-lnsl) are dealt with here.
#
# Arguments:
#	None.
#
# Results:
#
#	Might append to the following vars:
#		LIBS
#		MATH_LIBS
#
#	Might define the following vars:
#		HAVE_NET_ERRNO_H
#
#--------------------------------------------------------------------

AC_DEFUN([SC_TCL_LINK_LIBS], [
    #--------------------------------------------------------------------
    # On a few very rare systems, all of the libm.a stuff is
    # already in libc.a.  Set compiler flags accordingly.
    #--------------------------------------------------------------------

    AC_CHECK_FUNC(sin, MATH_LIBS="", MATH_LIBS="-lm")

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
])

#--------------------------------------------------------------------
# SC_TCL_EARLY_FLAGS
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

AC_DEFUN([SC_TCL_EARLY_FLAG],[
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

AC_DEFUN([SC_TCL_EARLY_FLAGS],[
    AC_MSG_CHECKING([for required early compiler flags])
    tcl_flags=""
    SC_TCL_EARLY_FLAG(_ISOC99_SOURCE,[#include <stdlib.h>],
	[char *p = (char *)strtoll; char *q = (char *)strtoull;])
    SC_TCL_EARLY_FLAG(_LARGEFILE64_SOURCE,[#include <sys/stat.h>],
	[struct stat64 buf; int i = stat64("/", &buf);])
    SC_TCL_EARLY_FLAG(_LARGEFILE_SOURCE64,[#include <sys/stat.h>],
	[char *p = (char *)open64;])
    if test "x${tcl_flags}" = "x" ; then
	AC_MSG_RESULT([none])
    else
	AC_MSG_RESULT([${tcl_flags}])
    fi
])

#--------------------------------------------------------------------
# SC_TCL_64BIT_FLAGS
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
#		HAVE_STRUCT_DIRENT64, HAVE_DIR64
#		HAVE_STRUCT_STAT64
#		HAVE_TYPE_OFF64_T
#
#--------------------------------------------------------------------

AC_DEFUN([SC_TCL_64BIT_FLAGS], [
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
    else
	AC_DEFINE_UNQUOTED(TCL_WIDE_INT_TYPE,${tcl_cv_type_64bit},
	    [What type should be used to define wide integers?])
	AC_MSG_RESULT([${tcl_cv_type_64bit}])

	# Now check for auxiliary declarations
	AC_CACHE_CHECK([for struct dirent64], tcl_cv_struct_dirent64,[
	    AC_TRY_COMPILE([#include <sys/types.h>
#include <dirent.h>],[struct dirent64 p;],
		tcl_cv_struct_dirent64=yes,tcl_cv_struct_dirent64=no)])
	if test "x${tcl_cv_struct_dirent64}" = "xyes" ; then
	    AC_DEFINE(HAVE_STRUCT_DIRENT64, 1, [Is 'struct dirent64' in <sys/types.h>?])
	fi

	AC_CACHE_CHECK([for DIR64], tcl_cv_DIR64,[
	    AC_TRY_COMPILE([#include <sys/types.h>
#include <dirent.h>],[struct dirent64 *p; DIR64 d = opendir64(".");
            p = readdir64(d); rewinddir64(d); closedir64(d);],
		tcl_cv_DIR64=yes,tcl_cv_DIR64=no)])
	if test "x${tcl_cv_DIR64}" = "xyes" ; then
	    AC_DEFINE(HAVE_DIR64, 1, [Is 'DIR64' in <sys/types.h>?])
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

#--------------------------------------------------------------------
# SC_TCL_CFG_ENCODING	TIP #59
#
#	Declare the encoding to use for embedded configuration information.
#
# Arguments:
#	None.
#
# Results:
#	Might append to the following vars:
#		DEFS	(implicit)
#
#	Will define the following vars:
#		TCL_CFGVAL_ENCODING
#
#--------------------------------------------------------------------

AC_DEFUN([SC_TCL_CFG_ENCODING], [
    AC_ARG_WITH(encoding,
	AC_HELP_STRING([--with-encoding],
	    [encoding for configuration values (default: iso8859-1)]),
	with_tcencoding=${withval})

    if test x"${with_tcencoding}" != x ; then
	AC_DEFINE_UNQUOTED(TCL_CFGVAL_ENCODING,"${with_tcencoding}",
	    [What encoding should be used for embedded configuration info?])
    else
	AC_DEFINE(TCL_CFGVAL_ENCODING,"iso8859-1",
	    [What encoding should be used for embedded configuration info?])
    fi
])

#--------------------------------------------------------------------
# SC_TCL_CHECK_BROKEN_FUNC
#
#	Check for broken function.
#
# Arguments:
#	funcName - function to test for
#	advancedTest - the advanced test to run if the function is present
#
# Results:
#	Might cause compatibility versions of the function to be used.
#	Might affect the following vars:
#		USE_COMPAT	(implicit)
#
#--------------------------------------------------------------------

AC_DEFUN([SC_TCL_CHECK_BROKEN_FUNC],[
    AC_CHECK_FUNC($1, tcl_ok=1, tcl_ok=0)
    if test ["$tcl_ok"] = 1; then
	AC_CACHE_CHECK([proper ]$1[ implementation], [tcl_cv_]$1[_unbroken],
	    AC_TRY_RUN([[int main() {]$2[}]],[tcl_cv_]$1[_unbroken]=ok,
		[tcl_cv_]$1[_unbroken]=broken,[tcl_cv_]$1[_unbroken]=unknown))
	if test ["$tcl_cv_]$1[_unbroken"] = "ok"; then
	    tcl_ok=1
	else
	    tcl_ok=0
	fi
    fi
    if test ["$tcl_ok"] = 0; then
	AC_LIBOBJ($1)
	USE_COMPAT=1
    fi
])

#--------------------------------------------------------------------
# SC_TCL_GETHOSTBYADDR_R
#
#	Check if we have MT-safe variant of gethostbyaddr().
#
# Arguments:
#	None
#
# Results:
#
#	Might define the following vars:
#		HAVE_GETHOSTBYADDR_R
#		HAVE_GETHOSTBYADDR_R_7
#		HAVE_GETHOSTBYADDR_R_8
#
#--------------------------------------------------------------------

AC_DEFUN([SC_TCL_GETHOSTBYADDR_R], [
    # Avoids picking hidden internal symbol from libc
    SC_TCL_GETHOSTBYADDR_R_DECL

    if test "$tcl_cv_api_gethostbyaddr_r" = yes; then
	SC_TCL_GETHOSTBYADDR_R_TYPE
    fi
])

AC_DEFUN([SC_TCL_GETHOSTBYADDR_R_DECL], [AC_CHECK_DECLS(gethostbyaddr_r, [
    tcl_cv_api_gethostbyaddr_r=yes],[tcl_cv_api_gethostbyaddr_r=no],[#include <netdb.h>])
])

AC_DEFUN([SC_TCL_GETHOSTBYADDR_R_TYPE], [AC_CHECK_FUNC(gethostbyaddr_r, [
    AC_CACHE_CHECK([for gethostbyaddr_r with 7 args], tcl_cv_api_gethostbyaddr_r_7, [
    AC_TRY_COMPILE([
	#include <netdb.h>
    ], [
	char *addr;
	int length;
	int type;
	struct hostent *result;
	char buffer[2048];
	int buflen = 2048;
	int h_errnop;

	(void) gethostbyaddr_r(addr, length, type, result, buffer, buflen,
			       &h_errnop);
    ], tcl_cv_api_gethostbyaddr_r_7=yes, tcl_cv_api_gethostbyaddr_r_7=no)])
    tcl_ok=$tcl_cv_api_gethostbyaddr_r_7
    if test "$tcl_ok" = yes; then
	AC_DEFINE(HAVE_GETHOSTBYADDR_R_7, 1,
	    [Define to 1 if gethostbyaddr_r takes 7 args.])
    else
	AC_CACHE_CHECK([for gethostbyaddr_r with 8 args], tcl_cv_api_gethostbyaddr_r_8, [
	AC_TRY_COMPILE([
	    #include <netdb.h>
	], [
	    char *addr;
	    int length;
	    int type;
	    struct hostent *result, *resultp;
	    char buffer[2048];
	    int buflen = 2048;
	    int h_errnop;

	    (void) gethostbyaddr_r(addr, length, type, result, buffer, buflen,
				   &resultp, &h_errnop);
	], tcl_cv_api_gethostbyaddr_r_8=yes, tcl_cv_api_gethostbyaddr_r_8=no)])
	tcl_ok=$tcl_cv_api_gethostbyaddr_r_8
	if test "$tcl_ok" = yes; then
	    AC_DEFINE(HAVE_GETHOSTBYADDR_R_8, 1,
		[Define to 1 if gethostbyaddr_r takes 8 args.])
	fi
    fi
    if test "$tcl_ok" = yes; then
	AC_DEFINE(HAVE_GETHOSTBYADDR_R, 1,
	    [Define to 1 if gethostbyaddr_r is available.])
    fi
])])

#--------------------------------------------------------------------
# SC_TCL_GETHOSTBYNAME_R
#
#	Check to see what variant of gethostbyname_r() we have.
#	Based on David Arnold's example from the comp.programming.threads
#	FAQ Q213
#
# Arguments:
#	None
#
# Results:
#
#	Might define the following vars:
#		HAVE_GETHOSTBYNAME_R
#		HAVE_GETHOSTBYNAME_R_3
#		HAVE_GETHOSTBYNAME_R_5
#		HAVE_GETHOSTBYNAME_R_6
#
#--------------------------------------------------------------------

AC_DEFUN([SC_TCL_GETHOSTBYNAME_R], [
    # Avoids picking hidden internal symbol from libc
    SC_TCL_GETHOSTBYNAME_R_DECL

    if test "$tcl_cv_api_gethostbyname_r" = yes; then
	SC_TCL_GETHOSTBYNAME_R_TYPE
    fi
])

AC_DEFUN([SC_TCL_GETHOSTBYNAME_R_DECL], [AC_CHECK_DECLS(gethostbyname_r, [
    tcl_cv_api_gethostbyname_r=yes],[tcl_cv_api_gethostbyname_r=no],[#include <netdb.h>])
])

AC_DEFUN([SC_TCL_GETHOSTBYNAME_R_TYPE], [AC_CHECK_FUNC(gethostbyname_r, [
    AC_CACHE_CHECK([for gethostbyname_r with 6 args], tcl_cv_api_gethostbyname_r_6, [
    AC_TRY_COMPILE([
	#include <netdb.h>
    ], [
	char *name;
	struct hostent *he, *res;
	char buffer[2048];
	int buflen = 2048;
	int h_errnop;

	(void) gethostbyname_r(name, he, buffer, buflen, &res, &h_errnop);
    ], tcl_cv_api_gethostbyname_r_6=yes, tcl_cv_api_gethostbyname_r_6=no)])
    tcl_ok=$tcl_cv_api_gethostbyname_r_6
    if test "$tcl_ok" = yes; then
	AC_DEFINE(HAVE_GETHOSTBYNAME_R_6, 1,
	    [Define to 1 if gethostbyname_r takes 6 args.])
    else
	AC_CACHE_CHECK([for gethostbyname_r with 5 args], tcl_cv_api_gethostbyname_r_5, [
	AC_TRY_COMPILE([
	    #include <netdb.h>
	], [
	    char *name;
	    struct hostent *he;
	    char buffer[2048];
	    int buflen = 2048;
	    int h_errnop;

	    (void) gethostbyname_r(name, he, buffer, buflen, &h_errnop);
	], tcl_cv_api_gethostbyname_r_5=yes, tcl_cv_api_gethostbyname_r_5=no)])
	tcl_ok=$tcl_cv_api_gethostbyname_r_5
	if test "$tcl_ok" = yes; then
	    AC_DEFINE(HAVE_GETHOSTBYNAME_R_5, 1,
		[Define to 1 if gethostbyname_r takes 5 args.])
	else
	    AC_CACHE_CHECK([for gethostbyname_r with 3 args], tcl_cv_api_gethostbyname_r_3, [
	    AC_TRY_COMPILE([
		#include <netdb.h>
	    ], [
		char *name;
		struct hostent *he;
		struct hostent_data data;

		(void) gethostbyname_r(name, he, &data);
	    ], tcl_cv_api_gethostbyname_r_3=yes, tcl_cv_api_gethostbyname_r_3=no)])
	    tcl_ok=$tcl_cv_api_gethostbyname_r_3
	    if test "$tcl_ok" = yes; then
		AC_DEFINE(HAVE_GETHOSTBYNAME_R_3, 1,
		    [Define to 1 if gethostbyname_r takes 3 args.])
	    fi
	fi
    fi
    if test "$tcl_ok" = yes; then
	AC_DEFINE(HAVE_GETHOSTBYNAME_R, 1,
	    [Define to 1 if gethostbyname_r is available.])
    fi
])])

#--------------------------------------------------------------------
# SC_TCL_GETPWUID_R
#
#	Check if we have MT-safe variant of getpwuid() and if yes,
#	which one exactly.
#
# Arguments:
#	None
#
# Results:
#
#	Might define the following vars:
#		HAVE_GETPWUID_R
#		HAVE_GETPWUID_R_4
#		HAVE_GETPWUID_R_5
#
#--------------------------------------------------------------------

AC_DEFUN([SC_TCL_GETPWUID_R], [AC_CHECK_FUNC(getpwuid_r, [
    AC_CACHE_CHECK([for getpwuid_r with 5 args], tcl_cv_api_getpwuid_r_5, [
    AC_TRY_COMPILE([
	#include <sys/types.h>
	#include <pwd.h>
    ], [
	uid_t uid;
	struct passwd pw, *pwp;
	char buf[512];
	int buflen = 512;

	(void) getpwuid_r(uid, &pw, buf, buflen, &pwp);
    ], tcl_cv_api_getpwuid_r_5=yes, tcl_cv_api_getpwuid_r_5=no)])
    tcl_ok=$tcl_cv_api_getpwuid_r_5
    if test "$tcl_ok" = yes; then
	AC_DEFINE(HAVE_GETPWUID_R_5, 1,
	    [Define to 1 if getpwuid_r takes 5 args.])
    else
	AC_CACHE_CHECK([for getpwuid_r with 4 args], tcl_cv_api_getpwuid_r_4, [
	AC_TRY_COMPILE([
	    #include <sys/types.h>
	    #include <pwd.h>
	], [
	    uid_t uid;
	    struct passwd pw;
	    char buf[512];
	    int buflen = 512;

	    (void)getpwnam_r(uid, &pw, buf, buflen);
	], tcl_cv_api_getpwuid_r_4=yes, tcl_cv_api_getpwuid_r_4=no)])
	tcl_ok=$tcl_cv_api_getpwuid_r_4
	if test "$tcl_ok" = yes; then
	    AC_DEFINE(HAVE_GETPWUID_R_4, 1,
		[Define to 1 if getpwuid_r takes 4 args.])
	fi
    fi
    if test "$tcl_ok" = yes; then
	AC_DEFINE(HAVE_GETPWUID_R, 1,
	    [Define to 1 if getpwuid_r is available.])
    fi
])])

#--------------------------------------------------------------------
# SC_TCL_GETPWNAM_R
#
#	Check if we have MT-safe variant of getpwnam() and if yes,
#	which one exactly.
#
# Arguments:
#	None
#
# Results:
#
#	Might define the following vars:
#		HAVE_GETPWNAM_R
#		HAVE_GETPWNAM_R_4
#		HAVE_GETPWNAM_R_5
#
#--------------------------------------------------------------------

AC_DEFUN([SC_TCL_GETPWNAM_R], [AC_CHECK_FUNC(getpwnam_r, [
    AC_CACHE_CHECK([for getpwnam_r with 5 args], tcl_cv_api_getpwnam_r_5, [
    AC_TRY_COMPILE([
	#include <sys/types.h>
	#include <pwd.h>
    ], [
	char *name;
	struct passwd pw, *pwp;
	char buf[512];
	int buflen = 512;

	(void) getpwnam_r(name, &pw, buf, buflen, &pwp);
    ], tcl_cv_api_getpwnam_r_5=yes, tcl_cv_api_getpwnam_r_5=no)])
    tcl_ok=$tcl_cv_api_getpwnam_r_5
    if test "$tcl_ok" = yes; then
	AC_DEFINE(HAVE_GETPWNAM_R_5, 1,
	    [Define to 1 if getpwnam_r takes 5 args.])
    else
	AC_CACHE_CHECK([for getpwnam_r with 4 args], tcl_cv_api_getpwnam_r_4, [
	AC_TRY_COMPILE([
	    #include <sys/types.h>
	    #include <pwd.h>
	], [
	    char *name;
	    struct passwd pw;
	    char buf[512];
	    int buflen = 512;

	    (void)getpwnam_r(name, &pw, buf, buflen);
	], tcl_cv_api_getpwnam_r_4=yes, tcl_cv_api_getpwnam_r_4=no)])
	tcl_ok=$tcl_cv_api_getpwnam_r_4
	if test "$tcl_ok" = yes; then
	    AC_DEFINE(HAVE_GETPWNAM_R_4, 1,
		[Define to 1 if getpwnam_r takes 4 args.])
	fi
    fi
    if test "$tcl_ok" = yes; then
	AC_DEFINE(HAVE_GETPWNAM_R, 1,
	    [Define to 1 if getpwnam_r is available.])
    fi
])])

#--------------------------------------------------------------------
# SC_TCL_GETGRGID_R
#
#	Check if we have MT-safe variant of getgrgid() and if yes,
#	which one exactly.
#
# Arguments:
#	None
#
# Results:
#
#	Might define the following vars:
#		HAVE_GETGRGID_R
#		HAVE_GETGRGID_R_4
#		HAVE_GETGRGID_R_5
#
#--------------------------------------------------------------------

AC_DEFUN([SC_TCL_GETGRGID_R], [AC_CHECK_FUNC(getgrgid_r, [
    AC_CACHE_CHECK([for getgrgid_r with 5 args], tcl_cv_api_getgrgid_r_5, [
    AC_TRY_COMPILE([
	#include <sys/types.h>
	#include <grp.h>
    ], [
	gid_t gid;
	struct group gr, *grp;
	char buf[512];
	int buflen = 512;

	(void) getgrgid_r(gid, &gr, buf, buflen, &grp);
    ], tcl_cv_api_getgrgid_r_5=yes, tcl_cv_api_getgrgid_r_5=no)])
    tcl_ok=$tcl_cv_api_getgrgid_r_5
    if test "$tcl_ok" = yes; then
	AC_DEFINE(HAVE_GETGRGID_R_5, 1,
	    [Define to 1 if getgrgid_r takes 5 args.])
    else
	AC_CACHE_CHECK([for getgrgid_r with 4 args], tcl_cv_api_getgrgid_r_4, [
	AC_TRY_COMPILE([
	    #include <sys/types.h>
	    #include <grp.h>
	], [
	    gid_t gid;
	    struct group gr;
	    char buf[512];
	    int buflen = 512;

	    (void)getgrgid_r(gid, &gr, buf, buflen);
	], tcl_cv_api_getgrgid_r_4=yes, tcl_cv_api_getgrgid_r_4=no)])
	tcl_ok=$tcl_cv_api_getgrgid_r_4
	if test "$tcl_ok" = yes; then
	    AC_DEFINE(HAVE_GETGRGID_R_4, 1,
		[Define to 1 if getgrgid_r takes 4 args.])
	fi
    fi
    if test "$tcl_ok" = yes; then
	AC_DEFINE(HAVE_GETGRGID_R, 1,
	    [Define to 1 if getgrgid_r is available.])
    fi
])])

#--------------------------------------------------------------------
# SC_TCL_GETGRNAM_R
#
#	Check if we have MT-safe variant of getgrnam() and if yes,
#	which one exactly.
#
# Arguments:
#	None
#
# Results:
#
#	Might define the following vars:
#		HAVE_GETGRNAM_R
#		HAVE_GETGRNAM_R_4
#		HAVE_GETGRNAM_R_5
#
#--------------------------------------------------------------------

AC_DEFUN([SC_TCL_GETGRNAM_R], [AC_CHECK_FUNC(getgrnam_r, [
    AC_CACHE_CHECK([for getgrnam_r with 5 args], tcl_cv_api_getgrnam_r_5, [
    AC_TRY_COMPILE([
	#include <sys/types.h>
	#include <grp.h>
    ], [
	char *name;
	struct group gr, *grp;
	char buf[512];
	int buflen = 512;

	(void) getgrnam_r(name, &gr, buf, buflen, &grp);
    ], tcl_cv_api_getgrnam_r_5=yes, tcl_cv_api_getgrnam_r_5=no)])
    tcl_ok=$tcl_cv_api_getgrnam_r_5
    if test "$tcl_ok" = yes; then
	AC_DEFINE(HAVE_GETGRNAM_R_5, 1,
	    [Define to 1 if getgrnam_r takes 5 args.])
    else
	AC_CACHE_CHECK([for getgrnam_r with 4 args], tcl_cv_api_getgrnam_r_4, [
	AC_TRY_COMPILE([
	    #include <sys/types.h>
	    #include <grp.h>
	], [
	    char *name;
	    struct group gr;
	    char buf[512];
	    int buflen = 512;

	    (void)getgrnam_r(name, &gr, buf, buflen);
	], tcl_cv_api_getgrnam_r_4=yes, tcl_cv_api_getgrnam_r_4=no)])
	tcl_ok=$tcl_cv_api_getgrnam_r_4
	if test "$tcl_ok" = yes; then
	    AC_DEFINE(HAVE_GETGRNAM_R_4, 1,
		[Define to 1 if getgrnam_r takes 4 args.])
	fi
    fi
    if test "$tcl_ok" = yes; then
	AC_DEFINE(HAVE_GETGRNAM_R, 1,
	    [Define to 1 if getgrnam_r is available.])
    fi
])])

AC_DEFUN([SC_TCL_IPV6],[
	NEED_FAKE_RFC2553=0
	AC_CHECK_FUNCS(getnameinfo getaddrinfo freeaddrinfo gai_strerror,,[NEED_FAKE_RFC2553=1])
	AC_CHECK_TYPES([
		struct addrinfo,
		struct in6_addr,
		struct sockaddr_in6,
		struct sockaddr_storage],,[NEED_FAKE_RFC2553=1],[[
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
]])
if test "x$NEED_FAKE_RFC2553" = "x1"; then
   AC_DEFINE([NEED_FAKE_RFC2553], 1,
        [Use compat implementation of getaddrinfo() and friends])
   AC_LIBOBJ([fake-rfc2553])
   AC_CHECK_FUNC(strlcpy)
fi
])
# Local Variables:
# mode: autoconf
# End:
