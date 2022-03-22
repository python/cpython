/*
 * Written in 2013 by Dmitry Chestnykh <dmitry@codingrobots.com>
 * Modified for CPython by Christian Heimes <christian@python.org>
 *
 * Modified for BLAKE3 by Larry Hastings <larry@hastings.org>
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

#include "impl/blake3.h"

extern PyType_Spec blake3_type_spec;

PyDoc_STRVAR(blake3mod__doc__,
"_blake3 provides BLAKE3 for hashlib\n"
);

typedef struct {
    PyTypeObject* blake3_type;
} Blake3State;

static inline Blake3State*
blake3_get_state(PyObject *module)
{
    void *state = PyModule_GetState(module);
    assert(state != NULL);
    return (Blake3State *)state;
}

static struct PyMethodDef blake3mod_functions[] = {
    {NULL, NULL}
};

static int
_blake3_traverse(PyObject *module, visitproc visit, void *arg)
{
    Blake3State *state = blake3_get_state(module);
    Py_VISIT(state->blake3_type);
    return 0;
}

static int
_blake3_clear(PyObject *module)
{
    Blake3State *state = blake3_get_state(module);
    Py_CLEAR(state->blake3_type);
    return 0;
}

static void
_blake3_free(void *module)
{
    _blake3_clear((PyObject *)module);
}

static int
blake3_exec(PyObject *m)
{
    Blake3State *state = blake3_get_state(m);

    state->blake3_type = (PyTypeObject *)PyType_FromModuleAndSpec(
        m, &blake3_type_spec, NULL);

    if (NULL == state->blake3_type)
        return -1;
    if (PyModule_AddType(m, state->blake3_type) < 0) {
        return -1;
    }

    PyObject *module_dict = state->blake3_type->tp_dict;

#define ADD_CONSTANT(name, o) do { \
    if (PyDict_SetItemString(module_dict, name, o) < 0) { \
        Py_DECREF(o); \
        return -1; \
    } \
} while(0)

#define ADD_INT(name, constant) do { \
    PyObject *o = PyLong_FromLong(constant); \
    if (!o) \
        return -1; \
    ADD_CONSTANT(name, o); \
    Py_DECREF(o); \
} while(0)

#define ADD_STRING(name, s) do { \
    PyObject *o = PyUnicode_FromString(s); \
    if (!o) \
        return -1; \
    ADD_CONSTANT(name, o); \
    Py_DECREF(o); \
} while(0)

    ADD_STRING("version", BLAKE3_VERSION_STRING);

    ADD_INT("key_length", BLAKE3_KEY_LEN);
    ADD_INT("output_length", BLAKE3_OUT_LEN);
    ADD_INT("block_length", BLAKE3_BLOCK_LEN);
    ADD_INT("chunk_length", BLAKE3_CHUNK_LEN);
    ADD_INT("max_depth", BLAKE3_MAX_DEPTH);
    ADD_INT("AUTO", -1);

    ADD_CONSTANT("supports_multithreading", Py_False);

    return 0;
}

static PyModuleDef_Slot _blake3_slots[] = {
    {Py_mod_exec, blake3_exec},
    {0, NULL}
};

static struct PyModuleDef blake3_module = {
    PyModuleDef_HEAD_INIT,
    "_blake3",
    .m_doc = blake3mod__doc__,
    .m_size = sizeof(Blake3State),
    .m_methods = blake3mod_functions,
    .m_slots = _blake3_slots,
    .m_traverse = _blake3_traverse,
    .m_clear = _blake3_clear,
    .m_free = _blake3_free,
};

PyMODINIT_FUNC
PyInit__blake3(void)
{
    return PyModuleDef_Init(&blake3_module);
}
