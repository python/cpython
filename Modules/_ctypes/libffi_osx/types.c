/* -----------------------------------------------------------------------
   types.c - Copyright (c) 1996, 1998  Red Hat, Inc.

   Predefined ffi_types needed by libffi.

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

#include <ffi.h>
#include <ffi_common.h>

/* Type definitions */
#define FFI_INTEGRAL_TYPEDEF(n, s, a, t)		\
	ffi_type ffi_type_##n = { s, a, t, NULL }
#define FFI_AGGREGATE_TYPEDEF(n, e)				\
	ffi_type ffi_type_##n = { 0, 0, FFI_TYPE_STRUCT, e }

FFI_INTEGRAL_TYPEDEF(uint8, 1, 1, FFI_TYPE_UINT8);
FFI_INTEGRAL_TYPEDEF(sint8, 1, 1, FFI_TYPE_SINT8);
FFI_INTEGRAL_TYPEDEF(uint16, 2, 2, FFI_TYPE_UINT16);
FFI_INTEGRAL_TYPEDEF(sint16, 2, 2, FFI_TYPE_SINT16);
FFI_INTEGRAL_TYPEDEF(uint32, 4, 4, FFI_TYPE_UINT32);
FFI_INTEGRAL_TYPEDEF(sint32, 4, 4, FFI_TYPE_SINT32);
FFI_INTEGRAL_TYPEDEF(float, 4, 4, FFI_TYPE_FLOAT);

/* Size and alignment are fake here. They must not be 0. */
FFI_INTEGRAL_TYPEDEF(void, 1, 1, FFI_TYPE_VOID);

#if defined ALPHA || defined SPARC64 || defined X86_64 ||	\
	defined S390X || defined IA64 || defined POWERPC64
FFI_INTEGRAL_TYPEDEF(pointer, 8, 8, FFI_TYPE_POINTER);
#else
FFI_INTEGRAL_TYPEDEF(pointer, 4, 4, FFI_TYPE_POINTER);
#endif

#if defined X86 || defined ARM || defined M68K || defined(X86_DARWIN)

#	ifdef X86_64
		FFI_INTEGRAL_TYPEDEF(uint64, 8, 8, FFI_TYPE_UINT64);
		FFI_INTEGRAL_TYPEDEF(sint64, 8, 8, FFI_TYPE_SINT64);
#	else
		FFI_INTEGRAL_TYPEDEF(uint64, 8, 4, FFI_TYPE_UINT64);
		FFI_INTEGRAL_TYPEDEF(sint64, 8, 4, FFI_TYPE_SINT64);
#	endif

#elif defined(POWERPC_DARWIN)
FFI_INTEGRAL_TYPEDEF(uint64, 8, 8, FFI_TYPE_UINT64);
FFI_INTEGRAL_TYPEDEF(sint64, 8, 8, FFI_TYPE_SINT64);
#elif defined SH
FFI_INTEGRAL_TYPEDEF(uint64, 8, 4, FFI_TYPE_UINT64);
FFI_INTEGRAL_TYPEDEF(sint64, 8, 4, FFI_TYPE_SINT64);
#else
FFI_INTEGRAL_TYPEDEF(uint64, 8, 8, FFI_TYPE_UINT64);
FFI_INTEGRAL_TYPEDEF(sint64, 8, 8, FFI_TYPE_SINT64);
#endif

#if defined X86 || defined X86_WIN32 || defined M68K || defined(X86_DARWIN)

#	if defined X86_WIN32 || defined X86_64
		FFI_INTEGRAL_TYPEDEF(double, 8, 8, FFI_TYPE_DOUBLE);
#	else
		FFI_INTEGRAL_TYPEDEF(double, 8, 4, FFI_TYPE_DOUBLE);
#	endif

#	ifdef X86_DARWIN
		FFI_INTEGRAL_TYPEDEF(longdouble, 16, 16, FFI_TYPE_LONGDOUBLE);
#	else
		FFI_INTEGRAL_TYPEDEF(longdouble, 12, 4, FFI_TYPE_LONGDOUBLE);
#	endif

#elif defined ARM || defined SH || defined POWERPC_AIX
FFI_INTEGRAL_TYPEDEF(double, 8, 4, FFI_TYPE_DOUBLE);
FFI_INTEGRAL_TYPEDEF(longdouble, 8, 4, FFI_TYPE_LONGDOUBLE);
#elif defined POWERPC_DARWIN
FFI_INTEGRAL_TYPEDEF(double, 8, 8, FFI_TYPE_DOUBLE);

#	if __GNUC__ >= 4
		FFI_INTEGRAL_TYPEDEF(longdouble, 16, 16, FFI_TYPE_LONGDOUBLE);
#	else
		FFI_INTEGRAL_TYPEDEF(longdouble, 8, 8, FFI_TYPE_LONGDOUBLE);
#	endif

#elif defined SPARC
FFI_INTEGRAL_TYPEDEF(double, 8, 8, FFI_TYPE_DOUBLE);

#	ifdef SPARC64
		FFI_INTEGRAL_TYPEDEF(longdouble, 16, 16, FFI_TYPE_LONGDOUBLE);
#	else
		FFI_INTEGRAL_TYPEDEF(longdouble, 16, 8, FFI_TYPE_LONGDOUBLE);
#	endif

#elif defined X86_64 || defined POWERPC64
FFI_INTEGRAL_TYPEDEF(double, 8, 8, FFI_TYPE_DOUBLE);
FFI_INTEGRAL_TYPEDEF(longdouble, 16, 16, FFI_TYPE_LONGDOUBLE);
#else
FFI_INTEGRAL_TYPEDEF(double, 8, 8, FFI_TYPE_DOUBLE);
FFI_INTEGRAL_TYPEDEF(longdouble, 8, 8, FFI_TYPE_LONGDOUBLE);
#endif