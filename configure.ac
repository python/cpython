dnl ***************************************************
dnl * Please run autoreconf -if to test your changes! *
dnl ***************************************************
dnl
dnl Python's configure.ac file requires autoconf 2.69 and autoconf-archive.
dnl

# Set VERSION so we only need to edit in one place (i.e., here)
m4_define(PYTHON_VERSION, 3.11)

AC_PREREQ([2.69])

AC_INIT([python],[PYTHON_VERSION],[https://github.com/python/cpython/issues/])

m4_ifdef(
    [AX_C_FLOAT_WORDS_BIGENDIAN],
    [],
    [AC_MSG_ERROR([Please install autoconf-archive package and re-run autoreconf])]
)dnl
m4_ifdef(
    [PKG_PROG_PKG_CONFIG],
    [],
    [AC_MSG_ERROR([Please install pkgconf's m4 macro package and re-run autoreconf])]
)dnl

dnl Helpers for saving and restoring environment variables:
dnl - _SAVE_VAR([VAR])         Helper for SAVE_ENV; stores VAR as save_VAR
dnl - _RESTORE_VAR([VAR])      Helper for RESTORE_ENV; restores VAR from save_VAR
dnl - SAVE_ENV                 Saves CFLAGS, LDFLAGS, LIBS, and CPPFLAGS
dnl - RESTORE_ENV              Restores CFLAGS, LDFLAGS, LIBS, and CPPFLAGS
dnl - WITH_SAVE_ENV([SCRIPT])  Runs SCRIPT wrapped with SAVE_ENV/RESTORE_ENV
AC_DEFUN([_SAVE_VAR], [AS_VAR_COPY([save_][$1], [$1])])dnl
AC_DEFUN([_RESTORE_VAR], [AS_VAR_COPY([$1], [save_][$1])])dnl
AC_DEFUN([SAVE_ENV],
[_SAVE_VAR([CFLAGS])]
[_SAVE_VAR([CPPFLAGS])]
[_SAVE_VAR([LDFLAGS])]
[_SAVE_VAR([LIBS])]
)dnl
AC_DEFUN([RESTORE_ENV],
[_RESTORE_VAR([CFLAGS])]
[_RESTORE_VAR([CPPFLAGS])]
[_RESTORE_VAR([LDFLAGS])]
[_RESTORE_VAR([LIBS])]
)dnl
AC_DEFUN([WITH_SAVE_ENV],
[SAVE_ENV]
[$1]
[RESTORE_ENV]
)dnl

dnl PY_CHECK_FUNC(FUNCTION, [INCLUDES], [AC_DEFINE-VAR])
AC_DEFUN([PY_CHECK_FUNC],
[ AS_VAR_PUSHDEF([py_var], [ac_cv_func_$1])
  AS_VAR_PUSHDEF([py_define], m4_ifblank([$3], [[HAVE_]m4_toupper($1)], [$3]))
  AC_CACHE_CHECK(
    [for $1],
    [py_var],
    [AC_COMPILE_IFELSE(
      [AC_LANG_PROGRAM([$2], [void *x=$1])],
      [AS_VAR_SET([py_var], [yes])],
      [AS_VAR_SET([py_var], [no])])]
  )
  AS_VAR_IF(
    [py_var],
    [yes],
    [AC_DEFINE([py_define], [1], [Define if you have the '$1' function.])])
  AS_VAR_POPDEF([py_var])
  AS_VAR_POPDEF([py_define])
])

dnl PY_CHECK_LIB(LIBRARY, FUNCTION, [ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND], [OTHER-LIBRARIES])
dnl Like AC_CHECK_LIB() but does not modify LIBS
AC_DEFUN([PY_CHECK_LIB],
[AS_VAR_COPY([py_check_lib_save_LIBS], [LIBS])]
[AC_CHECK_LIB($1, $2, $3, $4, $5)]
[AS_VAR_COPY([LIBS], [py_check_lib_save_LIBS])]
)

dnl PY_CHECK_EMSCRIPTEN_PORT(PKG_VAR, [EMPORT_ARGS])
dnl Use Emscripten port unless user passes ${PKG_VAR}_CFLAGS
dnl or ${PKG_VAR}_LIBS to configure.
AC_DEFUN([PY_CHECK_EMSCRIPTEN_PORT], [
  AS_VAR_PUSHDEF([py_cflags], [$1_CFLAGS])
  AS_VAR_PUSHDEF([py_libs], [$1_LIBS])
  AS_IF([test "$ac_sys_system" = "Emscripten" -a -z "$py_cflags" -a -z "$py_libs"], [
    py_cflags="$2"
    py_libs="$2"
  ])
  AS_VAR_POPDEF([py_cflags])
  AS_VAR_POPDEF([py_libs])
])

AC_SUBST(BASECPPFLAGS)
if test "$srcdir" != . -a "$srcdir" != "$(pwd)"; then
    # If we're building out-of-tree, we need to make sure the following
    # resources get picked up before their $srcdir counterparts.
    #   Objects/ -> typeslots.inc
    #   Include/ -> Python.h
    #   Python/  -> importlib.h
    # (A side effect of this is that these resources will automatically be
    #  regenerated when building out-of-tree, regardless of whether or not
    #  the $srcdir counterpart is up-to-date.  This is an acceptable trade
    #  off.)
    BASECPPFLAGS="-IObjects -IInclude -IPython"
else
    BASECPPFLAGS=""
fi

AC_SUBST(GITVERSION)
AC_SUBST(GITTAG)
AC_SUBST(GITBRANCH)

if test -e $srcdir/.git
then
AC_CHECK_PROG(HAS_GIT, git, found, not-found)
else
HAS_GIT=no-repository
fi
if test $HAS_GIT = found
then
    GITVERSION="git --git-dir \$(srcdir)/.git rev-parse --short HEAD"
    GITTAG="git --git-dir \$(srcdir)/.git describe --all --always --dirty"
    GITBRANCH="git --git-dir \$(srcdir)/.git name-rev --name-only HEAD"
else
    GITVERSION=""
    GITTAG=""
    GITBRANCH=""
fi

AC_CONFIG_SRCDIR([Include/object.h])
AC_CONFIG_HEADERS([pyconfig.h])

AC_CANONICAL_HOST
AC_SUBST(build)
AC_SUBST(host)

AS_VAR_IF([cross_compiling], [maybe],
 [AC_MSG_ERROR([Cross compiling required --host=HOST-TUPLE and --build=ARCH])]
)

# pybuilddir.txt will be created by --generate-posix-vars in the Makefile
rm -f pybuilddir.txt

AC_ARG_WITH(
  [build-python],
  [AS_HELP_STRING([--with-build-python=python]PYTHON_VERSION,
                  [path to build python binary for cross compiling (default: _bootstrap_python or python]PYTHON_VERSION[)])],
  [
    AC_MSG_CHECKING([for --with-build-python])

    AS_VAR_IF([with_build_python], [yes], [with_build_python=python$PACKAGE_VERSION])
    AS_VAR_IF([with_build_python], [no], [AC_MSG_ERROR([invalid --with-build-python option: expected path or "yes", not "no"])])

    if ! $(command -v "$with_build_python" >/dev/null 2>&1); then
      AC_MSG_ERROR([invalid or missing build python binary "$with_build_python"])
    fi
    build_python_ver=$($with_build_python -c "import sys; print(f'{sys.version_info.major}.{sys.version_info.minor}')")
    if test "$build_python_ver" != "$PACKAGE_VERSION"; then
      AC_MSG_ERROR(["$with_build_python" has incompatible version $build_python_ver (expected: $PACKAGE_VERSION)])
    fi
    dnl Build Python interpreter is used for regeneration and freezing.
    ac_cv_prog_PYTHON_FOR_REGEN=$with_build_python
    PYTHON_FOR_FREEZE="$with_build_python"
    PYTHON_FOR_BUILD='_PYTHON_PROJECT_BASE=$(abs_builddir) _PYTHON_HOST_PLATFORM=$(_PYTHON_HOST_PLATFORM) PYTHONPATH=$(shell test -f pybuilddir.txt && echo $(abs_builddir)/`cat pybuilddir.txt`:)$(srcdir)/Lib _PYTHON_SYSCONFIGDATA_NAME=_sysconfigdata_$(ABIFLAGS)_$(MACHDEP)_$(MULTIARCH) '$with_build_python
    AC_MSG_RESULT([$with_build_python])
  ], [
    AS_VAR_IF([cross_compiling], [yes],
      [AC_MSG_ERROR([Cross compiling requires --with-build-python])]
    )
    PYTHON_FOR_BUILD='./$(BUILDPYTHON) -E'
    PYTHON_FOR_FREEZE="./_bootstrap_python"
  ]
)
AC_SUBST([PYTHON_FOR_BUILD])

AC_MSG_CHECKING([for Python interpreter freezing])
AC_MSG_RESULT([$PYTHON_FOR_FREEZE])
AC_SUBST([PYTHON_FOR_FREEZE])

AS_VAR_IF([cross_compiling], [yes],
  [
    dnl external build Python, freezing depends on Programs/_freeze_module.py
    FREEZE_MODULE_BOOTSTRAP='$(PYTHON_FOR_FREEZE) $(srcdir)/Programs/_freeze_module.py'
    FREEZE_MODULE_BOOTSTRAP_DEPS='$(srcdir)/Programs/_freeze_module.py'
    FREEZE_MODULE='$(FREEZE_MODULE_BOOTSTRAP)'
    FREEZE_MODULE_DEPS='$(FREEZE_MODULE_BOOTSTRAP_DEPS)'
    PYTHON_FOR_BUILD_DEPS=''
  ],
  [
    dnl internal build tools also depend on Programs/_freeze_module and _bootstrap_python.
    FREEZE_MODULE_BOOTSTRAP='./Programs/_freeze_module'
    FREEZE_MODULE_BOOTSTRAP_DEPS="Programs/_freeze_module"
    FREEZE_MODULE='$(PYTHON_FOR_FREEZE) $(srcdir)/Programs/_freeze_module.py'
    FREEZE_MODULE_DEPS="_bootstrap_python \$(srcdir)/Programs/_freeze_module.py"
    PYTHON_FOR_BUILD_DEPS='$(BUILDPYTHON)'
  ]
)
AC_SUBST([FREEZE_MODULE_BOOTSTRAP])
AC_SUBST([FREEZE_MODULE_BOOTSTRAP_DEPS])
AC_SUBST([FREEZE_MODULE])
AC_SUBST([FREEZE_MODULE_DEPS])
AC_SUBST([PYTHON_FOR_BUILD_DEPS])

AC_CHECK_PROGS([PYTHON_FOR_REGEN],
  [python$PACKAGE_VERSION python3.10 python3.9 python3.8 python3.7 python3.6 python3 python],
  [python3])
AC_SUBST(PYTHON_FOR_REGEN)

AC_MSG_CHECKING([Python for regen version])
if command -v "$PYTHON_FOR_REGEN" >/dev/null 2>&1; then
  AC_MSG_RESULT([$($PYTHON_FOR_REGEN -V 2>/dev/null)])
else
  AC_MSG_RESULT([missing])
fi

dnl Ensure that if prefix is specified, it does not end in a slash. If
dnl it does, we get path names containing '//' which is both ugly and
dnl can cause trouble.

dnl Last slash shouldn't be stripped if prefix=/
if test "$prefix" != "/"; then
    prefix=`echo "$prefix" | sed -e 's/\/$//g'`
fi

dnl This is for stuff that absolutely must end up in pyconfig.h.
dnl Please use pyport.h instead, if possible.
AH_TOP([
#ifndef Py_PYCONFIG_H
#define Py_PYCONFIG_H
])
AH_BOTTOM([
/* Define the macros needed if on a UnixWare 7.x system. */
#if defined(__USLC__) && defined(__SCO_VERSION__)
#define STRICT_SYSV_CURSES /* Don't use ncurses extensions */
#endif

#endif /*Py_PYCONFIG_H*/
])

# We don't use PACKAGE_ variables, and they cause conflicts
# with other autoconf-based packages that include Python.h
grep -v 'define PACKAGE_' <confdefs.h >confdefs.h.new
rm confdefs.h
mv confdefs.h.new confdefs.h

AC_SUBST(VERSION)
VERSION=PYTHON_VERSION

# Version number of Python's own shared library file.
AC_SUBST(SOVERSION)
SOVERSION=1.0

# The later definition of _XOPEN_SOURCE and _POSIX_C_SOURCE disables
# certain features on NetBSD, so we need _NETBSD_SOURCE to re-enable
# them.
AC_DEFINE(_NETBSD_SOURCE, 1, [Define on NetBSD to activate all library features])

# The later definition of _XOPEN_SOURCE and _POSIX_C_SOURCE disables
# certain features on FreeBSD, so we need __BSD_VISIBLE to re-enable
# them.
AC_DEFINE(__BSD_VISIBLE, 1, [Define on FreeBSD to activate all library features])

# The later definition of _XOPEN_SOURCE and _POSIX_C_SOURCE disables
# certain features on Mac OS X, so we need _DARWIN_C_SOURCE to re-enable
# them.
AC_DEFINE(_DARWIN_C_SOURCE, 1, [Define on Darwin to activate all library features])


define_xopen_source=yes

# Arguments passed to configure.
AC_SUBST(CONFIG_ARGS)
CONFIG_ARGS="$ac_configure_args"

dnl Allow users to disable pkg-config or require pkg-config
AC_ARG_WITH(
  [pkg-config],
  [AS_HELP_STRING([[--with-pkg-config=[yes|no|check]]],
                  [use pkg-config to detect build options (default is check)])],
  [],
  [with_pkg_config=check]
)
AS_CASE([$with_pkg_config],
  [yes|check], [
    if test -z "$PKG_CONFIG"; then
      dnl invalidate stale config.cache values
      AS_UNSET([PKG_CONFIG])
      AS_UNSET([ac_cv_path_ac_pt_PKG_CONFIG])
      AS_UNSET([ac_cv_prog_ac_ct_PKG_CONFIG])
    fi
    PKG_PROG_PKG_CONFIG
  ],
  [no], [
    PKG_CONFIG=''
    dnl force AX_CHECK_OPENSSL to ignore pkg-config
    ac_cv_path_ac_pt_PKG_CONFIG=''
    ac_cv_prog_ac_ct_PKG_CONFIG=''
  ],
  [AC_MSG_ERROR([invalid argument --with-pkg-config=$with_pkg_config])]
)
if test "$with_pkg_config" = yes -a -z "$PKG_CONFIG"; then
  AC_MSG_ERROR([pkg-config is required])]
fi

AC_MSG_CHECKING([for --enable-universalsdk])
AC_ARG_ENABLE(universalsdk,
	AS_HELP_STRING([--enable-universalsdk@<:@=SDKDIR@:>@],
	               [create a universal binary build.
	                SDKDIR specifies which macOS SDK should be used to perform the build,
	                see Mac/README.rst. (default is no)]),
[
	case $enableval in
	yes)
		# Locate the best usable SDK, see Mac/README for more
		# information
		enableval="`/usr/bin/xcodebuild -version -sdk macosx Path 2>/dev/null`"
		if ! ( echo $enableval | grep -E '\.sdk' 1>/dev/null )
		then
			enableval=/Developer/SDKs/MacOSX10.4u.sdk
			if test ! -d "${enableval}"
			then
				enableval=/
			fi
		fi
		;;
	esac
	case $enableval in
	no)
		UNIVERSALSDK=
		enable_universalsdk=
		;;
	*)
		UNIVERSALSDK=$enableval
		if test ! -d "${UNIVERSALSDK}"
		then
			AC_MSG_ERROR([--enable-universalsdk specifies non-existing SDK: ${UNIVERSALSDK}])
		fi
		;;
	esac

],[
   	UNIVERSALSDK=
	enable_universalsdk=
])
if test -n "${UNIVERSALSDK}"
then
	AC_MSG_RESULT(${UNIVERSALSDK})
else
	AC_MSG_RESULT(no)
fi
AC_SUBST(UNIVERSALSDK)

AC_SUBST(ARCH_RUN_32BIT)
ARCH_RUN_32BIT=""

# For backward compatibility reasons we prefer to select '32-bit' if available,
# otherwise use 'intel'
UNIVERSAL_ARCHS="32-bit"
if test "`uname -s`" = "Darwin"
then
	if test -n "${UNIVERSALSDK}"
	then
		if test -z "`/usr/bin/file -L "${UNIVERSALSDK}/usr/lib/libSystem.dylib" | grep ppc`"
		then
			UNIVERSAL_ARCHS="intel"
		fi
	fi
fi

AC_SUBST(LIPO_32BIT_FLAGS)
AC_SUBST(LIPO_INTEL64_FLAGS)
AC_MSG_CHECKING(for --with-universal-archs)
AC_ARG_WITH(universal-archs,
    AS_HELP_STRING([--with-universal-archs=ARCH],
                   [specify the kind of macOS universal binary that should be created.
                    This option is only valid when --enable-universalsdk is set; options are:
                    ("universal2", "intel-64", "intel-32", "intel", "32-bit",
                    "64-bit", "3-way", or "all")
                    see Mac/README.rst]),
[
	UNIVERSAL_ARCHS="$withval"
],
[])
if test -n "${UNIVERSALSDK}"
then
	AC_MSG_RESULT(${UNIVERSAL_ARCHS})
else
	AC_MSG_RESULT(no)
fi

AC_ARG_WITH(framework-name,
              AS_HELP_STRING([--with-framework-name=FRAMEWORK],
                             [specify the name for the python framework on macOS
                              only valid when --enable-framework is set. see Mac/README.rst
                              (default is 'Python')]),
[
    PYTHONFRAMEWORK=${withval}
    PYTHONFRAMEWORKDIR=${withval}.framework
    PYTHONFRAMEWORKIDENTIFIER=org.python.`echo $withval | tr '[A-Z]' '[a-z]'`
    ],[
    PYTHONFRAMEWORK=Python
    PYTHONFRAMEWORKDIR=Python.framework
    PYTHONFRAMEWORKIDENTIFIER=org.python.python
])
dnl quadrigraphs "@<:@" and "@:>@" produce "[" and "]" in the output
AC_ARG_ENABLE(framework,
              AS_HELP_STRING([--enable-framework@<:@=INSTALLDIR@:>@],
                             [create a Python.framework rather than a traditional Unix install.
                              optional INSTALLDIR specifies the installation path. see Mac/README.rst
                              (default is no)]),
[
	case $enableval in
	yes)
		enableval=/Library/Frameworks
	esac
	case $enableval in
	no)
		PYTHONFRAMEWORK=
		PYTHONFRAMEWORKDIR=no-framework
		PYTHONFRAMEWORKPREFIX=
		PYTHONFRAMEWORKINSTALLDIR=
		FRAMEWORKINSTALLFIRST=
		FRAMEWORKINSTALLLAST=
		FRAMEWORKALTINSTALLFIRST=
		FRAMEWORKALTINSTALLLAST=
		FRAMEWORKPYTHONW=
		if test "x${prefix}" = "xNONE"; then
			FRAMEWORKUNIXTOOLSPREFIX="${ac_default_prefix}"
		else
			FRAMEWORKUNIXTOOLSPREFIX="${prefix}"
		fi
		enable_framework=
		;;
	*)
		PYTHONFRAMEWORKPREFIX="${enableval}"
		PYTHONFRAMEWORKINSTALLDIR=$PYTHONFRAMEWORKPREFIX/$PYTHONFRAMEWORKDIR
		FRAMEWORKINSTALLFIRST="frameworkinstallstructure"
		FRAMEWORKALTINSTALLFIRST="frameworkinstallstructure "
		FRAMEWORKINSTALLLAST="frameworkinstallmaclib frameworkinstallapps frameworkinstallunixtools"
		FRAMEWORKALTINSTALLLAST="frameworkinstallmaclib frameworkinstallapps frameworkaltinstallunixtools"
		FRAMEWORKPYTHONW="frameworkpythonw"
		FRAMEWORKINSTALLAPPSPREFIX="/Applications"

		if test "x${prefix}" = "xNONE" ; then
			FRAMEWORKUNIXTOOLSPREFIX="${ac_default_prefix}"

		else
			FRAMEWORKUNIXTOOLSPREFIX="${prefix}"
		fi

		case "${enableval}" in
		/System*)
			FRAMEWORKINSTALLAPPSPREFIX="/Applications"
			if test "${prefix}" = "NONE" ; then
				# See below
				FRAMEWORKUNIXTOOLSPREFIX="/usr"
			fi
			;;

		/Library*)
			FRAMEWORKINSTALLAPPSPREFIX="/Applications"
			;;

		*/Library/Frameworks)
			MDIR="`dirname "${enableval}"`"
			MDIR="`dirname "${MDIR}"`"
			FRAMEWORKINSTALLAPPSPREFIX="${MDIR}/Applications"

			if test "${prefix}" = "NONE"; then
				# User hasn't specified the
				# --prefix option, but wants to install
				# the framework in a non-default location,
				# ensure that the compatibility links get
				# installed relative to that prefix as well
				# instead of in /usr/local.
				FRAMEWORKUNIXTOOLSPREFIX="${MDIR}"
			fi
			;;

		*)
			FRAMEWORKINSTALLAPPSPREFIX="/Applications"
			;;
		esac

		prefix=$PYTHONFRAMEWORKINSTALLDIR/Versions/$VERSION

		# Add files for Mac specific code to the list of output
		# files:
		AC_CONFIG_FILES(Mac/Makefile)
		AC_CONFIG_FILES(Mac/PythonLauncher/Makefile)
		AC_CONFIG_FILES(Mac/Resources/framework/Info.plist)
		AC_CONFIG_FILES(Mac/Resources/app/Info.plist)
	esac
	],[
	PYTHONFRAMEWORK=
	PYTHONFRAMEWORKDIR=no-framework
	PYTHONFRAMEWORKPREFIX=
	PYTHONFRAMEWORKINSTALLDIR=
	FRAMEWORKINSTALLFIRST=
	FRAMEWORKINSTALLLAST=
	FRAMEWORKALTINSTALLFIRST=
	FRAMEWORKALTINSTALLLAST=
	FRAMEWORKPYTHONW=
	if test "x${prefix}" = "xNONE" ; then
		FRAMEWORKUNIXTOOLSPREFIX="${ac_default_prefix}"
	else
		FRAMEWORKUNIXTOOLSPREFIX="${prefix}"
	fi
	enable_framework=

])
AC_SUBST(PYTHONFRAMEWORK)
AC_SUBST(PYTHONFRAMEWORKIDENTIFIER)
AC_SUBST(PYTHONFRAMEWORKDIR)
AC_SUBST(PYTHONFRAMEWORKPREFIX)
AC_SUBST(PYTHONFRAMEWORKINSTALLDIR)
AC_SUBST(FRAMEWORKINSTALLFIRST)
AC_SUBST(FRAMEWORKINSTALLLAST)
AC_SUBST(FRAMEWORKALTINSTALLFIRST)
AC_SUBST(FRAMEWORKALTINSTALLLAST)
AC_SUBST(FRAMEWORKPYTHONW)
AC_SUBST(FRAMEWORKUNIXTOOLSPREFIX)
AC_SUBST(FRAMEWORKINSTALLAPPSPREFIX)

AC_DEFINE_UNQUOTED(_PYTHONFRAMEWORK, "${PYTHONFRAMEWORK}", [framework name])

# Set name for machine-dependent library files
AC_ARG_VAR([MACHDEP], [name for machine-dependent library files])
AC_MSG_CHECKING(MACHDEP)
if test -z "$MACHDEP"
then
    # avoid using uname for cross builds
    if test "$cross_compiling" = yes; then
       # ac_sys_system and ac_sys_release are used for setting
       # a lot of different things including 'define_xopen_source'
       # in the case statement below.
	case "$host" in
	*-*-linux-android*)
		ac_sys_system=Linux-android
		;;
	*-*-linux*)
		ac_sys_system=Linux
		;;
	*-*-cygwin*)
		ac_sys_system=Cygwin
		;;
	*-*-vxworks*)
	    ac_sys_system=VxWorks
	    ;;
	*-*-emscripten)
	    ac_sys_system=Emscripten
	    ;;
	*-*-wasi)
	    ac_sys_system=WASI
	    ;;
	*)
		# for now, limit cross builds to known configurations
		MACHDEP="unknown"
		AC_MSG_ERROR([cross build not supported for $host])
	esac
	ac_sys_release=
    else
	ac_sys_system=`uname -s`
	if test "$ac_sys_system" = "AIX" \
	-o "$ac_sys_system" = "UnixWare" -o "$ac_sys_system" = "OpenUNIX"; then
		ac_sys_release=`uname -v`
	else
		ac_sys_release=`uname -r`
	fi
    fi
    ac_md_system=`echo $ac_sys_system |
			tr -d '[/ ]' | tr '[[A-Z]]' '[[a-z]]'`
    ac_md_release=`echo $ac_sys_release |
			tr -d '[/ ]' | sed 's/^[[A-Z]]\.//' | sed 's/\..*//'`
    MACHDEP="$ac_md_system$ac_md_release"

    case $MACHDEP in
	aix*) MACHDEP="aix";;
	linux*) MACHDEP="linux";;
	cygwin*) MACHDEP="cygwin";;
	darwin*) MACHDEP="darwin";;
	'')	MACHDEP="unknown";;
    esac
fi
AC_MSG_RESULT("$MACHDEP")

AC_SUBST(_PYTHON_HOST_PLATFORM)
if test "$cross_compiling" = yes; then
	case "$host" in
	*-*-linux*)
		case "$host_cpu" in
		arm*)
			_host_cpu=arm
			;;
		*)
			_host_cpu=$host_cpu
		esac
		;;
	*-*-cygwin*)
		_host_cpu=
		;;
	*-*-vxworks*)
		_host_cpu=$host_cpu
		;;
	wasm32-*-* | wasm64-*-*)
		_host_cpu=$host_cpu
		;;
	*)
		# for now, limit cross builds to known configurations
		MACHDEP="unknown"
		AC_MSG_ERROR([cross build not supported for $host])
	esac
	_PYTHON_HOST_PLATFORM="$MACHDEP${_host_cpu:+-$_host_cpu}"
fi

# Some systems cannot stand _XOPEN_SOURCE being defined at all; they
# disable features if it is defined, without any means to access these
# features as extensions. For these systems, we skip the definition of
# _XOPEN_SOURCE. Before adding a system to the list to gain access to
# some feature, make sure there is no alternative way to access this
# feature. Also, when using wildcards, make sure you have verified the
# need for not defining _XOPEN_SOURCE on all systems matching the
# wildcard, and that the wildcard does not include future systems
# (which may remove their limitations).
dnl quadrigraphs "@<:@" and "@:>@" produce "[" and "]" in the output
case $ac_sys_system/$ac_sys_release in
  # On OpenBSD, select(2) is not available if _XOPEN_SOURCE is defined,
  # even though select is a POSIX function. Reported by J. Ribbens.
  # Reconfirmed for OpenBSD 3.3 by Zachary Hamm, for 3.4 by Jason Ish.
  # In addition, Stefan Krah confirms that issue #1244610 exists through
  # OpenBSD 4.6, but is fixed in 4.7.
  OpenBSD/2.* | OpenBSD/3.* | OpenBSD/4.@<:@0123456@:>@)
    define_xopen_source=no
    # OpenBSD undoes our definition of __BSD_VISIBLE if _XOPEN_SOURCE is
    # also defined. This can be overridden by defining _BSD_SOURCE
    # As this has a different meaning on Linux, only define it on OpenBSD
    AC_DEFINE(_BSD_SOURCE, 1, [Define on OpenBSD to activate all library features])
    ;;
  OpenBSD/*)
    # OpenBSD undoes our definition of __BSD_VISIBLE if _XOPEN_SOURCE is
    # also defined. This can be overridden by defining _BSD_SOURCE
    # As this has a different meaning on Linux, only define it on OpenBSD
    AC_DEFINE(_BSD_SOURCE, 1, [Define on OpenBSD to activate all library features])
    ;;
  # Defining _XOPEN_SOURCE on NetBSD version prior to the introduction of
  # _NETBSD_SOURCE disables certain features (eg. setgroups). Reported by
  # Marc Recht
  NetBSD/1.5 | NetBSD/1.5.* | NetBSD/1.6 | NetBSD/1.6.* | NetBSD/1.6@<:@A-S@:>@)
    define_xopen_source=no;;
  # From the perspective of Solaris, _XOPEN_SOURCE is not so much a
  # request to enable features supported by the standard as a request
  # to disable features not supported by the standard.  The best way
  # for Python to use Solaris is simply to leave _XOPEN_SOURCE out
  # entirely and define __EXTENSIONS__ instead.
  SunOS/*)
    define_xopen_source=no;;
  # On UnixWare 7, u_long is never defined with _XOPEN_SOURCE,
  # but used in /usr/include/netinet/tcp.h. Reported by Tim Rice.
  # Reconfirmed for 7.1.4 by Martin v. Loewis.
  OpenUNIX/8.0.0| UnixWare/7.1.@<:@0-4@:>@)
    define_xopen_source=no;;
  # On OpenServer 5, u_short is never defined with _XOPEN_SOURCE,
  # but used in struct sockaddr.sa_family. Reported by Tim Rice.
  SCO_SV/3.2)
    define_xopen_source=no;;
  # On MacOS X 10.2, a bug in ncurses.h means that it craps out if
  # _XOPEN_EXTENDED_SOURCE is defined. Apparently, this is fixed in 10.3, which
  # identifies itself as Darwin/7.*
  # On Mac OS X 10.4, defining _POSIX_C_SOURCE or _XOPEN_SOURCE
  # disables platform specific features beyond repair.
  # On Mac OS X 10.3, defining _POSIX_C_SOURCE or _XOPEN_SOURCE
  # has no effect, don't bother defining them
  Darwin/@<:@6789@:>@.*)
    define_xopen_source=no;;
  Darwin/@<:@[12]@:>@@<:@0-9@:>@.*)
    define_xopen_source=no;;
  # On QNX 6.3.2, defining _XOPEN_SOURCE prevents netdb.h from
  # defining NI_NUMERICHOST.
  QNX/6.3.2)
    define_xopen_source=no
    ;;
  # On VxWorks, defining _XOPEN_SOURCE causes compile failures
  # in network headers still using system V types.
  VxWorks/*)
    define_xopen_source=no
    ;;

  # On HP-UX, defining _XOPEN_SOURCE to 600 or greater hides
  # chroot() and other functions
  hp*|HP*)
    define_xopen_source=no
    ;;

esac

if test $define_xopen_source = yes
then
  # X/Open 7, incorporating POSIX.1-2008
  AC_DEFINE(_XOPEN_SOURCE, 700,
            Define to the level of X/Open that your system supports)

  # On Tru64 Unix 4.0F, defining _XOPEN_SOURCE also requires
  # definition of _XOPEN_SOURCE_EXTENDED and _POSIX_C_SOURCE, or else
  # several APIs are not declared. Since this is also needed in some
  # cases for HP-UX, we define it globally.
  AC_DEFINE(_XOPEN_SOURCE_EXTENDED, 1,
   	    Define to activate Unix95-and-earlier features)

  AC_DEFINE(_POSIX_C_SOURCE, 200809L, Define to activate features from IEEE Stds 1003.1-2008)
fi

# On HP-UX mbstate_t requires _INCLUDE__STDC_A1_SOURCE
case $ac_sys_system in
  hp*|HP*)
    define_stdc_a1=yes;;
  *)
    define_stdc_a1=no;;
esac

if test $define_stdc_a1 = yes
then
  AC_DEFINE(_INCLUDE__STDC_A1_SOURCE, 1, Define to include mbstate_t for mbrtowc)
fi

# Record the configure-time value of MACOSX_DEPLOYMENT_TARGET,
# it may influence the way we can build extensions, so distutils
# needs to check it
AC_SUBST(CONFIGURE_MACOSX_DEPLOYMENT_TARGET)
AC_SUBST(EXPORT_MACOSX_DEPLOYMENT_TARGET)
CONFIGURE_MACOSX_DEPLOYMENT_TARGET=
EXPORT_MACOSX_DEPLOYMENT_TARGET='#'

# checks for alternative programs

# compiler flags are generated in two sets, BASECFLAGS and OPT.  OPT is just
# for debug/optimization stuff.  BASECFLAGS is for flags that are required
# just to get things to compile and link.  Users are free to override OPT
# when running configure or make.  The build should not break if they do.
# BASECFLAGS should generally not be messed with, however.

# If the user switches compilers, we can't believe the cache
if test ! -z "$ac_cv_prog_CC" -a ! -z "$CC" -a "$CC" != "$ac_cv_prog_CC"
then
  AC_MSG_ERROR([cached CC is different -- throw away $cache_file
(it is also a good idea to do 'make clean' before compiling)])
fi

# Don't let AC_PROG_CC set the default CFLAGS. It normally sets -g -O2
# when the compiler supports them, but we don't always want -O2, and
# we set -g later.
if test -z "$CFLAGS"; then
        CFLAGS=
fi

dnl Emscripten SDK and WASI SDK default to wasm32.
dnl On Emscripten use MEMORY64 setting to build target wasm64-emscripten.
dnl for wasm64.
AS_CASE([$host],
  [wasm64-*-emscripten], [
    AS_VAR_APPEND([CFLAGS], [" -sMEMORY64=1"])
    AS_VAR_APPEND([LDFLAGS], [" -sMEMORY64=1"])
  ],
)

if test "$ac_sys_system" = "Darwin"
then
	# Compiler selection on MacOSX is more complicated than
	# AC_PROG_CC can handle, see Mac/README for more
	# information
	if test -z "${CC}"
	then
		found_gcc=
		found_clang=
		as_save_IFS=$IFS; IFS=:
		for as_dir in $PATH
		do
			IFS=$as_save_IFS
			if test -x "${as_dir}/gcc"; then
				if test -z "${found_gcc}"; then
					found_gcc="${as_dir}/gcc"
				fi
			fi
			if test -x "${as_dir}/clang"; then
				if test -z "${found_clang}"; then
					found_clang="${as_dir}/clang"
				fi
			fi
		done
		IFS=$as_save_IFS

		if test -n "$found_gcc" -a -n "$found_clang"
		then
			if test -n "`"$found_gcc" --version | grep llvm-gcc`"
			then
				AC_MSG_NOTICE([Detected llvm-gcc, falling back to clang])
				CC="$found_clang"
				CXX="$found_clang++"
			fi


		elif test -z "$found_gcc" -a -n "$found_clang"
		then
			AC_MSG_NOTICE([No GCC found, use CLANG])
			CC="$found_clang"
			CXX="$found_clang++"

		elif test -z "$found_gcc" -a -z "$found_clang"
		then
			found_clang=`/usr/bin/xcrun -find clang 2>/dev/null`
			if test -n "${found_clang}"
			then
				AC_MSG_NOTICE([Using clang from Xcode.app])
				CC="${found_clang}"
				CXX="`/usr/bin/xcrun -find clang++`"

			# else: use default behaviour
			fi
		fi
	fi
fi
AC_PROG_CC
AC_PROG_CPP
AC_PROG_GREP
AC_PROG_SED
AC_PROG_EGREP

dnl detect compiler name
dnl check for xlc before clang, newer xlc's can use clang as frontend.
dnl check for GCC last, other compilers set __GNUC__, too.
dnl msvc is listed for completeness.
AC_CACHE_CHECK([for CC compiler name], [ac_cv_cc_name], [
cat > conftest.c <<EOF
#if defined(__INTEL_COMPILER) || defined(__ICC)
  icc
#elif defined(__ibmxl__) || defined(__xlc__) || defined(__xlC__)
  xlc
#elif defined(_MSC_VER)
  msvc
#elif defined(__clang__)
  clang
#elif defined(__GNUC__)
  gcc
#else
#  error unknown compiler
#endif
EOF

if $CPP $CPPFLAGS conftest.c >conftest.out 2>/dev/null; then
  ac_cv_cc_name=`grep -v '^#' conftest.out | grep -v '^ *$' | tr -d ' 	'`
else
  ac_cv_cc_name="unknown"
fi
rm -f conftest.c conftest.out
])

# checks for UNIX variants that set C preprocessor variables
# may set _GNU_SOURCE, __EXTENSIONS__, _POSIX_PTHREAD_SEMANTICS,
# _POSIX_SOURCE, _POSIX_1_SOURCE, and more
AC_USE_SYSTEM_EXTENSIONS

AC_SUBST(CXX)
AC_SUBST(MAINCC)
AC_MSG_CHECKING(for --with-cxx-main=<compiler>)
AC_ARG_WITH(cxx_main,
            AS_HELP_STRING([--with-cxx-main@<:@=COMPILER@:>@],
                           [compile main() and link Python executable with C++ compiler specified in COMPILER (default is $CXX)]),
[

	case $withval in
	no)	with_cxx_main=no
		MAINCC='$(CC)';;
	yes)	with_cxx_main=yes
		MAINCC='$(CXX)';;
	*)	with_cxx_main=yes
		MAINCC=$withval
		if test -z "$CXX"
		then
			CXX=$withval
		fi;;
	esac], [
	with_cxx_main=no
	MAINCC='$(CC)'
])
AC_MSG_RESULT($with_cxx_main)

preset_cxx="$CXX"
if test -z "$CXX"
then
        case "$CC" in
        gcc)    AC_PATH_TOOL(CXX, [g++], [g++], [notfound]) ;;
        cc)     AC_PATH_TOOL(CXX, [c++], [c++], [notfound]) ;;
        clang|*/clang)     AC_PATH_TOOL(CXX, [clang++], [clang++], [notfound]) ;;
        icc|*/icc)         AC_PATH_TOOL(CXX, [icpc], [icpc], [notfound]) ;;
        esac
	if test "$CXX" = "notfound"
	then
		CXX=""
	fi
