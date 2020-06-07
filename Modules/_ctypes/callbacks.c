#include "Python.h"
#include "frameobject.h"

#include <ffi.h>
#ifdef MS_WIN32
#include <windows.h>
#endif
#include "ctypes.h"

/**************************************************************/

static void
CThunkObject_dealloc(PyObject *myself)
{
    CThunkObject *self = (CThunkObject *)myself;
    PyObject_GC_UnTrack(self);
    Py_XDECREF(self->converters);
    Py_XDECREF(self->callable);
    Py_XDECREF(self->restype);
    if (self->pcl_write)
        ffi_closure_free(self->pcl_write);
    PyObject_GC_Del(self);
}

static int
CThunkObject_traverse(PyObject *myself, visitproc visit, void *arg)
{
    CThunkObject *self = (CThunkObject *)myself;
    Py_VISIT(self->converters);
    Py_VISIT(self->callable);
    Py_VISIT(self->restype);
    return 0;
}

static int
CThunkObject_clear(PyObject *myself)
{
    CThunkObject *self = (CThunkObject *)myself;
    Py_CLEAR(self->converters);
    Py_CLEAR(self->callable);
    Py_CLEAR(self->restype);
    return 0;
}

PyTypeObject PyCThunk_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_ctypes.CThunkObject",
    sizeof(CThunkObject),                       /* tp_basicsize */
    sizeof(ffi_type),                           /* tp_itemsize */
    CThunkObject_dealloc,                       /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    0,                                          /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    0,                                          /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,                            /* tp_flags */
    "CThunkObject",                             /* tp_doc */
    CThunkObject_traverse,                      /* tp_traverse */
    CThunkObject_clear,                         /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    0,                                          /* tp_methods */
    0,                                          /* tp_members */
};

/**************************************************************/

static void
PrintError(const char *msg, ...)
{
    char buf[512];
    PyObject *f = PySys_GetObject("stderr");
    va_list marker;

    va_start(marker, msg);
    vsnprintf(buf, sizeof(buf), msg, marker);
    va_end(marker);
    if (f != NULL && f != Py_None)
        PyFile_WriteString(buf, f);
    PyErr_Print();
}


#ifdef MS_WIN32
/*
 * We must call AddRef() on non-NULL COM pointers we receive as arguments
 * to callback functions - these functions are COM method implementations.
 * The Python instances we create have a __del__ method which calls Release().
 *
 * The presence of a class attribute named '_needs_com_addref_' triggers this
 * behaviour.  It would also be possible to call the AddRef() Python method,
 * after checking for PyObject_IsTrue(), but this would probably be somewhat
 * slower.
 */
static void
TryAddRef(StgDictObject *dict, CDataObject *obj)
{
    IUnknown *punk;
    _Py_IDENTIFIER(_needs_com_addref_);

    if (!_PyDict_GetItemIdWithError((PyObject *)dict, &PyId__needs_com_addref_)) {
        if (PyErr_Occurred()) {
            PrintError("getting _needs_com_addref_");
        }
        return;
    }

    punk = *(IUnknown **)obj->b_ptr;
    if (punk)
        punk->lpVtbl->AddRef(punk);
    return;
}
#endif

/******************************************************************************
 *
 * Call the python object with all arguments
 *
 */
