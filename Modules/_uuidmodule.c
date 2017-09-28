#define PY_SSIZE_T_CLEAN

#include "Python.h"
#include <uuid/uuid.h>


static PyObject *
py_uuid_generate_time_safe(void)
{
    uuid_t out;
    int res;

    res = uuid_generate_time_safe(out);
    return Py_BuildValue("y#i", (const char *) out, sizeof(out), res);
}


static PyMethodDef uuid_methods[] = {
    {"generate_time_safe", (PyCFunction) py_uuid_generate_time_safe, METH_NOARGS, NULL},
    {NULL, NULL, 0, NULL}           /* sentinel */
};

static struct PyModuleDef uuidmodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "_uuid",
    .m_size = -1,
    .m_methods = uuid_methods,
};

PyMODINIT_FUNC
PyInit__uuid(void)
{
    assert(sizeof(uuid_t) == 16);
    return PyModule_Create(&uuidmodule);
}
