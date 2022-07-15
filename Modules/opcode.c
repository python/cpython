#include "Python.h"

#define NEED_OPARG_KIND_TABLE
#define NEED_STRING_FOR_ENUM
#include "opcode.h"

static PyObject *
add_to_module(PyObject *module, const char *name, PyObject *value) {
    if (!value) {
        return NULL;
    }
    int res = PyModule_AddObjectRef(module, name, value);
    Py_DECREF(value);
    return res == 0 ? value : NULL;
}

#define CHECK(X) if (!(X)) goto error
#define ARR_SIZE(X) (sizeof(X) / sizeof(*(X)))

PyMODINIT_FUNC
PyInit_opcode(void) {
    static PyModuleDef mod_def = { PyModuleDef_HEAD_INIT, "opcode",  "Opcode support module." };
    PyObject *module = PyModule_Create(&mod_def);
    CHECK(module);
    PyObject *opname = add_to_module(module, "opname", PyTuple_New(ARR_SIZE(opcode_names)));
    CHECK(opname);

    for (size_t i = 0; i < ARR_SIZE(opcode_names); i++) {
        PyObject *name;
        if (opcode_names[i]) {
            CHECK(name = PyUnicode_FromString(opcode_names[i]));
        } else {
            Py_INCREF(name = Py_None);
        }
        PyTuple_SET_ITEM(opname, i, name);
    }

    PyObject *oparg_kinds[ARR_SIZE(oparg_kind_names)];
    for (size_t i = 0; i < ARR_SIZE(oparg_kind_names); ++i) {
        CHECK(oparg_kinds[i] = add_to_module(module, oparg_kind_names[i], PyLong_FromSsize_t(i)));
    }

    PyObject *table = add_to_module(module, "oparg_kind_table", PyTuple_New(ARR_SIZE(opcode_names)));
    CHECK(table);
    for (size_t i = 0; i < ARR_SIZE(opcode_names); i++) {
        PyObject *row = PyTuple_New(3);
        CHECK(row);
        for (int j = 0; j < 3; j++) {
            PyObject *kind = oparg_kinds[oparg_kind_table[i][j]];
            Py_INCREF(kind);
            PyTuple_SET_ITEM(row, j, kind);
        }
        PyTuple_SET_ITEM(table, i, row);
    }

    return module;
error:
    Py_XDECREF(module);
    return NULL;
}
