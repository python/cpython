#ifndef Py_INTERNAL_COMPILE_H
#define Py_INTERNAL_COMPILE_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_symtable.h"  // _Py_SourceLocation
#include "pycore_instruction_sequence.h"

struct _arena;   // Type defined in pycore_pyarena.h
struct _mod;     // Type defined in pycore_ast.h

// Export for 'test_peg_generator' shared extension
PyAPI_FUNC(PyCodeObject*) _PyAST_Compile(
    struct _mod *mod,
    PyObject *filename,
    PyCompilerFlags *flags,
    int optimize,
    struct _arena *arena);

/* AST optimizations */
extern int _PyCompile_AstOptimize(
    struct _mod *mod,
    PyObject *filename,
    PyCompilerFlags *flags,
    int optimize,
    struct _arena *arena);

struct _Py_SourceLocation;

extern int _PyAST_Optimize(
    struct _mod *,
    struct _arena *arena,
    int optimize,
    int ff_features);


typedef struct {
    PyObject *u_name;
    PyObject *u_qualname;  /* dot-separated qualified name (lazy) */

    /* The following fields are dicts that map objects to
       the index of them in co_XXX.      The index is used as
       the argument for opcodes that refer to those collections.
    */
    PyObject *u_consts;    /* all constants */
    PyObject *u_names;     /* all names */
    PyObject *u_varnames;  /* local variables */
    PyObject *u_cellvars;  /* cell variables */
    PyObject *u_freevars;  /* free variables */
    PyObject *u_fasthidden; /* dict; keys are names that are fast-locals only
                               temporarily within an inlined comprehension. When
                               value is True, treat as fast-local. */

    Py_ssize_t u_argcount;        /* number of arguments for block */
    Py_ssize_t u_posonlyargcount;        /* number of positional only arguments for block */
    Py_ssize_t u_kwonlyargcount; /* number of keyword only arguments for block */

    int u_firstlineno; /* the first lineno of the block */
} _PyCompile_CodeUnitMetadata;


/* Utility for a number of growing arrays used in the compiler */
int _PyCompile_EnsureArrayLargeEnough(
        int idx,
        void **array,
        int *alloc,
        int default_alloc,
        size_t item_size);

int _PyCompile_ConstCacheMergeOne(PyObject *const_cache, PyObject **obj);


// Export for '_opcode' extension module
PyAPI_FUNC(int) _PyCompile_OpcodeIsValid(int opcode);
PyAPI_FUNC(int) _PyCompile_OpcodeHasArg(int opcode);
PyAPI_FUNC(int) _PyCompile_OpcodeHasConst(int opcode);
PyAPI_FUNC(int) _PyCompile_OpcodeHasName(int opcode);
PyAPI_FUNC(int) _PyCompile_OpcodeHasJump(int opcode);
PyAPI_FUNC(int) _PyCompile_OpcodeHasFree(int opcode);
PyAPI_FUNC(int) _PyCompile_OpcodeHasLocal(int opcode);
PyAPI_FUNC(int) _PyCompile_OpcodeHasExc(int opcode);

PyAPI_FUNC(PyObject*) _PyCompile_GetUnaryIntrinsicName(int index);
PyAPI_FUNC(PyObject*) _PyCompile_GetBinaryIntrinsicName(int index);

/* Access compiler internals for unit testing */

// Export for '_testinternalcapi' shared extension
PyAPI_FUNC(PyObject*) _PyCompile_CleanDoc(PyObject *doc);

// Export for '_testinternalcapi' shared extension
PyAPI_FUNC(PyObject*) _PyCompile_CodeGen(
        PyObject *ast,
        PyObject *filename,
        PyCompilerFlags *flags,
        int optimize,
        int compile_mode);

// Export for '_testinternalcapi' shared extension
PyAPI_FUNC(PyObject*) _PyCompile_OptimizeCfg(
        PyObject *instructions,
        PyObject *consts,
        int nlocals);

// Export for '_testinternalcapi' shared extension
PyAPI_FUNC(PyCodeObject*)
_PyCompile_Assemble(_PyCompile_CodeUnitMetadata *umd, PyObject *filename,
                    PyObject *instructions);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_COMPILE_H */
