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

#ifndef SystemSevenOrLater
#define SystemSevenOrLater 1
#endif

#include <Types.h>
#include <Files.h>
#include <Events.h>

#ifdef GENERATINGCFM	/* Defined to 0 or 1 in Universal headers */
#define HAVE_UNIVERSAL_HEADERS
#endif

char *macstrerror(int);				/* strerror with mac errors */

extern int PyMac_DoYieldEnabled;	/* Don't do eventloop when false */

extern PyObject *PyMac_OSErrException;		/* Exception for OSErr */
PyObject *PyMac_GetOSErrException(void);	/* Initialize & return it */

int PyMac_Idle Py_PROTO((void));			/* Idle routine */
void PyMac_Yield Py_PROTO((void));			/* optional idle routine for mainloop */
void PyMac_SetYield Py_PROTO((long, long, long, long));	/* Set timeouts */
PyObject *PyErr_Mac(PyObject *, int);		/* Exception with a mac error */
PyObject *PyMac_Error(OSErr);			/* Uses PyMac_GetOSErrException */
void PyMac_HandleEvent Py_PROTO((EventRecord *));	/* Handle one event, if possible */

int PyMac_Idle(void);				/* Idle routine */

char *PyMac_GetPythonDir();					/* Return the name of the python dir */

int PyMac_GetOSType(PyObject *, OSType *);	/* argument parser for OSType */
PyObject *PyMac_BuildOSType(OSType);		/* Convert OSType to PyObject */

int PyMac_GetStr255(PyObject *, Str255);	/* argument parser for Str255 */
PyObject *PyMac_BuildStr255(Str255);		/* Convert Str255 to PyObject */

int PyMac_GetFSSpec(PyObject *, FSSpec *);	/* argument parser for FSSpec */
PyObject *PyMac_BuildFSSpec(FSSpec *);		/* Convert FSSpec to PyObject */

int PyMac_GetRect(PyObject *, Rect *);		/* argument parser for Rect */
PyObject *PyMac_BuildRect(Rect *);		/* Convert Rect to PyObject */

int PyMac_GetPoint(PyObject *, Point *);	/* argument parser for Point */
PyObject *PyMac_BuildPoint(Point);		/* Convert Point to PyObject */

int PyMac_GetEventRecord(PyObject *, EventRecord *); /* argument parser for EventRecord */
PyObject *PyMac_BuildEventRecord(EventRecord *); /* Convert EventRecord to PyObject */
