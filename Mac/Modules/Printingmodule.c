/******************************************************************
Copyright 1998 by Just van Rossum, Den Haag, The Netherlands.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Just van Rossum not be 
used in advertising or publicity pertaining to distribution of the 
software without specific, written prior permission.

JUST VAN ROSSUM DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, 
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO 
EVENT SHALL JUST VAN ROSSUM BE LIABLE FOR ANY SPECIAL, INDIRECT OR 
CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF 
USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR 
OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

#include "Python.h"
#include "macglue.h"
#include <Printing.h>

extern int ResObj_Convert(PyObject *, Handle *);
extern PyObject *DlgObj_New(DialogPtr);
extern PyObject *GrafObj_New(GrafPtr);
extern int GrafObj_Convert(PyObject *, GrafPtr *);
extern PyObject *ResObj_New(Handle);


static PyObject *ErrorObject;

/* ----------------------------------------------------- */

static int
TPRect_Convert(PyObject *v, TPRect *r)
{
	if (v == Py_None) {
		*r = NULL;
		return 1;
	}
	return PyArg_Parse(v, "(hhhh)", &(*r)->left, &(*r)->top, &(*r)->right, &(*r)->bottom);
}


static char Pr_NewTPrintRecord__doc__[] =
"creates a new TPrint handle"
;

static PyObject *
Pr_NewTPrintRecord(self, args)
	PyObject *self;	/* Not used */
	PyObject *args;
{
	Handle hPrint;

	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	hPrint = NewHandleClear((long) sizeof(TPrint));
	if ( hPrint == NULL ) {
		PyErr_NoMemory();
		return NULL;
	}
	return (PyObject *)ResObj_New(hPrint);
}

static char Pr_PrPurge__doc__[] =
"PrPurge() -> None"
;

static PyObject *
Pr_PrPurge(self, args)
	PyObject *self;	/* Not used */
	PyObject *args;
{

	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	PrPurge();
	{
		OSErr _err = PrError();
		if (_err != noErr) return PyMac_Error(_err);
	}
	Py_INCREF(Py_None);
	return Py_None;
}

static char Pr_PrNoPurge__doc__[] =
"PrNoPurge() -> None"
;

static PyObject *
Pr_PrNoPurge(self, args)
	PyObject *self;	/* Not used */
	PyObject *args;
{

	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	PrNoPurge();
	{
		OSErr _err = PrError();
		if (_err != noErr) return PyMac_Error(_err);
	}
	Py_INCREF(Py_None);
	return Py_None;
}

static char Pr_PrOpen__doc__[] =
"PrOpen() -> None"
;

static PyObject *
Pr_PrOpen(self, args)
	PyObject *self;	/* Not used */
	PyObject *args;
{

	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	PrOpen();
	{
		OSErr _err = PrError();
		if (_err != noErr) return PyMac_Error(_err);
	}
	Py_INCREF(Py_None);
	return Py_None;
}

static char Pr_PrClose__doc__[] =
"PrClose() -> None"
;

static PyObject *
Pr_PrClose(self, args)
	PyObject *self;	/* Not used */
	PyObject *args;
{

	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	PrClose();
	{
		OSErr _err = PrError();
		if (_err != noErr) return PyMac_Error(_err);
	}
	Py_INCREF(Py_None);
	return Py_None;
}

static char Pr_PrintDefault__doc__[] =
"PrintDefault(THPrint hPrint) -> None"
;

static PyObject *
Pr_PrintDefault(self, args)
	PyObject *self;	/* Not used */
	PyObject *args;
{
	THPrint hPrint;

	if (!PyArg_ParseTuple(args, "O&", ResObj_Convert, &hPrint))
		return NULL;
	PrintDefault(hPrint);
	{
		OSErr _err = PrError();
		if (_err != noErr) return PyMac_Error(_err);
	}
	Py_INCREF(Py_None);
	return Py_None;
}

static char Pr_PrValidate__doc__[] =
"PrValidate(THPrint hPrint) -> None"
;

static PyObject *
Pr_PrValidate(self, args)
	PyObject *self;	/* Not used */
	PyObject *args;
{
	THPrint hPrint;

	if (!PyArg_ParseTuple(args, "O&", ResObj_Convert, &hPrint))
		return NULL;
	PrValidate(hPrint);
	{
		OSErr _err = PrError();
		if (_err != noErr) return PyMac_Error(_err);
	}
	Py_INCREF(Py_None);
	return Py_None;
}

static char Pr_PrStlDialog__doc__[] =
"PrStlDialog(THPrint hPrint) -> Boolean"
;