static void _CallPythonObject(void *mem,
                              ffi_type *restype,
                              SETFUNC setfunc,
                              PyObject *callable,
                              PyObject *converters,
                              int flags,
                              void **pArgs)
{
    Py_ssize_t i;
    PyObject *result;
    PyObject *arglist = NULL;
    Py_ssize_t nArgs;
    PyObject *error_object = NULL;
    int *space;
    PyGILState_STATE state = PyGILState_Ensure();

    nArgs = PySequence_Length(converters);
    /* Hm. What to return in case of error?
       For COM, 0xFFFFFFFF seems better than 0.
    */
    if (nArgs < 0) {
        PrintError("BUG: PySequence_Length");
        goto Done;
    }

    arglist = PyTuple_New(nArgs);
    if (!arglist) {
        PrintError("PyTuple_New()");
        goto Done;
    }
    for (i = 0; i < nArgs; ++i) {
        /* Note: new reference! */
        PyObject *cnv = PySequence_GetItem(converters, i);
        StgDictObject *dict;
        if (cnv)
            dict = PyType_stgdict(cnv);
        else {
            PrintError("Getting argument converter %zd\n", i);
            goto Done;
        }

        if (dict && dict->getfunc && !_ctypes_simple_instance(cnv)) {
            PyObject *v = dict->getfunc(*pArgs, dict->size);
            if (!v) {
                PrintError("create argument %zd:\n", i);
                Py_DECREF(cnv);
                goto Done;
            }
            PyTuple_SET_ITEM(arglist, i, v);
            /* XXX XXX XX
               We have the problem that c_byte or c_short have dict->size of
               1 resp. 4, but these parameters are pushed as sizeof(int) bytes.
               BTW, the same problem occurs when they are pushed as parameters
            */
        } else if (dict) {
            /* Hm, shouldn't we use PyCData_AtAddress() or something like that instead? */
            CDataObject *obj = (CDataObject *)_PyObject_CallNoArg(cnv);
            if (!obj) {
                PrintError("create argument %zd:\n", i);
                Py_DECREF(cnv);
                goto Done;
            }
            if (!CDataObject_Check(obj)) {
                Py_DECREF(obj);
                Py_DECREF(cnv);
                PrintError("unexpected result of create argument %zd:\n", i);
                goto Done;
            }
            memcpy(obj->b_ptr, *pArgs, dict->size);
            PyTuple_SET_ITEM(arglist, i, (PyObject *)obj);
#ifdef MS_WIN32
            TryAddRef(dict, obj);
#endif
        } else {
            PyErr_SetString(PyExc_TypeError,
                            "cannot build parameter");
            PrintError("Parsing argument %zd\n", i);
            Py_DECREF(cnv);
            goto Done;
        }
        Py_DECREF(cnv);
        /* XXX error handling! */
        pArgs++;
    }

    if (flags & (FUNCFLAG_USE_ERRNO | FUNCFLAG_USE_LASTERROR)) {
        error_object = _ctypes_get_errobj(&space);
        if (error_object == NULL)
            goto Done;
        if (flags & FUNCFLAG_USE_ERRNO) {
            int temp = space[0];
            space[0] = errno;
            errno = temp;
        }
#ifdef MS_WIN32
        if (flags & FUNCFLAG_USE_LASTERROR) {
            int temp = space[1];
            space[1] = GetLastError();
            SetLastError(temp);
        }
#endif
    }

    result = PyObject_CallObject(callable, arglist);
    if (result == NULL) {
        _PyErr_WriteUnraisableMsg("on calling ctypes callback function",
                                  callable);
    }

#ifdef MS_WIN32
    if (flags & FUNCFLAG_USE_LASTERROR) {
        int temp = space[1];
        space[1] = GetLastError();
        SetLastError(temp);
    }
#endif
    if (flags & FUNCFLAG_USE_ERRNO) {
        int temp = space[0];
        space[0] = errno;
        errno = temp;
    }
    Py_XDECREF(error_object);

    if (restype != &ffi_type_void && result) {
        assert(setfunc);

#ifdef WORDS_BIGENDIAN
        /* See the corresponding code in _ctypes_callproc():
           in callproc.c, around line 1219. */
        if (restype->type != FFI_TYPE_FLOAT && restype->size < sizeof(ffi_arg)) {
            mem = (char *)mem + sizeof(ffi_arg) - restype->size;
        }
#endif

        /* keep is an object we have to keep alive so that the result
           stays valid.  If there is no such object, the setfunc will
           have returned Py_None.

           If there is such an object, we have no choice than to keep
           it alive forever - but a refcount and/or memory leak will
           be the result.  EXCEPT when restype is py_object - Python
           itself knows how to manage the refcount of these objects.
        */
        PyObject *keep = setfunc(mem, result, 0);

        if (keep == NULL) {
            /* Could not convert callback result. */
            _PyErr_WriteUnraisableMsg("on converting result "
                                      "of ctypes callback function",
                                      callable);
        }
        else if (keep == Py_None) {
            /* Nothing to keep */
            Py_DECREF(keep);
        }
        else if (setfunc != _ctypes_get_fielddesc("O")->setfunc) {
            if (-1 == PyErr_WarnEx(PyExc_RuntimeWarning,
                                   "memory leak in callback function.",
                                   1))
            {
                _PyErr_WriteUnraisableMsg("on converting result "
                                          "of ctypes callback function",
                                          callable);
            }
        }
    }

    Py_XDECREF(result);

  Done:
    Py_XDECREF(arglist);
    PyGILState_Release(state);
}

