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

static PyObject *_PyFastLocalsProxy_New(PyObject *frame);

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
_frame_get_locals_mapping(PyFrameObject *f)
{
    PyObject *locals = _PyFrame_Specials(f)[FRAME_SPECIALS_LOCALS_OFFSET];
    if (locals == NULL) {
        locals = _PyFrame_Specials(f)[FRAME_SPECIALS_LOCALS_OFFSET] = PyDict_New();
    }
    return locals;
}

static PyObject *
_frame_get_updated_locals(PyFrameObject *f)
{
    if (PyFrame_FastToLocalsWithError(f) < 0)
        return NULL;
    PyObject *locals = _PyFrame_Specials(f)[FRAME_SPECIALS_LOCALS_OFFSET];
    assert(locals != NULL);
    // Internal borrowed reference, caller increfs for external sharing
    return locals;
}

PyObject *
_PyFrame_BorrowLocals(PyFrameObject *f)
{
    // This is called by PyEval_GetLocals(), which has historically returned
    // a borrowed reference, so this does the same
    return _frame_get_updated_locals(f);
}

PyObject *
PyFrame_GetLocals(PyFrameObject *f)
{
    // This API implements the Python level locals() builtin
    PyObject *updated_locals = _frame_get_updated_locals(f);
    PyCodeObject *co = _PyFrame_GetCode(f);

    assert(co);
    if (co->co_flags & CO_OPTIMIZED) {
        // PEP 558: Make a copy of optimised scopes to avoid quirky side effects
        updated_locals = PyDict_Copy(updated_locals);
    } else {
        // Share a direct locals reference for class and module scopes
        Py_INCREF(updated_locals);
    }

    return updated_locals;
}

