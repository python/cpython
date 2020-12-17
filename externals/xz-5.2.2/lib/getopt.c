/* Getopt for GNU.
   NOTE: getopt is now part of the C library, so if you don't know what
   "Keep this file name-space clean" means, talk to drepper@gnu.org
   before changing it!
   Copyright (C) 1987,88,89,90,91,92,93,94,95,96,98,99,2000,2001,2002,2003,2004,2006
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

#ifndef _LIBC
# include <config.h>
#endif

#include "getopt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef __VMS
# include <unixlib.h>
#endif

/* Completely disable NLS for getopt. We won't include translations for it
   anyway. If the system lacks getopt_long, missing translations probably
   aren't a problem.  */
/*
#ifdef _LIBC
# include <libintl.h>
#else
# include "gettext.h"
# define _(msgid) gettext (msgid)
#endif
*/
#define _(msgid) (msgid)

#if defined _LIBC && defined USE_IN_LIBIO
# include <wchar.h>
#endif

#ifndef attribute_hidden
# define attribute_hidden
#endif

/* Unlike standard Unix `getopt', functions like `getopt_long'
   let the user intersperse the options with the other arguments.

   As `getopt_long' works, it permutes the elements of ARGV so that,
   when it is done, all the options precede everything else.  Thus
   all application programs are extended to handle flexible argument order.

   Using `getopt' or setting the environment variable POSIXLY_CORRECT
   disables permutation.
   Then the application's behavior is completely standard.

   GNU application programs can use a third alternative mode in which
   they can distinguish the relative order of options and other arguments.  */

#include "getopt_int.h"

/* For communication from `getopt' to the caller.
   When `getopt' finds an option that takes an argument,
   the argument value is returned here.
   Also, when `ordering' is RETURN_IN_ORDER,
   each non-option ARGV-element is returned here.  */

char *optarg;

/* Index in ARGV of the next element to be scanned.
   This is used for communication to and from the caller
   and for communication between successive calls to `getopt'.

   On entry to `getopt', zero means this is the first call; initialize.

   When `getopt' returns -1, this is the index of the first of the
   non-option elements that the caller should itself scan.

   Otherwise, `optind' communicates from one call to the next
   how much of ARGV has been scanned so far.  */

/* 1003.2 says this must be 1 before any call.  */
int optind = 1;

/* Callers store zero here to inhibit the error message
   for unrecognized options.  */

int opterr = 1;

/* Set to an option character which was unrecognized.
   This must be initialized on some systems to avoid linking in the
   system's own getopt implementation.  */

int optopt = '?';

/* Keep a global copy of all internal members of getopt_data.  */

static struct _getopt_data getopt_data;


#if defined HAVE_DECL_GETENV && !HAVE_DECL_GETENV
extern char *getenv ();
#endif

#ifdef _LIBC
/* Stored original parameters.
   XXX This is no good solution.  We should rather copy the args so
   that we can compare them later.  But we must not use malloc(3).  */
extern int __libc_argc;
extern char **__libc_argv;

/* Bash 2.0 gives us an environment variable containing flags
   indicating ARGV elements that should not be considered arguments.  */

# ifdef USE_NONOPTION_FLAGS
/* Defined in getopt_init.c  */
extern char *__getopt_nonoption_flags;
# endif

# ifdef USE_NONOPTION_FLAGS
#  define SWAP_FLAGS(ch1, ch2) \
  if (d->__nonoption_flags_len > 0)					      \
    {									      \
      char __tmp = __getopt_nonoption_flags[ch1];			      \
      __getopt_nonoption_flags[ch1] = __getopt_nonoption_flags[ch2];	      \
      __getopt_nonoption_flags[ch2] = __tmp;				      \
    }
# else
#  define SWAP_FLAGS(ch1, ch2)
# endif
#else	/* !_LIBC */
# define SWAP_FLAGS(ch1, ch2)
#endif	/* _LIBC */

/* Exchange two adjacent subsequences of ARGV.
   One subsequence is elements [first_nonopt,last_nonopt)
   which contains all the non-options that have been skipped so far.
   The other is elements [last_nonopt,optind), which contains all
   the options processed since those non-options were skipped.

   `first_nonopt' and `last_nonopt' are relocated so that they describe
   the new indices of the non-options in ARGV after they are moved.  */

