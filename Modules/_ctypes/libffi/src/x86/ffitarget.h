/* -----------------------------------------------------------------*-C-*-
   ffitarget.h - Copyright (c) 1996-2003, 2010  Red Hat, Inc.
   Copyright (C) 2008  Free Software Foundation, Inc.

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

#if defined (X86_64) && defined (__i386__)
#undef X86_64
#define X86
#endif

#ifdef X86_WIN64
#define FFI_SIZEOF_ARG 8
#define USE_BUILTIN_FFS 0 // not yet implemented in mingw-64 
#endif

/* ---- Generic type definitions ----------------------------------------- */

#ifndef LIBFFI_ASM
#ifdef X86_WIN64
#ifdef _MSC_VER
typedef unsigned __int64       ffi_arg;
typedef __int64                ffi_sarg;
#else
typedef unsigned long long     ffi_arg;
typedef long long              ffi_sarg;
#endif
#else
typedef unsigned long          ffi_arg;
typedef signed long            ffi_sarg;
#endif

typedef enum ffi_abi {
  FFI_FIRST_ABI = 0,

  /* ---- Intel x86 Win32 ---------- */
#ifdef X86_WIN32
  FFI_SYSV,
  FFI_STDCALL,
  /* TODO: Add fastcall support for the sake of completeness */
  FFI_DEFAULT_ABI = FFI_SYSV,
#endif

#ifdef X86_WIN64
  FFI_WIN64,
  FFI_DEFAULT_ABI = FFI_WIN64,
#else

  /* ---- Intel x86 and AMD x86-64 - */
#if !defined(X86_WIN32) && (defined(__i386__) || defined(__x86_64__) || defined(__i386) || defined(__amd64))
  FFI_SYSV,
  FFI_UNIX64,   /* Unix variants all use the same ABI for x86-64  */
#if defined(__i386__) || defined(__i386)
  FFI_DEFAULT_ABI = FFI_SYSV,
#else
  FFI_DEFAULT_ABI = FFI_UNIX64,
#endif
#endif
#endif /* X86_WIN64 */

  FFI_LAST_ABI = FFI_DEFAULT_ABI + 1
} ffi_abi;
#endif

/* ---- Definitions for closures ----------------------------------------- */

#define FFI_CLOSURES 1
#define FFI_TYPE_SMALL_STRUCT_1B (FFI_TYPE_LAST + 1)
#define FFI_TYPE_SMALL_STRUCT_2B (FFI_TYPE_LAST + 2)
#define FFI_TYPE_SMALL_STRUCT_4B (FFI_TYPE_LAST + 3)

#if defined (X86_64) || (defined (__x86_64__) && defined (X86_DARWIN))
#define FFI_TRAMPOLINE_SIZE 24
#define FFI_NATIVE_RAW_API 0
#else
#ifdef X86_WIN32
#define FFI_TRAMPOLINE_SIZE 13
#else
#ifdef X86_WIN64
#define FFI_TRAMPOLINE_SIZE 29
#define FFI_NATIVE_RAW_API 0
#define FFI_NO_RAW_API 1
#else
#define FFI_TRAMPOLINE_SIZE 10
#endif
#endif
#ifndef X86_WIN64
#define FFI_NATIVE_RAW_API 1	/* x86 has native raw api support */
#endif
#endif

#endif

