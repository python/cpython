/***********************************************************
Copyright 1991 by Stichting Mathematisch Centrum, Amsterdam, The
Netherlands.

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

/* Definitions used by cgen output */

typedef char *string;

#define mknewlongobject(x) newintobject(x)
#define mknewshortobject(x) newintobject((long)x)
#define mknewfloatobject(x) newfloatobject(x)

extern object *mknewcharobject PROTO((int c));

extern int getiobjectarg PROTO((object *args, int nargs, int i, object **p_a));
extern int getilongarg PROTO((object *args, int nargs, int i, long *p_a));
extern int getishortarg PROTO((object *args, int nargs, int i, short *p_a));
extern int getifloatarg PROTO((object *args, int nargs, int i, float *p_a));
extern int getistringarg PROTO((object *args, int nargs, int i, string *p_a));
