/* Frame object implementation */

#include "Python.h"
#include "pycore_ceval.h"         // _PyEval_BuiltinsFromGlobals()
#include "pycore_object.h"        // _PyObject_GC_UNTRACK()

#include "frameobject.h"          // PyFrameObject
#include "opcode.h"               // EXTENDED_ARG
#include "structmember.h"         // PyMemberDef

#define OFF(x) offsetof(PyFrameObject, x)

static PyMemberDef frame_memberlist[] = {
    {"f_back",          T_OBJECT,       OFF(f_back),      READONLY},
    {"f_code",          T_OBJECT,       OFF(f_code),      READONLY},
    {"f_builtins",      T_OBJECT,       OFF(f_builtins),  READONLY},
    {"f_globals",       T_OBJECT,       OFF(f_globals),   READONLY},
    {"f_trace_lines",   T_BOOL,         OFF(f_trace_lines), 0},
    {"f_trace_opcodes", T_BOOL,         OFF(f_trace_opcodes), 0},
    {NULL}      /* Sentinel */
};

static struct _Py_frame_state *
get_frame_state(void)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    return &interp->frame;
}


static PyObject *
frame_getlocals(PyFrameObject *f, void *closure)
{
    if (PyFrame_FastToLocalsWithError(f) < 0)
        return NULL;
    Py_INCREF(f->f_locals);
    return f->f_locals;
}

int
PyFrame_GetLineNumber(PyFrameObject *f)
{
    assert(f != NULL);
    if (f->f_lineno != 0) {
        return f->f_lineno;
    }
    else {
        return PyCode_Addr2Line(f->f_code, f->f_lasti*2);
    }
}

static PyObject *
frame_getlineno(PyFrameObject *f, void *closure)
{
    return PyLong_FromLong(PyFrame_GetLineNumber(f));
}

static PyObject *
frame_getlasti(PyFrameObject *f, void *closure)
{
    if (f->f_lasti < 0) {
        return PyLong_FromLong(-1);
    }
    return PyLong_FromLong(f->f_lasti*2);
}


/* Given the index of the effective opcode,
   scan back to construct the oparg with EXTENDED_ARG */
static unsigned int
get_arg(const _Py_CODEUNIT *codestr, Py_ssize_t i)
{
    _Py_CODEUNIT word;
    unsigned int oparg = _Py_OPARG(codestr[i]);
    if (i >= 1 && _Py_OPCODE(word = codestr[i-1]) == EXTENDED_ARG) {
        oparg |= _Py_OPARG(word) << 8;
        if (i >= 2 && _Py_OPCODE(word = codestr[i-2]) == EXTENDED_ARG) {
            oparg |= _Py_OPARG(word) << 16;
            if (i >= 3 && _Py_OPCODE(word = codestr[i-3]) == EXTENDED_ARG) {
                oparg |= _Py_OPARG(word) << 24;
            }
        }
    }
    return oparg;
}

typedef enum kind {
    With = 1,
    Loop = 2,
    Try = 3,
    Except = 4,
} Kind;

#define BITS_PER_BLOCK 3

static inline int64_t
push_block(int64_t stack, Kind kind)
{
    assert(stack < ((int64_t)1)<<(BITS_PER_BLOCK*CO_MAXBLOCKS));
    return (stack << BITS_PER_BLOCK) | kind;
}

static inline int64_t
pop_block(int64_t stack)
{
    assert(stack > 0);
    return stack >> BITS_PER_BLOCK;
}

static inline Kind
top_block(int64_t stack)
{
    return stack & ((1<<BITS_PER_BLOCK)-1);
}

