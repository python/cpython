#ifndef Py_MODSUPPORT_H
#define Py_MODSUPPORT_H
#ifdef __cplusplus
extern "C" {
#endif

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

/* Module support interface */

#ifdef HAVE_STDARG_PROTOTYPES

#include <stdarg.h>

extern int getargs PROTO((object *, char *, ...));
extern int newgetargs PROTO((object *, char *, ...));
extern object *mkvalue PROTO((char *, ...));

#else

#include <varargs.h>

/* Better to have no prototypes at all for varargs functions in this case */
extern int getargs();
extern object *mkvalue();

#endif

extern int vgetargs PROTO((object *, char *, va_list));
extern object *vmkvalue PROTO((char *, va_list));

extern object *initmodule PROTO((char *, struct methodlist *));
extern object *initmodule3 PROTO((char *, struct methodlist *,
				  char *, object *));

/* The following are obsolete -- use getargs directly! */
#define getnoarg(v) getargs(v, "")
#define getintarg(v, a) getargs(v, "i", a)
#define getlongarg(v, a) getargs(v, "l", a)
#define getstrarg(v, a) getargs(v, "s", a)

#ifdef __cplusplus
}
#endif
#endif /* !Py_MODSUPPORT_H */
