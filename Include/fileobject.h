#ifndef Py_FILEOBJECT_H
#define Py_FILEOBJECT_H
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

/* File object interface */

extern DL_IMPORT(PyTypeObject) PyFile_Type;

#define PyFile_Check(op) ((op)->ob_type == &PyFile_Type)

extern DL_IMPORT(PyObject *) PyFile_FromString Py_PROTO((char *, char *));
extern DL_IMPORT(void) PyFile_SetBufSize Py_PROTO((PyObject *, int));
extern DL_IMPORT(PyObject *) PyFile_FromFile
	Py_PROTO((FILE *, char *, char *, int (*)Py_FPROTO((FILE *))));
extern DL_IMPORT(FILE *) PyFile_AsFile Py_PROTO((PyObject *));
extern DL_IMPORT(PyObject *) PyFile_Name Py_PROTO((PyObject *));
extern DL_IMPORT(PyObject *) PyFile_GetLine Py_PROTO((PyObject *, int));
extern DL_IMPORT(int) PyFile_WriteObject Py_PROTO((PyObject *, PyObject *, int));
extern DL_IMPORT(int) PyFile_SoftSpace Py_PROTO((PyObject *, int));
extern DL_IMPORT(int) PyFile_WriteString Py_PROTO((char *, PyObject *));

#ifdef __cplusplus
}
#endif
#endif /* !Py_FILEOBJECT_H */
