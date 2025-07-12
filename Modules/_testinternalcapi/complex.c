#include "parts.h"
#include "../_testcapi/util.h"

#define Py_BUILD_CORE
#include "pycore_complexobject.h"


#define _PY_CR_FUNC2(suffix)                                     \
    static PyObject *                                            \
    _py_cr_##suffix(PyObject *Py_UNUSED(module), PyObject *args) \
    {                                                            \
        Py_complex a, res;                                       \
        double b;                                                \
                                                                 \
        if (!PyArg_ParseTuple(args, "Dd", &a, &b)) {             \
            return NULL;                                         \
        }                                                        \
                                                                 \
        errno = 0;                                               \
        res = _Py_cr_##suffix(a, b);                             \
        return Py_BuildValue("Di", &res, errno);                 \
    };

#define _PY_RC_FUNC2(suffix)                                     \
    static PyObject *                                            \
    _py_rc_##suffix(PyObject *Py_UNUSED(module), PyObject *args) \
    {                                                            \
        Py_complex b, res;                                       \
        double a;                                                \
                                                                 \
        if (!PyArg_ParseTuple(args, "dD", &a, &b)) {             \
            return NULL;                                         \
        }                                                        \
                                                                 \
        errno = 0;                                               \
        res = _Py_rc_##suffix(a, b);                             \
        return Py_BuildValue("Di", &res, errno);                 \
    };

_PY_CR_FUNC2(sum)
_PY_CR_FUNC2(diff)
_PY_RC_FUNC2(diff)
_PY_CR_FUNC2(prod)
_PY_CR_FUNC2(quot)
_PY_RC_FUNC2(quot)


static PyMethodDef test_methods[] = {
    {"_py_cr_sum", _py_cr_sum, METH_VARARGS},
    {"_py_cr_diff", _py_cr_diff, METH_VARARGS},
    {"_py_rc_diff", _py_rc_diff, METH_VARARGS},
    {"_py_cr_prod", _py_cr_prod, METH_VARARGS},
    {"_py_cr_quot", _py_cr_quot, METH_VARARGS},
    {"_py_rc_quot", _py_rc_quot, METH_VARARGS},
    {NULL},
};

int
_PyTestInternalCapi_Init_Complex(PyObject *mod)
{
    if (PyModule_AddFunctions(mod, test_methods) < 0) {
        return -1;
    }

    return 0;
}
