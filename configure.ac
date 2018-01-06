dnl ***********************************************
dnl * Please run autoreconf to test your changes! *
dnl ***********************************************

# Set VERSION so we only need to edit in one place (i.e., here)
m4_define(PYTHON_VERSION, 3.7)

AC_PREREQ(2.65)

AC_INIT(python, PYTHON_VERSION, https://bugs.python.org/)

AC_SUBST(BASECPPFLAGS)
if test "$srcdir" != . -a "$srcdir" != "$(pwd)"; then
    # If we're building out-of-tree, we need to make sure the following
    # resources get picked up before their $srcdir counterparts.
    #   Objects/ -> typeslots.inc
    #   Include/ -> Python-ast.h, graminit.h
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
    GITVERSION="git -C \$(srcdir) rev-parse --short HEAD"
    GITTAG="git -C \$(srcdir) describe --all --always --dirty"
    GITBRANCH="git -C \$(srcdir) name-rev --name-only HEAD"
else
    GITVERSION=""
    GITTAG=""
    GITBRANCH=""
fi

AC_CONFIG_SRCDIR([Include/object.h])
AC_CONFIG_HEADER(pyconfig.h)

AC_CANONICAL_HOST
AC_SUBST(build)
AC_SUBST(host)

# pybuilddir.txt will be created by --generate-posix-vars in the Makefile
rm -f pybuilddir.txt

AC_CHECK_PROGS(PYTHON_FOR_REGEN, python$PACKAGE_VERSION python3 python, python3)
AC_SUBST(PYTHON_FOR_REGEN)

if test "$cross_compiling" = yes; then
    AC_MSG_CHECKING([for python interpreter for cross build])
    if test -z "$PYTHON_FOR_BUILD"; then
        for interp in python$PACKAGE_VERSION python3 python; do
	    which $interp >/dev/null 2>&1 || continue
	    if $interp -c "import sys;sys.exit(not '.'.join(str(n) for n in sys.version_info@<:@:2@:>@) == '$PACKAGE_VERSION')"; then
	        break
	    fi
            interp=
	done
        if test x$interp = x; then
	    AC_MSG_ERROR([python$PACKAGE_VERSION interpreter not found])
	fi
        AC_MSG_RESULT($interp)
	PYTHON_FOR_BUILD='_PYTHON_PROJECT_BASE=$(abs_builddir) _PYTHON_HOST_PLATFORM=$(_PYTHON_HOST_PLATFORM) PYTHONPATH=$(shell test -f pybuilddir.txt && echo $(abs_builddir)/`cat pybuilddir.txt`:)$(srcdir)/Lib _PYTHON_SYSCONFIGDATA_NAME=_sysconfigdata_$(ABIFLAGS)_$(MACHDEP)_$(MULTIARCH) '$interp
    fi
elif test "$cross_compiling" = maybe; then
    AC_MSG_ERROR([Cross compiling required --host=HOST-TUPLE and --build=ARCH])
else
    PYTHON_FOR_BUILD='./$(BUILDPYTHON) -E'
fi
AC_SUBST(PYTHON_FOR_BUILD)

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

# The later defininition of _XOPEN_SOURCE disables certain features
# on Linux, so we need _GNU_SOURCE to re-enable them (makedev, tm_zone).
AC_DEFINE(_GNU_SOURCE, 1, [Define on Linux to activate all library features])

# The later defininition of _XOPEN_SOURCE and _POSIX_C_SOURCE disables
# certain features on NetBSD, so we need _NETBSD_SOURCE to re-enable
# them.
AC_DEFINE(_NETBSD_SOURCE, 1, [Define on NetBSD to activate all library features])

# The later defininition of _XOPEN_SOURCE and _POSIX_C_SOURCE disables
# certain features on FreeBSD, so we need __BSD_VISIBLE to re-enable
# them.
AC_DEFINE(__BSD_VISIBLE, 1, [Define on FreeBSD to activate all library features])

# The later defininition of _XOPEN_SOURCE and _POSIX_C_SOURCE disables
# certain features on Mac OS X, so we need _DARWIN_C_SOURCE to re-enable
# them.
AC_DEFINE(_DARWIN_C_SOURCE, 1, [Define on Darwin to activate all library features])


define_xopen_source=yes

# Arguments passed to configure.
AC_SUBST(CONFIG_ARGS)
CONFIG_ARGS="$ac_configure_args"

AC_MSG_CHECKING([for --enable-universalsdk])
AC_ARG_ENABLE(universalsdk,
	AS_HELP_STRING([--enable-universalsdk@<:@=SDKDIR@:>@], [Build fat binary against Mac OS X SDK]),
[
	case $enableval in
	yes)
		# Locate the best usable SDK, see Mac/README.txt for more
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
AC_MSG_CHECKING(for --with-universal-archs)
AC_ARG_WITH(universal-archs,
    AS_HELP_STRING([--with-universal-archs=ARCH], [select architectures for universal build ("32-bit", "64-bit", "3-way", "intel", "intel-32", or "all")]),
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
                             [specify an alternate name of the framework built with --enable-framework]),
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
              AS_HELP_STRING([--enable-framework@<:@=INSTALLDIR@:>@], [Build (MacOSX|Darwin) framework]),
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

##AC_ARG_WITH(dyld,
##            AS_HELP_STRING([--with-dyld],
##                           [Use (OpenStep|Rhapsody) dynamic linker]))
##
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
	linux*) MACHDEP="linux";;
	cygwin*) MACHDEP="cygwin";;
	darwin*) MACHDEP="darwin";;
	'')	MACHDEP="unknown";;
    esac
fi

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
  # On FreeBSD 4, the math functions C89 does not cover are never defined
  # with _XOPEN_SOURCE and __BSD_VISIBLE does not re-enable them.
  FreeBSD/4.*)
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
  Darwin/1@<:@0-9@:>@.*)
    define_xopen_source=no;;
  # On AIX 4 and 5.1, mbstate_t is defined only when _XOPEN_SOURCE == 500 but
  # used in wcsnrtombs() and mbsnrtowcs() even if _XOPEN_SOURCE is not defined
  # or has another value. By not (re)defining it, the defaults come in place.
  AIX/4)
    define_xopen_source=no;;
  AIX/5)
    if test `uname -r` -eq 1; then
      define_xopen_source=no
    fi
    ;;
  # On QNX 6.3.2, defining _XOPEN_SOURCE prevents netdb.h from
  # defining NI_NUMERICHOST.
  QNX/6.3.2)
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

# XXX shouldn't some/most/all of this code be merged with the stuff later
# on that fiddles with OPT and BASECFLAGS?
AC_MSG_CHECKING(for --without-gcc)
AC_ARG_WITH(gcc,
            AS_HELP_STRING([--without-gcc], [never use gcc]),
[
	case $withval in
	no)	CC=${CC:-cc}
		without_gcc=yes;;
	yes)	CC=gcc
		without_gcc=no;;
	*)	CC=$withval
		without_gcc=$withval;;
	esac], [
	case $ac_sys_system in
	AIX*)   CC=${CC:-xlc_r}
		without_gcc=;;
	*)	without_gcc=no;;
	esac])
AC_MSG_RESULT($without_gcc)

AC_MSG_CHECKING(for --with-icc)
AC_ARG_WITH(icc,
            AS_HELP_STRING([--with-icc], [build with icc]),
[
	case $withval in
	no)	CC=${CC:-cc}
		with_icc=no;;
	yes)	CC=icc
		CXX=icpc
		with_icc=yes;;
	*)	CC=$withval
		with_icc=$withval;;
	esac], [
	with_icc=no])
AC_MSG_RESULT($with_icc)

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

if test "$ac_sys_system" = "Darwin"
then
	# Compiler selection on MacOSX is more complicated than
	# AC_PROG_CC can handle, see Mac/README.txt for more
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

AC_SUBST(CXX)
AC_SUBST(MAINCC)
AC_MSG_CHECKING(for --with-cxx-main=<compiler>)
AC_ARG_WITH(cxx_main,
            AS_HELP_STRING([--with-cxx-main=<compiler>],
                           [compile main() and link python executable with C++ compiler]),
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


MULTIARCH=$($CC --print-multiarch 2>/dev/null)
AC_SUBST(MULTIARCH)

AC_MSG_CHECKING([for the platform triplet based on compiler characteristics])
cat >> conftest.c <<EOF
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
#else
# error unknown platform triplet
#endif

EOF

if $CPP $CPPFLAGS conftest.c >conftest.out 2>/dev/null; then
  PLATFORM_TRIPLET=`grep -v '^#' conftest.out | grep -v '^ *$' | tr -d ' 	'`
  AC_MSG_RESULT([$PLATFORM_TRIPLET])
else
  AC_MSG_RESULT([none])
fi
rm -f conftest.c conftest.out

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

AC_MSG_CHECKING([for -Wl,--no-as-needed])
save_LDFLAGS="$LDFLAGS"
LDFLAGS="$LDFLAGS -Wl,--no-as-needed"
AC_LINK_IFELSE([AC_LANG_PROGRAM([[]], [[]])],
  [NO_AS_NEEDED="-Wl,--no-as-needed"
   AC_MSG_RESULT([yes])],
  [NO_AS_NEEDED=""
   AC_MSG_RESULT([no])])
LDFLAGS="$save_LDFLAGS"
AC_SUBST(NO_AS_NEEDED)


# checks for UNIX variants that set C preprocessor variables
AC_USE_SYSTEM_EXTENSIONS

AC_MSG_CHECKING([for the Android API level])
cat >> conftest.c <<EOF
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
    echo 'Fatal: you must define __ANDROID_API__'
    exit 1
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
case $ac_sys_system/$ac_sys_release in
atheos*|Linux*/1*)
   echo This system \($ac_sys_system/$ac_sys_release\) is no longer supported.
   echo See README for details.
   exit 1;;
esac

AC_EXEEXT
AC_MSG_CHECKING(for --with-suffix)
AC_ARG_WITH(suffix,
            AS_HELP_STRING([--with-suffix=.exe], [set executable suffix]),
[
	case $withval in
	no)	EXEEXT=;;
	yes)	EXEEXT=.exe;;
	*)	EXEEXT=$withval;;
	esac])
AC_MSG_RESULT($EXEEXT)

# Test whether we're running on a non-case-sensitive system, in which
# case we give a warning if no ext is given
AC_SUBST(BUILDEXEEXT)
AC_MSG_CHECKING(for case-insensitive build directory)
if test ! -d CaseSensitiveTestDir; then
mkdir CaseSensitiveTestDir
fi

if test -d casesensitivetestdir
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
# This is altered for AIX in order to build the export list before
# linking.
AC_SUBST(LINKCC)
AC_MSG_CHECKING(LINKCC)
if test -z "$LINKCC"
then
	LINKCC='$(PURIFY) $(MAINCC)'
	case $ac_sys_system in
	AIX*)
	   exp_extra="\"\""
	   if test $ac_sys_release -ge 5 -o \
		   $ac_sys_release -eq 4 -a `uname -r` -ge 2 ; then
	       exp_extra="."
	   fi
	   LINKCC="\$(srcdir)/Modules/makexp_aix Modules/python.exp $exp_extra \$(LIBRARY); $LINKCC";;
	QNX*)
	   # qcc must be used because the other compilers do not
	   # support -N.
	   LINKCC=qcc;;
	esac
fi
AC_MSG_RESULT($LINKCC)

# GNULD is set to "yes" if the GNU linker is used.  If this goes wrong
# make sure we default having it set to "no": this is used by
# distutils.unixccompiler to know if it should add --enable-new-dtags
# to linker command lines, and failing to detect GNU ld simply results
# in the same bahaviour as before.
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
              AS_HELP_STRING([--enable-shared], [disable/enable building shared python library]))

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

AC_MSG_CHECKING(for --enable-profiling)
AC_ARG_ENABLE(profiling,
              AS_HELP_STRING([--enable-profiling], [enable C-level code profiling]))
if test "x$enable_profiling" = xyes; then
  ac_save_cc="$CC"
  CC="$CC -pg"
  AC_LINK_IFELSE([AC_LANG_SOURCE([[int main() { return 0; }]])],
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
    Linux*|GNU*|NetBSD*|FreeBSD*|DragonFly*|OpenBSD*)
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

AC_MSG_RESULT($LDLIBRARY)

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
            AS_HELP_STRING([--with-pydebug], [build with Py_DEBUG defined]),
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

