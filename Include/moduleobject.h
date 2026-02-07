
/* Module object interface */

#ifndef Py_MODULEOBJECT_H
#define Py_MODULEOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

PyAPI_DATA(PyTypeObject) PyModule_Type;

#define PyModule_Check(op) PyObject_TypeCheck((op), &PyModule_Type)
#define PyModule_CheckExact(op) Py_IS_TYPE((op), &PyModule_Type)

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
PyAPI_FUNC(PyModuleDef*) PyModule_GetDef(PyObject*);
PyAPI_FUNC(void*) PyModule_GetState(PyObject*);

#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 >= 0x03050000
/* New in 3.5 */
PyAPI_FUNC(PyObject *) PyModuleDef_Init(PyModuleDef*);
PyAPI_DATA(PyTypeObject) PyModuleDef_Type;
#endif

#ifndef _Py_OPAQUE_PYOBJECT
typedef struct PyModuleDef_Base {
  PyObject_HEAD
  /* The function used to re-initialize the module.
     This is only set for legacy (single-phase init) extension modules
     and only used for those that support multiple initializations
     (m_size >= 0).
     It is set by _PyImport_LoadDynamicModuleWithSpec()
     and _imp.create_builtin(). */
  PyObject* (*m_init)(void);
  /* The module's index into its interpreter's modules_by_index cache.
     This is set for all extension modules but only used for legacy ones.
     (See PyInterpreterState.modules_by_index for more info.)
     It is set by PyModuleDef_Init(). */
  Py_ssize_t m_index;
  /* A copy of the module's __dict__ after the first time it was loaded.
     This is only set/used for legacy modules that do not support
     multiple initializations.
     It is set by fix_up_extension() in import.c. */
  PyObject* m_copy;
} PyModuleDef_Base;

#define PyModuleDef_HEAD_INIT {  \
    PyObject_HEAD_INIT(_Py_NULL) \
    _Py_NULL, /* m_init */       \
    0,        /* m_index */      \
    _Py_NULL, /* m_copy */       \
  }
#endif  // _Py_OPAQUE_PYOBJECT

#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 >= 0x03050000
/* New in 3.5 */
struct PyModuleDef_Slot {
    int slot;
    void *value;
};

#define Py_mod_create 1
#define Py_mod_exec 2
#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 >= 0x030c0000
#  define Py_mod_multiple_interpreters 3
#endif
#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 >= 0x030d0000
#  define Py_mod_gil 4
#endif
#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 >= _Py_PACK_VERSION(3, 15)
#  define Py_mod_abi 5
#  define Py_mod_name 6
#  define Py_mod_doc 7
#  define Py_mod_state_size 8
#  define Py_mod_methods 9
#  define Py_mod_state_traverse 10
#  define Py_mod_state_clear 11
#  define Py_mod_state_free 12
#  define Py_mod_token 13
#endif


#ifndef Py_LIMITED_API
#define _Py_mod_LAST_SLOT 13
#endif

#endif /* New in 3.5 */

/* for Py_mod_multiple_interpreters: */
#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 >= 0x030c0000
#  define Py_MOD_MULTIPLE_INTERPRETERS_NOT_SUPPORTED ((void *)0)
#  define Py_MOD_MULTIPLE_INTERPRETERS_SUPPORTED ((void *)1)
#  define Py_MOD_PER_INTERPRETER_GIL_SUPPORTED ((void *)2)
#endif

/* for Py_mod_gil: */
#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 >= 0x030d0000
#  define Py_MOD_GIL_USED ((void *)0)
#  define Py_MOD_GIL_NOT_USED ((void *)1)
#endif

#if !defined(Py_LIMITED_API) && defined(Py_GIL_DISABLED)
PyAPI_FUNC(int) PyUnstable_Module_SetGIL(PyObject *module, void *gil);
#endif

#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 >= _Py_PACK_VERSION(3, 15)
PyAPI_FUNC(PyObject *) PyModule_FromSlotsAndSpec(const PyModuleDef_Slot *,
                                                 PyObject *spec);
PyAPI_FUNC(int) PyModule_Exec(PyObject *mod);
PyAPI_FUNC(int) PyModule_GetStateSize(PyObject *mod, Py_ssize_t *result);
PyAPI_FUNC(int) PyModule_GetToken(PyObject *, void **result);
#endif

#ifndef _Py_OPAQUE_PYOBJECT
struct PyModuleDef {
  PyModuleDef_Base m_base;
  const char* m_name;
  const char* m_doc;
  Py_ssize_t m_size;
  PyMethodDef *m_methods;
  PyModuleDef_Slot *m_slots;
  traverseproc m_traverse;
  inquiry m_clear;
  freefunc m_free;
};
#endif  // _Py_OPAQUE_PYOBJECT

#ifdef __cplusplus
}
#endif
#endif /* !Py_MODULEOBJECT_H */
