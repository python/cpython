
/* ========================= Module _Launch ========================= */

#include "Python.h"



#include "pymactoolbox.h"

/* Macro to test whether a weak-loaded CFM function exists */
#define PyMac_PRECHECK(rtn) do { if ( &rtn == NULL )  {\
        PyErr_SetString(PyExc_NotImplementedError, \
        "Not available in this shared library/OS version"); \
        return NULL; \
    }} while(0)


#if PY_VERSION_HEX < 0x02040000
PyObject *PyMac_GetOSErrException(void);
#endif

#include <ApplicationServices/ApplicationServices.h>

/*
** Optional CFStringRef. None will pass NULL
*/
static int
OptCFStringRefObj_Convert(PyObject *v, CFStringRef *spec)
{
	if (v == Py_None) {
		*spec = NULL;
		return 1;
	}
	return CFStringRefObj_Convert(v, spec);
}

PyObject *
OptCFStringRefObj_New(CFStringRef it)
{
	if (it == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
	return CFStringRefObj_New(it);
}

/*
** Convert LSItemInfoRecord to Python.
*/
PyObject *
LSItemInfoRecord_New(LSItemInfoRecord *it)
{
	return Py_BuildValue("{s:is:O&s:O&s:O&s:O&s:i}", 
		"flags", it->flags,
		"filetype", PyMac_BuildOSType, it->filetype,
		"creator", PyMac_BuildOSType, it->creator,
		"extension", OptCFStringRefObj_New, it->extension,
		"iconFileName", OptCFStringRefObj_New, it->iconFileName,
		"kindID", it->kindID);
}

static PyObject *Launch_Error;

static PyObject *Launch_LSCopyItemInfoForRef(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	FSRef inItemRef;
	LSRequestedInfo inWhichInfo;
	LSItemInfoRecord outItemInfo;
	if (!PyArg_ParseTuple(_args, "O&l",
	                      PyMac_GetFSRef, &inItemRef,
	                      &inWhichInfo))
		return NULL;
	_err = LSCopyItemInfoForRef(&inItemRef,
	                            inWhichInfo,
	                            &outItemInfo);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     LSItemInfoRecord_New, &outItemInfo);
	return _res;
}

static PyObject *Launch_LSCopyItemInfoForURL(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	CFURLRef inURL;
	LSRequestedInfo inWhichInfo;
	LSItemInfoRecord outItemInfo;
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CFURLRefObj_Convert, &inURL,
	                      &inWhichInfo))
		return NULL;
	_err = LSCopyItemInfoForURL(inURL,
	                            inWhichInfo,
	                            &outItemInfo);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     LSItemInfoRecord_New, &outItemInfo);
	return _res;
}

static PyObject *Launch_LSGetExtensionInfo(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	UniChar *inNameLen__in__;
	UniCharCount inNameLen__len__;
	int inNameLen__in_len__;
	UniCharCount outExtStartIndex;
	if (!PyArg_ParseTuple(_args, "u#",
	                      &inNameLen__in__, &inNameLen__in_len__))
		return NULL;
	inNameLen__len__ = inNameLen__in_len__;
	_err = LSGetExtensionInfo(inNameLen__len__, inNameLen__in__,
	                          &outExtStartIndex);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     outExtStartIndex);
	return _res;
}

static PyObject *Launch_LSCopyDisplayNameForRef(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	FSRef inRef;
	CFStringRef outDisplayName;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetFSRef, &inRef))
		return NULL;
	_err = LSCopyDisplayNameForRef(&inRef,
	                               &outDisplayName);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     CFStringRefObj_New, outDisplayName);
	return _res;
}

static PyObject *Launch_LSCopyDisplayNameForURL(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	CFURLRef inURL;
	CFStringRef outDisplayName;
	if (!PyArg_ParseTuple(_args, "O&",
	                      CFURLRefObj_Convert, &inURL))
		return NULL;
	_err = LSCopyDisplayNameForURL(inURL,
	                               &outDisplayName);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     CFStringRefObj_New, outDisplayName);
	return _res;
}

static PyObject *Launch_LSSetExtensionHiddenForRef(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	FSRef inRef;
	Boolean inHide;
	if (!PyArg_ParseTuple(_args, "O&b",
	                      PyMac_GetFSRef, &inRef,
	                      &inHide))
		return NULL;
	_err = LSSetExtensionHiddenForRef(&inRef,
	                                  inHide);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Launch_LSSetExtensionHiddenForURL(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	CFURLRef inURL;
	Boolean inHide;
	if (!PyArg_ParseTuple(_args, "O&b",
	                      CFURLRefObj_Convert, &inURL,
	                      &inHide))
		return NULL;
	_err = LSSetExtensionHiddenForURL(inURL,
	                                  inHide);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Launch_LSCopyKindStringForRef(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	FSRef inFSRef;
	CFStringRef outKindString;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetFSRef, &inFSRef))
		return NULL;
	_err = LSCopyKindStringForRef(&inFSRef,
	                              &outKindString);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     CFStringRefObj_New, outKindString);
	return _res;
}

