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

/* Class object interface */

/*
Classes are really hacked in at the last moment.
It should be possible to use other object types as base classes,
but currently it isn't.  We'll see if we can fix that later, sigh...
*/

extern typeobject Classtype, Classmembertype, Classmethodtype;

#define is_classobject(op) ((op)->ob_type == &Classtype)
#define is_classmemberobject(op) ((op)->ob_type == &Classmembertype)
#define is_classmethodobject(op) ((op)->ob_type == &Classmethodtype)

extern object *newclassobject PROTO((object *, object *));
extern object *newclassmemberobject PROTO((object *));
extern object *newclassmethodobject PROTO((object *, object *));

extern object *classmethodgetfunc PROTO((object *));
extern object *classmethodgetself PROTO((object *));