static PyObject *
Pr_PrStlDialog(self, args)
	PyObject *self;	/* Not used */
	PyObject *args;
{
	THPrint hPrint;
	Boolean rv;
	
	if (!PyArg_ParseTuple(args, "O&", ResObj_Convert, &hPrint))
		return NULL;
	rv = PrStlDialog(hPrint);
	{
		OSErr _err = PrError();
		if (_err != noErr) return PyMac_Error(_err);
	}
	return Py_BuildValue("h", rv);
}

static char Pr_PrJobDialog__doc__[] =
"PrJobDialog(THPrint hPrint) -> Boolean"
;

static PyObject *
Pr_PrJobDialog(self, args)
	PyObject *self;	/* Not used */
	PyObject *args;
{
	THPrint hPrint;
	Boolean rv;

	if (!PyArg_ParseTuple(args, "O&", ResObj_Convert, &hPrint))
		return NULL;
	rv = PrJobDialog(hPrint);
	{
		OSErr _err = PrError();
		if (_err != noErr) return PyMac_Error(_err);
	}
	return Py_BuildValue("h", rv);
}

static char Pr_PrJobMerge__doc__[] =
"PrJobMerge(THPrint hPrintSrc, THPrint hPrintDst) -> none"
;

static PyObject *
Pr_PrJobMerge(self, args)
	PyObject *self;	/* Not used */
	PyObject *args;
{
	THPrint hPrintSrc, hPrintDst;

	if (!PyArg_ParseTuple(args, "O&O&", ResObj_Convert, &hPrintSrc, ResObj_Convert, &hPrintDst))
		return NULL;
	PrJobMerge(hPrintSrc, hPrintDst);
	{
		OSErr _err = PrError();
		if (_err != noErr) return PyMac_Error(_err);
	}
	Py_INCREF(Py_None);
	return Py_None;
}

static char Pr_PrOpenDoc__doc__[] =
"PrOpenDoc(THPrint hPrint) -> TPPrPort aTPPort"
;

static PyObject *
Pr_PrOpenDoc(self, args)
	PyObject *self;	/* Not used */
	PyObject *args;
{
	THPrint hPrint;
	TPPrPort aTPPort;

	if (!PyArg_ParseTuple(args, "O&", ResObj_Convert, &hPrint))
		return NULL;
	aTPPort = PrOpenDoc(hPrint, NULL, NULL);
	{
		OSErr _err = PrError();
		if (_err != noErr) return PyMac_Error(_err);
	}
	return Py_BuildValue("O&", GrafObj_New, aTPPort);
}

static char Pr_PrCloseDoc__doc__[] =
"PrCloseDoc(TPPrPort pPrPort) -> None"
;

static PyObject *
Pr_PrCloseDoc(self, args)
	PyObject *self;	/* Not used */
	PyObject *args;
{
	TPPrPort pPrPort;
	
	if (!PyArg_ParseTuple(args, "O&", GrafObj_Convert, &pPrPort))
		return NULL;
	PrCloseDoc(pPrPort);
	{
		OSErr _err = PrError();
		if (_err != noErr) return PyMac_Error(_err);
	}
	Py_INCREF(Py_None);
	return Py_None;
}

static char Pr_PrOpenPage__doc__[] =
"PrOpenPage(TPPrPort pPrPort, TPRect pPageFrame) -> None"
;

static PyObject *
Pr_PrOpenPage(self, args)
	PyObject *self;	/* Not used */
	PyObject *args;
{
	TPPrPort pPrPort;
	Rect dummyrect = {0, 0, 0, 0};
	TPRect pPageFrame = &dummyrect;
	
	if (!PyArg_ParseTuple(args, "O&O&", GrafObj_Convert, &pPrPort, TPRect_Convert, &pPageFrame))
		return NULL;
	PrOpenPage(pPrPort, pPageFrame);
	{
		OSErr _err = PrError();
		if (_err != noErr) return PyMac_Error(_err);
	}
	Py_INCREF(Py_None);
	return Py_None;
}

static char Pr_PrClosePage__doc__[] =
"PrClosePage(TPPrPort pPrPort) -> None"
;

static PyObject *
Pr_PrClosePage(self, args)
	PyObject *self;	/* Not used */
	PyObject *args;
{
	TPPrPort pPrPort;

	if (!PyArg_ParseTuple(args, "O&", GrafObj_Convert, &pPrPort))
		return NULL;
	PrClosePage(pPrPort);
	{
		OSErr _err = PrError();
		if (_err != noErr) return PyMac_Error(_err);
	}
	Py_INCREF(Py_None);
	return Py_None;
}

static char Pr_PrPicFile__doc__[] =
"PrPicFile(THPrint hPrint) -> none"
;

