/*
 * This file compiles an abstract syntax tree (AST) into Python bytecode.
 *
 * The primary entry point is PyAST_Compile(), which returns a
 * PyCodeObject.  The compiler makes several passes to build the code
 * object:
 *   1. Checks for future statements.  See future.c
 *   2. Builds a symbol table.  See symtable.c.
 *   3. Generate code for basic blocks.  See compiler_mod() in this file.
 *   4. Assemble the basic blocks into final code.  See assemble() in
 *      this file.
 *   5. Optimize the byte code (peephole optimizations).  See peephole.c
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

#include "Python.h"

#include "Python-ast.h"
#include "node.h"
#include "ast.h"
#include "code.h"
#include "symtable.h"
#include "opcode.h"
#include "wordcode_helpers.h"

#define DEFAULT_BLOCK_SIZE 16
#define DEFAULT_BLOCKS 8
#define DEFAULT_CODE_SIZE 128
#define DEFAULT_LNOTAB_SIZE 16

#define COMP_GENEXP   0
#define COMP_LISTCOMP 1
#define COMP_SETCOMP  2
#define COMP_DICTCOMP 3

struct instr {
    unsigned i_jabs : 1;
    unsigned i_jrel : 1;
    unsigned char i_opcode;
    int i_oparg;
    struct basicblock_ *i_target; /* target block (if jump instruction) */
    int i_lineno;
};

typedef struct basicblock_ {
    /* Each basicblock in a compilation unit is linked via b_list in the
       reverse order that the block are allocated.  b_list points to the next
       block, not to be confused with b_next, which is next by control flow. */
    struct basicblock_ *b_list;
    /* number of instructions used */
    int b_iused;
    /* length of instruction array (b_instr) */
    int b_ialloc;
    /* pointer to an array of instructions, initially NULL */
    struct instr *b_instr;
    /* If b_next is non-NULL, it is a pointer to the next
       block reached by normal control flow. */
    struct basicblock_ *b_next;
    /* b_seen is used to perform a DFS of basicblocks. */
    unsigned b_seen : 1;
    /* b_return is true if a RETURN_VALUE opcode is inserted. */
    unsigned b_return : 1;
    /* depth of stack upon entry of block, computed by stackdepth() */
    int b_startdepth;
    /* instruction offset for block, computed by assemble_jump_offsets() */
    int b_offset;
} basicblock;

/* fblockinfo tracks the current frame block.

A frame block is used to handle loops, try/except, and try/finally.
It's called a frame block to distinguish it from a basic block in the
compiler IR.
*/

enum fblocktype { LOOP, EXCEPT, FINALLY_TRY, FINALLY_END };

struct fblockinfo {
    enum fblocktype fb_type;
    basicblock *fb_block;
};

enum {
    COMPILER_SCOPE_MODULE,
    COMPILER_SCOPE_CLASS,
    COMPILER_SCOPE_FUNCTION,
    COMPILER_SCOPE_ASYNC_FUNCTION,
    COMPILER_SCOPE_LAMBDA,
    COMPILER_SCOPE_COMPREHENSION,
};

/* The following items change on entry and exit of code blocks.
   They must be saved and restored when returning to a block.
*/
struct compiler_unit {
    PySTEntryObject *u_ste;

    PyObject *u_name;
    PyObject *u_qualname;  /* dot-separated qualified name (lazy) */
    int u_scope_type;

    /* The following fields are dicts that map objects to
       the index of them in co_XXX.      The index is used as
       the argument for opcodes that refer to those collections.
    */
    PyObject *u_consts;    /* all constants */
    PyObject *u_names;     /* all names */
    PyObject *u_varnames;  /* local variables */
    PyObject *u_cellvars;  /* cell variables */
    PyObject *u_freevars;  /* free variables */

    PyObject *u_private;        /* for private name mangling */

    Py_ssize_t u_argcount;        /* number of arguments for block */
    Py_ssize_t u_kwonlyargcount; /* number of keyword only arguments for block */
    /* Pointer to the most recently allocated block.  By following b_list
       members, you can reach all early allocated blocks. */
    basicblock *u_blocks;
    basicblock *u_curblock; /* pointer to current block */

    int u_nfblocks;
    struct fblockinfo u_fblock[CO_MAXBLOCKS];

    int u_firstlineno; /* the first lineno of the block */
    int u_lineno;          /* the lineno for the current stmt */
    int u_col_offset;      /* the offset of the current stmt */
    int u_lineno_set;  /* boolean to indicate whether instr
                          has been generated with current lineno */
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
    PyFutureFeatures *c_future; /* pointer to module's __future__ */
    PyCompilerFlags *c_flags;

    int c_optimize;              /* optimization level */
    int c_interactive;           /* true if in interactive mode */
    int c_nestlevel;

    struct compiler_unit *u; /* compiler state for current block */
    PyObject *c_stack;           /* Python list holding compiler_unit ptrs */
    PyArena *c_arena;            /* pointer to memory allocation arena */
};

static int compiler_enter_scope(struct compiler *, identifier, int, void *, int);
static void compiler_free(struct compiler *);
static basicblock *compiler_new_block(struct compiler *);
static int compiler_next_instr(struct compiler *, basicblock *);
static int compiler_addop(struct compiler *, int);
static int compiler_addop_o(struct compiler *, int, PyObject *, PyObject *);
static int compiler_addop_i(struct compiler *, int, Py_ssize_t);
static int compiler_addop_j(struct compiler *, int, basicblock *, int);
static int compiler_error(struct compiler *, const char *);
static int compiler_nameop(struct compiler *, identifier, expr_context_ty);

static PyCodeObject *compiler_mod(struct compiler *, mod_ty);
static int compiler_visit_stmt(struct compiler *, stmt_ty);
static int compiler_visit_keyword(struct compiler *, keyword_ty);
static int compiler_visit_expr(struct compiler *, expr_ty);
static int compiler_augassign(struct compiler *, stmt_ty);
static int compiler_annassign(struct compiler *, stmt_ty);
static int compiler_visit_slice(struct compiler *, slice_ty,
                                expr_context_ty);

static int compiler_push_fblock(struct compiler *, enum fblocktype,
                                basicblock *);
static void compiler_pop_fblock(struct compiler *, enum fblocktype,
                                basicblock *);
/* Returns true if there is a loop on the fblock stack. */
static int compiler_in_loop(struct compiler *);

static int inplace_binop(struct compiler *, operator_ty);
static int expr_constant(struct compiler *, expr_ty);

static int compiler_with(struct compiler *, stmt_ty, int);
static int compiler_async_with(struct compiler *, stmt_ty, int);
static int compiler_async_for(struct compiler *, stmt_ty);
static int compiler_call_helper(struct compiler *c, int n,
                                asdl_seq *args,
                                asdl_seq *keywords);
static int compiler_try_except(struct compiler *, stmt_ty);
static int compiler_set_qualname(struct compiler *);

static int compiler_sync_comprehension_generator(
                                      struct compiler *c,
                                      asdl_seq *generators, int gen_index,
                                      expr_ty elt, expr_ty val, int type);

static int compiler_async_comprehension_generator(
                                      struct compiler *c,
                                      asdl_seq *generators, int gen_index,
                                      expr_ty elt, expr_ty val, int type);

static PyCodeObject *assemble(struct compiler *, int addNone);
static PyObject *__doc__;

#define CAPSULE_NAME "compile.c compiler unit"

PyObject *
_Py_Mangle(PyObject *privateobj, PyObject *ident)
{
    /* Name mangling: __private becomes _classname__private.
       This is independent from how the name is used. */
    PyObject *result;
    size_t nlen, plen, ipriv;
    Py_UCS4 maxchar;
    if (privateobj == NULL || !PyUnicode_Check(privateobj) ||
        PyUnicode_READ_CHAR(ident, 0) != '_' ||
        PyUnicode_READ_CHAR(ident, 1) != '_') {
        Py_INCREF(ident);
        return ident;
    }
    nlen = PyUnicode_GET_LENGTH(ident);
    plen = PyUnicode_GET_LENGTH(privateobj);
    /* Don't mangle __id__ or names with dots.

       The only time a name with a dot can occur is when
       we are compiling an import statement that has a
       package name.

       TODO(jhylton): Decide whether we want to support
       mangling of the module name, e.g. __M.X.
    */
    if ((PyUnicode_READ_CHAR(ident, nlen-1) == '_' &&
         PyUnicode_READ_CHAR(ident, nlen-2) == '_') ||
        PyUnicode_FindChar(ident, '.', 0, nlen, 1) != -1) {
        Py_INCREF(ident);
        return ident; /* Don't mangle __whatever__ */
    }
    /* Strip leading underscores from class name */
    ipriv = 0;
    while (PyUnicode_READ_CHAR(privateobj, ipriv) == '_')
        ipriv++;
    if (ipriv == plen) {
        Py_INCREF(ident);
        return ident; /* Don't mangle if class is just underscores */
    }
    plen -= ipriv;

    if (plen + nlen >= PY_SSIZE_T_MAX - 1) {
        PyErr_SetString(PyExc_OverflowError,
                        "private identifier too large to be mangled");
        return NULL;
    }

    maxchar = PyUnicode_MAX_CHAR_VALUE(ident);
    if (PyUnicode_MAX_CHAR_VALUE(privateobj) > maxchar)
        maxchar = PyUnicode_MAX_CHAR_VALUE(privateobj);

    result = PyUnicode_New(1 + nlen + plen, maxchar);
    if (!result)
        return 0;
    /* ident = "_" + priv[ipriv:] + ident # i.e. 1+plen+nlen bytes */
    PyUnicode_WRITE(PyUnicode_KIND(result), PyUnicode_DATA(result), 0, '_');
    if (PyUnicode_CopyCharacters(result, 1, privateobj, ipriv, plen) < 0) {
        Py_DECREF(result);
        return NULL;
    }
    if (PyUnicode_CopyCharacters(result, plen+1, ident, 0, nlen) < 0) {
        Py_DECREF(result);
        return NULL;
    }
    assert(_PyUnicode_CheckConsistency(result, 1));
    return result;
}

static int
compiler_init(struct compiler *c)
{
    memset(c, 0, sizeof(struct compiler));

    c->c_stack = PyList_New(0);
    if (!c->c_stack)
        return 0;

    return 1;
}

PyCodeObject *
PyAST_CompileObject(mod_ty mod, PyObject *filename, PyCompilerFlags *flags,
                   int optimize, PyArena *arena)
{
    struct compiler c;
    PyCodeObject *co = NULL;
    PyCompilerFlags local_flags;
    int merged;

    if (!__doc__) {
        __doc__ = PyUnicode_InternFromString("__doc__");
        if (!__doc__)
            return NULL;
    }

    if (!compiler_init(&c))
        return NULL;
    Py_INCREF(filename);
    c.c_filename = filename;
    c.c_arena = arena;
    c.c_future = PyFuture_FromASTObject(mod, filename);
    if (c.c_future == NULL)
        goto finally;
    if (!flags) {
        local_flags.cf_flags = 0;
        flags = &local_flags;
    }
    merged = c.c_future->ff_features | flags->cf_flags;
    c.c_future->ff_features = merged;
    flags->cf_flags = merged;
    c.c_flags = flags;
    c.c_optimize = (optimize == -1) ? Py_OptimizeFlag : optimize;
    c.c_nestlevel = 0;

    c.c_st = PySymtable_BuildObject(mod, filename, c.c_future);
    if (c.c_st == NULL) {
        if (!PyErr_Occurred())
            PyErr_SetString(PyExc_SystemError, "no symtable");
        goto finally;
    }

    co = compiler_mod(&c, mod);

 finally:
    compiler_free(&c);
    assert(co || PyErr_Occurred());
    return co;
}

PyCodeObject *
PyAST_CompileEx(mod_ty mod, const char *filename_str, PyCompilerFlags *flags,
                int optimize, PyArena *arena)
{
    PyObject *filename;
    PyCodeObject *co;
    filename = PyUnicode_DecodeFSDefault(filename_str);
    if (filename == NULL)
        return NULL;
    co = PyAST_CompileObject(mod, filename, flags, optimize, arena);
    Py_DECREF(filename);
    return co;

}

PyCodeObject *
PyNode_Compile(struct _node *n, const char *filename)
{
    PyCodeObject *co = NULL;
    mod_ty mod;
    PyArena *arena = PyArena_New();
    if (!arena)
        return NULL;
    mod = PyAST_FromNode(n, NULL, filename, arena);
    if (mod)
        co = PyAST_Compile(mod, filename, NULL, arena);
    PyArena_Free(arena);
    return co;
}

static void
compiler_free(struct compiler *c)
{
    if (c->c_st)
        PySymtable_Free(c->c_st);
    if (c->c_future)
        PyObject_Free(c->c_future);
    Py_XDECREF(c->c_filename);
    Py_DECREF(c->c_stack);
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
        k = _PyCode_ConstantKey(k);
        if (k == NULL || PyDict_SetItem(dict, k, v) < 0) {
            Py_XDECREF(k);
            Py_DECREF(v);
            Py_DECREF(dict);
            return NULL;
        }
        Py_DECREF(k);
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
        v = PyDict_GetItem(src, k);
        assert(PyLong_Check(v));
        vi = PyLong_AS_LONG(v);
        scope = (vi >> SCOPE_OFFSET) & SCOPE_MASK;

        if (scope == scope_type || vi & flag) {
            PyObject *tuple, *item = PyLong_FromSsize_t(i);
            if (item == NULL) {
                Py_DECREF(sorted_keys);
                Py_DECREF(dest);
                return NULL;
            }
            i++;
            tuple = _PyCode_ConstantKey(k);
            if (!tuple || PyDict_SetItem(dest, tuple, item) < 0) {
                Py_DECREF(sorted_keys);
                Py_DECREF(item);
                Py_DECREF(dest);
                Py_XDECREF(tuple);
                return NULL;
            }
            Py_DECREF(item);
            Py_DECREF(tuple);
        }
    }
    Py_DECREF(sorted_keys);
    return dest;
}

static void
compiler_unit_check(struct compiler_unit *u)
{
    basicblock *block;
    for (block = u->u_blocks; block != NULL; block = block->b_list) {
        assert((uintptr_t)block != 0xcbcbcbcbU);
        assert((uintptr_t)block != 0xfbfbfbfbU);
        assert((uintptr_t)block != 0xdbdbdbdbU);
        if (block->b_instr != NULL) {
            assert(block->b_ialloc > 0);
            assert(block->b_iused > 0);
            assert(block->b_ialloc >= block->b_iused);
        }
        else {
            assert (block->b_iused == 0);
            assert (block->b_ialloc == 0);
        }
    }
}

static void
compiler_unit_free(struct compiler_unit *u)
{
    basicblock *b, *next;

    compiler_unit_check(u);
    b = u->u_blocks;
    while (b != NULL) {
        if (b->b_instr)
            PyObject_Free((void *)b->b_instr);
        next = b->b_list;
        PyObject_Free((void *)b);
        b = next;
    }
    Py_CLEAR(u->u_ste);
    Py_CLEAR(u->u_name);
    Py_CLEAR(u->u_qualname);
    Py_CLEAR(u->u_consts);
    Py_CLEAR(u->u_names);
    Py_CLEAR(u->u_varnames);
    Py_CLEAR(u->u_freevars);
    Py_CLEAR(u->u_cellvars);
    Py_CLEAR(u->u_private);
    PyObject_Free(u);
}

static int
compiler_enter_scope(struct compiler *c, identifier name,
                     int scope_type, void *key, int lineno)
{
    struct compiler_unit *u;
    basicblock *block;

    u = (struct compiler_unit *)PyObject_Malloc(sizeof(
                                            struct compiler_unit));
    if (!u) {
        PyErr_NoMemory();
        return 0;
    }
    memset(u, 0, sizeof(struct compiler_unit));
    u->u_scope_type = scope_type;
    u->u_argcount = 0;
    u->u_kwonlyargcount = 0;
    u->u_ste = PySymtable_Lookup(c->c_st, key);
    if (!u->u_ste) {
        compiler_unit_free(u);
        return 0;
    }
    Py_INCREF(name);
    u->u_name = name;
    u->u_varnames = list2dict(u->u_ste->ste_varnames);
    u->u_cellvars = dictbytype(u->u_ste->ste_symbols, CELL, 0, 0);
    if (!u->u_varnames || !u->u_cellvars) {
        compiler_unit_free(u);
        return 0;
    }
    if (u->u_ste->ste_needs_class_closure) {
        /* Cook up an implicit __class__ cell. */
        _Py_IDENTIFIER(__class__);
        PyObject *tuple, *name, *zero;
        int res;
        assert(u->u_scope_type == COMPILER_SCOPE_CLASS);
        assert(PyDict_Size(u->u_cellvars) == 0);
        name = _PyUnicode_FromId(&PyId___class__);
        if (!name) {
            compiler_unit_free(u);
            return 0;
        }
        tuple = _PyCode_ConstantKey(name);
        if (!tuple) {
            compiler_unit_free(u);
            return 0;
        }
        zero = PyLong_FromLong(0);
        if (!zero) {
            Py_DECREF(tuple);
            compiler_unit_free(u);
            return 0;
        }
        res = PyDict_SetItem(u->u_cellvars, tuple, zero);
        Py_DECREF(tuple);
        Py_DECREF(zero);
        if (res < 0) {
            compiler_unit_free(u);
            return 0;
        }
    }

    u->u_freevars = dictbytype(u->u_ste->ste_symbols, FREE, DEF_FREE_CLASS,
                               PyDict_Size(u->u_cellvars));
    if (!u->u_freevars) {
        compiler_unit_free(u);
        return 0;
    }

    u->u_blocks = NULL;
    u->u_nfblocks = 0;
    u->u_firstlineno = lineno;
    u->u_lineno = 0;
    u->u_col_offset = 0;
    u->u_lineno_set = 0;
    u->u_consts = PyDict_New();
    if (!u->u_consts) {
        compiler_unit_free(u);
        return 0;
    }
    u->u_names = PyDict_New();
    if (!u->u_names) {
        compiler_unit_free(u);
        return 0;
    }

    u->u_private = NULL;

    /* Push the old compiler_unit on the stack. */
    if (c->u) {
        PyObject *capsule = PyCapsule_New(c->u, CAPSULE_NAME, NULL);
        if (!capsule || PyList_Append(c->c_stack, capsule) < 0) {
            Py_XDECREF(capsule);
            compiler_unit_free(u);
            return 0;
        }
        Py_DECREF(capsule);
        u->u_private = c->u->u_private;
        Py_XINCREF(u->u_private);
    }
    c->u = u;

    c->c_nestlevel++;

    block = compiler_new_block(c);
    if (block == NULL)
        return 0;
    c->u->u_curblock = block;

    if (u->u_scope_type != COMPILER_SCOPE_MODULE) {
        if (!compiler_set_qualname(c))
            return 0;
    }

    return 1;
}

static void
compiler_exit_scope(struct compiler *c)
{
    Py_ssize_t n;
    PyObject *capsule;

    c->c_nestlevel--;
    compiler_unit_free(c->u);
    /* Restore c->u to the parent unit. */
    n = PyList_GET_SIZE(c->c_stack) - 1;
    if (n >= 0) {
        capsule = PyList_GET_ITEM(c->c_stack, n);
        c->u = (struct compiler_unit *)PyCapsule_GetPointer(capsule, CAPSULE_NAME);
        assert(c->u);
        /* we are deleting from a list so this really shouldn't fail */
        if (PySequence_DelItem(c->c_stack, n) < 0)
            Py_FatalError("compiler_exit_scope()");
        compiler_unit_check(c->u);
    }
    else
        c->u = NULL;

}

