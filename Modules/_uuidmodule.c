#define PY_SSIZE_T_CLEAN

#include "Python.h"
#include <uuid/uuid.h>


static PyObject *
_uuid_generate_time_safe(void)
{
    uuid_t out;
    int res;

    res = uuid_generate_time_safe(out);
    return Py_BuildValue("y#i", (const char *) out, sizeof(out), res);
}


static PyMethodDef uuid_methods[] = {
    {"generate_time_safe", (PyCFunction)_uuid_generate_time_safe, METH_NOARGS, NULL},
    {NULL, NULL, 0, NULL}           /* sentinel */
};

static struct PyModuleDef uuidmodule = {
    PyModuleDef_HEAD_INIT,
    "_uuid",
    NULL,
    -1,
    uuid_methods,
    NULL,
    NULL,
    NULL,
    NULL
};

PyMODINIT_FUNC
PyInit__uuid(void)
{
    assert(sizeof(uuid_t) == 16);
    return PyModule_Create(&uuidmodule);
}
