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

#include <Types.h>
#include <Files.h>

char *macstrerror Py_PROTO((int));			/* strerror with mac errors */

PyObject *PyErr_Mac Py_PROTO((PyObject *, int));	/* Exception with a mac error */

int PyMac_Idle Py_PROTO((void));			/* Idle routine */

int PyMac_GetOSType Py_PROTO((PyObject *, OSType *));	/* argument parser for OSType */
PyObject *PyMac_BuildOSType Py_PROTO((OSType));		/* Convert OSType to PyObject */

int PyMac_GetStr255 Py_PROTO((PyObject *, Str255));	/* argument parser for Str255 */
PyObject *PyMac_BuildStr255 Py_PROTO((Str255));		/* Convert Str255 to PyObject */

int PyMac_GetFSSpec Py_PROTO((PyObject *, FSSpec *));	/* argument parser for FSSpec */
PyObject *PyMac_BuildFSSpec Py_PROTO((FSSpec *));	/* Convert FSSpec to PyObject */
