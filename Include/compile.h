#ifndef Py_COMPILE_H
#define Py_COMPILE_H
#ifdef __cplusplus
extern "C" {
#endif

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

/* Definitions for compiled intermediate code */


/* An intermediate code fragment contains:
   - a string that encodes the instructions,
   - a list of the constants,
   - a list of the names used,
   - the filename from which it was compiled,
   - the name of the object for which it was compiled. */

typedef struct {
	OB_HEAD
	stringobject *co_code;	/* instruction opcodes */
	object *co_consts;	/* list of immutable constant objects */
	object *co_names;	/* list of stringobjects */
	object *co_filename;	/* string */
	object *co_name;	/* string */
} codeobject;

extern typeobject Codetype;

#define is_codeobject(op) ((op)->ob_type == &Codetype)


/* Public interface */
struct _node; /* Declare the existence of this type */
codeobject *compile PROTO((struct _node *, char *));
codeobject *newcodeobject
	PROTO((object *, object *, object *, object *, object *));

#ifdef __cplusplus
}
#endif
#endif /* !Py_COMPILE_H */
