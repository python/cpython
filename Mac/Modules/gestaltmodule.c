/***********************************************************
Copyright 1991-1997 by Stichting Mathematisch Centrum, Amsterdam,
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

/* Macintosh Gestalt interface */

#include "Python.h"
#include "pymactoolbox.h"

#include <Carbon/Carbon.h>

static PyObject *
gestalt_gestalt(PyObject *self, PyObject *args)
{
	OSErr iErr;
	OSType selector;
	SInt32 response;
	if (!PyArg_ParseTuple(args, "O&", PyMac_GetOSType, &selector))
		return NULL;
	iErr = Gestalt ( selector, &response );
	if (iErr != 0) 
		return PyMac_Error(iErr);
	return PyInt_FromLong(response);
}

static struct PyMethodDef gestalt_methods[] = {
	{"gestalt", gestalt_gestalt, METH_VARARGS},
	{NULL, NULL} /* Sentinel */
};

void
initgestalt(void)
{
	Py_InitModule("gestalt", gestalt_methods);
}
