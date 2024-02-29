/*
 * This file compiles an abstract syntax tree (AST) into Python bytecode.
 *
 * The primary entry point is _PyAST_Compile(), which returns a
 * PyCodeObject.  The compiler makes several passes to build the code
 * object:
 *   1. Checks for future statements.  See future.c
 *   2. Builds a symbol table.  See symtable.c.
 *   3. Generate an instruction sequence. See compiler_mod() in this file.
 *   4. Generate a control flow graph and run optimizations on it.  See flowgraph.c.
 *   5. Assemble the basic blocks into final code.  See optimize_and_assemble() in
 *      this file, and assembler.c.
 *
 * Note that compiler_mod() suggests module, but the module ast type
 * (mod_ty) has cases for expressions and interactive statements.
 *
 * CAUTION: The VISIT_* macros abort the current function when they
 * encounter a problem. So don't invoke them when there is memory
 * which needs to be released. Code blocks are OK, as the compiler
 * structure takes care of releasing those.  Use the arena to manage
 * objects.
 */

#include <stdbool.h>

#include "Python.h"
#include "pycore_ast.h"           // _PyAST_GetDocString()
#define NEED_OPCODE_TABLES
#include "pycore_opcode_utils.h"
#undef NEED_OPCODE_TABLES
#include "pycore_flowgraph.h"
#include "pycore_code.h"          // _PyCode_New()
#include "pycore_compile.h"
#include "pycore_intrinsics.h"
#include "pycore_long.h"          // _PyLong_GetZero()
#include "pycore_pymem.h"         // _PyMem_IsPtrFreed()
#include "pycore_symtable.h"      // PySTEntryObject, _PyFuture_FromAST()

#include "opcode_metadata.h"      // _PyOpcode_opcode_metadata, _PyOpcode_num_popped/pushed

#define DEFAULT_CODE_SIZE 128
#define DEFAULT_LNOTAB_SIZE 16
#define DEFAULT_CNOTAB_SIZE 32

#define COMP_GENEXP   0
#define COMP_LISTCOMP 1
#define COMP_SETCOMP  2
#define COMP_DICTCOMP 3

/* A soft limit for stack use, to avoid excessive
 * memory use for large constants, etc.
 *
 * The value 30 is plucked out of thin air.
 * Code that could use more stack than this is
 * rare, so the exact value is unimportant.
 */
#define STACK_USE_GUIDELINE 30

#undef SUCCESS
#undef ERROR
#define SUCCESS 0
#define ERROR -1

#define RETURN_IF_ERROR(X)  \
    if ((X) == -1) {        \
        return ERROR;       \
    }

/* If we exceed this limit, it should
 * be considered a compiler bug.
 * Currently it should be impossible
 * to exceed STACK_USE_GUIDELINE * 100,
 * as 100 is the maximum parse depth.
 * For performance reasons we will
 * want to reduce this to a
 * few hundred in the future.
 *
 * NOTE: Whatever MAX_ALLOWED_STACK_USE is
 * set to, it should never restrict what Python
 * we can write, just how we compile it.
 */
#define MAX_ALLOWED_STACK_USE (STACK_USE_GUIDELINE * 100)

#define IS_TOP_LEVEL_AWAIT(C) ( \
        ((C)->c_flags.cf_flags & PyCF_ALLOW_TOP_LEVEL_AWAIT) \
        && ((C)->u->u_ste->ste_type == ModuleBlock))

typedef _PyCompilerSrcLocation location;
typedef _PyCfgInstruction cfg_instr;
typedef _PyCfgBasicblock basicblock;
typedef _PyCfgBuilder cfg_builder;

#define LOCATION(LNO, END_LNO, COL, END_COL) \
    ((const _PyCompilerSrcLocation){(LNO), (END_LNO), (COL), (END_COL)})

/* Return true if loc1 starts after loc2 ends. */
static inline bool
location_is_after(location loc1, location loc2) {
    return (loc1.lineno > loc2.end_lineno) ||
            ((loc1.lineno == loc2.end_lineno) &&
             (loc1.col_offset > loc2.end_col_offset));
}

#define LOC(x) SRC_LOCATION_FROM_AST(x)

typedef _PyCfgJumpTargetLabel jump_target_label;

static jump_target_label NO_LABEL = {-1};

#define SAME_LABEL(L1, L2) ((L1).id == (L2).id)
#define IS_LABEL(L) (!SAME_LABEL((L), (NO_LABEL)))

#define NEW_JUMP_TARGET_LABEL(C, NAME) \
    jump_target_label NAME = instr_sequence_new_label(INSTR_SEQUENCE(C)); \
    if (!IS_LABEL(NAME)) { \
        return ERROR; \
    }

#define USE_LABEL(C, LBL) \
    RETURN_IF_ERROR(instr_sequence_use_label(INSTR_SEQUENCE(C), (LBL).id))


/* fblockinfo tracks the current frame block.

A frame block is used to handle loops, try/except, and try/finally.
It's called a frame block to distinguish it from a basic block in the
compiler IR.
*/

enum fblocktype { WHILE_LOOP, FOR_LOOP, TRY_EXCEPT, FINALLY_TRY, FINALLY_END,
                  WITH, ASYNC_WITH, HANDLER_CLEANUP, POP_VALUE, EXCEPTION_HANDLER,
                  EXCEPTION_GROUP_HANDLER, ASYNC_COMPREHENSION_GENERATOR };

struct fblockinfo {
    enum fblocktype fb_type;
    jump_target_label fb_block;
    /* (optional) type-specific exit or cleanup block */
    jump_target_label fb_exit;
    /* (optional) additional information required for unwinding */
    void *fb_datum;
};

enum {
    COMPILER_SCOPE_MODULE,
    COMPILER_SCOPE_CLASS,
    COMPILER_SCOPE_FUNCTION,
    COMPILER_SCOPE_ASYNC_FUNCTION,
    COMPILER_SCOPE_LAMBDA,
    COMPILER_SCOPE_COMPREHENSION,
    COMPILER_SCOPE_TYPEPARAMS,
};


int
_PyCompile_InstrSize(int opcode, int oparg)
{
    assert(!IS_PSEUDO_OPCODE(opcode));
    assert(HAS_ARG(opcode) || oparg == 0);
    int extended_args = (0xFFFFFF < oparg) + (0xFFFF < oparg) + (0xFF < oparg);
    int caches = _PyOpcode_Caches[opcode];
    return extended_args + 1 + caches;
}

typedef _PyCompile_Instruction instruction;
typedef _PyCompile_InstructionSequence instr_sequence;

#define INITIAL_INSTR_SEQUENCE_SIZE 100
#define INITIAL_INSTR_SEQUENCE_LABELS_MAP_SIZE 10

/*
 * Resize the array if index is out of range.
 *
 * idx: the index we want to access
 * arr: pointer to the array
 * alloc: pointer to the capacity of the array
 * default_alloc: initial number of items
 * item_size: size of each item
 *
 */
int
_PyCompile_EnsureArrayLargeEnough(int idx, void **array, int *alloc,
                                  int default_alloc, size_t item_size)
{
    void *arr = *array;
    if (arr == NULL) {
        int new_alloc = default_alloc;
        if (idx >= new_alloc) {
            new_alloc = idx + default_alloc;
        }
        arr = PyObject_Calloc(new_alloc, item_size);
        if (arr == NULL) {
            PyErr_NoMemory();
            return ERROR;
        }
        *alloc = new_alloc;
    }
    else if (idx >= *alloc) {
        size_t oldsize = *alloc * item_size;
        int new_alloc = *alloc << 1;
        if (idx >= new_alloc) {
            new_alloc = idx + default_alloc;
        }
        size_t newsize = new_alloc * item_size;

        if (oldsize > (SIZE_MAX >> 1)) {
            PyErr_NoMemory();
            return ERROR;
        }

        assert(newsize > 0);
        void *tmp = PyObject_Realloc(arr, newsize);
        if (tmp == NULL) {
            PyErr_NoMemory();
            return ERROR;
        }
        *alloc = new_alloc;
        arr = tmp;
        memset((char *)arr + oldsize, 0, newsize - oldsize);
    }

    *array = arr;
    return SUCCESS;
}

static int
instr_sequence_next_inst(instr_sequence *seq) {
    assert(seq->s_instrs != NULL || seq->s_used == 0);

    RETURN_IF_ERROR(
        _PyCompile_EnsureArrayLargeEnough(seq->s_used + 1,
                                          (void**)&seq->s_instrs,
                                          &seq->s_allocated,
                                          INITIAL_INSTR_SEQUENCE_SIZE,
                                          sizeof(instruction)));
    assert(seq->s_allocated >= 0);
    assert(seq->s_used < seq->s_allocated);
    return seq->s_used++;
}

static jump_target_label
instr_sequence_new_label(instr_sequence *seq)
{
    jump_target_label lbl = {++seq->s_next_free_label};
    return lbl;
}

static int
instr_sequence_use_label(instr_sequence *seq, int lbl) {
    int old_size = seq->s_labelmap_size;
    RETURN_IF_ERROR(
        _PyCompile_EnsureArrayLargeEnough(lbl,
                                          (void**)&seq->s_labelmap,
                                           &seq->s_labelmap_size,
                                           INITIAL_INSTR_SEQUENCE_LABELS_MAP_SIZE,
                                           sizeof(int)));

    for(int i = old_size; i < seq->s_labelmap_size; i++) {
        seq->s_labelmap[i] = -111;  /* something weird, for debugging */
    }
    seq->s_labelmap[lbl] = seq->s_used; /* label refers to the next instruction */
    return SUCCESS;
}

static int
instr_sequence_addop(instr_sequence *seq, int opcode, int oparg, location loc)
{
    assert(IS_WITHIN_OPCODE_RANGE(opcode));
    assert(HAS_ARG(opcode) || HAS_TARGET(opcode) || oparg == 0);
    assert(0 <= oparg && oparg < (1 << 30));

    int idx = instr_sequence_next_inst(seq);
    RETURN_IF_ERROR(idx);
    instruction *ci = &seq->s_instrs[idx];
    ci->i_opcode = opcode;
    ci->i_oparg = oparg;
    ci->i_loc = loc;
    return SUCCESS;
}

static int
instr_sequence_insert_instruction(instr_sequence *seq, int pos,
                                  int opcode, int oparg, location loc)
{
    assert(pos >= 0 && pos <= seq->s_used);
    int last_idx = instr_sequence_next_inst(seq);
    RETURN_IF_ERROR(last_idx);
    for (int i=last_idx-1; i >= pos; i--) {
        seq->s_instrs[i+1] = seq->s_instrs[i];
    }
    instruction *ci = &seq->s_instrs[pos];
    ci->i_opcode = opcode;
    ci->i_oparg = oparg;
    ci->i_loc = loc;

    /* fix the labels map */
    for(int lbl=0; lbl < seq->s_labelmap_size; lbl++) {
        if (seq->s_labelmap[lbl] >= pos) {
            seq->s_labelmap[lbl]++;
        }
    }
    return SUCCESS;
}

static void
instr_sequence_fini(instr_sequence *seq) {
    PyObject_Free(seq->s_labelmap);
    seq->s_labelmap = NULL;

    PyObject_Free(seq->s_instrs);
    seq->s_instrs = NULL;
}

static int
instr_sequence_to_cfg(instr_sequence *seq, cfg_builder *g) {
    memset(g, 0, sizeof(cfg_builder));
    RETURN_IF_ERROR(_PyCfgBuilder_Init(g));

    /* There can be more than one label for the same offset. The
     * offset2lbl maping selects one of them which we use consistently.
     */

    int *offset2lbl = PyMem_Malloc(seq->s_used * sizeof(int));
    if (offset2lbl == NULL) {
        PyErr_NoMemory();
        return ERROR;
    }
    for (int i = 0; i < seq->s_used; i++) {
        offset2lbl[i] = -1;
    }
    for (int lbl=0; lbl < seq->s_labelmap_size; lbl++) {
        int offset = seq->s_labelmap[lbl];
        if (offset >= 0) {
            assert(offset < seq->s_used);
            offset2lbl[offset] = lbl;
        }
    }

    for (int i = 0; i < seq->s_used; i++) {
        int lbl = offset2lbl[i];
        if (lbl >= 0) {
            assert (lbl < seq->s_labelmap_size);
            jump_target_label lbl_ = {lbl};
            if (_PyCfgBuilder_UseLabel(g, lbl_) < 0) {
                goto error;
            }
        }
        instruction *instr = &seq->s_instrs[i];
        int opcode = instr->i_opcode;
        int oparg = instr->i_oparg;
        if (HAS_TARGET(opcode)) {
            int offset = seq->s_labelmap[oparg];
            assert(offset >= 0 && offset < seq->s_used);
            int lbl = offset2lbl[offset];
            assert(lbl >= 0 && lbl < seq->s_labelmap_size);
            oparg = lbl;
        }
        if (_PyCfgBuilder_Addop(g, opcode, oparg, instr->i_loc) < 0) {
            goto error;
        }
    }
    PyMem_Free(offset2lbl);

    int nblocks = 0;
    for (basicblock *b = g->g_block_list; b != NULL; b = b->b_list) {
        nblocks++;
    }
    if ((size_t)nblocks > SIZE_MAX / sizeof(basicblock *)) {
        PyErr_NoMemory();
        return ERROR;
    }
    return SUCCESS;
error:
    PyMem_Free(offset2lbl);
    return ERROR;
}


/* The following items change on entry and exit of code blocks.
   They must be saved and restored when returning to a block.
*/
struct compiler_unit {
    PySTEntryObject *u_ste;

    int u_scope_type;

    PyObject *u_private;        /* for private name mangling */

    instr_sequence u_instr_sequence; /* codegen output */

    int u_nfblocks;
    int u_in_inlined_comp;

    struct fblockinfo u_fblock[CO_MAXBLOCKS];

    _PyCompile_CodeUnitMetadata u_metadata;
};

/* This struct captures the global state of a compilation.

The u pointer points to the current compilation unit, while units
for enclosing blocks are stored in c_stack.     The u and c_stack are
managed by compiler_enter_scope() and compiler_exit_scope().

Note that we don't track recursion levels during compilation - the
task of detecting and rejecting excessive levels of nesting is
handled by the symbol analysis pass.

*/

struct compiler {
    PyObject *c_filename;
    struct symtable *c_st;
    PyFutureFeatures c_future;   /* module's __future__ */
    PyCompilerFlags c_flags;

    int c_optimize;              /* optimization level */
    int c_interactive;           /* true if in interactive mode */
    int c_nestlevel;
    PyObject *c_const_cache;     /* Python dict holding all constants,
                                    including names tuple */
    struct compiler_unit *u; /* compiler state for current block */
    PyObject *c_stack;           /* Python list holding compiler_unit ptrs */
    PyArena *c_arena;            /* pointer to memory allocation arena */
};

#define INSTR_SEQUENCE(C) (&((C)->u->u_instr_sequence))


typedef struct {
    // A list of strings corresponding to name captures. It is used to track:
    // - Repeated name assignments in the same pattern.
    // - Different name assignments in alternatives.
    // - The order of name assignments in alternatives.
    PyObject *stores;
    // If 0, any name captures against our subject will raise.
    int allow_irrefutable;
    // An array of blocks to jump to on failure. Jumping to fail_pop[i] will pop
    // i items off of the stack. The end result looks like this (with each block
    // falling through to the next):
    // fail_pop[4]: POP_TOP
    // fail_pop[3]: POP_TOP
    // fail_pop[2]: POP_TOP
    // fail_pop[1]: POP_TOP
    // fail_pop[0]: NOP
    jump_target_label *fail_pop;
    // The current length of fail_pop.
    Py_ssize_t fail_pop_size;
    // The number of items on top of the stack that need to *stay* on top of the
    // stack. Variable captures go beneath these. All of them will be popped on
    // failure.
    Py_ssize_t on_top;
} pattern_context;

static int codegen_addop_i(instr_sequence *seq, int opcode, Py_ssize_t oparg, location loc);

static void compiler_free(struct compiler *);
static int compiler_error(struct compiler *, location loc, const char *, ...);
static int compiler_warn(struct compiler *, location loc, const char *, ...);
static int compiler_nameop(struct compiler *, location, identifier, expr_context_ty);

static PyCodeObject *compiler_mod(struct compiler *, mod_ty);
static int compiler_visit_stmt(struct compiler *, stmt_ty);
static int compiler_visit_keyword(struct compiler *, keyword_ty);
static int compiler_visit_expr(struct compiler *, expr_ty);
static int compiler_augassign(struct compiler *, stmt_ty);
static int compiler_annassign(struct compiler *, stmt_ty);
static int compiler_subscript(struct compiler *, expr_ty);
static int compiler_slice(struct compiler *, expr_ty);

static bool are_all_items_const(asdl_expr_seq *, Py_ssize_t, Py_ssize_t);


static int compiler_with(struct compiler *, stmt_ty, int);
static int compiler_async_with(struct compiler *, stmt_ty, int);
static int compiler_async_for(struct compiler *, stmt_ty);
static int compiler_call_simple_kw_helper(struct compiler *c,
                                          location loc,
                                          asdl_keyword_seq *keywords,
                                          Py_ssize_t nkwelts);
static int compiler_call_helper(struct compiler *c, location loc,
                                int n, asdl_expr_seq *args,
                                asdl_keyword_seq *keywords);
static int compiler_try_except(struct compiler *, stmt_ty);
static int compiler_try_star_except(struct compiler *, stmt_ty);
static int compiler_set_qualname(struct compiler *);

static int compiler_sync_comprehension_generator(
                                      struct compiler *c, location loc,
                                      asdl_comprehension_seq *generators, int gen_index,
                                      int depth,
                                      expr_ty elt, expr_ty val, int type,
                                      int iter_on_stack);

static int compiler_async_comprehension_generator(
                                      struct compiler *c, location loc,
                                      asdl_comprehension_seq *generators, int gen_index,
                                      int depth,
                                      expr_ty elt, expr_ty val, int type,
                                      int iter_on_stack);

static int compiler_pattern(struct compiler *, pattern_ty, pattern_context *);
static int compiler_match(struct compiler *, stmt_ty);
static int compiler_pattern_subpattern(struct compiler *,
                                       pattern_ty, pattern_context *);

static PyCodeObject *optimize_and_assemble(struct compiler *, int addNone);

#define CAPSULE_NAME "compile.c compiler unit"


static int
compiler_setup(struct compiler *c, mod_ty mod, PyObject *filename,
               PyCompilerFlags *flags, int optimize, PyArena *arena)
{
    PyCompilerFlags local_flags = _PyCompilerFlags_INIT;

    c->c_const_cache = PyDict_New();
    if (!c->c_const_cache) {
        return ERROR;
    }

    c->c_stack = PyList_New(0);
    if (!c->c_stack) {
        return ERROR;
    }

    c->c_filename = Py_NewRef(filename);
    c->c_arena = arena;
    if (!_PyFuture_FromAST(mod, filename, &c->c_future)) {
        return ERROR;
    }
    if (!flags) {
        flags = &local_flags;
    }
    int merged = c->c_future.ff_features | flags->cf_flags;
    c->c_future.ff_features = merged;
    flags->cf_flags = merged;
    c->c_flags = *flags;
    c->c_optimize = (optimize == -1) ? _Py_GetConfig()->optimization_level : optimize;
    c->c_nestlevel = 0;

    _PyASTOptimizeState state;
    state.optimize = c->c_optimize;
    state.ff_features = merged;

    if (!_PyAST_Optimize(mod, arena, &state)) {
        return ERROR;
    }
    c->c_st = _PySymtable_Build(mod, filename, &c->c_future);
    if (c->c_st == NULL) {
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_SystemError, "no symtable");
        }
        return ERROR;
    }
    return SUCCESS;
}

static struct compiler*
new_compiler(mod_ty mod, PyObject *filename, PyCompilerFlags *pflags,
             int optimize, PyArena *arena)
{
    struct compiler *c = PyMem_Calloc(1, sizeof(struct compiler));
    if (c == NULL) {
        return NULL;
    }
    if (compiler_setup(c, mod, filename, pflags, optimize, arena) < 0) {
        compiler_free(c);
        return NULL;
    }
    return c;
}

PyCodeObject *
_PyAST_Compile(mod_ty mod, PyObject *filename, PyCompilerFlags *pflags,
               int optimize, PyArena *arena)
{
    assert(!PyErr_Occurred());
    struct compiler *c = new_compiler(mod, filename, pflags, optimize, arena);
    if (c == NULL) {
        return NULL;
    }

    PyCodeObject *co = compiler_mod(c, mod);
    compiler_free(c);
    assert(co || PyErr_Occurred());
    return co;
}

static void
compiler_free(struct compiler *c)
{
    if (c->c_st)
        _PySymtable_Free(c->c_st);
    Py_XDECREF(c->c_filename);
    Py_XDECREF(c->c_const_cache);
    Py_XDECREF(c->c_stack);
    PyMem_Free(c);
}

static PyObject *
list2dict(PyObject *list)
{
    Py_ssize_t i, n;
    PyObject *v, *k;
    PyObject *dict = PyDict_New();
    if (!dict) return NULL;

    n = PyList_Size(list);
    for (i = 0; i < n; i++) {
        v = PyLong_FromSsize_t(i);
        if (!v) {
            Py_DECREF(dict);
            return NULL;
        }
        k = PyList_GET_ITEM(list, i);
        if (PyDict_SetItem(dict, k, v) < 0) {
            Py_DECREF(v);
            Py_DECREF(dict);
            return NULL;
        }
        Py_DECREF(v);
    }
    return dict;
}

/* Return new dict containing names from src that match scope(s).

src is a symbol table dictionary.  If the scope of a name matches
either scope_type or flag is set, insert it into the new dict.  The
values are integers, starting at offset and increasing by one for
each key.
*/

static PyObject *
dictbytype(PyObject *src, int scope_type, int flag, Py_ssize_t offset)
{
    Py_ssize_t i = offset, scope, num_keys, key_i;
    PyObject *k, *v, *dest = PyDict_New();
    PyObject *sorted_keys;

    assert(offset >= 0);
    if (dest == NULL)
        return NULL;

    /* Sort the keys so that we have a deterministic order on the indexes
       saved in the returned dictionary.  These indexes are used as indexes
       into the free and cell var storage.  Therefore if they aren't
       deterministic, then the generated bytecode is not deterministic.
    */
    sorted_keys = PyDict_Keys(src);
    if (sorted_keys == NULL)
        return NULL;
    if (PyList_Sort(sorted_keys) != 0) {
        Py_DECREF(sorted_keys);
        return NULL;
    }
    num_keys = PyList_GET_SIZE(sorted_keys);

    for (key_i = 0; key_i < num_keys; key_i++) {
        /* XXX this should probably be a macro in symtable.h */
        long vi;
        k = PyList_GET_ITEM(sorted_keys, key_i);
        v = PyDict_GetItemWithError(src, k);
        assert(v && PyLong_Check(v));
        vi = PyLong_AS_LONG(v);
        scope = (vi >> SCOPE_OFFSET) & SCOPE_MASK;

        if (scope == scope_type || vi & flag) {
            PyObject *item = PyLong_FromSsize_t(i);
            if (item == NULL) {
                Py_DECREF(sorted_keys);
                Py_DECREF(dest);
                return NULL;
            }
            i++;
            if (PyDict_SetItem(dest, k, item) < 0) {
                Py_DECREF(sorted_keys);
                Py_DECREF(item);
                Py_DECREF(dest);
                return NULL;
            }
            Py_DECREF(item);
        }
    }
    Py_DECREF(sorted_keys);
    return dest;
}

static void
compiler_unit_free(struct compiler_unit *u)
{
    instr_sequence_fini(&u->u_instr_sequence);
    Py_CLEAR(u->u_ste);
    Py_CLEAR(u->u_metadata.u_name);
    Py_CLEAR(u->u_metadata.u_qualname);
    Py_CLEAR(u->u_metadata.u_consts);
    Py_CLEAR(u->u_metadata.u_names);
    Py_CLEAR(u->u_metadata.u_varnames);
    Py_CLEAR(u->u_metadata.u_freevars);
    Py_CLEAR(u->u_metadata.u_cellvars);
    Py_CLEAR(u->u_metadata.u_fasthidden);
    Py_CLEAR(u->u_private);
    PyObject_Free(u);
}

static int
compiler_set_qualname(struct compiler *c)
{
    Py_ssize_t stack_size;
    struct compiler_unit *u = c->u;
    PyObject *name, *base;

    base = NULL;
    stack_size = PyList_GET_SIZE(c->c_stack);
    assert(stack_size >= 1);
    if (stack_size > 1) {
        int scope, force_global = 0;
        struct compiler_unit *parent;
        PyObject *mangled, *capsule;

        capsule = PyList_GET_ITEM(c->c_stack, stack_size - 1);
        parent = (struct compiler_unit *)PyCapsule_GetPointer(capsule, CAPSULE_NAME);
        assert(parent);
        if (parent->u_scope_type == COMPILER_SCOPE_TYPEPARAMS) {
            /* The parent is a type parameter scope, so we need to
               look at the grandparent. */
            if (stack_size == 2) {
                // If we're immediately within the module, we can skip
                // the rest and just set the qualname to be the same as name.
                u->u_metadata.u_qualname = Py_NewRef(u->u_metadata.u_name);
                return SUCCESS;
            }
            capsule = PyList_GET_ITEM(c->c_stack, stack_size - 2);
            parent = (struct compiler_unit *)PyCapsule_GetPointer(capsule, CAPSULE_NAME);
            assert(parent);
        }

        if (u->u_scope_type == COMPILER_SCOPE_FUNCTION
            || u->u_scope_type == COMPILER_SCOPE_ASYNC_FUNCTION
            || u->u_scope_type == COMPILER_SCOPE_CLASS) {
            assert(u->u_metadata.u_name);
            mangled = _Py_Mangle(parent->u_private, u->u_metadata.u_name);
            if (!mangled) {
                return ERROR;
            }

            scope = _PyST_GetScope(parent->u_ste, mangled);
            Py_DECREF(mangled);
            assert(scope != GLOBAL_IMPLICIT);
            if (scope == GLOBAL_EXPLICIT)
                force_global = 1;
        }

        if (!force_global) {
            if (parent->u_scope_type == COMPILER_SCOPE_FUNCTION
                || parent->u_scope_type == COMPILER_SCOPE_ASYNC_FUNCTION
                || parent->u_scope_type == COMPILER_SCOPE_LAMBDA)
            {
                _Py_DECLARE_STR(dot_locals, ".<locals>");
                base = PyUnicode_Concat(parent->u_metadata.u_qualname,
                                        &_Py_STR(dot_locals));
                if (base == NULL) {
                    return ERROR;
                }
            }
            else {
                base = Py_NewRef(parent->u_metadata.u_qualname);
            }
        }
    }

    if (base != NULL) {
        _Py_DECLARE_STR(dot, ".");
        name = PyUnicode_Concat(base, &_Py_STR(dot));
        Py_DECREF(base);
        if (name == NULL) {
            return ERROR;
        }
        PyUnicode_Append(&name, u->u_metadata.u_name);
        if (name == NULL) {
            return ERROR;
        }
    }
    else {
        name = Py_NewRef(u->u_metadata.u_name);
    }
    u->u_metadata.u_qualname = name;

    return SUCCESS;
}

/* Return the stack effect of opcode with argument oparg.

   Some opcodes have different stack effect when jump to the target and
   when not jump. The 'jump' parameter specifies the case:

   * 0 -- when not jump
   * 1 -- when jump
   * -1 -- maximal
 */
static int
stack_effect(int opcode, int oparg, int jump)
{
    if (0 <= opcode && opcode <= MAX_REAL_OPCODE) {
        if (_PyOpcode_Deopt[opcode] != opcode) {
            // Specialized instructions are not supported.
            return PY_INVALID_STACK_EFFECT;
        }
        int popped, pushed;
        if (jump > 0) {
            popped = _PyOpcode_num_popped(opcode, oparg, true);
            pushed = _PyOpcode_num_pushed(opcode, oparg, true);
        }
        else {
            popped = _PyOpcode_num_popped(opcode, oparg, false);
            pushed = _PyOpcode_num_pushed(opcode, oparg, false);
        }
        if (popped < 0 || pushed < 0) {
            return PY_INVALID_STACK_EFFECT;
        }
        if (jump >= 0) {
            return pushed - popped;
        }
        if (jump < 0) {
            // Compute max(pushed - popped, alt_pushed - alt_popped)
            int alt_popped = _PyOpcode_num_popped(opcode, oparg, true);
            int alt_pushed = _PyOpcode_num_pushed(opcode, oparg, true);
            if (alt_popped < 0 || alt_pushed < 0) {
                return PY_INVALID_STACK_EFFECT;
            }
            int diff = pushed - popped;
            int alt_diff = alt_pushed - alt_popped;
            if (alt_diff > diff) {
                return alt_diff;
            }
            return diff;
        }
    }

    // Pseudo ops
    switch (opcode) {
        case POP_BLOCK:
        case JUMP:
        case JUMP_NO_INTERRUPT:
            return 0;

        /* Exception handling pseudo-instructions */
        case SETUP_FINALLY:
            /* 0 in the normal flow.
             * Restore the stack position and push 1 value before jumping to
             * the handler if an exception be raised. */
            return jump ? 1 : 0;
        case SETUP_CLEANUP:
            /* As SETUP_FINALLY, but pushes lasti as well */
            return jump ? 2 : 0;
        case SETUP_WITH:
            /* 0 in the normal flow.
             * Restore the stack position to the position before the result
             * of __(a)enter__ and push 2 values before jumping to the handler
             * if an exception be raised. */
            return jump ? 1 : 0;

        case STORE_FAST_MAYBE_NULL:
            return -1;
        case LOAD_METHOD:
            return 1;
        case LOAD_SUPER_METHOD:
        case LOAD_ZERO_SUPER_METHOD:
        case LOAD_ZERO_SUPER_ATTR:
            return -1;
        default:
            return PY_INVALID_STACK_EFFECT;
    }

    return PY_INVALID_STACK_EFFECT; /* not reachable */
}

int
PyCompile_OpcodeStackEffectWithJump(int opcode, int oparg, int jump)
{
    return stack_effect(opcode, oparg, jump);
}

int
PyCompile_OpcodeStackEffect(int opcode, int oparg)
{
    return stack_effect(opcode, oparg, -1);
}

static int
codegen_addop_noarg(instr_sequence *seq, int opcode, location loc)
{
    assert(!HAS_ARG(opcode));
    assert(!IS_ASSEMBLER_OPCODE(opcode));
    return instr_sequence_addop(seq, opcode, 0, loc);
}

static Py_ssize_t
dict_add_o(PyObject *dict, PyObject *o)
{
    PyObject *v;
    Py_ssize_t arg;

    v = PyDict_GetItemWithError(dict, o);
    if (!v) {
        if (PyErr_Occurred()) {
            return ERROR;
        }
        arg = PyDict_GET_SIZE(dict);
        v = PyLong_FromSsize_t(arg);
        if (!v) {
            return ERROR;
        }
        if (PyDict_SetItem(dict, o, v) < 0) {
            Py_DECREF(v);
            return ERROR;
        }
        Py_DECREF(v);
    }
    else
        arg = PyLong_AsLong(v);
    return arg;
}

// Merge const *o* recursively and return constant key object.
static PyObject*
merge_consts_recursive(PyObject *const_cache, PyObject *o)
{
    assert(PyDict_CheckExact(const_cache));
    // None and Ellipsis are singleton, and key is the singleton.
    // No need to merge object and key.
    if (o == Py_None || o == Py_Ellipsis) {
        return Py_NewRef(o);
    }

    PyObject *key = _PyCode_ConstantKey(o);
    if (key == NULL) {
        return NULL;
    }

    // t is borrowed reference
    PyObject *t = PyDict_SetDefault(const_cache, key, key);
    if (t != key) {
        // o is registered in const_cache.  Just use it.
        Py_XINCREF(t);
        Py_DECREF(key);
        return t;
    }

    // We registered o in const_cache.
    // When o is a tuple or frozenset, we want to merge its
    // items too.
    if (PyTuple_CheckExact(o)) {
        Py_ssize_t len = PyTuple_GET_SIZE(o);
        for (Py_ssize_t i = 0; i < len; i++) {
            PyObject *item = PyTuple_GET_ITEM(o, i);
            PyObject *u = merge_consts_recursive(const_cache, item);
            if (u == NULL) {
                Py_DECREF(key);
                return NULL;
            }

            // See _PyCode_ConstantKey()
            PyObject *v;  // borrowed
            if (PyTuple_CheckExact(u)) {
                v = PyTuple_GET_ITEM(u, 1);
            }
            else {
                v = u;
            }
            if (v != item) {
                PyTuple_SET_ITEM(o, i, Py_NewRef(v));
                Py_DECREF(item);
            }

            Py_DECREF(u);
        }
    }
    else if (PyFrozenSet_CheckExact(o)) {
        // *key* is tuple. And its first item is frozenset of
        // constant keys.
        // See _PyCode_ConstantKey() for detail.
        assert(PyTuple_CheckExact(key));
        assert(PyTuple_GET_SIZE(key) == 2);

        Py_ssize_t len = PySet_GET_SIZE(o);
        if (len == 0) {  // empty frozenset should not be re-created.
            return key;
        }
        PyObject *tuple = PyTuple_New(len);
        if (tuple == NULL) {
            Py_DECREF(key);
            return NULL;
        }
        Py_ssize_t i = 0, pos = 0;
        PyObject *item;
        Py_hash_t hash;
        while (_PySet_NextEntry(o, &pos, &item, &hash)) {
            PyObject *k = merge_consts_recursive(const_cache, item);
            if (k == NULL) {
                Py_DECREF(tuple);
                Py_DECREF(key);
                return NULL;
            }
            PyObject *u;
            if (PyTuple_CheckExact(k)) {
                u = Py_NewRef(PyTuple_GET_ITEM(k, 1));
                Py_DECREF(k);
            }
            else {
                u = k;
            }
            PyTuple_SET_ITEM(tuple, i, u);  // Steals reference of u.
            i++;
        }

        // Instead of rewriting o, we create new frozenset and embed in the
        // key tuple.  Caller should get merged frozenset from the key tuple.
        PyObject *new = PyFrozenSet_New(tuple);
        Py_DECREF(tuple);
        if (new == NULL) {
            Py_DECREF(key);
            return NULL;
        }
        assert(PyTuple_GET_ITEM(key, 1) == o);
        Py_DECREF(o);
        PyTuple_SET_ITEM(key, 1, new);
    }

    return key;
}

static Py_ssize_t
compiler_add_const(PyObject *const_cache, struct compiler_unit *u, PyObject *o)
{
    assert(PyDict_CheckExact(const_cache));
    PyObject *key = merge_consts_recursive(const_cache, o);
    if (key == NULL) {
        return ERROR;
    }

    Py_ssize_t arg = dict_add_o(u->u_metadata.u_consts, key);
    Py_DECREF(key);
    return arg;
}

static int
compiler_addop_load_const(PyObject *const_cache, struct compiler_unit *u, location loc, PyObject *o)
{
    Py_ssize_t arg = compiler_add_const(const_cache, u, o);
    if (arg < 0) {
        return ERROR;
    }
    return codegen_addop_i(&u->u_instr_sequence, LOAD_CONST, arg, loc);
}

static int
compiler_addop_o(struct compiler_unit *u, location loc,
                 int opcode, PyObject *dict, PyObject *o)
{
    Py_ssize_t arg = dict_add_o(dict, o);
    if (arg < 0) {
        return ERROR;
    }
    return codegen_addop_i(&u->u_instr_sequence, opcode, arg, loc);
}

static int
compiler_addop_name(struct compiler_unit *u, location loc,
                    int opcode, PyObject *dict, PyObject *o)
{
    PyObject *mangled = _Py_Mangle(u->u_private, o);
    if (!mangled) {
        return ERROR;
    }
    Py_ssize_t arg = dict_add_o(dict, mangled);
    Py_DECREF(mangled);
    if (arg < 0) {
        return ERROR;
    }
    if (opcode == LOAD_ATTR) {
        arg <<= 1;
    }
    if (opcode == LOAD_METHOD) {
        opcode = LOAD_ATTR;
        arg <<= 1;
        arg |= 1;
    }
    if (opcode == LOAD_SUPER_ATTR) {
        arg <<= 2;
        arg |= 2;
    }
    if (opcode == LOAD_SUPER_METHOD) {
        opcode = LOAD_SUPER_ATTR;
        arg <<= 2;
        arg |= 3;
    }
    if (opcode == LOAD_ZERO_SUPER_ATTR) {
        opcode = LOAD_SUPER_ATTR;
        arg <<= 2;
    }
    if (opcode == LOAD_ZERO_SUPER_METHOD) {
        opcode = LOAD_SUPER_ATTR;
        arg <<= 2;
        arg |= 1;
    }
    return codegen_addop_i(&u->u_instr_sequence, opcode, arg, loc);
}

