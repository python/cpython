
#include "Python.h"
#include "frameobject.h"
#include "pycore_framedata.h"
#include "pycore_object.h"        // _PyObject_GC_UNTRACK()

int
_Py_framedata_Traverse(_Py_framedata *fdata, visitproc visit, void *arg)
{
    Py_VISIT(fdata->frame_obj);
    Py_VISIT(fdata->globals);
    Py_VISIT(fdata->builtins);
    Py_VISIT(fdata->locals);
    Py_VISIT(fdata->code);
   /* locals */
    PyObject **locals = _Py_framedata_GetLocalsArray(fdata);
    for (int i = 0; i < fdata->nlocalsplus; i++) {
        Py_VISIT(locals[i]);
    }
    /* stack */
    for (int i = 0; i <fdata->stackdepth; i++) {
        Py_VISIT(fdata->stack[i]);
    }
    return 0;
}

PyFrameObject *
_Py_framedata_MakeAndSetFrameObject(_Py_framedata *fdata)
{
    assert(fdata->frame_obj == NULL);
    PyObject *error_type, *error_value, *error_traceback;
    PyErr_Fetch(&error_type, &error_value, &error_traceback);
    PyFrameObject *f = _PyFrame_New_NoTrack(fdata, 0);
    if (f == NULL) {
        Py_XDECREF(error_type);
        Py_XDECREF(error_value);
        Py_XDECREF(error_traceback);
    }
    else {
        PyErr_Restore(error_type, error_value, error_traceback);
    }
    fdata->frame_obj = f;
    return f;
}


static _Py_framedata *
copy_frame_to_heap(_Py_framedata *fdata)
{

    PyObject **locals = _Py_framedata_GetLocalsArray(fdata);
    Py_ssize_t size = ((char*)&fdata->stack[fdata->stackdepth]) - (char *)locals;
    PyObject **copy = PyMem_Malloc(size);
    if (copy == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    memcpy(copy, locals, size);
    _Py_framedata *res = (_Py_framedata *)(copy + fdata->nlocalsplus);
    return res;
}

static inline void
clear_specials(_Py_framedata *fdata)
{
    fdata->generator = NULL;
    Py_XDECREF(fdata->frame_obj);
    Py_XDECREF(fdata->locals);
    Py_DECREF(fdata->globals);
    Py_DECREF(fdata->builtins);
    Py_DECREF(fdata->code);
}

static void
take_ownership(PyFrameObject *f, _Py_framedata *fdata)
{
    assert(f->f_own_locals_memory == 0);
    assert(fdata->frame_obj == NULL);

    f->f_own_locals_memory = 1;
    f->f_fdata = fdata;
    assert(f->f_back == NULL);
    if (fdata->previous != NULL) {
        /* Link frame object's 'f_back' and remove link through frame data's 'previous' field */
        PyFrameObject *back = _Py_framedata_GetFrameObject(fdata->previous);
        if (back == NULL) {
            /* Memory error here. */
            assert(PyErr_ExceptionMatches(PyExc_MemoryError));
            /* Nothing we can do about it */
            PyErr_Clear();
            _PyErr_WriteUnraisableMsg("Out of memory lazily allocating frame->f_back", NULL);
        }
        else {
            f->f_back = (PyFrameObject *)Py_NewRef(back);
        }
        fdata->previous = NULL;
    }
    if (!_PyObject_GC_IS_TRACKED((PyObject *)f)) {
        _PyObject_GC_TRACK((PyObject *)f);
    }
}

int
_Py_framedata_Clear(_Py_framedata * fdata, int take)
{
    PyObject **localsarray = ((PyObject **)fdata)-fdata->nlocalsplus;
    if (fdata->frame_obj) {
        PyFrameObject *f = fdata->frame_obj;
        fdata->frame_obj = NULL;
        if (Py_REFCNT(f) > 1) {
            if (!take) {
                fdata = copy_frame_to_heap(fdata);
                if (fdata == NULL) {
                    return -1;
                }
            }
            take_ownership(f, fdata);
            Py_DECREF(f);
            return 0;
        }
        Py_DECREF(f);
    }
    for (int i = 0; i < fdata->nlocalsplus; i++) {
        Py_XDECREF(localsarray[i]);
    }
    assert(fdata->stackdepth >= 0);
    for (int i = 0; i < fdata->stackdepth; i++) {
        Py_DECREF(fdata->stack[i]);
    }
    clear_specials(fdata);
    if (take) {
        PyMem_Free(_Py_framedata_GetLocalsArray(fdata));
    }
    return 0;
}
