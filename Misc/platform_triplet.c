/* Detect platform triplet from builtin defines
 * cc -E Misc/platform_triplet.c | grep -v '^#' | grep -v '^ *$' | grep -v '^typedef' | tr -d ' '
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
// detect libc (glibc, musl)
# include <features.h>
# if defined(__UCLIBC__)
#  error "uclibc is not supported"
# elif defined(__dietlibc__)
#  error "dietlibc is not supported"
# elif defined(__GLIBC__)
#  define LIBC gnu
#  define LIBC_X32 gnux32
#  define LIBC_ARM_EABIHF gnueabihf
#  define LIBC_ARM_EABI gnueabi
#  define LIBC_PPC_SPE gnuspe
#  if defined(_MIPS_SIM)
#   if _MIPS_SIM == _ABIO32
        define LIBC_MIPS_SIM gnu
#   elif _MIPS_SIM == _ABIN32
        define LIBC_MIPS_SIM gnuabin32
#   elif _MIPS_SIM == _ABI64
        define LIBC_MIPS_SIM gnuabi64
#   endif
#  endif /* defined(_MIPS_SIM) */
# else
/* heuristic to detect musl libc (based on config.guess).
 * Adds "typedef __builtin_va_list va_list;" to output.
 */
#  include <stdarg.h>
#  ifdef __DEFINED_va_list
#   define LIBC musl
// #   define LIBC_X32
#   define LIBC_ARM_EABIHF musleabihf
#   define LIBC_ARM_EABI musleabi
//#   define LIBC_PPC_SPE
#   define LIBC_MIPS_SIM musl
#  else
#   error "unknown libc"
#  endif
#endif

# if defined(__x86_64__) && defined(__LP64__)
        x86_64-linux-LIBC
# elif defined(__x86_64__) && defined(__ILP32__) && defined(LIBC_X32)
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
# elif defined(__ARM_EABI__) && defined(__ARM_PCS_VFP) && defined(LIBC_ARM_EABIHF)
#  if defined(__ARMEL__)
        arm-linux-LIBC_ARM_EABIHF
#  else
        armeb-linux-LIBC_ARM_EABIHF
#  endif
# elif defined(__ARM_EABI__) && !defined(__ARM_PCS_VFP) && defined(LIBC_ARM_EABI)
#  if defined(__ARMEL__)
        arm-linux-LIBC_ARM_EABI
#  else
        armeb-linux-LIBC_ARM_EABI
#  endif
# elif defined(__hppa__)
        hppa-linux-LIBC
# elif defined(__ia64__)
        ia64-linux-LIBC
# elif defined(__m68k__) && !defined(__mcoldfire__)
        m68k-linux-LIBC
# elif defined(__mips_hard_float) && defined(__mips_isa_rev) && (__mips_isa_rev >=6) && defined(_MIPSEL) && defined(LIBC_MIPS_SIM)
#  if _MIPS_SIM == _ABIO32
        mipsisa32r6el-linux-LIBC_MIPS_SIM
#  elif (_MIPS_SIM == _ABIN32) || (_MIPS_SIM == _ABI64)
        mipsisa64r6el-linux-LIBC_MIPS_SIM
#  else
#   error unknown platform triplet
#  endif
# elif defined(__mips_hard_float) && defined(__mips_isa_rev) && (__mips_isa_rev >=6) && defined(LIBC_MIPS_SIM)
#  if _MIPS_SIM == _ABIO32
        mipsisa32r6-linux-LIBC_MIPS_SIM
#  elif (_MIPS_SIM == _ABIN32) || (_MIPS_SIM == _ABI64)
        mipsisa64r6-linux-LIBC_MIPS_SIM
#  else
#   error unknown platform triplet
#  endif
# elif defined(__mips_hard_float) && defined(_MIPSEL) && defined(LIBC_MIPS_SIM)
#  if _MIPS_SIM == _ABIO32
        mipsel-linux-LIBC_MIPS_SIM
#  elif (_MIPS_SIM == _ABIN32) || (_MIPS_SIM == _ABI64)
        mips64el-linux-LIBC_MIPS_SIM
#  else
#   error unknown platform triplet
#  endif
# elif defined(__mips_hard_float) && defined(LIBC_MIPS_SIM)
#  if _MIPS_SIM == _ABIO32
        mips-linux-LIBC_MIPS_SIM
#  elif (_MIPS_SIM == _ABIN32) || (_MIPS_SIM == _ABI64)
        mips64-linux-LIBC_MIPS_SIM
#  else
#   error unknown platform triplet
#  endif
# elif defined(__or1k__)
        or1k-linux-LIBC
# elif defined(__powerpc__) && defined(__SPE__) && defined(LIBC_PPC_SPE)
        powerpc-linux-LIBC_PPC_SPE
# elif defined(__powerpc64__)
#  if defined(__LITTLE_ENDIAN__)
        powerpc64le-linux-LIBC
#  else
        powerpc64-linux-LIBC
#  endif
# elif defined(__powerpc__)
        powerpc-linux-LIBC
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
#elif defined(__EMSCRIPTEN__)
#  if defined(__wasm32__)
        wasm32-emscripten
#  elif defined(__wasm64__)
        wasm64-emscripten
#  else
#    error unknown Emscripten platform
#  endif
#elif defined(__wasi__)
#  if defined(__wasm32__)
        wasm32-wasi
#  elif defined(__wasm64__)
        wasm64-wasi
#  else
#    error unknown WASI platform
#  endif
#else
# error unknown platform triplet
#endif
