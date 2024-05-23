/*
  winreg.c

  Windows Registry access module for Python.

  * Simple registry access written by Mark Hammond in win32api
    module circa 1995.
  * Bill Tutt expanded the support significantly not long after.
  * Numerous other people have submitted patches since then.
  * Ripped from win32api module 03-Feb-2000 by Mark Hammond, and
    basic Unicode support added.

*/

#include "Python.h"
#include "pycore_object.h"        // _PyObject_Init()
#include "pycore_moduleobject.h"

#include <windows.h>

#if defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM) || defined(MS_WINDOWS_GAMES)

typedef struct {
    PyTypeObject *PyHKEY_Type;
} winreg_state;

/* Forward declares */

static BOOL PyHKEY_AsHKEY(winreg_state *st, PyObject *ob, HKEY *pRes, BOOL bNoneOK);
static BOOL clinic_HKEY_converter(winreg_state *st, PyObject *ob, void *p);
static PyObject *PyHKEY_FromHKEY(winreg_state *st, HKEY h);
static BOOL PyHKEY_Close(winreg_state *st, PyObject *obHandle);

static char errNotAHandle[] = "Object is not a handle";

/* The win32api module reports the function name that failed,
   but this concept is not in the Python core.
   Hopefully it will one day, and in the meantime I don't
   want to lose this info...
*/
#define PyErr_SetFromWindowsErrWithFunction(rc, fnname) \
    PyErr_SetFromWindowsErr(rc)

/* Doc strings */
PyDoc_STRVAR(module_doc,
"This module provides access to the Windows registry API.\n"
"\n"
"Functions:\n"
"\n"
"CloseKey() - Closes a registry key.\n"
"ConnectRegistry() - Establishes a connection to a predefined registry handle\n"
"                    on another computer.\n"
"CreateKey() - Creates the specified key, or opens it if it already exists.\n"
"DeleteKey() - Deletes the specified key.\n"
"DeleteValue() - Removes a named value from the specified registry key.\n"
"EnumKey() - Enumerates subkeys of the specified open registry key.\n"
"EnumValue() - Enumerates values of the specified open registry key.\n"
"ExpandEnvironmentStrings() - Expand the env strings in a REG_EXPAND_SZ\n"
"                             string.\n"
"FlushKey() - Writes all the attributes of the specified key to the registry.\n"
"LoadKey() - Creates a subkey under HKEY_USER or HKEY_LOCAL_MACHINE and\n"
"            stores registration information from a specified file into that\n"
"            subkey.\n"
"OpenKey() - Opens the specified key.\n"
"OpenKeyEx() - Alias of OpenKey().\n"
"QueryValue() - Retrieves the value associated with the unnamed value for a\n"
"               specified key in the registry.\n"
"QueryValueEx() - Retrieves the type and data for a specified value name\n"
"                 associated with an open registry key.\n"
"QueryInfoKey() - Returns information about the specified key.\n"
"SaveKey() - Saves the specified key, and all its subkeys a file.\n"
"SetValue() - Associates a value with a specified key.\n"
"SetValueEx() - Stores data in the value field of an open registry key.\n"
"\n"
"Special objects:\n"
"\n"
"HKEYType -- type object for HKEY objects\n"
"error -- exception raised for Win32 errors\n"
"\n"
"Integer constants:\n"
"Many constants are defined - see the documentation for each function\n"
"to see what constants are used, and where.");



/* PyHKEY docstrings */
PyDoc_STRVAR(PyHKEY_doc,
"PyHKEY Object - A Python object, representing a win32 registry key.\n"
"\n"
"This object wraps a Windows HKEY object, automatically closing it when\n"
"the object is destroyed.  To guarantee cleanup, you can call either\n"
"the Close() method on the PyHKEY, or the CloseKey() method.\n"
"\n"
"All functions which accept a handle object also accept an integer --\n"
"however, use of the handle object is encouraged.\n"
"\n"
"Functions:\n"
"Close() - Closes the underlying handle.\n"
"Detach() - Returns the integer Win32 handle, detaching it from the object\n"
"\n"
"Properties:\n"
"handle - The integer Win32 handle.\n"
"\n"
"Operations:\n"
"__bool__ - Handles with an open object return true, otherwise false.\n"
"__int__ - Converting a handle to an integer returns the Win32 handle.\n"
"rich comparison - Handle objects are compared using the handle value.");



/************************************************************************

  The PyHKEY object definition

************************************************************************/
typedef struct {
    PyObject_VAR_HEAD
    HKEY hkey;
} PyHKEYObject;

#define PyHKEY_Check(st, op) Py_IS_TYPE(op, st->PyHKEY_Type)

static char *failMsg = "bad operand type";

static PyObject *
PyHKEY_unaryFailureFunc(PyObject *ob)
{
    PyErr_SetString(PyExc_TypeError, failMsg);
    return NULL;
}
static PyObject *
PyHKEY_binaryFailureFunc(PyObject *ob1, PyObject *ob2)
{
    PyErr_SetString(PyExc_TypeError, failMsg);
    return NULL;
}
static PyObject *
PyHKEY_ternaryFailureFunc(PyObject *ob1, PyObject *ob2, PyObject *ob3)
{
    PyErr_SetString(PyExc_TypeError, failMsg);
    return NULL;
}

static void
PyHKEY_deallocFunc(PyObject *ob)
{
    /* Can not call PyHKEY_Close, as the ob->tp_type
       has already been cleared, thus causing the type
       check to fail!
    */
    PyHKEYObject *obkey = (PyHKEYObject *)ob;
    if (obkey->hkey)
        RegCloseKey((HKEY)obkey->hkey);

    PyTypeObject *tp = Py_TYPE(ob);
    PyObject_GC_UnTrack(ob);
    PyObject_GC_Del(ob);
    Py_DECREF(tp);
}

static int
PyHKEY_traverseFunc(PyHKEYObject *self, visitproc visit, void *arg)
{
    Py_VISIT(Py_TYPE(self));
    return 0;
}

static int
PyHKEY_boolFunc(PyObject *ob)
{
    return ((PyHKEYObject *)ob)->hkey != 0;
}

static PyObject *
PyHKEY_intFunc(PyObject *ob)
{
    PyHKEYObject *pyhkey = (PyHKEYObject *)ob;
    return PyLong_FromVoidPtr(pyhkey->hkey);
}

static PyObject *
PyHKEY_strFunc(PyObject *ob)
{
    PyHKEYObject *pyhkey = (PyHKEYObject *)ob;
    return PyUnicode_FromFormat("<PyHKEY:%p>", pyhkey->hkey);
}

static int
PyHKEY_compareFunc(PyObject *ob1, PyObject *ob2)
{
    PyHKEYObject *pyhkey1 = (PyHKEYObject *)ob1;
    PyHKEYObject *pyhkey2 = (PyHKEYObject *)ob2;
    return pyhkey1 == pyhkey2 ? 0 :
         (pyhkey1 < pyhkey2 ? -1 : 1);
}

static Py_hash_t
PyHKEY_hashFunc(PyObject *ob)
{
    /* Just use the address.
       XXX - should we use the handle value?
    */
    return PyObject_GenericHash(ob);
}


/*[clinic input]
module winreg
class winreg.HKEYType "PyHKEYObject *" "&PyHKEY_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=4c964eba3bf914d6]*/

/*[python input]
class REGSAM_converter(int_converter):
    type = 'REGSAM'

class DWORD_converter(unsigned_long_converter):
    type = 'DWORD'

class HKEY_converter(CConverter):
    type = 'HKEY'
    converter = 'clinic_HKEY_converter'
    broken_limited_capi = True

    def parse_arg(self, argname, displayname, *, limited_capi):
        assert not limited_capi
        return self.format_code("""
            if (!{converter}(_PyModule_GetState(module), {argname}, &{paramname})) {{{{
                goto exit;
            }}}}
            """,
            argname=argname,
            converter=self.converter)

class HKEY_return_converter(CReturnConverter):
    type = 'HKEY'

    def render(self, function, data):
        self.declare(data)
        self.err_occurred_if_null_pointer("_return_value", data)
        data.return_conversion.append(
            'return_value = PyHKEY_FromHKEY(_PyModule_GetState(module), _return_value);\n')

# HACK: this only works for PyHKEYObjects, nothing else.
#       Should this be generalized and enshrined in clinic.py,
#       destroy this converter with prejudice.
class self_return_converter(CReturnConverter):
    type = 'PyHKEYObject *'

    def render(self, function, data):
        self.declare(data)
        data.return_conversion.append(
            'return_value = (PyObject *)_return_value;\n')
[python start generated code]*/
/*[python end generated code: output=da39a3ee5e6b4b0d input=4979f33998ffb6f8]*/

#include "clinic/winreg.c.h"

/************************************************************************

  The PyHKEY object methods

************************************************************************/
/*[clinic input]
winreg.HKEYType.Close

Closes the underlying Windows handle.

If the handle is already closed, no error is raised.
[clinic start generated code]*/

static PyObject *
winreg_HKEYType_Close_impl(PyHKEYObject *self)
/*[clinic end generated code: output=fced3a624fb0c344 input=6786ac75f6b89de6]*/
{
    winreg_state *st = _PyType_GetModuleState(Py_TYPE(self));
    assert(st != NULL);
    if (!PyHKEY_Close(st, (PyObject *)self)) {
        return NULL;
    }
    Py_RETURN_NONE;
}

