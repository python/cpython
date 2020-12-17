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
		    if test -f "$i/win/tclConfig.sh" ; then
			ac_cv_c_tclconfig="`(cd $i/win; pwd)`"
			break
		    fi
		done
	    fi

	    # check in a few common install locations
	    if test x"${ac_cv_c_tclconfig}" = x ; then
		for i in `ls -d ${libdir} 2>/dev/null` \
			`ls -d ${exec_prefix}/lib 2>/dev/null` \
			`ls -d ${prefix}/lib 2>/dev/null` \
			`ls -d /cygdrive/c/Tcl/lib 2>/dev/null` \
			`ls -d /cygdrive/c/Progra~1/Tcl/lib 2>/dev/null` \
			`ls -d /c/Tcl/lib 2>/dev/null` \
			`ls -d /c/Progra~1/Tcl/lib 2>/dev/null` \
			`ls -d C:/Tcl/lib 2>/dev/null` \
			`ls -d C:/Progra~1/Tcl/lib 2>/dev/null` \
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
		    if test -f "$i/win/tclConfig.sh" ; then
		    ac_cv_c_tclconfig="`(cd $i/win; pwd)`"
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
		    if test -f "$i/win/tkConfig.sh" ; then
			ac_cv_c_tkconfig="`(cd $i/win; pwd)`"
			break
		    fi
		done
	    fi

	    # check in a few common install locations
	    if test x"${ac_cv_c_tkconfig}" = x ; then
		for i in `ls -d ${libdir} 2>/dev/null` \
			`ls -d ${exec_prefix}/lib 2>/dev/null` \
			`ls -d ${prefix}/lib 2>/dev/null` \
			`ls -d /cygdrive/c/Tcl/lib 2>/dev/null` \
			`ls -d /cygdrive/c/Progra~1/Tcl/lib 2>/dev/null` \
			`ls -d /c/Tcl/lib 2>/dev/null` \
			`ls -d /c/Progra~1/Tcl/lib 2>/dev/null` \
			`ls -d C:/Tcl/lib 2>/dev/null` \
			`ls -d C:/Progra~1/Tcl/lib 2>/dev/null` \
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
		    if test -f "$i/win/tkConfig.sh" ; then
			ac_cv_c_tkconfig="`(cd $i/win; pwd)`"
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
#	Load the tclConfig.sh file.
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
#
#------------------------------------------------------------------------

