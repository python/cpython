#!/bin/sh
# configure script for zlib.
#
# Normally configure builds both a static and a shared library.
# If you want to build just a static library, use: ./configure --static
#
# To impose specific compiler or flags or install directory, use for example:
#    prefix=$HOME CC=cc CFLAGS="-O4" ./configure
# or for csh/tcsh users:
#    (setenv prefix $HOME; setenv CC cc; setenv CFLAGS "-O4"; ./configure)

# Incorrect settings of CC or CFLAGS may prevent creating a shared library.
# If you have problems, try without defining CC and CFLAGS before reporting
# an error.

if [ -n "${CHOST}" ]; then
    uname="$(echo "${CHOST}" | sed -e 's/^[^-]*-\([^-]*\)$/\1/' -e 's/^[^-]*-[^-]*-\([^-]*\)$/\1/' -e 's/^[^-]*-[^-]*-\([^-]*\)-.*$/\1/')"
    CROSS_PREFIX="${CHOST}-"
fi

STATICLIB=libz.a
LDFLAGS="${LDFLAGS} -L. ${STATICLIB}"
VER=`sed -n -e '/VERSION "/s/.*"\(.*\)".*/\1/p' < zlib.h`
VER3=`sed -n -e '/VERSION "/s/.*"\([0-9]*\\.[0-9]*\\.[0-9]*\).*/\1/p' < zlib.h`
VER2=`sed -n -e '/VERSION "/s/.*"\([0-9]*\\.[0-9]*\)\\..*/\1/p' < zlib.h`
VER1=`sed -n -e '/VERSION "/s/.*"\([0-9]*\)\\..*/\1/p' < zlib.h`
if "${CROSS_PREFIX}ar" --version >/dev/null 2>/dev/null || test $? -lt 126; then
    AR=${AR-"${CROSS_PREFIX}ar"}
    test -n "${CROSS_PREFIX}" && echo Using ${AR}
else
    AR=${AR-"ar"}
    test -n "${CROSS_PREFIX}" && echo Using ${AR}
fi
AR_RC="${AR} rc"
if "${CROSS_PREFIX}ranlib" --version >/dev/null 2>/dev/null || test $? -lt 126; then
    RANLIB=${RANLIB-"${CROSS_PREFIX}ranlib"}
    test -n "${CROSS_PREFIX}" && echo Using ${RANLIB}
else
    RANLIB=${RANLIB-"ranlib"}
fi
if "${CROSS_PREFIX}nm" --version >/dev/null 2>/dev/null || test $? -lt 126; then
    NM=${NM-"${CROSS_PREFIX}nm"}
    test -n "${CROSS_PREFIX}" && echo Using ${NM}
else
    NM=${NM-"nm"}
fi
LDCONFIG=${LDCONFIG-"ldconfig"}
LDSHAREDLIBC="${LDSHAREDLIBC--lc}"
prefix=${prefix-/usr/local}
exec_prefix=${exec_prefix-'${prefix}'}
libdir=${libdir-'${exec_prefix}/lib'}
sharedlibdir=${sharedlibdir-'${libdir}'}
includedir=${includedir-'${prefix}/include'}
mandir=${mandir-'${prefix}/share/man'}
shared_ext='.so'
shared=1
zprefix=0
build64=0
gcc=0
old_cc="$CC"
old_cflags="$CFLAGS"

while test $# -ge 1
do
case "$1" in
    -h* | --help)
      echo 'usage:'
      echo '  configure [--zprefix] [--prefix=PREFIX]  [--eprefix=EXPREFIX]'
      echo '    [--static] [--64] [--libdir=LIBDIR] [--sharedlibdir=LIBDIR]'
      echo '    [--includedir=INCLUDEDIR]'
        exit 0 ;;
    -p*=* | --prefix=*) prefix=`echo $1 | sed 's/.*=//'`; shift ;;
    -e*=* | --eprefix=*) exec_prefix=`echo $1 | sed 's/.*=//'`; shift ;;
    -l*=* | --libdir=*) libdir=`echo $1 | sed 's/.*=//'`; shift ;;
    --sharedlibdir=*) sharedlibdir=`echo $1 | sed 's/.*=//'`; shift ;;
    -i*=* | --includedir=*) includedir=`echo $1 | sed 's/.*=//'`;shift ;;
    -u*=* | --uname=*) uname=`echo $1 | sed 's/.*=//'`;shift ;;
    -p* | --prefix) prefix="$2"; shift; shift ;;
    -e* | --eprefix) exec_prefix="$2"; shift; shift ;;
    -l* | --libdir) libdir="$2"; shift; shift ;;
    -i* | --includedir) includedir="$2"; shift; shift ;;
    -s* | --shared | --enable-shared) shared=1; shift ;;
    -t | --static) shared=0; shift ;;
    -z* | --zprefix) zprefix=1; shift ;;
    -6* | --64) build64=1; shift ;;
    --sysconfdir=*) echo "ignored option: --sysconfdir"; shift ;;
    --localstatedir=*) echo "ignored option: --localstatedir"; shift ;;
    *) echo "unknown option: $1"; echo "$0 --help for help"; exit 1 ;;
    esac