/* Add an opcode with an integer argument */
static int
codegen_addop_i(instr_sequence *seq, int opcode, Py_ssize_t oparg, location loc)
{
    /* oparg value is unsigned, but a signed C int is usually used to store
       it in the C code (like Python/ceval.c).

       Limit to 32-bit signed C int (rather than INT_MAX) for portability.

       The argument of a concrete bytecode instruction is limited to 8-bit.
       EXTENDED_ARG is used for 16, 24, and 32-bit arguments. */

    int oparg_ = Py_SAFE_DOWNCAST(oparg, Py_ssize_t, int);
    assert(!IS_ASSEMBLER_OPCODE(opcode));
    return instr_sequence_addop(seq, opcode, oparg_, loc);
}

static int
codegen_addop_j(instr_sequence *seq, location loc,
                int opcode, jump_target_label target)
{
    assert(IS_LABEL(target));
    assert(IS_JUMP_OPCODE(opcode) || IS_BLOCK_PUSH_OPCODE(opcode));
    assert(!IS_ASSEMBLER_OPCODE(opcode));
    return instr_sequence_addop(seq, opcode, target.id, loc);
}

#define RETURN_IF_ERROR_IN_SCOPE(C, CALL) { \
    if ((CALL) < 0) { \
        compiler_exit_scope((C)); \
        return ERROR; \
    } \
}

#define ADDOP(C, LOC, OP) \
    RETURN_IF_ERROR(codegen_addop_noarg(INSTR_SEQUENCE(C), (OP), (LOC)))

#define ADDOP_IN_SCOPE(C, LOC, OP) RETURN_IF_ERROR_IN_SCOPE((C), codegen_addop_noarg(INSTR_SEQUENCE(C), (OP), (LOC)))

#define ADDOP_LOAD_CONST(C, LOC, O) \
    RETURN_IF_ERROR(compiler_addop_load_const((C)->c_const_cache, (C)->u, (LOC), (O)))

/* Same as ADDOP_LOAD_CONST, but steals a reference. */
#define ADDOP_LOAD_CONST_NEW(C, LOC, O) { \
    PyObject *__new_const = (O); \
    if (__new_const == NULL) { \
        return ERROR; \
    } \
    if (compiler_addop_load_const((C)->c_const_cache, (C)->u, (LOC), __new_const) < 0) { \
        Py_DECREF(__new_const); \
        return ERROR; \
    } \
    Py_DECREF(__new_const); \
}

#define ADDOP_N(C, LOC, OP, O, TYPE) { \
    assert(!HAS_CONST(OP)); /* use ADDOP_LOAD_CONST_NEW */ \
    if (compiler_addop_o((C)->u, (LOC), (OP), (C)->u->u_metadata.u_ ## TYPE, (O)) < 0) { \
        Py_DECREF((O)); \
        return ERROR; \
    } \
    Py_DECREF((O)); \
}

