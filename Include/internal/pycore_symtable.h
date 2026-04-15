#ifndef Py_INTERNAL_SYMTABLE_H
#define Py_INTERNAL_SYMTABLE_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

struct _mod;   // Type defined in pycore_ast.h

typedef enum _block_type {
    FunctionBlock, ClassBlock, ModuleBlock,
    // Used for annotations. If 'from __future__ import annotations' is active,
    // annotation blocks cannot bind names and are not evaluated. Otherwise, they
    // are lazily evaluated (see PEP 649).
    AnnotationBlock,

    // The following blocks are used for generics and type aliases. These work
    // mostly like functions (see PEP 695 for details). The three different
    // blocks function identically; they are different enum entries only so
    // that error messages can be more precise.

    // The block to enter when processing a "type" (PEP 695) construction,
    // e.g., "type MyGeneric[T] = list[T]".
    TypeAliasBlock,
    // The block to enter when processing a "generic" (PEP 695) object,
    // e.g., "def foo[T](): pass" or "class A[T]: pass".
    TypeParametersBlock,
    // The block to enter when processing the bound, the constraint tuple
    // or the default value of a single "type variable" in the formal sense,
    // i.e., a TypeVar, a TypeVarTuple or a ParamSpec object (the latter two
    // do not support a bound or a constraint tuple).
    TypeVariableBlock,
} _Py_block_ty;

typedef enum _comprehension_type {
    NoComprehension = 0,
    ListComprehension = 1,
    DictComprehension = 2,
    SetComprehension = 3,
    GeneratorExpression = 4 } _Py_comprehension_ty;

/* source location information */
typedef struct {
    int lineno;
    int end_lineno;
    int col_offset;
    int end_col_offset;
} _Py_SourceLocation;

#define SRC_LOCATION_FROM_AST(n) \
    (_Py_SourceLocation){ \
               .lineno = (n)->lineno, \
               .end_lineno = (n)->end_lineno, \
               .col_offset = (n)->col_offset, \
               .end_col_offset = (n)->end_col_offset }

static const _Py_SourceLocation NO_LOCATION = {-1, -1, -1, -1};
static const _Py_SourceLocation NEXT_LOCATION = {-2, -2, -2, -2};

/* __future__ information */
typedef struct {
    int ff_features;                    /* flags set by future statements */
    _Py_SourceLocation ff_location;     /* location of last future statement */
} _PyFutureFeatures;

struct _symtable_entry;

struct symtable {
    PyObject *st_filename;          /* name of file being compiled,
                                       decoded from the filesystem encoding */
    struct _symtable_entry *st_cur; /* current symbol table entry */
    struct _symtable_entry *st_top; /* symbol table entry for module */
    PyObject *st_blocks;            /* dict: map AST node addresses
                                     *       to symbol table entries */
    PyObject *st_stack;             /* list: stack of namespace info */
    PyObject *st_global;            /* borrowed ref to st_top->ste_symbols */
    int st_nblocks;                 /* number of blocks used. kept for
                                       consistency with the corresponding
                                       compiler structure */
    PyObject *st_private;           /* name of current class or NULL */
    _PyFutureFeatures *st_future;   /* module's future features that affect
                                       the symbol table */
};

