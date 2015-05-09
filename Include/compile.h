#ifndef Py_COMPILE_H
#define Py_COMPILE_H

#ifndef Py_LIMITED_API
#include "code.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Public interface */
struct _node; /* Declare the existence of this type */
PyAPI_FUNC(PyCodeObject *) PyNode_Compile(struct _node *, const char *);

/* Future feature support */

typedef struct {
    int ff_features;      /* flags set by future statements */
    int ff_lineno;        /* line number of last future statement */
} PyFutureFeatures;

#define FUTURE_NESTED_SCOPES "nested_scopes"
#define FUTURE_GENERATORS "generators"
#define FUTURE_DIVISION "division"
#define FUTURE_ABSOLUTE_IMPORT "absolute_import"
#define FUTURE_WITH_STATEMENT "with_statement"
#define FUTURE_PRINT_FUNCTION "print_function"
#define FUTURE_UNICODE_LITERALS "unicode_literals"
#define FUTURE_BARRY_AS_BDFL "barry_as_FLUFL"
#define FUTURE_GENERATOR_STOP "generator_stop"

struct _mod; /* Declare the existence of this type */
#define PyAST_Compile(mod, s, f, ar) PyAST_CompileEx(mod, s, f, -1, ar)
PyAPI_FUNC(PyCodeObject *) PyAST_CompileEx(
    struct _mod *mod,
    const char *filename,       /* decoded from the filesystem encoding */
    PyCompilerFlags *flags,
    int optimize,
    PyArena *arena);
PyAPI_FUNC(PyCodeObject *) PyAST_CompileObject(
    struct _mod *mod,
    PyObject *filename,
    PyCompilerFlags *flags,
    int optimize,
    PyArena *arena);
PyAPI_FUNC(PyFutureFeatures *) PyFuture_FromAST(
    struct _mod * mod,
    const char *filename        /* decoded from the filesystem encoding */
    );
PyAPI_FUNC(PyFutureFeatures *) PyFuture_FromASTObject(
    struct _mod * mod,
    PyObject *filename
    );

/* _Py_Mangle is defined in compile.c */
PyAPI_FUNC(PyObject*) _Py_Mangle(PyObject *p, PyObject *name);

#define PY_INVALID_STACK_EFFECT INT_MAX
PyAPI_FUNC(int) PyCompile_OpcodeStackEffect(int opcode, int oparg);

#ifdef __cplusplus
}
#endif

#endif /* !Py_LIMITED_API */

/* These definitions must match corresponding definitions in graminit.h.
   There's code in compile.c that checks that they are the same. */
#define Py_single_input 256
#define Py_file_input 257
#define Py_eval_input 258

#endif /* !Py_COMPILE_H */
