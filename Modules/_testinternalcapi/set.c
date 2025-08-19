#include "parts.h"
#include "../_testcapi/util.h"  // NULLABLE, RETURN_INT

#include "pycore_critical_section.h"
#include "pycore_setobject.h"


static PyObject *
set_update(PyObject *self, PyObject *args)
{
    PyObject *set, *iterable;
    if (!PyArg_ParseTuple(args, "OO", &set, &iterable)) {
        return NULL;
    }
    NULLABLE(set);
    NULLABLE(iterable);
    RETURN_INT(_PySet_Update(set, iterable));
}

static PyObject *
set_next_entry(PyObject *self, PyObject *args)
{
    int rc;
    Py_ssize_t pos;
    Py_hash_t hash = (Py_hash_t)UNINITIALIZED_SIZE;
    PyObject *set, *item = UNINITIALIZED_PTR;
    if (!PyArg_ParseTuple(args, "On", &set, &pos)) {
        return NULL;
    }
    NULLABLE(set);
    Py_BEGIN_CRITICAL_SECTION(set);
    rc = _PySet_NextEntryRef(set, &pos, &item, &hash);
    Py_END_CRITICAL_SECTION();
    if (rc == 1) {
        PyObject *ret = Py_BuildValue("innO", rc, pos, hash, item);
        Py_DECREF(item);
        return ret;
    }
    assert(item == UNINITIALIZED_PTR);
    assert(hash == (Py_hash_t)UNINITIALIZED_SIZE);
    if (rc == -1) {
        return NULL;
    }
    assert(rc == 0);
    Py_RETURN_NONE;
}


static PyMethodDef TestMethods[] = {
    {"set_update", set_update, METH_VARARGS},
    {"set_next_entry", set_next_entry, METH_VARARGS},

    {NULL},
};

int
_PyTestInternalCapi_Init_Set(PyObject *m)
{
    if (PyModule_AddFunctions(m, TestMethods) < 0) {
        return -1;
    }
    return 0;
}