static PyObject *
frame_getlocals(PyFrameObject *f, void *Py_UNUSED(ignored))
{
    // This API implements the Python level frame.f_locals descriptor
    PyObject *f_locals_attr = NULL;
    PyCodeObject *co = _PyFrame_GetCode(f);

    assert(co);
    if (co->co_flags & CO_OPTIMIZED) {
        /* PEP 558: If this is an optimized frame, ensure f_locals at the Python
         * layer is a new fastlocalsproxy instance, while f_locals at the C
         * layer still refers to the underlying shared namespace mapping.
         */
        if (PyFrame_FastToLocalsWithError(f) < 0) {
            return NULL;
        }
        f_locals_attr = _PyFastLocalsProxy_New((PyObject *) f);
    } else {
        // Share a direct locals reference for class and module scopes
        f_locals_attr = _frame_get_updated_locals(f);
        Py_INCREF(f_locals_attr);
    }
    return f_locals_attr;
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
    locals = _frame_get_locals_mapping(f);
    if (locals == NULL) {
        return -1;
    }
    co = _PyFrame_GetCode(f);
    fast = f->f_localsptr;
    for (int i = 0; i < co->co_nlocalsplus; i++) {
        _PyLocals_Kind kind = _PyLocals_GetKind(co->co_localspluskinds, i);

        /* If the namespace is unoptimized, then one of the
           following cases applies:
           1. It does not contain free variables, because it
              is a top-level namespace.
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
                    // (unlikely) ...or it was set to some initial value via
                    // the frame locals proxy
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
    PyErr_SetString(
        PyExc_RuntimeError,
        "PyFrame_LocalsToFast is no longer supported. "
        "Use PyObject_GetAttrString(frame, \"f_locals\") "
        "to obtain a write-through mapping proxy instead."
    );
}

static int
set_fast_ref(PyObject *fast_refs, PyObject *key, PyObject *value)
{
    // NOTE: Steals the "value" reference, so borrowed values need an INCREF
    assert(PyUnicode_Check(key));
    int status = PyDict_SetItem(fast_refs, key, value);
    Py_DECREF(value);
    return status;
}

static PyObject *
_PyFrame_BuildFastRefs(PyFrameObject *f)
{
    /* Construct a combined mapping from local variable names to indices
     * in the fast locals array, and from nonlocal variable names directly
     * to the corresponding cell objects
     */
    PyObject **fast_locals;
    PyCodeObject *co;
    PyObject *fast_refs;


    if (f == NULL) {
        PyErr_BadInternalCall();
        return NULL;
    }
    co = _PyFrame_GetCode(f);
    assert(co);
    fast_locals = f->f_localsptr;

    if (fast_locals == NULL || !(co->co_flags & CO_OPTIMIZED)) {
        PyErr_SetString(PyExc_SystemError,
            "attempted to build fast refs lookup table for non-optimized scope");
        return NULL;
    }

    fast_refs = PyDict_New();
    if (fast_refs == NULL) {
        return NULL;
    }

    if (f->f_state != FRAME_CLEARED) {
        for (int i = 0; i < co->co_nlocalsplus; i++) {
            _PyLocals_Kind kind = _PyLocals_GetKind(co->co_localspluskinds, i);
            PyObject *name = PyTuple_GET_ITEM(co->co_localsplusnames, i);
            PyObject *target = NULL;
            if (kind & CO_FAST_FREE) {
                // Reference to closure cell, save it as the proxy target
                target = fast_locals[i];
                assert(target != NULL && PyCell_Check(target));
                Py_INCREF(target);
            } else if (kind & CO_FAST_CELL) {
                // Closure cell referenced from nested scopes
                // Save it as the proxy target if the cell already exists,
                // otherwise save the index and fix it up later on access
                target = fast_locals[i];
                if (target != NULL && PyCell_Check(target) &&
                        _PyFrame_OpAlreadyRan(f, MAKE_CELL, i)) {
                    // MAKE_CELL built the cell, so use it as the proxy target
                    Py_INCREF(target);
                } else {
                    // MAKE_CELL hasn't run yet, so just store the lookup index
                    // The proxy will check the kind on access, and switch over
                    // to using the cell once MAKE_CELL creates it
                    target = PyLong_FromSsize_t(i);
                }
            } else if (kind & CO_FAST_LOCAL) {
                // Ordinary fast local variable. Save index as the proxy target
                target = PyLong_FromSsize_t(i);
            } else {
                PyErr_SetString(PyExc_SystemError,
                    "unknown local variable kind while building fast refs lookup table");
            }
            if (target == NULL) {
                Py_DECREF(fast_refs);
                return NULL;
            }
            if (set_fast_ref(fast_refs, name, target) != 0) {
                return NULL;
            }
        }
    }
    return fast_refs;
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

/* _PyFastLocalsProxy_Type
 *
 * Mapping object that provides name-based access to the fast locals on a frame
 */
/*[clinic input]
class fastlocalsproxy "fastlocalsproxyobject *" "&_PyFastLocalsProxy_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=a2dd0ae6e1642243]*/


typedef struct {
    PyObject_HEAD
    PyFrameObject *frame;
    PyObject      *fast_refs; /* Cell refs and local variable indices */
} fastlocalsproxyobject;

// PEP 558 TODO: Implement correct Python sizeof() support for fastlocalsproxyobject

static int
fastlocalsproxy_init_fast_refs(fastlocalsproxyobject *flp)
{
    // Build fast ref mapping if it hasn't been built yet
    assert(flp);
    if (flp->fast_refs != NULL) {
        return 0;
    }
    PyObject *fast_refs = _PyFrame_BuildFastRefs(flp->frame);
    if (fast_refs == NULL) {
        return -1;
    }
    flp->fast_refs = fast_refs;
    return 0;
}

static Py_ssize_t
fastlocalsproxy_len(fastlocalsproxyobject *flp)
{
    // Extra keys may have been added, and some variables may not have been
    // bound yet, so use the dynamic snapshot on the frame rather than the
    // keys in the fast locals reverse lookup mapping
    // Assume f_locals snapshot is up to date (as actually checking is O(n))
    PyObject *locals = _frame_get_locals_mapping(flp->frame);
    return PyObject_Size(locals);
}

static int
fastlocalsproxy_set_snapshot_entry(fastlocalsproxyobject *flp, PyObject *key, PyObject *value)
{
        PyFrameObject *f = flp->frame;
        if (f->f_state == FRAME_CLEARED) {
            // Don't touch the locals cache on already cleared frames
            return 0;
        }
        PyObject *locals = _frame_get_locals_mapping(f);
        if (locals == NULL) {
            return -1;
        }
        if (value == NULL) {
            // Ensure key is absent from cache (deleting if necessary)
            if (PyDict_Contains(locals, key)) {
                return PyObject_DelItem(locals, key);
            }
            return 0;
        }
        // Set cached value for the given key
        return PyObject_SetItem(locals, key, value);
}

static PyObject *
fastlocalsproxy_getitem(fastlocalsproxyobject *flp, PyObject *key)
{
    // PEP 558 TODO: try to factor out the common get/set key lookup code
    assert(flp);
    if (fastlocalsproxy_init_fast_refs(flp) != 0) {
        return NULL;
    }
    PyObject *fast_ref = PyDict_GetItem(flp->fast_refs, key);
    if (fast_ref == NULL) {
        // No such local variable, delegate the request to the f_locals mapping
        // Used by pdb (at least) to access __return__ and __exception__ values
        PyObject *locals = _frame_get_locals_mapping(flp->frame);
        if (locals == NULL) {
            return NULL;
        }
        return PyObject_GetItem(locals, key);
    }
    /* Key is a valid Python variable for the frame, so retrieve the value */
    PyObject *value = NULL;
    if (PyCell_Check(fast_ref)) {
        // Closure cells can be queried even after the frame terminates
        value = PyCell_GET(fast_ref);
    } else {
        PyFrameObject *f = flp->frame;
        if (f->f_state == FRAME_CLEARED) {
            PyErr_Format(PyExc_RuntimeError,
                "Fast locals proxy attempted to read from cleared frame (%R)", f);
            return NULL;
        }
        /* Fast ref is a Python int mapping into the fast locals array */
        assert(PyLong_CheckExact(fast_ref));
        Py_ssize_t offset = PyLong_AsSsize_t(fast_ref);
        if (offset < 0) {
            return NULL;
        }
        PyCodeObject *co = _PyFrame_GetCode(f);
        assert(co);
        Py_ssize_t max_offset = co->co_nlocalsplus - 1;
        if (offset > max_offset) {
            PyErr_Format(PyExc_SystemError,
                            "Fast locals ref (%zd) exceeds array bound (%zd)",
                            offset, max_offset);
            return NULL;
        }
        PyObject **fast_locals = f->f_localsptr;
        value = fast_locals[offset];
        // Check if MAKE_CELL has been called since the proxy was created
        _PyLocals_Kind kind = _PyLocals_GetKind(co->co_localspluskinds, offset);
        if (kind & CO_FAST_CELL) {
            // Value hadn't been converted to a cell yet when the proxy was created
            // Update the proxy if MAKE_CELL has run since the last access,
            // otherwise continue treating it as a regular local variable
            PyObject *target = value;
            if (target != NULL && PyCell_Check(target) &&
                    _PyFrame_OpAlreadyRan(f, MAKE_CELL, offset)) {
                // MAKE_CELL has built the cell, so use it as the proxy target
                Py_INCREF(target);
                if (set_fast_ref(flp->fast_refs, key, target) != 0) {
                    return NULL;
                }
                value = PyCell_GET(target);
            }
        }
    }

    // Local variable, or cell variable that either hasn't been converted yet
    // or was only just converted since the last cache sync
    // Ensure the value cache is up to date if the frame is still live
    if (!PyErr_Occurred()) {
        if (fastlocalsproxy_set_snapshot_entry(flp, key, value) != 0) {
            return NULL;
        }
    }
    if (value == NULL && !PyErr_Occurred()) {
        // Report KeyError if the variable hasn't been bound to a value yet
        // (akin to getting an UnboundLocalError in running code)
        PyErr_SetObject(PyExc_KeyError, key);
    } else {
        Py_XINCREF(value);
    }
    return value;
}


static int
fastlocalsproxy_write_to_frame(fastlocalsproxyobject *flp, PyObject *key, PyObject *value)
{
    // PEP 558 TODO: try to factor out the common get/set key lookup code
    assert(flp);
    if (fastlocalsproxy_init_fast_refs(flp) != 0) {
        return -1;
    }
    PyObject *fast_ref = PyDict_GetItem(flp->fast_refs, key);
    if (fast_ref == NULL) {
        // No such local variable, delegate the request to the f_locals mapping
        // Used by pdb (at least) to store __return__ and __exception__ values
        return fastlocalsproxy_set_snapshot_entry(flp, key, value);
    }
    /* Key is a valid Python variable for the frame, so update that reference */
    if (PyCell_Check(fast_ref)) {
        // Closure cells can be updated even after the frame terminates
        int result = PyCell_Set(fast_ref, value);
        if (result == 0) {
            // Ensure the value cache is up to date if the frame is still live
            result = fastlocalsproxy_set_snapshot_entry(flp, key, value);
        }
        return result;
    }
    PyFrameObject *f = flp->frame;
    if (f->f_state == FRAME_CLEARED) {
        PyErr_Format(PyExc_RuntimeError,
            "Fast locals proxy attempted to write to cleared frame (%R)", f);
        return -1;
    }
    /* Fast ref is a Python int mapping into the fast locals array */
    assert(PyLong_CheckExact(fast_ref));
    Py_ssize_t offset = PyLong_AsSsize_t(fast_ref);
    if (offset < 0) {
        return -1;
    }
    PyCodeObject *co = _PyFrame_GetCode(f);
    assert(co);
    Py_ssize_t max_offset = co->co_nlocalsplus - 1;
    if (offset > max_offset) {
        PyErr_Format(PyExc_SystemError,
                        "Fast locals ref (%zd) exceeds array bound (%zd)",
                        offset, max_offset);
        return -1;
    }
    PyObject **fast_locals = f->f_localsptr;
    // Check if MAKE_CELL has been called since the proxy was created
    _PyLocals_Kind kind = _PyLocals_GetKind(co->co_localspluskinds, offset);
    if (kind & CO_FAST_CELL) {
        // Value hadn't been converted to a cell yet when the proxy was created
        // Update the proxy if MAKE_CELL has run since the last access,
        // otherwise continue treating it as a regular local variable
        PyObject *target = fast_locals[offset];
        if (target != NULL && PyCell_Check(target) &&
                _PyFrame_OpAlreadyRan(f, MAKE_CELL, offset)) {
            // MAKE_CELL has built the cell, so use it as the proxy target
            Py_INCREF(target);
            if (set_fast_ref(flp->fast_refs, key, target) != 0) {
                return -1;
            }
            int result = PyCell_Set(target, value);
            if (result == 0) {
                // Ensure the value cache is up to date if the frame is still live
                result = fastlocalsproxy_set_snapshot_entry(flp, key, value);
            }
            return result;
        }
    }

    // Local variable, or future cell variable that hasn't been converted yet
    Py_XINCREF(value);
    Py_XSETREF(fast_locals[offset], value);
    // Ensure the value cache is up to date if the frame is still live
    return fastlocalsproxy_set_snapshot_entry(flp, key, value);
}

static int
fastlocalsproxy_setitem(fastlocalsproxyobject *flp, PyObject *key, PyObject *value)
{
    return fastlocalsproxy_write_to_frame(flp, key, value);
}

static int
fastlocalsproxy_delitem(fastlocalsproxyobject *flp, PyObject *key)
{
    return fastlocalsproxy_write_to_frame(flp, key, NULL);
}

static PyMappingMethods fastlocalsproxy_as_mapping = {
    (lenfunc)fastlocalsproxy_len,         /* mp_length */
    (binaryfunc)fastlocalsproxy_getitem,  /* mp_subscript */
    (objobjargproc)fastlocalsproxy_setitem, /* mp_ass_subscript */
};

static PyObject *
fastlocalsproxy_or(PyObject *Py_UNUSED(left), PyObject *Py_UNUSED(right))
{
    // Delegate to the other operand to determine the return type
    Py_RETURN_NOTIMPLEMENTED;
}

static PyObject *
fastlocalsproxy_ior(PyObject *self, PyObject *Py_UNUSED(other))
{
    // PEP 558 TODO: Support |= to update from arbitrary mappings
    // Check the latest mutablemapping_update code for __ior__ support
    PyErr_Format(PyExc_NotImplementedError,
                 "FastLocalsProxy does not yet implement __ior__");
    return NULL;
}

static PyNumberMethods fastlocalsproxy_as_number = {
    .nb_or = fastlocalsproxy_or,
    .nb_inplace_or = fastlocalsproxy_ior,
};

static int
fastlocalsproxy_contains(fastlocalsproxyobject *flp, PyObject *key)
{
    assert(flp);
    if (fastlocalsproxy_init_fast_refs(flp) != 0) {
        return -1;
    }
    int result = PyDict_Contains(flp->fast_refs, key);
    if (result) {
        // PEP 558 TODO: This should return false if the name hasn't been bound yet
        return result;
    }
    // Extra keys may have been stored directly in the frame locals
    PyObject *locals = _frame_get_locals_mapping(flp->frame);
    if (locals == NULL) {
        return -1;
    }
    return PyDict_Contains(locals, key);
}

static PySequenceMethods fastlocalsproxy_as_sequence = {
    0,                                          /* sq_length */
    0,                                          /* sq_concat */
    0,                                          /* sq_repeat */
    0,                                          /* sq_item */
    0,                                          /* sq_slice */
    0,                                          /* sq_ass_item */
    0,                                          /* sq_ass_slice */
    (objobjproc)fastlocalsproxy_contains,       /* sq_contains */
    0,                                          /* sq_inplace_concat */
    0,                                          /* sq_inplace_repeat */
};

static PyObject *
fastlocalsproxy_get(fastlocalsproxyobject *flp, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *key = NULL;
    PyObject *failobj = Py_None;

    if (!_PyArg_UnpackStack(args, nargs, "get", 1, 2,
                            &key, &failobj))
    {
        return NULL;
    }

    PyObject *value = fastlocalsproxy_getitem(flp, key);
    if (value == NULL && PyErr_ExceptionMatches(PyExc_KeyError)) {
        PyErr_Clear();
        value = failobj;
        Py_INCREF(value);
    }
    return value;
}

static PyObject *
fastlocalsproxy_keys(fastlocalsproxyobject *flp, PyObject *Py_UNUSED(ignored))
{
    // Extra keys may have been added, and some variables may not have been
    // bound yet, so use the dynamic snapshot on the frame rather than the
    // keys in the fast locals reverse lookup mapping
    // Assume f_locals snapshot is up to date (as actually checking is O(n))
    PyObject *locals = _frame_get_locals_mapping(flp->frame);
    return PyDict_Keys(locals);
}

static PyObject *
fastlocalsproxy_values(fastlocalsproxyobject *flp, PyObject *Py_UNUSED(ignored))
{
    // Need values, so use the dynamic snapshot on the frame
    // Assume f_locals snapshot is up to date (as actually checking is O(n))
    PyObject *locals = _frame_get_locals_mapping(flp->frame);
    return PyDict_Values(locals);
}

static PyObject *
fastlocalsproxy_items(fastlocalsproxyobject *flp, PyObject *Py_UNUSED(ignored))
{
    // Need values, so use the dynamic snapshot on the frame
    // Assume f_locals snapshot is up to date (as actually checking is O(n))
    PyObject *locals = _frame_get_locals_mapping(flp->frame);
    return PyDict_Items(locals);
}

static PyObject *
fastlocalsproxy_copy(fastlocalsproxyobject *flp, PyObject *Py_UNUSED(ignored))
{
    // Need values, so use the dynamic snapshot on the frame
    // Ensure it is up to date, as checking is O(n) anyway
    PyObject *locals = _frame_get_updated_locals(flp->frame);
    return PyDict_Copy(locals);
}

static PyObject *
fastlocalsproxy_reversed(fastlocalsproxyobject *flp, PyObject *Py_UNUSED(ignored))
{
    _Py_IDENTIFIER(__reversed__);
    // Extra keys may have been added, and some variables may not have been
    // bound yet, so use the dynamic snapshot on the frame rather than the
    // keys in the fast locals reverse lookup mapping
    // Assume f_locals snapshot is up to date (as actually checking is O(n))
    PyObject *locals = _frame_get_locals_mapping(flp->frame);
    return _PyObject_CallMethodIdNoArgs(locals, &PyId___reversed__);
}

static PyObject *
fastlocalsproxy_getiter(fastlocalsproxyobject *flp)
{
    // Extra keys may have been added, and some variables may not have been
    // bound yet, so use the dynamic snapshot on the frame rather than the
    // keys in the fast locals reverse lookup mapping
    // Assume f_locals snapshot is up to date (as actually checking is O(n))
    PyObject *locals = _frame_get_locals_mapping(flp->frame);
    return PyObject_GetIter(locals);
}

static PyObject *
fastlocalsproxy_richcompare(fastlocalsproxyobject *flp, PyObject *w, int op)
{
    // Need values, so use the dynamic snapshot on the frame
    // Ensure it is up to date, as checking is O(n) anyway
    PyObject *locals = _frame_get_updated_locals(flp->frame);
    return PyObject_RichCompare(locals, w, op);
}

/* setdefault() */

PyDoc_STRVAR(fastlocalsproxy_setdefault__doc__,
"flp.setdefault(k[, d=None]) -> v, Insert key with a value of default if key\n\
        is not in the dictionary.\n\n\
        Return the value for key if key is in the dictionary, else default.");

static PyObject *
fastlocalsproxy_setdefault(PyObject *flp, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = {"key", "default", 0};
    PyObject *key, *failobj = NULL;

    /* borrowed */
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|O:pop", kwlist,
                                     &key, &failobj)) {
        return NULL;
    }

    PyObject *value = NULL;

    // PEP 558 TODO: implement this
    PyErr_Format(PyExc_NotImplementedError,
                 "FastLocalsProxy does not yet implement setdefault()");
    return value;
}


