#include "parts.h"
#include "util.h"

/* Issue #4701: Check that PyObject_Hash implicitly calls
 *   PyType_Ready if it hasn't already been called
 */
static PyTypeObject _HashInheritanceTester_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "hashinheritancetester",
    .tp_basicsize = sizeof(PyObject),
    .tp_dealloc = (destructor)PyObject_Free,
    .tp_getattro = PyObject_GenericGetAttr,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = PyType_GenericNew,
};


static PyObject*
test_lazy_hash_inheritance(PyObject* self, PyObject *Py_UNUSED(ignored))
{
    PyTypeObject *type = &_HashInheritanceTester_Type;
    if (type->tp_dict != NULL)
        /* The type has already been initialized. This probably means
           -R is being used. */
        Py_RETURN_NONE;


    PyObject *obj = PyObject_New(PyObject, type);
    if (obj == NULL) {
        PyErr_Clear();
        PyErr_SetString(
            PyExc_AssertionError,
            "test_lazy_hash_inheritance: failed to create object");
        return NULL;
    }

    if (type->tp_dict != NULL) {
        PyErr_SetString(
            PyExc_AssertionError,
            "test_lazy_hash_inheritance: type initialised too soon");
        Py_DECREF(obj);
        return NULL;
    }

    Py_hash_t hash = PyObject_Hash(obj);
    if ((hash == -1) && PyErr_Occurred()) {
        PyErr_Clear();
        PyErr_SetString(
            PyExc_AssertionError,
            "test_lazy_hash_inheritance: could not hash object");
        Py_DECREF(obj);
        return NULL;
    }

    if (type->tp_dict == NULL) {
        PyErr_SetString(
            PyExc_AssertionError,
            "test_lazy_hash_inheritance: type not initialised by hash()");
        Py_DECREF(obj);
        return NULL;
    }

    if (type->tp_hash != PyType_Type.tp_hash) {
        PyErr_SetString(
            PyExc_AssertionError,
            "test_lazy_hash_inheritance: unexpected hash function");
        Py_DECREF(obj);
        return NULL;
    }

    Py_DECREF(obj);

    Py_RETURN_NONE;
}


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
    {"test_lazy_hash_inheritance", test_lazy_hash_inheritance, METH_NOARGS},
    {"hash_getfuncdef", hash_getfuncdef, METH_NOARGS},
    {"hash_pointer", hash_pointer, METH_O},
    {"hash_buffer", hash_buffer, METH_VARARGS},
    {"object_generichash", object_generichash, METH_O},
    {NULL},
};

int
_PyTestCapi_Init_Hash(PyObject *m)
{
    Py_SET_TYPE(&_HashInheritanceTester_Type, &PyType_Type);
    if (PyType_Ready(&_HashInheritanceTester_Type) < 0) {
        return -1;
    }

    return PyModule_AddFunctions(m, test_methods);
}
