#ifndef Py_INTERNAL_COMPILE_H
#define Py_INTERNAL_COMPILE_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_ast.h"       // mod_ty
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

struct compiler;

typedef enum { OP_FAST, OP_GLOBAL, OP_DEREF, OP_NAME } compiler_optype;

/* _PyCompile_FBlockInfo tracks the current frame block.
 *
 * A frame block is used to handle loops, try/except, and try/finally.
 * It's called a frame block to distinguish it from a basic block in the
 * compiler IR.
 */

enum _PyCompile_FBlockType : short;

typedef struct {
    enum _PyCompile_FBlockType fb_type;
    _PyJumpTargetLabel fb_block;
    _Py_SourceLocation fb_loc;
    /* (optional) type-specific exit or cleanup block */
    _PyJumpTargetLabel fb_exit;
    /* (optional) additional information required for unwinding */
    void *fb_datum;
} _PyCompile_FBlockInfo;


int _PyCompiler_PushFBlock(struct compiler *c, _Py_SourceLocation loc,
                           enum _PyCompile_FBlockType t,
                           _PyJumpTargetLabel block_label,
                           _PyJumpTargetLabel exit, void *datum);
void _PyCompiler_PopFBlock(struct compiler *c, enum _PyCompile_FBlockType t,
                           _PyJumpTargetLabel block_label);
_PyCompile_FBlockInfo *_PyCompiler_TopFBlock(struct compiler *c);

int _PyCompiler_EnterScope(struct compiler *c, identifier name, int scope_type,
                           void *key, int lineno, PyObject *private,
                           _PyCompile_CodeUnitMetadata *umd);
void _PyCompiler_ExitScope(struct compiler *c);
Py_ssize_t _PyCompiler_AddConst(struct compiler *c, PyObject *o);
_PyInstructionSequence *_PyCompiler_InstrSequence(struct compiler *c);
int _PyCompiler_FutureFeatures(struct compiler *c);
PyObject *_PyCompiler_DeferredAnnotations(struct compiler *c);
PyObject *_PyCompiler_Mangle(struct compiler *c, PyObject *name);
PyObject *_PyCompiler_MaybeMangle(struct compiler *c, PyObject *name);
int _PyCompiler_MaybeAddStaticAttributeToClass(struct compiler *c, expr_ty e);
int _PyCompiler_GetRefType(struct compiler *c, PyObject *name);
int _PyCompiler_LookupCellvar(struct compiler *c, PyObject *name);
int _PyCompiler_ResolveNameop(struct compiler *c, PyObject *mangled, int scope,
                              compiler_optype *optype, Py_ssize_t *arg);

int _PyCompiler_IsInteractive(struct compiler *c);
int _PyCompiler_IsNestedScope(struct compiler *c);
int _PyCompiler_IsInInlinedComp(struct compiler *c);
int _PyCompiler_ScopeType(struct compiler *c);
int _PyCompiler_OptimizationLevel(struct compiler *c);
PyArena *_PyCompiler_Arena(struct compiler *c);
int _PyCompiler_LookupArg(struct compiler *c, PyCodeObject *co, PyObject *name);
PyObject *_PyCompiler_Qualname(struct compiler *c);
_PyCompile_CodeUnitMetadata *_PyCompiler_Metadata(struct compiler *c);
PyObject *_PyCompiler_StaticAttributesAsTuple(struct compiler *c);

#ifndef NDEBUG
int _PyCompiler_IsTopLevelAwait(struct compiler *c);
#endif

struct symtable *_PyCompiler_Symtable(struct compiler *c);
PySTEntryObject *_PyCompiler_SymtableEntry(struct compiler *c);

enum {
    COMPILER_SCOPE_MODULE,
    COMPILER_SCOPE_CLASS,
    COMPILER_SCOPE_FUNCTION,
    COMPILER_SCOPE_ASYNC_FUNCTION,
    COMPILER_SCOPE_LAMBDA,
    COMPILER_SCOPE_COMPREHENSION,
    COMPILER_SCOPE_ANNOTATIONS,
};


typedef struct {
    PyObject *pushed_locals;
    PyObject *temp_symbols;
    PyObject *fast_hidden;
    _PyJumpTargetLabel cleanup;
} _PyCompille_InlinedComprehensionState;

int _PyCompiler_TweakInlinedComprehensionScopes(struct compiler *c, _Py_SourceLocation loc,
                                                PySTEntryObject *entry,
                                                _PyCompille_InlinedComprehensionState *state);
int _PyCompiler_RevertInlinedComprehensionScopes(struct compiler *c, _Py_SourceLocation loc,
                                                 _PyCompille_InlinedComprehensionState *state);
int _PyCompiler_AddDeferredAnnotaion(struct compiler *c, stmt_ty s);

int _PyCodegen_AddReturnAtEnd(struct compiler *c, int addNone);
int _PyCodegen_EnterAnonymousScope(struct compiler* c, mod_ty mod);
int _PyCodegen_Expression(struct compiler *c, expr_ty e);
int _PyCodegen_Body(struct compiler *c, _Py_SourceLocation loc, asdl_stmt_seq *stmts);

/* Utility for a number of growing arrays used in the compiler */
int _PyCompile_EnsureArrayLargeEnough(
        int idx,
        void **array,
        int *alloc,
        int default_alloc,
        size_t item_size);

int _PyCompiler_ConstCacheMergeOne(PyObject *const_cache, PyObject **obj);

PyCodeObject *_PyCompile_OptimizeAndAssemble(struct compiler *c, int addNone);

Py_ssize_t _PyCompile_DictAddObj(PyObject *dict, PyObject *o);
int _PyCompiler_Error(struct compiler *c, _Py_SourceLocation loc, const char *format, ...);
int _PyCompiler_Warn(struct compiler *c, _Py_SourceLocation loc, const char *format, ...);

// Export for '_opcode' extension module
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
PyAPI_FUNC(PyCodeObject*)
_PyCompile_Assemble(_PyCompile_CodeUnitMetadata *umd, PyObject *filename,
                    PyObject *instructions);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_COMPILE_H */
