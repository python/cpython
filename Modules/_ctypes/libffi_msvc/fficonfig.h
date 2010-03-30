/* fficonfig.h.  Originally created by configure, now hand_maintained for MSVC. */

/* fficonfig.h.  Generated automatically by configure.  */
/* fficonfig.h.in.  Generated automatically from configure.in by autoheader.  */

/* Define this for MSVC, but not for mingw32! */
#ifdef _MSC_VER
#define __attribute__(x) /* */
#endif
#define alloca _alloca

/*----------------------------------------------------------------*/

/* Define if using alloca.c.  */
/* #undef C_ALLOCA */

/* Define to one of _getb67, GETB67, getb67 for Cray-2 and Cray-YMP systems.
   This function is required for alloca.c support on those systems.  */
/* #undef CRAY_STACKSEG_END */

/* Define if you have alloca, as a function or macro.  */
#define HAVE_ALLOCA 1

/* Define if you have <alloca.h> and it should be used (not on Ultrix).  */
/* #define HAVE_ALLOCA_H 1 */

/* If using the C implementation of alloca, define if you know the
   direction of stack growth for your system; otherwise it will be
   automatically deduced at run-time.
 STACK_DIRECTION > 0 => grows toward higher addresses
 STACK_DIRECTION < 0 => grows toward lower addresses
 STACK_DIRECTION = 0 => direction of growth unknown
 */
/* #undef STACK_DIRECTION */

/* Define if you have the ANSI C header files.  */
#define STDC_HEADERS 1

/* Define if you have the memcpy function.  */
#define HAVE_MEMCPY 1

/* Define if read-only mmap of a plain file works. */
//#define HAVE_MMAP_FILE 1

/* Define if mmap of /dev/zero works. */
//#define HAVE_MMAP_DEV_ZERO 1

/* Define if mmap with MAP_ANON(YMOUS) works. */
//#define HAVE_MMAP_ANON 1

/* The number of bytes in type double */
#define SIZEOF_DOUBLE 8

/* The number of bytes in type long double */
#define SIZEOF_LONG_DOUBLE 12

/* Define if you have the long double type and it is bigger than a double */
#define HAVE_LONG_DOUBLE 1

/* whether byteorder is bigendian */
/* #undef WORDS_BIGENDIAN */

/* Define if the host machine stores words of multi-word integers in
   big-endian order. */
/* #undef HOST_WORDS_BIG_ENDIAN */

/* 1234 = LIL_ENDIAN, 4321 = BIGENDIAN */
#define BYTEORDER 1234

/* Define if your assembler and linker support unaligned PC relative relocs. */
/* #undef HAVE_AS_SPARC_UA_PCREL */

/* Define if your assembler supports .register. */
/* #undef HAVE_AS_REGISTER_PSEUDO_OP */

/* Define if .eh_frame sections should be read-only. */
/* #undef HAVE_RO_EH_FRAME */

/* Define to the flags needed for the .section .eh_frame directive. */
/* #define EH_FRAME_FLAGS "aw" */

/* Define to the flags needed for the .section .eh_frame directive. */
/* #define EH_FRAME_FLAGS "aw" */

/* Define this if you want extra debugging. */
/* #undef FFI_DEBUG */

/* Define this is you do not want support for aggregate types. */
/* #undef FFI_NO_STRUCTS */

/* Define this is you do not want support for the raw API. */
/* #undef FFI_NO_RAW_API */

/* Define this if you are using Purify and want to suppress spurious messages. */
/* #undef USING_PURIFY */

