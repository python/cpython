
/* ========================== Module Help =========================== */

#include "Python.h"



#include "macglue.h"
#include "pymactoolbox.h"

#include <Balloons.h>

static PyObject *Help_Error;

static PyObject *Help_HMGetHelpMenuHandle(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	MenuHandle mh;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = HMGetHelpMenuHandle(&mh);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     MenuObj_New, mh);
	return _res;
}

static PyObject *Help_HMRemoveBalloon(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = HMRemoveBalloon();
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Help_HMIsBalloon(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = HMIsBalloon();
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *Help_HMGetBalloons(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = HMGetBalloons();
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *Help_HMSetBalloons(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	Boolean flag;
	if (!PyArg_ParseTuple(_args, "b",
	                      &flag))
		return NULL;
	_err = HMSetBalloons(flag);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Help_HMSetFont(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	SInt16 font;
	if (!PyArg_ParseTuple(_args, "h",
	                      &font))
		return NULL;
	_err = HMSetFont(font);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Help_HMSetFontSize(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	UInt16 fontSize;
	if (!PyArg_ParseTuple(_args, "H",
	                      &fontSize))
		return NULL;
	_err = HMSetFontSize(fontSize);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Help_HMGetFont(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	SInt16 font;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = HMGetFont(&font);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("h",
	                     font);
	return _res;
}

static PyObject *Help_HMGetFontSize(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	UInt16 fontSize;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = HMGetFontSize(&fontSize);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("H",
	                     fontSize);
	return _res;
}

static PyObject *Help_HMSetDialogResID(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	SInt16 resID;
	if (!PyArg_ParseTuple(_args, "h",
	                      &resID))
		return NULL;
	_err = HMSetDialogResID(resID);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Help_HMSetMenuResID(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	SInt16 menuID;
	SInt16 resID;
	if (!PyArg_ParseTuple(_args, "hh",
	                      &menuID,
	                      &resID))
		return NULL;
	_err = HMSetMenuResID(menuID,
	                      resID);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Help_HMScanTemplateItems(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	SInt16 whichID;
	SInt16 whichResFile;
	ResType whichType;
	if (!PyArg_ParseTuple(_args, "hhO&",
	                      &whichID,
	                      &whichResFile,
	                      PyMac_GetOSType, &whichType))
		return NULL;
	_err = HMScanTemplateItems(whichID,
	                           whichResFile,
	                           whichType);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Help_HMGetDialogResID(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	SInt16 resID;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = HMGetDialogResID(&resID);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("h",
	                     resID);
	return _res;
}

static PyObject *Help_HMGetMenuResID(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	SInt16 menuID;
	SInt16 resID;
	if (!PyArg_ParseTuple(_args, "h",
	                      &menuID))
		return NULL;
	_err = HMGetMenuResID(menuID,
	                      &resID);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("h",
	                     resID);
	return _res;
}

static PyObject *Help_HMGetBalloonWindow(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	WindowPtr window;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = HMGetBalloonWindow(&window);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     WinObj_WhichWindow, window);
	return _res;
}

static PyMethodDef Help_methods[] = {
	{"HMGetHelpMenuHandle", (PyCFunction)Help_HMGetHelpMenuHandle, 1,
	 "() -> (MenuHandle mh)"},
	{"HMRemoveBalloon", (PyCFunction)Help_HMRemoveBalloon, 1,
	 "() -> None"},
	{"HMIsBalloon", (PyCFunction)Help_HMIsBalloon, 1,
	 "() -> (Boolean _rv)"},
	{"HMGetBalloons", (PyCFunction)Help_HMGetBalloons, 1,
	 "() -> (Boolean _rv)"},
	{"HMSetBalloons", (PyCFunction)Help_HMSetBalloons, 1,
	 "(Boolean flag) -> None"},
	{"HMSetFont", (PyCFunction)Help_HMSetFont, 1,
	 "(SInt16 font) -> None"},
	{"HMSetFontSize", (PyCFunction)Help_HMSetFontSize, 1,
	 "(UInt16 fontSize) -> None"},
	{"HMGetFont", (PyCFunction)Help_HMGetFont, 1,
	 "() -> (SInt16 font)"},
	{"HMGetFontSize", (PyCFunction)Help_HMGetFontSize, 1,
	 "() -> (UInt16 fontSize)"},
	{"HMSetDialogResID", (PyCFunction)Help_HMSetDialogResID, 1,
	 "(SInt16 resID) -> None"},
	{"HMSetMenuResID", (PyCFunction)Help_HMSetMenuResID, 1,
	 "(SInt16 menuID, SInt16 resID) -> None"},
	{"HMScanTemplateItems", (PyCFunction)Help_HMScanTemplateItems, 1,
	 "(SInt16 whichID, SInt16 whichResFile, ResType whichType) -> None"},
	{"HMGetDialogResID", (PyCFunction)Help_HMGetDialogResID, 1,
	 "() -> (SInt16 resID)"},
	{"HMGetMenuResID", (PyCFunction)Help_HMGetMenuResID, 1,
	 "(SInt16 menuID) -> (SInt16 resID)"},
	{"HMGetBalloonWindow", (PyCFunction)Help_HMGetBalloonWindow, 1,
	 "() -> (WindowPtr window)"},
	{NULL, NULL, 0}
};




void initHelp(void)
{
	PyObject *m;
	PyObject *d;




	m = Py_InitModule("Help", Help_methods);
	d = PyModule_GetDict(m);
	Help_Error = PyMac_GetOSErrException();
	if (Help_Error == NULL ||
	    PyDict_SetItemString(d, "Error", Help_Error) != 0)
		return;
}

/* ======================== End module Help ========================= */