AC_DEFUN([SC_LOAD_TCLCONFIG], [
    AC_MSG_CHECKING([for existence of ${TCL_BIN_DIR}/tclConfig.sh])

    if test -f "${TCL_BIN_DIR}/tclConfig.sh" ; then
        AC_MSG_RESULT([loading])
	. "${TCL_BIN_DIR}/tclConfig.sh"
    else
        AC_MSG_RESULT([could not find ${TCL_BIN_DIR}/tclConfig.sh])
    fi

    #
    # If the TCL_BIN_DIR is the build directory (not the install directory),
    # then set the common variable name to the value of the build variables.
    # For example, the variable TCL_LIB_SPEC will be set to the value
    # of TCL_BUILD_LIB_SPEC. An extension should make use of TCL_LIB_SPEC
    # instead of TCL_BUILD_LIB_SPEC since it will work with both an
    # installed and uninstalled version of Tcl.
    #

    if test -f $TCL_BIN_DIR/Makefile ; then
        TCL_LIB_SPEC=${TCL_BUILD_LIB_SPEC}
        TCL_STUB_LIB_SPEC=${TCL_BUILD_STUB_LIB_SPEC}
        TCL_STUB_LIB_PATH=${TCL_BUILD_STUB_LIB_PATH}
    fi

    #
    # eval is required to do the TCL_DBGX substitution
    #

    eval "TCL_LIB_FILE=\"${TCL_LIB_FILE}\""
    eval "TCL_LIB_FLAG=\"${TCL_LIB_FLAG}\""
    eval "TCL_LIB_SPEC=\"${TCL_LIB_SPEC}\""

    eval "TCL_STUB_LIB_FILE=\"${TCL_STUB_LIB_FILE}\""
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

    AC_SUBST(TCL_DEFS)
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


    AC_SUBST(TK_BIN_DIR)
    AC_SUBST(TK_SRC_DIR)
    AC_SUBST(TK_LIB_FILE)
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
	[  --enable-shared         build and link with shared libraries (default: on)],
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
#		--enable-threads=yes|no
#
#	Defines the following vars:
#		TCL_THREADS
#------------------------------------------------------------------------

AC_DEFUN([SC_ENABLE_THREADS], [
    AC_MSG_CHECKING(for building with threads)
    AC_ARG_ENABLE(threads, [  --enable-threads        build with threads (default: on)],
	[tcl_ok=$enableval], [tcl_ok=yes])

    if test "$tcl_ok" = "yes"; then
	AC_MSG_RESULT([yes (default)])
	TCL_THREADS=1
	AC_DEFINE(TCL_THREADS)
	# USE_THREAD_ALLOC tells us to try the special thread-based
	# allocator that significantly reduces lock contention
	AC_DEFINE(USE_THREAD_ALLOC)
    else
	TCL_THREADS=0
	AC_MSG_RESULT(no)
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
#		DBGX		Debug library extension
#
#------------------------------------------------------------------------

AC_DEFUN([SC_ENABLE_SYMBOLS], [
    AC_MSG_CHECKING([for build with symbols])
    AC_ARG_ENABLE(symbols, [  --enable-symbols        build with debugging symbols (default: off)],    [tcl_ok=$enableval], [tcl_ok=no])
# FIXME: Currently, LDFLAGS_DEFAULT is not used, it should work like CFLAGS_DEFAULT.
    if test "$tcl_ok" = "no"; then
	CFLAGS_DEFAULT='$(CFLAGS_OPTIMIZE)'
	LDFLAGS_DEFAULT='$(LDFLAGS_OPTIMIZE)'
	DBGX=""
	AC_DEFINE(NDEBUG, 1, [Is no debugging enabled?])
	AC_MSG_RESULT([no])

	AC_DEFINE(TCL_CFG_OPTIMIZED)
    else
	CFLAGS_DEFAULT='$(CFLAGS_DEBUG)'
	LDFLAGS_DEFAULT='$(LDFLAGS_DEBUG)'
	DBGX=g
	if test "$tcl_ok" = "yes"; then
	    AC_MSG_RESULT([yes (standard debugging)])
	fi
    fi
    AC_SUBST(CFLAGS_DEFAULT)
    AC_SUBST(LDFLAGS_DEFAULT)

    if test "$tcl_ok" = "mem" -o "$tcl_ok" = "all"; then
	AC_DEFINE(TCL_MEM_DEBUG, 1, [Is memory debugging enabled?])
    fi

    if test "$tcl_ok" = "compile" -o "$tcl_ok" = "all"; then
	AC_DEFINE(TCL_COMPILE_DEBUG, 1, [Is bytecode debugging enabled?])
	AC_DEFINE(TCL_COMPILE_STATS, 1, [Are bytecode statistics enabled?])
    fi

    if test "$tcl_ok" != "yes" -a "$tcl_ok" != "no"; then
	if test "$tcl_ok" = "all"; then
	    AC_MSG_RESULT([enabled symbols mem compile debugging])
	else
	    AC_MSG_RESULT([enabled $tcl_ok debugging])
	fi
    fi
])

#--------------------------------------------------------------------
# SC_CONFIG_CFLAGS
#
#	Try to determine the proper flags to pass to the compiler
#	for building shared libraries and other such nonsense.
#
#	NOTE: The backslashes in quotes below are substituted twice
#	due to the fact that they are in a macro and then inlined
#	in the final configure script.
#
# Arguments:
#	none
#
# Results:
#
#	Can the following vars:
#		EXTRA_CFLAGS
#		CFLAGS_DEBUG
#		CFLAGS_OPTIMIZE
#		CFLAGS_WARNING
#		LDFLAGS_DEBUG
#		LDFLAGS_OPTIMIZE
#		LDFLAGS_CONSOLE
#		LDFLAGS_WINDOW
#		CC_OBJNAME
#		CC_EXENAME
#		CYGPATH
#		STLIB_LD
#		SHLIB_LD
#		SHLIB_LD_LIBS
#		LIBS
#		AR
#		RC
#		RES
#
#		MAKE_LIB
#		MAKE_STUB_LIB
#		MAKE_EXE
#		MAKE_DLL
#
#		LIBSUFFIX
#		LIBFLAGSUFFIX
#		LIBPREFIX
#		LIBRARIES
#		EXESUFFIX
#		DLLSUFFIX
#
#--------------------------------------------------------------------

AC_DEFUN([SC_CONFIG_CFLAGS], [

    # Step 0: Enable 64 bit support?

    AC_MSG_CHECKING([if 64bit support is requested])
    AC_ARG_ENABLE(64bit,[  --enable-64bit          enable 64bit support (where applicable)], [do64bit=$enableval], [do64bit=no])
    AC_MSG_RESULT($do64bit)

    # Cross-compiling options for Windows/CE builds

    AC_MSG_CHECKING([if Windows/CE build is requested])
    AC_ARG_ENABLE(wince,[  --enable-wince          enable Win/CE support (where applicable)], [doWince=$enableval], [doWince=no])
    AC_MSG_RESULT($doWince)

    AC_MSG_CHECKING([for Windows/CE celib directory])
    AC_ARG_WITH(celib,[  --with-celib=DIR        use Windows/CE support library from DIR],
	    CELIB_DIR=$withval, CELIB_DIR=NO_CELIB)
    AC_MSG_RESULT([$CELIB_DIR])

    # Set some defaults (may get changed below)
    EXTRA_CFLAGS=""
	AC_DEFINE(MODULE_SCOPE, [extern], [No need to mark inidividual symbols as hidden])

    AC_CHECK_PROG(CYGPATH, cygpath, cygpath -m, echo)

    SHLIB_SUFFIX=".dll"

    # MACHINE is IX86 for LINK, but this is used by the manifest,
    # which requires x86|amd64|ia64.
    MACHINE="X86"

    if test "$GCC" = "yes"; then

      AC_CACHE_CHECK(for cross-compile version of gcc,
	ac_cv_cross,
	AC_TRY_COMPILE([
	    #ifndef _WIN32
		#error cross-compiler
	    #endif
	], [],
	ac_cv_cross=no,
	ac_cv_cross=yes)
      )

      if test "$ac_cv_cross" = "yes"; then
	case "$do64bit" in
	    amd64|x64|yes)
		CC="x86_64-w64-mingw32-gcc"
		LD="x86_64-w64-mingw32-ld"
		AR="x86_64-w64-mingw32-ar"
		RANLIB="x86_64-w64-mingw32-ranlib"
		RC="x86_64-w64-mingw32-windres"
	    ;;
	    *)
		CC="i686-w64-mingw32-gcc"
		LD="i686-w64-mingw32-ld"
		AR="i686-w64-mingw32-ar"
		RANLIB="i686-w64-mingw32-ranlib"
		RC="i686-w64-mingw32-windres"
	    ;;
	esac
      fi
    fi

    # Check for a bug in gcc's windres that causes the
    # compile to fail when a Windows native path is
    # passed into windres. The mingw toolchain requires
    # Windows native paths while Cygwin should work
    # with both. Avoid the bug by passing a POSIX
    # path when using the Cygwin toolchain.

    if test "$GCC" = "yes" && test "$CYGPATH" != "echo" ; then
	conftest=/tmp/conftest.rc
	echo "STRINGTABLE BEGIN" > $conftest
	echo "101 \"name\"" >> $conftest
	echo "END" >> $conftest

	AC_MSG_CHECKING([for Windows native path bug in windres])
	cyg_conftest=`$CYGPATH $conftest`
	if AC_TRY_COMMAND($RC -o conftest.res.o $cyg_conftest) ; then
	    AC_MSG_RESULT([no])
	else
	    AC_MSG_RESULT([yes])
	    CYGPATH=echo
	fi
	conftest=
	cyg_conftest=
    fi

    if test "$CYGPATH" = "echo"; then
        DEPARG='"$<"'
    else
        DEPARG='"$(shell $(CYGPATH) $<)"'
    fi

    # set various compiler flags depending on whether we are using gcc or cl

    if test "${GCC}" = "yes" ; then
	extra_cflags="-pipe"
	extra_ldflags="-pipe -static-libgcc"
	AC_CACHE_CHECK(for mingw32 version of gcc,
	    ac_cv_win32,
	    AC_TRY_COMPILE([
		#ifdef _WIN32
		    #error win32
		#endif
	    ], [],
	    ac_cv_win32=no,
	    ac_cv_win32=yes)
	)
	if test "$ac_cv_win32" != "yes"; then
	    AC_MSG_ERROR([${CC} cannot produce win32 executables.])
	fi

	hold_cflags=$CFLAGS; CFLAGS="$CFLAGS -mwindows -municode -Dmain=xxmain"
	AC_CACHE_CHECK(for working -municode linker flag,
	    ac_cv_municode,
	AC_TRY_LINK([
	#include <windows.h>
	int APIENTRY wWinMain(HINSTANCE a, HINSTANCE b, LPWSTR c, int d) {return 0;}
	],
	[],
	    ac_cv_municode=yes,
	    ac_cv_municode=no)
	)
	CFLAGS=$hold_cflags
	if test "$ac_cv_municode" = "yes" ; then
	    extra_ldflags="$extra_ldflags -municode"
	else
	    extra_cflags="$extra_cflags -DTCL_BROKEN_MAINARGS"
	fi
    fi

    AC_MSG_CHECKING([compiler flags])
    if test "${GCC}" = "yes" ; then
	SHLIB_LD=""
	SHLIB_LD_LIBS='${LIBS}'
	LIBS="-lnetapi32 -lkernel32 -luser32 -ladvapi32 -luserenv -lws2_32"
	# mingw needs to link ole32 and oleaut32 for [send], but MSVC doesn't
	LIBS_GUI="-lgdi32 -lcomdlg32 -limm32 -lcomctl32 -lshell32 -luuid -lole32 -loleaut32"
	STLIB_LD='${AR} cr'
	RC_OUT=-o
	RC_TYPE=
	RC_INCLUDE=--include
	RC_DEFINE=--define
	RES=res.o
	MAKE_LIB="\${STLIB_LD} \[$]@"
	MAKE_STUB_LIB="\${STLIB_LD} \[$]@"
	POST_MAKE_LIB="\${RANLIB} \[$]@"
	MAKE_EXE="\${CC} -o \[$]@"
	LIBPREFIX="lib"

	if test "${SHARED_BUILD}" = "0" ; then
	    # static
            AC_MSG_RESULT([using static flags])
	    runtime=
	    LIBRARIES="\${STATIC_LIBRARIES}"
	    EXESUFFIX="s\${DBGX}.exe"
	else
	    # dynamic
            AC_MSG_RESULT([using shared flags])

	    # ad-hoc check to see if CC supports -shared.
	    if "${CC}" -shared 2>&1 | egrep ': -shared not supported' >/dev/null; then
		AC_MSG_ERROR([${CC} does not support the -shared option.
                You will need to upgrade to a newer version of the toolchain.])
	    fi

	    runtime=
	    # Add SHLIB_LD_LIBS to the Make rule, not here.

	    EXESUFFIX="\${DBGX}.exe"
	    LIBRARIES="\${SHARED_LIBRARIES}"
	fi
	# Link with gcc since ld does not link to default libs like
	# -luser32 and -lmsvcrt by default.
	SHLIB_LD='${CC} -shared'
	SHLIB_LD_LIBS='${LIBS}'
	MAKE_DLL="\${SHLIB_LD} \$(LDFLAGS) -o \[$]@ ${extra_ldflags} \
	    -Wl,--out-implib,\$(patsubst %.dll,lib%.a,\[$]@)"
	# DLLSUFFIX is separate because it is the building block for
	# users of tclConfig.sh that may build shared or static.
	DLLSUFFIX="\${DBGX}.dll"
	LIBSUFFIX="\${DBGX}.a"
	LIBFLAGSUFFIX="\${DBGX}"
	SHLIB_SUFFIX=.dll

	EXTRA_CFLAGS="${extra_cflags}"

	CFLAGS_DEBUG=-g
	CFLAGS_OPTIMIZE="-O2 -fomit-frame-pointer"
	CFLAGS_WARNING="-Wall -Wdeclaration-after-statement"
	LDFLAGS_DEBUG=
	LDFLAGS_OPTIMIZE=

	# Specify the CC output file names based on the target name
	CC_OBJNAME="-o \[$]@"
	CC_EXENAME="-o \[$]@"

	# Specify linker flags depending on the type of app being
	# built -- Console vs. Window.
	#
	# ORIGINAL COMMENT:
	# We need to pass -e _WinMain@16 so that ld will use
	# WinMain() instead of main() as the entry point. We can't
	# use autoconf to check for this case since it would need
	# to run an executable and that does not work when
	# cross compiling. Remove this -e workaround once we
	# require a gcc that does not have this bug.
	#
	# MK NOTE: Tk should use a different mechanism. This causes
	# interesting problems, such as wish dying at startup.
	#LDFLAGS_WINDOW="-mwindows -e _WinMain@16 ${extra_ldflags}"
	LDFLAGS_CONSOLE="-mconsole ${extra_ldflags}"
	LDFLAGS_WINDOW="-mwindows ${extra_ldflags}"

	case "$do64bit" in
	    amd64|x64|yes)
		MACHINE="AMD64" ; # assume AMD64 as default 64-bit build
		AC_MSG_RESULT([   Using 64-bit $MACHINE mode])
		;;
	    ia64)
		MACHINE="IA64"
		AC_MSG_RESULT([   Using 64-bit $MACHINE mode])
		;;
	    *)
		AC_TRY_COMPILE([
		    #ifndef _WIN64
			#error 32-bit
		    #endif
		], [],
			tcl_win_64bit=yes,
			tcl_win_64bit=no
		)
		if test "$tcl_win_64bit" = "yes" ; then
			do64bit=amd64
			MACHINE="AMD64"
			AC_MSG_RESULT([   Using 64-bit $MACHINE mode])
		fi
		;;
	esac
    else
	if test "${SHARED_BUILD}" = "0" ; then
	    # static
            AC_MSG_RESULT([using static flags])
	    runtime=-MT
	    LIBRARIES="\${STATIC_LIBRARIES}"
	    EXESUFFIX="s\${DBGX}.exe"
	else
	    # dynamic
            AC_MSG_RESULT([using shared flags])
	    runtime=-MD
	    # Add SHLIB_LD_LIBS to the Make rule, not here.
	    LIBRARIES="\${SHARED_LIBRARIES}"
	    EXESUFFIX="\${DBGX}.exe"
	    case "x`echo \${VisualStudioVersion}`" in
		x1[[4-9]]*)
		    lflags="${lflags} -nodefaultlib:libucrt.lib"
		    ;;
		*)
		    ;;
	    esac
	fi
	MAKE_DLL="\${SHLIB_LD} \$(LDFLAGS) -out:\[$]@"
	# DLLSUFFIX is separate because it is the building block for
	# users of tclConfig.sh that may build shared or static.
	DLLSUFFIX="\${DBGX}.dll"
	LIBSUFFIX="\${DBGX}.lib"
	LIBFLAGSUFFIX="\${DBGX}"

	# This is a 2-stage check to make sure we have the 64-bit SDK
	# We have to know where the SDK is installed.
	# This magic is based on MS Platform SDK for Win2003 SP1 - hobbs
	if test "$do64bit" != "no" ; then
	    if test "x${MSSDK}x" = "xx" ; then
		MSSDK="C:/Progra~1/Microsoft Platform SDK"
	    fi
	    MSSDK=`echo "$MSSDK" | sed -e 's!\\\!/!g'`
	    PATH64=""
	    case "$do64bit" in
		amd64|x64|yes)
		    MACHINE="AMD64" ; # assume AMD64 as default 64-bit build
		    PATH64="${MSSDK}/Bin/Win64/x86/AMD64"
		    ;;
		ia64)
		    MACHINE="IA64"
		    PATH64="${MSSDK}/Bin/Win64"
		    ;;
	    esac
	    if test ! -d "${PATH64}" ; then
		AC_MSG_WARN([Could not find 64-bit $MACHINE SDK])
	    fi
	    AC_MSG_RESULT([   Using 64-bit $MACHINE mode])
	fi

	LIBS="netapi32.lib kernel32.lib user32.lib advapi32.lib userenv.lib ws2_32.lib"

	case "x`echo \${VisualStudioVersion}`" in
		x1[[4-9]]*)
		    LIBS="$LIBS ucrt.lib"
		    ;;
		*)
		    ;;
	esac

	if test "$do64bit" != "no" ; then
	    # The space-based-path will work for the Makefile, but will
	    # not work if AC_TRY_COMPILE is called.  TEA has the
	    # TEA_PATH_NOSPACE to avoid this issue.
	    # Check if _WIN64 is already recognized, and if so we don't
	    # need to modify CC.
	    AC_CHECK_DECL([_WIN64], [],
			  [CC="\"${PATH64}/cl.exe\" -I\"${MSSDK}/Include\" \
			 -I\"${MSSDK}/Include/crt\" \
			 -I\"${MSSDK}/Include/crt/sys\""])
	    RC="\"${MSSDK}/bin/rc.exe\""
	    CFLAGS_DEBUG="-nologo -Zi -Od ${runtime}d"
	    # Do not use -O2 for Win64 - this has proved buggy in code gen.
	    CFLAGS_OPTIMIZE="-nologo -O1 ${runtime}"
	    lflags="${lflags} -nologo -MACHINE:${MACHINE} -LIBPATH:\"${MSSDK}/Lib/${MACHINE}\""
	    LINKBIN="\"${PATH64}/link.exe\""
	    # Avoid 'unresolved external symbol __security_cookie' errors.
	    # c.f. http://support.microsoft.com/?id=894573
	    LIBS="$LIBS bufferoverflowU.lib"
	else
	    RC="rc"
	    # -Od - no optimization
	    # -WX - warnings as errors
	    CFLAGS_DEBUG="-nologo -Z7 -Od -WX ${runtime}d"
	    # -O2 - create fast code (/Og /Oi /Ot /Oy /Ob2 /Gs /GF /Gy)
	    CFLAGS_OPTIMIZE="-nologo -O2 ${runtime}"
	    lflags="${lflags} -nologo"
	    LINKBIN="link"
	fi

	if test "$doWince" != "no" ; then
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
		eval `echo $doWince | awk -F "," '{ \
	if (length([$]1)) { printf "CEVERSION=\"%s\"\n", [$]1; \
	if ([$]1 < 400)	  { printf "PLATFORM=\"Pocket PC 2002\"\n" } }; \
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
	    # The space-based-path will work for the Makefile, but will
	    # not work if AC_TRY_COMPILE is called.
	    WCEROOT=`echo "$WCEROOT" | sed -e 's!\\\!/!g'`
	    SDKROOT=`echo "$SDKROOT" | sed -e 's!\\\!/!g'`
	    CELIB_DIR=`echo "$CELIB_DIR" | sed -e 's!\\\!/!g'`
	    if test ! -d "${CELIB_DIR}/inc"; then
		AC_MSG_ERROR([Invalid celib directory "${CELIB_DIR}"])
	    fi
	    if test ! -d "${SDKROOT}/${OSVERSION}/${PLATFORM}/Lib/${TARGETCPU}"\
		-o ! -d "${WCEROOT}/EVC/${OSVERSION}/bin"; then
		AC_MSG_ERROR([could not find PocketPC SDK or target compiler to enable WinCE mode [$CEVERSION,$TARGETCPU,$ARCH,$PLATFORM]])
	    else
		CEINCLUDE="${SDKROOT}/${OSVERSION}/${PLATFORM}/include"
		if test -d "${CEINCLUDE}/${TARGETCPU}" ; then
		    CEINCLUDE="${CEINCLUDE}/${TARGETCPU}"
		fi
		CELIBPATH="${SDKROOT}/${OSVERSION}/${PLATFORM}/Lib/${TARGETCPU}"
	    fi
	fi

	if test "$doWince" != "no" ; then
	    CEBINROOT="${WCEROOT}/EVC/${OSVERSION}/bin"
	    if test "${TARGETCPU}" = "X86"; then
		CC="${CEBINROOT}/cl.exe"
	    else
		CC="${CEBINROOT}/cl${ARCH}.exe"
	    fi
	    CC="\"${CC}\" -I\"${CELIB_DIR}/inc\" -I\"${CEINCLUDE}\""
	    RC="\"${WCEROOT}/Common/EVC/bin/rc.exe\""
	    arch=`echo ${ARCH} | awk '{print tolower([$]0)}'`
	    defs="${ARCH} _${ARCH}_ ${arch} PALM_SIZE _MT _DLL _WINDOWS"
	    for i in $defs ; do
		AC_DEFINE_UNQUOTED($i)
	    done
#	    if test "${ARCH}" = "X86EM"; then
#		AC_DEFINE_UNQUOTED(_WIN32_WCE_EMULATION)
#	    fi
	    AC_DEFINE_UNQUOTED(_WIN32_WCE, $CEVERSION)
	    AC_DEFINE_UNQUOTED(UNDER_CE, $CEVERSION)
	    CFLAGS_DEBUG="-nologo -Zi -Od"
	    CFLAGS_OPTIMIZE="-nologo -O2"
	    lversion=`echo ${CEVERSION} | sed -e 's/\(.\)\(..\)/\1\.\2/'`
	    lflags="-nodefaultlib -MACHINE:${ARCH} -LIBPATH:\"${CELIBPATH}\" -subsystem:windowsce,${lversion} -nologo"
	    LINKBIN="\"${CEBINROOT}/link.exe\""
	    AC_SUBST(CELIB_DIR)
	    if test "${CEVERSION}" -lt 400 ; then
		LIBS="coredll.lib corelibc.lib winsock.lib"
	    else
		LIBS="coredll.lib corelibc.lib ws2.lib"
	    fi
	    # celib currently stuck at wce300 status
	    #LIBS="$LIBS \${CELIB_DIR}/wince-${ARCH}-pocket-${OSVERSION}-release/celib.lib"
	    LIBS="$LIBS \"\${CELIB_DIR}/wince-${ARCH}-pocket-wce300-release/celib.lib\""
	    LIBS_GUI="commctrl.lib commdlg.lib"
	else
	    LIBS_GUI="gdi32.lib comdlg32.lib imm32.lib comctl32.lib shell32.lib uuid.lib"
	fi

	SHLIB_LD="${LINKBIN} -dll -incremental:no ${lflags}"
	SHLIB_LD_LIBS='${LIBS}'
	# link -lib only works when -lib is the first arg
	STLIB_LD="${LINKBIN} -lib ${lflags}"
	RC_OUT=-fo
	RC_TYPE=-r
	RC_INCLUDE=-i
	RC_DEFINE=-d
	RES=res
	MAKE_LIB="\${STLIB_LD} -out:\[$]@"
	MAKE_STUB_LIB="\${STLIB_LD} -nodefaultlib -out:\[$]@"
	POST_MAKE_LIB=
	MAKE_EXE="\${CC} -Fe\[$]@"
	LIBPREFIX=""

	CFLAGS_DEBUG="${CFLAGS_DEBUG} -D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE"
	CFLAGS_OPTIMIZE="${CFLAGS_OPTIMIZE} -D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE"

	EXTRA_CFLAGS=""
	CFLAGS_WARNING="-W3"
	LDFLAGS_DEBUG="-debug"
	LDFLAGS_OPTIMIZE="-release"

	# Specify the CC output file names based on the target name
	CC_OBJNAME="-Fo\[$]@"
	CC_EXENAME="-Fe\"\$(shell \$(CYGPATH) '\[$]@')\""

	# Specify linker flags depending on the type of app being
	# built -- Console vs. Window.
	if test "$doWince" != "no" -a "${TARGETCPU}" != "X86"; then
	    LDFLAGS_CONSOLE="-link ${lflags}"
	    LDFLAGS_WINDOW=${LDFLAGS_CONSOLE}
	else
	    LDFLAGS_CONSOLE="-link -subsystem:console ${lflags}"
	    LDFLAGS_WINDOW="-link -subsystem:windows ${lflags}"
	fi
    fi

    if test "$do64bit" != "no" ; then
	AC_DEFINE(TCL_CFG_DO64BIT)
    fi

    if test "${GCC}" = "yes" ; then
	AC_CACHE_CHECK(for SEH support in compiler,
	    tcl_cv_seh,
	AC_TRY_RUN([
	    #define WIN32_LEAN_AND_MEAN
	    #include <windows.h>
	    #undef WIN32_LEAN_AND_MEAN

	    int main(int argc, char** argv) {
		int a, b = 0;
		__try {
		    a = 666 / b;
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
		    return 0;
		}
		return 1;
	    }
	],
	    tcl_cv_seh=yes,
	    tcl_cv_seh=no,
	    tcl_cv_seh=no)
	)
	if test "$tcl_cv_seh" = "no" ; then
	    AC_DEFINE(HAVE_NO_SEH, 1,
		    [Defined when mingw does not support SEH])
	fi

	#
	# Check to see if the excpt.h include file provided contains the
	# definition for EXCEPTION_DISPOSITION; if not, which is the case
	# with Cygwin's version as of 2002-04-10, define it to be int,
	# sufficient for getting the current code to work.
	#
	AC_CACHE_CHECK(for EXCEPTION_DISPOSITION support in include files,
	    tcl_cv_eh_disposition,
	    AC_TRY_COMPILE([
#	    define WIN32_LEAN_AND_MEAN
#	    include <windows.h>
#	    undef WIN32_LEAN_AND_MEAN
	    ],[
		EXCEPTION_DISPOSITION x;
	    ],
		tcl_cv_eh_disposition=yes,
		tcl_cv_eh_disposition=no)
	)
	if test "$tcl_cv_eh_disposition" = "no" ; then
	AC_DEFINE(EXCEPTION_DISPOSITION, int,
		[Defined when cygwin/mingw does not support EXCEPTION DISPOSITION])
	fi

	# Check to see if winnt.h defines CHAR, SHORT, and LONG
	# even if VOID has already been #defined. The win32api
	# used by mingw and cygwin is known to do this.

	AC_CACHE_CHECK(for winnt.h that ignores VOID define,
	    tcl_cv_winnt_ignore_void,
	    AC_TRY_COMPILE([
		#define VOID void
		#define WIN32_LEAN_AND_MEAN
		#include <windows.h>
		#undef WIN32_LEAN_AND_MEAN
	    ], [
		CHAR c;
		SHORT s;
		LONG l;
	    ],
        tcl_cv_winnt_ignore_void=yes,
        tcl_cv_winnt_ignore_void=no)
	)
	if test "$tcl_cv_winnt_ignore_void" = "yes" ; then
	    AC_DEFINE(HAVE_WINNT_IGNORE_VOID, 1,
		    [Defined when cygwin/mingw ignores VOID define in winnt.h])
	fi

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
    fi

    # DL_LIBS is empty, but then we match the Unix version
    AC_SUBST(DL_LIBS)
    AC_SUBST(CFLAGS_DEBUG)
    AC_SUBST(CFLAGS_OPTIMIZE)
    AC_SUBST(CFLAGS_WARNING)
])

#------------------------------------------------------------------------
# SC_WITH_TCL --
#
#	Location of the Tcl build directory.
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
#		TCL_BIN_DIR	Full path to the tcl build dir.
#------------------------------------------------------------------------

AC_DEFUN([SC_WITH_TCL], [
    if test -d ../../tcl8.6$1/win;  then
	TCL_BIN_DEFAULT=../../tcl8.6$1/win
    else
	TCL_BIN_DEFAULT=../../tcl8.6/win
    fi

    AC_ARG_WITH(tcl, [  --with-tcl=DIR          use Tcl 8.6 binaries from DIR],
	    TCL_BIN_DIR=$withval, TCL_BIN_DIR=`cd $TCL_BIN_DEFAULT; pwd`)
    if test ! -d $TCL_BIN_DIR; then
	AC_MSG_ERROR(Tcl directory $TCL_BIN_DIR does not exist)
    fi
    if test ! -f $TCL_BIN_DIR/Makefile; then
	AC_MSG_ERROR(There is no Makefile in $TCL_BIN_DIR:  perhaps you did not specify the Tcl *build* directory (not the toplevel Tcl directory) or you forgot to configure Tcl?)
    else
	echo "building against Tcl binaries in: $TCL_BIN_DIR"
    fi
    AC_SUBST(TCL_BIN_DIR)
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
# Arguments
#	none
#
# Results
#	Subst's the following values:
#		TCLSH_PROG
#------------------------------------------------------------------------

AC_DEFUN([SC_PROG_TCLSH], [
    AC_MSG_CHECKING([for tclsh])

    AC_CACHE_VAL(ac_cv_path_tclsh, [
	search_path=`echo ${PATH} | sed -e 's/:/ /g'`
	for dir in $search_path ; do
	    for j in `ls -r $dir/tclsh[[8-9]]*.exe 2> /dev/null` \
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
	AC_MSG_RESULT($TCLSH_PROG)
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
# Arguments
#	none
#
# Results
#	Subst's the following values:
#		BUILD_TCLSH
#------------------------------------------------------------------------

AC_DEFUN([SC_BUILD_TCLSH], [
    AC_MSG_CHECKING([for tclsh in Tcl build directory])
    BUILD_TCLSH=${TCL_BIN_DIR}/tclsh${TCL_MAJOR_VERSION}${TCL_MINOR_VERSION}${TCL_DBGX}${EXEEXT}
    AC_MSG_RESULT($BUILD_TCLSH)
    AC_SUBST(BUILD_TCLSH)
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
    AC_ARG_WITH(encoding, [  --with-encoding         encoding for configuration values], with_tcencoding=${withval})

    if test x"${with_tcencoding}" != x ; then
	AC_DEFINE_UNQUOTED(TCL_CFGVAL_ENCODING,"${with_tcencoding}")
    else
	# Default encoding on windows is not "iso8859-1"
	AC_DEFINE(TCL_CFGVAL_ENCODING,"cp1252")
    fi
])

#--------------------------------------------------------------------
# SC_EMBED_MANIFEST
#
#	Figure out if we can embed the manifest where necessary
#
# Arguments:
#	An optional manifest to merge into DLL/EXE.
#
# Results:
#	Will define the following vars:
#		VC_MANIFEST_EMBED_DLL
#		VC_MANIFEST_EMBED_EXE
#
#--------------------------------------------------------------------

AC_DEFUN([SC_EMBED_MANIFEST], [
    AC_MSG_CHECKING(whether to embed manifest)
    AC_ARG_ENABLE(embedded-manifest,
	AC_HELP_STRING([--enable-embedded-manifest],
		[embed manifest if possible (default: yes)]),
	[embed_ok=$enableval], [embed_ok=yes])

    VC_MANIFEST_EMBED_DLL=
    VC_MANIFEST_EMBED_EXE=
    result=no
    if test "$embed_ok" = "yes" -a "${SHARED_BUILD}" = "1" \
       -a "$GCC" != "yes" ; then
	# Add the magic to embed the manifest into the dll/exe
	AC_EGREP_CPP([manifest needed], [
#if defined(_MSC_VER) && _MSC_VER >= 1400
print("manifest needed")
#endif
	], [
	# Could do a CHECK_PROG for mt, but should always be with MSVC8+
	# Could add 'if test -f' check, but manifest should be created
	# in this compiler case
	# Add in a manifest argument that may be specified
	# XXX Needs improvement so that the test for existence accounts
	# XXX for a provided (known) manifest
	VC_MANIFEST_EMBED_DLL="if test -f \[$]@.manifest ; then mt.exe -nologo -manifest \[$]@.manifest $1 -outputresource:\[$]@\;2 ; fi"
	VC_MANIFEST_EMBED_EXE="if test -f \[$]@.manifest ; then mt.exe -nologo -manifest \[$]@.manifest $1 -outputresource:\[$]@\;1 ; fi"
	result=yes
	if test "x$1" != x ; then
	    result="yes ($1)"
	fi
	])
    fi
    AC_MSG_RESULT([$result])
    AC_SUBST(VC_MANIFEST_EMBED_DLL)
    AC_SUBST(VC_MANIFEST_EMBED_EXE)
])