static int64_t *
markblocks(PyCodeObject *code_obj, int len)
{
    const _Py_CODEUNIT *code =
        (const _Py_CODEUNIT *)PyBytes_AS_STRING(code_obj->co_code);
    int64_t *blocks = PyMem_New(int64_t, len+1);
    int i, j, opcode;

    if (blocks == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    memset(blocks, -1, (len+1)*sizeof(int64_t));
    blocks[0] = 0;
    int todo = 1;
    while (todo) {
        todo = 0;
        for (i = 0; i < len; i++) {
            int64_t block_stack = blocks[i];
            int64_t except_stack;
            if (block_stack == -1) {
                continue;
            }
            opcode = _Py_OPCODE(code[i]);
            switch (opcode) {
                case JUMP_IF_FALSE_OR_POP:
                case JUMP_IF_TRUE_OR_POP:
                case POP_JUMP_IF_FALSE:
                case POP_JUMP_IF_TRUE:
                case JUMP_IF_NOT_EXC_MATCH:
                    j = get_arg(code, i);
                    assert(j < len);
                    if (blocks[j] == -1 && j < i) {
                        todo = 1;
                    }
                    assert(blocks[j] == -1 || blocks[j] == block_stack);
                    blocks[j] = block_stack;
                    blocks[i+1] = block_stack;
                    break;
                case JUMP_ABSOLUTE:
                    j = get_arg(code, i);
                    assert(j < len);
                    if (blocks[j] == -1 && j < i) {
                        todo = 1;
                    }
                    assert(blocks[j] == -1 || blocks[j] == block_stack);
                    blocks[j] = block_stack;
                    break;
                case SETUP_FINALLY:
                    j = get_arg(code, i) + i + 1;
                    assert(j < len);
                    except_stack = push_block(block_stack, Except);
                    assert(blocks[j] == -1 || blocks[j] == except_stack);
                    blocks[j] = except_stack;
                    block_stack = push_block(block_stack, Try);
                    blocks[i+1] = block_stack;
                    break;
                case SETUP_WITH:
                case SETUP_ASYNC_WITH:
                    j = get_arg(code, i) + i + 1;
                    assert(j < len);
                    except_stack = push_block(block_stack, Except);
                    assert(blocks[j] == -1 || blocks[j] == except_stack);
                    blocks[j] = except_stack;
                    block_stack = push_block(block_stack, With);
                    blocks[i+1] = block_stack;
                    break;
                case JUMP_FORWARD:
                    j = get_arg(code, i) + i + 1;
                    assert(j < len);
                    assert(blocks[j] == -1 || blocks[j] == block_stack);
                    blocks[j] = block_stack;
                    break;
                case GET_ITER:
                case GET_AITER:
                    block_stack = push_block(block_stack, Loop);
                    blocks[i+1] = block_stack;
                    break;
                case FOR_ITER:
                    blocks[i+1] = block_stack;
                    block_stack = pop_block(block_stack);
                    j = get_arg(code, i) + i + 1;
                    assert(j < len);
                    assert(blocks[j] == -1 || blocks[j] == block_stack);
                    blocks[j] = block_stack;
                    break;
                case POP_BLOCK:
                case POP_EXCEPT:
                    block_stack = pop_block(block_stack);
                    blocks[i+1] = block_stack;
                    break;
                case END_ASYNC_FOR:
                    block_stack = pop_block(pop_block(block_stack));
                    blocks[i+1] = block_stack;
                    break;
                case RETURN_VALUE:
                case RAISE_VARARGS:
                case RERAISE:
                    /* End of block */
                    break;
                default:
                    blocks[i+1] = block_stack;

            }
        }
    }
    return blocks;
}

static int
compatible_block_stack(int64_t from_stack, int64_t to_stack)
{
    if (to_stack < 0) {
        return 0;
    }
    while(from_stack > to_stack) {
        from_stack = pop_block(from_stack);
    }
    return from_stack == to_stack;
}

static const char *
explain_incompatible_block_stack(int64_t to_stack)
{
    Kind target_kind = top_block(to_stack);
    switch(target_kind) {
        case Except:
            return "can't jump into an 'except' block as there's no exception";
        case Try:
            return "can't jump into the body of a try statement";
        case With:
            return "can't jump into the body of a with statement";
        case Loop:
            return "can't jump into the body of a for loop";
        default:
            Py_UNREACHABLE();
    }
}

static int *
marklines(PyCodeObject *code, int len)
{
    PyCodeAddressRange bounds;
    _PyCode_InitAddressRange(code, &bounds);
    assert (bounds.ar_end == 0);

    int *linestarts = PyMem_New(int, len);
    if (linestarts == NULL) {
        return NULL;
    }
    for (int i = 0; i < len; i++) {
        linestarts[i] = -1;
    }

    while (PyLineTable_NextAddressRange(&bounds)) {
        assert(bounds.ar_start/2 < len);
        linestarts[bounds.ar_start/2] = bounds.ar_line;
    }
    return linestarts;
}

static int
first_line_not_before(int *lines, int len, int line)
{
    int result = INT_MAX;
    for (int i = 0; i < len; i++) {
        if (lines[i] < result && lines[i] >= line) {
            result = lines[i];
        }
    }
    if (result == INT_MAX) {
        return -1;
    }
    return result;
}

static void
frame_stack_pop(PyFrameObject *f)
{
    assert(f->f_stackdepth >= 0);
    f->f_stackdepth--;
    PyObject *v = f->f_valuestack[f->f_stackdepth];
    Py_DECREF(v);
}

static void
frame_block_unwind(PyFrameObject *f)
{
    assert(f->f_stackdepth >= 0);
    assert(f->f_iblock > 0);
    f->f_iblock--;
    PyTryBlock *b = &f->f_blockstack[f->f_iblock];
    intptr_t delta = f->f_stackdepth - b->b_level;
    while (delta > 0) {
        frame_stack_pop(f);
        delta--;
    }
}


/* Setter for f_lineno - you can set f_lineno from within a trace function in
 * order to jump to a given line of code, subject to some restrictions.  Most
 * lines are OK to jump to because they don't make any assumptions about the
 * state of the stack (obvious because you could remove the line and the code
 * would still work without any stack errors), but there are some constructs
 * that limit jumping:
 *
 *  o Lines with an 'except' statement on them can't be jumped to, because
 *    they expect an exception to be on the top of the stack.
 *  o Lines that live in a 'finally' block can't be jumped from or to, since
 *    we cannot be sure which state the interpreter was in or would be in
 *    during execution of the finally block.
 *  o 'try', 'with' and 'async with' blocks can't be jumped into because
 *    the blockstack needs to be set up before their code runs.
 *  o 'for' and 'async for' loops can't be jumped into because the
 *    iterator needs to be on the stack.
 *  o Jumps cannot be made from within a trace function invoked with a
 *    'return' or 'exception' event since the eval loop has been exited at
 *    that time.
 */
static int
frame_setlineno(PyFrameObject *f, PyObject* p_new_lineno, void *Py_UNUSED(ignored))
{
    if (p_new_lineno == NULL) {
        PyErr_SetString(PyExc_AttributeError, "cannot delete attribute");
        return -1;
    }
    /* f_lineno must be an integer. */
    if (!PyLong_CheckExact(p_new_lineno)) {
        PyErr_SetString(PyExc_ValueError,
                        "lineno must be an integer");
        return -1;
    }

    /*
     * This code preserves the historical restrictions on
     * setting the line number of a frame.
     * Jumps are forbidden on a 'return' trace event (except after a yield).
     * Jumps from 'call' trace events are also forbidden.
     * In addition, jumps are forbidden when not tracing,
     * as this is a debugging feature.
     */
    switch(f->f_state) {
        case FRAME_CREATED:
            PyErr_Format(PyExc_ValueError,
                     "can't jump from the 'call' trace event of a new frame");
            return -1;
        case FRAME_RETURNED:
        case FRAME_UNWINDING:
        case FRAME_RAISED:
        case FRAME_CLEARED:
            PyErr_SetString(PyExc_ValueError,
                "can only jump from a 'line' trace event");
            return -1;
        case FRAME_EXECUTING:
        case FRAME_SUSPENDED:
            /* You can only do this from within a trace function, not via
            * _getframe or similar hackery. */
            if (!f->f_trace) {
                PyErr_Format(PyExc_ValueError,
                            "f_lineno can only be set by a trace function");
                return -1;
            }
            break;
    }

    int new_lineno;

    /* Fail if the line falls outside the code block and
        select first line with actual code. */
    int overflow;
    long l_new_lineno = PyLong_AsLongAndOverflow(p_new_lineno, &overflow);
    if (overflow
#if SIZEOF_LONG > SIZEOF_INT
        || l_new_lineno > INT_MAX
        || l_new_lineno < INT_MIN
#endif
    ) {
        PyErr_SetString(PyExc_ValueError,
                        "lineno out of range");
        return -1;
    }
    new_lineno = (int)l_new_lineno;

    if (new_lineno < f->f_code->co_firstlineno) {
        PyErr_Format(PyExc_ValueError,
                    "line %d comes before the current code block",
                    new_lineno);
        return -1;
    }

    /* PyCode_NewWithPosOnlyArgs limits co_code to be under INT_MAX so this
     * should never overflow. */
    int len = (int)(PyBytes_GET_SIZE(f->f_code->co_code) / sizeof(_Py_CODEUNIT));
    int *lines = marklines(f->f_code, len);
    if (lines == NULL) {
        return -1;
    }

    new_lineno = first_line_not_before(lines, len, new_lineno);
    if (new_lineno < 0) {
        PyErr_Format(PyExc_ValueError,
                    "line %d comes after the current code block",
                    (int)l_new_lineno);
        PyMem_Free(lines);
        return -1;
    }

    int64_t *blocks = markblocks(f->f_code, len);
    if (blocks == NULL) {
        PyMem_Free(lines);
        return -1;
    }

    int64_t target_block_stack = -1;
    int64_t best_block_stack = -1;
    int best_addr = -1;
    int64_t start_block_stack = blocks[f->f_lasti];
    const char *msg = "cannot find bytecode for specified line";
    for (int i = 0; i < len; i++) {
        if (lines[i] == new_lineno) {
            target_block_stack = blocks[i];
            if (compatible_block_stack(start_block_stack, target_block_stack)) {
                msg = NULL;
                if (target_block_stack > best_block_stack) {
                    best_block_stack = target_block_stack;
                    best_addr = i;
                }
            }
            else if (msg) {
                if (target_block_stack >= 0) {
                    msg = explain_incompatible_block_stack(target_block_stack);
                }
                else {
                    msg = "code may be unreachable.";
                }
            }
        }
    }
    PyMem_Free(blocks);
    PyMem_Free(lines);
    if (msg != NULL) {
        PyErr_SetString(PyExc_ValueError, msg);
        return -1;
    }

    /* Unwind block stack. */
    while (start_block_stack > best_block_stack) {
        Kind kind = top_block(start_block_stack);
        switch(kind) {
        case Loop:
            frame_stack_pop(f);
            break;
        case Try:
            frame_block_unwind(f);
            break;
        case With:
            frame_block_unwind(f);
            // Pop the exit function
            frame_stack_pop(f);
            break;
        case Except:
            PyErr_SetString(PyExc_ValueError,
                "can't jump out of an 'except' block");
            return -1;
        }
        start_block_stack = pop_block(start_block_stack);
    }

    /* Finally set the new f_lasti and return OK. */
    f->f_lineno = 0;
    f->f_lasti = best_addr;
    return 0;
}

static PyObject *
frame_gettrace(PyFrameObject *f, void *closure)
{
    PyObject* trace = f->f_trace;

    if (trace == NULL)
        trace = Py_None;

    Py_INCREF(trace);

    return trace;
}

static int
frame_settrace(PyFrameObject *f, PyObject* v, void *closure)
{
    if (v == Py_None) {
        v = NULL;
    }
    Py_XINCREF(v);
    Py_XSETREF(f->f_trace, v);

    return 0;
}


static PyGetSetDef frame_getsetlist[] = {
    {"f_locals",        (getter)frame_getlocals, NULL, NULL},
    {"f_lineno",        (getter)frame_getlineno,
                    (setter)frame_setlineno, NULL},
    {"f_trace",         (getter)frame_gettrace, (setter)frame_settrace, NULL},
    {"f_lasti",         (getter)frame_getlasti, NULL, NULL},
    {0}
};

/* Stack frames are allocated and deallocated at a considerable rate.
   In an attempt to improve the speed of function calls, we:

   1. Hold a single "zombie" frame on each code object. This retains
   the allocated and initialised frame object from an invocation of
   the code object. The zombie is reanimated the next time we need a
   frame object for that code object. Doing this saves the malloc/
   realloc required when using a free_list frame that isn't the
   correct size. It also saves some field initialisation.

   In zombie mode, no field of PyFrameObject holds a reference, but
   the following fields are still valid:

     * ob_type, ob_size, f_code, f_valuestack;

     * f_locals, f_trace are NULL;

     * f_localsplus does not require re-allocation and
       the local variables in f_localsplus are NULL.

   2. We also maintain a separate free list of stack frames (just like
   floats are allocated in a special way -- see floatobject.c).  When
   a stack frame is on the free list, only the following members have
   a meaning:
    ob_type             == &Frametype
    f_back              next item on free list, or NULL
    f_stacksize         size of value stack
    ob_size             size of localsplus
   Note that the value and block stacks are preserved -- this can save
   another malloc() call or two (and two free() calls as well!).
   Also note that, unlike for integers, each frame object is a
   malloc'ed object in its own right -- it is only the actual calls to
   malloc() that we are trying to save here, not the administration.
   After all, while a typical program may make millions of calls, a
   call depth of more than 20 or 30 is probably already exceptional
   unless the program contains run-away recursion.  I hope.

   Later, PyFrame_MAXFREELIST was added to bound the # of frames saved on
   free_list.  Else programs creating lots of cyclic trash involving
   frames could provoke free_list into growing without bound.
*/
/* max value for numfree */
#define PyFrame_MAXFREELIST 200

static void _Py_HOT_FUNCTION
frame_dealloc(PyFrameObject *f)
{
    if (_PyObject_GC_IS_TRACKED(f)) {
        _PyObject_GC_UNTRACK(f);
    }

    Py_TRASHCAN_SAFE_BEGIN(f)
    /* Kill all local variables */
    PyObject **valuestack = f->f_valuestack;
    for (PyObject **p = f->f_localsplus; p < valuestack; p++) {
        Py_CLEAR(*p);
    }

    /* Free stack */
    for (int i = 0; i < f->f_stackdepth; i++) {
        Py_XDECREF(f->f_valuestack[i]);
    }
    f->f_stackdepth = 0;

    Py_XDECREF(f->f_back);
    Py_DECREF(f->f_builtins);
    Py_DECREF(f->f_globals);
    Py_CLEAR(f->f_locals);
    Py_CLEAR(f->f_trace);

    PyCodeObject *co = f->f_code;
    if (co->co_zombieframe == NULL) {
        co->co_zombieframe = f;
    }
    else {
        struct _Py_frame_state *state = get_frame_state();
#ifdef Py_DEBUG
        // frame_dealloc() must not be called after _PyFrame_Fini()
        assert(state->numfree != -1);
#endif
        if (state->numfree < PyFrame_MAXFREELIST) {
            ++state->numfree;
            f->f_back = state->free_list;
            state->free_list = f;
        }
        else {
            PyObject_GC_Del(f);
        }
    }

    Py_DECREF(co);
    Py_TRASHCAN_SAFE_END(f)
}

static inline Py_ssize_t
frame_nslots(PyFrameObject *frame)
{
    PyCodeObject *code = frame->f_code;
    return (code->co_nlocals
            + PyTuple_GET_SIZE(code->co_cellvars)
            + PyTuple_GET_SIZE(code->co_freevars));
}

static int
frame_traverse(PyFrameObject *f, visitproc visit, void *arg)
{
    Py_VISIT(f->f_back);
    Py_VISIT(f->f_code);
    Py_VISIT(f->f_builtins);
    Py_VISIT(f->f_globals);
    Py_VISIT(f->f_locals);
    Py_VISIT(f->f_trace);

    /* locals */
    PyObject **fastlocals = f->f_localsplus;
    for (Py_ssize_t i = frame_nslots(f); --i >= 0; ++fastlocals) {
        Py_VISIT(*fastlocals);
    }

    /* stack */
    for (int i = 0; i < f->f_stackdepth; i++) {
        Py_VISIT(f->f_valuestack[i]);
    }
    return 0;
}

static int
frame_tp_clear(PyFrameObject *f)
{
    /* Before anything else, make sure that this frame is clearly marked
     * as being defunct!  Else, e.g., a generator reachable from this
     * frame may also point to this frame, believe itself to still be
     * active, and try cleaning up this frame again.
     */
    f->f_state = FRAME_CLEARED;

    Py_CLEAR(f->f_trace);

    /* locals */
    PyObject **fastlocals = f->f_localsplus;
    for (Py_ssize_t i = frame_nslots(f); --i >= 0; ++fastlocals) {
        Py_CLEAR(*fastlocals);
    }

    /* stack */
    for (int i = 0; i < f->f_stackdepth; i++) {
        Py_CLEAR(f->f_valuestack[i]);
    }
    f->f_stackdepth = 0;
    return 0;
}

static PyObject *
frame_clear(PyFrameObject *f, PyObject *Py_UNUSED(ignored))
{
    if (_PyFrame_IsExecuting(f)) {
        PyErr_SetString(PyExc_RuntimeError,
                        "cannot clear an executing frame");
        return NULL;
    }
    if (f->f_gen) {
        _PyGen_Finalize(f->f_gen);
        assert(f->f_gen == NULL);
    }
    (void)frame_tp_clear(f);
    Py_RETURN_NONE;
}

PyDoc_STRVAR(clear__doc__,
"F.clear(): clear most references held by the frame");

static PyObject *
frame_sizeof(PyFrameObject *f, PyObject *Py_UNUSED(ignored))
{
    Py_ssize_t res, extras, ncells, nfrees;

    PyCodeObject *code = f->f_code;
    ncells = PyTuple_GET_SIZE(code->co_cellvars);
    nfrees = PyTuple_GET_SIZE(code->co_freevars);
    extras = code->co_stacksize + code->co_nlocals + ncells + nfrees;
    /* subtract one as it is already included in PyFrameObject */
    res = sizeof(PyFrameObject) + (extras-1) * sizeof(PyObject *);

    return PyLong_FromSsize_t(res);
}

PyDoc_STRVAR(sizeof__doc__,
"F.__sizeof__() -> size of F in memory, in bytes");

static PyObject *
frame_repr(PyFrameObject *f)
{
    int lineno = PyFrame_GetLineNumber(f);
    PyCodeObject *code = f->f_code;
    return PyUnicode_FromFormat(
        "<frame at %p, file %R, line %d, code %S>",
        f, code->co_filename, lineno, code->co_name);
}

static PyMethodDef frame_methods[] = {
    {"clear",           (PyCFunction)frame_clear,       METH_NOARGS,
     clear__doc__},
    {"__sizeof__",      (PyCFunction)frame_sizeof,      METH_NOARGS,
     sizeof__doc__},
    {NULL,              NULL}   /* sentinel */
};

PyTypeObject PyFrame_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "frame",
    sizeof(PyFrameObject),
    sizeof(PyObject *),
    (destructor)frame_dealloc,                  /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    (reprfunc)frame_repr,                       /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    PyObject_GenericSetAttr,                    /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,/* tp_flags */
    0,                                          /* tp_doc */
    (traverseproc)frame_traverse,               /* tp_traverse */
    (inquiry)frame_tp_clear,                    /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    frame_methods,                              /* tp_methods */
    frame_memberlist,                           /* tp_members */
    frame_getsetlist,                           /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
};

