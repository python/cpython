/*
 * Written in 2013 by Dmitry Chestnykh <dmitry@codingrobots.com>
 * Modified for CPython by Christian Heimes <christian@python.org>
 *
 * To the extent possible under law, the author have dedicated all
 * copyright and related and neighboring rights to this software to
 * the public domain worldwide. This software is distributed without
 * any warranty. http://creativecommons.org/publicdomain/zero/1.0/
 */

#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "Python.h"
#include "blake2module.h"

extern PyType_Spec blake2b_type_spec;
extern PyType_Spec blake2s_type_spec;

PyDoc_STRVAR(blake2mod__doc__,
"_blake2b provides BLAKE2b for hashlib\n"
);

typedef struct {
    PyTypeObject* blake2b_type;
    PyTypeObject* blake2s_type;
} Blake2State;

static inline Blake2State*
blake2_get_state(PyObject *module)
{
    void *state = PyModule_GetState(module);
    assert(state != NULL);
    return (Blake2State *)state;
}

static struct PyMethodDef blake2mod_functions[] = {
    {NULL, NULL}
};

static int
_blake2_traverse(PyObject *module, visitproc visit, void *arg)
{
    Blake2State *state = blake2_get_state(module);
    Py_VISIT(state->blake2b_type);
    Py_VISIT(state->blake2s_type);
    return 0;
}

static int
_blake2_clear(PyObject *module)
{
    Blake2State *state = blake2_get_state(module);
    Py_CLEAR(state->blake2b_type);
    Py_CLEAR(state->blake2s_type);
    return 0;
}

static void
_blake2_free(void *module)
{
    _blake2_clear((PyObject *)module);
}

#define ADD_INT(d, name, value) do { \
    PyObject *x = PyLong_FromLong(value); \
    if (!x) \
        return -1; \
    if (PyDict_SetItemString(d, name, x) < 0) { \
        Py_DECREF(x); \
        return -1; \
    } \
    Py_DECREF(x); \
} while(0)

#define ADD_INT_CONST(NAME, VALUE) do { \
    if (PyModule_AddIntConstant(m, NAME, VALUE) < 0) { \
        return -1; \
    } \
} while (0)

static int
blake2_exec(PyObject *m)
{
    Blake2State* st = blake2_get_state(m);

    st->blake2b_type = (PyTypeObject *)PyType_FromModuleAndSpec(
        m, &blake2b_type_spec, NULL);

    if (NULL == st->blake2b_type)
        return -1;
    /* BLAKE2b */
    if (PyModule_AddType(m, st->blake2b_type) < 0) {
        return -1;
    }

    PyObject *d = st->blake2b_type->tp_dict;
    ADD_INT(d, "SALT_SIZE", BLAKE2B_SALTBYTES);
    ADD_INT(d, "PERSON_SIZE", BLAKE2B_PERSONALBYTES);
    ADD_INT(d, "MAX_KEY_SIZE", BLAKE2B_KEYBYTES);
    ADD_INT(d, "MAX_DIGEST_SIZE", BLAKE2B_OUTBYTES);

    ADD_INT_CONST("BLAKE2B_SALT_SIZE", BLAKE2B_SALTBYTES);
    ADD_INT_CONST("BLAKE2B_PERSON_SIZE", BLAKE2B_PERSONALBYTES);
    ADD_INT_CONST("BLAKE2B_MAX_KEY_SIZE", BLAKE2B_KEYBYTES);
    ADD_INT_CONST("BLAKE2B_MAX_DIGEST_SIZE", BLAKE2B_OUTBYTES);

    /* BLAKE2s */
    st->blake2s_type = (PyTypeObject *)PyType_FromModuleAndSpec(
        m, &blake2s_type_spec, NULL);

    if (NULL == st->blake2s_type)
        return -1;

    if (PyModule_AddType(m, st->blake2s_type) < 0) {
        return -1;
    }

    d = st->blake2s_type->tp_dict;
    ADD_INT(d, "SALT_SIZE", BLAKE2S_SALTBYTES);
    ADD_INT(d, "PERSON_SIZE", BLAKE2S_PERSONALBYTES);
    ADD_INT(d, "MAX_KEY_SIZE", BLAKE2S_KEYBYTES);
    ADD_INT(d, "MAX_DIGEST_SIZE", BLAKE2S_OUTBYTES);

    ADD_INT_CONST("BLAKE2S_SALT_SIZE", BLAKE2S_SALTBYTES);
    ADD_INT_CONST("BLAKE2S_PERSON_SIZE", BLAKE2S_PERSONALBYTES);
    ADD_INT_CONST("BLAKE2S_MAX_KEY_SIZE", BLAKE2S_KEYBYTES);
    ADD_INT_CONST("BLAKE2S_MAX_DIGEST_SIZE", BLAKE2S_OUTBYTES);

    return 0;
}

#undef ADD_INT
#undef ADD_INT_CONST

static PyModuleDef_Slot _blake2_slots[] = {
    {Py_mod_exec, blake2_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {0, NULL}
};

static struct PyModuleDef blake2_module = {
    PyModuleDef_HEAD_INIT,
    "_blake2",
    .m_doc = blake2mod__doc__,
    .m_size = sizeof(Blake2State),
    .m_methods = blake2mod_functions,
    .m_slots = _blake2_slots,
    .m_traverse = _blake2_traverse,
    .m_clear = _blake2_clear,
    .m_free = _blake2_free,
};

PyMODINIT_FUNC
PyInit__blake2(void)
{
    return PyModuleDef_Init(&blake2_module);
}
