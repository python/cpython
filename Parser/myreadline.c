/***********************************************************
Copyright 1991-1995 by Stichting Mathematisch Centrum, Amsterdam,
The Netherlands.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the names of Stichting Mathematisch
Centrum or CWI or Corporation for National Research Initiatives or
CNRI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior
permission.

While CWI is the initial source for this software, a modified version
is made available by the Corporation for National Research Initiatives
(CNRI) at the Internet address ftp://ftp.python.org.

STICHTING MATHEMATISCH CENTRUM AND CNRI DISCLAIM ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH
CENTRUM OR CNRI BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

/* Readline interface for tokenizer.c and [raw_]input() in bltinmodule.c.
   By default, or when stdin is not a tty device, we have a super
   simple my_readline function using fgets.
   Optionally, we can use the GNU readline library.
   my_readline() has a different return value from GNU readline():
   - NULL if an interrupt occurred or if an error occurred
   - a malloc'ed empty string if EOF was read
   - a malloc'ed string ending in \n normally
*/

#define Py_USE_NEW_NAMES 1

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "myproto.h"
#include "mymalloc.h"
#include "intrcheck.h"

int (*PyOS_InputHook)() = NULL;

/* This function restarts a fgets() after an EINTR error occurred
   except if PyOS_InterruptOccurred() returns true. */

static int
my_fgets(buf, len, fp)
	char *buf;
	int len;
	FILE *fp;
{
	char *p;
	for (;;) {
		if (PyOS_InputHook != NULL)
			(void)(PyOS_InputHook)();
		errno = 0;
		p = fgets(buf, len, fp);
		if (p != NULL)
			return 0; /* No error */
		if (feof(fp)) {
			return -1; /* EOF */
		}
#ifdef EINTR
		if (errno == EINTR) {
			if (PyOS_InterruptOccurred()) {
				return 1; /* Interrupt */
			}
			continue;
		}
#endif
		if (PyOS_InterruptOccurred()) {
			return 1; /* Interrupt */
		}
		return -2; /* Error */
	}
	/* NOTREACHED */
}


/* Readline implementation using fgets() */

char *
PyOS_StdioReadline(prompt)
	char *prompt;
{
	int n;
	char *p;
	n = 100;
	if ((p = malloc(n)) == NULL)
		return NULL;
	fflush(stdout);
	if (prompt)
		fprintf(stderr, "%s", prompt);
	fflush(stderr);
	switch (my_fgets(p, n, stdin)) {
	case 0: /* Normal case */
		break;
	case 1: /* Interrupt */
		free(p);
		return NULL;
	case -1: /* EOF */
	case -2: /* Error */
	default: /* Shouldn't happen */
		*p = '\0';
		break;
	}
#ifdef MPW
	/* Hack for MPW C where the prompt comes right back in the input */
	/* XXX (Actually this would be rather nice on most systems...) */
	n = strlen(prompt);
	if (strncmp(p, prompt, n) == 0)
		memmove(p, p + n, strlen(p) - n + 1);
#endif
	n = strlen(p);
	while (n > 0 && p[n-1] != '\n') {
		int incr = n+2;
		p = realloc(p, n + incr);
		if (p == NULL)
			return NULL;
		if (my_fgets(p+n, incr, stdin) != 0)
			break;
		n += strlen(p+n);
	}
	return realloc(p, n+1);
}


/* By initializing this function pointer, systems embedding Python can
   override the readline function. */

char *(*PyOS_ReadlineFunctionPointer) Py_PROTO((char *));


/* Interface used by tokenizer.c and bltinmodule.c */

char *
PyOS_Readline(prompt)
	char *prompt;
{
	if (PyOS_ReadlineFunctionPointer == NULL) {
			PyOS_ReadlineFunctionPointer = PyOS_StdioReadline;
	}
	return (*PyOS_ReadlineFunctionPointer)(prompt);
}