# Check for --with-assertions. Py_DEBUG implies assertions, but also changes
# the ABI. This allows enabling assertions without changing the ABI.
assertions='false'
AC_MSG_CHECKING(for --with-assertions)
AC_ARG_WITH(assertions,
            AC_HELP_STRING([--with-assertions], [build with C assertions enabled]),
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
AC_ARG_ENABLE(optimizations, AS_HELP_STRING([--enable-optimizations], [Enable expensive, stable optimizations (PGO, etc).  Disabled by default.]),
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
else
  DEF_MAKE_ALL_RULE="build_all"
  REQUIRE_PGO="no"
  DEF_MAKE_RULE="all"
fi

# Enable LTO flags
AC_MSG_CHECKING(for --with-lto)
AC_ARG_WITH(lto, AS_HELP_STRING([--with-lto], [Enable Link Time Optimization in any build. Disabled by default.]),
[
if test "$withval" != no
then
  Py_LTO='true'
  AC_MSG_RESULT(yes);
else
  Py_LTO='false'
  AC_MSG_RESULT(no);
fi],
[AC_MSG_RESULT(no)])
if test "$Py_LTO" = 'true' ; then
  case $CC in
    *clang*)
      case $ac_sys_system in
        Darwin*)
          # Any changes made here should be reflected in the GCC+Darwin case below
          LTOFLAGS="-flto -Wl,-export_dynamic"
          ;;
        *)
          LTOFLAGS="-flto"
          ;;
      esac
      ;;
    *gcc*)
      case $ac_sys_system in
        Darwin*)
          LTOFLAGS="-flto -Wl,-export_dynamic"
          ;;
        *)
          LTOFLAGS="-flto -fuse-linker-plugin -ffat-lto-objects -flto-partition=none"
          ;;
      esac
      ;;
  esac
  CFLAGS="$CFLAGS $LTOFLAGS"
  LDFLAGS="$LDFLAGS $LTOFLAGS"
fi

# Enable PGO flags.
AC_SUBST(PGO_PROF_GEN_FLAG)
AC_SUBST(PGO_PROF_USE_FLAG)
AC_SUBST(LLVM_PROF_MERGER)
AC_SUBST(LLVM_PROF_FILE)
AC_SUBST(LLVM_PROF_ERR)
# Make this work on systems where llvm tools are not installed with their
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
AC_SUBST(LLVM_PROFDATA)
AC_PATH_TARGET_TOOL(LLVM_PROFDATA, llvm-profdata, '', ${llvm_path})
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

# tweak OPT based on compiler and platform, only if the user didn't set
# it on the command line
AC_SUBST(OPT)
AC_SUBST(CFLAGS_ALIASING)
if test "${OPT-unset}" = "unset"
then
    case $GCC in
    yes)
        if test "$CC" != 'g++' ; then
	    STRICT_PROTO="-Wstrict-prototypes"
	fi
        # For gcc 4.x we need to use -fwrapv so lets check if its supported
        if "$CC" -v --help 2>/dev/null |grep -- -fwrapv > /dev/null; then
           WRAP="-fwrapv"
        fi

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
		# Optimization messes up debuggers, so turn it off for
		# debug builds.
                if "$CC" -v --help 2>/dev/null |grep -- -Og > /dev/null; then
                    OPT="-g -Og -Wall"
                else
                    OPT="-g -O0 -Wall"
                fi
	    else
		OPT="-g $WRAP -O3 -Wall"
	    fi
	    ;;
	*)
	    OPT="-O3 -Wall"
	    ;;
	esac

	OPT="$OPT $STRICT_PROTO"

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

AC_SUBST(BASECFLAGS)
AC_SUBST(CFLAGS_NODIST)

# The -arch flags for universal builds on OSX
UNIVERSAL_ARCH_FLAGS=
AC_SUBST(UNIVERSAL_ARCH_FLAGS)

# tweak BASECFLAGS based on compiler and platform
case $GCC in
yes)
    CFLAGS_NODIST="$CFLAGS_NODIST -std=c99"

    AC_MSG_CHECKING(for -Wextra)
     ac_save_cc="$CC"
     CC="$CC -Wextra -Werror"
     AC_CACHE_VAL(ac_cv_extra_warnings,
       AC_COMPILE_IFELSE(
         [
           AC_LANG_PROGRAM([[]], [[]])
         ],[
           ac_cv_extra_warnings=yes
         ],[
           ac_cv_extra_warnings=no
         ]))
     CC="$ac_save_cc"
    AC_MSG_RESULT($ac_cv_extra_warnings)

    if test $ac_cv_extra_warnings = yes
    then
      CFLAGS_NODIST="$CFLAGS_NODIST -Wextra"
    fi

    # Python doesn't violate C99 aliasing rules, but older versions of
    # GCC produce warnings for legal Python code.  Enable
    # -fno-strict-aliasing on versions of GCC that support but produce
    # warnings.  See Issue3326
    AC_MSG_CHECKING(whether $CC accepts and needs -fno-strict-aliasing)
     ac_save_cc="$CC"
     CC="$CC -fno-strict-aliasing"
     save_CFLAGS="$CFLAGS"
     AC_CACHE_VAL(ac_cv_no_strict_aliasing,
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
    AC_MSG_RESULT($ac_cv_no_strict_aliasing)
    if test $ac_cv_no_strict_aliasing = yes
    then
      BASECFLAGS="$BASECFLAGS -fno-strict-aliasing"
    fi

    # ICC doesn't recognize the option, but only emits a warning
    ## XXX does it emit an unused result warning and can it be disabled?
    case "$CC" in
    *icc*)
    ac_cv_disable_unused_result_warning=no
    ;;
    *)
    AC_MSG_CHECKING(if we can turn off $CC unused result warning)
     ac_save_cc="$CC"
     CC="$CC -Wunused-result -Werror"
     save_CFLAGS="$CFLAGS"
     AC_CACHE_VAL(ac_cv_disable_unused_result_warning,
       AC_COMPILE_IFELSE(
         [
	   AC_LANG_PROGRAM([[]], [[]])
	 ],[
           ac_cv_disable_unused_result_warning=yes
	 ],[
           ac_cv_disable_unused_result_warning=no
	 ]))
     CFLAGS="$save_CFLAGS"
     CC="$ac_save_cc"
    AC_MSG_RESULT($ac_cv_disable_unused_result_warning)
    ;;
    esac

    if test $ac_cv_disable_unused_result_warning = yes
    then
      BASECFLAGS="$BASECFLAGS -Wno-unused-result"
      CFLAGS_NODIST="$CFLAGS_NODIST -Wno-unused-result"
    fi

    AC_MSG_CHECKING(if we can turn off $CC unused parameter warning)
     ac_save_cc="$CC"
     CC="$CC -Wunused-parameter -Werror"
     AC_CACHE_VAL(ac_cv_disable_unused_parameter_warning,
       AC_COMPILE_IFELSE(
         [
           AC_LANG_PROGRAM([[]], [[]])
         ],[
           ac_cv_disable_unused_parameter_warning=yes
         ],[
           ac_cv_disable_unused_parameter_warning=no
         ]))
     CC="$ac_save_cc"
    AC_MSG_RESULT($ac_cv_disable_unused_parameter_warning)

    if test $ac_cv_disable_unused_parameter_warning = yes
    then
      CFLAGS_NODIST="$CFLAGS_NODIST -Wno-unused-parameter"
    fi

    AC_MSG_CHECKING(if we can turn off $CC missing field initializers warning)
     ac_save_cc="$CC"
     CC="$CC -Wmissing-field-initializers -Werror"
     AC_CACHE_VAL(ac_cv_disable_missing_field_initializers,
       AC_COMPILE_IFELSE(
         [
           AC_LANG_PROGRAM([[]], [[]])
         ],[
           ac_cv_disable_missing_field_initializers=yes
         ],[
           ac_cv_disable_missing_field_initializers=no
         ]))
     CC="$ac_save_cc"
    AC_MSG_RESULT($ac_cv_disable_missing_field_initializers)

    if test $ac_cv_disable_missing_field_initializers = yes
    then
      CFLAGS_NODIST="$CFLAGS_NODIST -Wno-missing-field-initializers"
    fi

    AC_MSG_CHECKING(if we can turn on $CC mixed sign comparison warning)
     ac_save_cc="$CC"
     CC="$CC -Wsign-compare"
     save_CFLAGS="$CFLAGS"
     AC_CACHE_VAL(ac_cv_enable_sign_compare_warning,
       AC_COMPILE_IFELSE(
         [
	   AC_LANG_PROGRAM([[]], [[]])
	 ],[
           ac_cv_enable_sign_compare_warning=yes
	 ],[
           ac_cv_enable_sign_compare_warning=no
	 ]))
     CFLAGS="$save_CFLAGS"
     CC="$ac_save_cc"
    AC_MSG_RESULT($ac_cv_enable_sign_compare_warning)

    if test $ac_cv_enable_sign_compare_warning = yes
    then
      BASECFLAGS="$BASECFLAGS -Wsign-compare"
    fi

    AC_MSG_CHECKING(if we can turn on $CC unreachable code warning)
     ac_save_cc="$CC"
     CC="$CC -Wunreachable-code"
     save_CFLAGS="$CFLAGS"
     AC_CACHE_VAL(ac_cv_enable_unreachable_code_warning,
       AC_COMPILE_IFELSE(
         [
	   AC_LANG_PROGRAM([[]], [[]])
	 ],[
           ac_cv_enable_unreachable_code_warning=yes
	 ],[
           ac_cv_enable_unreachable_code_warning=no
	 ]))
     CFLAGS="$save_CFLAGS"
     CC="$ac_save_cc"

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
    AC_MSG_RESULT($ac_cv_enable_unreachable_code_warning)

    AC_MSG_CHECKING(if we can make implicit function declaration an error in $CC)
     ac_save_cc="$CC"
     CC="$CC -Werror=implicit-function-declaration"
     AC_CACHE_VAL(ac_cv_enable_implicit_function_declaration_error,
       AC_COMPILE_IFELSE(
         [
	   AC_LANG_PROGRAM([[]], [[]])
	 ],[
           ac_cv_enable_implicit_function_declaration_error=yes
	 ],[
           ac_cv_enable_implicit_function_declaration_error=no
	 ]))
     CC="$ac_save_cc"
    AC_MSG_RESULT($ac_cv_enable_implicit_function_declaration_error)

    if test $ac_cv_enable_implicit_function_declaration_error = yes
    then
      CFLAGS_NODIST="$CFLAGS_NODIST -Werror=implicit-function-declaration"
    fi

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

    # is there any other compiler on Darwin besides gcc?
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
            3-way)
               UNIVERSAL_ARCH_FLAGS="-arch i386 -arch ppc -arch x86_64"
               LIPO_32BIT_FLAGS="-extract ppc7400 -extract i386"
               ARCH_RUN_32BIT="/usr/bin/arch -i386 -ppc"
               ;;
            *)
               AC_MSG_ERROR([proper usage is --with-universal-arch=32-bit|64-bit|all|intel|3-way])
               ;;
            esac

            CFLAGS="${UNIVERSAL_ARCH_FLAGS} -isysroot ${UNIVERSALSDK} ${CFLAGS}"
            LDFLAGS="${UNIVERSAL_ARCH_FLAGS} -isysroot ${UNIVERSALSDK} ${LDFLAGS}"
            if test "${UNIVERSALSDK}" != "/"
            then
                CPPFLAGS="-isysroot ${UNIVERSALSDK} ${CPPFLAGS}"
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

# ICC needs -fp-model strict or floats behave badly
case "$CC" in
*icc*)
    CFLAGS_NODIST="$CFLAGS_NODIST -fp-model strict"
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
AC_MSG_CHECKING(whether pthreads are available without options)
AC_CACHE_VAL(ac_cv_pthread_is_default,
[AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <stdio.h>
#include <pthread.h>

void* routine(void* p){return NULL;}

int main(){
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
AC_MSG_RESULT($ac_cv_pthread_is_default)


if test $ac_cv_pthread_is_default = yes
then
  ac_cv_kpthread=no
else
# -Kpthread, if available, provides the right #defines
# and linker options to make pthread_create available
# Some compilers won't report that they do not support -Kpthread,
# so we need to run a program to see whether it really made the
# function available.
AC_MSG_CHECKING(whether $CC accepts -Kpthread)
AC_CACHE_VAL(ac_cv_kpthread,
[ac_save_cc="$CC"
CC="$CC -Kpthread"
AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <stdio.h>
#include <pthread.h>

void* routine(void* p){return NULL;}

int main(){
  pthread_t p;
  if(pthread_create(&p,NULL,routine,NULL)!=0)
    return 1;
  (void)pthread_detach(p);
  return 0;
}
]])],[ac_cv_kpthread=yes],[ac_cv_kpthread=no],[ac_cv_kpthread=no])
CC="$ac_save_cc"])
AC_MSG_RESULT($ac_cv_kpthread)
fi

