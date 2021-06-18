/* Frame object implementation */

#include "Python.h"
#include "pycore_ceval.h"         // _PyEval_BuiltinsFromGlobals()
#include "pycore_moduleobject.h"  // _PyModule_GetDict()
#include "pycore_object.h"        // _PyObject_GC_UNTRACK()
#include "pycore_code.h"          // CO_FAST_LOCAL, etc.

#include "frameobject.h"          // PyFrameObject
#include "pycore_frame.h"
#include "opcode.h"               // EXTENDED_ARG
#include "structmember.h"         // PyMemberDef

#define OFF(x) offsetof(PyFrameObject, x)

static PyMemberDef frame_memberlist[] = {
    {"f_back",          T_OBJECT,       OFF(f_back),      READONLY},
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
    PyObject *locals = _PyFrame_Specials(f)[FRAME_SPECIALS_LOCALS_OFFSET];
    Py_INCREF(locals);
    return locals;
}

int
PyFrame_GetLineNumber(PyFrameObject *f)
{
    assert(f != NULL);
    if (f->f_lineno != 0) {
        return f->f_lineno;
    }
    else {
        return PyCode_Addr2Line(_PyFrame_GetCode(f), f->f_lasti*2);
    }
}

static PyObject *
frame_getlineno(PyFrameObject *f, void *closure)
{
    int lineno = PyFrame_GetLineNumber(f);
    if (lineno < 0) {
        Py_RETURN_NONE;
    }
    else {
        return PyLong_FromLong(lineno);
    }
}

static PyObject *
frame_getlasti(PyFrameObject *f, void *closure)
{
    if (f->f_lasti < 0) {
        return PyLong_FromLong(-1);
    }
    return PyLong_FromLong(f->f_lasti*2);
}

static PyObject *
frame_getglobals(PyFrameObject *f, void *closure)
{
    PyObject *globals = _PyFrame_GetGlobals(f);
    if (globals == NULL) {
        globals = Py_None;
    }
    Py_INCREF(globals);
    return globals;
}

static PyObject *
frame_getbuiltins(PyFrameObject *f, void *closure)
{
    PyObject *builtins = _PyFrame_GetBuiltins(f);
    if (builtins == NULL) {
        builtins = Py_None;
    }
    Py_INCREF(builtins);
    return builtins;
}

