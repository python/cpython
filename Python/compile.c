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
#include "pyarena.h"
#include "ast.h"
#include "code.h"
#include "compile.h"
#include "symtable.h"
#include "opcode.h"

int Py_OptimizeFlag = 0;

#define DEFAULT_BLOCK_SIZE 16
#define DEFAULT_BLOCKS 8
#define DEFAULT_CODE_SIZE 128
#define DEFAULT_LNOTAB_SIZE 16

#define COMP_GENEXP   0
#define COMP_SETCOMP  1
#define COMP_DICTCOMP 2

struct instr {
    unsigned i_jabs : 1;
    unsigned i_jrel : 1;
    unsigned i_hasarg : 1;
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

/* The following items change on entry and exit of code blocks.
   They must be saved and restored when returning to a block.
*/
struct compiler_unit {
    PySTEntryObject *u_ste;

    PyObject *u_name;
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

    int u_argcount;        /* number of arguments for block */
    /* Pointer to the most recently allocated block.  By following b_list
       members, you can reach all early allocated blocks. */
    basicblock *u_blocks;
    basicblock *u_curblock; /* pointer to current block */

    int u_nfblocks;
    struct fblockinfo u_fblock[CO_MAXBLOCKS];

    int u_firstlineno; /* the first lineno of the block */
    int u_lineno;          /* the lineno for the current stmt */
    bool u_lineno_set; /* boolean to indicate whether instr
                          has been generated with current lineno */
};

/* This struct captures the global state of a compilation.

The u pointer points to the current compilation unit, while units
for enclosing blocks are stored in c_stack.     The u and c_stack are
managed by compiler_enter_scope() and compiler_exit_scope().
*/

struct compiler {
    const char *c_filename;
    struct symtable *c_st;
    PyFutureFeatures *c_future; /* pointer to module's __future__ */
    PyCompilerFlags *c_flags;

    int c_interactive;           /* true if in interactive mode */
    int c_nestlevel;

