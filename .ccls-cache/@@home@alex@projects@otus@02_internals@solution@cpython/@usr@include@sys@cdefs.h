/* Copyright (C) 1992-2022 Free Software Foundation, Inc.
   Copyright The GNU Toolchain Authors.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <https://www.gnu.org/licenses/>.  */

#ifndef	_SYS_CDEFS_H
#define	_SYS_CDEFS_H	1

/* We are almost always included from features.h. */
#ifndef _FEATURES_H
# include <features.h>
#endif

/* The GNU libc does not support any K&R compilers or the traditional mode
   of ISO C compilers anymore.  Check for some of the combinations not
   supported anymore.  */
#if defined __GNUC__ && !defined __STDC__ && !defined __cplusplus
# error "You need a ISO C or C++ conforming compiler to use the glibc headers"
#endif

/* Some user header file might have defined this before.  */
#undef	__P
#undef	__PMT

/* Compilers that lack __has_attribute may object to
       #if defined __has_attribute && __has_attribute (...)
   even though they do not need to evaluate the right-hand side of the &&.
   Similarly for __has_builtin, etc.  */
#if (defined __has_attribute \
     && (!defined __clang_minor__ \
         || 3 < __clang_major__ + (5 <= __clang_minor__)))
# define __glibc_has_attribute(attr) __has_attribute (attr)
#else
# define __glibc_has_attribute(attr) 0
#endif
#ifdef __has_builtin
# define __glibc_has_builtin(name) __has_builtin (name)
#else
# define __glibc_has_builtin(name) 0
#endif
#ifdef __has_extension
# define __glibc_has_extension(ext) __has_extension (ext)
#else
# define __glibc_has_extension(ext) 0
#endif

#if defined __GNUC__ || defined __clang__

/* All functions, except those with callbacks or those that
   synchronize memory, are leaf functions.  */
# if __GNUC_PREREQ (4, 6) && !defined _LIBC
#  define __LEAF , __leaf__
#  define __LEAF_ATTR __attribute__ ((__leaf__))
# else
#  define __LEAF
#  define __LEAF_ATTR
# endif

/* GCC can always grok prototypes.  For C++ programs we add throw()
   to help it optimize the function calls.  But this only works with
   gcc 2.8.x and egcs.  For gcc 3.4 and up we even mark C functions
   as non-throwing using a function attribute since programs can use
   the -fexceptions options for C code as well.  */
# if !defined __cplusplus \
     && (__GNUC_PREREQ (3, 4) || __glibc_has_attribute (__nothrow__))
#  define __THROW	__attribute__ ((__nothrow__ __LEAF))
#  define __THROWNL	__attribute__ ((__nothrow__))
#  define __NTH(fct)	__attribute__ ((__nothrow__ __LEAF)) fct
#  define __NTHNL(fct)  __attribute__ ((__nothrow__)) fct
# else
#  if defined __cplusplus && (__GNUC_PREREQ (2,8) || __clang_major >= 4)
#   if __cplusplus >= 201103L
#    define __THROW	noexcept (true)
#   else
#    define __THROW	throw ()
#   endif
#   define __THROWNL	__THROW
#   define __NTH(fct)	__LEAF_ATTR fct __THROW
#   define __NTHNL(fct) fct __THROW
#  else
#   define __THROW
#   define __THROWNL
#   define __NTH(fct)	fct
#   define __NTHNL(fct) fct
#  endif
# endif

#else	/* Not GCC or clang.  */

# if (defined __cplusplus						\
      || (defined __STDC_VERSION__ && __STDC_VERSION__ >= 199901L))
#  define __inline	inline
# else
#  define __inline		/* No inline functions.  */
# endif

# define __THROW
# define __THROWNL
# define __NTH(fct)	fct

#endif	/* GCC || clang.  */

/* These two macros are not used in glibc anymore.  They are kept here
   only because some other projects expect the macros to be defined.  */
#define __P(args)	args
#define __PMT(args)	args

/* For these things, GCC behaves the ANSI way normally,
   and the non-ANSI way under -traditional.  */

#define __CONCAT(x,y)	x ## y
#define __STRING(x)	#x

/* This is not a typedef so `const __ptr_t' does the right thing.  */
#define __ptr_t void *