_Py_IDENTIFIER(__builtins__);

static inline PyFrameObject*
frame_alloc(PyCodeObject *code)
{
    PyFrameObject *f = code->co_zombieframe;
    if (f != NULL) {
        code->co_zombieframe = NULL;
        _Py_NewReference((PyObject *)f);
        assert(f->f_code == code);
        return f;
    }

    Py_ssize_t ncells = PyTuple_GET_SIZE(code->co_cellvars);
    Py_ssize_t nfrees = PyTuple_GET_SIZE(code->co_freevars);
    Py_ssize_t extras = code->co_stacksize + code->co_nlocals + ncells + nfrees;
    struct _Py_frame_state *state = get_frame_state();
    if (state->free_list == NULL)
    {
        f = PyObject_GC_NewVar(PyFrameObject, &PyFrame_Type, extras);
        if (f == NULL) {
            return NULL;
        }
    }
    else {
#ifdef Py_DEBUG
        // frame_alloc() must not be called after _PyFrame_Fini()
        assert(state->numfree != -1);
#endif
        assert(state->numfree > 0);
        --state->numfree;
        f = state->free_list;
        state->free_list = state->free_list->f_back;
        if (Py_SIZE(f) < extras) {
            PyFrameObject *new_f = PyObject_GC_Resize(PyFrameObject, f, extras);
            if (new_f == NULL) {
                PyObject_GC_Del(f);
                return NULL;
            }
            f = new_f;
        }
        _Py_NewReference((PyObject *)f);
    }

    extras = code->co_nlocals + ncells + nfrees;
    f->f_valuestack = f->f_localsplus + extras;
    for (Py_ssize_t i=0; i < extras; i++) {
        f->f_localsplus[i] = NULL;
    }
    return f;
}


