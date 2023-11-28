#include "Python.h"
#include "pycore_tracemalloc.h"  // _PyTraceMalloc_IsTracing

#include "clinic/_tracemalloc.c.h"


/*[clinic input]
module _tracemalloc
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=708a98302fc46e5f]*/


/*[clinic input]
_tracemalloc.is_tracing

Return True if the tracemalloc module is tracing Python memory allocations.
[clinic start generated code]*/

static PyObject *
_tracemalloc_is_tracing_impl(PyObject *module)
/*[clinic end generated code: output=2d763b42601cd3ef input=af104b0a00192f63]*/
{
    return PyBool_FromLong(_PyTraceMalloc_IsTracing());
}


/*[clinic input]
_tracemalloc.clear_traces

Clear traces of memory blocks allocated by Python.
[clinic start generated code]*/

static PyObject *
_tracemalloc_clear_traces_impl(PyObject *module)
/*[clinic end generated code: output=a86080ee41b84197 input=0dab5b6c785183a5]*/
{
    _PyTraceMalloc_ClearTraces();
    Py_RETURN_NONE;
}


/*[clinic input]
_tracemalloc._get_traces

Get traces of all memory blocks allocated by Python.

Return a list of (size: int, traceback: tuple) tuples.
traceback is a tuple of (filename: str, lineno: int) tuples.

Return an empty list if the tracemalloc module is disabled.
[clinic start generated code]*/

static PyObject *
_tracemalloc__get_traces_impl(PyObject *module)
/*[clinic end generated code: output=e9929876ced4b5cc input=6c7d2230b24255aa]*/
{
    return _PyTraceMalloc_GetTraces();
}



/*[clinic input]
_tracemalloc._get_object_traceback

    obj: object
    /

Get the traceback where the Python object obj was allocated.

Return a tuple of (filename: str, lineno: int) tuples.
Return None if the tracemalloc module is disabled or did not
trace the allocation of the object.
[clinic start generated code]*/

static PyObject *
_tracemalloc__get_object_traceback(PyObject *module, PyObject *obj)
/*[clinic end generated code: output=41ee0553a658b0aa input=29495f1b21c53212]*/
{
    return _PyTraceMalloc_GetObjectTraceback(obj);
}


/*[clinic input]
_tracemalloc.start

    nframe: int = 1
    /

Start tracing Python memory allocations.

Also set the maximum number of frames stored in the traceback of a
trace to nframe.
[clinic start generated code]*/

static PyObject *
_tracemalloc_start_impl(PyObject *module, int nframe)
/*[clinic end generated code: output=caae05c23c159d3c input=40d849b5b29d1933]*/
{
    if (_PyTraceMalloc_Start(nframe) < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}


/*[clinic input]
_tracemalloc.stop

Stop tracing Python memory allocations.

Also clear traces of memory blocks allocated by Python.
[clinic start generated code]*/

static PyObject *
_tracemalloc_stop_impl(PyObject *module)
/*[clinic end generated code: output=c3c42ae03e3955cd input=7478f075e51dae18]*/
{
    _PyTraceMalloc_Stop();
    Py_RETURN_NONE;
}


/*[clinic input]
_tracemalloc.get_traceback_limit

Get the maximum number of frames stored in the traceback of a trace.

By default, a trace of an allocated memory block only stores
the most recent frame: the limit is 1.
[clinic start generated code]*/

static PyObject *
_tracemalloc_get_traceback_limit_impl(PyObject *module)
/*[clinic end generated code: output=d556d9306ba95567 input=da3cd977fc68ae3b]*/
{
    return PyLong_FromLong(_PyTraceMalloc_GetTracebackLimit());
}

/*[clinic input]
_tracemalloc.get_tracemalloc_memory

Get the memory usage in bytes of the tracemalloc module.

This memory is used internally to trace memory allocations.
[clinic start generated code]*/

static PyObject *
_tracemalloc_get_tracemalloc_memory_impl(PyObject *module)
/*[clinic end generated code: output=e3f14e280a55f5aa input=5d919c0f4d5132ad]*/
{
    return PyLong_FromSize_t(_PyTraceMalloc_GetMemory());
}


/*[clinic input]
_tracemalloc.get_traced_memory

Get the current size and peak size of memory blocks traced by tracemalloc.

Returns a tuple: (current: int, peak: int).
[clinic start generated code]*/

static PyObject *
_tracemalloc_get_traced_memory_impl(PyObject *module)
/*[clinic end generated code: output=5b167189adb9e782 input=61ddb5478400ff66]*/
{
    return _PyTraceMalloc_GetTracedMemory();
}

/*[clinic input]
_tracemalloc.reset_peak

Set the peak size of memory blocks traced by tracemalloc to the current size.

Do nothing if the tracemalloc module is not tracing memory allocations.

[clinic start generated code]*/

static PyObject *
_tracemalloc_reset_peak_impl(PyObject *module)
/*[clinic end generated code: output=140c2870f691dbb2 input=18afd0635066e9ce]*/
{
    _PyTraceMalloc_ResetPeak();
    Py_RETURN_NONE;
}


static PyMethodDef module_methods[] = {
    _TRACEMALLOC_IS_TRACING_METHODDEF
    _TRACEMALLOC_CLEAR_TRACES_METHODDEF
    _TRACEMALLOC__GET_TRACES_METHODDEF
    _TRACEMALLOC__GET_OBJECT_TRACEBACK_METHODDEF
    _TRACEMALLOC_START_METHODDEF
    _TRACEMALLOC_STOP_METHODDEF
    _TRACEMALLOC_GET_TRACEBACK_LIMIT_METHODDEF
    _TRACEMALLOC_GET_TRACEMALLOC_MEMORY_METHODDEF
    _TRACEMALLOC_GET_TRACED_MEMORY_METHODDEF
    _TRACEMALLOC_RESET_PEAK_METHODDEF
    /* sentinel */
    {NULL, NULL}
};

PyDoc_STRVAR(module_doc,
"Debug module to trace memory blocks allocated by Python.");

static struct PyModuleDef module_def = {
    PyModuleDef_HEAD_INIT,
    "_tracemalloc",
    module_doc,
    0, /* non-negative size to be able to unload the module */
    module_methods,
    NULL,
};

PyMODINIT_FUNC
PyInit__tracemalloc(void)
{
    PyObject *m;
    m = PyModule_Create(&module_def);
    if (m == NULL)
        return NULL;

    if (_PyTraceMalloc_Init() < 0) {
        Py_DECREF(m);
        return NULL;
    }

    return m;
}