static void
exchange (char **argv, struct _getopt_data *d)
{
  int bottom = d->__first_nonopt;
  int middle = d->__last_nonopt;
  int top = d->optind;
  char *tem;

  /* Exchange the shorter segment with the far end of the longer segment.
     That puts the shorter segment into the right place.
     It leaves the longer segment in the right place overall,
     but it consists of two parts that need to be swapped next.  */

#if defined _LIBC && defined USE_NONOPTION_FLAGS
  /* First make sure the handling of the `__getopt_nonoption_flags'
     string can work normally.  Our top argument must be in the range
     of the string.  */
  if (d->__nonoption_flags_len > 0 && top >= d->__nonoption_flags_max_len)
    {
      /* We must extend the array.  The user plays games with us and
	 presents new arguments.  */
      char *new_str = malloc (top + 1);
      if (new_str == NULL)
	d->__nonoption_flags_len = d->__nonoption_flags_max_len = 0;
      else
	{
	  memset (__mempcpy (new_str, __getopt_nonoption_flags,
			     d->__nonoption_flags_max_len),
		  '\0', top + 1 - d->__nonoption_flags_max_len);
	  d->__nonoption_flags_max_len = top + 1;
	  __getopt_nonoption_flags = new_str;
	}
    }
#endif

  while (top > middle && middle > bottom)
    {
      if (top - middle > middle - bottom)
	{
	  /* Bottom segment is the short one.  */
	  int len = middle - bottom;
	  register int i;

	  /* Swap it with the top part of the top segment.  */
	  for (i = 0; i < len; i++)
	    {
	      tem = argv[bottom + i];
	      argv[bottom + i] = argv[top - (middle - bottom) + i];
	      argv[top - (middle - bottom) + i] = tem;
	      SWAP_FLAGS (bottom + i, top - (middle - bottom) + i);
	    }
	  /* Exclude the moved bottom segment from further swapping.  */
	  top -= len;
	}
      else
	{
	  /* Top segment is the short one.  */
	  int len = top - middle;
	  register int i;

	  /* Swap it with the bottom part of the bottom segment.  */
	  for (i = 0; i < len; i++)
	    {
	      tem = argv[bottom + i];
	      argv[bottom + i] = argv[middle + i];
	      argv[middle + i] = tem;
	      SWAP_FLAGS (bottom + i, middle + i);
	    }
	  /* Exclude the moved top segment from further swapping.  */
	  bottom += len;
	}
    }

  /* Update records for the slots the non-options now occupy.  */

  d->__first_nonopt += (d->optind - d->__last_nonopt);
  d->__last_nonopt = d->optind;
}

/* Initialize the internal data when the first call is made.  */

static const char *
_getopt_initialize (int argc, char **argv, const char *optstring,
		    int posixly_correct, struct _getopt_data *d)
{
  /* Start processing options with ARGV-element 1 (since ARGV-element 0
     is the program name); the sequence of previously skipped
     non-option ARGV-elements is empty.  */

  d->__first_nonopt = d->__last_nonopt = d->optind;

  d->__nextchar = NULL;

  d->__posixly_correct = posixly_correct || !!getenv ("POSIXLY_CORRECT");

  /* Determine how to handle the ordering of options and nonoptions.  */

  if (optstring[0] == '-')
    {
      d->__ordering = RETURN_IN_ORDER;
      ++optstring;
    }
  else if (optstring[0] == '+')
    {
      d->__ordering = REQUIRE_ORDER;
      ++optstring;
    }
  else if (d->__posixly_correct)
    d->__ordering = REQUIRE_ORDER;
  else
    d->__ordering = PERMUTE;

#if defined _LIBC && defined USE_NONOPTION_FLAGS
  if (!d->__posixly_correct
      && argc == __libc_argc && argv == __libc_argv)
    {
      if (d->__nonoption_flags_max_len == 0)
	{
	  if (__getopt_nonoption_flags == NULL
	      || __getopt_nonoption_flags[0] == '\0')
	    d->__nonoption_flags_max_len = -1;
	  else
	    {
	      const char *orig_str = __getopt_nonoption_flags;
	      int len = d->__nonoption_flags_max_len = strlen (orig_str);
	      if (d->__nonoption_flags_max_len < argc)
		d->__nonoption_flags_max_len = argc;
	      __getopt_nonoption_flags =
		(char *) malloc (d->__nonoption_flags_max_len);
	      if (__getopt_nonoption_flags == NULL)
		d->__nonoption_flags_max_len = -1;
	      else
		memset (__mempcpy (__getopt_nonoption_flags, orig_str, len),
			'\0', d->__nonoption_flags_max_len - len);
	    }
	}
      d->__nonoption_flags_len = d->__nonoption_flags_max_len;
    }
  else
    d->__nonoption_flags_len = 0;
#endif

  return optstring;
}

