#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "Python.h"
// windows.h must be included before pycore internal headers
#ifdef MS_WIN32
#  include <windows.h>
#endif

#include "pycore_call.h"          // _PyObject_CallNoArgs()
#include "pycore_runtime.h"       // _Py_ID()

#include <ffi.h>
#include "ctypes.h"

/**************************************************************/

static int
CThunkObject_traverse(PyObject *myself, visitproc visit, void *arg)
{
    CThunkObject *self = (CThunkObject *)myself;
    Py_VISIT(Py_TYPE(self));
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

static void
CThunkObject_dealloc(PyObject *myself)
{
    CThunkObject *self = (CThunkObject *)myself;
    PyTypeObject *tp = Py_TYPE(myself);
    PyObject_GC_UnTrack(self);
    (void)CThunkObject_clear(myself);
    if (self->pcl_write) {
        Py_ffi_closure_free(self->pcl_write);
    }
    PyObject_GC_Del(self);
    Py_DECREF(tp);
}

static PyType_Slot cthunk_slots[] = {
    {Py_tp_doc, (void *)PyDoc_STR("CThunkObject")},
    {Py_tp_dealloc, CThunkObject_dealloc},
    {Py_tp_traverse, CThunkObject_traverse},
    {Py_tp_clear, CThunkObject_clear},
    {0, NULL},
};

PyType_Spec cthunk_spec = {
    .name = "_ctypes.CThunkObject",
    .basicsize = sizeof(CThunkObject),
    .itemsize = sizeof(ffi_type),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
              Py_TPFLAGS_IMMUTABLETYPE | Py_TPFLAGS_DISALLOW_INSTANTIATION),
    .slots = cthunk_slots,
};

/**************************************************************/

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
static int
TryAddRef(PyObject *cnv, CDataObject *obj)
{
    IUnknown *punk;
    PyObject *attrdict = _PyType_GetDict((PyTypeObject *)cnv);
    if (!attrdict) {
        return 0;
    }
    int r = PyDict_Contains(attrdict, &_Py_ID(_needs_com_addref_));
    if (r <= 0) {
        return r;
    }

    punk = *(IUnknown **)obj->b_ptr;
    if (punk)
        punk->lpVtbl->AddRef(punk);
    return 0;
}
#endif

/******************************************************************************
 *
 * Call the python object with all arguments
 *
 */