if test $ac_cv_kpthread = no -a $ac_cv_pthread_is_default = no
then
# -Kthread, if available, provides the right #defines
# and linker options to make pthread_create available
# Some compilers won't report that they do not support -Kthread,
# so we need to run a program to see whether it really made the
# function available.
AC_MSG_CHECKING(whether $CC accepts -Kthread)
AC_CACHE_VAL(ac_cv_kthread,
[ac_save_cc="$CC"
CC="$CC -Kthread"
AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <stdio.h>
#include <pthread.h>

void* routine(void* p){return NULL;}

int main(){
  pthread_t p;
  if(pthread_create(&p,NULL,routine,NULL)!=0)
    return 1;
  (void)pthread_detach(p);
  return 0;
}
]])],[ac_cv_kthread=yes],[ac_cv_kthread=no],[ac_cv_kthread=no])
CC="$ac_save_cc"])
AC_MSG_RESULT($ac_cv_kthread)
fi

if test $ac_cv_kthread = no -a $ac_cv_pthread_is_default = no
then
# -pthread, if available, provides the right #defines
# and linker options to make pthread_create available
# Some compilers won't report that they do not support -pthread,
# so we need to run a program to see whether it really made the
# function available.
AC_MSG_CHECKING(whether $CC accepts -pthread)
AC_CACHE_VAL(ac_cv_pthread,
[ac_save_cc="$CC"
CC="$CC -pthread"
AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <stdio.h>
#include <pthread.h>

void* routine(void* p){return NULL;}

int main(){
  pthread_t p;
  if(pthread_create(&p,NULL,routine,NULL)!=0)
    return 1;
  (void)pthread_detach(p);
  return 0;
}
]])],[ac_cv_pthread=yes],[ac_cv_pthread=no],[ac_cv_pthread=no])
CC="$ac_save_cc"])
AC_MSG_RESULT($ac_cv_pthread)
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

# checks for header files
AC_HEADER_STDC
AC_CHECK_HEADERS(asm/types.h conio.h direct.h dlfcn.h errno.h \
fcntl.h grp.h \
ieeefp.h io.h langinfo.h libintl.h process.h pthread.h \
sched.h shadow.h signal.h stropts.h termios.h \
unistd.h utime.h \
poll.h sys/devpoll.h sys/epoll.h sys/poll.h \
sys/audioio.h sys/xattr.h sys/bsdtty.h sys/event.h sys/file.h sys/ioctl.h \
sys/kern_control.h sys/loadavg.h sys/lock.h sys/mkdev.h sys/modem.h \
sys/param.h sys/random.h sys/select.h sys/sendfile.h sys/socket.h sys/statvfs.h \
sys/stat.h sys/syscall.h sys/sys_domain.h sys/termio.h sys/time.h \
sys/times.h sys/types.h sys/uio.h sys/un.h sys/utsname.h sys/wait.h pty.h \
libutil.h sys/resource.h netpacket/packet.h sysexits.h bluetooth.h \
linux/tipc.h linux/random.h spawn.h util.h alloca.h endian.h \
sys/endian.h sys/sysmacros.h)
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
#ifdef STDC_HEADERS
# include <stdlib.h>
# include <stddef.h>
#else
# ifdef HAVE_STDLIB_H
#  include <stdlib.h>
# endif
#endif
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

AC_CHECK_HEADERS(linux/vm_sockets.h,,,[
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
])

# On Linux, can.h and can/raw.h require sys/socket.h
AC_CHECK_HEADERS(linux/can.h linux/can/raw.h linux/can/bcm.h,,,[
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
])

# checks for typedefs
was_it_defined=no
AC_MSG_CHECKING(for clock_t in time.h)
AC_EGREP_HEADER(clock_t, time.h, was_it_defined=yes, [
    AC_DEFINE(clock_t, long, [Define to 'long' if <time.h> doesn't define.])
])
AC_MSG_RESULT($was_it_defined)

AC_MSG_CHECKING(for makedev)
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
],[ac_cv_has_makedev=yes],[ac_cv_has_makedev=no])
AC_MSG_RESULT($ac_cv_has_makedev)
if test "$ac_cv_has_makedev" = "yes"; then
    AC_DEFINE(HAVE_MAKEDEV, 1, [Define this if you have the makedev macro.])
fi

# byte swapping
AC_MSG_CHECKING(for le64toh)
AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#ifdef HAVE_ENDIAN_H
#include <endian.h>
#elif defined(HAVE_SYS_ENDIAN_H)
#include <sys/endian.h>
#endif
]], [[
   le64toh(1) ]])
],[ac_cv_has_le64toh=yes],[ac_cv_has_le64toh=no])
AC_MSG_RESULT($ac_cv_has_le64toh)
if test "$ac_cv_has_le64toh" = "yes"; then
    AC_DEFINE(HAVE_HTOLE64, 1, [Define this if you have le64toh()])
fi

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

# Sizes of various common basic types
# ANSI C requires sizeof(char) == 1, so no need to check it
AC_CHECK_SIZEOF(int, 4)
AC_CHECK_SIZEOF(long, 4)
AC_CHECK_SIZEOF(long long, 8)
AC_CHECK_SIZEOF(void *, 4)
AC_CHECK_SIZEOF(short, 2)
AC_CHECK_SIZEOF(float, 4)
AC_CHECK_SIZEOF(double, 8)
AC_CHECK_SIZEOF(fpos_t, 4)
AC_CHECK_SIZEOF(size_t, 4)
AC_CHECK_SIZEOF(pid_t, 4)
AC_CHECK_SIZEOF(uintptr_t)

AC_MSG_CHECKING(for long double support)
have_long_double=no
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[long double x; x = (long double)0;]])],[
  AC_DEFINE(HAVE_LONG_DOUBLE, 1, [Define this if you have the type long double.])
  have_long_double=yes
],[])
AC_MSG_RESULT($have_long_double)
if test "$have_long_double" = yes ; then
AC_CHECK_SIZEOF(long double, 16)
fi

AC_CHECK_SIZEOF(_Bool, 1)

AC_CHECK_SIZEOF(off_t, [], [
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
])

AC_MSG_CHECKING(whether to enable large file support)
if test "$ac_cv_sizeof_off_t" -gt "$ac_cv_sizeof_long" -a \
	"$ac_cv_sizeof_long_long" -ge "$ac_cv_sizeof_off_t"; then
  AC_DEFINE(HAVE_LARGEFILE_SUPPORT, 1,
  [Defined to enable large file support when an off_t is bigger than a long
   and long long is available and at least as big as an off_t. You may need
   to add some flags for configuration and compilation to enable this mode.
   (For Solaris and Linux, the necessary defines are already defined.)])
  AC_MSG_RESULT(yes)
else
  AC_MSG_RESULT(no)
fi

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

AC_MSG_CHECKING(for pthread_t)
have_pthread_t=no
AC_COMPILE_IFELSE([
  AC_LANG_PROGRAM([[#include <pthread.h>]], [[pthread_t x; x = *(pthread_t*)0;]])
],[have_pthread_t=yes],[])
AC_MSG_RESULT($have_pthread_t)
if test "$have_pthread_t" = yes ; then
  AC_CHECK_SIZEOF(pthread_t, [], [
#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif
  ])
fi

# Issue #25658: POSIX hasn't defined that pthread_key_t is compatible with int.
# This checking will be unnecessary after removing deprecated TLS API.
AC_CHECK_SIZEOF(pthread_key_t, [], [[#include <pthread.h>]])
AC_MSG_CHECKING(whether pthread_key_t is compatible with int)
if test "$ac_cv_sizeof_pthread_key_t" -eq "$ac_cv_sizeof_int" ; then
  AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM([[#include <pthread.h>]], [[pthread_key_t k; k * 1;]])],
    [ac_pthread_key_t_is_arithmetic_type=yes],
    [ac_pthread_key_t_is_arithmetic_type=no]
  )
  AC_MSG_RESULT($ac_pthread_key_t_is_arithmetic_type)
  if test "$ac_pthread_key_t_is_arithmetic_type" = yes ; then
    AC_DEFINE(PTHREAD_KEY_T_IS_COMPATIBLE_WITH_INT, 1,
              [Define if pthread_key_t is compatible with int.])
  fi
else
  AC_MSG_RESULT(no)
fi
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
    		AC_MSG_ERROR([Unexpected output of 'arch' on OSX])
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
    	*)
    		AC_MSG_ERROR([Unexpected output of 'arch' on OSX])
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
			fi
		else
			# building for OS X 10.3 and later
			LDSHARED='$(CC) -bundle -undefined dynamic_lookup'
			LDCXXSHARED='$(CXX) -bundle -undefined dynamic_lookup'
			BLDSHARED="$LDSHARED"
		fi
		;;
	Linux*|GNU*|QNX*)
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
	CYGWIN*)
		LDSHARED="gcc -shared -Wl,--enable-auto-image-base"
		LDCXXSHARED="g++ -shared -Wl,--enable-auto-image-base";;
	*)	LDSHARED="ld";;
	esac
fi
AC_MSG_RESULT($LDSHARED)
LDCXXSHARED=${LDCXXSHARED-$LDSHARED}
BLDSHARED=${BLDSHARED-$LDSHARED}
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
	FreeBSD*|NetBSD*|OpenBSD*|DragonFly*) CCSHARED="-fPIC";;
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
		LINKFORSHARED="-Wl,-stack_size,1000000 $LINKFORSHARED"

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

# checks for uuid.h location
AC_CHECK_HEADERS([uuid/uuid.h uuid.h])

AC_MSG_CHECKING(for uuid_generate_time_safe)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <uuid/uuid.h>]], [[
#ifndef uuid_generate_time_safe
void *x = uuid_generate_time_safe
#endif
]])],
  [AC_DEFINE(HAVE_UUID_GENERATE_TIME_SAFE, 1, Define if uuid_generate_time_safe() exists.)
   AC_MSG_RESULT(yes)],
  [AC_MSG_RESULT(no)]
)

# AIX provides support for RFC4122 (uuid) in libc.a starting with AIX 6.1 (anno 2007)
AC_MSG_CHECKING(for RFC4122 - uuid support on AIX)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <uuid.h>]], [[
#ifndef uuid_create
void *x = uuid_create
#endif
]])],
  [AC_DEFINE(HAVE_UUID_CREATE, 1, Define if uuid_create() exists. AIX support for uuid:RFC4122)
   AC_MSG_RESULT(yes)],
  [AC_MSG_RESULT(no)]
)

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
		]);;
	*) ;;
esac

