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

/* Readline interface for tokenizer.c.
   By default, we have a super simple my_readline function.
   Optionally, we can use the GNU readline library (to be found in the
   bash distribution).
   my_readline() has a different return value from GNU readline():
   - NULL if an interrupt occurred or if an error occurred
   - a malloc'ed empty string if EOF was read
   - a malloc'ed string ending in \n normally
*/

#include <stdio.h>
#include <string.h>

#include "mymalloc.h"

#ifdef HAVE_READLINE

extern char *readline();

#include <setjmp.h>
#include <signal.h>

static jmp_buf jbuf;

/* ARGSUSED */
static RETSIGTYPE
onintr(sig)
	int sig;
{
	longjmp(jbuf, 1);
}

#endif /* HAVE_READLINE */

char *
my_readline(prompt)
	char *prompt;
{
	int n;
	char *p;
#ifdef HAVE_READLINE
	RETSIGTYPE (*old_inthandler)();
	static int been_here;
	if (!been_here) {
		/* Force rebind of TAB to insert-tab */
		extern int rl_insert();
		rl_bind_key('\t', rl_insert);
		been_here++;
	}
	old_inthandler = signal(SIGINT, onintr);
	if (setjmp(jbuf)) {
		signal(SIGINT, old_inthandler);
		return NULL;
	}
	p = readline(prompt);
	signal(SIGINT, old_inthandler);
	if (p == NULL) {
		p = malloc(1);
		if (p != NULL)
			*p = '\0';
		return p;
	}
	n = strlen(p);
	if (n > 0)
		add_history(p);
	if ((p = realloc(p, n+2)) != NULL) {
		p[n] = '\n';
		p[n+1] = '\0';
	}
	return p;
#else /* !HAVE_READLINE */
	n = 100;
	if ((p = malloc(n)) == NULL)
		return NULL;
	if (prompt)
		fprintf(stderr, "%s", prompt);
	if (fgets(p, n, stdin) == NULL)
		    *p = '\0';
	if (intrcheck()) {
		free(p);
		return NULL;
	}
	n = strlen(p);
	while (n > 0 && p[n-1] != '\n') {
		int incr = n+2;
		p = realloc(p, n + incr);
		if (p == NULL)
			return NULL;
		if (fgets(p+n, incr, stdin) == NULL)
			break;
		n += strlen(p+n);
	}
	return realloc(p, n+1);
#endif /* !HAVE_READLINE */
}
