/* Detect platform triplet from builtin defines
 * cc -E Misc/platform_triplet.c | grep -v '^#' | grep -v '^ *$' | tr -d ' 	'
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
# elif defined(__loongarch__)
#  if defined(__loongarch_lp64)
#   if defined(__loongarch_soft_float)
        loongarch64-linux-gnusf
#   elif defined(__loongarch_single_float)
        loongarch64-linux-gnuf32
#   elif defined(__loongarch_double_float)
        loongarch64-linux-gnu
#   else
#    error unknown platform triplet
#   endif
#  else
#   error unknown platform triplet
#  endif
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
