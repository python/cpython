/***********************************************************
Copyright 1991, 1992, 1993, 1994 by Stichting Mathematisch Centrum,
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

/* SGI module -- random SGI-specific things */

#include "allobjects.h"
#include "modsupport.h"
#include "ceval.h"

extern int sginap(long);

static object *
sgi_nap(self, args)
	object *self;
	object *args;
{
	long ticks;
	if (!getargs(args, "l", &ticks))
		return NULL;
	BGN_SAVE
	sginap(ticks);
	END_SAVE
	INCREF(None);
	return None;
}

static struct methodlist sgi_methods[] = {
	{"nap",		sgi_nap},
	{NULL,		NULL}		/* sentinel */
};


void
initsgi()
{
	initmodule("sgi", sgi_methods);
}

static int dummy; /* $%#@!& dl wants at least a byte of bss */
