#ifndef Py_CPYTHON_CEVAL_H
#  error "this header file must not be included directly"
#endif

PyAPI_FUNC(void) PyEval_SetProfile(Py_tracefunc, PyObject *);
PyAPI_FUNC(void) PyEval_SetProfileAllThreads(Py_tracefunc, PyObject *);
PyAPI_FUNC(void) PyEval_SetTrace(Py_tracefunc, PyObject *);
PyAPI_FUNC(void) PyEval_SetTraceAllThreads(Py_tracefunc, PyObject *);

/* Look at the current frame's (if any) code's co_flags, and turn on
   the corresponding compiler flags in cf->cf_flags.  Return 1 if any
   flag was set, else return 0. */
PyAPI_FUNC(int) PyEval_MergeCompilerFlags(PyCompilerFlags *cf);

PyAPI_FUNC(PyObject *) _PyEval_EvalFrameDefault(PyThreadState *tstate, struct _PyInterpreterFrame *f, int exc);

PyAPI_FUNC(Py_ssize_t) PyUnstable_Eval_RequestCodeExtraIndex(freefunc);
// Old name -- remove when this API changes:
_Py_DEPRECATED_EXTERNALLY(3.12) static inline Py_ssize_t
_PyEval_RequestCodeExtraIndex(freefunc f) {
    return PyUnstable_Eval_RequestCodeExtraIndex(f);
}

PyAPI_FUNC(int) _PyEval_SliceIndex(PyObject *, Py_ssize_t *);
PyAPI_FUNC(int) _PyEval_SliceIndexNotNone(PyObject *, Py_ssize_t *);


// Trampoline API

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
PyAPI_FUNC(int) PyUnstable_CopyPerfMapFile(const char* parent_filename);
PyAPI_FUNC(int) PyUnstable_PerfTrampoline_CompileCode(PyCodeObject *);
PyAPI_FUNC(int) PyUnstable_PerfTrampoline_SetPersistAfterFork(int enable);
