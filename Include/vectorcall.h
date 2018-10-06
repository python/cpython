#include "object.h"
#include "pyport.h"
#include "abstract.h"

#ifndef Py_VECTORCALL_H
#define Py_VECTORCALL_H
#ifdef __cplusplus
extern "C" {
#endif

#define PY_VECTORCALL_ARGUMENTS_OFFSET INTPTR_MIN

#ifndef Py_LIMITED_API
static inline PyObject *
_Py_VectorCall(
    PyObject *callable, PyObject **stack,
    Py_ssize_t nargs, PyObject *kwnames
) {
    PyTypeObject *tp = Py_TYPE(callable);
    uintptr_t offset = tp->tp_vectorcall_offset;
    if (offset != 0) {
        assert(tp->tp_flags & Py_TPFLAGS_HAVE_VECTORCALL);
        vectorcall_func func = *(vectorcall_func *)(((char *)callable) + offset);
        if (func) {
            return (func)(callable, stack, nargs, kwnames);
        }
    }
    return PyCall_MakeTpCall(callable, stack, nargs, kwnames);
}
#endif

#ifdef __cplusplus
}
#endif
#endif /* !Py_VECTORCALL_H */