/* Scan elements of ARGV (whose length is ARGC) for option characters
   given in OPTSTRING.

   If an element of ARGV starts with '-', and is not exactly "-" or "--",
   then it is an option element.  The characters of this element
   (aside from the initial '-') are option characters.  If `getopt'
   is called repeatedly, it returns successively each of the option characters
   from each of the option elements.

   If `getopt' finds another option character, it returns that character,
   updating `optind' and `nextchar' so that the next call to `getopt' can
   resume the scan with the following option character or ARGV-element.

   If there are no more option characters, `getopt' returns -1.
   Then `optind' is the index in ARGV of the first ARGV-element
   that is not an option.  (The ARGV-elements have been permuted
   so that those that are not options now come last.)

   OPTSTRING is a string containing the legitimate option characters.
   If an option character is seen that is not listed in OPTSTRING,
   return '?' after printing an error message.  If you set `opterr' to
   zero, the error message is suppressed but we still return '?'.

   If a char in OPTSTRING is followed by a colon, that means it wants an arg,
   so the following text in the same ARGV-element, or the text of the following
   ARGV-element, is returned in `optarg'.  Two colons mean an option that
   wants an optional arg; if there is text in the current ARGV-element,
   it is returned in `optarg', otherwise `optarg' is set to zero.

   If OPTSTRING starts with `-' or `+', it requests different methods of
   handling the non-option ARGV-elements.
   See the comments about RETURN_IN_ORDER and REQUIRE_ORDER, above.

   Long-named options begin with `--' instead of `-'.
   Their names may be abbreviated as long as the abbreviation is unique
   or is an exact match for some defined option.  If they have an
   argument, it follows the option name in the same ARGV-element, separated
   from the option name by a `=', or else the in next ARGV-element.
   When `getopt' finds a long-named option, it returns 0 if that option's
   `flag' field is nonzero, the value of the option's `val' field
   if the `flag' field is zero.

   LONGOPTS is a vector of `struct option' terminated by an
   element containing a name which is zero.

   LONGIND returns the index in LONGOPT of the long-named option found.
   It is only valid when a long-named option has been found by the most
   recent call.

   If LONG_ONLY is nonzero, '-' as well as '--' can introduce
   long-named options.

   If POSIXLY_CORRECT is nonzero, behave as if the POSIXLY_CORRECT
   environment variable were set.  */

int
_getopt_internal_r (int argc, char **argv, const char *optstring,
		    const struct option *longopts, int *longind,
		    int long_only, int posixly_correct, struct _getopt_data *d)
{
  int print_errors = d->opterr;
  if (optstring[0] == ':')
    print_errors = 0;

  if (argc < 1)
    return -1;

  d->optarg = NULL;

  if (d->optind == 0 || !d->__initialized)
    {
      if (d->optind == 0)
	d->optind = 1;	/* Don't scan ARGV[0], the program name.  */
      optstring = _getopt_initialize (argc, argv, optstring,
				      posixly_correct, d);
      d->__initialized = 1;
    }

  /* Test whether ARGV[optind] points to a non-option argument.
     Either it does not have option syntax, or there is an environment flag
     from the shell indicating it is not an option.  The later information
     is only used when the used in the GNU libc.  */
#if defined _LIBC && defined USE_NONOPTION_FLAGS
# define NONOPTION_P (argv[d->optind][0] != '-' || argv[d->optind][1] == '\0' \
		      || (d->optind < d->__nonoption_flags_len		      \
			  && __getopt_nonoption_flags[d->optind] == '1'))
