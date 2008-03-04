/* -----------------------------------------------------------------*-C-*-
   ffitarget.h - Copyright (c) 1996-2003  Red Hat, Inc.
   Copyright (C) 2007 Free Software Foundation, Inc
   Target configuration macros for PowerPC.

   Permission is hereby granted, free of charge, to any person obtaining
   a copy of this software and associated documentation files (the
   ``Software''), to deal in the Software without restriction, including
   without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense, and/or sell copies of the Software, and to
   permit persons to whom the Software is furnished to do so, subject to
   the following conditions:

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED ``AS IS'', WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
   HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   DEALINGS IN THE SOFTWARE.

   ----------------------------------------------------------------------- */

#ifndef LIBFFI_TARGET_H
#define LIBFFI_TARGET_H

/* ---- System specific configurations ----------------------------------- */

#if defined (POWERPC) && defined (__powerpc64__)
#define POWERPC64
#endif

#ifndef LIBFFI_ASM
typedef unsigned long          ffi_arg;
typedef signed long            ffi_sarg;

typedef enum ffi_abi {
  FFI_FIRST_ABI = 0,

#ifdef POWERPC
  FFI_SYSV,
  FFI_GCC_SYSV,
  FFI_LINUX64,
  FFI_LINUX,
  FFI_LINUX_SOFT_FLOAT,
# ifdef POWERPC64
  FFI_DEFAULT_ABI = FFI_LINUX64,
# else
#  if (!defined(__NO_FPRS__) && (__LDBL_MANT_DIG__ == 106))
  FFI_DEFAULT_ABI = FFI_LINUX,
#  else
#   ifdef __NO_FPRS__
  FFI_DEFAULT_ABI = FFI_LINUX_SOFT_FLOAT,
#   else
  FFI_DEFAULT_ABI = FFI_GCC_SYSV,
#   endif
#  endif
# endif
#endif

#ifdef POWERPC_AIX
  FFI_AIX,
  FFI_DARWIN,
  FFI_DEFAULT_ABI = FFI_AIX,
#endif

#ifdef POWERPC_DARWIN
  FFI_AIX,
  FFI_DARWIN,
  FFI_DEFAULT_ABI = FFI_DARWIN,
#endif

#ifdef POWERPC_FREEBSD
  FFI_SYSV,
  FFI_GCC_SYSV,
  FFI_LINUX64,
  FFI_DEFAULT_ABI = FFI_SYSV,
#endif

  FFI_LAST_ABI
} ffi_abi;
#endif

/* ---- Definitions for closures ----------------------------------------- */

#define FFI_CLOSURES 1
#define FFI_NATIVE_RAW_API 0

/* For additional types like the below, take care about the order in
   ppc_closures.S. They must follow after the FFI_TYPE_LAST.  */

/* Needed for soft-float long-double-128 support.  */
#define FFI_TYPE_UINT128 (FFI_TYPE_LAST + 1)

/* Needed for FFI_SYSV small structure returns.  */
#define FFI_SYSV_TYPE_SMALL_STRUCT (FFI_TYPE_LAST + 2)

#if defined(POWERPC64) || defined(POWERPC_AIX)
#define FFI_TRAMPOLINE_SIZE 24
#else /* POWERPC || POWERPC_AIX */
#define FFI_TRAMPOLINE_SIZE 40
#endif

#ifndef LIBFFI_ASM
#if defined(POWERPC_DARWIN) || defined(POWERPC_AIX)
struct ffi_aix_trampoline_struct {
    void * code_pointer;	/* Pointer to ffi_closure_ASM */
    void * toc;			/* TOC */
    void * static_chain;	/* Pointer to closure */
};
#endif
#endif

#endif
