#include "parts.h"
#include "util.h"

#include "frameobject.h"          // PyFrame_New()


static PyObject *
frame_getlocals(PyObject *self, PyObject *frame)
{
    if (!PyFrame_Check(frame)) {
        PyErr_SetString(PyExc_TypeError, "argument must be a frame");
        return NULL;
    }
    return PyFrame_GetLocals((PyFrameObject *)frame);
}


static PyObject *
frame_getglobals(PyObject *self, PyObject *frame)
{
    if (!PyFrame_Check(frame)) {
        PyErr_SetString(PyExc_TypeError, "argument must be a frame");
        return NULL;
    }
    return PyFrame_GetGlobals((PyFrameObject *)frame);
}


static PyObject *
frame_getgenerator(PyObject *self, PyObject *frame)
{
    if (!PyFrame_Check(frame)) {
        PyErr_SetString(PyExc_TypeError, "argument must be a frame");
        return NULL;
    }
    return PyFrame_GetGenerator((PyFrameObject *)frame);
}


static PyObject *
frame_getbuiltins(PyObject *self, PyObject *frame)
{
    if (!PyFrame_Check(frame)) {
        PyErr_SetString(PyExc_TypeError, "argument must be a frame");
        return NULL;
    }
    return PyFrame_GetBuiltins((PyFrameObject *)frame);
}


static PyObject *
frame_getlasti(PyObject *self, PyObject *frame)
{
    if (!PyFrame_Check(frame)) {
        PyErr_SetString(PyExc_TypeError, "argument must be a frame");
        return NULL;
    }
    int lasti = PyFrame_GetLasti((PyFrameObject *)frame);
    if (lasti < 0) {
        assert(lasti == -1);
        Py_RETURN_NONE;
    }
    return PyLong_FromLong(lasti);
}


static PyObject *
frame_new(PyObject *self, PyObject *args)
{
    PyObject *code, *globals, *locals;
    if (!PyArg_ParseTuple(args, "OOO", &code, &globals, &locals)) {
        return NULL;
    }
    if (!PyCode_Check(code)) {
        PyErr_SetString(PyExc_TypeError, "argument must be a code object");
        return NULL;
    }
    PyThreadState *tstate = PyThreadState_Get();

    return (PyObject *)PyFrame_New(tstate, (PyCodeObject *)code, globals, locals);
}


static PyObject *
frame_getvar(PyObject *self, PyObject *args)
{
    PyObject *frame, *name;
    if (!PyArg_ParseTuple(args, "OO", &frame, &name)) {
        return NULL;
    }
    if (!PyFrame_Check(frame)) {
        PyErr_SetString(PyExc_TypeError, "argument must be a frame");
        return NULL;
    }

    return PyFrame_GetVar((PyFrameObject *)frame, name);
}


static PyObject *
frame_getvarstring(PyObject *self, PyObject *args)
{
    PyObject *frame;
    const char *name;
    if (!PyArg_ParseTuple(args, "Oy", &frame, &name)) {
        return NULL;
    }
    if (!PyFrame_Check(frame)) {
        PyErr_SetString(PyExc_TypeError, "argument must be a frame");
        return NULL;
    }

    return PyFrame_GetVarString((PyFrameObject *)frame, name);
}


static PyMethodDef test_methods[] = {
    {"frame_getlocals", frame_getlocals, METH_O, NULL},
    {"frame_getglobals", frame_getglobals, METH_O, NULL},
    {"frame_getgenerator", frame_getgenerator, METH_O, NULL},
    {"frame_getbuiltins", frame_getbuiltins, METH_O, NULL},
    {"frame_getlasti", frame_getlasti, METH_O, NULL},
    {"frame_new", frame_new, METH_VARARGS, NULL},
    {"frame_getvar", frame_getvar, METH_VARARGS, NULL},
    {"frame_getvarstring", frame_getvarstring, METH_VARARGS, NULL},
    {NULL},
};

int
_PyTestCapi_Init_Frame(PyObject *m)
{
    return PyModule_AddFunctions(m, test_methods);
}

