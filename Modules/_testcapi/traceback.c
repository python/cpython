#include "parts.h"

#include "traceback.h"  // PyUnstable_CollectCallStack, PyUnstable_PrintCallStack


/* collect_call_stack([max_frames]) ->
 *     list of (filename, lineno, name, filename_truncated, name_truncated)
 *     | None
 *
 * Calls PyUnstable_CollectCallStack() on the current tstate and returns the
 * collected frames as a list of 5-tuples.  lineno is an int or None if
 * unknown; filename_truncated and name_truncated are 0 or 1.  Returns None
 * if the tstate has no Python frame (i.e. PyUnstable_CollectCallStack()
 * returned -1). */
static PyObject *
traceback_collect(PyObject *self, PyObject *args)
{
    int max_frames = 10;
    if (!PyArg_ParseTuple(args, "|i", &max_frames)) {
        return NULL;
    }
    if (max_frames <= 0) {
        PyErr_SetString(PyExc_ValueError, "max_frames must be positive");
        return NULL;
    }

    PyUnstable_FrameInfo *frames = PyMem_Malloc(
        sizeof(PyUnstable_FrameInfo) * max_frames);
    if (frames == NULL) {
        return PyErr_NoMemory();
    }

    PyThreadState *tstate = PyThreadState_Get();
    int n = PyUnstable_CollectCallStack(tstate, frames, max_frames);

    if (n < 0) {
        PyMem_Free(frames);
        Py_RETURN_NONE;
    }

    PyObject *result = PyList_New(n);
    if (result == NULL) {
        PyMem_Free(frames);
        return NULL;
    }

    for (int i = 0; i < n; i++) {
        PyObject *lineno = frames[i].lineno >= 0
            ? PyLong_FromLong(frames[i].lineno)
            : Py_NewRef(Py_None);
        if (lineno == NULL) {
            Py_DECREF(result);
            PyMem_Free(frames);
            return NULL;
        }
        PyObject *item = Py_BuildValue("(sNsii)",
            frames[i].filename, lineno, frames[i].name,
            frames[i].filename_truncated,
            frames[i].name_truncated);
        if (item == NULL) {
            Py_DECREF(result);
            PyMem_Free(frames);
            return NULL;
        }
        PyList_SET_ITEM(result, i, item);
    }

    PyMem_Free(frames);
    return result;
}


/* print_call_stack(fd, [(filename, lineno, name[, filename_truncated,
 *                        name_truncated]), ...][, write_header]) -> None
 *
 * Constructs a PyUnstable_FrameInfo array from a Python list of tuples and
 * calls PyUnstable_PrintCallStack().  Used to test the print format in
 * isolation from collection.  The optional filename_truncated and
 * name_truncated fields allow testing the truncation display path directly. */
static PyObject *
traceback_print(PyObject *self, PyObject *args)
{
    int fd;
    PyObject *frame_list;
    int write_header = 1;
    if (!PyArg_ParseTuple(args, "iO!|i",
                          &fd, &PyList_Type, &frame_list, &write_header)) {
        return NULL;
    }

    Py_ssize_t n = PyList_GET_SIZE(frame_list);
    PyUnstable_FrameInfo *frames = PyMem_Malloc(
        sizeof(PyUnstable_FrameInfo) * (n ? n : 1));
    if (frames == NULL) {
        return PyErr_NoMemory();
    }

    for (Py_ssize_t i = 0; i < n; i++) {
        PyObject *item = PyList_GET_ITEM(frame_list, i);
        const char *filename, *name;
        int lineno;
        int filename_truncated = 0, name_truncated = 0;
        if (!PyArg_ParseTuple(item, "sis|ii",
                              &filename, &lineno, &name,
                              &filename_truncated, &name_truncated)) {
            PyMem_Free(frames);
            return NULL;
        }
        PyOS_snprintf(frames[i].filename, Py_UNSTABLE_FRAMEINFO_STRSIZE,
                      "%s", filename);
        frames[i].lineno = lineno;
        PyOS_snprintf(frames[i].name, Py_UNSTABLE_FRAMEINFO_STRSIZE,
                      "%s", name);
        frames[i].filename_truncated = filename_truncated;
        frames[i].name_truncated = name_truncated;
    }

    PyUnstable_PrintCallStack(fd, frames, (int)n, write_header);
    PyMem_Free(frames);
    Py_RETURN_NONE;
}


/* collect_and_print_call_stack(fd[, max_frames[, write_header]]) -> None
 *
 * Calls PyUnstable_CollectCallStack() + PyUnstable_PrintCallStack() in one
 * step.  Used to test the end-to-end path with a real Python call stack. */
static PyObject *
traceback_collect_and_print(PyObject *self, PyObject *args)
{
    int fd;
    int max_frames = 10;
    int write_header = 1;
    if (!PyArg_ParseTuple(args, "i|ii", &fd, &max_frames, &write_header)) {
        return NULL;
    }
    if (max_frames <= 0) {
        PyErr_SetString(PyExc_ValueError, "max_frames must be positive");
        return NULL;
    }

    PyUnstable_FrameInfo *frames = PyMem_Malloc(
        sizeof(PyUnstable_FrameInfo) * max_frames);
    if (frames == NULL) {
        return PyErr_NoMemory();
    }

    PyThreadState *tstate = PyThreadState_Get();
    int n = PyUnstable_CollectCallStack(tstate, frames, max_frames);
    if (n >= 0) {
        PyUnstable_PrintCallStack(fd, frames, n, write_header);
    }
    PyMem_Free(frames);
    Py_RETURN_NONE;
}


static PyMethodDef traceback_methods[] = {
    {"collect_call_stack",           traceback_collect,             METH_VARARGS},
    {"print_call_stack",             traceback_print,               METH_VARARGS},
    {"collect_and_print_call_stack", traceback_collect_and_print,   METH_VARARGS},
    {NULL},
};

int
_PyTestCapi_Init_Traceback(PyObject *mod)
{
    if (PyModule_AddIntConstant(mod, "FRAMEINFO_STRSIZE",
                                Py_UNSTABLE_FRAMEINFO_STRSIZE) < 0) {
        return -1;
    }
    if (PyModule_AddFunctions(mod, traceback_methods) < 0) {
        return -1;
    }
    return 0;
}