#define ADDOP_NAME(C, LOC, OP, O, TYPE) \
    RETURN_IF_ERROR(compiler_addop_name((C)->u, (LOC), (OP), (C)->u->u_metadata.u_ ## TYPE, (O)))

#define ADDOP_I(C, LOC, OP, O) \
    RETURN_IF_ERROR(codegen_addop_i(INSTR_SEQUENCE(C), (OP), (O), (LOC)))

#define ADDOP_JUMP(C, LOC, OP, O) \
    RETURN_IF_ERROR(codegen_addop_j(INSTR_SEQUENCE(C), (LOC), (OP), (O)))

#define ADDOP_COMPARE(C, LOC, CMP) \
    RETURN_IF_ERROR(compiler_addcompare((C), (LOC), (cmpop_ty)(CMP)))

#define ADDOP_BINARY(C, LOC, BINOP) \
    RETURN_IF_ERROR(addop_binary((C), (LOC), (BINOP), false))

#define ADDOP_INPLACE(C, LOC, BINOP) \
    RETURN_IF_ERROR(addop_binary((C), (LOC), (BINOP), true))

#define ADD_YIELD_FROM(C, LOC, await) \
    RETURN_IF_ERROR(compiler_add_yield_from((C), (LOC), (await)))

#define POP_EXCEPT_AND_RERAISE(C, LOC) \
    RETURN_IF_ERROR(compiler_pop_except_and_reraise((C), (LOC)))

#define ADDOP_YIELD(C, LOC) \
    RETURN_IF_ERROR(addop_yield((C), (LOC)))

/* VISIT and VISIT_SEQ takes an ASDL type as their second argument.  They use
   the ASDL name to synthesize the name of the C type and the visit function.
*/

#define VISIT(C, TYPE, V) \
    RETURN_IF_ERROR(compiler_visit_ ## TYPE((C), (V)));

#define VISIT_IN_SCOPE(C, TYPE, V) \
    RETURN_IF_ERROR_IN_SCOPE((C), compiler_visit_ ## TYPE((C), (V)))

#define VISIT_SEQ(C, TYPE, SEQ) { \
    int _i; \
    asdl_ ## TYPE ## _seq *seq = (SEQ); /* avoid variable capture */ \
    for (_i = 0; _i < asdl_seq_LEN(seq); _i++) { \
        TYPE ## _ty elt = (TYPE ## _ty)asdl_seq_GET(seq, _i); \
        RETURN_IF_ERROR(compiler_visit_ ## TYPE((C), elt)); \
    } \
}

#define VISIT_SEQ_IN_SCOPE(C, TYPE, SEQ) { \
    int _i; \
    asdl_ ## TYPE ## _seq *seq = (SEQ); /* avoid variable capture */ \
    for (_i = 0; _i < asdl_seq_LEN(seq); _i++) { \
        TYPE ## _ty elt = (TYPE ## _ty)asdl_seq_GET(seq, _i); \
        if (compiler_visit_ ## TYPE((C), elt) < 0) { \
            compiler_exit_scope(C); \
            return ERROR; \
        } \
    } \
}


static int
compiler_enter_scope(struct compiler *c, identifier name,
                     int scope_type, void *key, int lineno)
{
    location loc = LOCATION(lineno, lineno, 0, 0);

    struct compiler_unit *u;

    u = (struct compiler_unit *)PyObject_Calloc(1, sizeof(
                                            struct compiler_unit));
    if (!u) {
        PyErr_NoMemory();
        return ERROR;
    }
    u->u_scope_type = scope_type;
    u->u_metadata.u_argcount = 0;
    u->u_metadata.u_posonlyargcount = 0;
    u->u_metadata.u_kwonlyargcount = 0;
    u->u_ste = PySymtable_Lookup(c->c_st, key);
    if (!u->u_ste) {
        compiler_unit_free(u);
        return ERROR;
    }
    u->u_metadata.u_name = Py_NewRef(name);
    u->u_metadata.u_varnames = list2dict(u->u_ste->ste_varnames);
    if (!u->u_metadata.u_varnames) {
        compiler_unit_free(u);
        return ERROR;
    }
    u->u_metadata.u_cellvars = dictbytype(u->u_ste->ste_symbols, CELL, DEF_COMP_CELL, 0);
    if (!u->u_metadata.u_cellvars) {
        compiler_unit_free(u);
        return ERROR;
    }
    if (u->u_ste->ste_needs_class_closure) {
        /* Cook up an implicit __class__ cell. */
        Py_ssize_t res;
        assert(u->u_scope_type == COMPILER_SCOPE_CLASS);
        res = dict_add_o(u->u_metadata.u_cellvars, &_Py_ID(__class__));
        if (res < 0) {
            compiler_unit_free(u);
            return ERROR;
        }
    }
    if (u->u_ste->ste_needs_classdict) {
        /* Cook up an implicit __classdict__ cell. */
        Py_ssize_t res;
        assert(u->u_scope_type == COMPILER_SCOPE_CLASS);
        res = dict_add_o(u->u_metadata.u_cellvars, &_Py_ID(__classdict__));
        if (res < 0) {
            compiler_unit_free(u);
            return ERROR;
        }
    }

    u->u_metadata.u_freevars = dictbytype(u->u_ste->ste_symbols, FREE, DEF_FREE_CLASS,
                               PyDict_GET_SIZE(u->u_metadata.u_cellvars));
    if (!u->u_metadata.u_freevars) {
        compiler_unit_free(u);
        return ERROR;
    }

    u->u_metadata.u_fasthidden = PyDict_New();
    if (!u->u_metadata.u_fasthidden) {
        compiler_unit_free(u);
        return ERROR;
    }

    u->u_nfblocks = 0;
    u->u_in_inlined_comp = 0;
    u->u_metadata.u_firstlineno = lineno;
    u->u_metadata.u_consts = PyDict_New();
    if (!u->u_metadata.u_consts) {
        compiler_unit_free(u);
        return ERROR;
    }
    u->u_metadata.u_names = PyDict_New();
    if (!u->u_metadata.u_names) {
        compiler_unit_free(u);
        return ERROR;
    }

    u->u_private = NULL;

    /* Push the old compiler_unit on the stack. */
    if (c->u) {
        PyObject *capsule = PyCapsule_New(c->u, CAPSULE_NAME, NULL);
        if (!capsule || PyList_Append(c->c_stack, capsule) < 0) {
            Py_XDECREF(capsule);
            compiler_unit_free(u);
            return ERROR;
        }
        Py_DECREF(capsule);
        u->u_private = Py_XNewRef(c->u->u_private);
    }
    c->u = u;

    c->c_nestlevel++;

    if (u->u_scope_type == COMPILER_SCOPE_MODULE) {
        loc.lineno = 0;
    }
    else {
        RETURN_IF_ERROR(compiler_set_qualname(c));
    }
    ADDOP_I(c, loc, RESUME, 0);

    if (u->u_scope_type == COMPILER_SCOPE_MODULE) {
        loc.lineno = -1;
    }
    return SUCCESS;
}

static void
compiler_exit_scope(struct compiler *c)
{
    // Don't call PySequence_DelItem() with an exception raised
    PyObject *exc = PyErr_GetRaisedException();

    c->c_nestlevel--;
    compiler_unit_free(c->u);
    /* Restore c->u to the parent unit. */
    Py_ssize_t n = PyList_GET_SIZE(c->c_stack) - 1;
    if (n >= 0) {
        PyObject *capsule = PyList_GET_ITEM(c->c_stack, n);
        c->u = (struct compiler_unit *)PyCapsule_GetPointer(capsule, CAPSULE_NAME);
        assert(c->u);
        /* we are deleting from a list so this really shouldn't fail */
        if (PySequence_DelItem(c->c_stack, n) < 0) {
            _PyErr_WriteUnraisableMsg("on removing the last compiler "
                                      "stack item", NULL);
        }
    }
    else {
        c->u = NULL;
    }

    PyErr_SetRaisedException(exc);
}

/* Search if variable annotations are present statically in a block. */

static bool
find_ann(asdl_stmt_seq *stmts)
{
    int i, j, res = 0;
    stmt_ty st;

    for (i = 0; i < asdl_seq_LEN(stmts); i++) {
        st = (stmt_ty)asdl_seq_GET(stmts, i);
        switch (st->kind) {
        case AnnAssign_kind:
            return true;
        case For_kind:
            res = find_ann(st->v.For.body) ||
                  find_ann(st->v.For.orelse);
            break;
        case AsyncFor_kind:
            res = find_ann(st->v.AsyncFor.body) ||
                  find_ann(st->v.AsyncFor.orelse);
            break;
        case While_kind:
            res = find_ann(st->v.While.body) ||
                  find_ann(st->v.While.orelse);
            break;
        case If_kind:
            res = find_ann(st->v.If.body) ||
                  find_ann(st->v.If.orelse);
            break;
        case With_kind:
            res = find_ann(st->v.With.body);
            break;
        case AsyncWith_kind:
            res = find_ann(st->v.AsyncWith.body);
            break;
        case Try_kind:
            for (j = 0; j < asdl_seq_LEN(st->v.Try.handlers); j++) {
                excepthandler_ty handler = (excepthandler_ty)asdl_seq_GET(
                    st->v.Try.handlers, j);
                if (find_ann(handler->v.ExceptHandler.body)) {
                    return true;
                }
            }
            res = find_ann(st->v.Try.body) ||
                  find_ann(st->v.Try.finalbody) ||
                  find_ann(st->v.Try.orelse);
            break;
        case TryStar_kind:
            for (j = 0; j < asdl_seq_LEN(st->v.TryStar.handlers); j++) {
                excepthandler_ty handler = (excepthandler_ty)asdl_seq_GET(
                    st->v.TryStar.handlers, j);
                if (find_ann(handler->v.ExceptHandler.body)) {
                    return true;
                }
            }
            res = find_ann(st->v.TryStar.body) ||
                  find_ann(st->v.TryStar.finalbody) ||
                  find_ann(st->v.TryStar.orelse);
            break;
        case Match_kind:
            for (j = 0; j < asdl_seq_LEN(st->v.Match.cases); j++) {
                match_case_ty match_case = (match_case_ty)asdl_seq_GET(
                    st->v.Match.cases, j);
                if (find_ann(match_case->body)) {
                    return true;
                }
            }
            break;
        default:
            res = false;
            break;
        }
        if (res) {
            break;
        }
    }
    return res;
}

/*
 * Frame block handling functions
 */

static int
compiler_push_fblock(struct compiler *c, location loc,
                     enum fblocktype t, jump_target_label block_label,
                     jump_target_label exit, void *datum)
{
    struct fblockinfo *f;
    if (c->u->u_nfblocks >= CO_MAXBLOCKS) {
        return compiler_error(c, loc, "too many statically nested blocks");
    }
    f = &c->u->u_fblock[c->u->u_nfblocks++];
    f->fb_type = t;
    f->fb_block = block_label;
    f->fb_exit = exit;
    f->fb_datum = datum;
    return SUCCESS;
}

static void
compiler_pop_fblock(struct compiler *c, enum fblocktype t, jump_target_label block_label)
{
    struct compiler_unit *u = c->u;
    assert(u->u_nfblocks > 0);
    u->u_nfblocks--;
    assert(u->u_fblock[u->u_nfblocks].fb_type == t);
    assert(SAME_LABEL(u->u_fblock[u->u_nfblocks].fb_block, block_label));
}

static int
compiler_call_exit_with_nones(struct compiler *c, location loc)
{
    ADDOP_LOAD_CONST(c, loc, Py_None);
    ADDOP_LOAD_CONST(c, loc, Py_None);
    ADDOP_LOAD_CONST(c, loc, Py_None);
    ADDOP_I(c, loc, CALL, 2);
    return SUCCESS;
}

static int
compiler_add_yield_from(struct compiler *c, location loc, int await)
{
    NEW_JUMP_TARGET_LABEL(c, send);
    NEW_JUMP_TARGET_LABEL(c, fail);
    NEW_JUMP_TARGET_LABEL(c, exit);

    USE_LABEL(c, send);
    ADDOP_JUMP(c, loc, SEND, exit);
    // Set up a virtual try/except to handle when StopIteration is raised during
    // a close or throw call. The only way YIELD_VALUE raises if they do!
    ADDOP_JUMP(c, loc, SETUP_FINALLY, fail);
    ADDOP_I(c, loc, YIELD_VALUE, 0);
    ADDOP(c, NO_LOCATION, POP_BLOCK);
    ADDOP_I(c, loc, RESUME, await ? 3 : 2);
    ADDOP_JUMP(c, loc, JUMP_NO_INTERRUPT, send);

    USE_LABEL(c, fail);
    ADDOP(c, loc, CLEANUP_THROW);

    USE_LABEL(c, exit);
    ADDOP(c, loc, END_SEND);
    return SUCCESS;
}

static int
compiler_pop_except_and_reraise(struct compiler *c, location loc)
{
    /* Stack contents
     * [exc_info, lasti, exc]            COPY        3
     * [exc_info, lasti, exc, exc_info]  POP_EXCEPT
     * [exc_info, lasti, exc]            RERAISE      1
     * (exception_unwind clears the stack)
     */

    ADDOP_I(c, loc, COPY, 3);
    ADDOP(c, loc, POP_EXCEPT);
    ADDOP_I(c, loc, RERAISE, 1);
    return SUCCESS;
}

/* Unwind a frame block.  If preserve_tos is true, the TOS before
 * popping the blocks will be restored afterwards, unless another
 * return, break or continue is found. In which case, the TOS will
 * be popped.
 */
static int
compiler_unwind_fblock(struct compiler *c, location *ploc,
                       struct fblockinfo *info, int preserve_tos)
{
    switch (info->fb_type) {
        case WHILE_LOOP:
        case EXCEPTION_HANDLER:
        case EXCEPTION_GROUP_HANDLER:
        case ASYNC_COMPREHENSION_GENERATOR:
            return SUCCESS;

        case FOR_LOOP:
            /* Pop the iterator */
            if (preserve_tos) {
                ADDOP_I(c, *ploc, SWAP, 2);
            }
            ADDOP(c, *ploc, POP_TOP);
            return SUCCESS;

        case TRY_EXCEPT:
            ADDOP(c, *ploc, POP_BLOCK);
            return SUCCESS;

        case FINALLY_TRY:
            /* This POP_BLOCK gets the line number of the unwinding statement */
            ADDOP(c, *ploc, POP_BLOCK);
            if (preserve_tos) {
                RETURN_IF_ERROR(
                    compiler_push_fblock(c, *ploc, POP_VALUE, NO_LABEL, NO_LABEL, NULL));
            }
            /* Emit the finally block */
            VISIT_SEQ(c, stmt, info->fb_datum);
            if (preserve_tos) {
                compiler_pop_fblock(c, POP_VALUE, NO_LABEL);
            }
            /* The finally block should appear to execute after the
             * statement causing the unwinding, so make the unwinding
             * instruction artificial */
            *ploc = NO_LOCATION;
            return SUCCESS;

        case FINALLY_END:
            if (preserve_tos) {
                ADDOP_I(c, *ploc, SWAP, 2);
            }
            ADDOP(c, *ploc, POP_TOP); /* exc_value */
            if (preserve_tos) {
                ADDOP_I(c, *ploc, SWAP, 2);
            }
            ADDOP(c, *ploc, POP_BLOCK);
            ADDOP(c, *ploc, POP_EXCEPT);
            return SUCCESS;

        case WITH:
        case ASYNC_WITH:
            *ploc = LOC((stmt_ty)info->fb_datum);
            ADDOP(c, *ploc, POP_BLOCK);
            if (preserve_tos) {
                ADDOP_I(c, *ploc, SWAP, 2);
            }
            RETURN_IF_ERROR(compiler_call_exit_with_nones(c, *ploc));
            if (info->fb_type == ASYNC_WITH) {
                ADDOP_I(c, *ploc, GET_AWAITABLE, 2);
                ADDOP_LOAD_CONST(c, *ploc, Py_None);
                ADD_YIELD_FROM(c, *ploc, 1);
            }
            ADDOP(c, *ploc, POP_TOP);
            /* The exit block should appear to execute after the
             * statement causing the unwinding, so make the unwinding
             * instruction artificial */
            *ploc = NO_LOCATION;
            return SUCCESS;

        case HANDLER_CLEANUP: {
            if (info->fb_datum) {
                ADDOP(c, *ploc, POP_BLOCK);
            }
            if (preserve_tos) {
                ADDOP_I(c, *ploc, SWAP, 2);
            }
            ADDOP(c, *ploc, POP_BLOCK);
            ADDOP(c, *ploc, POP_EXCEPT);
            if (info->fb_datum) {
                ADDOP_LOAD_CONST(c, *ploc, Py_None);
                RETURN_IF_ERROR(compiler_nameop(c, *ploc, info->fb_datum, Store));
                RETURN_IF_ERROR(compiler_nameop(c, *ploc, info->fb_datum, Del));
            }
            return SUCCESS;
        }
        case POP_VALUE: {
            if (preserve_tos) {
                ADDOP_I(c, *ploc, SWAP, 2);
            }
            ADDOP(c, *ploc, POP_TOP);
            return SUCCESS;
        }
    }
    Py_UNREACHABLE();
}

/** Unwind block stack. If loop is not NULL, then stop when the first loop is encountered. */
static int
compiler_unwind_fblock_stack(struct compiler *c, location *ploc,
                             int preserve_tos, struct fblockinfo **loop)
{
    if (c->u->u_nfblocks == 0) {
        return SUCCESS;
    }
    struct fblockinfo *top = &c->u->u_fblock[c->u->u_nfblocks-1];
    if (top->fb_type == EXCEPTION_GROUP_HANDLER) {
        return compiler_error(
            c, *ploc, "'break', 'continue' and 'return' cannot appear in an except* block");
    }
    if (loop != NULL && (top->fb_type == WHILE_LOOP || top->fb_type == FOR_LOOP)) {
        *loop = top;
        return SUCCESS;
    }
    struct fblockinfo copy = *top;
    c->u->u_nfblocks--;
    RETURN_IF_ERROR(compiler_unwind_fblock(c, ploc, &copy, preserve_tos));
    RETURN_IF_ERROR(compiler_unwind_fblock_stack(c, ploc, preserve_tos, loop));
    c->u->u_fblock[c->u->u_nfblocks] = copy;
    c->u->u_nfblocks++;
    return SUCCESS;
}

/* Compile a sequence of statements, checking for a docstring
   and for annotations. */

static int
compiler_body(struct compiler *c, location loc, asdl_stmt_seq *stmts)
{
    int i = 0;
    stmt_ty st;
    PyObject *docstring;

    /* Set current line number to the line number of first statement.
       This way line number for SETUP_ANNOTATIONS will always
       coincide with the line number of first "real" statement in module.
       If body is empty, then lineno will be set later in optimize_and_assemble. */
    if (c->u->u_scope_type == COMPILER_SCOPE_MODULE && asdl_seq_LEN(stmts)) {
        st = (stmt_ty)asdl_seq_GET(stmts, 0);
        loc = LOC(st);
    }
    /* Every annotated class and module should have __annotations__. */
    if (find_ann(stmts)) {
        ADDOP(c, loc, SETUP_ANNOTATIONS);
    }
    if (!asdl_seq_LEN(stmts)) {
        return SUCCESS;
    }
    /* if not -OO mode, set docstring */
    if (c->c_optimize < 2) {
        docstring = _PyAST_GetDocString(stmts);
        if (docstring) {
            i = 1;
            st = (stmt_ty)asdl_seq_GET(stmts, 0);
            assert(st->kind == Expr_kind);
            VISIT(c, expr, st->v.Expr.value);
            RETURN_IF_ERROR(compiler_nameop(c, NO_LOCATION, &_Py_ID(__doc__), Store));
        }
    }
    for (; i < asdl_seq_LEN(stmts); i++) {
        VISIT(c, stmt, (stmt_ty)asdl_seq_GET(stmts, i));
    }
    return SUCCESS;
}

static int
compiler_codegen(struct compiler *c, mod_ty mod)
{
    _Py_DECLARE_STR(anon_module, "<module>");
    RETURN_IF_ERROR(
        compiler_enter_scope(c, &_Py_STR(anon_module), COMPILER_SCOPE_MODULE,
                             mod, 1));

    location loc = LOCATION(1, 1, 0, 0);
    switch (mod->kind) {
    case Module_kind:
        if (compiler_body(c, loc, mod->v.Module.body) < 0) {
            compiler_exit_scope(c);
            return ERROR;
        }
        break;
    case Interactive_kind:
        if (find_ann(mod->v.Interactive.body)) {
            ADDOP(c, loc, SETUP_ANNOTATIONS);
        }
        c->c_interactive = 1;
        VISIT_SEQ_IN_SCOPE(c, stmt, mod->v.Interactive.body);
        break;
    case Expression_kind:
        VISIT_IN_SCOPE(c, expr, mod->v.Expression.body);
        break;
    default:
        PyErr_Format(PyExc_SystemError,
                     "module kind %d should not be possible",
                     mod->kind);
        return ERROR;
    }
    return SUCCESS;
}

static PyCodeObject *
compiler_mod(struct compiler *c, mod_ty mod)
{
    int addNone = mod->kind != Expression_kind;
    if (compiler_codegen(c, mod) < 0) {
        return NULL;
    }
    PyCodeObject *co = optimize_and_assemble(c, addNone);
    compiler_exit_scope(c);
    return co;
}

/* The test for LOCAL must come before the test for FREE in order to
   handle classes where name is both local and free.  The local var is
   a method and the free var is a free var referenced within a method.
*/

static int
get_ref_type(struct compiler *c, PyObject *name)
{
    int scope;
    if (c->u->u_scope_type == COMPILER_SCOPE_CLASS &&
        (_PyUnicode_EqualToASCIIString(name, "__class__") ||
         _PyUnicode_EqualToASCIIString(name, "__classdict__"))) {
        return CELL;
    }
    scope = _PyST_GetScope(c->u->u_ste, name);
    if (scope == 0) {
        PyErr_Format(PyExc_SystemError,
                     "_PyST_GetScope(name=%R) failed: "
                     "unknown scope in unit %S (%R); "
                     "symbols: %R; locals: %R; globals: %R",
                     name,
                     c->u->u_metadata.u_name, c->u->u_ste->ste_id,
                     c->u->u_ste->ste_symbols, c->u->u_metadata.u_varnames, c->u->u_metadata.u_names);
        return ERROR;
    }
    return scope;
}

static int
compiler_lookup_arg(PyObject *dict, PyObject *name)
{
    PyObject *v = PyDict_GetItemWithError(dict, name);
    if (v == NULL) {
        return ERROR;
    }
    return PyLong_AS_LONG(v);
}

static int
compiler_make_closure(struct compiler *c, location loc,
                      PyCodeObject *co, Py_ssize_t flags)
{
    if (co->co_nfreevars) {
        int i = PyCode_GetFirstFree(co);
        for (; i < co->co_nlocalsplus; ++i) {
            /* Bypass com_addop_varname because it will generate
               LOAD_DEREF but LOAD_CLOSURE is needed.
            */
            PyObject *name = PyTuple_GET_ITEM(co->co_localsplusnames, i);

            /* Special case: If a class contains a method with a
               free variable that has the same name as a method,
               the name will be considered free *and* local in the
               class.  It should be handled by the closure, as
               well as by the normal name lookup logic.
            */
            int reftype = get_ref_type(c, name);
            if (reftype == -1) {
                return ERROR;
            }
            int arg;
            if (reftype == CELL) {
                arg = compiler_lookup_arg(c->u->u_metadata.u_cellvars, name);
            }
            else {
                arg = compiler_lookup_arg(c->u->u_metadata.u_freevars, name);
            }
            if (arg == -1) {
                PyObject *freevars = _PyCode_GetFreevars(co);
                if (freevars == NULL) {
                    PyErr_Clear();
                }
                PyErr_Format(PyExc_SystemError,
                    "compiler_lookup_arg(name=%R) with reftype=%d failed in %S; "
                    "freevars of code %S: %R",
                    name,
                    reftype,
                    c->u->u_metadata.u_name,
                    co->co_name,
                    freevars);
                Py_DECREF(freevars);
                return ERROR;
            }
            ADDOP_I(c, loc, LOAD_CLOSURE, arg);
        }
        flags |= 0x08;
        ADDOP_I(c, loc, BUILD_TUPLE, co->co_nfreevars);
    }
    ADDOP_LOAD_CONST(c, loc, (PyObject*)co);
    ADDOP_I(c, loc, MAKE_FUNCTION, flags);
    return SUCCESS;
}

static int
compiler_decorators(struct compiler *c, asdl_expr_seq* decos)
{
    if (!decos) {
        return SUCCESS;
    }

    for (Py_ssize_t i = 0; i < asdl_seq_LEN(decos); i++) {
        VISIT(c, expr, (expr_ty)asdl_seq_GET(decos, i));
    }
    return SUCCESS;
}

static int
compiler_apply_decorators(struct compiler *c, asdl_expr_seq* decos)
{
    if (!decos) {
        return SUCCESS;
    }

    for (Py_ssize_t i = asdl_seq_LEN(decos) - 1; i > -1; i--) {
        location loc = LOC((expr_ty)asdl_seq_GET(decos, i));
        ADDOP_I(c, loc, CALL, 0);
    }
    return SUCCESS;
}

static int
compiler_visit_kwonlydefaults(struct compiler *c, location loc,
                              asdl_arg_seq *kwonlyargs, asdl_expr_seq *kw_defaults)
{
    /* Push a dict of keyword-only default values.

       Return -1 on error, 0 if no dict pushed, 1 if a dict is pushed.
       */
    int i;
    PyObject *keys = NULL;

    for (i = 0; i < asdl_seq_LEN(kwonlyargs); i++) {
        arg_ty arg = asdl_seq_GET(kwonlyargs, i);
        expr_ty default_ = asdl_seq_GET(kw_defaults, i);
        if (default_) {
            PyObject *mangled = _Py_Mangle(c->u->u_private, arg->arg);
            if (!mangled) {
                goto error;
            }
            if (keys == NULL) {
                keys = PyList_New(1);
                if (keys == NULL) {
                    Py_DECREF(mangled);
                    return ERROR;
                }
                PyList_SET_ITEM(keys, 0, mangled);
            }
            else {
                int res = PyList_Append(keys, mangled);
                Py_DECREF(mangled);
                if (res == -1) {
                    goto error;
                }
            }
            if (compiler_visit_expr(c, default_) < 0) {
                goto error;
            }
        }
    }
    if (keys != NULL) {
        Py_ssize_t default_count = PyList_GET_SIZE(keys);
        PyObject *keys_tuple = PyList_AsTuple(keys);
        Py_DECREF(keys);
        ADDOP_LOAD_CONST_NEW(c, loc, keys_tuple);
        ADDOP_I(c, loc, BUILD_CONST_KEY_MAP, default_count);
        assert(default_count > 0);
        return 1;
    }
    else {
        return 0;
    }

error:
    Py_XDECREF(keys);
    return ERROR;
}

static int
compiler_visit_annexpr(struct compiler *c, expr_ty annotation)
{
    location loc = LOC(annotation);
    ADDOP_LOAD_CONST_NEW(c, loc, _PyAST_ExprAsUnicode(annotation));
    return SUCCESS;
}

static int
compiler_visit_argannotation(struct compiler *c, identifier id,
    expr_ty annotation, Py_ssize_t *annotations_len, location loc)
{
    if (!annotation) {
        return SUCCESS;
    }
    PyObject *mangled = _Py_Mangle(c->u->u_private, id);
    if (!mangled) {
        return ERROR;
    }
    ADDOP_LOAD_CONST(c, loc, mangled);
    Py_DECREF(mangled);

    if (c->c_future.ff_features & CO_FUTURE_ANNOTATIONS) {
        VISIT(c, annexpr, annotation);
    }
    else {
        if (annotation->kind == Starred_kind) {
            // *args: *Ts (where Ts is a TypeVarTuple).
            // Do [annotation_value] = [*Ts].
            // (Note that in theory we could end up here even for an argument
            // other than *args, but in practice the grammar doesn't allow it.)
            VISIT(c, expr, annotation->v.Starred.value);
            ADDOP_I(c, loc, UNPACK_SEQUENCE, (Py_ssize_t) 1);
        }
        else {
            VISIT(c, expr, annotation);
        }
    }
    *annotations_len += 2;
    return SUCCESS;
}

static int
compiler_visit_argannotations(struct compiler *c, asdl_arg_seq* args,
                              Py_ssize_t *annotations_len, location loc)
{
    int i;
    for (i = 0; i < asdl_seq_LEN(args); i++) {
        arg_ty arg = (arg_ty)asdl_seq_GET(args, i);
        RETURN_IF_ERROR(
            compiler_visit_argannotation(
                        c,
                        arg->arg,
                        arg->annotation,
                        annotations_len,
                        loc));
    }
    return SUCCESS;
}

static int
compiler_visit_annotations(struct compiler *c, location loc,
                           arguments_ty args, expr_ty returns)
{
    /* Push arg annotation names and values.
       The expressions are evaluated out-of-order wrt the source code.

       Return -1 on error, 0 if no annotations pushed, 1 if a annotations is pushed.
       */
    Py_ssize_t annotations_len = 0;

    RETURN_IF_ERROR(
        compiler_visit_argannotations(c, args->args, &annotations_len, loc));

    RETURN_IF_ERROR(
        compiler_visit_argannotations(c, args->posonlyargs, &annotations_len, loc));

    if (args->vararg && args->vararg->annotation) {
        RETURN_IF_ERROR(
            compiler_visit_argannotation(c, args->vararg->arg,
                                         args->vararg->annotation, &annotations_len, loc));
    }

    RETURN_IF_ERROR(
        compiler_visit_argannotations(c, args->kwonlyargs, &annotations_len, loc));

    if (args->kwarg && args->kwarg->annotation) {
        RETURN_IF_ERROR(
            compiler_visit_argannotation(c, args->kwarg->arg,
                                         args->kwarg->annotation, &annotations_len, loc));
    }

    RETURN_IF_ERROR(
        compiler_visit_argannotation(c, &_Py_ID(return), returns, &annotations_len, loc));

    if (annotations_len) {
        ADDOP_I(c, loc, BUILD_TUPLE, annotations_len);
        return 1;
    }

    return 0;
}

static int
compiler_visit_defaults(struct compiler *c, arguments_ty args,
                        location loc)
{
    VISIT_SEQ(c, expr, args->defaults);
    ADDOP_I(c, loc, BUILD_TUPLE, asdl_seq_LEN(args->defaults));
    return SUCCESS;
}

static Py_ssize_t
compiler_default_arguments(struct compiler *c, location loc,
                           arguments_ty args)
{
    Py_ssize_t funcflags = 0;
    if (args->defaults && asdl_seq_LEN(args->defaults) > 0) {
        RETURN_IF_ERROR(compiler_visit_defaults(c, args, loc));
        funcflags |= 0x01;
    }
    if (args->kwonlyargs) {
        int res = compiler_visit_kwonlydefaults(c, loc,
                                                args->kwonlyargs,
                                                args->kw_defaults);
        RETURN_IF_ERROR(res);
        if (res > 0) {
            funcflags |= 0x02;
        }
    }
    return funcflags;
}

static bool
forbidden_name(struct compiler *c, location loc, identifier name,
               expr_context_ty ctx)
{
    if (ctx == Store && _PyUnicode_EqualToASCIIString(name, "__debug__")) {
        compiler_error(c, loc, "cannot assign to __debug__");
        return true;
    }
    if (ctx == Del && _PyUnicode_EqualToASCIIString(name, "__debug__")) {
        compiler_error(c, loc, "cannot delete __debug__");
        return true;
    }
    return false;
}

static int
compiler_check_debug_one_arg(struct compiler *c, arg_ty arg)
{
    if (arg != NULL) {
        if (forbidden_name(c, LOC(arg), arg->arg, Store)) {
            return ERROR;
        }
    }
    return SUCCESS;
}

static int
compiler_check_debug_args_seq(struct compiler *c, asdl_arg_seq *args)
{
    if (args != NULL) {
        for (Py_ssize_t i = 0, n = asdl_seq_LEN(args); i < n; i++) {
            RETURN_IF_ERROR(
                compiler_check_debug_one_arg(c, asdl_seq_GET(args, i)));
        }
    }
    return SUCCESS;
}

static int
compiler_check_debug_args(struct compiler *c, arguments_ty args)
{
    RETURN_IF_ERROR(compiler_check_debug_args_seq(c, args->posonlyargs));
    RETURN_IF_ERROR(compiler_check_debug_args_seq(c, args->args));
    RETURN_IF_ERROR(compiler_check_debug_one_arg(c, args->vararg));
    RETURN_IF_ERROR(compiler_check_debug_args_seq(c, args->kwonlyargs));
    RETURN_IF_ERROR(compiler_check_debug_one_arg(c, args->kwarg));
    return SUCCESS;
}

static int
wrap_in_stopiteration_handler(struct compiler *c)
{
    NEW_JUMP_TARGET_LABEL(c, handler);

    /* Insert SETUP_CLEANUP at start */
    RETURN_IF_ERROR(
        instr_sequence_insert_instruction(
            INSTR_SEQUENCE(c), 0,
            SETUP_CLEANUP, handler.id, NO_LOCATION));

    ADDOP_LOAD_CONST(c, NO_LOCATION, Py_None);
    ADDOP(c, NO_LOCATION, RETURN_VALUE);
    USE_LABEL(c, handler);
    ADDOP_I(c, NO_LOCATION, CALL_INTRINSIC_1, INTRINSIC_STOPITERATION_ERROR);
    ADDOP_I(c, NO_LOCATION, RERAISE, 1);
    return SUCCESS;
}

static int
compiler_type_params(struct compiler *c, asdl_type_param_seq *type_params)
{
    if (!type_params) {
        return SUCCESS;
    }
    Py_ssize_t n = asdl_seq_LEN(type_params);

    for (Py_ssize_t i = 0; i < n; i++) {
        type_param_ty typeparam = asdl_seq_GET(type_params, i);
        location loc = LOC(typeparam);
        switch(typeparam->kind) {
        case TypeVar_kind:
            ADDOP_LOAD_CONST(c, loc, typeparam->v.TypeVar.name);
            if (typeparam->v.TypeVar.bound) {
                expr_ty bound = typeparam->v.TypeVar.bound;
                if (compiler_enter_scope(c, typeparam->v.TypeVar.name, COMPILER_SCOPE_TYPEPARAMS,
                                        (void *)typeparam, bound->lineno) == -1) {
                    return ERROR;
                }
                VISIT_IN_SCOPE(c, expr, bound);
                ADDOP_IN_SCOPE(c, loc, RETURN_VALUE);
                PyCodeObject *co = optimize_and_assemble(c, 1);
                compiler_exit_scope(c);
                if (co == NULL) {
                    return ERROR;
                }
                if (compiler_make_closure(c, loc, co, 0) < 0) {
                    Py_DECREF(co);
                    return ERROR;
                }
                Py_DECREF(co);

                int intrinsic = bound->kind == Tuple_kind
                    ? INTRINSIC_TYPEVAR_WITH_CONSTRAINTS
                    : INTRINSIC_TYPEVAR_WITH_BOUND;
                ADDOP_I(c, loc, CALL_INTRINSIC_2, intrinsic);
            }
            else {
                ADDOP_I(c, loc, CALL_INTRINSIC_1, INTRINSIC_TYPEVAR);
            }
            ADDOP_I(c, loc, COPY, 1);
            RETURN_IF_ERROR(compiler_nameop(c, loc, typeparam->v.TypeVar.name, Store));
            break;
        case TypeVarTuple_kind:
            ADDOP_LOAD_CONST(c, loc, typeparam->v.TypeVarTuple.name);
            ADDOP_I(c, loc, CALL_INTRINSIC_1, INTRINSIC_TYPEVARTUPLE);
            ADDOP_I(c, loc, COPY, 1);
            RETURN_IF_ERROR(compiler_nameop(c, loc, typeparam->v.TypeVarTuple.name, Store));
            break;
        case ParamSpec_kind:
            ADDOP_LOAD_CONST(c, loc, typeparam->v.ParamSpec.name);
            ADDOP_I(c, loc, CALL_INTRINSIC_1, INTRINSIC_PARAMSPEC);
            ADDOP_I(c, loc, COPY, 1);
            RETURN_IF_ERROR(compiler_nameop(c, loc, typeparam->v.ParamSpec.name, Store));
            break;
        }
    }
    ADDOP_I(c, LOC(asdl_seq_GET(type_params, 0)), BUILD_TUPLE, n);
    return SUCCESS;
}

static int
compiler_function_body(struct compiler *c, stmt_ty s, int is_async, Py_ssize_t funcflags,
                       int firstlineno)
{
    PyObject *docstring = NULL;
    arguments_ty args;
    identifier name;
    asdl_stmt_seq *body;
    int scope_type;

    if (is_async) {
        assert(s->kind == AsyncFunctionDef_kind);

        args = s->v.AsyncFunctionDef.args;
        name = s->v.AsyncFunctionDef.name;
        body = s->v.AsyncFunctionDef.body;

        scope_type = COMPILER_SCOPE_ASYNC_FUNCTION;
    } else {
        assert(s->kind == FunctionDef_kind);

        args = s->v.FunctionDef.args;
        name = s->v.FunctionDef.name;
        body = s->v.FunctionDef.body;

        scope_type = COMPILER_SCOPE_FUNCTION;
    }

    RETURN_IF_ERROR(
        compiler_enter_scope(c, name, scope_type, (void *)s, firstlineno));

    /* if not -OO mode, add docstring */
    if (c->c_optimize < 2) {
        docstring = _PyAST_GetDocString(body);
    }
    if (compiler_add_const(c->c_const_cache, c->u, docstring ? docstring : Py_None) < 0) {
        compiler_exit_scope(c);
        return ERROR;
    }

    c->u->u_metadata.u_argcount = asdl_seq_LEN(args->args);
    c->u->u_metadata.u_posonlyargcount = asdl_seq_LEN(args->posonlyargs);
    c->u->u_metadata.u_kwonlyargcount = asdl_seq_LEN(args->kwonlyargs);
    for (Py_ssize_t i = docstring ? 1 : 0; i < asdl_seq_LEN(body); i++) {
        VISIT_IN_SCOPE(c, stmt, (stmt_ty)asdl_seq_GET(body, i));
    }
    if (c->u->u_ste->ste_coroutine || c->u->u_ste->ste_generator) {
        if (wrap_in_stopiteration_handler(c) < 0) {
            compiler_exit_scope(c);
            return ERROR;
        }
    }
    PyCodeObject *co = optimize_and_assemble(c, 1);
    compiler_exit_scope(c);
    if (co == NULL) {
        Py_XDECREF(co);
        return ERROR;
    }
    location loc = LOC(s);
    if (compiler_make_closure(c, loc, co, funcflags) < 0) {
        Py_DECREF(co);
        return ERROR;
    }
    Py_DECREF(co);
    return SUCCESS;
}

static int
compiler_function(struct compiler *c, stmt_ty s, int is_async)
{
    arguments_ty args;
    expr_ty returns;
    identifier name;
    asdl_expr_seq *decos;
    asdl_type_param_seq *type_params;
    Py_ssize_t funcflags;
    int annotations;
    int firstlineno;

    if (is_async) {
        assert(s->kind == AsyncFunctionDef_kind);

        args = s->v.AsyncFunctionDef.args;
        returns = s->v.AsyncFunctionDef.returns;
        decos = s->v.AsyncFunctionDef.decorator_list;
        name = s->v.AsyncFunctionDef.name;
        type_params = s->v.AsyncFunctionDef.type_params;
    } else {
        assert(s->kind == FunctionDef_kind);

        args = s->v.FunctionDef.args;
        returns = s->v.FunctionDef.returns;
        decos = s->v.FunctionDef.decorator_list;
        name = s->v.FunctionDef.name;
        type_params = s->v.FunctionDef.type_params;
    }

    RETURN_IF_ERROR(compiler_check_debug_args(c, args));
    RETURN_IF_ERROR(compiler_decorators(c, decos));

    firstlineno = s->lineno;
    if (asdl_seq_LEN(decos)) {
        firstlineno = ((expr_ty)asdl_seq_GET(decos, 0))->lineno;
    }

    location loc = LOC(s);

    int is_generic = asdl_seq_LEN(type_params) > 0;

    if (is_generic) {
        // Used by the CALL to the type parameters function.
        ADDOP(c, loc, PUSH_NULL);
    }

    funcflags = compiler_default_arguments(c, loc, args);
    if (funcflags == -1) {
        return ERROR;
    }

    int num_typeparam_args = 0;

    if (is_generic) {
        if (funcflags & 0x01) {
            num_typeparam_args += 1;
        }
        if (funcflags & 0x02) {
            num_typeparam_args += 1;
        }
        if (num_typeparam_args == 2) {
            ADDOP_I(c, loc, SWAP, 2);
        }
        PyObject *type_params_name = PyUnicode_FromFormat("<generic parameters of %U>", name);
        if (!type_params_name) {
            return ERROR;
        }
        if (compiler_enter_scope(c, type_params_name, COMPILER_SCOPE_TYPEPARAMS,
                                 (void *)type_params, firstlineno) == -1) {
            Py_DECREF(type_params_name);
            return ERROR;
        }
        Py_DECREF(type_params_name);
        RETURN_IF_ERROR_IN_SCOPE(c, compiler_type_params(c, type_params));
        if ((funcflags & 0x01) || (funcflags & 0x02)) {
            RETURN_IF_ERROR_IN_SCOPE(c, codegen_addop_i(INSTR_SEQUENCE(c), LOAD_FAST, 0, loc));
        }
        if ((funcflags & 0x01) && (funcflags & 0x02)) {
            RETURN_IF_ERROR_IN_SCOPE(c, codegen_addop_i(INSTR_SEQUENCE(c), LOAD_FAST, 1, loc));
        }
    }

    annotations = compiler_visit_annotations(c, loc, args, returns);
    if (annotations < 0) {
        if (is_generic) {
            compiler_exit_scope(c);
        }
        return ERROR;
    }
    if (annotations > 0) {
        funcflags |= 0x04;
    }

    if (compiler_function_body(c, s, is_async, funcflags, firstlineno) < 0) {
        if (is_generic) {
            compiler_exit_scope(c);
        }
        return ERROR;
    }

    if (is_generic) {
        RETURN_IF_ERROR_IN_SCOPE(c, codegen_addop_i(
            INSTR_SEQUENCE(c), SWAP, 2, loc));
        RETURN_IF_ERROR_IN_SCOPE(c, codegen_addop_i(
            INSTR_SEQUENCE(c), CALL_INTRINSIC_2, INTRINSIC_SET_FUNCTION_TYPE_PARAMS, loc));

        c->u->u_metadata.u_argcount = num_typeparam_args;
        PyCodeObject *co = optimize_and_assemble(c, 0);
        compiler_exit_scope(c);
        if (co == NULL) {
            return ERROR;
        }
        if (compiler_make_closure(c, loc, co, 0) < 0) {
            Py_DECREF(co);
            return ERROR;
        }
        Py_DECREF(co);
        if (num_typeparam_args > 0) {
            ADDOP_I(c, loc, SWAP, num_typeparam_args + 1);
        }
        ADDOP_I(c, loc, CALL, num_typeparam_args);
    }

    RETURN_IF_ERROR(compiler_apply_decorators(c, decos));
    return compiler_nameop(c, loc, name, Store);
}

static int
compiler_set_type_params_in_class(struct compiler *c, location loc)
{
    _Py_DECLARE_STR(type_params, ".type_params");
    RETURN_IF_ERROR(compiler_nameop(c, loc, &_Py_STR(type_params), Load));
    RETURN_IF_ERROR(compiler_nameop(c, loc, &_Py_ID(__type_params__), Store));
    return 1;
}

static int
compiler_class_body(struct compiler *c, stmt_ty s, int firstlineno)
{
    /* ultimately generate code for:
         <name> = __build_class__(<func>, <name>, *<bases>, **<keywords>)
       where:
         <func> is a zero arg function/closure created from the class body.
            It mutates its locals to build the class namespace.
         <name> is the class name
         <bases> is the positional arguments and *varargs argument
         <keywords> is the keyword arguments and **kwds argument
       This borrows from compiler_call.
    */

    /* 1. compile the class body into a code object */
    RETURN_IF_ERROR(
        compiler_enter_scope(c, s->v.ClassDef.name,
                             COMPILER_SCOPE_CLASS, (void *)s, firstlineno));

    location loc = LOCATION(firstlineno, firstlineno, 0, 0);
    /* use the class name for name mangling */
    Py_XSETREF(c->u->u_private, Py_NewRef(s->v.ClassDef.name));
    /* load (global) __name__ ... */
    if (compiler_nameop(c, loc, &_Py_ID(__name__), Load) < 0) {
        compiler_exit_scope(c);
        return ERROR;
    }
    /* ... and store it as __module__ */
    if (compiler_nameop(c, loc, &_Py_ID(__module__), Store) < 0) {
        compiler_exit_scope(c);
        return ERROR;
    }
    assert(c->u->u_metadata.u_qualname);
    ADDOP_LOAD_CONST(c, loc, c->u->u_metadata.u_qualname);
    if (compiler_nameop(c, loc, &_Py_ID(__qualname__), Store) < 0) {
        compiler_exit_scope(c);
        return ERROR;
    }
    asdl_type_param_seq *type_params = s->v.ClassDef.type_params;
    if (asdl_seq_LEN(type_params) > 0) {
        if (!compiler_set_type_params_in_class(c, loc)) {
            compiler_exit_scope(c);
            return ERROR;
        }
    }
    if (c->u->u_ste->ste_needs_classdict) {
        ADDOP(c, loc, LOAD_LOCALS);

        // We can't use compiler_nameop here because we need to generate a
        // STORE_DEREF in a class namespace, and compiler_nameop() won't do
        // that by default.
        PyObject *cellvars = c->u->u_metadata.u_cellvars;
        if (compiler_addop_o(c->u, loc, STORE_DEREF, cellvars,
                             &_Py_ID(__classdict__)) < 0) {
            compiler_exit_scope(c);
            return ERROR;
        }
    }
    /* compile the body proper */
    if (compiler_body(c, loc, s->v.ClassDef.body) < 0) {
        compiler_exit_scope(c);
        return ERROR;
    }
    /* The following code is artificial */
    /* Set __classdictcell__ if necessary */
    if (c->u->u_ste->ste_needs_classdict) {
        /* Store __classdictcell__ into class namespace */
        int i = compiler_lookup_arg(c->u->u_metadata.u_cellvars, &_Py_ID(__classdict__));
        if (i < 0) {
            compiler_exit_scope(c);
            return ERROR;
        }
        ADDOP_I(c, NO_LOCATION, LOAD_CLOSURE, i);
        if (compiler_nameop(c, NO_LOCATION, &_Py_ID(__classdictcell__), Store) < 0) {
            compiler_exit_scope(c);
            return ERROR;
        }
    }
    /* Return __classcell__ if it is referenced, otherwise return None */
    if (c->u->u_ste->ste_needs_class_closure) {
        /* Store __classcell__ into class namespace & return it */
        int i = compiler_lookup_arg(c->u->u_metadata.u_cellvars, &_Py_ID(__class__));
        if (i < 0) {
            compiler_exit_scope(c);
            return ERROR;
        }
        ADDOP_I(c, NO_LOCATION, LOAD_CLOSURE, i);
        ADDOP_I(c, NO_LOCATION, COPY, 1);
        if (compiler_nameop(c, NO_LOCATION, &_Py_ID(__classcell__), Store) < 0) {
            compiler_exit_scope(c);
            return ERROR;
        }
    }
    else {
        /* No methods referenced __class__, so just return None */
        ADDOP_LOAD_CONST(c, NO_LOCATION, Py_None);
    }
    ADDOP_IN_SCOPE(c, NO_LOCATION, RETURN_VALUE);
    /* create the code object */
    PyCodeObject *co = optimize_and_assemble(c, 1);

    /* leave the new scope */
    compiler_exit_scope(c);
    if (co == NULL) {
        return ERROR;
    }

    /* 2. load the 'build_class' function */

    // these instructions should be attributed to the class line,
    // not a decorator line
    loc = LOC(s);
    ADDOP(c, loc, PUSH_NULL);
    ADDOP(c, loc, LOAD_BUILD_CLASS);

    /* 3. load a function (or closure) made from the code object */
    if (compiler_make_closure(c, loc, co, 0) < 0) {
        Py_DECREF(co);
        return ERROR;
    }
    Py_DECREF(co);

    /* 4. load class name */
    ADDOP_LOAD_CONST(c, loc, s->v.ClassDef.name);

    return SUCCESS;
}

static int
compiler_class(struct compiler *c, stmt_ty s)
{
    asdl_expr_seq *decos = s->v.ClassDef.decorator_list;

    RETURN_IF_ERROR(compiler_decorators(c, decos));

    int firstlineno = s->lineno;
    if (asdl_seq_LEN(decos)) {
        firstlineno = ((expr_ty)asdl_seq_GET(decos, 0))->lineno;
    }
    location loc = LOC(s);

    asdl_type_param_seq *type_params = s->v.ClassDef.type_params;
    int is_generic = asdl_seq_LEN(type_params) > 0;
    if (is_generic) {
        Py_XSETREF(c->u->u_private, Py_NewRef(s->v.ClassDef.name));
        ADDOP(c, loc, PUSH_NULL);
        PyObject *type_params_name = PyUnicode_FromFormat("<generic parameters of %U>",
                                                         s->v.ClassDef.name);
        if (!type_params_name) {
            return ERROR;
        }
        if (compiler_enter_scope(c, type_params_name, COMPILER_SCOPE_TYPEPARAMS,
                                 (void *)type_params, firstlineno) == -1) {
            Py_DECREF(type_params_name);
            return ERROR;
        }
        Py_DECREF(type_params_name);
        RETURN_IF_ERROR_IN_SCOPE(c, compiler_type_params(c, type_params));
        _Py_DECLARE_STR(type_params, ".type_params");
        RETURN_IF_ERROR_IN_SCOPE(c, compiler_nameop(c, loc, &_Py_STR(type_params), Store));
    }

    if (compiler_class_body(c, s, firstlineno) < 0) {
        if (is_generic) {
            compiler_exit_scope(c);
        }
        return ERROR;
    }

    /* generate the rest of the code for the call */

    if (is_generic) {
        _Py_DECLARE_STR(type_params, ".type_params");
        _Py_DECLARE_STR(generic_base, ".generic_base");
        RETURN_IF_ERROR_IN_SCOPE(c, compiler_nameop(c, loc, &_Py_STR(type_params), Load));
        RETURN_IF_ERROR_IN_SCOPE(
            c, codegen_addop_i(INSTR_SEQUENCE(c), CALL_INTRINSIC_1, INTRINSIC_SUBSCRIPT_GENERIC, loc)
        )
        RETURN_IF_ERROR_IN_SCOPE(c, compiler_nameop(c, loc, &_Py_STR(generic_base), Store));

        Py_ssize_t original_len = asdl_seq_LEN(s->v.ClassDef.bases);
        asdl_expr_seq *bases = _Py_asdl_expr_seq_new(
            original_len + 1, c->c_arena);
        if (bases == NULL) {
            compiler_exit_scope(c);
            return ERROR;
        }
        for (Py_ssize_t i = 0; i < original_len; i++) {
            asdl_seq_SET(bases, i, asdl_seq_GET(s->v.ClassDef.bases, i));
        }
        expr_ty name_node = _PyAST_Name(
            &_Py_STR(generic_base), Load,
            loc.lineno, loc.col_offset, loc.end_lineno, loc.end_col_offset, c->c_arena
        );
        if (name_node == NULL) {
            compiler_exit_scope(c);
            return ERROR;
        }
        asdl_seq_SET(bases, original_len, name_node);
        RETURN_IF_ERROR_IN_SCOPE(c, compiler_call_helper(c, loc, 2,
                                                         bases,
                                                         s->v.ClassDef.keywords));

        PyCodeObject *co = optimize_and_assemble(c, 0);
        compiler_exit_scope(c);
        if (co == NULL) {
            return ERROR;
        }
        if (compiler_make_closure(c, loc, co, 0) < 0) {
            Py_DECREF(co);
            return ERROR;
        }
        Py_DECREF(co);
        ADDOP_I(c, loc, CALL, 0);
    } else {
        RETURN_IF_ERROR(compiler_call_helper(c, loc, 2,
                                            s->v.ClassDef.bases,
                                            s->v.ClassDef.keywords));
    }

    /* 6. apply decorators */
    RETURN_IF_ERROR(compiler_apply_decorators(c, decos));

    /* 7. store into <name> */
    RETURN_IF_ERROR(compiler_nameop(c, loc, s->v.ClassDef.name, Store));
    return SUCCESS;
}

static int
compiler_typealias_body(struct compiler *c, stmt_ty s)
{
    location loc = LOC(s);
    PyObject *name = s->v.TypeAlias.name->v.Name.id;
    RETURN_IF_ERROR(
        compiler_enter_scope(c, name, COMPILER_SCOPE_FUNCTION, s, loc.lineno));
    /* Make None the first constant, so the evaluate function can't have a
        docstring. */
    RETURN_IF_ERROR(compiler_add_const(c->c_const_cache, c->u, Py_None));
    VISIT_IN_SCOPE(c, expr, s->v.TypeAlias.value);
    ADDOP_IN_SCOPE(c, loc, RETURN_VALUE);
    PyCodeObject *co = optimize_and_assemble(c, 0);
    compiler_exit_scope(c);
    if (co == NULL) {
        return ERROR;
    }
    if (compiler_make_closure(c, loc, co, 0) < 0) {
        Py_DECREF(co);
        return ERROR;
    }
    Py_DECREF(co);
    ADDOP_I(c, loc, BUILD_TUPLE, 3);
    ADDOP_I(c, loc, CALL_INTRINSIC_1, INTRINSIC_TYPEALIAS);
    return SUCCESS;
}

static int
compiler_typealias(struct compiler *c, stmt_ty s)
{
    location loc = LOC(s);
    asdl_type_param_seq *type_params = s->v.TypeAlias.type_params;
    int is_generic = asdl_seq_LEN(type_params) > 0;
    PyObject *name = s->v.TypeAlias.name->v.Name.id;
    if (is_generic) {
        ADDOP(c, loc, PUSH_NULL);
        PyObject *type_params_name = PyUnicode_FromFormat("<generic parameters of %U>",
                                                         name);
        if (!type_params_name) {
            return ERROR;
        }
        if (compiler_enter_scope(c, type_params_name, COMPILER_SCOPE_TYPEPARAMS,
                                 (void *)type_params, loc.lineno) == -1) {
            Py_DECREF(type_params_name);
            return ERROR;
        }
        Py_DECREF(type_params_name);
        RETURN_IF_ERROR_IN_SCOPE(
            c, compiler_addop_load_const(c->c_const_cache, c->u, loc, name)
        );
        RETURN_IF_ERROR_IN_SCOPE(c, compiler_type_params(c, type_params));
    }
    else {
        ADDOP_LOAD_CONST(c, loc, name);
        ADDOP_LOAD_CONST(c, loc, Py_None);
    }

    if (compiler_typealias_body(c, s) < 0) {
        if (is_generic) {
            compiler_exit_scope(c);
        }
        return ERROR;
    }

    if (is_generic) {
        PyCodeObject *co = optimize_and_assemble(c, 0);
        compiler_exit_scope(c);
        if (co == NULL) {
            return ERROR;
        }
        if (compiler_make_closure(c, loc, co, 0) < 0) {
            Py_DECREF(co);
            return ERROR;
        }
        Py_DECREF(co);
        ADDOP_I(c, loc, CALL, 0);
    }
    RETURN_IF_ERROR(compiler_nameop(c, loc, name, Store));
    return SUCCESS;
}

/* Return false if the expression is a constant value except named singletons.
   Return true otherwise. */
static bool
check_is_arg(expr_ty e)
{
    if (e->kind != Constant_kind) {
        return true;
    }
    PyObject *value = e->v.Constant.value;
    return (value == Py_None
         || value == Py_False
         || value == Py_True
         || value == Py_Ellipsis);
}

static PyTypeObject * infer_type(expr_ty e);

/* Check operands of identity checks ("is" and "is not").
   Emit a warning if any operand is a constant except named singletons.
 */
static int
check_compare(struct compiler *c, expr_ty e)
{
    Py_ssize_t i, n;
    bool left = check_is_arg(e->v.Compare.left);
    expr_ty left_expr = e->v.Compare.left;
    n = asdl_seq_LEN(e->v.Compare.ops);
    for (i = 0; i < n; i++) {
        cmpop_ty op = (cmpop_ty)asdl_seq_GET(e->v.Compare.ops, i);
        expr_ty right_expr = (expr_ty)asdl_seq_GET(e->v.Compare.comparators, i);
        bool right = check_is_arg(right_expr);
        if (op == Is || op == IsNot) {
            if (!right || !left) {
                const char *msg = (op == Is)
                        ? "\"is\" with '%.200s' literal. Did you mean \"==\"?"
                        : "\"is not\" with '%.200s' literal. Did you mean \"!=\"?";
                expr_ty literal = !left ? left_expr : right_expr;
                return compiler_warn(
                    c, LOC(e), msg, infer_type(literal)->tp_name
                );
            }
        }
        left = right;
        left_expr = right_expr;
    }
    return SUCCESS;
}

static const int compare_masks[] = {
    [Py_LT] = COMPARISON_LESS_THAN,
    [Py_LE] = COMPARISON_LESS_THAN | COMPARISON_EQUALS,
    [Py_EQ] = COMPARISON_EQUALS,
    [Py_NE] = COMPARISON_NOT_EQUALS,
    [Py_GT] = COMPARISON_GREATER_THAN,
    [Py_GE] = COMPARISON_GREATER_THAN | COMPARISON_EQUALS,
};

static int compiler_addcompare(struct compiler *c, location loc,
                               cmpop_ty op)
{
    int cmp;
    switch (op) {
    case Eq:
        cmp = Py_EQ;
        break;
    case NotEq:
        cmp = Py_NE;
        break;
    case Lt:
        cmp = Py_LT;
        break;
    case LtE:
        cmp = Py_LE;
        break;
    case Gt:
        cmp = Py_GT;
        break;
    case GtE:
        cmp = Py_GE;
        break;
    case Is:
        ADDOP_I(c, loc, IS_OP, 0);
        return SUCCESS;
    case IsNot:
        ADDOP_I(c, loc, IS_OP, 1);
        return SUCCESS;
    case In:
        ADDOP_I(c, loc, CONTAINS_OP, 0);
        return SUCCESS;
    case NotIn:
        ADDOP_I(c, loc, CONTAINS_OP, 1);
        return SUCCESS;
    default:
        Py_UNREACHABLE();
    }
    /* cmp goes in top bits of the oparg, while the low bits are used by quickened
     * versions of this opcode to store the comparison mask. */
    ADDOP_I(c, loc, COMPARE_OP, (cmp << 4) | compare_masks[cmp]);
    return SUCCESS;
}



static int
compiler_jump_if(struct compiler *c, location loc,
                 expr_ty e, jump_target_label next, int cond)
{
    switch (e->kind) {
    case UnaryOp_kind:
        if (e->v.UnaryOp.op == Not) {
            return compiler_jump_if(c, loc, e->v.UnaryOp.operand, next, !cond);
        }
        /* fallback to general implementation */
        break;
    case BoolOp_kind: {
        asdl_expr_seq *s = e->v.BoolOp.values;
        Py_ssize_t i, n = asdl_seq_LEN(s) - 1;
        assert(n >= 0);
        int cond2 = e->v.BoolOp.op == Or;
        jump_target_label next2 = next;
        if (!cond2 != !cond) {
            NEW_JUMP_TARGET_LABEL(c, new_next2);
            next2 = new_next2;
        }
        for (i = 0; i < n; ++i) {
            RETURN_IF_ERROR(
                compiler_jump_if(c, loc, (expr_ty)asdl_seq_GET(s, i), next2, cond2));
        }
        RETURN_IF_ERROR(
            compiler_jump_if(c, loc, (expr_ty)asdl_seq_GET(s, n), next, cond));
        if (!SAME_LABEL(next2, next)) {
            USE_LABEL(c, next2);
        }
        return SUCCESS;
    }
    case IfExp_kind: {
        NEW_JUMP_TARGET_LABEL(c, end);
        NEW_JUMP_TARGET_LABEL(c, next2);
        RETURN_IF_ERROR(
            compiler_jump_if(c, loc, e->v.IfExp.test, next2, 0));
        RETURN_IF_ERROR(
            compiler_jump_if(c, loc, e->v.IfExp.body, next, cond));
        ADDOP_JUMP(c, NO_LOCATION, JUMP, end);

        USE_LABEL(c, next2);
        RETURN_IF_ERROR(
            compiler_jump_if(c, loc, e->v.IfExp.orelse, next, cond));

        USE_LABEL(c, end);
        return SUCCESS;
    }
    case Compare_kind: {
        Py_ssize_t n = asdl_seq_LEN(e->v.Compare.ops) - 1;
        if (n > 0) {
            RETURN_IF_ERROR(check_compare(c, e));
            NEW_JUMP_TARGET_LABEL(c, cleanup);
            VISIT(c, expr, e->v.Compare.left);
            for (Py_ssize_t i = 0; i < n; i++) {
                VISIT(c, expr,
                    (expr_ty)asdl_seq_GET(e->v.Compare.comparators, i));
                ADDOP_I(c, LOC(e), SWAP, 2);
                ADDOP_I(c, LOC(e), COPY, 2);
                ADDOP_COMPARE(c, LOC(e), asdl_seq_GET(e->v.Compare.ops, i));
                ADDOP_JUMP(c, LOC(e), POP_JUMP_IF_FALSE, cleanup);
            }
            VISIT(c, expr, (expr_ty)asdl_seq_GET(e->v.Compare.comparators, n));
            ADDOP_COMPARE(c, LOC(e), asdl_seq_GET(e->v.Compare.ops, n));
            ADDOP_JUMP(c, LOC(e), cond ? POP_JUMP_IF_TRUE : POP_JUMP_IF_FALSE, next);
            NEW_JUMP_TARGET_LABEL(c, end);
            ADDOP_JUMP(c, NO_LOCATION, JUMP, end);

            USE_LABEL(c, cleanup);
            ADDOP(c, LOC(e), POP_TOP);
            if (!cond) {
                ADDOP_JUMP(c, NO_LOCATION, JUMP, next);
            }

            USE_LABEL(c, end);
            return SUCCESS;
        }
        /* fallback to general implementation */
        break;
    }
    default:
        /* fallback to general implementation */
        break;
    }

    /* general implementation */
    VISIT(c, expr, e);
    ADDOP_JUMP(c, LOC(e), cond ? POP_JUMP_IF_TRUE : POP_JUMP_IF_FALSE, next);
    return SUCCESS;
}

static int
compiler_ifexp(struct compiler *c, expr_ty e)
{
    assert(e->kind == IfExp_kind);
    NEW_JUMP_TARGET_LABEL(c, end);
    NEW_JUMP_TARGET_LABEL(c, next);

    RETURN_IF_ERROR(
        compiler_jump_if(c, LOC(e), e->v.IfExp.test, next, 0));

    VISIT(c, expr, e->v.IfExp.body);
    ADDOP_JUMP(c, NO_LOCATION, JUMP, end);

    USE_LABEL(c, next);
    VISIT(c, expr, e->v.IfExp.orelse);

    USE_LABEL(c, end);
    return SUCCESS;
}

static int
compiler_lambda(struct compiler *c, expr_ty e)
{
    PyCodeObject *co;
    Py_ssize_t funcflags;
    arguments_ty args = e->v.Lambda.args;
    assert(e->kind == Lambda_kind);

    RETURN_IF_ERROR(compiler_check_debug_args(c, args));

    location loc = LOC(e);
    funcflags = compiler_default_arguments(c, loc, args);
    if (funcflags == -1) {
        return ERROR;
    }

    _Py_DECLARE_STR(anon_lambda, "<lambda>");
    RETURN_IF_ERROR(
        compiler_enter_scope(c, &_Py_STR(anon_lambda), COMPILER_SCOPE_LAMBDA,
                             (void *)e, e->lineno));

    /* Make None the first constant, so the lambda can't have a
       docstring. */
    RETURN_IF_ERROR(compiler_add_const(c->c_const_cache, c->u, Py_None));

    c->u->u_metadata.u_argcount = asdl_seq_LEN(args->args);
    c->u->u_metadata.u_posonlyargcount = asdl_seq_LEN(args->posonlyargs);
    c->u->u_metadata.u_kwonlyargcount = asdl_seq_LEN(args->kwonlyargs);
    VISIT_IN_SCOPE(c, expr, e->v.Lambda.body);
    if (c->u->u_ste->ste_generator) {
        co = optimize_and_assemble(c, 0);
    }
    else {
        location loc = LOCATION(e->lineno, e->lineno, 0, 0);
        ADDOP_IN_SCOPE(c, loc, RETURN_VALUE);
        co = optimize_and_assemble(c, 1);
    }
    compiler_exit_scope(c);
    if (co == NULL) {
        return ERROR;
    }

    if (compiler_make_closure(c, loc, co, funcflags) < 0) {
        Py_DECREF(co);
        return ERROR;
    }
    Py_DECREF(co);

    return SUCCESS;
}

static int
compiler_if(struct compiler *c, stmt_ty s)
{
    jump_target_label next;
    assert(s->kind == If_kind);
    NEW_JUMP_TARGET_LABEL(c, end);
    if (asdl_seq_LEN(s->v.If.orelse)) {
        NEW_JUMP_TARGET_LABEL(c, orelse);
        next = orelse;
    }
    else {
        next = end;
    }
    RETURN_IF_ERROR(
        compiler_jump_if(c, LOC(s), s->v.If.test, next, 0));

    VISIT_SEQ(c, stmt, s->v.If.body);
    if (asdl_seq_LEN(s->v.If.orelse)) {
        ADDOP_JUMP(c, NO_LOCATION, JUMP, end);

        USE_LABEL(c, next);
        VISIT_SEQ(c, stmt, s->v.If.orelse);
    }

    USE_LABEL(c, end);
    return SUCCESS;
}

static int
compiler_for(struct compiler *c, stmt_ty s)
{
    location loc = LOC(s);
    NEW_JUMP_TARGET_LABEL(c, start);
    NEW_JUMP_TARGET_LABEL(c, body);
    NEW_JUMP_TARGET_LABEL(c, cleanup);
    NEW_JUMP_TARGET_LABEL(c, end);

    RETURN_IF_ERROR(compiler_push_fblock(c, loc, FOR_LOOP, start, end, NULL));

    VISIT(c, expr, s->v.For.iter);
    ADDOP(c, loc, GET_ITER);

    USE_LABEL(c, start);
    ADDOP_JUMP(c, loc, FOR_ITER, cleanup);

    USE_LABEL(c, body);
    VISIT(c, expr, s->v.For.target);
    VISIT_SEQ(c, stmt, s->v.For.body);
    /* Mark jump as artificial */
    ADDOP_JUMP(c, NO_LOCATION, JUMP, start);

    USE_LABEL(c, cleanup);
    ADDOP(c, NO_LOCATION, END_FOR);

    compiler_pop_fblock(c, FOR_LOOP, start);

    VISIT_SEQ(c, stmt, s->v.For.orelse);

    USE_LABEL(c, end);
    return SUCCESS;
}


static int
compiler_async_for(struct compiler *c, stmt_ty s)
{
    location loc = LOC(s);
    if (IS_TOP_LEVEL_AWAIT(c)){
        c->u->u_ste->ste_coroutine = 1;
    } else if (c->u->u_scope_type != COMPILER_SCOPE_ASYNC_FUNCTION) {
        return compiler_error(c, loc, "'async for' outside async function");
    }

    NEW_JUMP_TARGET_LABEL(c, start);
    NEW_JUMP_TARGET_LABEL(c, except);
    NEW_JUMP_TARGET_LABEL(c, end);

    VISIT(c, expr, s->v.AsyncFor.iter);
    ADDOP(c, loc, GET_AITER);

    USE_LABEL(c, start);
    RETURN_IF_ERROR(compiler_push_fblock(c, loc, FOR_LOOP, start, end, NULL));

    /* SETUP_FINALLY to guard the __anext__ call */
    ADDOP_JUMP(c, loc, SETUP_FINALLY, except);
    ADDOP(c, loc, GET_ANEXT);
    ADDOP_LOAD_CONST(c, loc, Py_None);
    ADD_YIELD_FROM(c, loc, 1);
    ADDOP(c, loc, POP_BLOCK);  /* for SETUP_FINALLY */

    /* Success block for __anext__ */
    VISIT(c, expr, s->v.AsyncFor.target);
    VISIT_SEQ(c, stmt, s->v.AsyncFor.body);
    /* Mark jump as artificial */
    ADDOP_JUMP(c, NO_LOCATION, JUMP, start);

    compiler_pop_fblock(c, FOR_LOOP, start);

    /* Except block for __anext__ */
    USE_LABEL(c, except);

    /* Use same line number as the iterator,
     * as the END_ASYNC_FOR succeeds the `for`, not the body. */
    loc = LOC(s->v.AsyncFor.iter);
    ADDOP(c, loc, END_ASYNC_FOR);

    /* `else` block */
    VISIT_SEQ(c, stmt, s->v.For.orelse);

    USE_LABEL(c, end);
    return SUCCESS;
}

static int
compiler_while(struct compiler *c, stmt_ty s)
{
    NEW_JUMP_TARGET_LABEL(c, loop);
    NEW_JUMP_TARGET_LABEL(c, body);
    NEW_JUMP_TARGET_LABEL(c, end);
    NEW_JUMP_TARGET_LABEL(c, anchor);

    USE_LABEL(c, loop);

    RETURN_IF_ERROR(compiler_push_fblock(c, LOC(s), WHILE_LOOP, loop, end, NULL));
    RETURN_IF_ERROR(compiler_jump_if(c, LOC(s), s->v.While.test, anchor, 0));

    USE_LABEL(c, body);
    VISIT_SEQ(c, stmt, s->v.While.body);
    RETURN_IF_ERROR(compiler_jump_if(c, LOC(s), s->v.While.test, body, 1));

    compiler_pop_fblock(c, WHILE_LOOP, loop);

    USE_LABEL(c, anchor);
    if (s->v.While.orelse) {
        VISIT_SEQ(c, stmt, s->v.While.orelse);
    }

    USE_LABEL(c, end);
    return SUCCESS;
}

static int
compiler_return(struct compiler *c, stmt_ty s)
{
    location loc = LOC(s);
    int preserve_tos = ((s->v.Return.value != NULL) &&
                        (s->v.Return.value->kind != Constant_kind));
    if (!_PyST_IsFunctionLike(c->u->u_ste)) {
        return compiler_error(c, loc, "'return' outside function");
    }
    if (s->v.Return.value != NULL &&
        c->u->u_ste->ste_coroutine && c->u->u_ste->ste_generator)
    {
        return compiler_error(c, loc, "'return' with value in async generator");
    }

    if (preserve_tos) {
        VISIT(c, expr, s->v.Return.value);
    } else {
        /* Emit instruction with line number for return value */
        if (s->v.Return.value != NULL) {
            loc = LOC(s->v.Return.value);
            ADDOP(c, loc, NOP);
        }
    }
    if (s->v.Return.value == NULL || s->v.Return.value->lineno != s->lineno) {
        loc = LOC(s);
        ADDOP(c, loc, NOP);
    }

    RETURN_IF_ERROR(compiler_unwind_fblock_stack(c, &loc, preserve_tos, NULL));
    if (s->v.Return.value == NULL) {
        ADDOP_LOAD_CONST(c, loc, Py_None);
    }
    else if (!preserve_tos) {
        ADDOP_LOAD_CONST(c, loc, s->v.Return.value->v.Constant.value);
    }
    ADDOP(c, loc, RETURN_VALUE);

    return SUCCESS;
}

static int
compiler_break(struct compiler *c, location loc)
{
    struct fblockinfo *loop = NULL;
    location origin_loc = loc;
    /* Emit instruction with line number */
    ADDOP(c, loc, NOP);
    RETURN_IF_ERROR(compiler_unwind_fblock_stack(c, &loc, 0, &loop));
    if (loop == NULL) {
        return compiler_error(c, origin_loc, "'break' outside loop");
    }
    RETURN_IF_ERROR(compiler_unwind_fblock(c, &loc, loop, 0));
    ADDOP_JUMP(c, loc, JUMP, loop->fb_exit);
    return SUCCESS;
}

static int
compiler_continue(struct compiler *c, location loc)
{
    struct fblockinfo *loop = NULL;
    location origin_loc = loc;
    /* Emit instruction with line number */
    ADDOP(c, loc, NOP);
    RETURN_IF_ERROR(compiler_unwind_fblock_stack(c, &loc, 0, &loop));
    if (loop == NULL) {
        return compiler_error(c, origin_loc, "'continue' not properly in loop");
    }
    ADDOP_JUMP(c, loc, JUMP, loop->fb_block);
    return SUCCESS;
}


/* Code generated for "try: <body> finally: <finalbody>" is as follows:

        SETUP_FINALLY           L
        <code for body>
        POP_BLOCK
        <code for finalbody>
        JUMP E
    L:
        <code for finalbody>
    E:

   The special instructions use the block stack.  Each block
   stack entry contains the instruction that created it (here
   SETUP_FINALLY), the level of the value stack at the time the
   block stack entry was created, and a label (here L).

   SETUP_FINALLY:
    Pushes the current value stack level and the label
    onto the block stack.
   POP_BLOCK:
    Pops en entry from the block stack.

   The block stack is unwound when an exception is raised:
   when a SETUP_FINALLY entry is found, the raised and the caught
   exceptions are pushed onto the value stack (and the exception
   condition is cleared), and the interpreter jumps to the label
   gotten from the block stack.
*/

static int
compiler_try_finally(struct compiler *c, stmt_ty s)
{
    location loc = LOC(s);

    NEW_JUMP_TARGET_LABEL(c, body);
    NEW_JUMP_TARGET_LABEL(c, end);
    NEW_JUMP_TARGET_LABEL(c, exit);
    NEW_JUMP_TARGET_LABEL(c, cleanup);

    /* `try` block */
    ADDOP_JUMP(c, loc, SETUP_FINALLY, end);

    USE_LABEL(c, body);
    RETURN_IF_ERROR(
        compiler_push_fblock(c, loc, FINALLY_TRY, body, end,
                             s->v.Try.finalbody));

    if (s->v.Try.handlers && asdl_seq_LEN(s->v.Try.handlers)) {
        RETURN_IF_ERROR(compiler_try_except(c, s));
    }
    else {
        VISIT_SEQ(c, stmt, s->v.Try.body);
    }
    ADDOP(c, NO_LOCATION, POP_BLOCK);
    compiler_pop_fblock(c, FINALLY_TRY, body);
    VISIT_SEQ(c, stmt, s->v.Try.finalbody);

    ADDOP_JUMP(c, NO_LOCATION, JUMP, exit);
    /* `finally` block */

    USE_LABEL(c, end);

    loc = NO_LOCATION;
    ADDOP_JUMP(c, loc, SETUP_CLEANUP, cleanup);
    ADDOP(c, loc, PUSH_EXC_INFO);
    RETURN_IF_ERROR(
        compiler_push_fblock(c, loc, FINALLY_END, end, NO_LABEL, NULL));
    VISIT_SEQ(c, stmt, s->v.Try.finalbody);
    compiler_pop_fblock(c, FINALLY_END, end);

    loc = NO_LOCATION;
    ADDOP_I(c, loc, RERAISE, 0);

    USE_LABEL(c, cleanup);
    POP_EXCEPT_AND_RERAISE(c, loc);

    USE_LABEL(c, exit);
    return SUCCESS;
}

static int
compiler_try_star_finally(struct compiler *c, stmt_ty s)
{
    location loc = LOC(s);

    NEW_JUMP_TARGET_LABEL(c, body);
    NEW_JUMP_TARGET_LABEL(c, end);
    NEW_JUMP_TARGET_LABEL(c, exit);
    NEW_JUMP_TARGET_LABEL(c, cleanup);
    /* `try` block */
    ADDOP_JUMP(c, loc, SETUP_FINALLY, end);

    USE_LABEL(c, body);
    RETURN_IF_ERROR(
        compiler_push_fblock(c, loc, FINALLY_TRY, body, end,
                             s->v.TryStar.finalbody));

    if (s->v.TryStar.handlers && asdl_seq_LEN(s->v.TryStar.handlers)) {
        RETURN_IF_ERROR(compiler_try_star_except(c, s));
    }
    else {
        VISIT_SEQ(c, stmt, s->v.TryStar.body);
    }
    ADDOP(c, NO_LOCATION, POP_BLOCK);
    compiler_pop_fblock(c, FINALLY_TRY, body);
    VISIT_SEQ(c, stmt, s->v.TryStar.finalbody);

    ADDOP_JUMP(c, NO_LOCATION, JUMP, exit);

    /* `finally` block */
    USE_LABEL(c, end);

    loc = NO_LOCATION;
    ADDOP_JUMP(c, loc, SETUP_CLEANUP, cleanup);
    ADDOP(c, loc, PUSH_EXC_INFO);
    RETURN_IF_ERROR(
        compiler_push_fblock(c, loc, FINALLY_END, end, NO_LABEL, NULL));

    VISIT_SEQ(c, stmt, s->v.TryStar.finalbody);

    compiler_pop_fblock(c, FINALLY_END, end);
    loc = NO_LOCATION;
    ADDOP_I(c, loc, RERAISE, 0);

    USE_LABEL(c, cleanup);
    POP_EXCEPT_AND_RERAISE(c, loc);

    USE_LABEL(c, exit);
    return SUCCESS;
}


/*
   Code generated for "try: S except E1 as V1: S1 except E2 as V2: S2 ...":
   (The contents of the value stack is shown in [], with the top
   at the right; 'tb' is trace-back info, 'val' the exception's
   associated value, and 'exc' the exception.)

   Value stack          Label   Instruction     Argument
   []                           SETUP_FINALLY   L1
   []                           <code for S>
   []                           POP_BLOCK
   []                           JUMP            L0

   [exc]                L1:     <evaluate E1>           )
   [exc, E1]                    CHECK_EXC_MATCH         )
   [exc, bool]                  POP_JUMP_IF_FALSE L2    ) only if E1
   [exc]                        <assign to V1>  (or POP if no V1)
   []                           <code for S1>
                                JUMP            L0

   [exc]                L2:     <evaluate E2>
   .............................etc.......................

   [exc]                Ln+1:   RERAISE     # re-raise exception

   []                   L0:     <next statement>

   Of course, parts are not generated if Vi or Ei is not present.
*/
static int
compiler_try_except(struct compiler *c, stmt_ty s)
{
    location loc = LOC(s);
    Py_ssize_t i, n;

    NEW_JUMP_TARGET_LABEL(c, body);
    NEW_JUMP_TARGET_LABEL(c, except);
    NEW_JUMP_TARGET_LABEL(c, end);
    NEW_JUMP_TARGET_LABEL(c, cleanup);

    ADDOP_JUMP(c, loc, SETUP_FINALLY, except);

    USE_LABEL(c, body);
    RETURN_IF_ERROR(
        compiler_push_fblock(c, loc, TRY_EXCEPT, body, NO_LABEL, NULL));
    VISIT_SEQ(c, stmt, s->v.Try.body);
    compiler_pop_fblock(c, TRY_EXCEPT, body);
    ADDOP(c, NO_LOCATION, POP_BLOCK);
    if (s->v.Try.orelse && asdl_seq_LEN(s->v.Try.orelse)) {
        VISIT_SEQ(c, stmt, s->v.Try.orelse);
    }
    ADDOP_JUMP(c, NO_LOCATION, JUMP, end);
    n = asdl_seq_LEN(s->v.Try.handlers);

    USE_LABEL(c, except);

    ADDOP_JUMP(c, NO_LOCATION, SETUP_CLEANUP, cleanup);
    ADDOP(c, NO_LOCATION, PUSH_EXC_INFO);

    /* Runtime will push a block here, so we need to account for that */
    RETURN_IF_ERROR(
        compiler_push_fblock(c, loc, EXCEPTION_HANDLER, NO_LABEL, NO_LABEL, NULL));

    for (i = 0; i < n; i++) {
        excepthandler_ty handler = (excepthandler_ty)asdl_seq_GET(
            s->v.Try.handlers, i);
        location loc = LOC(handler);
        if (!handler->v.ExceptHandler.type && i < n-1) {
            return compiler_error(c, loc, "default 'except:' must be last");
        }
        NEW_JUMP_TARGET_LABEL(c, next_except);
        except = next_except;
        if (handler->v.ExceptHandler.type) {
            VISIT(c, expr, handler->v.ExceptHandler.type);
            ADDOP(c, loc, CHECK_EXC_MATCH);
            ADDOP_JUMP(c, loc, POP_JUMP_IF_FALSE, except);
        }
        if (handler->v.ExceptHandler.name) {
            NEW_JUMP_TARGET_LABEL(c, cleanup_end);
            NEW_JUMP_TARGET_LABEL(c, cleanup_body);

            RETURN_IF_ERROR(
                compiler_nameop(c, loc, handler->v.ExceptHandler.name, Store));

            /*
              try:
                  # body
              except type as name:
                  try:
                      # body
                  finally:
                      name = None # in case body contains "del name"
                      del name
            */

            /* second try: */
            ADDOP_JUMP(c, loc, SETUP_CLEANUP, cleanup_end);

            USE_LABEL(c, cleanup_body);
            RETURN_IF_ERROR(
                compiler_push_fblock(c, loc, HANDLER_CLEANUP, cleanup_body,
                                     NO_LABEL, handler->v.ExceptHandler.name));

            /* second # body */
            VISIT_SEQ(c, stmt, handler->v.ExceptHandler.body);
            compiler_pop_fblock(c, HANDLER_CLEANUP, cleanup_body);
            /* name = None; del name; # Mark as artificial */
            ADDOP(c, NO_LOCATION, POP_BLOCK);
            ADDOP(c, NO_LOCATION, POP_BLOCK);
            ADDOP(c, NO_LOCATION, POP_EXCEPT);
            ADDOP_LOAD_CONST(c, NO_LOCATION, Py_None);
            RETURN_IF_ERROR(
                compiler_nameop(c, NO_LOCATION, handler->v.ExceptHandler.name, Store));
            RETURN_IF_ERROR(
                compiler_nameop(c, NO_LOCATION, handler->v.ExceptHandler.name, Del));
            ADDOP_JUMP(c, NO_LOCATION, JUMP, end);

            /* except: */
            USE_LABEL(c, cleanup_end);

            /* name = None; del name; # artificial */
            ADDOP_LOAD_CONST(c, NO_LOCATION, Py_None);
            RETURN_IF_ERROR(
                compiler_nameop(c, NO_LOCATION, handler->v.ExceptHandler.name, Store));
            RETURN_IF_ERROR(
                compiler_nameop(c, NO_LOCATION, handler->v.ExceptHandler.name, Del));

            ADDOP_I(c, NO_LOCATION, RERAISE, 1);
        }
        else {
            NEW_JUMP_TARGET_LABEL(c, cleanup_body);

            ADDOP(c, loc, POP_TOP); /* exc_value */

            USE_LABEL(c, cleanup_body);
            RETURN_IF_ERROR(
                compiler_push_fblock(c, loc, HANDLER_CLEANUP, cleanup_body,
                                     NO_LABEL, NULL));

            VISIT_SEQ(c, stmt, handler->v.ExceptHandler.body);
            compiler_pop_fblock(c, HANDLER_CLEANUP, cleanup_body);
            ADDOP(c, NO_LOCATION, POP_BLOCK);
            ADDOP(c, NO_LOCATION, POP_EXCEPT);
            ADDOP_JUMP(c, NO_LOCATION, JUMP, end);
        }

        USE_LABEL(c, except);
    }
    /* artificial */
    compiler_pop_fblock(c, EXCEPTION_HANDLER, NO_LABEL);
    ADDOP_I(c, NO_LOCATION, RERAISE, 0);

    USE_LABEL(c, cleanup);
    POP_EXCEPT_AND_RERAISE(c, NO_LOCATION);

    USE_LABEL(c, end);
    return SUCCESS;
}

/*
   Code generated for "try: S except* E1 as V1: S1 except* E2 as V2: S2 ...":
   (The contents of the value stack is shown in [], with the top
   at the right; 'tb' is trace-back info, 'val' the exception instance,
   and 'typ' the exception's type.)

   Value stack                   Label         Instruction     Argument
   []                                         SETUP_FINALLY         L1
   []                                         <code for S>
   []                                         POP_BLOCK
   []                                         JUMP                  L0

   [exc]                            L1:       BUILD_LIST   )  list for raised/reraised excs ("result")
   [orig, res]                                COPY 2       )  make a copy of the original EG

   [orig, res, exc]                           <evaluate E1>
   [orig, res, exc, E1]                       CHECK_EG_MATCH
   [orig, res, rest/exc, match?]              COPY 1
   [orig, res, rest/exc, match?, match?]      POP_JUMP_IF_NONE      C1

   [orig, res, rest, match]                   <assign to V1>  (or POP if no V1)

   [orig, res, rest]                          SETUP_FINALLY         R1
   [orig, res, rest]                          <code for S1>
   [orig, res, rest]                          JUMP                  L2

   [orig, res, rest, i, v]          R1:       LIST_APPEND   3 ) exc raised in except* body - add to res
   [orig, res, rest, i]                       POP
   [orig, res, rest]                          JUMP                  LE2

   [orig, res, rest]                L2:       NOP  ) for lineno
   [orig, res, rest]                          JUMP                  LE2

   [orig, res, rest/exc, None]      C1:       POP

   [orig, res, rest]               LE2:       <evaluate E2>
   .............................etc.......................

   [orig, res, rest]                Ln+1:     LIST_APPEND 1  ) add unhandled exc to res (could be None)

   [orig, res]                                CALL_INTRINSIC_2 PREP_RERAISE_STAR
   [exc]                                      COPY 1
   [exc, exc]                                 POP_JUMP_IF_NOT_NONE  RER
   [exc]                                      POP_TOP
   []                                         JUMP                  L0

   [exc]                            RER:      SWAP 2
   [exc, prev_exc_info]                       POP_EXCEPT
   [exc]                                      RERAISE               0

   []                               L0:       <next statement>
*/
static int
compiler_try_star_except(struct compiler *c, stmt_ty s)
{
    location loc = LOC(s);

    NEW_JUMP_TARGET_LABEL(c, body);
    NEW_JUMP_TARGET_LABEL(c, except);
    NEW_JUMP_TARGET_LABEL(c, orelse);
    NEW_JUMP_TARGET_LABEL(c, end);
    NEW_JUMP_TARGET_LABEL(c, cleanup);
    NEW_JUMP_TARGET_LABEL(c, reraise_star);

    ADDOP_JUMP(c, loc, SETUP_FINALLY, except);

    USE_LABEL(c, body);
    RETURN_IF_ERROR(
        compiler_push_fblock(c, loc, TRY_EXCEPT, body, NO_LABEL, NULL));
    VISIT_SEQ(c, stmt, s->v.TryStar.body);
    compiler_pop_fblock(c, TRY_EXCEPT, body);
    ADDOP(c, NO_LOCATION, POP_BLOCK);
    ADDOP_JUMP(c, NO_LOCATION, JUMP, orelse);
    Py_ssize_t n = asdl_seq_LEN(s->v.TryStar.handlers);

    USE_LABEL(c, except);

    ADDOP_JUMP(c, NO_LOCATION, SETUP_CLEANUP, cleanup);
    ADDOP(c, NO_LOCATION, PUSH_EXC_INFO);

    /* Runtime will push a block here, so we need to account for that */
    RETURN_IF_ERROR(
        compiler_push_fblock(c, loc, EXCEPTION_GROUP_HANDLER,
                             NO_LABEL, NO_LABEL, "except handler"));

    for (Py_ssize_t i = 0; i < n; i++) {
        excepthandler_ty handler = (excepthandler_ty)asdl_seq_GET(
            s->v.TryStar.handlers, i);
        location loc = LOC(handler);
        NEW_JUMP_TARGET_LABEL(c, next_except);
        except = next_except;
        NEW_JUMP_TARGET_LABEL(c, except_with_error);
        NEW_JUMP_TARGET_LABEL(c, no_match);
        if (i == 0) {
            /* create empty list for exceptions raised/reraise in the except* blocks */
            /*
               [orig]       BUILD_LIST
            */
            /* Create a copy of the original EG */
            /*
               [orig, []]   COPY 2
               [orig, [], exc]
            */
            ADDOP_I(c, loc, BUILD_LIST, 0);
            ADDOP_I(c, loc, COPY, 2);
        }
        if (handler->v.ExceptHandler.type) {
            VISIT(c, expr, handler->v.ExceptHandler.type);
            ADDOP(c, loc, CHECK_EG_MATCH);
            ADDOP_I(c, loc, COPY, 1);
            ADDOP_JUMP(c, loc, POP_JUMP_IF_NONE, no_match);
        }

        NEW_JUMP_TARGET_LABEL(c, cleanup_end);
        NEW_JUMP_TARGET_LABEL(c, cleanup_body);

        if (handler->v.ExceptHandler.name) {
            RETURN_IF_ERROR(
                compiler_nameop(c, loc, handler->v.ExceptHandler.name, Store));
        }
        else {
            ADDOP(c, loc, POP_TOP);  // match
        }

        /*
          try:
              # body
          except type as name:
              try:
                  # body
              finally:
                  name = None # in case body contains "del name"
                  del name
        */
        /* second try: */
        ADDOP_JUMP(c, loc, SETUP_CLEANUP, cleanup_end);

        USE_LABEL(c, cleanup_body);
        RETURN_IF_ERROR(
            compiler_push_fblock(c, loc, HANDLER_CLEANUP, cleanup_body,
                                 NO_LABEL, handler->v.ExceptHandler.name));

        /* second # body */
        VISIT_SEQ(c, stmt, handler->v.ExceptHandler.body);
        compiler_pop_fblock(c, HANDLER_CLEANUP, cleanup_body);
        /* name = None; del name; # artificial */
        ADDOP(c, NO_LOCATION, POP_BLOCK);
        if (handler->v.ExceptHandler.name) {
            ADDOP_LOAD_CONST(c, NO_LOCATION, Py_None);
            RETURN_IF_ERROR(
                compiler_nameop(c, NO_LOCATION, handler->v.ExceptHandler.name, Store));
            RETURN_IF_ERROR(
                compiler_nameop(c, NO_LOCATION, handler->v.ExceptHandler.name, Del));
        }
        ADDOP_JUMP(c, NO_LOCATION, JUMP, except);

        /* except: */
        USE_LABEL(c, cleanup_end);

        /* name = None; del name; # artificial */
        if (handler->v.ExceptHandler.name) {
            ADDOP_LOAD_CONST(c, NO_LOCATION, Py_None);
            RETURN_IF_ERROR(
                compiler_nameop(c, NO_LOCATION, handler->v.ExceptHandler.name, Store));
            RETURN_IF_ERROR(
                compiler_nameop(c, NO_LOCATION, handler->v.ExceptHandler.name, Del));
        }

        /* add exception raised to the res list */
        ADDOP_I(c, NO_LOCATION, LIST_APPEND, 3); // exc
        ADDOP(c, NO_LOCATION, POP_TOP); // lasti
        ADDOP_JUMP(c, NO_LOCATION, JUMP, except_with_error);

        USE_LABEL(c, except);
        ADDOP(c, NO_LOCATION, NOP);  // to hold a propagated location info
        ADDOP_JUMP(c, NO_LOCATION, JUMP, except_with_error);

        USE_LABEL(c, no_match);
        ADDOP(c, loc, POP_TOP);  // match (None)

        USE_LABEL(c, except_with_error);

        if (i == n - 1) {
            /* Add exc to the list (if not None it's the unhandled part of the EG) */
            ADDOP_I(c, NO_LOCATION, LIST_APPEND, 1);
            ADDOP_JUMP(c, NO_LOCATION, JUMP, reraise_star);
        }
    }
    /* artificial */
    compiler_pop_fblock(c, EXCEPTION_GROUP_HANDLER, NO_LABEL);
    NEW_JUMP_TARGET_LABEL(c, reraise);

    USE_LABEL(c, reraise_star);
    ADDOP_I(c, NO_LOCATION, CALL_INTRINSIC_2, INTRINSIC_PREP_RERAISE_STAR);
    ADDOP_I(c, NO_LOCATION, COPY, 1);
    ADDOP_JUMP(c, NO_LOCATION, POP_JUMP_IF_NOT_NONE, reraise);

    /* Nothing to reraise */
    ADDOP(c, NO_LOCATION, POP_TOP);
    ADDOP(c, NO_LOCATION, POP_BLOCK);
    ADDOP(c, NO_LOCATION, POP_EXCEPT);
    ADDOP_JUMP(c, NO_LOCATION, JUMP, end);

    USE_LABEL(c, reraise);
    ADDOP(c, NO_LOCATION, POP_BLOCK);
    ADDOP_I(c, NO_LOCATION, SWAP, 2);
    ADDOP(c, NO_LOCATION, POP_EXCEPT);
    ADDOP_I(c, NO_LOCATION, RERAISE, 0);

    USE_LABEL(c, cleanup);
    POP_EXCEPT_AND_RERAISE(c, NO_LOCATION);

    USE_LABEL(c, orelse);
    VISIT_SEQ(c, stmt, s->v.TryStar.orelse);

    USE_LABEL(c, end);
    return SUCCESS;
}

static int
compiler_try(struct compiler *c, stmt_ty s) {
    if (s->v.Try.finalbody && asdl_seq_LEN(s->v.Try.finalbody))
        return compiler_try_finally(c, s);
    else
        return compiler_try_except(c, s);
}

static int
compiler_try_star(struct compiler *c, stmt_ty s)
{
    if (s->v.TryStar.finalbody && asdl_seq_LEN(s->v.TryStar.finalbody)) {
        return compiler_try_star_finally(c, s);
    }
    else {
        return compiler_try_star_except(c, s);
    }
}

static int
compiler_import_as(struct compiler *c, location loc,
                   identifier name, identifier asname)
{
    /* The IMPORT_NAME opcode was already generated.  This function
       merely needs to bind the result to a name.

       If there is a dot in name, we need to split it and emit a
       IMPORT_FROM for each name.
    */
    Py_ssize_t len = PyUnicode_GET_LENGTH(name);
    Py_ssize_t dot = PyUnicode_FindChar(name, '.', 0, len, 1);
    if (dot == -2) {
        return ERROR;
    }
    if (dot != -1) {
        /* Consume the base module name to get the first attribute */
        while (1) {
            Py_ssize_t pos = dot + 1;
            PyObject *attr;
            dot = PyUnicode_FindChar(name, '.', pos, len, 1);
            if (dot == -2) {
                return ERROR;
            }
            attr = PyUnicode_Substring(name, pos, (dot != -1) ? dot : len);
            if (!attr) {
                return ERROR;
            }
            ADDOP_N(c, loc, IMPORT_FROM, attr, names);
            if (dot == -1) {
                break;
            }
            ADDOP_I(c, loc, SWAP, 2);
            ADDOP(c, loc, POP_TOP);
        }
        RETURN_IF_ERROR(compiler_nameop(c, loc, asname, Store));
        ADDOP(c, loc, POP_TOP);
        return SUCCESS;
    }
    return compiler_nameop(c, loc, asname, Store);
}

static int
compiler_import(struct compiler *c, stmt_ty s)
{
    location loc = LOC(s);
    /* The Import node stores a module name like a.b.c as a single
       string.  This is convenient for all cases except
         import a.b.c as d
       where we need to parse that string to extract the individual
       module names.
       XXX Perhaps change the representation to make this case simpler?
     */
    Py_ssize_t i, n = asdl_seq_LEN(s->v.Import.names);

    PyObject *zero = _PyLong_GetZero();  // borrowed reference
    for (i = 0; i < n; i++) {
        alias_ty alias = (alias_ty)asdl_seq_GET(s->v.Import.names, i);
        int r;

        ADDOP_LOAD_CONST(c, loc, zero);
        ADDOP_LOAD_CONST(c, loc, Py_None);
        ADDOP_NAME(c, loc, IMPORT_NAME, alias->name, names);

        if (alias->asname) {
            r = compiler_import_as(c, loc, alias->name, alias->asname);
            RETURN_IF_ERROR(r);
        }
        else {
            identifier tmp = alias->name;
            Py_ssize_t dot = PyUnicode_FindChar(
                alias->name, '.', 0, PyUnicode_GET_LENGTH(alias->name), 1);
            if (dot != -1) {
                tmp = PyUnicode_Substring(alias->name, 0, dot);
                if (tmp == NULL) {
                    return ERROR;
                }
            }
            r = compiler_nameop(c, loc, tmp, Store);
            if (dot != -1) {
                Py_DECREF(tmp);
            }
            RETURN_IF_ERROR(r);
        }
    }
    return SUCCESS;
}

static int
compiler_from_import(struct compiler *c, stmt_ty s)
{
    Py_ssize_t n = asdl_seq_LEN(s->v.ImportFrom.names);

    ADDOP_LOAD_CONST_NEW(c, LOC(s), PyLong_FromLong(s->v.ImportFrom.level));

    PyObject *names = PyTuple_New(n);
    if (!names) {
        return ERROR;
    }

    /* build up the names */
    for (Py_ssize_t i = 0; i < n; i++) {
        alias_ty alias = (alias_ty)asdl_seq_GET(s->v.ImportFrom.names, i);
        PyTuple_SET_ITEM(names, i, Py_NewRef(alias->name));
    }

    if (location_is_after(LOC(s), c->c_future.ff_location) &&
        s->v.ImportFrom.module &&
        _PyUnicode_EqualToASCIIString(s->v.ImportFrom.module, "__future__"))
    {
        Py_DECREF(names);
        return compiler_error(c, LOC(s), "from __future__ imports must occur "
                              "at the beginning of the file");
    }
    ADDOP_LOAD_CONST_NEW(c, LOC(s), names);

    if (s->v.ImportFrom.module) {
        ADDOP_NAME(c, LOC(s), IMPORT_NAME, s->v.ImportFrom.module, names);
    }
    else {
        _Py_DECLARE_STR(empty, "");
        ADDOP_NAME(c, LOC(s), IMPORT_NAME, &_Py_STR(empty), names);
    }
    for (Py_ssize_t i = 0; i < n; i++) {
        alias_ty alias = (alias_ty)asdl_seq_GET(s->v.ImportFrom.names, i);
        identifier store_name;

        if (i == 0 && PyUnicode_READ_CHAR(alias->name, 0) == '*') {
            assert(n == 1);
            ADDOP_I(c, LOC(s), CALL_INTRINSIC_1, INTRINSIC_IMPORT_STAR);
            ADDOP(c, NO_LOCATION, POP_TOP);
            return SUCCESS;
        }

        ADDOP_NAME(c, LOC(s), IMPORT_FROM, alias->name, names);
        store_name = alias->name;
        if (alias->asname) {
            store_name = alias->asname;
        }

        RETURN_IF_ERROR(compiler_nameop(c, LOC(s), store_name, Store));
    }
    /* remove imported module */
    ADDOP(c, LOC(s), POP_TOP);
    return SUCCESS;
}

static int
compiler_assert(struct compiler *c, stmt_ty s)
{
    /* Always emit a warning if the test is a non-zero length tuple */
    if ((s->v.Assert.test->kind == Tuple_kind &&
        asdl_seq_LEN(s->v.Assert.test->v.Tuple.elts) > 0) ||
        (s->v.Assert.test->kind == Constant_kind &&
         PyTuple_Check(s->v.Assert.test->v.Constant.value) &&
         PyTuple_Size(s->v.Assert.test->v.Constant.value) > 0))
    {
        RETURN_IF_ERROR(
            compiler_warn(c, LOC(s), "assertion is always true, "
                                     "perhaps remove parentheses?"));
    }
    if (c->c_optimize) {
        return SUCCESS;
    }
    NEW_JUMP_TARGET_LABEL(c, end);
    RETURN_IF_ERROR(compiler_jump_if(c, LOC(s), s->v.Assert.test, end, 1));
    ADDOP(c, LOC(s), LOAD_ASSERTION_ERROR);
    if (s->v.Assert.msg) {
        VISIT(c, expr, s->v.Assert.msg);
        ADDOP_I(c, LOC(s), CALL, 0);
    }
    ADDOP_I(c, LOC(s->v.Assert.test), RAISE_VARARGS, 1);

    USE_LABEL(c, end);
    return SUCCESS;
}

static int
compiler_stmt_expr(struct compiler *c, location loc, expr_ty value)
{
    if (c->c_interactive && c->c_nestlevel <= 1) {
        VISIT(c, expr, value);
        ADDOP_I(c, loc, CALL_INTRINSIC_1, INTRINSIC_PRINT);
        ADDOP(c, NO_LOCATION, POP_TOP);
        return SUCCESS;
    }

    if (value->kind == Constant_kind) {
        /* ignore constant statement */
        ADDOP(c, loc, NOP);
        return SUCCESS;
    }

    VISIT(c, expr, value);
    ADDOP(c, NO_LOCATION, POP_TOP); /* artificial */
    return SUCCESS;
}

static int
compiler_visit_stmt(struct compiler *c, stmt_ty s)
{

    switch (s->kind) {
    case FunctionDef_kind:
        return compiler_function(c, s, 0);
    case ClassDef_kind:
        return compiler_class(c, s);
    case TypeAlias_kind:
        return compiler_typealias(c, s);
    case Return_kind:
        return compiler_return(c, s);
    case Delete_kind:
        VISIT_SEQ(c, expr, s->v.Delete.targets)
        break;
    case Assign_kind:
    {
        Py_ssize_t n = asdl_seq_LEN(s->v.Assign.targets);
        VISIT(c, expr, s->v.Assign.value);
        for (Py_ssize_t i = 0; i < n; i++) {
            if (i < n - 1) {
                ADDOP_I(c, LOC(s), COPY, 1);
            }
            VISIT(c, expr,
                  (expr_ty)asdl_seq_GET(s->v.Assign.targets, i));
        }
        break;
    }
    case AugAssign_kind:
        return compiler_augassign(c, s);
    case AnnAssign_kind:
        return compiler_annassign(c, s);
    case For_kind:
        return compiler_for(c, s);
    case While_kind:
        return compiler_while(c, s);
    case If_kind:
        return compiler_if(c, s);
    case Match_kind:
        return compiler_match(c, s);
    case Raise_kind:
    {
        Py_ssize_t n = 0;
        if (s->v.Raise.exc) {
            VISIT(c, expr, s->v.Raise.exc);
            n++;
            if (s->v.Raise.cause) {
                VISIT(c, expr, s->v.Raise.cause);
                n++;
            }
        }
        ADDOP_I(c, LOC(s), RAISE_VARARGS, (int)n);
        break;
    }
    case Try_kind:
        return compiler_try(c, s);
    case TryStar_kind:
        return compiler_try_star(c, s);
    case Assert_kind:
        return compiler_assert(c, s);
    case Import_kind:
        return compiler_import(c, s);
    case ImportFrom_kind:
        return compiler_from_import(c, s);
    case Global_kind:
    case Nonlocal_kind:
        break;
    case Expr_kind:
    {
        return compiler_stmt_expr(c, LOC(s), s->v.Expr.value);
    }
    case Pass_kind:
    {
        ADDOP(c, LOC(s), NOP);
        break;
    }
    case Break_kind:
    {
        return compiler_break(c, LOC(s));
    }
    case Continue_kind:
    {
        return compiler_continue(c, LOC(s));
    }
    case With_kind:
        return compiler_with(c, s, 0);
    case AsyncFunctionDef_kind:
        return compiler_function(c, s, 1);
    case AsyncWith_kind:
        return compiler_async_with(c, s, 0);
    case AsyncFor_kind:
        return compiler_async_for(c, s);
    }

    return SUCCESS;
}

static int
unaryop(unaryop_ty op)
{
    switch (op) {
    case Invert:
        return UNARY_INVERT;
    case Not:
        return UNARY_NOT;
    case USub:
        return UNARY_NEGATIVE;
    default:
        PyErr_Format(PyExc_SystemError,
            "unary op %d should not be possible", op);
        return 0;
    }
}

static int
addop_binary(struct compiler *c, location loc, operator_ty binop,
             bool inplace)
{
    int oparg;
    switch (binop) {
        case Add:
            oparg = inplace ? NB_INPLACE_ADD : NB_ADD;
            break;
        case Sub:
            oparg = inplace ? NB_INPLACE_SUBTRACT : NB_SUBTRACT;
            break;
        case Mult:
            oparg = inplace ? NB_INPLACE_MULTIPLY : NB_MULTIPLY;
            break;
        case MatMult:
            oparg = inplace ? NB_INPLACE_MATRIX_MULTIPLY : NB_MATRIX_MULTIPLY;
            break;
        case Div:
            oparg = inplace ? NB_INPLACE_TRUE_DIVIDE : NB_TRUE_DIVIDE;
            break;
        case Mod:
            oparg = inplace ? NB_INPLACE_REMAINDER : NB_REMAINDER;
            break;
        case Pow:
            oparg = inplace ? NB_INPLACE_POWER : NB_POWER;
            break;
        case LShift:
            oparg = inplace ? NB_INPLACE_LSHIFT : NB_LSHIFT;
            break;
        case RShift:
            oparg = inplace ? NB_INPLACE_RSHIFT : NB_RSHIFT;
            break;
        case BitOr:
            oparg = inplace ? NB_INPLACE_OR : NB_OR;
            break;
        case BitXor:
            oparg = inplace ? NB_INPLACE_XOR : NB_XOR;
            break;
        case BitAnd:
            oparg = inplace ? NB_INPLACE_AND : NB_AND;
            break;
        case FloorDiv:
            oparg = inplace ? NB_INPLACE_FLOOR_DIVIDE : NB_FLOOR_DIVIDE;
            break;
        default:
            PyErr_Format(PyExc_SystemError, "%s op %d should not be possible",
                         inplace ? "inplace" : "binary", binop);
            return ERROR;
    }
    ADDOP_I(c, loc, BINARY_OP, oparg);
    return SUCCESS;
}


static int
addop_yield(struct compiler *c, location loc) {
    if (c->u->u_ste->ste_generator && c->u->u_ste->ste_coroutine) {
        ADDOP_I(c, loc, CALL_INTRINSIC_1, INTRINSIC_ASYNC_GEN_WRAP);
    }
    ADDOP_I(c, loc, YIELD_VALUE, 0);
    ADDOP_I(c, loc, RESUME, 1);
    return SUCCESS;
}

static int
compiler_nameop(struct compiler *c, location loc,
                identifier name, expr_context_ty ctx)
{
    int op, scope;
    Py_ssize_t arg;
    enum { OP_FAST, OP_GLOBAL, OP_DEREF, OP_NAME } optype;

    PyObject *dict = c->u->u_metadata.u_names;
    PyObject *mangled;

    assert(!_PyUnicode_EqualToASCIIString(name, "None") &&
           !_PyUnicode_EqualToASCIIString(name, "True") &&
           !_PyUnicode_EqualToASCIIString(name, "False"));

    if (forbidden_name(c, loc, name, ctx)) {
        return ERROR;
    }

    mangled = _Py_Mangle(c->u->u_private, name);
    if (!mangled) {
        return ERROR;
    }

    op = 0;
    optype = OP_NAME;
    scope = _PyST_GetScope(c->u->u_ste, mangled);
    switch (scope) {
    case FREE:
        dict = c->u->u_metadata.u_freevars;
        optype = OP_DEREF;
        break;
    case CELL:
        dict = c->u->u_metadata.u_cellvars;
        optype = OP_DEREF;
        break;
    case LOCAL:
        if (_PyST_IsFunctionLike(c->u->u_ste) ||
                (PyDict_GetItem(c->u->u_metadata.u_fasthidden, mangled) == Py_True))
            optype = OP_FAST;
        break;
    case GLOBAL_IMPLICIT:
        if (_PyST_IsFunctionLike(c->u->u_ste))
            optype = OP_GLOBAL;
        break;
    case GLOBAL_EXPLICIT:
        optype = OP_GLOBAL;
        break;
    default:
        /* scope can be 0 */
        break;
    }

    /* XXX Leave assert here, but handle __doc__ and the like better */
    assert(scope || PyUnicode_READ_CHAR(name, 0) == '_');

    switch (optype) {
    case OP_DEREF:
        switch (ctx) {
        case Load:
            if (c->u->u_ste->ste_type == ClassBlock && !c->u->u_in_inlined_comp) {
                op = LOAD_FROM_DICT_OR_DEREF;
                // First load the locals
                if (codegen_addop_noarg(INSTR_SEQUENCE(c), LOAD_LOCALS, loc) < 0) {
                    return ERROR;
                }
            }
            else if (c->u->u_ste->ste_can_see_class_scope) {
                op = LOAD_FROM_DICT_OR_DEREF;
                // First load the classdict
                if (compiler_addop_o(c->u, loc, LOAD_DEREF,
                                     c->u->u_metadata.u_freevars, &_Py_ID(__classdict__)) < 0) {
                    return ERROR;
                }
            }
            else {
                op = LOAD_DEREF;
            }
            break;
        case Store: op = STORE_DEREF; break;
        case Del: op = DELETE_DEREF; break;
        }
        break;
    case OP_FAST:
        switch (ctx) {
        case Load: op = LOAD_FAST; break;
        case Store: op = STORE_FAST; break;
        case Del: op = DELETE_FAST; break;
        }
        ADDOP_N(c, loc, op, mangled, varnames);
        return SUCCESS;
    case OP_GLOBAL:
        switch (ctx) {
        case Load:
            if (c->u->u_ste->ste_can_see_class_scope && scope == GLOBAL_IMPLICIT) {
                op = LOAD_FROM_DICT_OR_GLOBALS;
                // First load the classdict
                if (compiler_addop_o(c->u, loc, LOAD_DEREF,
                                     c->u->u_metadata.u_freevars, &_Py_ID(__classdict__)) < 0) {
                    return ERROR;
                }
            } else {
                op = LOAD_GLOBAL;
            }
            break;
        case Store: op = STORE_GLOBAL; break;
        case Del: op = DELETE_GLOBAL; break;
        }
        break;
    case OP_NAME:
        switch (ctx) {
        case Load:
            op = (c->u->u_ste->ste_type == ClassBlock
                    && c->u->u_in_inlined_comp)
                ? LOAD_GLOBAL
                : LOAD_NAME;
            break;
        case Store: op = STORE_NAME; break;
        case Del: op = DELETE_NAME; break;
        }
        break;
    }

    assert(op);
    arg = dict_add_o(dict, mangled);
    Py_DECREF(mangled);
    if (arg < 0) {
        return ERROR;
    }
    if (op == LOAD_GLOBAL) {
        arg <<= 1;
    }
    return codegen_addop_i(INSTR_SEQUENCE(c), op, arg, loc);
}

static int
compiler_boolop(struct compiler *c, expr_ty e)
{
    int jumpi;
    Py_ssize_t i, n;
    asdl_expr_seq *s;

    location loc = LOC(e);
    assert(e->kind == BoolOp_kind);
    if (e->v.BoolOp.op == And)
        jumpi = POP_JUMP_IF_FALSE;
    else
        jumpi = POP_JUMP_IF_TRUE;
    NEW_JUMP_TARGET_LABEL(c, end);
    s = e->v.BoolOp.values;
    n = asdl_seq_LEN(s) - 1;
    assert(n >= 0);
    for (i = 0; i < n; ++i) {
        VISIT(c, expr, (expr_ty)asdl_seq_GET(s, i));
        ADDOP_I(c, loc, COPY, 1);
        ADDOP_JUMP(c, loc, jumpi, end);
        ADDOP(c, loc, POP_TOP);
    }
    VISIT(c, expr, (expr_ty)asdl_seq_GET(s, n));

    USE_LABEL(c, end);
    return SUCCESS;
}

static int
starunpack_helper(struct compiler *c, location loc,
                  asdl_expr_seq *elts, int pushed,
                  int build, int add, int extend, int tuple)
{
    Py_ssize_t n = asdl_seq_LEN(elts);
    if (n > 2 && are_all_items_const(elts, 0, n)) {
        PyObject *folded = PyTuple_New(n);
        if (folded == NULL) {
            return ERROR;
        }
        PyObject *val;
        for (Py_ssize_t i = 0; i < n; i++) {
            val = ((expr_ty)asdl_seq_GET(elts, i))->v.Constant.value;
            PyTuple_SET_ITEM(folded, i, Py_NewRef(val));
        }
        if (tuple && !pushed) {
            ADDOP_LOAD_CONST_NEW(c, loc, folded);
        } else {
            if (add == SET_ADD) {
                Py_SETREF(folded, PyFrozenSet_New(folded));
                if (folded == NULL) {
                    return ERROR;
                }
            }
            ADDOP_I(c, loc, build, pushed);
            ADDOP_LOAD_CONST_NEW(c, loc, folded);
            ADDOP_I(c, loc, extend, 1);
            if (tuple) {
                ADDOP_I(c, loc, CALL_INTRINSIC_1, INTRINSIC_LIST_TO_TUPLE);
            }
        }
        return SUCCESS;
    }

    int big = n+pushed > STACK_USE_GUIDELINE;
    int seen_star = 0;
    for (Py_ssize_t i = 0; i < n; i++) {
        expr_ty elt = asdl_seq_GET(elts, i);
        if (elt->kind == Starred_kind) {
            seen_star = 1;
            break;
        }
    }
    if (!seen_star && !big) {
        for (Py_ssize_t i = 0; i < n; i++) {
            expr_ty elt = asdl_seq_GET(elts, i);
            VISIT(c, expr, elt);
        }
        if (tuple) {
            ADDOP_I(c, loc, BUILD_TUPLE, n+pushed);
        } else {
            ADDOP_I(c, loc, build, n+pushed);
        }
        return SUCCESS;
    }
    int sequence_built = 0;
    if (big) {
        ADDOP_I(c, loc, build, pushed);
        sequence_built = 1;
    }
    for (Py_ssize_t i = 0; i < n; i++) {
        expr_ty elt = asdl_seq_GET(elts, i);
        if (elt->kind == Starred_kind) {
            if (sequence_built == 0) {
                ADDOP_I(c, loc, build, i+pushed);
                sequence_built = 1;
            }
            VISIT(c, expr, elt->v.Starred.value);
            ADDOP_I(c, loc, extend, 1);
        }
        else {
            VISIT(c, expr, elt);
            if (sequence_built) {
                ADDOP_I(c, loc, add, 1);
            }
        }
    }
    assert(sequence_built);
    if (tuple) {
        ADDOP_I(c, loc, CALL_INTRINSIC_1, INTRINSIC_LIST_TO_TUPLE);
    }
    return SUCCESS;
}

static int
unpack_helper(struct compiler *c, location loc, asdl_expr_seq *elts)
{
    Py_ssize_t n = asdl_seq_LEN(elts);
    int seen_star = 0;
    for (Py_ssize_t i = 0; i < n; i++) {
        expr_ty elt = asdl_seq_GET(elts, i);
        if (elt->kind == Starred_kind && !seen_star) {
            if ((i >= (1 << 8)) ||
                (n-i-1 >= (INT_MAX >> 8))) {
                return compiler_error(c, loc,
                    "too many expressions in "
                    "star-unpacking assignment");
            }
            ADDOP_I(c, loc, UNPACK_EX, (i + ((n-i-1) << 8)));
            seen_star = 1;
        }
        else if (elt->kind == Starred_kind) {
            return compiler_error(c, loc,
                "multiple starred expressions in assignment");
        }
    }
    if (!seen_star) {
        ADDOP_I(c, loc, UNPACK_SEQUENCE, n);
    }
    return SUCCESS;
}

static int
assignment_helper(struct compiler *c, location loc, asdl_expr_seq *elts)
{
    Py_ssize_t n = asdl_seq_LEN(elts);
    RETURN_IF_ERROR(unpack_helper(c, loc, elts));
    for (Py_ssize_t i = 0; i < n; i++) {
        expr_ty elt = asdl_seq_GET(elts, i);
        VISIT(c, expr, elt->kind != Starred_kind ? elt : elt->v.Starred.value);
    }
    return SUCCESS;
}

static int
compiler_list(struct compiler *c, expr_ty e)
{
    location loc = LOC(e);
    asdl_expr_seq *elts = e->v.List.elts;
    if (e->v.List.ctx == Store) {
        return assignment_helper(c, loc, elts);
    }
    else if (e->v.List.ctx == Load) {
        return starunpack_helper(c, loc, elts, 0,
                                 BUILD_LIST, LIST_APPEND, LIST_EXTEND, 0);
    }
    else {
        VISIT_SEQ(c, expr, elts);
    }
    return SUCCESS;
}

static int
compiler_tuple(struct compiler *c, expr_ty e)
{
    location loc = LOC(e);
    asdl_expr_seq *elts = e->v.Tuple.elts;
    if (e->v.Tuple.ctx == Store) {
        return assignment_helper(c, loc, elts);
    }
    else if (e->v.Tuple.ctx == Load) {
        return starunpack_helper(c, loc, elts, 0,
                                 BUILD_LIST, LIST_APPEND, LIST_EXTEND, 1);
    }
    else {
        VISIT_SEQ(c, expr, elts);
    }
    return SUCCESS;
}

static int
compiler_set(struct compiler *c, expr_ty e)
{
    location loc = LOC(e);
    return starunpack_helper(c, loc, e->v.Set.elts, 0,
                             BUILD_SET, SET_ADD, SET_UPDATE, 0);
}

static bool
are_all_items_const(asdl_expr_seq *seq, Py_ssize_t begin, Py_ssize_t end)
{
    for (Py_ssize_t i = begin; i < end; i++) {
        expr_ty key = (expr_ty)asdl_seq_GET(seq, i);
        if (key == NULL || key->kind != Constant_kind) {
            return false;
        }
    }
    return true;
}

static int
compiler_subdict(struct compiler *c, expr_ty e, Py_ssize_t begin, Py_ssize_t end)
{
    Py_ssize_t i, n = end - begin;
    PyObject *keys, *key;
    int big = n*2 > STACK_USE_GUIDELINE;
    location loc = LOC(e);
    if (n > 1 && !big && are_all_items_const(e->v.Dict.keys, begin, end)) {
        for (i = begin; i < end; i++) {
            VISIT(c, expr, (expr_ty)asdl_seq_GET(e->v.Dict.values, i));
        }
        keys = PyTuple_New(n);
        if (keys == NULL) {
            return SUCCESS;
        }
        for (i = begin; i < end; i++) {
            key = ((expr_ty)asdl_seq_GET(e->v.Dict.keys, i))->v.Constant.value;
            PyTuple_SET_ITEM(keys, i - begin, Py_NewRef(key));
        }
        ADDOP_LOAD_CONST_NEW(c, loc, keys);
        ADDOP_I(c, loc, BUILD_CONST_KEY_MAP, n);
        return SUCCESS;
    }
    if (big) {
        ADDOP_I(c, loc, BUILD_MAP, 0);
    }
    for (i = begin; i < end; i++) {
        VISIT(c, expr, (expr_ty)asdl_seq_GET(e->v.Dict.keys, i));
        VISIT(c, expr, (expr_ty)asdl_seq_GET(e->v.Dict.values, i));
        if (big) {
            ADDOP_I(c, loc, MAP_ADD, 1);
        }
    }
    if (!big) {
        ADDOP_I(c, loc, BUILD_MAP, n);
    }
    return SUCCESS;
}

static int
compiler_dict(struct compiler *c, expr_ty e)
{
    location loc = LOC(e);
    Py_ssize_t i, n, elements;
    int have_dict;
    int is_unpacking = 0;
    n = asdl_seq_LEN(e->v.Dict.values);
    have_dict = 0;
    elements = 0;
    for (i = 0; i < n; i++) {
        is_unpacking = (expr_ty)asdl_seq_GET(e->v.Dict.keys, i) == NULL;
        if (is_unpacking) {
            if (elements) {
                RETURN_IF_ERROR(compiler_subdict(c, e, i - elements, i));
                if (have_dict) {
                    ADDOP_I(c, loc, DICT_UPDATE, 1);
                }
                have_dict = 1;
                elements = 0;
            }
            if (have_dict == 0) {
                ADDOP_I(c, loc, BUILD_MAP, 0);
                have_dict = 1;
            }
            VISIT(c, expr, (expr_ty)asdl_seq_GET(e->v.Dict.values, i));
            ADDOP_I(c, loc, DICT_UPDATE, 1);
        }
        else {
            if (elements*2 > STACK_USE_GUIDELINE) {
                RETURN_IF_ERROR(compiler_subdict(c, e, i - elements, i + 1));
                if (have_dict) {
                    ADDOP_I(c, loc, DICT_UPDATE, 1);
                }
                have_dict = 1;
                elements = 0;
            }
            else {
                elements++;
            }
        }
    }
    if (elements) {
        RETURN_IF_ERROR(compiler_subdict(c, e, n - elements, n));
        if (have_dict) {
            ADDOP_I(c, loc, DICT_UPDATE, 1);
        }
        have_dict = 1;
    }
    if (!have_dict) {
        ADDOP_I(c, loc, BUILD_MAP, 0);
    }
    return SUCCESS;
}

static int
compiler_compare(struct compiler *c, expr_ty e)
{
    location loc = LOC(e);
    Py_ssize_t i, n;

    RETURN_IF_ERROR(check_compare(c, e));
    VISIT(c, expr, e->v.Compare.left);
    assert(asdl_seq_LEN(e->v.Compare.ops) > 0);
    n = asdl_seq_LEN(e->v.Compare.ops) - 1;
    if (n == 0) {
        VISIT(c, expr, (expr_ty)asdl_seq_GET(e->v.Compare.comparators, 0));
        ADDOP_COMPARE(c, loc, asdl_seq_GET(e->v.Compare.ops, 0));
    }
    else {
        NEW_JUMP_TARGET_LABEL(c, cleanup);
        for (i = 0; i < n; i++) {
            VISIT(c, expr,
                (expr_ty)asdl_seq_GET(e->v.Compare.comparators, i));
            ADDOP_I(c, loc, SWAP, 2);
            ADDOP_I(c, loc, COPY, 2);
            ADDOP_COMPARE(c, loc, asdl_seq_GET(e->v.Compare.ops, i));
            ADDOP_I(c, loc, COPY, 1);
            ADDOP_JUMP(c, loc, POP_JUMP_IF_FALSE, cleanup);
            ADDOP(c, loc, POP_TOP);
        }
        VISIT(c, expr, (expr_ty)asdl_seq_GET(e->v.Compare.comparators, n));
        ADDOP_COMPARE(c, loc, asdl_seq_GET(e->v.Compare.ops, n));
        NEW_JUMP_TARGET_LABEL(c, end);
        ADDOP_JUMP(c, NO_LOCATION, JUMP, end);

        USE_LABEL(c, cleanup);
        ADDOP_I(c, loc, SWAP, 2);
        ADDOP(c, loc, POP_TOP);

        USE_LABEL(c, end);
    }
    return SUCCESS;
}

static PyTypeObject *
infer_type(expr_ty e)
{
    switch (e->kind) {
    case Tuple_kind:
        return &PyTuple_Type;
    case List_kind:
    case ListComp_kind:
        return &PyList_Type;
    case Dict_kind:
    case DictComp_kind:
        return &PyDict_Type;
    case Set_kind:
    case SetComp_kind:
        return &PySet_Type;
    case GeneratorExp_kind:
        return &PyGen_Type;
    case Lambda_kind:
        return &PyFunction_Type;
    case JoinedStr_kind:
    case FormattedValue_kind:
        return &PyUnicode_Type;
    case Constant_kind:
        return Py_TYPE(e->v.Constant.value);
    default:
        return NULL;
    }
}

static int
check_caller(struct compiler *c, expr_ty e)
{
    switch (e->kind) {
    case Constant_kind:
    case Tuple_kind:
    case List_kind:
    case ListComp_kind:
    case Dict_kind:
    case DictComp_kind:
    case Set_kind:
    case SetComp_kind:
    case GeneratorExp_kind:
    case JoinedStr_kind:
    case FormattedValue_kind: {
        location loc = LOC(e);
        return compiler_warn(c, loc, "'%.200s' object is not callable; "
                                     "perhaps you missed a comma?",
                                     infer_type(e)->tp_name);
    }
    default:
        return SUCCESS;
    }
}

static int
check_subscripter(struct compiler *c, expr_ty e)
{
    PyObject *v;

    switch (e->kind) {
    case Constant_kind:
        v = e->v.Constant.value;
        if (!(v == Py_None || v == Py_Ellipsis ||
              PyLong_Check(v) || PyFloat_Check(v) || PyComplex_Check(v) ||
              PyAnySet_Check(v)))
        {
            return SUCCESS;
        }
        /* fall through */
    case Set_kind:
    case SetComp_kind:
    case GeneratorExp_kind:
    case Lambda_kind: {
        location loc = LOC(e);
        return compiler_warn(c, loc, "'%.200s' object is not subscriptable; "
                                     "perhaps you missed a comma?",
                                     infer_type(e)->tp_name);
    }
    default:
        return SUCCESS;
    }
}

static int
check_index(struct compiler *c, expr_ty e, expr_ty s)
{
    PyObject *v;

    PyTypeObject *index_type = infer_type(s);
    if (index_type == NULL
        || PyType_FastSubclass(index_type, Py_TPFLAGS_LONG_SUBCLASS)
        || index_type == &PySlice_Type) {
        return SUCCESS;
    }

    switch (e->kind) {
    case Constant_kind:
        v = e->v.Constant.value;
        if (!(PyUnicode_Check(v) || PyBytes_Check(v) || PyTuple_Check(v))) {
            return SUCCESS;
        }
        /* fall through */
    case Tuple_kind:
    case List_kind:
    case ListComp_kind:
    case JoinedStr_kind:
    case FormattedValue_kind: {
        location loc = LOC(e);
        return compiler_warn(c, loc, "%.200s indices must be integers "
                                     "or slices, not %.200s; "
                                     "perhaps you missed a comma?",
                                     infer_type(e)->tp_name,
                                     index_type->tp_name);
    }
    default:
        return SUCCESS;
    }
}

static int
is_import_originated(struct compiler *c, expr_ty e)
{
    /* Check whether the global scope has an import named
     e, if it is a Name object. For not traversing all the
     scope stack every time this function is called, it will
     only check the global scope to determine whether something
     is imported or not. */

    if (e->kind != Name_kind) {
        return 0;
    }

    long flags = _PyST_GetSymbol(c->c_st->st_top, e->v.Name.id);
    return flags & DEF_IMPORT;
}

static int
can_optimize_super_call(struct compiler *c, expr_ty attr)
{
    expr_ty e = attr->v.Attribute.value;
    if (e->kind != Call_kind ||
        e->v.Call.func->kind != Name_kind ||
        !_PyUnicode_EqualToASCIIString(e->v.Call.func->v.Name.id, "super") ||
        _PyUnicode_EqualToASCIIString(attr->v.Attribute.attr, "__class__") ||
        asdl_seq_LEN(e->v.Call.keywords) != 0) {
        return 0;
    }
    Py_ssize_t num_args = asdl_seq_LEN(e->v.Call.args);

    PyObject *super_name = e->v.Call.func->v.Name.id;
    // detect statically-visible shadowing of 'super' name
    int scope = _PyST_GetScope(c->u->u_ste, super_name);
    if (scope != GLOBAL_IMPLICIT) {
        return 0;
    }
    scope = _PyST_GetScope(c->c_st->st_top, super_name);
    if (scope != 0) {
        return 0;
    }

    if (num_args == 2) {
        for (Py_ssize_t i = 0; i < num_args; i++) {
            expr_ty elt = asdl_seq_GET(e->v.Call.args, i);
            if (elt->kind == Starred_kind) {
                return 0;
            }
        }
        // exactly two non-starred args; we can just load
        // the provided args
        return 1;
    }

    if (num_args != 0) {
        return 0;
    }
    // we need the following for zero-arg super():

    // enclosing function should have at least one argument
    if (c->u->u_metadata.u_argcount == 0 &&
        c->u->u_metadata.u_posonlyargcount == 0) {
        return 0;
    }
    // __class__ cell should be available
    if (get_ref_type(c, &_Py_ID(__class__)) == FREE) {
        return 1;
    }
    return 0;
}

static int
load_args_for_super(struct compiler *c, expr_ty e) {
    location loc = LOC(e);

    // load super() global
    PyObject *super_name = e->v.Call.func->v.Name.id;
    RETURN_IF_ERROR(compiler_nameop(c, LOC(e->v.Call.func), super_name, Load));

    if (asdl_seq_LEN(e->v.Call.args) == 2) {
        VISIT(c, expr, asdl_seq_GET(e->v.Call.args, 0));
        VISIT(c, expr, asdl_seq_GET(e->v.Call.args, 1));
        return SUCCESS;
    }

    // load __class__ cell
    PyObject *name = &_Py_ID(__class__);
    assert(get_ref_type(c, name) == FREE);
    RETURN_IF_ERROR(compiler_nameop(c, loc, name, Load));

    // load self (first argument)
    Py_ssize_t i = 0;
    PyObject *key, *value;
    if (!PyDict_Next(c->u->u_metadata.u_varnames, &i, &key, &value)) {
        return ERROR;
    }
    RETURN_IF_ERROR(compiler_nameop(c, loc, key, Load));

    return SUCCESS;
}

// If an attribute access spans multiple lines, update the current start
// location to point to the attribute name.
static location
update_start_location_to_match_attr(struct compiler *c, location loc,
                                    expr_ty attr)
{
    assert(attr->kind == Attribute_kind);
    if (loc.lineno != attr->end_lineno) {
        loc.lineno = attr->end_lineno;
        int len = (int)PyUnicode_GET_LENGTH(attr->v.Attribute.attr);
        if (len <= attr->end_col_offset) {
            loc.col_offset = attr->end_col_offset - len;
        }
        else {
            // GH-94694: Somebody's compiling weird ASTs. Just drop the columns:
            loc.col_offset = -1;
            loc.end_col_offset = -1;
        }
        // Make sure the end position still follows the start position, even for
        // weird ASTs:
        loc.end_lineno = Py_MAX(loc.lineno, loc.end_lineno);
        if (loc.lineno == loc.end_lineno) {
            loc.end_col_offset = Py_MAX(loc.col_offset, loc.end_col_offset);
        }
    }
    return loc;
}

// Return 1 if the method call was optimized, 0 if not, and -1 on error.
static int
maybe_optimize_method_call(struct compiler *c, expr_ty e)
{
    Py_ssize_t argsl, i, kwdsl;
    expr_ty meth = e->v.Call.func;
    asdl_expr_seq *args = e->v.Call.args;
    asdl_keyword_seq *kwds = e->v.Call.keywords;

    /* Check that the call node is an attribute access */
    if (meth->kind != Attribute_kind || meth->v.Attribute.ctx != Load) {
        return 0;
    }

    /* Check that the base object is not something that is imported */
    if (is_import_originated(c, meth->v.Attribute.value)) {
        return 0;
    }

    /* Check that there aren't too many arguments */
    argsl = asdl_seq_LEN(args);
    kwdsl = asdl_seq_LEN(kwds);
    if (argsl + kwdsl + (kwdsl != 0) >= STACK_USE_GUIDELINE) {
        return 0;
    }
    /* Check that there are no *varargs types of arguments. */
    for (i = 0; i < argsl; i++) {
        expr_ty elt = asdl_seq_GET(args, i);
        if (elt->kind == Starred_kind) {
            return 0;
        }
    }

    for (i = 0; i < kwdsl; i++) {
        keyword_ty kw = asdl_seq_GET(kwds, i);
        if (kw->arg == NULL) {
            return 0;
        }
    }

    /* Alright, we can optimize the code. */
    location loc = LOC(meth);

    if (can_optimize_super_call(c, meth)) {
        RETURN_IF_ERROR(load_args_for_super(c, meth->v.Attribute.value));
        int opcode = asdl_seq_LEN(meth->v.Attribute.value->v.Call.args) ?
            LOAD_SUPER_METHOD : LOAD_ZERO_SUPER_METHOD;
        ADDOP_NAME(c, loc, opcode, meth->v.Attribute.attr, names);
        loc = update_start_location_to_match_attr(c, loc, meth);
        ADDOP(c, loc, NOP);
    } else {
        VISIT(c, expr, meth->v.Attribute.value);
        loc = update_start_location_to_match_attr(c, loc, meth);
        ADDOP_NAME(c, loc, LOAD_METHOD, meth->v.Attribute.attr, names);
    }

    VISIT_SEQ(c, expr, e->v.Call.args);

    if (kwdsl) {
        VISIT_SEQ(c, keyword, kwds);
        RETURN_IF_ERROR(
            compiler_call_simple_kw_helper(c, loc, kwds, kwdsl));
    }
    loc = update_start_location_to_match_attr(c, LOC(e), meth);
    ADDOP_I(c, loc, CALL, argsl + kwdsl);
    return 1;
}

static int
validate_keywords(struct compiler *c, asdl_keyword_seq *keywords)
{
    Py_ssize_t nkeywords = asdl_seq_LEN(keywords);
    for (Py_ssize_t i = 0; i < nkeywords; i++) {
        keyword_ty key = ((keyword_ty)asdl_seq_GET(keywords, i));
        if (key->arg == NULL) {
            continue;
        }
        location loc = LOC(key);
        if (forbidden_name(c, loc, key->arg, Store)) {
            return ERROR;
        }
        for (Py_ssize_t j = i + 1; j < nkeywords; j++) {
            keyword_ty other = ((keyword_ty)asdl_seq_GET(keywords, j));
            if (other->arg && !PyUnicode_Compare(key->arg, other->arg)) {
                compiler_error(c, LOC(other), "keyword argument repeated: %U", key->arg);
                return ERROR;
            }
        }
    }
    return SUCCESS;
}

static int
compiler_call(struct compiler *c, expr_ty e)
{
    RETURN_IF_ERROR(validate_keywords(c, e->v.Call.keywords));
    int ret = maybe_optimize_method_call(c, e);
    if (ret < 0) {
        return ERROR;
    }
    if (ret == 1) {
        return SUCCESS;
    }
    RETURN_IF_ERROR(check_caller(c, e->v.Call.func));
    location loc = LOC(e->v.Call.func);
    ADDOP(c, loc, PUSH_NULL);
    VISIT(c, expr, e->v.Call.func);
    loc = LOC(e);
    return compiler_call_helper(c, loc, 0,
                                e->v.Call.args,
                                e->v.Call.keywords);
}

static int
compiler_joined_str(struct compiler *c, expr_ty e)
{
    location loc = LOC(e);
    Py_ssize_t value_count = asdl_seq_LEN(e->v.JoinedStr.values);
    if (value_count > STACK_USE_GUIDELINE) {
        _Py_DECLARE_STR(empty, "");
        ADDOP_LOAD_CONST_NEW(c, loc, Py_NewRef(&_Py_STR(empty)));
        ADDOP_NAME(c, loc, LOAD_METHOD, &_Py_ID(join), names);
        ADDOP_I(c, loc, BUILD_LIST, 0);
        for (Py_ssize_t i = 0; i < asdl_seq_LEN(e->v.JoinedStr.values); i++) {
            VISIT(c, expr, asdl_seq_GET(e->v.JoinedStr.values, i));
            ADDOP_I(c, loc, LIST_APPEND, 1);
        }
        ADDOP_I(c, loc, CALL, 1);
    }
    else {
        VISIT_SEQ(c, expr, e->v.JoinedStr.values);
        if (asdl_seq_LEN(e->v.JoinedStr.values) != 1) {
            ADDOP_I(c, loc, BUILD_STRING, asdl_seq_LEN(e->v.JoinedStr.values));
        }
    }
    return SUCCESS;
}

/* Used to implement f-strings. Format a single value. */
static int
compiler_formatted_value(struct compiler *c, expr_ty e)
{
    /* Our oparg encodes 2 pieces of information: the conversion
       character, and whether or not a format_spec was provided.

       Convert the conversion char to 3 bits:
           : 000  0x0  FVC_NONE   The default if nothing specified.
       !s  : 001  0x1  FVC_STR
       !r  : 010  0x2  FVC_REPR
       !a  : 011  0x3  FVC_ASCII

       next bit is whether or not we have a format spec:
       yes : 100  0x4
       no  : 000  0x0
    */

    int conversion = e->v.FormattedValue.conversion;
    int oparg;

    /* The expression to be formatted. */
    VISIT(c, expr, e->v.FormattedValue.value);

    switch (conversion) {
    case 's': oparg = FVC_STR;   break;
    case 'r': oparg = FVC_REPR;  break;
    case 'a': oparg = FVC_ASCII; break;
    case -1:  oparg = FVC_NONE;  break;
    default:
        PyErr_Format(PyExc_SystemError,
                     "Unrecognized conversion character %d", conversion);
        return ERROR;
    }
    if (e->v.FormattedValue.format_spec) {
        /* Evaluate the format spec, and update our opcode arg. */
        VISIT(c, expr, e->v.FormattedValue.format_spec);
        oparg |= FVS_HAVE_SPEC;
    }

    /* And push our opcode and oparg */
    location loc = LOC(e);
    ADDOP_I(c, loc, FORMAT_VALUE, oparg);

    return SUCCESS;
}

static int
compiler_subkwargs(struct compiler *c, location loc,
                   asdl_keyword_seq *keywords,
                   Py_ssize_t begin, Py_ssize_t end)
{
    Py_ssize_t i, n = end - begin;
    keyword_ty kw;
    PyObject *keys, *key;
    assert(n > 0);
    int big = n*2 > STACK_USE_GUIDELINE;
    if (n > 1 && !big) {
        for (i = begin; i < end; i++) {
            kw = asdl_seq_GET(keywords, i);
            VISIT(c, expr, kw->value);
        }
        keys = PyTuple_New(n);
        if (keys == NULL) {
            return ERROR;
        }
        for (i = begin; i < end; i++) {
            key = ((keyword_ty) asdl_seq_GET(keywords, i))->arg;
            PyTuple_SET_ITEM(keys, i - begin, Py_NewRef(key));
        }
        ADDOP_LOAD_CONST_NEW(c, loc, keys);
        ADDOP_I(c, loc, BUILD_CONST_KEY_MAP, n);
        return SUCCESS;
    }
    if (big) {
        ADDOP_I(c, NO_LOCATION, BUILD_MAP, 0);
    }
    for (i = begin; i < end; i++) {
        kw = asdl_seq_GET(keywords, i);
        ADDOP_LOAD_CONST(c, loc, kw->arg);
        VISIT(c, expr, kw->value);
        if (big) {
            ADDOP_I(c, NO_LOCATION, MAP_ADD, 1);
        }
    }
    if (!big) {
        ADDOP_I(c, loc, BUILD_MAP, n);
    }
    return SUCCESS;
}

/* Used by compiler_call_helper and maybe_optimize_method_call to emit
 * KW_NAMES before CALL.
 */
static int
compiler_call_simple_kw_helper(struct compiler *c, location loc,
                               asdl_keyword_seq *keywords, Py_ssize_t nkwelts)
{
    PyObject *names;
    names = PyTuple_New(nkwelts);
    if (names == NULL) {
        return ERROR;
    }
    for (int i = 0; i < nkwelts; i++) {
        keyword_ty kw = asdl_seq_GET(keywords, i);
        PyTuple_SET_ITEM(names, i, Py_NewRef(kw->arg));
    }
    Py_ssize_t arg = compiler_add_const(c->c_const_cache, c->u, names);
    if (arg < 0) {
        return ERROR;
    }
    Py_DECREF(names);
    ADDOP_I(c, loc, KW_NAMES, arg);
    return SUCCESS;
}


/* shared code between compiler_call and compiler_class */
static int
compiler_call_helper(struct compiler *c, location loc,
                     int n, /* Args already pushed */
                     asdl_expr_seq *args,
                     asdl_keyword_seq *keywords)
{
    Py_ssize_t i, nseen, nelts, nkwelts;

    RETURN_IF_ERROR(validate_keywords(c, keywords));

    nelts = asdl_seq_LEN(args);
    nkwelts = asdl_seq_LEN(keywords);

    if (nelts + nkwelts*2 > STACK_USE_GUIDELINE) {
         goto ex_call;
    }
    for (i = 0; i < nelts; i++) {
        expr_ty elt = asdl_seq_GET(args, i);
        if (elt->kind == Starred_kind) {
            goto ex_call;
        }
    }
    for (i = 0; i < nkwelts; i++) {
        keyword_ty kw = asdl_seq_GET(keywords, i);
        if (kw->arg == NULL) {
            goto ex_call;
        }
    }

    /* No * or ** args, so can use faster calling sequence */
    for (i = 0; i < nelts; i++) {
        expr_ty elt = asdl_seq_GET(args, i);
        assert(elt->kind != Starred_kind);
        VISIT(c, expr, elt);
    }
    if (nkwelts) {
        VISIT_SEQ(c, keyword, keywords);
        RETURN_IF_ERROR(
            compiler_call_simple_kw_helper(c, loc, keywords, nkwelts));
    }
    ADDOP_I(c, loc, CALL, n + nelts + nkwelts);
    return SUCCESS;

ex_call:

    /* Do positional arguments. */
    if (n ==0 && nelts == 1 && ((expr_ty)asdl_seq_GET(args, 0))->kind == Starred_kind) {
        VISIT(c, expr, ((expr_ty)asdl_seq_GET(args, 0))->v.Starred.value);
    }
    else {
        RETURN_IF_ERROR(starunpack_helper(c, loc, args, n, BUILD_LIST,
                                          LIST_APPEND, LIST_EXTEND, 1));
    }
    /* Then keyword arguments */
    if (nkwelts) {
        /* Has a new dict been pushed */
        int have_dict = 0;

        nseen = 0;  /* the number of keyword arguments on the stack following */
        for (i = 0; i < nkwelts; i++) {
            keyword_ty kw = asdl_seq_GET(keywords, i);
            if (kw->arg == NULL) {
                /* A keyword argument unpacking. */
                if (nseen) {
                    RETURN_IF_ERROR(compiler_subkwargs(c, loc, keywords, i - nseen, i));
                    if (have_dict) {
                        ADDOP_I(c, loc, DICT_MERGE, 1);
                    }
                    have_dict = 1;
                    nseen = 0;
                }
                if (!have_dict) {
                    ADDOP_I(c, loc, BUILD_MAP, 0);
                    have_dict = 1;
                }
                VISIT(c, expr, kw->value);
                ADDOP_I(c, loc, DICT_MERGE, 1);
            }
            else {
                nseen++;
            }
        }
        if (nseen) {
            /* Pack up any trailing keyword arguments. */
            RETURN_IF_ERROR(compiler_subkwargs(c, loc, keywords, nkwelts - nseen, nkwelts));
            if (have_dict) {
                ADDOP_I(c, loc, DICT_MERGE, 1);
            }
            have_dict = 1;
        }
        assert(have_dict);
    }
    ADDOP_I(c, loc, CALL_FUNCTION_EX, nkwelts > 0);
    return SUCCESS;
}


/* List and set comprehensions and generator expressions work by creating a
  nested function to perform the actual iteration. This means that the
  iteration variables don't leak into the current scope.
  The defined function is called immediately following its definition, with the
  result of that call being the result of the expression.
  The LC/SC version returns the populated container, while the GE version is
  flagged in symtable.c as a generator, so it returns the generator object
  when the function is called.

  Possible cleanups:
    - iterate over the generator sequence instead of using recursion
*/


static int
compiler_comprehension_generator(struct compiler *c, location loc,
                                 asdl_comprehension_seq *generators, int gen_index,
                                 int depth,
                                 expr_ty elt, expr_ty val, int type,
                                 int iter_on_stack)
{
    comprehension_ty gen;
    gen = (comprehension_ty)asdl_seq_GET(generators, gen_index);
    if (gen->is_async) {
        return compiler_async_comprehension_generator(
            c, loc, generators, gen_index, depth, elt, val, type,
            iter_on_stack);
    } else {
        return compiler_sync_comprehension_generator(
            c, loc, generators, gen_index, depth, elt, val, type,
            iter_on_stack);
    }
}

static int
compiler_sync_comprehension_generator(struct compiler *c, location loc,
                                      asdl_comprehension_seq *generators,
                                      int gen_index, int depth,
                                      expr_ty elt, expr_ty val, int type,
                                      int iter_on_stack)
{
    /* generate code for the iterator, then each of the ifs,
       and then write to the element */

    NEW_JUMP_TARGET_LABEL(c, start);
    NEW_JUMP_TARGET_LABEL(c, if_cleanup);
    NEW_JUMP_TARGET_LABEL(c, anchor);

    comprehension_ty gen = (comprehension_ty)asdl_seq_GET(generators,
                                                          gen_index);

    if (!iter_on_stack) {
        if (gen_index == 0) {
            /* Receive outermost iter as an implicit argument */
            c->u->u_metadata.u_argcount = 1;
            ADDOP_I(c, loc, LOAD_FAST, 0);
        }
        else {
            /* Sub-iter - calculate on the fly */
            /* Fast path for the temporary variable assignment idiom:
                for y in [f(x)]
            */
            asdl_expr_seq *elts;
            switch (gen->iter->kind) {
                case List_kind:
                    elts = gen->iter->v.List.elts;
                    break;
                case Tuple_kind:
                    elts = gen->iter->v.Tuple.elts;
                    break;
                default:
                    elts = NULL;
            }
            if (asdl_seq_LEN(elts) == 1) {
                expr_ty elt = asdl_seq_GET(elts, 0);
                if (elt->kind != Starred_kind) {
                    VISIT(c, expr, elt);
                    start = NO_LABEL;
                }
            }
            if (IS_LABEL(start)) {
                VISIT(c, expr, gen->iter);
                ADDOP(c, loc, GET_ITER);
            }
        }
    }
    if (IS_LABEL(start)) {
        depth++;
        USE_LABEL(c, start);
        ADDOP_JUMP(c, loc, FOR_ITER, anchor);
    }
    VISIT(c, expr, gen->target);

    /* XXX this needs to be cleaned up...a lot! */
    Py_ssize_t n = asdl_seq_LEN(gen->ifs);
    for (Py_ssize_t i = 0; i < n; i++) {
        expr_ty e = (expr_ty)asdl_seq_GET(gen->ifs, i);
        RETURN_IF_ERROR(compiler_jump_if(c, loc, e, if_cleanup, 0));
    }

    if (++gen_index < asdl_seq_LEN(generators)) {
        RETURN_IF_ERROR(
            compiler_comprehension_generator(c, loc,
                                             generators, gen_index, depth,
                                             elt, val, type, 0));
    }

    location elt_loc = LOC(elt);

    /* only append after the last for generator */
    if (gen_index >= asdl_seq_LEN(generators)) {
        /* comprehension specific code */
        switch (type) {
        case COMP_GENEXP:
            VISIT(c, expr, elt);
            ADDOP_YIELD(c, elt_loc);
            ADDOP(c, elt_loc, POP_TOP);
            break;
        case COMP_LISTCOMP:
            VISIT(c, expr, elt);
            ADDOP_I(c, elt_loc, LIST_APPEND, depth + 1);
            break;
        case COMP_SETCOMP:
            VISIT(c, expr, elt);
            ADDOP_I(c, elt_loc, SET_ADD, depth + 1);
            break;
        case COMP_DICTCOMP:
            /* With '{k: v}', k is evaluated before v, so we do
               the same. */
            VISIT(c, expr, elt);
            VISIT(c, expr, val);
            elt_loc = LOCATION(elt->lineno,
                               val->end_lineno,
                               elt->col_offset,
                               val->end_col_offset);
            ADDOP_I(c, elt_loc, MAP_ADD, depth + 1);
            break;
        default:
            return ERROR;
        }
    }

    USE_LABEL(c, if_cleanup);
    if (IS_LABEL(start)) {
        ADDOP_JUMP(c, elt_loc, JUMP, start);

        USE_LABEL(c, anchor);
        ADDOP(c, NO_LOCATION, END_FOR);
    }

    return SUCCESS;
}

static int
compiler_async_comprehension_generator(struct compiler *c, location loc,
                                      asdl_comprehension_seq *generators,
                                      int gen_index, int depth,
                                      expr_ty elt, expr_ty val, int type,
                                      int iter_on_stack)
{
    NEW_JUMP_TARGET_LABEL(c, start);
    NEW_JUMP_TARGET_LABEL(c, except);
    NEW_JUMP_TARGET_LABEL(c, if_cleanup);

    comprehension_ty gen = (comprehension_ty)asdl_seq_GET(generators,
                                                          gen_index);

    if (!iter_on_stack) {
        if (gen_index == 0) {
            /* Receive outermost iter as an implicit argument */
            c->u->u_metadata.u_argcount = 1;
            ADDOP_I(c, loc, LOAD_FAST, 0);
        }
        else {
            /* Sub-iter - calculate on the fly */
            VISIT(c, expr, gen->iter);
            ADDOP(c, loc, GET_AITER);
        }
    }

    USE_LABEL(c, start);
    /* Runtime will push a block here, so we need to account for that */
    RETURN_IF_ERROR(
        compiler_push_fblock(c, loc, ASYNC_COMPREHENSION_GENERATOR,
                             start, NO_LABEL, NULL));

    ADDOP_JUMP(c, loc, SETUP_FINALLY, except);
    ADDOP(c, loc, GET_ANEXT);
    ADDOP_LOAD_CONST(c, loc, Py_None);
    ADD_YIELD_FROM(c, loc, 1);
    ADDOP(c, loc, POP_BLOCK);
    VISIT(c, expr, gen->target);

    Py_ssize_t n = asdl_seq_LEN(gen->ifs);
    for (Py_ssize_t i = 0; i < n; i++) {
        expr_ty e = (expr_ty)asdl_seq_GET(gen->ifs, i);
        RETURN_IF_ERROR(compiler_jump_if(c, loc, e, if_cleanup, 0));
    }

    depth++;
    if (++gen_index < asdl_seq_LEN(generators)) {
        RETURN_IF_ERROR(
            compiler_comprehension_generator(c, loc,
                                             generators, gen_index, depth,
                                             elt, val, type, 0));
    }

    location elt_loc = LOC(elt);
    /* only append after the last for generator */
    if (gen_index >= asdl_seq_LEN(generators)) {
        /* comprehension specific code */
        switch (type) {
        case COMP_GENEXP:
            VISIT(c, expr, elt);
            ADDOP_YIELD(c, elt_loc);
            ADDOP(c, elt_loc, POP_TOP);
            break;
        case COMP_LISTCOMP:
            VISIT(c, expr, elt);
            ADDOP_I(c, elt_loc, LIST_APPEND, depth + 1);
            break;
        case COMP_SETCOMP:
            VISIT(c, expr, elt);
            ADDOP_I(c, elt_loc, SET_ADD, depth + 1);
            break;
        case COMP_DICTCOMP:
            /* With '{k: v}', k is evaluated before v, so we do
               the same. */
            VISIT(c, expr, elt);
            VISIT(c, expr, val);
            elt_loc = LOCATION(elt->lineno,
                               val->end_lineno,
                               elt->col_offset,
                               val->end_col_offset);
            ADDOP_I(c, elt_loc, MAP_ADD, depth + 1);
            break;
        default:
            return ERROR;
        }
    }

    USE_LABEL(c, if_cleanup);
    ADDOP_JUMP(c, elt_loc, JUMP, start);

    compiler_pop_fblock(c, ASYNC_COMPREHENSION_GENERATOR, start);

    USE_LABEL(c, except);

    ADDOP(c, loc, END_ASYNC_FOR);

    return SUCCESS;
}

typedef struct {
    PyObject *pushed_locals;
    PyObject *temp_symbols;
    PyObject *fast_hidden;
    jump_target_label cleanup;
    jump_target_label end;
} inlined_comprehension_state;

static int
push_inlined_comprehension_state(struct compiler *c, location loc,
                                 PySTEntryObject *entry,
                                 inlined_comprehension_state *state)
{
    int in_class_block = (c->u->u_ste->ste_type == ClassBlock) && !c->u->u_in_inlined_comp;
    c->u->u_in_inlined_comp++;
    // iterate over names bound in the comprehension and ensure we isolate
    // them from the outer scope as needed
    PyObject *k, *v;
    Py_ssize_t pos = 0;
    while (PyDict_Next(entry->ste_symbols, &pos, &k, &v)) {
        assert(PyLong_Check(v));
        long symbol = PyLong_AS_LONG(v);
        // only values bound in the comprehension (DEF_LOCAL) need to be handled
        // at all; DEF_LOCAL | DEF_NONLOCAL can occur in the case of an
        // assignment expression to a nonlocal in the comprehension, these don't
        // need handling here since they shouldn't be isolated
        if ((symbol & DEF_LOCAL && !(symbol & DEF_NONLOCAL)) || in_class_block) {
            if (!_PyST_IsFunctionLike(c->u->u_ste)) {
                // non-function scope: override this name to use fast locals
                PyObject *orig = PyDict_GetItem(c->u->u_metadata.u_fasthidden, k);
                if (orig != Py_True) {
                    if (PyDict_SetItem(c->u->u_metadata.u_fasthidden, k, Py_True) < 0) {
                        return ERROR;
                    }
                    if (state->fast_hidden == NULL) {
                        state->fast_hidden = PySet_New(NULL);
                        if (state->fast_hidden == NULL) {
                            return ERROR;
                        }
                    }
                    if (PySet_Add(state->fast_hidden, k) < 0) {
                        return ERROR;
                    }
                }
            }
            long scope = (symbol >> SCOPE_OFFSET) & SCOPE_MASK;
            PyObject *outv = PyDict_GetItemWithError(c->u->u_ste->ste_symbols, k);
            if (outv == NULL) {
                outv = _PyLong_GetZero();
            }
            assert(PyLong_Check(outv));
            long outsc = (PyLong_AS_LONG(outv) >> SCOPE_OFFSET) & SCOPE_MASK;
            if (scope != outsc && !(scope == CELL && outsc == FREE)) {
                // If a name has different scope inside than outside the
                // comprehension, we need to temporarily handle it with the
                // right scope while compiling the comprehension. (If it's free
                // in outer scope and cell in inner scope, we can't treat it as
                // both cell and free in the same function, but treating it as
                // free throughout is fine; it's *_DEREF either way.)

                if (state->temp_symbols == NULL) {
                    state->temp_symbols = PyDict_New();
                    if (state->temp_symbols == NULL) {
                        return ERROR;
                    }
                }
                // update the symbol to the in-comprehension version and save
                // the outer version; we'll restore it after running the
                // comprehension
                Py_INCREF(outv);
                if (PyDict_SetItem(c->u->u_ste->ste_symbols, k, v) < 0) {
                    Py_DECREF(outv);
                    return ERROR;
                }
                if (PyDict_SetItem(state->temp_symbols, k, outv) < 0) {
                    Py_DECREF(outv);
                    return ERROR;
                }
                Py_DECREF(outv);
            }
            // local names bound in comprehension must be isolated from
            // outer scope; push existing value (which may be NULL if
            // not defined) on stack
            if (state->pushed_locals == NULL) {
                state->pushed_locals = PyList_New(0);
                if (state->pushed_locals == NULL) {
                    return ERROR;
                }
            }
            // in the case of a cell, this will actually push the cell
            // itself to the stack, then we'll create a new one for the
            // comprehension and restore the original one after
            ADDOP_NAME(c, loc, LOAD_FAST_AND_CLEAR, k, varnames);
            if (scope == CELL) {
                if (outsc == FREE) {
                    ADDOP_NAME(c, loc, MAKE_CELL, k, freevars);
                } else {
                    ADDOP_NAME(c, loc, MAKE_CELL, k, cellvars);
                }
            }
            if (PyList_Append(state->pushed_locals, k) < 0) {
                return ERROR;
            }
        }
    }
    if (state->pushed_locals) {
        // Outermost iterable expression was already evaluated and is on the
        // stack, we need to swap it back to TOS. This also rotates the order of
        // `pushed_locals` on the stack, but this will be reversed when we swap
        // out the comprehension result in pop_inlined_comprehension_state
        ADDOP_I(c, loc, SWAP, PyList_GET_SIZE(state->pushed_locals) + 1);

        // Add our own cleanup handler to restore comprehension locals in case
        // of exception, so they have the correct values inside an exception
        // handler or finally block.
        NEW_JUMP_TARGET_LABEL(c, cleanup);
        state->cleanup = cleanup;
        NEW_JUMP_TARGET_LABEL(c, end);
        state->end = end;

        // no need to push an fblock for this "virtual" try/finally; there can't
        // be return/continue/break inside a comprehension
        ADDOP_JUMP(c, loc, SETUP_FINALLY, cleanup);
    }

    return SUCCESS;
}

static int
restore_inlined_comprehension_locals(struct compiler *c, location loc,
                                     inlined_comprehension_state state)
{
    PyObject *k;
    // pop names we pushed to stack earlier
    Py_ssize_t npops = PyList_GET_SIZE(state.pushed_locals);
    // Preserve the comprehension result (or exception) as TOS. This
    // reverses the SWAP we did in push_inlined_comprehension_state to get
    // the outermost iterable to TOS, so we can still just iterate
    // pushed_locals in simple reverse order
    ADDOP_I(c, loc, SWAP, npops + 1);
    for (Py_ssize_t i = npops - 1; i >= 0; --i) {
        k = PyList_GetItem(state.pushed_locals, i);
        if (k == NULL) {
            return ERROR;
        }
        ADDOP_NAME(c, loc, STORE_FAST_MAYBE_NULL, k, varnames);
    }
    return SUCCESS;
}

static int
pop_inlined_comprehension_state(struct compiler *c, location loc,
                                inlined_comprehension_state state)
{
    c->u->u_in_inlined_comp--;
    PyObject *k, *v;
    Py_ssize_t pos = 0;
    if (state.temp_symbols) {
        while (PyDict_Next(state.temp_symbols, &pos, &k, &v)) {
            if (PyDict_SetItem(c->u->u_ste->ste_symbols, k, v)) {
                return ERROR;
            }
        }
        Py_CLEAR(state.temp_symbols);
    }
    if (state.pushed_locals) {
        ADDOP(c, NO_LOCATION, POP_BLOCK);
        ADDOP_JUMP(c, NO_LOCATION, JUMP, state.end);

        // cleanup from an exception inside the comprehension
        USE_LABEL(c, state.cleanup);
        // discard incomplete comprehension result (beneath exc on stack)
        ADDOP_I(c, NO_LOCATION, SWAP, 2);
        ADDOP(c, NO_LOCATION, POP_TOP);
        if (restore_inlined_comprehension_locals(c, loc, state) < 0) {
            return ERROR;
        }
        ADDOP_I(c, NO_LOCATION, RERAISE, 0);

        USE_LABEL(c, state.end);
        if (restore_inlined_comprehension_locals(c, loc, state) < 0) {
            return ERROR;
        }
        Py_CLEAR(state.pushed_locals);
    }
    if (state.fast_hidden) {
        while (PySet_Size(state.fast_hidden) > 0) {
            PyObject *k = PySet_Pop(state.fast_hidden);
            if (k == NULL) {
                return ERROR;
            }
            // we set to False instead of clearing, so we can track which names
            // were temporarily fast-locals and should use CO_FAST_HIDDEN
            if (PyDict_SetItem(c->u->u_metadata.u_fasthidden, k, Py_False)) {
                Py_DECREF(k);
                return ERROR;
            }
            Py_DECREF(k);
        }
        Py_CLEAR(state.fast_hidden);
    }
    return SUCCESS;
}

static inline int
compiler_comprehension_iter(struct compiler *c, location loc,
                            comprehension_ty comp)
{
    VISIT(c, expr, comp->iter);
    if (comp->is_async) {
        ADDOP(c, loc, GET_AITER);
    }
    else {
        ADDOP(c, loc, GET_ITER);
    }
    return SUCCESS;
}

static int
compiler_comprehension(struct compiler *c, expr_ty e, int type,
                       identifier name, asdl_comprehension_seq *generators, expr_ty elt,
                       expr_ty val)
{
    PyCodeObject *co = NULL;
    inlined_comprehension_state inline_state = {NULL, NULL, NULL, NO_LABEL, NO_LABEL};
    comprehension_ty outermost;
    int scope_type = c->u->u_scope_type;
    int is_top_level_await = IS_TOP_LEVEL_AWAIT(c);
    PySTEntryObject *entry = PySymtable_Lookup(c->c_st, (void *)e);
    if (entry == NULL) {
        goto error;
    }
    int is_inlined = entry->ste_comp_inlined;
    int is_async_generator = entry->ste_coroutine;

    location loc = LOC(e);

    outermost = (comprehension_ty) asdl_seq_GET(generators, 0);
    if (is_inlined) {
        if (compiler_comprehension_iter(c, loc, outermost)) {
            goto error;
        }
        if (push_inlined_comprehension_state(c, loc, entry, &inline_state)) {
            goto error;
        }
    }
    else {
        if (compiler_enter_scope(c, name, COMPILER_SCOPE_COMPREHENSION,
                                (void *)e, e->lineno) < 0)
        {
            goto error;
        }
    }
    Py_CLEAR(entry);

    if (is_async_generator && type != COMP_GENEXP &&
        scope_type != COMPILER_SCOPE_ASYNC_FUNCTION &&
        scope_type != COMPILER_SCOPE_COMPREHENSION &&
        !is_top_level_await)
    {
        compiler_error(c, loc, "asynchronous comprehension outside of "
                               "an asynchronous function");
        goto error_in_scope;
    }

    if (type != COMP_GENEXP) {
        int op;
        switch (type) {
        case COMP_LISTCOMP:
            op = BUILD_LIST;
            break;
        case COMP_SETCOMP:
            op = BUILD_SET;
            break;
        case COMP_DICTCOMP:
            op = BUILD_MAP;
            break;
        default:
            PyErr_Format(PyExc_SystemError,
                         "unknown comprehension type %d", type);
            goto error_in_scope;
        }

        ADDOP_I(c, loc, op, 0);
        if (is_inlined) {
            ADDOP_I(c, loc, SWAP, 2);
        }
    }

    if (compiler_comprehension_generator(c, loc, generators, 0, 0,
                                         elt, val, type, is_inlined) < 0) {
        goto error_in_scope;
    }

    if (is_inlined) {
        if (pop_inlined_comprehension_state(c, loc, inline_state)) {
            goto error;
        }
        return SUCCESS;
    }

    if (type != COMP_GENEXP) {
        ADDOP(c, LOC(e), RETURN_VALUE);
    }
    if (type == COMP_GENEXP) {
        if (wrap_in_stopiteration_handler(c) < 0) {
            goto error_in_scope;
        }
    }

    co = optimize_and_assemble(c, 1);
    compiler_exit_scope(c);
    if (is_top_level_await && is_async_generator){
        c->u->u_ste->ste_coroutine = 1;
    }
    if (co == NULL) {
        goto error;
    }

    loc = LOC(e);
    if (compiler_make_closure(c, loc, co, 0) < 0) {
        goto error;
    }
    Py_CLEAR(co);

    if (compiler_comprehension_iter(c, loc, outermost)) {
        goto error;
    }

    ADDOP_I(c, loc, CALL, 0);

    if (is_async_generator && type != COMP_GENEXP) {
        ADDOP_I(c, loc, GET_AWAITABLE, 0);
        ADDOP_LOAD_CONST(c, loc, Py_None);
        ADD_YIELD_FROM(c, loc, 1);
    }

    return SUCCESS;
error_in_scope:
    if (!is_inlined) {
        compiler_exit_scope(c);
    }
error:
    Py_XDECREF(co);
    Py_XDECREF(entry);
    Py_XDECREF(inline_state.pushed_locals);
    Py_XDECREF(inline_state.temp_symbols);
    Py_XDECREF(inline_state.fast_hidden);
    return ERROR;
}

static int
compiler_genexp(struct compiler *c, expr_ty e)
{
    assert(e->kind == GeneratorExp_kind);
    _Py_DECLARE_STR(anon_genexpr, "<genexpr>");
    return compiler_comprehension(c, e, COMP_GENEXP, &_Py_STR(anon_genexpr),
                                  e->v.GeneratorExp.generators,
                                  e->v.GeneratorExp.elt, NULL);
}

static int
compiler_listcomp(struct compiler *c, expr_ty e)
{
    assert(e->kind == ListComp_kind);
    _Py_DECLARE_STR(anon_listcomp, "<listcomp>");
    return compiler_comprehension(c, e, COMP_LISTCOMP, &_Py_STR(anon_listcomp),
                                  e->v.ListComp.generators,
                                  e->v.ListComp.elt, NULL);
}

static int
compiler_setcomp(struct compiler *c, expr_ty e)
{
    assert(e->kind == SetComp_kind);
    _Py_DECLARE_STR(anon_setcomp, "<setcomp>");
    return compiler_comprehension(c, e, COMP_SETCOMP, &_Py_STR(anon_setcomp),
                                  e->v.SetComp.generators,
                                  e->v.SetComp.elt, NULL);
}


static int
compiler_dictcomp(struct compiler *c, expr_ty e)
{
    assert(e->kind == DictComp_kind);
    _Py_DECLARE_STR(anon_dictcomp, "<dictcomp>");
    return compiler_comprehension(c, e, COMP_DICTCOMP, &_Py_STR(anon_dictcomp),
                                  e->v.DictComp.generators,
                                  e->v.DictComp.key, e->v.DictComp.value);
}


static int
compiler_visit_keyword(struct compiler *c, keyword_ty k)
{
    VISIT(c, expr, k->value);
    return SUCCESS;
}


static int
compiler_with_except_finish(struct compiler *c, jump_target_label cleanup) {
    NEW_JUMP_TARGET_LABEL(c, suppress);
    ADDOP_JUMP(c, NO_LOCATION, POP_JUMP_IF_TRUE, suppress);
    ADDOP_I(c, NO_LOCATION, RERAISE, 2);

    USE_LABEL(c, suppress);
    ADDOP(c, NO_LOCATION, POP_TOP); /* exc_value */
    ADDOP(c, NO_LOCATION, POP_BLOCK);
    ADDOP(c, NO_LOCATION, POP_EXCEPT);
    ADDOP(c, NO_LOCATION, POP_TOP);
    ADDOP(c, NO_LOCATION, POP_TOP);
    NEW_JUMP_TARGET_LABEL(c, exit);
    ADDOP_JUMP(c, NO_LOCATION, JUMP, exit);

    USE_LABEL(c, cleanup);
    POP_EXCEPT_AND_RERAISE(c, NO_LOCATION);

    USE_LABEL(c, exit);
    return SUCCESS;
}

/*
   Implements the async with statement.

   The semantics outlined in that PEP are as follows:

   async with EXPR as VAR:
       BLOCK

   It is implemented roughly as:

   context = EXPR
   exit = context.__aexit__  # not calling it
   value = await context.__aenter__()
   try:
       VAR = value  # if VAR present in the syntax
       BLOCK
   finally:
       if an exception was raised:
           exc = copy of (exception, instance, traceback)
       else:
           exc = (None, None, None)
       if not (await exit(*exc)):
           raise
 */
static int
compiler_async_with(struct compiler *c, stmt_ty s, int pos)
{
    location loc = LOC(s);
    withitem_ty item = asdl_seq_GET(s->v.AsyncWith.items, pos);

    assert(s->kind == AsyncWith_kind);
    if (IS_TOP_LEVEL_AWAIT(c)){
        c->u->u_ste->ste_coroutine = 1;
    } else if (c->u->u_scope_type != COMPILER_SCOPE_ASYNC_FUNCTION){
        return compiler_error(c, loc, "'async with' outside async function");
    }

    NEW_JUMP_TARGET_LABEL(c, block);
    NEW_JUMP_TARGET_LABEL(c, final);
    NEW_JUMP_TARGET_LABEL(c, exit);
    NEW_JUMP_TARGET_LABEL(c, cleanup);

    /* Evaluate EXPR */
    VISIT(c, expr, item->context_expr);

    ADDOP(c, loc, BEFORE_ASYNC_WITH);
    ADDOP_I(c, loc, GET_AWAITABLE, 1);
    ADDOP_LOAD_CONST(c, loc, Py_None);
    ADD_YIELD_FROM(c, loc, 1);

    ADDOP_JUMP(c, loc, SETUP_WITH, final);

    /* SETUP_WITH pushes a finally block. */
    USE_LABEL(c, block);
    RETURN_IF_ERROR(compiler_push_fblock(c, loc, ASYNC_WITH, block, final, s));

    if (item->optional_vars) {
        VISIT(c, expr, item->optional_vars);
    }
    else {
        /* Discard result from context.__aenter__() */
        ADDOP(c, loc, POP_TOP);
    }

    pos++;
    if (pos == asdl_seq_LEN(s->v.AsyncWith.items)) {
        /* BLOCK code */
        VISIT_SEQ(c, stmt, s->v.AsyncWith.body)
    }
    else {
        RETURN_IF_ERROR(compiler_async_with(c, s, pos));
    }

    compiler_pop_fblock(c, ASYNC_WITH, block);

    ADDOP(c, loc, POP_BLOCK);
    /* End of body; start the cleanup */

    /* For successful outcome:
     * call __exit__(None, None, None)
     */
    RETURN_IF_ERROR(compiler_call_exit_with_nones(c, loc));
    ADDOP_I(c, loc, GET_AWAITABLE, 2);
    ADDOP_LOAD_CONST(c, loc, Py_None);
    ADD_YIELD_FROM(c, loc, 1);

    ADDOP(c, loc, POP_TOP);

    ADDOP_JUMP(c, loc, JUMP, exit);

    /* For exceptional outcome: */
    USE_LABEL(c, final);

    ADDOP_JUMP(c, loc, SETUP_CLEANUP, cleanup);
    ADDOP(c, loc, PUSH_EXC_INFO);
    ADDOP(c, loc, WITH_EXCEPT_START);
    ADDOP_I(c, loc, GET_AWAITABLE, 2);
    ADDOP_LOAD_CONST(c, loc, Py_None);
    ADD_YIELD_FROM(c, loc, 1);
    RETURN_IF_ERROR(compiler_with_except_finish(c, cleanup));

    USE_LABEL(c, exit);
    return SUCCESS;
}


/*
   Implements the with statement from PEP 343.
   with EXPR as VAR:
       BLOCK
   is implemented as:
        <code for EXPR>
        SETUP_WITH  E
        <code to store to VAR> or POP_TOP
        <code for BLOCK>
        LOAD_CONST (None, None, None)
        CALL_FUNCTION_EX 0
        JUMP  EXIT
    E:  WITH_EXCEPT_START (calls EXPR.__exit__)
        POP_JUMP_IF_TRUE T:
        RERAISE
    T:  POP_TOP (remove exception from stack)
        POP_EXCEPT
        POP_TOP
    EXIT:
 */

static int
compiler_with(struct compiler *c, stmt_ty s, int pos)
{
    withitem_ty item = asdl_seq_GET(s->v.With.items, pos);

    assert(s->kind == With_kind);

    NEW_JUMP_TARGET_LABEL(c, block);
    NEW_JUMP_TARGET_LABEL(c, final);
    NEW_JUMP_TARGET_LABEL(c, exit);
    NEW_JUMP_TARGET_LABEL(c, cleanup);

    /* Evaluate EXPR */
    VISIT(c, expr, item->context_expr);
    /* Will push bound __exit__ */
    location loc = LOC(s);
    ADDOP(c, loc, BEFORE_WITH);
    ADDOP_JUMP(c, loc, SETUP_WITH, final);

    /* SETUP_WITH pushes a finally block. */
    USE_LABEL(c, block);
    RETURN_IF_ERROR(compiler_push_fblock(c, loc, WITH, block, final, s));

    if (item->optional_vars) {
        VISIT(c, expr, item->optional_vars);
    }
    else {
    /* Discard result from context.__enter__() */
        ADDOP(c, loc, POP_TOP);
    }

    pos++;
    if (pos == asdl_seq_LEN(s->v.With.items)) {
        /* BLOCK code */
        VISIT_SEQ(c, stmt, s->v.With.body)
    }
    else {
        RETURN_IF_ERROR(compiler_with(c, s, pos));
    }

    ADDOP(c, NO_LOCATION, POP_BLOCK);
    compiler_pop_fblock(c, WITH, block);

    /* End of body; start the cleanup. */

    /* For successful outcome:
     * call __exit__(None, None, None)
     */
    loc = LOC(s);
    RETURN_IF_ERROR(compiler_call_exit_with_nones(c, loc));
    ADDOP(c, loc, POP_TOP);
    ADDOP_JUMP(c, loc, JUMP, exit);

    /* For exceptional outcome: */
    USE_LABEL(c, final);

    ADDOP_JUMP(c, loc, SETUP_CLEANUP, cleanup);
    ADDOP(c, loc, PUSH_EXC_INFO);
    ADDOP(c, loc, WITH_EXCEPT_START);
    RETURN_IF_ERROR(compiler_with_except_finish(c, cleanup));

    USE_LABEL(c, exit);
    return SUCCESS;
}

static int
compiler_visit_expr1(struct compiler *c, expr_ty e)
{
    location loc = LOC(e);
    switch (e->kind) {
    case NamedExpr_kind:
        VISIT(c, expr, e->v.NamedExpr.value);
        ADDOP_I(c, loc, COPY, 1);
        VISIT(c, expr, e->v.NamedExpr.target);
        break;
    case BoolOp_kind:
        return compiler_boolop(c, e);
    case BinOp_kind:
        VISIT(c, expr, e->v.BinOp.left);
        VISIT(c, expr, e->v.BinOp.right);
        ADDOP_BINARY(c, loc, e->v.BinOp.op);
        break;
    case UnaryOp_kind:
        VISIT(c, expr, e->v.UnaryOp.operand);
        if (e->v.UnaryOp.op == UAdd) {
            ADDOP_I(c, loc, CALL_INTRINSIC_1, INTRINSIC_UNARY_POSITIVE);
        }
        else {
            ADDOP(c, loc, unaryop(e->v.UnaryOp.op));
        }
        break;
    case Lambda_kind:
        return compiler_lambda(c, e);
    case IfExp_kind:
        return compiler_ifexp(c, e);
    case Dict_kind:
        return compiler_dict(c, e);
    case Set_kind:
        return compiler_set(c, e);
    case GeneratorExp_kind:
        return compiler_genexp(c, e);
    case ListComp_kind:
        return compiler_listcomp(c, e);
    case SetComp_kind:
        return compiler_setcomp(c, e);
    case DictComp_kind:
        return compiler_dictcomp(c, e);
    case Yield_kind:
        if (!_PyST_IsFunctionLike(c->u->u_ste)) {
            return compiler_error(c, loc, "'yield' outside function");
        }
        if (e->v.Yield.value) {
            VISIT(c, expr, e->v.Yield.value);
        }
        else {
            ADDOP_LOAD_CONST(c, loc, Py_None);
        }
        ADDOP_YIELD(c, loc);
        break;
    case YieldFrom_kind:
        if (!_PyST_IsFunctionLike(c->u->u_ste)) {
            return compiler_error(c, loc, "'yield' outside function");
        }
        if (c->u->u_scope_type == COMPILER_SCOPE_ASYNC_FUNCTION) {
            return compiler_error(c, loc, "'yield from' inside async function");
        }
        VISIT(c, expr, e->v.YieldFrom.value);
        ADDOP(c, loc, GET_YIELD_FROM_ITER);
        ADDOP_LOAD_CONST(c, loc, Py_None);
        ADD_YIELD_FROM(c, loc, 0);
        break;
    case Await_kind:
        if (!IS_TOP_LEVEL_AWAIT(c)){
            if (!_PyST_IsFunctionLike(c->u->u_ste)) {
                return compiler_error(c, loc, "'await' outside function");
            }

            if (c->u->u_scope_type != COMPILER_SCOPE_ASYNC_FUNCTION &&
                    c->u->u_scope_type != COMPILER_SCOPE_COMPREHENSION) {
                return compiler_error(c, loc, "'await' outside async function");
            }
        }

        VISIT(c, expr, e->v.Await.value);
        ADDOP_I(c, loc, GET_AWAITABLE, 0);
        ADDOP_LOAD_CONST(c, loc, Py_None);
        ADD_YIELD_FROM(c, loc, 1);
        break;
    case Compare_kind:
        return compiler_compare(c, e);
    case Call_kind:
        return compiler_call(c, e);
    case Constant_kind:
        ADDOP_LOAD_CONST(c, loc, e->v.Constant.value);
        break;
    case JoinedStr_kind:
        return compiler_joined_str(c, e);
    case FormattedValue_kind:
        return compiler_formatted_value(c, e);
    /* The following exprs can be assignment targets. */
    case Attribute_kind:
        if (e->v.Attribute.ctx == Load && can_optimize_super_call(c, e)) {
            RETURN_IF_ERROR(load_args_for_super(c, e->v.Attribute.value));
            int opcode = asdl_seq_LEN(e->v.Attribute.value->v.Call.args) ?
                LOAD_SUPER_ATTR : LOAD_ZERO_SUPER_ATTR;
            ADDOP_NAME(c, loc, opcode, e->v.Attribute.attr, names);
            loc = update_start_location_to_match_attr(c, loc, e);
            ADDOP(c, loc, NOP);
            return SUCCESS;
        }
        VISIT(c, expr, e->v.Attribute.value);
        loc = LOC(e);
        loc = update_start_location_to_match_attr(c, loc, e);
        switch (e->v.Attribute.ctx) {
        case Load:
            ADDOP_NAME(c, loc, LOAD_ATTR, e->v.Attribute.attr, names);
            break;
        case Store:
            if (forbidden_name(c, loc, e->v.Attribute.attr, e->v.Attribute.ctx)) {
                return ERROR;
            }
            ADDOP_NAME(c, loc, STORE_ATTR, e->v.Attribute.attr, names);
            break;
        case Del:
            ADDOP_NAME(c, loc, DELETE_ATTR, e->v.Attribute.attr, names);
            break;
        }
        break;
    case Subscript_kind:
        return compiler_subscript(c, e);
    case Starred_kind:
        switch (e->v.Starred.ctx) {
        case Store:
            /* In all legitimate cases, the Starred node was already replaced
             * by compiler_list/compiler_tuple. XXX: is that okay? */
            return compiler_error(c, loc,
                "starred assignment target must be in a list or tuple");
        default:
            return compiler_error(c, loc,
                "can't use starred expression here");
        }
        break;
    case Slice_kind:
    {
        int n = compiler_slice(c, e);
        RETURN_IF_ERROR(n);
        ADDOP_I(c, loc, BUILD_SLICE, n);
        break;
    }
    case Name_kind:
        return compiler_nameop(c, loc, e->v.Name.id, e->v.Name.ctx);
    /* child nodes of List and Tuple will have expr_context set */
    case List_kind:
        return compiler_list(c, e);
    case Tuple_kind:
        return compiler_tuple(c, e);
    }
    return SUCCESS;
}

static int
compiler_visit_expr(struct compiler *c, expr_ty e)
{
    int res = compiler_visit_expr1(c, e);
    return res;
}

static bool
is_two_element_slice(expr_ty s)
{
    return s->kind == Slice_kind &&
           s->v.Slice.step == NULL;
}

static int
compiler_augassign(struct compiler *c, stmt_ty s)
{
    assert(s->kind == AugAssign_kind);
    expr_ty e = s->v.AugAssign.target;

    location loc = LOC(e);

    switch (e->kind) {
    case Attribute_kind:
        VISIT(c, expr, e->v.Attribute.value);
        ADDOP_I(c, loc, COPY, 1);
        loc = update_start_location_to_match_attr(c, loc, e);
        ADDOP_NAME(c, loc, LOAD_ATTR, e->v.Attribute.attr, names);
        break;
    case Subscript_kind:
        VISIT(c, expr, e->v.Subscript.value);
        if (is_two_element_slice(e->v.Subscript.slice)) {
            RETURN_IF_ERROR(compiler_slice(c, e->v.Subscript.slice));
            ADDOP_I(c, loc, COPY, 3);
            ADDOP_I(c, loc, COPY, 3);
            ADDOP_I(c, loc, COPY, 3);
            ADDOP(c, loc, BINARY_SLICE);
        }
        else {
            VISIT(c, expr, e->v.Subscript.slice);
            ADDOP_I(c, loc, COPY, 2);
            ADDOP_I(c, loc, COPY, 2);
            ADDOP(c, loc, BINARY_SUBSCR);
        }
        break;
    case Name_kind:
        RETURN_IF_ERROR(compiler_nameop(c, loc, e->v.Name.id, Load));
        break;
    default:
        PyErr_Format(PyExc_SystemError,
            "invalid node type (%d) for augmented assignment",
            e->kind);
        return ERROR;
    }

    loc = LOC(s);

    VISIT(c, expr, s->v.AugAssign.value);
    ADDOP_INPLACE(c, loc, s->v.AugAssign.op);

    loc = LOC(e);

    switch (e->kind) {
    case Attribute_kind:
        loc = update_start_location_to_match_attr(c, loc, e);
        ADDOP_I(c, loc, SWAP, 2);
        ADDOP_NAME(c, loc, STORE_ATTR, e->v.Attribute.attr, names);
        break;
    case Subscript_kind:
        if (is_two_element_slice(e->v.Subscript.slice)) {
            ADDOP_I(c, loc, SWAP, 4);
            ADDOP_I(c, loc, SWAP, 3);
            ADDOP_I(c, loc, SWAP, 2);
            ADDOP(c, loc, STORE_SLICE);
        }
        else {
            ADDOP_I(c, loc, SWAP, 3);
            ADDOP_I(c, loc, SWAP, 2);
            ADDOP(c, loc, STORE_SUBSCR);
        }
        break;
    case Name_kind:
        return compiler_nameop(c, loc, e->v.Name.id, Store);
    default:
        Py_UNREACHABLE();
    }
    return SUCCESS;
}

static int
check_ann_expr(struct compiler *c, expr_ty e)
{
    VISIT(c, expr, e);
    ADDOP(c, LOC(e), POP_TOP);
    return SUCCESS;
}

static int
check_annotation(struct compiler *c, stmt_ty s)
{
    /* Annotations of complex targets does not produce anything
       under annotations future */
    if (c->c_future.ff_features & CO_FUTURE_ANNOTATIONS) {
        return SUCCESS;
    }

    /* Annotations are only evaluated in a module or class. */
    if (c->u->u_scope_type == COMPILER_SCOPE_MODULE ||
        c->u->u_scope_type == COMPILER_SCOPE_CLASS) {
        return check_ann_expr(c, s->v.AnnAssign.annotation);
    }
    return SUCCESS;
}

static int
check_ann_subscr(struct compiler *c, expr_ty e)
{
    /* We check that everything in a subscript is defined at runtime. */
    switch (e->kind) {
    case Slice_kind:
        if (e->v.Slice.lower && check_ann_expr(c, e->v.Slice.lower) < 0) {
            return ERROR;
        }
        if (e->v.Slice.upper && check_ann_expr(c, e->v.Slice.upper) < 0) {
            return ERROR;
        }
        if (e->v.Slice.step && check_ann_expr(c, e->v.Slice.step) < 0) {
            return ERROR;
        }
        return SUCCESS;
    case Tuple_kind: {
        /* extended slice */
        asdl_expr_seq *elts = e->v.Tuple.elts;
        Py_ssize_t i, n = asdl_seq_LEN(elts);
        for (i = 0; i < n; i++) {
            RETURN_IF_ERROR(check_ann_subscr(c, asdl_seq_GET(elts, i)));
        }
        return SUCCESS;
    }
    default:
        return check_ann_expr(c, e);
    }
}

static int
compiler_annassign(struct compiler *c, stmt_ty s)
{
    location loc = LOC(s);
    expr_ty targ = s->v.AnnAssign.target;
    PyObject* mangled;

    assert(s->kind == AnnAssign_kind);

    /* We perform the actual assignment first. */
    if (s->v.AnnAssign.value) {
        VISIT(c, expr, s->v.AnnAssign.value);
        VISIT(c, expr, targ);
    }
    switch (targ->kind) {
    case Name_kind:
        if (forbidden_name(c, loc, targ->v.Name.id, Store)) {
            return ERROR;
        }
        /* If we have a simple name in a module or class, store annotation. */
        if (s->v.AnnAssign.simple &&
            (c->u->u_scope_type == COMPILER_SCOPE_MODULE ||
             c->u->u_scope_type == COMPILER_SCOPE_CLASS)) {
            if (c->c_future.ff_features & CO_FUTURE_ANNOTATIONS) {
                VISIT(c, annexpr, s->v.AnnAssign.annotation)
            }
            else {
                VISIT(c, expr, s->v.AnnAssign.annotation);
            }
            ADDOP_NAME(c, loc, LOAD_NAME, &_Py_ID(__annotations__), names);
            mangled = _Py_Mangle(c->u->u_private, targ->v.Name.id);
            ADDOP_LOAD_CONST_NEW(c, loc, mangled);
            ADDOP(c, loc, STORE_SUBSCR);
        }
        break;
    case Attribute_kind:
        if (forbidden_name(c, loc, targ->v.Attribute.attr, Store)) {
            return ERROR;
        }
        if (!s->v.AnnAssign.value &&
            check_ann_expr(c, targ->v.Attribute.value) < 0) {
            return ERROR;
        }
        break;
    case Subscript_kind:
        if (!s->v.AnnAssign.value &&
            (check_ann_expr(c, targ->v.Subscript.value) < 0 ||
             check_ann_subscr(c, targ->v.Subscript.slice) < 0)) {
                return ERROR;
        }
        break;
    default:
        PyErr_Format(PyExc_SystemError,
                     "invalid node type (%d) for annotated assignment",
                     targ->kind);
        return ERROR;
    }
    /* Annotation is evaluated last. */
    if (!s->v.AnnAssign.simple && check_annotation(c, s) < 0) {
        return ERROR;
    }
    return SUCCESS;
}

/* Raises a SyntaxError and returns 0.
   If something goes wrong, a different exception may be raised.
*/

static int
compiler_error(struct compiler *c, location loc,
               const char *format, ...)
{
    va_list vargs;
    va_start(vargs, format);
    PyObject *msg = PyUnicode_FromFormatV(format, vargs);
    va_end(vargs);
    if (msg == NULL) {
        return ERROR;
    }
    PyObject *loc_obj = PyErr_ProgramTextObject(c->c_filename, loc.lineno);
    if (loc_obj == NULL) {
        loc_obj = Py_NewRef(Py_None);
    }
    PyObject *args = Py_BuildValue("O(OiiOii)", msg, c->c_filename,
                                   loc.lineno, loc.col_offset + 1, loc_obj,
                                   loc.end_lineno, loc.end_col_offset + 1);
    Py_DECREF(msg);
    if (args == NULL) {
        goto exit;
    }
    PyErr_SetObject(PyExc_SyntaxError, args);
 exit:
    Py_DECREF(loc_obj);
    Py_XDECREF(args);
    return ERROR;
}

/* Emits a SyntaxWarning and returns 1 on success.
   If a SyntaxWarning raised as error, replaces it with a SyntaxError
   and returns 0.
*/
static int
compiler_warn(struct compiler *c, location loc,
              const char *format, ...)
{
    va_list vargs;
    va_start(vargs, format);
    PyObject *msg = PyUnicode_FromFormatV(format, vargs);
    va_end(vargs);
    if (msg == NULL) {
        return ERROR;
    }
    if (PyErr_WarnExplicitObject(PyExc_SyntaxWarning, msg, c->c_filename,
                                 loc.lineno, NULL, NULL) < 0)
    {
        if (PyErr_ExceptionMatches(PyExc_SyntaxWarning)) {
            /* Replace the SyntaxWarning exception with a SyntaxError
               to get a more accurate error report */
            PyErr_Clear();
            assert(PyUnicode_AsUTF8(msg) != NULL);
            compiler_error(c, loc, PyUnicode_AsUTF8(msg));
        }
        Py_DECREF(msg);
        return ERROR;
    }
    Py_DECREF(msg);
    return SUCCESS;
}

static int
compiler_subscript(struct compiler *c, expr_ty e)
{
    location loc = LOC(e);
    expr_context_ty ctx = e->v.Subscript.ctx;
    int op = 0;

    if (ctx == Load) {
        RETURN_IF_ERROR(check_subscripter(c, e->v.Subscript.value));
        RETURN_IF_ERROR(check_index(c, e->v.Subscript.value, e->v.Subscript.slice));
    }

    VISIT(c, expr, e->v.Subscript.value);
    if (is_two_element_slice(e->v.Subscript.slice) && ctx != Del) {
        RETURN_IF_ERROR(compiler_slice(c, e->v.Subscript.slice));
        if (ctx == Load) {
            ADDOP(c, loc, BINARY_SLICE);
        }
        else {
            assert(ctx == Store);
            ADDOP(c, loc, STORE_SLICE);
        }
    }
    else {
        VISIT(c, expr, e->v.Subscript.slice);
        switch (ctx) {
            case Load:    op = BINARY_SUBSCR; break;
            case Store:   op = STORE_SUBSCR; break;
            case Del:     op = DELETE_SUBSCR; break;
        }
        assert(op);
        ADDOP(c, loc, op);
    }
    return SUCCESS;
}

/* Returns the number of the values emitted,
 * thus are needed to build the slice, or -1 if there is an error. */
static int
compiler_slice(struct compiler *c, expr_ty s)
{
    int n = 2;
    assert(s->kind == Slice_kind);

    /* only handles the cases where BUILD_SLICE is emitted */
    if (s->v.Slice.lower) {
        VISIT(c, expr, s->v.Slice.lower);
    }
    else {
        ADDOP_LOAD_CONST(c, LOC(s), Py_None);
    }

    if (s->v.Slice.upper) {
        VISIT(c, expr, s->v.Slice.upper);
    }
    else {
        ADDOP_LOAD_CONST(c, LOC(s), Py_None);
    }

    if (s->v.Slice.step) {
        n++;
        VISIT(c, expr, s->v.Slice.step);
    }
    return n;
}


// PEP 634: Structural Pattern Matching

// To keep things simple, all compiler_pattern_* and pattern_helper_* routines
// follow the convention of consuming TOS (the subject for the given pattern)
// and calling jump_to_fail_pop on failure (no match).

// When calling into these routines, it's important that pc->on_top be kept
// updated to reflect the current number of items that we are using on the top
// of the stack: they will be popped on failure, and any name captures will be
// stored *underneath* them on success. This lets us defer all names stores
// until the *entire* pattern matches.

#define WILDCARD_CHECK(N) \
    ((N)->kind == MatchAs_kind && !(N)->v.MatchAs.name)

#define WILDCARD_STAR_CHECK(N) \
    ((N)->kind == MatchStar_kind && !(N)->v.MatchStar.name)

// Limit permitted subexpressions, even if the parser & AST validator let them through
#define MATCH_VALUE_EXPR(N) \
    ((N)->kind == Constant_kind || (N)->kind == Attribute_kind)

// Allocate or resize pc->fail_pop to allow for n items to be popped on failure.
static int
ensure_fail_pop(struct compiler *c, pattern_context *pc, Py_ssize_t n)
{
    Py_ssize_t size = n + 1;
    if (size <= pc->fail_pop_size) {
        return SUCCESS;
    }
    Py_ssize_t needed = sizeof(jump_target_label) * size;
    jump_target_label *resized = PyObject_Realloc(pc->fail_pop, needed);
    if (resized == NULL) {
        PyErr_NoMemory();
        return ERROR;
    }
    pc->fail_pop = resized;
    while (pc->fail_pop_size < size) {
        NEW_JUMP_TARGET_LABEL(c, new_block);
        pc->fail_pop[pc->fail_pop_size++] = new_block;
    }
    return SUCCESS;
}

// Use op to jump to the correct fail_pop block.
static int
jump_to_fail_pop(struct compiler *c, location loc,
                 pattern_context *pc, int op)
{
    // Pop any items on the top of the stack, plus any objects we were going to
    // capture on success:
    Py_ssize_t pops = pc->on_top + PyList_GET_SIZE(pc->stores);
    RETURN_IF_ERROR(ensure_fail_pop(c, pc, pops));
    ADDOP_JUMP(c, loc, op, pc->fail_pop[pops]);
    return SUCCESS;
}

// Build all of the fail_pop blocks and reset fail_pop.
static int
emit_and_reset_fail_pop(struct compiler *c, location loc,
                        pattern_context *pc)
{
    if (!pc->fail_pop_size) {
        assert(pc->fail_pop == NULL);
        return SUCCESS;
    }
    while (--pc->fail_pop_size) {
        USE_LABEL(c, pc->fail_pop[pc->fail_pop_size]);
        if (codegen_addop_noarg(INSTR_SEQUENCE(c), POP_TOP, loc) < 0) {
            pc->fail_pop_size = 0;
            PyObject_Free(pc->fail_pop);
            pc->fail_pop = NULL;
            return ERROR;
        }
    }
    USE_LABEL(c, pc->fail_pop[0]);
    PyObject_Free(pc->fail_pop);
    pc->fail_pop = NULL;
    return SUCCESS;
}

static int
compiler_error_duplicate_store(struct compiler *c, location loc, identifier n)
{
    return compiler_error(c, loc,
        "multiple assignments to name %R in pattern", n);
}

// Duplicate the effect of 3.10's ROT_* instructions using SWAPs.
static int
pattern_helper_rotate(struct compiler *c, location loc, Py_ssize_t count)
{
    while (1 < count) {
        ADDOP_I(c, loc, SWAP, count--);
    }
    return SUCCESS;
}

static int
pattern_helper_store_name(struct compiler *c, location loc,
                          identifier n, pattern_context *pc)
{
    if (n == NULL) {
        ADDOP(c, loc, POP_TOP);
        return SUCCESS;
    }
    if (forbidden_name(c, loc, n, Store)) {
        return ERROR;
    }
    // Can't assign to the same name twice:
    int duplicate = PySequence_Contains(pc->stores, n);
    RETURN_IF_ERROR(duplicate);
    if (duplicate) {
        return compiler_error_duplicate_store(c, loc, n);
    }
    // Rotate this object underneath any items we need to preserve:
    Py_ssize_t rotations = pc->on_top + PyList_GET_SIZE(pc->stores) + 1;
    RETURN_IF_ERROR(pattern_helper_rotate(c, loc, rotations));
    RETURN_IF_ERROR(PyList_Append(pc->stores, n));
    return SUCCESS;
}


static int
pattern_unpack_helper(struct compiler *c, location loc,
                      asdl_pattern_seq *elts)
{
    Py_ssize_t n = asdl_seq_LEN(elts);
    int seen_star = 0;
    for (Py_ssize_t i = 0; i < n; i++) {
        pattern_ty elt = asdl_seq_GET(elts, i);
        if (elt->kind == MatchStar_kind && !seen_star) {
            if ((i >= (1 << 8)) ||
                (n-i-1 >= (INT_MAX >> 8))) {
                return compiler_error(c, loc,
                    "too many expressions in "
                    "star-unpacking sequence pattern");
            }
            ADDOP_I(c, loc, UNPACK_EX, (i + ((n-i-1) << 8)));
            seen_star = 1;
        }
        else if (elt->kind == MatchStar_kind) {
            return compiler_error(c, loc,
                "multiple starred expressions in sequence pattern");
        }
    }
    if (!seen_star) {
        ADDOP_I(c, loc, UNPACK_SEQUENCE, n);
    }
    return SUCCESS;
}

static int
pattern_helper_sequence_unpack(struct compiler *c, location loc,
                               asdl_pattern_seq *patterns, Py_ssize_t star,
                               pattern_context *pc)
{
    RETURN_IF_ERROR(pattern_unpack_helper(c, loc, patterns));
    Py_ssize_t size = asdl_seq_LEN(patterns);
    // We've now got a bunch of new subjects on the stack. They need to remain
    // there after each subpattern match:
    pc->on_top += size;
    for (Py_ssize_t i = 0; i < size; i++) {
        // One less item to keep track of each time we loop through:
        pc->on_top--;
        pattern_ty pattern = asdl_seq_GET(patterns, i);
        RETURN_IF_ERROR(compiler_pattern_subpattern(c, pattern, pc));
    }
    return SUCCESS;
}

// Like pattern_helper_sequence_unpack, but uses BINARY_SUBSCR instead of
// UNPACK_SEQUENCE / UNPACK_EX. This is more efficient for patterns with a
// starred wildcard like [first, *_] / [first, *_, last] / [*_, last] / etc.
static int
pattern_helper_sequence_subscr(struct compiler *c, location loc,
                               asdl_pattern_seq *patterns, Py_ssize_t star,
                               pattern_context *pc)
{
    // We need to keep the subject around for extracting elements:
    pc->on_top++;
    Py_ssize_t size = asdl_seq_LEN(patterns);
    for (Py_ssize_t i = 0; i < size; i++) {
        pattern_ty pattern = asdl_seq_GET(patterns, i);
        if (WILDCARD_CHECK(pattern)) {
            continue;
        }
        if (i == star) {
            assert(WILDCARD_STAR_CHECK(pattern));
            continue;
        }
        ADDOP_I(c, loc, COPY, 1);
        if (i < star) {
            ADDOP_LOAD_CONST_NEW(c, loc, PyLong_FromSsize_t(i));
        }
        else {
            // The subject may not support negative indexing! Compute a
            // nonnegative index:
            ADDOP(c, loc, GET_LEN);
            ADDOP_LOAD_CONST_NEW(c, loc, PyLong_FromSsize_t(size - i));
            ADDOP_BINARY(c, loc, Sub);
        }
        ADDOP(c, loc, BINARY_SUBSCR);
        RETURN_IF_ERROR(compiler_pattern_subpattern(c, pattern, pc));
    }
    // Pop the subject, we're done with it:
    pc->on_top--;
    ADDOP(c, loc, POP_TOP);
    return SUCCESS;
}

// Like compiler_pattern, but turn off checks for irrefutability.
static int
compiler_pattern_subpattern(struct compiler *c,
                            pattern_ty p, pattern_context *pc)
{
    int allow_irrefutable = pc->allow_irrefutable;
    pc->allow_irrefutable = 1;
    RETURN_IF_ERROR(compiler_pattern(c, p, pc));
    pc->allow_irrefutable = allow_irrefutable;
    return SUCCESS;
}

static int
compiler_pattern_as(struct compiler *c, pattern_ty p, pattern_context *pc)
{
    assert(p->kind == MatchAs_kind);
    if (p->v.MatchAs.pattern == NULL) {
        // An irrefutable match:
        if (!pc->allow_irrefutable) {
            if (p->v.MatchAs.name) {
                const char *e = "name capture %R makes remaining patterns unreachable";
                return compiler_error(c, LOC(p), e, p->v.MatchAs.name);
            }
            const char *e = "wildcard makes remaining patterns unreachable";
            return compiler_error(c, LOC(p), e);
        }
        return pattern_helper_store_name(c, LOC(p), p->v.MatchAs.name, pc);
    }
    // Need to make a copy for (possibly) storing later:
    pc->on_top++;
    ADDOP_I(c, LOC(p), COPY, 1);
    RETURN_IF_ERROR(compiler_pattern(c, p->v.MatchAs.pattern, pc));
    // Success! Store it:
    pc->on_top--;
    RETURN_IF_ERROR(pattern_helper_store_name(c, LOC(p), p->v.MatchAs.name, pc));
    return SUCCESS;
}

static int
compiler_pattern_star(struct compiler *c, pattern_ty p, pattern_context *pc)
{
    assert(p->kind == MatchStar_kind);
    RETURN_IF_ERROR(
        pattern_helper_store_name(c, LOC(p), p->v.MatchStar.name, pc));
    return SUCCESS;
}

static int
validate_kwd_attrs(struct compiler *c, asdl_identifier_seq *attrs, asdl_pattern_seq* patterns)
{
    // Any errors will point to the pattern rather than the arg name as the
    // parser is only supplying identifiers rather than Name or keyword nodes
    Py_ssize_t nattrs = asdl_seq_LEN(attrs);
    for (Py_ssize_t i = 0; i < nattrs; i++) {
        identifier attr = ((identifier)asdl_seq_GET(attrs, i));
        location loc = LOC((pattern_ty) asdl_seq_GET(patterns, i));
        if (forbidden_name(c, loc, attr, Store)) {
            return ERROR;
        }
        for (Py_ssize_t j = i + 1; j < nattrs; j++) {
            identifier other = ((identifier)asdl_seq_GET(attrs, j));
            if (!PyUnicode_Compare(attr, other)) {
                location loc = LOC((pattern_ty) asdl_seq_GET(patterns, j));
                compiler_error(c, loc, "attribute name repeated in class pattern: %U", attr);
                return ERROR;
            }
        }
    }
    return SUCCESS;
}

static int
compiler_pattern_class(struct compiler *c, pattern_ty p, pattern_context *pc)
{
    assert(p->kind == MatchClass_kind);
    asdl_pattern_seq *patterns = p->v.MatchClass.patterns;
    asdl_identifier_seq *kwd_attrs = p->v.MatchClass.kwd_attrs;
    asdl_pattern_seq *kwd_patterns = p->v.MatchClass.kwd_patterns;
    Py_ssize_t nargs = asdl_seq_LEN(patterns);
    Py_ssize_t nattrs = asdl_seq_LEN(kwd_attrs);
    Py_ssize_t nkwd_patterns = asdl_seq_LEN(kwd_patterns);
    if (nattrs != nkwd_patterns) {
        // AST validator shouldn't let this happen, but if it does,
        // just fail, don't crash out of the interpreter
        const char * e = "kwd_attrs (%d) / kwd_patterns (%d) length mismatch in class pattern";
        return compiler_error(c, LOC(p), e, nattrs, nkwd_patterns);
    }
    if (INT_MAX < nargs || INT_MAX < nargs + nattrs - 1) {
        const char *e = "too many sub-patterns in class pattern %R";
        return compiler_error(c, LOC(p), e, p->v.MatchClass.cls);
    }
    if (nattrs) {
        RETURN_IF_ERROR(validate_kwd_attrs(c, kwd_attrs, kwd_patterns));
    }
    VISIT(c, expr, p->v.MatchClass.cls);
    PyObject *attr_names = PyTuple_New(nattrs);
    if (attr_names == NULL) {
        return ERROR;
    }
    Py_ssize_t i;
    for (i = 0; i < nattrs; i++) {
        PyObject *name = asdl_seq_GET(kwd_attrs, i);
        PyTuple_SET_ITEM(attr_names, i, Py_NewRef(name));
    }
    ADDOP_LOAD_CONST_NEW(c, LOC(p), attr_names);
    ADDOP_I(c, LOC(p), MATCH_CLASS, nargs);
    ADDOP_I(c, LOC(p), COPY, 1);
    ADDOP_LOAD_CONST(c, LOC(p), Py_None);
    ADDOP_I(c, LOC(p), IS_OP, 1);
    // TOS is now a tuple of (nargs + nattrs) attributes (or None):
    pc->on_top++;
    RETURN_IF_ERROR(jump_to_fail_pop(c, LOC(p), pc, POP_JUMP_IF_FALSE));
    ADDOP_I(c, LOC(p), UNPACK_SEQUENCE, nargs + nattrs);
    pc->on_top += nargs + nattrs - 1;
    for (i = 0; i < nargs + nattrs; i++) {
        pc->on_top--;
        pattern_ty pattern;
        if (i < nargs) {
            // Positional:
            pattern = asdl_seq_GET(patterns, i);
        }
        else {
            // Keyword:
            pattern = asdl_seq_GET(kwd_patterns, i - nargs);
        }
        if (WILDCARD_CHECK(pattern)) {
            ADDOP(c, LOC(p), POP_TOP);
            continue;
        }
        RETURN_IF_ERROR(compiler_pattern_subpattern(c, pattern, pc));
    }
    // Success! Pop the tuple of attributes:
    return SUCCESS;
}

static int
compiler_pattern_mapping(struct compiler *c, pattern_ty p,
                         pattern_context *pc)
{
    assert(p->kind == MatchMapping_kind);
    asdl_expr_seq *keys = p->v.MatchMapping.keys;
    asdl_pattern_seq *patterns = p->v.MatchMapping.patterns;
    Py_ssize_t size = asdl_seq_LEN(keys);
    Py_ssize_t npatterns = asdl_seq_LEN(patterns);
    if (size != npatterns) {
        // AST validator shouldn't let this happen, but if it does,
        // just fail, don't crash out of the interpreter
        const char * e = "keys (%d) / patterns (%d) length mismatch in mapping pattern";
        return compiler_error(c, LOC(p), e, size, npatterns);
    }
    // We have a double-star target if "rest" is set
    PyObject *star_target = p->v.MatchMapping.rest;
    // We need to keep the subject on top during the mapping and length checks:
    pc->on_top++;
    ADDOP(c, LOC(p), MATCH_MAPPING);
    RETURN_IF_ERROR(jump_to_fail_pop(c, LOC(p), pc, POP_JUMP_IF_FALSE));
    if (!size && !star_target) {
        // If the pattern is just "{}", we're done! Pop the subject:
        pc->on_top--;
        ADDOP(c, LOC(p), POP_TOP);
        return SUCCESS;
    }
    if (size) {
        // If the pattern has any keys in it, perform a length check:
        ADDOP(c, LOC(p), GET_LEN);
        ADDOP_LOAD_CONST_NEW(c, LOC(p), PyLong_FromSsize_t(size));
        ADDOP_COMPARE(c, LOC(p), GtE);
        RETURN_IF_ERROR(jump_to_fail_pop(c, LOC(p), pc, POP_JUMP_IF_FALSE));
    }
    if (INT_MAX < size - 1) {
        return compiler_error(c, LOC(p), "too many sub-patterns in mapping pattern");
    }
    // Collect all of the keys into a tuple for MATCH_KEYS and
    // **rest. They can either be dotted names or literals:

    // Maintaining a set of Constant_kind kind keys allows us to raise a
    // SyntaxError in the case of duplicates.
    PyObject *seen = PySet_New(NULL);
    if (seen == NULL) {
        return ERROR;
    }

    // NOTE: goto error on failure in the loop below to avoid leaking `seen`
    for (Py_ssize_t i = 0; i < size; i++) {
        expr_ty key = asdl_seq_GET(keys, i);
        if (key == NULL) {
            const char *e = "can't use NULL keys in MatchMapping "
                            "(set 'rest' parameter instead)";
            location loc = LOC((pattern_ty) asdl_seq_GET(patterns, i));
            compiler_error(c, loc, e);
            goto error;
        }

        if (key->kind == Constant_kind) {
            int in_seen = PySet_Contains(seen, key->v.Constant.value);
            if (in_seen < 0) {
                goto error;
            }
            if (in_seen) {
                const char *e = "mapping pattern checks duplicate key (%R)";
                compiler_error(c, LOC(p), e, key->v.Constant.value);
                goto error;
            }
            if (PySet_Add(seen, key->v.Constant.value)) {
                goto error;
            }
        }

        else if (key->kind != Attribute_kind) {
            const char *e = "mapping pattern keys may only match literals and attribute lookups";
            compiler_error(c, LOC(p), e);
            goto error;
        }
        if (compiler_visit_expr(c, key) < 0) {
            goto error;
        }
    }

    // all keys have been checked; there are no duplicates
    Py_DECREF(seen);

    ADDOP_I(c, LOC(p), BUILD_TUPLE, size);
    ADDOP(c, LOC(p), MATCH_KEYS);
    // There's now a tuple of keys and a tuple of values on top of the subject:
    pc->on_top += 2;
    ADDOP_I(c, LOC(p), COPY, 1);
    ADDOP_LOAD_CONST(c, LOC(p), Py_None);
    ADDOP_I(c, LOC(p), IS_OP, 1);
    RETURN_IF_ERROR(jump_to_fail_pop(c, LOC(p), pc, POP_JUMP_IF_FALSE));
    // So far so good. Use that tuple of values on the stack to match
    // sub-patterns against:
    ADDOP_I(c, LOC(p), UNPACK_SEQUENCE, size);
    pc->on_top += size - 1;
    for (Py_ssize_t i = 0; i < size; i++) {
        pc->on_top--;
        pattern_ty pattern = asdl_seq_GET(patterns, i);
        RETURN_IF_ERROR(compiler_pattern_subpattern(c, pattern, pc));
    }
    // If we get this far, it's a match! Whatever happens next should consume
    // the tuple of keys and the subject:
    pc->on_top -= 2;
    if (star_target) {
        // If we have a starred name, bind a dict of remaining items to it (this may
        // seem a bit inefficient, but keys is rarely big enough to actually impact
        // runtime):
        // rest = dict(TOS1)
        // for key in TOS:
        //     del rest[key]
        ADDOP_I(c, LOC(p), BUILD_MAP, 0);           // [subject, keys, empty]
        ADDOP_I(c, LOC(p), SWAP, 3);                // [empty, keys, subject]
        ADDOP_I(c, LOC(p), DICT_UPDATE, 2);         // [copy, keys]
        ADDOP_I(c, LOC(p), UNPACK_SEQUENCE, size);  // [copy, keys...]
        while (size) {
            ADDOP_I(c, LOC(p), COPY, 1 + size--);   // [copy, keys..., copy]
            ADDOP_I(c, LOC(p), SWAP, 2);            // [copy, keys..., copy, key]
            ADDOP(c, LOC(p), DELETE_SUBSCR);        // [copy, keys...]
        }
        RETURN_IF_ERROR(pattern_helper_store_name(c, LOC(p), star_target, pc));
    }
    else {
        ADDOP(c, LOC(p), POP_TOP);  // Tuple of keys.
        ADDOP(c, LOC(p), POP_TOP);  // Subject.
    }
    return SUCCESS;

error:
    Py_DECREF(seen);
    return ERROR;
}

static int
compiler_pattern_or(struct compiler *c, pattern_ty p, pattern_context *pc)
{
    assert(p->kind == MatchOr_kind);
    NEW_JUMP_TARGET_LABEL(c, end);
    Py_ssize_t size = asdl_seq_LEN(p->v.MatchOr.patterns);
    assert(size > 1);
    // We're going to be messing with pc. Keep the original info handy:
    pattern_context old_pc = *pc;
    Py_INCREF(pc->stores);
    // control is the list of names bound by the first alternative. It is used
    // for checking different name bindings in alternatives, and for correcting
    // the order in which extracted elements are placed on the stack.
    PyObject *control = NULL;
    // NOTE: We can't use returning macros anymore! goto error on error.
    for (Py_ssize_t i = 0; i < size; i++) {
        pattern_ty alt = asdl_seq_GET(p->v.MatchOr.patterns, i);
        PyObject *pc_stores = PyList_New(0);
        if (pc_stores == NULL) {
            goto error;
        }
        Py_SETREF(pc->stores, pc_stores);
        // An irrefutable sub-pattern must be last, if it is allowed at all:
        pc->allow_irrefutable = (i == size - 1) && old_pc.allow_irrefutable;
        pc->fail_pop = NULL;
        pc->fail_pop_size = 0;
        pc->on_top = 0;
        if (codegen_addop_i(INSTR_SEQUENCE(c), COPY, 1, LOC(alt)) < 0 ||
            compiler_pattern(c, alt, pc) < 0) {
            goto error;
        }
        // Success!
        Py_ssize_t nstores = PyList_GET_SIZE(pc->stores);
        if (!i) {
            // This is the first alternative, so save its stores as a "control"
            // for the others (they can't bind a different set of names, and
            // might need to be reordered):
            assert(control == NULL);
            control = Py_NewRef(pc->stores);
        }
        else if (nstores != PyList_GET_SIZE(control)) {
            goto diff;
        }
        else if (nstores) {
            // There were captures. Check to see if we differ from control:
            Py_ssize_t icontrol = nstores;
            while (icontrol--) {
                PyObject *name = PyList_GET_ITEM(control, icontrol);
                Py_ssize_t istores = PySequence_Index(pc->stores, name);
                if (istores < 0) {
                    PyErr_Clear();
                    goto diff;
                }
                if (icontrol != istores) {
                    // Reorder the names on the stack to match the order of the
                    // names in control. There's probably a better way of doing
                    // this; the current solution is potentially very
                    // inefficient when each alternative subpattern binds lots
                    // of names in different orders. It's fine for reasonable
                    // cases, though, and the peephole optimizer will ensure
                    // that the final code is as efficient as possible.
                    assert(istores < icontrol);
                    Py_ssize_t rotations = istores + 1;
                    // Perform the same rotation on pc->stores:
                    PyObject *rotated = PyList_GetSlice(pc->stores, 0,
                                                        rotations);
                    if (rotated == NULL ||
                        PyList_SetSlice(pc->stores, 0, rotations, NULL) ||
                        PyList_SetSlice(pc->stores, icontrol - istores,
                                        icontrol - istores, rotated))
                    {
                        Py_XDECREF(rotated);
                        goto error;
                    }
                    Py_DECREF(rotated);
                    // That just did:
                    // rotated = pc_stores[:rotations]
                    // del pc_stores[:rotations]
                    // pc_stores[icontrol-istores:icontrol-istores] = rotated
                    // Do the same thing to the stack, using several
                    // rotations:
                    while (rotations--) {
                        if (pattern_helper_rotate(c, LOC(alt), icontrol + 1) < 0) {
                            goto error;
                        }
                    }
                }
            }
        }
        assert(control);
        if (codegen_addop_j(INSTR_SEQUENCE(c), LOC(alt), JUMP, end) < 0 ||
            emit_and_reset_fail_pop(c, LOC(alt), pc) < 0)
        {
            goto error;
        }
    }
    Py_DECREF(pc->stores);
    *pc = old_pc;
    Py_INCREF(pc->stores);
    // Need to NULL this for the PyObject_Free call in the error block.
    old_pc.fail_pop = NULL;
    // No match. Pop the remaining copy of the subject and fail:
    if (codegen_addop_noarg(INSTR_SEQUENCE(c), POP_TOP, LOC(p)) < 0 ||
        jump_to_fail_pop(c, LOC(p), pc, JUMP) < 0) {
        goto error;
    }

    USE_LABEL(c, end);
    Py_ssize_t nstores = PyList_GET_SIZE(control);
    // There's a bunch of stuff on the stack between where the new stores
    // are and where they need to be:
    // - The other stores.
    // - A copy of the subject.
    // - Anything else that may be on top of the stack.
    // - Any previous stores we've already stashed away on the stack.
    Py_ssize_t nrots = nstores + 1 + pc->on_top + PyList_GET_SIZE(pc->stores);
    for (Py_ssize_t i = 0; i < nstores; i++) {
        // Rotate this capture to its proper place on the stack:
        if (pattern_helper_rotate(c, LOC(p), nrots) < 0) {
            goto error;
        }
        // Update the list of previous stores with this new name, checking for
        // duplicates:
        PyObject *name = PyList_GET_ITEM(control, i);
        int dupe = PySequence_Contains(pc->stores, name);
        if (dupe < 0) {
            goto error;
        }
        if (dupe) {
            compiler_error_duplicate_store(c, LOC(p), name);
            goto error;
        }
        if (PyList_Append(pc->stores, name)) {
            goto error;
        }
    }
    Py_DECREF(old_pc.stores);
    Py_DECREF(control);
    // NOTE: Returning macros are safe again.
    // Pop the copy of the subject:
    ADDOP(c, LOC(p), POP_TOP);
    return SUCCESS;
diff:
    compiler_error(c, LOC(p), "alternative patterns bind different names");
error:
    PyObject_Free(old_pc.fail_pop);
    Py_DECREF(old_pc.stores);
    Py_XDECREF(control);
    return ERROR;
}


static int
compiler_pattern_sequence(struct compiler *c, pattern_ty p,
                          pattern_context *pc)
{
    assert(p->kind == MatchSequence_kind);
    asdl_pattern_seq *patterns = p->v.MatchSequence.patterns;
    Py_ssize_t size = asdl_seq_LEN(patterns);
    Py_ssize_t star = -1;
    int only_wildcard = 1;
    int star_wildcard = 0;
    // Find a starred name, if it exists. There may be at most one:
    for (Py_ssize_t i = 0; i < size; i++) {
        pattern_ty pattern = asdl_seq_GET(patterns, i);
        if (pattern->kind == MatchStar_kind) {
            if (star >= 0) {
                const char *e = "multiple starred names in sequence pattern";
                return compiler_error(c, LOC(p), e);
            }
            star_wildcard = WILDCARD_STAR_CHECK(pattern);
            only_wildcard &= star_wildcard;
            star = i;
            continue;
        }
        only_wildcard &= WILDCARD_CHECK(pattern);
    }
    // We need to keep the subject on top during the sequence and length checks:
    pc->on_top++;
    ADDOP(c, LOC(p), MATCH_SEQUENCE);
    RETURN_IF_ERROR(jump_to_fail_pop(c, LOC(p), pc, POP_JUMP_IF_FALSE));
    if (star < 0) {
        // No star: len(subject) == size
        ADDOP(c, LOC(p), GET_LEN);
        ADDOP_LOAD_CONST_NEW(c, LOC(p), PyLong_FromSsize_t(size));
        ADDOP_COMPARE(c, LOC(p), Eq);
        RETURN_IF_ERROR(jump_to_fail_pop(c, LOC(p), pc, POP_JUMP_IF_FALSE));
    }
    else if (size > 1) {
        // Star: len(subject) >= size - 1
        ADDOP(c, LOC(p), GET_LEN);
        ADDOP_LOAD_CONST_NEW(c, LOC(p), PyLong_FromSsize_t(size - 1));
        ADDOP_COMPARE(c, LOC(p), GtE);
        RETURN_IF_ERROR(jump_to_fail_pop(c, LOC(p), pc, POP_JUMP_IF_FALSE));
    }
    // Whatever comes next should consume the subject:
    pc->on_top--;
    if (only_wildcard) {
        // Patterns like: [] / [_] / [_, _] / [*_] / [_, *_] / [_, _, *_] / etc.
        ADDOP(c, LOC(p), POP_TOP);
    }
    else if (star_wildcard) {
        RETURN_IF_ERROR(pattern_helper_sequence_subscr(c, LOC(p), patterns, star, pc));
    }
    else {
        RETURN_IF_ERROR(pattern_helper_sequence_unpack(c, LOC(p), patterns, star, pc));
    }
    return SUCCESS;
}

static int
compiler_pattern_value(struct compiler *c, pattern_ty p, pattern_context *pc)
{
    assert(p->kind == MatchValue_kind);
    expr_ty value = p->v.MatchValue.value;
    if (!MATCH_VALUE_EXPR(value)) {
        const char *e = "patterns may only match literals and attribute lookups";
        return compiler_error(c, LOC(p), e);
    }
    VISIT(c, expr, value);
    ADDOP_COMPARE(c, LOC(p), Eq);
    RETURN_IF_ERROR(jump_to_fail_pop(c, LOC(p), pc, POP_JUMP_IF_FALSE));
    return SUCCESS;
}

static int
compiler_pattern_singleton(struct compiler *c, pattern_ty p, pattern_context *pc)
{
    assert(p->kind == MatchSingleton_kind);
    ADDOP_LOAD_CONST(c, LOC(p), p->v.MatchSingleton.value);
    ADDOP_COMPARE(c, LOC(p), Is);
    RETURN_IF_ERROR(jump_to_fail_pop(c, LOC(p), pc, POP_JUMP_IF_FALSE));
    return SUCCESS;
}

static int
compiler_pattern(struct compiler *c, pattern_ty p, pattern_context *pc)
{
    switch (p->kind) {
        case MatchValue_kind:
            return compiler_pattern_value(c, p, pc);
        case MatchSingleton_kind:
            return compiler_pattern_singleton(c, p, pc);
        case MatchSequence_kind:
            return compiler_pattern_sequence(c, p, pc);
        case MatchMapping_kind:
            return compiler_pattern_mapping(c, p, pc);
        case MatchClass_kind:
            return compiler_pattern_class(c, p, pc);
        case MatchStar_kind:
            return compiler_pattern_star(c, p, pc);
        case MatchAs_kind:
            return compiler_pattern_as(c, p, pc);
        case MatchOr_kind:
            return compiler_pattern_or(c, p, pc);
    }
    // AST validator shouldn't let this happen, but if it does,
    // just fail, don't crash out of the interpreter
    const char *e = "invalid match pattern node in AST (kind=%d)";
    return compiler_error(c, LOC(p), e, p->kind);
}

static int
compiler_match_inner(struct compiler *c, stmt_ty s, pattern_context *pc)
{
    VISIT(c, expr, s->v.Match.subject);
    NEW_JUMP_TARGET_LABEL(c, end);
    Py_ssize_t cases = asdl_seq_LEN(s->v.Match.cases);
    assert(cases > 0);
    match_case_ty m = asdl_seq_GET(s->v.Match.cases, cases - 1);
    int has_default = WILDCARD_CHECK(m->pattern) && 1 < cases;
    for (Py_ssize_t i = 0; i < cases - has_default; i++) {
        m = asdl_seq_GET(s->v.Match.cases, i);
        // Only copy the subject if we're *not* on the last case:
        if (i != cases - has_default - 1) {
            ADDOP_I(c, LOC(m->pattern), COPY, 1);
        }
        pc->stores = PyList_New(0);
        if (pc->stores == NULL) {
            return ERROR;
        }
        // Irrefutable cases must be either guarded, last, or both:
        pc->allow_irrefutable = m->guard != NULL || i == cases - 1;
        pc->fail_pop = NULL;
        pc->fail_pop_size = 0;
        pc->on_top = 0;
        // NOTE: Can't use returning macros here (they'll leak pc->stores)!
        if (compiler_pattern(c, m->pattern, pc) < 0) {
            Py_DECREF(pc->stores);
            return ERROR;
        }
        assert(!pc->on_top);
        // It's a match! Store all of the captured names (they're on the stack).
        Py_ssize_t nstores = PyList_GET_SIZE(pc->stores);
        for (Py_ssize_t n = 0; n < nstores; n++) {
            PyObject *name = PyList_GET_ITEM(pc->stores, n);
            if (compiler_nameop(c, LOC(m->pattern), name, Store) < 0) {
                Py_DECREF(pc->stores);
                return ERROR;
            }
        }
        Py_DECREF(pc->stores);
        // NOTE: Returning macros are safe again.
        if (m->guard) {
            RETURN_IF_ERROR(ensure_fail_pop(c, pc, 0));
            RETURN_IF_ERROR(compiler_jump_if(c, LOC(m->pattern), m->guard, pc->fail_pop[0], 0));
        }
        // Success! Pop the subject off, we're done with it:
        if (i != cases - has_default - 1) {
            ADDOP(c, LOC(m->pattern), POP_TOP);
        }
        VISIT_SEQ(c, stmt, m->body);
        ADDOP_JUMP(c, NO_LOCATION, JUMP, end);
        // If the pattern fails to match, we want the line number of the
        // cleanup to be associated with the failed pattern, not the last line
        // of the body
        RETURN_IF_ERROR(emit_and_reset_fail_pop(c, LOC(m->pattern), pc));
    }
    if (has_default) {
        // A trailing "case _" is common, and lets us save a bit of redundant
        // pushing and popping in the loop above:
        m = asdl_seq_GET(s->v.Match.cases, cases - 1);
        if (cases == 1) {
            // No matches. Done with the subject:
            ADDOP(c, LOC(m->pattern), POP_TOP);
        }
        else {
            // Show line coverage for default case (it doesn't create bytecode)
            ADDOP(c, LOC(m->pattern), NOP);
        }
        if (m->guard) {
            RETURN_IF_ERROR(compiler_jump_if(c, LOC(m->pattern), m->guard, end, 0));
        }
        VISIT_SEQ(c, stmt, m->body);
    }
    USE_LABEL(c, end);
    return SUCCESS;
}

static int
compiler_match(struct compiler *c, stmt_ty s)
{
    pattern_context pc;
    pc.fail_pop = NULL;
    int result = compiler_match_inner(c, s, &pc);
    PyObject_Free(pc.fail_pop);
    return result;
}

#undef WILDCARD_CHECK
#undef WILDCARD_STAR_CHECK

static PyObject *
consts_dict_keys_inorder(PyObject *dict)
{
    PyObject *consts, *k, *v;
    Py_ssize_t i, pos = 0, size = PyDict_GET_SIZE(dict);

    consts = PyList_New(size);   /* PyCode_Optimize() requires a list */
    if (consts == NULL)
        return NULL;
    while (PyDict_Next(dict, &pos, &k, &v)) {
        i = PyLong_AS_LONG(v);
        /* The keys of the dictionary can be tuples wrapping a constant.
         * (see dict_add_o and _PyCode_ConstantKey). In that case
         * the object we want is always second. */
        if (PyTuple_CheckExact(k)) {
            k = PyTuple_GET_ITEM(k, 1);
        }
        assert(i < size);
        assert(i >= 0);
        PyList_SET_ITEM(consts, i, Py_NewRef(k));
    }
    return consts;
}

static int
compute_code_flags(struct compiler *c)
{
    PySTEntryObject *ste = c->u->u_ste;
    int flags = 0;
    if (_PyST_IsFunctionLike(c->u->u_ste)) {
        flags |= CO_NEWLOCALS | CO_OPTIMIZED;
        if (ste->ste_nested)
            flags |= CO_NESTED;
        if (ste->ste_generator && !ste->ste_coroutine)
            flags |= CO_GENERATOR;
        if (!ste->ste_generator && ste->ste_coroutine)
            flags |= CO_COROUTINE;
        if (ste->ste_generator && ste->ste_coroutine)
            flags |= CO_ASYNC_GENERATOR;
        if (ste->ste_varargs)
            flags |= CO_VARARGS;
        if (ste->ste_varkeywords)
            flags |= CO_VARKEYWORDS;
    }

    /* (Only) inherit compilerflags in PyCF_MASK */
    flags |= (c->c_flags.cf_flags & PyCF_MASK);

    if ((IS_TOP_LEVEL_AWAIT(c)) &&
         ste->ste_coroutine &&
         !ste->ste_generator) {
        flags |= CO_COROUTINE;
    }

    return flags;
}

// Merge *obj* with constant cache.
// Unlike merge_consts_recursive(), this function doesn't work recursively.
int
_PyCompile_ConstCacheMergeOne(PyObject *const_cache, PyObject **obj)
{
    assert(PyDict_CheckExact(const_cache));
    PyObject *key = _PyCode_ConstantKey(*obj);
    if (key == NULL) {
        return ERROR;
    }

    // t is borrowed reference
    PyObject *t = PyDict_SetDefault(const_cache, key, key);
    Py_DECREF(key);
    if (t == NULL) {
        return ERROR;
    }
    if (t == key) {  // obj is new constant.
        return SUCCESS;
    }

    if (PyTuple_CheckExact(t)) {
        // t is still borrowed reference
        t = PyTuple_GET_ITEM(t, 1);
    }

    Py_SETREF(*obj, Py_NewRef(t));
    return SUCCESS;
}


static int *
build_cellfixedoffsets(_PyCompile_CodeUnitMetadata *umd)
{
    int nlocals = (int)PyDict_GET_SIZE(umd->u_varnames);
    int ncellvars = (int)PyDict_GET_SIZE(umd->u_cellvars);
    int nfreevars = (int)PyDict_GET_SIZE(umd->u_freevars);

    int noffsets = ncellvars + nfreevars;
    int *fixed = PyMem_New(int, noffsets);
    if (fixed == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    for (int i = 0; i < noffsets; i++) {
        fixed[i] = nlocals + i;
    }

    PyObject *varname, *cellindex;
    Py_ssize_t pos = 0;
    while (PyDict_Next(umd->u_cellvars, &pos, &varname, &cellindex)) {
        PyObject *varindex = PyDict_GetItem(umd->u_varnames, varname);
        if (varindex != NULL) {
            assert(PyLong_AS_LONG(cellindex) < INT_MAX);
            assert(PyLong_AS_LONG(varindex) < INT_MAX);
            int oldindex = (int)PyLong_AS_LONG(cellindex);
            int argoffset = (int)PyLong_AS_LONG(varindex);
            fixed[oldindex] = argoffset;
        }
    }

    return fixed;
}

static int
insert_prefix_instructions(_PyCompile_CodeUnitMetadata *umd, basicblock *entryblock,
                           int *fixed, int nfreevars, int code_flags)
{
    assert(umd->u_firstlineno > 0);

    /* Add the generator prefix instructions. */
    if (code_flags & (CO_GENERATOR | CO_COROUTINE | CO_ASYNC_GENERATOR)) {
        cfg_instr make_gen = {
            .i_opcode = RETURN_GENERATOR,
            .i_oparg = 0,
            .i_loc = LOCATION(umd->u_firstlineno, umd->u_firstlineno, -1, -1),
            .i_target = NULL,
        };
        RETURN_IF_ERROR(_PyBasicblock_InsertInstruction(entryblock, 0, &make_gen));
        cfg_instr pop_top = {
            .i_opcode = POP_TOP,
            .i_oparg = 0,
            .i_loc = NO_LOCATION,
            .i_target = NULL,
        };
        RETURN_IF_ERROR(_PyBasicblock_InsertInstruction(entryblock, 1, &pop_top));
    }

    /* Set up cells for any variable that escapes, to be put in a closure. */
    const int ncellvars = (int)PyDict_GET_SIZE(umd->u_cellvars);
    if (ncellvars) {
        // umd->u_cellvars has the cells out of order so we sort them
        // before adding the MAKE_CELL instructions.  Note that we
        // adjust for arg cells, which come first.
        const int nvars = ncellvars + (int)PyDict_GET_SIZE(umd->u_varnames);
        int *sorted = PyMem_RawCalloc(nvars, sizeof(int));
        if (sorted == NULL) {
            PyErr_NoMemory();
            return ERROR;
        }
        for (int i = 0; i < ncellvars; i++) {
            sorted[fixed[i]] = i + 1;
        }
        for (int i = 0, ncellsused = 0; ncellsused < ncellvars; i++) {
            int oldindex = sorted[i] - 1;
            if (oldindex == -1) {
                continue;
            }
            cfg_instr make_cell = {
                .i_opcode = MAKE_CELL,
                // This will get fixed in offset_derefs().
                .i_oparg = oldindex,
                .i_loc = NO_LOCATION,
                .i_target = NULL,
            };
            if (_PyBasicblock_InsertInstruction(entryblock, ncellsused, &make_cell) < 0) {
                PyMem_RawFree(sorted);
                return ERROR;
            }
            ncellsused += 1;
        }
        PyMem_RawFree(sorted);
    }

    if (nfreevars) {
        cfg_instr copy_frees = {
            .i_opcode = COPY_FREE_VARS,
            .i_oparg = nfreevars,
            .i_loc = NO_LOCATION,
            .i_target = NULL,
        };
        RETURN_IF_ERROR(_PyBasicblock_InsertInstruction(entryblock, 0, &copy_frees));
    }

    return SUCCESS;
}

static int
fix_cell_offsets(_PyCompile_CodeUnitMetadata *umd, basicblock *entryblock, int *fixedmap)
{
    int nlocals = (int)PyDict_GET_SIZE(umd->u_varnames);
    int ncellvars = (int)PyDict_GET_SIZE(umd->u_cellvars);
    int nfreevars = (int)PyDict_GET_SIZE(umd->u_freevars);
    int noffsets = ncellvars + nfreevars;

    // First deal with duplicates (arg cells).
    int numdropped = 0;
    for (int i = 0; i < noffsets ; i++) {
        if (fixedmap[i] == i + nlocals) {
            fixedmap[i] -= numdropped;
        }
        else {
            // It was a duplicate (cell/arg).
            numdropped += 1;
        }
    }

    // Then update offsets, either relative to locals or by cell2arg.
    for (basicblock *b = entryblock; b != NULL; b = b->b_next) {
        for (int i = 0; i < b->b_iused; i++) {
            cfg_instr *inst = &b->b_instr[i];
            // This is called before extended args are generated.
            assert(inst->i_opcode != EXTENDED_ARG);
            int oldoffset = inst->i_oparg;
            switch(inst->i_opcode) {
                case MAKE_CELL:
                case LOAD_CLOSURE:
                case LOAD_DEREF:
                case STORE_DEREF:
                case DELETE_DEREF:
                case LOAD_FROM_DICT_OR_DEREF:
                    assert(oldoffset >= 0);
                    assert(oldoffset < noffsets);
                    assert(fixedmap[oldoffset] >= 0);
                    inst->i_oparg = fixedmap[oldoffset];
            }
        }
    }

    return numdropped;
}


static int
prepare_localsplus(_PyCompile_CodeUnitMetadata *umd, cfg_builder *g, int code_flags)
{
    assert(PyDict_GET_SIZE(umd->u_varnames) < INT_MAX);
    assert(PyDict_GET_SIZE(umd->u_cellvars) < INT_MAX);
    assert(PyDict_GET_SIZE(umd->u_freevars) < INT_MAX);
    int nlocals = (int)PyDict_GET_SIZE(umd->u_varnames);
    int ncellvars = (int)PyDict_GET_SIZE(umd->u_cellvars);
    int nfreevars = (int)PyDict_GET_SIZE(umd->u_freevars);
    assert(INT_MAX - nlocals - ncellvars > 0);
    assert(INT_MAX - nlocals - ncellvars - nfreevars > 0);
    int nlocalsplus = nlocals + ncellvars + nfreevars;
    int* cellfixedoffsets = build_cellfixedoffsets(umd);
    if (cellfixedoffsets == NULL) {
        return ERROR;
    }


    // This must be called before fix_cell_offsets().
    if (insert_prefix_instructions(umd, g->g_entryblock, cellfixedoffsets, nfreevars, code_flags)) {
        PyMem_Free(cellfixedoffsets);
        return ERROR;
    }

    int numdropped = fix_cell_offsets(umd, g->g_entryblock, cellfixedoffsets);
    PyMem_Free(cellfixedoffsets);  // At this point we're done with it.
    cellfixedoffsets = NULL;
    if (numdropped < 0) {
        return ERROR;
    }

    nlocalsplus -= numdropped;
    return nlocalsplus;
}

static int
add_return_at_end(struct compiler *c, int addNone)
{
    /* Make sure every instruction stream that falls off the end returns None.
     * This also ensures that no jump target offsets are out of bounds.
     */
    if (addNone) {
        ADDOP_LOAD_CONST(c, NO_LOCATION, Py_None);
    }
    ADDOP(c, NO_LOCATION, RETURN_VALUE);
    return SUCCESS;
}

static int cfg_to_instr_sequence(cfg_builder *g, instr_sequence *seq);

static PyCodeObject *
optimize_and_assemble_code_unit(struct compiler_unit *u, PyObject *const_cache,
                   int code_flags, PyObject *filename)
{
    instr_sequence optimized_instrs;
    memset(&optimized_instrs, 0, sizeof(instr_sequence));

    PyCodeObject *co = NULL;
    PyObject *consts = consts_dict_keys_inorder(u->u_metadata.u_consts);
    if (consts == NULL) {
        goto error;
    }
    cfg_builder g;
    if (instr_sequence_to_cfg(&u->u_instr_sequence, &g) < 0) {
        goto error;
    }
    int nparams = (int)PyList_GET_SIZE(u->u_ste->ste_varnames);
    int nlocals = (int)PyDict_GET_SIZE(u->u_metadata.u_varnames);
    assert(u->u_metadata.u_firstlineno);
    if (_PyCfg_OptimizeCodeUnit(&g, consts, const_cache, code_flags, nlocals,
                                nparams, u->u_metadata.u_firstlineno) < 0) {
        goto error;
    }

    /** Assembly **/
    int nlocalsplus = prepare_localsplus(&u->u_metadata, &g, code_flags);
    if (nlocalsplus < 0) {
        goto error;
    }

    int maxdepth = _PyCfg_Stackdepth(g.g_entryblock, code_flags);
    if (maxdepth < 0) {
        goto error;
    }

    _PyCfg_ConvertPseudoOps(g.g_entryblock);

    /* Order of basic blocks must have been determined by now */

    if (_PyCfg_ResolveJumps(&g) < 0) {
        goto error;
    }

    /* Can't modify the bytecode after computing jump offsets. */

    if (cfg_to_instr_sequence(&g, &optimized_instrs) < 0) {
        goto error;
    }

    co = _PyAssemble_MakeCodeObject(&u->u_metadata, const_cache, consts,
                                    maxdepth, &optimized_instrs, nlocalsplus,
                                    code_flags, filename);

error:
    Py_XDECREF(consts);
    instr_sequence_fini(&optimized_instrs);
    _PyCfgBuilder_Fini(&g);
    return co;
}

static PyCodeObject *
optimize_and_assemble(struct compiler *c, int addNone)
{
    struct compiler_unit *u = c->u;
    PyObject *const_cache = c->c_const_cache;
    PyObject *filename = c->c_filename;

    int code_flags = compute_code_flags(c);
    if (code_flags < 0) {
        return NULL;
    }

    if (add_return_at_end(c, addNone) < 0) {
        return NULL;
    }

    return optimize_and_assemble_code_unit(u, const_cache, code_flags, filename);
}

static int
cfg_to_instr_sequence(cfg_builder *g, instr_sequence *seq)
{
    int lbl = 0;
    for (basicblock *b = g->g_entryblock; b != NULL; b = b->b_next) {
        b->b_label = (jump_target_label){lbl};
        lbl += b->b_iused;
    }
    for (basicblock *b = g->g_entryblock; b != NULL; b = b->b_next) {
        RETURN_IF_ERROR(instr_sequence_use_label(seq, b->b_label.id));
        for (int i = 0; i < b->b_iused; i++) {
            cfg_instr *instr = &b->b_instr[i];
            RETURN_IF_ERROR(
                instr_sequence_addop(seq, instr->i_opcode, instr->i_oparg, instr->i_loc));

            _PyCompile_ExceptHandlerInfo *hi = &seq->s_instrs[seq->s_used-1].i_except_handler_info;
            if (instr->i_except != NULL) {
                hi->h_offset = instr->i_except->b_offset;
                hi->h_startdepth = instr->i_except->b_startdepth;
                hi->h_preserve_lasti = instr->i_except->b_preserve_lasti;
            }
            else {
                hi->h_offset = -1;
            }
        }
    }
    return SUCCESS;
}


/* Access to compiler optimizations for unit tests.
 *
 * _PyCompile_CodeGen takes and AST, applies code-gen and
 * returns the unoptimized CFG as an instruction list.
 *
 * _PyCompile_OptimizeCfg takes an instruction list, constructs
 * a CFG, optimizes it and converts back to an instruction list.
 *
 * An instruction list is a PyList where each item is either
 * a tuple describing a single instruction:
 * (opcode, oparg, lineno, end_lineno, col, end_col), or
 * a jump target label marking the beginning of a basic block.
 */

static int
instructions_to_instr_sequence(PyObject *instructions, instr_sequence *seq)
{
    assert(PyList_Check(instructions));

    Py_ssize_t num_insts = PyList_GET_SIZE(instructions);
    bool *is_target = PyMem_Calloc(num_insts, sizeof(bool));
    if (is_target == NULL) {
        return ERROR;
    }
    for (Py_ssize_t i = 0; i < num_insts; i++) {
        PyObject *item = PyList_GET_ITEM(instructions, i);
        if (!PyTuple_Check(item) || PyTuple_GET_SIZE(item) != 6) {
            PyErr_SetString(PyExc_ValueError, "expected a 6-tuple");
            goto error;
        }
        int opcode = PyLong_AsLong(PyTuple_GET_ITEM(item, 0));
        if (PyErr_Occurred()) {
            goto error;
        }
        if (HAS_TARGET(opcode)) {
            int oparg = PyLong_AsLong(PyTuple_GET_ITEM(item, 1));
            if (PyErr_Occurred()) {
                goto error;
            }
            if (oparg < 0 || oparg >= num_insts) {
                PyErr_SetString(PyExc_ValueError, "label out of range");
                goto error;
            }
            is_target[oparg] = true;
        }
    }

    for (int i = 0; i < num_insts; i++) {
        if (is_target[i]) {
            if (instr_sequence_use_label(seq, i) < 0) {
                goto error;
            }
        }
        PyObject *item = PyList_GET_ITEM(instructions, i);
        if (!PyTuple_Check(item) || PyTuple_GET_SIZE(item) != 6) {
            PyErr_SetString(PyExc_ValueError, "expected a 6-tuple");
            goto error;
        }
        int opcode = PyLong_AsLong(PyTuple_GET_ITEM(item, 0));
        if (PyErr_Occurred()) {
            goto error;
        }
        int oparg;
        if (HAS_ARG(opcode)) {
            oparg = PyLong_AsLong(PyTuple_GET_ITEM(item, 1));
            if (PyErr_Occurred()) {
                goto error;
            }
        }
        else {
            oparg = 0;
        }
        location loc;
        loc.lineno = PyLong_AsLong(PyTuple_GET_ITEM(item, 2));
        if (PyErr_Occurred()) {
            goto error;
        }
        loc.end_lineno = PyLong_AsLong(PyTuple_GET_ITEM(item, 3));
        if (PyErr_Occurred()) {
            goto error;
        }
        loc.col_offset = PyLong_AsLong(PyTuple_GET_ITEM(item, 4));
        if (PyErr_Occurred()) {
            goto error;
        }
        loc.end_col_offset = PyLong_AsLong(PyTuple_GET_ITEM(item, 5));
        if (PyErr_Occurred()) {
            goto error;
        }
        if (instr_sequence_addop(seq, opcode, oparg, loc) < 0) {
            goto error;
        }
    }
    PyMem_Free(is_target);
    return SUCCESS;
error:
    PyMem_Free(is_target);
    return ERROR;
}

static int
instructions_to_cfg(PyObject *instructions, cfg_builder *g)
{
    instr_sequence seq;
    memset(&seq, 0, sizeof(instr_sequence));

    if (instructions_to_instr_sequence(instructions, &seq) < 0) {
        goto error;
    }
    if (instr_sequence_to_cfg(&seq, g) < 0) {
        goto error;
    }
    instr_sequence_fini(&seq);
    return SUCCESS;
error:
    instr_sequence_fini(&seq);
    return ERROR;
}

static PyObject *
instr_sequence_to_instructions(instr_sequence *seq)
{
    PyObject *instructions = PyList_New(0);
    if (instructions == NULL) {
        return NULL;
    }
    for (int i = 0; i < seq->s_used; i++) {
        instruction *instr = &seq->s_instrs[i];
        location loc = instr->i_loc;
        int arg = HAS_TARGET(instr->i_opcode) ?
                  seq->s_labelmap[instr->i_oparg] : instr->i_oparg;

        PyObject *inst_tuple = Py_BuildValue(
            "(iiiiii)", instr->i_opcode, arg,
            loc.lineno, loc.end_lineno,
            loc.col_offset, loc.end_col_offset);
        if (inst_tuple == NULL) {
            goto error;
        }

        int res = PyList_Append(instructions, inst_tuple);
        Py_DECREF(inst_tuple);
        if (res != 0) {
            goto error;
        }
    }
    return instructions;
error:
    Py_XDECREF(instructions);
    return NULL;
}

static PyObject *
cfg_to_instructions(cfg_builder *g)
{
    PyObject *instructions = PyList_New(0);
    if (instructions == NULL) {
        return NULL;
    }
    int lbl = 0;
    for (basicblock *b = g->g_entryblock; b != NULL; b = b->b_next) {
        b->b_label = (jump_target_label){lbl};
        lbl += b->b_iused;
    }
    for (basicblock *b = g->g_entryblock; b != NULL; b = b->b_next) {
        for (int i = 0; i < b->b_iused; i++) {
            cfg_instr *instr = &b->b_instr[i];
            location loc = instr->i_loc;
            int arg = HAS_TARGET(instr->i_opcode) ?
                      instr->i_target->b_label.id : instr->i_oparg;

            PyObject *inst_tuple = Py_BuildValue(
                "(iiiiii)", instr->i_opcode, arg,
                loc.lineno, loc.end_lineno,
                loc.col_offset, loc.end_col_offset);
            if (inst_tuple == NULL) {
                goto error;
            }

            if (PyList_Append(instructions, inst_tuple) != 0) {
                Py_DECREF(inst_tuple);
                goto error;
            }
            Py_DECREF(inst_tuple);
        }
    }

    return instructions;
error:
    Py_DECREF(instructions);
    return NULL;
}

PyObject *
_PyCompile_CodeGen(PyObject *ast, PyObject *filename, PyCompilerFlags *pflags,
                   int optimize, int compile_mode)
{
    PyObject *res = NULL;
    PyObject *metadata = NULL;

    if (!PyAST_Check(ast)) {
        PyErr_SetString(PyExc_TypeError, "expected an AST");
        return NULL;
    }

    PyArena *arena = _PyArena_New();
    if (arena == NULL) {
        return NULL;
    }

    mod_ty mod = PyAST_obj2mod(ast, arena, compile_mode);
    if (mod == NULL || !_PyAST_Validate(mod)) {
        _PyArena_Free(arena);
        return NULL;
    }

    struct compiler *c = new_compiler(mod, filename, pflags, optimize, arena);
    if (c == NULL) {
        _PyArena_Free(arena);
        return NULL;
    }

    if (compiler_codegen(c, mod) < 0) {
        goto finally;
    }

    _PyCompile_CodeUnitMetadata *umd = &c->u->u_metadata;
    metadata = PyDict_New();
    if (metadata == NULL) {
        goto finally;
    }
#define SET_MATADATA_ITEM(key, value) \
    if (value != NULL) { \
        if (PyDict_SetItemString(metadata, key, value) < 0) goto finally; \
    }

    SET_MATADATA_ITEM("name", umd->u_name);
    SET_MATADATA_ITEM("qualname", umd->u_qualname);
    SET_MATADATA_ITEM("consts", umd->u_consts);
    SET_MATADATA_ITEM("names", umd->u_names);
    SET_MATADATA_ITEM("varnames", umd->u_varnames);
    SET_MATADATA_ITEM("cellvars", umd->u_cellvars);
    SET_MATADATA_ITEM("freevars", umd->u_freevars);
#undef SET_MATADATA_ITEM

#define SET_MATADATA_INT(key, value) do { \
        PyObject *v = PyLong_FromLong((long)value); \
        if (v == NULL) goto finally; \
        int res = PyDict_SetItemString(metadata, key, v); \
        Py_XDECREF(v); \
        if (res < 0) goto finally; \
    } while (0);

    SET_MATADATA_INT("argcount", umd->u_argcount);
    SET_MATADATA_INT("posonlyargcount", umd->u_posonlyargcount);
    SET_MATADATA_INT("kwonlyargcount", umd->u_kwonlyargcount);
#undef SET_MATADATA_INT

    int addNone = mod->kind != Expression_kind;
    if (add_return_at_end(c, addNone) < 0) {
        goto finally;
    }

    PyObject *insts = instr_sequence_to_instructions(INSTR_SEQUENCE(c));
    if (insts == NULL) {
        goto finally;
    }
    res = PyTuple_Pack(2, insts, metadata);
    Py_DECREF(insts);

finally:
    Py_XDECREF(metadata);
    compiler_exit_scope(c);
    compiler_free(c);
    _PyArena_Free(arena);
    return res;
}

PyObject *
_PyCompile_OptimizeCfg(PyObject *instructions, PyObject *consts, int nlocals)
{
    PyObject *res = NULL;
    PyObject *const_cache = PyDict_New();
    if (const_cache == NULL) {
        return NULL;
    }

    cfg_builder g;
    if (instructions_to_cfg(instructions, &g) < 0) {
        goto error;
    }
    int code_flags = 0, nparams = 0, firstlineno = 1;
    if (_PyCfg_OptimizeCodeUnit(&g, consts, const_cache, code_flags, nlocals,
                                nparams, firstlineno) < 0) {
        goto error;
    }
    res = cfg_to_instructions(&g);
error:
    Py_DECREF(const_cache);
    _PyCfgBuilder_Fini(&g);
    return res;
}

int _PyCfg_JumpLabelsToTargets(basicblock *entryblock);

PyCodeObject *
_PyCompile_Assemble(_PyCompile_CodeUnitMetadata *umd, PyObject *filename,
                    PyObject *instructions)
{
    PyCodeObject *co = NULL;
    instr_sequence optimized_instrs;
    memset(&optimized_instrs, 0, sizeof(instr_sequence));

    PyObject *const_cache = PyDict_New();
    if (const_cache == NULL) {
        return NULL;
    }

    cfg_builder g;
    if (instructions_to_cfg(instructions, &g) < 0) {
        goto error;
    }

    if (_PyCfg_JumpLabelsToTargets(g.g_entryblock) < 0) {
        goto error;
    }

    int code_flags = 0;
    int nlocalsplus = prepare_localsplus(umd, &g, code_flags);
    if (nlocalsplus < 0) {
        goto error;
    }

    int maxdepth = _PyCfg_Stackdepth(g.g_entryblock, code_flags);
    if (maxdepth < 0) {
        goto error;
    }

    _PyCfg_ConvertPseudoOps(g.g_entryblock);

    /* Order of basic blocks must have been determined by now */

    if (_PyCfg_ResolveJumps(&g) < 0) {
        goto error;
    }

    /* Can't modify the bytecode after computing jump offsets. */

    if (cfg_to_instr_sequence(&g, &optimized_instrs) < 0) {
        goto error;
    }

    PyObject *consts = consts_dict_keys_inorder(umd->u_consts);
    if (consts == NULL) {
        goto error;
    }
    co = _PyAssemble_MakeCodeObject(umd, const_cache,
                                    consts, maxdepth, &optimized_instrs,
                                    nlocalsplus, code_flags, filename);
    Py_DECREF(consts);

error:
    Py_DECREF(const_cache);
    _PyCfgBuilder_Fini(&g);
    instr_sequence_fini(&optimized_instrs);
    return co;
}


/* Retained for API compatibility.
 * Optimization is now done in _PyCfg_OptimizeCodeUnit */

PyObject *
PyCode_Optimize(PyObject *code, PyObject* Py_UNUSED(consts),
                PyObject *Py_UNUSED(names), PyObject *Py_UNUSED(lnotab_obj))
{
    return Py_NewRef(code);
}
