
#include "Python.h"
#include "frameobject.h"
#include "pycore_xframe.h"
#include "pycore_object.h"        // _PyObject_GC_UNTRACK()

int
_PyExecFrame_Traverse(_PyExecFrame *xframe, visitproc visit, void *arg)
{
    Py_VISIT(xframe->xf_frame_obj);
    Py_VISIT(xframe->xf_globals);
    Py_VISIT(xframe->xf_builtins);
    Py_VISIT(xframe->xf_locals);
    Py_VISIT(xframe->xf_code);
   /* locals */
    PyObject **locals = _PyExecFrame_GetLocalsArray(xframe);
    for (int i = 0; i < xframe->xf_nlocalsplus; i++) {
        Py_VISIT(locals[i]);
    }
    /* stack */
    for (int i = 0; i <xframe->xf_stackdepth; i++) {
        Py_VISIT(xframe->xf_stack[i]);
    }
    return 0;
}

PyFrameObject *
_PyExecFrame_MakeAndSetFrameObject(_PyExecFrame *xframe)
{
    assert(xframe->xf_frame_obj == NULL);
    PyObject *error_type, *error_value, *error_traceback;
    PyErr_Fetch(&error_type, &error_value, &error_traceback);
    PyFrameObject *f = _PyFrame_New_NoTrack(xframe, 0);
    if (f == NULL) {
        Py_XDECREF(error_type);
        Py_XDECREF(error_value);
        Py_XDECREF(error_traceback);
    }
    else {
        PyErr_Restore(error_type, error_value, error_traceback);
    }
    xframe->xf_frame_obj = f;
    return f;
}


static _PyExecFrame *
copy_frame_to_heap(_PyExecFrame *xframe)
{

    PyObject **locals = _PyExecFrame_GetLocalsArray(xframe);
    Py_ssize_t size = ((char*)&xframe->xf_stack[xframe->xf_stackdepth]) - (char *)locals;
    PyObject **copy = PyMem_Malloc(size);
    if (copy == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    memcpy(copy, locals, size);
    _PyExecFrame *res = (_PyExecFrame *)(copy + xframe->xf_nlocalsplus);
    return res;
}

static inline void
clear_specials(_PyExecFrame *xframe)
{
    xframe->xf_generator = NULL;
    Py_XDECREF(xframe->xf_frame_obj);
    Py_XDECREF(xframe->xf_locals);
    Py_DECREF(xframe->xf_globals);
    Py_DECREF(xframe->xf_builtins);
    Py_DECREF(xframe->xf_code);
}

static void
take_ownership(PyFrameObject *f, _PyExecFrame *xframe)
{
    assert(f->f_own_locals_memory == 0);
    assert(xframe->xf_frame_obj == NULL);

    f->f_own_locals_memory = 1;
    f->f_xframe = xframe;
    assert(f->f_back == NULL);
    if (xframe->xf_previous != NULL) {
        /* Link introspection frame's f_back and remove link through execution frame's xf_previous */
        PyFrameObject *back = _PyExecFrame_GetFrameObject(xframe->xf_previous);
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
        xframe->xf_previous = NULL;
    }
    if (!_PyObject_GC_IS_TRACKED((PyObject *)f)) {
        _PyObject_GC_TRACK((PyObject *)f);
    }
}

int
_PyExecFrame_Clear(_PyExecFrame * xframe, int take)
{
    PyObject **localsarray = ((PyObject **)xframe)-xframe->xf_nlocalsplus;
    if (xframe->xf_frame_obj) {
        PyFrameObject *f = xframe->xf_frame_obj;
        xframe->xf_frame_obj = NULL;
        if (Py_REFCNT(f) > 1) {
            if (!take) {
                xframe = copy_frame_to_heap(xframe);
                if (xframe == NULL) {
                    return -1;
                }
            }
            take_ownership(f, xframe);
            Py_DECREF(f);
            return 0;
        }
        Py_DECREF(f);
    }
    for (int i = 0; i < xframe->xf_nlocalsplus; i++) {
        Py_XDECREF(localsarray[i]);
    }
    assert(xframe->xf_stackdepth >= 0);
    for (int i = 0; i < xframe->xf_stackdepth; i++) {
        Py_DECREF(xframe->xf_stack[i]);
    }
    clear_specials(xframe);
    if (take) {
        PyMem_Free(_PyExecFrame_GetLocalsArray(xframe));
    }
    return 0;
}
