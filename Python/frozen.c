/***********************************************************
Copyright 1991-1995 by Stichting Mathematisch Centrum, Amsterdam,
The Netherlands.

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

/* Dummy frozen modules initializer */

#include "Python.h"

/* In order to test the support for frozen modules, by default we
   define a single frozen module, __hello__.  Loading it will print
   some famous words... */

static unsigned char M___hello__[] = {
	99,0,0,0,0,0,0,115,15,0,0,0,127,0,0,127,
	1,0,100,0,0,71,72,100,1,0,83,40,2,0,0,0,
	115,14,0,0,0,72,101,108,108,111,32,119,111,114,108,100,
	46,46,46,78,40,0,0,0,0,40,0,0,0,0,115,8,
	0,0,0,104,101,108,108,111,46,112,121,115,1,0,0,0,
	63,
};

struct frozen {
	char *name;
	unsigned char *code;
	int size;
} _PyImport_FrozenModules[] = {
	{"__hello__", M___hello__, 81},
	{0, 0, 0} /* sentinel */
};