static void closure_fcn(ffi_cif *cif,
                        void *resp,
                        void **args,
                        void *userdata)
{
    CThunkObject *p = (CThunkObject *)userdata;

    _CallPythonObject(resp,
                      p->ffi_restype,
                      p->setfunc,
                      p->callable,
                      p->converters,
                      p->flags,
                      args);
}

static CThunkObject* CThunkObject_new(Py_ssize_t nArgs)
{
    CThunkObject *p;
    Py_ssize_t i;

    p = PyObject_GC_NewVar(CThunkObject, &PyCThunk_Type, nArgs);
    if (p == NULL) {
        return NULL;
    }

    p->pcl_write = NULL;
    p->pcl_exec = NULL;
    memset(&p->cif, 0, sizeof(p->cif));
    p->flags = 0;
    p->converters = NULL;
    p->callable = NULL;
    p->restype = NULL;
    p->setfunc = NULL;
    p->ffi_restype = NULL;

    for (i = 0; i < nArgs + 1; ++i)
        p->atypes[i] = NULL;
    PyObject_GC_Track((PyObject *)p);
    return p;
}

CThunkObject *_ctypes_alloc_callback(PyObject *callable,
                                    PyObject *converters,
                                    PyObject *restype,
                                    int flags)
{
    int result;
    CThunkObject *p;
    Py_ssize_t nArgs, i;
    ffi_abi cc;

    nArgs = PySequence_Size(converters);
    p = CThunkObject_new(nArgs);
    if (p == NULL)
        return NULL;

    assert(CThunk_CheckExact((PyObject *)p));

    p->pcl_write = ffi_closure_alloc(sizeof(ffi_closure),
                                                                         &p->pcl_exec);
    if (p->pcl_write == NULL) {
        PyErr_NoMemory();
        goto error;
    }

    p->flags = flags;
    for (i = 0; i < nArgs; ++i) {
        PyObject *cnv = PySequence_GetItem(converters, i);
        if (cnv == NULL)
            goto error;
        p->atypes[i] = _ctypes_get_ffi_type(cnv);
        Py_DECREF(cnv);
    }
    p->atypes[i] = NULL;

    Py_INCREF(restype);
    p->restype = restype;
    if (restype == Py_None) {
        p->setfunc = NULL;
        p->ffi_restype = &ffi_type_void;
    } else {
        StgDictObject *dict = PyType_stgdict(restype);
        if (dict == NULL || dict->setfunc == NULL) {
          PyErr_SetString(PyExc_TypeError,
                          "invalid result type for callback function");
          goto error;
        }
        p->setfunc = dict->setfunc;
        p->ffi_restype = &dict->ffi_type_pointer;
    }

    cc = FFI_DEFAULT_ABI;
#if defined(MS_WIN32) && !defined(_WIN32_WCE) && !defined(MS_WIN64) && !defined(_M_ARM)
    if ((flags & FUNCFLAG_CDECL) == 0)
        cc = FFI_STDCALL;
#endif
    result = ffi_prep_cif(&p->cif, cc,
                          Py_SAFE_DOWNCAST(nArgs, Py_ssize_t, int),
                          _ctypes_get_ffi_type(restype),
                          &p->atypes[0]);
    if (result != FFI_OK) {
        PyErr_Format(PyExc_RuntimeError,
                     "ffi_prep_cif failed with %d", result);
        goto error;
    }
#if defined(X86_DARWIN) || defined(POWERPC_DARWIN)
    result = ffi_prep_closure(p->pcl_write, &p->cif, closure_fcn, p);
#else
    result = ffi_prep_closure_loc(p->pcl_write, &p->cif, closure_fcn,
                                  p,
                                  p->pcl_exec);
#endif
    if (result != FFI_OK) {
        PyErr_Format(PyExc_RuntimeError,
                     "ffi_prep_closure failed with %d", result);
        goto error;
    }

    Py_INCREF(converters);
    p->converters = converters;
    Py_INCREF(callable);
    p->callable = callable;
    return p;

  error:
    Py_XDECREF(p);
    return NULL;
}

