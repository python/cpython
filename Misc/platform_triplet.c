/* Detect platform triplet from builtin defines
 * cc -E Misc/platform_triplet.c | grep -v '^#' | grep -v '^ *$' | grep -v '^typedef' | tr -d ' 	'
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
    # Android is not a multiarch system.
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
/* Heuristic to detect musl libc
 * Adds "typedef __builtin_va_list va_list;" to output
 */
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
        x86_64-linux-LIBC
# elif defined(__x86_64__) && defined(__ILP32__)
        x86_64-linux-LIBC_X32
# elif defined(__i386__)
        i386-linux-LIBC
# elif defined(__aarch64__) && defined(__AARCH64EL__)
#  if defined(__ILP32__)
        aarch64_ilp32-linux-LIBC
#  else
        aarch64-linux-LIBC
#  endif
# elif defined(__aarch64__) && defined(__AARCH64EB__)
#  if defined(__ILP32__)
        aarch64_be_ilp32-linux-LIBC
#  else
        aarch64_be-linux-LIBC
#  endif
# elif defined(__alpha__)
        alpha-linux-LIBC
# elif defined(__ARM_EABI__)
#  if defined(__ARMEL__)
        arm-linux-LIBC_ARM
#  else
        armeb-linux-LIBC_ARM
#  endif
# elif defined(__hppa__)
        hppa-linux-LIBC
# elif defined(__ia64__)
        ia64-linux-LIBC
# elif defined(__loongarch__) && defined(__loongarch_lp64)
        loongarch64-linux-LIBC_LA
# elif defined(__m68k__) && !defined(__mcoldfire__)
        m68k-linux-LIBC
# elif defined(__mips__)
#  if defined(__mips_isa_rev) && (__mips_isa_rev >=6)
#   if defined(_MIPSEL) && defined(__mips64)
        mipsisa64r6el-linux-LIBC_MIPS
#   elif defined(_MIPSEL)
        mipsisa32r6el-linux-LIBC_MIPS
#   elif defined(__mips64)
        mipsisa64r6-linux-LIBC_MIPS
#   else
        mipsisa32r6-linux-LIBC_MIPS
#   endif
#  else
#   if defined(_MIPSEL) && defined(__mips64)
        mips64el-linux-LIBC_MIPS
#   elif defined(_MIPSEL)
        mipsel-linux-LIBC_MIPS
#   elif defined(__mips64)
        mips64-linux-LIBC_MIPS
#   else
        mips-linux-LIBC_MIPS
#   endif
#  endif
# elif defined(__or1k__)
        or1k-linux-LIBC
# elif defined(__powerpc__)
        powerpc-linux-LIBC_PPC
# elif defined(__powerpc64__)
#  if defined(__LITTLE_ENDIAN__)
        powerpc64le-linux-LIBC
#  else
        powerpc64-linux-LIBC
#  endif
# elif defined(__s390x__)
        s390x-linux-LIBC
# elif defined(__s390__)
        s390-linux-LIBC
# elif defined(__sh__) && defined(__LITTLE_ENDIAN__)
        sh4-linux-LIBC
# elif defined(__sparc__) && defined(__arch64__)
        sparc64-linux-LIBC
# elif defined(__sparc__)
        sparc-linux-LIBC
# elif defined(__riscv)
#  if __riscv_xlen == 32
        riscv32-linux-LIBC
#  elif __riscv_xlen == 64
        riscv64-linux-LIBC
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
#    if defined(_REENTRANT)
	wasm32-wasi-threads
#    else
	wasm32-wasi
#    endif
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
