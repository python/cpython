
/* ========================== Module _Help ========================== */

#include "Python.h"



#include "pymactoolbox.h"

/* Macro to test whether a weak-loaded CFM function exists */
#define PyMac_PRECHECK(rtn) do { if ( &rtn == NULL )  {\
        PyErr_SetString(PyExc_NotImplementedError, \
        "Not available in this shared library/OS version"); \
        return NULL; \
    }} while(0)


#include <Carbon/Carbon.h>

static PyObject *Help_Error;

static PyObject *Help_HMGetHelpMenu(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	MenuRef outHelpMenu;
	MenuItemIndex outFirstCustomItemIndex;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = HMGetHelpMenu(&outHelpMenu,
	                     &outFirstCustomItemIndex);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&H",
	                     MenuObj_New, outHelpMenu,
	                     outFirstCustomItemIndex);
	return _res;
}

static PyObject *Help_HMAreHelpTagsDisplayed(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = HMAreHelpTagsDisplayed();
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *Help_HMSetHelpTagsDisplayed(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	Boolean inDisplayTags;
	if (!PyArg_ParseTuple(_args, "b",
	                      &inDisplayTags))
		return NULL;
	_err = HMSetHelpTagsDisplayed(inDisplayTags);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Help_HMSetTagDelay(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	Duration inDelay;
	if (!PyArg_ParseTuple(_args, "l",
	                      &inDelay))
		return NULL;
	_err = HMSetTagDelay(inDelay);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Help_HMGetTagDelay(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	Duration outDelay;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = HMGetTagDelay(&outDelay);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     outDelay);
	return _res;
}

static PyObject *Help_HMSetMenuHelpFromBalloonRsrc(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	MenuRef inMenu;
	SInt16 inHmnuRsrcID;
	if (!PyArg_ParseTuple(_args, "O&h",
	                      MenuObj_Convert, &inMenu,
	                      &inHmnuRsrcID))
		return NULL;
	_err = HMSetMenuHelpFromBalloonRsrc(inMenu,
	                                    inHmnuRsrcID);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Help_HMSetDialogHelpFromBalloonRsrc(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	DialogPtr inDialog;
	SInt16 inHdlgRsrcID;
	SInt16 inItemStart;
	if (!PyArg_ParseTuple(_args, "O&hh",
	                      DlgObj_Convert, &inDialog,
	                      &inHdlgRsrcID,
	                      &inItemStart))
		return NULL;
	_err = HMSetDialogHelpFromBalloonRsrc(inDialog,
	                                      inHdlgRsrcID,
	                                      inItemStart);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Help_HMHideTag(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = HMHideTag();
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyMethodDef Help_methods[] = {
	{"HMGetHelpMenu", (PyCFunction)Help_HMGetHelpMenu, 1,
	 PyDoc_STR("() -> (MenuRef outHelpMenu, MenuItemIndex outFirstCustomItemIndex)")},
	{"HMAreHelpTagsDisplayed", (PyCFunction)Help_HMAreHelpTagsDisplayed, 1,
	 PyDoc_STR("() -> (Boolean _rv)")},
	{"HMSetHelpTagsDisplayed", (PyCFunction)Help_HMSetHelpTagsDisplayed, 1,
	 PyDoc_STR("(Boolean inDisplayTags) -> None")},
	{"HMSetTagDelay", (PyCFunction)Help_HMSetTagDelay, 1,
	 PyDoc_STR("(Duration inDelay) -> None")},
	{"HMGetTagDelay", (PyCFunction)Help_HMGetTagDelay, 1,
	 PyDoc_STR("() -> (Duration outDelay)")},
	{"HMSetMenuHelpFromBalloonRsrc", (PyCFunction)Help_HMSetMenuHelpFromBalloonRsrc, 1,
	 PyDoc_STR("(MenuRef inMenu, SInt16 inHmnuRsrcID) -> None")},
	{"HMSetDialogHelpFromBalloonRsrc", (PyCFunction)Help_HMSetDialogHelpFromBalloonRsrc, 1,
	 PyDoc_STR("(DialogPtr inDialog, SInt16 inHdlgRsrcID, SInt16 inItemStart) -> None")},
	{"HMHideTag", (PyCFunction)Help_HMHideTag, 1,
	 PyDoc_STR("() -> None")},
	{NULL, NULL, 0}
};




void init_Help(void)
{
	PyObject *m;
	PyObject *d;




	m = Py_InitModule("_Help", Help_methods);
	d = PyModule_GetDict(m);
	Help_Error = PyMac_GetOSErrException();
	if (Help_Error == NULL ||
	    PyDict_SetItemString(d, "Error", Help_Error) != 0)
		return;
}

/* ======================== End module _Help ======================== */

