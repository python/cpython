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

extern PyTypeObject PyBlake2_BLAKE2bType;
extern PyTypeObject PyBlake2_BLAKE2sType;


PyDoc_STRVAR(blake2mod__doc__,
"_blake2b provides BLAKE2b for hashlib\n"
);


static struct PyMethodDef blake2mod_functions[] = {
    {NULL, NULL}
};

static struct PyModuleDef blake2_module = {
    PyModuleDef_HEAD_INIT,
    "_blake2",
    blake2mod__doc__,
    -1,
    blake2mod_functions,
    NULL,
    NULL,
    NULL,
    NULL
};

#define ADD_INT(d, name, value) do { \
    PyObject *x = PyLong_FromLong(value); \
    if (!x) { \
        Py_DECREF(m); \
        return NULL; \
    } \
    if (PyDict_SetItemString(d, name, x) < 0) { \
        Py_DECREF(m); \
        return NULL; \
    } \
    Py_DECREF(x); \
} while(0)


PyMODINIT_FUNC
PyInit__blake2(void)
{
    PyObject *m;
    PyObject *d;

    m = PyModule_Create(&blake2_module);
    if (m == NULL)
        return NULL;

    /* BLAKE2b */
    Py_SET_TYPE(&PyBlake2_BLAKE2bType, &PyType_Type);
    if (PyModule_AddType(m, &PyBlake2_BLAKE2bType) < 0) {
        return NULL;
    }

    d = PyBlake2_BLAKE2bType.tp_dict;
    ADD_INT(d, "SALT_SIZE", BLAKE2B_SALTBYTES);
    ADD_INT(d, "PERSON_SIZE", BLAKE2B_PERSONALBYTES);
    ADD_INT(d, "MAX_KEY_SIZE", BLAKE2B_KEYBYTES);
    ADD_INT(d, "MAX_DIGEST_SIZE", BLAKE2B_OUTBYTES);

    PyModule_AddIntConstant(m, "BLAKE2B_SALT_SIZE", BLAKE2B_SALTBYTES);
    PyModule_AddIntConstant(m, "BLAKE2B_PERSON_SIZE", BLAKE2B_PERSONALBYTES);
    PyModule_AddIntConstant(m, "BLAKE2B_MAX_KEY_SIZE", BLAKE2B_KEYBYTES);
    PyModule_AddIntConstant(m, "BLAKE2B_MAX_DIGEST_SIZE", BLAKE2B_OUTBYTES);

    /* BLAKE2s */
    Py_SET_TYPE(&PyBlake2_BLAKE2sType, &PyType_Type);
    if (PyModule_AddType(m, &PyBlake2_BLAKE2sType) < 0) {
        return NULL;
    }

    d = PyBlake2_BLAKE2sType.tp_dict;
    ADD_INT(d, "SALT_SIZE", BLAKE2S_SALTBYTES);
    ADD_INT(d, "PERSON_SIZE", BLAKE2S_PERSONALBYTES);
    ADD_INT(d, "MAX_KEY_SIZE", BLAKE2S_KEYBYTES);
    ADD_INT(d, "MAX_DIGEST_SIZE", BLAKE2S_OUTBYTES);

    PyModule_AddIntConstant(m, "BLAKE2S_SALT_SIZE", BLAKE2S_SALTBYTES);
    PyModule_AddIntConstant(m, "BLAKE2S_PERSON_SIZE", BLAKE2S_PERSONALBYTES);
    PyModule_AddIntConstant(m, "BLAKE2S_MAX_KEY_SIZE", BLAKE2S_KEYBYTES);
    PyModule_AddIntConstant(m, "BLAKE2S_MAX_DIGEST_SIZE", BLAKE2S_OUTBYTES);

    return m;
}