typedef struct _symtable_entry {
    PyObject_HEAD
    PyObject *ste_id;        /* int: key in ste_table->st_blocks */
    PyObject *ste_symbols;   /* dict: variable names to flags */
    PyObject *ste_name;      /* string: name of current block */
    PyObject *ste_varnames;  /* list of function parameters */
    PyObject *ste_children;  /* list of child blocks */
    PyObject *ste_directives;/* locations of global and nonlocal statements */
    PyObject *ste_mangled_names; /* set of names for which mangling should be applied */

    _Py_block_ty ste_type;
    // Optional string set by symtable.c and used when reporting errors.
    // The content of that string is a description of the current "context".
    //
    // For instance, if we are processing the default value of the type
    // variable "T" in "def foo[T = int](): pass", `ste_scope_info` is
    // set to "a TypeVar default".
    const char *ste_scope_info;

    int ste_nested;      /* true if block is nested */
    unsigned ste_generator : 1;   /* true if namespace is a generator */
    unsigned ste_coroutine : 1;   /* true if namespace is a coroutine */
    unsigned ste_annotations_used : 1;  /* true if there are any annotations in this scope */
    _Py_comprehension_ty ste_comprehension;  /* Kind of comprehension (if any) */
    unsigned ste_varargs : 1;     /* true if block has varargs */
    unsigned ste_varkeywords : 1; /* true if block has varkeywords */
    unsigned ste_returns_value : 1;  /* true if namespace uses return with
                                        an argument */
    unsigned ste_needs_class_closure : 1; /* for class scopes, true if a
                                             closure over __class__
                                             should be created */
    unsigned ste_needs_classdict : 1; /* for class scopes, true if a closure
                                         over the class dict should be created */
    unsigned ste_comp_inlined : 1; /* true if this comprehension is inlined */
    unsigned ste_comp_iter_target : 1; /* true if visiting comprehension target */
    unsigned ste_can_see_class_scope : 1; /* true if this block can see names bound in an
                                             enclosing class scope */
    unsigned ste_has_docstring : 1; /* true if docstring present */
    unsigned ste_method : 1; /* true if block is a function block defined in class scope */
    unsigned ste_has_conditional_annotations : 1; /* true if block has conditionally executed annotations */
    unsigned ste_in_conditional_block : 1; /* set while we are inside a conditionally executed block */
    unsigned ste_in_try_block : 1; /* set while we are inside a try/except block */
    unsigned ste_in_unevaluated_annotation : 1; /* set while we are processing an annotation that will not be evaluated */
    int ste_comp_iter_expr; /* non-zero if visiting a comprehension range expression */
    _Py_SourceLocation ste_loc; /* source location of block */
    struct _symtable_entry *ste_annotation_block; /* symbol table entry for this entry's annotations */
    struct symtable *ste_table;
} PySTEntryObject;

extern PyTypeObject PySTEntry_Type;

#define PySTEntry_Check(op) Py_IS_TYPE((op), &PySTEntry_Type)

extern long _PyST_GetSymbol(PySTEntryObject *, PyObject *);
extern int _PyST_GetScope(PySTEntryObject *, PyObject *);
extern int _PyST_IsFunctionLike(PySTEntryObject *);

extern struct symtable* _PySymtable_Build(
    struct _mod *mod,
    PyObject *filename,
    _PyFutureFeatures *future);
extern PySTEntryObject* _PySymtable_Lookup(struct symtable *, void *);
extern int _PySymtable_LookupOptional(struct symtable *, void *, PySTEntryObject **);

extern void _PySymtable_Free(struct symtable *);

extern PyObject *_Py_MaybeMangle(PyObject *privateobj, PySTEntryObject *ste, PyObject *name);

// Export for '_pickle' shared extension
PyAPI_FUNC(PyObject *)
_Py_Mangle(PyObject *, PyObject *);
PyAPI_FUNC(int)
_Py_IsPrivateName(PyObject *);

/* Flags for def-use information */

#define DEF_GLOBAL 1             /* global stmt */
#define DEF_LOCAL 2              /* assignment in code block */
#define DEF_PARAM (2<<1)         /* formal parameter */
#define DEF_NONLOCAL (2<<2)      /* nonlocal stmt */
#define USE (2<<3)               /* name is used */
#define DEF_FREE_CLASS (2<<5)    /* free variable from class's method */
#define DEF_IMPORT (2<<6)        /* assignment occurred via import */
#define DEF_ANNOT (2<<7)         /* this name is annotated */
#define DEF_COMP_ITER (2<<8)     /* this name is a comprehension iteration variable */
#define DEF_TYPE_PARAM (2<<9)    /* this name is a type parameter */
#define DEF_COMP_CELL (2<<10)    /* this name is a cell in an inlined comprehension */

#define DEF_BOUND (DEF_LOCAL | DEF_PARAM | DEF_IMPORT)

/* GLOBAL_EXPLICIT and GLOBAL_IMPLICIT are used internally by the symbol
   table.  GLOBAL is returned from _PyST_GetScope() for either of them.
   It is stored in ste_symbols at bits 13-16.
*/
#define SCOPE_OFFSET 12
#define SCOPE_MASK (DEF_GLOBAL | DEF_LOCAL | DEF_PARAM | DEF_NONLOCAL)
#define SYMBOL_TO_SCOPE(S) (((S) >> SCOPE_OFFSET) & SCOPE_MASK)

#define LOCAL 1
#define GLOBAL_EXPLICIT 2
#define GLOBAL_IMPLICIT 3
#define FREE 4
#define CELL 5

// Used by symtablemodule.c
extern struct symtable* _Py_SymtableStringObjectFlags(
    const char *str,
    PyObject *filename,
    int start,
    PyCompilerFlags *flags,
    PyObject *module);

int _PyFuture_FromAST(
    struct _mod * mod,
    PyObject *filename,
    _PyFutureFeatures* futures);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_SYMTABLE_H */
