#ifndef Py_ERRORS_H
#define Py_ERRORS_H
#ifdef __cplusplus
extern "C" {
#endif

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

/* Error handling definitions */

void err_set PROTO((object *));
void err_setval PROTO((object *, object *));
void err_setstr PROTO((object *, char *));
object *err_occurred PROTO((void));
void err_get PROTO((object **, object **));
void err_clear PROTO((void));

/* Predefined exceptions */

extern object *AccessError;
extern object *AttributeError;
extern object *ConflictError;
extern object *EOFError;
extern object *IOError;
extern object *ImportError;
extern object *IndexError;
extern object *KeyError;
extern object *KeyboardInterrupt;
extern object *MemoryError;
extern object *NameError;
extern object *OverflowError;
extern object *RuntimeError;
extern object *SyntaxError;
extern object *SystemError;
extern object *SystemExit;
extern object *TypeError;
extern object *ValueError;
extern object *ZeroDivisionError;

/* Convenience functions */

extern int err_badarg PROTO((void));
extern object *err_nomem PROTO((void));
extern object *err_errno PROTO((object *));

extern void err_badcall PROTO((void));

extern object *err_getexc PROTO((void));

extern int sigcheck PROTO((void)); /* In sigcheck.c or signalmodule.c */

#ifdef __cplusplus
}
#endif
#endif /* !Py_ERRORS_H */
