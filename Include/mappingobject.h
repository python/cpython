#ifndef Py_MAPPINGOBJECT_H
#define Py_MAPPINGOBJECT_H
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

/* Mapping object type -- mapping from hashable object to object */

extern typeobject Mappingtype;

#define is_mappingobject(op) ((op)->ob_type == &Mappingtype)

extern object *newmappingobject PROTO((void));
extern object *mappinglookup PROTO((object *mp, object *key));
extern int mappinginsert PROTO((object *mp, object *key, object *item));
extern int mappingremove PROTO((object *mp, object *key));
extern void mappingclear PROTO((object *mp));
extern int mappinggetnext
	PROTO((object *mp, int *pos, object **key, object **value));
extern object *getmappingkeys PROTO((object *mp));
extern object *getmappingvalues PROTO((object *mp));
extern object *getmappingitems PROTO((object *mp));
extern int getmappingsize PROTO((object *mp));

#ifdef __cplusplus
}
#endif
#endif /* !Py_MAPPINGOBJECT_H */