PyFrameObject* _Py_HOT_FUNCTION
_PyFrame_New_NoTrack(PyThreadState *tstate, PyFrameConstructor *con, PyObject *locals)
{
    assert(con != NULL);
    assert(con->fc_globals != NULL);
    assert(con->fc_builtins != NULL);
    assert(con->fc_code != NULL);
    assert(locals == NULL || PyMapping_Check(locals));

    PyFrameObject *f = frame_alloc((PyCodeObject *)con->fc_code);
    if (f == NULL) {
        return NULL;
    }

    f->f_back = (PyFrameObject*)Py_XNewRef(tstate->frame);
    f->f_code = (PyCodeObject *)Py_NewRef(con->fc_code);
    f->f_builtins = Py_NewRef(con->fc_builtins);
    f->f_globals = Py_NewRef(con->fc_globals);
    f->f_locals = Py_XNewRef(locals);
    // f_valuestack initialized by frame_alloc()
    f->f_trace = NULL;
    f->f_stackdepth = 0;
    f->f_trace_lines = 1;
    f->f_trace_opcodes = 0;
    f->f_gen = NULL;
    f->f_lasti = -1;
    f->f_lineno = 0;
    f->f_iblock = 0;
    f->f_state = FRAME_CREATED;
    // f_blockstack and f_localsplus initialized by frame_alloc()
    return f;
}