/*[clinic input]
winreg.HKEYType.Detach

Detaches the Windows handle from the handle object.

The result is the value of the handle before it is detached.  If the
handle is already detached, this will return zero.

After calling this function, the handle is effectively invalidated,
but the handle is not closed.  You would call this function when you
need the underlying win32 handle to exist beyond the lifetime of the
handle object.
[clinic start generated code]*/

static PyObject *
winreg_HKEYType_Detach_impl(PyHKEYObject *self)
/*[clinic end generated code: output=dda5a9e1a01ae78f input=dd2cc09e6c6ba833]*/
{
    void* ret;
    if (PySys_Audit("winreg.PyHKEY.Detach", "n", (Py_ssize_t)self->hkey) < 0) {
        return NULL;
    }
    ret = (void*)self->hkey;
    self->hkey = 0;
    return PyLong_FromVoidPtr(ret);
}

/*[clinic input]
winreg.HKEYType.__enter__ -> self
[clinic start generated code]*/

static PyHKEYObject *
winreg_HKEYType___enter___impl(PyHKEYObject *self)
/*[clinic end generated code: output=52c34986dab28990 input=c40fab1f0690a8e2]*/
{
    return (PyHKEYObject*)Py_XNewRef(self);
}


/*[clinic input]
winreg.HKEYType.__exit__

    exc_type: object
    exc_value: object
    traceback: object
    /

[clinic start generated code]*/

static PyObject *
winreg_HKEYType___exit___impl(PyHKEYObject *self, PyObject *exc_type,
                              PyObject *exc_value, PyObject *traceback)
/*[clinic end generated code: output=923ebe7389e6a263 input=1eac83cd06962689]*/
{
    winreg_state *st = _PyType_GetModuleState(Py_TYPE(self));
    assert(st != NULL);
    if (!PyHKEY_Close(st, (PyObject *)self)) {
        return NULL;
    }
    Py_RETURN_NONE;
}

/*[clinic input]
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=da39a3ee5e6b4b0d]*/

static struct PyMethodDef PyHKEY_methods[] = {
    WINREG_HKEYTYPE_CLOSE_METHODDEF
    WINREG_HKEYTYPE_DETACH_METHODDEF
    WINREG_HKEYTYPE___ENTER___METHODDEF
    WINREG_HKEYTYPE___EXIT___METHODDEF
    {NULL}
};

#define OFF(e) offsetof(PyHKEYObject, e)
static PyMemberDef PyHKEY_memberlist[] = {
    {"handle",      Py_T_INT,      OFF(hkey), Py_READONLY},
    {NULL}    /* Sentinel */
};

static PyType_Slot pyhkey_type_slots[] = {
    {Py_tp_dealloc, PyHKEY_deallocFunc},
    {Py_tp_members, PyHKEY_memberlist},
    {Py_tp_methods, PyHKEY_methods},
    {Py_tp_doc, (char *)PyHKEY_doc},
    {Py_tp_traverse, PyHKEY_traverseFunc},
    {Py_tp_hash, PyHKEY_hashFunc},
    {Py_tp_str, PyHKEY_strFunc},

    // Number protocol
    {Py_nb_add, PyHKEY_binaryFailureFunc},
    {Py_nb_subtract, PyHKEY_binaryFailureFunc},
    {Py_nb_multiply, PyHKEY_binaryFailureFunc},
    {Py_nb_remainder, PyHKEY_binaryFailureFunc},
    {Py_nb_divmod, PyHKEY_binaryFailureFunc},
    {Py_nb_power, PyHKEY_ternaryFailureFunc},
    {Py_nb_negative, PyHKEY_unaryFailureFunc},
    {Py_nb_positive, PyHKEY_unaryFailureFunc},
    {Py_nb_absolute, PyHKEY_unaryFailureFunc},
    {Py_nb_bool, PyHKEY_boolFunc},
    {Py_nb_invert, PyHKEY_unaryFailureFunc},
    {Py_nb_lshift, PyHKEY_binaryFailureFunc},
    {Py_nb_rshift, PyHKEY_binaryFailureFunc},
    {Py_nb_and, PyHKEY_binaryFailureFunc},
    {Py_nb_xor, PyHKEY_binaryFailureFunc},
    {Py_nb_or, PyHKEY_binaryFailureFunc},
    {Py_nb_int, PyHKEY_intFunc},
    {Py_nb_float, PyHKEY_unaryFailureFunc},
    {0, NULL},
};

static PyType_Spec pyhkey_type_spec = {
    .name = "winreg.PyHKEY",
    .basicsize = sizeof(PyHKEYObject),
    .flags = (Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_IMMUTABLETYPE |
              Py_TPFLAGS_DISALLOW_INSTANTIATION),
    .slots = pyhkey_type_slots,
};

/************************************************************************
   The public PyHKEY API (well, not public yet :-)
************************************************************************/
PyObject *
PyHKEY_New(PyObject *m, HKEY hInit)
{
    winreg_state *st = _PyModule_GetState(m);
    PyHKEYObject *key = PyObject_GC_New(PyHKEYObject, st->PyHKEY_Type);
    if (key == NULL) {
        return NULL;
    }
    key->hkey = hInit;
    PyObject_GC_Track(key);
    return (PyObject *)key;
}

BOOL
PyHKEY_Close(winreg_state *st, PyObject *ob_handle)
{
    LONG rc;
    HKEY key;

    if (!PyHKEY_AsHKEY(st, ob_handle, &key, TRUE)) {
        return FALSE;
    }
    if (PyHKEY_Check(st, ob_handle)) {
        ((PyHKEYObject*)ob_handle)->hkey = 0;
    }
    rc = key ? RegCloseKey(key) : ERROR_SUCCESS;
    if (rc != ERROR_SUCCESS)
        PyErr_SetFromWindowsErrWithFunction(rc, "RegCloseKey");
    return rc == ERROR_SUCCESS;
}

BOOL
PyHKEY_AsHKEY(winreg_state *st, PyObject *ob, HKEY *pHANDLE, BOOL bNoneOK)
{
    if (ob == Py_None) {
        if (!bNoneOK) {
            PyErr_SetString(
                      PyExc_TypeError,
                      "None is not a valid HKEY in this context");
            return FALSE;
        }
        *pHANDLE = (HKEY)0;
    }
    else if (PyHKEY_Check(st ,ob)) {
        PyHKEYObject *pH = (PyHKEYObject *)ob;
        *pHANDLE = pH->hkey;
    }
    else if (PyLong_Check(ob)) {
        /* We also support integers */
        PyErr_Clear();
        *pHANDLE = (HKEY)PyLong_AsVoidPtr(ob);
        if (PyErr_Occurred())
            return FALSE;
    }
    else {
        PyErr_SetString(
                        PyExc_TypeError,
            "The object is not a PyHKEY object");
        return FALSE;
    }
    return TRUE;
}

BOOL
clinic_HKEY_converter(winreg_state *st, PyObject *ob, void *p)
{
    if (!PyHKEY_AsHKEY(st, ob, (HKEY *)p, FALSE)) {
        return FALSE;
    }
    return TRUE;
}

PyObject *
PyHKEY_FromHKEY(winreg_state *st, HKEY h)
{
    PyHKEYObject *op = (PyHKEYObject *)PyObject_GC_New(PyHKEYObject,
                                                       st->PyHKEY_Type);
    if (op == NULL) {
        return NULL;
    }
    op->hkey = h;
    PyObject_GC_Track(op);
    return (PyObject *)op;
}


/************************************************************************
  The module methods
************************************************************************/
BOOL
PyWinObject_CloseHKEY(winreg_state *st, PyObject *obHandle)
{
    BOOL ok;
    if (PyHKEY_Check(st, obHandle)) {
        ok = PyHKEY_Close(st, obHandle);
    }
#if SIZEOF_LONG >= SIZEOF_HKEY
    else if (PyLong_Check(obHandle)) {
        long rc = RegCloseKey((HKEY)PyLong_AsLong(obHandle));
        ok = (rc == ERROR_SUCCESS);
        if (!ok)
            PyErr_SetFromWindowsErrWithFunction(rc, "RegCloseKey");
    }
#else
    else if (PyLong_Check(obHandle)) {
        long rc = RegCloseKey((HKEY)PyLong_AsVoidPtr(obHandle));
        ok = (rc == ERROR_SUCCESS);
        if (!ok)
            PyErr_SetFromWindowsErrWithFunction(rc, "RegCloseKey");
    }
#endif
    else {
        PyErr_SetString(
            PyExc_TypeError,
            "A handle must be a HKEY object or an integer");
        return FALSE;
    }
    return ok;
}


/*
   Private Helper functions for the registry interfaces

** Note that fixupMultiSZ and countString have both had changes
** made to support "incorrect strings".  The registry specification
** calls for strings to be terminated with 2 null bytes.  It seems
** some commercial packages install strings which don't conform,
** causing this code to fail - however, "regedit" etc still work
** with these strings (ie only we don't!).
*/
static void
fixupMultiSZ(wchar_t **str, wchar_t *data, int len)
{
    wchar_t *P;
    int i;
    wchar_t *Q;

    if (len > 0 && data[len - 1] == '\0') {
        Q = data + len - 1;
    }
    else {
        Q = data + len;
    }

    for (P = data, i = 0; P < Q; P++, i++) {
        str[i] = P;
        for (; P < Q && *P != '\0'; P++) {
            ;
        }
    }
}

