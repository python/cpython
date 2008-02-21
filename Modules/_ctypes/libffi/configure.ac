dnl Process this with autoconf to create configure

AC_PREREQ(2.59)

AC_INIT([libffi], [2.1], [http://gcc.gnu.org/bugs.html])
AC_CONFIG_HEADERS([fficonfig.h])

AC_CANONICAL_SYSTEM
target_alias=${target_alias-$host_alias}

m4_rename([_AC_ARG_VAR_PRECIOUS],[real_PRECIOUS])
m4_define([_AC_ARG_VAR_PRECIOUS],[])
AC_PROG_CC
m4_rename([real_PRECIOUS],[_AC_ARG_VAR_PRECIOUS])

AC_SUBST(CFLAGS)

AC_CHECK_HEADERS(sys/mman.h)
AC_CHECK_FUNCS(mmap)
AC_FUNC_MMAP_BLACKLIST

TARGETDIR="unknown"
case "$host" in
x86_64-*-openbsd*) TARGET=X86_64; TARGETDIR=x86;;
mips*-*-openbsd*) TARGET=MIPS; TARGETDIR=mips;;
sparc-*-openbsd*) TARGET=SPARC; TARGETDIR=sparc;;
sparc64-*-openbsd*) TARGET=SPARC; TARGETDIR=sparc;;
alpha*-*-openbsd*) TARGET=ALPHA; TARGETDIR=alpha;;
m68k-*-openbsd*) TARGET=M68K; TARGETDIR=m68k;;
powerpc-*-openbsd*) TARGET=POWERPC; TARGETDIR=powerpc;;
i*86-*-darwin*) TARGET=X86_DARWIN; TARGETDIR=x86;;
i*86-*-linux*) TARGET=X86; TARGETDIR=x86;;
i*86-*-gnu*) TARGET=X86; TARGETDIR=x86;;
i*86-*-solaris2.1[[0-9]]*) TARGET=X86_64; TARGETDIR=x86;;
i*86-*-solaris*) TARGET=X86; TARGETDIR=x86;;
i*86-*-beos*) TARGET=X86; TARGETDIR=x86;;
i*86-*-freebsd* | i*86-*-kfreebsd*-gnu) TARGET=X86; TARGETDIR=x86;;
i*86-*-netbsdelf* | i*86-*-knetbsd*-gnu) TARGET=X86; TARGETDIR=x86;;
i*86-*-openbsd*) TARGET=X86; TARGETDIR=x86;;
i*86-*-rtems*) TARGET=X86; TARGETDIR=x86;;
i*86-*-win32*) TARGET=X86_WIN32; TARGETDIR=x86;;
i*86-*-cygwin*) TARGET=X86_WIN32; TARGETDIR=x86;;
i*86-*-mingw*) TARGET=X86_WIN32; TARGETDIR=x86;;
frv-*-*) TARGET=FRV; TARGETDIR=frv;;
sparc-sun-4*) TARGET=SPARC; TARGETDIR=sparc;;
sparc*-sun-*) TARGET=SPARC; TARGETDIR=sparc;;
sparc-*-linux* | sparc-*-netbsdelf* | sparc-*-knetbsd*-gnu) TARGET=SPARC; TARGETDIR=sparc;;
sparc*-*-rtems*) TARGET=SPARC; TARGETDIR=sparc;;
sparc64-*-linux* | sparc64-*-freebsd* | sparc64-*-netbsd* | sparc64-*-knetbsd*-gnu) TARGET=SPARC; TARGETDIR=sparc;;
alpha*-*-linux* | alpha*-*-osf* | alpha*-*-freebsd* | alpha*-*-kfreebsd*-gnu | alpha*-*-netbsd* | alpha*-*-knetbsd*-gnu) TARGET=ALPHA; TARGETDIR=alpha;;
ia64*-*-*) TARGET=IA64; TARGETDIR=ia64;;
m32r*-*-linux* ) TARGET=M32R; TARGETDIR=m32r;;
m68k-*-linux*) TARGET=M68K; TARGETDIR=m68k;;
mips64*-*);;
mips-sgi-irix5.* | mips-sgi-irix6.*) TARGET=MIPS_IRIX; TARGETDIR=mips;;
mips*-*-linux*) TARGET=MIPS_LINUX; TARGETDIR=mips;;
powerpc*-*-linux* | powerpc-*-sysv*) TARGET=POWERPC; TARGETDIR=powerpc;;
powerpc-*-beos*) TARGET=POWERPC; TARGETDIR=powerpc;;
powerpc-*-darwin*) TARGET=POWERPC_DARWIN; TARGETDIR=powerpc;;
powerpc-*-aix*) TARGET=POWERPC_AIX; TARGETDIR=powerpc;;
powerpc-*-freebsd*) TARGET=POWERPC_FREEBSD; TARGETDIR=powerpc;;
powerpc*-*-rtems*) TARGET=POWERPC; TARGETDIR=powerpc;;
rs6000-*-aix*) TARGET=POWERPC_AIX; TARGETDIR=powerpc;;
arm*-*-linux-*) TARGET=ARM; TARGETDIR=arm;;
arm*-*-netbsdelf* | arm*-*-knetbsd*-gnu) TARGET=ARM; TARGETDIR=arm;;
arm*-*-rtems*) TARGET=ARM; TARGETDIR=arm;;
cris-*-*) TARGET=LIBFFI_CRIS; TARGETDIR=cris;;
s390-*-linux-*) TARGET=S390; TARGETDIR=s390;;
s390x-*-linux-*) TARGET=S390; TARGETDIR=s390;;
amd64-*-freebsd* | x86_64-*-linux* | x86_64-*-freebsd* | x86_64-*-kfreebsd*-gnu) TARGET=X86_64; TARGETDIR=x86;;
sh-*-linux* | sh[[34]]*-*-linux*) TARGET=SH; TARGETDIR=sh;;
sh-*-rtems*) TARGET=SH; TARGETDIR=sh;;
sh64-*-linux* | sh5*-*-linux*) TARGET=SH64; TARGETDIR=sh64;;
hppa*-*-linux* | parisc*-*-linux*) TARGET=PA; TARGETDIR=pa;;
esac