#else
# define NONOPTION_P (argv[d->optind][0] != '-' || argv[d->optind][1] == '\0')
#endif

  if (d->__nextchar == NULL || *d->__nextchar == '\0')
    {
      /* Advance to the next ARGV-element.  */

      /* Give FIRST_NONOPT & LAST_NONOPT rational values if OPTIND has been
	 moved back by the user (who may also have changed the arguments).  */
      if (d->__last_nonopt > d->optind)
	d->__last_nonopt = d->optind;
      if (d->__first_nonopt > d->optind)
	d->__first_nonopt = d->optind;

      if (d->__ordering == PERMUTE)
	{
	  /* If we have just processed some options following some non-options,
	     exchange them so that the options come first.  */

	  if (d->__first_nonopt != d->__last_nonopt
	      && d->__last_nonopt != d->optind)
	    exchange ((char **) argv, d);
	  else if (d->__last_nonopt != d->optind)
	    d->__first_nonopt = d->optind;

	  /* Skip any additional non-options
	     and extend the range of non-options previously skipped.  */

	  while (d->optind < argc && NONOPTION_P)
	    d->optind++;
	  d->__last_nonopt = d->optind;
	}

      /* The special ARGV-element `--' means premature end of options.
	 Skip it like a null option,
	 then exchange with previous non-options as if it were an option,
	 then skip everything else like a non-option.  */

      if (d->optind != argc && !strcmp (argv[d->optind], "--"))
	{
	  d->optind++;

	  if (d->__first_nonopt != d->__last_nonopt
	      && d->__last_nonopt != d->optind)
	    exchange ((char **) argv, d);
	  else if (d->__first_nonopt == d->__last_nonopt)
	    d->__first_nonopt = d->optind;
	  d->__last_nonopt = argc;

	  d->optind = argc;
	}

      /* If we have done all the ARGV-elements, stop the scan
	 and back over any non-options that we skipped and permuted.  */

      if (d->optind == argc)
	{
	  /* Set the next-arg-index to point at the non-options
	     that we previously skipped, so the caller will digest them.  */
	  if (d->__first_nonopt != d->__last_nonopt)
	    d->optind = d->__first_nonopt;
	  return -1;
	}

      /* If we have come to a non-option and did not permute it,
	 either stop the scan or describe it to the caller and pass it by.  */

      if (NONOPTION_P)
	{
	  if (d->__ordering == REQUIRE_ORDER)
	    return -1;
	  d->optarg = argv[d->optind++];
	  return 1;
	}

      /* We have found another option-ARGV-element.
	 Skip the initial punctuation.  */

      d->__nextchar = (argv[d->optind] + 1
		  + (longopts != NULL && argv[d->optind][1] == '-'));
    }

  /* Decode the current option-ARGV-element.  */

  /* Check whether the ARGV-element is a long option.

     If long_only and the ARGV-element has the form "-f", where f is
     a valid short option, don't consider it an abbreviated form of
     a long option that starts with f.  Otherwise there would be no
     way to give the -f short option.

     On the other hand, if there's a long option "fubar" and
     the ARGV-element is "-fu", do consider that an abbreviation of
     the long option, just like "--fu", and not "-f" with arg "u".

     This distinction seems to be the most useful approach.  */

  if (longopts != NULL
      && (argv[d->optind][1] == '-'
	  || (long_only && (argv[d->optind][2]
			    || !strchr (optstring, argv[d->optind][1])))))
    {
      char *nameend;
      const struct option *p;
      const struct option *pfound = NULL;
      int exact = 0;
      int ambig = 0;
      int indfound = -1;
      int option_index;

      for (nameend = d->__nextchar; *nameend && *nameend != '='; nameend++)
	/* Do nothing.  */ ;

      /* Test all long options for either exact match
	 or abbreviated matches.  */
      for (p = longopts, option_index = 0; p->name; p++, option_index++)
	if (!strncmp (p->name, d->__nextchar, nameend - d->__nextchar))
	  {
	    if ((unsigned int) (nameend - d->__nextchar)
		== (unsigned int) strlen (p->name))
	      {
		/* Exact match found.  */
		pfound = p;
		indfound = option_index;
		exact = 1;
		break;
	      }
	    else if (pfound == NULL)
	      {
		/* First nonexact match found.  */
		pfound = p;
		indfound = option_index;
	      }
	    else if (long_only
		     || pfound->has_arg != p->has_arg
		     || pfound->flag != p->flag
		     || pfound->val != p->val)
	      /* Second or later nonexact match found.  */
	      ambig = 1;
	  }

      if (ambig && !exact)
	{
	  if (print_errors)
	    {
#if defined _LIBC && defined USE_IN_LIBIO
	      char *buf;

	      if (__asprintf (&buf, _("%s: option `%s' is ambiguous\n"),
			      argv[0], argv[d->optind]) >= 0)
		{
		  _IO_flockfile (stderr);

		  int old_flags2 = ((_IO_FILE *) stderr)->_flags2;
		  ((_IO_FILE *) stderr)->_flags2 |= _IO_FLAGS2_NOTCANCEL;

		  __fxprintf (NULL, "%s", buf);

		  ((_IO_FILE *) stderr)->_flags2 = old_flags2;
		  _IO_funlockfile (stderr);

		  free (buf);
		}
#else
	      fprintf (stderr, _("%s: option `%s' is ambiguous\n"),
		       argv[0], argv[d->optind]);
#endif
	    }
	  d->__nextchar += strlen (d->__nextchar);
	  d->optind++;
	  d->optopt = 0;
	  return '?';
	}

      if (pfound != NULL)
	{
	  option_index = indfound;
	  d->optind++;
	  if (*nameend)
	    {
	      /* Don't test has_arg with >, because some C compilers don't
		 allow it to be used on enums.  */
	      if (pfound->has_arg)
		d->optarg = nameend + 1;
	      else
		{
		  if (print_errors)
		    {
#if defined _LIBC && defined USE_IN_LIBIO
		      char *buf;
		      int n;
#endif

		      if (argv[d->optind - 1][1] == '-')
			{
			  /* --option */
#if defined _LIBC && defined USE_IN_LIBIO
			  n = __asprintf (&buf, _("\
%s: option `--%s' doesn't allow an argument\n"),
					  argv[0], pfound->name);
#else
			  fprintf (stderr, _("\
%s: option `--%s' doesn't allow an argument\n"),
				   argv[0], pfound->name);
#endif
			}
		      else
			{
			  /* +option or -option */
#if defined _LIBC && defined USE_IN_LIBIO
			  n = __asprintf (&buf, _("\
%s: option `%c%s' doesn't allow an argument\n"),
					  argv[0], argv[d->optind - 1][0],
					  pfound->name);
#else
			  fprintf (stderr, _("\
%s: option `%c%s' doesn't allow an argument\n"),
				   argv[0], argv[d->optind - 1][0],
				   pfound->name);
#endif
			}

#if defined _LIBC && defined USE_IN_LIBIO
		      if (n >= 0)
			{
			  _IO_flockfile (stderr);

			  int old_flags2 = ((_IO_FILE *) stderr)->_flags2;
			  ((_IO_FILE *) stderr)->_flags2
			    |= _IO_FLAGS2_NOTCANCEL;

			  __fxprintf (NULL, "%s", buf);

			  ((_IO_FILE *) stderr)->_flags2 = old_flags2;
			  _IO_funlockfile (stderr);

			  free (buf);
			}
#endif
		    }

		  d->__nextchar += strlen (d->__nextchar);

		  d->optopt = pfound->val;
		  return '?';
		}
	    }
	  else if (pfound->has_arg == 1)
	    {
	      if (d->optind < argc)
		d->optarg = argv[d->optind++];
	      else
		{
		  if (print_errors)
		    {
#if defined _LIBC && defined USE_IN_LIBIO
		      char *buf;

		      if (__asprintf (&buf, _("\
%s: option `%s' requires an argument\n"),
				      argv[0], argv[d->optind - 1]) >= 0)
			{
			  _IO_flockfile (stderr);

			  int old_flags2 = ((_IO_FILE *) stderr)->_flags2;
			  ((_IO_FILE *) stderr)->_flags2
			    |= _IO_FLAGS2_NOTCANCEL;

			  __fxprintf (NULL, "%s", buf);

			  ((_IO_FILE *) stderr)->_flags2 = old_flags2;
			  _IO_funlockfile (stderr);

			  free (buf);
			}
#else
		      fprintf (stderr,
			       _("%s: option `%s' requires an argument\n"),
			       argv[0], argv[d->optind - 1]);
#endif
		    }
		  d->__nextchar += strlen (d->__nextchar);
		  d->optopt = pfound->val;
		  return optstring[0] == ':' ? ':' : '?';
		}
	    }
	  d->__nextchar += strlen (d->__nextchar);
	  if (longind != NULL)
	    *longind = option_index;
	  if (pfound->flag)
	    {
	      *(pfound->flag) = pfound->val;
	      return 0;
	    }
	  return pfound->val;
	}

      /* Can't find it as a long option.  If this is not getopt_long_only,
	 or the option starts with '--' or is not a valid short
	 option, then it's an error.
	 Otherwise interpret it as a short option.  */
      if (!long_only || argv[d->optind][1] == '-'
	  || strchr (optstring, *d->__nextchar) == NULL)
	{
	  if (print_errors)
	    {
#if defined _LIBC && defined USE_IN_LIBIO
	      char *buf;
	      int n;
#endif

	      if (argv[d->optind][1] == '-')
		{
		  /* --option */
#if defined _LIBC && defined USE_IN_LIBIO
		  n = __asprintf (&buf, _("%s: unrecognized option `--%s'\n"),
				  argv[0], d->__nextchar);
#else
		  fprintf (stderr, _("%s: unrecognized option `--%s'\n"),
			   argv[0], d->__nextchar);
#endif
		}
	      else
		{
		  /* +option or -option */
#if defined _LIBC && defined USE_IN_LIBIO
		  n = __asprintf (&buf, _("%s: unrecognized option `%c%s'\n"),
				  argv[0], argv[d->optind][0], d->__nextchar);
#else
		  fprintf (stderr, _("%s: unrecognized option `%c%s'\n"),
			   argv[0], argv[d->optind][0], d->__nextchar);
#endif
		}

#if defined _LIBC && defined USE_IN_LIBIO
	      if (n >= 0)
		{
		  _IO_flockfile (stderr);

		  int old_flags2 = ((_IO_FILE *) stderr)->_flags2;
		  ((_IO_FILE *) stderr)->_flags2 |= _IO_FLAGS2_NOTCANCEL;

		  __fxprintf (NULL, "%s", buf);

		  ((_IO_FILE *) stderr)->_flags2 = old_flags2;
		  _IO_funlockfile (stderr);

		  free (buf);
		}
#endif
	    }
	  d->__nextchar = (char *) "";
	  d->optind++;
	  d->optopt = 0;
	  return '?';
	}
    }

  /* Look at and handle the next short option-character.  */

  {
    char c = *d->__nextchar++;
    char *temp = strchr (optstring, c);

    /* Increment `optind' when we start to process its last character.  */
    if (*d->__nextchar == '\0')
      ++d->optind;

    if (temp == NULL || c == ':')
      {
	if (print_errors)
	  {
#if defined _LIBC && defined USE_IN_LIBIO
	      char *buf;
	      int n;
#endif

	    if (d->__posixly_correct)
	      {
		/* 1003.2 specifies the format of this message.  */
#if defined _LIBC && defined USE_IN_LIBIO
		n = __asprintf (&buf, _("%s: illegal option -- %c\n"),
				argv[0], c);
#else
		fprintf (stderr, _("%s: illegal option -- %c\n"), argv[0], c);
#endif
	      }
	    else
	      {
#if defined _LIBC && defined USE_IN_LIBIO
		n = __asprintf (&buf, _("%s: invalid option -- %c\n"),
				argv[0], c);
#else
		fprintf (stderr, _("%s: invalid option -- %c\n"), argv[0], c);
#endif
	      }

#if defined _LIBC && defined USE_IN_LIBIO
	    if (n >= 0)
	      {
		_IO_flockfile (stderr);

		int old_flags2 = ((_IO_FILE *) stderr)->_flags2;
		((_IO_FILE *) stderr)->_flags2 |= _IO_FLAGS2_NOTCANCEL;

		__fxprintf (NULL, "%s", buf);

		((_IO_FILE *) stderr)->_flags2 = old_flags2;
		_IO_funlockfile (stderr);

		free (buf);
	      }
#endif
	  }
	d->optopt = c;
	return '?';
      }
    /* Convenience. Treat POSIX -W foo same as long option --foo */
    if (temp[0] == 'W' && temp[1] == ';')
      {
	char *nameend;
	const struct option *p;
	const struct option *pfound = NULL;
	int exact = 0;
	int ambig = 0;
	int indfound = 0;
	int option_index;

	/* This is an option that requires an argument.  */
	if (*d->__nextchar != '\0')
	  {
	    d->optarg = d->__nextchar;
	    /* If we end this ARGV-element by taking the rest as an arg,
	       we must advance to the next element now.  */
	    d->optind++;
	  }
	else if (d->optind == argc)
	  {
	    if (print_errors)
	      {
		/* 1003.2 specifies the format of this message.  */
#if defined _LIBC && defined USE_IN_LIBIO
		char *buf;

		if (__asprintf (&buf,
				_("%s: option requires an argument -- %c\n"),
				argv[0], c) >= 0)
		  {
		    _IO_flockfile (stderr);

		    int old_flags2 = ((_IO_FILE *) stderr)->_flags2;
		    ((_IO_FILE *) stderr)->_flags2 |= _IO_FLAGS2_NOTCANCEL;

		    __fxprintf (NULL, "%s", buf);

		    ((_IO_FILE *) stderr)->_flags2 = old_flags2;
		    _IO_funlockfile (stderr);

		    free (buf);
		  }
#else
		fprintf (stderr, _("%s: option requires an argument -- %c\n"),
			 argv[0], c);
#endif
	      }
	    d->optopt = c;
	    if (optstring[0] == ':')
	      c = ':';
	    else
	      c = '?';
	    return c;
	  }
	else
	  /* We already incremented `d->optind' once;
	     increment it again when taking next ARGV-elt as argument.  */
	  d->optarg = argv[d->optind++];

	/* optarg is now the argument, see if it's in the
	   table of longopts.  */

	for (d->__nextchar = nameend = d->optarg; *nameend && *nameend != '=';
	     nameend++)
	  /* Do nothing.  */ ;

	/* Test all long options for either exact match
	   or abbreviated matches.  */
	for (p = longopts, option_index = 0; p->name; p++, option_index++)
	  if (!strncmp (p->name, d->__nextchar, nameend - d->__nextchar))
	    {
	      if ((unsigned int) (nameend - d->__nextchar) == strlen (p->name))
		{
		  /* Exact match found.  */
		  pfound = p;
		  indfound = option_index;
		  exact = 1;
		  break;
		}
	      else if (pfound == NULL)
		{
		  /* First nonexact match found.  */
		  pfound = p;
		  indfound = option_index;
		}
	      else
		/* Second or later nonexact match found.  */
		ambig = 1;
	    }
	if (ambig && !exact)
	  {
	    if (print_errors)
	      {
#if defined _LIBC && defined USE_IN_LIBIO
		char *buf;

		if (__asprintf (&buf, _("%s: option `-W %s' is ambiguous\n"),
				argv[0], argv[d->optind]) >= 0)
		  {
		    _IO_flockfile (stderr);

		    int old_flags2 = ((_IO_FILE *) stderr)->_flags2;
		    ((_IO_FILE *) stderr)->_flags2 |= _IO_FLAGS2_NOTCANCEL;

		    __fxprintf (NULL, "%s", buf);

		    ((_IO_FILE *) stderr)->_flags2 = old_flags2;
		    _IO_funlockfile (stderr);

		    free (buf);
		  }
#else
		fprintf (stderr, _("%s: option `-W %s' is ambiguous\n"),
			 argv[0], argv[d->optind]);
#endif
	      }
	    d->__nextchar += strlen (d->__nextchar);
	    d->optind++;
	    return '?';
	  }
	if (pfound != NULL)
	  {
	    option_index = indfound;
	    if (*nameend)
	      {
		/* Don't test has_arg with >, because some C compilers don't
		   allow it to be used on enums.  */
		if (pfound->has_arg)
		  d->optarg = nameend + 1;
		else
		  {
		    if (print_errors)
		      {
#if defined _LIBC && defined USE_IN_LIBIO
			char *buf;

			if (__asprintf (&buf, _("\
%s: option `-W %s' doesn't allow an argument\n"),
					argv[0], pfound->name) >= 0)
			  {
			    _IO_flockfile (stderr);

			    int old_flags2 = ((_IO_FILE *) stderr)->_flags2;
			    ((_IO_FILE *) stderr)->_flags2
			      |= _IO_FLAGS2_NOTCANCEL;

			    __fxprintf (NULL, "%s", buf);

			    ((_IO_FILE *) stderr)->_flags2 = old_flags2;
			    _IO_funlockfile (stderr);

			    free (buf);
			  }
#else
			fprintf (stderr, _("\
%s: option `-W %s' doesn't allow an argument\n"),
				 argv[0], pfound->name);
#endif
		      }

		    d->__nextchar += strlen (d->__nextchar);
		    return '?';
		  }
	      }
	    else if (pfound->has_arg == 1)
	      {
		if (d->optind < argc)
		  d->optarg = argv[d->optind++];
		else
		  {
		    if (print_errors)
		      {
#if defined _LIBC && defined USE_IN_LIBIO
			char *buf;

			if (__asprintf (&buf, _("\
%s: option `%s' requires an argument\n"),
					argv[0], argv[d->optind - 1]) >= 0)
			  {
			    _IO_flockfile (stderr);

			    int old_flags2 = ((_IO_FILE *) stderr)->_flags2;
			    ((_IO_FILE *) stderr)->_flags2
			      |= _IO_FLAGS2_NOTCANCEL;

			    __fxprintf (NULL, "%s", buf);

			    ((_IO_FILE *) stderr)->_flags2 = old_flags2;
			    _IO_funlockfile (stderr);

			    free (buf);
			  }
#else
			fprintf (stderr,
				 _("%s: option `%s' requires an argument\n"),
				 argv[0], argv[d->optind - 1]);
#endif
		      }
		    d->__nextchar += strlen (d->__nextchar);
		    return optstring[0] == ':' ? ':' : '?';
		  }
	      }
	    d->__nextchar += strlen (d->__nextchar);
	    if (longind != NULL)
	      *longind = option_index;
	    if (pfound->flag)
	      {
		*(pfound->flag) = pfound->val;
		return 0;
	      }
	    return pfound->val;
	  }
	  d->__nextchar = NULL;
	  return 'W';	/* Let the application handle it.   */
      }
    if (temp[1] == ':')
      {
	if (temp[2] == ':')
	  {
	    /* This is an option that accepts an argument optionally.  */
	    if (*d->__nextchar != '\0')
	      {
		d->optarg = d->__nextchar;
		d->optind++;
	      }
	    else
	      d->optarg = NULL;
	    d->__nextchar = NULL;
	  }
	else
	  {
	    /* This is an option that requires an argument.  */
	    if (*d->__nextchar != '\0')
	      {
		d->optarg = d->__nextchar;
		/* If we end this ARGV-element by taking the rest as an arg,
		   we must advance to the next element now.  */
		d->optind++;
	      }
	    else if (d->optind == argc)
	      {
		if (print_errors)
		  {
		    /* 1003.2 specifies the format of this message.  */
#if defined _LIBC && defined USE_IN_LIBIO
		    char *buf;

		    if (__asprintf (&buf, _("\
%s: option requires an argument -- %c\n"),
				    argv[0], c) >= 0)
		      {
			_IO_flockfile (stderr);

			int old_flags2 = ((_IO_FILE *) stderr)->_flags2;
			((_IO_FILE *) stderr)->_flags2 |= _IO_FLAGS2_NOTCANCEL;

			__fxprintf (NULL, "%s", buf);

			((_IO_FILE *) stderr)->_flags2 = old_flags2;
			_IO_funlockfile (stderr);

			free (buf);
		      }
#else
		    fprintf (stderr,
			     _("%s: option requires an argument -- %c\n"),
			     argv[0], c);
#endif
		  }
		d->optopt = c;
		if (optstring[0] == ':')
		  c = ':';
		else
		  c = '?';
	      }
	    else
	      /* We already incremented `optind' once;
		 increment it again when taking next ARGV-elt as argument.  */
	      d->optarg = argv[d->optind++];
	    d->__nextchar = NULL;
	  }
      }
    return c;
  }
}