static int
compiler_set_qualname(struct compiler *c)
{
    _Py_static_string(dot, ".");
    _Py_static_string(dot_locals, ".<locals>");
    Py_ssize_t stack_size;
    struct compiler_unit *u = c->u;
    PyObject *name, *base, *dot_str, *dot_locals_str;

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

        if (u->u_scope_type == COMPILER_SCOPE_FUNCTION
            || u->u_scope_type == COMPILER_SCOPE_ASYNC_FUNCTION
            || u->u_scope_type == COMPILER_SCOPE_CLASS) {
            assert(u->u_name);
            mangled = _Py_Mangle(parent->u_private, u->u_name);
            if (!mangled)
                return 0;
            scope = PyST_GetScope(parent->u_ste, mangled);
            Py_DECREF(mangled);
            assert(scope != GLOBAL_IMPLICIT);
            if (scope == GLOBAL_EXPLICIT)
                force_global = 1;
        }

        if (!force_global) {
            if (parent->u_scope_type == COMPILER_SCOPE_FUNCTION
                || parent->u_scope_type == COMPILER_SCOPE_ASYNC_FUNCTION
                || parent->u_scope_type == COMPILER_SCOPE_LAMBDA) {
                dot_locals_str = _PyUnicode_FromId(&dot_locals);
                if (dot_locals_str == NULL)
                    return 0;
                base = PyUnicode_Concat(parent->u_qualname, dot_locals_str);
                if (base == NULL)
                    return 0;
            }
            else {
                Py_INCREF(parent->u_qualname);
                base = parent->u_qualname;
            }
        }
    }

    if (base != NULL) {
        dot_str = _PyUnicode_FromId(&dot);
        if (dot_str == NULL) {
            Py_DECREF(base);
            return 0;
        }
        name = PyUnicode_Concat(base, dot_str);
        Py_DECREF(base);
        if (name == NULL)
            return 0;
        PyUnicode_Append(&name, u->u_name);
        if (name == NULL)
            return 0;
    }
    else {
        Py_INCREF(u->u_name);
        name = u->u_name;
    }
    u->u_qualname = name;

    return 1;
}


/* Allocate a new block and return a pointer to it.
   Returns NULL on error.
*/

static basicblock *
compiler_new_block(struct compiler *c)
{
    basicblock *b;
    struct compiler_unit *u;

    u = c->u;
    b = (basicblock *)PyObject_Malloc(sizeof(basicblock));
    if (b == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    memset((void *)b, 0, sizeof(basicblock));
    /* Extend the singly linked list of blocks with new block. */
    b->b_list = u->u_blocks;
    u->u_blocks = b;
    return b;
}

static basicblock *
compiler_next_block(struct compiler *c)
{
    basicblock *block = compiler_new_block(c);
    if (block == NULL)
        return NULL;
    c->u->u_curblock->b_next = block;
    c->u->u_curblock = block;
    return block;
}

static basicblock *
compiler_use_next_block(struct compiler *c, basicblock *block)
{
    assert(block != NULL);
    c->u->u_curblock->b_next = block;
    c->u->u_curblock = block;
    return block;
}

/* Returns the offset of the next instruction in the current block's
   b_instr array.  Resizes the b_instr as necessary.
   Returns -1 on failure.
*/

static int
compiler_next_instr(struct compiler *c, basicblock *b)
{
    assert(b != NULL);
    if (b->b_instr == NULL) {
        b->b_instr = (struct instr *)PyObject_Malloc(
                         sizeof(struct instr) * DEFAULT_BLOCK_SIZE);
        if (b->b_instr == NULL) {
            PyErr_NoMemory();
            return -1;
        }
        b->b_ialloc = DEFAULT_BLOCK_SIZE;
        memset((char *)b->b_instr, 0,
               sizeof(struct instr) * DEFAULT_BLOCK_SIZE);
    }
    else if (b->b_iused == b->b_ialloc) {
        struct instr *tmp;
        size_t oldsize, newsize;
        oldsize = b->b_ialloc * sizeof(struct instr);
        newsize = oldsize << 1;

        if (oldsize > (SIZE_MAX >> 1)) {
            PyErr_NoMemory();
            return -1;
        }

        if (newsize == 0) {
            PyErr_NoMemory();
            return -1;
        }
        b->b_ialloc <<= 1;
        tmp = (struct instr *)PyObject_Realloc(
                                        (void *)b->b_instr, newsize);
        if (tmp == NULL) {
            PyErr_NoMemory();
            return -1;
        }
        b->b_instr = tmp;
        memset((char *)b->b_instr + oldsize, 0, newsize - oldsize);
    }
    return b->b_iused++;
}

/* Set the i_lineno member of the instruction at offset off if the
   line number for the current expression/statement has not
   already been set.  If it has been set, the call has no effect.

   The line number is reset in the following cases:
   - when entering a new scope
   - on each statement
   - on each expression that start a new line
   - before the "except" clause
   - before the "for" and "while" expressions
*/

static void
compiler_set_lineno(struct compiler *c, int off)
{
    basicblock *b;
    if (c->u->u_lineno_set)
        return;
    c->u->u_lineno_set = 1;
    b = c->u->u_curblock;
    b->b_instr[off].i_lineno = c->u->u_lineno;
}

int
PyCompile_OpcodeStackEffect(int opcode, int oparg)
{
    switch (opcode) {
        case POP_TOP:
            return -1;
        case ROT_TWO:
        case ROT_THREE:
            return 0;
        case DUP_TOP:
            return 1;
        case DUP_TOP_TWO:
            return 2;

        case UNARY_POSITIVE:
        case UNARY_NEGATIVE:
        case UNARY_NOT:
        case UNARY_INVERT:
            return 0;

        case SET_ADD:
        case LIST_APPEND:
            return -1;
        case MAP_ADD:
            return -2;

        case BINARY_POWER:
        case BINARY_MULTIPLY:
        case BINARY_MATRIX_MULTIPLY:
        case BINARY_MODULO:
        case BINARY_ADD:
        case BINARY_SUBTRACT:
        case BINARY_SUBSCR:
        case BINARY_FLOOR_DIVIDE:
        case BINARY_TRUE_DIVIDE:
            return -1;
        case INPLACE_FLOOR_DIVIDE:
        case INPLACE_TRUE_DIVIDE:
            return -1;

        case INPLACE_ADD:
        case INPLACE_SUBTRACT:
        case INPLACE_MULTIPLY:
        case INPLACE_MATRIX_MULTIPLY:
        case INPLACE_MODULO:
            return -1;
        case STORE_SUBSCR:
            return -3;
        case DELETE_SUBSCR:
            return -2;

        case BINARY_LSHIFT:
        case BINARY_RSHIFT:
        case BINARY_AND:
        case BINARY_XOR:
        case BINARY_OR:
            return -1;
        case INPLACE_POWER:
            return -1;
        case GET_ITER:
            return 0;

        case PRINT_EXPR:
            return -1;
        case LOAD_BUILD_CLASS:
            return 1;
        case INPLACE_LSHIFT:
        case INPLACE_RSHIFT:
        case INPLACE_AND:
        case INPLACE_XOR:
        case INPLACE_OR:
            return -1;
        case BREAK_LOOP:
            return 0;
        case SETUP_WITH:
            return 7;
        case WITH_CLEANUP_START:
            return 1;
        case WITH_CLEANUP_FINISH:
            return -1; /* XXX Sometimes more */
        case RETURN_VALUE:
            return -1;
        case IMPORT_STAR:
            return -1;
        case SETUP_ANNOTATIONS:
            return 0;
        case YIELD_VALUE:
            return 0;
        case YIELD_FROM:
            return -1;
        case POP_BLOCK:
            return 0;
        case POP_EXCEPT:
            return 0;  /* -3 except if bad bytecode */
        case END_FINALLY:
            return -1; /* or -2 or -3 if exception occurred */

        case STORE_NAME:
            return -1;
        case DELETE_NAME:
            return 0;
        case UNPACK_SEQUENCE:
            return oparg-1;
        case UNPACK_EX:
            return (oparg&0xFF) + (oparg>>8);
        case FOR_ITER:
            return 1; /* or -1, at end of iterator */

        case STORE_ATTR:
            return -2;
        case DELETE_ATTR:
            return -1;
        case STORE_GLOBAL:
            return -1;
        case DELETE_GLOBAL:
            return 0;
        case LOAD_CONST:
            return 1;
        case LOAD_NAME:
            return 1;
        case BUILD_TUPLE:
        case BUILD_LIST:
        case BUILD_SET:
        case BUILD_STRING:
            return 1-oparg;
        case BUILD_LIST_UNPACK:
        case BUILD_TUPLE_UNPACK:
        case BUILD_TUPLE_UNPACK_WITH_CALL:
        case BUILD_SET_UNPACK:
        case BUILD_MAP_UNPACK:
        case BUILD_MAP_UNPACK_WITH_CALL:
            return 1 - oparg;
        case BUILD_MAP:
            return 1 - 2*oparg;
        case BUILD_CONST_KEY_MAP:
            return -oparg;
        case LOAD_ATTR:
            return 0;
        case COMPARE_OP:
            return -1;
        case IMPORT_NAME:
            return -1;
        case IMPORT_FROM:
            return 1;

        case JUMP_FORWARD:
        case JUMP_IF_TRUE_OR_POP:  /* -1 if jump not taken */
        case JUMP_IF_FALSE_OR_POP:  /*  "" */
        case JUMP_ABSOLUTE:
            return 0;

        case POP_JUMP_IF_FALSE:
        case POP_JUMP_IF_TRUE:
            return -1;

        case LOAD_GLOBAL:
            return 1;

        case CONTINUE_LOOP:
            return 0;
        case SETUP_LOOP:
            return 0;
        case SETUP_EXCEPT:
        case SETUP_FINALLY:
            return 6; /* can push 3 values for the new exception
                + 3 others for the previous exception state */

        case LOAD_FAST:
            return 1;
        case STORE_FAST:
            return -1;
        case DELETE_FAST:
            return 0;
        case STORE_ANNOTATION:
            return -1;

        case RAISE_VARARGS:
            return -oparg;
        case CALL_FUNCTION:
            return -oparg;
        case CALL_FUNCTION_KW:
            return -oparg-1;
        case CALL_FUNCTION_EX:
            return -1 - ((oparg & 0x01) != 0);
        case MAKE_FUNCTION:
            return -1 - ((oparg & 0x01) != 0) - ((oparg & 0x02) != 0) -
                ((oparg & 0x04) != 0) - ((oparg & 0x08) != 0);
        case BUILD_SLICE:
            if (oparg == 3)
                return -2;
            else
                return -1;

        case LOAD_CLOSURE:
            return 1;
        case LOAD_DEREF:
        case LOAD_CLASSDEREF:
            return 1;
        case STORE_DEREF:
            return -1;
        case DELETE_DEREF:
            return 0;
        case GET_AWAITABLE:
            return 0;
        case SETUP_ASYNC_WITH:
            return 6;
        case BEFORE_ASYNC_WITH:
            return 1;
        case GET_AITER:
            return 0;
        case GET_ANEXT:
            return 1;
        case GET_YIELD_FROM_ITER:
            return 0;
        case FORMAT_VALUE:
            /* If there's a fmt_spec on the stack, we go from 2->1,
               else 1->1. */
            return (oparg & FVS_MASK) == FVS_HAVE_SPEC ? -1 : 0;
        default:
            return PY_INVALID_STACK_EFFECT;
    }
    return PY_INVALID_STACK_EFFECT; /* not reachable */
}

/* Add an opcode with no argument.
   Returns 0 on failure, 1 on success.
*/

static int
compiler_addop(struct compiler *c, int opcode)
{
    basicblock *b;
    struct instr *i;
    int off;
    assert(!HAS_ARG(opcode));
    off = compiler_next_instr(c, c->u->u_curblock);
    if (off < 0)
        return 0;
    b = c->u->u_curblock;
    i = &b->b_instr[off];
    i->i_opcode = opcode;
    i->i_oparg = 0;
    if (opcode == RETURN_VALUE)
        b->b_return = 1;
    compiler_set_lineno(c, off);
    return 1;
}

static Py_ssize_t
compiler_add_o(struct compiler *c, PyObject *dict, PyObject *o)
{
    PyObject *t, *v;
    Py_ssize_t arg;

    t = _PyCode_ConstantKey(o);
    if (t == NULL)
        return -1;

    v = PyDict_GetItem(dict, t);
    if (!v) {
        if (PyErr_Occurred()) {
            Py_DECREF(t);
            return -1;
        }
        arg = PyDict_Size(dict);
        v = PyLong_FromSsize_t(arg);
        if (!v) {
            Py_DECREF(t);
            return -1;
        }
        if (PyDict_SetItem(dict, t, v) < 0) {
            Py_DECREF(t);
            Py_DECREF(v);
            return -1;
        }
        Py_DECREF(v);
    }
    else
        arg = PyLong_AsLong(v);
    Py_DECREF(t);
    return arg;
}

static int
compiler_addop_o(struct compiler *c, int opcode, PyObject *dict,
                     PyObject *o)
{
    Py_ssize_t arg = compiler_add_o(c, dict, o);
    if (arg < 0)
        return 0;
    return compiler_addop_i(c, opcode, arg);
}

static int
compiler_addop_name(struct compiler *c, int opcode, PyObject *dict,
                    PyObject *o)
{
    Py_ssize_t arg;
    PyObject *mangled = _Py_Mangle(c->u->u_private, o);
    if (!mangled)
        return 0;
    arg = compiler_add_o(c, dict, mangled);
    Py_DECREF(mangled);
    if (arg < 0)
        return 0;
    return compiler_addop_i(c, opcode, arg);
}

/* Add an opcode with an integer argument.
   Returns 0 on failure, 1 on success.
*/

static int
compiler_addop_i(struct compiler *c, int opcode, Py_ssize_t oparg)
{
    struct instr *i;
    int off;

    /* oparg value is unsigned, but a signed C int is usually used to store
       it in the C code (like Python/ceval.c).

       Limit to 32-bit signed C int (rather than INT_MAX) for portability.

       The argument of a concrete bytecode instruction is limited to 8-bit.
       EXTENDED_ARG is used for 16, 24, and 32-bit arguments. */
    assert(HAS_ARG(opcode));
    assert(0 <= oparg && oparg <= 2147483647);

    off = compiler_next_instr(c, c->u->u_curblock);
    if (off < 0)
        return 0;
    i = &c->u->u_curblock->b_instr[off];
    i->i_opcode = opcode;
    i->i_oparg = Py_SAFE_DOWNCAST(oparg, Py_ssize_t, int);
    compiler_set_lineno(c, off);
    return 1;
}

static int
compiler_addop_j(struct compiler *c, int opcode, basicblock *b, int absolute)
{
    struct instr *i;
    int off;

    assert(HAS_ARG(opcode));
    assert(b != NULL);
    off = compiler_next_instr(c, c->u->u_curblock);
    if (off < 0)
        return 0;
    i = &c->u->u_curblock->b_instr[off];
    i->i_opcode = opcode;
    i->i_target = b;
    if (absolute)
        i->i_jabs = 1;
    else
        i->i_jrel = 1;
    compiler_set_lineno(c, off);
    return 1;
}

/* NEXT_BLOCK() creates an implicit jump from the current block
   to the new block.

   The returns inside this macro make it impossible to decref objects
   created in the local function. Local objects should use the arena.
*/
#define NEXT_BLOCK(C) { \
    if (compiler_next_block((C)) == NULL) \
        return 0; \
}

#define ADDOP(C, OP) { \
    if (!compiler_addop((C), (OP))) \
        return 0; \
}

#define ADDOP_IN_SCOPE(C, OP) { \
    if (!compiler_addop((C), (OP))) { \
        compiler_exit_scope(c); \
        return 0; \
    } \
}

#define ADDOP_O(C, OP, O, TYPE) { \
    if (!compiler_addop_o((C), (OP), (C)->u->u_ ## TYPE, (O))) \
        return 0; \
}

/* Same as ADDOP_O, but steals a reference. */
#define ADDOP_N(C, OP, O, TYPE) { \
    if (!compiler_addop_o((C), (OP), (C)->u->u_ ## TYPE, (O))) { \
        Py_DECREF((O)); \
        return 0; \
    } \
    Py_DECREF((O)); \
}

#define ADDOP_NAME(C, OP, O, TYPE) { \
    if (!compiler_addop_name((C), (OP), (C)->u->u_ ## TYPE, (O))) \
        return 0; \
}

#define ADDOP_I(C, OP, O) { \
    if (!compiler_addop_i((C), (OP), (O))) \
        return 0; \
}

#define ADDOP_JABS(C, OP, O) { \
    if (!compiler_addop_j((C), (OP), (O), 1)) \
        return 0; \
}

#define ADDOP_JREL(C, OP, O) { \
    if (!compiler_addop_j((C), (OP), (O), 0)) \
        return 0; \
}

/* VISIT and VISIT_SEQ takes an ASDL type as their second argument.  They use
   the ASDL name to synthesize the name of the C type and the visit function.
*/

#define VISIT(C, TYPE, V) {\
    if (!compiler_visit_ ## TYPE((C), (V))) \
        return 0; \
}

#define VISIT_IN_SCOPE(C, TYPE, V) {\
    if (!compiler_visit_ ## TYPE((C), (V))) { \
        compiler_exit_scope(c); \
        return 0; \
    } \
}

#define VISIT_SLICE(C, V, CTX) {\
    if (!compiler_visit_slice((C), (V), (CTX))) \
        return 0; \
}

#define VISIT_SEQ(C, TYPE, SEQ) { \
    int _i; \
    asdl_seq *seq = (SEQ); /* avoid variable capture */ \
    for (_i = 0; _i < asdl_seq_LEN(seq); _i++) { \
        TYPE ## _ty elt = (TYPE ## _ty)asdl_seq_GET(seq, _i); \
        if (!compiler_visit_ ## TYPE((C), elt)) \
            return 0; \
    } \
}

#define VISIT_SEQ_IN_SCOPE(C, TYPE, SEQ) { \
    int _i; \
    asdl_seq *seq = (SEQ); /* avoid variable capture */ \
    for (_i = 0; _i < asdl_seq_LEN(seq); _i++) { \
        TYPE ## _ty elt = (TYPE ## _ty)asdl_seq_GET(seq, _i); \
        if (!compiler_visit_ ## TYPE((C), elt)) { \
            compiler_exit_scope(c); \
            return 0; \
        } \
    } \
}

static int
compiler_isdocstring(stmt_ty s)
{
    if (s->kind != Expr_kind)
        return 0;
    if (s->v.Expr.value->kind == Str_kind)
        return 1;
    if (s->v.Expr.value->kind == Constant_kind)
        return PyUnicode_CheckExact(s->v.Expr.value->v.Constant.value);
    return 0;
}

static int
is_const(expr_ty e)
{
    switch (e->kind) {
    case Constant_kind:
    case Num_kind:
    case Str_kind:
    case Bytes_kind:
    case Ellipsis_kind:
    case NameConstant_kind:
        return 1;
    case Name_kind:
        return _PyUnicode_EqualToASCIIString(e->v.Name.id, "__debug__");
    default:
        return 0;
    }
}

static PyObject *
get_const_value(struct compiler *c, expr_ty e)
{
    switch (e->kind) {
    case Constant_kind:
        return e->v.Constant.value;
    case Num_kind:
        return e->v.Num.n;
    case Str_kind:
        return e->v.Str.s;
    case Bytes_kind:
        return e->v.Bytes.s;
    case Ellipsis_kind:
        return Py_Ellipsis;
    case NameConstant_kind:
        return e->v.NameConstant.value;
    case Name_kind:
        assert(_PyUnicode_EqualToASCIIString(e->v.Name.id, "__debug__"));
        return c->c_optimize ? Py_False : Py_True;
    default:
        assert(!is_const(e));
        return NULL;
    }
}

/* Search if variable annotations are present statically in a block. */

static int
find_ann(asdl_seq *stmts)
{
    int i, j, res = 0;
    stmt_ty st;

    for (i = 0; i < asdl_seq_LEN(stmts); i++) {
        st = (stmt_ty)asdl_seq_GET(stmts, i);
        switch (st->kind) {
        case AnnAssign_kind:
            return 1;
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
                    return 1;
                }
            }
            res = find_ann(st->v.Try.body) ||
                  find_ann(st->v.Try.finalbody) ||
                  find_ann(st->v.Try.orelse);
            break;
        default:
            res = 0;
        }
        if (res) {
            break;
        }
    }
    return res;
}

