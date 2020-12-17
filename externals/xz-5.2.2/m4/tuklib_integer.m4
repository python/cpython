#
# SYNOPSIS
#
#   TUKLIB_INTEGER
#
# DESCRIPTION
#
#   Checks for tuklib_integer.h:
#     - Endianness
#     - Does the compiler or the operating system provide byte swapping macros
#     - Does the hardware support fast unaligned access to 16-bit
#       and 32-bit integers
#
# COPYING
#
#   Author: Lasse Collin
#
#   This file has been put into the public domain.
#   You can do whatever you want with this file.
#

AC_DEFUN_ONCE([TUKLIB_INTEGER], [
AC_REQUIRE([TUKLIB_COMMON])
AC_REQUIRE([AC_C_BIGENDIAN])

AC_MSG_CHECKING([if __builtin_bswap16/32/64 are supported])
AC_LINK_IFELSE([AC_LANG_PROGRAM([[]],
			[[__builtin_bswap16(1);
			__builtin_bswap32(1);
			__builtin_bswap64(1);]])],
[
	AC_DEFINE([HAVE___BUILTIN_BSWAPXX], [1],
		[Define to 1 if the GNU C extensions
		__builtin_bswap16/32/64 are supported.])
	AC_MSG_RESULT([yes])
], [
	AC_MSG_RESULT([no])

	# Look for other byteswapping methods.
	AC_CHECK_HEADERS([byteswap.h sys/endian.h sys/byteorder.h], [break])

	# Even if we have byteswap.h we may lack the specific macros/functions.
	if test x$ac_cv_header_byteswap_h = xyes ; then
		m4_foreach([FUNC], [bswap_16,bswap_32,bswap_64], [
			AC_MSG_CHECKING([if FUNC is available])
			AC_LINK_IFELSE([AC_LANG_SOURCE([
#include <byteswap.h>
int
main(void)
{
	FUNC[](42);
	return 0;
}
			])], [
				AC_DEFINE(HAVE_[]m4_toupper(FUNC), [1],
					[Define to 1 if] FUNC [is available.])
				AC_MSG_RESULT([yes])
			], [AC_MSG_RESULT([no])])

		])dnl
	fi
])

AC_MSG_CHECKING([if unaligned memory access should be used])
AC_ARG_ENABLE([unaligned-access], AS_HELP_STRING([--enable-unaligned-access],
		[Enable if the system supports *fast* unaligned memory access
		with 16-bit and 32-bit integers. By default, this is enabled
		only on x86, x86_64, big endian PowerPC,
		and some ARM systems.]),
	[], [enable_unaligned_access=auto])
if test "x$enable_unaligned_access" = xauto ; then
	# TODO: There may be other architectures, on which unaligned access
	# is OK.
	case $host_cpu in
		i?86|x86_64|powerpc|powerpc64)
			enable_unaligned_access=yes
			;;
		arm*|aarch64*)
			# On 32-bit and 64-bit ARM, GCC and Clang
			# #define __ARM_FEATURE_UNALIGNED if
			# unaligned access is supported.
			AC_COMPILE_IFELSE([AC_LANG_SOURCE([
#ifndef __ARM_FEATURE_UNALIGNED
compile error
#endif
int main(void) { return 0; }
])], [enable_unaligned_access=yes], [enable_unaligned_access=no])
			;;
		*)
			enable_unaligned_access=no
			;;
	esac
fi
if test "x$enable_unaligned_access" = xyes ; then
	AC_DEFINE([TUKLIB_FAST_UNALIGNED_ACCESS], [1], [Define to 1 if
		the system supports fast unaligned access to 16-bit and
		32-bit integers.])
	AC_MSG_RESULT([yes])
else
	AC_MSG_RESULT([no])
fi

AC_MSG_CHECKING([if unsafe type punning should be used])
AC_ARG_ENABLE([unsafe-type-punning],
	AS_HELP_STRING([--enable-unsafe-type-punning],
		[This introduces strict aliasing violations and may result
		in broken code. However, this might improve performance in
		some cases, especially with old compilers (e.g.
		GCC 3 and early 4.x on x86, GCC < 6 on ARMv6 and ARMv7).]),
	[], [enable_unsafe_type_punning=no])
if test "x$enable_unsafe_type_punning" = xyes ; then
	AC_DEFINE([TUKLIB_USE_UNSAFE_TYPE_PUNNING], [1], [Define to 1 to use
		unsafe type punning, e.g. char *x = ...; *(int *)x = 123;
		which violates strict aliasing rules and thus is
		undefined behavior and might result in broken code.])
	AC_MSG_RESULT([yes])
else
	AC_MSG_RESULT([no])
fi

AC_MSG_CHECKING([if __builtin_assume_aligned is supported])
AC_LINK_IFELSE([AC_LANG_PROGRAM([[]], [[__builtin_assume_aligned("", 1);]])],
	[
		AC_DEFINE([HAVE___BUILTIN_ASSUME_ALIGNED], [1],
			[Define to 1 if the GNU C extension
			__builtin_assume_aligned is supported.])
		AC_MSG_RESULT([yes])
	], [
		AC_MSG_RESULT([no])
	])
])dnl