fi
if test -z "$CXX"
then
	AC_CHECK_TOOLS(CXX, $CCC c++ g++ gcc CC cxx cc++ cl, notfound)
	if test "$CXX" = "notfound"
	then
		CXX=""
	fi
fi
if test "$preset_cxx" != "$CXX"
then
        AC_MSG_NOTICE([

  By default, distutils will build C++ extension modules with "$CXX".
  If this is not intended, then set CXX on the configure command line.
  ])
fi


AC_MSG_CHECKING([for the platform triplet based on compiler characteristics])
cat > conftest.c <<EOF
#undef bfin
#undef cris
#undef fr30
#undef linux
#undef hppa
#undef hpux
#undef i386
#undef mips
#undef powerpc
#undef sparc
#undef unix
#if defined(__ANDROID__)
    # Android is not a multiarch system.
#elif defined(__linux__)
# if defined(__x86_64__) && defined(__LP64__)
        x86_64-linux-gnu
# elif defined(__x86_64__) && defined(__ILP32__)
        x86_64-linux-gnux32
# elif defined(__i386__)
        i386-linux-gnu
# elif defined(__aarch64__) && defined(__AARCH64EL__)
#  if defined(__ILP32__)
        aarch64_ilp32-linux-gnu
#  else
        aarch64-linux-gnu
#  endif
# elif defined(__aarch64__) && defined(__AARCH64EB__)
#  if defined(__ILP32__)
        aarch64_be_ilp32-linux-gnu
#  else
        aarch64_be-linux-gnu
#  endif
# elif defined(__alpha__)
        alpha-linux-gnu
# elif defined(__ARM_EABI__) && defined(__ARM_PCS_VFP)
#  if defined(__ARMEL__)
        arm-linux-gnueabihf
#  else
        armeb-linux-gnueabihf
#  endif
# elif defined(__ARM_EABI__) && !defined(__ARM_PCS_VFP)
#  if defined(__ARMEL__)
        arm-linux-gnueabi
#  else
        armeb-linux-gnueabi
#  endif
# elif defined(__hppa__)
        hppa-linux-gnu
# elif defined(__ia64__)
        ia64-linux-gnu
# elif defined(__m68k__) && !defined(__mcoldfire__)
        m68k-linux-gnu
# elif defined(__mips_hard_float) && defined(__mips_isa_rev) && (__mips_isa_rev >=6) && defined(_MIPSEL)
#  if _MIPS_SIM == _ABIO32
        mipsisa32r6el-linux-gnu
#  elif _MIPS_SIM == _ABIN32
        mipsisa64r6el-linux-gnuabin32
#  elif _MIPS_SIM == _ABI64
        mipsisa64r6el-linux-gnuabi64
#  else
#   error unknown platform triplet
#  endif
# elif defined(__mips_hard_float) && defined(__mips_isa_rev) && (__mips_isa_rev >=6)
#  if _MIPS_SIM == _ABIO32
        mipsisa32r6-linux-gnu
#  elif _MIPS_SIM == _ABIN32
        mipsisa64r6-linux-gnuabin32
#  elif _MIPS_SIM == _ABI64
        mipsisa64r6-linux-gnuabi64
#  else
#   error unknown platform triplet
#  endif
# elif defined(__mips_hard_float) && defined(_MIPSEL)
#  if _MIPS_SIM == _ABIO32
        mipsel-linux-gnu
#  elif _MIPS_SIM == _ABIN32
        mips64el-linux-gnuabin32
#  elif _MIPS_SIM == _ABI64
        mips64el-linux-gnuabi64
#  else
#   error unknown platform triplet
#  endif
# elif defined(__mips_hard_float)
#  if _MIPS_SIM == _ABIO32
        mips-linux-gnu
#  elif _MIPS_SIM == _ABIN32
        mips64-linux-gnuabin32
#  elif _MIPS_SIM == _ABI64
        mips64-linux-gnuabi64
#  else
#   error unknown platform triplet
#  endif
# elif defined(__or1k__)
        or1k-linux-gnu
# elif defined(__powerpc__) && defined(__SPE__)
        powerpc-linux-gnuspe
# elif defined(__powerpc64__)
#  if defined(__LITTLE_ENDIAN__)
        powerpc64le-linux-gnu
#  else
        powerpc64-linux-gnu
#  endif
# elif defined(__powerpc__)
        powerpc-linux-gnu
# elif defined(__s390x__)
        s390x-linux-gnu
# elif defined(__s390__)
        s390-linux-gnu
# elif defined(__sh__) && defined(__LITTLE_ENDIAN__)
        sh4-linux-gnu
# elif defined(__sparc__) && defined(__arch64__)
        sparc64-linux-gnu
# elif defined(__sparc__)
        sparc-linux-gnu
# elif defined(__riscv)
#  if __riscv_xlen == 32
        riscv32-linux-gnu
#  elif __riscv_xlen == 64
        riscv64-linux-gnu
#  else
#   error unknown platform triplet
#  endif
# else
#   error unknown platform triplet
# endif
#elif defined(__FreeBSD_kernel__)
# if defined(__LP64__)
        x86_64-kfreebsd-gnu
# elif defined(__i386__)
        i386-kfreebsd-gnu
# else
#   error unknown platform triplet
# endif
#elif defined(__gnu_hurd__)
        i386-gnu
#elif defined(__APPLE__)
        darwin
#elif defined(__VXWORKS__)
        vxworks
#elif defined(__wasm32__)
#  if defined(__EMSCRIPTEN__)
	wasm32-emscripten
#  elif defined(__wasi__)
	wasm32-wasi
#  else
#    error unknown wasm32 platform
#  endif
#elif defined(__wasm64__)
#  if defined(__EMSCRIPTEN__)
	wasm64-emscripten
#  elif defined(__wasi__)
	wasm64-wasi
#  else
#    error unknown wasm64 platform
#  endif
#else
# error unknown platform triplet
#endif

EOF

if $CPP $CPPFLAGS conftest.c >conftest.out 2>/dev/null; then
  PLATFORM_TRIPLET=`grep -v '^#' conftest.out | grep -v '^ *$' | tr -d ' 	'`
  case "$build_os" in
  linux-musl*)
    PLATFORM_TRIPLET=`echo "$PLATFORM_TRIPLET" | sed 's/linux-gnu/linux-musl/'`
    ;;
  esac
  AC_MSG_RESULT([$PLATFORM_TRIPLET])
else
  AC_MSG_RESULT([none])
fi
rm -f conftest.c conftest.out

AC_MSG_CHECKING([for multiarch])
AS_CASE([$ac_sys_system],
  [Darwin*], [MULTIARCH=""],
  [FreeBSD*], [MULTIARCH=""],
  [MULTIARCH=$($CC --print-multiarch 2>/dev/null)]
)
AC_SUBST([MULTIARCH])
AC_MSG_RESULT([$MULTIARCH])

if test x$PLATFORM_TRIPLET != x && test x$MULTIARCH != x; then
  if test x$PLATFORM_TRIPLET != x$MULTIARCH; then
    AC_MSG_ERROR([internal configure error for the platform triplet, please file a bug report])
  fi
elif test x$PLATFORM_TRIPLET != x && test x$MULTIARCH = x; then
  MULTIARCH=$PLATFORM_TRIPLET
fi
AC_SUBST(PLATFORM_TRIPLET)

if test x$MULTIARCH != x; then
  MULTIARCH_CPPFLAGS="-DMULTIARCH=\\\"$MULTIARCH\\\""
fi
AC_SUBST(MULTIARCH_CPPFLAGS)

dnl Support tiers according to https://peps.python.org/pep-0011/
dnl
dnl NOTE: Windows support tiers are defined in PC/pyconfig.h.
dnl
AC_MSG_CHECKING([for PEP 11 support tier])
AS_CASE([$host/$ac_cv_cc_name],
  [x86_64-*-linux-gnu/gcc],          [PY_SUPPORT_TIER=1], dnl Linux on AMD64, any vendor, glibc, gcc
  [x86_64-apple-darwin*/clang],      [PY_SUPPORT_TIER=1], dnl macOS on Intel, any version
  [i686-pc-windows-msvc/msvc],       [PY_SUPPORT_TIER=1], dnl 32bit Windows on Intel, MSVC
  [x86_64-pc-windows-msvc/msvc],     [PY_SUPPORT_TIER=1], dnl 64bit Windows on AMD64, MSVC

  [aarch64-apple-darwin*/clang],     [PY_SUPPORT_TIER=2], dnl macOS on M1, any version
  [aarch64-*-linux-gnu/gcc],         [PY_SUPPORT_TIER=2], dnl Linux ARM64, glibc, gcc+clang
  [aarch64-*-linux-gnu/clang],       [PY_SUPPORT_TIER=2],
  [powerpc64le-*-linux-gnu/gcc],     [PY_SUPPORT_TIER=2], dnl Linux on PPC64 little endian, glibc, gcc
  [x86_64-*-linux-gnu/clang],        [PY_SUPPORT_TIER=2], dnl Linux on AMD64, any vendor, glibc, clang

  [aarch64-pc-windows-msvc/msvc],    [PY_SUPPORT_TIER=3], dnl Windows ARM64, MSVC
  [armv7l-*-linux-gnueabihf/gcc],    [PY_SUPPORT_TIER=3], dnl ARMv7 LE with hardware floats, any vendor, glibc, gcc
  [powerpc64le-*-linux-gnu/clang],   [PY_SUPPORT_TIER=3], dnl Linux on PPC64 little endian, glibc, clang
  [s390x-*-linux-gnu/gcc],           [PY_SUPPORT_TIER=3], dnl Linux on 64bit s390x (big endian), glibc, gcc
  [wasm32-unknown-emscripten/clang], [PY_SUPPORT_TIER=3], dnl WebAssembly Emscripten
  [wasm32-unknown-wasi/clang],       [PY_SUPPORT_TIER=3], dnl WebAssembly System Interface
  [x86_64-*-freebsd*/clang],         [PY_SUPPORT_TIER=3], dnl FreeBSD on AMD64
  [PY_SUPPORT_TIER=0]
)

AS_CASE([$PY_SUPPORT_TIER],
  [1], [AC_MSG_RESULT([$host/$ac_cv_cc_name has tier 1 (supported)])],
  [2], [AC_MSG_RESULT([$host/$ac_cv_cc_name has tier 2 (supported)])],
  [3], [AC_MSG_RESULT([$host/$ac_cv_cc_name has tier 3 (partially supported)])],
  [AC_MSG_WARN([$host/$ac_cv_cc_name is not supported])]
)

AC_DEFINE_UNQUOTED([PY_SUPPORT_TIER], [$PY_SUPPORT_TIER], [PEP 11 Support tier (1, 2, 3 or 0 for unsupported)])

AC_CACHE_CHECK([for -Wl,--no-as-needed], [ac_cv_wl_no_as_needed], [
  save_LDFLAGS="$LDFLAGS"
  AS_VAR_APPEND([LDFLAGS], [-Wl,--no-as-needed])
  AC_LINK_IFELSE([AC_LANG_PROGRAM([[]], [[]])],
    [NO_AS_NEEDED="-Wl,--no-as-needed"
     ac_cv_wl_no_as_needed=yes],
    [NO_AS_NEEDED=""
     ac_cv_wl_no_as_needed=no])
  LDFLAGS="$save_LDFLAGS"
])
AC_SUBST(NO_AS_NEEDED)

AC_MSG_CHECKING([for the Android API level])
cat > conftest.c <<EOF
#ifdef __ANDROID__
android_api = __ANDROID_API__
arm_arch = __ARM_ARCH
#else
#error not Android
#endif
EOF

if $CPP $CPPFLAGS conftest.c >conftest.out 2>/dev/null; then
  ANDROID_API_LEVEL=`sed -n -e '/__ANDROID_API__/d' -e 's/^android_api = //p' conftest.out`
  _arm_arch=`sed -n -e '/__ARM_ARCH/d' -e 's/^arm_arch = //p' conftest.out`
  AC_MSG_RESULT([$ANDROID_API_LEVEL])
  if test -z "$ANDROID_API_LEVEL"; then
    AC_MSG_ERROR([Fatal: you must define __ANDROID_API__])
  fi
  AC_DEFINE_UNQUOTED(ANDROID_API_LEVEL, $ANDROID_API_LEVEL, [The Android API level.])

  AC_MSG_CHECKING([for the Android arm ABI])
  AC_MSG_RESULT([$_arm_arch])
  if test "$_arm_arch" = 7; then
    BASECFLAGS="${BASECFLAGS} -mfloat-abi=softfp -mfpu=vfpv3-d16"
    LDFLAGS="${LDFLAGS} -march=armv7-a -Wl,--fix-cortex-a8"
  fi
else
  AC_MSG_RESULT([not Android])
fi
rm -f conftest.c conftest.out

# Check for unsupported systems
AS_CASE([$ac_sys_system/$ac_sys_release],
  [atheos*|Linux*/1*], [
    AC_MSG_ERROR([m4_normalize([
      This system \($ac_sys_system/$ac_sys_release\) is no longer supported.
      See README for details.
   ])])
 ]
)

AC_MSG_CHECKING([for --with-emscripten-target])
AC_ARG_WITH([emscripten-target],
  [AS_HELP_STRING([--with-emscripten-target=@<:@browser|node@:>@], [Emscripten platform])],
[
	AS_VAR_IF([ac_sys_system], [Emscripten], [
    AS_CASE([$with_emscripten_target],
      [browser], [ac_sys_emscripten_target=browser],
      [node], [ac_sys_emscripten_target=node],
dnl Debug builds with source map / dwarf symbols. Py_DEBUG builds easily
dnl run out of stack space. Detached sybmols and map prohibit some
dnl optimizations and increase file size. Options are undocumented so we
dnl are free to remove them in the future.
      [browser-debug], [ac_sys_emscripten_target=browser-debug],
      [node-debug], [ac_sys_emscripten_target=node-debug],
      [AC_MSG_ERROR([Invalid argument: --with-emscripten-target=browser|node])]
    )
  ], [
    AC_MSG_ERROR([--with-emscripten-target only applies to Emscripten])
  ])
], [
  AS_VAR_IF([ac_sys_system], [Emscripten], [ac_sys_emscripten_target=browser])
])
AC_MSG_RESULT([$ac_sys_emscripten_target])

dnl On Emscripten dlopen() requires -s MAIN_MODULE and -fPIC. The flags
dnl disables dead code elimination and increases the size of the WASM module
dnl by about 1.5 to 2MB. MAIN_MODULE defines __wasm_mutable_globals__.
dnl See https://emscripten.org/docs/compiling/Dynamic-Linking.html
AC_MSG_CHECKING([for --enable-wasm-dynamic-linking])
AC_ARG_ENABLE([wasm-dynamic-linking],
  [AS_HELP_STRING([--enable-wasm-dynamic-linking],
                  [Enable dynamic linking support for WebAssembly (default is no)])],
[
  AS_CASE([$ac_sys_system],
    [Emscripten], [],
    [WASI], [AC_MSG_ERROR([WASI dynamic linking is not implemented yet.])],
    [AC_MSG_ERROR([--enable-wasm-dynamic-linking only applies to Emscripten and WASI])]
  )
], [
  enable_wasm_dynamic_linking=missing
])
AC_MSG_RESULT([$enable_wasm_dynamic_linking])

AC_MSG_CHECKING([for --enable-wasm-pthreads])
AC_ARG_ENABLE([wasm-pthreads],
  [AS_HELP_STRING([--enable-wasm-pthreads],
                  [Enable pthread emulation for WebAssembly (default is no)])],
[
  AS_CASE([$ac_sys_system],
    [Emscripten], [],
    [WASI], [AC_MSG_ERROR([WASI threading is not implemented yet.])],
    [AC_MSG_ERROR([--enable-wasm-pthreads only applies to Emscripten and WASI])]
  )
], [
  enable_wasm_pthreads=missing
])
AC_MSG_RESULT([$enable_wasm_pthreads])

AC_MSG_CHECKING([for --with-suffix])
AC_ARG_WITH([suffix],
            [AS_HELP_STRING([--with-suffix=SUFFIX], [set executable suffix to SUFFIX (default is empty, yes is mapped to '.exe')])],
[
	AS_CASE([$with_suffix],
    [no], [EXEEXT=],
    [yes], [EXEEXT=.exe],
    [EXEEXT=$with_suffix]
  )
], [
  AS_CASE([$ac_sys_system/$ac_sys_emscripten_target],
    [Emscripten/browser*], [EXEEXT=.js],
    [Emscripten/node*], [EXEEXT=.js],
    [WASI/*], [EXEEXT=.wasm],
    [EXEEXT=]
  )
])
AC_MSG_RESULT([$EXEEXT])

# Test whether we're running on a non-case-sensitive system, in which
# case we give a warning if no ext is given
AC_SUBST(BUILDEXEEXT)
AC_MSG_CHECKING(for case-insensitive build directory)
if test ! -d CaseSensitiveTestDir; then
mkdir CaseSensitiveTestDir
fi

if test -d casesensitivetestdir && test -z "$EXEEXT"
then
    AC_MSG_RESULT(yes)
    BUILDEXEEXT=.exe
else
	AC_MSG_RESULT(no)
	BUILDEXEEXT=$EXEEXT
fi
rmdir CaseSensitiveTestDir

case $ac_sys_system in
hp*|HP*)
    case $CC in
    cc|*/cc) CC="$CC -Ae";;
    esac;;
esac

AC_SUBST(LIBRARY)
AC_MSG_CHECKING(LIBRARY)
if test -z "$LIBRARY"
then
	LIBRARY='libpython$(VERSION)$(ABIFLAGS).a'
fi
AC_MSG_RESULT($LIBRARY)

# LDLIBRARY is the name of the library to link against (as opposed to the
# name of the library into which to insert object files). BLDLIBRARY is also
# the library to link against, usually. On Mac OS X frameworks, BLDLIBRARY
# is blank as the main program is not linked directly against LDLIBRARY.
# LDLIBRARYDIR is the path to LDLIBRARY, which is made in a subdirectory. On
# systems without shared libraries, LDLIBRARY is the same as LIBRARY
# (defined in the Makefiles). On Cygwin LDLIBRARY is the import library,
# DLLLIBRARY is the shared (i.e., DLL) library.
#
# RUNSHARED is used to run shared python without installed libraries
#
# INSTSONAME is the name of the shared library that will be use to install
# on the system - some systems like version suffix, others don't
#
# LDVERSION is the shared library version number, normally the Python version
# with the ABI build flags appended.
AC_SUBST(LDLIBRARY)
AC_SUBST(DLLLIBRARY)
AC_SUBST(BLDLIBRARY)
AC_SUBST(PY3LIBRARY)
AC_SUBST(LDLIBRARYDIR)
AC_SUBST(INSTSONAME)
AC_SUBST(RUNSHARED)
AC_SUBST(LDVERSION)
LDLIBRARY="$LIBRARY"
BLDLIBRARY='$(LDLIBRARY)'
INSTSONAME='$(LDLIBRARY)'
DLLLIBRARY=''
LDLIBRARYDIR=''
RUNSHARED=''
LDVERSION="$VERSION"

# LINKCC is the command that links the python executable -- default is $(CC).
# If CXX is set, and if it is needed to link a main function that was
# compiled with CXX, LINKCC is CXX instead. Always using CXX is undesirable:
# python might then depend on the C++ runtime
AC_SUBST(LINKCC)
AC_MSG_CHECKING(LINKCC)
if test -z "$LINKCC"
then
	LINKCC='$(PURIFY) $(MAINCC)'
	case $ac_sys_system in
	QNX*)
	   # qcc must be used because the other compilers do not
	   # support -N.
	   LINKCC=qcc;;
	esac
fi
AC_MSG_RESULT($LINKCC)

# EXPORTSYMS holds the list of exported symbols for AIX.
# EXPORTSFROM holds the module name exporting symbols on AIX.
EXPORTSYMS=
EXPORTSFROM=
AC_SUBST(EXPORTSYMS)
AC_SUBST(EXPORTSFROM)
AC_MSG_CHECKING(EXPORTSYMS)
case $ac_sys_system in
AIX*)
	EXPORTSYMS="Modules/python.exp"
	EXPORTSFROM=. # the main executable
	;;
esac
AC_MSG_RESULT($EXPORTSYMS)

# GNULD is set to "yes" if the GNU linker is used.  If this goes wrong
# make sure we default having it set to "no": this is used by
# distutils.unixccompiler to know if it should add --enable-new-dtags
# to linker command lines, and failing to detect GNU ld simply results
# in the same behaviour as before.
AC_SUBST(GNULD)
AC_MSG_CHECKING(for GNU ld)
ac_prog=ld
if test "$GCC" = yes; then
       ac_prog=`$CC -print-prog-name=ld`
fi
case `"$ac_prog" -V 2>&1 < /dev/null` in
      *GNU*)
          GNULD=yes;;
      *)
          GNULD=no;;
esac
AC_MSG_RESULT($GNULD)

AC_MSG_CHECKING(for --enable-shared)
AC_ARG_ENABLE(shared,
              AS_HELP_STRING([--enable-shared], [enable building a shared Python library (default is no)]))

if test -z "$enable_shared"
then
  case $ac_sys_system in
  CYGWIN*)
    enable_shared="yes";;
  *)
    enable_shared="no";;
  esac
fi
AC_MSG_RESULT($enable_shared)

# --with-static-libpython
STATIC_LIBPYTHON=1
AC_MSG_CHECKING(for --with-static-libpython)
AC_ARG_WITH(static-libpython,
  AS_HELP_STRING([--without-static-libpython],
                 [do not build libpythonMAJOR.MINOR.a and do not install python.o (default is yes)]),
[
if test "$withval" = no
then
  AC_MSG_RESULT(no);
  STATIC_LIBPYTHON=0
else
  AC_MSG_RESULT(yes);
fi],
[AC_MSG_RESULT(yes)])
AC_SUBST(STATIC_LIBPYTHON)

AC_MSG_CHECKING(for --enable-profiling)
AC_ARG_ENABLE(profiling,
              AS_HELP_STRING([--enable-profiling], [enable C-level code profiling with gprof (default is no)]))
if test "x$enable_profiling" = xyes; then
  ac_save_cc="$CC"
  CC="$CC -pg"
  AC_LINK_IFELSE([AC_LANG_SOURCE([[int main(void) { return 0; }]])],
    [],
    [enable_profiling=no])
  CC="$ac_save_cc"
else
  enable_profiling=no
fi
AC_MSG_RESULT($enable_profiling)

if test "x$enable_profiling" = xyes; then
  BASECFLAGS="-pg $BASECFLAGS"
  LDFLAGS="-pg $LDFLAGS"
fi

AC_MSG_CHECKING(LDLIBRARY)

# MacOSX framework builds need more magic. LDLIBRARY is the dynamic
# library that we build, but we do not want to link against it (we
# will find it with a -framework option). For this reason there is an
# extra variable BLDLIBRARY against which Python and the extension
# modules are linked, BLDLIBRARY. This is normally the same as
# LDLIBRARY, but empty for MacOSX framework builds.
if test "$enable_framework"
then
  LDLIBRARY='$(PYTHONFRAMEWORKDIR)/Versions/$(VERSION)/$(PYTHONFRAMEWORK)'
  RUNSHARED=DYLD_FRAMEWORK_PATH=`pwd`${DYLD_FRAMEWORK_PATH:+:${DYLD_FRAMEWORK_PATH}}
  BLDLIBRARY=''
else
  BLDLIBRARY='$(LDLIBRARY)'
fi

# Other platforms follow
if test $enable_shared = "yes"; then
  PY_ENABLE_SHARED=1
  AC_DEFINE(Py_ENABLE_SHARED, 1, [Defined if Python is built as a shared library.])
  case $ac_sys_system in
    CYGWIN*)
          LDLIBRARY='libpython$(LDVERSION).dll.a'
          DLLLIBRARY='libpython$(LDVERSION).dll'
          ;;
    SunOS*)
	  LDLIBRARY='libpython$(LDVERSION).so'
	  BLDLIBRARY='-Wl,-R,$(LIBDIR) -L. -lpython$(LDVERSION)'
	  RUNSHARED=LD_LIBRARY_PATH=`pwd`${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}
	  INSTSONAME="$LDLIBRARY".$SOVERSION
	  if test "$with_pydebug" != yes
	  then
	      PY3LIBRARY=libpython3.so
	  fi
          ;;
    Linux*|GNU*|NetBSD*|FreeBSD*|DragonFly*|OpenBSD*|VxWorks*)
	  LDLIBRARY='libpython$(LDVERSION).so'
	  BLDLIBRARY='-L. -lpython$(LDVERSION)'
	  RUNSHARED=LD_LIBRARY_PATH=`pwd`${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}
	  INSTSONAME="$LDLIBRARY".$SOVERSION
	  if test "$with_pydebug" != yes
          then
	      PY3LIBRARY=libpython3.so
	  fi
	  ;;
    hp*|HP*)
	  case `uname -m` in
		ia64)
			LDLIBRARY='libpython$(LDVERSION).so'
			;;
		*)
			LDLIBRARY='libpython$(LDVERSION).sl'
			;;
	  esac
	  BLDLIBRARY='-Wl,+b,$(LIBDIR) -L. -lpython$(LDVERSION)'
	  RUNSHARED=SHLIB_PATH=`pwd`${SHLIB_PATH:+:${SHLIB_PATH}}
	  ;;
    Darwin*)
    	LDLIBRARY='libpython$(LDVERSION).dylib'
	BLDLIBRARY='-L. -lpython$(LDVERSION)'
	RUNSHARED=DYLD_LIBRARY_PATH=`pwd`${DYLD_LIBRARY_PATH:+:${DYLD_LIBRARY_PATH}}
	;;
    AIX*)
	LDLIBRARY='libpython$(LDVERSION).so'
	RUNSHARED=LIBPATH=`pwd`${LIBPATH:+:${LIBPATH}}
	;;

  esac
else # shared is disabled
  PY_ENABLE_SHARED=0
  case $ac_sys_system in
    CYGWIN*)
          BLDLIBRARY='$(LIBRARY)'
          LDLIBRARY='libpython$(LDVERSION).dll.a'
          ;;
  esac
fi

if test "$cross_compiling" = yes; then
	RUNSHARED=
fi