#ifdef MS_WIN32

static void LoadPython(void)
{
    if (!Py_IsInitialized()) {
        Py_Initialize();
    }
}

/******************************************************************/

long Call_GetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    PyObject *mod, *func, *result;
    long retval;
    static PyObject *context;

    if (context == NULL)
        context = PyUnicode_InternFromString("_ctypes.DllGetClassObject");

    mod = PyImport_ImportModuleNoBlock("ctypes");
    if (!mod) {
        PyErr_WriteUnraisable(context ? context : Py_None);
        /* There has been a warning before about this already */
        return E_FAIL;
    }

    func = PyObject_GetAttrString(mod, "DllGetClassObject");
    Py_DECREF(mod);
    if (!func) {
        PyErr_WriteUnraisable(context ? context : Py_None);
        return E_FAIL;
    }

    {
        PyObject *py_rclsid = PyLong_FromVoidPtr((void *)rclsid);
        PyObject *py_riid = PyLong_FromVoidPtr((void *)riid);
        PyObject *py_ppv = PyLong_FromVoidPtr(ppv);
        if (!py_rclsid || !py_riid || !py_ppv) {
            Py_XDECREF(py_rclsid);
            Py_XDECREF(py_riid);
            Py_XDECREF(py_ppv);
            Py_DECREF(func);
            PyErr_WriteUnraisable(context ? context : Py_None);
            return E_FAIL;
        }
        result = PyObject_CallFunctionObjArgs(func,
                                              py_rclsid,
                                              py_riid,
                                              py_ppv,
                                              NULL);
        Py_DECREF(py_rclsid);
        Py_DECREF(py_riid);
        Py_DECREF(py_ppv);
    }
    Py_DECREF(func);
    if (!result) {
        PyErr_WriteUnraisable(context ? context : Py_None);
        return E_FAIL;
    }

    retval = PyLong_AsLong(result);
    if (PyErr_Occurred()) {
        PyErr_WriteUnraisable(context ? context : Py_None);
        retval = E_FAIL;
    }
    Py_DECREF(result);
    return retval;
}

STDAPI DllGetClassObject(REFCLSID rclsid,
                         REFIID riid,
                         LPVOID *ppv)
{
    long result;
    PyGILState_STATE state;

    LoadPython();
    state = PyGILState_Ensure();
    result = Call_GetClassObject(rclsid, riid, ppv);
    PyGILState_Release(state);
    return result;
}

long Call_CanUnloadNow(void)
{
    PyObject *mod, *func, *result;
    long retval;
    static PyObject *context;

    if (context == NULL)
        context = PyUnicode_InternFromString("_ctypes.DllCanUnloadNow");

    mod = PyImport_ImportModuleNoBlock("ctypes");
    if (!mod) {
/*              OutputDebugString("Could not import ctypes"); */
        /* We assume that this error can only occur when shutting
           down, so we silently ignore it */
        PyErr_Clear();
        return E_FAIL;
    }
    /* Other errors cannot be raised, but are printed to stderr */
    func = PyObject_GetAttrString(mod, "DllCanUnloadNow");
    Py_DECREF(mod);
    if (!func) {
        PyErr_WriteUnraisable(context ? context : Py_None);
        return E_FAIL;
    }

    result = _PyObject_CallNoArg(func);
    Py_DECREF(func);
    if (!result) {
        PyErr_WriteUnraisable(context ? context : Py_None);
        return E_FAIL;
    }

    retval = PyLong_AsLong(result);
    if (PyErr_Occurred()) {
        PyErr_WriteUnraisable(context ? context : Py_None);
        retval = E_FAIL;
    }
    Py_DECREF(result);
    return retval;
}

/*
  DllRegisterServer and DllUnregisterServer still missing
*/

STDAPI DllCanUnloadNow(void)
{
    long result;
    PyGILState_STATE state = PyGILState_Ensure();
    result = Call_CanUnloadNow();
    PyGILState_Release(state);
    return result;
}

#ifndef Py_NO_ENABLE_SHARED
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvRes)
{
    switch(fdwReason) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hinstDLL);
        break;
    }
    return TRUE;
}
#endif

#endif

/*
 Local Variables:
 compile-command: "cd .. && python setup.py -q build_ext"
 End:
*/