static PyObject *
frame_getcode(PyFrameObject *f, void *closure)
{
    if (PySys_Audit("object.__getattr__", "Os", f, "f_code") < 0) {
        return NULL;
    }
    return (PyObject *)PyFrame_GetCode(f);
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

/* Model the evaluation stack, to determine which jumps
 * are safe and how many values needs to be popped.
 * The stack is modelled by a 64 integer, treating any
 * stack that can't fit into 64 bits as "overflowed".
 */

typedef enum kind {
    Iterator = 1,
    Except = 2,
    Object = 3,
} Kind;

#define BITS_PER_BLOCK 2

#define UNINITIALIZED -2
#define OVERFLOWED -1

#define MAX_STACK_ENTRIES (63/BITS_PER_BLOCK)
#define WILL_OVERFLOW (1ULL<<((MAX_STACK_ENTRIES-1)*BITS_PER_BLOCK))

static inline int64_t
push_value(int64_t stack, Kind kind)
{
    if (((uint64_t)stack) >= WILL_OVERFLOW) {
        return OVERFLOWED;
    }
    else {
        return (stack << BITS_PER_BLOCK) | kind;
    }
}

static inline int64_t
pop_value(int64_t stack)
{
    return Py_ARITHMETIC_RIGHT_SHIFT(int64_t, stack, BITS_PER_BLOCK);
}

static inline Kind
top_of_stack(int64_t stack)
{
    return stack & ((1<<BITS_PER_BLOCK)-1);
}

static int64_t *
mark_stacks(PyCodeObject *code_obj, int len)
{
    const _Py_CODEUNIT *code =
        (const _Py_CODEUNIT *)PyBytes_AS_STRING(code_obj->co_code);
    int64_t *stacks = PyMem_New(int64_t, len+1);
    int i, j, opcode;

    if (stacks == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    for (int i = 1; i <= len; i++) {
        stacks[i] = UNINITIALIZED;
    }
    stacks[0] = 0;
    int todo = 1;
    while (todo) {
        todo = 0;
        for (i = 0; i < len; i++) {
            int64_t next_stack = stacks[i];
            if (next_stack == UNINITIALIZED) {
                continue;
            }
            opcode = _Py_OPCODE(code[i]);
            switch (opcode) {
                case JUMP_IF_FALSE_OR_POP:
                case JUMP_IF_TRUE_OR_POP:
                case POP_JUMP_IF_FALSE:
                case POP_JUMP_IF_TRUE:
                case JUMP_IF_NOT_EXC_MATCH:
                {
                    int64_t target_stack;
                    int j = get_arg(code, i);
                    assert(j < len);
                    if (stacks[j] == UNINITIALIZED && j < i) {
                        todo = 1;
                    }
                    if (opcode == JUMP_IF_NOT_EXC_MATCH) {
                        next_stack = pop_value(pop_value(next_stack));
                        target_stack = next_stack;
                    }
                    else if (opcode == JUMP_IF_FALSE_OR_POP ||
                             opcode == JUMP_IF_TRUE_OR_POP)
                    {
                        target_stack = next_stack;
                        next_stack = pop_value(next_stack);
                    }
                    else {
                        next_stack = pop_value(next_stack);
                        target_stack = next_stack;
                    }
                    assert(stacks[j] == UNINITIALIZED || stacks[j] == target_stack);
                    stacks[j] = target_stack;
                    stacks[i+1] = next_stack;
                    break;
                }
                case JUMP_ABSOLUTE:
                    j = get_arg(code, i);
                    assert(j < len);
                    if (stacks[j] == UNINITIALIZED && j < i) {
                        todo = 1;
                    }
                    assert(stacks[j] == UNINITIALIZED || stacks[j] == next_stack);
                    stacks[j] = next_stack;
                    break;
                case POP_EXCEPT:
                    next_stack = pop_value(pop_value(pop_value(next_stack)));
                    stacks[i+1] = next_stack;
                    break;

                case JUMP_FORWARD:
                    j = get_arg(code, i) + i + 1;
                    assert(j < len);
                    assert(stacks[j] == UNINITIALIZED || stacks[j] == next_stack);
                    stacks[j] = next_stack;
                    break;
                case GET_ITER:
                case GET_AITER:
                    next_stack = push_value(pop_value(next_stack), Iterator);
                    stacks[i+1] = next_stack;
                    break;
                case FOR_ITER:
                {
                    int64_t target_stack = pop_value(next_stack);
                    stacks[i+1] = push_value(next_stack, Object);
                    j = get_arg(code, i) + i + 1;
                    assert(j < len);
                    assert(stacks[j] == UNINITIALIZED || stacks[j] == target_stack);
                    stacks[j] = target_stack;
                    break;
                }
                case END_ASYNC_FOR:
                    next_stack = pop_value(pop_value(pop_value(next_stack)));
                    stacks[i+1] = next_stack;
                    break;
                case PUSH_EXC_INFO:
                    next_stack = push_value(next_stack, Except);
                    next_stack = push_value(next_stack, Except);
                    next_stack = push_value(next_stack, Except);
                    stacks[i+1] = next_stack;
                case RETURN_VALUE:
                case RAISE_VARARGS:
                case RERAISE:
                case POP_EXCEPT_AND_RERAISE:
                    /* End of block */
                    break;
                case GEN_START:
                    stacks[i+1] = next_stack;
                    break;
                default:
                {
                    int delta = PyCompile_OpcodeStackEffect(opcode, _Py_OPARG(code[i]));
                    while (delta < 0) {
                        next_stack = pop_value(next_stack);
                        delta++;
                    }
                    while (delta > 0) {
                        next_stack = push_value(next_stack, Object);
                        delta--;
                    }
                    stacks[i+1] = next_stack;
                }
            }
        }
    }
    return stacks;
}

static int
compatible_kind(Kind from, Kind to) {
    if (to == 0) {
        return 0;
    }
    if (to == Object) {
        return 1;
    }
    return from == to;
}

static int
compatible_stack(int64_t from_stack, int64_t to_stack)
{
    if (from_stack < 0 || to_stack < 0) {
        return 0;
    }
    while(from_stack > to_stack) {
        from_stack = pop_value(from_stack);
    }
    while(from_stack) {
        Kind from_top = top_of_stack(from_stack);
        Kind to_top = top_of_stack(to_stack);
        if (!compatible_kind(from_top, to_top)) {
            return 0;
        }
        from_stack = pop_value(from_stack);
        to_stack = pop_value(to_stack);
    }
    return to_stack == 0;
}

static const char *
explain_incompatible_stack(int64_t to_stack)
{
    assert(to_stack != 0);
    if (to_stack == OVERFLOWED) {
        return "stack is too deep to analyze";
    }
    if (to_stack == UNINITIALIZED) {
        return "can't jump into an exception handler, or code may be unreachable";
    }
    Kind target_kind = top_of_stack(to_stack);
    switch(target_kind) {
        case Except:
            return "can't jump into an 'except' block as there's no exception";
        case Object:
            return "differing stack depth";
        case Iterator:
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
    assert(f->f_stackdepth > 0);
    f->f_stackdepth--;
    PyObject *v = f->f_valuestack[f->f_stackdepth];
    Py_DECREF(v);
}

/* Setter for f_lineno - you can set f_lineno from within a trace function in
 * order to jump to a given line of code, subject to some restrictions.  Most
 * lines are OK to jump to because they don't make any assumptions about the
 * state of the stack (obvious because you could remove the line and the code
 * would still work without any stack errors), but there are some constructs
 * that limit jumping:
 *
 *  o Any excpetion handlers.
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

    if (new_lineno < _PyFrame_GetCode(f)->co_firstlineno) {
        PyErr_Format(PyExc_ValueError,
                    "line %d comes before the current code block",
                    new_lineno);
        return -1;
    }

    /* PyCode_NewWithPosOnlyArgs limits co_code to be under INT_MAX so this
     * should never overflow. */
    int len = (int)(PyBytes_GET_SIZE(_PyFrame_GetCode(f)->co_code) / sizeof(_Py_CODEUNIT));
    int *lines = marklines(_PyFrame_GetCode(f), len);
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

    int64_t *stacks = mark_stacks(_PyFrame_GetCode(f), len);
    if (stacks == NULL) {
        PyMem_Free(lines);
        return -1;
    }

    int64_t best_stack = OVERFLOWED;
    int best_addr = -1;
    int64_t start_stack = stacks[f->f_lasti];
    int err = -1;
    const char *msg = "cannot find bytecode for specified line";
    for (int i = 0; i < len; i++) {
        if (lines[i] == new_lineno) {
            int64_t target_stack = stacks[i];
            if (compatible_stack(start_stack, target_stack)) {
                err = 0;
                if (target_stack > best_stack) {
                    best_stack = target_stack;
                    best_addr = i;
                }
            }
            else if (err < 0) {
                if (start_stack == OVERFLOWED) {
                    msg = "stack to deep to analyze";
                }
                else if (start_stack == UNINITIALIZED) {
                    msg = "can't jump from within an exception handler";
                }
                else {
                    msg = explain_incompatible_stack(target_stack);
                    err = 1;
                }
            }
        }
    }
    PyMem_Free(stacks);
    PyMem_Free(lines);
    if (err) {
        PyErr_SetString(PyExc_ValueError, msg);
        return -1;
    }
    /* Unwind block stack. */
    if (f->f_state == FRAME_SUSPENDED) {
        /* Account for value popped by yield */
        start_stack = pop_value(start_stack);
    }
    while (start_stack > best_stack) {
        frame_stack_pop(f);
        start_stack = pop_value(start_stack);
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
    {"f_globals",       (getter)frame_getglobals, NULL, NULL},
    {"f_builtins",      (getter)frame_getbuiltins, NULL, NULL},
    {"f_code",          (getter)frame_getcode, NULL, NULL},
    {0}
};

/* Stack frames are allocated and deallocated at a considerable rate.
   In an attempt to improve the speed of function calls, we maintain
   a separate free list of stack frames (just like floats are
   allocated in a special way -- see floatobject.c).  When a stack
   frame is on the free list, only the following members have a meaning:
    ob_type             == &Frametype
    f_back              next item on free list, or NULL
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
    PyCodeObject *co = NULL;

    /* Kill all local variables including specials. */
    if (f->f_localsptr) {
        /* Don't clear code object until the end */
        co = _PyFrame_GetCode(f);
        PyObject **specials = _PyFrame_Specials(f);
        Py_CLEAR(specials[FRAME_SPECIALS_GLOBALS_OFFSET]);
        Py_CLEAR(specials[FRAME_SPECIALS_BUILTINS_OFFSET]);
        Py_CLEAR(specials[FRAME_SPECIALS_LOCALS_OFFSET]);
        for (int i = 0; i < co->co_nlocalsplus; i++) {
            Py_CLEAR(f->f_localsptr[i]);
        }
        /* Free items on stack */
        for (int i = 0; i < f->f_stackdepth; i++) {
            Py_XDECREF(f->f_valuestack[i]);
        }
        if (f->f_own_locals_memory) {
            PyMem_Free(f->f_localsptr);
            f->f_own_locals_memory = 0;
        }
        f->f_localsptr = NULL;
    }
    f->f_stackdepth = 0;
    Py_XDECREF(f->f_back);
    Py_CLEAR(f->f_trace);
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

    Py_XDECREF(co);
    Py_TRASHCAN_SAFE_END(f)
}

static inline Py_ssize_t
frame_nslots(PyFrameObject *frame)
{
    return frame->f_valuestack - frame->f_localsptr;
}

static int
frame_traverse(PyFrameObject *f, visitproc visit, void *arg)
{
    Py_VISIT(f->f_back);
    Py_VISIT(f->f_trace);

    /* locals */
    PyObject **localsplus = f->f_localsptr;
    for (Py_ssize_t i = frame_nslots(f); --i >= 0; ++localsplus) {
        Py_VISIT(*localsplus);
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
    PyCodeObject *co = _PyFrame_GetCode(f);
    /* locals */
    for (int i = 0; i < co->co_nlocalsplus; i++) {
        Py_CLEAR(f->f_localsptr[i]);
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
    Py_ssize_t res;
    res = sizeof(PyFrameObject);
    if (f->f_own_locals_memory) {
        PyCodeObject *code = _PyFrame_GetCode(f);
        res += (code->co_nlocalsplus+code->co_stacksize) * sizeof(PyObject *);
    }
    return PyLong_FromSsize_t(res);
}

PyDoc_STRVAR(sizeof__doc__,
"F.__sizeof__() -> size of F in memory, in bytes");

static PyObject *
frame_repr(PyFrameObject *f)
{
    int lineno = PyFrame_GetLineNumber(f);
    PyCodeObject *code = _PyFrame_GetCode(f);
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
frame_alloc(PyCodeObject *code, PyObject **localsarray)
{
    int owns;
    PyFrameObject *f;
    if (localsarray == NULL) {
        int size = code->co_nlocalsplus+code->co_stacksize + FRAME_SPECIALS_SIZE;
        localsarray = PyMem_Malloc(sizeof(PyObject *)*size);
        if (localsarray == NULL) {
            PyErr_NoMemory();
            return NULL;
        }
        for (Py_ssize_t i=0; i < code->co_nlocalsplus; i++) {
            localsarray[i] = NULL;
        }
        owns = 1;
    }
    else {
        owns = 0;
    }
    struct _Py_frame_state *state = get_frame_state();
    if (state->free_list == NULL)
    {
        f = PyObject_GC_New(PyFrameObject, &PyFrame_Type);
        if (f == NULL) {
            if (owns) {
                PyMem_Free(localsarray);
            }
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
        _Py_NewReference((PyObject *)f);
    }
    f->f_localsptr = localsarray;
    f->f_own_locals_memory = owns;
    return f;
}

int
_PyFrame_TakeLocals(PyFrameObject *f)
{
    assert(f->f_own_locals_memory == 0);
    assert(f->f_stackdepth == 0);
    Py_ssize_t size = frame_nslots(f);
    PyObject **copy = PyMem_Malloc(sizeof(PyObject *)*size);
    if (copy == NULL) {
        for (int i = 0; i < size; i++) {
            PyObject *o = f->f_localsptr[i];
            Py_XDECREF(o);
        }
        PyErr_NoMemory();
        return -1;
    }
    for (int i = 0; i < size; i++) {
        PyObject *o = f->f_localsptr[i];
        copy[i] = o;
    }
    f->f_own_locals_memory = 1;
    f->f_localsptr = copy;
    f->f_valuestack = copy + size;
    return 0;
}

PyFrameObject* _Py_HOT_FUNCTION
_PyFrame_New_NoTrack(PyThreadState *tstate, PyFrameConstructor *con, PyObject *locals, PyObject **localsarray)
{
    assert(con != NULL);
    assert(con->fc_globals != NULL);
    assert(con->fc_builtins != NULL);
    assert(con->fc_code != NULL);
    assert(locals == NULL || PyMapping_Check(locals));
    PyCodeObject *code = (PyCodeObject *)con->fc_code;

    PyFrameObject *f = frame_alloc(code, localsarray);
    if (f == NULL) {
        return NULL;
    }

    PyObject **specials = f->f_localsptr + code->co_nlocalsplus;
    f->f_valuestack = specials + FRAME_SPECIALS_SIZE;
    f->f_back = (PyFrameObject*)Py_XNewRef(tstate->frame);
    specials[FRAME_SPECIALS_CODE_OFFSET] = Py_NewRef(con->fc_code);
    specials[FRAME_SPECIALS_BUILTINS_OFFSET] = Py_NewRef(con->fc_builtins);
    specials[FRAME_SPECIALS_GLOBALS_OFFSET] = Py_NewRef(con->fc_globals);
    specials[FRAME_SPECIALS_LOCALS_OFFSET] = Py_XNewRef(locals);
    f->f_trace = NULL;
    f->f_stackdepth = 0;
    f->f_trace_lines = 1;
    f->f_trace_opcodes = 0;
    f->f_gen = NULL;
    f->f_lasti = -1;
    f->f_lineno = 0;
    f->f_state = FRAME_CREATED;
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
    PyFrameObject *f = _PyFrame_New_NoTrack(tstate, &desc, locals, NULL);
    if (f) {
        _PyObject_GC_TRACK(f);
    }
    return f;
}

static int
_PyFrame_OpAlreadyRan(PyFrameObject *f, int opcode, int oparg)
{
    const _Py_CODEUNIT *code =
        (const _Py_CODEUNIT *)PyBytes_AS_STRING(_PyFrame_GetCode(f)->co_code);
    for (int i = 0; i < f->f_lasti; i++) {
        if (_Py_OPCODE(code[i]) == opcode && _Py_OPARG(code[i]) == oparg) {
            return 1;
        }
    }
    return 0;
}

int
PyFrame_FastToLocalsWithError(PyFrameObject *f)
{
    /* Merge fast locals into f->f_locals */
    PyObject *locals;
    PyObject **fast;
    PyCodeObject *co;

    if (f == NULL) {
        PyErr_BadInternalCall();
        return -1;
    }
    locals = _PyFrame_Specials(f)[FRAME_SPECIALS_LOCALS_OFFSET];
    if (locals == NULL) {
        locals = _PyFrame_Specials(f)[FRAME_SPECIALS_LOCALS_OFFSET] = PyDict_New();
        if (locals == NULL)
            return -1;
    }
    co = _PyFrame_GetCode(f);
    fast = f->f_localsptr;
    for (int i = 0; i < co->co_nlocalsplus; i++) {
        _PyLocalsPlusKind kind = co->co_localspluskinds[i];

        /* If the namespace is unoptimized, then one of the
           following cases applies:
           1. It does not contain free variables, because it
              uses import * or is a top-level namespace.
           2. It is a class namespace.
           We don't want to accidentally copy free variables
           into the locals dict used by the class.
        */
        if (kind & CO_FAST_FREE && !(co->co_flags & CO_OPTIMIZED)) {
            continue;
        }

        PyObject *name = PyTuple_GET_ITEM(co->co_localsplusnames, i);
        PyObject *value = fast[i];
        if (f->f_state != FRAME_CLEARED) {
            if (kind & CO_FAST_FREE) {
                // The cell was set by _PyEval_MakeFrameVector() from
                // the function's closure.
                assert(value != NULL && PyCell_Check(value));
                value = PyCell_GET(value);
            }
            else if (kind & CO_FAST_CELL) {
                // Note that no *_DEREF ops can happen before MAKE_CELL
                // executes.  So there's no need to duplicate the work
                // that MAKE_CELL would otherwise do later, if it hasn't
                // run yet.
                if (value != NULL) {
                    if (PyCell_Check(value) &&
                            _PyFrame_OpAlreadyRan(f, MAKE_CELL, i)) {
                        // (likely) MAKE_CELL must have executed already.
                        value = PyCell_GET(value);
                    }
                    // (likely) Otherwise it it is an arg (kind & CO_FAST_LOCAL),
                    // with the initial value set by _PyEval_MakeFrameVector()...
                    // (unlikely) ...or it was set to some initial value by
                    // an earlier call to PyFrame_LocalsToFast().
                }
            }
        }
        else {
            assert(value == NULL);
        }
        if (value == NULL) {
            if (PyObject_DelItem(locals, name) != 0) {
                if (PyErr_ExceptionMatches(PyExc_KeyError)) {
                    PyErr_Clear();
                }
                else {
                    return -1;
                }
            }
        }
        else {
            if (PyObject_SetItem(locals, name, value) != 0) {
                return -1;
            }
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
    /* Merge locals into fast locals */
    PyObject *locals;
    PyObject **fast;
    PyObject *error_type, *error_value, *error_traceback;
    PyCodeObject *co;
    if (f == NULL || f->f_state == FRAME_CLEARED) {
        return;
    }
    locals = _PyFrame_Specials(f)[FRAME_SPECIALS_LOCALS_OFFSET];
    if (locals == NULL)
        return;
    fast = f->f_localsptr;
    co = _PyFrame_GetCode(f);

    PyErr_Fetch(&error_type, &error_value, &error_traceback);
    for (int i = 0; i < co->co_nlocalsplus; i++) {
        _PyLocalsPlusKind kind = co->co_localspluskinds[i];

        /* Same test as in PyFrame_FastToLocals() above. */
        if (kind & CO_FAST_FREE && !(co->co_flags & CO_OPTIMIZED)) {
            continue;
        }
        PyObject *name = PyTuple_GET_ITEM(co->co_localsplusnames, i);
        PyObject *value = PyObject_GetItem(locals, name);
        /* We only care about NULLs if clear is true. */
        if (value == NULL) {
            PyErr_Clear();
            if (!clear) {
                continue;
            }
        }
        PyObject *oldvalue = fast[i];
        PyObject *cell = NULL;
        if (kind == CO_FAST_FREE) {
            // The cell was set by _PyEval_MakeFrameVector() from
            // the function's closure.
            assert(oldvalue != NULL && PyCell_Check(oldvalue));
            cell = oldvalue;
        }
        else if (kind & CO_FAST_CELL && oldvalue != NULL) {
            /* Same test as in PyFrame_FastToLocals() above. */
            if (PyCell_Check(oldvalue) &&
                    _PyFrame_OpAlreadyRan(f, MAKE_CELL, i)) {
                // (likely) MAKE_CELL must have executed already.
                cell = oldvalue;
            }
            // (unlikely) Otherwise, it must have been set to some
            // initial value by an earlier call to PyFrame_LocalsToFast().
        }
        if (cell != NULL) {
            oldvalue = PyCell_GET(cell);
            if (value != oldvalue) {
                Py_XDECREF(oldvalue);
                Py_XINCREF(value);
                PyCell_SET(cell, value);
            }
        }
        else if (value != oldvalue) {
            Py_XINCREF(value);
            Py_XSETREF(fast[i], value);
        }
        Py_XDECREF(value);
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
    PyCodeObject *code = _PyFrame_GetCode(frame);
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
            builtins = _PyModule_GetDict(builtins);
            assert(builtins != NULL);
        }
        return builtins;
    }
    if (PyErr_Occurred()) {
        return NULL;
    }

    return _PyEval_GetBuiltins(tstate);
}
