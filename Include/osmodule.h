
/* os module interface */

#ifndef Py_OSMODULE_H
#define Py_OSMODULE_H
#ifdef __cplusplus
extern "C" {
#endif

#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 >= 0x03060000
PyAPI_FUNC(PyObject *) PyOS_FSPath(PyObject *path);
#endif

#if !defined(Py_LIMITED_API)
typedef struct {
     FILE* perf_map;
    PyThread_type_lock map_lock;
} PerfMapState;

PyAPI_FUNC(int) PyUnstable_PerfMapState_Init(void);

PyAPI_FUNC(int) PyUnstable_WritePerfMapEntry(const void *code_addr, unsigned int code_size, const char *entry_name);

PyAPI_FUNC(void) PyUnstable_PerfMapState_Fini(void);
#endif

#ifdef __cplusplus
}
#endif
#endif /* !Py_OSMODULE_H */