static PyObject *Launch_LSCopyKindStringForURL(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	CFURLRef inURL;
	CFStringRef outKindString;
	if (!PyArg_ParseTuple(_args, "O&",
	                      CFURLRefObj_Convert, &inURL))
		return NULL;
	_err = LSCopyKindStringForURL(inURL,
	                              &outKindString);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     CFStringRefObj_New, outKindString);
	return _res;
}

static PyObject *Launch_LSGetApplicationForItem(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	FSRef inItemRef;
	LSRolesMask inRoleMask;
	FSRef outAppRef;
	CFURLRef outAppURL;
	if (!PyArg_ParseTuple(_args, "O&l",
	                      PyMac_GetFSRef, &inItemRef,
	                      &inRoleMask))
		return NULL;
	_err = LSGetApplicationForItem(&inItemRef,
	                               inRoleMask,
	                               &outAppRef,
	                               &outAppURL);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&O&",
	                     PyMac_BuildFSRef, &outAppRef,
	                     CFURLRefObj_New, outAppURL);
	return _res;
}

static PyObject *Launch_LSGetApplicationForInfo(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	OSType inType;
	OSType inCreator;
	CFStringRef inExtension;
	LSRolesMask inRoleMask;
	FSRef outAppRef;
	CFURLRef outAppURL;
	if (!PyArg_ParseTuple(_args, "O&O&O&l",
	                      PyMac_GetOSType, &inType,
	                      PyMac_GetOSType, &inCreator,
	                      OptCFStringRefObj_Convert, &inExtension,
	                      &inRoleMask))
		return NULL;
	_err = LSGetApplicationForInfo(inType,
	                               inCreator,
	                               inExtension,
	                               inRoleMask,
	                               &outAppRef,
	                               &outAppURL);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&O&",
	                     PyMac_BuildFSRef, &outAppRef,
	                     CFURLRefObj_New, outAppURL);
	return _res;
}

static PyObject *Launch_LSGetApplicationForURL(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	CFURLRef inURL;
	LSRolesMask inRoleMask;
	FSRef outAppRef;
	CFURLRef outAppURL;
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CFURLRefObj_Convert, &inURL,
	                      &inRoleMask))
		return NULL;
	_err = LSGetApplicationForURL(inURL,
	                              inRoleMask,
	                              &outAppRef,
	                              &outAppURL);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&O&",
	                     PyMac_BuildFSRef, &outAppRef,
	                     CFURLRefObj_New, outAppURL);
	return _res;
}

static PyObject *Launch_LSFindApplicationForInfo(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	OSType inCreator;
	CFStringRef inBundleID;
	CFStringRef inName;
	FSRef outAppRef;
	CFURLRef outAppURL;
	if (!PyArg_ParseTuple(_args, "O&O&O&",
	                      PyMac_GetOSType, &inCreator,
	                      OptCFStringRefObj_Convert, &inBundleID,
	                      OptCFStringRefObj_Convert, &inName))
		return NULL;
	_err = LSFindApplicationForInfo(inCreator,
	                                inBundleID,
	                                inName,
	                                &outAppRef,
	                                &outAppURL);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&O&",
	                     PyMac_BuildFSRef, &outAppRef,
	                     CFURLRefObj_New, outAppURL);
	return _res;
}

static PyObject *Launch_LSCanRefAcceptItem(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	FSRef inItemFSRef;
	FSRef inTargetRef;
	LSRolesMask inRoleMask;
	LSAcceptanceFlags inFlags;
	Boolean outAcceptsItem;
	if (!PyArg_ParseTuple(_args, "O&O&ll",
	                      PyMac_GetFSRef, &inItemFSRef,
	                      PyMac_GetFSRef, &inTargetRef,
	                      &inRoleMask,
	                      &inFlags))
		return NULL;
	_err = LSCanRefAcceptItem(&inItemFSRef,
	                          &inTargetRef,
	                          inRoleMask,
	                          inFlags,
	                          &outAcceptsItem);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("b",
	                     outAcceptsItem);
	return _res;
}

static PyObject *Launch_LSCanURLAcceptURL(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	CFURLRef inItemURL;
	CFURLRef inTargetURL;
	LSRolesMask inRoleMask;
	LSAcceptanceFlags inFlags;
	Boolean outAcceptsItem;
	if (!PyArg_ParseTuple(_args, "O&O&ll",
	                      CFURLRefObj_Convert, &inItemURL,
	                      CFURLRefObj_Convert, &inTargetURL,
	                      &inRoleMask,
	                      &inFlags))
		return NULL;
	_err = LSCanURLAcceptURL(inItemURL,
	                         inTargetURL,
	                         inRoleMask,
	                         inFlags,
	                         &outAcceptsItem);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("b",
	                     outAcceptsItem);
	return _res;
}

static PyObject *Launch_LSOpenFSRef(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	FSRef inRef;
	FSRef outLaunchedRef;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetFSRef, &inRef))
		return NULL;
	_err = LSOpenFSRef(&inRef,
	                   &outLaunchedRef);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildFSRef, &outLaunchedRef);
	return _res;
}