/* Legacy API */
PyFrameObject*
PyFrame_New(PyThreadState *tstate, PyCodeObject *code,
            PyObject *globals, PyObject *locals)
{
    PyObject *builtins = _PyEval_BuiltinsFromGlobals(tstate, globals); // borrowed ref
    if (builtins == NULL) {
        return NULL;
    }
    PyFrameConstructor desc = {
        .fc_globals = globals,
        .fc_builtins = builtins,
        .fc_name = code->co_name,
        .fc_qualname = code->co_name,
        .fc_code = (PyObject *)code,
        .fc_defaults = NULL,
        .fc_kwdefaults = NULL,
        .fc_closure = NULL
    };
    PyFrameObject *f = _PyFrame_New_NoTrack(tstate, &desc, locals);
    if (f) {
        _PyObject_GC_TRACK(f);
    }
    return f;
}


/* Block management */

void
PyFrame_BlockSetup(PyFrameObject *f, int type, int handler, int level)
{
    PyTryBlock *b;
    if (f->f_iblock >= CO_MAXBLOCKS) {
        Py_FatalError("block stack overflow");
    }
    b = &f->f_blockstack[f->f_iblock++];
    b->b_type = type;
    b->b_level = level;
    b->b_handler = handler;
}

PyTryBlock *
PyFrame_BlockPop(PyFrameObject *f)
{
    PyTryBlock *b;
    if (f->f_iblock <= 0) {
        Py_FatalError("block stack underflow");
    }
    b = &f->f_blockstack[--f->f_iblock];
    return b;
}