/* pop() */

PyDoc_STRVAR(fastlocalsproxy_pop__doc__,
"flp.pop(k[,d]) -> v, unbind specified variable and return the corresponding\n\
        value.  If key is not found, d is returned if given, otherwise KeyError\n\
        is raised.");

static PyObject *
_fastlocalsproxy_popkey(fastlocalsproxyobject *flp, PyObject *key, PyObject *failobj)
{
    // TODO: Similar to the odict implementation, the fast locals proxy
    // could benefit from an internal API that accepts already calculated
    // hashes, rather than recalculating the hash multiple times for the
    // same key in a single operation (see _odict_popkey_hash)
    assert(flp);
    if (fastlocalsproxy_init_fast_refs(flp) != 0) {
        return NULL;
    }

    // Just implement naive lookup through the object based C API for now
    PyObject *value = fastlocalsproxy_getitem(flp, key);
    if (value != NULL) {
        if (fastlocalsproxy_delitem(flp, key) != 0) {
            Py_CLEAR(value);
        }
    } else if (failobj != NULL && PyErr_ExceptionMatches(PyExc_KeyError)) {
        PyErr_Clear();
        value = failobj;
        Py_INCREF(value);
    }

    return value;
}

static PyObject *
fastlocalsproxy_pop(PyObject *flp, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = {"key", "default", 0};
    PyObject *key, *failobj = NULL;

    /* borrowed */
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|O:pop", kwlist,
                                     &key, &failobj)) {
        return NULL;
    }

    return _fastlocalsproxy_popkey((fastlocalsproxyobject *)flp, key, failobj);
}