/* Compile a sequence of statements, checking for a docstring
   and for annotations. */

static int
compiler_body(struct compiler *c, asdl_seq *stmts)
{
    int i = 0;
    stmt_ty st;

    /* Set current line number to the line number of first statement.
       This way line number for SETUP_ANNOTATIONS will always
       coincide with the line number of first "real" statement in module.
       If body is empy, then lineno will be set later in assemble. */
    if (c->u->u_scope_type == COMPILER_SCOPE_MODULE &&
        !c->u->u_lineno && asdl_seq_LEN(stmts)) {
        st = (stmt_ty)asdl_seq_GET(stmts, 0);
        c->u->u_lineno = st->lineno;
    }
    /* Every annotated class and module should have __annotations__. */
    if (find_ann(stmts)) {
        ADDOP(c, SETUP_ANNOTATIONS);
    }
    if (!asdl_seq_LEN(stmts))
        return 1;
    st = (stmt_ty)asdl_seq_GET(stmts, 0);
    if (compiler_isdocstring(st) && c->c_optimize < 2) {
        /* don't generate docstrings if -OO */
        i = 1;
        VISIT(c, expr, st->v.Expr.value);
        if (!compiler_nameop(c, __doc__, Store))
            return 0;
    }
    for (; i < asdl_seq_LEN(stmts); i++)
        VISIT(c, stmt, (stmt_ty)asdl_seq_GET(stmts, i));
    return 1;
}

static PyCodeObject *
compiler_mod(struct compiler *c, mod_ty mod)
{
    PyCodeObject *co;
    int addNone = 1;
    static PyObject *module;
    if (!module) {
        module = PyUnicode_InternFromString("<module>");
        if (!module)
            return NULL;
    }
    /* Use 0 for firstlineno initially, will fixup in assemble(). */
    if (!compiler_enter_scope(c, module, COMPILER_SCOPE_MODULE, mod, 0))
        return NULL;
    switch (mod->kind) {
    case Module_kind:
        if (!compiler_body(c, mod->v.Module.body)) {
            compiler_exit_scope(c);
            return 0;
        }
        break;
    case Interactive_kind:
        if (find_ann(mod->v.Interactive.body)) {
            ADDOP(c, SETUP_ANNOTATIONS);
        }
        c->c_interactive = 1;
        VISIT_SEQ_IN_SCOPE(c, stmt,
                                mod->v.Interactive.body);
        break;
    case Expression_kind:
        VISIT_IN_SCOPE(c, expr, mod->v.Expression.body);
        addNone = 0;
        break;
    case Suite_kind:
        PyErr_SetString(PyExc_SystemError,
                        "suite should not be possible");
        return 0;
    default:
        PyErr_Format(PyExc_SystemError,
                     "module kind %d should not be possible",
                     mod->kind);
        return 0;
    }
    co = assemble(c, addNone);
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
        _PyUnicode_EqualToASCIIString(name, "__class__"))
        return CELL;
    scope = PyST_GetScope(c->u->u_ste, name);
    if (scope == 0) {
        char buf[350];
        PyOS_snprintf(buf, sizeof(buf),
                      "unknown scope for %.100s in %.100s(%s)\n"
                      "symbols: %s\nlocals: %s\nglobals: %s",
                      PyUnicode_AsUTF8(name),
                      PyUnicode_AsUTF8(c->u->u_name),
                      PyUnicode_AsUTF8(PyObject_Repr(c->u->u_ste->ste_id)),
                      PyUnicode_AsUTF8(PyObject_Repr(c->u->u_ste->ste_symbols)),
                      PyUnicode_AsUTF8(PyObject_Repr(c->u->u_varnames)),
                      PyUnicode_AsUTF8(PyObject_Repr(c->u->u_names))
        );
        Py_FatalError(buf);
    }

    return scope;
}

static int
compiler_lookup_arg(PyObject *dict, PyObject *name)
{
    PyObject *k, *v;
    k = _PyCode_ConstantKey(name);
    if (k == NULL)
        return -1;
    v = PyDict_GetItem(dict, k);
    Py_DECREF(k);
    if (v == NULL)
        return -1;
    return PyLong_AS_LONG(v);
}

static int
compiler_make_closure(struct compiler *c, PyCodeObject *co, Py_ssize_t flags, PyObject *qualname)
{
    Py_ssize_t i, free = PyCode_GetNumFree(co);
    if (qualname == NULL)
        qualname = co->co_name;

    if (free) {
        for (i = 0; i < free; ++i) {
            /* Bypass com_addop_varname because it will generate
               LOAD_DEREF but LOAD_CLOSURE is needed.
            */
            PyObject *name = PyTuple_GET_ITEM(co->co_freevars, i);
            int arg, reftype;

            /* Special case: If a class contains a method with a
               free variable that has the same name as a method,
               the name will be considered free *and* local in the
               class.  It should be handled by the closure, as
               well as by the normal name loookup logic.
            */
            reftype = get_ref_type(c, name);
            if (reftype == CELL)
                arg = compiler_lookup_arg(c->u->u_cellvars, name);
            else /* (reftype == FREE) */
                arg = compiler_lookup_arg(c->u->u_freevars, name);
            if (arg == -1) {
                fprintf(stderr,
                    "lookup %s in %s %d %d\n"
                    "freevars of %s: %s\n",
                    PyUnicode_AsUTF8(PyObject_Repr(name)),
                    PyUnicode_AsUTF8(c->u->u_name),
                    reftype, arg,
                    PyUnicode_AsUTF8(co->co_name),
                    PyUnicode_AsUTF8(PyObject_Repr(co->co_freevars)));
                Py_FatalError("compiler_make_closure()");
            }
            ADDOP_I(c, LOAD_CLOSURE, arg);
        }
        flags |= 0x08;
        ADDOP_I(c, BUILD_TUPLE, free);
    }
    ADDOP_O(c, LOAD_CONST, (PyObject*)co, consts);
    ADDOP_O(c, LOAD_CONST, qualname, consts);
    ADDOP_I(c, MAKE_FUNCTION, flags);
    return 1;
}

static int
compiler_decorators(struct compiler *c, asdl_seq* decos)
{
    int i;

    if (!decos)
        return 1;

    for (i = 0; i < asdl_seq_LEN(decos); i++) {
        VISIT(c, expr, (expr_ty)asdl_seq_GET(decos, i));
    }
    return 1;
}

static int
compiler_visit_kwonlydefaults(struct compiler *c, asdl_seq *kwonlyargs,
                              asdl_seq *kw_defaults)
{
    /* Push a dict of keyword-only default values.

       Return 0 on error, -1 if no dict pushed, 1 if a dict is pushed.
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
                    return 0;
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
            if (!compiler_visit_expr(c, default_)) {
                goto error;
            }
        }
    }
    if (keys != NULL) {
        Py_ssize_t default_count = PyList_GET_SIZE(keys);
        PyObject *keys_tuple = PyList_AsTuple(keys);
        Py_DECREF(keys);
        if (keys_tuple == NULL) {
            return 0;
        }
        ADDOP_N(c, LOAD_CONST, keys_tuple, consts);
        ADDOP_I(c, BUILD_CONST_KEY_MAP, default_count);
        assert(default_count > 0);
        return 1;
    }
    else {
        return -1;
    }

error:
    Py_XDECREF(keys);
    return 0;
}

static int
compiler_visit_argannotation(struct compiler *c, identifier id,
    expr_ty annotation, PyObject *names)
{
    if (annotation) {
        PyObject *mangled;
        VISIT(c, expr, annotation);
        mangled = _Py_Mangle(c->u->u_private, id);
        if (!mangled)
            return 0;
        if (PyList_Append(names, mangled) < 0) {
            Py_DECREF(mangled);
            return 0;
        }
        Py_DECREF(mangled);
    }
    return 1;
}

static int
compiler_visit_argannotations(struct compiler *c, asdl_seq* args,
                              PyObject *names)
{
    int i;
    for (i = 0; i < asdl_seq_LEN(args); i++) {
        arg_ty arg = (arg_ty)asdl_seq_GET(args, i);
        if (!compiler_visit_argannotation(
                        c,
                        arg->arg,
                        arg->annotation,
                        names))
            return 0;
    }
    return 1;
}

static int
compiler_visit_annotations(struct compiler *c, arguments_ty args,
                           expr_ty returns)
{
    /* Push arg annotation dict.
       The expressions are evaluated out-of-order wrt the source code.

       Return 0 on error, -1 if no dict pushed, 1 if a dict is pushed.
       */
    static identifier return_str;
    PyObject *names;
    Py_ssize_t len;
    names = PyList_New(0);
    if (!names)
        return 0;

    if (!compiler_visit_argannotations(c, args->args, names))
        goto error;
    if (args->vararg && args->vararg->annotation &&
        !compiler_visit_argannotation(c, args->vararg->arg,
                                     args->vararg->annotation, names))
        goto error;
    if (!compiler_visit_argannotations(c, args->kwonlyargs, names))
        goto error;
    if (args->kwarg && args->kwarg->annotation &&
        !compiler_visit_argannotation(c, args->kwarg->arg,
                                     args->kwarg->annotation, names))
        goto error;

    if (!return_str) {
        return_str = PyUnicode_InternFromString("return");
        if (!return_str)
            goto error;
    }
    if (!compiler_visit_argannotation(c, return_str, returns, names)) {
        goto error;
    }

    len = PyList_GET_SIZE(names);
    if (len) {
        PyObject *keytuple = PyList_AsTuple(names);
        Py_DECREF(names);
        if (keytuple == NULL) {
            return 0;
        }
        ADDOP_N(c, LOAD_CONST, keytuple, consts);
        ADDOP_I(c, BUILD_CONST_KEY_MAP, len);
        return 1;
    }
    else {
        Py_DECREF(names);
        return -1;
    }

error:
    Py_DECREF(names);
    return 0;
}

static int
compiler_visit_defaults(struct compiler *c, arguments_ty args)
{
    VISIT_SEQ(c, expr, args->defaults);
    ADDOP_I(c, BUILD_TUPLE, asdl_seq_LEN(args->defaults));
    return 1;
}

static Py_ssize_t
compiler_default_arguments(struct compiler *c, arguments_ty args)
{
    Py_ssize_t funcflags = 0;
    if (args->defaults && asdl_seq_LEN(args->defaults) > 0) {
        if (!compiler_visit_defaults(c, args))
            return -1;
        funcflags |= 0x01;
    }
    if (args->kwonlyargs) {
        int res = compiler_visit_kwonlydefaults(c, args->kwonlyargs,
                                                args->kw_defaults);
        if (res == 0) {
            return -1;
        }
        else if (res > 0) {
            funcflags |= 0x02;
        }
    }
    return funcflags;
}

static int
compiler_function(struct compiler *c, stmt_ty s, int is_async)
{
    PyCodeObject *co;
    PyObject *qualname, *first_const = Py_None;
    arguments_ty args;
    expr_ty returns;
    identifier name;
    asdl_seq* decos;
    asdl_seq *body;
    stmt_ty st;
    Py_ssize_t i, n, funcflags;
    int docstring;
    int annotations;
    int scope_type;

    if (is_async) {
        assert(s->kind == AsyncFunctionDef_kind);

        args = s->v.AsyncFunctionDef.args;
        returns = s->v.AsyncFunctionDef.returns;
        decos = s->v.AsyncFunctionDef.decorator_list;
        name = s->v.AsyncFunctionDef.name;
        body = s->v.AsyncFunctionDef.body;

        scope_type = COMPILER_SCOPE_ASYNC_FUNCTION;
    } else {
        assert(s->kind == FunctionDef_kind);

        args = s->v.FunctionDef.args;
        returns = s->v.FunctionDef.returns;
        decos = s->v.FunctionDef.decorator_list;
        name = s->v.FunctionDef.name;
        body = s->v.FunctionDef.body;

        scope_type = COMPILER_SCOPE_FUNCTION;
    }

    if (!compiler_decorators(c, decos))
        return 0;

    funcflags = compiler_default_arguments(c, args);
    if (funcflags == -1) {
        return 0;
    }

    annotations = compiler_visit_annotations(c, args, returns);
    if (annotations == 0) {
        return 0;
    }
    else if (annotations > 0) {
        funcflags |= 0x04;
    }

    if (!compiler_enter_scope(c, name, scope_type, (void *)s, s->lineno)) {
        return 0;
    }

    st = (stmt_ty)asdl_seq_GET(body, 0);
    docstring = compiler_isdocstring(st);
    if (docstring && c->c_optimize < 2) {
        if (st->v.Expr.value->kind == Constant_kind)
            first_const = st->v.Expr.value->v.Constant.value;
        else
            first_const = st->v.Expr.value->v.Str.s;
    }
    if (compiler_add_o(c, c->u->u_consts, first_const) < 0)      {
        compiler_exit_scope(c);
        return 0;
    }

    c->u->u_argcount = asdl_seq_LEN(args->args);
    c->u->u_kwonlyargcount = asdl_seq_LEN(args->kwonlyargs);
    n = asdl_seq_LEN(body);
    /* if there was a docstring, we need to skip the first statement */
    for (i = docstring; i < n; i++) {
        st = (stmt_ty)asdl_seq_GET(body, i);
        VISIT_IN_SCOPE(c, stmt, st);
    }
    co = assemble(c, 1);
    qualname = c->u->u_qualname;
    Py_INCREF(qualname);
    compiler_exit_scope(c);
    if (co == NULL) {
        Py_XDECREF(qualname);
        Py_XDECREF(co);
        return 0;
    }

    compiler_make_closure(c, co, funcflags, qualname);
    Py_DECREF(qualname);
    Py_DECREF(co);

    /* decorators */
    for (i = 0; i < asdl_seq_LEN(decos); i++) {
        ADDOP_I(c, CALL_FUNCTION, 1);
    }

    return compiler_nameop(c, name, Store);
}

static int
compiler_class(struct compiler *c, stmt_ty s)
{
    PyCodeObject *co;
    PyObject *str;
    int i;
    asdl_seq* decos = s->v.ClassDef.decorator_list;

    if (!compiler_decorators(c, decos))
        return 0;

    /* ultimately generate code for:
         <name> = __build_class__(<func>, <name>, *<bases>, **<keywords>)
       where:
         <func> is a function/closure created from the class body;
            it has a single argument (__locals__) where the dict
            (or MutableSequence) representing the locals is passed
         <name> is the class name
         <bases> is the positional arguments and *varargs argument
         <keywords> is the keyword arguments and **kwds argument
       This borrows from compiler_call.
    */

    /* 1. compile the class body into a code object */
    if (!compiler_enter_scope(c, s->v.ClassDef.name,
                              COMPILER_SCOPE_CLASS, (void *)s, s->lineno))
        return 0;
    /* this block represents what we do in the new scope */
    {
        /* use the class name for name mangling */
        Py_INCREF(s->v.ClassDef.name);
        Py_XSETREF(c->u->u_private, s->v.ClassDef.name);
        /* load (global) __name__ ... */
        str = PyUnicode_InternFromString("__name__");
        if (!str || !compiler_nameop(c, str, Load)) {
            Py_XDECREF(str);
            compiler_exit_scope(c);
            return 0;
        }
        Py_DECREF(str);
        /* ... and store it as __module__ */
        str = PyUnicode_InternFromString("__module__");
        if (!str || !compiler_nameop(c, str, Store)) {
            Py_XDECREF(str);
            compiler_exit_scope(c);
            return 0;
        }
        Py_DECREF(str);
        assert(c->u->u_qualname);
        ADDOP_O(c, LOAD_CONST, c->u->u_qualname, consts);
        str = PyUnicode_InternFromString("__qualname__");
        if (!str || !compiler_nameop(c, str, Store)) {
            Py_XDECREF(str);
            compiler_exit_scope(c);
            return 0;
        }
        Py_DECREF(str);
        /* compile the body proper */
        if (!compiler_body(c, s->v.ClassDef.body)) {
            compiler_exit_scope(c);
            return 0;
        }
        /* Return __classcell__ if it is referenced, otherwise return None */
        if (c->u->u_ste->ste_needs_class_closure) {
            /* Store __classcell__ into class namespace & return it */
            str = PyUnicode_InternFromString("__class__");
            if (str == NULL) {
                compiler_exit_scope(c);
                return 0;
            }
            i = compiler_lookup_arg(c->u->u_cellvars, str);
            Py_DECREF(str);
            if (i < 0) {
                compiler_exit_scope(c);
                return 0;
            }
            assert(i == 0);

            ADDOP_I(c, LOAD_CLOSURE, i);
            ADDOP(c, DUP_TOP);
            str = PyUnicode_InternFromString("__classcell__");
            if (!str || !compiler_nameop(c, str, Store)) {
                Py_XDECREF(str);
                compiler_exit_scope(c);
                return 0;
            }
            Py_DECREF(str);
        }
        else {
            /* No methods referenced __class__, so just return None */
            assert(PyDict_Size(c->u->u_cellvars) == 0);
            ADDOP_O(c, LOAD_CONST, Py_None, consts);
        }
        ADDOP_IN_SCOPE(c, RETURN_VALUE);
        /* create the code object */
        co = assemble(c, 1);
    }
    /* leave the new scope */
    compiler_exit_scope(c);
    if (co == NULL)
        return 0;

    /* 2. load the 'build_class' function */
    ADDOP(c, LOAD_BUILD_CLASS);

    /* 3. load a function (or closure) made from the code object */
    compiler_make_closure(c, co, 0, NULL);
    Py_DECREF(co);

    /* 4. load class name */
    ADDOP_O(c, LOAD_CONST, s->v.ClassDef.name, consts);

    /* 5. generate the rest of the code for the call */
    if (!compiler_call_helper(c, 2,
                              s->v.ClassDef.bases,
                              s->v.ClassDef.keywords))
        return 0;

    /* 6. apply decorators */
    for (i = 0; i < asdl_seq_LEN(decos); i++) {
        ADDOP_I(c, CALL_FUNCTION, 1);
    }

    /* 7. store into <name> */
    if (!compiler_nameop(c, s->v.ClassDef.name, Store))
        return 0;
    return 1;
}

static int
compiler_ifexp(struct compiler *c, expr_ty e)
{
    basicblock *end, *next;

    assert(e->kind == IfExp_kind);
    end = compiler_new_block(c);
    if (end == NULL)
        return 0;
    next = compiler_new_block(c);
    if (next == NULL)
        return 0;
    VISIT(c, expr, e->v.IfExp.test);
    ADDOP_JABS(c, POP_JUMP_IF_FALSE, next);
    VISIT(c, expr, e->v.IfExp.body);
    ADDOP_JREL(c, JUMP_FORWARD, end);
    compiler_use_next_block(c, next);
    VISIT(c, expr, e->v.IfExp.orelse);
    compiler_use_next_block(c, end);
    return 1;
}

static int
compiler_lambda(struct compiler *c, expr_ty e)
{
    PyCodeObject *co;
    PyObject *qualname;
    static identifier name;
    Py_ssize_t funcflags;
    arguments_ty args = e->v.Lambda.args;
    assert(e->kind == Lambda_kind);

    if (!name) {
        name = PyUnicode_InternFromString("<lambda>");
        if (!name)
            return 0;
    }

    funcflags = compiler_default_arguments(c, args);
    if (funcflags == -1) {
        return 0;
    }

    if (!compiler_enter_scope(c, name, COMPILER_SCOPE_LAMBDA,
                              (void *)e, e->lineno))
        return 0;

    /* Make None the first constant, so the lambda can't have a
       docstring. */
    if (compiler_add_o(c, c->u->u_consts, Py_None) < 0)
        return 0;

    c->u->u_argcount = asdl_seq_LEN(args->args);
    c->u->u_kwonlyargcount = asdl_seq_LEN(args->kwonlyargs);
    VISIT_IN_SCOPE(c, expr, e->v.Lambda.body);
    if (c->u->u_ste->ste_generator) {
        co = assemble(c, 0);
    }
    else {
        ADDOP_IN_SCOPE(c, RETURN_VALUE);
        co = assemble(c, 1);
    }
    qualname = c->u->u_qualname;
    Py_INCREF(qualname);
    compiler_exit_scope(c);
    if (co == NULL)
        return 0;

    compiler_make_closure(c, co, funcflags, qualname);
    Py_DECREF(qualname);
    Py_DECREF(co);

    return 1;
}

