#ifndef Py_MARSHAL_H
#define Py_MARSHAL_H
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

/* Interface for marshal.c */

void PyMarshal_WriteLongToFile Py_PROTO((long, FILE *));
void wr_short Py_PROTO((int, FILE *));
void PyMarshal_WriteObjectToFile Py_PROTO((PyObject *, FILE *));

long PyMarshal_ReadLongFromFile Py_PROTO((FILE *));
int rd_short Py_PROTO((FILE *));
PyObject *PyMarshal_ReadObjectFromFile Py_PROTO((FILE *));
PyObject *PyMarshal_ReadObjectFromString Py_PROTO((char *, int));

#ifdef __cplusplus
}
#endif
#endif /* !Py_MARSHAL_H */