AC_ARG_VAR([HOSTRUNNER], [Program to run CPython for the host platform])
if test -z "$HOSTRUNNER"
then
  AS_CASE([$ac_sys_system/$ac_sys_emscripten_target],
    [Emscripten/node*], [
      AC_PATH_TOOL([NODE], [node], [node])
      HOSTRUNNER="$NODE"
      # bigint for ctypes c_longlong, c_longdouble
      # no longer available in Node 16
      AC_CACHE_CHECK([for node --experimental-wasm-bigint], [ac_cv_tool_node_wasm_bigint], [
        if $NODE -v --experimental-wasm-bigint > /dev/null 2>&1; then
          ac_cv_tool_node_wasm_bigint=yes
        else
          ac_cv_tool_node_wasm_bigint=no
        fi
      ])
      AS_VAR_IF([ac_cv_tool_node_wasm_bigint], [yes], [
        AS_VAR_APPEND([HOSTRUNNER], [" --experimental-wasm-bigint"])
      ])

      AS_VAR_IF([enable_wasm_pthreads], [yes], [
        AS_VAR_APPEND([HOSTRUNNER], [" --experimental-wasm-threads"])
        # no longer available in Node 16
        AC_CACHE_CHECK([for node --experimental-wasm-bulk-memory], [ac_cv_tool_node_wasm_bulk_memory], [
          if $NODE -v --experimental-wasm-bulk-memory > /dev/null 2>&1; then
            ac_cv_tool_node_wasm_bulk_memory=yes
          else
            ac_cv_tool_node_wasm_bulk_memory=no
          fi
        ])
        AS_VAR_IF([ac_cv_tool_node_wasm_bulk_memory], [yes], [
          AS_VAR_APPEND([HOSTRUNNER], [" --experimental-wasm-bulk-memory"])
        ])
      ])

      AS_VAR_IF([host_cpu], [wasm64], [AS_VAR_APPEND([HOSTRUNNER], [" --experimental-wasm-memory64"])])
    ],
    dnl TODO: support other WASI runtimes
    dnl wasmtime starts the proces with "/" as CWD. For OOT builds add the
    dnl directory containing _sysconfigdata to PYTHONPATH.
    [WASI/*], [HOSTRUNNER='wasmtime run --env PYTHONPATH=/$(shell realpath --relative-to $(abs_srcdir) $(abs_builddir))/$(shell cat pybuilddir.txt) --mapdir /::$(srcdir) --'],
    [HOSTRUNNER='']
  )
fi
AC_SUBST([HOSTRUNNER])
AC_MSG_CHECKING([HOSTRUNNER])
AC_MSG_RESULT([$HOSTRUNNER])

if test -n "$HOSTRUNNER"; then
  dnl Pass hostrunner variable as env var in order to expand shell expressions.
  PYTHON_FOR_BUILD="_PYTHON_HOSTRUNNER='$HOSTRUNNER' $PYTHON_FOR_BUILD"
fi

AC_MSG_RESULT($LDLIBRARY)

# LIBRARY_DEPS, LINK_PYTHON_OBJS and LINK_PYTHON_DEPS variable
AS_CASE([$ac_sys_system/$ac_sys_emscripten_target],
  [Emscripten/browser*], [LIBRARY_DEPS='$(PY3LIBRARY) $(WASM_STDLIB) python.html python.worker.js'],
  [LIBRARY_DEPS='$(PY3LIBRARY) $(EXPORTSYMS)']
)
LINK_PYTHON_DEPS='$(LIBRARY_DEPS)'
if test "$PY_ENABLE_SHARED" = 1 || test "$enable_framework" ; then
    LIBRARY_DEPS="\$(LDLIBRARY) $LIBRARY_DEPS"
    if test "$STATIC_LIBPYTHON" = 1; then
        LIBRARY_DEPS="\$(LIBRARY) $LIBRARY_DEPS"
    fi
    # Link Python program to the shared library
    LINK_PYTHON_OBJS='$(BLDLIBRARY)'
else
    if test "$STATIC_LIBPYTHON" = 0; then
        # Build Python needs object files but don't need to build
        # Python static library
        LINK_PYTHON_DEPS="$LIBRARY_DEPS \$(LIBRARY_OBJS)"
    fi
    LIBRARY_DEPS="\$(LIBRARY) $LIBRARY_DEPS"
    # Link Python program to object files
    LINK_PYTHON_OBJS='$(LIBRARY_OBJS)'
fi
AC_SUBST(LIBRARY_DEPS)
AC_SUBST(LINK_PYTHON_DEPS)
AC_SUBST(LINK_PYTHON_OBJS)

# ar program
AC_SUBST(AR)
AC_CHECK_TOOLS(AR, ar aal, ar)

# tweak ARFLAGS only if the user didn't set it on the command line
AC_SUBST(ARFLAGS)
if test -z "$ARFLAGS"
then
        ARFLAGS="rcs"
fi

AC_CHECK_TOOLS([READELF], [readelf], [:])
if test "$cross_compiling" = yes; then
    case "$READELF" in
	readelf|:)
	AC_MSG_ERROR([readelf for the host is required for cross builds])
	;;
    esac
fi
AC_SUBST(READELF)


case $MACHDEP in
hp*|HP*)
	# install -d does not work on HP-UX
	if test -z "$INSTALL"
	then
		INSTALL="${srcdir}/install-sh -c"
	fi
esac
AC_PROG_INSTALL
AC_PROG_MKDIR_P

# Not every filesystem supports hard links
AC_SUBST(LN)
if test -z "$LN" ; then
	case $ac_sys_system in
		CYGWIN*) LN="ln -s";;
		*) LN=ln;;
	esac
fi

# For calculating the .so ABI tag.
AC_SUBST(ABIFLAGS)
ABIFLAGS=""

# Check for --with-pydebug
AC_MSG_CHECKING(for --with-pydebug)
AC_ARG_WITH(pydebug,
            AS_HELP_STRING([--with-pydebug], [build with Py_DEBUG defined (default is no)]),
[
if test "$withval" != no
then
  AC_DEFINE(Py_DEBUG, 1,
  [Define if you want to build an interpreter with many run-time checks.])
  AC_MSG_RESULT(yes);
  Py_DEBUG='true'
  ABIFLAGS="${ABIFLAGS}d"
else AC_MSG_RESULT(no); Py_DEBUG='false'
fi],
[AC_MSG_RESULT(no)])

# Check for --with-trace-refs
# --with-trace-refs
AC_MSG_CHECKING(for --with-trace-refs)
AC_ARG_WITH(trace-refs,
  AS_HELP_STRING(
    [--with-trace-refs],
    [enable tracing references for debugging purpose (default is no)]),,
  with_trace_refs=no)
AC_MSG_RESULT($with_trace_refs)

if test "$with_trace_refs" = "yes"
then
  AC_DEFINE(Py_TRACE_REFS, 1, [Define if you want to enable tracing references for debugging purpose])
fi


# Check for --enable-pystats
AC_MSG_CHECKING([for --enable-pystats])
AC_ARG_ENABLE([pystats],
  [AS_HELP_STRING(
    [--enable-pystats],
    [enable internal statistics gathering (default is no)])],,
  [enable_pystats=no]
)
AC_MSG_RESULT([$enable_pystats])

AS_VAR_IF([enable_pystats], [yes], [
  AC_DEFINE([Py_STATS], [1], [Define if you want to enable internal statistics gathering.])
])

# Check for --with-assertions.
# This allows enabling assertions without Py_DEBUG.
assertions='false'
AC_MSG_CHECKING(for --with-assertions)
AC_ARG_WITH(assertions,
            AS_HELP_STRING([--with-assertions],[build with C assertions enabled (default is no)]),
[
if test "$withval" != no
then
  assertions='true'
fi],
[])
if test "$assertions" = 'true'; then
  AC_MSG_RESULT(yes)
elif test "$Py_DEBUG" = 'true'; then
  assertions='true'
  AC_MSG_RESULT(implied by --with-pydebug)
else
  AC_MSG_RESULT(no)
fi

# Enable optimization flags
AC_SUBST(DEF_MAKE_ALL_RULE)
AC_SUBST(DEF_MAKE_RULE)
Py_OPT='false'
AC_MSG_CHECKING(for --enable-optimizations)
AC_ARG_ENABLE(optimizations, AS_HELP_STRING(
                [--enable-optimizations],
                [enable expensive, stable optimizations (PGO, etc.) (default is no)]),
[
if test "$enableval" != no
then
  Py_OPT='true'
  AC_MSG_RESULT(yes);
else
  Py_OPT='false'
  AC_MSG_RESULT(no);
fi],
[AC_MSG_RESULT(no)])

if test "$Py_OPT" = 'true' ; then
  # Intentionally not forcing Py_LTO='true' here.  Too many toolchains do not
  # compile working code using it and both test_distutils and test_gdb are
  # broken when you do manage to get a toolchain that works with it.  People
  # who want LTO need to use --with-lto themselves.
  DEF_MAKE_ALL_RULE="profile-opt"
  REQUIRE_PGO="yes"
  DEF_MAKE_RULE="build_all"
  case $CC in
    *gcc*)
      AX_CHECK_COMPILE_FLAG([-fno-semantic-interposition],[
      CFLAGS_NODIST="$CFLAGS_NODIST -fno-semantic-interposition"
      LDFLAGS_NODIST="$LDFLAGS_NODIST -fno-semantic-interposition"
      ])
      ;;
  esac
elif test "$ac_sys_system" = "Emscripten" -o "$ac_sys_system" = "WASI"; then
  dnl Emscripten does not support shared extensions yet. Build
  dnl "python.[js,wasm]", "pybuilddir.txt", and "platform" files.
  DEF_MAKE_ALL_RULE="build_wasm"
  REQUIRE_PGO="no"
  DEF_MAKE_RULE="all"
else
  DEF_MAKE_ALL_RULE="build_all"
  REQUIRE_PGO="no"
  DEF_MAKE_RULE="all"
fi

AC_ARG_VAR(PROFILE_TASK, Python args for PGO generation task)
AC_MSG_CHECKING(PROFILE_TASK)
if test -z "$PROFILE_TASK"
then
	PROFILE_TASK='-m test --pgo --timeout=$(TESTTIMEOUT)'
fi
AC_MSG_RESULT($PROFILE_TASK)

# Make llvm-related checks work on systems where llvm tools are not installed with their
# normal names in the default $PATH (ie: Ubuntu).  They exist under the
# non-suffixed name in their versioned llvm directory.

llvm_bin_dir=''
llvm_path="${PATH}"
if test "${CC}" = "clang"
then
  clang_bin=`which clang`
  # Some systems install clang elsewhere as a symlink to the real path
  # which is where the related llvm tools are located.
  if test -L "${clang_bin}"
  then
    clang_dir=`dirname "${clang_bin}"`
    clang_bin=`readlink "${clang_bin}"`
    llvm_bin_dir="${clang_dir}/"`dirname "${clang_bin}"`
    llvm_path="${llvm_path}${PATH_SEPARATOR}${llvm_bin_dir}"
  fi
fi

# Enable LTO flags
AC_MSG_CHECKING(for --with-lto)
AC_ARG_WITH(lto, AS_HELP_STRING([--with-lto=@<:@full|thin|no|yes@:>@], [enable Link-Time-Optimization in any build (default is no)]),
[
case "$withval" in
    full)
        Py_LTO='true'
        Py_LTO_POLICY='full'
        AC_MSG_RESULT(yes)
        ;;
    thin)
        Py_LTO='true'
        Py_LTO_POLICY='thin'
        AC_MSG_RESULT(yes)
        ;;
    yes)
        Py_LTO='true'
        Py_LTO_POLICY='default'
        AC_MSG_RESULT(yes)
        ;;
    no)
        Py_LTO='false'
        AC_MSG_RESULT(no)
        ;;
    *)
        Py_LTO='false'
        AC_MSG_ERROR([unknown lto option: '$withval'])
        ;;
esac
],
[AC_MSG_RESULT(no)])
if test "$Py_LTO" = 'true' ; then
  case $CC in
    *clang*)
      LDFLAGS_NOLTO="-fno-lto"
      dnl Clang linker requires -flto in order to link objects with LTO information.
      dnl Thin LTO is faster and works for object files with full LTO information, too.
      AX_CHECK_COMPILE_FLAG([-flto=thin],[LDFLAGS_NOLTO="-flto=thin"],[LDFLAGS_NOLTO="-flto"])
      AC_SUBST(LLVM_AR)
      AC_PATH_TOOL(LLVM_AR, llvm-ar, '', ${llvm_path})
      AC_SUBST(LLVM_AR_FOUND)
      if test -n "${LLVM_AR}" -a -x "${LLVM_AR}"
      then
        LLVM_AR_FOUND="found"
      else
        LLVM_AR_FOUND="not-found"
      fi
      if test "$ac_sys_system" = "Darwin" -a "${LLVM_AR_FOUND}" = "not-found"
      then
        # The Apple-supplied ar in Xcode or the Command Line Tools is apparently sufficient
        found_llvm_ar=`/usr/bin/xcrun -find ar 2>/dev/null`
        if test -n "${found_llvm_ar}"
        then
          LLVM_AR='/usr/bin/xcrun ar'
          LLVM_AR_FOUND=found
          AC_MSG_NOTICE([llvm-ar found via xcrun: ${LLVM_AR}])
        fi
      fi
      if test $LLVM_AR_FOUND = not-found
      then
        LLVM_PROFR_ERR=yes
        AC_MSG_ERROR([llvm-ar is required for a --with-lto build with clang but could not be found.])
      else
        LLVM_AR_ERR=no
      fi
      AR="${LLVM_AR}"
      case $ac_sys_system in
        Darwin*)
          # Any changes made here should be reflected in the GCC+Darwin case below
          if test $Py_LTO_POLICY = default
          then
            LTOFLAGS="-flto -Wl,-export_dynamic"
            LTOCFLAGS="-flto"
          else
            LTOFLAGS="-flto=${Py_LTO_POLICY} -Wl,-export_dynamic"
            LTOCFLAGS="-flto=${Py_LTO_POLICY}"
          fi
          ;;
        *)
          if test $Py_LTO_POLICY = default
          then
            LTOFLAGS="-flto"
          else
            LTOFLAGS="-flto=${Py_LTO_POLICY}"
          fi
          ;;
      esac
      ;;
    *emcc*)
      if test "$Py_LTO_POLICY" != "default"; then
        AC_MSG_ERROR([emcc supports only default lto.])
      fi
      LTOFLAGS="-flto"
      LTOCFLAGS="-flto"
      ;;
    *gcc*)
      if test $Py_LTO_POLICY = thin
      then
        AC_MSG_ERROR([thin lto is not supported under gcc compiler.])
      fi
      dnl flag to disable lto during linking
      LDFLAGS_NOLTO="-fno-lto"
      case $ac_sys_system in
        Darwin*)
          LTOFLAGS="-flto -Wl,-export_dynamic"
          LTOCFLAGS="-flto"
          ;;
        *)
          LTOFLAGS="-flto -fuse-linker-plugin -ffat-lto-objects -flto-partition=none"
          ;;
      esac
      ;;
  esac

  if test "$ac_cv_prog_cc_g" = "yes"
  then
      # bpo-30345: Add -g to LDFLAGS when compiling with LTO
      # to get debug symbols.
      LTOFLAGS="$LTOFLAGS -g"
  fi

  CFLAGS_NODIST="$CFLAGS_NODIST ${LTOCFLAGS-$LTOFLAGS}"
  LDFLAGS_NODIST="$LDFLAGS_NODIST $LTOFLAGS"
fi

# Enable PGO flags.
AC_SUBST(PGO_PROF_GEN_FLAG)
AC_SUBST(PGO_PROF_USE_FLAG)
AC_SUBST(LLVM_PROF_MERGER)
AC_SUBST(LLVM_PROF_FILE)
AC_SUBST(LLVM_PROF_ERR)
AC_SUBST(LLVM_PROFDATA)
AC_PATH_TOOL(LLVM_PROFDATA, llvm-profdata, '', ${llvm_path})
AC_SUBST(LLVM_PROF_FOUND)
if test -n "${LLVM_PROFDATA}" -a -x "${LLVM_PROFDATA}"
then
  LLVM_PROF_FOUND="found"
else
  LLVM_PROF_FOUND="not-found"
fi
if test "$ac_sys_system" = "Darwin" -a "${LLVM_PROF_FOUND}" = "not-found"
then
  found_llvm_profdata=`/usr/bin/xcrun -find llvm-profdata 2>/dev/null`
  if test -n "${found_llvm_profdata}"
  then
    # llvm-profdata isn't directly in $PATH in some cases.
    # https://apple.stackexchange.com/questions/197053/
    LLVM_PROFDATA='/usr/bin/xcrun llvm-profdata'
    LLVM_PROF_FOUND=found
    AC_MSG_NOTICE([llvm-profdata found via xcrun: ${LLVM_PROFDATA}])
  fi
fi
LLVM_PROF_ERR=no
case $CC in
  *clang*)
    # Any changes made here should be reflected in the GCC+Darwin case below
    PGO_PROF_GEN_FLAG="-fprofile-instr-generate"
    PGO_PROF_USE_FLAG="-fprofile-instr-use=code.profclangd"
    LLVM_PROF_MERGER="${LLVM_PROFDATA} merge -output=code.profclangd *.profclangr"
    LLVM_PROF_FILE="LLVM_PROFILE_FILE=\"code-%p.profclangr\""
    if test $LLVM_PROF_FOUND = not-found
    then
      LLVM_PROF_ERR=yes
      if test "${REQUIRE_PGO}" = "yes"
      then
        AC_MSG_ERROR([llvm-profdata is required for a --enable-optimizations build but could not be found.])
      fi
    fi
    ;;
  *gcc*)
    case $ac_sys_system in
      Darwin*)
        PGO_PROF_GEN_FLAG="-fprofile-instr-generate"
        PGO_PROF_USE_FLAG="-fprofile-instr-use=code.profclangd"
        LLVM_PROF_MERGER="${LLVM_PROFDATA} merge -output=code.profclangd *.profclangr"
        LLVM_PROF_FILE="LLVM_PROFILE_FILE=\"code-%p.profclangr\""
        if test "${LLVM_PROF_FOUND}" = "not-found"
        then
          LLVM_PROF_ERR=yes
          if test "${REQUIRE_PGO}" = "yes"
	  then
	    AC_MSG_ERROR([llvm-profdata is required for a --enable-optimizations build but could not be found.])
	  fi
        fi
        ;;
      *)
        PGO_PROF_GEN_FLAG="-fprofile-generate"
        PGO_PROF_USE_FLAG="-fprofile-use -fprofile-correction"
        LLVM_PROF_MERGER="true"
        LLVM_PROF_FILE=""
        ;;
    esac
    ;;
  *icc*)
    PGO_PROF_GEN_FLAG="-prof-gen"
    PGO_PROF_USE_FLAG="-prof-use"
    LLVM_PROF_MERGER="true"
    LLVM_PROF_FILE=""
    ;;
esac

# XXX Shouldn't the code above that fiddles with BASECFLAGS and OPT be
# merged with this chunk of code?

# Optimizer/debugger flags
# ------------------------
# (The following bit of code is complicated enough - please keep things
# indented properly.  Just pretend you're editing Python code. ;-)

# There are two parallel sets of case statements below, one that checks to
# see if OPT was set and one that does BASECFLAGS setting based upon
# compiler and platform.  BASECFLAGS tweaks need to be made even if the
# user set OPT.

case $CC in
    *clang*)
        cc_is_clang=1
        ;;
    *)
        if $CC --version 2>&1 | grep -q clang
        then
            cc_is_clang=1
        else
            cc_is_clang=
        fi
esac

# Check if CC supports -Og optimization level
_SAVE_VAR([CFLAGS])
CFLAGS="-Og"
AC_CACHE_CHECK([if $CC supports -Og optimization level],
               [ac_cv_cc_supports_og],
  AC_COMPILE_IFELSE(
    [
      AC_LANG_PROGRAM([[]], [[]])
    ],[
      ac_cv_cc_supports_og=yes
    ],[
      ac_cv_cc_supports_og=no
    ])
)
_RESTORE_VAR([CFLAGS])

# Optimization messes up debuggers, so turn it off for
# debug builds.
PYDEBUG_CFLAGS="-O0"
AS_VAR_IF([ac_cv_cc_supports_og], [yes],
          [PYDEBUG_CFLAGS="-Og"])

# tweak OPT based on compiler and platform, only if the user didn't set
# it on the command line
AC_SUBST(OPT)
AC_SUBST(CFLAGS_ALIASING)
if test "${OPT-unset}" = "unset"
then
    case $GCC in
    yes)
        # For gcc 4.x we need to use -fwrapv so lets check if its supported
        if "$CC" -v --help 2>/dev/null |grep -- -fwrapv > /dev/null; then
           WRAP="-fwrapv"
        fi

        if test -n "${cc_is_clang}"
        then
            # Clang also needs -fwrapv
            WRAP="-fwrapv"
            # bpo-30104: disable strict aliasing to compile correctly dtoa.c,
            # see Makefile.pre.in for more information
            CFLAGS_ALIASING="-fno-strict-aliasing"
        fi

	case $ac_cv_prog_cc_g in
	yes)
	    if test "$Py_DEBUG" = 'true' ; then
		OPT="-g $PYDEBUG_CFLAGS -Wall"
	    else
		OPT="-g $WRAP -O3 -Wall"
	    fi
	    ;;
	*)
	    OPT="-O3 -Wall"
	    ;;
	esac

	case $ac_sys_system in
	    SCO_SV*) OPT="$OPT -m486 -DSCO5"
	    ;;
        esac
	;;

    *)
	OPT="-O"
	;;
    esac
fi

# WASM flags
AS_CASE([$ac_sys_system],
  [Emscripten], [
    dnl build with WASM debug info if either Py_DEBUG is set or the target is
    dnl node-debug or browser-debug.
    AS_VAR_IF([Py_DEBUG], [yes], [wasm_debug=yes], [wasm_debug=no])

    dnl Start with 20 MB and allow to grow
    AS_VAR_APPEND([LDFLAGS_NODIST], [" -sALLOW_MEMORY_GROWTH -sTOTAL_MEMORY=20971520"])

    dnl map int64_t and uint64_t to JS bigint
    AS_VAR_APPEND([LDFLAGS_NODIST], [" -sWASM_BIGINT"])

    dnl Include file system support
    AS_VAR_APPEND([LDFLAGS_NODIST], [" -sFORCE_FILESYSTEM -lidbfs.js -lnodefs.js -lproxyfs.js -lworkerfs.js"])

    AS_VAR_IF([enable_wasm_dynamic_linking], [yes], [
      AS_VAR_APPEND([LINKFORSHARED], [" -sMAIN_MODULE"])
    ])

    AS_VAR_IF([enable_wasm_pthreads], [yes], [
      AS_VAR_APPEND([CFLAGS_NODIST], [" -pthread"])
      AS_VAR_APPEND([LDFLAGS_NODIST], [" -sUSE_PTHREADS"])
      AS_VAR_APPEND([LINKFORSHARED], [" -sPROXY_TO_PTHREAD"])
    ])

    AS_CASE([$ac_sys_emscripten_target],
      [browser*], [
        AS_VAR_IF([ac_sys_emscripten_target], [browser-debug], [wasm_debug=yes])
        AS_VAR_APPEND([LINKFORSHARED], [" --preload-file=\$(WASM_ASSETS_DIR)"])
        WASM_ASSETS_DIR=".\$(prefix)"
        WASM_STDLIB="\$(WASM_ASSETS_DIR)/local/lib/python\$(VERSION)/os.py"
        dnl separate-dwarf does not seem to work in Chrome DevTools Support.
        WASM_LINKFORSHARED_DEBUG="-gsource-map --emit-symbol-map"
      ],
      [node*], [
        AS_VAR_IF([ac_sys_emscripten_target], [node-debug], [wasm_debug=yes])
        AS_VAR_APPEND([LDFLAGS_NODIST], [" -sALLOW_MEMORY_GROWTH -sNODERAWFS"])
        AS_VAR_APPEND([LINKFORSHARED], [" -sEXIT_RUNTIME"])
        WASM_LINKFORSHARED_DEBUG="-gseparate-dwarf --emit-symbol-map"
      ]
    )

    AS_VAR_IF([wasm_debug], [yes], [
      AS_VAR_APPEND([LDFLAGS_NODIST], [" -sASSERTIONS"])
      AS_VAR_APPEND([LINKFORSHARED], [" $WASM_LINKFORSHARED_DEBUG"])
    ], [
      AS_VAR_APPEND([LINKFORSHARED], [" -O2 -g0"])
    ])
  ],
  [WASI], [
    AC_DEFINE([_WASI_EMULATED_SIGNAL], [1], [Define to 1 if you want to emulate signals on WASI])
    AC_DEFINE([_WASI_EMULATED_GETPID], [1], [Define to 1 if you want to emulate getpid() on WASI])
    AC_DEFINE([_WASI_EMULATED_PROCESS_CLOCKS], [1], [Define to 1 if you want to emulate process clocks on WASI])
    LIBS="$LIBS -lwasi-emulated-signal -lwasi-emulated-getpid -lwasi-emulated-process-clocks"
    echo "#define _WASI_EMULATED_SIGNAL 1" >> confdefs.h

    dnl increase initial memory and stack size, move stack first
    dnl https://github.com/WebAssembly/wasi-libc/issues/233
    AS_VAR_APPEND([LDFLAGS_NODIST], [" -z stack-size=524288 -Wl,--stack-first -Wl,--initial-memory=10485760"])
  ]
)

AS_CASE([$enable_wasm_dynamic_linking],
  [yes], [ac_cv_func_dlopen=yes],
  [no], [ac_cv_func_dlopen=no],
  [missing], []
)

AC_SUBST(BASECFLAGS)
AC_SUBST(CFLAGS_NODIST)
AC_SUBST(LDFLAGS_NODIST)
AC_SUBST(LDFLAGS_NOLTO)
AC_SUBST([WASM_ASSETS_DIR])
AC_SUBST([WASM_STDLIB])

# The -arch flags for universal builds on macOS
UNIVERSAL_ARCH_FLAGS=
AC_SUBST(UNIVERSAL_ARCH_FLAGS)

dnl PY_CHECK_CC_WARNING(ENABLE, WARNING, [MSG])
AC_DEFUN([PY_CHECK_CC_WARNING], [
  AS_VAR_PUSHDEF([py_var], [ac_cv_$1_]m4_normalize($2)[_warning])
  AC_CACHE_CHECK(m4_ifblank([$3], [if we can $1 $CC $2 warning], [$3]), [py_var], [
    AS_VAR_COPY([py_cflags], [CFLAGS])
    AS_VAR_APPEND([CFLAGS], ["-W$2 -Werror"])
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[]])],
                      [AS_VAR_SET([py_var], [yes])],
                      [AS_VAR_SET([py_var], [no])])
    AS_VAR_COPY([CFLAGS], [py_cflags])
  ])
  AS_VAR_POPDEF([py_var])
])

# tweak BASECFLAGS based on compiler and platform
case $GCC in
yes)
    CFLAGS_NODIST="$CFLAGS_NODIST -std=c11"

    PY_CHECK_CC_WARNING([enable], [extra], [if we can add -Wextra])
    AS_VAR_IF([ac_cv_enable_extra_warning], [yes],
              [CFLAGS_NODIST="$CFLAGS_NODIST -Wextra"])

    # Python doesn't violate C99 aliasing rules, but older versions of
    # GCC produce warnings for legal Python code.  Enable
    # -fno-strict-aliasing on versions of GCC that support but produce
    # warnings.  See Issue3326
     ac_save_cc="$CC"
     CC="$CC -fno-strict-aliasing"
     save_CFLAGS="$CFLAGS"
     AC_CACHE_CHECK([whether $CC accepts and needs -fno-strict-aliasing],
                    [ac_cv_no_strict_aliasing],
       AC_COMPILE_IFELSE(
         [
	   AC_LANG_PROGRAM([[]], [[]])
	 ],[
	   CC="$ac_save_cc -fstrict-aliasing"
           CFLAGS="$CFLAGS -Werror -Wstrict-aliasing"
           AC_COMPILE_IFELSE(
	     [
	       AC_LANG_PROGRAM([[void f(int **x) {}]],
	         [[double *x; f((int **) &x);]])
	     ],[
	       ac_cv_no_strict_aliasing=no
	     ],[
               ac_cv_no_strict_aliasing=yes
	     ])
	 ],[
	   ac_cv_no_strict_aliasing=no
	 ]))
     CFLAGS="$save_CFLAGS"
     CC="$ac_save_cc"
    AS_VAR_IF([ac_cv_no_strict_aliasing], [yes],
              [BASECFLAGS="$BASECFLAGS -fno-strict-aliasing"])

    # ICC doesn't recognize the option, but only emits a warning
    ## XXX does it emit an unused result warning and can it be disabled?
    AS_CASE([$CC],
            [*icc*], [ac_cv_disable_unused_result_warning=no]
            [PY_CHECK_CC_WARNING([disable], [unused-result])])
    AS_VAR_IF([ac_cv_disable_unused_result_warning], [yes],
              [BASECFLAGS="$BASECFLAGS -Wno-unused-result"
               CFLAGS_NODIST="$CFLAGS_NODIST -Wno-unused-result"])

    PY_CHECK_CC_WARNING([disable], [unused-parameter])
    AS_VAR_IF([ac_cv_disable_unused_parameter_warning], [yes],
              [CFLAGS_NODIST="$CFLAGS_NODIST -Wno-unused-parameter"])

    PY_CHECK_CC_WARNING([disable], [int-conversion])
    AS_VAR_IF([ac_cv_disable_int_conversion], [yes],
              [CFLAGS_NODIST="$CFLAGS_NODIST -Wno-int-conversion"])

    PY_CHECK_CC_WARNING([disable], [missing-field-initializers])
    AS_VAR_IF([ac_cv_disable_missing_field_initializers_warning], [yes],
              [CFLAGS_NODIST="$CFLAGS_NODIST -Wno-missing-field-initializers"])

    PY_CHECK_CC_WARNING([enable], [sign-compare])
    AS_VAR_IF([ac_cv_enable_sign_compare_warning], [yes],
              [BASECFLAGS="$BASECFLAGS -Wsign-compare"])

    PY_CHECK_CC_WARNING([enable], [unreachable-code])
    # Don't enable unreachable code warning in debug mode, since it usually
    # results in non-standard code paths.
    # Issue #24324: Unfortunately, the unreachable code warning does not work
    # correctly on gcc and has been silently removed from the compiler.
    # It is supported on clang but on OS X systems gcc may be an alias
    # for clang.  Try to determine if the compiler is not really gcc and,
    # if so, only then enable the warning.
    if test $ac_cv_enable_unreachable_code_warning = yes && \
        test "$Py_DEBUG" != "true" && \
        test -z "`$CC --version 2>/dev/null | grep 'Free Software Foundation'`"
    then
      BASECFLAGS="$BASECFLAGS -Wunreachable-code"
    else
      ac_cv_enable_unreachable_code_warning=no
    fi

    PY_CHECK_CC_WARNING([enable], [strict-prototypes])
    AS_VAR_IF([ac_cv_enable_strict_prototypes_warning], [yes],
              [CFLAGS_NODIST="$CFLAGS_NODIST -Wstrict-prototypes"])

     ac_save_cc="$CC"
     CC="$CC -Werror=implicit-function-declaration"
     AC_CACHE_CHECK([if we can make implicit function declaration an error in $CC],
                    [ac_cv_enable_implicit_function_declaration_error],
       AC_COMPILE_IFELSE(
         [
	   AC_LANG_PROGRAM([[]], [[]])
	 ],[
           ac_cv_enable_implicit_function_declaration_error=yes
	 ],[
           ac_cv_enable_implicit_function_declaration_error=no
	 ]))
     CC="$ac_save_cc"

    AS_VAR_IF([ac_cv_enable_implicit_function_declaration_error], [yes],
              [CFLAGS_NODIST="$CFLAGS_NODIST -Werror=implicit-function-declaration"])

     ac_save_cc="$CC"
     CC="$CC -fvisibility=hidden"
     AC_CACHE_CHECK([if we can use visibility in $CC], [ac_cv_enable_visibility],
       AC_COMPILE_IFELSE(
         [
	   AC_LANG_PROGRAM([[]], [[]])
	 ],[
           ac_cv_enable_visibility=yes
	 ],[
           ac_cv_enable_visibility=no
	 ]))
     CC="$ac_save_cc"

    AS_VAR_IF([ac_cv_enable_visibility], [yes],
              [CFLAGS_NODIST="$CFLAGS_NODIST -fvisibility=hidden"])

    # if using gcc on alpha, use -mieee to get (near) full IEEE 754
    # support.  Without this, treatment of subnormals doesn't follow
    # the standard.
    case $host in
         alpha*)
                BASECFLAGS="$BASECFLAGS -mieee"
                ;;
    esac

    case $ac_sys_system in
	SCO_SV*)
	    BASECFLAGS="$BASECFLAGS -m486 -DSCO5"
	    ;;

    Darwin*)
        # -Wno-long-double, -no-cpp-precomp, and -mno-fused-madd
        # used to be here, but non-Apple gcc doesn't accept them.
        if test "${CC}" = gcc
        then
            AC_MSG_CHECKING(which compiler should be used)
            case "${UNIVERSALSDK}" in
            */MacOSX10.4u.sdk)
                # Build using 10.4 SDK, force usage of gcc when the
                # compiler is gcc, otherwise the user will get very
                # confusing error messages when building on OSX 10.6
                CC=gcc-4.0
                CPP=cpp-4.0
                ;;
            esac
            AC_MSG_RESULT($CC)
        fi

        LIPO_INTEL64_FLAGS=""
        if test "${enable_universalsdk}"
        then
            case "$UNIVERSAL_ARCHS" in
            32-bit)
               UNIVERSAL_ARCH_FLAGS="-arch ppc -arch i386"
               LIPO_32BIT_FLAGS=""
               ARCH_RUN_32BIT=""
               ;;
            64-bit)
               UNIVERSAL_ARCH_FLAGS="-arch ppc64 -arch x86_64"
               LIPO_32BIT_FLAGS=""
               ARCH_RUN_32BIT="true"
               ;;
            all)
               UNIVERSAL_ARCH_FLAGS="-arch i386 -arch ppc -arch ppc64 -arch x86_64"
               LIPO_32BIT_FLAGS="-extract ppc7400 -extract i386"
               ARCH_RUN_32BIT="/usr/bin/arch -i386 -ppc"
               ;;
            universal2)
               UNIVERSAL_ARCH_FLAGS="-arch arm64 -arch x86_64"
               LIPO_32BIT_FLAGS=""
               LIPO_INTEL64_FLAGS="-extract x86_64"
               ARCH_RUN_32BIT="true"
               ;;
            intel)
               UNIVERSAL_ARCH_FLAGS="-arch i386 -arch x86_64"
               LIPO_32BIT_FLAGS="-extract i386"
               ARCH_RUN_32BIT="/usr/bin/arch -i386"
               ;;
            intel-32)
               UNIVERSAL_ARCH_FLAGS="-arch i386"
               LIPO_32BIT_FLAGS=""
               ARCH_RUN_32BIT=""
               ;;
            intel-64)
               UNIVERSAL_ARCH_FLAGS="-arch x86_64"
               LIPO_32BIT_FLAGS=""
               ARCH_RUN_32BIT="true"
               ;;
            3-way)
               UNIVERSAL_ARCH_FLAGS="-arch i386 -arch ppc -arch x86_64"
               LIPO_32BIT_FLAGS="-extract ppc7400 -extract i386"
               ARCH_RUN_32BIT="/usr/bin/arch -i386 -ppc"
               ;;
            *)
               AC_MSG_ERROR([proper usage is --with-universal-arch=universal2|32-bit|64-bit|all|intel|3-way])
               ;;
            esac

            if test "${UNIVERSALSDK}" != "/"
            then
                CFLAGS="${UNIVERSAL_ARCH_FLAGS} -isysroot ${UNIVERSALSDK} ${CFLAGS}"
                LDFLAGS="${UNIVERSAL_ARCH_FLAGS} -isysroot ${UNIVERSALSDK} ${LDFLAGS}"
                CPPFLAGS="-isysroot ${UNIVERSALSDK} ${CPPFLAGS}"
            else
                CFLAGS="${UNIVERSAL_ARCH_FLAGS} ${CFLAGS}"
                LDFLAGS="${UNIVERSAL_ARCH_FLAGS} ${LDFLAGS}"
            fi
        fi

        # Calculate an appropriate deployment target for this build:
        # The deployment target value is used explicitly to enable certain
        # features are enabled (such as builtin libedit support for readline)
        # through the use of Apple's Availability Macros and is used as a
        # component of the string returned by distutils.get_platform().
        #
        # Use the value from:
        # 1. the MACOSX_DEPLOYMENT_TARGET environment variable if specified
        # 2. the operating system version of the build machine if >= 10.6
        # 3. If running on OS X 10.3 through 10.5, use the legacy tests
        #       below to pick either 10.3, 10.4, or 10.5 as the target.
        # 4. If we are running on OS X 10.2 or earlier, good luck!

        AC_MSG_CHECKING(which MACOSX_DEPLOYMENT_TARGET to use)
        cur_target_major=`sw_vers -productVersion | \
                sed 's/\([[0-9]]*\)\.\([[0-9]]*\).*/\1/'`
        cur_target_minor=`sw_vers -productVersion | \
                sed 's/\([[0-9]]*\)\.\([[0-9]]*\).*/\2/'`
        cur_target="${cur_target_major}.${cur_target_minor}"
        if test ${cur_target_major} -eq 10 && \
           test ${cur_target_minor} -ge 3 && \
           test ${cur_target_minor} -le 5
        then
            # OS X 10.3 through 10.5
            cur_target=10.3
            if test ${enable_universalsdk}
            then
                case "$UNIVERSAL_ARCHS" in
                all|3-way|intel|64-bit)
                    # These configurations were first supported in 10.5
                    cur_target='10.5'
                    ;;
                esac
            else
                if test `/usr/bin/arch` = "i386"
                then
                    # 10.4 was the first release to support Intel archs
                    cur_target="10.4"
                fi
            fi
        fi
        CONFIGURE_MACOSX_DEPLOYMENT_TARGET=${MACOSX_DEPLOYMENT_TARGET-${cur_target}}

        # Make sure that MACOSX_DEPLOYMENT_TARGET is set in the
        # environment with a value that is the same as what we'll use
        # in the Makefile to ensure that we'll get the same compiler
        # environment during configure and build time.
        MACOSX_DEPLOYMENT_TARGET="$CONFIGURE_MACOSX_DEPLOYMENT_TARGET"
        export MACOSX_DEPLOYMENT_TARGET
        EXPORT_MACOSX_DEPLOYMENT_TARGET=''
        AC_MSG_RESULT($MACOSX_DEPLOYMENT_TARGET)

        AC_MSG_CHECKING(if specified universal architectures work)
        AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <stdio.h>]], [[printf("%d", 42);]])],
            [AC_MSG_RESULT(yes)],
            [AC_MSG_RESULT(no)
             AC_MSG_ERROR(check config.log and use the '--with-universal-archs' option)
        ])

        # end of Darwin* tests
        ;;
    esac
    ;;

*)
    case $ac_sys_system in
    OpenUNIX*|UnixWare*)
	BASECFLAGS="$BASECFLAGS -K pentium,host,inline,loop_unroll,alloca "
	;;
    SCO_SV*)
	BASECFLAGS="$BASECFLAGS -belf -Ki486 -DSCO5"
	;;
    esac
    ;;
esac

case "$CC" in
*icc*)
    # ICC needs -fp-model strict or floats behave badly
    CFLAGS_NODIST="$CFLAGS_NODIST -fp-model strict"
    ;;
*xlc*)
    CFLAGS_NODIST="$CFLAGS_NODIST -qalias=noansi -qmaxmem=-1"
    ;;
esac

if test "$assertions" = 'true'; then
  :
else
  OPT="-DNDEBUG $OPT"
fi

if test "$ac_arch_flags"
then
	BASECFLAGS="$BASECFLAGS $ac_arch_flags"
fi

# On some compilers, pthreads are available without further options
# (e.g. MacOS X). On some of these systems, the compiler will not
# complain if unaccepted options are passed (e.g. gcc on Mac OS X).
# So we have to see first whether pthreads are available without
# options before we can check whether -Kpthread improves anything.
AC_CACHE_CHECK([whether pthreads are available without options],
               [ac_cv_pthread_is_default],
[AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <stdio.h>
#include <pthread.h>

void* routine(void* p){return NULL;}

int main(void){
  pthread_t p;
  if(pthread_create(&p,NULL,routine,NULL)!=0)
    return 1;
  (void)pthread_detach(p);
  return 0;
}
]])],[
  ac_cv_pthread_is_default=yes
  ac_cv_kthread=no
  ac_cv_pthread=no
],[ac_cv_pthread_is_default=no],[ac_cv_pthread_is_default=no])
])


if test $ac_cv_pthread_is_default = yes
then
  ac_cv_kpthread=no
else
# -Kpthread, if available, provides the right #defines
# and linker options to make pthread_create available
# Some compilers won't report that they do not support -Kpthread,
# so we need to run a program to see whether it really made the
# function available.
AC_CACHE_CHECK([whether $CC accepts -Kpthread], [ac_cv_kpthread],
[ac_save_cc="$CC"
CC="$CC -Kpthread"
AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <stdio.h>
#include <pthread.h>

void* routine(void* p){return NULL;}

int main(void){
  pthread_t p;
  if(pthread_create(&p,NULL,routine,NULL)!=0)
    return 1;
  (void)pthread_detach(p);
  return 0;
}
]])],[ac_cv_kpthread=yes],[ac_cv_kpthread=no],[ac_cv_kpthread=no])
CC="$ac_save_cc"])
fi

if test $ac_cv_kpthread = no -a $ac_cv_pthread_is_default = no
then
# -Kthread, if available, provides the right #defines
# and linker options to make pthread_create available
# Some compilers won't report that they do not support -Kthread,
# so we need to run a program to see whether it really made the
# function available.
AC_CACHE_CHECK([whether $CC accepts -Kthread], [ac_cv_kthread],
[ac_save_cc="$CC"
CC="$CC -Kthread"
AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <stdio.h>
#include <pthread.h>

void* routine(void* p){return NULL;}

int main(void){
  pthread_t p;
  if(pthread_create(&p,NULL,routine,NULL)!=0)
    return 1;
  (void)pthread_detach(p);
  return 0;
}
]])],[ac_cv_kthread=yes],[ac_cv_kthread=no],[ac_cv_kthread=no])
CC="$ac_save_cc"])
fi

if test $ac_cv_kthread = no -a $ac_cv_pthread_is_default = no
then
# -pthread, if available, provides the right #defines
# and linker options to make pthread_create available
# Some compilers won't report that they do not support -pthread,
# so we need to run a program to see whether it really made the
# function available.
AC_CACHE_CHECK([whether $CC accepts -pthread], [ac_cv_pthread],
[ac_save_cc="$CC"
CC="$CC -pthread"
AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <stdio.h>
#include <pthread.h>

void* routine(void* p){return NULL;}

int main(void){
  pthread_t p;
  if(pthread_create(&p,NULL,routine,NULL)!=0)
    return 1;
  (void)pthread_detach(p);
  return 0;
}
]])],[ac_cv_pthread=yes],[ac_cv_pthread=no],[ac_cv_pthread=no])
CC="$ac_save_cc"])
fi

# If we have set a CC compiler flag for thread support then
# check if it works for CXX, too.
ac_cv_cxx_thread=no
if test ! -z "$CXX"
then
AC_MSG_CHECKING(whether $CXX also accepts flags for thread support)
ac_save_cxx="$CXX"

if test "$ac_cv_kpthread" = "yes"
then
  CXX="$CXX -Kpthread"
  ac_cv_cxx_thread=yes
elif test "$ac_cv_kthread" = "yes"
then
  CXX="$CXX -Kthread"
  ac_cv_cxx_thread=yes
elif test "$ac_cv_pthread" = "yes"
then
  CXX="$CXX -pthread"
  ac_cv_cxx_thread=yes
fi

if test $ac_cv_cxx_thread = yes
then
  echo 'void foo();int main(){foo();}void foo(){}' > conftest.$ac_ext
  $CXX -c conftest.$ac_ext 2>&5
  if $CXX -o conftest$ac_exeext conftest.$ac_objext 2>&5 \
     && test -s conftest$ac_exeext && ./conftest$ac_exeext
  then
    ac_cv_cxx_thread=yes
  else
    ac_cv_cxx_thread=no
  fi
  rm -fr conftest*
fi
AC_MSG_RESULT($ac_cv_cxx_thread)
fi
CXX="$ac_save_cxx"

dnl # check for ANSI or K&R ("traditional") preprocessor
dnl AC_MSG_CHECKING(for C preprocessor type)
dnl AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
dnl #define spam(name, doc) {#name, &name, #name "() -- " doc}
dnl int foo;
dnl struct {char *name; int *addr; char *doc;} desc = spam(foo, "something");
dnl ]], [[;]])],[cpp_type=ansi],[AC_DEFINE(HAVE_OLD_CPP) cpp_type=traditional])
dnl AC_MSG_RESULT($cpp_type)

dnl autoconf 2.71 deprecates STDC_HEADERS, keep for backwards compatibility
dnl assume C99 compilers provide ANSI C headers
AC_DEFINE(STDC_HEADERS, 1, [Define to 1 if you have the ANSI C header files.])

# checks for header files
AC_CHECK_HEADERS([ \
  alloca.h asm/types.h bluetooth.h conio.h crypt.h direct.h dlfcn.h endian.h errno.h fcntl.h grp.h \
  ieeefp.h io.h langinfo.h libintl.h libutil.h linux/auxvec.h sys/auxv.h linux/limits.h linux/memfd.h \
  linux/random.h linux/soundcard.h \
  linux/tipc.h linux/wait.h netdb.h netinet/in.h netpacket/packet.h poll.h process.h pthread.h pty.h \
  sched.h setjmp.h shadow.h signal.h spawn.h stropts.h sys/audioio.h sys/bsdtty.h sys/devpoll.h \
  sys/endian.h sys/epoll.h sys/event.h sys/eventfd.h sys/file.h sys/ioctl.h sys/kern_control.h \
  sys/loadavg.h sys/lock.h sys/memfd.h sys/mkdev.h sys/mman.h sys/modem.h sys/param.h sys/poll.h \
  sys/random.h sys/resource.h sys/select.h sys/sendfile.h sys/socket.h sys/soundcard.h sys/stat.h \
  sys/statvfs.h sys/sys_domain.h sys/syscall.h sys/sysmacros.h sys/termio.h sys/time.h sys/times.h \
  sys/types.h sys/uio.h sys/un.h sys/utsname.h sys/wait.h sys/xattr.h sysexits.h syslog.h \
  termios.h util.h utime.h utmp.h \
])
AC_HEADER_DIRENT
AC_HEADER_MAJOR

# bluetooth/bluetooth.h has been known to not compile with -std=c99.
# http://permalink.gmane.org/gmane.linux.bluez.kernel/22294
SAVE_CFLAGS=$CFLAGS
CFLAGS="-std=c99 $CFLAGS"
AC_CHECK_HEADERS(bluetooth/bluetooth.h)
CFLAGS=$SAVE_CFLAGS

# On Darwin (OS X) net/if.h requires sys/socket.h to be imported first.
AC_CHECK_HEADERS([net/if.h], [], [],
[#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
])

# On Linux, netlink.h requires asm/types.h
AC_CHECK_HEADERS(linux/netlink.h,,,[
#ifdef HAVE_ASM_TYPES_H
#include <asm/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
])

