#ifndef Py_INTERNAL_COMPILE_H
#define Py_INTERNAL_COMPILE_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

struct _arena;   // Type defined in pycore_pyarena.h
struct _mod;     // Type defined in pycore_ast.h

// Export the symbol for test_peg_generator (built as a library)
PyAPI_FUNC(PyCodeObject*) _PyAST_Compile(
    struct _mod *mod,
    PyObject *filename,
    PyCompilerFlags *flags,
    int optimize,
    struct _arena *arena);


typedef struct {
    int optimize;
    int ff_features;

    int recursion_depth;            /* current recursion depth */
    int recursion_limit;            /* recursion limit */
} _PyASTOptimizeState;

extern int _PyAST_Optimize(
    struct _mod *,
    struct _arena *arena,
    _PyASTOptimizeState *state);

/* Utility for a number of growing arrays used in the compiler */
int _PyCompile_EnsureArrayLargeEnough(
        int idx,
        void **array,
        int *alloc,
        int default_alloc,
        size_t item_size);

int _PyCompile_ConstCacheMergeOne(PyObject *const_cache, PyObject **obj);

/* Access compiler internals for unit testing */

PyAPI_FUNC(PyObject*) _PyCompile_CodeGen(
        PyObject *ast,
        PyObject *filename,
        PyCompilerFlags *flags,
        int optimize);

PyAPI_FUNC(PyObject*) _PyCompile_OptimizeCfg(
        PyObject *instructions,
        PyObject *consts);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_COMPILE_H */