/* Convert between "fast" version of locals and dictionary version.

   map and values are input arguments.  map is a tuple of strings.
   values is an array of PyObject*.  At index i, map[i] is the name of
   the variable with value values[i].  The function copies the first
   nmap variable from map/values into dict.  If values[i] is NULL,
   the variable is deleted from dict.

   If deref is true, then the values being copied are cell variables
   and the value is extracted from the cell variable before being put
   in dict.
 */

static int
map_to_dict(PyObject *map, Py_ssize_t nmap, PyObject *dict, PyObject **values,
            int deref)
{
    Py_ssize_t j;
    assert(PyTuple_Check(map));
    assert(PyDict_Check(dict));
    assert(PyTuple_Size(map) >= nmap);
    for (j=0; j < nmap; j++) {
        PyObject *key = PyTuple_GET_ITEM(map, j);
        PyObject *value = values[j];
        assert(PyUnicode_Check(key));
        if (deref && value != NULL) {
            assert(PyCell_Check(value));
            value = PyCell_GET(value);
        }
        if (value == NULL) {
            if (PyObject_DelItem(dict, key) != 0) {
                if (PyErr_ExceptionMatches(PyExc_KeyError))
                    PyErr_Clear();
                else
                    return -1;
            }
        }
        else {
            if (PyObject_SetItem(dict, key, value) != 0)
                return -1;
        }
    }
    return 0;
}

