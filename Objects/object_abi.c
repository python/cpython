// Stable ABI implementation for PyObject functions.


// Rename static inline functions with macros
#define _Py_STABLE_ABI_IMPL

#include "Python.h"


#undef Py_NewRef
#undef Py_XNewRef

// Export Py_NewRef() and Py_XNewRef() as regular functions for the stable ABI.
PyObject*
Py_NewRef(PyObject *obj)
{
    return _Py_NewRef(obj);
}

PyObject*
Py_XNewRef(PyObject *obj)
{
    return _Py_XNewRef(obj);
}


#undef Py_Is
#undef Py_IsNone
#undef Py_IsTrue
#undef Py_IsFalse

// Export Py_Is(), Py_IsNone(), Py_IsTrue(), Py_IsFalse() as regular functions
// for the stable ABI.
int Py_Is(PyObject *x, PyObject *y)
{
    return (x == y);
}

int Py_IsNone(PyObject *x)
{
    return Py_Is(x, Py_None);
}

int Py_IsTrue(PyObject *x)
{
    return Py_Is(x, Py_True);
}

int Py_IsFalse(PyObject *x)
{
    return Py_Is(x, Py_False);
}


#undef Py_SET_REFCNT

void
Py_SET_REFCNT(PyObject *ob, Py_ssize_t refcnt)
{
    _Py_SET_REFCNT(ob, refcnt);
}