/* C++ needs to know that types and declarations are C, not C++.  */
#ifdef	__cplusplus
# define __BEGIN_DECLS	extern "C" {
# define __END_DECLS	}
#else
# define __BEGIN_DECLS
# define __END_DECLS
#endif


/* Fortify support.  */
#define __bos(ptr) __builtin_object_size (ptr, __USE_FORTIFY_LEVEL > 1)
#define __bos0(ptr) __builtin_object_size (ptr, 0)

/* Use __builtin_dynamic_object_size at _FORTIFY_SOURCE=3 when available.  */
#if __USE_FORTIFY_LEVEL == 3 && (__glibc_clang_prereq (9, 0)		      \
				 || __GNUC_PREREQ (12, 0))
# define __glibc_objsize0(__o) __builtin_dynamic_object_size (__o, 0)
# define __glibc_objsize(__o) __builtin_dynamic_object_size (__o, 1)
#else
# define __glibc_objsize0(__o) __bos0 (__o)
# define __glibc_objsize(__o) __bos (__o)
#endif

/* Compile time conditions to choose between the regular, _chk and _chk_warn
   variants.  These conditions should get evaluated to constant and optimized
   away.  */

#define __glibc_safe_len_cond(__l, __s, __osz) ((__l) <= (__osz) / (__s))
#define __glibc_unsigned_or_positive(__l) \
  ((__typeof (__l)) 0 < (__typeof (__l)) -1				      \
   || (__builtin_constant_p (__l) && (__l) > 0))

/* Length is known to be safe at compile time if the __L * __S <= __OBJSZ
   condition can be folded to a constant and if it is true, or unknown (-1) */
#define __glibc_safe_or_unknown_len(__l, __s, __osz) \
  ((__builtin_constant_p (__osz) && (__osz) == (__SIZE_TYPE__) -1)	      \
   || (__glibc_unsigned_or_positive (__l)				      \
       && __builtin_constant_p (__glibc_safe_len_cond ((__SIZE_TYPE__) (__l), \
						       (__s), (__osz)))	      \
       && __glibc_safe_len_cond ((__SIZE_TYPE__) (__l), (__s), (__osz))))

/* Conversely, we know at compile time that the length is unsafe if the
   __L * __S <= __OBJSZ condition can be folded to a constant and if it is
   false.  */
#define __glibc_unsafe_len(__l, __s, __osz) \
  (__glibc_unsigned_or_positive (__l)					      \
   && __builtin_constant_p (__glibc_safe_len_cond ((__SIZE_TYPE__) (__l),     \
						   __s, __osz))		      \
   && !__glibc_safe_len_cond ((__SIZE_TYPE__) (__l), __s, __osz))

/* Fortify function f.  __f_alias, __f_chk and __f_chk_warn must be
   declared.  */

