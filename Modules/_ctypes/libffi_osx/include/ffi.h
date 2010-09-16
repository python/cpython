/* -----------------------------------------------------------------*-C-*-
   libffi PyOBJC - Copyright (c) 1996-2003  Red Hat, Inc.

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

/* -------------------------------------------------------------------
   The basic API is described in the README file.

   The raw API is designed to bypass some of the argument packing
   and unpacking on architectures for which it can be avoided.

   The closure API allows interpreted functions to be packaged up
   inside a C function pointer, so that they can be called as C functions,
   with no understanding on the client side that they are interpreted.
   It can also be used in other cases in which it is necessary to package
   up a user specified parameter and a function pointer as a single
   function pointer.

   The closure API must be implemented in order to get its functionality,
   e.g. for use by gij.  Routines are provided to emulate the raw API
   if the underlying platform doesn't allow faster implementation.

   More details on the raw and closure API can be found in:

   http://gcc.gnu.org/ml/java/1999-q3/msg00138.html

   and

   http://gcc.gnu.org/ml/java/1999-q3/msg00174.html
   -------------------------------------------------------------------- */

#ifndef LIBFFI_H
#define LIBFFI_H

#ifdef __cplusplus
extern "C" {
#endif

/*	Specify which architecture libffi is configured for. */
#ifdef MACOSX
#	if defined(__i386__) || defined(__x86_64__)
#		define X86_DARWIN
#	elif defined(__ppc__) || defined(__ppc64__)
#		define POWERPC_DARWIN
#	else
#	error "Unsupported MacOS X CPU type"
#	endif
#else
#error "Unsupported OS type"
#endif

/* ---- System configuration information --------------------------------- */

#include "ffitarget.h"
#include "fficonfig.h"

#ifndef LIBFFI_ASM

#include <stddef.h>
#include <limits.h>

/*	LONG_LONG_MAX is not always defined (not if STRICT_ANSI, for example).
	But we can find it either under the correct ANSI name, or under GNU
	C's internal name.  */
#ifdef LONG_LONG_MAX
#	define FFI_LONG_LONG_MAX LONG_LONG_MAX
#else
#	ifdef LLONG_MAX
#		define FFI_LONG_LONG_MAX LLONG_MAX
#	else
#		ifdef __GNUC__
#			define FFI_LONG_LONG_MAX __LONG_LONG_MAX__
#		endif
#	endif
#endif

#if SCHAR_MAX == 127
#	define ffi_type_uchar	ffi_type_uint8
#	define ffi_type_schar	ffi_type_sint8
#else
#error "char size not supported"
#endif

#if SHRT_MAX == 32767
#	define ffi_type_ushort	ffi_type_uint16
#	define ffi_type_sshort	ffi_type_sint16
#elif SHRT_MAX == 2147483647
#	define ffi_type_ushort	ffi_type_uint32
#	define ffi_type_sshort	ffi_type_sint32
#else
#error "short size not supported"
#endif

#if INT_MAX == 32767
#	define ffi_type_uint	ffi_type_uint16
#	define ffi_type_sint	ffi_type_sint16
#elif INT_MAX == 2147483647
#	define ffi_type_uint	ffi_type_uint32
#	define ffi_type_sint	ffi_type_sint32
#elif INT_MAX == 9223372036854775807
#	define ffi_type_uint	ffi_type_uint64
#	define ffi_type_sint	ffi_type_sint64
#else
#error "int size not supported"
#endif

#define ffi_type_ulong	ffi_type_uint64
#define ffi_type_slong	ffi_type_sint64

#if LONG_MAX == 2147483647
#	if FFI_LONG_LONG_MAX != 9223372036854775807
#		error "no 64-bit data type supported"
#	endif
#elif LONG_MAX != 9223372036854775807
#error "long size not supported"
#endif

/*	The closure code assumes that this works on pointers, i.e. a size_t
	can hold a pointer.	*/

typedef struct _ffi_type {
			size_t				size;
			unsigned short		alignment;
			unsigned short		type;
/*@null@*/	struct _ffi_type**	elements;
} ffi_type;

/*	These are defined in types.c */
extern ffi_type	ffi_type_void;
extern ffi_type	ffi_type_uint8;
extern ffi_type	ffi_type_sint8;
extern ffi_type	ffi_type_uint16;
extern ffi_type	ffi_type_sint16;
extern ffi_type	ffi_type_uint32;
extern ffi_type	ffi_type_sint32;
extern ffi_type	ffi_type_uint64;
extern ffi_type	ffi_type_sint64;
extern ffi_type	ffi_type_float;
extern ffi_type	ffi_type_double;
extern ffi_type	ffi_type_longdouble;
extern ffi_type	ffi_type_pointer;

typedef enum ffi_status {
	FFI_OK = 0,
	FFI_BAD_TYPEDEF,
	FFI_BAD_ABI
} ffi_status;

typedef unsigned	FFI_TYPE;

typedef struct	ffi_cif {
				ffi_abi		abi;
				unsigned	nargs;
/*@dependent@*/	ffi_type**	arg_types;
/*@dependent@*/	ffi_type*	rtype;
				unsigned	bytes;
				unsigned	flags;
#ifdef FFI_EXTRA_CIF_FIELDS
				FFI_EXTRA_CIF_FIELDS;
#endif
} ffi_cif;

/* ---- Definitions for the raw API -------------------------------------- */

#ifndef FFI_SIZEOF_ARG
#	if LONG_MAX == 2147483647
#		define FFI_SIZEOF_ARG	4
#	elif LONG_MAX == 9223372036854775807
#		define FFI_SIZEOF_ARG	8
#	endif
#endif

typedef union {
	ffi_sarg	sint;
	ffi_arg		uint;
	float		flt;
	char		data[FFI_SIZEOF_ARG];
	void*		ptr;
} ffi_raw;

void
ffi_raw_call(
/*@dependent@*/	ffi_cif*	cif, 
				void		(*fn)(void), 
/*@out@*/		void*		rvalue, 
/*@dependent@*/	ffi_raw*	avalue);

void
ffi_ptrarray_to_raw(
	ffi_cif*	cif,
	void**		args,
	ffi_raw*	raw);

void
ffi_raw_to_ptrarray(
	ffi_cif*	cif,
	ffi_raw*	raw,
	void**		args);

size_t
ffi_raw_size(
	ffi_cif*	cif);

/*	This is analogous to the raw API, except it uses Java parameter
	packing, even on 64-bit machines.  I.e. on 64-bit machines
	longs and doubles are followed by an empty 64-bit word.	*/
void
ffi_java_raw_call(
/*@dependent@*/	ffi_cif*	cif, 
				void		(*fn)(void), 
/*@out@*/		void*		rvalue, 
/*@dependent@*/	ffi_raw*	avalue);

void
ffi_java_ptrarray_to_raw(
	ffi_cif*	cif,
	void**		args,
	ffi_raw*	raw);

void
ffi_java_raw_to_ptrarray(
	ffi_cif*	cif,
	ffi_raw*	raw,
	void**		args);

size_t
ffi_java_raw_size(
	ffi_cif*	cif);

/* ---- Definitions for closures ----------------------------------------- */

#if FFI_CLOSURES

typedef struct ffi_closure {
	char		tramp[FFI_TRAMPOLINE_SIZE];
	ffi_cif*	cif;
	void		(*fun)(ffi_cif*,void*,void**,void*);
	void*		user_data;
} ffi_closure;

ffi_status
ffi_prep_closure(
	ffi_closure*	closure,
	ffi_cif*		cif,
	void			(*fun)(ffi_cif*,void*,void**,void*),
	void*			user_data);

void ffi_closure_free(void *);
void *ffi_closure_alloc (size_t size, void **code);

typedef struct ffi_raw_closure {
	char		tramp[FFI_TRAMPOLINE_SIZE];
	ffi_cif*	cif;

#if !FFI_NATIVE_RAW_API
	/*	if this is enabled, then a raw closure has the same layout 
		as a regular closure.  We use this to install an intermediate 
		handler to do the transaltion, void** -> ffi_raw*. */
	void	(*translate_args)(ffi_cif*,void*,void**,void*);
	void*	this_closure;
#endif

	void	(*fun)(ffi_cif*,void*,ffi_raw*,void*);
	void*	user_data;
} ffi_raw_closure;

ffi_status
ffi_prep_raw_closure(
	ffi_raw_closure*	closure,
	ffi_cif*			cif,
	void				(*fun)(ffi_cif*,void*,ffi_raw*,void*),
	void*				user_data);

ffi_status
ffi_prep_java_raw_closure(
	ffi_raw_closure*	closure,
	ffi_cif*			cif,
	void				(*fun)(ffi_cif*,void*,ffi_raw*,void*),
	void*				user_data);

#endif	// FFI_CLOSURES

/* ---- Public interface definition -------------------------------------- */

ffi_status
ffi_prep_cif(
/*@out@*/ /*@partial@*/					ffi_cif*		cif, 
										ffi_abi			abi,
										unsigned int	nargs, 
/*@dependent@*/ /*@out@*/ /*@partial@*/	ffi_type*		rtype, 
/*@dependent@*/							ffi_type**		atypes);

void
ffi_call(
/*@dependent@*/	ffi_cif*	cif, 
				void		(*fn)(void), 
/*@out@*/		void*		rvalue, 
/*@dependent@*/	void**		avalue);

/* Useful for eliminating compiler warnings */
#define FFI_FN(f) ((void (*)(void))f)

#endif	// #ifndef LIBFFI_ASM
/* ---- Definitions shared with assembly code ---------------------------- */

/*	If these change, update src/mips/ffitarget.h. */
#define FFI_TYPE_VOID       0
#define FFI_TYPE_INT        1
#define FFI_TYPE_FLOAT      2
#define FFI_TYPE_DOUBLE     3

#ifdef HAVE_LONG_DOUBLE
#	define FFI_TYPE_LONGDOUBLE 4
#else
#	define FFI_TYPE_LONGDOUBLE FFI_TYPE_DOUBLE
#endif

#define FFI_TYPE_UINT8      5
#define FFI_TYPE_SINT8      6
#define FFI_TYPE_UINT16     7
#define FFI_TYPE_SINT16     8
#define FFI_TYPE_UINT32     9
#define FFI_TYPE_SINT32     10
#define FFI_TYPE_UINT64     11
#define FFI_TYPE_SINT64     12
#define FFI_TYPE_STRUCT     13
#define FFI_TYPE_POINTER    14

/*	This should always refer to the last type code (for sanity checks) */
#define FFI_TYPE_LAST       FFI_TYPE_POINTER

#ifdef __cplusplus
}
#endif

#endif	// #ifndef LIBFFI_H
