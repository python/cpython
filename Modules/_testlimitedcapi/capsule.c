#include "parts.h"

static void
capsule_destructor(PyObject *op)
{
    /* If non-NULL, the name and the pointer are the same allocation. */
    free((char *)PyCapsule_GetName(op));
}

static PyObject *
capsule_new(PyObject *self, PyObject *arg)
{
    const char *name;
    Py_ssize_t size;
    if (!PyArg_Parse(arg, "z#", &name, &size)) {
        return NULL;
    }
    char *name_copy = NULL;
    if (name != NULL) {
        name_copy = strdup(name);
        if (name_copy == NULL) {
            return PyErr_NoMemory();
        }
    }
    static const char dummy = 0;
    void *pointer = name_copy != NULL ? (void *)name_copy : (void *)&dummy;
    PyObject *capsule = PyCapsule_New(pointer, name_copy, capsule_destructor);
    if (capsule == NULL) {
        free(name_copy);
    }
    return capsule;
}

static PyObject *
pycapsule_import(PyObject *self, PyObject *args)
{
    const char *name;
    Py_ssize_t size;
    int no_block = 0;
    if (!PyArg_ParseTuple(args, "z#|i", &name, &size, &no_block)) {
        return NULL;
    }
    void *pointer = PyCapsule_Import(name, no_block);
    if (pointer == NULL) {
        return NULL;
    }
    /* Capsules created by capsule_new() store a copy of their name as the
       pointer, so a successful import round-trips the name.  Only use this
       function with such capsules. */
    return PyUnicode_FromString((const char *)pointer);
}

static PyMethodDef test_methods[] = {
    {"capsule_new", capsule_new, METH_O},
    {"PyCapsule_Import", pycapsule_import, METH_VARARGS},
    {NULL},
};

int
_PyTestLimitedCAPI_Init_Capsule(PyObject *m)
{
    return PyModule_AddFunctions(m, test_methods);
}