static PyObject *
Pr_PrPicFile(self, args)
	PyObject *self;	/* Not used */
	PyObject *args;
{
	THPrint hPrint;
	TPrStatus prStatus;
	
	if (!PyArg_ParseTuple(args, "O&", ResObj_Convert, &hPrint))
		return NULL;
	PrPicFile(hPrint, NULL, NULL, NULL, &prStatus);
	{
		OSErr _err = PrError();
		if (_err != noErr) return PyMac_Error(_err);
	}
	Py_INCREF(Py_None);
	return Py_None;
}

static char Pr_PrGeneral__doc__[] =
"not implemented"
;

static PyObject *
Pr_PrGeneral(self, args)
	PyObject *self;	/* Not used */
	PyObject *args;
{

	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	//PrGeneral();
	{
		OSErr _err = PrError();
		if (_err != noErr) return PyMac_Error(_err);
	}
	Py_INCREF(Py_None);
	return Py_None;
}

static char Pr_PrDrvrVers__doc__[] =
"PrDrvrVers() -> version"
;

static PyObject *
Pr_PrDrvrVers(self, args)
	PyObject *self;	/* Not used */
	PyObject *args;
{
	short rv;
	
	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	rv = PrDrvrVers();
	{
		OSErr _err = PrError();
		if (_err != noErr) return PyMac_Error(_err);
	}
	return Py_BuildValue("h", rv);
}

/* List of methods defined in the module */

static struct PyMethodDef Pr_methods[] = {
	{"NewTPrintRecord", (PyCFunction)Pr_NewTPrintRecord, METH_VARARGS, Pr_NewTPrintRecord__doc__},
	{"PrPurge",		(PyCFunction)Pr_PrPurge,		METH_VARARGS,	Pr_PrPurge__doc__},
	{"PrNoPurge",	(PyCFunction)Pr_PrNoPurge,		METH_VARARGS,	Pr_PrNoPurge__doc__},
	{"PrOpen",		(PyCFunction)Pr_PrOpen,			METH_VARARGS,	Pr_PrOpen__doc__},
	{"PrClose",		(PyCFunction)Pr_PrClose,		METH_VARARGS,	Pr_PrClose__doc__},
	{"PrintDefault",(PyCFunction)Pr_PrintDefault,	METH_VARARGS,	Pr_PrintDefault__doc__},
	{"PrValidate",	(PyCFunction)Pr_PrValidate,		METH_VARARGS,	Pr_PrValidate__doc__},
	{"PrStlDialog",	(PyCFunction)Pr_PrStlDialog,	METH_VARARGS,	Pr_PrStlDialog__doc__},
	{"PrJobDialog",	(PyCFunction)Pr_PrJobDialog,	METH_VARARGS,	Pr_PrJobDialog__doc__},
	{"PrJobMerge",	(PyCFunction)Pr_PrJobMerge,		METH_VARARGS,	Pr_PrJobMerge__doc__},
	{"PrOpenDoc",	(PyCFunction)Pr_PrOpenDoc,		METH_VARARGS,	Pr_PrOpenDoc__doc__},
	{"PrCloseDoc",	(PyCFunction)Pr_PrCloseDoc,		METH_VARARGS,	Pr_PrCloseDoc__doc__},
	{"PrOpenPage",	(PyCFunction)Pr_PrOpenPage,		METH_VARARGS,	Pr_PrOpenPage__doc__},
	{"PrClosePage",	(PyCFunction)Pr_PrClosePage,	METH_VARARGS,	Pr_PrClosePage__doc__},
	{"PrPicFile",	(PyCFunction)Pr_PrPicFile,		METH_VARARGS,	Pr_PrPicFile__doc__},
	{"PrGeneral",	(PyCFunction)Pr_PrGeneral,		METH_VARARGS,	Pr_PrGeneral__doc__},
	{"PrDrvrVers",	(PyCFunction)Pr_PrDrvrVers,		METH_VARARGS,	Pr_PrDrvrVers__doc__},
	
	{NULL,	(PyCFunction)NULL, 0, NULL}		/* sentinel */
};


/* Initialization function for the module (*must* be called initPrinting) */

static char Printing_module_documentation[] = 
""
;

void initPrinting();

void
initPrinting()
{
	PyObject *m, *d;

	/* Create the module and add the functions */
	m = Py_InitModule4("Printing", Pr_methods,
		Printing_module_documentation,
		(PyObject*)NULL,PYTHON_API_VERSION);

	/* Add some symbolic constants to the module */
	d = PyModule_GetDict(m);
	ErrorObject = PyString_FromString("Printing.error");
	PyDict_SetItemString(d, "error", ErrorObject);

	/* XXXX Add constants here */
	
	/* Check for errors */
	if (PyErr_Occurred())
		Py_FatalError("can't initialize module Printing");
}

