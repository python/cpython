/* fficonfig.h created manually for Windows CE on ARM */
/* fficonfig.h.in.  Generated from configure.ac by autoheader.  */

/* 1234 = LIL_ENDIAN, 4321 = BIGENDIAN */
#define BYTEORDER 1234

/* Define to one of `_getb67', `GETB67', `getb67' for Cray-2 and Cray-YMP
   systems. This function is required for `alloca.c' support on those systems.
   */
/* #undef CRAY_STACKSEG_END */

/* Define to 1 if using `alloca.c'. */
/* #undef C_ALLOCA */

/* Define to the flags needed for the .section .eh_frame directive. */
/* #undef EH_FRAME_FLAGS */

/* Define this if you want extra debugging. */
#ifdef DEBUG  /* Defined by the project settings for Debug builds */
#define FFI_DEBUG
#else
#undef FFI_DEBUG
#endif

/* Define this is you do not want support for the raw API. */
/* #undef FFI_NO_RAW_API */

/* Define this is you do not want support for aggregate types. */
/* #undef FFI_NO_STRUCTS */

/* Define to 1 if you have `alloca', as a function or macro. */
#define HAVE_ALLOCA 1

/* Define to 1 if you have <alloca.h> and it should be used (not on Ultrix).
   */
/* #undef HAVE_ALLOCA_H */

/* Define if your assembler supports .register. */
/* #undef HAVE_AS_REGISTER_PSEUDO_OP */

/* Define if your assembler and linker support unaligned PC relative relocs.
   */
/* #undef HAVE_AS_SPARC_UA_PCREL */

/* Define to 1 if you have the <inttypes.h> header file. */
/* #undef HAVE_INTTYPES_H */

/* Define if you have the long double type and it is bigger than a double */
/* This differs from the MSVC build, but even there it should not be defined */
/* #undef HAVE_LONG_DOUBLE */

/* Define to 1 if you have the `memcpy' function. */
#define HAVE_MEMCPY 1

/* Define to 1 if you have the <memory.h> header file. */
/* WinCE has this but I don't think we need to use it */
/* #undef HAVE_MEMORY_H */

/* Define to 1 if you have the `mmap' function. */
/* #undef HAVE_MMAP */

/* Define if mmap with MAP_ANON(YMOUS) works. */
/* #undef HAVE_MMAP_ANON */

/* Define if mmap of /dev/zero works. */
/* #undef HAVE_MMAP_DEV_ZERO */

/* Define if read-only mmap of a plain file works. */
/* #undef HAVE_MMAP_FILE */

/* Define if .eh_frame sections should be read-only. */
/* #undef HAVE_RO_EH_FRAME */

/* Define to 1 if you have the <stdint.h> header file. */
/* #undef HAVE_STDINT_H */

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
/* #undef HAVE_STRINGS_H */

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the <sys/mman.h> header file. */
/* #undef HAVE_SYS_MMAN_H */

/* Define to 1 if you have the <sys/stat.h> header file. */
/* #undef HAVE_SYS_STAT_H */

/* Define to 1 if you have the <sys/types.h> header file. */
/* #undef HAVE_SYS_TYPES_H */

/* Define to 1 if you have the <unistd.h> header file. */
/* #undef HAVE_UNISTD_H */

/* Define if the host machine stores words of multi-word integers in
   big-endian order. */
/* #undef HOST_WORDS_BIG_ENDIAN */

/* Define to 1 if your C compiler doesn't accept -c and -o together. */
/* #undef NO_MINUS_C_MINUS_O */

/* Name of package */
/* #undef PACKAGE */

/* Define to the address where bug reports for this package should be sent. */
/* #undef PACKAGE_BUGREPORT */

/* Define to the full name of this package. */
/* #undef PACKAGE_NAME */

/* Define to the full name and version of this package. */
/* #undef PACKAGE_STRING */

/* Define to the one symbol short name of this package. */
/* #undef PACKAGE_TARNAME */

/* Define to the version of this package. */
/* #undef PACKAGE_VERSION */

/* The number of bytes in type double */
#define SIZEOF_DOUBLE 8

/* The number of bytes in type long double */
#define SIZEOF_LONG_DOUBLE 8

/* If using the C implementation of alloca, define if you know the
   direction of stack growth for your system; otherwise it will be
   automatically deduced at run-time.
	STACK_DIRECTION > 0 => grows toward higher addresses
	STACK_DIRECTION < 0 => grows toward lower addresses
	STACK_DIRECTION = 0 => direction of growth unknown */
/* #undef STACK_DIRECTION */

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Define this if you are using Purify and want to suppress spurious messages.
   */
/* #undef USING_PURIFY */

/* Version number of package */
/* #undef VERSION */

/* whether byteorder is bigendian */
/* #undef WORDS_BIGENDIAN */

#define alloca _alloca

#define abort() exit(999)