static int
compiler_if(struct compiler *c, stmt_ty s)
{
    basicblock *end, *next;
    int constant;
    assert(s->kind == If_kind);
    end = compiler_new_block(c);
    if (end == NULL)
        return 0;

    constant = expr_constant(c, s->v.If.test);
    /* constant = 0: "if 0"
     * constant = 1: "if 1", "if 2", ...
     * constant = -1: rest */
    if (constant == 0) {
        if (s->v.If.orelse)
            VISIT_SEQ(c, stmt, s->v.If.orelse);
    } else if (constant == 1) {
        VISIT_SEQ(c, stmt, s->v.If.body);
    } else {
        if (asdl_seq_LEN(s->v.If.orelse)) {
            next = compiler_new_block(c);
            if (next == NULL)
                return 0;
        }
        else
            next = end;
        VISIT(c, expr, s->v.If.test);
        ADDOP_JABS(c, POP_JUMP_IF_FALSE, next);
        VISIT_SEQ(c, stmt, s->v.If.body);
        if (asdl_seq_LEN(s->v.If.orelse)) {
            ADDOP_JREL(c, JUMP_FORWARD, end);
            compiler_use_next_block(c, next);
            VISIT_SEQ(c, stmt, s->v.If.orelse);
        }
    }
    compiler_use_next_block(c, end);
    return 1;
}

static int
compiler_for(struct compiler *c, stmt_ty s)
{
    basicblock *start, *cleanup, *end;

    start = compiler_new_block(c);
    cleanup = compiler_new_block(c);
    end = compiler_new_block(c);
    if (start == NULL || end == NULL || cleanup == NULL)
        return 0;
    ADDOP_JREL(c, SETUP_LOOP, end);
    if (!compiler_push_fblock(c, LOOP, start))
        return 0;
    VISIT(c, expr, s->v.For.iter);
    ADDOP(c, GET_ITER);
    compiler_use_next_block(c, start);
    ADDOP_JREL(c, FOR_ITER, cleanup);
    VISIT(c, expr, s->v.For.target);
    VISIT_SEQ(c, stmt, s->v.For.body);
    ADDOP_JABS(c, JUMP_ABSOLUTE, start);
    compiler_use_next_block(c, cleanup);
    ADDOP(c, POP_BLOCK);
    compiler_pop_fblock(c, LOOP, start);
    VISIT_SEQ(c, stmt, s->v.For.orelse);
    compiler_use_next_block(c, end);
    return 1;
}


static int
compiler_async_for(struct compiler *c, stmt_ty s)
{
    _Py_IDENTIFIER(StopAsyncIteration);

    basicblock *try, *except, *end, *after_try, *try_cleanup,
               *after_loop_else;

    PyObject *stop_aiter_error = _PyUnicode_FromId(&PyId_StopAsyncIteration);
    if (stop_aiter_error == NULL) {
        return 0;
    }

    try = compiler_new_block(c);
    except = compiler_new_block(c);
    end = compiler_new_block(c);
    after_try = compiler_new_block(c);
    try_cleanup = compiler_new_block(c);
    after_loop_else = compiler_new_block(c);

    if (try == NULL || except == NULL || end == NULL
            || after_try == NULL || try_cleanup == NULL
            || after_loop_else == NULL)
        return 0;

    ADDOP_JREL(c, SETUP_LOOP, end);
    if (!compiler_push_fblock(c, LOOP, try))
        return 0;

    VISIT(c, expr, s->v.AsyncFor.iter);
    ADDOP(c, GET_AITER);
    ADDOP_O(c, LOAD_CONST, Py_None, consts);
    ADDOP(c, YIELD_FROM);

    compiler_use_next_block(c, try);


    ADDOP_JREL(c, SETUP_EXCEPT, except);
    if (!compiler_push_fblock(c, EXCEPT, try))
        return 0;

    ADDOP(c, GET_ANEXT);
    ADDOP_O(c, LOAD_CONST, Py_None, consts);
    ADDOP(c, YIELD_FROM);
    VISIT(c, expr, s->v.AsyncFor.target);
    ADDOP(c, POP_BLOCK);
    compiler_pop_fblock(c, EXCEPT, try);
    ADDOP_JREL(c, JUMP_FORWARD, after_try);


    compiler_use_next_block(c, except);
    ADDOP(c, DUP_TOP);
    ADDOP_O(c, LOAD_GLOBAL, stop_aiter_error, names);
    ADDOP_I(c, COMPARE_OP, PyCmp_EXC_MATCH);
    ADDOP_JABS(c, POP_JUMP_IF_FALSE, try_cleanup);

    ADDOP(c, POP_TOP);
    ADDOP(c, POP_TOP);
    ADDOP(c, POP_TOP);
    ADDOP(c, POP_EXCEPT); /* for SETUP_EXCEPT */
    ADDOP(c, POP_BLOCK); /* for SETUP_LOOP */
    ADDOP_JABS(c, JUMP_ABSOLUTE, after_loop_else);


    compiler_use_next_block(c, try_cleanup);
    ADDOP(c, END_FINALLY);

    compiler_use_next_block(c, after_try);
    VISIT_SEQ(c, stmt, s->v.AsyncFor.body);
    ADDOP_JABS(c, JUMP_ABSOLUTE, try);

    ADDOP(c, POP_BLOCK); /* for SETUP_LOOP */
    compiler_pop_fblock(c, LOOP, try);

    compiler_use_next_block(c, after_loop_else);
    VISIT_SEQ(c, stmt, s->v.For.orelse);

    compiler_use_next_block(c, end);

    return 1;
}

static int
compiler_while(struct compiler *c, stmt_ty s)
{
    basicblock *loop, *orelse, *end, *anchor = NULL;
    int constant = expr_constant(c, s->v.While.test);

    if (constant == 0) {
        if (s->v.While.orelse)
            VISIT_SEQ(c, stmt, s->v.While.orelse);
        return 1;
    }
    loop = compiler_new_block(c);
    end = compiler_new_block(c);
    if (constant == -1) {
        anchor = compiler_new_block(c);
        if (anchor == NULL)
            return 0;
    }
    if (loop == NULL || end == NULL)
        return 0;
    if (s->v.While.orelse) {
        orelse = compiler_new_block(c);
        if (orelse == NULL)
            return 0;
    }
    else
        orelse = NULL;

    ADDOP_JREL(c, SETUP_LOOP, end);
    compiler_use_next_block(c, loop);
    if (!compiler_push_fblock(c, LOOP, loop))
        return 0;
    if (constant == -1) {
        VISIT(c, expr, s->v.While.test);
        ADDOP_JABS(c, POP_JUMP_IF_FALSE, anchor);
    }
    VISIT_SEQ(c, stmt, s->v.While.body);
    ADDOP_JABS(c, JUMP_ABSOLUTE, loop);

    /* XXX should the two POP instructions be in a separate block
       if there is no else clause ?
    */

    if (constant == -1)
        compiler_use_next_block(c, anchor);
    ADDOP(c, POP_BLOCK);
    compiler_pop_fblock(c, LOOP, loop);
    if (orelse != NULL) /* what if orelse is just pass? */
        VISIT_SEQ(c, stmt, s->v.While.orelse);
    compiler_use_next_block(c, end);

    return 1;
}

static int
compiler_continue(struct compiler *c)
{
    static const char LOOP_ERROR_MSG[] = "'continue' not properly in loop";
    static const char IN_FINALLY_ERROR_MSG[] =
                    "'continue' not supported inside 'finally' clause";
    int i;

    if (!c->u->u_nfblocks)
        return compiler_error(c, LOOP_ERROR_MSG);
    i = c->u->u_nfblocks - 1;
    switch (c->u->u_fblock[i].fb_type) {
    case LOOP:
        ADDOP_JABS(c, JUMP_ABSOLUTE, c->u->u_fblock[i].fb_block);
        break;
    case EXCEPT:
    case FINALLY_TRY:
        while (--i >= 0 && c->u->u_fblock[i].fb_type != LOOP) {
            /* Prevent continue anywhere under a finally
                  even if hidden in a sub-try or except. */
            if (c->u->u_fblock[i].fb_type == FINALLY_END)
                return compiler_error(c, IN_FINALLY_ERROR_MSG);
        }
        if (i == -1)
            return compiler_error(c, LOOP_ERROR_MSG);
        ADDOP_JABS(c, CONTINUE_LOOP, c->u->u_fblock[i].fb_block);
        break;
    case FINALLY_END:
        return compiler_error(c, IN_FINALLY_ERROR_MSG);
    }

    return 1;
}

/* Code generated for "try: <body> finally: <finalbody>" is as follows:

        SETUP_FINALLY           L
        <code for body>
        POP_BLOCK
        LOAD_CONST              <None>
    L:          <code for finalbody>
        END_FINALLY

   The special instructions use the block stack.  Each block
   stack entry contains the instruction that created it (here
   SETUP_FINALLY), the level of the value stack at the time the
   block stack entry was created, and a label (here L).

   SETUP_FINALLY:
    Pushes the current value stack level and the label
    onto the block stack.
   POP_BLOCK:
    Pops en entry from the block stack, and pops the value
    stack until its level is the same as indicated on the
    block stack.  (The label is ignored.)
   END_FINALLY:
    Pops a variable number of entries from the *value* stack
    and re-raises the exception they specify.  The number of
    entries popped depends on the (pseudo) exception type.

   The block stack is unwound when an exception is raised:
   when a SETUP_FINALLY entry is found, the exception is pushed
   onto the value stack (and the exception condition is cleared),
   and the interpreter jumps to the label gotten from the block
   stack.
*/

static int
compiler_try_finally(struct compiler *c, stmt_ty s)
{
    basicblock *body, *end;
    body = compiler_new_block(c);
    end = compiler_new_block(c);
    if (body == NULL || end == NULL)
        return 0;

    ADDOP_JREL(c, SETUP_FINALLY, end);
    compiler_use_next_block(c, body);
    if (!compiler_push_fblock(c, FINALLY_TRY, body))
        return 0;
    if (s->v.Try.handlers && asdl_seq_LEN(s->v.Try.handlers)) {
        if (!compiler_try_except(c, s))
            return 0;
    }
    else {
        VISIT_SEQ(c, stmt, s->v.Try.body);
    }
    ADDOP(c, POP_BLOCK);
    compiler_pop_fblock(c, FINALLY_TRY, body);

    ADDOP_O(c, LOAD_CONST, Py_None, consts);
    compiler_use_next_block(c, end);
    if (!compiler_push_fblock(c, FINALLY_END, end))
        return 0;
    VISIT_SEQ(c, stmt, s->v.Try.finalbody);
    ADDOP(c, END_FINALLY);
    compiler_pop_fblock(c, FINALLY_END, end);

    return 1;
}

/*
   Code generated for "try: S except E1 as V1: S1 except E2 as V2: S2 ...":
   (The contents of the value stack is shown in [], with the top
   at the right; 'tb' is trace-back info, 'val' the exception's
   associated value, and 'exc' the exception.)

   Value stack          Label   Instruction     Argument
   []                           SETUP_EXCEPT    L1
   []                           <code for S>
   []                           POP_BLOCK
   []                           JUMP_FORWARD    L0

   [tb, val, exc]       L1:     DUP                             )
   [tb, val, exc, exc]          <evaluate E1>                   )
   [tb, val, exc, exc, E1]      COMPARE_OP      EXC_MATCH       ) only if E1
   [tb, val, exc, 1-or-0]       POP_JUMP_IF_FALSE       L2      )
   [tb, val, exc]               POP
   [tb, val]                    <assign to V1>  (or POP if no V1)
   [tb]                         POP
   []                           <code for S1>
                                JUMP_FORWARD    L0

   [tb, val, exc]       L2:     DUP
   .............................etc.......................

   [tb, val, exc]       Ln+1:   END_FINALLY     # re-raise exception

   []                   L0:     <next statement>

   Of course, parts are not generated if Vi or Ei is not present.
*/
static int
compiler_try_except(struct compiler *c, stmt_ty s)
{
    basicblock *body, *orelse, *except, *end;
    Py_ssize_t i, n;

    body = compiler_new_block(c);
    except = compiler_new_block(c);
    orelse = compiler_new_block(c);
    end = compiler_new_block(c);
    if (body == NULL || except == NULL || orelse == NULL || end == NULL)
        return 0;
    ADDOP_JREL(c, SETUP_EXCEPT, except);
    compiler_use_next_block(c, body);
    if (!compiler_push_fblock(c, EXCEPT, body))
        return 0;
    VISIT_SEQ(c, stmt, s->v.Try.body);
    ADDOP(c, POP_BLOCK);
    compiler_pop_fblock(c, EXCEPT, body);
    ADDOP_JREL(c, JUMP_FORWARD, orelse);
    n = asdl_seq_LEN(s->v.Try.handlers);
    compiler_use_next_block(c, except);
    for (i = 0; i < n; i++) {
        excepthandler_ty handler = (excepthandler_ty)asdl_seq_GET(
            s->v.Try.handlers, i);
        if (!handler->v.ExceptHandler.type && i < n-1)
            return compiler_error(c, "default 'except:' must be last");
        c->u->u_lineno_set = 0;
        c->u->u_lineno = handler->lineno;
        c->u->u_col_offset = handler->col_offset;
        except = compiler_new_block(c);
        if (except == NULL)
            return 0;
        if (handler->v.ExceptHandler.type) {
            ADDOP(c, DUP_TOP);
            VISIT(c, expr, handler->v.ExceptHandler.type);
            ADDOP_I(c, COMPARE_OP, PyCmp_EXC_MATCH);
            ADDOP_JABS(c, POP_JUMP_IF_FALSE, except);
        }
        ADDOP(c, POP_TOP);
        if (handler->v.ExceptHandler.name) {
            basicblock *cleanup_end, *cleanup_body;

            cleanup_end = compiler_new_block(c);
            cleanup_body = compiler_new_block(c);
            if (!(cleanup_end || cleanup_body))
                return 0;

            compiler_nameop(c, handler->v.ExceptHandler.name, Store);
            ADDOP(c, POP_TOP);

            /*
              try:
                  # body
              except type as name:
                  try:
                      # body
                  finally:
                      name = None
                      del name
            */

            /* second try: */
            ADDOP_JREL(c, SETUP_FINALLY, cleanup_end);
            compiler_use_next_block(c, cleanup_body);
            if (!compiler_push_fblock(c, FINALLY_TRY, cleanup_body))
                return 0;

            /* second # body */
            VISIT_SEQ(c, stmt, handler->v.ExceptHandler.body);
            ADDOP(c, POP_BLOCK);
            ADDOP(c, POP_EXCEPT);
            compiler_pop_fblock(c, FINALLY_TRY, cleanup_body);

            /* finally: */
            ADDOP_O(c, LOAD_CONST, Py_None, consts);
            compiler_use_next_block(c, cleanup_end);
            if (!compiler_push_fblock(c, FINALLY_END, cleanup_end))
                return 0;

            /* name = None */
            ADDOP_O(c, LOAD_CONST, Py_None, consts);
            compiler_nameop(c, handler->v.ExceptHandler.name, Store);

            /* del name */
            compiler_nameop(c, handler->v.ExceptHandler.name, Del);

            ADDOP(c, END_FINALLY);
            compiler_pop_fblock(c, FINALLY_END, cleanup_end);
        }
        else {
            basicblock *cleanup_body;

            cleanup_body = compiler_new_block(c);
            if (!cleanup_body)
                return 0;

            ADDOP(c, POP_TOP);
            ADDOP(c, POP_TOP);
            compiler_use_next_block(c, cleanup_body);
            if (!compiler_push_fblock(c, FINALLY_TRY, cleanup_body))
                return 0;
            VISIT_SEQ(c, stmt, handler->v.ExceptHandler.body);
            ADDOP(c, POP_EXCEPT);
            compiler_pop_fblock(c, FINALLY_TRY, cleanup_body);
        }
        ADDOP_JREL(c, JUMP_FORWARD, end);
        compiler_use_next_block(c, except);
    }
    ADDOP(c, END_FINALLY);
    compiler_use_next_block(c, orelse);
    VISIT_SEQ(c, stmt, s->v.Try.orelse);
    compiler_use_next_block(c, end);
    return 1;
}

static int
compiler_try(struct compiler *c, stmt_ty s) {
    if (s->v.Try.finalbody && asdl_seq_LEN(s->v.Try.finalbody))
        return compiler_try_finally(c, s);
    else
        return compiler_try_except(c, s);
}


static int
compiler_import_as(struct compiler *c, identifier name, identifier asname)
{
    /* The IMPORT_NAME opcode was already generated.  This function
       merely needs to bind the result to a name.

       If there is a dot in name, we need to split it and emit a
       LOAD_ATTR for each name.
    */
    Py_ssize_t dot = PyUnicode_FindChar(name, '.', 0,
                                        PyUnicode_GET_LENGTH(name), 1);
    if (dot == -2)
        return 0;
    if (dot != -1) {
        /* Consume the base module name to get the first attribute */
        Py_ssize_t pos = dot + 1;
        while (dot != -1) {
            PyObject *attr;
            dot = PyUnicode_FindChar(name, '.', pos,
                                     PyUnicode_GET_LENGTH(name), 1);
            if (dot == -2)
                return 0;
            attr = PyUnicode_Substring(name, pos,
                                       (dot != -1) ? dot :
                                       PyUnicode_GET_LENGTH(name));
            if (!attr)
                return 0;
            ADDOP_O(c, LOAD_ATTR, attr, names);
            Py_DECREF(attr);
            pos = dot + 1;
        }
    }
    return compiler_nameop(c, asname, Store);
}

static int
compiler_import(struct compiler *c, stmt_ty s)
{
    /* The Import node stores a module name like a.b.c as a single
       string.  This is convenient for all cases except
         import a.b.c as d
       where we need to parse that string to extract the individual
       module names.
       XXX Perhaps change the representation to make this case simpler?
     */
    Py_ssize_t i, n = asdl_seq_LEN(s->v.Import.names);

    for (i = 0; i < n; i++) {
        alias_ty alias = (alias_ty)asdl_seq_GET(s->v.Import.names, i);
        int r;
        PyObject *level;

        level = PyLong_FromLong(0);
        if (level == NULL)
            return 0;

        ADDOP_O(c, LOAD_CONST, level, consts);
        Py_DECREF(level);
        ADDOP_O(c, LOAD_CONST, Py_None, consts);
        ADDOP_NAME(c, IMPORT_NAME, alias->name, names);

        if (alias->asname) {
            r = compiler_import_as(c, alias->name, alias->asname);
            if (!r)
                return r;
        }
        else {
            identifier tmp = alias->name;
            Py_ssize_t dot = PyUnicode_FindChar(
                alias->name, '.', 0, PyUnicode_GET_LENGTH(alias->name), 1);
            if (dot != -1) {
                tmp = PyUnicode_Substring(alias->name, 0, dot);
                if (tmp == NULL)
                    return 0;
            }
            r = compiler_nameop(c, tmp, Store);
            if (dot != -1) {
                Py_DECREF(tmp);
            }
            if (!r)
                return r;
        }
    }
    return 1;
}

