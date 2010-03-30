/* -----------------------------------------------------------------*-C-*-
   ffitarget.h - Copyright (c) 1996-2003  Red Hat, Inc.
   Target configuration macros for x86 and x86-64.

   Permission is hereby granted, free of charge, to any person obtaining
   a copy of this software and associated documentation files (the
   ``Software''), to deal in the Software without restriction, including
   without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense, and/or sell copies of the Software, and to
   permit persons to whom the Software is furnished to do so, subject to
   the following conditions:

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED ``AS IS'', WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL CYGNUS SOLUTIONS BE LIABLE FOR ANY CLAIM, DAMAGES OR
   OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
   ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
   OTHER DEALINGS IN THE SOFTWARE.

   ----------------------------------------------------------------------- */

#ifndef LIBFFI_TARGET_H
#define LIBFFI_TARGET_H

/* ---- System specific configurations ----------------------------------- */

#if defined (X86_64) && defined (__i386__)
#undef X86_64
#define X86
#endif

/* ---- Generic type definitions ----------------------------------------- */

#ifndef LIBFFI_ASM
#ifndef _WIN64
typedef unsigned long          ffi_arg;
#else
typedef unsigned __int64       ffi_arg;
#endif
typedef signed long            ffi_sarg;

typedef enum ffi_abi {
  FFI_FIRST_ABI = 0,

  /* ---- Intel x86 Win32 ---------- */
  FFI_SYSV,
#ifndef _WIN64
  FFI_STDCALL,
#endif
  /* TODO: Add fastcall support for the sake of completeness */
  FFI_DEFAULT_ABI = FFI_SYSV,

  /* ---- Intel x86 and AMD x86-64 - */
/* #if !defined(X86_WIN32) && (defined(__i386__) || defined(__x86_64__)) */
/*   FFI_SYSV, */
/*   FFI_UNIX64,*/   /* Unix variants all use the same ABI for x86-64  */
/* #ifdef __i386__ */
/*   FFI_DEFAULT_ABI = FFI_SYSV, */
/* #else */
/*   FFI_DEFAULT_ABI = FFI_UNIX64, */
/* #endif */
/* #endif */

  FFI_LAST_ABI = FFI_DEFAULT_ABI + 1
} ffi_abi;
#endif

/* ---- Definitions for closures ----------------------------------------- */

#define FFI_CLOSURES 1

#ifdef _WIN64
#define FFI_TRAMPOLINE_SIZE 29
#define FFI_NATIVE_RAW_API 0
#else
#define FFI_TRAMPOLINE_SIZE 15
#define FFI_NATIVE_RAW_API 1	/* x86 has native raw api support */
#endif

#endif

