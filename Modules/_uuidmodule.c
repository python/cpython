#define PY_SSIZE_T_CLEAN

#include "Python.h"
#ifdef HAVE_UUID_UUID_H
#include <uuid/uuid.h>
#endif
#ifdef HAVE_UUID_H
#include <uuid.h>
#endif


static PyObject *
py_uuid_generate_time_safe(PyObject *Py_UNUSED(context),
                           PyObject *Py_UNUSED(ignored))
{
    uuid_t uuid;
#ifdef HAVE_UUID_GENERATE_TIME_SAFE
    int res;

    res = uuid_generate_time_safe(uuid);
    return Py_BuildValue("y#i", (const char *) uuid, sizeof(uuid), res);
#elif defined(HAVE_UUID_CREATE)
    uint32_t status;
    uuid_create(&uuid, &status);
# if defined(HAVE_UUID_ENC_BE)
    unsigned char buf[sizeof(uuid)];
    uuid_enc_be(buf, &uuid);
    return Py_BuildValue("y#i", buf, sizeof(uuid), (int) status);
# else
    return Py_BuildValue("y#i", (const char *) &uuid, sizeof(uuid), (int) status);
# endif
#else
    uuid_generate_time(uuid);
    return Py_BuildValue("y#O", (const char *) uuid, sizeof(uuid), Py_None);
#endif
}


static PyMethodDef uuid_methods[] = {
    {"generate_time_safe", py_uuid_generate_time_safe, METH_NOARGS, NULL},
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
        Py_DECREF(mod);
        return NULL;
    }

    return mod;
}