/* Copy values from the "locals" dict into the fast locals.

   dict is an input argument containing string keys representing
   variables names and arbitrary PyObject* as values.

   map and values are input arguments.  map is a tuple of strings.
   values is an array of PyObject*.  At index i, map[i] is the name of
   the variable with value values[i].  The function copies the first
   nmap variable from map/values into dict.  If values[i] is NULL,
   the variable is deleted from dict.

   If deref is true, then the values being copied are cell variables
   and the value is extracted from the cell variable before being put
   in dict.  If clear is true, then variables in map but not in dict
   are set to NULL in map; if clear is false, variables missing in
   dict are ignored.

   Exceptions raised while modifying the dict are silently ignored,
   because there is no good way to report them.
*/

static void
dict_to_map(PyObject *map, Py_ssize_t nmap, PyObject *dict, PyObject **values,
            int deref, int clear)
{
    Py_ssize_t j;
    assert(PyTuple_Check(map));
    assert(PyDict_Check(dict));
    assert(PyTuple_Size(map) >= nmap);
    for (j=0; j < nmap; j++) {
        PyObject *key = PyTuple_GET_ITEM(map, j);
        PyObject *value = PyObject_GetItem(dict, key);
        assert(PyUnicode_Check(key));
        /* We only care about NULLs if clear is true. */
        if (value == NULL) {
            PyErr_Clear();
            if (!clear)
                continue;
        }
        if (deref) {
            assert(PyCell_Check(values[j]));
            if (PyCell_GET(values[j]) != value) {
                if (PyCell_Set(values[j], value) < 0)
                    PyErr_Clear();
            }
        } else if (values[j] != value) {
            Py_XINCREF(value);
            Py_XSETREF(values[j], value);
        }
        Py_XDECREF(value);
    }
}

int
PyFrame_FastToLocalsWithError(PyFrameObject *f)
{
    /* Merge fast locals into f->f_locals */
    PyObject *locals, *map;
    PyObject **fast;
    PyCodeObject *co;
    Py_ssize_t j;
    Py_ssize_t ncells, nfreevars;

    if (f == NULL) {
        PyErr_BadInternalCall();
        return -1;
    }
    locals = f->f_locals;
    if (locals == NULL) {
        locals = f->f_locals = PyDict_New();
        if (locals == NULL)
            return -1;
    }
    co = f->f_code;
    map = co->co_varnames;
    if (!PyTuple_Check(map)) {
        PyErr_Format(PyExc_SystemError,
                     "co_varnames must be a tuple, not %s",
                     Py_TYPE(map)->tp_name);
        return -1;
    }
    fast = f->f_localsplus;
    j = PyTuple_GET_SIZE(map);
    if (j > co->co_nlocals)
        j = co->co_nlocals;
    if (co->co_nlocals) {
        if (map_to_dict(map, j, locals, fast, 0) < 0)
            return -1;
    }
    ncells = PyTuple_GET_SIZE(co->co_cellvars);
    nfreevars = PyTuple_GET_SIZE(co->co_freevars);
    if (ncells || nfreevars) {
        if (map_to_dict(co->co_cellvars, ncells,
                        locals, fast + co->co_nlocals, 1))
            return -1;

        /* If the namespace is unoptimized, then one of the
           following cases applies:
           1. It does not contain free variables, because it
              uses import * or is a top-level namespace.
           2. It is a class namespace.
           We don't want to accidentally copy free variables
           into the locals dict used by the class.
        */
        if (co->co_flags & CO_OPTIMIZED) {
            if (map_to_dict(co->co_freevars, nfreevars,
                            locals, fast + co->co_nlocals + ncells, 1) < 0)
                return -1;
        }
    }
    return 0;
}