static PyObject *Launch_LSOpenCFURLRef(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	CFURLRef inURL;
	CFURLRef outLaunchedURL;
	if (!PyArg_ParseTuple(_args, "O&",
	                      CFURLRefObj_Convert, &inURL))
		return NULL;
	_err = LSOpenCFURLRef(inURL,
	                      &outLaunchedURL);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     CFURLRefObj_New, outLaunchedURL);
	return _res;
}

static PyMethodDef Launch_methods[] = {
	{"LSCopyItemInfoForRef", (PyCFunction)Launch_LSCopyItemInfoForRef, 1,
	 PyDoc_STR("(FSRef inItemRef, LSRequestedInfo inWhichInfo) -> (LSItemInfoRecord outItemInfo)")},
	{"LSCopyItemInfoForURL", (PyCFunction)Launch_LSCopyItemInfoForURL, 1,
	 PyDoc_STR("(CFURLRef inURL, LSRequestedInfo inWhichInfo) -> (LSItemInfoRecord outItemInfo)")},
	{"LSGetExtensionInfo", (PyCFunction)Launch_LSGetExtensionInfo, 1,
	 PyDoc_STR("(Buffer inNameLen) -> (UniCharCount outExtStartIndex)")},
	{"LSCopyDisplayNameForRef", (PyCFunction)Launch_LSCopyDisplayNameForRef, 1,
	 PyDoc_STR("(FSRef inRef) -> (CFStringRef outDisplayName)")},
	{"LSCopyDisplayNameForURL", (PyCFunction)Launch_LSCopyDisplayNameForURL, 1,
	 PyDoc_STR("(CFURLRef inURL) -> (CFStringRef outDisplayName)")},
	{"LSSetExtensionHiddenForRef", (PyCFunction)Launch_LSSetExtensionHiddenForRef, 1,
	 PyDoc_STR("(FSRef inRef, Boolean inHide) -> None")},
	{"LSSetExtensionHiddenForURL", (PyCFunction)Launch_LSSetExtensionHiddenForURL, 1,
	 PyDoc_STR("(CFURLRef inURL, Boolean inHide) -> None")},
	{"LSCopyKindStringForRef", (PyCFunction)Launch_LSCopyKindStringForRef, 1,
	 PyDoc_STR("(FSRef inFSRef) -> (CFStringRef outKindString)")},
	{"LSCopyKindStringForURL", (PyCFunction)Launch_LSCopyKindStringForURL, 1,
	 PyDoc_STR("(CFURLRef inURL) -> (CFStringRef outKindString)")},
	{"LSGetApplicationForItem", (PyCFunction)Launch_LSGetApplicationForItem, 1,
	 PyDoc_STR("(FSRef inItemRef, LSRolesMask inRoleMask) -> (FSRef outAppRef, CFURLRef outAppURL)")},
	{"LSGetApplicationForInfo", (PyCFunction)Launch_LSGetApplicationForInfo, 1,
	 PyDoc_STR("(OSType inType, OSType inCreator, CFStringRef inExtension, LSRolesMask inRoleMask) -> (FSRef outAppRef, CFURLRef outAppURL)")},
	{"LSGetApplicationForURL", (PyCFunction)Launch_LSGetApplicationForURL, 1,
	 PyDoc_STR("(CFURLRef inURL, LSRolesMask inRoleMask) -> (FSRef outAppRef, CFURLRef outAppURL)")},
	{"LSFindApplicationForInfo", (PyCFunction)Launch_LSFindApplicationForInfo, 1,
	 PyDoc_STR("(OSType inCreator, CFStringRef inBundleID, CFStringRef inName) -> (FSRef outAppRef, CFURLRef outAppURL)")},
	{"LSCanRefAcceptItem", (PyCFunction)Launch_LSCanRefAcceptItem, 1,
	 PyDoc_STR("(FSRef inItemFSRef, FSRef inTargetRef, LSRolesMask inRoleMask, LSAcceptanceFlags inFlags) -> (Boolean outAcceptsItem)")},
	{"LSCanURLAcceptURL", (PyCFunction)Launch_LSCanURLAcceptURL, 1,
	 PyDoc_STR("(CFURLRef inItemURL, CFURLRef inTargetURL, LSRolesMask inRoleMask, LSAcceptanceFlags inFlags) -> (Boolean outAcceptsItem)")},
	{"LSOpenFSRef", (PyCFunction)Launch_LSOpenFSRef, 1,
	 PyDoc_STR("(FSRef inRef) -> (FSRef outLaunchedRef)")},
	{"LSOpenCFURLRef", (PyCFunction)Launch_LSOpenCFURLRef, 1,
	 PyDoc_STR("(CFURLRef inURL) -> (CFURLRef outLaunchedURL)")},
	{NULL, NULL, 0}
};




void init_Launch(void)
{
	PyObject *m;
	PyObject *d;




	m = Py_InitModule("_Launch", Launch_methods);
	d = PyModule_GetDict(m);
	Launch_Error = PyMac_GetOSErrException();
	if (Launch_Error == NULL ||
	    PyDict_SetItemString(d, "Error", Launch_Error) != 0)
		return;
}

/* ======================= End module _Launch ======================= */