# On Linux, qrtr.h requires asm/types.h
AC_CHECK_HEADERS(linux/qrtr.h,,,[
#ifdef HAVE_ASM_TYPES_H
#include <asm/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
])

AC_CHECK_HEADERS(linux/vm_sockets.h,,,[
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
])

# On Linux, can.h, can/bcm.h, can/j1939.h, can/raw.h require sys/socket.h
# On NetBSD, netcan/can.h requires sys/socket.h
AC_CHECK_HEADERS(linux/can.h linux/can/bcm.h linux/can/j1939.h linux/can/raw.h netcan/can.h,,,[
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
])

# checks for typedefs
AC_CACHE_CHECK([for clock_t in time.h], [ac_cv_clock_t_time_h], [
  AC_EGREP_HEADER([clock_t], [time.h], [ac_cv_clock_t_time_h=yes], [ac_cv_clock_t_time_h=no])
])
dnl checks for "no"
AS_VAR_IF([ac_cv_clock_t_time_h], [no], [
  AC_DEFINE(clock_t, long, [Define to 'long' if <time.h> doesn't define.])
])

AC_CACHE_CHECK([for makedev], [ac_cv_func_makedev], [
AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#if defined(MAJOR_IN_MKDEV)
#include <sys/mkdev.h>
#elif defined(MAJOR_IN_SYSMACROS)
#include <sys/sysmacros.h>
#else
#include <sys/types.h>
#endif
]], [[
  makedev(0, 0) ]])
],[ac_cv_func_makedev=yes],[ac_cv_func_makedev=no])
])

AS_VAR_IF([ac_cv_func_makedev], [yes], [
    AC_DEFINE(HAVE_MAKEDEV, 1, [Define this if you have the makedev macro.])
])

# byte swapping
AC_CACHE_CHECK([for le64toh], [ac_cv_func_le64toh], [
AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#ifdef HAVE_ENDIAN_H
#include <endian.h>
#elif defined(HAVE_SYS_ENDIAN_H)
#include <sys/endian.h>
#endif
]], [[
   le64toh(1) ]])
],[ac_cv_func_le64toh=yes],[ac_cv_func_le64toh=no])
])

AS_VAR_IF([ac_cv_func_le64toh], [yes], [
    AC_DEFINE(HAVE_HTOLE64, 1, [Define this if you have le64toh()])
])

use_lfs=yes
# Don't use largefile support for GNU/Hurd
case $ac_sys_system in GNU*)
  use_lfs=no
esac

if test "$use_lfs" = "yes"; then
# Two defines needed to enable largefile support on various platforms
# These may affect some typedefs
case $ac_sys_system/$ac_sys_release in
AIX*)
    AC_DEFINE(_LARGE_FILES, 1,
    [This must be defined on AIX systems to enable large file support.])
    ;;
esac
AC_DEFINE(_LARGEFILE_SOURCE, 1,
[This must be defined on some systems to enable large file support.])
AC_DEFINE(_FILE_OFFSET_BITS, 64,
[This must be set to 64 on some systems to enable large file support.])
fi

# Add some code to confdefs.h so that the test for off_t works on SCO
cat >> confdefs.h <<\EOF
#if defined(SCO_DS)
#undef _OFF_T
#endif
EOF

# Type availability checks
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_DEFINE_UNQUOTED([RETSIGTYPE],[void],[assume C89 semantics that RETSIGTYPE is always void])
AC_TYPE_SIZE_T
AC_TYPE_UID_T

AC_CHECK_TYPE(ssize_t,
  AC_DEFINE(HAVE_SSIZE_T, 1, [Define if your compiler provides ssize_t]),,)
AC_CHECK_TYPE(__uint128_t,
  AC_DEFINE(HAVE_GCC_UINT128_T, 1, [Define if your compiler provides __uint128_t]),,)

# Sizes and alignments of various common basic types
# ANSI C requires sizeof(char) == 1, so no need to check it
AC_CHECK_SIZEOF(int, 4)
AC_CHECK_SIZEOF(long, 4)
AC_CHECK_ALIGNOF(long)
AC_CHECK_SIZEOF(long long, 8)
AC_CHECK_SIZEOF(void *, 4)
AC_CHECK_SIZEOF(short, 2)
AC_CHECK_SIZEOF(float, 4)
AC_CHECK_SIZEOF(double, 8)
AC_CHECK_SIZEOF(fpos_t, 4)
AC_CHECK_SIZEOF(size_t, 4)
AC_CHECK_ALIGNOF(size_t)
AC_CHECK_SIZEOF(pid_t, 4)
AC_CHECK_SIZEOF(uintptr_t)

AC_TYPE_LONG_DOUBLE
AC_CHECK_SIZEOF(long double, 16)

AC_CHECK_SIZEOF(_Bool, 1)

AC_CHECK_SIZEOF(off_t, [], [
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
])

AC_MSG_CHECKING(whether to enable large file support)
if test "$ac_cv_sizeof_off_t" -gt "$ac_cv_sizeof_long" -a \
	"$ac_cv_sizeof_long_long" -ge "$ac_cv_sizeof_off_t"; then
  have_largefile_support="yes"
else
  have_largefile_support="no"
fi
dnl LFS does not work with Emscripten 3.1
AS_CASE([$ac_sys_system],
  [Emscripten], [have_largefile_support="no"]
)
AS_VAR_IF([have_largefile_support], [yes], [
  AC_DEFINE(HAVE_LARGEFILE_SUPPORT, 1,
  [Defined to enable large file support when an off_t is bigger than a long
   and long long is at least as big as an off_t. You may need
   to add some flags for configuration and compilation to enable this mode.
   (For Solaris and Linux, the necessary defines are already defined.)])
  AC_MSG_RESULT(yes)
], [
  AC_MSG_RESULT(no)
])

AC_CHECK_SIZEOF(time_t, [], [
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_TIME_H
#include <time.h>
#endif
])

# if have pthread_t then define SIZEOF_PTHREAD_T
ac_save_cc="$CC"
if test "$ac_cv_kpthread" = "yes"
then CC="$CC -Kpthread"
elif test "$ac_cv_kthread" = "yes"
then CC="$CC -Kthread"
elif test "$ac_cv_pthread" = "yes"
then CC="$CC -pthread"
fi

AC_CACHE_CHECK([for pthread_t], [ac_cv_have_pthread_t], [
AC_COMPILE_IFELSE([
  AC_LANG_PROGRAM([[#include <pthread.h>]], [[pthread_t x; x = *(pthread_t*)0;]])
], [ac_cv_have_pthread_t=yes], [ac_cv_have_pthread_t=no])
])
AS_VAR_IF([ac_cv_have_pthread_t], [yes], [
  AC_CHECK_SIZEOF(pthread_t, [], [
#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif
  ])
])

# Issue #25658: POSIX hasn't defined that pthread_key_t is compatible with int.
# This checking will be unnecessary after removing deprecated TLS API.
AC_CHECK_SIZEOF(pthread_key_t, [], [[#include <pthread.h>]])
AC_CACHE_CHECK([whether pthread_key_t is compatible with int], [ac_cv_pthread_key_t_is_arithmetic_type], [
if test "$ac_cv_sizeof_pthread_key_t" -eq "$ac_cv_sizeof_int" ; then
  AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM([[#include <pthread.h>]], [[pthread_key_t k; k * 1;]])],
    [ac_cv_pthread_key_t_is_arithmetic_type=yes],
    [ac_cv_pthread_key_t_is_arithmetic_type=no]
  )
else
  ac_cv_pthread_key_t_is_arithmetic_type=no
fi
])
AS_VAR_IF([ac_cv_pthread_key_t_is_arithmetic_type], [yes], [
    AC_DEFINE(PTHREAD_KEY_T_IS_COMPATIBLE_WITH_INT, 1,
              [Define if pthread_key_t is compatible with int.])
])

CC="$ac_save_cc"

AC_SUBST(OTHER_LIBTOOL_OPT)
case $ac_sys_system/$ac_sys_release in
  Darwin/@<:@01567@:>@\..*)
    OTHER_LIBTOOL_OPT="-prebind -seg1addr 0x10000000"
    ;;
  Darwin/*)
    OTHER_LIBTOOL_OPT=""
    ;;
esac


AC_SUBST(LIBTOOL_CRUFT)
case $ac_sys_system/$ac_sys_release in
  Darwin/@<:@01567@:>@\..*)
    LIBTOOL_CRUFT="-framework System -lcc_dynamic"
    if test "${enable_universalsdk}"; then
	    :
    else
        LIBTOOL_CRUFT="${LIBTOOL_CRUFT} -arch_only `/usr/bin/arch`"
    fi
    LIBTOOL_CRUFT=$LIBTOOL_CRUFT' -install_name $(PYTHONFRAMEWORKINSTALLDIR)/Versions/$(VERSION)/$(PYTHONFRAMEWORK)'
    LIBTOOL_CRUFT=$LIBTOOL_CRUFT' -compatibility_version $(VERSION) -current_version $(VERSION)';;
  Darwin/*)
    gcc_version=`gcc -dumpversion`
    if test ${gcc_version} '<' 4.0
        then
            LIBTOOL_CRUFT="-lcc_dynamic"
        else
            LIBTOOL_CRUFT=""
    fi
    AC_RUN_IFELSE([AC_LANG_SOURCE([[
    #include <unistd.h>
    int main(int argc, char*argv[])
    {
      if (sizeof(long) == 4) {
    	  return 0;
      } else {
      	  return 1;
      }
    }
    ]])],[ac_osx_32bit=yes],[ac_osx_32bit=no],[ac_osx_32bit=yes])

    if test "${ac_osx_32bit}" = "yes"; then
    	case `/usr/bin/arch` in
    	i386)
    		MACOSX_DEFAULT_ARCH="i386"
    		;;
    	ppc)
    		MACOSX_DEFAULT_ARCH="ppc"
    		;;
    	*)
    		AC_MSG_ERROR([Unexpected output of 'arch' on macOS])
    		;;
    	esac
    else
    	case `/usr/bin/arch` in
    	i386)
    		MACOSX_DEFAULT_ARCH="x86_64"
    		;;
    	ppc)
    		MACOSX_DEFAULT_ARCH="ppc64"
		;;
    	arm64)
    		MACOSX_DEFAULT_ARCH="arm64"
    		;;
    	*)
    		AC_MSG_ERROR([Unexpected output of 'arch' on macOS])
    		;;
    	esac

    fi

    LIBTOOL_CRUFT=$LIBTOOL_CRUFT" -lSystem -lSystemStubs -arch_only ${MACOSX_DEFAULT_ARCH}"
    LIBTOOL_CRUFT=$LIBTOOL_CRUFT' -install_name $(PYTHONFRAMEWORKINSTALLDIR)/Versions/$(VERSION)/$(PYTHONFRAMEWORK)'
    LIBTOOL_CRUFT=$LIBTOOL_CRUFT' -compatibility_version $(VERSION) -current_version $(VERSION)';;
esac
AC_MSG_CHECKING(for --enable-framework)
if test "$enable_framework"
then
	BASECFLAGS="$BASECFLAGS -fno-common -dynamic"
	# -F. is needed to allow linking to the framework while
	# in the build location.
	AC_DEFINE(WITH_NEXT_FRAMEWORK, 1,
         [Define if you want to produce an OpenStep/Rhapsody framework
         (shared library plus accessory files).])
	AC_MSG_RESULT(yes)
	if test $enable_shared = "yes"
	then
		AC_MSG_ERROR([Specifying both --enable-shared and --enable-framework is not supported, use only --enable-framework instead])
	fi
else
	AC_MSG_RESULT(no)
fi

AC_MSG_CHECKING(for dyld)
case $ac_sys_system/$ac_sys_release in
  Darwin/*)
  	AC_DEFINE(WITH_DYLD, 1,
        [Define if you want to use the new-style (Openstep, Rhapsody, MacOS)
         dynamic linker (dyld) instead of the old-style (NextStep) dynamic
         linker (rld). Dyld is necessary to support frameworks.])
  	AC_MSG_RESULT(always on for Darwin)
  	;;
  *)
	AC_MSG_RESULT(no)
	;;
esac

AC_MSG_CHECKING(for --with-address-sanitizer)
AC_ARG_WITH(address_sanitizer,
            AS_HELP_STRING([--with-address-sanitizer],
                           [enable AddressSanitizer memory error detector, 'asan' (default is no)]),
[
AC_MSG_RESULT($withval)
BASECFLAGS="-fsanitize=address -fno-omit-frame-pointer $BASECFLAGS"
LDFLAGS="-fsanitize=address $LDFLAGS"
# ASan works by controlling memory allocation, our own malloc interferes.
with_pymalloc="no"
],
[AC_MSG_RESULT(no)])

AC_MSG_CHECKING(for --with-memory-sanitizer)
AC_ARG_WITH(memory_sanitizer,
            AS_HELP_STRING([--with-memory-sanitizer],
                           [enable MemorySanitizer allocation error detector, 'msan' (default is no)]),
[
AC_MSG_RESULT($withval)
AX_CHECK_COMPILE_FLAG([-fsanitize=memory],[
BASECFLAGS="-fsanitize=memory -fsanitize-memory-track-origins=2 -fno-omit-frame-pointer $BASECFLAGS"
LDFLAGS="-fsanitize=memory -fsanitize-memory-track-origins=2 $LDFLAGS"
],[AC_MSG_ERROR([The selected compiler doesn't support memory sanitizer])])
# MSan works by controlling memory allocation, our own malloc interferes.
with_pymalloc="no"
],
[AC_MSG_RESULT(no)])

AC_MSG_CHECKING(for --with-undefined-behavior-sanitizer)
AC_ARG_WITH(undefined_behavior_sanitizer,
            AS_HELP_STRING([--with-undefined-behavior-sanitizer],
                           [enable UndefinedBehaviorSanitizer undefined behaviour detector, 'ubsan' (default is no)]),
[
AC_MSG_RESULT($withval)
BASECFLAGS="-fsanitize=undefined $BASECFLAGS"
LDFLAGS="-fsanitize=undefined $LDFLAGS"
with_ubsan="yes"
],
[
AC_MSG_RESULT(no)
with_ubsan="no"
])

# Set info about shared libraries.
AC_SUBST(SHLIB_SUFFIX)
AC_SUBST(LDSHARED)
AC_SUBST(LDCXXSHARED)
AC_SUBST(BLDSHARED)
AC_SUBST(CCSHARED)
AC_SUBST(LINKFORSHARED)

# SHLIB_SUFFIX is the extension of shared libraries `(including the dot!)
# -- usually .so, .sl on HP-UX, .dll on Cygwin
AC_MSG_CHECKING(the extension of shared libraries)
if test -z "$SHLIB_SUFFIX"; then
	case $ac_sys_system in
	hp*|HP*)
		case `uname -m` in
			ia64) SHLIB_SUFFIX=.so;;
	  		*)    SHLIB_SUFFIX=.sl;;
		esac
		;;
	CYGWIN*)   SHLIB_SUFFIX=.dll;;
	*)	   SHLIB_SUFFIX=.so;;
	esac
fi
AC_MSG_RESULT($SHLIB_SUFFIX)

# LDSHARED is the ld *command* used to create shared library
# -- "cc -G" on SunOS 5.x.
# (Shared libraries in this instance are shared modules to be loaded into
# Python, as opposed to building Python itself as a shared library.)
AC_MSG_CHECKING(LDSHARED)
if test -z "$LDSHARED"
then
	case $ac_sys_system/$ac_sys_release in
	AIX*)
		BLDSHARED="Modules/ld_so_aix \$(CC) -bI:Modules/python.exp"
		LDSHARED="\$(LIBPL)/ld_so_aix \$(CC) -bI:\$(LIBPL)/python.exp"
		;;
	SunOS/5*)
		if test "$GCC" = "yes" ; then
			LDSHARED='$(CC) -shared'
			LDCXXSHARED='$(CXX) -shared'
		else
			LDSHARED='$(CC) -G'
			LDCXXSHARED='$(CXX) -G'
		fi ;;
	hp*|HP*)
		if test "$GCC" = "yes" ; then
			LDSHARED='$(CC) -shared'
			LDCXXSHARED='$(CXX) -shared'
		else
			LDSHARED='$(CC) -b'
			LDCXXSHARED='$(CXX) -b'
		fi ;;
	Darwin/1.3*)
		LDSHARED='$(CC) -bundle'
		LDCXXSHARED='$(CXX) -bundle'
		if test "$enable_framework" ; then
			# Link against the framework. All externals should be defined.
			BLDSHARED="$LDSHARED "'$(PYTHONFRAMEWORKDIR)/Versions/$(VERSION)/$(PYTHONFRAMEWORK)'
			LDSHARED="$LDSHARED "'$(PYTHONFRAMEWORKPREFIX)/$(PYTHONFRAMEWORKDIR)/Versions/$(VERSION)/$(PYTHONFRAMEWORK)'
			LDCXXSHARED="$LDCXXSHARED "'$(PYTHONFRAMEWORKPREFIX)/$(PYTHONFRAMEWORKDIR)/Versions/$(VERSION)/$(PYTHONFRAMEWORK)'
		else
			# No framework. Ignore undefined symbols, assuming they come from Python
			LDSHARED="$LDSHARED -undefined suppress"
			LDCXXSHARED="$LDCXXSHARED -undefined suppress"
		fi ;;
	Darwin/1.4*|Darwin/5.*|Darwin/6.*)
		LDSHARED='$(CC) -bundle'
		LDCXXSHARED='$(CXX) -bundle'
		if test "$enable_framework" ; then
			# Link against the framework. All externals should be defined.
			BLDSHARED="$LDSHARED "'$(PYTHONFRAMEWORKDIR)/Versions/$(VERSION)/$(PYTHONFRAMEWORK)'
			LDSHARED="$LDSHARED "'$(PYTHONFRAMEWORKPREFIX)/$(PYTHONFRAMEWORKDIR)/Versions/$(VERSION)/$(PYTHONFRAMEWORK)'
			LDCXXSHARED="$LDCXXSHARED "'$(PYTHONFRAMEWORKPREFIX)/$(PYTHONFRAMEWORKDIR)/Versions/$(VERSION)/$(PYTHONFRAMEWORK)'
		else
			# No framework, use the Python app as bundle-loader
			BLDSHARED="$LDSHARED "'-bundle_loader $(BUILDPYTHON)'
			LDSHARED="$LDSHARED "'-bundle_loader $(BINDIR)/python$(VERSION)$(EXE)'
			LDCXXSHARED="$LDCXXSHARED "'-bundle_loader $(BINDIR)/python$(VERSION)$(EXE)'
		fi ;;
	Darwin/*)
		# Use -undefined dynamic_lookup whenever possible (10.3 and later).
		# This allows an extension to be used in any Python

		dep_target_major=`echo ${MACOSX_DEPLOYMENT_TARGET} | \
				sed 's/\([[0-9]]*\)\.\([[0-9]]*\).*/\1/'`
		dep_target_minor=`echo ${MACOSX_DEPLOYMENT_TARGET} | \
				sed 's/\([[0-9]]*\)\.\([[0-9]]*\).*/\2/'`
		if test ${dep_target_major} -eq 10 && \
		   test ${dep_target_minor} -le 2
		then
			# building for OS X 10.0 through 10.2
			AC_MSG_ERROR([MACOSX_DEPLOYMENT_TARGET too old ($MACOSX_DEPLOYMENT_TARGET), only 10.3 or later is supported])
		else
			# building for OS X 10.3 and later
			LDSHARED='$(CC) -bundle -undefined dynamic_lookup'
			LDCXXSHARED='$(CXX) -bundle -undefined dynamic_lookup'
			BLDSHARED="$LDSHARED"
		fi
		;;
	Emscripten|WASI)
		LDSHARED='$(CC) -shared'
		LDCXXSHARED='$(CXX) -shared';;
	Linux*|GNU*|QNX*|VxWorks*|Haiku*)
		LDSHARED='$(CC) -shared'
		LDCXXSHARED='$(CXX) -shared';;
	FreeBSD*)
		if [[ "`$CC -dM -E - </dev/null | grep __ELF__`" != "" ]]
		then
			LDSHARED='$(CC) -shared'
			LDCXXSHARED='$(CXX) -shared'
		else
			LDSHARED="ld -Bshareable"
		fi;;
	OpenBSD*)
		if [[ "`$CC -dM -E - </dev/null | grep __ELF__`" != "" ]]
		then
				LDSHARED='$(CC) -shared $(CCSHARED)'
				LDCXXSHARED='$(CXX) -shared $(CCSHARED)'
		else
				case `uname -r` in
				[[01]].* | 2.[[0-7]] | 2.[[0-7]].*)
				   LDSHARED="ld -Bshareable ${LDFLAGS}"
				   ;;
				*)
				   LDSHARED='$(CC) -shared $(CCSHARED)'
				   LDCXXSHARED='$(CXX) -shared $(CCSHARED)'
				   ;;
				esac
		fi;;
	NetBSD*|DragonFly*)
		LDSHARED='$(CC) -shared'
		LDCXXSHARED='$(CXX) -shared';;
	OpenUNIX*|UnixWare*)
		if test "$GCC" = "yes" ; then
			LDSHARED='$(CC) -shared'
			LDCXXSHARED='$(CXX) -shared'
		else
			LDSHARED='$(CC) -G'
			LDCXXSHARED='$(CXX) -G'
		fi;;
	SCO_SV*)
		LDSHARED='$(CC) -Wl,-G,-Bexport'
		LDCXXSHARED='$(CXX) -Wl,-G,-Bexport';;
	WASI*)
		AS_VAR_IF([enable_wasm_dynamic_linking], [yes], [
			dnl not iplemented yet
		]);;
	CYGWIN*)
		LDSHARED="gcc -shared -Wl,--enable-auto-image-base"
		LDCXXSHARED="g++ -shared -Wl,--enable-auto-image-base";;
	*)	LDSHARED="ld";;
	esac
fi

dnl Emscripten's emconfigure sets LDSHARED. Set BLDSHARED outside the
dnl test -z $LDSHARED block to configure BLDSHARED for side module support.
if test "$enable_wasm_dynamic_linking" = "yes" -a "$ac_sys_system" = "Emscripten"; then
  BLDSHARED='$(CC) -shared -sSIDE_MODULE=1'
fi

AC_MSG_RESULT($LDSHARED)
LDCXXSHARED=${LDCXXSHARED-$LDSHARED}

AC_MSG_CHECKING([BLDSHARED flags])
BLDSHARED=${BLDSHARED-$LDSHARED}
AC_MSG_RESULT([$BLDSHARED])

# CCSHARED are the C *flags* used to create objects to go into a shared
# library (module) -- this is only needed for a few systems
AC_MSG_CHECKING(CCSHARED)
if test -z "$CCSHARED"
then
	case $ac_sys_system/$ac_sys_release in
	SunOS*) if test "$GCC" = yes;
		then CCSHARED="-fPIC";
		elif test `uname -p` = sparc;
		then CCSHARED="-xcode=pic32";
		else CCSHARED="-Kpic";
		fi;;
	hp*|HP*) if test "$GCC" = yes;
		 then CCSHARED="-fPIC";
		 else CCSHARED="+z";
		 fi;;
	Linux-android*) ;;
	Linux*|GNU*) CCSHARED="-fPIC";;
	Emscripten*|WASI*)
		AS_VAR_IF([enable_wasm_dynamic_linking], [yes], [
			CCSHARED="-fPIC"
		]);;
	FreeBSD*|NetBSD*|OpenBSD*|DragonFly*) CCSHARED="-fPIC";;
	Haiku*) CCSHARED="-fPIC";;
	OpenUNIX*|UnixWare*)
		if test "$GCC" = "yes"
		then CCSHARED="-fPIC"
		else CCSHARED="-KPIC"
		fi;;
	SCO_SV*)
		if test "$GCC" = "yes"
		then CCSHARED="-fPIC"
		else CCSHARED="-Kpic -belf"
		fi;;
	VxWorks*)
		CCSHARED="-fpic -D__SO_PICABILINUX__  -ftls-model=global-dynamic"
	esac
fi
AC_MSG_RESULT($CCSHARED)
# LINKFORSHARED are the flags passed to the $(CC) command that links
# the python executable -- this is only needed for a few systems
AC_MSG_CHECKING(LINKFORSHARED)
if test -z "$LINKFORSHARED"
then
	case $ac_sys_system/$ac_sys_release in
	AIX*)	LINKFORSHARED='-Wl,-bE:Modules/python.exp -lld';;
	hp*|HP*)
	    LINKFORSHARED="-Wl,-E -Wl,+s";;
