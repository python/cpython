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

/*
Dictionary object type -- mapping from char * to object.
NB: the key is given as a char *, not as a stringobject.
These functions set errno for errors.  Functions dictremove() and
dictinsert() return nonzero for errors, getdictsize() returns -1,
the others NULL.  A successful call to dictinsert() calls INCREF()
for the inserted item.
*/

extern typeobject Dicttype;

#define is_dictobject(op) ((op)->ob_type == &Dicttype)

extern object *newdictobject PROTO((void));
extern object *dictlookup PROTO((object *dp, char *key));
extern int dictinsert PROTO((object *dp, char *key, object *item));
extern int dictremove PROTO((object *dp, char *key));
extern int getdictsize PROTO((object *dp));
extern char *getdictkey PROTO((object *dp, int i));
extern object *getdictkeys PROTO((object *dp));