static int
countStrings(wchar_t *data, int len)
{
    int strings;
    wchar_t *P, *Q;

    if (len > 0 && data[len - 1] == '\0') {
        Q = data + len - 1;
    }
    else {
        Q = data + len;
    }

    for (P = data, strings = 0; P < Q; P++, strings++) {
        for (; P < Q && *P != '\0'; P++) {
            ;
        }
    }
    return strings;
}

/* Convert PyObject into Registry data.
   Allocates space as needed. */
static BOOL
Py2Reg(PyObject *value, DWORD typ, BYTE **retDataBuf, DWORD *retDataSize)
{
    Py_ssize_t i,j;
    switch (typ) {
        case REG_DWORD:
            {
                if (value != Py_None && !PyLong_Check(value)) {
                    return FALSE;
                }
                DWORD d;
                if (value == Py_None) {
                    d = 0;
                }
                else if (PyLong_Check(value)) {
                    d = PyLong_AsUnsignedLong(value);
                    if (d == (DWORD)(-1) && PyErr_Occurred()) {
                        return FALSE;
                    }
                }
                *retDataBuf = (BYTE *)PyMem_NEW(DWORD, 1);
                if (*retDataBuf == NULL) {
                    PyErr_NoMemory();
                    return FALSE;
                }
                memcpy(*retDataBuf, &d, sizeof(DWORD));
                *retDataSize = sizeof(DWORD);
                break;
            }
        case REG_QWORD:
            {
                if (value != Py_None && !PyLong_Check(value)) {
                    return FALSE;
                }
                DWORD64 d;
                if (value == Py_None) {
                    d = 0;
                }
                else if (PyLong_Check(value)) {
                    d = PyLong_AsUnsignedLongLong(value);
                    if (d == (DWORD64)(-1) && PyErr_Occurred()) {
                        return FALSE;
                    }
                }
                *retDataBuf = (BYTE *)PyMem_NEW(DWORD64, 1);
                if (*retDataBuf == NULL) {
                    PyErr_NoMemory();
                    return FALSE;
                }
                memcpy(*retDataBuf, &d, sizeof(DWORD64));
                *retDataSize = sizeof(DWORD64);
                break;
            }
        case REG_SZ:
        case REG_EXPAND_SZ:
            {
                if (value != Py_None) {
                    Py_ssize_t len;
                    if (!PyUnicode_Check(value))
                        return FALSE;
                    *retDataBuf = (BYTE*)PyUnicode_AsWideCharString(value, &len);
                    if (*retDataBuf == NULL)
                        return FALSE;
                    *retDataSize = Py_SAFE_DOWNCAST(
                        (len + 1) * sizeof(wchar_t),
                        Py_ssize_t, DWORD);
                }
                else {
                    *retDataBuf = (BYTE *)PyMem_NEW(wchar_t, 1);
                    if (*retDataBuf == NULL) {
                        PyErr_NoMemory();
                        return FALSE;
                    }
                    ((wchar_t *)*retDataBuf)[0] = L'\0';
                    *retDataSize = 1 * sizeof(wchar_t);
                }
                break;
            }
        case REG_MULTI_SZ:
            {
                DWORD size = 0;
                wchar_t *P;

                if (value == Py_None)
                    i = 0;
                else {
                    if (!PyList_Check(value))
                        return FALSE;
                    i = PyList_Size(value);
                }
                for (j = 0; j < i; j++)
                {
                    PyObject *t;
                    Py_ssize_t len;

                    t = PyList_GET_ITEM(value, j);
                    if (!PyUnicode_Check(t))
                        return FALSE;
                    len = PyUnicode_AsWideChar(t, NULL, 0);
                    if (len < 0)
                        return FALSE;
                    size += Py_SAFE_DOWNCAST(len * sizeof(wchar_t),
                                             size_t, DWORD);
                }

                *retDataSize = size + 2;
                *retDataBuf = (BYTE *)PyMem_NEW(char,
                                                *retDataSize);
                if (*retDataBuf == NULL){
                    PyErr_NoMemory();
                    return FALSE;
                }
                P = (wchar_t *)*retDataBuf;

                for (j = 0; j < i; j++)
                {
                    PyObject *t;
                    Py_ssize_t len;

                    t = PyList_GET_ITEM(value, j);
                    assert(size > 0);
                    len = PyUnicode_AsWideChar(t, P, size);
                    assert(len >= 0);
                    assert((unsigned)len < size);
                    size -= (DWORD)len + 1;
                    P += len + 1;
                }
                /* And doubly-terminate the list... */
                *P = L'\0';
                break;
            }
        case REG_BINARY:
        /* ALSO handle ALL unknown data types here.  Even if we can't
           support it natively, we should handle the bits. */
        default:
            if (value == Py_None) {
                *retDataSize = 0;
                *retDataBuf = NULL;
            }
            else {
                Py_buffer view;

                if (!PyObject_CheckBuffer(value)) {
                    PyErr_Format(PyExc_TypeError,
                        "Objects of type '%s' can not "
                        "be used as binary registry values",
                        Py_TYPE(value)->tp_name);
                    return FALSE;
                }

                if (PyObject_GetBuffer(value, &view, PyBUF_SIMPLE) < 0)
                    return FALSE;

                *retDataBuf = (BYTE *)PyMem_NEW(char, view.len);
                if (*retDataBuf == NULL){
                    PyBuffer_Release(&view);
                    PyErr_NoMemory();
                    return FALSE;
                }
                *retDataSize = Py_SAFE_DOWNCAST(view.len, Py_ssize_t, DWORD);
                memcpy(*retDataBuf, view.buf, view.len);
                PyBuffer_Release(&view);
            }
            break;
    }
    return TRUE;
}

/* Convert Registry data into PyObject*/
static PyObject *
Reg2Py(BYTE *retDataBuf, DWORD retDataSize, DWORD typ)
{
    PyObject *obData;

    switch (typ) {
        case REG_DWORD:
            if (retDataSize == 0)
                obData = PyLong_FromUnsignedLong(0);
            else
                obData = PyLong_FromUnsignedLong(*(DWORD *)retDataBuf);
            break;
        case REG_QWORD:
            if (retDataSize == 0)
                obData = PyLong_FromUnsignedLongLong(0);
            else
                obData = PyLong_FromUnsignedLongLong(*(DWORD64 *)retDataBuf);
            break;
        case REG_SZ:
        case REG_EXPAND_SZ:
            {
                /* REG_SZ should be a NUL terminated string, but only by
                 * convention. The buffer may have been saved without a NUL
                 * or with embedded NULs. To be consistent with reg.exe and
                 * regedit.exe, consume only up to the first NUL. */
                wchar_t *data = (wchar_t *)retDataBuf;
                size_t len = wcsnlen(data, retDataSize / sizeof(wchar_t));
                obData = PyUnicode_FromWideChar(data, len);
                break;
            }
        case REG_MULTI_SZ:
            if (retDataSize == 0)
                obData = PyList_New(0);
            else
            {
                int index = 0;
                wchar_t *data = (wchar_t *)retDataBuf;
                int len = retDataSize / 2;
                int s = countStrings(data, len);
                wchar_t **str = PyMem_New(wchar_t *, s);
                if (str == NULL)
                    return PyErr_NoMemory();

                fixupMultiSZ(str, data, len);
                obData = PyList_New(s);
                if (obData == NULL) {
                    PyMem_Free(str);
                    return NULL;
                }
                for (index = 0; index < s; index++)
                {
                    size_t slen = wcsnlen(str[index], len);
                    PyObject *uni = PyUnicode_FromWideChar(str[index], slen);
                    if (uni == NULL) {
                        Py_DECREF(obData);
                        PyMem_Free(str);
                        return NULL;
                    }
                    PyList_SET_ITEM(obData, index, uni);
                    len -= Py_SAFE_DOWNCAST(slen + 1, size_t, int);
                }
                PyMem_Free(str);

                break;
            }
        case REG_BINARY:
        /* ALSO handle ALL unknown data types here.  Even if we can't
           support it natively, we should handle the bits. */
        default:
            if (retDataSize == 0) {
                obData = Py_NewRef(Py_None);
            }
            else
                obData = PyBytes_FromStringAndSize(
                             (char *)retDataBuf, retDataSize);
            break;
    }
    return obData;
}

/* The Python methods */

/*[clinic input]
winreg.CloseKey

    hkey: object
        A previously opened key.
    /

Closes a previously opened registry key.

Note that if the key is not closed using this method, it will be
closed when the hkey object is destroyed by Python.
[clinic start generated code]*/

static PyObject *
winreg_CloseKey(PyObject *module, PyObject *hkey)
/*[clinic end generated code: output=a4fa537019a80d15 input=5b1aac65ba5127ad]*/
{
    if (!PyHKEY_Close(_PyModule_GetState(module), hkey)) {
        return NULL;
    }
    Py_RETURN_NONE;
}

#if defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM)

/*[clinic input]
winreg.ConnectRegistry -> HKEY

    computer_name: Py_UNICODE(accept={str, NoneType})
        The name of the remote computer, of the form r"\\computername".  If
        None, the local computer is used.
    key: HKEY
        The predefined key to connect to.
    /

Establishes a connection to the registry on another computer.

The return value is the handle of the opened key.
If the function fails, an OSError exception is raised.
[clinic start generated code]*/

