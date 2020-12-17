/* Internal declarations for getopt.
   Copyright (C) 1989-1994,1996-1999,2001,2003,2004
   Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

#ifndef _GETOPT_INT_H
#define _GETOPT_INT_H	1

extern int _getopt_internal (int ___argc, char **___argv,
			     const char *__shortopts,
		             const struct option *__longopts, int *__longind,
			     int __long_only, int __posixly_correct);


/* Reentrant versions which can handle parsing multiple argument
   vectors at the same time.  */

/* Data type for reentrant functions.  */
struct _getopt_data
{
  /* These have exactly the same meaning as the corresponding global
     variables, except that they are used for the reentrant
     versions of getopt.  */
  int optind;
  int opterr;
  int optopt;
  char *optarg;

  /* Internal members.  */

  /* True if the internal members have been initialized.  */
  int __initialized;

  /* The next char to be scanned in the option-element
     in which the last option character we returned was found.
     This allows us to pick up the scan where we left off.

     If this is zero, or a null string, it means resume the scan
     by advancing to the next ARGV-element.  */
  char *__nextchar;

  /* Describe how to deal with options that follow non-option ARGV-elements.

     If the caller did not specify anything,
     the default is REQUIRE_ORDER if the environment variable
     POSIXLY_CORRECT is defined, PERMUTE otherwise.

     REQUIRE_ORDER means don't recognize them as options;
     stop option processing when the first non-option is seen.
     This is what Unix does.
     This mode of operation is selected by either setting the environment
     variable POSIXLY_CORRECT, or using `+' as the first character
     of the list of option characters, or by calling getopt.

     PERMUTE is the default.  We permute the contents of ARGV as we
     scan, so that eventually all the non-options are at the end.
     This allows options to be given in any order, even with programs
     that were not written to expect this.

     RETURN_IN_ORDER is an option available to programs that were
     written to expect options and other ARGV-elements in any order
     and that care about the ordering of the two.  We describe each
     non-option ARGV-element as if it were the argument of an option
     with character code 1.  Using `-' as the first character of the
     list of option characters selects this mode of operation.

     The special argument `--' forces an end of option-scanning regardless
     of the value of `ordering'.  In the case of RETURN_IN_ORDER, only
     `--' can cause `getopt' to return -1 with `optind' != ARGC.  */

  enum
    {
      REQUIRE_ORDER, PERMUTE, RETURN_IN_ORDER
    } __ordering;

  /* If the POSIXLY_CORRECT environment variable is set
     or getopt was called.  */
  int __posixly_correct;


  /* Handle permutation of arguments.  */

  /* Describe the part of ARGV that contains non-options that have
     been skipped.  `first_nonopt' is the index in ARGV of the first
     of them; `last_nonopt' is the index after the last of them.  */

  int __first_nonopt;
  int __last_nonopt;

#if defined _LIBC && defined USE_NONOPTION_FLAGS
  int __nonoption_flags_max_len;
  int __nonoption_flags_len;
# endif
};

/* The initializer is necessary to set OPTIND and OPTERR to their
   default values and to clear the initialization flag.  */
#define _GETOPT_DATA_INITIALIZER	{ 1, 1 }

extern int _getopt_internal_r (int ___argc, char **___argv,
			       const char *__shortopts,
			       const struct option *__longopts, int *__longind,
			       int __long_only, int __posixly_correct,
			       struct _getopt_data *__data);

extern int _getopt_long_r (int ___argc, char **___argv,
			   const char *__shortopts,
			   const struct option *__longopts, int *__longind,
			   struct _getopt_data *__data);

extern int _getopt_long_only_r (int ___argc, char **___argv,
				const char *__shortopts,
				const struct option *__longopts,
				int *__longind,
				struct _getopt_data *__data);

#endif /* getopt_int.h */
