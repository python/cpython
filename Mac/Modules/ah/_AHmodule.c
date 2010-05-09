
/* =========================== Module _AH =========================== */

#include "Python.h"



#include "pymactoolbox.h"

/* Macro to test whether a weak-loaded CFM function exists */
#define PyMac_PRECHECK(rtn) do { if ( &rtn == NULL )  {\
    PyErr_SetString(PyExc_NotImplementedError, \
    "Not available in this shared library/OS version"); \
    return NULL; \
    }} while(0)


#include <Carbon/Carbon.h>


static PyObject *Ah_Error;

static PyObject *Ah_AHSearch(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    CFStringRef bookname;
    CFStringRef query;
    if (!PyArg_ParseTuple(_args, "O&O&",
                          CFStringRefObj_Convert, &bookname,
                          CFStringRefObj_Convert, &query))
        return NULL;
    _err = AHSearch(bookname,
                    query);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *Ah_AHGotoMainTOC(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    AHTOCType toctype;
    if (!PyArg_ParseTuple(_args, "h",
                          &toctype))
        return NULL;
    _err = AHGotoMainTOC(toctype);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *Ah_AHGotoPage(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    CFStringRef bookname;
    CFStringRef path;
    CFStringRef anchor;
    if (!PyArg_ParseTuple(_args, "O&O&O&",
                          CFStringRefObj_Convert, &bookname,
                          CFStringRefObj_Convert, &path,
                          CFStringRefObj_Convert, &anchor))
        return NULL;
    _err = AHGotoPage(bookname,
                      path,
                      anchor);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *Ah_AHLookupAnchor(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    CFStringRef bookname;
    CFStringRef anchor;
    if (!PyArg_ParseTuple(_args, "O&O&",
                          CFStringRefObj_Convert, &bookname,
                          CFStringRefObj_Convert, &anchor))
        return NULL;
    _err = AHLookupAnchor(bookname,
                          anchor);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *Ah_AHRegisterHelpBook(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    FSRef appBundleRef;
    if (!PyArg_ParseTuple(_args, "O&",
                          PyMac_GetFSRef, &appBundleRef))
        return NULL;
    _err = AHRegisterHelpBook(&appBundleRef);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyMethodDef Ah_methods[] = {
    {"AHSearch", (PyCFunction)Ah_AHSearch, 1,
     PyDoc_STR("(CFStringRef bookname, CFStringRef query) -> None")},
    {"AHGotoMainTOC", (PyCFunction)Ah_AHGotoMainTOC, 1,
     PyDoc_STR("(AHTOCType toctype) -> None")},
    {"AHGotoPage", (PyCFunction)Ah_AHGotoPage, 1,
     PyDoc_STR("(CFStringRef bookname, CFStringRef path, CFStringRef anchor) -> None")},
    {"AHLookupAnchor", (PyCFunction)Ah_AHLookupAnchor, 1,
     PyDoc_STR("(CFStringRef bookname, CFStringRef anchor) -> None")},
    {"AHRegisterHelpBook", (PyCFunction)Ah_AHRegisterHelpBook, 1,
     PyDoc_STR("(FSRef appBundleRef) -> None")},
    {NULL, NULL, 0}
};




void init_AH(void)
{
    PyObject *m;
    PyObject *d;




    m = Py_InitModule("_AH", Ah_methods);
    d = PyModule_GetDict(m);
    Ah_Error = PyMac_GetOSErrException();
    if (Ah_Error == NULL ||
        PyDict_SetItemString(d, "Error", Ah_Error) != 0)
        return;
}

/* ========================= End module _AH ========================= */