static HKEY
winreg_ConnectRegistry_impl(PyObject *module, const wchar_t *computer_name,
                            HKEY key)
/*[clinic end generated code: output=c77d12428f4bfe29 input=5f98a891a347e68e]*/
{
    HKEY retKey;
    long rc;
    if (PySys_Audit("winreg.ConnectRegistry", "un",
                    computer_name, (Py_ssize_t)key) < 0) {
        return NULL;
    }
    Py_BEGIN_ALLOW_THREADS
    rc = RegConnectRegistryW(computer_name, key, &retKey);
    Py_END_ALLOW_THREADS
    if (rc != ERROR_SUCCESS) {
        PyErr_SetFromWindowsErrWithFunction(rc, "ConnectRegistry");
        return NULL;
    }
    return retKey;
}

#endif /* MS_WINDOWS_DESKTOP || MS_WINDOWS_SYSTEM */

/*[clinic input]
winreg.CreateKey -> HKEY

    key: HKEY
        An already open key, or one of the predefined HKEY_* constants.
    sub_key: Py_UNICODE(accept={str, NoneType})
        The name of the key this method opens or creates.
    /

Creates or opens the specified key.

If key is one of the predefined keys, sub_key may be None. In that case,
the handle returned is the same key handle passed in to the function.

If the key already exists, this function opens the existing key.

The return value is the handle of the opened key.
If the function fails, an OSError exception is raised.
[clinic start generated code]*/

static HKEY
winreg_CreateKey_impl(PyObject *module, HKEY key, const wchar_t *sub_key)
/*[clinic end generated code: output=58d3eb2ed428a84d input=3cdd1622488acea2]*/
{
    HKEY retKey;
    long rc;

    if (PySys_Audit("winreg.CreateKey", "nun",
                    (Py_ssize_t)key, sub_key,
                    (Py_ssize_t)KEY_WRITE) < 0) {
        return NULL;
    }
    rc = RegCreateKeyW(key, sub_key, &retKey);
    if (rc != ERROR_SUCCESS) {
        PyErr_SetFromWindowsErrWithFunction(rc, "CreateKey");
        return NULL;
    }
    if (PySys_Audit("winreg.OpenKey/result", "n",
                    (Py_ssize_t)retKey) < 0) {
        return NULL;
    }
    return retKey;
}

/*[clinic input]
winreg.CreateKeyEx -> HKEY

    key: HKEY
        An already open key, or one of the predefined HKEY_* constants.
    sub_key: Py_UNICODE(accept={str, NoneType})
        The name of the key this method opens or creates.
    reserved: int = 0
        A reserved integer, and must be zero.  Default is zero.
    access: REGSAM(c_default='KEY_WRITE') = winreg.KEY_WRITE
        An integer that specifies an access mask that describes the
        desired security access for the key. Default is KEY_WRITE.

Creates or opens the specified key.

If key is one of the predefined keys, sub_key may be None. In that case,
the handle returned is the same key handle passed in to the function.

If the key already exists, this function opens the existing key

The return value is the handle of the opened key.
If the function fails, an OSError exception is raised.
[clinic start generated code]*/

static HKEY
winreg_CreateKeyEx_impl(PyObject *module, HKEY key, const wchar_t *sub_key,
                        int reserved, REGSAM access)
/*[clinic end generated code: output=51b53e38d5e00d4b input=42c2b03f98406b66]*/
{
    HKEY retKey;
    long rc;

    if (PySys_Audit("winreg.CreateKey", "nun",
                    (Py_ssize_t)key, sub_key,
                    (Py_ssize_t)access) < 0) {
        return NULL;
    }
    rc = RegCreateKeyExW(key, sub_key, reserved, NULL, 0,
                         access, NULL, &retKey, NULL);
    if (rc != ERROR_SUCCESS) {
        PyErr_SetFromWindowsErrWithFunction(rc, "CreateKeyEx");
        return NULL;
    }
    if (PySys_Audit("winreg.OpenKey/result", "n",
                    (Py_ssize_t)retKey) < 0) {
        return NULL;
    }
    return retKey;
}

/*[clinic input]
winreg.DeleteKey
    key: HKEY
        An already open key, or any one of the predefined HKEY_* constants.
    sub_key: Py_UNICODE
        A string that must be the name of a subkey of the key identified by
        the key parameter. This value must not be None, and the key may not
        have subkeys.
    /

Deletes the specified key.

This method can not delete keys with subkeys.

If the function succeeds, the entire key, including all of its values,
is removed.  If the function fails, an OSError exception is raised.
[clinic start generated code]*/

static PyObject *
winreg_DeleteKey_impl(PyObject *module, HKEY key, const wchar_t *sub_key)
/*[clinic end generated code: output=2e9f7c09eb7701b8 input=b31d225b935e4211]*/
{
    long rc;
    if (PySys_Audit("winreg.DeleteKey", "nun",
                    (Py_ssize_t)key, sub_key,
                    (Py_ssize_t)0) < 0) {
        return NULL;
    }
    Py_BEGIN_ALLOW_THREADS
    rc = RegDeleteKeyW(key, sub_key);
    Py_END_ALLOW_THREADS
    if (rc != ERROR_SUCCESS)
        return PyErr_SetFromWindowsErrWithFunction(rc, "RegDeleteKey");
    Py_RETURN_NONE;
}

/*[clinic input]
winreg.DeleteKeyEx

    key: HKEY
        An already open key, or any one of the predefined HKEY_* constants.
    sub_key: Py_UNICODE
        A string that must be the name of a subkey of the key identified by
        the key parameter. This value must not be None, and the key may not
        have subkeys.
    access: REGSAM(c_default='KEY_WOW64_64KEY') = winreg.KEY_WOW64_64KEY
        An integer that specifies an access mask that describes the
        desired security access for the key. Default is KEY_WOW64_64KEY.
    reserved: int = 0
        A reserved integer, and must be zero.  Default is zero.

Deletes the specified key (intended for 64-bit OS).

While this function is intended to be used for 64-bit OS, it is also
 available on 32-bit systems.

This method can not delete keys with subkeys.

If the function succeeds, the entire key, including all of its values,
is removed.  If the function fails, an OSError exception is raised.
On unsupported Windows versions, NotImplementedError is raised.
[clinic start generated code]*/

static PyObject *
winreg_DeleteKeyEx_impl(PyObject *module, HKEY key, const wchar_t *sub_key,
                        REGSAM access, int reserved)
/*[clinic end generated code: output=3bf4865c783fe7b2 input=a3186db079b3bf85]*/
{
    long rc;
    if (PySys_Audit("winreg.DeleteKey", "nun",
                    (Py_ssize_t)key, sub_key,
                    (Py_ssize_t)access) < 0) {
        return NULL;
    }
    Py_BEGIN_ALLOW_THREADS
    rc = RegDeleteKeyExW(key, sub_key, access, reserved);
    Py_END_ALLOW_THREADS
    if (rc != ERROR_SUCCESS)
        return PyErr_SetFromWindowsErrWithFunction(rc, "RegDeleteKeyEx");
    Py_RETURN_NONE;
}

/*[clinic input]
winreg.DeleteValue

    key: HKEY
        An already open key, or any one of the predefined HKEY_* constants.
    value: Py_UNICODE(accept={str, NoneType})
        A string that identifies the value to remove.
    /

Removes a named value from a registry key.
[clinic start generated code]*/

static PyObject *
winreg_DeleteValue_impl(PyObject *module, HKEY key, const wchar_t *value)
/*[clinic end generated code: output=ed24b297aab137a5 input=a78d3407a4197b21]*/
{
    long rc;
    if (PySys_Audit("winreg.DeleteValue", "nu",
                    (Py_ssize_t)key, value) < 0) {
        return NULL;
    }
    Py_BEGIN_ALLOW_THREADS
    rc = RegDeleteValueW(key, value);
    Py_END_ALLOW_THREADS
    if (rc !=ERROR_SUCCESS)
        return PyErr_SetFromWindowsErrWithFunction(rc,
                                                   "RegDeleteValue");
    Py_RETURN_NONE;
}

/*[clinic input]
winreg.EnumKey

    key: HKEY
        An already open key, or any one of the predefined HKEY_* constants.
    index: int
        An integer that identifies the index of the key to retrieve.
    /

Enumerates subkeys of an open registry key.

The function retrieves the name of one subkey each time it is called.
It is typically called repeatedly until an OSError exception is
raised, indicating no more values are available.
[clinic start generated code]*/

static PyObject *
winreg_EnumKey_impl(PyObject *module, HKEY key, int index)
/*[clinic end generated code: output=25a6ec52cd147bc4 input=fad9a7c00ab0e04b]*/
{
    long rc;
    PyObject *retStr;

    if (PySys_Audit("winreg.EnumKey", "ni",
                    (Py_ssize_t)key, index) < 0) {
        return NULL;
    }
    /* The Windows docs claim that the max key name length is 255
     * characters, plus a terminating nul character.  However,
     * empirical testing demonstrates that it is possible to
     * create a 256 character key that is missing the terminating
     * nul.  RegEnumKeyEx requires a 257 character buffer to
     * retrieve such a key name. */
    wchar_t tmpbuf[257];
    DWORD len = sizeof(tmpbuf)/sizeof(wchar_t); /* includes NULL terminator */

    Py_BEGIN_ALLOW_THREADS
    rc = RegEnumKeyExW(key, index, tmpbuf, &len, NULL, NULL, NULL, NULL);
    Py_END_ALLOW_THREADS
    if (rc != ERROR_SUCCESS)
        return PyErr_SetFromWindowsErrWithFunction(rc, "RegEnumKeyEx");

    retStr = PyUnicode_FromWideChar(tmpbuf, len);
    return retStr;  /* can be NULL */
}

