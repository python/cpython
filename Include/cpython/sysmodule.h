#ifndef Py_CPYTHON_SYSMODULE_H
#  error "this header file must not be included directly"
#endif

typedef int(*Py_AuditHookFunction)(const char *, PyObject *, void *);

PyAPI_FUNC(int) PySys_Audit(
    const char *event,
    const char *format,
    ...);
PyAPI_FUNC(int) PySys_AddAuditHook(Py_AuditHookFunction, void*);

PyAPI_FUNC(PyObject *) PySys_GetAttr(PyObject *);
PyAPI_FUNC(PyObject *) PySys_GetAttrString(const char *);
PyAPI_FUNC(int) PySys_GetOptionalAttr(PyObject *, PyObject **);
PyAPI_FUNC(int) PySys_GetOptionalAttrString(const char *, PyObject **);

typedef struct {
    FILE* perf_map;
    PyThread_type_lock map_lock;
} PerfMapState;

PyAPI_FUNC(int) PyUnstable_PerfMapState_Init(void);
PyAPI_FUNC(int) PyUnstable_WritePerfMapEntry(
    const void *code_addr,
    unsigned int code_size,
    const char *entry_name);
PyAPI_FUNC(void) PyUnstable_PerfMapState_Fini(void);
