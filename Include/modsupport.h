// Module support interface

#ifndef Py_MODSUPPORT_H
#define Py_MODSUPPORT_H
#ifdef __cplusplus
extern "C" {
#endif

PyAPI_FUNC(int) PyArg_Parse(PyObject *, const char *, ...);
PyAPI_FUNC(int) PyArg_ParseTuple(PyObject *, const char *, ...);
PyAPI_FUNC(int) PyArg_ParseTupleAndKeywords(PyObject *, PyObject *,
                                            const char *, PY_CXX_CONST char * const *, ...);
PyAPI_FUNC(int) PyArg_VaParse(PyObject *, const char *, va_list);
PyAPI_FUNC(int) PyArg_VaParseTupleAndKeywords(PyObject *, PyObject *,
                                              const char *, PY_CXX_CONST char * const *, va_list);

PyAPI_FUNC(int) PyArg_ValidateKeywordArguments(PyObject *);
PyAPI_FUNC(int) PyArg_UnpackTuple(PyObject *, const char *, Py_ssize_t, Py_ssize_t, ...);
PyAPI_FUNC(PyObject *) Py_BuildValue(const char *, ...);
PyAPI_FUNC(PyObject *) Py_VaBuildValue(const char *, va_list);

#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 >= 0x030a0000
// Add an attribute with name 'name' and value 'obj' to the module 'mod.
// On success, return 0.
// On error, raise an exception and return -1.
PyAPI_FUNC(int) PyModule_AddObjectRef(PyObject *mod, const char *name, PyObject *value);
#endif   /* Py_LIMITED_API */

#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 >= 0x030d0000
// Similar to PyModule_AddObjectRef() but steal a reference to 'value'.
PyAPI_FUNC(int) PyModule_Add(PyObject *mod, const char *name, PyObject *value);
#endif   /* Py_LIMITED_API */

// Similar to PyModule_AddObjectRef() and PyModule_Add() but steal
// a reference to 'value' on success and only on success.
// Errorprone. Should not be used in new code.
PyAPI_FUNC(int) PyModule_AddObject(PyObject *mod, const char *, PyObject *value);

PyAPI_FUNC(int) PyModule_AddIntConstant(PyObject *, const char *, long);
PyAPI_FUNC(int) PyModule_AddStringConstant(PyObject *, const char *, const char *);

#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 >= 0x03090000
/* New in 3.9 */
PyAPI_FUNC(int) PyModule_AddType(PyObject *module, PyTypeObject *type);
#endif /* Py_LIMITED_API */

#define PyModule_AddIntMacro(m, c) PyModule_AddIntConstant((m), #c, (c))
#define PyModule_AddStringMacro(m, c) PyModule_AddStringConstant((m), #c, (c))

#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 >= 0x03050000
/* New in 3.5 */
PyAPI_FUNC(int) PyModule_SetDocString(PyObject *, const char *);
PyAPI_FUNC(int) PyModule_AddFunctions(PyObject *, PyMethodDef *);
PyAPI_FUNC(int) PyModule_ExecDef(PyObject *module, PyModuleDef *def);
#endif

#define Py_CLEANUP_SUPPORTED 0x20000

PyAPI_FUNC(PyObject *) PyModule_Create2(PyModuleDef*, int apiver);

#ifdef Py_LIMITED_API
#define PyModule_Create(module) \
        PyModule_Create2((module), PYTHON_ABI_VERSION)
#else
#define PyModule_Create(module) \
        PyModule_Create2((module), PYTHON_API_VERSION)
#endif

#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 >= 0x03050000
/* New in 3.5 */
PyAPI_FUNC(PyObject *) PyModule_FromDefAndSpec2(PyModuleDef *def,
                                                PyObject *spec,
                                                int module_api_version);

#ifdef Py_LIMITED_API
#define PyModule_FromDefAndSpec(module, spec) \
    PyModule_FromDefAndSpec2((module), (spec), PYTHON_ABI_VERSION)
#else
#define PyModule_FromDefAndSpec(module, spec) \
    PyModule_FromDefAndSpec2((module), (spec), PYTHON_API_VERSION)
#endif /* Py_LIMITED_API */

#endif /* New in 3.5 */

/* ABI info & checking (new in 3.15) */
#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 >= _Py_PACK_VERSION(3, 15)
typedef struct PyABIInfo {
    uint8_t abiinfo_major_version;
    uint8_t abiinfo_minor_version;
    uint16_t flags;
    uint32_t build_version;
    uint32_t abi_version;
} PyABIInfo;
#define PyABIInfo_STABLE        0x0001
#define PyABIInfo_GIL           0x0002
#define PyABIInfo_FREETHREADED  0x0004
#define PyABIInfo_INTERNAL      0x0008

#define PyABIInfo_FREETHREADING_AGNOSTIC (PyABIInfo_GIL|PyABIInfo_FREETHREADED)

PyAPI_FUNC(int) PyABIInfo_Check(PyABIInfo *info, const char *module_name);

// Define the defaults
#ifdef Py_LIMITED_API
    #define _PyABIInfo_DEFAULT_FLAG_STABLE PyABIInfo_STABLE
    #if Py_LIMITED_API == 3
        #define PyABIInfo_DEFAULT_ABI_VERSION _Py_PACK_VERSION(3, 2)
    #else
        #define PyABIInfo_DEFAULT_ABI_VERSION Py_LIMITED_API
    #endif
#else
    #define _PyABIInfo_DEFAULT_FLAG_STABLE 0
    #define PyABIInfo_DEFAULT_ABI_VERSION PY_VERSION_HEX
#endif
#if defined(Py_LIMITED_API) && defined(_Py_OPAQUE_PYOBJECT)
    #define _PyABIInfo_DEFAULT_FLAG_FT PyABIInfo_FREETHREADING_AGNOSTIC
#elif defined(Py_GIL_DISABLED)
    #define _PyABIInfo_DEFAULT_FLAG_FT PyABIInfo_FREETHREADED
#else
    #define _PyABIInfo_DEFAULT_FLAG_FT PyABIInfo_GIL
#endif
#if defined(Py_BUILD_CORE)
    #define _PyABIInfo_DEFAULT_FLAG_INTERNAL PyABIInfo_INTERNAL
#else
    #define _PyABIInfo_DEFAULT_FLAG_INTERNAL 0
#endif

#define PyABIInfo_DEFAULT_FLAGS (                           \
        _PyABIInfo_DEFAULT_FLAG_STABLE                      \
        | _PyABIInfo_DEFAULT_FLAG_FT                        \
        | _PyABIInfo_DEFAULT_FLAG_INTERNAL                  \
    )                                                       \
    /////////////////////////////////////////////////////////

#define _PyABIInfo_DEFAULT() {                              \
        1, 0,                                               \
        PyABIInfo_DEFAULT_FLAGS,                            \
        PY_VERSION_HEX,                                     \
        PyABIInfo_DEFAULT_ABI_VERSION }                     \
    /////////////////////////////////////////////////////////

#define PyABIInfo_VAR(NAME) \
    static PyABIInfo NAME = _PyABIInfo_DEFAULT;

#undef _PyABIInfo_DEFAULT_STABLE
#undef _PyABIInfo_DEFAULT_FT
#undef _PyABIInfo_DEFAULT_INTERNAL

#endif /* ABI info (new in 3.15) */

#ifndef Py_LIMITED_API
#  define Py_CPYTHON_MODSUPPORT_H
#  include "cpython/modsupport.h"
#  undef Py_CPYTHON_MODSUPPORT_H
#endif

#ifdef __cplusplus
}
#endif
#endif /* !Py_MODSUPPORT_H */