/*[clinic input]
winreg.EnumValue

    key: HKEY
            An already open key, or any one of the predefined HKEY_* constants.
    index: int
        An integer that identifies the index of the value to retrieve.
    /

Enumerates values of an open registry key.

The function retrieves the name of one subkey each time it is called.
It is typically called repeatedly, until an OSError exception
is raised, indicating no more values.

The result is a tuple of 3 items:
  value_name
    A string that identifies the value.
  value_data
    An object that holds the value data, and whose type depends
    on the underlying registry type.
  data_type
    An integer that identifies the type of the value data.
[clinic start generated code]*/

static PyObject *
winreg_EnumValue_impl(PyObject *module, HKEY key, int index)
/*[clinic end generated code: output=d363b5a06f8789ac input=4414f47a6fb238b5]*/
{
    long rc;
    wchar_t *retValueBuf;
    BYTE *tmpBuf;
    BYTE *retDataBuf;
    DWORD retValueSize, bufValueSize;
    DWORD retDataSize, bufDataSize;
    DWORD typ;
    PyObject *obData;
    PyObject *retVal;

    if (PySys_Audit("winreg.EnumValue", "ni",
                    (Py_ssize_t)key, index) < 0) {
        return NULL;
    }
    if ((rc = RegQueryInfoKeyW(key, NULL, NULL, NULL, NULL, NULL, NULL,
                              NULL,
                              &retValueSize, &retDataSize, NULL, NULL))
        != ERROR_SUCCESS)
        return PyErr_SetFromWindowsErrWithFunction(rc,
                                                   "RegQueryInfoKey");
    ++retValueSize;    /* include null terminators */
    ++retDataSize;
    bufDataSize = retDataSize;
    bufValueSize = retValueSize;
    retValueBuf = PyMem_New(wchar_t, retValueSize);
    if (retValueBuf == NULL)
        return PyErr_NoMemory();
    retDataBuf = (BYTE *)PyMem_Malloc(retDataSize);
    if (retDataBuf == NULL) {
        PyMem_Free(retValueBuf);
        return PyErr_NoMemory();
    }

    while (1) {
        Py_BEGIN_ALLOW_THREADS
        rc = RegEnumValueW(key,
                  index,
                  retValueBuf,
                  &retValueSize,
                  NULL,
                  &typ,
                  (BYTE *)retDataBuf,
                  &retDataSize);
        Py_END_ALLOW_THREADS

        if (rc != ERROR_MORE_DATA)
            break;

        bufDataSize *= 2;
        tmpBuf = (BYTE *)PyMem_Realloc(retDataBuf, bufDataSize);
        if (tmpBuf == NULL) {
            PyErr_NoMemory();
            retVal = NULL;
            goto fail;
        }
        retDataBuf = tmpBuf;
        retDataSize = bufDataSize;
        retValueSize = bufValueSize;
    }

    if (rc != ERROR_SUCCESS) {
        retVal = PyErr_SetFromWindowsErrWithFunction(rc,
                                                     "PyRegEnumValue");
        goto fail;
    }
    obData = Reg2Py(retDataBuf, retDataSize, typ);
    if (obData == NULL) {
        retVal = NULL;
        goto fail;
    }
    retVal = Py_BuildValue("uOi", retValueBuf, obData, typ);
    Py_DECREF(obData);
  fail:
    PyMem_Free(retValueBuf);
    PyMem_Free(retDataBuf);
    return retVal;
}

/*[clinic input]
winreg.ExpandEnvironmentStrings

    string: Py_UNICODE
    /

Expand environment vars.
[clinic start generated code]*/

static PyObject *
winreg_ExpandEnvironmentStrings_impl(PyObject *module, const wchar_t *string)
/*[clinic end generated code: output=53f120bbe788fa6f input=b2a9714d2b751aa6]*/
{
    wchar_t *retValue = NULL;
    DWORD retValueSize;
    DWORD rc;
    PyObject *o;

    if (PySys_Audit("winreg.ExpandEnvironmentStrings", "u",
                    string) < 0) {
        return NULL;
    }

    retValueSize = ExpandEnvironmentStringsW(string, retValue, 0);
    if (retValueSize == 0) {
        return PyErr_SetFromWindowsErrWithFunction(retValueSize,
                                        "ExpandEnvironmentStrings");
    }
    retValue = PyMem_New(wchar_t, retValueSize);
    if (retValue == NULL) {
        return PyErr_NoMemory();
    }

    rc = ExpandEnvironmentStringsW(string, retValue, retValueSize);
    if (rc == 0) {
        PyMem_Free(retValue);
        return PyErr_SetFromWindowsErrWithFunction(retValueSize,
                                        "ExpandEnvironmentStrings");
    }
    o = PyUnicode_FromWideChar(retValue, wcslen(retValue));
    PyMem_Free(retValue);
    return o;
}

#if defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM)

/*[clinic input]
winreg.FlushKey

    key: HKEY
        An already open key, or any one of the predefined HKEY_* constants.
    /

Writes all the attributes of a key to the registry.

It is not necessary to call FlushKey to change a key.  Registry changes
are flushed to disk by the registry using its lazy flusher.  Registry
changes are also flushed to disk at system shutdown.  Unlike
CloseKey(), the FlushKey() method returns only when all the data has
been written to the registry.

An application should only call FlushKey() if it requires absolute
certainty that registry changes are on disk.  If you don't know whether
a FlushKey() call is required, it probably isn't.
[clinic start generated code]*/

static PyObject *
winreg_FlushKey_impl(PyObject *module, HKEY key)
/*[clinic end generated code: output=e6fc230d4c5dc049 input=f57457c12297d82f]*/
{
    long rc;
    Py_BEGIN_ALLOW_THREADS
    rc = RegFlushKey(key);
    Py_END_ALLOW_THREADS
    if (rc != ERROR_SUCCESS)
        return PyErr_SetFromWindowsErrWithFunction(rc, "RegFlushKey");
    Py_RETURN_NONE;
}

#endif /* MS_WINDOWS_DESKTOP || MS_WINDOWS_SYSTEM */

#if defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM)

/*[clinic input]
winreg.LoadKey

    key: HKEY
        An already open key, or any one of the predefined HKEY_* constants.
    sub_key: Py_UNICODE
        A string that identifies the sub-key to load.
    file_name: Py_UNICODE
        The name of the file to load registry data from.  This file must
        have been created with the SaveKey() function.  Under the file
        allocation table (FAT) file system, the filename may not have an
        extension.
    /

Insert data into the registry from a file.

Creates a subkey under the specified key and stores registration
information from a specified file into that subkey.

A call to LoadKey() fails if the calling process does not have the
SE_RESTORE_PRIVILEGE privilege.

If key is a handle returned by ConnectRegistry(), then the path
specified in fileName is relative to the remote computer.

The MSDN docs imply key must be in the HKEY_USER or HKEY_LOCAL_MACHINE
tree.
[clinic start generated code]*/

static PyObject *
winreg_LoadKey_impl(PyObject *module, HKEY key, const wchar_t *sub_key,
                    const wchar_t *file_name)
/*[clinic end generated code: output=5561b0216e5ab263 input=e3b5b45ade311582]*/
{
    long rc;

    if (PySys_Audit("winreg.LoadKey", "nuu",
                    (Py_ssize_t)key, sub_key, file_name) < 0) {
        return NULL;
    }
    Py_BEGIN_ALLOW_THREADS
    rc = RegLoadKeyW(key, sub_key, file_name );
    Py_END_ALLOW_THREADS
    if (rc != ERROR_SUCCESS)
        return PyErr_SetFromWindowsErrWithFunction(rc, "RegLoadKey");
    Py_RETURN_NONE;
}

#endif /* MS_WINDOWS_DESKTOP || MS_WINDOWS_SYSTEM */

/*[clinic input]
winreg.OpenKey -> HKEY

    key: HKEY
        An already open key, or any one of the predefined HKEY_* constants.
    sub_key: Py_UNICODE(accept={str, NoneType})
        A string that identifies the sub_key to open.
    reserved: int = 0
        A reserved integer that must be zero.  Default is zero.
    access: REGSAM(c_default='KEY_READ') = winreg.KEY_READ
        An integer that specifies an access mask that describes the desired
        security access for the key.  Default is KEY_READ.

Opens the specified key.

The result is a new handle to the specified key.
If the function fails, an OSError exception is raised.
[clinic start generated code]*/

static HKEY
winreg_OpenKey_impl(PyObject *module, HKEY key, const wchar_t *sub_key,
                    int reserved, REGSAM access)
/*[clinic end generated code: output=5efbad23b3ffe2e7 input=098505ac36a9ae28]*/
{
    HKEY retKey;
    long rc;

    if (PySys_Audit("winreg.OpenKey", "nun",
                    (Py_ssize_t)key, sub_key,
                    (Py_ssize_t)access) < 0) {
        return NULL;
    }
    Py_BEGIN_ALLOW_THREADS
    rc = RegOpenKeyExW(key, sub_key, reserved, access, &retKey);
    Py_END_ALLOW_THREADS
    if (rc != ERROR_SUCCESS) {
        PyErr_SetFromWindowsErrWithFunction(rc, "RegOpenKeyEx");
        return NULL;
    }
    if (PySys_Audit("winreg.OpenKey/result", "n",
                    (Py_ssize_t)retKey) < 0) {
        return NULL;
    }
    return retKey;
}