#	    LINKFORSHARED="-Wl,-E -Wl,+s -Wl,+b\$(BINLIBDEST)/lib-dynload";;
	Linux-android*) LINKFORSHARED="-pie -Xlinker -export-dynamic";;
	Linux*|GNU*) LINKFORSHARED="-Xlinker -export-dynamic";;
	# -u libsys_s pulls in all symbols in libsys
	Darwin/*)
		LINKFORSHARED="$extra_undefs -framework CoreFoundation"

		# Issue #18075: the default maximum stack size (8MBytes) is too
		# small for the default recursion limit. Increase the stack size
		# to ensure that tests don't crash
    stack_size="1000000"  # 16 MB
    if test "$with_ubsan" = "yes"
    then
        # Undefined behavior sanitizer requires an even deeper stack
        stack_size="4000000"  # 64 MB
    fi

    LINKFORSHARED="-Wl,-stack_size,$stack_size $LINKFORSHARED"

    AC_DEFINE_UNQUOTED(THREAD_STACK_SIZE,
        0x$stack_size,
        [Custom thread stack size depending on chosen sanitizer runtimes.])

		if test "$enable_framework"
		then
			LINKFORSHARED="$LINKFORSHARED "'$(PYTHONFRAMEWORKDIR)/Versions/$(VERSION)/$(PYTHONFRAMEWORK)'
		fi
		LINKFORSHARED="$LINKFORSHARED";;
	OpenUNIX*|UnixWare*) LINKFORSHARED="-Wl,-Bexport";;
	SCO_SV*) LINKFORSHARED="-Wl,-Bexport";;
	ReliantUNIX*) LINKFORSHARED="-W1 -Blargedynsym";;
	FreeBSD*|NetBSD*|OpenBSD*|DragonFly*)
		if [[ "`$CC -dM -E - </dev/null | grep __ELF__`" != "" ]]
		then
			LINKFORSHARED="-Wl,--export-dynamic"
		fi;;
	SunOS/5*) case $CC in
		  *gcc*)
		    if $CC -Xlinker --help 2>&1 | grep export-dynamic >/dev/null
		    then
			LINKFORSHARED="-Xlinker --export-dynamic"
		    fi;;
		  esac;;
	CYGWIN*)
		if test $enable_shared = "no"
		then
			LINKFORSHARED='-Wl,--out-implib=$(LDLIBRARY)'
		fi;;
	QNX*)
		# -Wl,-E causes the symbols to be added to the dynamic
		# symbol table so that they can be found when a module
		# is loaded.  -N 2048K causes the stack size to be set
		# to 2048 kilobytes so that the stack doesn't overflow
		# when running test_compile.py.
		LINKFORSHARED='-Wl,-E -N 2048K';;
	VxWorks*)
		LINKFORSHARED='-Wl,-export-dynamic';;
	esac
fi
AC_MSG_RESULT($LINKFORSHARED)


AC_SUBST(CFLAGSFORSHARED)
AC_MSG_CHECKING(CFLAGSFORSHARED)
if test ! "$LIBRARY" = "$LDLIBRARY"
then
	case $ac_sys_system in
	CYGWIN*)
		# Cygwin needs CCSHARED when building extension DLLs
		# but not when building the interpreter DLL.
		CFLAGSFORSHARED='';;
	*)
		CFLAGSFORSHARED='$(CCSHARED)'
	esac
fi

dnl WASM dynamic linking requires -fPIC.
AS_VAR_IF([enable_wasm_dynamic_linking], [yes], [
  CFLAGSFORSHARED='$(CCSHARED)'
])

AC_MSG_RESULT($CFLAGSFORSHARED)

# SHLIBS are libraries (except -lc and -lm) to link to the python shared
# library (with --enable-shared).
# For platforms on which shared libraries are not allowed to have unresolved
# symbols, this must be set to $(LIBS) (expanded by make). We do this even
# if it is not required, since it creates a dependency of the shared library
# to LIBS. This, in turn, means that applications linking the shared libpython
# don't need to link LIBS explicitly. The default should be only changed
# on systems where this approach causes problems.
AC_SUBST(SHLIBS)
AC_MSG_CHECKING(SHLIBS)
case "$ac_sys_system" in
	*)
		SHLIBS='$(LIBS)';;
esac
AC_MSG_RESULT($SHLIBS)


# checks for libraries
AC_CHECK_LIB(sendfile, sendfile)
AC_CHECK_LIB(dl, dlopen)	# Dynamic linking for SunOS/Solaris and SYSV
AC_CHECK_LIB(dld, shl_load)	# Dynamic linking for HP-UX


dnl check for uuid dependencies
AH_TEMPLATE([HAVE_UUID_H], [Define to 1 if you have the <uuid.h> header file.])
AH_TEMPLATE([HAVE_UUID_UUID_H], [Define to 1 if you have the <uuid/uuid.h> header file.])
AH_TEMPLATE([HAVE_UUID_GENERATE_TIME_SAFE], [Define if uuid_generate_time_safe() exists.])
have_uuid=missing

dnl AIX provides support for RFC4122 (uuid) in libc.a starting with AIX 6.1
dnl (anno 2007). FreeBSD and OpenBSD provides support in libc as well.
dnl Little-endian FreeBSD, OpenBSD and NetBSD needs encoding into an octet
dnl stream in big-endian byte-order
AC_CHECK_HEADERS([uuid.h], [
  AC_CHECK_FUNCS([uuid_create uuid_enc_be], [
    have_uuid=yes
    LIBUUID_CFLAGS=${LIBUUID_CFLAGS-""}
    LIBUUID_LIBS=${LIBUUID_LIBS-""}
  ])
])

AS_VAR_IF([have_uuid], [missing], [
  PKG_CHECK_MODULES(
    [LIBUUID], [uuid >= 2.20], [
      dnl linux-util's libuuid has uuid_generate_time_safe() since v2.20 (2011)
      dnl and provides <uuid.h>.
      have_uuid=yes
      AC_DEFINE([HAVE_UUID_H], [1])
      AC_DEFINE([HAVE_UUID_GENERATE_TIME_SAFE], [1])
    ], [
      WITH_SAVE_ENV([
        CPPFLAGS="$CPPFLAGS $LIBUUID_CFLAGS"
        LDFLAGS="$LDFLAGS $LIBUUID_LIBS"
        AC_CHECK_HEADERS([uuid/uuid.h], [
          PY_CHECK_LIB([uuid], [uuid_generate_time], [have_uuid=yes])
          PY_CHECK_LIB([uuid], [uuid_generate_time_safe], [
            have_uuid=yes
            AC_DEFINE([HAVE_UUID_GENERATE_TIME_SAFE], [1])
          ])
        ])
        AS_VAR_IF([have_uuid], [yes], [
          LIBUUID_CFLAGS=${LIBUUID_CFLAGS-""}
          LIBUUID_LIBS=${LIBUUID_LIBS-"-luuid"}
        ])
      ])
    ]
  )
])

dnl macOS has uuid/uuid.h but uuid_generate_time is in libc
AS_VAR_IF([have_uuid], [missing], [
  AC_CHECK_HEADERS([uuid/uuid.h], [
    AC_CHECK_FUNC([uuid_generate_time], [
      have_uuid=yes
      LIBUUID_CFLAGS=${LIBUUID_CFLAGS-""}
      LIBUUID_LIBS=${LIBUUID_LIBS-""}
    ])
  ])
])

AS_VAR_IF([have_uuid], [missing], [have_uuid=no])

# 'Real Time' functions on Solaris
# posix4 on Solaris 2.6
# pthread (first!) on Linux
AC_SEARCH_LIBS(sem_init, pthread rt posix4)

# check if we need libintl for locale functions
AC_CHECK_LIB(intl, textdomain,
	[AC_DEFINE(WITH_LIBINTL, 1,
	[Define to 1 if libintl is needed for locale functions.])
        LIBS="-lintl $LIBS"])

# checks for system dependent C++ extensions support
case "$ac_sys_system" in
	AIX*)	AC_MSG_CHECKING(for genuine AIX C++ extensions support)
		AC_LINK_IFELSE([
		  AC_LANG_PROGRAM([[#include <load.h>]],
				  [[loadAndInit("", 0, "")]])
		],[
		  AC_DEFINE(AIX_GENUINE_CPLUSPLUS, 1,
                      [Define for AIX if your compiler is a genuine IBM xlC/xlC_r
                       and you want support for AIX C++ shared extension modules.])
		  AC_MSG_RESULT(yes)
		],[
		  AC_MSG_RESULT(no)
		])
dnl The AIX_BUILDDATE is obtained from the kernel fileset - bos.mp64
# BUILD_GNU_TYPE + AIX_BUILDDATE are used to construct the platform_tag
# of the AIX system used to build/package Python executable. This tag serves
# as a baseline for bdist module packages
               AC_MSG_CHECKING(for the system builddate)
               AIX_BUILDDATE=$(lslpp -Lcq bos.mp64 | awk -F:  '{ print $NF }')
               AC_DEFINE_UNQUOTED([AIX_BUILDDATE], [$AIX_BUILDDATE],
                   [BUILD_GNU_TYPE + AIX_BUILDDATE are used to construct the PEP425 tag of the build system.])
               AC_MSG_RESULT($AIX_BUILDDATE)
               ;;
	*) ;;
esac

# check for systems that require aligned memory access
AC_CACHE_CHECK([aligned memory access is required], [ac_cv_aligned_required],
[AC_RUN_IFELSE([AC_LANG_SOURCE([[
int main(void)
{
    char s[16];
    int i, *p1, *p2;
    for (i=0; i < 16; i++)
        s[i] = i;
    p1 = (int*)(s+1);
    p2 = (int*)(s+2);
    if (*p1 == *p2)
        return 1;
    return 0;
}]])],
[ac_cv_aligned_required=no],
[ac_cv_aligned_required=yes],
[ac_cv_aligned_required=yes])
])
if test "$ac_cv_aligned_required" = yes ; then
  AC_DEFINE([HAVE_ALIGNED_REQUIRED], [1],
    [Define if aligned memory access is required])
fi

# str, bytes and memoryview hash algorithm
AH_TEMPLATE(Py_HASH_ALGORITHM,
  [Define hash algorithm for str, bytes and memoryview.
   SipHash24: 1, FNV: 2, SipHash13: 3, externally defined: 0])

AC_MSG_CHECKING(for --with-hash-algorithm)
dnl quadrigraphs "@<:@" and "@:>@" produce "[" and "]" in the output
AC_ARG_WITH(hash_algorithm,
            AS_HELP_STRING([--with-hash-algorithm=@<:@fnv|siphash13|siphash24@:>@],
                           [select hash algorithm for use in Python/pyhash.c (default is SipHash13)]),
[
AC_MSG_RESULT($withval)
case "$withval" in
    siphash13)
        AC_DEFINE(Py_HASH_ALGORITHM, 3)
        ;;
    siphash24)
        AC_DEFINE(Py_HASH_ALGORITHM, 1)
        ;;
    fnv)
        AC_DEFINE(Py_HASH_ALGORITHM, 2)
        ;;
    *)
        AC_MSG_ERROR([unknown hash algorithm '$withval'])
        ;;
esac
],
[AC_MSG_RESULT(default)])

validate_tzpath() {
    # Checks that each element of the path is an absolute path
    if test -z "$1"; then
        # Empty string is allowed: it indicates no system TZPATH
        return 0
    fi

    # Bad paths are those that don't start with /
    dnl quadrigraphs "@<:@" and "@:>@" produce "[" and "]" in the output
    if ( echo $1 | grep '\(^\|:\)\(@<:@^/@:>@\|$\)' > /dev/null); then
        AC_MSG_ERROR([--with-tzpath must contain only absolute paths, not $1])
        return 1;
    fi
}

TZPATH="/usr/share/zoneinfo:/usr/lib/zoneinfo:/usr/share/lib/zoneinfo:/etc/zoneinfo"
AC_MSG_CHECKING(for --with-tzpath)
AC_ARG_WITH(tzpath,
            AS_HELP_STRING([--with-tzpath=<list of absolute paths separated by pathsep>]
                           [Select the default time zone search path for zoneinfo.TZPATH]),
[
case "$withval" in
    yes)
        AC_MSG_ERROR([--with-tzpath requires a value])
        ;;
    *)
        validate_tzpath "$withval"
        TZPATH="$withval"
        AC_MSG_RESULT("$withval")
        ;;
esac
],
[validate_tzpath "$TZPATH"
 AC_MSG_RESULT("$TZPATH")])
AC_SUBST(TZPATH)

# Most SVR4 platforms (e.g. Solaris) need -lsocket and -lnsl.
AC_CHECK_LIB(nsl, t_open, [LIBS="-lnsl $LIBS"]) # SVR4
AC_CHECK_LIB(socket, socket, [LIBS="-lsocket $LIBS"], [], $LIBS) # SVR4 sockets

case $ac_sys_system/$ac_sys_release in
    Haiku*)
        AC_CHECK_LIB(network, socket, [LIBS="-lnetwork $LIBS"], [], $LIBS)
    ;;
esac

AC_MSG_CHECKING(for --with-libs)
AC_ARG_WITH(libs,
            AS_HELP_STRING([--with-libs='lib1 ...'], [link against additional libs (default is no)]),
[
AC_MSG_RESULT($withval)
LIBS="$withval $LIBS"
],
[AC_MSG_RESULT(no)])

# Check for use of the system expat library
AC_MSG_CHECKING(for --with-system-expat)
AC_ARG_WITH(system_expat,
            AS_HELP_STRING([--with-system-expat], [build pyexpat module using an installed expat library, see Doc/library/pyexpat.rst (default is no)]),
            [],
            [with_system_expat="no"])

AC_MSG_RESULT($with_system_expat)

AS_VAR_IF([with_system_expat], [yes], [
  LIBEXPAT_CFLAGS=${LIBEXPAT_CFLAGS-""}
  LIBEXPAT_LDFLAGS=${LIBEXPAT_LDFLAGS-"-lexpat"}
  LIBEXPAT_INTERNAL=
], [
  LIBEXPAT_CFLAGS="-I\$(srcdir)/Modules/expat"
  LIBEXPAT_LDFLAGS="-lm \$(LIBEXPAT_A)"
  LIBEXPAT_INTERNAL="\$(LIBEXPAT_HEADERS) \$(LIBEXPAT_A)"
])

AC_SUBST([LIBEXPAT_CFLAGS])
AC_SUBST([LIBEXPAT_LDFLAGS])
AC_SUBST([LIBEXPAT_INTERNAL])

# Check for use of the system libffi library
AC_MSG_CHECKING(for --with-system-ffi)
AC_ARG_WITH(system_ffi,
            AS_HELP_STRING([--with-system-ffi], [build _ctypes module using an installed ffi library, see Doc/library/ctypes.rst (default is system-dependent)]),,,)

if test "$ac_sys_system" = "Darwin"
then
    case "$with_system_ffi" in
        "")
            with_system_ffi="no"
            ;;
        yes|no)
            ;;
        *)
            AC_MSG_ERROR([--with-system-ffi accepts no arguments])
            ;;
    esac
    AC_MSG_RESULT($with_system_ffi)
else
    AC_MSG_RESULT(yes)
    if test "$with_system_ffi" != ""
    then
        AC_MSG_WARN([--with(out)-system-ffi is ignored on this platform])
    fi
    with_system_ffi="yes"
fi

if test "$with_system_ffi" = "yes" && test -n "$PKG_CONFIG"; then
    LIBFFI_INCLUDEDIR="`"$PKG_CONFIG" libffi --cflags-only-I 2>/dev/null | sed -e 's/^-I//;s/ *$//'`"
else
    LIBFFI_INCLUDEDIR=""
fi
AC_SUBST(LIBFFI_INCLUDEDIR)

# Check for use of the system libmpdec library
AC_MSG_CHECKING(for --with-system-libmpdec)
AC_ARG_WITH(system_libmpdec,
            AS_HELP_STRING([--with-system-libmpdec], [build _decimal module using an installed libmpdec library, see Doc/library/decimal.rst (default is no)]),
            [],
            [with_system_libmpdec="no"])
AC_MSG_RESULT($with_system_libmpdec)

AS_VAR_IF([with_system_libmpdec], [yes], [
  LIBMPDEC_CFLAGS=${LIBMPDEC_CFLAGS-""}
  LIBMPDEC_LDFLAGS=${LIBMPDEC_LDFLAGS-"-lmpdec"}
  LIBMPDEC_INTERNAL=
], [
  LIBMPDEC_CFLAGS="-I\$(srcdir)/Modules/_decimal/libmpdec"
  LIBMPDEC_LDFLAGS="-lm \$(LIBMPDEC_A)"
  LIBMPDEC_INTERNAL="\$(LIBMPDEC_HEADERS) \$(LIBMPDEC_A)"

  dnl Disable forced inlining in debug builds, see GH-94847
  AS_VAR_IF([with_pydebug], [yes], [
    AS_VAR_APPEND([LIBMPDEC_CFLAGS], [" -DTEST_COVERAGE"])
  ])
])

AC_SUBST([LIBMPDEC_CFLAGS])
AC_SUBST([LIBMPDEC_LDFLAGS])
AC_SUBST([LIBMPDEC_INTERNAL])

# Check whether _decimal should use a coroutine-local or thread-local context
AC_MSG_CHECKING(for --with-decimal-contextvar)
AC_ARG_WITH(decimal_contextvar,
            AS_HELP_STRING([--with-decimal-contextvar], [build _decimal module using a coroutine-local rather than a thread-local context (default is yes)]),
            [],
            [with_decimal_contextvar="yes"])

if test "$with_decimal_contextvar" != "no"
then
    AC_DEFINE(WITH_DECIMAL_CONTEXTVAR, 1,
      [Define if you want build the _decimal module using a coroutine-local rather than a thread-local context])
fi

AC_MSG_RESULT($with_decimal_contextvar)

# Check for libmpdec machine flavor
AC_MSG_CHECKING(for decimal libmpdec machine)
AS_CASE([$ac_sys_system],
  [Darwin*], [libmpdec_system=Darwin],
  [SunOS*], [libmpdec_system=sunos],
  [libmpdec_system=other]
)

libmpdec_machine=unknown
if test "$libmpdec_system" = Darwin; then
    # universal here means: build libmpdec with the same arch options
    # the python interpreter was built with
    libmpdec_machine=universal
elif test $ac_cv_sizeof_size_t -eq 8; then
    if test "$ac_cv_gcc_asm_for_x64" = yes; then
        libmpdec_machine=x64
    elif test "$ac_cv_type___uint128_t" = yes; then
        libmpdec_machine=uint128
    else
        libmpdec_machine=ansi64
    fi
elif test $ac_cv_sizeof_size_t -eq 4; then
    if test "$ac_cv_gcc_asm_for_x87" = yes -a "$libmpdec_system" != sunos; then
        AS_CASE([$CC],
            [*gcc*],   [libmpdec_machine=ppro],
            [*clang*], [libmpdec_machine=ppro],
            [libmpdec_machine=ansi32]
        )
    else
        libmpdec_machine=ansi32
    fi
fi
AC_MSG_RESULT([$libmpdec_machine])

AS_CASE([$libmpdec_machine],
  [x64],         [AS_VAR_APPEND([LIBMPDEC_CFLAGS], [" -DCONFIG_64=1 -DASM=1"])],
  [uint128],     [AS_VAR_APPEND([LIBMPDEC_CFLAGS], [" -DCONFIG_64=1 -DANSI=1 -DHAVE_UINT128_T=1"])],
  [ansi64],      [AS_VAR_APPEND([LIBMPDEC_CFLAGS], [" -DCONFIG_64=1 -DANSI=1"])],
  [ppro],        [AS_VAR_APPEND([LIBMPDEC_CFLAGS], [" -DCONFIG_32=1 -DANSI=1 -DASM=1 -Wno-unknown-pragmas"])],
  [ansi32],      [AS_VAR_APPEND([LIBMPDEC_CFLAGS], [" -DCONFIG_32=1 -DANSI=1"])],
  [ansi-legacy], [AS_VAR_APPEND([LIBMPDEC_CFLAGS], [" -DCONFIG_32=1 -DANSI=1 -DLEGACY_COMPILER=1"])],
  [universal],   [AS_VAR_APPEND([LIBMPDEC_CFLAGS], [" -DUNIVERSAL=1"])],
  [AC_MSG_ERROR([_decimal: unsupported architecture])]
)

if test "$have_ipa_pure_const_bug" = yes; then
    # Some versions of gcc miscompile inline asm:
    # https://gcc.gnu.org/bugzilla/show_bug.cgi?id=46491
    # https://gcc.gnu.org/ml/gcc/2010-11/msg00366.html
    AS_VAR_APPEND([LIBMPDEC_CFLAGS], [" -fno-ipa-pure-const"])
fi

if test "$have_glibc_memmove_bug" = yes; then
    # _FORTIFY_SOURCE wrappers for memmove and bcopy are incorrect:
    # https://sourceware.org/ml/libc-alpha/2010-12/msg00009.html
    AS_VAR_APPEND([LIBMPDEC_CFLAGS], [" -U_FORTIFY_SOURCE"])
fi


dnl check for NIS / libnsl dependencies
dnl libnsl dependencies include tirpc includes and lib
PKG_CHECK_MODULES([LIBNSL], [libnsl], [have_nis=yes], [
  LIBNSL_CFLAGS=${LIBNSL_CFLAGS-""}
  WITH_SAVE_ENV([
    AC_SEARCH_LIBS([yp_match], [nsl], [have_nis=yes], [have_nis=no])
  ])
  AS_CASE([$ac_cv_search_yp_match],
    [no], [libnsl=""],
    ["none required"], [libnsl=""],
    [libnsl="$ac_cv_search_yp_match"]
  )
  LIBNSL_LIBS=${LIBNSL_LIBS-$libnsl}
])

AS_VAR_IF([have_nis], [yes], [
  WITH_SAVE_ENV([
    CPPFLAGS="$CPPFLAGS $LIBNSL_CFLAGS"
    AC_CHECK_HEADERS([rpc/rpc.h])
  ])
])

dnl On NetBSD, when using OSS audio, you need to link against libossaudio
AS_CASE([$ac_sys_system],
  [NetBSD*], [OSSAUDIODEV_LIBS="-lossaudio"],
  [OSSAUDIODEV_LIBS=""]
)

dnl detect sqlite3 from Emscripten emport
PY_CHECK_EMSCRIPTEN_PORT([LIBSQLITE3], [-sUSE_SQLITE3])

dnl Check for SQLite library. Use pkg-config if available.
PKG_CHECK_MODULES(
  [LIBSQLITE3], [sqlite3 >= 3.7.15], [], [
    LIBSQLITE3_CFLAGS=${LIBSQLITE3_CFLAGS-""}
    LIBSQLITE3_LIBS=${LIBSQLITE3_LIBS-"-lsqlite3"}
  ]
)
AS_VAR_APPEND([LIBSQLITE3_CFLAGS], [' -I$(srcdir)/Modules/_sqlite'])

dnl PY_CHECK_SQLITE_FUNC(FUNCTION, IF-FOUND, IF-NOT-FOUND)
AC_DEFUN([PY_CHECK_SQLITE_FUNC], [
  AC_CHECK_LIB([sqlite3], [$1], [$2], [
    m4_ifblank([$3], [have_supported_sqlite3=no], [$3])
  ])
])

WITH_SAVE_ENV([
dnl bpo-45774/GH-29507: The CPP check in AC_CHECK_HEADER can fail on FreeBSD,
dnl hence CPPFLAGS instead of CFLAGS.
  CPPFLAGS="$CPPFLAGS $LIBSQLITE3_CFLAGS"
  LDFLAGS="$LIBSQLITE3_LIBS $LDFLAGS"

  AC_CHECK_HEADER([sqlite3.h], [
    have_sqlite3=yes

    AC_COMPILE_IFELSE([
      AC_LANG_PROGRAM([
        #include <sqlite3.h>
        #if SQLITE_VERSION_NUMBER < 3007015
        #  error "SQLite 3.7.15 or higher required"
        #endif
      ], [])
    ], [
      have_supported_sqlite3=yes
      dnl Check that required functions are in place. A lot of stuff may be
      dnl omitted with SQLITE_OMIT_* compile time defines.
      PY_CHECK_SQLITE_FUNC([sqlite3_bind_double])
      PY_CHECK_SQLITE_FUNC([sqlite3_column_decltype])
      PY_CHECK_SQLITE_FUNC([sqlite3_column_double])
      PY_CHECK_SQLITE_FUNC([sqlite3_complete])
      PY_CHECK_SQLITE_FUNC([sqlite3_enable_shared_cache])
      PY_CHECK_SQLITE_FUNC([sqlite3_progress_handler])
      PY_CHECK_SQLITE_FUNC([sqlite3_result_double])
      PY_CHECK_SQLITE_FUNC([sqlite3_set_authorizer])
      PY_CHECK_SQLITE_FUNC([sqlite3_trace_v2], [], [
        PY_CHECK_SQLITE_FUNC([sqlite3_trace])
      ])
      PY_CHECK_SQLITE_FUNC([sqlite3_value_double])
      AC_CHECK_LIB([sqlite3], [sqlite3_load_extension],
        [have_sqlite3_load_extension=yes],
        [have_sqlite3_load_extension=no]
      )
      AC_CHECK_LIB([sqlite3], [sqlite3_serialize], [
        AC_DEFINE(
          [PY_SQLITE_HAVE_SERIALIZE], [1],
          [Define if SQLite was compiled with the serialize API]
        )
      ])
    ], [
      have_supported_sqlite3=no
    ])
  ])
])

dnl Check for support for loadable sqlite extensions
AC_MSG_CHECKING([for --enable-loadable-sqlite-extensions])
AC_ARG_ENABLE([loadable-sqlite-extensions],
  AS_HELP_STRING(
    [--enable-loadable-sqlite-extensions], [
      support loadable extensions in the sqlite3 module, see
      Doc/library/sqlite3.rst (default is no)
    ]
  ), [
    AS_VAR_IF([have_sqlite3_load_extension], [no], [
      AC_MSG_RESULT([n/a])
      AC_MSG_WARN([Your version of SQLite does not support loadable extensions])
    ], [
      AC_MSG_RESULT([yes])
      AC_DEFINE(
        [PY_SQLITE_ENABLE_LOAD_EXTENSION], [1],
        [Define to 1 to build the sqlite module with loadable extensions support.]
      )
    ])
  ], [
    AC_MSG_RESULT([no])
  ]
)

dnl
dnl Detect Tcl/Tk. Use pkg-config if available.
dnl
found_tcltk=no
for _QUERY in \
  "tcl >= 8.5.12 tk >= 8.5.12" \
  "tcl8.6 tk8.6" \
  "tcl86 tk86" \
  "tcl8.5 >= 8.5.12 tk8.5 >= 8.5.12" \
  "tcl85 >= 8.5.12 tk85 >= 8.5.12" \
; do
  PKG_CHECK_EXISTS([$_QUERY], [
    PKG_CHECK_MODULES([TCLTK], [$_QUERY], [found_tcltk=yes], [found_tcltk=no])
  ])
  AS_VAR_IF([found_tcltk], [yes], [break])
done

AS_VAR_IF([found_tcltk], [no], [
  TCLTK_CFLAGS=${TCLTK_CFLAGS-""}
  TCLTK_LIBS=${TCLTK_LIBS-""}
])

dnl FreeBSD has an X11 dependency which is not implicitly resolved.
AS_CASE([$ac_sys_system],
  [FreeBSD*], [
    PKG_CHECK_EXISTS([x11], [
      PKG_CHECK_MODULES([X11], [x11], [
        TCLTK_CFLAGS="$TCLTK_CFLAGS $X11_CFLAGS"
        TCLTK_LIBS="$TCLTK_LIBS $X11_LIBS"
      ])
    ])
  ]
)

WITH_SAVE_ENV([
  CPPFLAGS="$CPPFLAGS $TCLTK_CFLAGS"
  LIBS="$TCLTK_LIBS $LDFLAGS"

  AC_LINK_IFELSE([
    AC_LANG_PROGRAM([
      #include <tcl.h>
      #include <tk.h>
      #if defined(TK_HEX_VERSION)
      #  if TK_HEX_VERSION < 0x0805020c
      #    error "Tk older than 8.5.12 not supported"
      #  endif
      #endif
      #if (TCL_MAJOR_VERSION < 8) || \
          ((TCL_MAJOR_VERSION == 8) && (TCL_MINOR_VERSION < 5)) || \
          ((TCL_MAJOR_VERSION == 8) && (TCL_MINOR_VERSION == 5) && (TCL_RELEASE_SERIAL < 12))
      #  error "Tcl older than 8.5.12 not supported"
      #endif
      #if (TK_MAJOR_VERSION < 8) || \
          ((TK_MAJOR_VERSION == 8) && (TK_MINOR_VERSION < 5)) || \
          ((TK_MAJOR_VERSION == 8) && (TK_MINOR_VERSION == 5) && (TK_RELEASE_SERIAL < 12))
      #  error "Tk older than 8.5.12 not supported"
      #endif
    ], [
      void *x1 = Tcl_Init;
      void *x2 = Tk_Init;
    ])
  ], [
    have_tcltk=yes
    dnl The X11/xlib.h file bundled in the Tk sources can cause function
    dnl prototype warnings from the compiler. Since we cannot easily fix
    dnl that, suppress the warnings here instead.
    AS_VAR_APPEND([TCLTK_CFLAGS], [" -Wno-strict-prototypes -DWITH_APPINIT=1"])
  ], [
    have_tcltk=no
  ])
])

dnl check for _gdbmmodule dependencies
dnl NOTE: gdbm does not provide a pkgconf file.
AC_ARG_VAR([GDBM_CFLAGS], [C compiler flags for gdbm])
AC_ARG_VAR([GDBM_LIBS], [additional linker flags for gdbm])
WITH_SAVE_ENV([
  CPPFLAGS="$CPPFLAGS $GDBM_CFLAGS"
  LDFLAGS="$GDBM_LIBS $LDFLAGS"
  AC_CHECK_HEADERS([gdbm.h], [
    AC_CHECK_LIB([gdbm], [gdbm_open], [
      have_gdbm=yes
      GDBM_LIBS=${GDBM_LIBS-"-lgdbm"}
    ], [have_gdbm=no])
  ], [have_gdbm=no])
])

# check for _dbmmodule.c dependencies
AC_CHECK_HEADERS([ndbm.h], [
  LIBS_SAVE="$LIBS"
  AC_CHECK_LIB([ndbm], [dbm_open])
  LIBS="$LIBS_SAVE"
  AC_CHECK_LIB([gdbm_compat], [dbm_open])
  LIBS="$LIBS_SAVE"
])

# "gdbm-ndbm.h" and "gdbm/ndbm.h" are both normalized to "gdbm_ndbm_h"
# unset ac_cv_header_gdbm_ndbm_h to prevent false positive cache hits.
AS_UNSET([ac_cv_header_gdbm_ndbm_h])
AC_CACHE_VAL([ac_cv_header_gdbm_slash_ndbm_h], [
  AC_CHECK_HEADER(
    [gdbm/ndbm.h],
    [ac_cv_header_gdbm_slash_ndbm_h=yes], [ac_cv_header_gdbm_slash_ndbm_h=no]
  )
])
AS_VAR_IF([ac_cv_header_gdbm_slash_ndbm_h], [yes], [
  AC_DEFINE([HAVE_GDBM_NDBM_H], [1], [Define to 1 if you have the <gdbm/ndbm.h> header file.])
])

AS_UNSET([ac_cv_header_gdbm_ndbm_h])
AC_CACHE_VAL([ac_cv_header_gdbm_dash_ndbm_h], [
  AC_CHECK_HEADER(
    [gdbm-ndbm.h],
    [ac_cv_header_gdbm_dash_ndbm_h=yes], [ac_cv_header_gdbm_dash_ndbm_h=no]
  )
])
AS_VAR_IF([ac_cv_header_gdbm_dash_ndbm_h], [yes], [
  AC_DEFINE([HAVE_GDBM_DASH_NDBM_H], [1], [Define to 1 if you have the <gdbm-ndbm.h> header file.])
])
AS_UNSET([ac_cv_header_gdbm_ndbm_h])

if test "$ac_cv_header_gdbm_slash_ndbm_h" = yes -o "$ac_cv_header_gdbm_dash_ndbm_h" = yes; then
  LIBS_SAVE="$LIBS"
  AC_CHECK_LIB([gdbm_compat], [dbm_open])
  LIBS="$LIBS_SAVE"
fi

# Check for libdb >= 5 with dbm_open()
# db.h re-defines the name of the function
AC_CHECK_HEADERS([db.h], [
  AC_CACHE_CHECK([for libdb], [ac_cv_have_libdb], [
    LIBS_SAVE="$LIBS"
    LIBS="$LIBS -ldb"
    AC_LINK_IFELSE([AC_LANG_PROGRAM([
      #define DB_DBM_HSEARCH 1
      #include <db.h>
      #if DB_VERSION_MAJOR < 5
        #error "dh.h: DB_VERSION_MAJOR < 5 is not supported."
      #endif
      ], [DBM *dbm = dbm_open(NULL, 0, 0)])
    ], [ac_cv_have_libdb=yes], [ac_cv_have_libdb=no])
    LIBS="$LIBS_SAVE"
  ])
  AS_VAR_IF([ac_cv_have_libdb], [yes], [
    AC_DEFINE([HAVE_LIBDB], [1], [Define to 1 if you have the `db' library (-ldb).])
  ])
])

# Check for --with-dbmliborder
AC_MSG_CHECKING(for --with-dbmliborder)
AC_ARG_WITH(dbmliborder,
            AS_HELP_STRING([--with-dbmliborder=db1:db2:...], [override order to check db backends for dbm; a valid value is a colon separated string with the backend names `ndbm', `gdbm' and `bdb'.]),
[], [with_dbmliborder=gdbm:ndbm:bdb])

have_gdbm_dbmliborder=no
as_save_IFS=$IFS
IFS=:
for db in $with_dbmliborder; do
    AS_CASE([$db],
      [ndbm], [],
      [gdbm], [have_gdbm_dbmliborder=yes],
      [bdb], [],
      [with_dbmliborder=error]
    )
done
IFS=$as_save_IFS
AS_VAR_IF([with_dbmliborder], [error], [
  AC_MSG_ERROR([proper usage is --with-dbmliborder=db1:db2:... (gdbm:ndbm:bdb)])
])
AC_MSG_RESULT($with_dbmliborder)

# Templates for things AC_DEFINEd more than once.
# For a single AC_DEFINE, no template is needed.
AH_TEMPLATE(_REENTRANT,
  [Define to force use of thread-safe errno, h_errno, and other functions])

if test "$ac_cv_pthread_is_default" = yes
then
    # Defining _REENTRANT on system with POSIX threads should not hurt.
    AC_DEFINE(_REENTRANT)
    posix_threads=yes
    if test "$ac_sys_system" = "SunOS"; then
        CFLAGS="$CFLAGS -D_REENTRANT"
    fi
elif test "$ac_cv_kpthread" = "yes"
then
    CC="$CC -Kpthread"
    if test "$ac_cv_cxx_thread" = "yes"; then
        CXX="$CXX -Kpthread"
    fi
    posix_threads=yes
elif test "$ac_cv_kthread" = "yes"
then
    CC="$CC -Kthread"
    if test "$ac_cv_cxx_thread" = "yes"; then
        CXX="$CXX -Kthread"
    fi
    posix_threads=yes
elif test "$ac_cv_pthread" = "yes"
then
    CC="$CC -pthread"
    if test "$ac_cv_cxx_thread" = "yes"; then
        CXX="$CXX -pthread"
    fi
    posix_threads=yes
else
    if test ! -z "$withval" -a -d "$withval"
    then LDFLAGS="$LDFLAGS -L$withval"
    fi

    # According to the POSIX spec, a pthreads implementation must
    # define _POSIX_THREADS in unistd.h. Some apparently don't
    # (e.g. gnu pth with pthread emulation)
    AC_MSG_CHECKING(for _POSIX_THREADS in unistd.h)
    AC_EGREP_CPP(yes,
    [
#include <unistd.h>
#ifdef _POSIX_THREADS
yes
#endif
    ], unistd_defines_pthreads=yes, unistd_defines_pthreads=no)
    AC_MSG_RESULT($unistd_defines_pthreads)

    AC_DEFINE(_REENTRANT)
    # Just looking for pthread_create in libpthread is not enough:
    # on HP/UX, pthread.h renames pthread_create to a different symbol name.
    # So we really have to include pthread.h, and then link.
    _libs=$LIBS
    LIBS="$LIBS -lpthread"
    AC_MSG_CHECKING([for pthread_create in -lpthread])
    AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

void * start_routine (void *arg) { exit (0); }]], [[
pthread_create (NULL, NULL, start_routine, NULL)]])],[
    AC_MSG_RESULT(yes)
    posix_threads=yes
    ],[
    LIBS=$_libs
    AC_CHECK_FUNC(pthread_detach, [
    posix_threads=yes
    ],[
    AC_CHECK_LIB(pthreads, pthread_create, [
    posix_threads=yes
    LIBS="$LIBS -lpthreads"
    ], [
    AC_CHECK_LIB(c_r, pthread_create, [
    posix_threads=yes
    LIBS="$LIBS -lc_r"
    ], [
    AC_CHECK_LIB(pthread, __pthread_create_system, [
    posix_threads=yes
    LIBS="$LIBS -lpthread"
    ], [
    AC_CHECK_LIB(cma, pthread_create, [
    posix_threads=yes
    LIBS="$LIBS -lcma"
    ],[
    AS_CASE([$ac_sys_system],
      [WASI], [posix_threads=stub],
      [AC_MSG_ERROR([could not find pthreads on your system])]
    )
    ])])])])])])

    AC_CHECK_LIB(mpc, usconfig, [
    LIBS="$LIBS -lmpc"
    ])

fi

if test "$posix_threads" = "yes"; then
      if test "$unistd_defines_pthreads" = "no"; then
         AC_DEFINE(_POSIX_THREADS, 1,
         [Define if you have POSIX threads,
          and your system does not define that.])
      fi

      # Bug 662787: Using semaphores causes unexplicable hangs on Solaris 8.
      case  $ac_sys_system/$ac_sys_release in
      SunOS/5.6) AC_DEFINE(HAVE_PTHREAD_DESTRUCTOR, 1,
                       [Defined for Solaris 2.6 bug in pthread header.])
		       ;;
      SunOS/5.8) AC_DEFINE(HAVE_BROKEN_POSIX_SEMAPHORES, 1,
		       [Define if the Posix semaphores do not work on your system])
		       ;;
      AIX/*) AC_DEFINE(HAVE_BROKEN_POSIX_SEMAPHORES, 1,
		       [Define if the Posix semaphores do not work on your system])
		       ;;
      NetBSD/*) AC_DEFINE(HAVE_BROKEN_POSIX_SEMAPHORES, 1,
		       [Define if the Posix semaphores do not work on your system])
		       ;;
      esac

      AC_CACHE_CHECK([if PTHREAD_SCOPE_SYSTEM is supported], [ac_cv_pthread_system_supported],
      [AC_RUN_IFELSE([AC_LANG_SOURCE([[
      #include <stdio.h>
      #include <pthread.h>
      void *foo(void *parm) {
        return NULL;
      }
      int main(void) {
        pthread_attr_t attr;
        pthread_t id;
        if (pthread_attr_init(&attr)) return (-1);
        if (pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM)) return (-1);
        if (pthread_create(&id, &attr, foo, NULL)) return (-1);
        return (0);
      }]])],
      [ac_cv_pthread_system_supported=yes],
      [ac_cv_pthread_system_supported=no],
      [ac_cv_pthread_system_supported=no])
      ])
      if test "$ac_cv_pthread_system_supported" = "yes"; then
        AC_DEFINE(PTHREAD_SYSTEM_SCHED_SUPPORTED, 1, [Defined if PTHREAD_SCOPE_SYSTEM supported.])
      fi
      AC_CHECK_FUNCS(pthread_sigmask,
        [case $ac_sys_system in
        CYGWIN*)
          AC_DEFINE(HAVE_BROKEN_PTHREAD_SIGMASK, 1,
            [Define if pthread_sigmask() does not work on your system.])
            ;;
        esac])
      AC_CHECK_FUNCS(pthread_getcpuclockid)
fi

AS_VAR_IF([posix_threads], [stub], [
  AC_DEFINE([HAVE_PTHREAD_STUBS], [1], [Define if platform requires stubbed pthreads support])
])

# Check for enable-ipv6
AH_TEMPLATE(ENABLE_IPV6, [Define if --enable-ipv6 is specified])
AC_MSG_CHECKING([if --enable-ipv6 is specified])
AC_ARG_ENABLE(ipv6,
    AS_HELP_STRING([--enable-ipv6],
               [enable ipv6 (with ipv4) support, see Doc/library/socket.rst (default is yes if supported)]),
[ case "$enableval" in
  no)
       AC_MSG_RESULT(no)
       ipv6=no
       ;;
  *)   AC_MSG_RESULT(yes)
       AC_DEFINE(ENABLE_IPV6)
       ipv6=yes
       ;;
  esac ],

[
dnl the check does not work on cross compilation case...
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[ /* AF_INET6 available check */
#include <sys/types.h>
#include <sys/socket.h>]],
[[int domain = AF_INET6;]])],[
  ipv6=yes
],[
  ipv6=no
])

AS_CASE([$ac_sys_system],
  [WASI], [ipv6=no]
)

AC_MSG_RESULT([$ipv6])

if test "$ipv6" = "yes"; then
	AC_MSG_CHECKING(if RFC2553 API is available)
	AC_COMPILE_IFELSE([
	  AC_LANG_PROGRAM([[#include <sys/types.h>
#include <netinet/in.h>]],
			  [[struct sockaddr_in6 x;
			    x.sin6_scope_id;]])
	],[
	  AC_MSG_RESULT(yes)
	  ipv6=yes
	],[
	  AC_MSG_RESULT(no, IPv6 disabled)
	  ipv6=no
	])
fi

if test "$ipv6" = "yes"; then
	AC_DEFINE(ENABLE_IPV6)
fi
])

ipv6type=unknown
ipv6lib=none
ipv6trylibc=no

if test "$ipv6" = "yes"; then
	AC_MSG_CHECKING([ipv6 stack type])
	for i in inria kame linux-glibc linux-inet6 solaris toshiba v6d zeta;
	do
		case $i in
		inria)
			dnl http://www.kame.net/
			AC_EGREP_CPP(yes, [
#include <netinet/in.h>
#ifdef IPV6_INRIA_VERSION
yes
#endif],
				[ipv6type=$i])
			;;
		kame)
			dnl http://www.kame.net/
			AC_EGREP_CPP(yes, [
#include <netinet/in.h>
#ifdef __KAME__
yes
#endif],
				[ipv6type=$i;
				ipv6lib=inet6
				ipv6libdir=/usr/local/v6/lib
				ipv6trylibc=yes])
			;;
		linux-glibc)
			dnl http://www.v6.linux.or.jp/
			AC_EGREP_CPP(yes, [
#include <features.h>
#if defined(__GLIBC__) && ((__GLIBC__ == 2 && __GLIBC_MINOR__ >= 1) || (__GLIBC__ > 2))
yes
#endif],
				[ipv6type=$i;
				ipv6trylibc=yes])
			;;
		linux-inet6)
			dnl http://www.v6.linux.or.jp/
			if test -d /usr/inet6; then
				ipv6type=$i
				ipv6lib=inet6
				ipv6libdir=/usr/inet6/lib
				BASECFLAGS="-I/usr/inet6/include $BASECFLAGS"
			fi
			;;
		solaris)
			if test -f /etc/netconfig; then
                          if $GREP -q tcp6 /etc/netconfig; then
				ipv6type=$i
				ipv6trylibc=yes
                          fi
                        fi
			;;
		toshiba)
			AC_EGREP_CPP(yes, [
#include <sys/param.h>
#ifdef _TOSHIBA_INET6
yes
#endif],
				[ipv6type=$i;
				ipv6lib=inet6;
				ipv6libdir=/usr/local/v6/lib])
			;;
		v6d)
			AC_EGREP_CPP(yes, [
#include </usr/local/v6/include/sys/v6config.h>
#ifdef __V6D__
yes
#endif],
				[ipv6type=$i;
				ipv6lib=v6;
				ipv6libdir=/usr/local/v6/lib;
				BASECFLAGS="-I/usr/local/v6/include $BASECFLAGS"])
			;;
		zeta)
			AC_EGREP_CPP(yes, [
#include <sys/param.h>
#ifdef _ZETA_MINAMI_INET6
yes
#endif],
				[ipv6type=$i;
				ipv6lib=inet6;
				ipv6libdir=/usr/local/v6/lib])
			;;
		esac
		if test "$ipv6type" != "unknown"; then
			break
		fi
	done
	AC_MSG_RESULT($ipv6type)
fi

if test "$ipv6" = "yes" -a "$ipv6lib" != "none"; then
	if test -d $ipv6libdir -a -f $ipv6libdir/lib$ipv6lib.a; then
		LIBS="-L$ipv6libdir -l$ipv6lib $LIBS"
		AC_MSG_NOTICE([using lib$ipv6lib])
	else
    AS_VAR_IF([ipv6trylibc], [yes], [
      AC_MSG_NOTICE([using libc])
    ], [
      AC_MSG_ERROR([m4_normalize([
        No $ipv6lib library found; cannot continue.
        You need to fetch lib$ipv6lib.a from appropriate
        ipv6 kit and compile beforehand.
      ])])
    ])
	fi
fi


AC_CACHE_CHECK([CAN_RAW_FD_FRAMES], [ac_cv_can_raw_fd_frames], [
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[ /* CAN_RAW_FD_FRAMES available check */
#include <linux/can/raw.h>]],
[[int can_raw_fd_frames = CAN_RAW_FD_FRAMES;]])],
[ac_cv_can_raw_fd_frames=yes],
[ac_cv_can_raw_fd_frames=no])
])
AS_VAR_IF([ac_cv_can_raw_fd_frames], [yes], [
    AC_DEFINE(HAVE_LINUX_CAN_RAW_FD_FRAMES, 1, [Define if compiling using Linux 3.6 or later.])
])

AC_CACHE_CHECK([for CAN_RAW_JOIN_FILTERS], [ac_cv_can_raw_join_filters], [
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <linux/can/raw.h>]],
[[int can_raw_join_filters = CAN_RAW_JOIN_FILTERS;]])],
[ac_cv_can_raw_join_filters=yes],
[ac_cv_can_raw_join_filters=no])
])
AS_VAR_IF([ac_cv_can_raw_join_filters], [yes], [
    AC_DEFINE(HAVE_LINUX_CAN_RAW_JOIN_FILTERS, 1, [Define if compiling using Linux 4.1 or later.])
])

# Check for --with-doc-strings
AC_MSG_CHECKING(for --with-doc-strings)
AC_ARG_WITH(doc-strings,
            AS_HELP_STRING([--with-doc-strings], [enable documentation strings (default is yes)]))

if test -z "$with_doc_strings"
then with_doc_strings="yes"
fi
if test "$with_doc_strings" != "no"
then
    AC_DEFINE(WITH_DOC_STRINGS, 1,
      [Define if you want documentation strings in extension modules])
fi
AC_MSG_RESULT($with_doc_strings)

# Check for Python-specific malloc support
AC_MSG_CHECKING(for --with-pymalloc)
AC_ARG_WITH(pymalloc,
            AS_HELP_STRING([--with-pymalloc], [enable specialized mallocs (default is yes)]))

if test -z "$with_pymalloc"
then
  dnl default to yes except for wasm32-emscripten and wasm32-wasi.
  AS_CASE([$ac_sys_system],
    [Emscripten], [with_pymalloc="no"],
    [WASI], [with_pymalloc="no"],
    [with_pymalloc="yes"]
  )
fi
if test "$with_pymalloc" != "no"
then
    AC_DEFINE(WITH_PYMALLOC, 1,
     [Define if you want to compile in Python-specific mallocs])
fi
AC_MSG_RESULT($with_pymalloc)

# Check whether objects such as float, tuple and dict are using
# freelists to optimization memory allocation.
AC_MSG_CHECKING(for --with-freelists)
AC_ARG_WITH(freelists,
            AS_HELP_STRING([--with-freelists], [enable object freelists (default is yes)]))

if test -z "$with_freelists"
then
    with_freelists="yes"
fi
if test "$with_freelists" != "no"
then
    AC_DEFINE(WITH_FREELISTS, 1,
     [Define if you want to compile in object freelists optimization])
fi
AC_MSG_RESULT($with_freelists)

# Check for --with-c-locale-coercion
AC_MSG_CHECKING(for --with-c-locale-coercion)
AC_ARG_WITH(c-locale-coercion,
            AS_HELP_STRING([--with-c-locale-coercion],
              [enable C locale coercion to a UTF-8 based locale (default is yes)]))

if test -z "$with_c_locale_coercion"
then
    with_c_locale_coercion="yes"
fi
if test "$with_c_locale_coercion" != "no"
then
    AC_DEFINE(PY_COERCE_C_LOCALE, 1,
      [Define if you want to coerce the C locale to a UTF-8 based locale])
fi
AC_MSG_RESULT($with_c_locale_coercion)

# Check for Valgrind support
AC_MSG_CHECKING([for --with-valgrind])
AC_ARG_WITH([valgrind],
  AS_HELP_STRING([--with-valgrind], [enable Valgrind support (default is no)]),,
  with_valgrind=no)
AC_MSG_RESULT([$with_valgrind])
if test "$with_valgrind" != no; then
    AC_CHECK_HEADER([valgrind/valgrind.h],
      [AC_DEFINE([WITH_VALGRIND], 1, [Define if you want pymalloc to be disabled when running under valgrind])],
      [AC_MSG_ERROR([Valgrind support requested but headers not available])]
    )
    OPT="-DDYNAMIC_ANNOTATIONS_ENABLED=1 $OPT"
