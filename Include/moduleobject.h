
/* Module object interface */

#ifndef Py_MODULEOBJECT_H
#define Py_MODULEOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

PyAPI_DATA(PyTypeObject) PyModule_Type;

#define PyModule_Check(op) PyObject_TypeCheck(op, &PyModule_Type)
#define PyModule_CheckExact(op) Py_IS_TYPE(op, &PyModule_Type)

#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 >= 0x03030000
PyAPI_FUNC(PyObject *) PyModule_NewObject(
    PyObject *name
    );
#endif
PyAPI_FUNC(PyObject *) PyModule_New(
    const char *name            /* UTF-8 encoded string */
    );
PyAPI_FUNC(PyObject *) PyModule_GetDict(PyObject *);
#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 >= 0x03030000
PyAPI_FUNC(PyObject *) PyModule_GetNameObject(PyObject *);
#endif
PyAPI_FUNC(const char *) PyModule_GetName(PyObject *);
Py_DEPRECATED(3.2) PyAPI_FUNC(const char *) PyModule_GetFilename(PyObject *);
PyAPI_FUNC(PyObject *) PyModule_GetFilenameObject(PyObject *);
#ifndef Py_LIMITED_API
PyAPI_FUNC(void) _PyModule_Clear(PyObject *);
PyAPI_FUNC(void) _PyModule_ClearDict(PyObject *);
PyAPI_FUNC(int) _PyModuleSpec_IsInitializing(PyObject *);
#endif
PyAPI_FUNC(struct PyModuleDef*) PyModule_GetDef(PyObject*);
PyAPI_FUNC(void*) PyModule_GetState(PyObject*);

#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 >= 0x03050000
/* New in 3.5 */
PyAPI_FUNC(PyObject *) PyModuleDef_Init(struct PyModuleDef*);
PyAPI_DATA(PyTypeObject) PyModuleDef_Type;
#endif

typedef struct PyModuleDef_Base {
  PyObject_HEAD
  PyObject* (*m_init)(void);
  Py_ssize_t m_index;
  PyObject* m_copy;
} PyModuleDef_Base;

#define PyModuleDef_HEAD_INIT { \
    PyObject_HEAD_INIT(NULL)    \
    NULL, /* m_init */          \
    0,    /* m_index */         \
    NULL, /* m_copy */          \
  }

struct PyModuleDef_Slot;
#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 >= 0x03050000
/* New in 3.5 */
typedef struct PyModuleDef_Slot{
    int slot;
    void *value;
} PyModuleDef_Slot;

#define Py_mod_create 1
#define Py_mod_exec 2

#ifndef Py_LIMITED_API
#define _Py_mod_LAST_SLOT 2
#endif

#endif /* New in 3.5 */

struct PyModuleConst_Def;
#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 >= 0x03100000
/* New in 3.10 */
enum PyModuleConst_type {
    PyModuleConst_none_type = 1,
    PyModuleConst_long_type = 2,
    PyModuleConst_bool_type = 3,
    PyModuleConst_double_type = 4,
    PyModuleConst_string_type = 5,
    PyModuleConst_call_type = 6,
};

typedef struct PyModuleConst_Def {
    const char *name;
    enum PyModuleConst_type type;
    union {
        const char *m_str;
        long m_long;
        double m_double;
        PyObject* (*m_call)(void);
    } value;
} PyModuleConst_Def;

PyAPI_FUNC(int) PyModule_AddConstants(PyObject *, PyModuleConst_Def *);

#define PyModuleConst_None(name) \
    {(name), PyModuleConst_none_type, {.m_long=0}}
#define PyModuleConst_Long(name, value) \
    {(name), PyModuleConst_long_type, {.m_long=(value)}}
#define PyModuleConst_Bool(name, value) \
    {(name), PyModuleConst_bool_type, {.m_long=(value)}}
#define PyModuleConst_Double(name, value) \
    {(name), PyModuleConst_double_type, {.m_double=(value)}}
#define PyModuleConst_String(name, value) \
    {(name), PyModuleConst_string_type, {.m_string=(value)}}
#define PyModuleConst_Call(name, value) \
    {(name), PyModuleConst_call_type, {.m_call=(value)}}

#define PyModuleConst_LongMacro(m) PyModuleConst_Long(#m, m)
#define PyModuleConst_StringMacro(m) PyModuleConst_String(#m, m)

#endif /* New in 3.10 */

typedef struct PyModuleDef{
  PyModuleDef_Base m_base;
  const char* m_name;
  const char* m_doc;
  Py_ssize_t m_size;
  PyMethodDef *m_methods;
  struct PyModuleDef_Slot* m_slots;
  traverseproc m_traverse;
  inquiry m_clear;
  freefunc m_free;
} PyModuleDef;


// Internal C API
#ifdef Py_BUILD_CORE
extern int _PyModule_IsExtension(PyObject *obj);
#endif

#ifdef __cplusplus
}
#endif
#endif /* !Py_MODULEOBJECT_H */