/*[clinic input]
winreg.OpenKeyEx = winreg.OpenKey

Opens the specified key.

The result is a new handle to the specified key.
If the function fails, an OSError exception is raised.
[clinic start generated code]*/

static HKEY
winreg_OpenKeyEx_impl(PyObject *module, HKEY key, const wchar_t *sub_key,
                      int reserved, REGSAM access)
/*[clinic end generated code: output=435e675800fa78c2 input=c6c4972af8622959]*/
{
    return winreg_OpenKey_impl(module, key, sub_key, reserved, access);
}

/*[clinic input]
winreg.QueryInfoKey

    key: HKEY
        An already open key, or any one of the predefined HKEY_* constants.
    /

Returns information about a key.

The result is a tuple of 3 items:
An integer that identifies the number of sub keys this key has.
An integer that identifies the number of values this key has.
An integer that identifies when the key was last modified (if available)
as 100's of nanoseconds since Jan 1, 1600.
[clinic start generated code]*/

static PyObject *
winreg_QueryInfoKey_impl(PyObject *module, HKEY key)
/*[clinic end generated code: output=dc657b8356a4f438 input=c3593802390cde1f]*/
{
    long rc;
    DWORD nSubKeys, nValues;
    FILETIME ft;
    LARGE_INTEGER li;
    PyObject *l;
    PyObject *ret;

    if (PySys_Audit("winreg.QueryInfoKey", "n", (Py_ssize_t)key) < 0) {
        return NULL;
    }
    if ((rc = RegQueryInfoKeyW(key, NULL, NULL, 0, &nSubKeys, NULL, NULL,
                               &nValues,  NULL,  NULL, NULL, &ft))
                               != ERROR_SUCCESS) {
        return PyErr_SetFromWindowsErrWithFunction(rc, "RegQueryInfoKey");
    }
    li.LowPart = ft.dwLowDateTime;
    li.HighPart = ft.dwHighDateTime;
    l = PyLong_FromLongLong(li.QuadPart);
    if (l == NULL) {
        return NULL;
    }
    ret = Py_BuildValue("iiO", nSubKeys, nValues, l);
    Py_DECREF(l);
    return ret;
}


/*[clinic input]
winreg.QueryValue

    key: HKEY
        An already open key, or any one of the predefined HKEY_* constants.
    sub_key: Py_UNICODE(accept={str, NoneType})
        A string that holds the name of the subkey with which the value
        is associated.  If this parameter is None or empty, the function
        retrieves the value set by the SetValue() method for the key
        identified by key.
    /

Retrieves the unnamed value for a key.

Values in the registry have name, type, and data components. This method
retrieves the data for a key's first value that has a NULL name.
But since the underlying API call doesn't return the type, you'll
probably be happier using QueryValueEx; this function is just here for
completeness.
[clinic start generated code]*/

static PyObject *
winreg_QueryValue_impl(PyObject *module, HKEY key, const wchar_t *sub_key)
/*[clinic end generated code: output=b665ce9ae391fda9 input=41cafbbf423b21d6]*/
{
    LONG rc;
    HKEY childKey = key;
    WCHAR buf[256], *pbuf = buf;
    DWORD size = sizeof(buf);
    DWORD type;
    Py_ssize_t length;
    PyObject *result = NULL;

    if (PySys_Audit("winreg.QueryValue", "nuu",
                    (Py_ssize_t)key, sub_key, NULL) < 0)
    {
        return NULL;
    }

    if (key == HKEY_PERFORMANCE_DATA) {
        return PyErr_SetFromWindowsErrWithFunction(ERROR_INVALID_HANDLE,
                                                   "RegQueryValue");
    }

    if (sub_key && sub_key[0]) {
        Py_BEGIN_ALLOW_THREADS
        rc = RegOpenKeyExW(key, sub_key, 0, KEY_QUERY_VALUE, &childKey);
        Py_END_ALLOW_THREADS
        if (rc != ERROR_SUCCESS) {
            return PyErr_SetFromWindowsErrWithFunction(rc, "RegOpenKeyEx");
        }
    }

    while (1) {
        Py_BEGIN_ALLOW_THREADS
        rc = RegQueryValueExW(childKey, NULL, NULL, &type, (LPBYTE)pbuf,
                              &size);
        Py_END_ALLOW_THREADS
        if (rc != ERROR_MORE_DATA) {
            break;
        }
        void *tmp = PyMem_Realloc(pbuf != buf ? pbuf : NULL, size);
        if (tmp == NULL) {
            PyErr_NoMemory();
            goto exit;
        }
        pbuf = tmp;
    }

    if (rc == ERROR_SUCCESS) {
        if (type != REG_SZ) {
            PyErr_SetFromWindowsErrWithFunction(ERROR_INVALID_DATA,
                                                "RegQueryValue");
            goto exit;
        }
        length = wcsnlen(pbuf, size / sizeof(WCHAR));
    }
    else if (rc == ERROR_FILE_NOT_FOUND) {
        // Return an empty string if there's no default value.
        length = 0;
    }
    else {
        PyErr_SetFromWindowsErrWithFunction(rc, "RegQueryValueEx");
        goto exit;
    }

    result = PyUnicode_FromWideChar(pbuf, length);

exit:
    if (pbuf != buf) {
        PyMem_Free(pbuf);
    }
    if (childKey != key) {
        RegCloseKey(childKey);
    }
    return result;
}


/*[clinic input]
winreg.QueryValueEx

    key: HKEY
        An already open key, or any one of the predefined HKEY_* constants.
    name: Py_UNICODE(accept={str, NoneType})
        A string indicating the value to query.
    /

Retrieves the type and value of a specified sub-key.

Behaves mostly like QueryValue(), but also returns the type of the
specified value name associated with the given open registry key.

The return value is a tuple of the value and the type_id.
[clinic start generated code]*/

static PyObject *
winreg_QueryValueEx_impl(PyObject *module, HKEY key, const wchar_t *name)
/*[clinic end generated code: output=2cdecaa44c8c333e input=cf366cada4836891]*/
{
    long rc;
    BYTE *retBuf, *tmp;
    DWORD bufSize = 0, retSize;
    DWORD typ;
    PyObject *obData;
    PyObject *result;

    if (PySys_Audit("winreg.QueryValue", "nuu",
                    (Py_ssize_t)key, NULL, name) < 0) {
        return NULL;
    }
    rc = RegQueryValueExW(key, name, NULL, NULL, NULL, &bufSize);
    if (rc == ERROR_MORE_DATA)
        bufSize = 256;
    else if (rc != ERROR_SUCCESS)
        return PyErr_SetFromWindowsErrWithFunction(rc,
                                                   "RegQueryValueEx");
    retBuf = (BYTE *)PyMem_Malloc(bufSize);
    if (retBuf == NULL)
        return PyErr_NoMemory();

    while (1) {
        retSize = bufSize;
        rc = RegQueryValueExW(key, name, NULL, &typ,
                             (BYTE *)retBuf, &retSize);
        if (rc != ERROR_MORE_DATA)
            break;

        bufSize *= 2;
        tmp = (char *) PyMem_Realloc(retBuf, bufSize);
        if (tmp == NULL) {
            PyMem_Free(retBuf);
            return PyErr_NoMemory();
        }
       retBuf = tmp;
    }

    if (rc != ERROR_SUCCESS) {
        PyMem_Free(retBuf);
        return PyErr_SetFromWindowsErrWithFunction(rc,
                                                   "RegQueryValueEx");
    }
    obData = Reg2Py(retBuf, bufSize, typ);
    PyMem_Free(retBuf);
    if (obData == NULL)
        return NULL;
    result = Py_BuildValue("Oi", obData, typ);
    Py_DECREF(obData);
    return result;
}

#if defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM)

/*[clinic input]
winreg.SaveKey

    key: HKEY
        An already open key, or any one of the predefined HKEY_* constants.
    file_name: Py_UNICODE
        The name of the file to save registry data to.  This file cannot
        already exist. If this filename includes an extension, it cannot be
        used on file allocation table (FAT) file systems by the LoadKey(),
        ReplaceKey() or RestoreKey() methods.
    /

Saves the specified key, and all its subkeys to the specified file.

If key represents a key on a remote computer, the path described by
file_name is relative to the remote computer.

The caller of this method must possess the SeBackupPrivilege
security privilege.  This function passes NULL for security_attributes
to the API.
[clinic start generated code]*/

static PyObject *
winreg_SaveKey_impl(PyObject *module, HKEY key, const wchar_t *file_name)
/*[clinic end generated code: output=249b1b58b9598eef input=da735241f91ac7a2]*/
{
    LPSECURITY_ATTRIBUTES pSA = NULL;

    long rc;
/*  One day we may get security into the core?
    if (!PyWinObject_AsSECURITY_ATTRIBUTES(obSA, &pSA, TRUE))
        return NULL;
*/
    if (PySys_Audit("winreg.SaveKey", "nu",
                    (Py_ssize_t)key, file_name) < 0) {
        return NULL;
    }
    Py_BEGIN_ALLOW_THREADS
    rc = RegSaveKeyW(key, file_name, pSA );
    Py_END_ALLOW_THREADS
    if (rc != ERROR_SUCCESS)
        return PyErr_SetFromWindowsErrWithFunction(rc, "RegSaveKey");
    Py_RETURN_NONE;
}

