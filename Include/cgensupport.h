#ifndef Py_CGENSUPPORT_H
#define Py_CGENSUPPORT_H
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

/* Definitions used by cgen output */

/* XXX This file is obsolete.  It is *only* used by glmodule.c. */

typedef char *string;

#define mknewlongobject(x) PyInt_FromLong(x)
#define mknewshortobject(x) PyInt_FromLong((long)x)
#define mknewfloatobject(x) PyFloat_FromDouble(x)
#define mknewcharobject(ch) Py_BuildValue("c", ch)

#define getichararg PyArg_GetChar
#define getidoublearray PyArg_GetDoubleArray
#define getifloatarg PyArg_GetFloat
#define getifloatarray PyArg_GetFloatArray
#define getilongarg PyArg_GetLong
#define getilongarray PyArg_GetLongArray
#define getilongarraysize PyArg_GetLongArraySize
#define getiobjectarg PyArg_GetObject
#define getishortarg PyArg_GetShort
#define getishortarray PyArg_GetShortArray
#define getishortarraysize PyArg_GetShortArraySize
#define getistringarg PyArg_GetString

extern int PyArg_GetObject Py_PROTO((PyObject *args, int nargs,
				     int i, PyObject **p_a));
extern int PyArg_GetLong Py_PROTO((PyObject *args, int nargs,
				   int i, long *p_a));
extern int PyArg_GetShort Py_PROTO((PyObject *args, int nargs,
				    int i, short *p_a));
extern int PyArg_GetFloat Py_PROTO((PyObject *args, int nargs,
				    int i, float *p_a));
extern int PyArg_GetString Py_PROTO((PyObject *args, int nargs,
				     int i, string *p_a));
extern int PyArg_GetChar Py_PROTO((PyObject *args, int nargs,
				   int i, char *p_a));
extern int PyArg_GetLongArray Py_PROTO((PyObject *args, int nargs,
					int i, int n, long *p_a));
extern int PyArg_GetShortArray Py_PROTO((PyObject *args, int nargs,
					 int i, int n, short *p_a));
extern int PyArg_GetDoubleArray Py_PROTO((PyObject *args, int nargs,
					  int i, int n, double *p_a));
extern int PyArg_GetFloatArray Py_PROTO((PyObject *args, int nargs,
					 int i, int n, float *p_a));
extern int PyArg_GetLongArraySize Py_PROTO((PyObject *args, int nargs,
					    int i, long *p_a));
extern int PyArg_GetShortArraySize Py_PROTO((PyObject *args, int nargs,
					     int i, short *p_a));
extern int PyArg_GetDoubleArraySize Py_PROTO((PyObject *args, int nargs,
					      int i, double *p_a));
extern int PyArg_GetFloatArraySize Py_PROTO((PyObject *args, int nargs,
					 int i, float *p_a));

#ifdef __cplusplus
}
#endif
#endif /* !Py_CGENSUPPORT_H */
