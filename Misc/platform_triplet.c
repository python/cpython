/* Detect platform triplet from builtin defines
 * cc -E Misc/platform_triplet.c | grep '^PLATFORM_TRIPLET=' | tr -d ' '
 */
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
#  if defined(__x86_64__)
PLATFORM_TRIPLET=x86_64-linux-android
#  elif defined(__i386__)
PLATFORM_TRIPLET=i686-linux-android
#  elif defined(__aarch64__)
PLATFORM_TRIPLET=aarch64-linux-android
#  elif defined(__arm__)
PLATFORM_TRIPLET=arm-linux-androideabi
#  else
#    error unknown Android platform
#  endif

#elif defined(__linux__)
/*
 * BEGIN of Linux block
 */
// Detect libc (based on config.guess)
# include <features.h>
# if defined(__UCLIBC__)
#  error uclibc not supported
# elif defined(__dietlibc__)
#  error dietlibc not supported
# elif defined(__GLIBC__)
#  define LIBC gnu
#  define LIBC_X32 gnux32
#  if defined(__ARM_PCS_VFP)
#   define LIBC_ARM gnueabihf
#  else
#   define LIBC_ARM gnueabi
#  endif
#  if defined(__loongarch__)
#   if defined(__loongarch_soft_float)
#    define LIBC_LA gnusf
#   elif defined(__loongarch_single_float)
#    define LIBC_LA gnuf32
#   elif defined(__loongarch_double_float)
#    define LIBC_LA gnu
#   else
#    error unknown loongarch floating-point base abi
#   endif
#  endif
#  if defined(_MIPS_SIM)
#   if defined(__mips_hard_float)
#    if _MIPS_SIM == _ABIO32
#     define LIBC_MIPS gnu
#    elif _MIPS_SIM == _ABIN32
#     define LIBC_MIPS gnuabin32
#    elif _MIPS_SIM == _ABI64
#     define LIBC_MIPS gnuabi64
#    else
#     error unknown mips sim value
#    endif
#   else
#    if _MIPS_SIM == _ABIO32
#     define LIBC_MIPS gnusf
#    elif _MIPS_SIM == _ABIN32
#     define LIBC_MIPS gnuabin32sf
#    elif _MIPS_SIM == _ABI64
#     define LIBC_MIPS gnuabi64sf
#    else
#     error unknown mips sim value
#    endif
#   endif
#  endif
#  if defined(__SPE__)
#   define LIBC_PPC gnuspe
#  else
#   define LIBC_PPC gnu
#  endif
# else
// Heuristic to detect musl libc
#  include <stdarg.h>
#  ifdef __DEFINED_va_list
#   define LIBC musl
#   define LIBC_X32 muslx32
#   if defined(__ARM_PCS_VFP)
#    define LIBC_ARM musleabihf
#   else
#    define LIBC_ARM musleabi
#   endif
#   if defined(__loongarch__)
#    if defined(__loongarch_soft_float)
#     define LIBC_LA muslsf
#    elif defined(__loongarch_single_float)
#     define LIBC_LA muslf32
#    elif defined(__loongarch_double_float)
#     define LIBC_LA musl
#    else
#     error unknown loongarch floating-point base abi
#    endif
#   endif
#   if defined(_MIPS_SIM)
#    if defined(__mips_hard_float)
#     if _MIPS_SIM == _ABIO32
#      define LIBC_MIPS musl
#     elif _MIPS_SIM == _ABIN32
#      define LIBC_MIPS musln32
#     elif _MIPS_SIM == _ABI64
#      define LIBC_MIPS musl
#     else
#      error unknown mips sim value
#     endif
#    else
#     if _MIPS_SIM == _ABIO32
#      define LIBC_MIPS muslsf
#     elif _MIPS_SIM == _ABIN32
#      define LIBC_MIPS musln32sf
#     elif _MIPS_SIM == _ABI64
#      define LIBC_MIPS muslsf
#     else
#      error unknown mips sim value
#     endif
#    endif
#   endif
#   if defined(_SOFT_FLOAT) || defined(__NO_FPRS__)
#    define LIBC_PPC muslsf
#   else
#    define LIBC_PPC musl
#   endif
#  else
#   error unknown libc
#  endif
# endif