#endif /* MS_WINDOWS_DESKTOP || MS_WINDOWS_SYSTEM */

/*[clinic input]
winreg.SetValue

    key: HKEY
        An already open key, or any one of the predefined HKEY_* constants.
    sub_key: Py_UNICODE(accept={str, NoneType})
        A string that names the subkey with which the value is associated.
    type: DWORD
        An integer that specifies the type of the data.  Currently this must
        be REG_SZ, meaning only strings are supported.
    value as value_obj: unicode
        A string that specifies the new value.
    /

Associates a value with a specified key.

If the key specified by the sub_key parameter does not exist, the
SetValue function creates it.

Value lengths are limited by available memory. Long values (more than
2048 bytes) should be stored as files with the filenames stored in
the configuration registry to help the registry perform efficiently.

The key identified by the key parameter must have been opened with
KEY_SET_VALUE access.
[clinic start generated code]*/

static PyObject *
winreg_SetValue_impl(PyObject *module, HKEY key, const wchar_t *sub_key,
                     DWORD type, PyObject *value_obj)
/*[clinic end generated code: output=de590747df47d2c7 input=bf088494ae2d24fd]*/
{
    LONG rc;
    HKEY childKey = key;
    LPWSTR value;
    Py_ssize_t size;
    Py_ssize_t length;
    PyObject *result = NULL;

    if (type != REG_SZ) {
        PyErr_SetString(PyExc_TypeError, "type must be winreg.REG_SZ");
        return NULL;
    }

    value = PyUnicode_AsWideCharString(value_obj, &length);
    if (value == NULL) {
        return NULL;
    }

    size = (length + 1) * sizeof(WCHAR);
    if ((Py_ssize_t)(DWORD)size != size) {
        PyErr_SetString(PyExc_OverflowError, "value is too long");
        goto exit;
    }

    if (PySys_Audit("winreg.SetValue", "nunu#",
                    (Py_ssize_t)key, sub_key, (Py_ssize_t)type,
                    value, length) < 0)
    {
        goto exit;
    }

    if (key == HKEY_PERFORMANCE_DATA) {
        PyErr_SetFromWindowsErrWithFunction(ERROR_INVALID_HANDLE,
                                            "RegSetValue");
        goto exit;
    }

    if (sub_key && sub_key[0]) {
        Py_BEGIN_ALLOW_THREADS
        rc = RegCreateKeyExW(key, sub_key, 0, NULL, 0, KEY_SET_VALUE, NULL,
                             &childKey, NULL);
        Py_END_ALLOW_THREADS
        if (rc != ERROR_SUCCESS) {
            PyErr_SetFromWindowsErrWithFunction(rc, "RegCreateKeyEx");
            goto exit;
        }
    }

    Py_BEGIN_ALLOW_THREADS
    rc = RegSetValueExW(childKey, NULL, 0, REG_SZ, (LPBYTE)value, (DWORD)size);
    Py_END_ALLOW_THREADS
    if (rc == ERROR_SUCCESS) {
        result = Py_NewRef(Py_None);
    }
    else {
        PyErr_SetFromWindowsErrWithFunction(rc, "RegSetValueEx");
    }

exit:
    PyMem_Free(value);
    if (childKey != key) {
        RegCloseKey(childKey);
    }
    return result;
}


/*[clinic input]
winreg.SetValueEx

    key: HKEY
        An already open key, or any one of the predefined HKEY_* constants.
    value_name: Py_UNICODE(accept={str, NoneType})
        A string containing the name of the value to set, or None.
    reserved: object
        Can be anything - zero is always passed to the API.
    type: DWORD
        An integer that specifies the type of the data, one of:
        REG_BINARY -- Binary data in any form.
        REG_DWORD -- A 32-bit number.
        REG_DWORD_LITTLE_ENDIAN -- A 32-bit number in little-endian format. Equivalent to REG_DWORD
        REG_DWORD_BIG_ENDIAN -- A 32-bit number in big-endian format.
        REG_EXPAND_SZ -- A null-terminated string that contains unexpanded
                         references to environment variables (for example,
                         %PATH%).
        REG_LINK -- A Unicode symbolic link.
        REG_MULTI_SZ -- A sequence of null-terminated strings, terminated
                        by two null characters.  Note that Python handles
                        this termination automatically.
        REG_NONE -- No defined value type.
        REG_QWORD -- A 64-bit number.
        REG_QWORD_LITTLE_ENDIAN -- A 64-bit number in little-endian format. Equivalent to REG_QWORD.
        REG_RESOURCE_LIST -- A device-driver resource list.
        REG_SZ -- A null-terminated string.
    value: object
        A string that specifies the new value.
    /

Stores data in the value field of an open registry key.

This method can also set additional value and type information for the
specified key.  The key identified by the key parameter must have been
opened with KEY_SET_VALUE access.

To open the key, use the CreateKeyEx() or OpenKeyEx() methods.

Value lengths are limited by available memory. Long values (more than
2048 bytes) should be stored as files with the filenames stored in
the configuration registry to help the registry perform efficiently.
[clinic start generated code]*/

static PyObject *
winreg_SetValueEx_impl(PyObject *module, HKEY key, const wchar_t *value_name,
                       PyObject *reserved, DWORD type, PyObject *value)
/*[clinic end generated code: output=295db04deb456d9e input=900a9e3990bfb196]*/
{
    LONG rc;
    BYTE *data = NULL;
    DWORD size;
    PyObject *result = NULL;

    if (!Py2Reg(value, type, &data, &size))
    {
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_ValueError,
                     "Could not convert the data to the specified type.");
        }
        return NULL;
    }
    if (PySys_Audit("winreg.SetValue", "nunO",
                    (Py_ssize_t)key, value_name, (Py_ssize_t)type,
                    value) < 0)
    {
        goto exit;
    }

    Py_BEGIN_ALLOW_THREADS
    rc = RegSetValueExW(key, value_name, 0, type, data, size);
    Py_END_ALLOW_THREADS
    if (rc == ERROR_SUCCESS) {
        result = Py_NewRef(Py_None);
    }
    else {
        PyErr_SetFromWindowsErrWithFunction(rc, "RegSetValueEx");
    }

exit:
    PyMem_Free(data);
    return result;
}

#if defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM)

/*[clinic input]
winreg.DisableReflectionKey

    key: HKEY
        An already open key, or any one of the predefined HKEY_* constants.
    /

Disables registry reflection for 32bit processes running on a 64bit OS.

Will generally raise NotImplementedError if executed on a 32bit OS.

If the key is not on the reflection list, the function succeeds but has
no effect.  Disabling reflection for a key does not affect reflection
of any subkeys.
[clinic start generated code]*/

static PyObject *
winreg_DisableReflectionKey_impl(PyObject *module, HKEY key)
/*[clinic end generated code: output=830cce504cc764b4 input=70bece2dee02e073]*/
{
    HMODULE hMod;
    typedef LONG (WINAPI *RDRKFunc)(HKEY);
    RDRKFunc pfn = NULL;
    LONG rc;

    if (PySys_Audit("winreg.DisableReflectionKey", "n", (Py_ssize_t)key) < 0) {
        return NULL;
    }

    /* Only available on 64bit platforms, so we must load it
       dynamically.*/
    Py_BEGIN_ALLOW_THREADS
    hMod = GetModuleHandleW(L"advapi32.dll");
    if (hMod)
        pfn = (RDRKFunc)GetProcAddress(hMod,
                                       "RegDisableReflectionKey");
    Py_END_ALLOW_THREADS
    if (!pfn) {
        PyErr_SetString(PyExc_NotImplementedError,
                        "not implemented on this platform");
        return NULL;
    }
    Py_BEGIN_ALLOW_THREADS
    rc = (*pfn)(key);
    Py_END_ALLOW_THREADS
    if (rc != ERROR_SUCCESS)
        return PyErr_SetFromWindowsErrWithFunction(rc,
                                                   "RegDisableReflectionKey");
    Py_RETURN_NONE;
}

/*[clinic input]
winreg.EnableReflectionKey

    key: HKEY
        An already open key, or any one of the predefined HKEY_* constants.
    /

Restores registry reflection for the specified disabled key.

Will generally raise NotImplementedError if executed on a 32bit OS.
Restoring reflection for a key does not affect reflection of any
subkeys.
[clinic start generated code]*/

static PyObject *
winreg_EnableReflectionKey_impl(PyObject *module, HKEY key)
/*[clinic end generated code: output=86fa1385fdd9ce57 input=eeae770c6eb9f559]*/
{
    HMODULE hMod;
    typedef LONG (WINAPI *RERKFunc)(HKEY);
    RERKFunc pfn = NULL;
    LONG rc;

    if (PySys_Audit("winreg.EnableReflectionKey", "n", (Py_ssize_t)key) < 0) {
        return NULL;
    }

    /* Only available on 64bit platforms, so we must load it
       dynamically.*/
    Py_BEGIN_ALLOW_THREADS
    hMod = GetModuleHandleW(L"advapi32.dll");
    if (hMod)
        pfn = (RERKFunc)GetProcAddress(hMod,
                                       "RegEnableReflectionKey");
    Py_END_ALLOW_THREADS
    if (!pfn) {
        PyErr_SetString(PyExc_NotImplementedError,
                        "not implemented on this platform");
        return NULL;
    }
    Py_BEGIN_ALLOW_THREADS
    rc = (*pfn)(key);
    Py_END_ALLOW_THREADS
    if (rc != ERROR_SUCCESS)
        return PyErr_SetFromWindowsErrWithFunction(rc,
                                                   "RegEnableReflectionKey");
    Py_RETURN_NONE;
}