static int
compiler_from_import(struct compiler *c, stmt_ty s)
{
    Py_ssize_t i, n = asdl_seq_LEN(s->v.ImportFrom.names);

    PyObject *names = PyTuple_New(n);
    PyObject *level;
    static PyObject *empty_string;

    if (!empty_string) {
        empty_string = PyUnicode_FromString("");
        if (!empty_string)
            return 0;
    }

    if (!names)
        return 0;

    level = PyLong_FromLong(s->v.ImportFrom.level);
    if (!level) {
        Py_DECREF(names);
        return 0;
    }

    /* build up the names */
    for (i = 0; i < n; i++) {
        alias_ty alias = (alias_ty)asdl_seq_GET(s->v.ImportFrom.names, i);
        Py_INCREF(alias->name);
        PyTuple_SET_ITEM(names, i, alias->name);
    }

    if (s->lineno > c->c_future->ff_lineno && s->v.ImportFrom.module &&
        _PyUnicode_EqualToASCIIString(s->v.ImportFrom.module, "__future__")) {
        Py_DECREF(level);
        Py_DECREF(names);
        return compiler_error(c, "from __future__ imports must occur "
                              "at the beginning of the file");
    }

    ADDOP_O(c, LOAD_CONST, level, consts);
    Py_DECREF(level);
    ADDOP_O(c, LOAD_CONST, names, consts);
    Py_DECREF(names);
    if (s->v.ImportFrom.module) {
        ADDOP_NAME(c, IMPORT_NAME, s->v.ImportFrom.module, names);
    }
    else {
        ADDOP_NAME(c, IMPORT_NAME, empty_string, names);
    }
    for (i = 0; i < n; i++) {
        alias_ty alias = (alias_ty)asdl_seq_GET(s->v.ImportFrom.names, i);
        identifier store_name;

        if (i == 0 && PyUnicode_READ_CHAR(alias->name, 0) == '*') {
            assert(n == 1);
            ADDOP(c, IMPORT_STAR);
            return 1;
        }

        ADDOP_NAME(c, IMPORT_FROM, alias->name, names);
        store_name = alias->name;
        if (alias->asname)
            store_name = alias->asname;

        if (!compiler_nameop(c, store_name, Store)) {
            Py_DECREF(names);
            return 0;
        }
    }
    /* remove imported module */
    ADDOP(c, POP_TOP);
    return 1;
}

static int
compiler_assert(struct compiler *c, stmt_ty s)
{
    static PyObject *assertion_error = NULL;
    basicblock *end;
    PyObject* msg;

    if (c->c_optimize)
        return 1;
    if (assertion_error == NULL) {
        assertion_error = PyUnicode_InternFromString("AssertionError");
        if (assertion_error == NULL)
            return 0;
    }
    if (s->v.Assert.test->kind == Tuple_kind &&
        asdl_seq_LEN(s->v.Assert.test->v.Tuple.elts) > 0) {
        msg = PyUnicode_FromString("assertion is always true, "
                                   "perhaps remove parentheses?");
        if (msg == NULL)
            return 0;
        if (PyErr_WarnExplicitObject(PyExc_SyntaxWarning, msg,
                                     c->c_filename, c->u->u_lineno,
                                     NULL, NULL) == -1) {
            Py_DECREF(msg);
            return 0;
        }
        Py_DECREF(msg);
    }
    VISIT(c, expr, s->v.Assert.test);
    end = compiler_new_block(c);
    if (end == NULL)
        return 0;
    ADDOP_JABS(c, POP_JUMP_IF_TRUE, end);
    ADDOP_O(c, LOAD_GLOBAL, assertion_error, names);
    if (s->v.Assert.msg) {
        VISIT(c, expr, s->v.Assert.msg);
        ADDOP_I(c, CALL_FUNCTION, 1);
    }
    ADDOP_I(c, RAISE_VARARGS, 1);
    compiler_use_next_block(c, end);
    return 1;
}

static int
compiler_visit_stmt_expr(struct compiler *c, expr_ty value)
{
    if (c->c_interactive && c->c_nestlevel <= 1) {
        VISIT(c, expr, value);
        ADDOP(c, PRINT_EXPR);
        return 1;
    }

    if (is_const(value)) {
        /* ignore constant statement */
        return 1;
    }

    VISIT(c, expr, value);
    ADDOP(c, POP_TOP);
    return 1;
}

static int
compiler_visit_stmt(struct compiler *c, stmt_ty s)
{
    Py_ssize_t i, n;

    /* Always assign a lineno to the next instruction for a stmt. */
    c->u->u_lineno = s->lineno;
    c->u->u_col_offset = s->col_offset;
    c->u->u_lineno_set = 0;

    switch (s->kind) {
    case FunctionDef_kind:
        return compiler_function(c, s, 0);
    case ClassDef_kind:
        return compiler_class(c, s);
    case Return_kind:
        if (c->u->u_ste->ste_type != FunctionBlock)
            return compiler_error(c, "'return' outside function");
        if (s->v.Return.value) {
            if (c->u->u_ste->ste_coroutine && c->u->u_ste->ste_generator)
                return compiler_error(
                    c, "'return' with value in async generator");
            VISIT(c, expr, s->v.Return.value);
        }
        else
            ADDOP_O(c, LOAD_CONST, Py_None, consts);
        ADDOP(c, RETURN_VALUE);
        break;
    case Delete_kind:
        VISIT_SEQ(c, expr, s->v.Delete.targets)
        break;
    case Assign_kind:
        n = asdl_seq_LEN(s->v.Assign.targets);
        VISIT(c, expr, s->v.Assign.value);
        for (i = 0; i < n; i++) {
            if (i < n - 1)
                ADDOP(c, DUP_TOP);
            VISIT(c, expr,
                  (expr_ty)asdl_seq_GET(s->v.Assign.targets, i));
        }
        break;
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
    case Raise_kind:
        n = 0;
        if (s->v.Raise.exc) {
            VISIT(c, expr, s->v.Raise.exc);
            n++;
            if (s->v.Raise.cause) {
                VISIT(c, expr, s->v.Raise.cause);
                n++;
            }
        }
        ADDOP_I(c, RAISE_VARARGS, (int)n);
        break;
    case Try_kind:
        return compiler_try(c, s);
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
        return compiler_visit_stmt_expr(c, s->v.Expr.value);
    case Pass_kind:
        break;
    case Break_kind:
        if (!compiler_in_loop(c))
            return compiler_error(c, "'break' outside loop");
        ADDOP(c, BREAK_LOOP);
        break;
    case Continue_kind:
        return compiler_continue(c);
    case With_kind:
        return compiler_with(c, s, 0);
    case AsyncFunctionDef_kind:
        return compiler_function(c, s, 1);
    case AsyncWith_kind:
        return compiler_async_with(c, s, 0);
    case AsyncFor_kind:
        return compiler_async_for(c, s);
    }

    return 1;
}

static int
unaryop(unaryop_ty op)
{
    switch (op) {
    case Invert:
        return UNARY_INVERT;
    case Not:
        return UNARY_NOT;
    case UAdd:
        return UNARY_POSITIVE;
    case USub:
        return UNARY_NEGATIVE;
    default:
        PyErr_Format(PyExc_SystemError,
            "unary op %d should not be possible", op);
        return 0;
    }
}

static int
binop(struct compiler *c, operator_ty op)
{
    switch (op) {
    case Add:
        return BINARY_ADD;
    case Sub:
        return BINARY_SUBTRACT;
    case Mult:
        return BINARY_MULTIPLY;
    case MatMult:
        return BINARY_MATRIX_MULTIPLY;
    case Div:
        return BINARY_TRUE_DIVIDE;
    case Mod:
        return BINARY_MODULO;
    case Pow:
        return BINARY_POWER;
    case LShift:
        return BINARY_LSHIFT;
    case RShift:
        return BINARY_RSHIFT;
    case BitOr:
        return BINARY_OR;
    case BitXor:
        return BINARY_XOR;
    case BitAnd:
        return BINARY_AND;
    case FloorDiv:
        return BINARY_FLOOR_DIVIDE;
    default:
        PyErr_Format(PyExc_SystemError,
            "binary op %d should not be possible", op);
        return 0;
    }
}

static int
cmpop(cmpop_ty op)
{
    switch (op) {
    case Eq:
        return PyCmp_EQ;
    case NotEq:
        return PyCmp_NE;
    case Lt:
        return PyCmp_LT;
    case LtE:
        return PyCmp_LE;
    case Gt:
        return PyCmp_GT;
    case GtE:
        return PyCmp_GE;
    case Is:
        return PyCmp_IS;
    case IsNot:
        return PyCmp_IS_NOT;
    case In:
        return PyCmp_IN;
    case NotIn:
        return PyCmp_NOT_IN;
    default:
        return PyCmp_BAD;
    }
}

static int
inplace_binop(struct compiler *c, operator_ty op)
{
    switch (op) {
    case Add:
        return INPLACE_ADD;
    case Sub:
        return INPLACE_SUBTRACT;
    case Mult:
        return INPLACE_MULTIPLY;
    case MatMult:
        return INPLACE_MATRIX_MULTIPLY;
    case Div:
        return INPLACE_TRUE_DIVIDE;
    case Mod:
        return INPLACE_MODULO;
    case Pow:
        return INPLACE_POWER;
    case LShift:
        return INPLACE_LSHIFT;
    case RShift:
        return INPLACE_RSHIFT;
    case BitOr:
        return INPLACE_OR;
    case BitXor:
        return INPLACE_XOR;
    case BitAnd:
        return INPLACE_AND;
    case FloorDiv:
        return INPLACE_FLOOR_DIVIDE;
    default:
        PyErr_Format(PyExc_SystemError,
            "inplace binary op %d should not be possible", op);
        return 0;
    }
}

static int
compiler_nameop(struct compiler *c, identifier name, expr_context_ty ctx)
{
    int op, scope;
    Py_ssize_t arg;
    enum { OP_FAST, OP_GLOBAL, OP_DEREF, OP_NAME } optype;

    PyObject *dict = c->u->u_names;
    PyObject *mangled;
    /* XXX AugStore isn't used anywhere! */

    assert(!_PyUnicode_EqualToASCIIString(name, "None") &&
           !_PyUnicode_EqualToASCIIString(name, "True") &&
           !_PyUnicode_EqualToASCIIString(name, "False"));

    if (ctx == Load && _PyUnicode_EqualToASCIIString(name, "__debug__")) {
        ADDOP_O(c, LOAD_CONST, c->c_optimize ? Py_False : Py_True, consts);
        return 1;
    }

    mangled = _Py_Mangle(c->u->u_private, name);
    if (!mangled)
        return 0;

    op = 0;
    optype = OP_NAME;
    scope = PyST_GetScope(c->u->u_ste, mangled);
    switch (scope) {
    case FREE:
        dict = c->u->u_freevars;
        optype = OP_DEREF;
        break;
    case CELL:
        dict = c->u->u_cellvars;
        optype = OP_DEREF;
        break;
    case LOCAL:
        if (c->u->u_ste->ste_type == FunctionBlock)
            optype = OP_FAST;
        break;
    case GLOBAL_IMPLICIT:
        if (c->u->u_ste->ste_type == FunctionBlock)
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
            op = (c->u->u_ste->ste_type == ClassBlock) ? LOAD_CLASSDEREF : LOAD_DEREF;
            break;
        case Store: op = STORE_DEREF; break;
        case AugLoad:
        case AugStore:
            break;
        case Del: op = DELETE_DEREF; break;
        case Param:
        default:
            PyErr_SetString(PyExc_SystemError,
                            "param invalid for deref variable");
            return 0;
        }
        break;
    case OP_FAST:
        switch (ctx) {
        case Load: op = LOAD_FAST; break;
        case Store: op = STORE_FAST; break;
        case Del: op = DELETE_FAST; break;
        case AugLoad:
        case AugStore:
            break;
        case Param:
        default:
            PyErr_SetString(PyExc_SystemError,
                            "param invalid for local variable");
            return 0;
        }
        ADDOP_O(c, op, mangled, varnames);
        Py_DECREF(mangled);
        return 1;
    case OP_GLOBAL:
        switch (ctx) {
        case Load: op = LOAD_GLOBAL; break;
        case Store: op = STORE_GLOBAL; break;
        case Del: op = DELETE_GLOBAL; break;
        case AugLoad:
        case AugStore:
            break;
        case Param:
        default:
            PyErr_SetString(PyExc_SystemError,
                            "param invalid for global variable");
            return 0;
        }
        break;
    case OP_NAME:
        switch (ctx) {
        case Load: op = LOAD_NAME; break;
        case Store: op = STORE_NAME; break;
        case Del: op = DELETE_NAME; break;
        case AugLoad:
        case AugStore:
            break;
        case Param:
        default:
            PyErr_SetString(PyExc_SystemError,
                            "param invalid for name variable");
            return 0;
        }
        break;
    }

    assert(op);
    arg = compiler_add_o(c, dict, mangled);
    Py_DECREF(mangled);
    if (arg < 0)
        return 0;
    return compiler_addop_i(c, op, arg);
}

static int
compiler_boolop(struct compiler *c, expr_ty e)
{
    basicblock *end;
    int jumpi;
    Py_ssize_t i, n;
    asdl_seq *s;

    assert(e->kind == BoolOp_kind);
    if (e->v.BoolOp.op == And)
        jumpi = JUMP_IF_FALSE_OR_POP;
    else
        jumpi = JUMP_IF_TRUE_OR_POP;
    end = compiler_new_block(c);
    if (end == NULL)
        return 0;
    s = e->v.BoolOp.values;
    n = asdl_seq_LEN(s) - 1;
    assert(n >= 0);
    for (i = 0; i < n; ++i) {
        VISIT(c, expr, (expr_ty)asdl_seq_GET(s, i));
        ADDOP_JABS(c, jumpi, end);
    }
    VISIT(c, expr, (expr_ty)asdl_seq_GET(s, n));
    compiler_use_next_block(c, end);
    return 1;
}

static int
starunpack_helper(struct compiler *c, asdl_seq *elts,
                  int single_op, int inner_op, int outer_op)
{
    Py_ssize_t n = asdl_seq_LEN(elts);
    Py_ssize_t i, nsubitems = 0, nseen = 0;
    for (i = 0; i < n; i++) {
        expr_ty elt = asdl_seq_GET(elts, i);
        if (elt->kind == Starred_kind) {
            if (nseen) {
                ADDOP_I(c, inner_op, nseen);
                nseen = 0;
                nsubitems++;
            }
            VISIT(c, expr, elt->v.Starred.value);
            nsubitems++;
        }
        else {
            VISIT(c, expr, elt);
            nseen++;
        }
    }
    if (nsubitems) {
        if (nseen) {
            ADDOP_I(c, inner_op, nseen);
            nsubitems++;
        }
        ADDOP_I(c, outer_op, nsubitems);
    }
    else
        ADDOP_I(c, single_op, nseen);
    return 1;
}

static int
assignment_helper(struct compiler *c, asdl_seq *elts)
{
    Py_ssize_t n = asdl_seq_LEN(elts);
    Py_ssize_t i;
    int seen_star = 0;
    for (i = 0; i < n; i++) {
        expr_ty elt = asdl_seq_GET(elts, i);
        if (elt->kind == Starred_kind && !seen_star) {
            if ((i >= (1 << 8)) ||
                (n-i-1 >= (INT_MAX >> 8)))
                return compiler_error(c,
                    "too many expressions in "
                    "star-unpacking assignment");
            ADDOP_I(c, UNPACK_EX, (i + ((n-i-1) << 8)));
            seen_star = 1;
            asdl_seq_SET(elts, i, elt->v.Starred.value);
        }
        else if (elt->kind == Starred_kind) {
            return compiler_error(c,
                "two starred expressions in assignment");
        }
    }
    if (!seen_star) {
        ADDOP_I(c, UNPACK_SEQUENCE, n);
    }
    VISIT_SEQ(c, expr, elts);
    return 1;
}

static int
compiler_list(struct compiler *c, expr_ty e)
{
    asdl_seq *elts = e->v.List.elts;
    if (e->v.List.ctx == Store) {
        return assignment_helper(c, elts);
    }
    else if (e->v.List.ctx == Load) {
        return starunpack_helper(c, elts,
                                 BUILD_LIST, BUILD_TUPLE, BUILD_LIST_UNPACK);
    }
    else
        VISIT_SEQ(c, expr, elts);
    return 1;
}

static int
compiler_tuple(struct compiler *c, expr_ty e)
{
    asdl_seq *elts = e->v.Tuple.elts;
    if (e->v.Tuple.ctx == Store) {
        return assignment_helper(c, elts);
    }
    else if (e->v.Tuple.ctx == Load) {
        return starunpack_helper(c, elts,
                                 BUILD_TUPLE, BUILD_TUPLE, BUILD_TUPLE_UNPACK);
    }
    else
        VISIT_SEQ(c, expr, elts);
    return 1;
}

static int
compiler_set(struct compiler *c, expr_ty e)
{
    return starunpack_helper(c, e->v.Set.elts, BUILD_SET,
                             BUILD_SET, BUILD_SET_UNPACK);
}

static int
are_all_items_const(asdl_seq *seq, Py_ssize_t begin, Py_ssize_t end)
{
    Py_ssize_t i;
    for (i = begin; i < end; i++) {
        expr_ty key = (expr_ty)asdl_seq_GET(seq, i);
        if (key == NULL || !is_const(key))
            return 0;
    }
    return 1;
}

static int
compiler_subdict(struct compiler *c, expr_ty e, Py_ssize_t begin, Py_ssize_t end)
{
    Py_ssize_t i, n = end - begin;
    PyObject *keys, *key;
    if (n > 1 && are_all_items_const(e->v.Dict.keys, begin, end)) {
        for (i = begin; i < end; i++) {
            VISIT(c, expr, (expr_ty)asdl_seq_GET(e->v.Dict.values, i));
        }
        keys = PyTuple_New(n);
        if (keys == NULL) {
            return 0;
        }
        for (i = begin; i < end; i++) {
            key = get_const_value(c, (expr_ty)asdl_seq_GET(e->v.Dict.keys, i));
            Py_INCREF(key);
            PyTuple_SET_ITEM(keys, i - begin, key);
        }
        ADDOP_N(c, LOAD_CONST, keys, consts);
        ADDOP_I(c, BUILD_CONST_KEY_MAP, n);
    }
    else {
        for (i = begin; i < end; i++) {
            VISIT(c, expr, (expr_ty)asdl_seq_GET(e->v.Dict.keys, i));
            VISIT(c, expr, (expr_ty)asdl_seq_GET(e->v.Dict.values, i));
        }
        ADDOP_I(c, BUILD_MAP, n);
    }
    return 1;
}

static int
compiler_dict(struct compiler *c, expr_ty e)
{
    Py_ssize_t i, n, elements;
    int containers;
    int is_unpacking = 0;
    n = asdl_seq_LEN(e->v.Dict.values);
    containers = 0;
    elements = 0;
    for (i = 0; i < n; i++) {
        is_unpacking = (expr_ty)asdl_seq_GET(e->v.Dict.keys, i) == NULL;
        if (elements == 0xFFFF || (elements && is_unpacking)) {
            if (!compiler_subdict(c, e, i - elements, i))
                return 0;
            containers++;
            elements = 0;
        }
        if (is_unpacking) {
            VISIT(c, expr, (expr_ty)asdl_seq_GET(e->v.Dict.values, i));
            containers++;
        }
        else {
            elements++;
        }
    }
    if (elements || containers == 0) {
        if (!compiler_subdict(c, e, n - elements, n))
            return 0;
        containers++;
    }
    /* If there is more than one dict, they need to be merged into a new
     * dict.  If there is one dict and it's an unpacking, then it needs
     * to be copied into a new dict." */
    while (containers > 1 || is_unpacking) {
        int oparg = containers < 255 ? containers : 255;
        ADDOP_I(c, BUILD_MAP_UNPACK, oparg);
        containers -= (oparg - 1);
        is_unpacking = 0;
    }
    return 1;
}

