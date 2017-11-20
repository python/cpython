#define PY_SSIZE_T_CLEAN

#include "Python.h"
#include <uuid/uuid.h>


static PyObject *
py_uuid_generate_time_safe(void)
{
#ifdef HAVE_UUID_GENERATE_TIME_SAFE
    uuid_t out;
    int res;

    res = uuid_generate_time_safe(out);
    return Py_BuildValue("y#i", (const char *) out, sizeof(out), res);
#else
    uuid_t out;
    uuid_generate_time(out);
    return Py_BuildValue("y#O", (const char *) out, sizeof(out), Py_None);
#endif
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
    PyObject *mod;
    assert(sizeof(uuid_t) == 16);
#ifdef HAVE_UUID_GENERATE_TIME_SAFE
    int has_uuid_generate_time_safe = 1;
#else
    int has_uuid_generate_time_safe = 0;
#endif
    mod = PyModule_Create(&uuidmodule);
    if (mod == NULL) {
        return NULL;
    }
    if (PyModule_AddIntConstant(mod, "has_uuid_generate_time_safe",
                                has_uuid_generate_time_safe) < 0) {
        return NULL;
    }

    return mod;
}