/* popitem() */

PyDoc_STRVAR(fastlocalsproxy_popitem__doc__,
"flp.popitem() -> (k, v), remove and return some (key, value) pair as a\n\
        2-tuple; but raise KeyError if D is empty.");

static PyObject *
fastlocalsproxy_popitem(PyObject *flp, PyObject *Py_UNUSED(ignored))
{
    // PEP 558 TODO: implement this
    PyErr_Format(PyExc_NotImplementedError,
                 "FastLocalsProxy does not yet implement popitem()");
    return NULL;
}

/* update() */

/* MutableMapping.update() does not have a docstring. */
PyDoc_STRVAR(fastlocalsproxy_update__doc__, "");

/* forward */
static PyObject * mutablemapping_update(PyObject *, PyObject *, PyObject *);

#define fastlocalsproxy_update mutablemapping_update

/* clear() */

PyDoc_STRVAR(fastlocalsproxy_clear__doc__,
             "flp.clear() -> None. Unbind all variables in frame.");

static PyObject *
fastlocalsproxy_clear(register PyObject *flp, PyObject *Py_UNUSED(ignored))
{
    // PEP 558 TODO: implement this
    PyErr_Format(PyExc_NotImplementedError,
                 "FastLocalsProxy does not yet implement clear()");
    return NULL;
}