/*[clinic input]
winreg.QueryReflectionKey

    key: HKEY
        An already open key, or any one of the predefined HKEY_* constants.
    /

Returns the reflection state for the specified key as a bool.

Will generally raise NotImplementedError if executed on a 32bit OS.
[clinic start generated code]*/

static PyObject *
winreg_QueryReflectionKey_impl(PyObject *module, HKEY key)
/*[clinic end generated code: output=4e774af288c3ebb9 input=a98fa51d55ade186]*/
{
    HMODULE hMod;
    typedef LONG (WINAPI *RQRKFunc)(HKEY, BOOL *);
    RQRKFunc pfn = NULL;
    BOOL result;
    LONG rc;

    if (PySys_Audit("winreg.QueryReflectionKey", "n", (Py_ssize_t)key) < 0) {
        return NULL;
    }

    /* Only available on 64bit platforms, so we must load it
       dynamically.*/
    Py_BEGIN_ALLOW_THREADS
    hMod = GetModuleHandleW(L"advapi32.dll");
    if (hMod)
        pfn = (RQRKFunc)GetProcAddress(hMod,
                                       "RegQueryReflectionKey");
    Py_END_ALLOW_THREADS
    if (!pfn) {
        PyErr_SetString(PyExc_NotImplementedError,
                        "not implemented on this platform");
        return NULL;
    }
    Py_BEGIN_ALLOW_THREADS
    rc = (*pfn)(key, &result);
    Py_END_ALLOW_THREADS
    if (rc != ERROR_SUCCESS)
        return PyErr_SetFromWindowsErrWithFunction(rc,
                                                   "RegQueryReflectionKey");
    return PyBool_FromLong(result);
}

#endif /* MS_WINDOWS_DESKTOP || MS_WINDOWS_SYSTEM */

static struct PyMethodDef winreg_methods[] = {
    WINREG_CLOSEKEY_METHODDEF
    WINREG_CONNECTREGISTRY_METHODDEF
    WINREG_CREATEKEY_METHODDEF
    WINREG_CREATEKEYEX_METHODDEF
    WINREG_DELETEKEY_METHODDEF
    WINREG_DELETEKEYEX_METHODDEF
    WINREG_DELETEVALUE_METHODDEF
    WINREG_DISABLEREFLECTIONKEY_METHODDEF
    WINREG_ENABLEREFLECTIONKEY_METHODDEF
    WINREG_ENUMKEY_METHODDEF
    WINREG_ENUMVALUE_METHODDEF
    WINREG_EXPANDENVIRONMENTSTRINGS_METHODDEF
    WINREG_FLUSHKEY_METHODDEF
    WINREG_LOADKEY_METHODDEF
    WINREG_OPENKEY_METHODDEF
    WINREG_OPENKEYEX_METHODDEF
    WINREG_QUERYVALUE_METHODDEF
    WINREG_QUERYVALUEEX_METHODDEF
    WINREG_QUERYINFOKEY_METHODDEF
    WINREG_QUERYREFLECTIONKEY_METHODDEF
    WINREG_SAVEKEY_METHODDEF
    WINREG_SETVALUE_METHODDEF
    WINREG_SETVALUEEX_METHODDEF
    NULL,
};

#define ADD_INT(VAL) do {                               \
    if (PyModule_AddIntConstant(m, #VAL, VAL) < 0) {    \
        return -1;                                      \
    }                                                   \
} while (0)

static int
inskey(PyObject *mod, const char *name, HKEY key)
{
    return PyModule_Add(mod, name, PyLong_FromVoidPtr(key));
}

#define ADD_KEY(VAL) do {           \
    if (inskey(m, #VAL, VAL) < 0) { \
        return -1;                  \
    }                               \
} while (0)

static int
exec_module(PyObject *m)
{
    winreg_state *st = (winreg_state *)_PyModule_GetState(m);

    st->PyHKEY_Type = (PyTypeObject *)
                       PyType_FromModuleAndSpec(m, &pyhkey_type_spec, NULL);
    if (st->PyHKEY_Type == NULL) {
        return -1;
    }
    if (PyModule_AddObjectRef(m, "HKEYType", (PyObject *)st->PyHKEY_Type) < 0) {
        return -1;
    }
    if (PyModule_AddObjectRef(m, "error", PyExc_OSError) < 0) {
        return -1;
    }

    /* Add the relevant constants */
    ADD_KEY(HKEY_CLASSES_ROOT);
    ADD_KEY(HKEY_CURRENT_USER);
    ADD_KEY(HKEY_LOCAL_MACHINE);
    ADD_KEY(HKEY_USERS);
    ADD_KEY(HKEY_PERFORMANCE_DATA);
#ifdef HKEY_CURRENT_CONFIG
    ADD_KEY(HKEY_CURRENT_CONFIG);
#endif
#ifdef HKEY_DYN_DATA
    ADD_KEY(HKEY_DYN_DATA);
#endif
    ADD_INT(KEY_QUERY_VALUE);
    ADD_INT(KEY_SET_VALUE);
    ADD_INT(KEY_CREATE_SUB_KEY);
    ADD_INT(KEY_ENUMERATE_SUB_KEYS);
    ADD_INT(KEY_NOTIFY);
    ADD_INT(KEY_CREATE_LINK);
    ADD_INT(KEY_READ);
    ADD_INT(KEY_WRITE);
    ADD_INT(KEY_EXECUTE);
    ADD_INT(KEY_ALL_ACCESS);
#ifdef KEY_WOW64_64KEY
    ADD_INT(KEY_WOW64_64KEY);
#endif
#ifdef KEY_WOW64_32KEY
    ADD_INT(KEY_WOW64_32KEY);
#endif
    ADD_INT(REG_OPTION_RESERVED);
    ADD_INT(REG_OPTION_NON_VOLATILE);
    ADD_INT(REG_OPTION_VOLATILE);
    ADD_INT(REG_OPTION_CREATE_LINK);
    ADD_INT(REG_OPTION_BACKUP_RESTORE);
    ADD_INT(REG_OPTION_OPEN_LINK);
    ADD_INT(REG_LEGAL_OPTION);
    ADD_INT(REG_CREATED_NEW_KEY);
    ADD_INT(REG_OPENED_EXISTING_KEY);
    ADD_INT(REG_WHOLE_HIVE_VOLATILE);
    ADD_INT(REG_REFRESH_HIVE);
    ADD_INT(REG_NO_LAZY_FLUSH);
    ADD_INT(REG_NOTIFY_CHANGE_NAME);
    ADD_INT(REG_NOTIFY_CHANGE_ATTRIBUTES);
    ADD_INT(REG_NOTIFY_CHANGE_LAST_SET);
    ADD_INT(REG_NOTIFY_CHANGE_SECURITY);
    ADD_INT(REG_LEGAL_CHANGE_FILTER);
    ADD_INT(REG_NONE);
    ADD_INT(REG_SZ);
    ADD_INT(REG_EXPAND_SZ);
    ADD_INT(REG_BINARY);
    ADD_INT(REG_DWORD);
    ADD_INT(REG_DWORD_LITTLE_ENDIAN);
    ADD_INT(REG_DWORD_BIG_ENDIAN);
    ADD_INT(REG_QWORD);
    ADD_INT(REG_QWORD_LITTLE_ENDIAN);
    ADD_INT(REG_LINK);
    ADD_INT(REG_MULTI_SZ);
    ADD_INT(REG_RESOURCE_LIST);
    ADD_INT(REG_FULL_RESOURCE_DESCRIPTOR);
    ADD_INT(REG_RESOURCE_REQUIREMENTS_LIST);

#undef ADD_INT
    return 0;
}

static PyModuleDef_Slot winreg_slots[] = {
    {Py_mod_exec, exec_module},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL}
};

static int
winreg_traverse(PyObject *module, visitproc visit, void *arg)
{
    winreg_state *state = _PyModule_GetState(module);
    Py_VISIT(state->PyHKEY_Type);
    return 0;
}

static int
winreg_clear(PyObject *module)
{
    winreg_state *state = _PyModule_GetState(module);
    Py_CLEAR(state->PyHKEY_Type);
    return 0;
}

static struct PyModuleDef winregmodule = {
    .m_base = PyModuleDef_HEAD_INIT,
    .m_name = "winreg",
    .m_doc = module_doc,
    .m_size = sizeof(winreg_state),
    .m_methods = winreg_methods,
    .m_slots = winreg_slots,
    .m_traverse = winreg_traverse,
    .m_clear = winreg_clear,
};

PyMODINIT_FUNC PyInit_winreg(void)
{
    return PyModuleDef_Init(&winregmodule);
}

#endif /* MS_WINDOWS_DESKTOP || MS_WINDOWS_SYSTEM || MS_WINDOWS_GAMES */