#define __glibc_fortify(f, __l, __s, __osz, ...) \
  (__glibc_safe_or_unknown_len (__l, __s, __osz)			      \
   ? __ ## f ## _alias (__VA_ARGS__)					      \
   : (__glibc_unsafe_len (__l, __s, __osz)				      \
      ? __ ## f ## _chk_warn (__VA_ARGS__, __osz)			      \
      : __ ## f ## _chk (__VA_ARGS__, __osz)))			      \

/* Fortify function f, where object size argument passed to f is the number of
   elements and not total size.  */

#define __glibc_fortify_n(f, __l, __s, __osz, ...) \
  (__glibc_safe_or_unknown_len (__l, __s, __osz)			      \
   ? __ ## f ## _alias (__VA_ARGS__)					      \
   : (__glibc_unsafe_len (__l, __s, __osz)				      \
      ? __ ## f ## _chk_warn (__VA_ARGS__, (__osz) / (__s))		      \
      : __ ## f ## _chk (__VA_ARGS__, (__osz) / (__s))))		      \

#if __GNUC_PREREQ (4,3)
# define __warnattr(msg) __attribute__((__warning__ (msg)))
# define __errordecl(name, msg) \
  extern void name (void) __attribute__((__error__ (msg)))
#else
# define __warnattr(msg)
# define __errordecl(name, msg) extern void name (void)
#endif

/* Support for flexible arrays.
   Headers that should use flexible arrays only if they're "real"
   (e.g. only if they won't affect sizeof()) should test
   #if __glibc_c99_flexarr_available.  */
#if defined __STDC_VERSION__ && __STDC_VERSION__ >= 199901L && !defined __HP_cc
# define __flexarr	[]
# define __glibc_c99_flexarr_available 1
#elif __GNUC_PREREQ (2,97) || defined __clang__
/* GCC 2.97 and clang support C99 flexible array members as an extension,
   even when in C89 mode or compiling C++ (any version).  */
# define __flexarr	[]
# define __glibc_c99_flexarr_available 1
#elif defined __GNUC__
/* Pre-2.97 GCC did not support C99 flexible arrays but did have
   an equivalent extension with slightly different notation.  */
# define __flexarr	[0]
# define __glibc_c99_flexarr_available 1
#else
/* Some other non-C99 compiler.  Approximate with [1].  */
# define __flexarr	[1]
# define __glibc_c99_flexarr_available 0
#endif


/* __asm__ ("xyz") is used throughout the headers to rename functions
   at the assembly language level.  This is wrapped by the __REDIRECT
   macro, in order to support compilers that can do this some other
   way.  When compilers don't support asm-names at all, we have to do
   preprocessor tricks instead (which don't have exactly the right
   semantics, but it's the best we can do).

   Example:
   int __REDIRECT(setpgrp, (__pid_t pid, __pid_t pgrp), setpgid); */

#if (defined __GNUC__ && __GNUC__ >= 2) || (__clang_major__ >= 4)

# define __REDIRECT(name, proto, alias) name proto __asm__ (__ASMNAME (#alias))
# ifdef __cplusplus
#  define __REDIRECT_NTH(name, proto, alias) \
     name proto __THROW __asm__ (__ASMNAME (#alias))
#  define __REDIRECT_NTHNL(name, proto, alias) \
     name proto __THROWNL __asm__ (__ASMNAME (#alias))
# else
#  define __REDIRECT_NTH(name, proto, alias) \
     name proto __asm__ (__ASMNAME (#alias)) __THROW
#  define __REDIRECT_NTHNL(name, proto, alias) \
     name proto __asm__ (__ASMNAME (#alias)) __THROWNL
# endif
# define __ASMNAME(cname)  __ASMNAME2 (__USER_LABEL_PREFIX__, cname)
# define __ASMNAME2(prefix, cname) __STRING (prefix) cname

/*
#elif __SOME_OTHER_COMPILER__

# define __REDIRECT(name, proto, alias) name proto; \
	_Pragma("let " #name " = " #alias)
*/
#endif

/* GCC and clang have various useful declarations that can be made with
   the '__attribute__' syntax.  All of the ways we use this do fine if
   they are omitted for compilers that don't understand it.  */
#if !(defined __GNUC__ || defined __clang__)
# define __attribute__(xyz)	/* Ignore */
#endif

/* At some point during the gcc 2.96 development the `malloc' attribute
   for functions was introduced.  We don't want to use it unconditionally
   (although this would be possible) since it generates warnings.  */
#if __GNUC_PREREQ (2,96) || __glibc_has_attribute (__malloc__)
# define __attribute_malloc__ __attribute__ ((__malloc__))
#else
# define __attribute_malloc__ /* Ignore */
#endif

/* Tell the compiler which arguments to an allocation function
   indicate the size of the allocation.  */
#if __GNUC_PREREQ (4, 3)
# define __attribute_alloc_size__(params) \
  __attribute__ ((__alloc_size__ params))
#else
# define __attribute_alloc_size__(params) /* Ignore.  */
#endif

/* Tell the compiler which argument to an allocation function
   indicates the alignment of the allocation.  */
#if __GNUC_PREREQ (4, 9) || __glibc_has_attribute (__alloc_align__)
# define __attribute_alloc_align__(param) \
  __attribute__ ((__alloc_align__ param))
#else
# define __attribute_alloc_align__(param) /* Ignore.  */
#endif

/* At some point during the gcc 2.96 development the `pure' attribute
   for functions was introduced.  We don't want to use it unconditionally
   (although this would be possible) since it generates warnings.  */
#if __GNUC_PREREQ (2,96) || __glibc_has_attribute (__pure__)
# define __attribute_pure__ __attribute__ ((__pure__))
#else
# define __attribute_pure__ /* Ignore */
#endif

/* This declaration tells the compiler that the value is constant.  */
#if __GNUC_PREREQ (2,5) || __glibc_has_attribute (__const__)
# define __attribute_const__ __attribute__ ((__const__))
#else
# define __attribute_const__ /* Ignore */
#endif

#if __GNUC_PREREQ (2,7) || __glibc_has_attribute (__unused__)
# define __attribute_maybe_unused__ __attribute__ ((__unused__))
#else
# define __attribute_maybe_unused__ /* Ignore */
#endif

/* At some point during the gcc 3.1 development the `used' attribute
   for functions was introduced.  We don't want to use it unconditionally
   (although this would be possible) since it generates warnings.  */
#if __GNUC_PREREQ (3,1) || __glibc_has_attribute (__used__)
# define __attribute_used__ __attribute__ ((__used__))
# define __attribute_noinline__ __attribute__ ((__noinline__))
#else
# define __attribute_used__ __attribute__ ((__unused__))
# define __attribute_noinline__ /* Ignore */
#endif

/* Since version 3.2, gcc allows marking deprecated functions.  */
#if __GNUC_PREREQ (3,2) || __glibc_has_attribute (__deprecated__)
# define __attribute_deprecated__ __attribute__ ((__deprecated__))
#else
# define __attribute_deprecated__ /* Ignore */
#endif

/* Since version 4.5, gcc also allows one to specify the message printed
   when a deprecated function is used.  clang claims to be gcc 4.2, but
   may also support this feature.  */
#if __GNUC_PREREQ (4,5) \
    || __glibc_has_extension (__attribute_deprecated_with_message__)
# define __attribute_deprecated_msg__(msg) \
	 __attribute__ ((__deprecated__ (msg)))
#else
# define __attribute_deprecated_msg__(msg) __attribute_deprecated__
#endif

/* At some point during the gcc 2.8 development the `format_arg' attribute
   for functions was introduced.  We don't want to use it unconditionally
   (although this would be possible) since it generates warnings.
   If several `format_arg' attributes are given for the same function, in
   gcc-3.0 and older, all but the last one are ignored.  In newer gccs,
   all designated arguments are considered.  */
#if __GNUC_PREREQ (2,8) || __glibc_has_attribute (__format_arg__)
# define __attribute_format_arg__(x) __attribute__ ((__format_arg__ (x)))
#else
# define __attribute_format_arg__(x) /* Ignore */
#endif

/* At some point during the gcc 2.97 development the `strfmon' format
   attribute for functions was introduced.  We don't want to use it
   unconditionally (although this would be possible) since it
   generates warnings.  */
#if __GNUC_PREREQ (2,97) || __glibc_has_attribute (__format__)
# define __attribute_format_strfmon__(a,b) \
  __attribute__ ((__format__ (__strfmon__, a, b)))
#else
# define __attribute_format_strfmon__(a,b) /* Ignore */
#endif

/* The nonnull function attribute marks pointer parameters that
   must not be NULL.  This has the name __nonnull in glibc,
   and __attribute_nonnull__ in files shared with Gnulib to avoid
   collision with a different __nonnull in DragonFlyBSD 5.9.  */
#ifndef __attribute_nonnull__
# if __GNUC_PREREQ (3,3) || __glibc_has_attribute (__nonnull__)
#  define __attribute_nonnull__(params) __attribute__ ((__nonnull__ params))
# else
#  define __attribute_nonnull__(params)
# endif
#endif
#ifndef __nonnull
# define __nonnull(params) __attribute_nonnull__ (params)
#endif

/* The returns_nonnull function attribute marks the return type of the function
   as always being non-null.  */
#ifndef __returns_nonnull
# if __GNUC_PREREQ (4, 9) || __glibc_has_attribute (__returns_nonnull__)
# define __returns_nonnull __attribute__ ((__returns_nonnull__))
# else
# define __returns_nonnull
# endif
#endif

/* If fortification mode, we warn about unused results of certain
   function calls which can lead to problems.  */
#if __GNUC_PREREQ (3,4) || __glibc_has_attribute (__warn_unused_result__)
# define __attribute_warn_unused_result__ \
   __attribute__ ((__warn_unused_result__))
# if defined __USE_FORTIFY_LEVEL && __USE_FORTIFY_LEVEL > 0
#  define __wur __attribute_warn_unused_result__
# endif
#else
# define __attribute_warn_unused_result__ /* empty */
#endif
#ifndef __wur
# define __wur /* Ignore */
#endif

/* Forces a function to be always inlined.  */
#if __GNUC_PREREQ (3,2) || __glibc_has_attribute (__always_inline__)
/* The Linux kernel defines __always_inline in stddef.h (283d7573), and
   it conflicts with this definition.  Therefore undefine it first to
   allow either header to be included first.  */
# undef __always_inline
# define __always_inline __inline __attribute__ ((__always_inline__))
#else
# undef __always_inline
# define __always_inline __inline
#endif

/* Associate error messages with the source location of the call site rather
   than with the source location inside the function.  */
#if __GNUC_PREREQ (4,3) || __glibc_has_attribute (__artificial__)
# define __attribute_artificial__ __attribute__ ((__artificial__))
#else
# define __attribute_artificial__ /* Ignore */
#endif

/* GCC 4.3 and above with -std=c99 or -std=gnu99 implements ISO C99
   inline semantics, unless -fgnu89-inline is used.  Using __GNUC_STDC_INLINE__
   or __GNUC_GNU_INLINE is not a good enough check for gcc because gcc versions
   older than 4.3 may define these macros and still not guarantee GNU inlining
   semantics.

   clang++ identifies itself as gcc-4.2, but has support for GNU inlining
   semantics, that can be checked for by using the __GNUC_STDC_INLINE_ and
   __GNUC_GNU_INLINE__ macro definitions.  */
#if (!defined __cplusplus || __GNUC_PREREQ (4,3) \
     || (defined __clang__ && (defined __GNUC_STDC_INLINE__ \
			       || defined __GNUC_GNU_INLINE__)))
# if defined __GNUC_STDC_INLINE__ || defined __cplusplus
#  define __extern_inline extern __inline __attribute__ ((__gnu_inline__))
#  define __extern_always_inline \
  extern __always_inline __attribute__ ((__gnu_inline__))
# else
#  define __extern_inline extern __inline
#  define __extern_always_inline extern __always_inline
# endif
#endif

#ifdef __extern_always_inline
# define __fortify_function __extern_always_inline __attribute_artificial__
#endif

/* GCC 4.3 and above allow passing all anonymous arguments of an
   __extern_always_inline function to some other vararg function.  */
#if __GNUC_PREREQ (4,3)
# define __va_arg_pack() __builtin_va_arg_pack ()
# define __va_arg_pack_len() __builtin_va_arg_pack_len ()
#endif

/* It is possible to compile containing GCC extensions even if GCC is
   run in pedantic mode if the uses are carefully marked using the
   `__extension__' keyword.  But this is not generally available before
   version 2.8.  */
#if !(__GNUC_PREREQ (2,8) || defined __clang__)
# define __extension__		/* Ignore */
#endif

/* __restrict is known in EGCS 1.2 and above, and in clang.
   It works also in C++ mode (outside of arrays), but only when spelled
   as '__restrict', not 'restrict'.  */
#if !(__GNUC_PREREQ (2,92) || __clang_major__ >= 3)
# if defined __STDC_VERSION__ && __STDC_VERSION__ >= 199901L
#  define __restrict	restrict
# else
#  define __restrict	/* Ignore */
# endif
#endif

/* ISO C99 also allows to declare arrays as non-overlapping.  The syntax is
     array_name[restrict]
   GCC 3.1 and clang support this.
   This syntax is not usable in C++ mode.  */
#if (__GNUC_PREREQ (3,1) || __clang_major__ >= 3) && !defined __cplusplus
# define __restrict_arr	__restrict
#else
# ifdef __GNUC__
#  define __restrict_arr	/* Not supported in old GCC.  */
# else
#  if defined __STDC_VERSION__ && __STDC_VERSION__ >= 199901L
#   define __restrict_arr	restrict
#  else
/* Some other non-C99 compiler.  */
#   define __restrict_arr	/* Not supported.  */
#  endif
# endif
#endif

#if (__GNUC__ >= 3) || __glibc_has_builtin (__builtin_expect)
# define __glibc_unlikely(cond)	__builtin_expect ((cond), 0)
# define __glibc_likely(cond)	__builtin_expect ((cond), 1)
#else
# define __glibc_unlikely(cond)	(cond)
# define __glibc_likely(cond)	(cond)
#endif

#if (!defined _Noreturn \
     && (defined __STDC_VERSION__ ? __STDC_VERSION__ : 0) < 201112 \
     &&  !(__GNUC_PREREQ (4,7) \
           || (3 < __clang_major__ + (5 <= __clang_minor__))))
# if __GNUC_PREREQ (2,8)
#  define _Noreturn __attribute__ ((__noreturn__))
# else
#  define _Noreturn
# endif
#endif

#if __GNUC_PREREQ (8, 0)
/* Describes a char array whose address can safely be passed as the first
   argument to strncpy and strncat, as the char array is not necessarily
   a NUL-terminated string.  */
# define __attribute_nonstring__ __attribute__ ((__nonstring__))
#else
# define __attribute_nonstring__
#endif

/* Undefine (also defined in libc-symbols.h).  */
#undef __attribute_copy__
#if __GNUC_PREREQ (9, 0)
/* Copies attributes from the declaration or type referenced by
   the argument.  */
# define __attribute_copy__(arg) __attribute__ ((__copy__ (arg)))
#else
# define __attribute_copy__(arg)
#endif

#if (!defined _Static_assert && !defined __cplusplus \
     && (defined __STDC_VERSION__ ? __STDC_VERSION__ : 0) < 201112 \
     && (!(__GNUC_PREREQ (4, 6) || __clang_major__ >= 4) \
         || defined __STRICT_ANSI__))
# define _Static_assert(expr, diagnostic) \
    extern int (*__Static_assert_function (void)) \
      [!!sizeof (struct { int __error_if_negative: (expr) ? 2 : -1; })]
#endif

/* Gnulib avoids including these, as they don't work on non-glibc or
   older glibc platforms.  */
#ifndef __GNULIB_CDEFS
# include <bits/wordsize.h>
# include <bits/long-double.h>
#endif

#if __LDOUBLE_REDIRECTS_TO_FLOAT128_ABI == 1
# ifdef __REDIRECT

/* Alias name defined automatically.  */
#  define __LDBL_REDIR(name, proto) ... unused__ldbl_redir
#  define __LDBL_REDIR_DECL(name) \
  extern __typeof (name) name __asm (__ASMNAME ("__" #name "ieee128"));

/* Alias name defined automatically, with leading underscores.  */
#  define __LDBL_REDIR2_DECL(name) \
  extern __typeof (__##name) __##name \
    __asm (__ASMNAME ("__" #name "ieee128"));

/* Alias name defined manually.  */
#  define __LDBL_REDIR1(name, proto, alias) ... unused__ldbl_redir1
#  define __LDBL_REDIR1_DECL(name, alias) \
  extern __typeof (name) name __asm (__ASMNAME (#alias));

#  define __LDBL_REDIR1_NTH(name, proto, alias) \
  __REDIRECT_NTH (name, proto, alias)
#  define __REDIRECT_NTH_LDBL(name, proto, alias) \
  __LDBL_REDIR1_NTH (name, proto, __##alias##ieee128)

/* Unused.  */
#  define __REDIRECT_LDBL(name, proto, alias) ... unused__redirect_ldbl
#  define __LDBL_REDIR_NTH(name, proto) ... unused__ldbl_redir_nth

# else
_Static_assert (0, "IEEE 128-bits long double requires redirection on this platform");
# endif
#elif defined __LONG_DOUBLE_MATH_OPTIONAL && defined __NO_LONG_DOUBLE_MATH
# define __LDBL_COMPAT 1
# ifdef __REDIRECT
#  define __LDBL_REDIR1(name, proto, alias) __REDIRECT (name, proto, alias)
#  define __LDBL_REDIR(name, proto) \
  __LDBL_REDIR1 (name, proto, __nldbl_##name)
#  define __LDBL_REDIR1_NTH(name, proto, alias) __REDIRECT_NTH (name, proto, alias)
#  define __LDBL_REDIR_NTH(name, proto) \
  __LDBL_REDIR1_NTH (name, proto, __nldbl_##name)
#  define __LDBL_REDIR2_DECL(name) \
  extern __typeof (__##name) __##name __asm (__ASMNAME ("__nldbl___" #name));
#  define __LDBL_REDIR1_DECL(name, alias) \
  extern __typeof (name) name __asm (__ASMNAME (#alias));
#  define __LDBL_REDIR_DECL(name) \
  extern __typeof (name) name __asm (__ASMNAME ("__nldbl_" #name));
#  define __REDIRECT_LDBL(name, proto, alias) \
  __LDBL_REDIR1 (name, proto, __nldbl_##alias)
#  define __REDIRECT_NTH_LDBL(name, proto, alias) \
  __LDBL_REDIR1_NTH (name, proto, __nldbl_##alias)
# endif
#endif
#if (!defined __LDBL_COMPAT && __LDOUBLE_REDIRECTS_TO_FLOAT128_ABI == 0) \
    || !defined __REDIRECT
# define __LDBL_REDIR1(name, proto, alias) name proto
# define __LDBL_REDIR(name, proto) name proto
# define __LDBL_REDIR1_NTH(name, proto, alias) name proto __THROW
# define __LDBL_REDIR_NTH(name, proto) name proto __THROW
# define __LDBL_REDIR2_DECL(name)
# define __LDBL_REDIR_DECL(name)
# ifdef __REDIRECT
#  define __REDIRECT_LDBL(name, proto, alias) __REDIRECT (name, proto, alias)
#  define __REDIRECT_NTH_LDBL(name, proto, alias) \
  __REDIRECT_NTH (name, proto, alias)
# endif
#endif

/* __glibc_macro_warning (MESSAGE) issues warning MESSAGE.  This is
   intended for use in preprocessor macros.

   Note: MESSAGE must be a _single_ string; concatenation of string
   literals is not supported.  */
#if __GNUC_PREREQ (4,8) || __glibc_clang_prereq (3,5)
# define __glibc_macro_warning1(message) _Pragma (#message)
# define __glibc_macro_warning(message) \
  __glibc_macro_warning1 (GCC warning message)
#else
# define __glibc_macro_warning(msg)
#endif

/* Generic selection (ISO C11) is a C-only feature, available in GCC
   since version 4.9.  Previous versions do not provide generic
   selection, even though they might set __STDC_VERSION__ to 201112L,
   when in -std=c11 mode.  Thus, we must check for !defined __GNUC__
   when testing __STDC_VERSION__ for generic selection support.
   On the other hand, Clang also defines __GNUC__, so a clang-specific
   check is required to enable the use of generic selection.  */
#if !defined __cplusplus \
    && (__GNUC_PREREQ (4, 9) \
	|| __glibc_has_extension (c_generic_selections) \
	|| (!defined __GNUC__ && defined __STDC_VERSION__ \
	    && __STDC_VERSION__ >= 201112L))
# define __HAVE_GENERIC_SELECTION 1
#else
# define __HAVE_GENERIC_SELECTION 0
#endif

#if __GNUC_PREREQ (10, 0)
/* Designates a 1-based positional argument ref-index of pointer type
   that can be used to access size-index elements of the pointed-to
   array according to access mode, or at least one element when
   size-index is not provided:
     access (access-mode, <ref-index> [, <size-index>])  */
#  define __attr_access(x) __attribute__ ((__access__ x))
/* For _FORTIFY_SOURCE == 3 we use __builtin_dynamic_object_size, which may
   use the access attribute to get object sizes from function definition
   arguments, so we can't use them on functions we fortify.  Drop the object
   size hints for such functions.  */
#  if __USE_FORTIFY_LEVEL == 3
#    define __fortified_attr_access(a, o, s) __attribute__ ((__access__ (a, o)))
#  else
#    define __fortified_attr_access(a, o, s) __attr_access ((a, o, s))
#  endif
#  if __GNUC_PREREQ (11, 0)
#    define __attr_access_none(argno) __attribute__ ((__access__ (__none__, argno)))
#  else
#    define __attr_access_none(argno)
#  endif
#else
#  define __fortified_attr_access(a, o, s)
#  define __attr_access(x)
#  define __attr_access_none(argno)
#endif

#if __GNUC_PREREQ (11, 0)
/* Designates dealloc as a function to call to deallocate objects
   allocated by the declared function.  */
# define __attr_dealloc(dealloc, argno) \
    __attribute__ ((__malloc__ (dealloc, argno)))
# define __attr_dealloc_free __attr_dealloc (__builtin_free, 1)
#else
# define __attr_dealloc(dealloc, argno)
# define __attr_dealloc_free
#endif

/* Specify that a function such as setjmp or vfork may return
   twice.  */
#if __GNUC_PREREQ (4, 1)
# define __attribute_returns_twice__ __attribute__ ((__returns_twice__))
#else
# define __attribute_returns_twice__ /* Ignore.  */
#endif

#endif	 /* sys/cdefs.h */