# check for systems that require aligned memory access
AC_MSG_CHECKING(aligned memory access is required)
AC_CACHE_VAL(ac_cv_aligned_required,
[AC_RUN_IFELSE([AC_LANG_SOURCE([[
int main()
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
AC_MSG_RESULT($ac_cv_aligned_required)
if test "$ac_cv_aligned_required" = yes ; then
  AC_DEFINE([HAVE_ALIGNED_REQUIRED], [1],
    [Define if aligned memory access is required])
fi

# str, bytes and memoryview hash algorithm
AH_TEMPLATE(Py_HASH_ALGORITHM,
  [Define hash algorithm for str, bytes and memoryview.
   SipHash24: 1, FNV: 2, externally defined: 0])

AC_MSG_CHECKING(for --with-hash-algorithm)
dnl quadrigraphs "@<:@" and "@:>@" produce "[" and "]" in the output
AC_ARG_WITH(hash_algorithm,
            AS_HELP_STRING([--with-hash-algorithm=@<:@fnv|siphash24@:>@],
                           [select hash algorithm]),
[
AC_MSG_RESULT($withval)
case "$withval" in
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

AC_MSG_CHECKING(for --with-address-sanitizer)
AC_ARG_WITH(address_sanitizer,
            AS_HELP_STRING([--with-address-sanitizer],
                           [enable AddressSanitizer]),
[
AC_MSG_RESULT($withval)
BASECFLAGS="-fsanitize=address -fno-omit-frame-pointer $BASECFLAGS"
LDFLAGS="-fsanitize=address $LDFLAGS"
],
[AC_MSG_RESULT(no)])

# Most SVR4 platforms (e.g. Solaris) need -lsocket and -lnsl.
AC_CHECK_LIB(nsl, t_open, [LIBS="-lnsl $LIBS"]) # SVR4
AC_CHECK_LIB(socket, socket, [LIBS="-lsocket $LIBS"], [], $LIBS) # SVR4 sockets

AC_MSG_CHECKING(for --with-libs)
AC_ARG_WITH(libs,
            AS_HELP_STRING([--with-libs='lib1 ...'], [link against additional libs]),
[
AC_MSG_RESULT($withval)
LIBS="$withval $LIBS"
],
[AC_MSG_RESULT(no)])

PKG_PROG_PKG_CONFIG

# Check for use of the system expat library
AC_MSG_CHECKING(for --with-system-expat)
AC_ARG_WITH(system_expat,
            AS_HELP_STRING([--with-system-expat], [build pyexpat module using an installed expat library]),
            [],
            [with_system_expat="no"])

AC_MSG_RESULT($with_system_expat)

# Check for use of the system libffi library
AC_MSG_CHECKING(for --with-system-ffi)
AC_ARG_WITH(system_ffi,
            AS_HELP_STRING([--with-system-ffi], [build _ctypes module using an installed ffi library]),,,)

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
            AS_HELP_STRING([--with-system-libmpdec], [build _decimal module using an installed libmpdec library]),
            [],
            [with_system_libmpdec="no"])

AC_MSG_RESULT($with_system_libmpdec)

# Check for support for loadable sqlite extensions
AC_MSG_CHECKING(for --enable-loadable-sqlite-extensions)
AC_ARG_ENABLE(loadable-sqlite-extensions,
              AS_HELP_STRING([--enable-loadable-sqlite-extensions], [support loadable extensions in _sqlite module]),
              [],
              [enable_loadable_sqlite_extensions="no"])

AC_MSG_RESULT($enable_loadable_sqlite_extensions)

# Check for --with-tcltk-includes=path and --with-tcltk-libs=path
AC_SUBST(TCLTK_INCLUDES)
AC_SUBST(TCLTK_LIBS)
AC_MSG_CHECKING(for --with-tcltk-includes)
AC_ARG_WITH(tcltk-includes,
            AS_HELP_STRING([--with-tcltk-includes='-I...'], [override search for Tcl and Tk include files]),
            [],
            [with_tcltk_includes="default"])
AC_MSG_RESULT($with_tcltk_includes)
AC_MSG_CHECKING(for --with-tcltk-libs)
AC_ARG_WITH(tcltk-libs,
            AS_HELP_STRING([--with-tcltk-libs='-L...'], [override search for Tcl and Tk libs]),
            [],
            [with_tcltk_libs="default"])
AC_MSG_RESULT($with_tcltk_libs)
if test "x$with_tcltk_includes" = xdefault || test "x$with_tcltk_libs" = xdefault
then
  if test "x$with_tcltk_includes" != "x$with_tcltk_libs"
  then
    AC_MSG_ERROR([use both --with-tcltk-includes='...' and --with-tcltk-libs='...' or neither])
  fi
  TCLTK_INCLUDES=""
  TCLTK_LIBS=""
else
  TCLTK_INCLUDES="$with_tcltk_includes"
  TCLTK_LIBS="$with_tcltk_libs"
fi

# Check for --with-dbmliborder
AC_MSG_CHECKING(for --with-dbmliborder)
AC_ARG_WITH(dbmliborder,
            AS_HELP_STRING([--with-dbmliborder=db1:db2:...], [order to check db backends for dbm. Valid value is a colon separated string with the backend names `ndbm', `gdbm' and `bdb'.]),
[
if test x$with_dbmliborder = xyes
then
AC_MSG_ERROR([proper usage is --with-dbmliborder=db1:db2:...])
else
  for db in `echo $with_dbmliborder | sed 's/:/ /g'`; do
    if test x$db != xndbm && test x$db != xgdbm && test x$db != xbdb
    then
      AC_MSG_ERROR([proper usage is --with-dbmliborder=db1:db2:...])
    fi
  done
fi])
AC_MSG_RESULT($with_dbmliborder)

AC_SUBST(LDLAST)

# Templates for things AC_DEFINEd more than once.
# For a single AC_DEFINE, no template is needed.
AH_TEMPLATE(_REENTRANT,
  [Define to force use of thread-safe errno, h_errno, and other functions])

if test "$ac_cv_pthread_is_default" = yes
then
    # Defining _REENTRANT on system with POSIX threads should not hurt.
    AC_DEFINE(_REENTRANT)
    posix_threads=yes
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
    AC_MSG_ERROR([could not find pthreads on your system])
    ])
    ])])])])])

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
      esac

      AC_MSG_CHECKING(if PTHREAD_SCOPE_SYSTEM is supported)
      AC_CACHE_VAL(ac_cv_pthread_system_supported,
      [AC_RUN_IFELSE([AC_LANG_SOURCE([[
      #include <stdio.h>
      #include <pthread.h>
      void *foo(void *parm) {
        return NULL;
      }
      main() {
        pthread_attr_t attr;
        pthread_t id;
        if (pthread_attr_init(&attr)) exit(-1);
        if (pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM)) exit(-1);
        if (pthread_create(&id, &attr, foo, NULL)) exit(-1);
        exit(0);
      }]])],
      [ac_cv_pthread_system_supported=yes],
      [ac_cv_pthread_system_supported=no],
      [ac_cv_pthread_system_supported=no])
      ])
      AC_MSG_RESULT($ac_cv_pthread_system_supported)
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


# Check for enable-ipv6
AH_TEMPLATE(ENABLE_IPV6, [Define if --enable-ipv6 is specified])
AC_MSG_CHECKING([if --enable-ipv6 is specified])
AC_ARG_ENABLE(ipv6,
[  --enable-ipv6           Enable ipv6 (with ipv4) support
  --disable-ipv6          Disable ipv6 support],
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
  AC_MSG_RESULT(yes)
  ipv6=yes
],[
  AC_MSG_RESULT(no)
  ipv6=no
])

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
		echo "using lib$ipv6lib"
	else
		if test $ipv6trylibc = "yes"; then
			echo "using libc"
		else
			echo 'Fatal: no $ipv6lib library found.  cannot continue.'
			echo "You need to fetch lib$ipv6lib.a from appropriate"
			echo 'ipv6 kit and compile beforehand.'
			exit 1
		fi
	fi
fi

AC_MSG_CHECKING(for CAN_RAW_FD_FRAMES)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[ /* CAN_RAW_FD_FRAMES available check */
#include <linux/can/raw.h>]],
[[int can_raw_fd_frames = CAN_RAW_FD_FRAMES;]])],[
  AC_DEFINE(HAVE_LINUX_CAN_RAW_FD_FRAMES, 1, [Define if compiling using Linux 3.6 or later.])
  AC_MSG_RESULT(yes)
],[
  AC_MSG_RESULT(no)
])

# Check for --with-doc-strings
AC_MSG_CHECKING(for --with-doc-strings)
AC_ARG_WITH(doc-strings,
            AS_HELP_STRING([--with(out)-doc-strings], [disable/enable documentation strings]))

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
            AS_HELP_STRING([--with(out)-pymalloc], [disable/enable specialized mallocs]))

if test -z "$with_pymalloc"
then
    with_pymalloc="yes"
fi
if test "$with_pymalloc" != "no"
then
    AC_DEFINE(WITH_PYMALLOC, 1,
     [Define if you want to compile in Python-specific mallocs])
    ABIFLAGS="${ABIFLAGS}m"
fi
AC_MSG_RESULT($with_pymalloc)

# Check for --with-c-locale-coercion
AC_MSG_CHECKING(for --with-c-locale-coercion)
AC_ARG_WITH(c-locale-coercion,
            AS_HELP_STRING([--with(out)-c-locale-coercion],
              [disable/enable C locale coercion to a UTF-8 based locale]))

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

# Check for --with-c-locale-warning
AC_MSG_CHECKING(for --with-c-locale-warning)
AC_ARG_WITH(c-locale-warning,
            AS_HELP_STRING([--with(out)-c-locale-warning],
              [disable/enable locale compatibility warning in the C locale]))

if test -z "$with_c_locale_warning"
then
    with_c_locale_warning="yes"
fi
if test "$with_c_locale_warning" != "no"
then
    AC_DEFINE(PY_WARN_ON_C_LOCALE, 1,
      [Define to emit a locale compatibility warning in the C locale])
fi
AC_MSG_RESULT($with_c_locale_warning)

# Check for Valgrind support
AC_MSG_CHECKING([for --with-valgrind])
AC_ARG_WITH([valgrind],
  AS_HELP_STRING([--with-valgrind], [Enable Valgrind support]),,
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
  AC_HELP_STRING(--with(out)-dtrace, [disable/enable DTrace support]),,
  with_dtrace=no)
AC_MSG_RESULT($with_dtrace)

AC_SUBST(DTRACE)
AC_SUBST(DFLAGS)
AC_SUBST(DTRACE_HEADERS)
AC_SUBST(DTRACE_OBJS)
DTRACE=
DFLAGS=
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
            echo 'BEGIN' > conftest.d
            "$DTRACE" -G -s conftest.d -o conftest.o > /dev/null 2>&1 && \
                ac_cv_dtrace_link=yes
      ])
    if test "$ac_cv_dtrace_link" = "yes"; then
        DTRACE_OBJS="Python/pydtrace.o"
    fi
fi

# -I${DLINCLDIR} is added to the compile rule for importdl.o
AC_SUBST(DLINCLDIR)
DLINCLDIR=.

# the dlopen() function means we might want to use dynload_shlib.o. some
# platforms, such as AIX, have dlopen(), but don't want to use it.
AC_CHECK_FUNCS(dlopen)

# DYNLOADFILE specifies which dynload_*.o file we will use for dynamic
# loading of modules.
AC_SUBST(DYNLOADFILE)
AC_MSG_CHECKING(DYNLOADFILE)
if test -z "$DYNLOADFILE"
then
	case $ac_sys_system/$ac_sys_release in
	AIX*) # Use dynload_shlib.c and dlopen() if we have it; otherwise dynload_aix.c
	if test "$ac_cv_func_dlopen" = yes
	then DYNLOADFILE="dynload_shlib.o"
	else DYNLOADFILE="dynload_aix.o"
	fi
	;;
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
AC_CHECK_FUNCS(alarm accept4 setitimer getitimer bind_textdomain_codeset chown \
 clock confstr ctermid dup3 execv faccessat fchmod fchmodat fchown fchownat \
 fexecve fdopendir fork fpathconf fstatat ftime ftruncate futimesat \
 futimens futimes gai_strerror getentropy \
 getgrouplist getgroups getlogin getloadavg getpeername getpgid getpid \
 getpriority getresuid getresgid getpwent getspnam getspent getsid getwd \
 if_nameindex \
 initgroups kill killpg lchmod lchown lockf linkat lstat lutimes mmap \
 memrchr mbrtowc mkdirat mkfifo \
 mkfifoat mknod mknodat mktime mremap nice openat pathconf pause pipe2 plock poll \
 posix_fallocate posix_fadvise pread \
 pthread_init pthread_kill putenv pwrite readlink readlinkat readv realpath renameat \
 sem_open sem_timedwait sem_getvalue sem_unlink sendfile setegid seteuid \
 setgid sethostname \
 setlocale setregid setreuid setresuid setresgid setsid setpgid setpgrp setpriority setuid setvbuf \
 sched_get_priority_max sched_setaffinity sched_setscheduler sched_setparam \
 sched_rr_get_interval \
 sigaction sigaltstack siginterrupt sigpending sigrelse \
 sigtimedwait sigwait sigwaitinfo snprintf strftime strlcpy symlinkat sync \
 sysconf tcgetpgrp tcsetpgrp tempnam timegm times tmpfile tmpnam tmpnam_r \
 truncate uname unlinkat unsetenv utimensat utimes waitid waitpid wait3 wait4 \
 wcscoll wcsftime wcsxfrm wmemcmp writev _getpty)

