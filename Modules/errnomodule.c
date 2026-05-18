/* Errno module */

// Need limited C API version 3.13 for Py_mod_gil
#include "pyconfig.h"   // Py_GIL_DISABLED
#ifndef Py_GIL_DISABLED
#  define Py_LIMITED_API 0x030d0000
#endif

#include "Python.h"
#include <errno.h>                // EPIPE

/* Windows socket errors (WSA*)  */
#ifdef MS_WINDOWS
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h>

   // The following constants were added to errno.h in VS2010 but have
   // preferred WSA equivalents.
#  undef EADDRINUSE
#  undef EADDRNOTAVAIL
#  undef EAFNOSUPPORT
#  undef EALREADY
#  undef ECONNABORTED
#  undef ECONNREFUSED
#  undef ECONNRESET
#  undef EDESTADDRREQ
#  undef EHOSTUNREACH
#  undef EINPROGRESS
#  undef EISCONN
#  undef ELOOP
#  undef EMSGSIZE
#  undef ENETDOWN
#  undef ENETRESET
#  undef ENETUNREACH
#  undef ENOBUFS
#  undef ENOPROTOOPT
#  undef ENOTCONN
#  undef ENOTSOCK
#  undef EOPNOTSUPP
#  undef EPROTONOSUPPORT
#  undef EPROTOTYPE
#  undef ETIMEDOUT
#  undef EWOULDBLOCK
#endif

/*
 * Pull in the system error definitions
 */

static PyMethodDef errno_methods[] = {
    {NULL,              NULL}
};

/* Helper function doing the dictionary inserting */

static int
_add_errcode(PyObject *module_dict, PyObject *error_dict, const char *name_str, int code_int)
{
    PyObject *name = PyUnicode_FromString(name_str);
    if (!name) {
        return -1;
    }

    PyObject *code = PyLong_FromLong(code_int);
    if (!code) {
        Py_DECREF(name);
        return -1;
    }

    int ret = -1;
    /* insert in modules dict */
    if (PyDict_SetItem(module_dict, name, code) < 0) {
        goto end;
    }
    /* insert in errorcode dict */
    if (PyDict_SetItem(error_dict, code, name) < 0) {
        goto end;
    }
    ret = 0;
end:
    Py_DECREF(name);
    Py_DECREF(code);
    return ret;
}

static int
errno_exec(PyObject *module)
{
    PyObject *module_dict = PyModule_GetDict(module);  // Borrowed ref.
    if (module_dict == NULL) {
        return -1;
    }
    PyObject *error_dict = PyDict_New();
    if (error_dict == NULL) {
        return -1;
    }
    if (PyDict_SetItemString(module_dict, "errorcode", error_dict) < 0) {
        Py_DECREF(error_dict);
        return -1;
    }

#define add_errcode(name, code, comment)                               \
    do {                                                               \
        if (_add_errcode(module_dict, error_dict, name, code) < 0) {   \
            Py_DECREF(error_dict);                                     \
            return -1;                                                 \
        }                                                              \
    } while (0);
#include "../Objects/errnonames.h"
#undef add_errcode

    Py_DECREF(error_dict);
    return 0;
}

static PyModuleDef_Slot errno_slots[] = {
    {Py_mod_exec, errno_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL}
};

PyDoc_STRVAR(errno__doc__,
"This module makes available standard errno system symbols.\n\
\n\
The value of each symbol is the corresponding integer value,\n\
e.g., on most systems, errno.ENOENT equals the integer 2.\n\
\n\
The dictionary errno.errorcode maps numeric codes to symbol names,\n\
e.g., errno.errorcode[2] could be the string 'ENOENT'.\n\
\n\
Symbols that are not relevant to the underlying system are not defined.\n\
\n\
To map error codes to error messages, use the function os.strerror(),\n\
e.g. os.strerror(2) could return 'No such file or directory'.");

static struct PyModuleDef errnomodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "errno",
    .m_doc = errno__doc__,
    .m_size = 0,
    .m_methods = errno_methods,
    .m_slots = errno_slots,
};

PyMODINIT_FUNC
PyInit_errno(void)
{
    return PyModuleDef_Init(&errnomodule);
}