PyDoc_STRVAR(fastlocalsproxy_sync_frame_cache__doc__,
             "flp.sync_frame_cache() -> None. Ensure f_locals snapshot is in sync with underlying frame.");

static PyObject *
fastlocalsproxy_sync_frame_cache(register PyObject *self, PyObject *Py_UNUSED(ignored))
{
    fastlocalsproxyobject *flp = (fastlocalsproxyobject *)self;
    if (PyFrame_FastToLocalsWithError(flp->frame) < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}


static PyMethodDef fastlocalsproxy_methods[] = {
    {"get",       (PyCFunction)(void(*)(void))fastlocalsproxy_get, METH_FASTCALL,
     PyDoc_STR("D.get(k[,d]) -> D[k] if k in D, else d."
               "  d defaults to None.")},
    {"keys",      (PyCFunction)fastlocalsproxy_keys,       METH_NOARGS,
     PyDoc_STR("D.keys() -> virtual set of D's keys")},
    {"values",    (PyCFunction)fastlocalsproxy_values,     METH_NOARGS,
     PyDoc_STR("D.values() -> virtual multiset of D's values")},
    {"items",     (PyCFunction)fastlocalsproxy_items,      METH_NOARGS,
     PyDoc_STR("D.items() -> virtual set of D's (key, value) pairs, as 2-tuples")},
    {"copy",      (PyCFunction)fastlocalsproxy_copy,       METH_NOARGS,
     PyDoc_STR("D.copy() -> a shallow copy of D as a regular dict")},
    {"__class_getitem__", (PyCFunction)Py_GenericAlias, METH_O|METH_CLASS,
     PyDoc_STR("See PEP 585")},
    {"__reversed__", (PyCFunction)fastlocalsproxy_reversed, METH_NOARGS,
     PyDoc_STR("D.__reversed__() -> reverse iterator over D's keys")},
     // PEP 558 TODO: Convert METH_VARARGS/METH_KEYWORDS methods to METH_FASTCALL
    {"setdefault",      (PyCFunction)(void(*)(void))fastlocalsproxy_setdefault,
     METH_VARARGS | METH_KEYWORDS, fastlocalsproxy_setdefault__doc__},
    {"pop",             (PyCFunction)(void(*)(void))fastlocalsproxy_pop,
     METH_VARARGS | METH_KEYWORDS, fastlocalsproxy_pop__doc__},
    {"popitem",           (PyCFunction)fastlocalsproxy_popitem,
     METH_NOARGS, fastlocalsproxy_popitem__doc__},
    {"update",          (PyCFunction)(void(*)(void))fastlocalsproxy_update,
     METH_VARARGS | METH_KEYWORDS, fastlocalsproxy_update__doc__},
    {"clear",           (PyCFunction)fastlocalsproxy_clear,
     METH_NOARGS, fastlocalsproxy_clear__doc__},
    {"sync_frame_cache", (PyCFunction)fastlocalsproxy_sync_frame_cache,
     METH_NOARGS, fastlocalsproxy_sync_frame_cache__doc__},
    {NULL, NULL}   /* sentinel */
};

static void
fastlocalsproxy_dealloc(fastlocalsproxyobject *flp)
{
    if (_PyObject_GC_IS_TRACKED(flp))
        _PyObject_GC_UNTRACK(flp);

    Py_TRASHCAN_SAFE_BEGIN(flp)

    Py_CLEAR(flp->frame);
    Py_CLEAR(flp->fast_refs);
    PyObject_GC_Del(flp);

    Py_TRASHCAN_SAFE_END(flp)
}

static PyObject *
fastlocalsproxy_repr(fastlocalsproxyobject *flp)
{
    return PyUnicode_FromFormat("fastlocalsproxy(%R)", flp->frame);
}

static PyObject *
fastlocalsproxy_str(fastlocalsproxyobject *flp)
{
    // Need values, so use the dynamic snapshot on the frame
    // Ensure it is up to date, as checking is O(n) anyway
    PyObject *locals = _frame_get_updated_locals(flp->frame);
    return PyObject_Str(locals);
}

static int
fastlocalsproxy_traverse(PyObject *self, visitproc visit, void *arg)
{
    fastlocalsproxyobject *flp = (fastlocalsproxyobject *)self;
    Py_VISIT(flp->frame);
    Py_VISIT(flp->fast_refs);
    return 0;
}

static int
fastlocalsproxy_check_frame(PyObject *maybe_frame)
{
    /* This is an internal-only API, so getting bad arguments means something
     * already went wrong elsewhere in the interpreter code.
     */
    if (!PyFrame_Check(maybe_frame)) {
        PyErr_Format(PyExc_SystemError,
                    "fastlocalsproxy() argument must be a frame, not %s",
                    Py_TYPE(maybe_frame)->tp_name);
        return -1;
    }

    PyFrameObject *frame = (PyFrameObject *) maybe_frame;
    if (!(_PyFrame_GetCode(frame)->co_flags & CO_OPTIMIZED)) {
        PyErr_SetString(PyExc_SystemError,
            "fastlocalsproxy() argument must be a frame using fast locals");
        return -1;
    }
    return 0;
}


static PyObject *
_PyFastLocalsProxy_New(PyObject *frame)
{
    fastlocalsproxyobject *flp;

    if (fastlocalsproxy_check_frame(frame) == -1) {
        return NULL;
    }

    flp = PyObject_GC_New(fastlocalsproxyobject, &_PyFastLocalsProxy_Type);
    if (flp == NULL)
        return NULL;
    flp->frame = (PyFrameObject *) frame;
    Py_INCREF(flp->frame);
    flp->fast_refs = NULL;
    _PyObject_GC_TRACK(flp);
    return (PyObject *)flp;
}

/*[clinic input]
@classmethod
fastlocalsproxy.__new__ as fastlocalsproxy_new

    frame: object

[clinic start generated code]*/

static PyObject *
fastlocalsproxy_new_impl(PyTypeObject *type, PyObject *frame)
/*[clinic end generated code: output=058c588af86f1525 input=049f74502e02fc63]*/
{
    return _PyFastLocalsProxy_New(frame);
}

#include "clinic/frameobject.c.h"

PyTypeObject _PyFastLocalsProxy_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "fastlocalsproxy",                          /* tp_name */
    sizeof(fastlocalsproxyobject),              /* tp_basicsize */
    0,                                          /* tp_itemsize */
    (destructor)fastlocalsproxy_dealloc,        /* tp_dealloc */
    0,                                          /* tp_print */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_reserved */
    (reprfunc)fastlocalsproxy_repr,             /* tp_repr */
    &fastlocalsproxy_as_number,                 /* tp_as_number */
    &fastlocalsproxy_as_sequence,               /* tp_as_sequence */
    &fastlocalsproxy_as_mapping,                /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    (reprfunc)fastlocalsproxy_str,              /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT |
    Py_TPFLAGS_HAVE_GC |
    Py_TPFLAGS_MAPPING,                         /* tp_flags */
    0,                                          /* tp_doc */
    (traverseproc)fastlocalsproxy_traverse,     /* tp_traverse */
    0,                                          /* tp_clear */
    (richcmpfunc)fastlocalsproxy_richcompare,   /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    (getiterfunc)fastlocalsproxy_getiter,       /* tp_iter */
    0,                                          /* tp_iternext */
    fastlocalsproxy_methods,                    /* tp_methods */
    0,                                          /* tp_members */
    0,                                          /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    0,                                          /* tp_init */
    0,                                          /* tp_alloc */
    fastlocalsproxy_new,                        /* tp_new */
    0,                                          /* tp_free */
};


