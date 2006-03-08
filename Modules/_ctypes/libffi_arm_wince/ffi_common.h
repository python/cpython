/* -----------------------------------------------------------------------
   ffi_common.h - Copyright (c) 1996  Red Hat, Inc.

   Common internal definitions and macros. Only necessary for building
   libffi.
   ----------------------------------------------------------------------- */

#ifndef FFI_COMMON_H
#define FFI_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

#include <fficonfig.h>

/* Do not move this. Some versions of AIX are very picky about where
   this is positioned. */
#ifdef __GNUC__
# define alloca __builtin_alloca
#else
# if HAVE_ALLOCA_H
#  include <alloca.h>
# else
#  ifdef _AIX
 #pragma alloca
#  else
#   ifndef alloca /* predefined by HP cc +Olibcalls */
char *alloca ();
#   endif
#  endif
# endif
#endif

/* Check for the existence of memcpy. */
#if STDC_HEADERS
# include <string.h>
#else
# ifndef HAVE_MEMCPY
#  define memcpy(d, s, n) bcopy ((s), (d), (n))
# endif
#endif

#if defined(FFI_DEBUG) 
#include <stdio.h>
#endif

#ifdef FFI_DEBUG
/*@exits@*/ void ffi_assert(/*@temp@*/ char *expr, /*@temp@*/ char *file, int line);
void ffi_stop_here(void);
void ffi_type_test(/*@temp@*/ /*@out@*/ ffi_type *a, /*@temp@*/ char *file, int line);

#define FFI_ASSERT(x) ((x) ? (void)0 : ffi_assert(#x, __FILE__,__LINE__))
#define FFI_ASSERT_AT(x, f, l) ((x) ? 0 : ffi_assert(#x, (f), (l)))
#define FFI_ASSERT_VALID_TYPE(x) ffi_type_test (x, __FILE__, __LINE__)
#else
#define FFI_ASSERT(x) 
#define FFI_ASSERT_AT(x, f, l)
#define FFI_ASSERT_VALID_TYPE(x)
#endif

#define ALIGN(v, a)  (((((size_t) (v))-1) | ((a)-1))+1)

/* Perform machine dependent cif processing */
ffi_status ffi_prep_cif_machdep(ffi_cif *cif);

/* Extended cif, used in callback from assembly routine */
typedef struct
{
  /*@dependent@*/ ffi_cif *cif;
  /*@dependent@*/ void *rvalue;
  /*@dependent@*/ void **avalue;
} extended_cif;

/* Terse sized type definitions.  */
#if defined(__GNUC__)

typedef unsigned int UINT8  __attribute__((__mode__(__QI__)));
typedef signed int   SINT8  __attribute__((__mode__(__QI__)));
typedef unsigned int UINT16 __attribute__((__mode__(__HI__)));
typedef signed int   SINT16 __attribute__((__mode__(__HI__)));
typedef unsigned int UINT32 __attribute__((__mode__(__SI__)));
typedef signed int   SINT32 __attribute__((__mode__(__SI__)));
typedef unsigned int UINT64 __attribute__((__mode__(__DI__)));
typedef signed int   SINT64 __attribute__((__mode__(__DI__)));

#elif defined(_MSC_VER)

typedef unsigned __int8  UINT8;
typedef signed __int8    SINT8;
typedef unsigned __int16 UINT16;
typedef signed __int16   SINT16;
typedef unsigned __int32 UINT32;
typedef signed __int32   SINT32;
typedef unsigned __int64 UINT64;
typedef signed __int64   SINT64;

#else
#error "Need typedefs here"
#endif

typedef float FLOAT32;


#ifdef __cplusplus
}
#endif

#endif


