// Need limited C API version 3.16 for Py_HashBuffer()
#define Py_LIMITED_API 0x03100000

#include "parts.h"
#include "util.h"


static PyObject*
long_from_hash(Py_hash_t hash)
{
    Py_BUILD_ASSERT(sizeof(long long) >= sizeof(hash));
    return PyLong_FromLongLong(hash);
}


// Test Py_HashBuffer()
static PyObject*
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


static PyMethodDef test_methods[] = {
    {"hash_buffer", hash_buffer, METH_VARARGS},
    {NULL},
};

int
_PyTestLimitedCAPI_Init_Hash(PyObject *m)
{
    return PyModule_AddFunctions(m, test_methods);
}
