
/* =========================== Module Qd ============================ */

#include "Python.h"



#define SystemSevenOrLater 1

#include "macglue.h"
#include <Memory.h>
#include <Dialogs.h>
#include <Menus.h>
#include <Controls.h>

extern PyObject *ResObj_New(Handle);
extern int ResObj_Convert(PyObject *, Handle *);

extern PyObject *WinObj_New(WindowPtr);
extern int WinObj_Convert(PyObject *, WindowPtr *);

extern PyObject *DlgObj_New(DialogPtr);
extern int DlgObj_Convert(PyObject *, DialogPtr *);
extern PyTypeObject Dialog_Type;
#define DlgObj_Check(x) ((x)->ob_type == &Dialog_Type)

extern PyObject *MenuObj_New(MenuHandle);
extern int MenuObj_Convert(PyObject *, MenuHandle *);

extern PyObject *CtlObj_New(ControlHandle);
extern int CtlObj_Convert(PyObject *, ControlHandle *);

extern PyObject *WinObj_WhichWindow(WindowPtr);

#include <QuickDraw.h>
#include <Desk.h>

#define resNotFound -192 /* Can't include <Errors.h> because of Python's "errors.h" */

static PyObject *Qd_Error;

static PyObject *Qd_GlobalToLocal(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Point thePoint;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetPoint, &thePoint))
		return NULL;
	GlobalToLocal(&thePoint);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildPoint, thePoint);
	return _res;
}

static PyObject *Qd_LocalToGlobal(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Point thePoint;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetPoint, &thePoint))
		return NULL;
	LocalToGlobal(&thePoint);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildPoint, thePoint);
	return _res;
}

static PyObject *Qd_SetPort(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	WindowPtr thePort;
	if (!PyArg_ParseTuple(_args, "O&",
	                      WinObj_Convert, &thePort))
		return NULL;
	SetPort(thePort);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_ClipRect(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Rect r;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetRect, &r))
		return NULL;
	ClipRect(&r);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_EraseRect(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Rect r;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetRect, &r))
		return NULL;
	EraseRect(&r);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_OpenDeskAcc(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Str255 name;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetStr255, name))
		return NULL;
	OpenDeskAcc(name);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyMethodDef Qd_methods[] = {
	{"GlobalToLocal", (PyCFunction)Qd_GlobalToLocal, 1,
	 "(Point thePoint) -> (Point thePoint)"},
	{"LocalToGlobal", (PyCFunction)Qd_LocalToGlobal, 1,
	 "(Point thePoint) -> (Point thePoint)"},
	{"SetPort", (PyCFunction)Qd_SetPort, 1,
	 "(WindowPtr thePort) -> None"},
	{"ClipRect", (PyCFunction)Qd_ClipRect, 1,
	 "(Rect r) -> None"},
	{"EraseRect", (PyCFunction)Qd_EraseRect, 1,
	 "(Rect r) -> None"},
	{"OpenDeskAcc", (PyCFunction)Qd_OpenDeskAcc, 1,
	 "(Str255 name) -> None"},
	{NULL, NULL, 0}
};




void initQd()
{
	PyObject *m;
	PyObject *d;




	m = Py_InitModule("Qd", Qd_methods);
	d = PyModule_GetDict(m);
	Qd_Error = PyMac_GetOSErrException();
	if (Qd_Error == NULL ||
	    PyDict_SetItemString(d, "Error", Qd_Error) != 0)
		Py_FatalError("can't initialize Qd.Error");
}

/* ========================= End module Qd ========================== */

