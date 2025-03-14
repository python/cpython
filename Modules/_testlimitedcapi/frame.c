#include "parts.h"
#include "util.h"


static PyObject *
frame_getlinenumber(PyObject *self, PyObject *frame)
{
    int lineno = PyFrame_GetLineNumber((PyFrameObject*)frame);
    return PyLong_FromLong(lineno);
}


static PyMethodDef test_methods[] = {
    {"frame_getlinenumber", frame_getlinenumber, METH_O, NULL},
    {NULL},
};

int
_PyTestLimitedCAPI_Init_Frame(PyObject *m)
{
    return PyModule_AddFunctions(m, test_methods);
}