    struct compiler_unit *u; /* compiler state for current block */
    PyObject *c_stack;           /* Python list holding compiler_unit ptrs */
    PyArena *c_arena;            /* pointer to memory allocation arena */
};

static int compiler_enter_scope(struct compiler *, identifier, void *, int);
static void compiler_free(struct compiler *);
static basicblock *compiler_new_block(struct compiler *);
static int compiler_next_instr(struct compiler *, basicblock *);
static int compiler_addop(struct compiler *, int);
static int compiler_addop_o(struct compiler *, int, PyObject *, PyObject *);
static int compiler_addop_i(struct compiler *, int, int);
static int compiler_addop_j(struct compiler *, int, basicblock *, int);
static basicblock *compiler_use_new_block(struct compiler *);
static int compiler_error(struct compiler *, const char *);
static int compiler_nameop(struct compiler *, identifier, expr_context_ty);

static PyCodeObject *compiler_mod(struct compiler *, mod_ty);
static int compiler_visit_stmt(struct compiler *, stmt_ty);
static int compiler_visit_keyword(struct compiler *, keyword_ty);
static int compiler_visit_expr(struct compiler *, expr_ty);
static int compiler_augassign(struct compiler *, stmt_ty);
static int compiler_visit_slice(struct compiler *, slice_ty,
                                expr_context_ty);

static int compiler_push_fblock(struct compiler *, enum fblocktype,
                                basicblock *);
static void compiler_pop_fblock(struct compiler *, enum fblocktype,
                                basicblock *);
/* Returns true if there is a loop on the fblock stack. */
static int compiler_in_loop(struct compiler *);

static int inplace_binop(struct compiler *, operator_ty);
static int expr_constant(expr_ty e);

static int compiler_with(struct compiler *, stmt_ty);

static PyCodeObject *assemble(struct compiler *, int addNone);
static PyObject *__doc__;

#define COMPILER_CAPSULE_NAME_COMPILER_UNIT "compile.c compiler unit"

PyObject *
_Py_Mangle(PyObject *privateobj, PyObject *ident)
{
    /* Name mangling: __private becomes _classname__private.
       This is independent from how the name is used. */
    const char *p, *name = PyString_AsString(ident);
    char *buffer;
    size_t nlen, plen;
    if (privateobj == NULL || !PyString_Check(privateobj) ||
        name == NULL || name[0] != '_' || name[1] != '_') {
        Py_INCREF(ident);
        return ident;
    }
    p = PyString_AsString(privateobj);
    nlen = strlen(name);
    /* Don't mangle __id__ or names with dots.

       The only time a name with a dot can occur is when
       we are compiling an import statement that has a
       package name.

       TODO(jhylton): Decide whether we want to support
       mangling of the module name, e.g. __M.X.
    */
    if ((name[nlen-1] == '_' && name[nlen-2] == '_')
        || strchr(name, '.')) {
        Py_INCREF(ident);
        return ident; /* Don't mangle __whatever__ */
    }
    /* Strip leading underscores from class name */
    while (*p == '_')
        p++;
    if (*p == '\0') {
        Py_INCREF(ident);
        return ident; /* Don't mangle if class is just underscores */
    }
    plen = strlen(p);

    assert(1 <= PY_SSIZE_T_MAX - nlen);
    assert(1 + nlen <= PY_SSIZE_T_MAX - plen);

    ident = PyString_FromStringAndSize(NULL, 1 + nlen + plen);
    if (!ident)
        return 0;
    /* ident = "_" + p[:plen] + name # i.e. 1+plen+nlen bytes */
    buffer = PyString_AS_STRING(ident);
    buffer[0] = '_';
    strncpy(buffer+1, p, plen);
    strcpy(buffer+1+plen, name);
    return ident;
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
PyAST_Compile(mod_ty mod, const char *filename, PyCompilerFlags *flags,
              PyArena *arena)
{
    struct compiler c;
    PyCodeObject *co = NULL;
    PyCompilerFlags local_flags;
    int merged;

    if (!__doc__) {
        __doc__ = PyString_InternFromString("__doc__");
        if (!__doc__)
            return NULL;
    }

    if (!compiler_init(&c))
        return NULL;
    c.c_filename = filename;
    c.c_arena = arena;
    c.c_future = PyFuture_FromAST(mod, filename);
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
    c.c_nestlevel = 0;

    c.c_st = PySymtable_Build(mod, filename, c.c_future);
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
        v = PyInt_FromLong(i);
        if (!v) {
            Py_DECREF(dict);
            return NULL;
        }
        k = PyList_GET_ITEM(list, i);
        k = PyTuple_Pack(2, k, k->ob_type);
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
dictbytype(PyObject *src, int scope_type, int flag, int offset)
{
    Py_ssize_t pos = 0, i = offset, scope;
    PyObject *k, *v, *dest = PyDict_New();

    assert(offset >= 0);
    if (dest == NULL)
        return NULL;

    while (PyDict_Next(src, &pos, &k, &v)) {
        /* XXX this should probably be a macro in symtable.h */
        assert(PyInt_Check(v));
        scope = (PyInt_AS_LONG(v) >> SCOPE_OFF) & SCOPE_MASK;

        if (scope == scope_type || PyInt_AS_LONG(v) & flag) {
            PyObject *tuple, *item = PyInt_FromLong(i);
            if (item == NULL) {
                Py_DECREF(dest);
                return NULL;
            }
            i++;
            tuple = PyTuple_Pack(2, k, k->ob_type);
            if (!tuple || PyDict_SetItem(dest, tuple, item) < 0) {
                Py_DECREF(item);
                Py_DECREF(dest);
                Py_XDECREF(tuple);
                return NULL;
            }
            Py_DECREF(item);
            Py_DECREF(tuple);
        }
    }
    return dest;
}

static void
compiler_unit_check(struct compiler_unit *u)
{
    basicblock *block;
    for (block = u->u_blocks; block != NULL; block = block->b_list) {
        assert((void *)block != (void *)0xcbcbcbcb);
        assert((void *)block != (void *)0xfbfbfbfb);
        assert((void *)block != (void *)0xdbdbdbdb);
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
    Py_CLEAR(u->u_consts);
    Py_CLEAR(u->u_names);
    Py_CLEAR(u->u_varnames);
    Py_CLEAR(u->u_freevars);
    Py_CLEAR(u->u_cellvars);
    Py_CLEAR(u->u_private);
    PyObject_Free(u);
}

static int
compiler_enter_scope(struct compiler *c, identifier name, void *key,
                     int lineno)
{
    struct compiler_unit *u;

    u = (struct compiler_unit *)PyObject_Malloc(sizeof(
                                            struct compiler_unit));
    if (!u) {
        PyErr_NoMemory();
        return 0;
    }
    memset(u, 0, sizeof(struct compiler_unit));
    u->u_argcount = 0;
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
    u->u_lineno_set = false;
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
        PyObject *capsule = PyCapsule_New(c->u, COMPILER_CAPSULE_NAME_COMPILER_UNIT, NULL);
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
    if (compiler_use_new_block(c) == NULL)
        return 0;

    return 1;
}

static void
compiler_exit_scope(struct compiler *c)
{
    int n;
    PyObject *capsule;

    c->c_nestlevel--;
    compiler_unit_free(c->u);
    /* Restore c->u to the parent unit. */
    n = PyList_GET_SIZE(c->c_stack) - 1;
    if (n >= 0) {
        capsule = PyList_GET_ITEM(c->c_stack, n);
        c->u = (struct compiler_unit *)PyCapsule_GetPointer(capsule, COMPILER_CAPSULE_NAME_COMPILER_UNIT);
        assert(c->u);
        /* we are deleting from a list so this really shouldn't fail */
        if (PySequence_DelItem(c->c_stack, n) < 0)
            Py_FatalError("compiler_exit_scope()");
        compiler_unit_check(c->u);
    }
    else
        c->u = NULL;

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
compiler_use_new_block(struct compiler *c)
{
    basicblock *block = compiler_new_block(c);
    if (block == NULL)
        return NULL;
    c->u->u_curblock = block;
    return block;
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

        if (oldsize > (PY_SIZE_MAX >> 1)) {
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
    c->u->u_lineno_set = true;
    b = c->u->u_curblock;
    b->b_instr[off].i_lineno = c->u->u_lineno;
}

static int
opcode_stack_effect(int opcode, int oparg)
{
    switch (opcode) {
        case POP_TOP:
            return -1;
        case ROT_TWO:
        case ROT_THREE:
            return 0;
        case DUP_TOP:
            return 1;
        case ROT_FOUR:
            return 0;

        case UNARY_POSITIVE:
        case UNARY_NEGATIVE:
        case UNARY_NOT:
        case UNARY_CONVERT:
        case UNARY_INVERT:
            return 0;

        case SET_ADD:
        case LIST_APPEND:
            return -1;

        case MAP_ADD:
            return -2;

        case BINARY_POWER:
        case BINARY_MULTIPLY:
        case BINARY_DIVIDE:
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

        case SLICE+0:
            return 0;
        case SLICE+1:
            return -1;
        case SLICE+2:
            return -1;
        case SLICE+3:
            return -2;

        case STORE_SLICE+0:
            return -2;
        case STORE_SLICE+1:
            return -3;
        case STORE_SLICE+2:
            return -3;
        case STORE_SLICE+3:
            return -4;

        case DELETE_SLICE+0:
            return -1;
        case DELETE_SLICE+1:
            return -2;
        case DELETE_SLICE+2:
            return -2;
        case DELETE_SLICE+3:
            return -3;

        case INPLACE_ADD:
        case INPLACE_SUBTRACT:
        case INPLACE_MULTIPLY:
        case INPLACE_DIVIDE:
        case INPLACE_MODULO:
            return -1;
        case STORE_SUBSCR:
            return -3;
        case STORE_MAP:
            return -2;
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
        case PRINT_ITEM:
            return -1;
        case PRINT_NEWLINE:
            return 0;
        case PRINT_ITEM_TO:
            return -2;
        case PRINT_NEWLINE_TO:
            return -1;
        case INPLACE_LSHIFT:
        case INPLACE_RSHIFT:
        case INPLACE_AND:
        case INPLACE_XOR:
        case INPLACE_OR:
            return -1;
        case BREAK_LOOP:
            return 0;
        case SETUP_WITH:
            return 4;
        case WITH_CLEANUP:
            return -1; /* XXX Sometimes more */
        case LOAD_LOCALS:
            return 1;
        case RETURN_VALUE:
            return -1;
        case IMPORT_STAR:
            return -1;
        case EXEC_STMT:
            return -3;
        case YIELD_VALUE:
            return 0;

        case POP_BLOCK:
            return 0;
        case END_FINALLY:
            return -3; /* or -1 or -2 if no exception occurred or
                          return/break/continue */
        case BUILD_CLASS:
            return -2;

        case STORE_NAME:
            return -1;
        case DELETE_NAME:
            return 0;
        case UNPACK_SEQUENCE:
            return oparg-1;
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
        case DUP_TOPX:
            return oparg;
        case LOAD_CONST:
            return 1;
        case LOAD_NAME:
            return 1;
        case BUILD_TUPLE:
        case BUILD_LIST:
        case BUILD_SET:
            return 1-oparg;
        case BUILD_MAP:
            return 1;
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
        case SETUP_EXCEPT:
        case SETUP_FINALLY:
            return 0;

        case LOAD_FAST:
            return 1;
        case STORE_FAST:
            return -1;
        case DELETE_FAST:
            return 0;

        case RAISE_VARARGS:
            return -oparg;
#define NARGS(o) (((o) % 256) + 2*((o) / 256))
        case CALL_FUNCTION:
            return -NARGS(oparg);
        case CALL_FUNCTION_VAR:
        case CALL_FUNCTION_KW:
            return -NARGS(oparg)-1;
        case CALL_FUNCTION_VAR_KW:
            return -NARGS(oparg)-2;
#undef NARGS
        case MAKE_FUNCTION:
            return -oparg;
        case BUILD_SLICE:
            if (oparg == 3)
                return -2;
            else
                return -1;

        case MAKE_CLOSURE:
            return -oparg-1;
        case LOAD_CLOSURE:
            return 1;
        case LOAD_DEREF:
            return 1;
        case STORE_DEREF:
            return -1;
        default:
            fprintf(stderr, "opcode = %d\n", opcode);
            Py_FatalError("opcode_stack_effect()");

    }
    return 0; /* not reachable */
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
    off = compiler_next_instr(c, c->u->u_curblock);
    if (off < 0)
        return 0;
    b = c->u->u_curblock;
    i = &b->b_instr[off];
    i->i_opcode = opcode;
    i->i_hasarg = 0;
    if (opcode == RETURN_VALUE)
        b->b_return = 1;
    compiler_set_lineno(c, off);
    return 1;
}

static int
compiler_add_o(struct compiler *c, PyObject *dict, PyObject *o)
{
    PyObject *t, *v;
    Py_ssize_t arg;
    double d;

    /* necessary to make sure types aren't coerced (e.g., int and long) */
    /* _and_ to distinguish 0.0 from -0.0 e.g. on IEEE platforms */
    if (PyFloat_Check(o)) {
        d = PyFloat_AS_DOUBLE(o);
        /* all we need is to make the tuple different in either the 0.0
         * or -0.0 case from all others, just to avoid the "coercion".
         */
        if (d == 0.0 && copysign(1.0, d) < 0.0)
            t = PyTuple_Pack(3, o, o->ob_type, Py_None);
        else
            t = PyTuple_Pack(2, o, o->ob_type);
    }
#ifndef WITHOUT_COMPLEX
    else if (PyComplex_Check(o)) {
        Py_complex z;
        int real_negzero, imag_negzero;
        /* For the complex case we must make complex(x, 0.)
           different from complex(x, -0.) and complex(0., y)
           different from complex(-0., y), for any x and y.
           All four complex zeros must be distinguished.*/
        z = PyComplex_AsCComplex(o);
        real_negzero = z.real == 0.0 && copysign(1.0, z.real) < 0.0;
        imag_negzero = z.imag == 0.0 && copysign(1.0, z.imag) < 0.0;
        if (real_negzero && imag_negzero) {
            t = PyTuple_Pack(5, o, o->ob_type,
                             Py_None, Py_None, Py_None);
        }
        else if (imag_negzero) {
            t = PyTuple_Pack(4, o, o->ob_type, Py_None, Py_None);
        }
        else if (real_negzero) {
            t = PyTuple_Pack(3, o, o->ob_type, Py_None);
        }
        else {
            t = PyTuple_Pack(2, o, o->ob_type);
        }
    }
#endif /* WITHOUT_COMPLEX */
    else {
        t = PyTuple_Pack(2, o, o->ob_type);
    }
    if (t == NULL)
        return -1;

    v = PyDict_GetItem(dict, t);
    if (!v) {
        arg = PyDict_Size(dict);
        v = PyInt_FromLong(arg);
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
        arg = PyInt_AsLong(v);
    Py_DECREF(t);
    return arg;
}

static int
compiler_addop_o(struct compiler *c, int opcode, PyObject *dict,
                     PyObject *o)
{
    int arg = compiler_add_o(c, dict, o);
    if (arg < 0)
        return 0;
    return compiler_addop_i(c, opcode, arg);
}

static int
compiler_addop_name(struct compiler *c, int opcode, PyObject *dict,
                    PyObject *o)
{
    int arg;
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
compiler_addop_i(struct compiler *c, int opcode, int oparg)
{
    struct instr *i;
    int off;
    off = compiler_next_instr(c, c->u->u_curblock);
    if (off < 0)
        return 0;
    i = &c->u->u_curblock->b_instr[off];
    i->i_opcode = opcode;
    i->i_oparg = oparg;
    i->i_hasarg = 1;
    compiler_set_lineno(c, off);
    return 1;
}

static int
compiler_addop_j(struct compiler *c, int opcode, basicblock *b, int absolute)
{
    struct instr *i;
    int off;

    assert(b != NULL);
    off = compiler_next_instr(c, c->u->u_curblock);
    if (off < 0)
        return 0;
    i = &c->u->u_curblock->b_instr[off];
    i->i_opcode = opcode;
    i->i_target = b;
    i->i_hasarg = 1;
    if (absolute)
        i->i_jabs = 1;
    else
        i->i_jrel = 1;
    compiler_set_lineno(c, off);
    return 1;
}

/* The distinction between NEW_BLOCK and NEXT_BLOCK is subtle.  (I'd
   like to find better names.)  NEW_BLOCK() creates a new block and sets
   it as the current block.  NEXT_BLOCK() also creates an implicit jump
   from the current block to the new block.
*/

/* The returns inside these macros make it impossible to decref objects
   created in the local function.  Local objects should use the arena.
*/


#define NEW_BLOCK(C) { \
    if (compiler_use_new_block((C)) == NULL) \
        return 0; \
}

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
    return s->v.Expr.value->kind == Str_kind;
}

/* Compile a sequence of statements, checking for a docstring. */

static int
compiler_body(struct compiler *c, asdl_seq *stmts)
{
    int i = 0;
    stmt_ty st;

    if (!asdl_seq_LEN(stmts))
        return 1;
    st = (stmt_ty)asdl_seq_GET(stmts, 0);
    if (compiler_isdocstring(st) && Py_OptimizeFlag < 2) {
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
        module = PyString_InternFromString("<module>");
        if (!module)
            return NULL;
    }
    /* Use 0 for firstlineno initially, will fixup in assemble(). */
    if (!compiler_enter_scope(c, module, mod, 0))
        return NULL;
    switch (mod->kind) {
    case Module_kind:
        if (!compiler_body(c, mod->v.Module.body)) {
            compiler_exit_scope(c);
            return 0;
        }
        break;
    case Interactive_kind:
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
    int scope = PyST_GetScope(c->u->u_ste, name);
    if (scope == 0) {
        char buf[350];
        PyOS_snprintf(buf, sizeof(buf),
                      "unknown scope for %.100s in %.100s(%s) in %s\n"
                      "symbols: %s\nlocals: %s\nglobals: %s",
                      PyString_AS_STRING(name),
                      PyString_AS_STRING(c->u->u_name),
                      PyObject_REPR(c->u->u_ste->ste_id),
                      c->c_filename,
                      PyObject_REPR(c->u->u_ste->ste_symbols),
                      PyObject_REPR(c->u->u_varnames),
                      PyObject_REPR(c->u->u_names)
        );
        Py_FatalError(buf);
    }

    return scope;
}

static int
compiler_lookup_arg(PyObject *dict, PyObject *name)
{
    PyObject *k, *v;
    k = PyTuple_Pack(2, name, name->ob_type);
    if (k == NULL)
        return -1;
    v = PyDict_GetItem(dict, k);
    Py_DECREF(k);
    if (v == NULL)
        return -1;
    return PyInt_AS_LONG(v);
}

static int
compiler_make_closure(struct compiler *c, PyCodeObject *co, int args)
{
    int i, free = PyCode_GetNumFree(co);
    if (free == 0) {
        ADDOP_O(c, LOAD_CONST, (PyObject*)co, consts);
        ADDOP_I(c, MAKE_FUNCTION, args);
        return 1;
    }
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
            printf("lookup %s in %s %d %d\n"
                "freevars of %s: %s\n",
                PyObject_REPR(name),
                PyString_AS_STRING(c->u->u_name),
                reftype, arg,
                PyString_AS_STRING(co->co_name),
                PyObject_REPR(co->co_freevars));
            Py_FatalError("compiler_make_closure()");
        }
        ADDOP_I(c, LOAD_CLOSURE, arg);
    }
    ADDOP_I(c, BUILD_TUPLE, free);
    ADDOP_O(c, LOAD_CONST, (PyObject*)co, consts);
    ADDOP_I(c, MAKE_CLOSURE, args);
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
compiler_arguments(struct compiler *c, arguments_ty args)
{
    int i;
    int n = asdl_seq_LEN(args->args);
    /* Correctly handle nested argument lists */
    for (i = 0; i < n; i++) {
        expr_ty arg = (expr_ty)asdl_seq_GET(args->args, i);
        if (arg->kind == Tuple_kind) {
            PyObject *id = PyString_FromFormat(".%d", i);
            if (id == NULL) {
                return 0;
            }
            if (!compiler_nameop(c, id, Load)) {
                Py_DECREF(id);
                return 0;
            }
            Py_DECREF(id);
            VISIT(c, expr, arg);
        }
    }
    return 1;
}

static int
compiler_function(struct compiler *c, stmt_ty s)
{
    PyCodeObject *co;
    PyObject *first_const = Py_None;
    arguments_ty args = s->v.FunctionDef.args;
    asdl_seq* decos = s->v.FunctionDef.decorator_list;
    stmt_ty st;
    int i, n, docstring;

    assert(s->kind == FunctionDef_kind);

    if (!compiler_decorators(c, decos))
        return 0;
    if (args->defaults)
        VISIT_SEQ(c, expr, args->defaults);
    if (!compiler_enter_scope(c, s->v.FunctionDef.name, (void *)s,
                              s->lineno))
        return 0;

    st = (stmt_ty)asdl_seq_GET(s->v.FunctionDef.body, 0);
    docstring = compiler_isdocstring(st);
    if (docstring && Py_OptimizeFlag < 2)
        first_const = st->v.Expr.value->v.Str.s;
    if (compiler_add_o(c, c->u->u_consts, first_const) < 0)      {
        compiler_exit_scope(c);
        return 0;
    }

    /* unpack nested arguments */
    compiler_arguments(c, args);

    c->u->u_argcount = asdl_seq_LEN(args->args);
    n = asdl_seq_LEN(s->v.FunctionDef.body);
    /* if there was a docstring, we need to skip the first statement */
    for (i = docstring; i < n; i++) {
        st = (stmt_ty)asdl_seq_GET(s->v.FunctionDef.body, i);
        VISIT_IN_SCOPE(c, stmt, st);
    }
    co = assemble(c, 1);
    compiler_exit_scope(c);
    if (co == NULL)
        return 0;

    compiler_make_closure(c, co, asdl_seq_LEN(args->defaults));
    Py_DECREF(co);

    for (i = 0; i < asdl_seq_LEN(decos); i++) {
        ADDOP_I(c, CALL_FUNCTION, 1);
    }

    return compiler_nameop(c, s->v.FunctionDef.name, Store);
}

static int
compiler_class(struct compiler *c, stmt_ty s)
{
    int n, i;
    PyCodeObject *co;
    PyObject *str;
    asdl_seq* decos = s->v.ClassDef.decorator_list;

    if (!compiler_decorators(c, decos))
        return 0;

    /* push class name on stack, needed by BUILD_CLASS */
    ADDOP_O(c, LOAD_CONST, s->v.ClassDef.name, consts);
    /* push the tuple of base classes on the stack */
    n = asdl_seq_LEN(s->v.ClassDef.bases);
    if (n > 0)
        VISIT_SEQ(c, expr, s->v.ClassDef.bases);
    ADDOP_I(c, BUILD_TUPLE, n);
    if (!compiler_enter_scope(c, s->v.ClassDef.name, (void *)s,
                              s->lineno))
        return 0;
    Py_XDECREF(c->u->u_private);
    c->u->u_private = s->v.ClassDef.name;
    Py_INCREF(c->u->u_private);
    str = PyString_InternFromString("__name__");
    if (!str || !compiler_nameop(c, str, Load)) {
        Py_XDECREF(str);
        compiler_exit_scope(c);
        return 0;
    }

    Py_DECREF(str);
    str = PyString_InternFromString("__module__");
    if (!str || !compiler_nameop(c, str, Store)) {
        Py_XDECREF(str);
        compiler_exit_scope(c);
        return 0;
    }
    Py_DECREF(str);

    if (!compiler_body(c, s->v.ClassDef.body)) {
        compiler_exit_scope(c);
        return 0;
    }

    ADDOP_IN_SCOPE(c, LOAD_LOCALS);
    ADDOP_IN_SCOPE(c, RETURN_VALUE);
    co = assemble(c, 1);
    compiler_exit_scope(c);
    if (co == NULL)
        return 0;

    compiler_make_closure(c, co, 0);
    Py_DECREF(co);

    ADDOP_I(c, CALL_FUNCTION, 0);
    ADDOP(c, BUILD_CLASS);
    /* apply decorators */
    for (i = 0; i < asdl_seq_LEN(decos); i++) {
        ADDOP_I(c, CALL_FUNCTION, 1);
    }
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
    static identifier name;
    arguments_ty args = e->v.Lambda.args;
    assert(e->kind == Lambda_kind);

    if (!name) {
        name = PyString_InternFromString("<lambda>");
        if (!name)
            return 0;
    }

    if (args->defaults)
        VISIT_SEQ(c, expr, args->defaults);
    if (!compiler_enter_scope(c, name, (void *)e, e->lineno))
        return 0;

    /* unpack nested arguments */
    compiler_arguments(c, args);

    /* Make None the first constant, so the lambda can't have a
       docstring. */
    if (compiler_add_o(c, c->u->u_consts, Py_None) < 0)
        return 0;

    c->u->u_argcount = asdl_seq_LEN(args->args);
    VISIT_IN_SCOPE(c, expr, e->v.Lambda.body);
    if (c->u->u_ste->ste_generator) {
        ADDOP_IN_SCOPE(c, POP_TOP);
    }
    else {
        ADDOP_IN_SCOPE(c, RETURN_VALUE);
    }
    co = assemble(c, 1);
    compiler_exit_scope(c);
    if (co == NULL)
        return 0;

    compiler_make_closure(c, co, asdl_seq_LEN(args->defaults));
    Py_DECREF(co);

    return 1;
}

static int
compiler_print(struct compiler *c, stmt_ty s)
{
    int i, n;
    bool dest;

    assert(s->kind == Print_kind);
    n = asdl_seq_LEN(s->v.Print.values);
    dest = false;
    if (s->v.Print.dest) {
        VISIT(c, expr, s->v.Print.dest);
        dest = true;
    }
    for (i = 0; i < n; i++) {
        expr_ty e = (expr_ty)asdl_seq_GET(s->v.Print.values, i);
        if (dest) {
            ADDOP(c, DUP_TOP);
            VISIT(c, expr, e);
            ADDOP(c, ROT_TWO);
            ADDOP(c, PRINT_ITEM_TO);
        }
        else {
            VISIT(c, expr, e);
            ADDOP(c, PRINT_ITEM);
        }
    }
    if (s->v.Print.nl) {
        if (dest)
            ADDOP(c, PRINT_NEWLINE_TO)
        else
            ADDOP(c, PRINT_NEWLINE)
    }
    else if (dest)
        ADDOP(c, POP_TOP);
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

    constant = expr_constant(s->v.If.test);
    /* constant = 0: "if 0"
     * constant = 1: "if 1", "if 2", ...
     * constant = -1: rest */
    if (constant == 0) {
        if (s->v.If.orelse)
            VISIT_SEQ(c, stmt, s->v.If.orelse);
    } else if (constant == 1) {
        VISIT_SEQ(c, stmt, s->v.If.body);
    } else {
        if (s->v.If.orelse) {
            next = compiler_new_block(c);
            if (next == NULL)
                return 0;
        }
        else
            next = end;
        VISIT(c, expr, s->v.If.test);
        ADDOP_JABS(c, POP_JUMP_IF_FALSE, next);
        VISIT_SEQ(c, stmt, s->v.If.body);
        ADDOP_JREL(c, JUMP_FORWARD, end);
        if (s->v.If.orelse) {
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
compiler_while(struct compiler *c, stmt_ty s)
{
    basicblock *loop, *orelse, *end, *anchor = NULL;
    int constant = expr_constant(s->v.While.test);

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

    if (constant == -1) {
        compiler_use_next_block(c, anchor);
        ADDOP(c, POP_BLOCK);
    }
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
    VISIT_SEQ(c, stmt, s->v.TryFinally.body);
    ADDOP(c, POP_BLOCK);
    compiler_pop_fblock(c, FINALLY_TRY, body);

    ADDOP_O(c, LOAD_CONST, Py_None, consts);
    compiler_use_next_block(c, end);
    if (!compiler_push_fblock(c, FINALLY_END, end))
        return 0;
    VISIT_SEQ(c, stmt, s->v.TryFinally.finalbody);
    ADDOP(c, END_FINALLY);
    compiler_pop_fblock(c, FINALLY_END, end);

    return 1;
}

/*
   Code generated for "try: S except E1, V1: S1 except E2, V2: S2 ...":
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
    int i, n;

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
    VISIT_SEQ(c, stmt, s->v.TryExcept.body);
    ADDOP(c, POP_BLOCK);
    compiler_pop_fblock(c, EXCEPT, body);
    ADDOP_JREL(c, JUMP_FORWARD, orelse);
    n = asdl_seq_LEN(s->v.TryExcept.handlers);
    compiler_use_next_block(c, except);
    for (i = 0; i < n; i++) {
        excepthandler_ty handler = (excepthandler_ty)asdl_seq_GET(
                                        s->v.TryExcept.handlers, i);
        if (!handler->v.ExceptHandler.type && i < n-1)
            return compiler_error(c, "default 'except:' must be last");
        c->u->u_lineno_set = false;
        c->u->u_lineno = handler->lineno;
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
            VISIT(c, expr, handler->v.ExceptHandler.name);
        }
        else {
            ADDOP(c, POP_TOP);
        }
        ADDOP(c, POP_TOP);
        VISIT_SEQ(c, stmt, handler->v.ExceptHandler.body);
        ADDOP_JREL(c, JUMP_FORWARD, end);
        compiler_use_next_block(c, except);
    }
    ADDOP(c, END_FINALLY);
    compiler_use_next_block(c, orelse);
    VISIT_SEQ(c, stmt, s->v.TryExcept.orelse);
    compiler_use_next_block(c, end);
    return 1;
}

static int
compiler_import_as(struct compiler *c, identifier name, identifier asname)
{
    /* The IMPORT_NAME opcode was already generated.  This function
       merely needs to bind the result to a name.

       If there is a dot in name, we need to split it and emit a
       LOAD_ATTR for each name.
    */
    const char *src = PyString_AS_STRING(name);
    const char *dot = strchr(src, '.');
    if (dot) {
        /* Consume the base module name to get the first attribute */
        src = dot + 1;
        while (dot) {
            /* NB src is only defined when dot != NULL */
            PyObject *attr;
            dot = strchr(src, '.');
            attr = PyString_FromStringAndSize(src,
                                dot ? dot - src : strlen(src));
            if (!attr)
                return -1;
            ADDOP_O(c, LOAD_ATTR, attr, names);
            Py_DECREF(attr);
            src = dot + 1;
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
    int i, n = asdl_seq_LEN(s->v.Import.names);

    for (i = 0; i < n; i++) {
        alias_ty alias = (alias_ty)asdl_seq_GET(s->v.Import.names, i);
        int r;
        PyObject *level;

        if (c->c_flags && (c->c_flags->cf_flags & CO_FUTURE_ABSOLUTE_IMPORT))
            level = PyInt_FromLong(0);
        else
            level = PyInt_FromLong(-1);

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
            const char *base = PyString_AS_STRING(alias->name);
            char *dot = strchr(base, '.');
            if (dot)
                tmp = PyString_FromStringAndSize(base,
                                                 dot - base);
            r = compiler_nameop(c, tmp, Store);
            if (dot) {
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
    int i, n = asdl_seq_LEN(s->v.ImportFrom.names);

    PyObject *names = PyTuple_New(n);
    PyObject *level;
    static PyObject *empty_string;

    if (!empty_string) {
        empty_string = PyString_FromString("");
        if (!empty_string)
            return 0;
    }

    if (!names)
        return 0;

    if (s->v.ImportFrom.level == 0 && c->c_flags &&
        !(c->c_flags->cf_flags & CO_FUTURE_ABSOLUTE_IMPORT))
        level = PyInt_FromLong(-1);
    else
        level = PyInt_FromLong(s->v.ImportFrom.level);

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
        !strcmp(PyString_AS_STRING(s->v.ImportFrom.module), "__future__")) {
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

        if (i == 0 && *PyString_AS_STRING(alias->name) == '*') {
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

    if (Py_OptimizeFlag)
        return 1;
    if (assertion_error == NULL) {
        assertion_error = PyString_InternFromString("AssertionError");
        if (assertion_error == NULL)
            return 0;
    }
    if (s->v.Assert.test->kind == Tuple_kind &&
        asdl_seq_LEN(s->v.Assert.test->v.Tuple.elts) > 0) {
        const char* msg =
            "assertion is always true, perhaps remove parentheses?";
        if (PyErr_WarnExplicit(PyExc_SyntaxWarning, msg, c->c_filename,
                               c->u->u_lineno, NULL, NULL) == -1)
            return 0;
    }
    VISIT(c, expr, s->v.Assert.test);
    end = compiler_new_block(c);
    if (end == NULL)
        return 0;
    ADDOP_JABS(c, POP_JUMP_IF_TRUE, end);
    ADDOP_O(c, LOAD_GLOBAL, assertion_error, names);
    if (s->v.Assert.msg) {
        VISIT(c, expr, s->v.Assert.msg);
        ADDOP_I(c, RAISE_VARARGS, 2);
    }
    else {
        ADDOP_I(c, RAISE_VARARGS, 1);
    }
    compiler_use_next_block(c, end);
    return 1;
}

static int
compiler_visit_stmt(struct compiler *c, stmt_ty s)
{
    int i, n;

    /* Always assign a lineno to the next instruction for a stmt. */
    c->u->u_lineno = s->lineno;
    c->u->u_lineno_set = false;

    switch (s->kind) {
    case FunctionDef_kind:
        return compiler_function(c, s);
    case ClassDef_kind:
        return compiler_class(c, s);
    case Return_kind:
        if (c->u->u_ste->ste_type != FunctionBlock)
            return compiler_error(c, "'return' outside function");
        if (s->v.Return.value) {
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
    case Print_kind:
        return compiler_print(c, s);
    case For_kind:
        return compiler_for(c, s);
    case While_kind:
        return compiler_while(c, s);
    case If_kind:
        return compiler_if(c, s);
    case Raise_kind:
        n = 0;
        if (s->v.Raise.type) {
            VISIT(c, expr, s->v.Raise.type);
            n++;
            if (s->v.Raise.inst) {
                VISIT(c, expr, s->v.Raise.inst);
                n++;
                if (s->v.Raise.tback) {
                    VISIT(c, expr, s->v.Raise.tback);
                    n++;
                }
            }
        }
        ADDOP_I(c, RAISE_VARARGS, n);
        break;
    case TryExcept_kind:
        return compiler_try_except(c, s);
    case TryFinally_kind:
        return compiler_try_finally(c, s);
    case Assert_kind:
        return compiler_assert(c, s);
    case Import_kind:
        return compiler_import(c, s);
    case ImportFrom_kind:
        return compiler_from_import(c, s);
    case Exec_kind:
        VISIT(c, expr, s->v.Exec.body);
        if (s->v.Exec.globals) {
            VISIT(c, expr, s->v.Exec.globals);
            if (s->v.Exec.locals) {
                VISIT(c, expr, s->v.Exec.locals);
            } else {
                ADDOP(c, DUP_TOP);
            }
        } else {
            ADDOP_O(c, LOAD_CONST, Py_None, consts);
            ADDOP(c, DUP_TOP);
        }
        ADDOP(c, EXEC_STMT);
        break;
    case Global_kind:
        break;
    case Expr_kind:
        if (c->c_interactive && c->c_nestlevel <= 1) {
            VISIT(c, expr, s->v.Expr.value);
            ADDOP(c, PRINT_EXPR);
        }
        else if (s->v.Expr.value->kind != Str_kind &&
                 s->v.Expr.value->kind != Num_kind) {
            VISIT(c, expr, s->v.Expr.value);
            ADDOP(c, POP_TOP);
        }
        break;
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
        return compiler_with(c, s);
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
    case Div:
        if (c->c_flags && c->c_flags->cf_flags & CO_FUTURE_DIVISION)
            return BINARY_TRUE_DIVIDE;
        else
            return BINARY_DIVIDE;
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
    case Div:
        if (c->c_flags && c->c_flags->cf_flags & CO_FUTURE_DIVISION)
            return INPLACE_TRUE_DIVIDE;
        else
            return INPLACE_DIVIDE;
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
    int op, scope, arg;
    enum { OP_FAST, OP_GLOBAL, OP_DEREF, OP_NAME } optype;

    PyObject *dict = c->u->u_names;
    PyObject *mangled;
    /* XXX AugStore isn't used anywhere! */

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
        if (c->u->u_ste->ste_type == FunctionBlock &&
            !c->u->u_ste->ste_unoptimized)
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
    assert(scope || PyString_AS_STRING(name)[0] == '_');

    switch (optype) {
    case OP_DEREF:
        switch (ctx) {
        case Load: op = LOAD_DEREF; break;
        case Store: op = STORE_DEREF; break;
        case AugLoad:
        case AugStore:
            break;
        case Del:
            PyErr_Format(PyExc_SyntaxError,
                         "can not delete variable '%s' referenced "
                         "in nested scope",
                         PyString_AS_STRING(name));
            Py_DECREF(mangled);
            return 0;
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
    int jumpi, i, n;
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
compiler_list(struct compiler *c, expr_ty e)
{
    int n = asdl_seq_LEN(e->v.List.elts);
    if (e->v.List.ctx == Store) {
        ADDOP_I(c, UNPACK_SEQUENCE, n);
    }
    VISIT_SEQ(c, expr, e->v.List.elts);
    if (e->v.List.ctx == Load) {
        ADDOP_I(c, BUILD_LIST, n);
    }
    return 1;
}

static int
compiler_tuple(struct compiler *c, expr_ty e)
{
    int n = asdl_seq_LEN(e->v.Tuple.elts);
    if (e->v.Tuple.ctx == Store) {
        ADDOP_I(c, UNPACK_SEQUENCE, n);
    }
    VISIT_SEQ(c, expr, e->v.Tuple.elts);
    if (e->v.Tuple.ctx == Load) {
        ADDOP_I(c, BUILD_TUPLE, n);
    }
    return 1;
}

static int
compiler_compare(struct compiler *c, expr_ty e)
{
    int i, n;
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
    int n, code = 0;

    VISIT(c, expr, e->v.Call.func);
    n = asdl_seq_LEN(e->v.Call.args);
    VISIT_SEQ(c, expr, e->v.Call.args);
    if (e->v.Call.keywords) {
        VISIT_SEQ(c, keyword, e->v.Call.keywords);
        n |= asdl_seq_LEN(e->v.Call.keywords) << 8;
    }
    if (e->v.Call.starargs) {
        VISIT(c, expr, e->v.Call.starargs);
        code |= 1;
    }
    if (e->v.Call.kwargs) {
        VISIT(c, expr, e->v.Call.kwargs);
        code |= 2;
    }
    switch (code) {
    case 0:
        ADDOP_I(c, CALL_FUNCTION, n);
        break;
    case 1:
        ADDOP_I(c, CALL_FUNCTION_VAR, n);
        break;
    case 2:
        ADDOP_I(c, CALL_FUNCTION_KW, n);
        break;
    case 3:
        ADDOP_I(c, CALL_FUNCTION_VAR_KW, n);
        break;
    }
    return 1;
}

static int
compiler_listcomp_generator(struct compiler *c, asdl_seq *generators,
                            int gen_index, expr_ty elt)
{
    /* generate code for the iterator, then each of the ifs,
       and then write to the element */

    comprehension_ty l;
    basicblock *start, *anchor, *skip, *if_cleanup;
    int i, n;

    start = compiler_new_block(c);
    skip = compiler_new_block(c);
    if_cleanup = compiler_new_block(c);
    anchor = compiler_new_block(c);

    if (start == NULL || skip == NULL || if_cleanup == NULL ||
        anchor == NULL)
        return 0;

    l = (comprehension_ty)asdl_seq_GET(generators, gen_index);
    VISIT(c, expr, l->iter);
    ADDOP(c, GET_ITER);
    compiler_use_next_block(c, start);
    ADDOP_JREL(c, FOR_ITER, anchor);
    NEXT_BLOCK(c);
    VISIT(c, expr, l->target);

    /* XXX this needs to be cleaned up...a lot! */
    n = asdl_seq_LEN(l->ifs);
    for (i = 0; i < n; i++) {
        expr_ty e = (expr_ty)asdl_seq_GET(l->ifs, i);
        VISIT(c, expr, e);
        ADDOP_JABS(c, POP_JUMP_IF_FALSE, if_cleanup);
        NEXT_BLOCK(c);
    }

    if (++gen_index < asdl_seq_LEN(generators))
        if (!compiler_listcomp_generator(c, generators, gen_index, elt))
        return 0;

    /* only append after the last for generator */
    if (gen_index >= asdl_seq_LEN(generators)) {
        VISIT(c, expr, elt);
        ADDOP_I(c, LIST_APPEND, gen_index+1);

        compiler_use_next_block(c, skip);
    }
    compiler_use_next_block(c, if_cleanup);
    ADDOP_JABS(c, JUMP_ABSOLUTE, start);
    compiler_use_next_block(c, anchor);

    return 1;
}

static int
compiler_listcomp(struct compiler *c, expr_ty e)
{
    assert(e->kind == ListComp_kind);
    ADDOP_I(c, BUILD_LIST, 0);
    return compiler_listcomp_generator(c, e->v.ListComp.generators, 0,
                                       e->v.ListComp.elt);
}

/* Dict and set comprehensions and generator expressions work by creating a
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
    /* generate code for the iterator, then each of the ifs,
       and then write to the element */

    comprehension_ty gen;
    basicblock *start, *anchor, *skip, *if_cleanup;
    int i, n;

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
compiler_comprehension(struct compiler *c, expr_ty e, int type, identifier name,
                       asdl_seq *generators, expr_ty elt, expr_ty val)
{
    PyCodeObject *co = NULL;
    expr_ty outermost_iter;

    outermost_iter = ((comprehension_ty)
                      asdl_seq_GET(generators, 0))->iter;

    if (!compiler_enter_scope(c, name, (void *)e, e->lineno))
        goto error;

    if (type != COMP_GENEXP) {
        int op;
        switch (type) {
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
    compiler_exit_scope(c);
    if (co == NULL)
        goto error;

    if (!compiler_make_closure(c, co, 0))
        goto error;
    Py_DECREF(co);

    VISIT(c, expr, outermost_iter);
    ADDOP(c, GET_ITER);
    ADDOP_I(c, CALL_FUNCTION, 1);
    return 1;
error_in_scope:
    compiler_exit_scope(c);
error:
    Py_XDECREF(co);
    return 0;
}

static int
compiler_genexp(struct compiler *c, expr_ty e)
{
    static identifier name;
    if (!name) {
        name = PyString_FromString("<genexpr>");
        if (!name)
            return 0;
    }
    assert(e->kind == GeneratorExp_kind);
    return compiler_comprehension(c, e, COMP_GENEXP, name,
                                  e->v.GeneratorExp.generators,
                                  e->v.GeneratorExp.elt, NULL);
}

static int
compiler_setcomp(struct compiler *c, expr_ty e)
{
    static identifier name;
    if (!name) {
        name = PyString_FromString("<setcomp>");
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
        name = PyString_FromString("<dictcomp>");
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
    ADDOP_O(c, LOAD_CONST, k->arg, consts);
    VISIT(c, expr, k->value);
    return 1;
}

/* Test whether expression is constant.  For constants, report
   whether they are true or false.

   Return values: 1 for true, 0 for false, -1 for non-constant.
 */

static int
expr_constant(expr_ty e)
{
    switch (e->kind) {
    case Num_kind:
        return PyObject_IsTrue(e->v.Num.n);
    case Str_kind:
        return PyObject_IsTrue(e->v.Str.s);
    case Name_kind:
        /* __debug__ is not assignable, so we can optimize
         * it away in if and while statements */
        if (strcmp(PyString_AS_STRING(e->v.Name.id),
                   "__debug__") == 0)
                   return ! Py_OptimizeFlag;
        /* fall through */
    default:
        return -1;
    }
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
compiler_with(struct compiler *c, stmt_ty s)
{
    basicblock *block, *finally;

    assert(s->kind == With_kind);

    block = compiler_new_block(c);
    finally = compiler_new_block(c);
    if (!block || !finally)
        return 0;

    /* Evaluate EXPR */
    VISIT(c, expr, s->v.With.context_expr);
    ADDOP_JREL(c, SETUP_WITH, finally);

    /* SETUP_WITH pushes a finally block. */
    compiler_use_next_block(c, block);
    /* Note that the block is actually called SETUP_WITH in ceval.c, but
       functions the same as SETUP_FINALLY except that exceptions are
       normalized. */
    if (!compiler_push_fblock(c, FINALLY_TRY, block)) {
        return 0;
    }

    if (s->v.With.optional_vars) {
        VISIT(c, expr, s->v.With.optional_vars);
    }
    else {
    /* Discard result from context.__enter__() */
        ADDOP(c, POP_TOP);
    }

    /* BLOCK code */
    VISIT_SEQ(c, stmt, s->v.With.body);

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
    ADDOP(c, WITH_CLEANUP);

    /* Finally block ends. */
    ADDOP(c, END_FINALLY);
    compiler_pop_fblock(c, FINALLY_END, finally);
    return 1;
}

static int
compiler_visit_expr(struct compiler *c, expr_ty e)
{
    int i, n;

    /* If expr e has a different line number than the last expr/stmt,
       set a new line number for the next instruction.
    */
    if (e->lineno > c->u->u_lineno) {
        c->u->u_lineno = e->lineno;
        c->u->u_lineno_set = false;
    }
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
        n = asdl_seq_LEN(e->v.Dict.values);
        ADDOP_I(c, BUILD_MAP, (n>0xFFFF ? 0xFFFF : n));
        for (i = 0; i < n; i++) {
            VISIT(c, expr,
                (expr_ty)asdl_seq_GET(e->v.Dict.values, i));
            VISIT(c, expr,
                (expr_ty)asdl_seq_GET(e->v.Dict.keys, i));
            ADDOP(c, STORE_MAP);
        }
        break;
    case Set_kind:
        n = asdl_seq_LEN(e->v.Set.elts);
        VISIT_SEQ(c, expr, e->v.Set.elts);
        ADDOP_I(c, BUILD_SET, n);
        break;
    case ListComp_kind:
        return compiler_listcomp(c, e);
    case SetComp_kind:
        return compiler_setcomp(c, e);
    case DictComp_kind:
        return compiler_dictcomp(c, e);
    case GeneratorExp_kind:
        return compiler_genexp(c, e);
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
    case Compare_kind:
        return compiler_compare(c, e);
    case Call_kind:
        return compiler_call(c, e);
    case Repr_kind:
        VISIT(c, expr, e->v.Repr.value);
        ADDOP(c, UNARY_CONVERT);
        break;
    case Num_kind:
        ADDOP_O(c, LOAD_CONST, e->v.Num.n, consts);
        break;
    case Str_kind:
        ADDOP_O(c, LOAD_CONST, e->v.Str.s, consts);
        break;
    /* The following exprs can be assignment targets. */
    case Attribute_kind:
        if (e->v.Attribute.ctx != AugStore)
            VISIT(c, expr, e->v.Attribute.value);
        switch (e->v.Attribute.ctx) {
        case AugLoad:
            ADDOP(c, DUP_TOP);
            /* Fall through to load */
        case Load:
            ADDOP_NAME(c, LOAD_ATTR, e->v.Attribute.attr, names);
            break;
        case AugStore:
            ADDOP(c, ROT_TWO);
            /* Fall through to save */
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
compiler_push_fblock(struct compiler *c, enum fblocktype t, basicblock *b)
{
    struct fblockinfo *f;
    if (c->u->u_nfblocks >= CO_MAXBLOCKS) {
        PyErr_SetString(PyExc_SystemError,
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

    loc = PyErr_ProgramText(c->c_filename, c->u->u_lineno);
    if (!loc) {
        Py_INCREF(Py_None);
        loc = Py_None;
    }
    u = Py_BuildValue("(ziOO)", c->c_filename, c->u->u_lineno,
                      Py_None, loc);
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
        ADDOP_I(c, DUP_TOPX, 2);
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
compiler_simple_slice(struct compiler *c, slice_ty s, expr_context_ty ctx)
{
    int op = 0, slice_offset = 0, stack_count = 0;

    assert(s->v.Slice.step == NULL);
    if (s->v.Slice.lower) {
        slice_offset++;
        stack_count++;
        if (ctx != AugStore)
            VISIT(c, expr, s->v.Slice.lower);
    }
    if (s->v.Slice.upper) {
        slice_offset += 2;
        stack_count++;
        if (ctx != AugStore)
            VISIT(c, expr, s->v.Slice.upper);
    }

    if (ctx == AugLoad) {
        switch (stack_count) {
        case 0: ADDOP(c, DUP_TOP); break;
        case 1: ADDOP_I(c, DUP_TOPX, 2); break;
        case 2: ADDOP_I(c, DUP_TOPX, 3); break;
        }
    }
    else if (ctx == AugStore) {
        switch (stack_count) {
        case 0: ADDOP(c, ROT_TWO); break;
        case 1: ADDOP(c, ROT_THREE); break;
        case 2: ADDOP(c, ROT_FOUR); break;
        }
    }

    switch (ctx) {
    case AugLoad: /* fall through to Load */
    case Load: op = SLICE; break;
    case AugStore:/* fall through to Store */
    case Store: op = STORE_SLICE; break;
    case Del: op = DELETE_SLICE; break;
    case Param:
    default:
        PyErr_SetString(PyExc_SystemError,
                        "param invalid in simple slice");
        return 0;
    }

    ADDOP(c, op + slice_offset);
    return 1;
}

static int
compiler_visit_nested_slice(struct compiler *c, slice_ty s,
                            expr_context_ty ctx)
{
    switch (s->kind) {
    case Ellipsis_kind:
        ADDOP_O(c, LOAD_CONST, Py_Ellipsis, consts);
        break;
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
    case Ellipsis_kind:
        kindname = "ellipsis";
        if (ctx != AugStore) {
            ADDOP_O(c, LOAD_CONST, Py_Ellipsis, consts);
        }
        break;
    case Slice_kind:
        kindname = "slice";
        if (!s->v.Slice.step)
            return compiler_simple_slice(c, s, ctx);
        if (ctx != AugStore) {
            if (!compiler_slice(c, s, ctx))
                return 0;
        }
        break;
    case ExtSlice_kind:
        kindname = "extended slice";
        if (ctx != AugStore) {
            int i, n = asdl_seq_LEN(s->v.ExtSlice.dims);
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
    int i, target_depth;
    struct instr *instr;
    if (b->b_seen || b->b_startdepth >= depth)
        return maxdepth;
    b->b_seen = 1;
    b->b_startdepth = depth;
    for (i = 0; i < b->b_iused; i++) {
        instr = &b->b_instr[i];
        depth += opcode_stack_effect(instr->i_opcode, instr->i_oparg);
        if (depth > maxdepth)
            maxdepth = depth;
        assert(depth >= 0); /* invalid code or bug in stackdepth() */
        if (instr->i_jrel || instr->i_jabs) {
            target_depth = depth;
            if (instr->i_opcode == FOR_ITER) {
                target_depth = depth-2;
            } else if (instr->i_opcode == SETUP_FINALLY ||
                       instr->i_opcode == SETUP_EXCEPT) {
                target_depth = depth+3;
                if (target_depth > maxdepth)
                    maxdepth = target_depth;
            }
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
    a->a_bytecode = PyString_FromStringAndSize(NULL, DEFAULT_CODE_SIZE);
    if (!a->a_bytecode)
        return 0;
    a->a_lnotab = PyString_FromStringAndSize(NULL, DEFAULT_LNOTAB_SIZE);
    if (!a->a_lnotab)
        return 0;
    if (nblocks > PY_SIZE_MAX / sizeof(basicblock *)) {
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

/* Return the size of a basic block in bytes. */

static int
instrsize(struct instr *instr)
{
    if (!instr->i_hasarg)
        return 1;               /* 1 byte for the opcode*/
    if (instr->i_oparg > 0xffff)
        return 6;               /* 1 (opcode) + 1 (EXTENDED_ARG opcode) + 2 (oparg) + 2(oparg extended) */
    return 3;                   /* 1 (opcode) + 2 (oparg) */
}

static int
blocksize(basicblock *b)
{
    int i;
    int size = 0;

    for (i = 0; i < b->b_iused; i++)
        size += instrsize(&b->b_instr[i]);
    return size;
}

/* Appends a pair to the end of the line number table, a_lnotab, representing
   the instruction's bytecode offset and line number.  See
   Objects/lnotab_notes.txt for the description of the line number table. */

static int
assemble_lnotab(struct assembler *a, struct instr *i)
{
    int d_bytecode, d_lineno;
    int len;
    unsigned char *lnotab;

    d_bytecode = a->a_offset - a->a_lineno_off;
    d_lineno = i->i_lineno - a->a_lineno;

    assert(d_bytecode >= 0);
    assert(d_lineno >= 0);

    if(d_bytecode == 0 && d_lineno == 0)
        return 1;

    if (d_bytecode > 255) {
        int j, nbytes, ncodes = d_bytecode / 255;
        nbytes = a->a_lnotab_off + 2 * ncodes;
        len = PyString_GET_SIZE(a->a_lnotab);
        if (nbytes >= len) {
            if ((len <= INT_MAX / 2) && (len * 2 < nbytes))
                len = nbytes;
            else if (len <= INT_MAX / 2)
                len *= 2;
            else {
                PyErr_NoMemory();
                return 0;
            }
            if (_PyString_Resize(&a->a_lnotab, len) < 0)
                return 0;
        }
        lnotab = (unsigned char *)
                   PyString_AS_STRING(a->a_lnotab) + a->a_lnotab_off;
        for (j = 0; j < ncodes; j++) {
            *lnotab++ = 255;
            *lnotab++ = 0;
        }
        d_bytecode -= ncodes * 255;
        a->a_lnotab_off += ncodes * 2;
    }
    assert(d_bytecode <= 255);
    if (d_lineno > 255) {
        int j, nbytes, ncodes = d_lineno / 255;
        nbytes = a->a_lnotab_off + 2 * ncodes;
        len = PyString_GET_SIZE(a->a_lnotab);
        if (nbytes >= len) {
            if ((len <= INT_MAX / 2) && len * 2 < nbytes)
                len = nbytes;
            else if (len <= INT_MAX / 2)
                len *= 2;
            else {
                PyErr_NoMemory();
                return 0;
            }
            if (_PyString_Resize(&a->a_lnotab, len) < 0)
                return 0;
        }
        lnotab = (unsigned char *)
                   PyString_AS_STRING(a->a_lnotab) + a->a_lnotab_off;
        *lnotab++ = d_bytecode;
        *lnotab++ = 255;
        d_bytecode = 0;
        for (j = 1; j < ncodes; j++) {
            *lnotab++ = 0;
            *lnotab++ = 255;
        }
        d_lineno -= ncodes * 255;
        a->a_lnotab_off += ncodes * 2;
    }

    len = PyString_GET_SIZE(a->a_lnotab);
    if (a->a_lnotab_off + 2 >= len) {
        if (_PyString_Resize(&a->a_lnotab, len * 2) < 0)
            return 0;
    }
    lnotab = (unsigned char *)
                    PyString_AS_STRING(a->a_lnotab) + a->a_lnotab_off;

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
    int size, arg = 0, ext = 0;
    Py_ssize_t len = PyString_GET_SIZE(a->a_bytecode);
    char *code;

    size = instrsize(i);
    if (i->i_hasarg) {
        arg = i->i_oparg;
        ext = arg >> 16;
    }
    if (i->i_lineno && !assemble_lnotab(a, i))
        return 0;
    if (a->a_offset + size >= len) {
        if (len > PY_SSIZE_T_MAX / 2)
            return 0;
        if (_PyString_Resize(&a->a_bytecode, len * 2) < 0)
            return 0;
    }
    code = PyString_AS_STRING(a->a_bytecode) + a->a_offset;
    a->a_offset += size;
    if (size == 6) {
        assert(i->i_hasarg);
        *code++ = (char)EXTENDED_ARG;
        *code++ = ext & 0xff;
        *code++ = ext >> 8;
        arg &= 0xffff;
    }
    *code++ = i->i_opcode;
    if (i->i_hasarg) {
        assert(size == 3 || size == 6);
        *code++ = arg & 0xff;
        *code++ = arg >> 8;
    }
    return 1;
}

static void
assemble_jump_offsets(struct assembler *a, struct compiler *c)
{
    basicblock *b;
    int bsize, totsize, extended_arg_count = 0, last_extended_arg_count;
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
        last_extended_arg_count = extended_arg_count;
        extended_arg_count = 0;
        for (b = c->u->u_blocks; b != NULL; b = b->b_list) {
            bsize = b->b_offset;
            for (i = 0; i < b->b_iused; i++) {
                struct instr *instr = &b->b_instr[i];
                /* Relative jumps are computed relative to
                   the instruction pointer after fetching
                   the jump instruction.
                */
                bsize += instrsize(instr);
                if (instr->i_jabs)
                    instr->i_oparg = instr->i_target->b_offset;
                else if (instr->i_jrel) {
                    int delta = instr->i_target->b_offset - bsize;
                    instr->i_oparg = delta;
                }
                else
                    continue;
                if (instr->i_oparg > 0xffff)
                    extended_arg_count++;
            }
        }

    /* XXX: This is an awful hack that could hurt performance, but
        on the bright side it should work until we come up
        with a better solution.

        The issue is that in the first loop blocksize() is called
        which calls instrsize() which requires i_oparg be set
        appropriately.          There is a bootstrap problem because
        i_oparg is calculated in the second loop above.

        So we loop until we stop seeing new EXTENDED_ARGs.
        The only EXTENDED_ARGs that could be popping up are
        ones in jump instructions.  So this should converge
        fairly quickly.
    */
    } while (last_extended_arg_count != extended_arg_count);
}

static PyObject *
dict_keys_inorder(PyObject *dict, int offset)
{
    PyObject *tuple, *k, *v;
    Py_ssize_t i, pos = 0, size = PyDict_Size(dict);

    tuple = PyTuple_New(size);
    if (tuple == NULL)
        return NULL;
    while (PyDict_Next(dict, &pos, &k, &v)) {
        i = PyInt_AS_LONG(v);
        /* The keys of the dictionary are tuples. (see compiler_add_o)
           The object we want is always first, though. */
        k = PyTuple_GET_ITEM(k, 0);
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
    int flags = 0, n;
    if (ste->ste_type != ModuleBlock)
        flags |= CO_NEWLOCALS;
    if (ste->ste_type == FunctionBlock) {
        if (!ste->ste_unoptimized)
            flags |= CO_OPTIMIZED;
        if (ste->ste_nested)
            flags |= CO_NESTED;
        if (ste->ste_generator)
            flags |= CO_GENERATOR;
        if (ste->ste_varargs)
            flags |= CO_VARARGS;
        if (ste->ste_varkeywords)
            flags |= CO_VARKEYWORDS;
    }

    /* (Only) inherit compilerflags in PyCF_MASK */
    flags |= (c->c_flags->cf_flags & PyCF_MASK);

    n = PyDict_Size(c->u->u_freevars);
    if (n < 0)
        return -1;
    if (n == 0) {
        n = PyDict_Size(c->u->u_cellvars);
        if (n < 0)
        return -1;
        if (n == 0) {
        flags |= CO_NOFREE;
        }
    }

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
    PyObject *filename = NULL;
    PyObject *name = NULL;
    PyObject *freevars = NULL;
    PyObject *cellvars = NULL;
    PyObject *bytecode = NULL;
    int nlocals, flags;

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
    filename = PyString_FromString(c->c_filename);
    if (!filename)
        goto error;

    nlocals = PyDict_Size(c->u->u_varnames);
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

    co = PyCode_New(c->u->u_argcount, nlocals, stackdepth(c), flags,
                    bytecode, consts, names, varnames,
                    freevars, cellvars,
                    filename, c->u->u_name,
                    c->u->u_firstlineno,
                    a->a_lnotab);
 error:
    Py_XDECREF(consts);
    Py_XDECREF(names);
    Py_XDECREF(varnames);
    Py_XDECREF(filename);
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
    if (i->i_hasarg)
        sprintf(arg, "arg: %d ", i->i_oparg);

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
        if (entryblock && entryblock->b_instr)
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

    if (_PyString_Resize(&a.a_lnotab, a.a_lnotab_off) < 0)
        goto error;
    if (_PyString_Resize(&a.a_bytecode, a.a_offset) < 0)
        goto error;

    co = makecode(c, &a);
 error:
    assemble_free(&a);
    return co;
}