static int
compiler_compare(struct compiler *c, expr_ty e)
{
    Py_ssize_t i, n;
    basicblock *cleanup = NULL;

    /* XXX the logic can be cleaned up for 1 or multiple comparisons */
    VISIT(c, expr, e->v.Compare.left);
    n = asdl_seq_LEN(e->v.Compare.ops);
    assert(n > 0);
    if (n > 1) {
        cleanup = compiler_new_block(c);
        if (cleanup == NULL)
            return 0;
        VISIT(c, expr,
            (expr_ty)asdl_seq_GET(e->v.Compare.comparators, 0));
    }
    for (i = 1; i < n; i++) {
        ADDOP(c, DUP_TOP);
        ADDOP(c, ROT_THREE);
        ADDOP_I(c, COMPARE_OP,
            cmpop((cmpop_ty)(asdl_seq_GET(
                                      e->v.Compare.ops, i - 1))));
        ADDOP_JABS(c, JUMP_IF_FALSE_OR_POP, cleanup);
        NEXT_BLOCK(c);
        if (i < (n - 1))
            VISIT(c, expr,
                (expr_ty)asdl_seq_GET(e->v.Compare.comparators, i));
    }
    VISIT(c, expr, (expr_ty)asdl_seq_GET(e->v.Compare.comparators, n - 1));
    ADDOP_I(c, COMPARE_OP,
           cmpop((cmpop_ty)(asdl_seq_GET(e->v.Compare.ops, n - 1))));
    if (n > 1) {
        basicblock *end = compiler_new_block(c);
        if (end == NULL)
            return 0;
        ADDOP_JREL(c, JUMP_FORWARD, end);
        compiler_use_next_block(c, cleanup);
        ADDOP(c, ROT_TWO);
        ADDOP(c, POP_TOP);
        compiler_use_next_block(c, end);
    }
    return 1;
}

static int
compiler_call(struct compiler *c, expr_ty e)
{
    VISIT(c, expr, e->v.Call.func);
    return compiler_call_helper(c, 0,
                                e->v.Call.args,
                                e->v.Call.keywords);
}

static int
compiler_joined_str(struct compiler *c, expr_ty e)
{
    VISIT_SEQ(c, expr, e->v.JoinedStr.values);
    if (asdl_seq_LEN(e->v.JoinedStr.values) != 1)
        ADDOP_I(c, BUILD_STRING, asdl_seq_LEN(e->v.JoinedStr.values));
    return 1;
}

/* Used to implement f-strings. Format a single value. */
static int
compiler_formatted_value(struct compiler *c, expr_ty e)
{
    /* Our oparg encodes 2 pieces of information: the conversion
       character, and whether or not a format_spec was provided.

       Convert the conversion char to 2 bits:
       None: 000  0x0  FVC_NONE
       !s  : 001  0x1  FVC_STR
       !r  : 010  0x2  FVC_REPR
       !a  : 011  0x3  FVC_ASCII

       next bit is whether or not we have a format spec:
       yes : 100  0x4
       no  : 000  0x0
    */

    int oparg;

    /* Evaluate the expression to be formatted. */
    VISIT(c, expr, e->v.FormattedValue.value);

    switch (e->v.FormattedValue.conversion) {
    case 's': oparg = FVC_STR;   break;
    case 'r': oparg = FVC_REPR;  break;
    case 'a': oparg = FVC_ASCII; break;
    case -1:  oparg = FVC_NONE;  break;
    default:
        PyErr_SetString(PyExc_SystemError,
                        "Unrecognized conversion character");
        return 0;
    }
    if (e->v.FormattedValue.format_spec) {
        /* Evaluate the format spec, and update our opcode arg. */
        VISIT(c, expr, e->v.FormattedValue.format_spec);
        oparg |= FVS_HAVE_SPEC;
    }

    /* And push our opcode and oparg */
    ADDOP_I(c, FORMAT_VALUE, oparg);
    return 1;
}

static int
compiler_subkwargs(struct compiler *c, asdl_seq *keywords, Py_ssize_t begin, Py_ssize_t end)
{
    Py_ssize_t i, n = end - begin;
    keyword_ty kw;
    PyObject *keys, *key;
    assert(n > 0);
    if (n > 1) {
        for (i = begin; i < end; i++) {
            kw = asdl_seq_GET(keywords, i);
            VISIT(c, expr, kw->value);
        }
        keys = PyTuple_New(n);
        if (keys == NULL) {
            return 0;
        }
        for (i = begin; i < end; i++) {
            key = ((keyword_ty) asdl_seq_GET(keywords, i))->arg;
            Py_INCREF(key);
            PyTuple_SET_ITEM(keys, i - begin, key);
        }
        ADDOP_N(c, LOAD_CONST, keys, consts);
        ADDOP_I(c, BUILD_CONST_KEY_MAP, n);
    }
    else {
        /* a for loop only executes once */
        for (i = begin; i < end; i++) {
            kw = asdl_seq_GET(keywords, i);
            ADDOP_O(c, LOAD_CONST, kw->arg, consts);
            VISIT(c, expr, kw->value);
        }
        ADDOP_I(c, BUILD_MAP, n);
    }
    return 1;
}

/* shared code between compiler_call and compiler_class */
static int
compiler_call_helper(struct compiler *c,
                     int n, /* Args already pushed */
                     asdl_seq *args,
                     asdl_seq *keywords)
{
    Py_ssize_t i, nseen, nelts, nkwelts;
    int mustdictunpack = 0;

    /* the number of tuples and dictionaries on the stack */
    Py_ssize_t nsubargs = 0, nsubkwargs = 0;

    nelts = asdl_seq_LEN(args);
    nkwelts = asdl_seq_LEN(keywords);

    for (i = 0; i < nkwelts; i++) {
        keyword_ty kw = asdl_seq_GET(keywords, i);
        if (kw->arg == NULL) {
            mustdictunpack = 1;
            break;
        }
    }

    nseen = n;  /* the number of positional arguments on the stack */
    for (i = 0; i < nelts; i++) {
        expr_ty elt = asdl_seq_GET(args, i);
        if (elt->kind == Starred_kind) {
            /* A star-arg. If we've seen positional arguments,
               pack the positional arguments into a tuple. */
            if (nseen) {
                ADDOP_I(c, BUILD_TUPLE, nseen);
                nseen = 0;
                nsubargs++;
            }
            VISIT(c, expr, elt->v.Starred.value);
            nsubargs++;
        }
        else {
            VISIT(c, expr, elt);
            nseen++;
        }
    }

    /* Same dance again for keyword arguments */
    if (nsubargs || mustdictunpack) {
        if (nseen) {
            /* Pack up any trailing positional arguments. */
            ADDOP_I(c, BUILD_TUPLE, nseen);
            nsubargs++;
        }
        if (nsubargs > 1) {
            /* If we ended up with more than one stararg, we need
               to concatenate them into a single sequence. */
            ADDOP_I(c, BUILD_TUPLE_UNPACK_WITH_CALL, nsubargs);
        }
        else if (nsubargs == 0) {
            ADDOP_I(c, BUILD_TUPLE, 0);
        }
        nseen = 0;  /* the number of keyword arguments on the stack following */
        for (i = 0; i < nkwelts; i++) {
            keyword_ty kw = asdl_seq_GET(keywords, i);
            if (kw->arg == NULL) {
                /* A keyword argument unpacking. */
                if (nseen) {
                    if (!compiler_subkwargs(c, keywords, i - nseen, i))
                        return 0;
                    nsubkwargs++;
                    nseen = 0;
                }
                VISIT(c, expr, kw->value);
                nsubkwargs++;
            }
            else {
                nseen++;
            }
        }
        if (nseen) {
            /* Pack up any trailing keyword arguments. */
            if (!compiler_subkwargs(c, keywords, nkwelts - nseen, nkwelts))
                return 0;
            nsubkwargs++;
        }
        if (nsubkwargs > 1) {
            /* Pack it all up */
            ADDOP_I(c, BUILD_MAP_UNPACK_WITH_CALL, nsubkwargs);
        }
        ADDOP_I(c, CALL_FUNCTION_EX, nsubkwargs > 0);
        return 1;
    }
    else if (nkwelts) {
        PyObject *names;
        VISIT_SEQ(c, keyword, keywords);
        names = PyTuple_New(nkwelts);
        if (names == NULL) {
            return 0;
        }
        for (i = 0; i < nkwelts; i++) {
            keyword_ty kw = asdl_seq_GET(keywords, i);
            Py_INCREF(kw->arg);
            PyTuple_SET_ITEM(names, i, kw->arg);
        }
        ADDOP_N(c, LOAD_CONST, names, consts);
        ADDOP_I(c, CALL_FUNCTION_KW, n + nelts + nkwelts);
        return 1;
    }
    else {
        ADDOP_I(c, CALL_FUNCTION, n + nelts);
        return 1;
    }
}


/* List and set comprehensions and generator expressions work by creating a
  nested function to perform the actual iteration. This means that the
  iteration variables don't leak into the current scope.
  The defined function is called immediately following its definition, with the
  result of that call being the result of the expression.
  The LC/SC version returns the populated container, while the GE version is
  flagged in symtable.c as a generator, so it returns the generator object
  when the function is called.
  This code *knows* that the loop cannot contain break, continue, or return,
  so it cheats and skips the SETUP_LOOP/POP_BLOCK steps used in normal loops.

  Possible cleanups:
    - iterate over the generator sequence instead of using recursion
*/


static int
compiler_comprehension_generator(struct compiler *c,
                                 asdl_seq *generators, int gen_index,
                                 expr_ty elt, expr_ty val, int type)
{
    comprehension_ty gen;
    gen = (comprehension_ty)asdl_seq_GET(generators, gen_index);
    if (gen->is_async) {
        return compiler_async_comprehension_generator(
            c, generators, gen_index, elt, val, type);
    } else {
        return compiler_sync_comprehension_generator(
            c, generators, gen_index, elt, val, type);
    }
}

static int
compiler_sync_comprehension_generator(struct compiler *c,
                                      asdl_seq *generators, int gen_index,
                                      expr_ty elt, expr_ty val, int type)
{
    /* generate code for the iterator, then each of the ifs,
       and then write to the element */

    comprehension_ty gen;
    basicblock *start, *anchor, *skip, *if_cleanup;
    Py_ssize_t i, n;

    start = compiler_new_block(c);
    skip = compiler_new_block(c);
    if_cleanup = compiler_new_block(c);
    anchor = compiler_new_block(c);

    if (start == NULL || skip == NULL || if_cleanup == NULL ||
        anchor == NULL)
        return 0;

    gen = (comprehension_ty)asdl_seq_GET(generators, gen_index);

    if (gen_index == 0) {
        /* Receive outermost iter as an implicit argument */
        c->u->u_argcount = 1;
        ADDOP_I(c, LOAD_FAST, 0);
    }
    else {
        /* Sub-iter - calculate on the fly */
        VISIT(c, expr, gen->iter);
        ADDOP(c, GET_ITER);
    }
    compiler_use_next_block(c, start);
    ADDOP_JREL(c, FOR_ITER, anchor);
    NEXT_BLOCK(c);
    VISIT(c, expr, gen->target);

    /* XXX this needs to be cleaned up...a lot! */
    n = asdl_seq_LEN(gen->ifs);
    for (i = 0; i < n; i++) {
        expr_ty e = (expr_ty)asdl_seq_GET(gen->ifs, i);
        VISIT(c, expr, e);
        ADDOP_JABS(c, POP_JUMP_IF_FALSE, if_cleanup);
        NEXT_BLOCK(c);
    }

    if (++gen_index < asdl_seq_LEN(generators))
        if (!compiler_comprehension_generator(c,
                                              generators, gen_index,
                                              elt, val, type))
        return 0;

    /* only append after the last for generator */
    if (gen_index >= asdl_seq_LEN(generators)) {
        /* comprehension specific code */
        switch (type) {
        case COMP_GENEXP:
            VISIT(c, expr, elt);
            ADDOP(c, YIELD_VALUE);
            ADDOP(c, POP_TOP);
            break;
        case COMP_LISTCOMP:
            VISIT(c, expr, elt);
            ADDOP_I(c, LIST_APPEND, gen_index + 1);
            break;
        case COMP_SETCOMP:
            VISIT(c, expr, elt);
            ADDOP_I(c, SET_ADD, gen_index + 1);
            break;
        case COMP_DICTCOMP:
            /* With 'd[k] = v', v is evaluated before k, so we do
               the same. */
            VISIT(c, expr, val);
            VISIT(c, expr, elt);
            ADDOP_I(c, MAP_ADD, gen_index + 1);
            break;
        default:
            return 0;
        }

        compiler_use_next_block(c, skip);
    }
    compiler_use_next_block(c, if_cleanup);
    ADDOP_JABS(c, JUMP_ABSOLUTE, start);
    compiler_use_next_block(c, anchor);

    return 1;
}

static int
compiler_async_comprehension_generator(struct compiler *c,
                                      asdl_seq *generators, int gen_index,
                                      expr_ty elt, expr_ty val, int type)
{
    _Py_IDENTIFIER(StopAsyncIteration);

    comprehension_ty gen;
    basicblock *anchor, *if_cleanup, *try,
               *after_try, *except, *try_cleanup;
    Py_ssize_t i, n;

    PyObject *stop_aiter_error = _PyUnicode_FromId(&PyId_StopAsyncIteration);
    if (stop_aiter_error == NULL) {
        return 0;
    }

    try = compiler_new_block(c);
    after_try = compiler_new_block(c);
    try_cleanup = compiler_new_block(c);
    except = compiler_new_block(c);
    if_cleanup = compiler_new_block(c);
    anchor = compiler_new_block(c);

    if (if_cleanup == NULL || anchor == NULL ||
            try == NULL || after_try == NULL ||
            except == NULL || try_cleanup == NULL) {
        return 0;
    }

    gen = (comprehension_ty)asdl_seq_GET(generators, gen_index);

    if (gen_index == 0) {
        /* Receive outermost iter as an implicit argument */
        c->u->u_argcount = 1;
        ADDOP_I(c, LOAD_FAST, 0);
    }
    else {
        /* Sub-iter - calculate on the fly */
        VISIT(c, expr, gen->iter);
        ADDOP(c, GET_AITER);
        ADDOP_O(c, LOAD_CONST, Py_None, consts);
        ADDOP(c, YIELD_FROM);
    }

    compiler_use_next_block(c, try);


    ADDOP_JREL(c, SETUP_EXCEPT, except);
    if (!compiler_push_fblock(c, EXCEPT, try))
        return 0;

    ADDOP(c, GET_ANEXT);
    ADDOP_O(c, LOAD_CONST, Py_None, consts);
    ADDOP(c, YIELD_FROM);
    VISIT(c, expr, gen->target);
    ADDOP(c, POP_BLOCK);
    compiler_pop_fblock(c, EXCEPT, try);
    ADDOP_JREL(c, JUMP_FORWARD, after_try);


    compiler_use_next_block(c, except);
    ADDOP(c, DUP_TOP);
    ADDOP_O(c, LOAD_GLOBAL, stop_aiter_error, names);
    ADDOP_I(c, COMPARE_OP, PyCmp_EXC_MATCH);
    ADDOP_JABS(c, POP_JUMP_IF_FALSE, try_cleanup);

    ADDOP(c, POP_TOP);
    ADDOP(c, POP_TOP);
    ADDOP(c, POP_TOP);
    ADDOP(c, POP_EXCEPT); /* for SETUP_EXCEPT */
    ADDOP_JABS(c, JUMP_ABSOLUTE, anchor);


    compiler_use_next_block(c, try_cleanup);
    ADDOP(c, END_FINALLY);

    compiler_use_next_block(c, after_try);

    n = asdl_seq_LEN(gen->ifs);
    for (i = 0; i < n; i++) {
        expr_ty e = (expr_ty)asdl_seq_GET(gen->ifs, i);
        VISIT(c, expr, e);
        ADDOP_JABS(c, POP_JUMP_IF_FALSE, if_cleanup);
        NEXT_BLOCK(c);
    }

    if (++gen_index < asdl_seq_LEN(generators))
        if (!compiler_comprehension_generator(c,
                                              generators, gen_index,
                                              elt, val, type))
        return 0;

    /* only append after the last for generator */
    if (gen_index >= asdl_seq_LEN(generators)) {
        /* comprehension specific code */
        switch (type) {
        case COMP_GENEXP:
            VISIT(c, expr, elt);
            ADDOP(c, YIELD_VALUE);
            ADDOP(c, POP_TOP);
            break;
        case COMP_LISTCOMP:
            VISIT(c, expr, elt);
            ADDOP_I(c, LIST_APPEND, gen_index + 1);
            break;
        case COMP_SETCOMP:
            VISIT(c, expr, elt);
            ADDOP_I(c, SET_ADD, gen_index + 1);
            break;
        case COMP_DICTCOMP:
            /* With 'd[k] = v', v is evaluated before k, so we do
               the same. */
            VISIT(c, expr, val);
            VISIT(c, expr, elt);
            ADDOP_I(c, MAP_ADD, gen_index + 1);
            break;
        default:
            return 0;
        }
    }
    compiler_use_next_block(c, if_cleanup);
    ADDOP_JABS(c, JUMP_ABSOLUTE, try);
    compiler_use_next_block(c, anchor);
    ADDOP(c, POP_TOP);

    return 1;
}

