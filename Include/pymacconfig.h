#ifndef PYMACCONFIG_H
#define PYMACCONFIG_H
     /*
      * This file moves some of the autoconf magic to compile-time
      * when building on MacOSX. This is needed for building 4-way
      * universal binaries and for 64-bit universal binaries because
      * the values redefined below aren't configure-time constant but
      * only compile-time constant in these scenarios.
      */

#if defined(__APPLE__)

# undef SIZEOF_LONG
# undef SIZEOF_PTHREAD_T
# undef SIZEOF_SIZE_T
# undef SIZEOF_TIME_T
# undef SIZEOF_VOID_P
# undef SIZEOF__BOOL
# undef WORDS_BIGENDIAN

#    undef VA_LIST_IS_ARRAY
#    if defined(__LP64__) && defined(__x86_64__)
#        define VA_LIST_IS_ARRAY 1
#    endif

#    undef HAVE_LARGEFILE_SUPPORT
#    ifndef __LP64__
#         define HAVE_LARGEFILE_SUPPORT 1
#    endif

#    undef SIZEOF_LONG
#    ifdef __LP64__
#        define SIZEOF__BOOL            1
#        define SIZEOF__BOOL            1
#        define SIZEOF_LONG             8
#        define SIZEOF_PTHREAD_T        8
#        define SIZEOF_SIZE_T           8
#        define SIZEOF_TIME_T           8
#        define SIZEOF_VOID_P           8
#    else
#        ifdef __ppc__
#           define SIZEOF__BOOL         4
#        else
#           define SIZEOF__BOOL         1
#        endif
#        define SIZEOF_LONG             4
#        define SIZEOF_PTHREAD_T        4
#        define SIZEOF_SIZE_T           4
#        define SIZEOF_TIME_T           4
#        define SIZEOF_VOID_P           4
#    endif

#    if defined(__LP64__)
     /* MacOSX 10.4 (the first release to suppport 64-bit code
      * at all) only supports 64-bit in the UNIX layer.
      * Therefore surpress the toolbox-glue in 64-bit mode.
      */

    /* In 64-bit mode setpgrp always has no argments, in 32-bit
     * mode that depends on the compilation environment
     */
#       undef SETPGRP_HAVE_ARG

#    endif

#ifdef __BIG_ENDIAN__
#define WORDS_BIGENDIAN 1
#endif /* __BIG_ENDIAN */

    /*
     * The definition in pyconfig.h is only valid on the OS release
     * where configure ran on and not necessarily for all systems where
     * the executable can be used on.
     *
     * Specifically: OSX 10.4 has limited supported for '%zd', while
     * 10.5 has full support for '%zd'. A binary built on 10.5 won't
     * work properly on 10.4 unless we surpress the definition
     * of PY_FORMAT_SIZE_T
     */
#undef  PY_FORMAT_SIZE_T


#endif /* defined(_APPLE__) */

#endif /* PYMACCONFIG_H */