fi

# Check for DTrace support
AC_MSG_CHECKING(for --with-dtrace)
AC_ARG_WITH(dtrace,
  AS_HELP_STRING([--with-dtrace],[enable DTrace support (default is no)]),,
  with_dtrace=no)
AC_MSG_RESULT($with_dtrace)

AC_SUBST(DTRACE)
AC_SUBST(DFLAGS)
AC_SUBST(DTRACE_HEADERS)
AC_SUBST(DTRACE_OBJS)
DTRACE=
DTRACE_HEADERS=
DTRACE_OBJS=

if test "$with_dtrace" = "yes"
then
    AC_PATH_PROG(DTRACE, [dtrace], [not found])
    if test "$DTRACE" = "not found"; then
        AC_MSG_ERROR([dtrace command not found on \$PATH])
    fi
    AC_DEFINE(WITH_DTRACE, 1, [Define if you want to compile in DTrace support])
    DTRACE_HEADERS="Include/pydtrace_probes.h"

    # On OS X, DTrace providers do not need to be explicitly compiled and
    # linked into the binary. Correspondingly, dtrace(1) is missing the ELF
    # generation flag '-G'. We check for presence of this flag, rather than
    # hardcoding support by OS, in the interest of robustness.
    AC_CACHE_CHECK([whether DTrace probes require linking],
        [ac_cv_dtrace_link], [dnl
            ac_cv_dtrace_link=no
            echo 'BEGIN{}' > conftest.d
            "$DTRACE" $DFLAGS -G -s conftest.d -o conftest.o > /dev/null 2>&1 && \
                ac_cv_dtrace_link=yes
      ])
    if test "$ac_cv_dtrace_link" = "yes"; then
        DTRACE_OBJS="Python/pydtrace.o"
    fi
fi

dnl Platform-specific C and header files.
PLATFORM_HEADERS=
PLATFORM_OBJS=

AS_CASE([$ac_sys_system],
  [Emscripten], [
    AS_VAR_APPEND([PLATFORM_OBJS], [' Python/emscripten_signal.o'])
    AS_VAR_APPEND([PLATFORM_HEADERS], [' $(srcdir)/Include/internal/pycore_emscripten_signal.h'])
  ],
)
AC_SUBST([PLATFORM_HEADERS])
AC_SUBST([PLATFORM_OBJS])

# -I${DLINCLDIR} is added to the compile rule for importdl.o
AC_SUBST(DLINCLDIR)
DLINCLDIR=.

# the dlopen() function means we might want to use dynload_shlib.o. some
# platforms have dlopen(), but don't want to use it.
AC_CHECK_FUNCS(dlopen)

# DYNLOADFILE specifies which dynload_*.o file we will use for dynamic
# loading of modules.
AC_SUBST(DYNLOADFILE)
AC_MSG_CHECKING(DYNLOADFILE)
if test -z "$DYNLOADFILE"
then
	case $ac_sys_system/$ac_sys_release in
	hp*|HP*) DYNLOADFILE="dynload_hpux.o";;
	*)
	# use dynload_shlib.c and dlopen() if we have it; otherwise stub
	# out any dynamic loading
	if test "$ac_cv_func_dlopen" = yes
	then DYNLOADFILE="dynload_shlib.o"
	else DYNLOADFILE="dynload_stub.o"
	fi
	;;
	esac
fi
AC_MSG_RESULT($DYNLOADFILE)
if test "$DYNLOADFILE" != "dynload_stub.o"
then
	AC_DEFINE(HAVE_DYNAMIC_LOADING, 1,
        [Defined when any dynamic module loading is enabled.])
fi

# MACHDEP_OBJS can be set to platform-specific object files needed by Python

AC_SUBST(MACHDEP_OBJS)
AC_MSG_CHECKING(MACHDEP_OBJS)
if test -z "$MACHDEP_OBJS"
then
	MACHDEP_OBJS=$extra_machdep_objs
else
	MACHDEP_OBJS="$MACHDEP_OBJS $extra_machdep_objs"
fi
if test -z "$MACHDEP_OBJS"; then
  AC_MSG_RESULT([none])
else
  AC_MSG_RESULT([$MACHDEP_OBJS])
fi

# checks for library functions
AC_CHECK_FUNCS([ \
  accept4 alarm bind_textdomain_codeset chmod chown clock close_range confstr \
  copy_file_range ctermid dup dup3 execv explicit_bzero explicit_memset \
  faccessat fchmod fchmodat fchown fchownat fdopendir fdwalk fexecve \
  fork fork1 fpathconf fstatat ftime ftruncate futimens futimes futimesat \
  gai_strerror getegid getentropy geteuid getgid getgrgid getgrgid_r \
  getgrnam_r getgrouplist getgroups gethostname getitimer getloadavg getlogin \
  getpeername getpgid getpid getppid getpriority _getpty \
  getpwent getpwnam_r getpwuid getpwuid_r getresgid getresuid getrusage getsid getspent \
  getspnam getuid getwd if_nameindex initgroups kill killpg lchown linkat \
  lockf lstat lutimes madvise mbrtowc memrchr mkdirat mkfifo mkfifoat \
  mknod mknodat mktime mmap mremap nice openat opendir pathconf pause pipe \
  pipe2 plock poll posix_fadvise posix_fallocate posix_spawn posix_spawnp \
  pread preadv preadv2 pthread_condattr_setclock pthread_init pthread_kill \
  pwrite pwritev pwritev2 readlink readlinkat readv realpath renameat \
  rtpSpawn sched_get_priority_max sched_rr_get_interval sched_setaffinity \
  sched_setparam sched_setscheduler sem_clockwait sem_getvalue sem_open \
  sem_timedwait sem_unlink sendfile setegid seteuid setgid sethostname \
  setitimer setlocale setpgid setpgrp setpriority setregid setresgid \
  setresuid setreuid setsid setuid setvbuf shutdown sigaction sigaltstack \
  sigfillset siginterrupt sigpending sigrelse sigtimedwait sigwait \
  sigwaitinfo snprintf splice strftime strlcpy strsignal symlinkat sync \
  sysconf system tcgetpgrp tcsetpgrp tempnam timegm times tmpfile \
  tmpnam tmpnam_r truncate ttyname umask uname unlinkat utimensat utimes vfork \
  wait wait3 wait4 waitid waitpid wcscoll wcsftime wcsxfrm wmemcmp writev \
])

# Force lchmod off for Linux. Linux disallows changing the mode of symbolic
# links. Some libc implementations have a stub lchmod implementation that always
# returns an error.
if test "$MACHDEP" != linux; then
  AC_CHECK_FUNCS(lchmod)
fi