static int
compiler_comprehension(struct compiler *c, expr_ty e, int type,
                       identifier name, asdl_seq *generators, expr_ty elt,
                       expr_ty val)
{
    PyCodeObject *co = NULL;
    comprehension_ty outermost;
    PyObject *qualname = NULL;
    int is_async_function = c->u->u_ste->ste_coroutine;
    int is_async_generator = 0;

    outermost = (comprehension_ty) asdl_seq_GET(generators, 0);

    if (!compiler_enter_scope(c, name, COMPILER_SCOPE_COMPREHENSION,
                              (void *)e, e->lineno))
    {
        goto error;
    }

    is_async_generator = c->u->u_ste->ste_coroutine;

    if (is_async_generator && !is_async_function) {
        if (e->lineno > c->u->u_lineno) {
            c->u->u_lineno = e->lineno;
            c->u->u_lineno_set = 0;
        }
        compiler_error(c, "asynchronous comprehension outside of "
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

        ADDOP_I(c, op, 0);
    }

    if (!compiler_comprehension_generator(c, generators, 0, elt,
                                          val, type))
        goto error_in_scope;

    if (type != COMP_GENEXP) {
        ADDOP(c, RETURN_VALUE);
    }

    co = assemble(c, 1);
    qualname = c->u->u_qualname;
    Py_INCREF(qualname);
    compiler_exit_scope(c);
    if (co == NULL)
        goto error;

    if (!compiler_make_closure(c, co, 0, qualname))
        goto error;
    Py_DECREF(qualname);
    Py_DECREF(co);

    VISIT(c, expr, outermost->iter);

    if (outermost->is_async) {
        ADDOP(c, GET_AITER);
        ADDOP_O(c, LOAD_CONST, Py_None, consts);
        ADDOP(c, YIELD_FROM);
    } else {
        ADDOP(c, GET_ITER);
    }

    ADDOP_I(c, CALL_FUNCTION, 1);

    if (is_async_generator && type != COMP_GENEXP) {
        ADDOP(c, GET_AWAITABLE);
        ADDOP_O(c, LOAD_CONST, Py_None, consts);
        ADDOP(c, YIELD_FROM);
    }

    return 1;
error_in_scope:
    compiler_exit_scope(c);
error:
    Py_XDECREF(qualname);
    Py_XDECREF(co);
    return 0;
}

static int
compiler_genexp(struct compiler *c, expr_ty e)
{
    static identifier name;
    if (!name) {
        name = PyUnicode_FromString("<genexpr>");
        if (!name)
            return 0;
    }
    assert(e->kind == GeneratorExp_kind);
    return compiler_comprehension(c, e, COMP_GENEXP, name,
                                  e->v.GeneratorExp.generators,
                                  e->v.GeneratorExp.elt, NULL);
}

static int
compiler_listcomp(struct compiler *c, expr_ty e)
{
    static identifier name;
    if (!name) {
        name = PyUnicode_FromString("<listcomp>");
        if (!name)
            return 0;
    }
    assert(e->kind == ListComp_kind);
    return compiler_comprehension(c, e, COMP_LISTCOMP, name,
                                  e->v.ListComp.generators,
                                  e->v.ListComp.elt, NULL);
}

static int
compiler_setcomp(struct compiler *c, expr_ty e)
{
    static identifier name;
    if (!name) {
        name = PyUnicode_FromString("<setcomp>");
        if (!name)
            return 0;
    }
    assert(e->kind == SetComp_kind);
    return compiler_comprehension(c, e, COMP_SETCOMP, name,
                                  e->v.SetComp.generators,
                                  e->v.SetComp.elt, NULL);
}


static int
compiler_dictcomp(struct compiler *c, expr_ty e)
{
    static identifier name;
    if (!name) {
        name = PyUnicode_FromString("<dictcomp>");
        if (!name)
            return 0;
    }
    assert(e->kind == DictComp_kind);
    return compiler_comprehension(c, e, COMP_DICTCOMP, name,
                                  e->v.DictComp.generators,
                                  e->v.DictComp.key, e->v.DictComp.value);
}


static int
compiler_visit_keyword(struct compiler *c, keyword_ty k)
{
    VISIT(c, expr, k->value);
    return 1;
}

/* Test whether expression is constant.  For constants, report
   whether they are true or false.

   Return values: 1 for true, 0 for false, -1 for non-constant.
 */

static int
expr_constant(struct compiler *c, expr_ty e)
{
    if (is_const(e)) {
        return PyObject_IsTrue(get_const_value(c, e));
    }
    return -1;
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
    basicblock *block, *finally;
    withitem_ty item = asdl_seq_GET(s->v.AsyncWith.items, pos);

    assert(s->kind == AsyncWith_kind);

    block = compiler_new_block(c);
    finally = compiler_new_block(c);
    if (!block || !finally)
        return 0;

    /* Evaluate EXPR */
    VISIT(c, expr, item->context_expr);

    ADDOP(c, BEFORE_ASYNC_WITH);
    ADDOP(c, GET_AWAITABLE);
    ADDOP_O(c, LOAD_CONST, Py_None, consts);
    ADDOP(c, YIELD_FROM);

    ADDOP_JREL(c, SETUP_ASYNC_WITH, finally);

    /* SETUP_ASYNC_WITH pushes a finally block. */
    compiler_use_next_block(c, block);
    if (!compiler_push_fblock(c, FINALLY_TRY, block)) {
        return 0;
    }

    if (item->optional_vars) {
        VISIT(c, expr, item->optional_vars);
    }
    else {
    /* Discard result from context.__aenter__() */
        ADDOP(c, POP_TOP);
    }

    pos++;
    if (pos == asdl_seq_LEN(s->v.AsyncWith.items))
        /* BLOCK code */
        VISIT_SEQ(c, stmt, s->v.AsyncWith.body)
    else if (!compiler_async_with(c, s, pos))
            return 0;

    /* End of try block; start the finally block */
    ADDOP(c, POP_BLOCK);
    compiler_pop_fblock(c, FINALLY_TRY, block);

    ADDOP_O(c, LOAD_CONST, Py_None, consts);
    compiler_use_next_block(c, finally);
    if (!compiler_push_fblock(c, FINALLY_END, finally))
        return 0;

    /* Finally block starts; context.__exit__ is on the stack under
       the exception or return information. Just issue our magic
       opcode. */
    ADDOP(c, WITH_CLEANUP_START);

    ADDOP(c, GET_AWAITABLE);
    ADDOP_O(c, LOAD_CONST, Py_None, consts);
    ADDOP(c, YIELD_FROM);

    ADDOP(c, WITH_CLEANUP_FINISH);

    /* Finally block ends. */
    ADDOP(c, END_FINALLY);
    compiler_pop_fblock(c, FINALLY_END, finally);
    return 1;
}


/*
   Implements the with statement from PEP 343.

   The semantics outlined in that PEP are as follows:

   with EXPR as VAR:
       BLOCK

   It is implemented roughly as:

   context = EXPR
   exit = context.__exit__  # not calling it
   value = context.__enter__()
   try:
       VAR = value  # if VAR present in the syntax
       BLOCK
   finally:
       if an exception was raised:
           exc = copy of (exception, instance, traceback)
       else:
           exc = (None, None, None)
       exit(*exc)
 */
static int
compiler_with(struct compiler *c, stmt_ty s, int pos)
{
    basicblock *block, *finally;
    withitem_ty item = asdl_seq_GET(s->v.With.items, pos);

    assert(s->kind == With_kind);

    block = compiler_new_block(c);
    finally = compiler_new_block(c);
    if (!block || !finally)
        return 0;

    /* Evaluate EXPR */
    VISIT(c, expr, item->context_expr);
    ADDOP_JREL(c, SETUP_WITH, finally);

    /* SETUP_WITH pushes a finally block. */
    compiler_use_next_block(c, block);
    if (!compiler_push_fblock(c, FINALLY_TRY, block)) {
        return 0;
    }

    if (item->optional_vars) {
        VISIT(c, expr, item->optional_vars);
    }
    else {
    /* Discard result from context.__enter__() */
        ADDOP(c, POP_TOP);
    }

    pos++;
    if (pos == asdl_seq_LEN(s->v.With.items))
        /* BLOCK code */
        VISIT_SEQ(c, stmt, s->v.With.body)
    else if (!compiler_with(c, s, pos))
            return 0;

    /* End of try block; start the finally block */
    ADDOP(c, POP_BLOCK);
    compiler_pop_fblock(c, FINALLY_TRY, block);

    ADDOP_O(c, LOAD_CONST, Py_None, consts);
    compiler_use_next_block(c, finally);
    if (!compiler_push_fblock(c, FINALLY_END, finally))
        return 0;

    /* Finally block starts; context.__exit__ is on the stack under
       the exception or return information. Just issue our magic
       opcode. */
    ADDOP(c, WITH_CLEANUP_START);
    ADDOP(c, WITH_CLEANUP_FINISH);

    /* Finally block ends. */
    ADDOP(c, END_FINALLY);
    compiler_pop_fblock(c, FINALLY_END, finally);
    return 1;
}

static int
compiler_visit_expr(struct compiler *c, expr_ty e)
{
    /* If expr e has a different line number than the last expr/stmt,
       set a new line number for the next instruction.
    */
    if (e->lineno > c->u->u_lineno) {
        c->u->u_lineno = e->lineno;
        c->u->u_lineno_set = 0;
    }
    /* Updating the column offset is always harmless. */
    c->u->u_col_offset = e->col_offset;
    switch (e->kind) {
    case BoolOp_kind:
        return compiler_boolop(c, e);
    case BinOp_kind:
        VISIT(c, expr, e->v.BinOp.left);
        VISIT(c, expr, e->v.BinOp.right);
        ADDOP(c, binop(c, e->v.BinOp.op));
        break;
    case UnaryOp_kind:
        VISIT(c, expr, e->v.UnaryOp.operand);
        ADDOP(c, unaryop(e->v.UnaryOp.op));
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
        if (c->u->u_ste->ste_type != FunctionBlock)
            return compiler_error(c, "'yield' outside function");
        if (e->v.Yield.value) {
            VISIT(c, expr, e->v.Yield.value);
        }
        else {
            ADDOP_O(c, LOAD_CONST, Py_None, consts);
        }
        ADDOP(c, YIELD_VALUE);
        break;
    case YieldFrom_kind:
        if (c->u->u_ste->ste_type != FunctionBlock)
            return compiler_error(c, "'yield' outside function");

        if (c->u->u_scope_type == COMPILER_SCOPE_ASYNC_FUNCTION)
            return compiler_error(c, "'yield from' inside async function");

        VISIT(c, expr, e->v.YieldFrom.value);
        ADDOP(c, GET_YIELD_FROM_ITER);
        ADDOP_O(c, LOAD_CONST, Py_None, consts);
        ADDOP(c, YIELD_FROM);
        break;
    case Await_kind:
        if (c->u->u_ste->ste_type != FunctionBlock)
            return compiler_error(c, "'await' outside function");

        if (c->u->u_scope_type != COMPILER_SCOPE_ASYNC_FUNCTION &&
                c->u->u_scope_type != COMPILER_SCOPE_COMPREHENSION)
            return compiler_error(c, "'await' outside async function");

        VISIT(c, expr, e->v.Await.value);
        ADDOP(c, GET_AWAITABLE);
        ADDOP_O(c, LOAD_CONST, Py_None, consts);
        ADDOP(c, YIELD_FROM);
        break;
    case Compare_kind:
        return compiler_compare(c, e);
    case Call_kind:
        return compiler_call(c, e);
    case Constant_kind:
        ADDOP_O(c, LOAD_CONST, e->v.Constant.value, consts);
        break;
    case Num_kind:
        ADDOP_O(c, LOAD_CONST, e->v.Num.n, consts);
        break;
    case Str_kind:
        ADDOP_O(c, LOAD_CONST, e->v.Str.s, consts);
        break;
    case JoinedStr_kind:
        return compiler_joined_str(c, e);
    case FormattedValue_kind:
        return compiler_formatted_value(c, e);
    case Bytes_kind:
        ADDOP_O(c, LOAD_CONST, e->v.Bytes.s, consts);
        break;
    case Ellipsis_kind:
        ADDOP_O(c, LOAD_CONST, Py_Ellipsis, consts);
        break;
    case NameConstant_kind:
        ADDOP_O(c, LOAD_CONST, e->v.NameConstant.value, consts);
        break;
    /* The following exprs can be assignment targets. */
    case Attribute_kind:
        if (e->v.Attribute.ctx != AugStore)
            VISIT(c, expr, e->v.Attribute.value);
        switch (e->v.Attribute.ctx) {
        case AugLoad:
            ADDOP(c, DUP_TOP);
            /* Fall through */
        case Load:
            ADDOP_NAME(c, LOAD_ATTR, e->v.Attribute.attr, names);
            break;
        case AugStore:
            ADDOP(c, ROT_TWO);
            /* Fall through */
        case Store:
            ADDOP_NAME(c, STORE_ATTR, e->v.Attribute.attr, names);
            break;
        case Del:
            ADDOP_NAME(c, DELETE_ATTR, e->v.Attribute.attr, names);
            break;
        case Param:
        default:
            PyErr_SetString(PyExc_SystemError,
                            "param invalid in attribute expression");
            return 0;
        }
        break;
    case Subscript_kind:
        switch (e->v.Subscript.ctx) {
        case AugLoad:
            VISIT(c, expr, e->v.Subscript.value);
            VISIT_SLICE(c, e->v.Subscript.slice, AugLoad);
            break;
        case Load:
            VISIT(c, expr, e->v.Subscript.value);
            VISIT_SLICE(c, e->v.Subscript.slice, Load);
            break;
        case AugStore:
            VISIT_SLICE(c, e->v.Subscript.slice, AugStore);
            break;
        case Store:
            VISIT(c, expr, e->v.Subscript.value);
            VISIT_SLICE(c, e->v.Subscript.slice, Store);
            break;
        case Del:
            VISIT(c, expr, e->v.Subscript.value);
            VISIT_SLICE(c, e->v.Subscript.slice, Del);
            break;
        case Param:
        default:
            PyErr_SetString(PyExc_SystemError,
                "param invalid in subscript expression");
            return 0;
        }
        break;
    case Starred_kind:
        switch (e->v.Starred.ctx) {
        case Store:
            /* In all legitimate cases, the Starred node was already replaced
             * by compiler_list/compiler_tuple. XXX: is that okay? */
            return compiler_error(c,
                "starred assignment target must be in a list or tuple");
        default:
            return compiler_error(c,
                "can't use starred expression here");
        }
        break;
    case Name_kind:
        return compiler_nameop(c, e->v.Name.id, e->v.Name.ctx);
    /* child nodes of List and Tuple will have expr_context set */
    case List_kind:
        return compiler_list(c, e);
    case Tuple_kind:
        return compiler_tuple(c, e);
    }
    return 1;
}

static int
compiler_augassign(struct compiler *c, stmt_ty s)
{
    expr_ty e = s->v.AugAssign.target;
    expr_ty auge;

    assert(s->kind == AugAssign_kind);

    switch (e->kind) {
    case Attribute_kind:
        auge = Attribute(e->v.Attribute.value, e->v.Attribute.attr,
                         AugLoad, e->lineno, e->col_offset, c->c_arena);
        if (auge == NULL)
            return 0;
        VISIT(c, expr, auge);
        VISIT(c, expr, s->v.AugAssign.value);
        ADDOP(c, inplace_binop(c, s->v.AugAssign.op));
        auge->v.Attribute.ctx = AugStore;
        VISIT(c, expr, auge);
        break;
    case Subscript_kind:
        auge = Subscript(e->v.Subscript.value, e->v.Subscript.slice,
                         AugLoad, e->lineno, e->col_offset, c->c_arena);
        if (auge == NULL)
            return 0;
        VISIT(c, expr, auge);
        VISIT(c, expr, s->v.AugAssign.value);
        ADDOP(c, inplace_binop(c, s->v.AugAssign.op));
        auge->v.Subscript.ctx = AugStore;
        VISIT(c, expr, auge);
        break;
    case Name_kind:
        if (!compiler_nameop(c, e->v.Name.id, Load))
            return 0;
        VISIT(c, expr, s->v.AugAssign.value);
        ADDOP(c, inplace_binop(c, s->v.AugAssign.op));
        return compiler_nameop(c, e->v.Name.id, Store);
    default:
        PyErr_Format(PyExc_SystemError,
            "invalid node type (%d) for augmented assignment",
            e->kind);
        return 0;
    }
    return 1;
}

static int
check_ann_expr(struct compiler *c, expr_ty e)
{
    VISIT(c, expr, e);
    ADDOP(c, POP_TOP);
    return 1;
}

static int
check_annotation(struct compiler *c, stmt_ty s)
{
    /* Annotations are only evaluated in a module or class. */
    if (c->u->u_scope_type == COMPILER_SCOPE_MODULE ||
        c->u->u_scope_type == COMPILER_SCOPE_CLASS) {
        return check_ann_expr(c, s->v.AnnAssign.annotation);
    }
    return 1;
}

static int
check_ann_slice(struct compiler *c, slice_ty sl)
{
    switch(sl->kind) {
    case Index_kind:
        return check_ann_expr(c, sl->v.Index.value);
    case Slice_kind:
        if (sl->v.Slice.lower && !check_ann_expr(c, sl->v.Slice.lower)) {
            return 0;
        }
        if (sl->v.Slice.upper && !check_ann_expr(c, sl->v.Slice.upper)) {
            return 0;
        }
        if (sl->v.Slice.step && !check_ann_expr(c, sl->v.Slice.step)) {
            return 0;
        }
        break;
    default:
        PyErr_SetString(PyExc_SystemError,
                        "unexpected slice kind");
        return 0;
    }
    return 1;
}

static int
check_ann_subscr(struct compiler *c, slice_ty sl)
{
    /* We check that everything in a subscript is defined at runtime. */
    Py_ssize_t i, n;

    switch (sl->kind) {
    case Index_kind:
    case Slice_kind:
        if (!check_ann_slice(c, sl)) {
            return 0;
        }
        break;
    case ExtSlice_kind:
        n = asdl_seq_LEN(sl->v.ExtSlice.dims);
        for (i = 0; i < n; i++) {
            slice_ty subsl = (slice_ty)asdl_seq_GET(sl->v.ExtSlice.dims, i);
            switch (subsl->kind) {
            case Index_kind:
            case Slice_kind:
                if (!check_ann_slice(c, subsl)) {
                    return 0;
                }
                break;
            case ExtSlice_kind:
            default:
                PyErr_SetString(PyExc_SystemError,
                                "extended slice invalid in nested slice");
                return 0;
            }
        }
        break;
    default:
        PyErr_Format(PyExc_SystemError,
                     "invalid subscript kind %d", sl->kind);
        return 0;
    }
    return 1;
}

static int
compiler_annassign(struct compiler *c, stmt_ty s)
{
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
        /* If we have a simple name in a module or class, store annotation. */
        if (s->v.AnnAssign.simple &&
            (c->u->u_scope_type == COMPILER_SCOPE_MODULE ||
             c->u->u_scope_type == COMPILER_SCOPE_CLASS)) {
            mangled = _Py_Mangle(c->u->u_private, targ->v.Name.id);
            if (!mangled) {
                return 0;
            }
            VISIT(c, expr, s->v.AnnAssign.annotation);
            /* ADDOP_N decrefs its argument */
            ADDOP_N(c, STORE_ANNOTATION, mangled, names);
        }
        break;
    case Attribute_kind:
        if (!s->v.AnnAssign.value &&
            !check_ann_expr(c, targ->v.Attribute.value)) {
            return 0;
        }
        break;
    case Subscript_kind:
        if (!s->v.AnnAssign.value &&
            (!check_ann_expr(c, targ->v.Subscript.value) ||
             !check_ann_subscr(c, targ->v.Subscript.slice))) {
                return 0;
        }
        break;
    default:
        PyErr_Format(PyExc_SystemError,
                     "invalid node type (%d) for annotated assignment",
                     targ->kind);
            return 0;
    }
    /* Annotation is evaluated last. */
    if (!s->v.AnnAssign.simple && !check_annotation(c, s)) {
        return 0;
    }
    return 1;
}

static int
compiler_push_fblock(struct compiler *c, enum fblocktype t, basicblock *b)
{
    struct fblockinfo *f;
    if (c->u->u_nfblocks >= CO_MAXBLOCKS) {
        PyErr_SetString(PyExc_SyntaxError,
                        "too many statically nested blocks");
        return 0;
    }
    f = &c->u->u_fblock[c->u->u_nfblocks++];
    f->fb_type = t;
    f->fb_block = b;
    return 1;
}

static void
compiler_pop_fblock(struct compiler *c, enum fblocktype t, basicblock *b)
{
    struct compiler_unit *u = c->u;
    assert(u->u_nfblocks > 0);
    u->u_nfblocks--;
    assert(u->u_fblock[u->u_nfblocks].fb_type == t);
    assert(u->u_fblock[u->u_nfblocks].fb_block == b);
}

static int
compiler_in_loop(struct compiler *c) {
    int i;
    struct compiler_unit *u = c->u;
    for (i = 0; i < u->u_nfblocks; ++i) {
        if (u->u_fblock[i].fb_type == LOOP)
            return 1;
    }
    return 0;
}
/* Raises a SyntaxError and returns 0.
   If something goes wrong, a different exception may be raised.
*/

static int
compiler_error(struct compiler *c, const char *errstr)
{
    PyObject *loc;
    PyObject *u = NULL, *v = NULL;

    loc = PyErr_ProgramTextObject(c->c_filename, c->u->u_lineno);
    if (!loc) {
        Py_INCREF(Py_None);
        loc = Py_None;
    }
    u = Py_BuildValue("(OiiO)", c->c_filename, c->u->u_lineno,
                      c->u->u_col_offset, loc);
    if (!u)
        goto exit;
    v = Py_BuildValue("(zO)", errstr, u);
    if (!v)
        goto exit;
    PyErr_SetObject(PyExc_SyntaxError, v);
 exit:
    Py_DECREF(loc);
    Py_XDECREF(u);
    Py_XDECREF(v);
    return 0;
}

static int
compiler_handle_subscr(struct compiler *c, const char *kind,
                       expr_context_ty ctx)
{
    int op = 0;

    /* XXX this code is duplicated */
    switch (ctx) {
        case AugLoad: /* fall through to Load */
        case Load:    op = BINARY_SUBSCR; break;
        case AugStore:/* fall through to Store */
        case Store:   op = STORE_SUBSCR; break;
        case Del:     op = DELETE_SUBSCR; break;
        case Param:
            PyErr_Format(PyExc_SystemError,
                         "invalid %s kind %d in subscript\n",
                         kind, ctx);
            return 0;
    }
    if (ctx == AugLoad) {
        ADDOP(c, DUP_TOP_TWO);
    }
    else if (ctx == AugStore) {
        ADDOP(c, ROT_THREE);
    }
    ADDOP(c, op);
    return 1;
}

static int
compiler_slice(struct compiler *c, slice_ty s, expr_context_ty ctx)
{
    int n = 2;
    assert(s->kind == Slice_kind);

    /* only handles the cases where BUILD_SLICE is emitted */
    if (s->v.Slice.lower) {
        VISIT(c, expr, s->v.Slice.lower);
    }
    else {
        ADDOP_O(c, LOAD_CONST, Py_None, consts);
    }

    if (s->v.Slice.upper) {
        VISIT(c, expr, s->v.Slice.upper);
    }
    else {
        ADDOP_O(c, LOAD_CONST, Py_None, consts);
    }

    if (s->v.Slice.step) {
        n++;
        VISIT(c, expr, s->v.Slice.step);
    }
    ADDOP_I(c, BUILD_SLICE, n);
    return 1;
}

