#include "parts.h"
#include "util.h"

static PyObject *
threadstate_set_async_exc(PyObject *module, PyObject *args)
{
    unsigned long id;
    PyObject *exc;
    if (!PyArg_ParseTuple(args, "kO", &id, &exc)) {
        return NULL;
    }
    int result = PyThreadState_SetAsyncExc(id, exc);
    return PyLong_FromLong(result);
}

static PyMethodDef test_methods[] = {
    {"threadstate_set_async_exc", threadstate_set_async_exc, METH_VARARGS, NULL},
    {NULL},
};

int
_PyTestLimitedCAPI_Init_ThreadState(PyObject *m)
{
    return PyModule_AddFunctions(m, test_methods);
}
