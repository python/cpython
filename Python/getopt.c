/*---------------------------------------------------------------------------*
 * <RCS keywords>
 *
 * C++ Library
 *
 * Copyright 1992-1994, David Gottner
 *
 *                    All Rights Reserved
 *
 * Permission to use, copy, modify, and distribute this software and its 
 * documentation for any purpose and without fee is hereby granted, 
 * provided that the above copyright notice, this permission notice and
 * the following disclaimer notice appear unmodified in all copies.
 *
 * I DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.  IN NO EVENT SHALL I
 * BE LIABLE FOR ANY SPECIAL, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA, OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Nevertheless, I would like to know about bugs in this library or
 * suggestions for improvment.  Send bug reports and feedback to
 * davegottner@delphi.com.
 *---------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>

#define bool	int
#ifndef TRUE
#define TRUE	1
#endif
#ifndef FALSE
#define FALSE	0
#endif

bool    opterr = TRUE;          /* generate error messages */
int     optind = 1;             /* index into argv array   */
char *  optarg = NULL;          /* optional argument       */


#ifndef __BEOS__
int getopt(int argc, char *argv[], char optstring[])
#else
int getopt(int argc, char *const *argv, const char *optstring)
#endif
{
	static   char *opt_ptr = "";
	register char *ptr;
			 int   option;

	if (*opt_ptr == '\0') {

		if (optind >= argc || argv[optind][0] != '-' ||
		    argv[optind][1] == '\0' /* lone dash */ )
			return -1;

		else if (strcmp(argv[optind], "--") == 0) {
			++optind;
			return -1;
		}

		opt_ptr = &argv[optind++][1]; 
	}

	if ( (option = *opt_ptr++) == '\0')
	  return -1;
	
	if ((ptr = strchr(optstring, option)) == NULL) {
		if (opterr)
			fprintf(stderr, "Unknown option: -%c\n", option);

		return '?';
	}

	if (*(ptr + 1) == ':') {
		if (*opt_ptr != '\0') {
			optarg  = opt_ptr;
			opt_ptr = "";
		}

		else {
			if (optind >= argc) {
				if (opterr)
					fprintf(stderr,
			    "Argument expected for the -%c option\n", option);
				return '?';
			}

			optarg = argv[optind++];
		}
	}

	return option;
}