AC_CHECK_DECL(dirfd,
    AC_DEFINE(HAVE_DIRFD, 1,
              Define if you have the 'dirfd' function or macro.), ,
      [#include <sys/types.h>
       #include <dirent.h>])

# For some functions, having a definition is not sufficient, since
# we want to take their address.
AC_MSG_CHECKING(for chroot)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <unistd.h>]], [[void *x=chroot]])],
  [AC_DEFINE(HAVE_CHROOT, 1, Define if you have the 'chroot' function.)
   AC_MSG_RESULT(yes)],
  [AC_MSG_RESULT(no)
])
AC_MSG_CHECKING(for link)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <unistd.h>]], [[void *x=link]])],
  [AC_DEFINE(HAVE_LINK, 1, Define if you have the 'link' function.)
   AC_MSG_RESULT(yes)],
  [AC_MSG_RESULT(no)
])
AC_MSG_CHECKING(for symlink)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <unistd.h>]], [[void *x=symlink]])],
  [AC_DEFINE(HAVE_SYMLINK, 1, Define if you have the 'symlink' function.)
   AC_MSG_RESULT(yes)],
  [AC_MSG_RESULT(no)
])
AC_MSG_CHECKING(for fchdir)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <unistd.h>]], [[void *x=fchdir]])],
  [AC_DEFINE(HAVE_FCHDIR, 1, Define if you have the 'fchdir' function.)
   AC_MSG_RESULT(yes)],
  [AC_MSG_RESULT(no)
])
AC_MSG_CHECKING(for fsync)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <unistd.h>]], [[void *x=fsync]])],
  [AC_DEFINE(HAVE_FSYNC, 1, Define if you have the 'fsync' function.)
   AC_MSG_RESULT(yes)],
  [AC_MSG_RESULT(no)
])
AC_MSG_CHECKING(for fdatasync)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <unistd.h>]], [[void *x=fdatasync]])],
  [AC_DEFINE(HAVE_FDATASYNC, 1, Define if you have the 'fdatasync' function.)
   AC_MSG_RESULT(yes)],
  [AC_MSG_RESULT(no)
])
AC_MSG_CHECKING(for epoll)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <sys/epoll.h>]], [[void *x=epoll_create]])],
  [AC_DEFINE(HAVE_EPOLL, 1, Define if you have the 'epoll' functions.)
   AC_MSG_RESULT(yes)],
  [AC_MSG_RESULT(no)
])
AC_MSG_CHECKING(for epoll_create1)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <sys/epoll.h>]], [[void *x=epoll_create1]])],
  [AC_DEFINE(HAVE_EPOLL_CREATE1, 1, Define if you have the 'epoll_create1' function.)
   AC_MSG_RESULT(yes)],
  [AC_MSG_RESULT(no)
])
AC_MSG_CHECKING(for kqueue)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <sys/types.h>
#include <sys/event.h>
    ]], [[int x=kqueue()]])],
  [AC_DEFINE(HAVE_KQUEUE, 1, Define if you have the 'kqueue' functions.)
   AC_MSG_RESULT(yes)],
  [AC_MSG_RESULT(no)
])
AC_MSG_CHECKING(for prlimit)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <sys/time.h>
#include <sys/resource.h>
    ]], [[void *x=prlimit]])],
  [AC_DEFINE(HAVE_PRLIMIT, 1, Define if you have the 'prlimit' functions.)
   AC_MSG_RESULT(yes)],
  [AC_MSG_RESULT(no)
])

# On some systems (eg. FreeBSD 5), we would find a definition of the
# functions ctermid_r, setgroups in the library, but no prototype
# (e.g. because we use _XOPEN_SOURCE). See whether we can take their
# address to avoid compiler warnings and potential miscompilations
# because of the missing prototypes.

AC_MSG_CHECKING(for ctermid_r)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <stdio.h>
]], [[void* p = ctermid_r]])],
  [AC_DEFINE(HAVE_CTERMID_R, 1, Define if you have the 'ctermid_r' function.)
   AC_MSG_RESULT(yes)],
  [AC_MSG_RESULT(no)
])

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
if test "x${ac_cv_flock_decl}" = xyes; then
  AC_CHECK_FUNCS(flock,,
    AC_CHECK_LIB(bsd,flock,
      [AC_DEFINE(HAVE_FLOCK)
       AC_DEFINE(FLOCK_NEEDS_LIBBSD, 1, Define if flock needs to be linked with bsd library.)
    ])
  )
fi

AC_MSG_CHECKING(for getpagesize)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <unistd.h>
]], [[void* p = getpagesize]])],
  [AC_DEFINE(HAVE_GETPAGESIZE, 1, Define if you have the 'getpagesize' function.)
   AC_MSG_RESULT(yes)],
  [AC_MSG_RESULT(no)
])

