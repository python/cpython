/***********************************************************
Copyright 1991, 1992 by Stichting Mathematisch Centrum, Amsterdam, The
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

/* Module support interface */

extern object *initmodule PROTO((char *, struct methodlist *));
extern int getargs PROTO((object *, char *, ...));
extern object *mkvalue PROTO((char *, ...));

#define getnoarg(v) getargs(v, "")
#define getintarg(v, a) getargs(v, "i", a)
#define getintintarg(v, a, b) getargs(v, "(ii)", a, b)
#define getintintintarg(v, a, b, c) getargs(v, "(iii)", a, b, c)
#define getlongarg(v, a) getargs(v, "l", a)
#define getlonglongarg(v, a, b) getargs(v, "(ll)", a, b)
#define getlonglongobjectarg(v, a, b, c) getargs(v, "(llO)", a, b, c)
#define getStrarg(v, a) getargs(v, "S", a)
#define getstrarg(v, a) getargs(v, "s", a)
#define getstrstrarg(v, a, b) getargs(v, "(ss)", a, b)
#define getStrStrarg(v, a, b) getargs(v, "(SS)", a, b)
#define getstrstrintarg(v, a, b, c) getargs(v, "(ssi)", a, b, c)
#define getStrintarg(v, a, b) getargs(v, "(Si)", a, b)
#define getstrintarg(v, a, b) getargs(v, "(si)", a, b)
#define getintstrarg(v, a, b) getargs(v, "(is)", a, b)
#define getpointarg(v, a) getargs(v, "(ii)", a, (a)+1)
#define get3pointarg(v, a) getargs(v, "((ii)(ii)(ii))", \
				a, a+1, a+2, a+3, a+4, a+5)
#define getrectarg(v, a) getargs(v, "((ii)(ii))", a, a+1, a+2, a+3)
#define getrectintarg(v, a) getargs(v, "(((ii)(ii))i)", a, a+1, a+2, a+3, a+4)
#define getpointintarg(v, a) getargs(v, "((ii)i)", a, a+1, a+2)
#define getpointstrarg(v, a, b) getargs(v, "((ii)s)", a, a+1, b)
#define getrectpointarg(v, a) getargs(v, "(((ii)(ii))(ii))", \
				a, a+1, a+2, a+3, a+4, a+5)
#define getdoublearg(v, a) getargs(v, "d", a)
#define get2doublearg(v, a, b) getargs(v, "(dd)", a, b)