void
PyFrame_FastToLocals(PyFrameObject *f)
{
    int res;

    assert(!PyErr_Occurred());

    res = PyFrame_FastToLocalsWithError(f);
    if (res < 0)
        PyErr_Clear();
}

void
PyFrame_LocalsToFast(PyFrameObject *f, int clear)
{
    /* Merge f->f_locals into fast locals */
    PyObject *locals, *map;
    PyObject **fast;
    PyObject *error_type, *error_value, *error_traceback;
    PyCodeObject *co;
    Py_ssize_t j;
    Py_ssize_t ncells, nfreevars;
    if (f == NULL)
        return;
    locals = f->f_locals;
    co = f->f_code;
    map = co->co_varnames;
    if (locals == NULL)
        return;
    if (!PyTuple_Check(map))
        return;
    PyErr_Fetch(&error_type, &error_value, &error_traceback);
    fast = f->f_localsplus;
    j = PyTuple_GET_SIZE(map);
    if (j > co->co_nlocals)
        j = co->co_nlocals;
    if (co->co_nlocals)
        dict_to_map(co->co_varnames, j, locals, fast, 0, clear);
    ncells = PyTuple_GET_SIZE(co->co_cellvars);
    nfreevars = PyTuple_GET_SIZE(co->co_freevars);
    if (ncells || nfreevars) {
        dict_to_map(co->co_cellvars, ncells,
                    locals, fast + co->co_nlocals, 1, clear);
        /* Same test as in PyFrame_FastToLocals() above. */
        if (co->co_flags & CO_OPTIMIZED) {
            dict_to_map(co->co_freevars, nfreevars,
                locals, fast + co->co_nlocals + ncells, 1,
                clear);
        }
    }
    PyErr_Restore(error_type, error_value, error_traceback);
}

/* Clear out the free list */
void
_PyFrame_ClearFreeList(PyInterpreterState *interp)
{
    struct _Py_frame_state *state = &interp->frame;
    while (state->free_list != NULL) {
        PyFrameObject *f = state->free_list;
        state->free_list = state->free_list->f_back;
        PyObject_GC_Del(f);
        --state->numfree;
    }
    assert(state->numfree == 0);
}

void
_PyFrame_Fini(PyInterpreterState *interp)
{
    _PyFrame_ClearFreeList(interp);
#ifdef Py_DEBUG
    struct _Py_frame_state *state = &interp->frame;
    state->numfree = -1;
#endif
}

/* Print summary info about the state of the optimized allocator */
void
_PyFrame_DebugMallocStats(FILE *out)
{
    struct _Py_frame_state *state = get_frame_state();
    _PyDebugAllocatorStats(out,
                           "free PyFrameObject",
                           state->numfree, sizeof(PyFrameObject));
}


PyCodeObject *
PyFrame_GetCode(PyFrameObject *frame)
{
    assert(frame != NULL);
    PyCodeObject *code = frame->f_code;
    assert(code != NULL);
    Py_INCREF(code);
    return code;
}


PyFrameObject*
PyFrame_GetBack(PyFrameObject *frame)
{
    assert(frame != NULL);
    PyFrameObject *back = frame->f_back;
    Py_XINCREF(back);
    return back;
}

PyObject*
_PyEval_BuiltinsFromGlobals(PyThreadState *tstate, PyObject *globals)
{
    PyObject *builtins = _PyDict_GetItemIdWithError(globals, &PyId___builtins__);
    if (builtins) {
        if (PyModule_Check(builtins)) {
            builtins = PyModule_GetDict(builtins);
            assert(builtins != NULL);
        }
        return builtins;
    }
    if (PyErr_Occurred()) {
        return NULL;
    }

    return _PyEval_GetBuiltins(tstate);
}
