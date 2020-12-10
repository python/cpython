#
# SYNOPSIS
#
#   TUKLIB_CPUCORES
#
# DESCRIPTION
#
#   Check how to find out the number of available CPU cores in the system.
#   This information is used by tuklib_cpucores.c.
#
#   Supported methods:
#     - GetSystemInfo(): Windows (including Cygwin)
#     - sysctl(): BSDs, OS/2
#     - sysconf(): GNU/Linux, Solaris, Tru64, IRIX, AIX, QNX, Cygwin (but
#       GetSystemInfo() is used on Cygwin)
#     - pstat_getdynamic(): HP-UX
#
# COPYING
#
#   Author: Lasse Collin
#
#   This file has been put into the public domain.
#   You can do whatever you want with this file.
#

AC_DEFUN_ONCE([TUKLIB_CPUCORES], [
AC_REQUIRE([TUKLIB_COMMON])

# sys/param.h might be needed by sys/sysctl.h.
AC_CHECK_HEADERS([sys/param.h])

AC_CACHE_CHECK([how to detect the number of available CPU cores],
	[tuklib_cv_cpucores_method], [

# Maybe checking $host_os would be enough but this matches what
# tuklib_cpucores.c does.
#
# NOTE: IRIX has a compiler that doesn't error out with #error, so use
# a non-compilable text instead of #error to generate an error.
AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
#if defined(_WIN32) || defined(__CYGWIN__)
int main(void) { return 0; }
#else
compile error
#endif
]])], [tuklib_cv_cpucores_method=special], [

# FreeBSD has both cpuset and sysctl. Look for cpuset first because
# it's a better approach.
AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
#include <sys/param.h>
#include <sys/cpuset.h>

int
main(void)
{
	cpuset_t set;
	cpuset_getaffinity(CPU_LEVEL_WHICH, CPU_WHICH_PID, -1,
			sizeof(set), &set);
	return 0;
}
]])], [tuklib_cv_cpucores_method=cpuset], [

# On OS/2, both sysconf() and sysctl() pass the tests in this file,
# but only sysctl() works. On QNX it's the opposite: only sysconf() works
# (although it assumes that _POSIX_SOURCE, _XOPEN_SOURCE, and _POSIX_C_SOURCE
# are undefined or alternatively _QNX_SOURCE is defined).
#
# We test sysctl() first and intentionally break the sysctl() test on QNX
# so that sysctl() is never used on QNX.
AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
#ifdef __QNX__
compile error
#endif
#include <sys/types.h>
#ifdef HAVE_SYS_PARAM_H
#	include <sys/param.h>
#endif
#include <sys/sysctl.h>
int
main(void)
{
	int name[2] = { CTL_HW, HW_NCPU };
	int cpus;
	size_t cpus_size = sizeof(cpus);
	sysctl(name, 2, &cpus, &cpus_size, NULL, 0);
	return 0;
}
]])], [tuklib_cv_cpucores_method=sysctl], [

AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
#include <unistd.h>
int
main(void)
{
	long i;
#ifdef _SC_NPROCESSORS_ONLN
	/* Many systems using sysconf() */
	i = sysconf(_SC_NPROCESSORS_ONLN);
#else
	/* IRIX */
	i = sysconf(_SC_NPROC_ONLN);
#endif
	return 0;
}
]])], [tuklib_cv_cpucores_method=sysconf], [

AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
#include <sys/param.h>
#include <sys/pstat.h>

int
main(void)
{
	struct pst_dynamic pst;
	pstat_getdynamic(&pst, sizeof(pst), 1, 0);
	(void)pst.psd_proc_cnt;
	return 0;
}
]])], [tuklib_cv_cpucores_method=pstat_getdynamic], [

	tuklib_cv_cpucores_method=unknown
])])])])])])

case $tuklib_cv_cpucores_method in
	cpuset)
		AC_DEFINE([TUKLIB_CPUCORES_CPUSET], [1],
			[Define to 1 if the number of available CPU cores
			can be detected with cpuset(2).])
		;;
	sysctl)
		AC_DEFINE([TUKLIB_CPUCORES_SYSCTL], [1],
			[Define to 1 if the number of available CPU cores
			can be detected with sysctl().])
		;;
	sysconf)
		AC_DEFINE([TUKLIB_CPUCORES_SYSCONF], [1],
			[Define to 1 if the number of available CPU cores
			can be detected with sysconf(_SC_NPROCESSORS_ONLN)
			or sysconf(_SC_NPROC_ONLN).])
		;;
	pstat_getdynamic)
		AC_DEFINE([TUKLIB_CPUCORES_PSTAT_GETDYNAMIC], [1],
			[Define to 1 if the number of available CPU cores
			can be detected with pstat_getdynamic().])
		;;
esac
])dnl
