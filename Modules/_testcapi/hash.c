#include "parts.h"
#include "util.h"

static PyObject *
hash_getfuncdef(PyObject *Py_UNUSED(module), PyObject *Py_UNUSED(args))
{
    // bind PyHash_GetFuncDef()
    PyHash_FuncDef *def = PyHash_GetFuncDef();

    PyObject *types = PyImport_ImportModule("types");
    if (types == NULL) {
        return NULL;
    }

    PyObject *result = PyObject_CallMethod(types, "SimpleNamespace", NULL);
    Py_DECREF(types);
    if (result == NULL) {
        return NULL;
    }

    // ignore PyHash_FuncDef.hash

    PyObject *value = PyUnicode_FromString(def->name);
    int res = PyObject_SetAttrString(result, "name", value);
    Py_DECREF(value);
    if (res < 0) {
        return NULL;
    }

    value = PyLong_FromLong(def->hash_bits);
    res = PyObject_SetAttrString(result, "hash_bits", value);
    Py_DECREF(value);
    if (res < 0) {
        return NULL;
    }

    value = PyLong_FromLong(def->seed_bits);
    res = PyObject_SetAttrString(result, "seed_bits", value);
    Py_DECREF(value);
    if (res < 0) {
        return NULL;
    }

    return result;
}


static PyObject *
long_from_hash(Py_hash_t hash)
{
    Py_BUILD_ASSERT(sizeof(long long) >= sizeof(hash));
    return PyLong_FromLongLong(hash);
}


static PyObject *
hash_pointer(PyObject *Py_UNUSED(module), PyObject *arg)
{
    void *ptr = PyLong_AsVoidPtr(arg);
    if (ptr == NULL && PyErr_Occurred()) {
        return NULL;
    }

    Py_hash_t hash = Py_HashPointer(ptr);
    return long_from_hash(hash);
}


static PyObject *
hash_buffer(PyObject *Py_UNUSED(module), PyObject *args)
{
    char *ptr;
    Py_ssize_t len;
    if (!PyArg_ParseTuple(args, "y#", &ptr, &len)) {
        return NULL;
    }

    Py_hash_t hash = Py_HashBuffer(ptr, len);
    return long_from_hash(hash);
}


static PyObject *
object_generichash(PyObject *Py_UNUSED(module), PyObject *arg)
{
    NULLABLE(arg);
    Py_hash_t hash = PyObject_GenericHash(arg);
    return long_from_hash(hash);
}


static PyMethodDef test_methods[] = {
    {"hash_getfuncdef", hash_getfuncdef, METH_NOARGS},
    {"hash_pointer", hash_pointer, METH_O},
    {"hash_buffer", hash_buffer, METH_VARARGS},
    {"object_generichash", object_generichash, METH_O},
    {NULL},
};

int
_PyTestCapi_Init_Hash(PyObject *m)
{
    return PyModule_AddFunctions(m, test_methods);
}