AC_MSG_CHECKING(for broken unsetenv)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <stdlib.h>
]], [[int res = unsetenv("DUMMY")]])],
  [AC_MSG_RESULT(no)],
  [AC_DEFINE(HAVE_BROKEN_UNSETENV, 1, Define if `unsetenv` does not return an int.)
   AC_MSG_RESULT(yes)
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
int main(int argc, char*argv[])
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
int main(int argc, char*argv[])
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

dnl Check if system zlib has *Copy() functions
dnl
dnl On MacOSX the linker will search for dylibs on the entire linker path
dnl before searching for static libraries. setup.py adds -Wl,-search_paths_first
dnl to revert to a more traditional unix behaviour and make it possible to
dnl override the system libz with a local static library of libz. Temporarily
dnl add that flag to our CFLAGS as well to ensure that we check the version
dnl of libz that will be used by setup.py.
dnl The -L/usr/local/lib is needed as wel to get the same compilation
dnl environment as setup.py (and leaving it out can cause configure to use the
dnl wrong version of the library)
case $ac_sys_system/$ac_sys_release in
Darwin/*)
	_CUR_CFLAGS="${CFLAGS}"
	_CUR_LDFLAGS="${LDFLAGS}"
	CFLAGS="${CFLAGS} -Wl,-search_paths_first"
	LDFLAGS="${LDFLAGS} -Wl,-search_paths_first -L/usr/local/lib"
	;;
esac

AC_CHECK_LIB(z, inflateCopy, AC_DEFINE(HAVE_ZLIB_COPY, 1, [Define if the zlib library has inflateCopy]))

case $ac_sys_system/$ac_sys_release in
Darwin/*)
	CFLAGS="${_CUR_CFLAGS}"
	LDFLAGS="${_CUR_LDFLAGS}"
	;;
esac

AC_MSG_CHECKING(for hstrerror)
AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#include <netdb.h>
]], [[void* p = hstrerror; hstrerror(0)]])],
  [AC_DEFINE(HAVE_HSTRERROR, 1, Define if you have the 'hstrerror' function.)
   AC_MSG_RESULT(yes)],
  [AC_MSG_RESULT(no)
])

AC_MSG_CHECKING(for inet_aton)
AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
]], [[void* p = inet_aton;inet_aton(0,0)]])],
  [AC_DEFINE(HAVE_INET_ATON, 1, Define if you have the 'inet_aton' function.)
   AC_MSG_RESULT(yes)],
  [AC_MSG_RESULT(no)
])

AC_MSG_CHECKING(for inet_pton)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
]], [[void* p = inet_pton]])],
  [AC_DEFINE(HAVE_INET_PTON, 1, Define if you have the 'inet_pton' function.)
   AC_MSG_RESULT(yes)],
  [AC_MSG_RESULT(no)
])

# On some systems, setgroups is in unistd.h, on others, in grp.h
AC_MSG_CHECKING(for setgroups)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <unistd.h>
#ifdef HAVE_GRP_H
#include <grp.h>
#endif
]], [[void* p = setgroups]])],
  [AC_DEFINE(HAVE_SETGROUPS, 1, Define if you have the 'setgroups' function.)
   AC_MSG_RESULT(yes)],
  [AC_MSG_RESULT(no)
])

# check for openpty and forkpty

AC_CHECK_FUNCS(openpty,,
   AC_CHECK_LIB(util,openpty,
     [AC_DEFINE(HAVE_OPENPTY) LIBS="$LIBS -lutil"],
     AC_CHECK_LIB(bsd,openpty, [AC_DEFINE(HAVE_OPENPTY) LIBS="$LIBS -lbsd"])
   )
)
AC_CHECK_FUNCS(forkpty,,
   AC_CHECK_LIB(util,forkpty,
     [AC_DEFINE(HAVE_FORKPTY) LIBS="$LIBS -lutil"],
     AC_CHECK_LIB(bsd,forkpty, [AC_DEFINE(HAVE_FORKPTY) LIBS="$LIBS -lbsd"])
   )
)

# check for long file support functions
AC_CHECK_FUNCS(fseek64 fseeko fstatvfs ftell64 ftello statvfs)

AC_REPLACE_FUNCS(dup2 strdup)
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
AC_CHECK_FUNCS(gettimeofday,
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <sys/time.h>]],
  				     [[gettimeofday((struct timeval*)0,(struct timezone*)0);]])],
    [],
    [AC_DEFINE(GETTIMEOFDAY_NO_TZ, 1,
      [Define if gettimeofday() does not have second (timezone) argument
       This is the case on Motorola V4 (R40V4.2)])
    ])
)

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

AC_MSG_CHECKING(for major, minor, and makedev)
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
]])],[
  AC_DEFINE(HAVE_DEVICE_MACROS, 1,
	    [Define to 1 if you have the device macros.])
  AC_MSG_RESULT(yes)
],[
  AC_MSG_RESULT(no)
])

# On OSF/1 V5.1, getaddrinfo is available, but a define
# for [no]getaddrinfo in netdb.h.
AC_MSG_CHECKING(for getaddrinfo)
AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
]], [[getaddrinfo(NULL, NULL, NULL, NULL);]])],
[have_getaddrinfo=yes],
[have_getaddrinfo=no])
AC_MSG_RESULT($have_getaddrinfo)
if test $have_getaddrinfo = yes
then
  AC_MSG_CHECKING(getaddrinfo bug)
  AC_CACHE_VAL(ac_cv_buggy_getaddrinfo,
  AC_RUN_IFELSE([AC_LANG_SOURCE([[[
#include <stdio.h>
#include <sys/types.h>
#include <netdb.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>

int main()
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
fi

AC_MSG_RESULT($ac_cv_buggy_getaddrinfo)

if test $have_getaddrinfo = no || test "$ac_cv_buggy_getaddrinfo" = yes
then
	if test $ipv6 = yes
	then
		echo 'Fatal: You must get working getaddrinfo() function.'
		echo '       or you can specify "--disable-ipv6"'.
		exit 1
	fi
else
	AC_DEFINE(HAVE_GETADDRINFO, 1, [Define if you have the getaddrinfo function.])
fi

AC_CHECK_FUNCS(getnameinfo)

# checks for structures
AC_HEADER_TIME
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

AC_MSG_CHECKING(for time.h that defines altzone)
AC_CACHE_VAL(ac_cv_header_time_altzone,[
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <time.h>]], [[return altzone;]])],
    [ac_cv_header_time_altzone=yes],
    [ac_cv_header_time_altzone=no])
  ])
AC_MSG_RESULT($ac_cv_header_time_altzone)
if test $ac_cv_header_time_altzone = yes; then
  AC_DEFINE(HAVE_ALTZONE, 1, [Define this if your time.h defines altzone.])
fi

was_it_defined=no
AC_MSG_CHECKING(whether sys/select.h and sys/time.h may both be included)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <sys/types.h>
#include <sys/select.h>
#include <sys/time.h>
]], [[;]])],[
  AC_DEFINE(SYS_SELECT_WITH_SYS_TIME, 1,
  [Define if  you can safely include both <sys/select.h> and <sys/time.h>
   (which you can't on SCO ODT 3.0).])
  was_it_defined=yes
],[])
AC_MSG_RESULT($was_it_defined)

AC_MSG_CHECKING(for addrinfo)
AC_CACHE_VAL(ac_cv_struct_addrinfo,
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <netdb.h>]], [[struct addrinfo a]])],
  [ac_cv_struct_addrinfo=yes],
  [ac_cv_struct_addrinfo=no]))
AC_MSG_RESULT($ac_cv_struct_addrinfo)
if test $ac_cv_struct_addrinfo = yes; then
	AC_DEFINE(HAVE_ADDRINFO, 1, [struct addrinfo (netdb.h)])
fi

AC_MSG_CHECKING(for sockaddr_storage)
AC_CACHE_VAL(ac_cv_struct_sockaddr_storage,
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#		include <sys/types.h>
#		include <sys/socket.h>]], [[struct sockaddr_storage s]])],
  [ac_cv_struct_sockaddr_storage=yes],
  [ac_cv_struct_sockaddr_storage=no]))
AC_MSG_RESULT($ac_cv_struct_sockaddr_storage)
if test $ac_cv_struct_sockaddr_storage = yes; then
	AC_DEFINE(HAVE_SOCKADDR_STORAGE, 1, [struct sockaddr_storage (sys/socket.h)])
fi

AC_MSG_CHECKING(for sockaddr_alg)
AC_CACHE_VAL(ac_cv_struct_sockaddr_alg,
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#		include <sys/types.h>
#		include <sys/socket.h>
#		include <linux/if_alg.h>]], [[struct sockaddr_alg s]])],
  [ac_cv_struct_sockaddr_alg=yes],
  [ac_cv_struct_sockaddr_alg=no]))
AC_MSG_RESULT($ac_cv_struct_sockaddr_alg)
if test $ac_cv_struct_sockaddr_alg = yes; then
	AC_DEFINE(HAVE_SOCKADDR_ALG, 1, [struct sockaddr_alg (linux/if_alg.h)])
fi

# checks for compiler characteristics

AC_C_CHAR_UNSIGNED
AC_C_CONST

works=no
AC_MSG_CHECKING(for working signed char)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[signed char c;]])],
  [works=yes],
  [AC_DEFINE(signed, , [Define to empty if the keyword does not work.])]
)
AC_MSG_RESULT($works)

have_prototypes=no
AC_MSG_CHECKING(for prototypes)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[int foo(int x) { return 0; }]], [[return foo(10);]])],
  [AC_DEFINE(HAVE_PROTOTYPES, 1,
     [Define if your compiler supports function prototype])
   have_prototypes=yes],
  []
)
AC_MSG_RESULT($have_prototypes)

works=no
AC_MSG_CHECKING(for variable length prototypes and stdarg.h)
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
]], [[return foo(10, "", 3.14);]])],[
  AC_DEFINE(HAVE_STDARG_PROTOTYPES, 1,
   [Define if your compiler supports variable length function prototypes
   (e.g. void fprintf(FILE *, char *, ...);) *and* <stdarg.h>])
  works=yes
],[])
AC_MSG_RESULT($works)

# check for socketpair
AC_MSG_CHECKING(for socketpair)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <sys/types.h>
#include <sys/socket.h>
]], [[void *x=socketpair]])],
  [AC_DEFINE(HAVE_SOCKETPAIR, 1, [Define if you have the 'socketpair' function.])
   AC_MSG_RESULT(yes)],
  [AC_MSG_RESULT(no)]
)

# check if sockaddr has sa_len member
AC_MSG_CHECKING(if sockaddr has sa_len member)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <sys/types.h>
#include <sys/socket.h>]], [[struct sockaddr x;
x.sa_len = 0;]])],
  [AC_MSG_RESULT(yes)
   AC_DEFINE(HAVE_SOCKADDR_SA_LEN, 1, [Define if sockaddr has sa_len member])],
  [AC_MSG_RESULT(no)]
)

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
            AS_HELP_STRING([--with-libm=STRING], [math library]),
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
            AS_HELP_STRING([--with-libc=STRING], [C library]),
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

AC_MSG_CHECKING(for x64 gcc inline assembler)
AC_LINK_IFELSE(   [AC_LANG_PROGRAM([[]], [[
  __asm__ __volatile__ ("movq %rcx, %rax");
]])],[have_gcc_asm_for_x64=yes],[have_gcc_asm_for_x64=no])
AC_MSG_RESULT($have_gcc_asm_for_x64)
if test "$have_gcc_asm_for_x64" = yes
then
    AC_DEFINE(HAVE_GCC_ASM_FOR_X64, 1,
    [Define if we can use x64 gcc inline assembler])
fi

# **************************************************
# * Check for various properties of floating point *
# **************************************************

AC_MSG_CHECKING(whether C doubles are little-endian IEEE 754 binary64)
AC_CACHE_VAL(ac_cv_little_endian_double, [
AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <string.h>
int main() {
    double x = 9006104071832581.0;
    if (memcmp(&x, "\x05\x04\x03\x02\x01\xff\x3f\x43", 8) == 0)
        return 0;
    else
        return 1;
}
]])],
[ac_cv_little_endian_double=yes],
[ac_cv_little_endian_double=no],
[ac_cv_little_endian_double=no])])
AC_MSG_RESULT($ac_cv_little_endian_double)
if test "$ac_cv_little_endian_double" = yes
then
  AC_DEFINE(DOUBLE_IS_LITTLE_ENDIAN_IEEE754, 1,
  [Define if C doubles are 64-bit IEEE 754 binary format, stored
   with the least significant byte first])
fi

AC_MSG_CHECKING(whether C doubles are big-endian IEEE 754 binary64)
AC_CACHE_VAL(ac_cv_big_endian_double, [
AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <string.h>
int main() {
    double x = 9006104071832581.0;
    if (memcmp(&x, "\x43\x3f\xff\x01\x02\x03\x04\x05", 8) == 0)
        return 0;
    else
        return 1;
}
]])],
[ac_cv_big_endian_double=yes],
[ac_cv_big_endian_double=no],
[ac_cv_big_endian_double=no])])
AC_MSG_RESULT($ac_cv_big_endian_double)
if test "$ac_cv_big_endian_double" = yes
then
  AC_DEFINE(DOUBLE_IS_BIG_ENDIAN_IEEE754, 1,
  [Define if C doubles are 64-bit IEEE 754 binary format, stored
   with the most significant byte first])
fi

# Some ARM platforms use a mixed-endian representation for doubles.
# While Python doesn't currently have full support for these platforms
# (see e.g., issue 1762561), we can at least make sure that float <-> string
# conversions work.
AC_MSG_CHECKING(whether C doubles are ARM mixed-endian IEEE 754 binary64)
AC_CACHE_VAL(ac_cv_mixed_endian_double, [
AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <string.h>
int main() {
    double x = 9006104071832581.0;
    if (memcmp(&x, "\x01\xff\x3f\x43\x05\x04\x03\x02", 8) == 0)
        return 0;
    else
        return 1;
}
]])],
[ac_cv_mixed_endian_double=yes],
[ac_cv_mixed_endian_double=no],
[ac_cv_mixed_endian_double=no])])
AC_MSG_RESULT($ac_cv_mixed_endian_double)
if test "$ac_cv_mixed_endian_double" = yes
then
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

AC_MSG_CHECKING(whether we can use gcc inline assembler to get and set x87 control word)
AC_LINK_IFELSE(   [AC_LANG_PROGRAM([[]], [[
  unsigned short cw;
  __asm__ __volatile__ ("fnstcw %0" : "=m" (cw));
  __asm__ __volatile__ ("fldcw %0" : : "m" (cw));
]])],[have_gcc_asm_for_x87=yes],[have_gcc_asm_for_x87=no])
AC_MSG_RESULT($have_gcc_asm_for_x87)
if test "$have_gcc_asm_for_x87" = yes
then
    AC_DEFINE(HAVE_GCC_ASM_FOR_X87, 1,
    [Define if we can use gcc inline assembler to get and set x87 control word])
fi

AC_MSG_CHECKING(whether we can use gcc inline assembler to get and set mc68881 fpcr)
AC_LINK_IFELSE(   [AC_LANG_PROGRAM([[]], [[
  unsigned int fpcr;
  __asm__ __volatile__ ("fmove.l %%fpcr,%0" : "=g" (fpcr));
  __asm__ __volatile__ ("fmove.l %0,%%fpcr" : : "g" (fpcr));
]])],[have_gcc_asm_for_mc68881=yes],[have_gcc_asm_for_mc68881=no])
AC_MSG_RESULT($have_gcc_asm_for_mc68881)
if test "$have_gcc_asm_for_mc68881" = yes
then
    AC_DEFINE(HAVE_GCC_ASM_FOR_MC68881, 1,
    [Define if we can use gcc inline assembler to get and set mc68881 fpcr])
fi

# Detect whether system arithmetic is subject to x87-style double
# rounding issues.  The result of this test has little meaning on non
# IEEE 754 platforms.  On IEEE 754, test should return 1 if rounding
# mode is round-to-nearest and double rounding issues are present, and
# 0 otherwise.  See http://bugs.python.org/issue2937 for more info.
AC_MSG_CHECKING(for x87-style double rounding)
# $BASECFLAGS may affect the result
ac_save_cc="$CC"
CC="$CC $BASECFLAGS"
AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <stdlib.h>
#include <math.h>
int main() {
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
AC_MSG_RESULT($ac_cv_x87_double_rounding)
if test "$ac_cv_x87_double_rounding" = yes
then
  AC_DEFINE(X87_DOUBLE_ROUNDING, 1,
  [Define if arithmetic is subject to x87-style double rounding issue])
fi

# ************************************
# * Check for mathematical functions *
# ************************************

LIBS_SAVE=$LIBS
LIBS="$LIBS $LIBM"

AC_CHECK_FUNCS([acosh asinh atanh copysign erf erfc expm1 finite gamma])
AC_CHECK_FUNCS([hypot lgamma log1p log2 round tgamma])
AC_CHECK_DECLS([isinf, isnan, isfinite], [], [], [[#include <math.h>]])

# On FreeBSD 6.2, it appears that tanh(-0.) returns 0. instead of
# -0. on some architectures.
AC_MSG_CHECKING(whether tanh preserves the sign of zero)
AC_CACHE_VAL(ac_cv_tanh_preserves_zero_sign, [
AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <math.h>
#include <stdlib.h>
int main() {
    /* return 0 if either negative zeros don't exist
       on this platform or if negative zeros exist
       and tanh(-0.) == -0. */
  if (atan2(0., -1.) == atan2(-0., -1.) ||
      atan2(tanh(-0.), -1.) == atan2(-0., -1.)) exit(0);
  else exit(1);
}
]])],
[ac_cv_tanh_preserves_zero_sign=yes],
[ac_cv_tanh_preserves_zero_sign=no],
[ac_cv_tanh_preserves_zero_sign=no])])
AC_MSG_RESULT($ac_cv_tanh_preserves_zero_sign)
if test "$ac_cv_tanh_preserves_zero_sign" = yes
then
  AC_DEFINE(TANH_PRESERVES_ZERO_SIGN, 1,
  [Define if tanh(-0.) is -0., or if platform doesn't have signed zeros])
fi

if test "$ac_cv_func_log1p" = yes
then
    # On some versions of AIX, log1p(-0.) returns 0. instead of
    # -0.  See issue #9920.
    AC_MSG_CHECKING(whether log1p drops the sign of negative zero)
    AC_CACHE_VAL(ac_cv_log1p_drops_zero_sign, [
    AC_RUN_IFELSE([AC_LANG_SOURCE([[
    #include <math.h>
    #include <stdlib.h>
    int main() {
        /* Fail if the signs of log1p(-0.) and -0. can be
	   distinguished. */
        if (atan2(log1p(-0.), -1.) == atan2(-0., -1.))
            return 0;
        else
            return 1;
    }
    ]])],
    [ac_cv_log1p_drops_zero_sign=no],
    [ac_cv_log1p_drops_zero_sign=yes],
    [ac_cv_log1p_drops_zero_sign=no])])
    AC_MSG_RESULT($ac_cv_log1p_drops_zero_sign)
fi
if test "$ac_cv_log1p_drops_zero_sign" = yes
then
  AC_DEFINE(LOG1P_DROPS_ZERO_SIGN, 1,
  [Define if log1p(-0.) is 0. rather than -0.])
fi

LIBS=$LIBS_SAVE

# For multiprocessing module, check that sem_open
# actually works.  For FreeBSD versions <= 7.2,
# the kernel module that provides POSIX semaphores
# isn't loaded by default, so an attempt to call
# sem_open results in a 'Signal 12' error.
AC_MSG_CHECKING(whether POSIX semaphores are enabled)
AC_CACHE_VAL(ac_cv_posix_semaphores_enabled,
AC_RUN_IFELSE([AC_LANG_SOURCE([[
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
]])],
[ac_cv_posix_semaphores_enabled=yes],
[ac_cv_posix_semaphores_enabled=no],
[ac_cv_posix_semaphores_enabled=yes])
)
AC_MSG_RESULT($ac_cv_posix_semaphores_enabled)
if test $ac_cv_posix_semaphores_enabled = no
then
  AC_DEFINE(POSIX_SEMAPHORES_NOT_ENABLED, 1,
            [Define if POSIX semaphores aren't enabled on your system])
fi

# Multiprocessing check for broken sem_getvalue
AC_MSG_CHECKING(for broken sem_getvalue)
AC_CACHE_VAL(ac_cv_broken_sem_getvalue,
AC_RUN_IFELSE([AC_LANG_SOURCE([[
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
]])],
[ac_cv_broken_sem_getvalue=no],
[ac_cv_broken_sem_getvalue=yes],
[ac_cv_broken_sem_getvalue=yes])
)
AC_MSG_RESULT($ac_cv_broken_sem_getvalue)
if test $ac_cv_broken_sem_getvalue = yes
then
  AC_DEFINE(HAVE_BROKEN_SEM_GETVALUE, 1,
  [define to 1 if your sem_getvalue is broken.])
fi

AC_CHECK_DECLS([RTLD_LAZY, RTLD_NOW, RTLD_GLOBAL, RTLD_LOCAL, RTLD_NODELETE, RTLD_NOLOAD, RTLD_DEEPBIND, RTLD_MEMBER], [], [], [[#include <dlfcn.h>]])

# determine what size digit to use for Python's longs
AC_MSG_CHECKING([digit size for Python's longs])
AC_ARG_ENABLE(big-digits,
AS_HELP_STRING([--enable-big-digits@<:@=BITS@:>@],[use big digits for Python longs [[BITS=30]]]),
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

AC_MSG_CHECKING(for UCS-4 tcl)
have_ucs4_tcl=no
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <tcl.h>
#if TCL_UTF_MAX != 6
# error "NOT UCS4_TCL"
#endif]], [[]])],[
  AC_DEFINE(HAVE_UCS4_TCL, 1, [Define this if you have tcl and TCL_UTF_MAX==6])
  have_ucs4_tcl=yes
],[])
AC_MSG_RESULT($have_ucs4_tcl)

# check whether wchar_t is signed or not
if test "$wchar_h" = yes
then
  # check whether wchar_t is signed or not
  AC_MSG_CHECKING(whether wchar_t is signed)
  AC_CACHE_VAL(ac_cv_wchar_t_signed, [
  AC_RUN_IFELSE([AC_LANG_SOURCE([[
  #include <wchar.h>
  int main()
  {
	/* Success: exit code 0 */
        exit((((wchar_t) -1) < ((wchar_t) 0)) ? 0 : 1);
  }
  ]])],
  [ac_cv_wchar_t_signed=yes],
  [ac_cv_wchar_t_signed=no],
  [ac_cv_wchar_t_signed=yes])])
  AC_MSG_RESULT($ac_cv_wchar_t_signed)
fi

# wchar_t is only usable if it maps to an unsigned type
if test "$ac_cv_sizeof_wchar_t" -ge 2 \
          -a "$ac_cv_wchar_t_signed" = "no"
then
  HAVE_USABLE_WCHAR_T="yes"
  AC_DEFINE(HAVE_USABLE_WCHAR_T, 1,
  [Define if you have a useable wchar_t type defined in wchar.h; useable
   means wchar_t must be an unsigned type with at least 16 bits. (see
   Include/unicodeobject.h).])
else
  HAVE_USABLE_WCHAR_T="no usable wchar_t found"
fi
AC_MSG_RESULT($HAVE_USABLE_WCHAR_T)

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
# * --with-pymalloc (adds a 'm')
# * --with-wide-unicode (adds a 'u')
#
# Thus for example, Python 3.2 built with wide unicode, pydebug, and pymalloc,
# would get a shared library ABI version tag of 'cpython-32dmu' and shared
# libraries would be named 'foo.cpython-32dmu.so'.
AC_SUBST(SOABI)
AC_MSG_CHECKING(ABIFLAGS)
AC_MSG_RESULT($ABIFLAGS)
AC_MSG_CHECKING(SOABI)
SOABI='cpython-'`echo $VERSION | tr -d .`${ABIFLAGS}${PLATFORM_TRIPLET:+-$PLATFORM_TRIPLET}
AC_MSG_RESULT($SOABI)

AC_SUBST(EXT_SUFFIX)
case $ac_sys_system in
    Linux*|GNU*|Darwin)
	EXT_SUFFIX=.${SOABI}${SHLIB_SUFFIX};;
    *)
	EXT_SUFFIX=${SHLIB_SUFFIX};;
esac

AC_MSG_CHECKING(LDVERSION)
LDVERSION='$(VERSION)$(ABIFLAGS)'
AC_MSG_RESULT($LDVERSION)

dnl define LIBPL after ABIFLAGS and LDVERSION is defined.
AC_SUBST(PY_ENABLE_SHARED)
if test x$PLATFORM_TRIPLET = x; then
  LIBPL='$(prefix)'"/lib/python${VERSION}/config-${LDVERSION}"
else
  LIBPL='$(prefix)'"/lib/python${VERSION}/config-${LDVERSION}-${PLATFORM_TRIPLET}"
fi
AC_SUBST(LIBPL)

# Check whether right shifting a negative integer extends the sign bit
# or fills with zeros (like the Cray J90, according to Tim Peters).
AC_MSG_CHECKING(whether right shift extends the sign bit)
AC_CACHE_VAL(ac_cv_rshift_extends_sign, [
AC_RUN_IFELSE([AC_LANG_SOURCE([[
int main()
{
	exit(((-1)>>3 == -1) ? 0 : 1);
}
]])],
[ac_cv_rshift_extends_sign=yes],
[ac_cv_rshift_extends_sign=no],
[ac_cv_rshift_extends_sign=yes])])
AC_MSG_RESULT($ac_cv_rshift_extends_sign)
if test "$ac_cv_rshift_extends_sign" = no
then
  AC_DEFINE(SIGNED_RIGHT_SHIFT_ZERO_FILLS, 1,
  [Define if i>>j for signed int i does not extend the sign bit
   when i < 0])
fi

# check for getc_unlocked and related locking functions
AC_MSG_CHECKING(for getc_unlocked() and friends)
AC_CACHE_VAL(ac_cv_have_getc_unlocked, [
AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <stdio.h>]], [[
	FILE *f = fopen("/dev/null", "r");
	flockfile(f);
	getc_unlocked(f);
	funlockfile(f);
]])],[ac_cv_have_getc_unlocked=yes],[ac_cv_have_getc_unlocked=no])])
AC_MSG_RESULT($ac_cv_have_getc_unlocked)
if test "$ac_cv_have_getc_unlocked" = yes
then
  AC_DEFINE(HAVE_GETC_UNLOCKED, 1,
  [Define this if you have flockfile(), getc_unlocked(), and funlockfile()])
fi

# check where readline lives
# save the value of LIBS so we don't actually link Python with readline
LIBS_no_readline=$LIBS

# On some systems we need to link readline to a termcap compatible
# library.  NOTE: Keep the precedence of listed libraries synchronised
# with setup.py.
py_cv_lib_readline=no
AC_MSG_CHECKING([how to link readline libs])
for py_libtermcap in "" tinfo ncursesw ncurses curses termcap; do
  if test -z "$py_libtermcap"; then
    READLINE_LIBS="-lreadline"
  else
    READLINE_LIBS="-lreadline -l$py_libtermcap"
  fi
  LIBS="$READLINE_LIBS $LIBS_no_readline"
  AC_LINK_IFELSE(
    [AC_LANG_CALL([],[readline])],
    [py_cv_lib_readline=yes])
  if test $py_cv_lib_readline = yes; then
    break
  fi
done
# Uncomment this line if you want to use READINE_LIBS in Makefile or scripts
#AC_SUBST([READLINE_LIBS])
if test $py_cv_lib_readline = no; then
  AC_MSG_RESULT([none])
else
  AC_MSG_RESULT([$READLINE_LIBS])
  AC_DEFINE(HAVE_LIBREADLINE, 1,
    [Define if you have the readline library (-lreadline).])
fi

# check for readline 2.2
AC_PREPROC_IFELSE([AC_LANG_SOURCE([[#include <readline/readline.h>]])],
  [have_readline=yes],
  [have_readline=no]
)
if test $have_readline = yes
then
  AC_EGREP_HEADER([extern int rl_completion_append_character;],
  [readline/readline.h],
  AC_DEFINE(HAVE_RL_COMPLETION_APPEND_CHARACTER, 1,
  [Define if you have readline 2.2]), )
  AC_EGREP_HEADER([extern int rl_completion_suppress_append;],
  [readline/readline.h],
  AC_DEFINE(HAVE_RL_COMPLETION_SUPPRESS_APPEND, 1,
  [Define if you have rl_completion_suppress_append]), )
fi

# check for readline 4.0
AC_CHECK_LIB(readline, rl_pre_input_hook,
	AC_DEFINE(HAVE_RL_PRE_INPUT_HOOK, 1,
        [Define if you have readline 4.0]), ,$READLINE_LIBS)

# also in 4.0
AC_CHECK_LIB(readline, rl_completion_display_matches_hook,
	AC_DEFINE(HAVE_RL_COMPLETION_DISPLAY_MATCHES_HOOK, 1,
        [Define if you have readline 4.0]), ,$READLINE_LIBS)

# also in 4.0, but not in editline
AC_CHECK_LIB(readline, rl_resize_terminal,
	AC_DEFINE(HAVE_RL_RESIZE_TERMINAL, 1,
        [Define if you have readline 4.0]), ,$READLINE_LIBS)

# check for readline 4.2
AC_CHECK_LIB(readline, rl_completion_matches,
	AC_DEFINE(HAVE_RL_COMPLETION_MATCHES, 1,
        [Define if you have readline 4.2]), ,$READLINE_LIBS)

# also in readline 4.2
AC_PREPROC_IFELSE([AC_LANG_SOURCE([[#include <readline/readline.h>]])],
  [have_readline=yes],
  [have_readline=no]
)
if test $have_readline = yes
then
  AC_EGREP_HEADER([extern int rl_catch_signals;],
  [readline/readline.h],
  AC_DEFINE(HAVE_RL_CATCH_SIGNAL, 1,
  [Define if you can turn off readline's signal handling.]), )
fi

AC_CHECK_LIB(readline, append_history,
	AC_DEFINE(HAVE_RL_APPEND_HISTORY, 1,
        [Define if readline supports append_history]), ,$READLINE_LIBS)

# End of readline checks: restore LIBS
LIBS=$LIBS_no_readline

AC_MSG_CHECKING(for broken nice())
AC_CACHE_VAL(ac_cv_broken_nice, [
AC_RUN_IFELSE([AC_LANG_SOURCE([[
int main()
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
AC_MSG_RESULT($ac_cv_broken_nice)
if test "$ac_cv_broken_nice" = yes
then
  AC_DEFINE(HAVE_BROKEN_NICE, 1,
  [Define if nice() returns success/failure instead of the new priority.])
fi

AC_MSG_CHECKING(for broken poll())
AC_CACHE_VAL(ac_cv_broken_poll,
AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <poll.h>

int main()
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
AC_MSG_RESULT($ac_cv_broken_poll)
if test "$ac_cv_broken_poll" = yes
then
  AC_DEFINE(HAVE_BROKEN_POLL, 1,
      [Define if poll() sets errno on invalid file descriptors.])
fi

# Before we can test tzset, we need to check if struct tm has a tm_zone
# (which is not required by ISO C or UNIX spec) and/or if we support
# tzname[]
AC_STRUCT_TIMEZONE

# check tzset(3) exists and works like we expect it to
AC_MSG_CHECKING(for working tzset())
AC_CACHE_VAL(ac_cv_working_tzset, [
AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <stdlib.h>
#include <time.h>
#include <string.h>

#if HAVE_TZNAME
extern char *tzname[];
#endif

int main()
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
AC_MSG_RESULT($ac_cv_working_tzset)
if test "$ac_cv_working_tzset" = yes
then
  AC_DEFINE(HAVE_WORKING_TZSET, 1,
  [Define if tzset() actually switches the local timezone in a meaningful way.])
fi

# Look for subsecond timestamps in struct stat
AC_MSG_CHECKING(for tv_nsec in struct stat)
AC_CACHE_VAL(ac_cv_stat_tv_nsec,
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <sys/stat.h>]], [[
struct stat st;
st.st_mtim.tv_nsec = 1;
]])],
[ac_cv_stat_tv_nsec=yes],
[ac_cv_stat_tv_nsec=no]))
AC_MSG_RESULT($ac_cv_stat_tv_nsec)
if test "$ac_cv_stat_tv_nsec" = yes
then
  AC_DEFINE(HAVE_STAT_TV_NSEC, 1,
  [Define if you have struct stat.st_mtim.tv_nsec])
fi

# Look for BSD style subsecond timestamps in struct stat
AC_MSG_CHECKING(for tv_nsec2 in struct stat)
AC_CACHE_VAL(ac_cv_stat_tv_nsec2,
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <sys/stat.h>]], [[
struct stat st;
st.st_mtimespec.tv_nsec = 1;
]])],
[ac_cv_stat_tv_nsec2=yes],
[ac_cv_stat_tv_nsec2=no]))
AC_MSG_RESULT($ac_cv_stat_tv_nsec2)
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
AC_MSG_CHECKING(whether mvwdelch is an expression)
AC_CACHE_VAL(ac_cv_mvwdelch_is_expression,
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <curses.h>]], [[
  int rtn;
  rtn = mvwdelch(0,0,0);
]])],
[ac_cv_mvwdelch_is_expression=yes],
[ac_cv_mvwdelch_is_expression=no]))
AC_MSG_RESULT($ac_cv_mvwdelch_is_expression)

if test "$ac_cv_mvwdelch_is_expression" = yes
then
  AC_DEFINE(MVWDELCH_IS_EXPRESSION, 1,
  [Define if mvwdelch in curses.h is an expression.])
fi

# Issue #25720: ncurses has introduced the NCURSES_OPAQUE symbol making opaque
# structs since version 5.7.  If the macro is defined as zero before including
# [n]curses.h, ncurses will expose fields of the structs regardless of the
# configuration.
AC_MSG_CHECKING(whether WINDOW has _flags)
AC_CACHE_VAL(ac_cv_window_has_flags,
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
  #define NCURSES_OPAQUE 0
  #include <curses.h>
]], [[
  WINDOW *w;
  w->_flags = 0;
]])],
[ac_cv_window_has_flags=yes],
[ac_cv_window_has_flags=no]))
AC_MSG_RESULT($ac_cv_window_has_flags)


if test "$ac_cv_window_has_flags" = yes
then
  AC_DEFINE(WINDOW_HAS_FLAGS, 1,
  [Define if WINDOW in curses.h offers a field _flags.])
fi

AC_MSG_CHECKING(for is_pad)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <curses.h>]], [[
#ifndef is_pad
void *x=is_pad
#endif
]])],
  [AC_DEFINE(HAVE_CURSES_IS_PAD, 1, Define if you have the 'is_pad' function or macro.)
   AC_MSG_RESULT(yes)],
  [AC_MSG_RESULT(no)]
)

