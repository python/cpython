#
# SYNOPSIS
#
#   TUKLIB_PHYSMEM
#
# DESCRIPTION
#
#   Check how to get the amount of physical memory.
#   This information is used in tuklib_physmem.c.
#
#   Supported methods:
#
#     - Windows (including Cygwin), OS/2, DJGPP (DOS), OpenVMS, AROS,
#       and QNX have operating-system specific functions.
#
#     - AIX has _system_configuration.physmem.
#
#     - sysconf() works on GNU/Linux and Solaris, and possibly on
#       some BSDs.
#
#     - BSDs use sysctl().
#
#     - Tru64 uses getsysinfo().
#
#     - HP-UX uses pstat_getstatic().
#
#     - IRIX has setinvent_r(), getinvent_r(), and endinvent_r().
#
#     - sysinfo() works on Linux/dietlibc and probably on other Linux
#       systems whose libc may lack sysconf().
#
# COPYING
#
#   Author: Lasse Collin
#
#   This file has been put into the public domain.
#   You can do whatever you want with this file.
#

AC_DEFUN_ONCE([TUKLIB_PHYSMEM], [
AC_REQUIRE([TUKLIB_COMMON])

# sys/param.h might be needed by sys/sysctl.h.
AC_CHECK_HEADERS([sys/param.h])

AC_CACHE_CHECK([how to detect the amount of physical memory],
	[tuklib_cv_physmem_method], [

# Maybe checking $host_os would be enough but this matches what
# tuklib_physmem.c does.
#
# NOTE: IRIX has a compiler that doesn't error out with #error, so use
# a non-compilable text instead of #error to generate an error.
AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
#if defined(_WIN32) || defined(__CYGWIN__) || defined(__OS2__) \
		|| defined(__DJGPP__) || defined(__VMS) \
		|| defined(AMIGA) || defined(__AROS__) || defined(__QNX__)
int main(void) { return 0; }
#else
compile error
#endif
]])], [tuklib_cv_physmem_method=special], [

# Look for AIX-specific solution before sysconf(), because the test
# for sysconf() will pass on AIX but won't actually work
# (sysconf(_SC_PHYS_PAGES) compiles but always returns -1 on AIX).
AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
#include <sys/systemcfg.h>

int
main(void)
{
	(void)_system_configuration.physmem;
	return 0;
}
]])], [tuklib_cv_physmem_method=aix], [

AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
#include <unistd.h>
int
main(void)
{
	long i;
	i = sysconf(_SC_PAGESIZE);
	i = sysconf(_SC_PHYS_PAGES);
	return 0;
}
]])], [tuklib_cv_physmem_method=sysconf], [

AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
#include <sys/types.h>
#ifdef HAVE_SYS_PARAM_H
#	include <sys/param.h>
#endif
#include <sys/sysctl.h>
int
main(void)
{
	int name[2] = { CTL_HW, HW_PHYSMEM };
	unsigned long mem;
	size_t mem_ptr_size = sizeof(mem);
	sysctl(name, 2, &mem, &mem_ptr_size, NULL, 0);
	return 0;
}
]])], [tuklib_cv_physmem_method=sysctl], [

AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
#include <sys/sysinfo.h>
#include <machine/hal_sysinfo.h>

int
main(void)
{
	int memkb;
	int start = 0;
	getsysinfo(GSI_PHYSMEM, (caddr_t)&memkb, sizeof(memkb), &start);
	return 0;
}
]])], [tuklib_cv_physmem_method=getsysinfo],[

AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
#include <sys/param.h>
#include <sys/pstat.h>

int
main(void)
{
	struct pst_static pst;
	pstat_getstatic(&pst, sizeof(pst), 1, 0);
	(void)pst.physical_memory;
	(void)pst.page_size;
	return 0;
}
]])], [tuklib_cv_physmem_method=pstat_getstatic],[

AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
#include <invent.h>
int
main(void)
{
	inv_state_t *st = NULL;
	setinvent_r(&st);
	getinvent_r(st);
	endinvent_r(st);
	return 0;
}
]])], [tuklib_cv_physmem_method=getinvent_r], [

# This version of sysinfo() is Linux-specific. Some non-Linux systems have
# different sysinfo() so we must check $host_os.
case $host_os in
	linux*)
		AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
#include <sys/sysinfo.h>
int
main(void)
{
	struct sysinfo si;
	sysinfo(&si);
	return 0;
}
		]])], [
			tuklib_cv_physmem_method=sysinfo
		], [
			tuklib_cv_physmem_method=unknown
		])
		;;
	*)
		tuklib_cv_physmem_method=unknown
		;;
esac
])])])])])])])])

case $tuklib_cv_physmem_method in
	aix)
		AC_DEFINE([TUKLIB_PHYSMEM_AIX], [1],
			[Define to 1 if the amount of physical memory
			can be detected with _system_configuration.physmem.])
		;;
	sysconf)
		AC_DEFINE([TUKLIB_PHYSMEM_SYSCONF], [1],
			[Define to 1 if the amount of physical memory can
			be detected with sysconf(_SC_PAGESIZE) and
			sysconf(_SC_PHYS_PAGES).])
		;;
	sysctl)
		AC_DEFINE([TUKLIB_PHYSMEM_SYSCTL], [1],
			[Define to 1 if the amount of physical memory can
			be detected with sysctl().])
		;;
	getsysinfo)
		AC_DEFINE([TUKLIB_PHYSMEM_GETSYSINFO], [1],
			[Define to 1 if the amount of physical memory can
			be detected with getsysinfo().])
		;;
	pstat_getstatic)
		AC_DEFINE([TUKLIB_PHYSMEM_PSTAT_GETSTATIC], [1],
			[Define to 1 if the amount of physical memory can
			be detected with pstat_getstatic().])
		;;
	getinvent_r)
		AC_DEFINE([TUKLIB_PHYSMEM_GETINVENT_R], [1],
			[Define to 1 if the amount of physical memory
			can be detected with getinvent_r().])
		;;
	sysinfo)
		AC_DEFINE([TUKLIB_PHYSMEM_SYSINFO], [1],
			[Define to 1 if the amount of physical memory
			can be detected with Linux sysinfo().])
		;;
esac
])dnl