//==========================================================================
// The rest of this file is currently DUPLICATED CODE from odictobject.c
//
// PEP 558 TODO: move the duplicated code to Objects/mutablemapping.c and
//               expose it to the linker as a private API
//
//==========================================================================

static int
mutablemapping_add_pairs(PyObject *self, PyObject *pairs)
{
    PyObject *pair, *iterator, *unexpected;
    int res = 0;

    iterator = PyObject_GetIter(pairs);
    if (iterator == NULL)
        return -1;
    PyErr_Clear();

    while ((pair = PyIter_Next(iterator)) != NULL) {
        /* could be more efficient (see UNPACK_SEQUENCE in ceval.c) */
        PyObject *key = NULL, *value = NULL;
        PyObject *pair_iterator = PyObject_GetIter(pair);
        if (pair_iterator == NULL)
            goto Done;

        key = PyIter_Next(pair_iterator);
        if (key == NULL) {
            if (!PyErr_Occurred())
                PyErr_SetString(PyExc_ValueError,
                                "need more than 0 values to unpack");
            goto Done;
        }

        value = PyIter_Next(pair_iterator);
        if (value == NULL) {
            if (!PyErr_Occurred())
                PyErr_SetString(PyExc_ValueError,
                                "need more than 1 value to unpack");
            goto Done;
        }

        unexpected = PyIter_Next(pair_iterator);
        if (unexpected != NULL) {
            Py_DECREF(unexpected);
            PyErr_SetString(PyExc_ValueError,
                            "too many values to unpack (expected 2)");
            goto Done;
        }
        else if (PyErr_Occurred())
            goto Done;

        res = PyObject_SetItem(self, key, value);

Done:
        Py_DECREF(pair);
        Py_XDECREF(pair_iterator);
        Py_XDECREF(key);
        Py_XDECREF(value);
        if (PyErr_Occurred())
            break;
    }
    Py_DECREF(iterator);

    if (res < 0 || PyErr_Occurred() != NULL)
        return -1;
    else
        return 0;
}