AC_MSG_CHECKING(for is_term_resized)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <curses.h>]], [[void *x=is_term_resized]])],
  [AC_DEFINE(HAVE_CURSES_IS_TERM_RESIZED, 1, Define if you have the 'is_term_resized' function.)
   AC_MSG_RESULT(yes)],
  [AC_MSG_RESULT(no)]
)

AC_MSG_CHECKING(for resize_term)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <curses.h>]], [[void *x=resize_term]])],
  [AC_DEFINE(HAVE_CURSES_RESIZE_TERM, 1, Define if you have the 'resize_term' function.)
   AC_MSG_RESULT(yes)],
  [AC_MSG_RESULT(no)]
)

AC_MSG_CHECKING(for resizeterm)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <curses.h>]], [[void *x=resizeterm]])],
  [AC_DEFINE(HAVE_CURSES_RESIZETERM, 1, Define if you have the 'resizeterm' function.)
   AC_MSG_RESULT(yes)],
  [AC_MSG_RESULT(no)]
)

AC_MSG_CHECKING(for immedok)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <curses.h>]], [[
#ifndef immedok
void *x=immedok
#endif
]])],
  [AC_DEFINE(HAVE_CURSES_IMMEDOK, 1, Define if you have the 'immedok' function.)
   AC_MSG_RESULT(yes)],
  [AC_MSG_RESULT(no)]
)