// BEWARE: The GIL needs to be held throughout the function
static void _CallPythonObject(ctypes_state *st,
                              void *mem,
                              ffi_type *restype,
                              SETFUNC setfunc,
                              PyObject *callable,
                              PyObject *converters,
                              int flags,
                              void **pArgs)
{
    PyObject *result = NULL;
    Py_ssize_t i = 0, j = 0, nargs = 0;
    PyObject *error_object = NULL;
    int *space;

    assert(PyTuple_Check(converters));
    nargs = PyTuple_GET_SIZE(converters);
    assert(nargs <= CTYPES_MAX_ARGCOUNT);
    PyObject **args = alloca(nargs * sizeof(PyObject *));
    PyObject **cnvs = PySequence_Fast_ITEMS(converters);
    for (i = 0; i < nargs; i++) {
        PyObject *cnv = cnvs[i]; // borrowed ref

        StgInfo *info;
        if (PyStgInfo_FromType(st, cnv, &info) < 0) {
            goto Error;
        }

        if (info && info->getfunc && !_ctypes_simple_instance(st, cnv)) {
            PyObject *v = info->getfunc(*pArgs, info->size);
            if (!v) {
                goto Error;
            }
            args[i] = v;
            /* XXX XXX XX
               We have the problem that c_byte or c_short have info->size of
               1 resp. 4, but these parameters are pushed as sizeof(int) bytes.
               BTW, the same problem occurs when they are pushed as parameters
            */
        }
        else if (info) {
            /* Hm, shouldn't we use PyCData_AtAddress() or something like that instead? */
            CDataObject *obj = (CDataObject *)_PyObject_CallNoArgs(cnv);
            if (!obj) {
                goto Error;
            }
            if (!CDataObject_Check(st, obj)) {
                PyErr_Format(PyExc_TypeError,
                             "%R returned unexpected result of type %T", cnv, obj);
                Py_DECREF(obj);
                goto Error;
            }
            memcpy(obj->b_ptr, *pArgs, info->size);
            args[i] = (PyObject *)obj;
#ifdef MS_WIN32
            if (TryAddRef(cnv, obj) < 0) {
                goto Error;
            }
#endif
        } else {
            PyErr_Format(PyExc_TypeError,
                         "cannot build parameter of type %R", cnv);
            goto Error;
        }
        /* XXX error handling! */
        pArgs++;
    }

    if (flags & (FUNCFLAG_USE_ERRNO | FUNCFLAG_USE_LASTERROR)) {
        error_object = _ctypes_get_errobj(st, &space);
        if (error_object == NULL) {
            PyErr_FormatUnraisable(
                    "Exception ignored while setting error for "
                    "ctypes callback function %R",
                    callable);
            goto Done;
        }
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

    result = PyObject_Vectorcall(callable, args, nargs, NULL);
    if (result == NULL) {
        PyErr_FormatUnraisable("Exception ignored while "
                               "calling ctypes callback function %R",
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
        PyObject *keep = setfunc(mem, result, restype->size);

        if (keep == NULL) {
            /* Could not convert callback result. */
            PyErr_FormatUnraisable(
                    "Exception ignored while converting result "
                    "of ctypes callback function %R",
                    callable);
        }
        else if (setfunc != _ctypes_get_fielddesc("O")->setfunc) {
            if (keep == Py_None) {
                /* Nothing to keep */
                Py_DECREF(keep);
            }
            else if (PyErr_WarnEx(PyExc_RuntimeWarning,
                                  "memory leak in callback function.",
                                  1) == -1) {
                PyErr_FormatUnraisable(
                        "Exception ignored while converting result "
                        "of ctypes callback function %R",
                        callable);
            }
        }
    }

    Py_XDECREF(result);

  Done:
    for (j = 0; j < i; j++) {
        Py_DECREF(args[j]);
    }
    return;

  Error:
    PyErr_FormatUnraisable(
            "Exception ignored while creating argument %zd for "
            "ctypes callback function %R",
            i, callable);
    goto Done;
}

static void closure_fcn(ffi_cif *cif,
                        void *resp,
                        void **args,
                        void *userdata)
{
    PyGILState_STATE state = PyGILState_Ensure();

    CThunkObject *p = (CThunkObject *)userdata;
    ctypes_state *st = get_module_state_by_class(Py_TYPE(p));

    _CallPythonObject(st,
                      resp,
                      p->ffi_restype,
                      p->setfunc,
                      p->callable,
                      p->converters,
                      p->flags,
                      args);

    PyGILState_Release(state);
}

static CThunkObject* CThunkObject_new(ctypes_state *st, Py_ssize_t nargs)
{
    CThunkObject *p;
    Py_ssize_t i;

    p = PyObject_GC_NewVar(CThunkObject, st->PyCThunk_Type, nargs);
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

    for (i = 0; i < nargs + 1; ++i)
        p->atypes[i] = NULL;
    PyObject_GC_Track((PyObject *)p);
    return p;
}

CThunkObject *_ctypes_alloc_callback(ctypes_state *st,
                                    PyObject *callable,
                                    PyObject *converters,
                                    PyObject *restype,
                                    int flags)
{
    int result;
    CThunkObject *p;
    Py_ssize_t nargs, i;
    ffi_abi cc;

    assert(PyTuple_Check(converters));
    nargs = PyTuple_GET_SIZE(converters);
    p = CThunkObject_new(st, nargs);
    if (p == NULL)
        return NULL;

    assert(CThunk_CheckExact(st, (PyObject *)p));

    p->pcl_write = Py_ffi_closure_alloc(sizeof(ffi_closure), &p->pcl_exec);
    if (p->pcl_write == NULL) {
        PyErr_NoMemory();
        goto error;
    }

    p->flags = flags;
    PyObject **cnvs = PySequence_Fast_ITEMS(converters);
    for (i = 0; i < nargs; ++i) {
        PyObject *cnv = cnvs[i]; // borrowed ref
        p->atypes[i] = _ctypes_get_ffi_type(st, cnv);
    }
    p->atypes[i] = NULL;

    p->restype = Py_NewRef(restype);
    if (restype == Py_None) {
        p->setfunc = NULL;
        p->ffi_restype = &ffi_type_void;
    } else {
        StgInfo *info;
        if (PyStgInfo_FromType(st, restype, &info) < 0) {
            goto error;
        }

        if (info == NULL || info->setfunc == NULL) {
          PyErr_SetString(PyExc_TypeError,
                          "invalid result type for callback function");
          goto error;
        }
        p->setfunc = info->setfunc;
        p->ffi_restype = &info->ffi_type_pointer;
    }

    cc = FFI_DEFAULT_ABI;
#if defined(MS_WIN32) && !defined(_WIN32_WCE) && !defined(MS_WIN64) && !defined(_M_ARM)
    if ((flags & FUNCFLAG_CDECL) == 0)
        cc = FFI_STDCALL;
#endif
    result = ffi_prep_cif(&p->cif, cc,
                          Py_SAFE_DOWNCAST(nargs, Py_ssize_t, int),
                          p->ffi_restype,
                          &p->atypes[0]);
    if (result != FFI_OK) {
        PyErr_Format(PyExc_RuntimeError,
                     "ffi_prep_cif failed with %d", result);
        goto error;
    }


#if HAVE_FFI_PREP_CLOSURE_LOC
#   ifdef USING_APPLE_OS_LIBFFI
#    ifdef HAVE_BUILTIN_AVAILABLE
#      define HAVE_FFI_PREP_CLOSURE_LOC_RUNTIME __builtin_available(macos 10.15, ios 13, watchos 6, tvos 13, *)
#    else
#      define HAVE_FFI_PREP_CLOSURE_LOC_RUNTIME (ffi_prep_closure_loc != NULL)
#    endif
#   else
#      define HAVE_FFI_PREP_CLOSURE_LOC_RUNTIME 1
#   endif
    if (HAVE_FFI_PREP_CLOSURE_LOC_RUNTIME) {
        result = ffi_prep_closure_loc(p->pcl_write, &p->cif, closure_fcn,
                                    p,
                                    p->pcl_exec);
    } else
#endif
    {
#if defined(USING_APPLE_OS_LIBFFI) && defined(__arm64__)
        PyErr_Format(PyExc_NotImplementedError, "ffi_prep_closure_loc() is missing");
        goto error;
#else
        // GH-85272, GH-23327, GH-100540: On macOS,
        // HAVE_FFI_PREP_CLOSURE_LOC_RUNTIME is checked at runtime because the
        // symbol might not be available at runtime when targeting macOS 10.14
        // or earlier. Even if ffi_prep_closure_loc() is called in practice,
        // the deprecated ffi_prep_closure() code path is needed if
        // HAVE_FFI_PREP_CLOSURE_LOC_RUNTIME is false.
        //
        // On non-macOS platforms, even if HAVE_FFI_PREP_CLOSURE_LOC_RUNTIME is
        // defined as 1 and ffi_prep_closure_loc() is used in practice, this
        // code path is still compiled and emits a compiler warning. The
        // deprecated code path is likely to be removed by a simple
        // optimization pass.
        //
        // Ignore the compiler warning on the ffi_prep_closure() deprecation,
        // rather than using complex #if/#else code paths for the different
        // platforms.
        _Py_COMP_DIAG_PUSH
        _Py_COMP_DIAG_IGNORE_DEPR_DECLS
        result = ffi_prep_closure(p->pcl_write, &p->cif, closure_fcn, p);
        _Py_COMP_DIAG_POP
#endif
    }

    if (result != FFI_OK) {
        PyErr_Format(PyExc_RuntimeError,
                     "ffi_prep_closure failed with %d", result);
        goto error;
    }

    p->converters = Py_NewRef(converters);
    p->callable = Py_NewRef(callable);
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
    PyObject *func, *result;
    long retval;

    func = PyImport_ImportModuleAttrString("ctypes", "DllGetClassObject");
    if (!func) {
        /* There has been a warning before about this already */
        goto error;
    }

    {
        PyObject *py_rclsid = PyLong_FromVoidPtr((void *)rclsid);
        if (py_rclsid == NULL) {
            Py_DECREF(func);
            goto error;
        }
        PyObject *py_riid = PyLong_FromVoidPtr((void *)riid);
        if (py_riid == NULL) {
            Py_DECREF(func);
            Py_DECREF(py_rclsid);
            goto error;
        }
        PyObject *py_ppv = PyLong_FromVoidPtr(ppv);
        if (py_ppv == NULL) {
            Py_DECREF(py_rclsid);
            Py_DECREF(py_riid);
            Py_DECREF(func);
            goto error;
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
        goto error;
    }

    retval = PyLong_AsLong(result);
    if (PyErr_Occurred()) {
        Py_DECREF(result);
        goto error;
    }
    Py_DECREF(result);
    return retval;

error:
    PyErr_FormatUnraisable("Exception ignored while calling "
                           "ctypes.DllGetClassObject");
    return E_FAIL;
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
    PyObject *func = PyImport_ImportModuleAttrString("ctypes",
                                                     "DllCanUnloadNow");
    if (!func) {
        goto error;
    }

    PyObject *result = _PyObject_CallNoArgs(func);
    Py_DECREF(func);
    if (!result) {
        goto error;
    }

    long retval = PyLong_AsLong(result);
    if (PyErr_Occurred()) {
        Py_DECREF(result);
        goto error;
    }
    Py_DECREF(result);
    return retval;

error:
    PyErr_FormatUnraisable("Exception ignored while calling "
                           "ctypes.DllCanUnloadNow");
    return E_FAIL;
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