# if defined(__x86_64__) && defined(__LP64__)
PLATFORM_TRIPLET=x86_64-linux-LIBC
# elif defined(__x86_64__) && defined(__ILP32__)
PLATFORM_TRIPLET=x86_64-linux-LIBC_X32
# elif defined(__i386__)
PLATFORM_TRIPLET=i386-linux-LIBC
# elif defined(__aarch64__) && defined(__AARCH64EL__)
#  if defined(__ILP32__)
PLATFORM_TRIPLET=aarch64_ilp32-linux-LIBC
#  else
PLATFORM_TRIPLET=aarch64-linux-LIBC
#  endif
# elif defined(__aarch64__) && defined(__AARCH64EB__)
#  if defined(__ILP32__)
PLATFORM_TRIPLET=aarch64_be_ilp32-linux-LIBC
#  else
PLATFORM_TRIPLET=aarch64_be-linux-LIBC
#  endif
# elif defined(__alpha__)
PLATFORM_TRIPLET=alpha-linux-LIBC
# elif defined(__ARM_EABI__)
#  if defined(__ARMEL__)
PLATFORM_TRIPLET=arm-linux-LIBC_ARM
#  else
PLATFORM_TRIPLET=armeb-linux-LIBC_ARM
#  endif
# elif defined(__hppa__)
PLATFORM_TRIPLET=hppa-linux-LIBC
# elif defined(__ia64__)
PLATFORM_TRIPLET=ia64-linux-LIBC
# elif defined(__loongarch__) && defined(__loongarch_lp64)
PLATFORM_TRIPLET=loongarch64-linux-LIBC_LA
# elif defined(__m68k__) && !defined(__mcoldfire__)
PLATFORM_TRIPLET=m68k-linux-LIBC
# elif defined(__mips__)
#  if defined(__mips_isa_rev) && (__mips_isa_rev >=6)
#   if defined(_MIPSEL) && defined(__mips64)
PLATFORM_TRIPLET=mipsisa64r6el-linux-LIBC_MIPS
#   elif defined(_MIPSEL)
PLATFORM_TRIPLET=mipsisa32r6el-linux-LIBC_MIPS
#   elif defined(__mips64)
PLATFORM_TRIPLET=mipsisa64r6-linux-LIBC_MIPS
#   else
PLATFORM_TRIPLET=mipsisa32r6-linux-LIBC_MIPS
#   endif
#  else
#   if defined(_MIPSEL) && defined(__mips64)
PLATFORM_TRIPLET=mips64el-linux-LIBC_MIPS
#   elif defined(_MIPSEL)
PLATFORM_TRIPLET=mipsel-linux-LIBC_MIPS
#   elif defined(__mips64)
PLATFORM_TRIPLET=mips64-linux-LIBC_MIPS
#   else
PLATFORM_TRIPLET=mips-linux-LIBC_MIPS
#   endif
#  endif
# elif defined(__or1k__)
PLATFORM_TRIPLET=or1k-linux-LIBC
# elif defined(__powerpc64__)
#  if defined(__LITTLE_ENDIAN__)
PLATFORM_TRIPLET=powerpc64le-linux-LIBC
#  else
PLATFORM_TRIPLET=powerpc64-linux-LIBC
#  endif
# elif defined(__powerpc__)
PLATFORM_TRIPLET=powerpc-linux-LIBC_PPC
# elif defined(__s390x__)
PLATFORM_TRIPLET=s390x-linux-LIBC
# elif defined(__s390__)
PLATFORM_TRIPLET=s390-linux-LIBC
# elif defined(__sh__) && defined(__LITTLE_ENDIAN__)
PLATFORM_TRIPLET=sh4-linux-LIBC
# elif defined(__sparc__) && defined(__arch64__)
PLATFORM_TRIPLET=sparc64-linux-LIBC
# elif defined(__sparc__)
PLATFORM_TRIPLET=sparc-linux-LIBC
# elif defined(__riscv)
#  if __riscv_xlen == 32
PLATFORM_TRIPLET=riscv32-linux-LIBC
#  elif __riscv_xlen == 64
PLATFORM_TRIPLET=riscv64-linux-LIBC
#  else
#   error unknown platform triplet
#  endif
# else
#   error unknown platform triplet
# endif
/*
 * END of Linux block
 */
#elif defined(__FreeBSD_kernel__)
# if defined(__LP64__)
PLATFORM_TRIPLET=x86_64-kfreebsd-gnu
# elif defined(__i386__)
PLATFORM_TRIPLET=i386-kfreebsd-gnu
# else
#   error unknown platform triplet
# endif
#elif defined(__gnu_hurd__)
# if defined(__x86_64__) && defined(__LP64__)
PLATFORM_TRIPLET=x86_64-gnu
# elif defined(__i386__)
PLATFORM_TRIPLET=i386-gnu
# else
#   error unknown platform triplet
# endif
#elif defined(__APPLE__)
#  include "TargetConditionals.h"
// Older macOS SDKs do not define TARGET_OS_*
#  if defined(TARGET_OS_IOS) && TARGET_OS_IOS
#    if defined(TARGET_OS_SIMULATOR) && TARGET_OS_SIMULATOR
#      if __x86_64__
PLATFORM_TRIPLET=x86_64-iphonesimulator
#      else
PLATFORM_TRIPLET=arm64-iphonesimulator
#      endif
#    else
PLATFORM_TRIPLET=arm64-iphoneos
#    endif
// Older macOS SDKs do not define TARGET_OS_OSX
#  elif !defined(TARGET_OS_OSX) || TARGET_OS_OSX
PLATFORM_TRIPLET=darwin
#  else
#    error unknown Apple platform
#  endif
#elif defined(__VXWORKS__)
PLATFORM_TRIPLET=vxworks
#elif defined(__wasm32__)
#  if defined(__EMSCRIPTEN__)
PLATFORM_TRIPLET=wasm32-emscripten
#  elif defined(__wasi__)
#    if defined(_REENTRANT)
PLATFORM_TRIPLET=wasm32-wasi-threads
#    else
PLATFORM_TRIPLET=wasm32-wasi
#    endif
#  else
#    error unknown wasm32 platform
#  endif
#elif defined(__wasm64__)
#  if defined(__EMSCRIPTEN__)
PLATFORM_TRIPLET=wasm64-emscripten
#  elif defined(__wasi__)
PLATFORM_TRIPLET=wasm64-wasi
#  else
#    error unknown wasm64 platform
#  endif
#else
# error unknown platform triplet
#endif