static int
compiler_visit_nested_slice(struct compiler *c, slice_ty s,
                            expr_context_ty ctx)
{
    switch (s->kind) {
    case Slice_kind:
        return compiler_slice(c, s, ctx);
    case Index_kind:
        VISIT(c, expr, s->v.Index.value);
        break;
    case ExtSlice_kind:
    default:
        PyErr_SetString(PyExc_SystemError,
                        "extended slice invalid in nested slice");
        return 0;
    }
    return 1;
}

static int
compiler_visit_slice(struct compiler *c, slice_ty s, expr_context_ty ctx)
{
    char * kindname = NULL;
    switch (s->kind) {
    case Index_kind:
        kindname = "index";
        if (ctx != AugStore) {
            VISIT(c, expr, s->v.Index.value);
        }
        break;
    case Slice_kind:
        kindname = "slice";
        if (ctx != AugStore) {
            if (!compiler_slice(c, s, ctx))
                return 0;
        }
        break;
    case ExtSlice_kind:
        kindname = "extended slice";
        if (ctx != AugStore) {
            Py_ssize_t i, n = asdl_seq_LEN(s->v.ExtSlice.dims);
            for (i = 0; i < n; i++) {
                slice_ty sub = (slice_ty)asdl_seq_GET(
                    s->v.ExtSlice.dims, i);
                if (!compiler_visit_nested_slice(c, sub, ctx))
                    return 0;
            }
            ADDOP_I(c, BUILD_TUPLE, n);
        }
        break;
    default:
        PyErr_Format(PyExc_SystemError,
                     "invalid subscript kind %d", s->kind);
        return 0;
    }
    return compiler_handle_subscr(c, kindname, ctx);
}

/* End of the compiler section, beginning of the assembler section */

/* do depth-first search of basic block graph, starting with block.
   post records the block indices in post-order.

   XXX must handle implicit jumps from one block to next
*/

struct assembler {
    PyObject *a_bytecode;  /* string containing bytecode */
    int a_offset;              /* offset into bytecode */
    int a_nblocks;             /* number of reachable blocks */
    basicblock **a_postorder; /* list of blocks in dfs postorder */
    PyObject *a_lnotab;    /* string containing lnotab */
    int a_lnotab_off;      /* offset into lnotab */
    int a_lineno;              /* last lineno of emitted instruction */
    int a_lineno_off;      /* bytecode offset of last lineno */
};

static void
dfs(struct compiler *c, basicblock *b, struct assembler *a)
{
    int i;
    struct instr *instr = NULL;

    if (b->b_seen)
        return;
    b->b_seen = 1;
    if (b->b_next != NULL)
        dfs(c, b->b_next, a);
    for (i = 0; i < b->b_iused; i++) {
        instr = &b->b_instr[i];
        if (instr->i_jrel || instr->i_jabs)
            dfs(c, instr->i_target, a);
    }
    a->a_postorder[a->a_nblocks++] = b;
}

static int
stackdepth_walk(struct compiler *c, basicblock *b, int depth, int maxdepth)
{
    int i, target_depth, effect;
    struct instr *instr;
    if (b->b_seen || b->b_startdepth >= depth)
        return maxdepth;
    b->b_seen = 1;
    b->b_startdepth = depth;
    for (i = 0; i < b->b_iused; i++) {
        instr = &b->b_instr[i];
        effect = PyCompile_OpcodeStackEffect(instr->i_opcode, instr->i_oparg);
        if (effect == PY_INVALID_STACK_EFFECT) {
            fprintf(stderr, "opcode = %d\n", instr->i_opcode);
            Py_FatalError("PyCompile_OpcodeStackEffect()");
        }
        depth += effect;

        if (depth > maxdepth)
            maxdepth = depth;
        assert(depth >= 0); /* invalid code or bug in stackdepth() */
        if (instr->i_jrel || instr->i_jabs) {
            target_depth = depth;
            if (instr->i_opcode == FOR_ITER) {
                target_depth = depth-2;
            }
            else if (instr->i_opcode == SETUP_FINALLY ||
                     instr->i_opcode == SETUP_EXCEPT) {
                target_depth = depth+3;
                if (target_depth > maxdepth)
                    maxdepth = target_depth;
            }
            else if (instr->i_opcode == JUMP_IF_TRUE_OR_POP ||
                     instr->i_opcode == JUMP_IF_FALSE_OR_POP)
                depth = depth - 1;
            maxdepth = stackdepth_walk(c, instr->i_target,
                                       target_depth, maxdepth);
            if (instr->i_opcode == JUMP_ABSOLUTE ||
                instr->i_opcode == JUMP_FORWARD) {
                goto out; /* remaining code is dead */
            }
        }
    }
    if (b->b_next)
        maxdepth = stackdepth_walk(c, b->b_next, depth, maxdepth);
out:
    b->b_seen = 0;
    return maxdepth;
}

/* Find the flow path that needs the largest stack.  We assume that
 * cycles in the flow graph have no net effect on the stack depth.
 */
static int
stackdepth(struct compiler *c)
{
    basicblock *b, *entryblock;
    entryblock = NULL;
    for (b = c->u->u_blocks; b != NULL; b = b->b_list) {
        b->b_seen = 0;
        b->b_startdepth = INT_MIN;
        entryblock = b;
    }
    if (!entryblock)
        return 0;
    return stackdepth_walk(c, entryblock, 0, 0);
}

static int
assemble_init(struct assembler *a, int nblocks, int firstlineno)
{
    memset(a, 0, sizeof(struct assembler));
    a->a_lineno = firstlineno;
    a->a_bytecode = PyBytes_FromStringAndSize(NULL, DEFAULT_CODE_SIZE);
    if (!a->a_bytecode)
        return 0;
    a->a_lnotab = PyBytes_FromStringAndSize(NULL, DEFAULT_LNOTAB_SIZE);
    if (!a->a_lnotab)
        return 0;
    if ((size_t)nblocks > SIZE_MAX / sizeof(basicblock *)) {
        PyErr_NoMemory();
        return 0;
    }
    a->a_postorder = (basicblock **)PyObject_Malloc(
                                        sizeof(basicblock *) * nblocks);
    if (!a->a_postorder) {
        PyErr_NoMemory();
        return 0;
    }
    return 1;
}

static void
assemble_free(struct assembler *a)
{
    Py_XDECREF(a->a_bytecode);
    Py_XDECREF(a->a_lnotab);
    if (a->a_postorder)
        PyObject_Free(a->a_postorder);
}

static int
blocksize(basicblock *b)
{
    int i;
    int size = 0;

    for (i = 0; i < b->b_iused; i++)
        size += instrsize(b->b_instr[i].i_oparg);
    return size;
}

/* Appends a pair to the end of the line number table, a_lnotab, representing
   the instruction's bytecode offset and line number.  See
   Objects/lnotab_notes.txt for the description of the line number table. */

static int
assemble_lnotab(struct assembler *a, struct instr *i)
{
    int d_bytecode, d_lineno;
    Py_ssize_t len;
    unsigned char *lnotab;

    d_bytecode = (a->a_offset - a->a_lineno_off) * sizeof(_Py_CODEUNIT);
    d_lineno = i->i_lineno - a->a_lineno;

    assert(d_bytecode >= 0);

    if(d_bytecode == 0 && d_lineno == 0)
        return 1;

    if (d_bytecode > 255) {
        int j, nbytes, ncodes = d_bytecode / 255;
        nbytes = a->a_lnotab_off + 2 * ncodes;
        len = PyBytes_GET_SIZE(a->a_lnotab);
        if (nbytes >= len) {
            if ((len <= INT_MAX / 2) && (len * 2 < nbytes))
                len = nbytes;
            else if (len <= INT_MAX / 2)
                len *= 2;
            else {
                PyErr_NoMemory();
                return 0;
            }
            if (_PyBytes_Resize(&a->a_lnotab, len) < 0)
                return 0;
        }
        lnotab = (unsigned char *)
                   PyBytes_AS_STRING(a->a_lnotab) + a->a_lnotab_off;
        for (j = 0; j < ncodes; j++) {
            *lnotab++ = 255;
            *lnotab++ = 0;
        }
        d_bytecode -= ncodes * 255;
        a->a_lnotab_off += ncodes * 2;
    }
    assert(0 <= d_bytecode && d_bytecode <= 255);

    if (d_lineno < -128 || 127 < d_lineno) {
        int j, nbytes, ncodes, k;
        if (d_lineno < 0) {
            k = -128;
            /* use division on positive numbers */
            ncodes = (-d_lineno) / 128;
        }
        else {
            k = 127;
            ncodes = d_lineno / 127;
        }
        d_lineno -= ncodes * k;
        assert(ncodes >= 1);
        nbytes = a->a_lnotab_off + 2 * ncodes;
        len = PyBytes_GET_SIZE(a->a_lnotab);
        if (nbytes >= len) {
            if ((len <= INT_MAX / 2) && len * 2 < nbytes)
                len = nbytes;
            else if (len <= INT_MAX / 2)
                len *= 2;
            else {
                PyErr_NoMemory();
                return 0;
            }
            if (_PyBytes_Resize(&a->a_lnotab, len) < 0)
                return 0;
        }
        lnotab = (unsigned char *)
                   PyBytes_AS_STRING(a->a_lnotab) + a->a_lnotab_off;
        *lnotab++ = d_bytecode;
        *lnotab++ = k;
        d_bytecode = 0;
        for (j = 1; j < ncodes; j++) {
            *lnotab++ = 0;
            *lnotab++ = k;
        }
        a->a_lnotab_off += ncodes * 2;
    }
    assert(-128 <= d_lineno && d_lineno <= 127);

    len = PyBytes_GET_SIZE(a->a_lnotab);
    if (a->a_lnotab_off + 2 >= len) {
        if (_PyBytes_Resize(&a->a_lnotab, len * 2) < 0)
            return 0;
    }
    lnotab = (unsigned char *)
                    PyBytes_AS_STRING(a->a_lnotab) + a->a_lnotab_off;

    a->a_lnotab_off += 2;
    if (d_bytecode) {
        *lnotab++ = d_bytecode;
        *lnotab++ = d_lineno;
    }
    else {      /* First line of a block; def stmt, etc. */
        *lnotab++ = 0;
        *lnotab++ = d_lineno;
    }
    a->a_lineno = i->i_lineno;
    a->a_lineno_off = a->a_offset;
    return 1;
}

/* assemble_emit()
   Extend the bytecode with a new instruction.
   Update lnotab if necessary.
*/

static int
assemble_emit(struct assembler *a, struct instr *i)
{
    int size, arg = 0;
    Py_ssize_t len = PyBytes_GET_SIZE(a->a_bytecode);
    _Py_CODEUNIT *code;

    arg = i->i_oparg;
    size = instrsize(arg);
    if (i->i_lineno && !assemble_lnotab(a, i))
        return 0;
    if (a->a_offset + size >= len / (int)sizeof(_Py_CODEUNIT)) {
        if (len > PY_SSIZE_T_MAX / 2)
            return 0;
        if (_PyBytes_Resize(&a->a_bytecode, len * 2) < 0)
            return 0;
    }
    code = (_Py_CODEUNIT *)PyBytes_AS_STRING(a->a_bytecode) + a->a_offset;
    a->a_offset += size;
    write_op_arg(code, i->i_opcode, arg, size);
    return 1;
}

static void
assemble_jump_offsets(struct assembler *a, struct compiler *c)
{
    basicblock *b;
    int bsize, totsize, extended_arg_recompile;
    int i;

    /* Compute the size of each block and fixup jump args.
       Replace block pointer with position in bytecode. */
    do {
        totsize = 0;
        for (i = a->a_nblocks - 1; i >= 0; i--) {
            b = a->a_postorder[i];
            bsize = blocksize(b);
            b->b_offset = totsize;
            totsize += bsize;
        }
        extended_arg_recompile = 0;
        for (b = c->u->u_blocks; b != NULL; b = b->b_list) {
            bsize = b->b_offset;
            for (i = 0; i < b->b_iused; i++) {
                struct instr *instr = &b->b_instr[i];
                int isize = instrsize(instr->i_oparg);
                /* Relative jumps are computed relative to
                   the instruction pointer after fetching
                   the jump instruction.
                */
                bsize += isize;
                if (instr->i_jabs || instr->i_jrel) {
                    instr->i_oparg = instr->i_target->b_offset;
                    if (instr->i_jrel) {
                        instr->i_oparg -= bsize;
                    }
                    instr->i_oparg *= sizeof(_Py_CODEUNIT);
                    if (instrsize(instr->i_oparg) != isize) {
                        extended_arg_recompile = 1;
                    }
                }
            }
        }

    /* XXX: This is an awful hack that could hurt performance, but
        on the bright side it should work until we come up
        with a better solution.

        The issue is that in the first loop blocksize() is called
        which calls instrsize() which requires i_oparg be set
        appropriately. There is a bootstrap problem because
        i_oparg is calculated in the second loop above.

        So we loop until we stop seeing new EXTENDED_ARGs.
        The only EXTENDED_ARGs that could be popping up are
        ones in jump instructions.  So this should converge
        fairly quickly.
    */
    } while (extended_arg_recompile);
}

static PyObject *
dict_keys_inorder(PyObject *dict, Py_ssize_t offset)
{
    PyObject *tuple, *k, *v;
    Py_ssize_t i, pos = 0, size = PyDict_Size(dict);

    tuple = PyTuple_New(size);
    if (tuple == NULL)
        return NULL;
    while (PyDict_Next(dict, &pos, &k, &v)) {
        i = PyLong_AS_LONG(v);
        /* The keys of the dictionary are tuples. (see compiler_add_o
         * and _PyCode_ConstantKey). The object we want is always second,
         * though. */
        k = PyTuple_GET_ITEM(k, 1);
        Py_INCREF(k);
        assert((i - offset) < size);
        assert((i - offset) >= 0);
        PyTuple_SET_ITEM(tuple, i - offset, k);
    }
    return tuple;
}

static int
compute_code_flags(struct compiler *c)
{
    PySTEntryObject *ste = c->u->u_ste;
    int flags = 0;
    if (ste->ste_type == FunctionBlock) {
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
    flags |= (c->c_flags->cf_flags & PyCF_MASK);

    return flags;
}

static PyCodeObject *
makecode(struct compiler *c, struct assembler *a)
{
    PyObject *tmp;
    PyCodeObject *co = NULL;
    PyObject *consts = NULL;
    PyObject *names = NULL;
    PyObject *varnames = NULL;
    PyObject *name = NULL;
    PyObject *freevars = NULL;
    PyObject *cellvars = NULL;
    PyObject *bytecode = NULL;
    Py_ssize_t nlocals;
    int nlocals_int;
    int flags;
    int argcount, kwonlyargcount;

    tmp = dict_keys_inorder(c->u->u_consts, 0);
    if (!tmp)
        goto error;
    consts = PySequence_List(tmp); /* optimize_code requires a list */
    Py_DECREF(tmp);

    names = dict_keys_inorder(c->u->u_names, 0);
    varnames = dict_keys_inorder(c->u->u_varnames, 0);
    if (!consts || !names || !varnames)
        goto error;

    cellvars = dict_keys_inorder(c->u->u_cellvars, 0);
    if (!cellvars)
        goto error;
    freevars = dict_keys_inorder(c->u->u_freevars, PyTuple_Size(cellvars));
    if (!freevars)
        goto error;

    nlocals = PyDict_Size(c->u->u_varnames);
    assert(nlocals < INT_MAX);
    nlocals_int = Py_SAFE_DOWNCAST(nlocals, Py_ssize_t, int);

    flags = compute_code_flags(c);
    if (flags < 0)
        goto error;

    bytecode = PyCode_Optimize(a->a_bytecode, consts, names, a->a_lnotab);
    if (!bytecode)
        goto error;

    tmp = PyList_AsTuple(consts); /* PyCode_New requires a tuple */
    if (!tmp)
        goto error;
    Py_DECREF(consts);
    consts = tmp;

    argcount = Py_SAFE_DOWNCAST(c->u->u_argcount, Py_ssize_t, int);
    kwonlyargcount = Py_SAFE_DOWNCAST(c->u->u_kwonlyargcount, Py_ssize_t, int);
    co = PyCode_New(argcount, kwonlyargcount,
                    nlocals_int, stackdepth(c), flags,
                    bytecode, consts, names, varnames,
                    freevars, cellvars,
                    c->c_filename, c->u->u_name,
                    c->u->u_firstlineno,
                    a->a_lnotab);
 error:
    Py_XDECREF(consts);
    Py_XDECREF(names);
    Py_XDECREF(varnames);
    Py_XDECREF(name);
    Py_XDECREF(freevars);
    Py_XDECREF(cellvars);
    Py_XDECREF(bytecode);
    return co;
}


/* For debugging purposes only */
#if 0
static void
dump_instr(const struct instr *i)
{
    const char *jrel = i->i_jrel ? "jrel " : "";
    const char *jabs = i->i_jabs ? "jabs " : "";
    char arg[128];

    *arg = '\0';
    if (HAS_ARG(i->i_opcode)) {
        sprintf(arg, "arg: %d ", i->i_oparg);
    }
    fprintf(stderr, "line: %d, opcode: %d %s%s%s\n",
                    i->i_lineno, i->i_opcode, arg, jabs, jrel);
}

static void
dump_basicblock(const basicblock *b)
{
    const char *seen = b->b_seen ? "seen " : "";
    const char *b_return = b->b_return ? "return " : "";
    fprintf(stderr, "used: %d, depth: %d, offset: %d %s%s\n",
        b->b_iused, b->b_startdepth, b->b_offset, seen, b_return);
    if (b->b_instr) {
        int i;
        for (i = 0; i < b->b_iused; i++) {
            fprintf(stderr, "  [%02d] ", i);
            dump_instr(b->b_instr + i);
        }
    }
}
#endif

static PyCodeObject *
assemble(struct compiler *c, int addNone)
{
    basicblock *b, *entryblock;
    struct assembler a;
    int i, j, nblocks;
    PyCodeObject *co = NULL;

    /* Make sure every block that falls off the end returns None.
       XXX NEXT_BLOCK() isn't quite right, because if the last
       block ends with a jump or return b_next shouldn't set.
     */
    if (!c->u->u_curblock->b_return) {
        NEXT_BLOCK(c);
        if (addNone)
            ADDOP_O(c, LOAD_CONST, Py_None, consts);
        ADDOP(c, RETURN_VALUE);
    }

    nblocks = 0;
    entryblock = NULL;
    for (b = c->u->u_blocks; b != NULL; b = b->b_list) {
        nblocks++;
        entryblock = b;
    }

    /* Set firstlineno if it wasn't explicitly set. */
    if (!c->u->u_firstlineno) {
        if (entryblock && entryblock->b_instr && entryblock->b_instr->i_lineno)
            c->u->u_firstlineno = entryblock->b_instr->i_lineno;
        else
            c->u->u_firstlineno = 1;
    }
    if (!assemble_init(&a, nblocks, c->u->u_firstlineno))
        goto error;
    dfs(c, entryblock, &a);

    /* Can't modify the bytecode after computing jump offsets. */
    assemble_jump_offsets(&a, c);

    /* Emit code in reverse postorder from dfs. */
    for (i = a.a_nblocks - 1; i >= 0; i--) {
        b = a.a_postorder[i];
        for (j = 0; j < b->b_iused; j++)
            if (!assemble_emit(&a, &b->b_instr[j]))
                goto error;
    }

    if (_PyBytes_Resize(&a.a_lnotab, a.a_lnotab_off) < 0)
        goto error;
    if (_PyBytes_Resize(&a.a_bytecode, a.a_offset * sizeof(_Py_CODEUNIT)) < 0)
        goto error;

    co = makecode(c, &a);
 error:
    assemble_free(&a);
    return co;
}

#undef PyAST_Compile
PyAPI_FUNC(PyCodeObject *)
PyAST_Compile(mod_ty mod, const char *filename, PyCompilerFlags *flags,
              PyArena *arena)
{
    return PyAST_CompileEx(mod, filename, flags, -1, arena);
}
