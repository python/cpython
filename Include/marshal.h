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
Centrum or CWI or Corporation for National Research Initiatives or
CNRI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior
permission.

While CWI is the initial source for this software, a modified version
is made available by the Corporation for National Research Initiatives
(CNRI) at the Internet address ftp://ftp.python.org.

STICHTING MATHEMATISCH CENTRUM AND CNRI DISCLAIM ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH
CENTRUM OR CNRI BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

/* Interface for marshal.c */

DL_IMPORT(void) PyMarshal_WriteLongToFile Py_PROTO((long, FILE *));
DL_IMPORT(void) PyMarshal_WriteShortToFile Py_PROTO((int, FILE *));
DL_IMPORT(void) PyMarshal_WriteObjectToFile Py_PROTO((PyObject *, FILE *));
DL_IMPORT(PyObject *) PyMarshal_WriteObjectToString Py_PROTO((PyObject *));

DL_IMPORT(long) PyMarshal_ReadLongFromFile Py_PROTO((FILE *));
DL_IMPORT(int) PyMarshal_ReadShortFromFile Py_PROTO((FILE *));
DL_IMPORT(PyObject *) PyMarshal_ReadObjectFromFile Py_PROTO((FILE *));
DL_IMPORT(PyObject *) PyMarshal_ReadObjectFromString Py_PROTO((char *, int));

#ifdef __cplusplus
}
#endif
#endif /* !Py_MARSHAL_H */