int
_getopt_internal (int argc, char **argv, const char *optstring,
		  const struct option *longopts, int *longind,
		  int long_only, int posixly_correct)
{
  int result;

  getopt_data.optind = optind;
  getopt_data.opterr = opterr;

  result = _getopt_internal_r (argc, argv, optstring, longopts, longind,
			       long_only, posixly_correct, &getopt_data);

  optind = getopt_data.optind;
  optarg = getopt_data.optarg;
  optopt = getopt_data.optopt;

  return result;
}

/* glibc gets a LSB-compliant getopt.
   Standalone applications get a POSIX-compliant getopt.  */
#if _LIBC
enum { POSIXLY_CORRECT = 0 };
#else
enum { POSIXLY_CORRECT = 1 };
#endif

int
getopt (int argc, char *const *argv, const char *optstring)
{
  return _getopt_internal (argc, (char **) argv, optstring, NULL, NULL, 0,
			   POSIXLY_CORRECT);
}


#ifdef TEST

/* Compile with -DTEST to make an executable for use in testing
   the above definition of `getopt'.  */

int
main (int argc, char **argv)
{
  int c;
  int digit_optind = 0;

  while (1)
    {
      int this_option_optind = optind ? optind : 1;

      c = getopt (argc, argv, "abc:d:0123456789");
      if (c == -1)
	break;

      switch (c)
	{
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	  if (digit_optind != 0 && digit_optind != this_option_optind)
	    printf ("digits occur in two different argv-elements.\n");
	  digit_optind = this_option_optind;
	  printf ("option %c\n", c);
	  break;

	case 'a':
	  printf ("option a\n");
	  break;

	case 'b':
	  printf ("option b\n");
	  break;

	case 'c':
	  printf ("option c with value `%s'\n", optarg);
	  break;

	case '?':
	  break;

	default:
	  printf ("?? getopt returned character code 0%o ??\n", c);
	}
    }

  if (optind < argc)
    {
      printf ("non-option ARGV-elements: ");
      while (optind < argc)
	printf ("%s ", argv[optind++]);
      printf ("\n");
    }

  exit (0);
}

#endif /* TEST */
