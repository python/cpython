/***********************************************************
Copyright 1991, 1992, 1993 by Stichting Mathematisch Centrum,
Amsterdam, The Netherlands.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Stichting Mathematisch
Centrum or CWI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior permission.

STICHTING MATHEMATISCH CENTRUM DISCLAIMS ALL WARRANTIES WITH REGARD TO
THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH CENTRUM BE LIABLE
FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

/* Python interpreter main program for frozen scripts */

#include "allobjects.h"

extern char *getenv();

extern int debugging;
extern int verbose;

main(argc, argv)
	int argc;
	char **argv;
{
	char *p;
	int n;
	if ((p = getenv("PYTHONDEBUG")) && *p != '\0')
		debugging = 1;
	if ((p = getenv("PYTHONVERBOSE")) && *p != '\0')
		verbose = 1;
	initargs(&argc, &argv); /* Defined in config*.c */
	initall();
	setpythonargv(argc, argv);
	n = init_frozen("__main__");
	if (n == 0)
		fatal("__main__ not frozen");
	if (n < 0) {
		print_error();
		goaway(1);
	}
	else
		goaway(0);
	/*NOTREACHED*/
}