done

test=ztest$$
cat > $test.c <<EOF
extern int getchar();
int hello() {return getchar();}
EOF

test -z "$CC" && echo Checking for ${CROSS_PREFIX}gcc...
cc=${CC-${CROSS_PREFIX}gcc}
cflags=${CFLAGS-"-O3"}
# to force the asm version use: CFLAGS="-O3 -DASMV" ./configure
case "$cc" in
  *gcc*) gcc=1 ;;
esac

if test "$gcc" -eq 1 && ($cc -c $cflags $test.c) 2>/dev/null; then
  CC="$cc"
  SFLAGS="${CFLAGS--O3} -fPIC"
  CFLAGS="${CFLAGS--O3}"
  if test $build64 -eq 1; then
    CFLAGS="${CFLAGS} -m64"
    SFLAGS="${SFLAGS} -m64"
  fi
  if test "${ZLIBGCCWARN}" = "YES"; then
    CFLAGS="${CFLAGS} -Wall -Wextra -pedantic"
  fi
  if test -z "$uname"; then
    uname=`(uname -s || echo unknown) 2>/dev/null`
  fi
  case "$uname" in
  Linux* | linux* | GNU | GNU/* | *BSD | DragonFly) LDSHARED=${LDSHARED-"$cc -shared -Wl,-soname,libz.so.1,--version-script,zlib.map"} ;;
  CYGWIN* | Cygwin* | cygwin* | OS/2*)
        EXE='.exe' ;;
  MINGW*|mingw*)
# temporary bypass
        rm -f $test.[co] $test $test$shared_ext
        echo "Please use win32/Makefile.gcc instead."
        exit 1
        LDSHARED=${LDSHARED-"$cc -shared"}
        LDSHAREDLIBC=""
        EXE='.exe' ;;
  QNX*)  # This is for QNX6. I suppose that the QNX rule below is for QNX2,QNX4
         # (alain.bonnefoy@icbt.com)
                 LDSHARED=${LDSHARED-"$cc -shared -Wl,-hlibz.so.1"} ;;
  HP-UX*)
         LDSHARED=${LDSHARED-"$cc -shared $SFLAGS"}
         case `(uname -m || echo unknown) 2>/dev/null` in
         ia64)
                 shared_ext='.so'
                 SHAREDLIB='libz.so' ;;
         *)
                 shared_ext='.sl'
                 SHAREDLIB='libz.sl' ;;
         esac ;;
  Darwin*)   shared_ext='.dylib'
             SHAREDLIB=libz$shared_ext
             SHAREDLIBV=libz.$VER$shared_ext
             SHAREDLIBM=libz.$VER1$shared_ext
             LDSHARED=${LDSHARED-"$cc -dynamiclib -install_name $libdir/$SHAREDLIBM -compatibility_version $VER1 -current_version $VER3"} ;;
  *)             LDSHARED=${LDSHARED-"$cc -shared"} ;;
  esac
else
  # find system name and corresponding cc options
  CC=${CC-cc}
  gcc=0
  if test -z "$uname"; then
    uname=`(uname -sr || echo unknown) 2>/dev/null`
  fi
  case "$uname" in
  HP-UX*)    SFLAGS=${CFLAGS-"-O +z"}
             CFLAGS=${CFLAGS-"-O"}
#            LDSHARED=${LDSHARED-"ld -b +vnocompatwarnings"}
             LDSHARED=${LDSHARED-"ld -b"}
         case `(uname -m || echo unknown) 2>/dev/null` in
         ia64)
             shared_ext='.so'
             SHAREDLIB='libz.so' ;;
         *)
             shared_ext='.sl'
             SHAREDLIB='libz.sl' ;;
         esac ;;
  IRIX*)     SFLAGS=${CFLAGS-"-ansi -O2 -rpath ."}
             CFLAGS=${CFLAGS-"-ansi -O2"}
             LDSHARED=${LDSHARED-"cc -shared -Wl,-soname,libz.so.1"} ;;
  OSF1\ V4*) SFLAGS=${CFLAGS-"-O -std1"}
             CFLAGS=${CFLAGS-"-O -std1"}
             LDFLAGS="${LDFLAGS} -Wl,-rpath,."
             LDSHARED=${LDSHARED-"cc -shared  -Wl,-soname,libz.so -Wl,-msym -Wl,-rpath,$(libdir) -Wl,-set_version,${VER}:1.0"} ;;
  OSF1*)     SFLAGS=${CFLAGS-"-O -std1"}
             CFLAGS=${CFLAGS-"-O -std1"}
             LDSHARED=${LDSHARED-"cc -shared -Wl,-soname,libz.so.1"} ;;
  QNX*)      SFLAGS=${CFLAGS-"-4 -O"}
             CFLAGS=${CFLAGS-"-4 -O"}
             LDSHARED=${LDSHARED-"cc"}
             RANLIB=${RANLIB-"true"}
             AR_RC="cc -A" ;;
  SCO_SV\ 3.2*) SFLAGS=${CFLAGS-"-O3 -dy -KPIC "}
             CFLAGS=${CFLAGS-"-O3"}
             LDSHARED=${LDSHARED-"cc -dy -KPIC -G"} ;;
  SunOS\ 5*) LDSHARED=${LDSHARED-"cc -G"}
         case `(uname -m || echo unknown) 2>/dev/null` in
         i86*)
             SFLAGS=${CFLAGS-"-xpentium -fast -KPIC -R."}
             CFLAGS=${CFLAGS-"-xpentium -fast"} ;;
         *)
             SFLAGS=${CFLAGS-"-fast -xcg92 -KPIC -R."}
             CFLAGS=${CFLAGS-"-fast -xcg92"} ;;
         esac ;;
  SunOS\ 4*) SFLAGS=${CFLAGS-"-O2 -PIC"}
             CFLAGS=${CFLAGS-"-O2"}
             LDSHARED=${LDSHARED-"ld"} ;;
  SunStudio\ 9*) SFLAGS=${CFLAGS-"-fast -xcode=pic32 -xtarget=ultra3 -xarch=v9b"}
             CFLAGS=${CFLAGS-"-fast -xtarget=ultra3 -xarch=v9b"}
             LDSHARED=${LDSHARED-"cc -xarch=v9b"} ;;
  UNIX_System_V\ 4.2.0)
             SFLAGS=${CFLAGS-"-KPIC -O"}
             CFLAGS=${CFLAGS-"-O"}
             LDSHARED=${LDSHARED-"cc -G"} ;;
  UNIX_SV\ 4.2MP)
             SFLAGS=${CFLAGS-"-Kconform_pic -O"}
             CFLAGS=${CFLAGS-"-O"}
             LDSHARED=${LDSHARED-"cc -G"} ;;
  OpenUNIX\ 5)
             SFLAGS=${CFLAGS-"-KPIC -O"}
             CFLAGS=${CFLAGS-"-O"}
             LDSHARED=${LDSHARED-"cc -G"} ;;
  AIX*)  # Courtesy of dbakker@arrayasolutions.com
             SFLAGS=${CFLAGS-"-O -qmaxmem=8192"}
             CFLAGS=${CFLAGS-"-O -qmaxmem=8192"}
             LDSHARED=${LDSHARED-"xlc -G"} ;;
  # send working options for other systems to zlib@gzip.org
  *)         SFLAGS=${CFLAGS-"-O"}
             CFLAGS=${CFLAGS-"-O"}
             LDSHARED=${LDSHARED-"cc -shared"} ;;
  esac
fi

SHAREDLIB=${SHAREDLIB-"libz$shared_ext"}
SHAREDLIBV=${SHAREDLIBV-"libz$shared_ext.$VER"}
SHAREDLIBM=${SHAREDLIBM-"libz$shared_ext.$VER1"}

if test $shared -eq 1; then
  echo Checking for shared library support...
  # we must test in two steps (cc then ld), required at least on SunOS 4.x
  if test "`($CC -w -c $SFLAGS $test.c) 2>&1`" = "" &&
     test "`($LDSHARED $SFLAGS -o $test$shared_ext $test.o) 2>&1`" = ""; then
    echo Building shared library $SHAREDLIBV with $CC.
  elif test -z "$old_cc" -a -z "$old_cflags"; then
    echo No shared library support.
    shared=0;
  else
    echo Tested $CC -w -c $SFLAGS $test.c
    $CC -w -c $SFLAGS $test.c
    echo Tested $LDSHARED $SFLAGS -o $test$shared_ext $test.o
    $LDSHARED $SFLAGS -o $test$shared_ext $test.o
    echo 'No shared library support; try without defining CC and CFLAGS'
    shared=0;
  fi
fi
if test $shared -eq 0; then
  LDSHARED="$CC"
  ALL="static"
  TEST="all teststatic"
  SHAREDLIB=""
  SHAREDLIBV=""
  SHAREDLIBM=""
  echo Building static library $STATICLIB version $VER with $CC.
else
  ALL="static shared"
  TEST="all teststatic testshared"
fi

cat > $test.c <<EOF
#include <sys/types.h>
off64_t dummy = 0;
EOF
if test "`($CC -c $CFLAGS -D_LARGEFILE64_SOURCE=1 $test.c) 2>&1`" = ""; then
  CFLAGS="${CFLAGS} -D_LARGEFILE64_SOURCE=1"
  SFLAGS="${SFLAGS} -D_LARGEFILE64_SOURCE=1"
  ALL="${ALL} all64"
  TEST="${TEST} test64"
  echo "Checking for off64_t... Yes."
  echo "Checking for fseeko... Yes."
else
  echo "Checking for off64_t... No."
  cat > $test.c <<EOF
#include <stdio.h>
int main(void) {
  fseeko(NULL, 0, 0);
  return 0;
}
EOF
  if test "`($CC $CFLAGS -o $test $test.c) 2>&1`" = ""; then
    echo "Checking for fseeko... Yes."
  else
    CFLAGS="${CFLAGS} -DNO_FSEEKO"
    SFLAGS="${SFLAGS} -DNO_FSEEKO"
    echo "Checking for fseeko... No."
  fi
fi

cp -p zconf.h.in zconf.h

cat > $test.c <<EOF
#include <unistd.h>
int main() { return 0; }
EOF
if test "`($CC -c $CFLAGS $test.c) 2>&1`" = ""; then
  sed < zconf.h "/^#ifdef HAVE_UNISTD_H.* may be/s/def HAVE_UNISTD_H\(.*\) may be/ 1\1 was/" > zconf.temp.h
  mv zconf.temp.h zconf.h
  echo "Checking for unistd.h... Yes."
else
  echo "Checking for unistd.h... No."
fi

if test $zprefix -eq 1; then
  sed < zconf.h "/#ifdef Z_PREFIX.* may be/s/def Z_PREFIX\(.*\) may be/ 1\1 was/" > zconf.temp.h
  mv zconf.temp.h zconf.h
  echo "Using z_ prefix on all symbols."
fi

cat > $test.c <<EOF
#include <stdio.h>
#include <stdarg.h>
#include "zconf.h"

int main()
{
#ifndef STDC
  choke me
#endif

  return 0;
}
EOF

if test "`($CC -c $CFLAGS $test.c) 2>&1`" = ""; then
  echo "Checking whether to use vs[n]printf() or s[n]printf()... using vs[n]printf()."

  cat > $test.c <<EOF
#include <stdio.h>
#include <stdarg.h>

int mytest(const char *fmt, ...)
{
  char buf[20];
  va_list ap;

  va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  return 0;
}

int main()
{
  return (mytest("Hello%d\n", 1));
}
EOF

  if test "`($CC $CFLAGS -o $test $test.c) 2>&1`" = ""; then
    echo "Checking for vsnprintf() in stdio.h... Yes."

    cat >$test.c <<EOF
#include <stdio.h>
#include <stdarg.h>

int mytest(const char *fmt, ...)
{
  int n;
  char buf[20];
  va_list ap;

  va_start(ap, fmt);
  n = vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  return n;
}

int main()
{
  return (mytest("Hello%d\n", 1));
}
EOF

    if test "`($CC -c $CFLAGS $test.c) 2>&1`" = ""; then
      echo "Checking for return value of vsnprintf()... Yes."
    else
      CFLAGS="$CFLAGS -DHAS_vsnprintf_void"
      SFLAGS="$SFLAGS -DHAS_vsnprintf_void"
      echo "Checking for return value of vsnprintf()... No."
      echo "  WARNING: apparently vsnprintf() does not return a value. zlib"
      echo "  can build but will be open to possible string-format security"
      echo "  vulnerabilities."
    fi
  else
    CFLAGS="$CFLAGS -DNO_vsnprintf"
    SFLAGS="$SFLAGS -DNO_vsnprintf"
    echo "Checking for vsnprintf() in stdio.h... No."
    echo "  WARNING: vsnprintf() not found, falling back to vsprintf(). zlib"
    echo "  can build but will be open to possible buffer-overflow security"
    echo "  vulnerabilities."

    cat >$test.c <<EOF
#include <stdio.h>
#include <stdarg.h>

int mytest(const char *fmt, ...)
{
  int n;
  char buf[20];
  va_list ap;

  va_start(ap, fmt);
  n = vsprintf(buf, fmt, ap);
  va_end(ap);
  return n;
}

int main()
{
  return (mytest("Hello%d\n", 1));
}
EOF

    if test "`($CC -c $CFLAGS $test.c) 2>&1`" = ""; then
      echo "Checking for return value of vsprintf()... Yes."
    else
      CFLAGS="$CFLAGS -DHAS_vsprintf_void"
      SFLAGS="$SFLAGS -DHAS_vsprintf_void"
      echo "Checking for return value of vsprintf()... No."
      echo "  WARNING: apparently vsprintf() does not return a value. zlib"
      echo "  can build but will be open to possible string-format security"
      echo "  vulnerabilities."
    fi
  fi
else
  echo "Checking whether to use vs[n]printf() or s[n]printf()... using s[n]printf()."

  cat >$test.c <<EOF
#include <stdio.h>

int mytest()
{
  char buf[20];

  snprintf(buf, sizeof(buf), "%s", "foo");
  return 0;
}

int main()
{
  return (mytest());
}
EOF

  if test "`($CC $CFLAGS -o $test $test.c) 2>&1`" = ""; then
    echo "Checking for snprintf() in stdio.h... Yes."

    cat >$test.c <<EOF
#include <stdio.h>

int mytest()
{
  char buf[20];

  return snprintf(buf, sizeof(buf), "%s", "foo");
}

int main()
{
  return (mytest());
}
EOF

    if test "`($CC -c $CFLAGS $test.c) 2>&1`" = ""; then
      echo "Checking for return value of snprintf()... Yes."
    else
      CFLAGS="$CFLAGS -DHAS_snprintf_void"
      SFLAGS="$SFLAGS -DHAS_snprintf_void"
      echo "Checking for return value of snprintf()... No."
      echo "  WARNING: apparently snprintf() does not return a value. zlib"
      echo "  can build but will be open to possible string-format security"
      echo "  vulnerabilities."
    fi
  else
    CFLAGS="$CFLAGS -DNO_snprintf"
    SFLAGS="$SFLAGS -DNO_snprintf"
    echo "Checking for snprintf() in stdio.h... No."
    echo "  WARNING: snprintf() not found, falling back to sprintf(). zlib"
    echo "  can build but will be open to possible buffer-overflow security"
    echo "  vulnerabilities."

    cat >$test.c <<EOF
#include <stdio.h>

int mytest()
{
  char buf[20];

  return sprintf(buf, "%s", "foo");
}

int main()
{
  return (mytest());
}
EOF

    if test "`($CC -c $CFLAGS $test.c) 2>&1`" = ""; then
      echo "Checking for return value of sprintf()... Yes."
    else
      CFLAGS="$CFLAGS -DHAS_sprintf_void"
      SFLAGS="$SFLAGS -DHAS_sprintf_void"
      echo "Checking for return value of sprintf()... No."
      echo "  WARNING: apparently sprintf() does not return a value. zlib"
      echo "  can build but will be open to possible string-format security"
      echo "  vulnerabilities."
    fi
  fi
fi

if test "$gcc" -eq 1; then
  cat > $test.c <<EOF
#if ((__GNUC__-0) * 10 + __GNUC_MINOR__-0 >= 33)
#  define ZLIB_INTERNAL __attribute__((visibility ("hidden")))
#else
#  define ZLIB_INTERNAL
#endif
int ZLIB_INTERNAL foo;
int main()
{
  return 0;
}
EOF
  if test "`($CC -c $CFLAGS $test.c) 2>&1`" = ""; then
    echo "Checking for attribute(visibility) support... Yes."
  else
    CFLAGS="$CFLAGS -DNO_VIZ"
    SFLAGS="$SFLAGS -DNO_VIZ"
    echo "Checking for attribute(visibility) support... No."
  fi
fi

CPP=${CPP-"$CC -E"}
case $CFLAGS in
  *ASMV*)
    if test "`$NM $test.o | grep _hello`" = ""; then
      CPP="$CPP -DNO_UNDERLINE"
      echo Checking for underline in external names... No.
    else
      echo Checking for underline in external names... Yes.
    fi ;;
esac

rm -f $test.[co] $test $test$shared_ext

# udpate Makefile
sed < Makefile.in "
/^CC *=/s#=.*#=$CC#
/^CFLAGS *=/s#=.*#=$CFLAGS#
/^SFLAGS *=/s#=.*#=$SFLAGS#
/^LDFLAGS *=/s#=.*#=$LDFLAGS#
/^LDSHARED *=/s#=.*#=$LDSHARED#
/^CPP *=/s#=.*#=$CPP#
/^STATICLIB *=/s#=.*#=$STATICLIB#
/^SHAREDLIB *=/s#=.*#=$SHAREDLIB#
/^SHAREDLIBV *=/s#=.*#=$SHAREDLIBV#
/^SHAREDLIBM *=/s#=.*#=$SHAREDLIBM#
/^AR *=/s#=.*#=$AR_RC#
/^RANLIB *=/s#=.*#=$RANLIB#
/^LDCONFIG *=/s#=.*#=$LDCONFIG#
/^LDSHAREDLIBC *=/s#=.*#=$LDSHAREDLIBC#
/^EXE *=/s#=.*#=$EXE#
/^prefix *=/s#=.*#=$prefix#
/^exec_prefix *=/s#=.*#=$exec_prefix#
/^libdir *=/s#=.*#=$libdir#
/^sharedlibdir *=/s#=.*#=$sharedlibdir#
/^includedir *=/s#=.*#=$includedir#
/^mandir *=/s#=.*#=$mandir#
/^all: */s#:.*#: $ALL#
/^test: */s#:.*#: $TEST#
" > Makefile

sed < zlib.pc.in "
/^CC *=/s#=.*#=$CC#
/^CFLAGS *=/s#=.*#=$CFLAGS#
/^CPP *=/s#=.*#=$CPP#
/^LDSHARED *=/s#=.*#=$LDSHARED#
/^STATICLIB *=/s#=.*#=$STATICLIB#
/^SHAREDLIB *=/s#=.*#=$SHAREDLIB#
/^SHAREDLIBV *=/s#=.*#=$SHAREDLIBV#
/^SHAREDLIBM *=/s#=.*#=$SHAREDLIBM#
/^AR *=/s#=.*#=$AR_RC#
/^RANLIB *=/s#=.*#=$RANLIB#
/^EXE *=/s#=.*#=$EXE#
/^prefix *=/s#=.*#=$prefix#
/^exec_prefix *=/s#=.*#=$exec_prefix#
/^libdir *=/s#=.*#=$libdir#
/^sharedlibdir *=/s#=.*#=$sharedlibdir#
/^includedir *=/s#=.*#=$includedir#
/^mandir *=/s#=.*#=$mandir#
/^LDFLAGS *=/s#=.*#=$LDFLAGS#
" | sed -e "
s/\@VERSION\@/$VER/g;
" > zlib.pc
