
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

struct PyModuleConstants_Def;
#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 >= 0x03100000
/* New in 3.10 */
#define Py_mc_none 1
#define Py_mc_long 2
#define Py_mc_bool 3
#define Py_mc_double 4
#define Py_mc_string 5
#define Py_mc_call 6
#define Py_mc_type 7

typedef struct PyModuleConstants_Def {
    const char *name;
    int type;
    union {
        const char *m_str;
        long m_long;
        double m_double;
        PyObject* (*m_call)(void);
    } value;
} PyModuleConstants_Def;

PyAPI_FUNC(int) PyModule_AddConstants(PyObject *, PyModuleConstants_Def *);

#define PyMC_None(name) {(name), Py_mc_none, {.m_long=0}}
#define PyMC_Long(name, value) {(name), Py_mc_long, {.m_long=(value)}}
#define PyMC_Bool(name, value) {(name), Py_mc_bool, {.m_long=(value)}}
#define PyMC_Double(name, value) {(name), Py_mc_double, {.m_double=(value)}}
#define PyMC_String(name, value) {(name), Py_mc_string, {.m_string=(value)}}
#define PyMC_Call(name, value) {(name), Py_mc_call, {.m_call=(value)}}

#define PyMC_LongMacro(m) PyMC_Long(#m, m)
#define PyMC_StringMacro(m) PyMC_String(#m, m)

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
  struct PyModuleConstants_Def* m_constants;
} PyModuleDef;


// Internal C API
#ifdef Py_BUILD_CORE
extern int _PyModule_IsExtension(PyObject *obj);
#endif

#ifdef __cplusplus
}
#endif
#endif /* !Py_MODULEOBJECT_H */