if test $TARGETDIR = unknown; then
  AC_MSG_ERROR(["libffi has not been ported to $host."])
fi

dnl  libffi changes TARGET for MIPS to define a such macro in the header
dnl  while MIPS_IRIX or MIPS_LINUX is separatedly used to decide which
dnl  files will be compiled.  So, we need to keep the original decision
dnl  of TARGET to use in fficonfig.py.in.
MKTARGET=$TARGET

case x$TARGET in
  xMIPS*) TARGET=MIPS ;;
  *) ;;
esac

AC_HEADER_STDC
AC_CHECK_FUNCS(memcpy)
AC_FUNC_ALLOCA

AC_CHECK_SIZEOF(double)
AC_CHECK_SIZEOF(long double)

# Also AC_SUBST this variable for ffi.h.
HAVE_LONG_DOUBLE=0
if test $ac_cv_sizeof_double != $ac_cv_sizeof_long_double; then
  if test $ac_cv_sizeof_long_double != 0; then
    HAVE_LONG_DOUBLE=1
    AC_DEFINE(HAVE_LONG_DOUBLE, 1, [Define if you have the long double type and it is bigger than a double])
  fi
fi
AC_SUBST(HAVE_LONG_DOUBLE)

AC_C_BIGENDIAN
AH_VERBATIM([WORDS_BIGENDIAN],
[
/* Define to 1 if your processor stores words with the most significant byte
   first (like Motorola and SPARC, unlike Intel and VAX). 

   The block below does compile-time checking for endianness on platforms
   that use GCC and therefore allows compiling fat binaries on OSX by using
   '-arch ppc -arch i386' as the compile flags. The phrasing was choosen
   such that the configure-result is used on systems that don't use GCC.
*/
#ifdef __BIG_ENDIAN__
#define WORDS_BIGENDIAN 1
#else
#ifndef __LITTLE_ENDIAN__
#undef WORDS_BIGENDIAN
#endif
#endif])


if test x$TARGET = xSPARC; then
    AC_CACHE_CHECK([assembler and linker support unaligned pc related relocs],
	libffi_cv_as_sparc_ua_pcrel, [
	save_CFLAGS="$CFLAGS"
	save_LDFLAGS="$LDFLAGS"
	CFLAGS="$CFLAGS -fpic"
	LDFLAGS="$LDFLAGS -shared"
	AC_TRY_LINK([asm (".text; foo: nop; .data; .align 4; .byte 0; .uaword %r_disp32(foo); .text");],,
		    [libffi_cv_as_sparc_ua_pcrel=yes],
		    [libffi_cv_as_sparc_ua_pcrel=no])
	CFLAGS="$save_CFLAGS"
	LDFLAGS="$save_LDFLAGS"])
    if test "x$libffi_cv_as_sparc_ua_pcrel" = xyes; then
	AC_DEFINE(HAVE_AS_SPARC_UA_PCREL, 1,
		  [Define if your assembler and linker support unaligned PC relative relocs.])
    fi

    AC_CACHE_CHECK([assembler .register pseudo-op support],
       libffi_cv_as_register_pseudo_op, [
       libffi_cv_as_register_pseudo_op=unknown
       # Check if we have .register
       AC_TRY_COMPILE([asm (".register %g2, #scratch");],,
		       [libffi_cv_as_register_pseudo_op=yes],
		       [libffi_cv_as_register_pseudo_op=no])
    ])
    if test "x$libffi_cv_as_register_pseudo_op" = xyes; then
       AC_DEFINE(HAVE_AS_REGISTER_PSEUDO_OP, 1,
	       [Define if your assembler supports .register.])
    fi
fi

AC_CACHE_CHECK([whether .eh_frame section should be read-only],
    libffi_cv_ro_eh_frame, [
	libffi_cv_ro_eh_frame=no
	echo 'extern void foo (void); void bar (void) { foo (); foo (); }' > conftest.c
	if $CC $CFLAGS -S -fpic -fexceptions -o conftest.s conftest.c > /dev/null 2>&1; then
	    if grep '.section.*eh_frame.*"a"' conftest.s > /dev/null; then
		libffi_cv_ro_eh_frame=yes
	    elif grep '.section.*eh_frame.*#alloc' conftest.c \
		 | grep -v '#write' > /dev/null; then
		libffi_cv_ro_eh_frame=yes
	    fi
	fi
	rm -f conftest.*
    ])
if test "x$libffi_cv_ro_eh_frame" = xyes; then
    AC_DEFINE(HAVE_RO_EH_FRAME, 1,
	      [Define if .eh_frame sections should be read-only.])
    AC_DEFINE(EH_FRAME_FLAGS, "a",
	      [Define to the flags needed for the .section .eh_frame directive.])
else
    AC_DEFINE(EH_FRAME_FLAGS, "aw",
	      [Define to the flags needed for the .section .eh_frame directive.])
fi

AC_CACHE_CHECK([for __attribute__((visibility("hidden")))],
    libffi_cv_hidden_visibility_attribute, [
	echo 'int __attribute__ ((visibility ("hidden"))) foo (void) { return 1; }' > conftest.c
	libffi_cv_hidden_visibility_attribute=no
	if AC_TRY_COMMAND(${CC-cc} -Werror -S conftest.c -o conftest.s 1>&AS_MESSAGE_LOG_FD); then
	    if grep '\.hidden.*foo' conftest.s >/dev/null; then
		libffi_cv_hidden_visibility_attribute=yes
	    fi
	fi
	rm -f conftest.*
    ])
if test $libffi_cv_hidden_visibility_attribute = yes; then
    AC_DEFINE(HAVE_HIDDEN_VISIBILITY_ATTRIBUTE, 1,
	      [Define if __attribute__((visibility("hidden"))) is supported.])
fi

AH_BOTTOM([
#ifdef HAVE_HIDDEN_VISIBILITY_ATTRIBUTE
#ifdef LIBFFI_ASM
#define FFI_HIDDEN(name) .hidden name
#else
#define FFI_HIDDEN __attribute__ ((visibility ("hidden")))
#endif
#else
#ifdef LIBFFI_ASM
#define FFI_HIDDEN(name)
#else
#define FFI_HIDDEN
#endif
#endif
])

AC_SUBST(TARGET)
AC_SUBST(TARGETDIR)
AC_SUBST(MKTARGET)

AC_SUBST(SHELL)

AC_DEFINE(FFI_NO_RAW_API, 1, [Define this is you do not want support for the raw API.])

AC_CONFIG_COMMANDS(include, [test -d include || mkdir include])
AC_CONFIG_COMMANDS(src, [
test -d src || mkdir src
test -d src/$TARGETDIR || mkdir src/$TARGETDIR
], [TARGETDIR="$TARGETDIR"])

TARGETINCDIR=$TARGETDIR
case $host in
*-*-darwin*)
   TARGETINCDIR="darwin"
   ;;
esac


AC_CONFIG_LINKS(include/ffitarget.h:src/$TARGETINCDIR/ffitarget.h)
AC_CONFIG_LINKS(include/ffi_common.h:include/ffi_common.h)

AC_CONFIG_FILES(include/ffi.h fficonfig.py)

AC_OUTPUT
