#ifndef Py_LISTOBJECT_H
#define Py_LISTOBJECT_H
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

/* List object interface */

/*
123456789-123456789-123456789-123456789-123456789-123456789-123456789-12

Another generally useful object type is an list of object pointers.
This is a mutable type: the list items can be changed, and items can be
added or removed.  Out-of-range indices or non-list objects are ignored.

*** WARNING *** setlistitem does not increment the new item's reference
count, but does decrement the reference count of the item it replaces,
if not nil.  It does *decrement* the reference count if it is *not*
inserted in the list.  Similarly, getlistitem does not increment the
returned item's reference count.
*/

typedef struct {
	OB_VARHEAD
	object **ob_item;
} listobject;

extern DL_IMPORT typeobject Listtype;

#define is_listobject(op) ((op)->ob_type == &Listtype)

extern object *newlistobject PROTO((int size));
extern int getlistsize PROTO((object *));
extern object *getlistitem PROTO((object *, int));
extern int setlistitem PROTO((object *, int, object *));
extern int inslistitem PROTO((object *, int, object *));
extern int addlistitem PROTO((object *, object *));
extern object *getlistslice PROTO((object *, int, int));
extern int setlistslice PROTO((object *, int, int, object *));
extern int sortlist PROTO((object *));
extern object *listtuple PROTO((object *));

/* Macro, trading safety for speed */
#define GETLISTITEM(op, i) ((op)->ob_item[i])

#ifdef __cplusplus
}
#endif
#endif /* !Py_LISTOBJECT_H */
