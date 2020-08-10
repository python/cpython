/*
 * Written in 2013 by Dmitry Chestnykh <dmitry@codingrobots.com>
 * Modified for CPython by Christian Heimes <christian@python.org>
 *
 * To the extent possible under law, the author have dedicated all
 * copyright and related and neighboring rights to this software to
 * the public domain worldwide. This software is distributed without
 * any warranty. http://creativecommons.org/publicdomain/zero/1.0/
 */

#include "Python.h"

#include "impl/blake2.h"

extern PyType_Spec blake2b_type_spec;
extern PyType_Spec blake2s_type_spec;

PyDoc_STRVAR(blake2mod__doc__,
"_blake2b provides BLAKE2b for hashlib\n"
);

typedef struct {
    PyTypeObject* blake2b_type;
    PyTypeObject* blake2s_type;
} Blake2State;

static inline Blake2State* blake2_get_state(PyObject *module) {
    void *state = PyModule_GetState(module);
    assert(state != NULL);
    return (Blake2State *)state;
}

static struct PyMethodDef blake2mod_functions[] = {
    {NULL, NULL}
};

#define ADD_INT(d, name, value) do { \
    PyObject *x = PyLong_FromLong(value); \
    if (!x) { \
        return -1; \
    } \
    if (PyDict_SetItemString(d, name, x) < 0) { \
        return -1; \
    } \
    Py_DECREF(x); \
} while(0)


static int blake2_exec(PyObject *m)
{
    Blake2State* st = blake2_get_state(m);

    st->blake2b_type = (PyTypeObject *)PyType_FromModuleAndSpec(
        m, &blake2b_type_spec, NULL);

    /* BLAKE2b */
    if (PyModule_AddType(m, st->blake2b_type) < 0) {
        return -1;
    }

    PyObject *d = st->blake2b_type->tp_dict;
    ADD_INT(d, "SALT_SIZE", BLAKE2B_SALTBYTES);
    ADD_INT(d, "PERSON_SIZE", BLAKE2B_PERSONALBYTES);
    ADD_INT(d, "MAX_KEY_SIZE", BLAKE2B_KEYBYTES);
    ADD_INT(d, "MAX_DIGEST_SIZE", BLAKE2B_OUTBYTES);

    PyModule_AddIntConstant(m, "BLAKE2B_SALT_SIZE", BLAKE2B_SALTBYTES);
    PyModule_AddIntConstant(m, "BLAKE2B_PERSON_SIZE", BLAKE2B_PERSONALBYTES);
    PyModule_AddIntConstant(m, "BLAKE2B_MAX_KEY_SIZE", BLAKE2B_KEYBYTES);
    PyModule_AddIntConstant(m, "BLAKE2B_MAX_DIGEST_SIZE", BLAKE2B_OUTBYTES);

    /* BLAKE2s */
    st->blake2s_type = (PyTypeObject *)PyType_FromModuleAndSpec(
        m, &blake2s_type_spec, NULL);

    if (PyModule_AddType(m, st->blake2s_type) < 0) {
        return -1;
    }

    d = st->blake2s_type->tp_dict;
    ADD_INT(d, "SALT_SIZE", BLAKE2S_SALTBYTES);
    ADD_INT(d, "PERSON_SIZE", BLAKE2S_PERSONALBYTES);
    ADD_INT(d, "MAX_KEY_SIZE", BLAKE2S_KEYBYTES);
    ADD_INT(d, "MAX_DIGEST_SIZE", BLAKE2S_OUTBYTES);

    PyModule_AddIntConstant(m, "BLAKE2S_SALT_SIZE", BLAKE2S_SALTBYTES);
    PyModule_AddIntConstant(m, "BLAKE2S_PERSON_SIZE", BLAKE2S_PERSONALBYTES);
    PyModule_AddIntConstant(m, "BLAKE2S_MAX_KEY_SIZE", BLAKE2S_KEYBYTES);
    PyModule_AddIntConstant(m, "BLAKE2S_MAX_DIGEST_SIZE", BLAKE2S_OUTBYTES);

    return 0;
}

static PyModuleDef_Slot _blake2_slots[] = {
    {Py_mod_exec, blake2_exec},
    {0, NULL}
};

static struct PyModuleDef blake2_module = {
    PyModuleDef_HEAD_INIT,
    "_blake2",
    .m_doc = blake2mod__doc__,
    .m_size = sizeof(Blake2State),
    .m_methods = blake2mod_functions,
    .m_slots = _blake2_slots
};

PyMODINIT_FUNC
PyInit__blake2(void)
{
    return PyModuleDef_Init(&blake2_module);
}