static PyObject *
mutablemapping_update(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int res = 0;
    Py_ssize_t len;
    _Py_IDENTIFIER(items);
    _Py_IDENTIFIER(keys);

    /* first handle args, if any */
    assert(args == NULL || PyTuple_Check(args));
    len = (args != NULL) ? PyTuple_GET_SIZE(args) : 0;
    if (len > 1) {
        const char *msg = "update() takes at most 1 positional argument (%zd given)";
        PyErr_Format(PyExc_TypeError, msg, len);
        return NULL;
    }

    if (len) {
        PyObject *func;
        PyObject *other = PyTuple_GET_ITEM(args, 0);  /* borrowed reference */
        assert(other != NULL);
        Py_INCREF(other);
        if (PyDict_CheckExact(other)) {
            PyObject *items = PyDict_Items(other);
            Py_DECREF(other);
            if (items == NULL)
                return NULL;
            res = mutablemapping_add_pairs(self, items);
            Py_DECREF(items);
            if (res == -1)
                return NULL;
            goto handle_kwargs;
        }

        if (_PyObject_LookupAttrId(other, &PyId_keys, &func) < 0) {
            Py_DECREF(other);
            return NULL;
        }
        if (func != NULL) {
            PyObject *keys, *iterator, *key;
            keys = _PyObject_CallNoArg(func);
            Py_DECREF(func);
            if (keys == NULL) {
                Py_DECREF(other);
                return NULL;
            }
            iterator = PyObject_GetIter(keys);
            Py_DECREF(keys);
            if (iterator == NULL) {
                Py_DECREF(other);
                return NULL;
            }
            while (res == 0 && (key = PyIter_Next(iterator))) {
                PyObject *value = PyObject_GetItem(other, key);
                if (value != NULL) {
                    res = PyObject_SetItem(self, key, value);
                    Py_DECREF(value);
                }
                else {
                    res = -1;
                }
                Py_DECREF(key);
            }
            Py_DECREF(other);
            Py_DECREF(iterator);
            if (res != 0 || PyErr_Occurred())
                return NULL;
            goto handle_kwargs;
        }

        if (_PyObject_LookupAttrId(other, &PyId_items, &func) < 0) {
            Py_DECREF(other);
            return NULL;
        }
        if (func != NULL) {
            PyObject *items;
            Py_DECREF(other);
            items = _PyObject_CallNoArg(func);
            Py_DECREF(func);
            if (items == NULL)
                return NULL;
            res = mutablemapping_add_pairs(self, items);
            Py_DECREF(items);
            if (res == -1)
                return NULL;
            goto handle_kwargs;
        }

        res = mutablemapping_add_pairs(self, other);
        Py_DECREF(other);
        if (res != 0)
            return NULL;
    }

  handle_kwargs:
    /* now handle kwargs */
    assert(kwargs == NULL || PyDict_Check(kwargs));
    if (kwargs != NULL && PyDict_GET_SIZE(kwargs)) {
        PyObject *items = PyDict_Items(kwargs);
        if (items == NULL)
            return NULL;
        res = mutablemapping_add_pairs(self, items);
        Py_DECREF(items);
        if (res == -1)
            return NULL;
    }

    Py_RETURN_NONE;
}
