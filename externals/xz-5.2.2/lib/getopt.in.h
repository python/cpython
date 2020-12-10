/* Declarations for getopt.
   Copyright (C) 1989-1994,1996-1999,2001,2003,2004,2005,2006,2007
   Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation; either version 2.1, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License along
   with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

#ifndef _GETOPT_H

#ifndef __need_getopt
# define _GETOPT_H 1
#endif

/* Standalone applications should #define __GETOPT_PREFIX to an
   identifier that prefixes the external functions and variables
   defined in this header.  When this happens, include the
   headers that might declare getopt so that they will not cause
   confusion if included after this file.  Then systematically rename
   identifiers so that they do not collide with the system functions
   and variables.  Renaming avoids problems with some compilers and
   linkers.  */
#if defined __GETOPT_PREFIX && !defined __need_getopt
# include <stdlib.h>
# include <stdio.h>
# include <unistd.h>
# undef __need_getopt
# undef getopt
# undef getopt_long
# undef getopt_long_only
# undef optarg
# undef opterr
# undef optind
# undef optopt
# define __GETOPT_CONCAT(x, y) x ## y
# define __GETOPT_XCONCAT(x, y) __GETOPT_CONCAT (x, y)
# define __GETOPT_ID(y) __GETOPT_XCONCAT (__GETOPT_PREFIX, y)
# define getopt __GETOPT_ID (getopt)
# define getopt_long __GETOPT_ID (getopt_long)
# define getopt_long_only __GETOPT_ID (getopt_long_only)
# define optarg __GETOPT_ID (optarg)
# define opterr __GETOPT_ID (opterr)
# define optind __GETOPT_ID (optind)
# define optopt __GETOPT_ID (optopt)
#endif

/* Standalone applications get correct prototypes for getopt_long and
   getopt_long_only; they declare "char **argv".  libc uses prototypes
   with "char *const *argv" that are incorrect because getopt_long and
   getopt_long_only can permute argv; this is required for backward
   compatibility (e.g., for LSB 2.0.1).

   This used to be `#if defined __GETOPT_PREFIX && !defined __need_getopt',
   but it caused redefinition warnings if both unistd.h and getopt.h were
   included, since unistd.h includes getopt.h having previously defined
   __need_getopt.

   The only place where __getopt_argv_const is used is in definitions
   of getopt_long and getopt_long_only below, but these are visible
   only if __need_getopt is not defined, so it is quite safe to rewrite
   the conditional as follows:
*/
#if !defined __need_getopt
# if defined __GETOPT_PREFIX
#  define __getopt_argv_const /* empty */
# else
#  define __getopt_argv_const const
# endif
#endif

/* If __GNU_LIBRARY__ is not already defined, either we are being used
   standalone, or this is the first header included in the source file.
   If we are being used with glibc, we need to include <features.h>, but
   that does not exist if we are standalone.  So: if __GNU_LIBRARY__ is
   not defined, include <ctype.h>, which will pull in <features.h> for us
   if it's from glibc.  (Why ctype.h?  It's guaranteed to exist and it
   doesn't flood the namespace with stuff the way some other headers do.)  */
#if !defined __GNU_LIBRARY__
# include <ctype.h>
#endif

#ifndef __THROW
# ifndef __GNUC_PREREQ
#  define __GNUC_PREREQ(maj, min) (0)
# endif
# if defined __cplusplus && __GNUC_PREREQ (2,8)
#  define __THROW	throw ()
# else
#  define __THROW
# endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* For communication from `getopt' to the caller.
   When `getopt' finds an option that takes an argument,
   the argument value is returned here.
   Also, when `ordering' is RETURN_IN_ORDER,
   each non-option ARGV-element is returned here.  */

extern char *optarg;

/* Index in ARGV of the next element to be scanned.
   This is used for communication to and from the caller
   and for communication between successive calls to `getopt'.

   On entry to `getopt', zero means this is the first call; initialize.

   When `getopt' returns -1, this is the index of the first of the
   non-option elements that the caller should itself scan.

   Otherwise, `optind' communicates from one call to the next
   how much of ARGV has been scanned so far.  */

extern int optind;

/* Callers store zero here to inhibit the error message `getopt' prints
   for unrecognized options.  */

extern int opterr;

/* Set to an option character which was unrecognized.  */

extern int optopt;

#ifndef __need_getopt
/* Describe the long-named options requested by the application.
   The LONG_OPTIONS argument to getopt_long or getopt_long_only is a vector
   of `struct option' terminated by an element containing a name which is
   zero.

   The field `has_arg' is:
   no_argument		(or 0) if the option does not take an argument,
   required_argument	(or 1) if the option requires an argument,
   optional_argument	(or 2) if the option takes an optional argument.

   If the field `flag' is not NULL, it points to a variable that is set
   to the value given in the field `val' when the option is found, but
   left unchanged if the option is not found.

   To have a long-named option do something other than set an `int' to
   a compiled-in constant, such as set a value from `optarg', set the
   option's `flag' field to zero and its `val' field to a nonzero
   value (the equivalent single-letter option character, if there is
   one).  For long options that have a zero `flag' field, `getopt'
   returns the contents of the `val' field.  */

struct option
{
  const char *name;
  /* has_arg can't be an enum because some compilers complain about
     type mismatches in all the code that assumes it is an int.  */
  int has_arg;
  int *flag;
  int val;
};

/* Names for the values of the `has_arg' field of `struct option'.  */

# define no_argument		0
# define required_argument	1
# define optional_argument	2
#endif	/* need getopt */


/* Get definitions and prototypes for functions to process the
   arguments in ARGV (ARGC of them, minus the program name) for
   options given in OPTS.

   Return the option character from OPTS just read.  Return -1 when
   there are no more options.  For unrecognized options, or options
   missing arguments, `optopt' is set to the option letter, and '?' is
   returned.

   The OPTS string is a list of characters which are recognized option
   letters, optionally followed by colons, specifying that that letter
   takes an argument, to be placed in `optarg'.

   If a letter in OPTS is followed by two colons, its argument is
   optional.  This behavior is specific to the GNU `getopt'.

   The argument `--' causes premature termination of argument
   scanning, explicitly telling `getopt' that there are no more
   options.

   If OPTS begins with `-', then non-option arguments are treated as
   arguments to the option '\1'.  This behavior is specific to the GNU
   `getopt'.  If OPTS begins with `+', or POSIXLY_CORRECT is set in
   the environment, then do not permute arguments.  */

extern int getopt (int ___argc, char *const *___argv, const char *__shortopts)
       __THROW;

#ifndef __need_getopt
extern int getopt_long (int ___argc, char *__getopt_argv_const *___argv,
			const char *__shortopts,
		        const struct option *__longopts, int *__longind)
       __THROW;
extern int getopt_long_only (int ___argc, char *__getopt_argv_const *___argv,
			     const char *__shortopts,
		             const struct option *__longopts, int *__longind)
       __THROW;

#endif

#ifdef __cplusplus
}
#endif

/* Make sure we later can get all the definitions and declarations.  */
#undef __need_getopt

#endif /* getopt.h */