AC_MSG_CHECKING(for syncok)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <curses.h>]], [[
#ifndef syncok
void *x=syncok
#endif
]])],
  [AC_DEFINE(HAVE_CURSES_SYNCOK, 1, Define if you have the 'syncok' function.)
   AC_MSG_RESULT(yes)],
  [AC_MSG_RESULT(no)]
)

AC_MSG_CHECKING(for wchgat)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <curses.h>]], [[
#ifndef wchgat
void *x=wchgat
#endif
]])],
  [AC_DEFINE(HAVE_CURSES_WCHGAT, 1, Define if you have the 'wchgat' function.)
   AC_MSG_RESULT(yes)],
  [AC_MSG_RESULT(no)]
)

AC_MSG_CHECKING(for filter)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <curses.h>]], [[
#ifndef filter
void *x=filter
#endif
]])],
  [AC_DEFINE(HAVE_CURSES_FILTER, 1, Define if you have the 'filter' function.)
   AC_MSG_RESULT(yes)],
  [AC_MSG_RESULT(no)]
)

AC_MSG_CHECKING(for has_key)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <curses.h>]], [[
#ifndef has_key
void *x=has_key
#endif
]])],
  [AC_DEFINE(HAVE_CURSES_HAS_KEY, 1, Define if you have the 'has_key' function.)
   AC_MSG_RESULT(yes)],
  [AC_MSG_RESULT(no)]
)

AC_MSG_CHECKING(for typeahead)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <curses.h>]], [[
#ifndef typeahead
void *x=typeahead
#endif
]])],
  [AC_DEFINE(HAVE_CURSES_TYPEAHEAD, 1, Define if you have the 'typeahead' function.)
   AC_MSG_RESULT(yes)],
  [AC_MSG_RESULT(no)]
)

AC_MSG_CHECKING(for use_env)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <curses.h>]], [[
#ifndef use_env
void *x=use_env
#endif
]])],
  [AC_DEFINE(HAVE_CURSES_USE_ENV, 1, Define if you have the 'use_env' function.)
   AC_MSG_RESULT(yes)],
  [AC_MSG_RESULT(no)]
)
# last curses configure check
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

AC_MSG_CHECKING(for broken mbstowcs)
AC_CACHE_VAL(ac_cv_broken_mbstowcs,
AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <stdio.h>
#include<stdlib.h>
int main() {
    size_t len = -1;
    const char *str = "text";
    len = mbstowcs(NULL, str, 0);
    return (len != 4);
}
]])],
[ac_cv_broken_mbstowcs=no],
[ac_cv_broken_mbstowcs=yes],
[ac_cv_broken_mbstowcs=no]))
AC_MSG_RESULT($ac_cv_broken_mbstowcs)
if test "$ac_cv_broken_mbstowcs" = yes
then
  AC_DEFINE(HAVE_BROKEN_MBSTOWCS, 1,
  [Define if mbstowcs(NULL, "text", 0) does not return the number of
   wide chars that would be converted.])
fi

# Check for --with-computed-gotos
AC_MSG_CHECKING(for --with-computed-gotos)
AC_ARG_WITH(computed-gotos,
            AS_HELP_STRING([--with(out)-computed-gotos],
                           [Use computed gotos in evaluation loop (enabled by default on supported compilers)]),
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

AC_MSG_CHECKING(whether $CC supports computed gotos)
AC_CACHE_VAL(ac_cv_computed_gotos,
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
AC_MSG_RESULT($ac_cv_computed_gotos)
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
SRCDIRS="Parser Objects Python Modules Programs"
AC_MSG_CHECKING(for build directories)
for dir in $SRCDIRS; do
    if test ! -d $dir; then
        mkdir $dir
    fi
done
AC_MSG_RESULT(done)

# Availability of -O2:
AC_MSG_CHECKING(for -O2)
saved_cflags="$CFLAGS"
CFLAGS="-O2"
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[
]])],[have_O2=yes],[have_O2=no])
AC_MSG_RESULT($have_O2)
CFLAGS="$saved_cflags"

# _FORTIFY_SOURCE wrappers for memmove and bcopy are incorrect:
# http://sourceware.org/ml/libc-alpha/2010-12/msg00009.html
AC_MSG_CHECKING(for glibc _FORTIFY_SOURCE/memmove bug)
saved_cflags="$CFLAGS"
CFLAGS="-O2 -D_FORTIFY_SOURCE=2"
if test "$have_O2" = no; then
    CFLAGS=""
fi
AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
void foo(void *p, void *q) { memmove(p, q, 19); }
int main() {
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

if test "$have_gcc_asm_for_x87" = yes; then
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
            int main() {
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
AC_MSG_CHECKING(for stdatomic.h)
AC_LINK_IFELSE(
[
  AC_LANG_SOURCE([[
    #include <stdatomic.h>
    atomic_int value = ATOMIC_VAR_INIT(1);
    _Atomic void *py_atomic_address = (void*) &value;
    int main() {
      int loaded_value = atomic_load(&value);
      return 0;
    }
  ]])
],[have_stdatomic_h=yes],[have_stdatomic_h=no])

AC_MSG_RESULT($have_stdatomic_h)

if test "$have_stdatomic_h" = yes; then
    AC_DEFINE(HAVE_STD_ATOMIC, 1,
              [Has stdatomic.h, atomic_int and _Atomic void* types work])
fi

# Check for GCC >= 4.7 __atomic builtins
AC_MSG_CHECKING(for GCC >= 4.7 __atomic builtins)
AC_LINK_IFELSE(
[
  AC_LANG_SOURCE([[
    volatile int val = 1;
    int main() {
      __atomic_load_n(&val, __ATOMIC_SEQ_CST);
      return 0;
    }
  ]])
],[have_builtin_atomic=yes],[have_builtin_atomic=no])

AC_MSG_RESULT($have_builtin_atomic)

if test "$have_builtin_atomic" = yes; then
    AC_DEFINE(HAVE_BUILTIN_ATOMIC, 1, [Has builtin atomics])
fi

# ensurepip option
AC_MSG_CHECKING(for ensurepip)
AC_ARG_WITH(ensurepip,
    [AS_HELP_STRING([--with(out)-ensurepip=@<:@=upgrade@:>@],
        ["install" or "upgrade" using bundled pip])],
    [],
    [with_ensurepip=upgrade])
AS_CASE($with_ensurepip,
    [yes|upgrade],[ENSUREPIP=upgrade],
    [install],[ENSUREPIP=install],
    [no],[ENSUREPIP=no],
    [AC_MSG_ERROR([--with-ensurepip=upgrade|install|no])])
AC_MSG_RESULT($ENSUREPIP)
AC_SUBST(ENSUREPIP)

# check if the dirent structure of a d_type field and DT_UNKNOWN is defined
AC_MSG_CHECKING(if the dirent structure of a d_type field)
AC_LINK_IFELSE(
[
  AC_LANG_SOURCE([[
    #include <dirent.h>

    int main() {
      struct dirent entry;
      return entry.d_type == DT_UNKNOWN;
    }
  ]])
],[have_dirent_d_type=yes],[have_dirent_d_type=no])
AC_MSG_RESULT($have_dirent_d_type)

if test "$have_dirent_d_type" = yes; then
    AC_DEFINE(HAVE_DIRENT_D_TYPE, 1,
              [Define to 1 if the dirent structure has a d_type field])
fi

# check if the Linux getrandom() syscall is available
AC_MSG_CHECKING(for the Linux getrandom() syscall)
AC_LINK_IFELSE(
[
  AC_LANG_SOURCE([[
    #include <unistd.h>
    #include <sys/syscall.h>
    #include <linux/random.h>

    int main() {
        char buffer[1];
        const size_t buflen = sizeof(buffer);
        const int flags = GRND_NONBLOCK;
        /* ignore the result, Python checks for ENOSYS and EAGAIN at runtime */
        (void)syscall(SYS_getrandom, buffer, buflen, flags);
        return 0;
    }
  ]])
],[have_getrandom_syscall=yes],[have_getrandom_syscall=no])
AC_MSG_RESULT($have_getrandom_syscall)

if test "$have_getrandom_syscall" = yes; then
    AC_DEFINE(HAVE_GETRANDOM_SYSCALL, 1,
              [Define to 1 if the Linux getrandom() syscall is available])
fi

# check if the getrandom() function is available
# the test was written for the Solaris function of <sys/random.h>
AC_MSG_CHECKING(for the getrandom() function)
AC_LINK_IFELSE(
[
  AC_LANG_SOURCE([[
    #include <sys/random.h>

    int main() {
        char buffer[1];
        const size_t buflen = sizeof(buffer);
        const int flags = 0;
        /* ignore the result, Python checks for ENOSYS at runtime */
        (void)getrandom(buffer, buflen, flags);
        return 0;
    }
  ]])
],[have_getrandom=yes],[have_getrandom=no])
AC_MSG_RESULT($have_getrandom)

if test "$have_getrandom" = yes; then
    AC_DEFINE(HAVE_GETRANDOM, 1,
              [Define to 1 if the getrandom() function is available])
fi

# generate output files
AC_CONFIG_FILES(Makefile.pre Misc/python.pc Misc/python-config.sh)
AC_CONFIG_FILES([Modules/ld_so_aix], [chmod +x Modules/ld_so_aix])
AC_OUTPUT

echo "creating Modules/Setup" >&AS_MESSAGE_FD
if test ! -f Modules/Setup
then
	cp $srcdir/Modules/Setup.dist Modules/Setup
fi

echo "creating Modules/Setup.local" >&AS_MESSAGE_FD
if test ! -f Modules/Setup.local
then
	echo "# Edit this file for local setup changes" >Modules/Setup.local
fi

echo "creating Makefile" >&AS_MESSAGE_FD
$SHELL $srcdir/Modules/makesetup -c $srcdir/Modules/config.c.in \
			-s Modules \
			Modules/Setup.local Modules/Setup
mv config.c Modules

if test "$Py_OPT" = 'false' -a "$Py_DEBUG" != 'true'; then
    echo "" >&AS_MESSAGE_FD
    echo "" >&AS_MESSAGE_FD
    echo "If you want a release build with all stable optimizations active (PGO, etc)," >&AS_MESSAGE_FD
    echo "please run ./configure --enable-optimizations" >&AS_MESSAGE_FD
    echo "" >&AS_MESSAGE_FD
    echo "" >&AS_MESSAGE_FD
fi