AC_CHECK_DECL(dirfd,
    AC_DEFINE(HAVE_DIRFD, 1,
              Define if you have the 'dirfd' function or macro.), ,
      [#include <sys/types.h>
       #include <dirent.h>])

dnl PY_CHECK_FUNC(FUNCTION, [INCLUDES], [AC_DEFINE-VAR])
AC_DEFUN([PY_CHECK_FUNC],
[ AS_VAR_PUSHDEF([py_var], [ac_cv_func_$1])
  AS_VAR_PUSHDEF([py_define], m4_ifblank([$3], [[HAVE_]m4_toupper($1)], [$3]))
  AC_CACHE_CHECK(
    [for $1],
    [py_var],
    [AC_COMPILE_IFELSE(
      [AC_LANG_PROGRAM([$2], [void *x=$1])],
      [AS_VAR_SET([py_var], [yes])],
      [AS_VAR_SET([py_var], [no])])]
  )
  AS_VAR_IF(
    [py_var],
    [yes],
    [AC_DEFINE([py_define], [1], [Define if you have the '$1' function.])])
  AS_VAR_POPDEF([py_var])
  AS_VAR_POPDEF([py_define])
])

# For some functions, having a definition is not sufficient, since
# we want to take their address.
PY_CHECK_FUNC([chroot], [#include <unistd.h>])
PY_CHECK_FUNC([link], [#include <unistd.h>])
PY_CHECK_FUNC([symlink], [#include <unistd.h>])
PY_CHECK_FUNC([fchdir], [#include <unistd.h>])
PY_CHECK_FUNC([fsync], [#include <unistd.h>])
PY_CHECK_FUNC([fdatasync], [#include <unistd.h>])
PY_CHECK_FUNC([epoll_create], [#include <sys/epoll.h>], [HAVE_EPOLL])
PY_CHECK_FUNC([epoll_create1], [#include <sys/epoll.h>])
PY_CHECK_FUNC([kqueue],[
#include <sys/types.h>
#include <sys/event.h>
])
PY_CHECK_FUNC([prlimit], [
#include <sys/time.h>
#include <sys/resource.h>
])

PY_CHECK_FUNC([_dyld_shared_cache_contains_path], [#include <mach-o/dyld.h>], [HAVE_DYLD_SHARED_CACHE_CONTAINS_PATH])

PY_CHECK_FUNC([memfd_create], [
#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif
#ifdef HAVE_SYS_MEMFD_H
#include <sys/memfd.h>
#endif
])

PY_CHECK_FUNC([eventfd], [
#ifdef HAVE_SYS_EVENTFD_H
#include <sys/eventfd.h>
#endif
])

# On some systems (eg. FreeBSD 5), we would find a definition of the
# functions ctermid_r, setgroups in the library, but no prototype
# (e.g. because we use _XOPEN_SOURCE). See whether we can take their
# address to avoid compiler warnings and potential miscompilations
# because of the missing prototypes.

PY_CHECK_FUNC([ctermid_r], [#include <stdio.h>])

AC_CACHE_CHECK([for flock declaration], [ac_cv_flock_decl],
  [AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM(
      [#include <sys/file.h>],
      [void* p = flock]
    )],
    [ac_cv_flock_decl=yes],
    [ac_cv_flock_decl=no]
  )
])
dnl Linking with libbsd may be necessary on AIX for flock function.
AS_VAR_IF([ac_cv_flock_decl], [yes],
  [AC_CHECK_FUNCS([flock], [],
    [AC_CHECK_LIB([bsd], [flock], [FCNTL_LIBS="-lbsd"])])])

PY_CHECK_FUNC([getpagesize], [#include <unistd.h>])

AC_CACHE_CHECK([for broken unsetenv], [ac_cv_broken_unsetenv],
  [AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM(
      [#include <stdlib.h>],
      [int res = unsetenv("DUMMY")])],
    [ac_cv_broken_unsetenv=no],
    [ac_cv_broken_unsetenv=yes]
  )
])
AS_VAR_IF([ac_cv_broken_unsetenv], [yes], [
  AC_DEFINE(HAVE_BROKEN_UNSETENV, 1, [Define if 'unsetenv' does not return an int.])
])

dnl check for true
AC_CHECK_PROGS(TRUE, true, /bin/true)

dnl On some systems (e.g. Solaris 9), hstrerror and inet_aton are in -lresolv
dnl On others, they are in the C library, so we to take no action
AC_CHECK_LIB(c, inet_aton, [$ac_cv_prog_TRUE],
  AC_CHECK_LIB(resolv, inet_aton)
)

# On Tru64, chflags seems to be present, but calling it will
# exit Python
AC_CACHE_CHECK([for chflags], [ac_cv_have_chflags], [dnl
AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <sys/stat.h>
#include <unistd.h>
int main(int argc, char *argv[])
{
  if(chflags(argv[0], 0) != 0)
    return 1;
  return 0;
}
]])],
[ac_cv_have_chflags=yes],
[ac_cv_have_chflags=no],
[ac_cv_have_chflags=cross])
])
if test "$ac_cv_have_chflags" = cross ; then
  AC_CHECK_FUNC([chflags], [ac_cv_have_chflags="yes"], [ac_cv_have_chflags="no"])
fi
if test "$ac_cv_have_chflags" = yes ; then
  AC_DEFINE(HAVE_CHFLAGS, 1, [Define to 1 if you have the 'chflags' function.])
fi

AC_CACHE_CHECK([for lchflags], [ac_cv_have_lchflags], [dnl
AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <sys/stat.h>
#include <unistd.h>
int main(int argc, char *argv[])
{
  if(lchflags(argv[0], 0) != 0)
    return 1;
  return 0;
}
]])],[ac_cv_have_lchflags=yes],[ac_cv_have_lchflags=no],[ac_cv_have_lchflags=cross])
])
if test "$ac_cv_have_lchflags" = cross ; then
  AC_CHECK_FUNC([lchflags], [ac_cv_have_lchflags="yes"], [ac_cv_have_lchflags="no"])
fi
if test "$ac_cv_have_lchflags" = yes ; then
  AC_DEFINE(HAVE_LCHFLAGS, 1, [Define to 1 if you have the 'lchflags' function.])
fi

dnl Check for compression libraries
AH_TEMPLATE([HAVE_ZLIB_COPY], [Define if the zlib library has inflateCopy])

dnl detect zlib from Emscripten emport
PY_CHECK_EMSCRIPTEN_PORT([ZLIB], [-sUSE_ZLIB])

PKG_CHECK_MODULES([ZLIB], [zlib >= 1.2.0], [
  have_zlib=yes
  dnl zlib 1.2.0 (2003) added inflateCopy
  AC_DEFINE([HAVE_ZLIB_COPY], [1])
], [
  WITH_SAVE_ENV([
    CPPFLAGS="$CPPFLAGS $ZLIB_CFLAGS"
    LDFLAGS="$LDFLAGS $ZLIB_LIBS"
    AC_CHECK_HEADERS([zlib.h], [
      PY_CHECK_LIB([z], [gzread], [have_zlib=yes], [have_zlib=no])
    ], [have_zlib=no])
    AS_VAR_IF([have_zlib], [yes], [
      ZLIB_CFLAGS=${ZLIB_CFLAGS-""}
      ZLIB_LIBS=${ZLIB_LIBS-"-lz"}
      PY_CHECK_LIB([z], [inflateCopy], [AC_DEFINE([HAVE_ZLIB_COPY], [1])])
    ])
  ])
])

dnl binascii can use zlib for optimized crc32.
AS_VAR_IF([have_zlib], [yes], [
  BINASCII_CFLAGS="-DUSE_ZLIB_CRC32 $ZLIB_CFLAGS"
  BINASCII_LIBS="$ZLIB_LIBS"
])

dnl detect bzip2 from Emscripten emport
PY_CHECK_EMSCRIPTEN_PORT([BZIP2], [-sUSE_BZIP2])

PKG_CHECK_MODULES([BZIP2], [bzip2], [have_bzip2=yes], [
  WITH_SAVE_ENV([
    CPPFLAGS="$CPPFLAGS $BZIP2_CFLAGS"
    LDFLAGS="$LDFLAGS $BZIP2_LIBS"
    AC_CHECK_HEADERS([bzlib.h], [
      AC_CHECK_LIB([bz2], [BZ2_bzCompress], [have_bzip2=yes], [have_bzip2=no])
    ], [have_bzip2=no])
    AS_VAR_IF([have_bzip2], [yes], [
      BZIP2_CFLAGS=${BZIP2_CFLAGS-""}
      BZIP2_LIBS=${BZIP2_LIBS-"-lbz2"}
    ])
  ])
])

PKG_CHECK_MODULES([LIBLZMA], [liblzma], [have_liblzma=yes], [
  WITH_SAVE_ENV([
    CPPFLAGS="$CPPFLAGS $LIBLZMA_CFLAGS"
    LDFLAGS="$LDFLAGS $LIBLZMA_LIBS"
    AC_CHECK_HEADERS([lzma.h], [
      AC_CHECK_LIB([lzma], [lzma_easy_encoder], [have_liblzma=yes], [have_liblzma=no])
    ], [have_liblzma=no])
    AS_VAR_IF([have_liblzma], [yes], [
      LIBLZMA_CFLAGS=${LIBLZMA_CFLAGS-""}
      LIBLZMA_LIBS=${LIBLZMA_LIBS-"-llzma"}
    ])
  ])
])

dnl PY_CHECK_NETDB_FUNC(FUNCTION)
AC_DEFUN([PY_CHECK_NETDB_FUNC], [PY_CHECK_FUNC([$1], [#include <netdb.h>])])

PY_CHECK_NETDB_FUNC([hstrerror])
dnl not available in WASI yet
PY_CHECK_NETDB_FUNC([getservbyname])
PY_CHECK_NETDB_FUNC([getservbyport])
PY_CHECK_NETDB_FUNC([gethostbyname])
PY_CHECK_NETDB_FUNC([gethostbyaddr])
PY_CHECK_NETDB_FUNC([getprotobyname])

dnl PY_CHECK_SOCKET_FUNC(FUNCTION)
AC_DEFUN([PY_CHECK_SOCKET_FUNC], [PY_CHECK_FUNC([$1], [
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
])])

PY_CHECK_SOCKET_FUNC([inet_aton])
PY_CHECK_SOCKET_FUNC([inet_ntoa])
PY_CHECK_SOCKET_FUNC([inet_pton])
dnl not available in WASI yet
PY_CHECK_SOCKET_FUNC([getpeername])
PY_CHECK_SOCKET_FUNC([getsockname])
PY_CHECK_SOCKET_FUNC([accept])
PY_CHECK_SOCKET_FUNC([bind])
PY_CHECK_SOCKET_FUNC([connect])
PY_CHECK_SOCKET_FUNC([listen])
PY_CHECK_SOCKET_FUNC([recvfrom])
PY_CHECK_SOCKET_FUNC([sendto])
PY_CHECK_SOCKET_FUNC([setsockopt])
PY_CHECK_SOCKET_FUNC([socket])

# On some systems, setgroups is in unistd.h, on others, in grp.h
PY_CHECK_FUNC([setgroups], [
#include <unistd.h>
#ifdef HAVE_GRP_H
#include <grp.h>
#endif
])

# check for openpty, login_tty, and forkpty

AC_CHECK_FUNCS(openpty,,
   AC_CHECK_LIB(util,openpty,
     [AC_DEFINE(HAVE_OPENPTY) LIBS="$LIBS -lutil"],
     AC_CHECK_LIB(bsd,openpty, [AC_DEFINE(HAVE_OPENPTY) LIBS="$LIBS -lbsd"])
   )
)
AC_SEARCH_LIBS([login_tty], [util],
 [AC_DEFINE([HAVE_LOGIN_TTY], [1], [Define to 1 if you have the `login_tty' function.])]
)
AC_CHECK_FUNCS(forkpty,,
   AC_CHECK_LIB(util,forkpty,
     [AC_DEFINE(HAVE_FORKPTY) LIBS="$LIBS -lutil"],
     AC_CHECK_LIB(bsd,forkpty, [AC_DEFINE(HAVE_FORKPTY) LIBS="$LIBS -lbsd"])
   )
)

# check for long file support functions
AC_CHECK_FUNCS(fseek64 fseeko fstatvfs ftell64 ftello statvfs)

AC_REPLACE_FUNCS(dup2)
AC_CHECK_FUNCS(getpgrp,
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <unistd.h>]], [[getpgrp(0);]])],
    [AC_DEFINE(GETPGRP_HAVE_ARG, 1, [Define if getpgrp() must be called as getpgrp(0).])],
    [])
)
AC_CHECK_FUNCS(setpgrp,
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <unistd.h>]], [[setpgrp(0,0);]])],
    [AC_DEFINE(SETPGRP_HAVE_ARG, 1, [Define if setpgrp() must be called as setpgrp(0, 0).])],
    [])
)

dnl We search for both crypt and crypt_r as one or the other may be defined
dnl libxcrypt provides <crypt.h> and libcrypt with crypt_r() since
dnl at least 3.1.1 from 2015.
dnl FreeBSD defines crypt_r() in <unistd.h>
AH_TEMPLATE([HAVE_CRYPT_R], [Define if you have the crypt_r() function.])

PKG_CHECK_MODULES([LIBCRYPT], [libxcrypt >= 3.1.1], [
  AC_DEFINE([HAVE_CRYPT_R], [1])
], [
  WITH_SAVE_ENV([
    AC_SEARCH_LIBS([crypt_r], [crypt], [
      AC_DEFINE([HAVE_CRYPT_R], [1])
      if test "$ac_cv_search_crypt_r" = "none required"; then
        libcrypt=
      else
        libcrypt="$ac_cv_search_crypt_r"
      fi
      LIBCRYPT_LIBS=${LIBCRYPT_LIBS-$libcrypt}
    ])
  ])
])

WITH_SAVE_ENV([
  CPPFLAGS="$CPPFLAGS $LIBCRYPT_CFLAGS"
  LIBS="$LIBCRYPT_LIBS $LIBS"
  AC_CACHE_CHECK([for crypt or crypt_r], [ac_cv_crypt_crypt], [
    AC_LINK_IFELSE([AC_LANG_PROGRAM([
        #ifdef HAVE_CRYPT_H
          #include <crypt.h>
        #endif
        #include <unistd.h>
      ], [
        #ifdef HAVE_CRYPT_R
          void *x = crypt_r;
        #else
          void *x = crypt;
        #endif
      ])
    ], [ac_cv_crypt_crypt=yes], [ac_cv_crypt_crypt=no])
  ])
])

AC_CHECK_FUNCS(clock_gettime, [], [
    AC_CHECK_LIB(rt, clock_gettime, [
        LIBS="$LIBS -lrt"
        AC_DEFINE(HAVE_CLOCK_GETTIME, 1)
        AC_DEFINE(TIMEMODULE_LIB, [rt],
                  [Library needed by timemodule.c: librt may be needed for clock_gettime()])
    ])
])

AC_CHECK_FUNCS(clock_getres, [], [
    AC_CHECK_LIB(rt, clock_getres, [
        AC_DEFINE(HAVE_CLOCK_GETRES, 1)
    ])
])

AC_CHECK_FUNCS(clock_settime, [], [
    AC_CHECK_LIB(rt, clock_settime, [
        AC_DEFINE(HAVE_CLOCK_SETTIME, 1)
    ])
])

AC_CHECK_FUNCS(clock_nanosleep, [], [
    AC_CHECK_LIB(rt, clock_nanosleep, [
        AC_DEFINE(HAVE_CLOCK_NANOSLEEP, 1)
    ])
])

AC_CHECK_FUNCS(nanosleep, [], [
    AC_CHECK_LIB(rt, nanosleep, [
        AC_DEFINE(HAVE_NANOSLEEP, 1)
    ])
])

AC_CACHE_CHECK([for major, minor, and makedev], [ac_cv_device_macros], [
AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#if defined(MAJOR_IN_MKDEV)
#include <sys/mkdev.h>
#elif defined(MAJOR_IN_SYSMACROS)
#include <sys/sysmacros.h>
#else
#include <sys/types.h>
#endif
]], [[
  makedev(major(0),minor(0));
]])],[ac_cv_device_macros=yes], [ac_cv_device_macros=no])
])
AS_VAR_IF([ac_cv_device_macros], [yes], [
  AC_DEFINE(HAVE_DEVICE_MACROS, 1,
	    [Define to 1 if you have the device macros.])
])

dnl no longer used, now always defined for backwards compatibility
AC_DEFINE(SYS_SELECT_WITH_SYS_TIME, 1,
  [Define if  you can safely include both <sys/select.h> and <sys/time.h>
   (which you can't on SCO ODT 3.0).])

# On OSF/1 V5.1, getaddrinfo is available, but a define
# for [no]getaddrinfo in netdb.h.
AC_CACHE_CHECK([for getaddrinfo], [ac_cv_func_getaddrinfo], [
AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
]], [[getaddrinfo(NULL, NULL, NULL, NULL);]])],
[ac_cv_func_getaddrinfo=yes],
[ac_cv_func_getaddrinfo=no])
])

AS_VAR_IF([ac_cv_func_getaddrinfo], [yes], [
  AC_CACHE_CHECK([getaddrinfo bug], [ac_cv_buggy_getaddrinfo],
  AC_RUN_IFELSE([AC_LANG_SOURCE([[[
#include <stdio.h>
#include <sys/types.h>
#include <netdb.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>

int main(void)
{
  int passive, gaierr, inet4 = 0, inet6 = 0;
  struct addrinfo hints, *ai, *aitop;
  char straddr[INET6_ADDRSTRLEN], strport[16];

  for (passive = 0; passive <= 1; passive++) {
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_flags = passive ? AI_PASSIVE : 0;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    if ((gaierr = getaddrinfo(NULL, "54321", &hints, &aitop)) != 0) {
      (void)gai_strerror(gaierr);
      goto bad;
    }
    for (ai = aitop; ai; ai = ai->ai_next) {
      if (ai->ai_addr == NULL ||
          ai->ai_addrlen == 0 ||
          getnameinfo(ai->ai_addr, ai->ai_addrlen,
                      straddr, sizeof(straddr), strport, sizeof(strport),
                      NI_NUMERICHOST|NI_NUMERICSERV) != 0) {
        goto bad;
      }
      switch (ai->ai_family) {
      case AF_INET:
        if (strcmp(strport, "54321") != 0) {
          goto bad;
        }
        if (passive) {
          if (strcmp(straddr, "0.0.0.0") != 0) {
            goto bad;
          }
        } else {
          if (strcmp(straddr, "127.0.0.1") != 0) {
            goto bad;
          }
        }
        inet4++;
        break;
      case AF_INET6:
        if (strcmp(strport, "54321") != 0) {
          goto bad;
        }
        if (passive) {
          if (strcmp(straddr, "::") != 0) {
            goto bad;
          }
        } else {
          if (strcmp(straddr, "::1") != 0) {
            goto bad;
          }
        }
        inet6++;
        break;
      case AF_UNSPEC:
        goto bad;
        break;
      default:
        /* another family support? */
        break;
      }
    }
    freeaddrinfo(aitop);
    aitop = NULL;
  }

  if (!(inet4 == 0 || inet4 == 2))
    goto bad;
  if (!(inet6 == 0 || inet6 == 2))
    goto bad;

  if (aitop)
    freeaddrinfo(aitop);
  return 0;

 bad:
  if (aitop)
    freeaddrinfo(aitop);
  return 1;
}
]]])],
[ac_cv_buggy_getaddrinfo=no],
[ac_cv_buggy_getaddrinfo=yes],
[
if test "${enable_ipv6+set}" = set; then
  ac_cv_buggy_getaddrinfo="no -- configured with --(en|dis)able-ipv6"
else
  ac_cv_buggy_getaddrinfo=yes
fi]))

dnl if ac_cv_func_getaddrinfo
])

if test "$ac_cv_func_getaddrinfo" = no -o "$ac_cv_buggy_getaddrinfo" = yes
then
  AS_VAR_IF([ipv6], [yes], [
    AC_MSG_ERROR([m4_normalize([
      You must get working getaddrinfo() function
      or pass the "--disable-ipv6" option to configure.
    ])])
  ])
else
	AC_DEFINE(HAVE_GETADDRINFO, 1, [Define if you have the getaddrinfo function.])
fi

AC_CHECK_FUNCS(getnameinfo)

dnl autoconf 2.71 deprecates AC_HEADER_TIME, keep for backwards compatibility
dnl TIME_WITH_SYS_TIME works on all supported systems that have sys/time.h
AS_VAR_IF([ac_cv_header_sys_time_h], [yes], [
  AC_DEFINE([TIME_WITH_SYS_TIME], 1, [Define to 1 if you can safely include both <sys/time.h> and <time.h>.])
])

# checks for structures
AC_STRUCT_TM
AC_STRUCT_TIMEZONE
AC_CHECK_MEMBERS([struct stat.st_rdev])
AC_CHECK_MEMBERS([struct stat.st_blksize])
AC_CHECK_MEMBERS([struct stat.st_flags])
AC_CHECK_MEMBERS([struct stat.st_gen])
AC_CHECK_MEMBERS([struct stat.st_birthtime])
AC_CHECK_MEMBERS([struct stat.st_blocks])
AC_CHECK_MEMBERS([struct passwd.pw_gecos, struct passwd.pw_passwd], [], [], [[
  #include <sys/types.h>
  #include <pwd.h>
]])
# Issue #21085: In Cygwin, siginfo_t does not have si_band field.
AC_CHECK_MEMBERS([siginfo_t.si_band], [], [], [[#include <signal.h>]])

AC_CACHE_CHECK([for time.h that defines altzone], [ac_cv_header_time_altzone], [
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <time.h>]], [[return altzone;]])],
    [ac_cv_header_time_altzone=yes],
    [ac_cv_header_time_altzone=no])
  ])
if test $ac_cv_header_time_altzone = yes; then
  AC_DEFINE(HAVE_ALTZONE, 1, [Define this if your time.h defines altzone.])
fi

AC_CACHE_CHECK([for addrinfo], [ac_cv_struct_addrinfo],
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <netdb.h>]], [[struct addrinfo a]])],
  [ac_cv_struct_addrinfo=yes],
  [ac_cv_struct_addrinfo=no]))
if test $ac_cv_struct_addrinfo = yes; then
	AC_DEFINE(HAVE_ADDRINFO, 1, [struct addrinfo (netdb.h)])
fi

AC_CACHE_CHECK([for sockaddr_storage], [ac_cv_struct_sockaddr_storage],
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#		include <sys/types.h>
#		include <sys/socket.h>]], [[struct sockaddr_storage s]])],
  [ac_cv_struct_sockaddr_storage=yes],
  [ac_cv_struct_sockaddr_storage=no]))
if test $ac_cv_struct_sockaddr_storage = yes; then
	AC_DEFINE(HAVE_SOCKADDR_STORAGE, 1, [struct sockaddr_storage (sys/socket.h)])
fi

AC_CACHE_CHECK([for sockaddr_alg], [ac_cv_struct_sockaddr_alg],
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#		include <sys/types.h>
#		include <sys/socket.h>
#		include <linux/if_alg.h>]], [[struct sockaddr_alg s]])],
  [ac_cv_struct_sockaddr_alg=yes],
  [ac_cv_struct_sockaddr_alg=no]))
if test $ac_cv_struct_sockaddr_alg = yes; then
	AC_DEFINE(HAVE_SOCKADDR_ALG, 1, [struct sockaddr_alg (linux/if_alg.h)])
fi

# checks for compiler characteristics

AC_C_CONST

AC_CACHE_CHECK([for working signed char], [ac_cv_working_signed_char_c], [
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[signed char c;]])],
  [ac_cv_working_signed_char_c=yes], [ac_cv_working_signed_char_c=no])
])
AS_VAR_IF([ac_cv_working_signed_char_c], [no], [
  AC_DEFINE(signed, , [Define to empty if the keyword does not work.])
])

AC_CACHE_CHECK([for prototypes], [ac_cv_function_prototypes], [
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[int foo(int x) { return 0; }]], [[return foo(10);]])],
  [ac_cv_function_prototypes=yes], [ac_cv_function_prototypes=no])
])
AS_VAR_IF([ac_cv_function_prototypes], [yes], [
  AC_DEFINE(HAVE_PROTOTYPES, 1,
     [Define if your compiler supports function prototype])
])

works=no
AC_CACHE_CHECK([for variable length prototypes and stdarg.h], [ac_cv_stdarg_prototypes], [
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <stdarg.h>
int foo(int x, ...) {
	va_list va;
	va_start(va, x);
	va_arg(va, int);
	va_arg(va, char *);
	va_arg(va, double);
	return 0;
}
]], [[return foo(10, "", 3.14);]])],
  [ac_cv_stdarg_prototypes=yes], [ac_cv_stdarg_prototypes=no])
])
AS_VAR_IF([ac_cv_stdarg_prototypes], [yes], [
  AC_DEFINE(HAVE_STDARG_PROTOTYPES, 1,
   [Define if your compiler supports variable length function prototypes
   (e.g. void fprintf(FILE *, char *, ...);) *and* <stdarg.h>])
])


# check for socketpair
PY_CHECK_FUNC([socketpair], [
#include <sys/types.h>
#include <sys/socket.h>
])

# check if sockaddr has sa_len member
AC_CACHE_CHECK([if sockaddr has sa_len member], [ac_cv_struct_sockaddr_sa_len], [
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <sys/types.h>
#include <sys/socket.h>]], [[struct sockaddr x;
x.sa_len = 0;]])],
  [ac_cv_struct_sockaddr_sa_len=yes], [ac_cv_struct_sockaddr_sa_len=no])
])
AS_VAR_IF([ac_cv_struct_sockaddr_sa_len], [yes], [
   AC_DEFINE(HAVE_SOCKADDR_SA_LEN, 1, [Define if sockaddr has sa_len member])
])

# sigh -- gethostbyname_r is a mess; it can have 3, 5 or 6 arguments :-(
AH_TEMPLATE(HAVE_GETHOSTBYNAME_R,
  [Define this if you have some version of gethostbyname_r()])

AC_CHECK_FUNC(gethostbyname_r, [
  AC_DEFINE(HAVE_GETHOSTBYNAME_R)
  AC_MSG_CHECKING([gethostbyname_r with 6 args])
  OLD_CFLAGS=$CFLAGS
  CFLAGS="$CFLAGS $MY_CPPFLAGS $MY_THREAD_CPPFLAGS $MY_CFLAGS"
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#   include <netdb.h>
  ]], [[
    char *name;
    struct hostent *he, *res;
    char buffer[2048];
    int buflen = 2048;
    int h_errnop;

    (void) gethostbyname_r(name, he, buffer, buflen, &res, &h_errnop)
  ]])],[
    AC_DEFINE(HAVE_GETHOSTBYNAME_R)
    AC_DEFINE(HAVE_GETHOSTBYNAME_R_6_ARG, 1,
    [Define this if you have the 6-arg version of gethostbyname_r().])
    AC_MSG_RESULT(yes)
  ],[
    AC_MSG_RESULT(no)
    AC_MSG_CHECKING([gethostbyname_r with 5 args])
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#       include <netdb.h>
      ]], [[
        char *name;
        struct hostent *he;
        char buffer[2048];
        int buflen = 2048;
        int h_errnop;

        (void) gethostbyname_r(name, he, buffer, buflen, &h_errnop)
      ]])],
      [
        AC_DEFINE(HAVE_GETHOSTBYNAME_R)
        AC_DEFINE(HAVE_GETHOSTBYNAME_R_5_ARG, 1,
          [Define this if you have the 5-arg version of gethostbyname_r().])
        AC_MSG_RESULT(yes)
      ], [
        AC_MSG_RESULT(no)
        AC_MSG_CHECKING([gethostbyname_r with 3 args])
        AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#           include <netdb.h>
          ]], [[
            char *name;
            struct hostent *he;
            struct hostent_data data;

            (void) gethostbyname_r(name, he, &data);
          ]])],
          [
            AC_DEFINE(HAVE_GETHOSTBYNAME_R)
            AC_DEFINE(HAVE_GETHOSTBYNAME_R_3_ARG, 1,
              [Define this if you have the 3-arg version of gethostbyname_r().])
            AC_MSG_RESULT(yes)
          ], [
           AC_MSG_RESULT(no)
        ])
    ])
  ])
  CFLAGS=$OLD_CFLAGS
], [
  AC_CHECK_FUNCS(gethostbyname)
])
AC_SUBST(HAVE_GETHOSTBYNAME_R_6_ARG)
AC_SUBST(HAVE_GETHOSTBYNAME_R_5_ARG)
AC_SUBST(HAVE_GETHOSTBYNAME_R_3_ARG)
AC_SUBST(HAVE_GETHOSTBYNAME_R)
AC_SUBST(HAVE_GETHOSTBYNAME)

# checks for system services
# (none yet)

# Linux requires this for correct f.p. operations
AC_CHECK_FUNC(__fpu_control,
  [],
  [AC_CHECK_LIB(ieee, __fpu_control)
])

# check for --with-libm=...
AC_SUBST(LIBM)
case $ac_sys_system in
Darwin) ;;
*) LIBM=-lm
esac
AC_MSG_CHECKING(for --with-libm=STRING)
AC_ARG_WITH(libm,
            AS_HELP_STRING([--with-libm=STRING], [override libm math library to STRING (default is system-dependent)]),
[
if test "$withval" = no
then LIBM=
     AC_MSG_RESULT(force LIBM empty)
elif test "$withval" != yes
then LIBM=$withval
     AC_MSG_RESULT(set LIBM="$withval")
else AC_MSG_ERROR([proper usage is --with-libm=STRING])
fi],
[AC_MSG_RESULT(default LIBM="$LIBM")])

# check for --with-libc=...
AC_SUBST(LIBC)
AC_MSG_CHECKING(for --with-libc=STRING)
AC_ARG_WITH(libc,
            AS_HELP_STRING([--with-libc=STRING], [override libc C library to STRING (default is system-dependent)]),
[
if test "$withval" = no
then LIBC=
     AC_MSG_RESULT(force LIBC empty)
elif test "$withval" != yes
then LIBC=$withval
     AC_MSG_RESULT(set LIBC="$withval")
else AC_MSG_ERROR([proper usage is --with-libc=STRING])
fi],
[AC_MSG_RESULT(default LIBC="$LIBC")])

# **************************************
# * Check for gcc x64 inline assembler *
# **************************************


AC_CACHE_CHECK([for x64 gcc inline assembler], [ac_cv_gcc_asm_for_x64], [
AC_LINK_IFELSE([AC_LANG_PROGRAM([[]], [[
  __asm__ __volatile__ ("movq %rcx, %rax");
]])],[ac_cv_gcc_asm_for_x64=yes],[ac_cv_gcc_asm_for_x64=no])
])

AS_VAR_IF([ac_cv_gcc_asm_for_x64], [yes], [
    AC_DEFINE(HAVE_GCC_ASM_FOR_X64, 1,
    [Define if we can use x64 gcc inline assembler])
])

# **************************************************
# * Check for various properties of floating point *
# **************************************************

AX_C_FLOAT_WORDS_BIGENDIAN
if test "$ax_cv_c_float_words_bigendian" = "yes"
then
  AC_DEFINE(DOUBLE_IS_BIG_ENDIAN_IEEE754, 1,
  [Define if C doubles are 64-bit IEEE 754 binary format, stored
   with the most significant byte first])
elif test "$ax_cv_c_float_words_bigendian" = "no"
then
  AC_DEFINE(DOUBLE_IS_LITTLE_ENDIAN_IEEE754, 1,
  [Define if C doubles are 64-bit IEEE 754 binary format, stored
   with the least significant byte first])
else
  # Some ARM platforms use a mixed-endian representation for doubles.
  # While Python doesn't currently have full support for these platforms
  # (see e.g., issue 1762561), we can at least make sure that float <-> string
  # conversions work.
  # FLOAT_WORDS_BIGENDIAN doesnt actually detect this case, but if it's not big
  # or little, then it must be this?
  AC_DEFINE(DOUBLE_IS_ARM_MIXED_ENDIAN_IEEE754, 1,
  [Define if C doubles are 64-bit IEEE 754 binary format, stored
   in ARM mixed-endian order (byte order 45670123)])
fi

# The short float repr introduced in Python 3.1 requires the
# correctly-rounded string <-> double conversion functions from
# Python/dtoa.c, which in turn require that the FPU uses 53-bit
# rounding; this is a problem on x86, where the x87 FPU has a default
# rounding precision of 64 bits.  For gcc/x86, we can fix this by
# using inline assembler to get and set the x87 FPU control word.

# This inline assembler syntax may also work for suncc and icc,
# so we try it on all platforms.

AC_CACHE_CHECK([whether we can use gcc inline assembler to get and set x87 control word], [ac_cv_gcc_asm_for_x87], [
AC_LINK_IFELSE(   [AC_LANG_PROGRAM([[]], [[
  unsigned short cw;
  __asm__ __volatile__ ("fnstcw %0" : "=m" (cw));
  __asm__ __volatile__ ("fldcw %0" : : "m" (cw));
]])],[ac_cv_gcc_asm_for_x87=yes],[ac_cv_gcc_asm_for_x87=no])
])
AS_VAR_IF([ac_cv_gcc_asm_for_x87], [yes], [
    AC_DEFINE(HAVE_GCC_ASM_FOR_X87, 1,
    [Define if we can use gcc inline assembler to get and set x87 control word])
])

AC_CACHE_CHECK([whether we can use gcc inline assembler to get and set mc68881 fpcr], [ac_cv_gcc_asm_for_mc68881], [
AC_LINK_IFELSE(   [AC_LANG_PROGRAM([[]], [[
  unsigned int fpcr;
  __asm__ __volatile__ ("fmove.l %%fpcr,%0" : "=g" (fpcr));
  __asm__ __volatile__ ("fmove.l %0,%%fpcr" : : "g" (fpcr));
]])],[ac_cv_gcc_asm_for_mc68881=yes],[ac_cv_gcc_asm_for_mc68881=no])
])
AS_VAR_IF([ac_cv_gcc_asm_for_mc68881], [yes], [
    AC_DEFINE(HAVE_GCC_ASM_FOR_MC68881, 1,
    [Define if we can use gcc inline assembler to get and set mc68881 fpcr])
])

# Detect whether system arithmetic is subject to x87-style double
# rounding issues.  The result of this test has little meaning on non
# IEEE 754 platforms.  On IEEE 754, test should return 1 if rounding
# mode is round-to-nearest and double rounding issues are present, and
# 0 otherwise.  See https://github.com/python/cpython/issues/47186 for more info.
AC_CACHE_CHECK([for x87-style double rounding], [ac_cv_x87_double_rounding], [
# $BASECFLAGS may affect the result
ac_save_cc="$CC"
CC="$CC $BASECFLAGS"
AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <stdlib.h>
#include <math.h>
int main(void) {
    volatile double x, y, z;
    /* 1./(1-2**-53) -> 1+2**-52 (correct), 1.0 (double rounding) */
    x = 0.99999999999999989; /* 1-2**-53 */
    y = 1./x;
    if (y != 1.)
        exit(0);
    /* 1e16+2.99999 -> 1e16+2. (correct), 1e16+4. (double rounding) */
    x = 1e16;
    y = 2.99999;
    z = x + y;
    if (z != 1e16+4.)
        exit(0);
    /* both tests show evidence of double rounding */
    exit(1);
}
]])],
[ac_cv_x87_double_rounding=no],
[ac_cv_x87_double_rounding=yes],
[ac_cv_x87_double_rounding=no])
CC="$ac_save_cc"
])

AS_VAR_IF([ac_cv_x87_double_rounding], [yes], [
  AC_DEFINE(X87_DOUBLE_ROUNDING, 1,
  [Define if arithmetic is subject to x87-style double rounding issue])
])

# ************************************
# * Check for mathematical functions *
# ************************************

LIBS_SAVE=$LIBS
LIBS="$LIBS $LIBM"

AC_CHECK_FUNCS(
  [acosh asinh atanh erf erfc expm1 log1p log2],
  [],
  [AC_MSG_ERROR([Python requires C99 compatible libm])]
)
LIBS=$LIBS_SAVE

dnl For multiprocessing module, check that sem_open
dnl actually works.  For FreeBSD versions <= 7.2,
dnl the kernel module that provides POSIX semaphores
dnl isn't loaded by default, so an attempt to call
dnl sem_open results in a 'Signal 12' error.
AC_CACHE_CHECK([whether POSIX semaphores are enabled], [ac_cv_posix_semaphores_enabled],
  AC_RUN_IFELSE([
    AC_LANG_SOURCE([
      #include <unistd.h>
      #include <fcntl.h>
      #include <stdio.h>
      #include <semaphore.h>
      #include <sys/stat.h>

      int main(void) {
        sem_t *a = sem_open("/autoconf", O_CREAT, S_IRUSR|S_IWUSR, 0);
        if (a == SEM_FAILED) {
          perror("sem_open");
          return 1;
        }
        sem_close(a);
        sem_unlink("/autoconf");
        return 0;
      }
    ])
  ],
  [ac_cv_posix_semaphores_enabled=yes],
  [ac_cv_posix_semaphores_enabled=no],
  [ac_cv_posix_semaphores_enabled=yes])
)
AS_VAR_IF([ac_cv_posix_semaphores_enabled], [no], [
  AC_DEFINE(
    [POSIX_SEMAPHORES_NOT_ENABLED], [1],
    [Define if POSIX semaphores aren't enabled on your system]
  )
])

dnl Multiprocessing check for broken sem_getvalue
AC_CACHE_CHECK([for broken sem_getvalue], [ac_cv_broken_sem_getvalue],
  AC_RUN_IFELSE([
    AC_LANG_SOURCE([
      #include <unistd.h>
      #include <fcntl.h>
      #include <stdio.h>
      #include <semaphore.h>
      #include <sys/stat.h>

      int main(void){
        sem_t *a = sem_open("/autocftw", O_CREAT, S_IRUSR|S_IWUSR, 0);
        int count;
        int res;
        if(a==SEM_FAILED){
          perror("sem_open");
          return 1;

        }
        res = sem_getvalue(a, &count);
        sem_close(a);
        sem_unlink("/autocftw");
        return res==-1 ? 1 : 0;
      }
    ])
  ],
  [ac_cv_broken_sem_getvalue=no],
  [ac_cv_broken_sem_getvalue=yes],
  [ac_cv_broken_sem_getvalue=yes])
)
AS_VAR_IF([ac_cv_broken_sem_getvalue], [yes], [
  AC_DEFINE(
    [HAVE_BROKEN_SEM_GETVALUE], [1],
    [define to 1 if your sem_getvalue is broken.]
  )
])

AC_CHECK_DECLS([RTLD_LAZY, RTLD_NOW, RTLD_GLOBAL, RTLD_LOCAL, RTLD_NODELETE, RTLD_NOLOAD, RTLD_DEEPBIND, RTLD_MEMBER], [], [], [[#include <dlfcn.h>]])

# determine what size digit to use for Python's longs
AC_MSG_CHECKING([digit size for Python's longs])
AC_ARG_ENABLE(big-digits,
AS_HELP_STRING([--enable-big-digits@<:@=15|30@:>@],[use big digits (30 or 15 bits) for Python longs (default is 30)]]),
[case $enable_big_digits in
yes)
  enable_big_digits=30 ;;
no)
  enable_big_digits=15 ;;
[15|30])
  ;;
*)
  AC_MSG_ERROR([bad value $enable_big_digits for --enable-big-digits; value should be 15 or 30]) ;;
esac
AC_MSG_RESULT($enable_big_digits)
AC_DEFINE_UNQUOTED(PYLONG_BITS_IN_DIGIT, $enable_big_digits, [Define as the preferred size in bits of long digits])
],
[AC_MSG_RESULT(no value specified)])

# check for wchar.h
AC_CHECK_HEADER(wchar.h, [
  AC_DEFINE(HAVE_WCHAR_H, 1,
  [Define if the compiler provides a wchar.h header file.])
  wchar_h="yes"
],
wchar_h="no"
)

# determine wchar_t size
if test "$wchar_h" = yes
then
  AC_CHECK_SIZEOF(wchar_t, 4, [#include <wchar.h>])
fi

# check whether wchar_t is signed or not
if test "$wchar_h" = yes
then
  # check whether wchar_t is signed or not
  AC_CACHE_CHECK([whether wchar_t is signed], [ac_cv_wchar_t_signed], [
  AC_RUN_IFELSE([AC_LANG_SOURCE([[
  #include <wchar.h>
  int main()
  {
	/* Success: exit code 0 */
        return ((((wchar_t) -1) < ((wchar_t) 0)) ? 0 : 1);
  }
  ]])],
  [ac_cv_wchar_t_signed=yes],
  [ac_cv_wchar_t_signed=no],
  [ac_cv_wchar_t_signed=yes])])
fi

AC_MSG_CHECKING(whether wchar_t is usable)
# wchar_t is only usable if it maps to an unsigned type
if test "$ac_cv_sizeof_wchar_t" -ge 2 \
          -a "$ac_cv_wchar_t_signed" = "no"
then
  AC_DEFINE(HAVE_USABLE_WCHAR_T, 1,
  [Define if you have a useable wchar_t type defined in wchar.h; useable
   means wchar_t must be an unsigned type with at least 16 bits. (see
   Include/unicodeobject.h).])
  AC_MSG_RESULT(yes)
else
  AC_MSG_RESULT(no)
fi

case $ac_sys_system/$ac_sys_release in
SunOS/*)
  if test -f /etc/os-release; then
    OS_NAME=$(awk -F= '/^NAME=/ {print substr($2,2,length($2)-2)}' /etc/os-release)
    if test "x$OS_NAME" = "xOracle Solaris"; then
      # bpo-43667: In Oracle Solaris, the internal form of wchar_t in
      # non-Unicode locales is not Unicode and hence cannot be used directly.
      # https://docs.oracle.com/cd/E37838_01/html/E61053/gmwke.html
      AC_DEFINE(HAVE_NON_UNICODE_WCHAR_T_REPRESENTATION, 1,
      [Define if the internal form of wchar_t in non-Unicode locales
       is not Unicode.])
    fi
  fi
  ;;
esac

# check for endianness
AC_C_BIGENDIAN

# ABI version string for Python extension modules.  This appears between the
# periods in shared library file names, e.g. foo.<SOABI>.so.  It is calculated
# from the following attributes which affect the ABI of this Python build (in
# this order):
#
# * The Python implementation (always 'cpython-' for us)
# * The major and minor version numbers
# * --with-pydebug (adds a 'd')
#
# Thus for example, Python 3.2 built with wide unicode, pydebug, and pymalloc,
# would get a shared library ABI version tag of 'cpython-32dmu' and shared
# libraries would be named 'foo.cpython-32dmu.so'.
#
# In Python 3.2 and older, --with-wide-unicode added a 'u' flag.
# In Python 3.7 and older, --with-pymalloc added a 'm' flag.
AC_SUBST(SOABI)
AC_MSG_CHECKING(ABIFLAGS)
AC_MSG_RESULT($ABIFLAGS)
AC_MSG_CHECKING(SOABI)
SOABI='cpython-'`echo $VERSION | tr -d .`${ABIFLAGS}${PLATFORM_TRIPLET:+-$PLATFORM_TRIPLET}
AC_MSG_RESULT($SOABI)

# Release and debug (Py_DEBUG) ABI are compatible, but not Py_TRACE_REFS ABI
if test "$Py_DEBUG" = 'true' -a "$with_trace_refs" != "yes"; then
  # Similar to SOABI but remove "d" flag from ABIFLAGS
  AC_SUBST(ALT_SOABI)
  ALT_SOABI='cpython-'`echo $VERSION | tr -d .``echo $ABIFLAGS | tr -d d`${PLATFORM_TRIPLET:+-$PLATFORM_TRIPLET}
  AC_DEFINE_UNQUOTED(ALT_SOABI, "${ALT_SOABI}",
            [Alternative SOABI used in debug build to load C extensions built in release mode])
fi

AC_SUBST(EXT_SUFFIX)
EXT_SUFFIX=.${SOABI}${SHLIB_SUFFIX}

AC_MSG_CHECKING(LDVERSION)
LDVERSION='$(VERSION)$(ABIFLAGS)'
AC_MSG_RESULT($LDVERSION)

# On Android and Cygwin the shared libraries must be linked with libpython.
AC_SUBST(LIBPYTHON)
if test -n "$ANDROID_API_LEVEL" -o "$MACHDEP" = "cygwin"; then
  LIBPYTHON="-lpython${VERSION}${ABIFLAGS}"
else
  LIBPYTHON=''
fi


AC_SUBST(BINLIBDEST)
BINLIBDEST='$(LIBDIR)/python$(VERSION)'


# Check for --with-platlibdir
# /usr/$LIDIRNAME/python$VERSION
AC_SUBST(PLATLIBDIR)
PLATLIBDIR="lib"
AC_MSG_CHECKING(for --with-platlibdir)
AC_ARG_WITH(platlibdir,
            AS_HELP_STRING([--with-platlibdir=DIRNAME],
            [Python library directory name (default is "lib")]),
[
# ignore 3 options:
#   --with-platlibdir
#   --with-platlibdir=
#   --without-platlibdir
if test -n "$withval" -a "$withval" != yes -a "$withval" != no
then
  AC_MSG_RESULT(yes)
  PLATLIBDIR="$withval"
  BINLIBDEST='${exec_prefix}/${PLATLIBDIR}/python$(VERSION)'
else
  AC_MSG_RESULT(no)
fi],
[AC_MSG_RESULT(no)])


dnl define LIBPL after ABIFLAGS and LDVERSION is defined.
AC_SUBST(PY_ENABLE_SHARED)
if test x$PLATFORM_TRIPLET = x; then
  LIBPL='$(prefix)'"/${PLATLIBDIR}/python${VERSION}/config-${LDVERSION}"
else
  LIBPL='$(prefix)'"/${PLATLIBDIR}/python${VERSION}/config-${LDVERSION}-${PLATFORM_TRIPLET}"
fi
AC_SUBST(LIBPL)

# Check for --with-wheel-pkg-dir=PATH
AC_SUBST(WHEEL_PKG_DIR)
WHEEL_PKG_DIR=""
AC_MSG_CHECKING(for --with-wheel-pkg-dir)
AC_ARG_WITH(wheel-pkg-dir,
            AS_HELP_STRING([--with-wheel-pkg-dir=PATH],
            [Directory of wheel packages used by ensurepip (default: none)]),
[
if test -n "$withval"; then
  AC_MSG_RESULT(yes)
  WHEEL_PKG_DIR="$withval"
else
  AC_MSG_RESULT(no)
fi],
[AC_MSG_RESULT(no)])

# Check whether right shifting a negative integer extends the sign bit
# or fills with zeros (like the Cray J90, according to Tim Peters).
AC_CACHE_CHECK([whether right shift extends the sign bit], [ac_cv_rshift_extends_sign], [
AC_RUN_IFELSE([AC_LANG_SOURCE([[
int main(void)
{
	return (((-1)>>3 == -1) ? 0 : 1);
}
]])],
[ac_cv_rshift_extends_sign=yes],
[ac_cv_rshift_extends_sign=no],
[ac_cv_rshift_extends_sign=yes])])
if test "$ac_cv_rshift_extends_sign" = no
then
  AC_DEFINE(SIGNED_RIGHT_SHIFT_ZERO_FILLS, 1,
  [Define if i>>j for signed int i does not extend the sign bit
   when i < 0])
fi

# check for getc_unlocked and related locking functions
AC_CACHE_CHECK([for getc_unlocked() and friends], [ac_cv_have_getc_unlocked], [
AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <stdio.h>]], [[
	FILE *f = fopen("/dev/null", "r");
	flockfile(f);
	getc_unlocked(f);
	funlockfile(f);
]])],[ac_cv_have_getc_unlocked=yes],[ac_cv_have_getc_unlocked=no])])
if test "$ac_cv_have_getc_unlocked" = yes
then
  AC_DEFINE(HAVE_GETC_UNLOCKED, 1,
  [Define this if you have flockfile(), getc_unlocked(), and funlockfile()])
fi

AC_ARG_WITH([readline],
  [AS_HELP_STRING([--with(out)-readline@<:@=editline@:>@],
    [use Editline for backend or disable readline module])],
    [],
    [with_readline=yes])

# check where readline lives
py_cv_lib_readline=no
# save the value of LIBS so we don't actually link Python with readline
LIBS_no_readline=$LIBS

if test "$with_readline" != no; then
  case "$with_readline" in
  editline|edit)
    LIBREADLINE=edit
    AC_DEFINE(WITH_EDITLINE, 1,
      [Define to build the readline module against Editline.])
    ;;
  yes|readline)
    LIBREADLINE=readline
    ;;
  *)
    AC_MSG_ERROR([proper usage is --with(out)-readline@<:@=editline@:>@])
    ;;
  esac

  # On some systems we need to link readline to a termcap compatible
  # library.  NOTE: Keep the precedence of listed libraries synchronised
  # with setup.py.
  AC_MSG_CHECKING([how to link readline libs])
  for py_libtermcap in "" tinfo ncursesw ncurses curses termcap; do
    if test -z "$py_libtermcap"; then
      READLINE_LIBS="-l$LIBREADLINE"
    else
      READLINE_LIBS="-l$LIBREADLINE -l$py_libtermcap"
    fi
    LIBS="$READLINE_LIBS $LIBS_no_readline"
    AC_LINK_IFELSE(
      [AC_LANG_CALL([],[readline])],
      [py_cv_lib_readline=yes])
    if test $py_cv_lib_readline = yes; then
      break
    fi
  done

  # Uncomment this line if you want to use READLINE_LIBS in Makefile or scripts
  #AC_SUBST([READLINE_LIBS])
  if test $py_cv_lib_readline = no; then
    AC_MSG_RESULT([none])
  else
    AC_MSG_RESULT([$READLINE_LIBS])
    AC_DEFINE(HAVE_LIBREADLINE, 1,
      [Define to build the readline module.])
  fi
fi

if test "$py_cv_lib_readline" = yes; then
  # check for readline 2.2
  AC_CHECK_DECL(rl_completion_append_character,
    AC_DEFINE(HAVE_RL_COMPLETION_APPEND_CHARACTER, 1,
      [Define if you have readline 2.2]),,
    [
#include <stdio.h> /* Must be first for Gnu Readline */
#ifdef WITH_EDITLINE
# include <editline/readline.h>
#else
# include <readline/readline.h>
#endif
    ])
  AC_CHECK_DECL(rl_completion_suppress_append,
    AC_DEFINE(HAVE_RL_COMPLETION_SUPPRESS_APPEND, 1,
      [Define if you have rl_completion_suppress_append]),,
    [
#include <stdio.h> /* Must be first for Gnu Readline */
#ifdef WITH_EDITLINE
# include <editline/readline.h>
#else
# include <readline/readline.h>
#endif
    ])

  # check for readline 4.0
  AC_CHECK_LIB($LIBREADLINE, rl_pre_input_hook,
    AC_DEFINE(HAVE_RL_PRE_INPUT_HOOK, 1,
      [Define if you have readline 4.0]),,$READLINE_LIBS)

  # also in 4.0
  AC_CHECK_LIB($LIBREADLINE, rl_completion_display_matches_hook,
    AC_DEFINE(HAVE_RL_COMPLETION_DISPLAY_MATCHES_HOOK, 1,
      [Define if you have readline 4.0]),,$READLINE_LIBS)

  # also in 4.0, but not in editline
  AC_CHECK_LIB($LIBREADLINE, rl_resize_terminal,
    AC_DEFINE(HAVE_RL_RESIZE_TERMINAL, 1,
      [Define if you have readline 4.0]),,$READLINE_LIBS)

  # check for readline 4.2
  AC_CHECK_LIB($LIBREADLINE, rl_completion_matches,
    AC_DEFINE(HAVE_RL_COMPLETION_MATCHES, 1,
      [Define if you have readline 4.2]),,$READLINE_LIBS)

  # also in readline 4.2
  AC_CHECK_DECL(rl_catch_signals,
    AC_DEFINE(HAVE_RL_CATCH_SIGNAL, 1,
      [Define if you can turn off readline's signal handling.]),,
    [
#include <stdio.h> /* Must be first for Gnu Readline */
#ifdef WITH_EDITLINE
# include <editline/readline.h>
#else
# include <readline/readline.h>
#endif
    ])

  AC_CHECK_LIB($LIBREADLINE, append_history,
    AC_DEFINE(HAVE_RL_APPEND_HISTORY, 1,
      [Define if readline supports append_history]),,$READLINE_LIBS)
fi

# End of readline checks: restore LIBS
LIBS=$LIBS_no_readline

AC_CACHE_CHECK([for broken nice()], [ac_cv_broken_nice], [
AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <stdlib.h>
#include <unistd.h>
int main(void)
{
	int val1 = nice(1);
	if (val1 != -1 && val1 == nice(2))
		exit(0);
	exit(1);
}
]])],
[ac_cv_broken_nice=yes],
[ac_cv_broken_nice=no],
[ac_cv_broken_nice=no])])
if test "$ac_cv_broken_nice" = yes
then
  AC_DEFINE(HAVE_BROKEN_NICE, 1,
  [Define if nice() returns success/failure instead of the new priority.])
fi

AC_CACHE_CHECK([for broken poll()], [ac_cv_broken_poll],
AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <poll.h>
#include <unistd.h>

int main(void)
{
    struct pollfd poll_struct = { 42, POLLIN|POLLPRI|POLLOUT, 0 };
    int poll_test;

    close (42);

    poll_test = poll(&poll_struct, 1, 0);
    if (poll_test < 0)
        return 0;
    else if (poll_test == 0 && poll_struct.revents != POLLNVAL)
        return 0;
    else
        return 1;
}
]])],
[ac_cv_broken_poll=yes],
[ac_cv_broken_poll=no],
[ac_cv_broken_poll=no]))
if test "$ac_cv_broken_poll" = yes
then
  AC_DEFINE(HAVE_BROKEN_POLL, 1,
      [Define if poll() sets errno on invalid file descriptors.])
fi

# check tzset(3) exists and works like we expect it to
AC_CACHE_CHECK([for working tzset()], [ac_cv_working_tzset], [
AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <stdlib.h>
#include <time.h>
#include <string.h>

#if HAVE_TZNAME
extern char *tzname[];
#endif

int main(void)
{
	/* Note that we need to ensure that not only does tzset(3)
	   do 'something' with localtime, but it works as documented
	   in the library reference and as expected by the test suite.
	   This includes making sure that tzname is set properly if
	   tm->tm_zone does not exist since it is the alternative way
	   of getting timezone info.

	   Red Hat 6.2 doesn't understand the southern hemisphere
	   after New Year's Day.
	*/

	time_t groundhogday = 1044144000; /* GMT-based */
	time_t midyear = groundhogday + (365 * 24 * 3600 / 2);

	putenv("TZ=UTC+0");
	tzset();
	if (localtime(&groundhogday)->tm_hour != 0)
	    exit(1);
#if HAVE_TZNAME
	/* For UTC, tzname[1] is sometimes "", sometimes "   " */
	if (strcmp(tzname[0], "UTC") ||
		(tzname[1][0] != 0 && tzname[1][0] != ' '))
	    exit(1);
#endif

	putenv("TZ=EST+5EDT,M4.1.0,M10.5.0");
	tzset();
	if (localtime(&groundhogday)->tm_hour != 19)
	    exit(1);
#if HAVE_TZNAME
	if (strcmp(tzname[0], "EST") || strcmp(tzname[1], "EDT"))
	    exit(1);
#endif

	putenv("TZ=AEST-10AEDT-11,M10.5.0,M3.5.0");
	tzset();
	if (localtime(&groundhogday)->tm_hour != 11)
	    exit(1);
#if HAVE_TZNAME
	if (strcmp(tzname[0], "AEST") || strcmp(tzname[1], "AEDT"))
	    exit(1);
#endif

#if HAVE_STRUCT_TM_TM_ZONE
	if (strcmp(localtime(&groundhogday)->tm_zone, "AEDT"))
	    exit(1);
	if (strcmp(localtime(&midyear)->tm_zone, "AEST"))
	    exit(1);
#endif

	exit(0);
}
]])],
[ac_cv_working_tzset=yes],
[ac_cv_working_tzset=no],
[ac_cv_working_tzset=no])])
if test "$ac_cv_working_tzset" = yes
then
  AC_DEFINE(HAVE_WORKING_TZSET, 1,
  [Define if tzset() actually switches the local timezone in a meaningful way.])
fi

# Look for subsecond timestamps in struct stat
AC_CACHE_CHECK([for tv_nsec in struct stat], [ac_cv_stat_tv_nsec],
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <sys/stat.h>]], [[
struct stat st;
st.st_mtim.tv_nsec = 1;
]])],
[ac_cv_stat_tv_nsec=yes],
[ac_cv_stat_tv_nsec=no]))
if test "$ac_cv_stat_tv_nsec" = yes
then
  AC_DEFINE(HAVE_STAT_TV_NSEC, 1,
  [Define if you have struct stat.st_mtim.tv_nsec])
fi

# Look for BSD style subsecond timestamps in struct stat
AC_CACHE_CHECK([for tv_nsec2 in struct stat], [ac_cv_stat_tv_nsec2],
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <sys/stat.h>]], [[
struct stat st;
st.st_mtimespec.tv_nsec = 1;
]])],
[ac_cv_stat_tv_nsec2=yes],
[ac_cv_stat_tv_nsec2=no]))
if test "$ac_cv_stat_tv_nsec2" = yes
then
  AC_DEFINE(HAVE_STAT_TV_NSEC2, 1,
  [Define if you have struct stat.st_mtimensec])
fi

# first curses header check
ac_save_cppflags="$CPPFLAGS"
if test "$cross_compiling" = no; then
  CPPFLAGS="$CPPFLAGS -I/usr/include/ncursesw"
fi

AC_CHECK_HEADERS(curses.h ncurses.h)

# On Solaris, term.h requires curses.h
AC_CHECK_HEADERS(term.h,,,[
#ifdef HAVE_CURSES_H
#include <curses.h>
#endif
])

# On HP/UX 11.0, mvwdelch is a block with a return statement
AC_CACHE_CHECK([whether mvwdelch is an expression], [ac_cv_mvwdelch_is_expression],
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <curses.h>]], [[
  int rtn;
  rtn = mvwdelch(0,0,0);
]])],
[ac_cv_mvwdelch_is_expression=yes],
[ac_cv_mvwdelch_is_expression=no]))

if test "$ac_cv_mvwdelch_is_expression" = yes
then
  AC_DEFINE(MVWDELCH_IS_EXPRESSION, 1,
  [Define if mvwdelch in curses.h is an expression.])
fi

# Issue #25720: ncurses has introduced the NCURSES_OPAQUE symbol making opaque
# structs since version 5.7.  If the macro is defined as zero before including
# [n]curses.h, ncurses will expose fields of the structs regardless of the
# configuration.
AC_CACHE_CHECK([whether WINDOW has _flags], [ac_cv_window_has_flags],
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
  #define NCURSES_OPAQUE 0
  #include <curses.h>
]], [[
  WINDOW *w;
  w->_flags = 0;
]])],
[ac_cv_window_has_flags=yes],
[ac_cv_window_has_flags=no]))


if test "$ac_cv_window_has_flags" = yes
then
  AC_DEFINE(WINDOW_HAS_FLAGS, 1,
  [Define if WINDOW in curses.h offers a field _flags.])
fi

dnl PY_CHECK_CURSES_FUNC(FUNCTION)
AC_DEFUN([PY_CHECK_CURSES_FUNC],
[ AS_VAR_PUSHDEF([py_var], [ac_cv_lib_curses_$1])
  AS_VAR_PUSHDEF([py_define], [HAVE_CURSES_]m4_toupper($1))
  AC_CACHE_CHECK(
    [for curses function $1],
    [py_var],
    [AC_COMPILE_IFELSE(
      [AC_LANG_PROGRAM(
        [#include <curses.h>], [
        #ifndef $1
        void *x=$1
        #endif
      ])],
      [AS_VAR_SET([py_var], [yes])],
      [AS_VAR_SET([py_var], [no])])]
  )
  AS_VAR_IF(
    [py_var],
    [yes],
    [AC_DEFINE([py_define], [1], [Define if you have the '$1' function.])])
  AS_VAR_POPDEF([py_var])
  AS_VAR_POPDEF([py_define])
])

PY_CHECK_CURSES_FUNC([is_pad])
PY_CHECK_CURSES_FUNC([is_term_resized])
PY_CHECK_CURSES_FUNC([resize_term])
PY_CHECK_CURSES_FUNC([resizeterm])
PY_CHECK_CURSES_FUNC([immedok])
PY_CHECK_CURSES_FUNC([syncok])
PY_CHECK_CURSES_FUNC([wchgat])
PY_CHECK_CURSES_FUNC([filter])
PY_CHECK_CURSES_FUNC([has_key])
PY_CHECK_CURSES_FUNC([typeahead])
PY_CHECK_CURSES_FUNC([use_env])
CPPFLAGS=$ac_save_cppflags

AC_MSG_NOTICE([checking for device files])

dnl NOTE: Inform user how to proceed with files when cross compiling.
if test "x$cross_compiling" = xyes; then
  if test "${ac_cv_file__dev_ptmx+set}" != set; then
    AC_MSG_CHECKING([for /dev/ptmx])
    AC_MSG_RESULT([not set])
    AC_MSG_ERROR([set ac_cv_file__dev_ptmx to yes/no in your CONFIG_SITE file when cross compiling])
  fi
  if test "${ac_cv_file__dev_ptc+set}" != set; then
    AC_MSG_CHECKING([for /dev/ptc])
    AC_MSG_RESULT([not set])
    AC_MSG_ERROR([set ac_cv_file__dev_ptc to yes/no in your CONFIG_SITE file when cross compiling])
  fi
fi

AC_CHECK_FILE(/dev/ptmx, [], [])
if test "x$ac_cv_file__dev_ptmx" = xyes; then
  AC_DEFINE(HAVE_DEV_PTMX, 1,
  [Define to 1 if you have the /dev/ptmx device file.])
fi
AC_CHECK_FILE(/dev/ptc, [], [])
if test "x$ac_cv_file__dev_ptc" = xyes; then
  AC_DEFINE(HAVE_DEV_PTC, 1,
  [Define to 1 if you have the /dev/ptc device file.])
fi

if test $ac_sys_system = Darwin
then
	LIBS="$LIBS -framework CoreFoundation"
fi

AC_CACHE_CHECK([for %zd printf() format support], ac_cv_have_size_t_format, [dnl
AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <stdio.h>
#include <stddef.h>
#include <string.h>

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_SSIZE_T
typedef ssize_t Py_ssize_t;
#elif SIZEOF_VOID_P == SIZEOF_LONG
typedef long Py_ssize_t;
#else
typedef int Py_ssize_t;
#endif

int main()
{
    char buffer[256];

    if(sprintf(buffer, "%zd", (size_t)123) < 0)
       	return 1;

    if (strcmp(buffer, "123"))
	return 1;

    if (sprintf(buffer, "%zd", (Py_ssize_t)-123) < 0)
       	return 1;

    if (strcmp(buffer, "-123"))
	return 1;

    return 0;
}
]])],
  [ac_cv_have_size_t_format=yes],
  [ac_cv_have_size_t_format=no],
  [ac_cv_have_size_t_format="cross -- assuming yes"
])])
if test "$ac_cv_have_size_t_format" != no ; then
  AC_DEFINE(PY_FORMAT_SIZE_T, "z",
  [Define to printf format modifier for Py_ssize_t])
fi

AC_CHECK_TYPE(socklen_t,,
  AC_DEFINE(socklen_t,int,
            [Define to `int' if <sys/socket.h> does not define.]),[
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
])

AC_CACHE_CHECK([for broken mbstowcs], [ac_cv_broken_mbstowcs],
AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
int main(void) {
    size_t len = -1;
    const char *str = "text";
    len = mbstowcs(NULL, str, 0);
    return (len != 4);
}
]])],
[ac_cv_broken_mbstowcs=no],
[ac_cv_broken_mbstowcs=yes],
[ac_cv_broken_mbstowcs=no]))
if test "$ac_cv_broken_mbstowcs" = yes
then
  AC_DEFINE(HAVE_BROKEN_MBSTOWCS, 1,
  [Define if mbstowcs(NULL, "text", 0) does not return the number of
   wide chars that would be converted.])
fi

# Check for --with-computed-gotos
AC_MSG_CHECKING(for --with-computed-gotos)
AC_ARG_WITH(computed-gotos,
            AS_HELP_STRING([--with-computed-gotos],
                           [enable computed gotos in evaluation loop (enabled by default on supported compilers)]),
[
if test "$withval" = yes
then
  AC_DEFINE(USE_COMPUTED_GOTOS, 1,
  [Define if you want to use computed gotos in ceval.c.])
  AC_MSG_RESULT(yes)
fi
if test "$withval" = no
then
  AC_DEFINE(USE_COMPUTED_GOTOS, 0,
  [Define if you want to use computed gotos in ceval.c.])
  AC_MSG_RESULT(no)
fi
],
[AC_MSG_RESULT(no value specified)])

AC_CACHE_CHECK([whether $CC supports computed gotos], [ac_cv_computed_gotos],
AC_RUN_IFELSE([AC_LANG_SOURCE([[[
int main(int argc, char **argv)
{
    static void *targets[1] = { &&LABEL1 };
    goto LABEL2;
LABEL1:
    return 0;
LABEL2:
    goto *targets[0];
    return 1;
}
]]])],
[ac_cv_computed_gotos=yes],
[ac_cv_computed_gotos=no],
[if test "${with_computed_gotos+set}" = set; then
   ac_cv_computed_gotos="$with_computed_gotos -- configured --with(out)-computed-gotos"
 else
   ac_cv_computed_gotos=no
 fi]))
case "$ac_cv_computed_gotos" in yes*)
  AC_DEFINE(HAVE_COMPUTED_GOTOS, 1,
  [Define if the C compiler supports computed gotos.])
esac

case $ac_sys_system in
AIX*)
  AC_DEFINE(HAVE_BROKEN_PIPE_BUF, 1, [Define if the system reports an invalid PIPE_BUF value.]) ;;
esac


AC_SUBST(THREADHEADERS)

for h in `(cd $srcdir;echo Python/thread_*.h)`
do
  THREADHEADERS="$THREADHEADERS \$(srcdir)/$h"
done

AC_SUBST(SRCDIRS)
SRCDIRS="\
  Modules \
  Modules/_blake2 \
  Modules/_ctypes \
  Modules/_decimal \
  Modules/_decimal/libmpdec \
  Modules/_io \
  Modules/_multiprocessing \
  Modules/_sha3 \
  Modules/_sqlite \
  Modules/_sre \
  Modules/_xxtestfuzz \
  Modules/cjkcodecs \
  Modules/expat \
  Objects \
  Parser \
  Programs \
  Python \
  Python/frozen_modules \
  Python/deepfreeze"
AC_MSG_CHECKING(for build directories)
for dir in $SRCDIRS; do
    if test ! -d $dir; then
        mkdir $dir
    fi
done
AC_MSG_RESULT(done)

# Availability of -O2:
AC_CACHE_CHECK([for -O2], [ac_cv_compile_o2], [
saved_cflags="$CFLAGS"
CFLAGS="-O2"
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([], [])], [ac_cv_compile_o2=yes], [ac_cv_compile_o2=no])
CFLAGS="$saved_cflags"
])

# _FORTIFY_SOURCE wrappers for memmove and bcopy are incorrect:
# http://sourceware.org/ml/libc-alpha/2010-12/msg00009.html
AC_MSG_CHECKING(for glibc _FORTIFY_SOURCE/memmove bug)
saved_cflags="$CFLAGS"
CFLAGS="-O2 -D_FORTIFY_SOURCE=2"
if test "$ac_cv_compile_o2" = no; then
    CFLAGS=""
fi
AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
void foo(void *p, void *q) { memmove(p, q, 19); }
int main(void) {
  char a[32] = "123456789000000000";
  foo(&a[9], a);
  if (strcmp(a, "123456789123456789000000000") != 0)
    return 1;
  foo(a, &a[9]);
  if (strcmp(a, "123456789000000000") != 0)
    return 1;
  return 0;
}
]])],
[have_glibc_memmove_bug=no],
[have_glibc_memmove_bug=yes],
[have_glibc_memmove_bug=undefined])
CFLAGS="$saved_cflags"
AC_MSG_RESULT($have_glibc_memmove_bug)
if test "$have_glibc_memmove_bug" = yes; then
    AC_DEFINE(HAVE_GLIBC_MEMMOVE_BUG, 1,
    [Define if glibc has incorrect _FORTIFY_SOURCE wrappers
     for memmove and bcopy.])
fi

if test "$ac_cv_gcc_asm_for_x87" = yes; then
    # Some versions of gcc miscompile inline asm:
    # http://gcc.gnu.org/bugzilla/show_bug.cgi?id=46491
    # http://gcc.gnu.org/ml/gcc/2010-11/msg00366.html
    case $CC in
        *gcc*)
            AC_MSG_CHECKING(for gcc ipa-pure-const bug)
            saved_cflags="$CFLAGS"
            CFLAGS="-O2"
            AC_RUN_IFELSE([AC_LANG_SOURCE([[
            __attribute__((noinline)) int
            foo(int *p) {
              int r;
              asm ( "movl \$6, (%1)\n\t"
                    "xorl %0, %0\n\t"
                    : "=r" (r) : "r" (p) : "memory"
              );
              return r;
            }
            int main(void) {
              int p = 8;
              if ((foo(&p) ? : p) != 6)
                return 1;
              return 0;
            }
            ]])],
            [have_ipa_pure_const_bug=no],
            [have_ipa_pure_const_bug=yes],
            [have_ipa_pure_const_bug=undefined])
            CFLAGS="$saved_cflags"
            AC_MSG_RESULT($have_ipa_pure_const_bug)
            if test "$have_ipa_pure_const_bug" = yes; then
                AC_DEFINE(HAVE_IPA_PURE_CONST_BUG, 1,
                          [Define if gcc has the ipa-pure-const bug.])
            fi
        ;;
    esac
fi

# Check for stdatomic.h
AC_CACHE_CHECK([for stdatomic.h], [ac_cv_header_stdatomic_h], [
AC_LINK_IFELSE(
[
  AC_LANG_SOURCE([[
    #include <stdatomic.h>
    atomic_int int_var;
    atomic_uintptr_t uintptr_var;
    int main(void) {
      atomic_store_explicit(&int_var, 5, memory_order_relaxed);
      atomic_store_explicit(&uintptr_var, 0, memory_order_relaxed);
      int loaded_value = atomic_load_explicit(&int_var, memory_order_seq_cst);
      return 0;
    }
  ]])
],[ac_cv_header_stdatomic_h=yes],[ac_cv_header_stdatomic_h=no])
])

AS_VAR_IF([ac_cv_header_stdatomic_h], [yes], [
    AC_DEFINE(HAVE_STD_ATOMIC, 1,
              [Has stdatomic.h with atomic_int and atomic_uintptr_t])
])

# Check for GCC >= 4.7 and clang __atomic builtin functions
AC_CACHE_CHECK([for builtin __atomic_load_n and __atomic_store_n functions], [ac_cv_builtin_atomic], [
AC_LINK_IFELSE(
[
  AC_LANG_SOURCE([[
    int val;
    int main(void) {
      __atomic_store_n(&val, 1, __ATOMIC_SEQ_CST);
      (void)__atomic_load_n(&val, __ATOMIC_SEQ_CST);
      return 0;
    }
  ]])
],[ac_cv_builtin_atomic=yes],[ac_cv_builtin_atomic=no])
])

AS_VAR_IF([ac_cv_builtin_atomic], [yes], [
    AC_DEFINE(HAVE_BUILTIN_ATOMIC, 1, [Has builtin __atomic_load_n() and __atomic_store_n() functions])
])

# ensurepip option
AC_MSG_CHECKING(for ensurepip)
AC_ARG_WITH(ensurepip,
    [AS_HELP_STRING([--with-ensurepip@<:@=install|upgrade|no@:>@],
        ["install" or "upgrade" using bundled pip (default is upgrade)])],
    [],
    [
      AS_CASE([$ac_sys_system],
        [Emscripten], [with_ensurepip=no],
        [WASI], [with_ensurepip=no],
        [with_ensurepip=upgrade]
      )
    ])
AS_CASE($with_ensurepip,
    [yes|upgrade],[ENSUREPIP=upgrade],
    [install],[ENSUREPIP=install],
    [no],[ENSUREPIP=no],
    [AC_MSG_ERROR([--with-ensurepip=upgrade|install|no])])
AC_MSG_RESULT($ENSUREPIP)
AC_SUBST(ENSUREPIP)

# check if the dirent structure of a d_type field and DT_UNKNOWN is defined
AC_CACHE_CHECK([if the dirent structure of a d_type field], [ac_cv_dirent_d_type], [
AC_LINK_IFELSE(
[
  AC_LANG_SOURCE([[
    #include <dirent.h>

    int main(void) {
      struct dirent entry;
      return entry.d_type == DT_UNKNOWN;
    }
  ]])
],[ac_cv_dirent_d_type=yes],[ac_cv_dirent_d_type=no])
])

AS_VAR_IF([ac_cv_dirent_d_type], [yes], [
    AC_DEFINE(HAVE_DIRENT_D_TYPE, 1,
              [Define to 1 if the dirent structure has a d_type field])
])

# check if the Linux getrandom() syscall is available
AC_CACHE_CHECK([for the Linux getrandom() syscall], [ac_cv_getrandom_syscall], [
AC_LINK_IFELSE(
[
  AC_LANG_SOURCE([[
    #include <stddef.h>
    #include <unistd.h>
    #include <sys/syscall.h>
    #include <linux/random.h>

    int main(void) {
        char buffer[1];
        const size_t buflen = sizeof(buffer);
        const int flags = GRND_NONBLOCK;
        /* ignore the result, Python checks for ENOSYS and EAGAIN at runtime */
        (void)syscall(SYS_getrandom, buffer, buflen, flags);
        return 0;
    }
  ]])
],[ac_cv_getrandom_syscall=yes],[ac_cv_getrandom_syscall=no])
])

AS_VAR_IF([ac_cv_getrandom_syscall], [yes], [
    AC_DEFINE(HAVE_GETRANDOM_SYSCALL, 1,
              [Define to 1 if the Linux getrandom() syscall is available])
])

# check if the getrandom() function is available
# the test was written for the Solaris function of <sys/random.h>
AC_CACHE_CHECK([for the getrandom() function], [ac_cv_func_getrandom], [
AC_LINK_IFELSE(
[
  AC_LANG_SOURCE([[
    #include <stddef.h>
    #include <sys/random.h>

    int main(void) {
        char buffer[1];
        const size_t buflen = sizeof(buffer);
        const int flags = 0;
        /* ignore the result, Python checks for ENOSYS at runtime */
        (void)getrandom(buffer, buflen, flags);
        return 0;
    }
  ]])
],[ac_cv_func_getrandom=yes],[ac_cv_func_getrandom=no])
])

AS_VAR_IF([ac_cv_func_getrandom], [yes], [
    AC_DEFINE(HAVE_GETRANDOM, 1,
              [Define to 1 if the getrandom() function is available])
])

# checks for POSIX shared memory, used by Modules/_multiprocessing/posixshmem.c
# shm_* may only be available if linking against librt
POSIXSHMEM_CFLAGS='-I$(srcdir)/Modules/_multiprocessing'
WITH_SAVE_ENV([
  AC_SEARCH_LIBS([shm_open], [rt])
  AS_VAR_IF([ac_cv_search_shm_open], [-lrt], [POSIXSHMEM_LIBS="-lrt"])

  dnl Temporarily override ac_includes_default for AC_CHECK_FUNCS below.
  _SAVE_VAR([ac_includes_default])
  ac_includes_default="\
  ${ac_includes_default}
  #ifndef __cplusplus
  #  ifdef HAVE_SYS_MMAN_H
  #    include <sys/mman.h>
  #  endif
  #endif
  "
  AC_CHECK_FUNCS([shm_open shm_unlink], [have_posix_shmem=yes], [have_posix_shmem=no])
  _RESTORE_VAR([ac_includes_default])
])

# Check for usable OpenSSL
AX_CHECK_OPENSSL([have_openssl=yes],[have_openssl=no])

# rpath to libssl and libcrypto
AS_VAR_IF([GNULD], [yes], [
  rpath_arg="-Wl,--enable-new-dtags,-rpath="
], [
  rpath_arg="-Wl,-rpath="
])

AC_MSG_CHECKING(for --with-openssl-rpath)
AC_ARG_WITH(openssl-rpath,
    AS_HELP_STRING([--with-openssl-rpath=@<:@DIR|auto|no@:>@],
                   [Set runtime library directory (rpath) for OpenSSL libraries,
                    no (default): don't set rpath,
                    auto: auto-detect rpath from --with-openssl and pkg-config,
                    DIR: set an explicit rpath
                   ]),
    [],
    [with_openssl_rpath=no]
)
AS_CASE($with_openssl_rpath,
    [auto|yes], [
      OPENSSL_RPATH=auto
      dnl look for linker directories
      for arg in "$OPENSSL_LDFLAGS"; do
        AS_CASE([$arg],
          [-L*], [OPENSSL_LDFLAGS_RPATH="$OPENSSL_LDFLAGS_RPATH ${rpath_arg}$(echo $arg | cut -c3-)"]
        )
      done
    ],
    [no], [OPENSSL_RPATH=],
    [AS_IF(
        [test -d "$with_openssl_rpath"],
        [
          OPENSSL_RPATH="$with_openssl_rpath"
          OPENSSL_LDFLAGS_RPATH="${rpath_arg}$with_openssl_rpath"
        ],
        AC_MSG_ERROR([--with-openssl-rpath "$with_openssl_rpath" is not a directory]))
    ]
)
AC_MSG_RESULT($OPENSSL_RPATH)
AC_SUBST([OPENSSL_RPATH])

# This static linking is NOT OFFICIALLY SUPPORTED and not advertised.
# Requires static OpenSSL build with position-independent code. Some features
# like DSO engines or external OSSL providers don't work. Only tested with GCC
# and clang on X86_64.
AS_VAR_IF([PY_UNSUPPORTED_OPENSSL_BUILD], [static], [
  AC_MSG_CHECKING([for unsupported static openssl build])
  new_OPENSSL_LIBS=
  for arg in $OPENSSL_LIBS; do
    AS_CASE([$arg],
      [-l*], [
        libname=$(echo $arg | cut -c3-)
        new_OPENSSL_LIBS="$new_OPENSSL_LIBS -l:lib${libname}.a -Wl,--exclude-libs,lib${libname}.a"
      ],
      [new_OPENSSL_LIBS="$new_OPENSSL_LIBS $arg"]
    )
  done
  dnl include libz for OpenSSL build flavors with compression support
  OPENSSL_LIBS="$new_OPENSSL_LIBS $ZLIB_LIBS"
  AC_MSG_RESULT([$OPENSSL_LIBS])
])

dnl AX_CHECK_OPENSSL does not export libcrypto-only libs
LIBCRYPTO_LIBS=
for arg in $OPENSSL_LIBS; do
  AS_CASE([$arg],
    [-l*ssl*|-Wl*ssl*], [],
    [LIBCRYPTO_LIBS="$LIBCRYPTO_LIBS $arg"]
  )
done

# check if OpenSSL libraries work as expected
WITH_SAVE_ENV([
  LIBS="$LIBS $OPENSSL_LIBS"
  CFLAGS="$CFLAGS $OPENSSL_INCLUDES"
  LDFLAGS="$LDFLAGS $OPENSSL_LDFLAGS $OPENSSL_LDFLAGS_RPATH"

  AC_CACHE_CHECK([whether OpenSSL provides required ssl module APIs], [ac_cv_working_openssl_ssl], [
    AC_LINK_IFELSE([AC_LANG_PROGRAM([
      #include <openssl/opensslv.h>
      #include <openssl/ssl.h>
      #if OPENSSL_VERSION_NUMBER < 0x10101000L
        #error "OpenSSL >= 1.1.1 is required"
      #endif
      static void keylog_cb(const SSL *ssl, const char *line) {}
    ], [
      SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());
      SSL_CTX_set_keylog_callback(ctx, keylog_cb);
      SSL *ssl = SSL_new(ctx);
      X509_VERIFY_PARAM *param = SSL_get0_param(ssl);
      X509_VERIFY_PARAM_set1_host(param, "python.org", 0);
      SSL_free(ssl);
      SSL_CTX_free(ctx);
    ])], [ac_cv_working_openssl_ssl=yes], [ac_cv_working_openssl_ssl=no])
  ])
])

WITH_SAVE_ENV([
  LIBS="$LIBS $LIBCRYPTO_LIBS"
  CFLAGS="$CFLAGS $OPENSSL_INCLUDES"
  LDFLAGS="$LDFLAGS $OPENSSL_LDFLAGS $OPENSSL_LDFLAGS_RPATH"

  AC_CACHE_CHECK([whether OpenSSL provides required hashlib module APIs], [ac_cv_working_openssl_hashlib], [
    AC_LINK_IFELSE([AC_LANG_PROGRAM([
      #include <openssl/opensslv.h>
      #include <openssl/evp.h>
      #if OPENSSL_VERSION_NUMBER < 0x10101000L
        #error "OpenSSL >= 1.1.1 is required"
      #endif
    ], [
      OBJ_nid2sn(NID_md5);
      OBJ_nid2sn(NID_sha1);
      OBJ_nid2sn(NID_sha3_512);
      OBJ_nid2sn(NID_blake2b512);
      EVP_PBE_scrypt(NULL, 0, NULL, 0, 2, 8, 1, 0, NULL, 0);
    ])], [ac_cv_working_openssl_hashlib=yes], [ac_cv_working_openssl_hashlib=no])
  ])
])

# ssl module default cipher suite string
AH_TEMPLATE(PY_SSL_DEFAULT_CIPHERS,
  [Default cipher suites list for ssl module.
   1: Python's preferred selection, 2: leave OpenSSL defaults untouched, 0: custom string])
AH_TEMPLATE(PY_SSL_DEFAULT_CIPHER_STRING,
  [Cipher suite string for PY_SSL_DEFAULT_CIPHERS=0]
)

AC_MSG_CHECKING(for --with-ssl-default-suites)
AC_ARG_WITH(ssl-default-suites,
            AS_HELP_STRING([--with-ssl-default-suites=@<:@python|openssl|STRING@:>@],
                           [override default cipher suites string,
                            python: use Python's preferred selection (default),
                            openssl: leave OpenSSL's defaults untouched,
                            STRING: use a custom string,
                            python and STRING also set TLS 1.2 as minimum TLS version]),
[
AC_MSG_RESULT($withval)
case "$withval" in
    python)
        AC_DEFINE(PY_SSL_DEFAULT_CIPHERS, 1)
        ;;
    openssl)
        AC_DEFINE(PY_SSL_DEFAULT_CIPHERS, 2)
        ;;
    *)
        AC_DEFINE(PY_SSL_DEFAULT_CIPHERS, 0)
        AC_DEFINE_UNQUOTED(PY_SSL_DEFAULT_CIPHER_STRING, "$withval")
        ;;
esac
],
[
AC_MSG_RESULT(python)
AC_DEFINE(PY_SSL_DEFAULT_CIPHERS, 1)
])

# builtin hash modules
default_hashlib_hashes="md5,sha1,sha256,sha512,sha3,blake2"
AC_DEFINE([PY_BUILTIN_HASHLIB_HASHES], [], [enabled builtin hash modules]
)
AC_MSG_CHECKING(for --with-builtin-hashlib-hashes)
AC_ARG_WITH(builtin-hashlib-hashes,
            AS_HELP_STRING([--with-builtin-hashlib-hashes=md5,sha1,sha256,sha512,sha3,blake2],
                           [builtin hash modules,
                            md5, sha1, sha256, sha512, sha3 (with shake), blake2]),
[
  AS_CASE([$with_builtin_hashlib_hashes],
    [yes], [with_builtin_hashlib_hashes=$default_hashlib_hashes],
    [no], [with_builtin_hashlib_hashes=""]
  )
], [with_builtin_hashlib_hashes=$default_hashlib_hashes])

AC_MSG_RESULT($with_builtin_hashlib_hashes)
AC_DEFINE_UNQUOTED(PY_BUILTIN_HASHLIB_HASHES, "$with_builtin_hashlib_hashes")

as_save_IFS=$IFS
IFS=,
for builtin_hash in $with_builtin_hashlib_hashes; do
    AS_CASE($builtin_hash,
      [md5], [with_builtin_md5=yes],
      [sha1], [with_builtin_sha1=yes],
      [sha256], [with_builtin_sha256=yes],
      [sha512], [with_builtin_sha512=yes],
      [sha3], [with_builtin_sha3=yes],
      [blake2], [with_builtin_blake2=yes]
    )
done
IFS=$as_save_IFS

dnl libb2 for blake2. _blake2 module falls back to vendored copy.
AS_VAR_IF([with_builtin_blake2], [yes], [
  PKG_CHECK_MODULES([LIBB2], [libb2], [
    have_libb2=yes
    AC_DEFINE([HAVE_LIBB2], [1],
              [Define to 1 if you want to build _blake2 module with libb2])
  ], [have_libb2=no])
])

# Check whether to disable test modules. Once set, setup.py will not build
# test extension modules and "make install" will not install test suites.
AC_MSG_CHECKING([for --disable-test-modules])
AC_ARG_ENABLE([test-modules],
  [AS_HELP_STRING([--disable-test-modules], [don't build nor install test modules])], [
  AS_VAR_IF([enable_test_modules], [yes], [TEST_MODULES=yes], [TEST_MODULES=no])
], [
  AS_CASE([$ac_sys_system/$ac_sys_emscripten_target],
    [Emscripten/browser*], [TEST_MODULES=no],
    [TEST_MODULES=yes]
  )
])
AC_MSG_RESULT([$TEST_MODULES])
AC_SUBST([TEST_MODULES])

AC_DEFUN([PY_STDLIB_MOD_SET_NA], [
  m4_foreach([mod], [$@], [
    AS_VAR_SET([py_cv_module_]mod, [n/a])])
])

# stdlib not available
dnl Modules that are not available on some platforms
dnl AIX has shadow passwords, but access is not via getspent()
dnl VxWorks does not provide crypt() function
AS_CASE([$ac_sys_system],
  [AIX], [PY_STDLIB_MOD_SET_NA([_scproxy], [spwd])],
  [VxWorks*], [PY_STDLIB_MOD_SET_NA([_scproxy], [_crypt], [termios], [grp])],
  [Darwin], [PY_STDLIB_MOD_SET_NA([ossaudiodev], [spwd])],
  [CYGWIN*], [PY_STDLIB_MOD_SET_NA([_scproxy], [nis])],
  [QNX*], [PY_STDLIB_MOD_SET_NA([_scproxy], [nis])],
  [FreeBSD*], [PY_STDLIB_MOD_SET_NA([_scproxy], [spwd])],
  [Emscripten|WASI], [
    dnl subprocess and multiprocessing are not supported (no fork syscall).
    dnl curses and tkinter user interface are not available.
    dnl dbm and gdbm aren't available, too.
    dnl Emscripten and WASI provide only stubs for pwd, grp APIs.
    dnl resource functions (get/setrusage) are stubs, too.
    PY_STDLIB_MOD_SET_NA(
      [_curses],
      [_curses_panel],
      [_dbm],
      [_gdbm],
      [_multiprocessing],
      [_posixshmem],
      [_posixsubprocess],
      [_scproxy],
      [_tkinter],
      [_xxsubinterpreters],
      [grp],
      [nis],
      [ossaudiodev],
      [pwd],
      [resource],
      [spwd],
      [syslog],
    )
    AS_CASE([$ac_sys_system/$ac_sys_emscripten_target],
      [Emscripten/browser*], [
        dnl These modules are not particularly useful in browsers.
        PY_STDLIB_MOD_SET_NA(
          [fcntl],
          [readline],
          [termios],
        )
      ],
      [Emscripten/node*], [],
      [WASI/*], [
        dnl WASI SDK 15.0 does not support file locking, mmap, and more.
        PY_STDLIB_MOD_SET_NA(
          [_ctypes_test],
          [fcntl],
          [mmap],
          [termios],
        )
      ]
    )
  ],
  [PY_STDLIB_MOD_SET_NA([_scproxy])]
)

dnl AC_MSG_NOTICE([m4_set_list([_PY_STDLIB_MOD_SET_NA])])

dnl Default value for Modules/Setup.stdlib build type
AS_CASE([$host_cpu],
  [wasm32|wasm64], [MODULE_BUILDTYPE=static],
  [MODULE_BUILDTYPE=${MODULE_BUILDTYPE:-shared}]
)
AC_SUBST([MODULE_BUILDTYPE])

dnl Use Modules/Setup.stdlib as additional provider?
AC_MSG_CHECKING([for additional Modules/Setup files])
AS_CASE([$ac_sys_system],
    [Emscripten], [MODULES_SETUP_STDLIB=Modules/Setup.stdlib],
    [WASI], [MODULES_SETUP_STDLIB=Modules/Setup.stdlib],
    [MODULES_SETUP_STDLIB=]
)
AC_MSG_RESULT([$MODULES_SETUP_STDLIB])
AC_SUBST([MODULES_SETUP_STDLIB])


dnl _MODULE_BLOCK_ADD([VAR], [VALUE])
dnl internal: adds $1=quote($2) to MODULE_BLOCK
AC_DEFUN([_MODULE_BLOCK_ADD], [AS_VAR_APPEND([MODULE_BLOCK], ["$1=_AS_QUOTE([$2])$as_nl"])])
MODULE_BLOCK=

dnl Check for stdlib extension modules
dnl PY_STDLIB_MOD([NAME], [ENABLED-TEST], [SUPPORTED-TEST], [CFLAGS], [LDFLAGS])
dnl sets MODULE_$NAME_STATE based on PY_STDLIB_MOD_SET_NA(), ENABLED-TEST,
dnl and SUPPORTED_TEST. ENABLED-TEST and SUPPORTED-TEST default to true if
dnl empty.
dnl    n/a: marked unavailable on platform by PY_STDLIB_MOD_SET_NA()
dnl    yes: enabled and supported
dnl    missing: enabled and not supported
dnl    disabled: not enabled
dnl sets MODULE_$NAME_CFLAGS and MODULE_$NAME_LDFLAGS
AC_DEFUN([PY_STDLIB_MOD], [
  AC_MSG_CHECKING([for stdlib extension module $1])
  m4_pushdef([modcond], [MODULE_]m4_toupper([$1]))dnl
  m4_pushdef([modstate], [py_cv_module_$1])dnl
  dnl Check if module has been disabled by PY_STDLIB_MOD_SET_NA()
  AS_IF([test "$modstate" != "n/a"], [
    AS_IF(m4_ifblank([$2], [true], [$2]),
       [AS_IF([m4_ifblank([$3], [true], [$3])], [modstate=yes], [modstate=missing])],
       [modstate=disabled])
  ])
  _MODULE_BLOCK_ADD(modcond[_STATE], [$modstate])
  AS_VAR_IF([modstate], [yes], [
    m4_ifblank([$4], [], [_MODULE_BLOCK_ADD([MODULE_]m4_toupper([$1])[_CFLAGS], [$4])])
    m4_ifblank([$5], [], [_MODULE_BLOCK_ADD([MODULE_]m4_toupper([$1])[_LDFLAGS], [$5])])
  ])
  AM_CONDITIONAL(modcond, [test "$modstate" = yes])
  AC_MSG_RESULT([$modstate])
  m4_popdef([modcond])dnl
  m4_popdef([modstate])dnl
])

dnl Define simple stdlib extension module
dnl Always enable unless the module is disabled by PY_STDLIB_MOD_SET_NA
dnl PY_STDLIB_MOD_SIMPLE([NAME], [CFLAGS], [LDFLAGS])
dnl cflags and ldflags are optional
AC_DEFUN([PY_STDLIB_MOD_SIMPLE], [
  m4_pushdef([modcond], [MODULE_]m4_toupper([$1]))dnl
  m4_pushdef([modstate], [py_cv_module_$1])dnl
  dnl Check if module has been disabled by PY_STDLIB_MOD_SET_NA()
  AS_IF([test "$modstate" != "n/a"], [modstate=yes])
  AM_CONDITIONAL(modcond, [test "$modstate" = yes])
  _MODULE_BLOCK_ADD(modcond[_STATE], [$modstate])
  AS_VAR_IF([modstate], [yes], [
    m4_ifblank([$2], [], [_MODULE_BLOCK_ADD([MODULE_]m4_toupper([$1])[_CFLAGS], [$2])])
    m4_ifblank([$3], [], [_MODULE_BLOCK_ADD([MODULE_]m4_toupper([$1])[_LDFLAGS], [$3])])
  ])
  m4_popdef([modcond])dnl
  m4_popdef([modstate])dnl
])

dnl static modules in Modules/Setup.bootstrap
PY_STDLIB_MOD_SIMPLE([_io], [-I\$(srcdir)/Modules/_io], [])
PY_STDLIB_MOD_SIMPLE([time], [], [$TIMEMODULE_LIB])

dnl always enabled extension modules
PY_STDLIB_MOD_SIMPLE([array])
PY_STDLIB_MOD_SIMPLE([_asyncio])
PY_STDLIB_MOD_SIMPLE([_bisect])
PY_STDLIB_MOD_SIMPLE([_contextvars])
PY_STDLIB_MOD_SIMPLE([_csv])
PY_STDLIB_MOD_SIMPLE([_heapq])
PY_STDLIB_MOD_SIMPLE([_json])
PY_STDLIB_MOD_SIMPLE([_lsprof])
PY_STDLIB_MOD_SIMPLE([_opcode])
PY_STDLIB_MOD_SIMPLE([_pickle])
PY_STDLIB_MOD_SIMPLE([_posixsubprocess])
PY_STDLIB_MOD_SIMPLE([_queue])
PY_STDLIB_MOD_SIMPLE([_random])
PY_STDLIB_MOD_SIMPLE([select])
PY_STDLIB_MOD_SIMPLE([_struct])
PY_STDLIB_MOD_SIMPLE([_typing])
PY_STDLIB_MOD_SIMPLE([_xxsubinterpreters])
PY_STDLIB_MOD_SIMPLE([_zoneinfo])

dnl multiprocessing modules
PY_STDLIB_MOD([_multiprocessing],
  [], [test "$ac_cv_func_sem_unlink" = "yes"],
  [-I\$(srcdir)/Modules/_multiprocessing])
PY_STDLIB_MOD([_posixshmem],
  [], [test "$have_posix_shmem" = "yes"],
  [$POSIXSHMEM_CFLAGS], [$POSIXSHMEM_LIBS])

dnl needs libm
PY_STDLIB_MOD_SIMPLE([audioop], [], [$LIBM])
PY_STDLIB_MOD_SIMPLE([_statistics], [], [$LIBM])
PY_STDLIB_MOD_SIMPLE([cmath], [], [$LIBM])
PY_STDLIB_MOD_SIMPLE([math], [], [$LIBM])

dnl needs libm and on some platforms librt
PY_STDLIB_MOD_SIMPLE([_datetime], [], [$TIMEMODULE_LIB $LIBM])

dnl modules with some unix dependencies
PY_STDLIB_MOD([fcntl],
  [], [test "$ac_cv_header_sys_ioctl_h" = "yes" -a "$ac_cv_header_fcntl_h" = "yes"],
  [], [$FCNTL_LIBS])
PY_STDLIB_MOD([mmap],
  [], [test "$ac_cv_header_sys_mman_h" = "yes" -a "$ac_cv_header_sys_stat_h" = "yes"])
PY_STDLIB_MOD([_socket],
  [], m4_flatten([test "$ac_cv_header_sys_socket_h" = "yes"
                    -a "$ac_cv_header_sys_types_h" = "yes"
                    -a "$ac_cv_header_netinet_in_h" = "yes"]))

dnl platform specific extensions
PY_STDLIB_MOD([grp], [], [test "$ac_cv_func_getgrgid" = yes -o "$ac_cv_func_getgrgid_r" = yes])
PY_STDLIB_MOD([ossaudiodev],
  [], [test "$ac_cv_header_linux_soundcard_h" = yes -o "$ac_cv_header_sys_soundcard_h" = yes],
  [], [$OSSAUDIODEV_LIBS])
PY_STDLIB_MOD([pwd], [], [test "$ac_cv_func_getpwuid" = yes -o "$ac_cv_func_getpwuid_r" = yes])
PY_STDLIB_MOD([resource], [], [test "$ac_cv_header_sys_resource_h" = yes])
PY_STDLIB_MOD([_scproxy],
  [test "$ac_sys_system" = "Darwin"], [],
  [], [-framework SystemConfiguration -framework CoreFoundation])
PY_STDLIB_MOD([spwd], [], [test "$ac_cv_func_getspent" = yes -o "$ac_cv_func_getspnam" = yes])
PY_STDLIB_MOD([syslog], [], [test "$ac_cv_header_syslog_h" = yes])
PY_STDLIB_MOD([termios], [], [test "$ac_cv_header_termios_h" = yes])

dnl _elementtree loads libexpat via CAPI hook in pyexpat
PY_STDLIB_MOD([pyexpat], [], [], [$LIBEXPAT_CFLAGS], [$LIBEXPAT_LDFLAGS])
PY_STDLIB_MOD([_elementtree], [], [], [$LIBEXPAT_CFLAGS], [])
PY_STDLIB_MOD_SIMPLE([_codecs_cn])
PY_STDLIB_MOD_SIMPLE([_codecs_hk])
PY_STDLIB_MOD_SIMPLE([_codecs_iso2022])
PY_STDLIB_MOD_SIMPLE([_codecs_jp])
PY_STDLIB_MOD_SIMPLE([_codecs_kr])
PY_STDLIB_MOD_SIMPLE([_codecs_tw])
PY_STDLIB_MOD_SIMPLE([_multibytecodec])
PY_STDLIB_MOD_SIMPLE([unicodedata])

dnl By default we always compile these even when OpenSSL is available
dnl (issue #14693). The modules are small.
PY_STDLIB_MOD([_md5], [test "$with_builtin_md5" = yes])
PY_STDLIB_MOD([_sha1], [test "$with_builtin_sha1" = yes])
PY_STDLIB_MOD([_sha256], [test "$with_builtin_sha256" = yes])
PY_STDLIB_MOD([_sha512], [test "$with_builtin_sha512" = yes])
PY_STDLIB_MOD([_sha3], [test "$with_builtin_sha3" = yes])
PY_STDLIB_MOD([_blake2],
  [test "$with_builtin_blake2" = yes], [],
  [$LIBB2_CFLAGS], [$LIBB2_LIBS])

PY_STDLIB_MOD([_crypt],
  [], [test "$ac_cv_crypt_crypt" = yes],
  [$LIBCRYPT_CFLAGS], [$LIBCRYPT_LIBS])
dnl PY_STDLIB_MOD([_ctypes], [], [], [], [])
dnl PY_STDLIB_MOD([_curses], [], [], [], [])
dnl PY_STDLIB_MOD([_curses_panel], [], [], [], [])
PY_STDLIB_MOD([_decimal], [], [], [$LIBMPDEC_CFLAGS], [$LIBMPDEC_LDFLAGS])
dnl PY_STDLIB_MOD([_dbm], [], [], [], [])
PY_STDLIB_MOD([_gdbm],
  [test "$have_gdbm_dbmliborder" = yes], [test "$have_gdbm" = yes],
  [$GDBM_CFLAGS], [$GDBM_LIBS])
PY_STDLIB_MOD([nis],
  [], [test "$have_nis" = yes -a "$ac_cv_header_rpc_rpc_h" = yes],
  [$LIBNSL_CFLAGS], [$LIBNSL_LIBS])
dnl PY_STDLIB_MOD([readline], [], [], [], [])
PY_STDLIB_MOD([_sqlite3],
  [test "$have_sqlite3" = "yes"],
  [test "$have_supported_sqlite3" = "yes"],
  [$LIBSQLITE3_CFLAGS], [$LIBSQLITE3_LIBS])
PY_STDLIB_MOD([_tkinter],
  [], [test "$have_tcltk" = "yes"],
  [$TCLTK_CFLAGS], [$TCLTK_LIBS])
PY_STDLIB_MOD([_uuid],
  [], [test "$have_uuid" = "yes"],
  [$LIBUUID_CFLAGS], [$LIBUUID_LIBS])

dnl compression libs
PY_STDLIB_MOD([zlib], [], [test "$have_zlib" = yes],
  [$ZLIB_CFLAGS], [$ZLIB_LIBS])
dnl binascii can use zlib for optimized crc32.
PY_STDLIB_MOD_SIMPLE([binascii], [$BINASCII_CFLAGS], [$BINASCII_LIBS])
PY_STDLIB_MOD([_bz2], [], [test "$have_bzip2" = yes],
  [$BZIP2_CFLAGS], [$BZIP2_LIBS])
PY_STDLIB_MOD([_lzma], [], [test "$have_liblzma" = yes],
  [$LIBLZMA_CFLAGS], [$LIBLZMA_LIBS])

dnl OpenSSL bindings
PY_STDLIB_MOD([_ssl], [], [test "$ac_cv_working_openssl_ssl" = yes],
  [$OPENSSL_INCLUDES], [$OPENSSL_LDFLAGS $OPENSSL_LDFLAGS_RPATH $OPENSSL_LIBS])
PY_STDLIB_MOD([_hashlib], [], [test "$ac_cv_working_openssl_hashlib" = yes],
  [$OPENSSL_INCLUDES], [$OPENSSL_LDFLAGS $OPENSSL_LDFLAGS_RPATH $LIBCRYPTO_LIBS])

dnl test modules
PY_STDLIB_MOD([_testcapi], [test "$TEST_MODULES" = yes])
PY_STDLIB_MOD([_testclinic], [test "$TEST_MODULES" = yes])
PY_STDLIB_MOD([_testinternalcapi], [test "$TEST_MODULES" = yes])
PY_STDLIB_MOD([_testbuffer], [test "$TEST_MODULES" = yes])
PY_STDLIB_MOD([_testimportmultiple], [test "$TEST_MODULES" = yes], [test "$ac_cv_func_dlopen" = yes])
PY_STDLIB_MOD([_testmultiphase], [test "$TEST_MODULES" = yes], [test "$ac_cv_func_dlopen" = yes])
PY_STDLIB_MOD([_xxtestfuzz], [test "$TEST_MODULES" = yes])
PY_STDLIB_MOD([_ctypes_test], [test "$TEST_MODULES" = yes], [test "$ac_cv_func_dlopen" = yes], [], [-lm])

dnl Limited API template modules.
dnl The limited C API is not compatible with the Py_TRACE_REFS macro.
dnl Emscripten does not support shared libraries yet.
PY_STDLIB_MOD([xxlimited], [test "$with_trace_refs" = "no"], [test "$ac_cv_func_dlopen" = yes])
PY_STDLIB_MOD([xxlimited_35], [test "$with_trace_refs" = "no"], [test "$ac_cv_func_dlopen" = yes])

# substitute multiline block, must come after last PY_STDLIB_MOD()
AC_SUBST([MODULE_BLOCK])

# generate output files
AC_CONFIG_FILES(Makefile.pre Misc/python.pc Misc/python-embed.pc Misc/python-config.sh)
AC_CONFIG_FILES([Modules/Setup.bootstrap Modules/Setup.stdlib])
AC_CONFIG_FILES([Modules/ld_so_aix], [chmod +x Modules/ld_so_aix])
AC_OUTPUT

AC_MSG_NOTICE([creating Modules/Setup.local])
if test ! -f Modules/Setup.local
then
	echo "# Edit this file for local setup changes" >Modules/Setup.local
fi

AC_MSG_NOTICE([creating Makefile])
$SHELL $srcdir/Modules/makesetup -c $srcdir/Modules/config.c.in \
			-s Modules \
			Modules/Setup.local $MODULES_SETUP_STDLIB Modules/Setup.bootstrap $srcdir/Modules/Setup
mv config.c Modules

if test -z "$PKG_CONFIG"; then
  AC_MSG_WARN([pkg-config is missing. Some dependencies may not be detected correctly.])
fi

if test "$Py_OPT" = 'false' -a "$Py_DEBUG" != 'true'; then
  AC_MSG_NOTICE([

If you want a release build with all stable optimizations active (PGO, etc),
please run ./configure --enable-optimizations
])
fi

AS_VAR_IF([PY_SUPPORT_TIER], [0], [AC_MSG_WARN([

Platform "$host" with compiler "$ac_cv_cc_name" is not supported by the
CPython core team, see https://peps.python.org/pep-0011/ for more information.
])])
