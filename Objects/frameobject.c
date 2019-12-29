/* Frame object implementation */

#include "Python.h"
#include "pycore_object.h"
#include "pycore_pystate.h"

#include "code.h"
#include "frameobject.h"
#include "opcode.h"
#include "structmember.h"

#define OFF(x) offsetof(PyFrameObject, x)

static PyObject *_PyFastLocalsProxy_New(PyObject *frame);
static PyObject *_PyFastLocalsProxy_BorrowLocals(PyObject *flp);
static void _PyFastLocalsProxy_BreakReferenceCycle(PyObject *flp);

static PyMemberDef frame_memberlist[] = {
    {"f_back",          T_OBJECT,       OFF(f_back),      READONLY},
    {"f_code",          T_OBJECT,       OFF(f_code),      READONLY},
    {"f_builtins",      T_OBJECT,       OFF(f_builtins),  READONLY},
    {"f_globals",       T_OBJECT,       OFF(f_globals),   READONLY},
    {"f_lasti",         T_INT,          OFF(f_lasti),     READONLY},
    {"f_trace_lines",   T_BOOL,         OFF(f_trace_lines), 0},
    {"f_trace_opcodes", T_BOOL,         OFF(f_trace_opcodes), 0},
    {NULL}      /* Sentinel */
};

static PyObject *
_frame_get_updated_locals(PyFrameObject *f)
{
    if (PyFrame_FastToLocalsWithError(f) < 0)
        return NULL;
    assert(f->f_locals != NULL);
    return f->f_locals;
}

void _PyFrame_PostEvalCleanup(PyFrameObject *f)
{
    // Don't clean up still running coroutines and generators
    if (f->f_executing) {
        return;
    }

    // Ensure that any reference cycle between the frame and f_locals gets
    // broken when the frame finishes execution.
    if (f->f_locals && _PyFastLocalsProxy_CheckExact(f->f_locals)) {
        _PyFastLocalsProxy_BreakReferenceCycle(f->f_locals);
    }
}

PyObject *
PyFrame_GetLocalsAttribute(PyFrameObject *f)
{
    PyObject *updated_locals = _frame_get_updated_locals(f);
    Py_INCREF(updated_locals);
    return updated_locals;
}

PyObject *
_PyFrame_BorrowPyLocals(PyFrameObject *f)
{
    // This is called by PyEval_GetLocals(), which has historically returned
    // a borrowed reference, so this does the same
    PyObject *updated_locals = _frame_get_updated_locals(f);
    if (_PyFastLocalsProxy_CheckExact(updated_locals)) {
        updated_locals = _PyFastLocalsProxy_BorrowLocals(updated_locals);
    }
    return updated_locals;
}

PyObject *
PyFrame_GetPyLocals(PyFrameObject *f)
{
    PyObject *updated_locals = _frame_get_updated_locals(f);
    if (_PyFastLocalsProxy_CheckExact(updated_locals)) {
        // Take a snapshot of optimised scopes to avoid quirky side effects
        PyObject *d = _PyFastLocalsProxy_BorrowLocals(updated_locals);
        updated_locals = PyDict_Copy(d);
    } else {
        // Share a direct locals reference for class and module scopes
        Py_INCREF(updated_locals);
    }

    return updated_locals;
}

static PyObject *
frame_getlocals(PyFrameObject *f, void *__unused)
{
    return PyFrame_GetLocalsAttribute(f);
}

int
PyFrame_GetLineNumber(PyFrameObject *f)
{
    if (f->f_trace)
        return f->f_lineno;
    else
        return PyCode_Addr2Line(f->f_code, f->f_lasti);
}

static PyObject *
frame_getlineno(PyFrameObject *f, void *closure)
{
    return PyLong_FromLong(PyFrame_GetLineNumber(f));
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

typedef struct _codetracker {
    unsigned char *code;
    Py_ssize_t code_len;
    unsigned char *lnotab;
    Py_ssize_t lnotab_len;
    int start_line;
    int offset;
    int line;
    int addr;
    int line_addr;
} codetracker;

/* Reset the mutable parts of the tracker */
static void
reset(codetracker *tracker)
{
    tracker->offset = 0;
    tracker->addr = 0;
    tracker->line_addr = 0;
    tracker->line = tracker->start_line;
}

/* Initialise the tracker */
static void
init_codetracker(codetracker *tracker, PyCodeObject *code_obj)
{
    PyBytes_AsStringAndSize(code_obj->co_code,
                            (char **)&tracker->code, &tracker->code_len);
    PyBytes_AsStringAndSize(code_obj->co_lnotab,
                            (char **)&tracker->lnotab, &tracker->lnotab_len);
    tracker->start_line = code_obj->co_firstlineno;
    reset(tracker);
}

static void
advance_tracker(codetracker *tracker)
{
    tracker->addr += sizeof(_Py_CODEUNIT);
    if (tracker->offset >= tracker->lnotab_len) {
        return;
    }
    while (tracker->offset < tracker->lnotab_len &&
           tracker->addr >= tracker->line_addr + tracker->lnotab[tracker->offset]) {
        tracker->line_addr += tracker->lnotab[tracker->offset];
        tracker->line += (signed char)tracker->lnotab[tracker->offset+1];
        tracker->offset += 2;
    }
}


static void
retreat_tracker(codetracker *tracker)
{
    tracker->addr -= sizeof(_Py_CODEUNIT);
    while (tracker->addr < tracker->line_addr) {
        tracker->offset -= 2;
        tracker->line_addr -= tracker->lnotab[tracker->offset];
        tracker->line -= (signed char)tracker->lnotab[tracker->offset+1];
    }
}

static int
move_to_addr(codetracker *tracker, int addr)
{
    while (addr > tracker->addr) {
        advance_tracker(tracker);
        if (tracker->addr >= tracker->code_len) {
            return -1;
        }
    }
    while (addr < tracker->addr) {
        retreat_tracker(tracker);
        if (tracker->addr < 0) {
            return -1;
        }
    }
    return 0;
}

static int
first_line_not_before(codetracker *tracker, int line)
{
    int result = INT_MAX;
    reset(tracker);
    while (tracker->addr < tracker->code_len) {
        if (tracker->line == line) {
            return line;
        }
        if (tracker->line > line && tracker->line < result) {
            result = tracker->line;
        }
        advance_tracker(tracker);
    }
    if (result == INT_MAX) {
        return -1;
    }
    return result;
}

static int
move_to_nearest_start_of_line(codetracker *tracker, int line)
{
    if (line > tracker->line) {
        while (line != tracker->line) {
            advance_tracker(tracker);
            if (tracker->addr >= tracker->code_len) {
                return -1;
            }
        }
    }
    else {
        while (line != tracker->line) {
            retreat_tracker(tracker);
            if (tracker->addr < 0) {
                return -1;
            }
        }
        while (tracker->addr > tracker->line_addr) {
            retreat_tracker(tracker);
        }
    }
    return 0;
}

typedef struct _blockitem
{
    unsigned char kind;
    int end_addr;
    int start_line;
} blockitem;

typedef struct _blockstack
{
    blockitem stack[CO_MAXBLOCKS];
    int depth;
} blockstack;


static void
init_blockstack(blockstack *blocks)
{
    blocks->depth = 0;
}

static void
push_block(blockstack *blocks, unsigned char kind,
           int end_addr, int start_line)
{
    assert(blocks->depth < CO_MAXBLOCKS);
    blocks->stack[blocks->depth].kind = kind;
    blocks->stack[blocks->depth].end_addr = end_addr;
    blocks->stack[blocks->depth].start_line = start_line;
    blocks->depth++;
}

static unsigned char
pop_block(blockstack *blocks)
{
    assert(blocks->depth > 0);
    blocks->depth--;
    return blocks->stack[blocks->depth].kind;
}

static blockitem *
top_block(blockstack *blocks)
{
    assert(blocks->depth > 0);
    return &blocks->stack[blocks->depth-1];
}

static inline int
is_try_except(unsigned char op, int target_op)
{
    return op == SETUP_FINALLY && (target_op == DUP_TOP || target_op == POP_TOP);
}

static inline int
is_async_for(unsigned char op, int target_op)
{
    return op == SETUP_FINALLY && target_op == END_ASYNC_FOR;
}

static inline int
is_try_finally(unsigned char op, int target_op)
{
    return op == SETUP_FINALLY && !is_try_except(op, target_op) && !is_async_for(op, target_op);
}

/* Kind for finding except blocks in the jump to line code */
#define TRY_EXCEPT 250

static int
block_stack_for_line(codetracker *tracker, int line, blockstack *blocks)
{
    if (line < tracker->start_line) {
        return -1;
    }
    init_blockstack(blocks);
    reset(tracker);
    while (tracker->addr < tracker->code_len) {
        if (tracker->line == line) {
            return 0;
        }
        if (blocks->depth > 0 && tracker->addr == top_block(blocks)->end_addr) {
            unsigned char kind = pop_block(blocks);
            assert(kind != SETUP_FINALLY);
            if (kind == TRY_EXCEPT) {
                push_block(blocks, POP_EXCEPT, -1, tracker->line);
            }
            if (kind == SETUP_WITH || kind == SETUP_ASYNC_WITH) {
                push_block(blocks, WITH_EXCEPT_START, -1, tracker->line);
            }
        }
        unsigned char op = tracker->code[tracker->addr];
        if (op == SETUP_FINALLY || op == SETUP_ASYNC_WITH || op == SETUP_WITH || op == FOR_ITER) {
            unsigned int oparg = get_arg((const _Py_CODEUNIT *)tracker->code,
                                        tracker->addr / sizeof(_Py_CODEUNIT));
            int target_addr = tracker->addr + oparg + sizeof(_Py_CODEUNIT);
            int target_op = tracker->code[target_addr];
            if (is_async_for(op, target_op)) {
                push_block(blocks, FOR_ITER, target_addr, tracker->line);
            }
            else if (op == FOR_ITER) {
                push_block(blocks, FOR_ITER, target_addr-sizeof(_Py_CODEUNIT), tracker->line);
            }
            else if (is_try_except(op, target_op)) {
                push_block(blocks, TRY_EXCEPT, target_addr-sizeof(_Py_CODEUNIT), tracker->line);
            }
            else if (is_try_finally(op, target_op)) {
                int addr = tracker->addr;
                // Skip over duplicate 'finally' blocks if line is after body.
                move_to_addr(tracker, target_addr);
                if (tracker->line > line) {
                    // Target is in body, rewind to start.
                    move_to_addr(tracker, addr);
                    push_block(blocks, op, target_addr, tracker->line);
                }
                else {
                    // Now in finally block.
                    push_block(blocks, RERAISE, -1, tracker->line);
                }
            }
            else {
                push_block(blocks, op, target_addr, tracker->line);
            }
        }
        else if (op == RERAISE) {
            assert(blocks->depth > 0);
            unsigned char kind = top_block(blocks)->kind;
            if (kind == RERAISE || kind == WITH_EXCEPT_START || kind == POP_EXCEPT) {
                pop_block(blocks);
            }

        }
        advance_tracker(tracker);
    }
    return -1;
}

static void
frame_stack_pop(PyFrameObject *f)
{
    PyObject *v = (*--f->f_stacktop);
    Py_DECREF(v);
}

static void
frame_block_unwind(PyFrameObject *f)
{
    assert(f->f_iblock > 0);
    f->f_iblock--;
    PyTryBlock *b = &f->f_blockstack[f->f_iblock];
    int delta = (f->f_stacktop - f->f_valuestack) - b->b_level;
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

    /* Upon the 'call' trace event of a new frame, f->f_lasti is -1 and
     * f->f_trace is NULL, check first on the first condition.
     * Forbidding jumps from the 'call' event of a new frame is a side effect
     * of allowing to set f_lineno only from trace functions. */
    if (f->f_lasti == -1) {
        PyErr_Format(PyExc_ValueError,
                     "can't jump from the 'call' trace event of a new frame");
        return -1;
    }

    /* You can only do this from within a trace function, not via
     * _getframe or similar hackery. */
    if (!f->f_trace) {
        PyErr_Format(PyExc_ValueError,
                     "f_lineno can only be set by a trace function");
        return -1;
    }

    /* Forbid jumps upon a 'return' trace event (except after executing a
     * YIELD_VALUE or YIELD_FROM opcode, f_stacktop is not NULL in that case)
     * and upon an 'exception' trace event.
     * Jumps from 'call' trace events have already been forbidden above for new
     * frames, so this check does not change anything for 'call' events. */
    if (f->f_stacktop == NULL) {
        PyErr_SetString(PyExc_ValueError,
                "can only jump from a 'line' trace event");
        return -1;
    }


    codetracker tracker;
    init_codetracker(&tracker, f->f_code);
    move_to_addr(&tracker, f->f_lasti);
    int current_line = tracker.line;
    assert(current_line >= 0);
    int new_lineno;

    {
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

        new_lineno = first_line_not_before(&tracker, new_lineno);
        if (new_lineno < 0) {
            PyErr_Format(PyExc_ValueError,
                        "line %d comes after the current code block",
                        (int)l_new_lineno);
            return -1;
        }
    }

    if (tracker.code[f->f_lasti] == YIELD_VALUE || tracker.code[f->f_lasti] == YIELD_FROM) {
        PyErr_SetString(PyExc_ValueError,
                "can't jump from a 'yield' statement");
        return -1;
    }

    /* Find block stack for current line and target line. */
    blockstack current_stack, new_stack;
    block_stack_for_line(&tracker, new_lineno, &new_stack);
    block_stack_for_line(&tracker, current_line, &current_stack);

    /* The trace function is called with a 'return' trace event after the
     * execution of a yield statement. */
    if (tracker.code[tracker.addr] == DUP_TOP || tracker.code[tracker.addr] == POP_TOP) {
        PyErr_SetString(PyExc_ValueError,
            "can't jump to 'except' line as there's no exception");
        return -1;
    }

    /* Validate change of block stack. */
    if (new_stack.depth > 0) {
        blockitem *current_block_at_new_depth = &(current_stack.stack[new_stack.depth-1]);
        if (new_stack.depth > current_stack.depth ||
            top_block(&new_stack)->start_line != current_block_at_new_depth->start_line) {
            unsigned char target_kind = top_block(&new_stack)->kind;
            char *msg;
            if (target_kind == POP_EXCEPT) {
                msg = "can't jump into an 'except' block as there's no exception";
            }
            else if (target_kind == RERAISE) {
                msg = "can't jump into a 'finally' block";
            }
            else {
                msg = "can't jump into the middle of a block";
            }
            PyErr_SetString(PyExc_ValueError, msg);
            return -1;
        }
    }

    /* Check for illegal jumps out of finally or except blocks. */
    for (int depth = new_stack.depth; depth < current_stack.depth; depth++) {
        switch(current_stack.stack[depth].kind) {
        case RERAISE:
            PyErr_SetString(PyExc_ValueError,
                "can't jump out of a 'finally' block");
            return -1;
        case POP_EXCEPT:
            PyErr_SetString(PyExc_ValueError,
                "can't jump out of an 'except' block");
            return -1;
        }
    }

    /* Unwind block stack. */
    while (current_stack.depth > new_stack.depth) {
        unsigned char kind = pop_block(&current_stack);
        switch(kind) {
        case FOR_ITER:
            frame_stack_pop(f);
            break;
        case SETUP_FINALLY:
        case TRY_EXCEPT:
            frame_block_unwind(f);
            break;
        case SETUP_WITH:
        case SETUP_ASYNC_WITH:
            frame_block_unwind(f);
            // Pop the exit function
            frame_stack_pop(f);
            break;
        default:
            PyErr_SetString(PyExc_SystemError,
                "unexpected block kind");
            return -1;
        }
    }

    move_to_addr(&tracker, f->f_lasti);
    move_to_nearest_start_of_line(&tracker, new_lineno);

    /* Finally set the new f_lineno and f_lasti and return OK. */
    f->f_lineno = new_lineno;
    f->f_lasti = tracker.addr;
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
    /* We rely on f_lineno being accurate when f_trace is set. */
    f->f_lineno = PyFrame_GetLineNumber(f);

    if (v == Py_None)
        v = NULL;
    Py_XINCREF(v);
    Py_XSETREF(f->f_trace, v);

    return 0;
}


static PyGetSetDef frame_getsetlist[] = {
    {"f_locals",        (getter)frame_getlocals, NULL, NULL},
    {"f_lineno",        (getter)frame_getlineno,
                    (setter)frame_setlineno, NULL},
    {"f_trace",         (getter)frame_gettrace, (setter)frame_settrace, NULL},
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

static PyFrameObject *free_list = NULL;
static int numfree = 0;         /* number of frames currently in free_list */
/* max value for numfree */
#define PyFrame_MAXFREELIST 200

static void _Py_HOT_FUNCTION
frame_dealloc(PyFrameObject *f)
{
    PyObject **p, **valuestack;
    PyCodeObject *co;

    if (_PyObject_GC_IS_TRACKED(f))
        _PyObject_GC_UNTRACK(f);

    Py_TRASHCAN_SAFE_BEGIN(f)
    /* Kill all local variables */
    valuestack = f->f_valuestack;
    for (p = f->f_localsplus; p < valuestack; p++)
        Py_CLEAR(*p);

    /* Free stack */
    if (f->f_stacktop != NULL) {
        for (p = valuestack; p < f->f_stacktop; p++)
            Py_XDECREF(*p);
    }

    Py_XDECREF(f->f_back);
    Py_DECREF(f->f_builtins);
    Py_DECREF(f->f_globals);
    Py_CLEAR(f->f_locals);
    Py_CLEAR(f->f_trace);

    co = f->f_code;
    if (co->co_zombieframe == NULL)
        co->co_zombieframe = f;
    else if (numfree < PyFrame_MAXFREELIST) {
        ++numfree;
        f->f_back = free_list;
        free_list = f;
    }
    else
        PyObject_GC_Del(f);

    Py_DECREF(co);
    Py_TRASHCAN_SAFE_END(f)
}

static int
frame_traverse(PyFrameObject *f, visitproc visit, void *arg)
{
    PyObject **fastlocals, **p;
    Py_ssize_t i, slots;

    Py_VISIT(f->f_back);
    Py_VISIT(f->f_code);
    Py_VISIT(f->f_builtins);
    Py_VISIT(f->f_globals);
    Py_VISIT(f->f_locals);
    Py_VISIT(f->f_trace);

    /* locals */
    slots = f->f_code->co_nlocals + PyTuple_GET_SIZE(f->f_code->co_cellvars) + PyTuple_GET_SIZE(f->f_code->co_freevars);
    fastlocals = f->f_localsplus;
    for (i = slots; --i >= 0; ++fastlocals)
        Py_VISIT(*fastlocals);

    /* stack */
    if (f->f_stacktop != NULL) {
        for (p = f->f_valuestack; p < f->f_stacktop; p++)
            Py_VISIT(*p);
    }
    return 0;
}

static int
frame_tp_clear(PyFrameObject *f)
{
    PyObject **fastlocals, **p, **oldtop;
    Py_ssize_t i, slots;

    /* Before anything else, make sure that this frame is clearly marked
     * as being defunct!  Else, e.g., a generator reachable from this
     * frame may also point to this frame, believe itself to still be
     * active, and try cleaning up this frame again.
     */
    oldtop = f->f_stacktop;
    f->f_stacktop = NULL;
    f->f_executing = 0;

    Py_CLEAR(f->f_trace);

    /* locals */
    slots = f->f_code->co_nlocals + PyTuple_GET_SIZE(f->f_code->co_cellvars) + PyTuple_GET_SIZE(f->f_code->co_freevars);
    fastlocals = f->f_localsplus;
    for (i = slots; --i >= 0; ++fastlocals)
        Py_CLEAR(*fastlocals);

    Py_CLEAR(f->f_locals);

    /* stack */
    if (oldtop != NULL) {
        for (p = f->f_valuestack; p < oldtop; p++)
            Py_CLEAR(*p);
    }
    return 0;
}

static PyObject *
frame_clear(PyFrameObject *f, PyObject *Py_UNUSED(ignored))
{
    if (f->f_executing) {
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

    ncells = PyTuple_GET_SIZE(f->f_code->co_cellvars);
    nfrees = PyTuple_GET_SIZE(f->f_code->co_freevars);
    extras = f->f_code->co_stacksize + f->f_code->co_nlocals +
             ncells + nfrees;
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
    return PyUnicode_FromFormat(
        "<frame at %p, file %R, line %d, code %S>",
        f, f->f_code->co_filename, lineno, f->f_code->co_name);
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

PyFrameObject* _Py_HOT_FUNCTION
_PyFrame_New_NoTrack(PyThreadState *tstate, PyCodeObject *code,
                     PyObject *globals, PyObject *locals)
{
    PyFrameObject *back = tstate->frame;
    PyFrameObject *f;
    PyObject *builtins;
    Py_ssize_t i;

#ifdef Py_DEBUG
    if (code == NULL || globals == NULL || !PyDict_Check(globals) ||
        (locals != NULL && !PyMapping_Check(locals))) {
        PyErr_BadInternalCall();
        return NULL;
    }
#endif
    if (back == NULL || back->f_globals != globals) {
        builtins = _PyDict_GetItemIdWithError(globals, &PyId___builtins__);
        if (builtins) {
            if (PyModule_Check(builtins)) {
                builtins = PyModule_GetDict(builtins);
                assert(builtins != NULL);
            }
        }
        if (builtins == NULL) {
            if (PyErr_Occurred()) {
                return NULL;
            }
            /* No builtins!              Make up a minimal one
               Give them 'None', at least. */
            builtins = PyDict_New();
            if (builtins == NULL ||
                PyDict_SetItemString(
                    builtins, "None", Py_None) < 0)
                return NULL;
        }
        else
            Py_INCREF(builtins);

    }
    else {
        /* If we share the globals, we share the builtins.
           Save a lookup and a call. */
        builtins = back->f_builtins;
        assert(builtins != NULL);
        Py_INCREF(builtins);
    }
    if (code->co_zombieframe != NULL) {
        f = code->co_zombieframe;
        code->co_zombieframe = NULL;
        _Py_NewReference((PyObject *)f);
        assert(f->f_code == code);
    }
    else {
        Py_ssize_t extras, ncells, nfrees;
        ncells = PyTuple_GET_SIZE(code->co_cellvars);
        nfrees = PyTuple_GET_SIZE(code->co_freevars);
        extras = code->co_stacksize + code->co_nlocals + ncells +
            nfrees;
        if (free_list == NULL) {
            f = PyObject_GC_NewVar(PyFrameObject, &PyFrame_Type,
            extras);
            if (f == NULL) {
                Py_DECREF(builtins);
                return NULL;
            }
        }
        else {
            assert(numfree > 0);
            --numfree;
            f = free_list;
            free_list = free_list->f_back;
            if (Py_SIZE(f) < extras) {
                PyFrameObject *new_f = PyObject_GC_Resize(PyFrameObject, f, extras);
                if (new_f == NULL) {
                    PyObject_GC_Del(f);
                    Py_DECREF(builtins);
                    return NULL;
                }
                f = new_f;
            }
            _Py_NewReference((PyObject *)f);
        }

        f->f_code = code;
        extras = code->co_nlocals + ncells + nfrees;
        f->f_valuestack = f->f_localsplus + extras;
        for (i=0; i<extras; i++)
            f->f_localsplus[i] = NULL;
        f->f_locals = NULL;
        f->f_trace = NULL;
    }
    f->f_stacktop = f->f_valuestack;
    f->f_builtins = builtins;
    Py_XINCREF(back);
    f->f_back = back;
    Py_INCREF(code);
    Py_INCREF(globals);
    f->f_globals = globals;
    /* Most functions have CO_NEWLOCALS and CO_OPTIMIZED set. */
    if ((code->co_flags & (CO_NEWLOCALS | CO_OPTIMIZED)) ==
        (CO_NEWLOCALS | CO_OPTIMIZED))
        ; /* f_locals = NULL; will be set by PyFrame_FastToLocals() */
    else if (code->co_flags & CO_NEWLOCALS) {
        locals = PyDict_New();
        if (locals == NULL) {
            Py_DECREF(f);
            return NULL;
        }
        f->f_locals = locals;
    }
    else {
        if (locals == NULL)
            locals = globals;
        Py_INCREF(locals);
        f->f_locals = locals;
    }

    f->f_lasti = -1;
    f->f_lineno = code->co_firstlineno;
    f->f_iblock = 0;
    f->f_executing = 0;
    f->f_gen = NULL;
    f->f_trace_opcodes = 0;
    f->f_trace_lines = 1;

    return f;
}

PyFrameObject*
PyFrame_New(PyThreadState *tstate, PyCodeObject *code,
            PyObject *globals, PyObject *locals)
{
    PyFrameObject *f = _PyFrame_New_NoTrack(tstate, code, globals, locals);
    if (f)
        _PyObject_GC_TRACK(f);
    return f;
}


/* Block management */

void
PyFrame_BlockSetup(PyFrameObject *f, int type, int handler, int level)
{
    PyTryBlock *b;
    if (f->f_iblock >= CO_MAXBLOCKS)
        Py_FatalError("XXX block stack overflow");
    b = &f->f_blockstack[f->f_iblock++];
    b->b_type = type;
    b->b_level = level;
    b->b_handler = handler;
}

PyTryBlock *
PyFrame_BlockPop(PyFrameObject *f)
{
    PyTryBlock *b;
    if (f->f_iblock <= 0)
        Py_FatalError("XXX block stack underflow");
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
    co = f->f_code;
    locals = f->f_locals;
    if (locals == NULL) {
        if (co->co_flags & CO_OPTIMIZED) {
            /* PEP 558: If this is an optimized frame, ensure f_locals is a
            * fastlocalsproxy instance, while locals refers to the underlying mapping.
            */
            PyObject *flp = _PyFastLocalsProxy_New((PyObject *) f);
            if (flp == NULL) {
                return -1;
            }
            f->f_locals = flp;
            locals = _PyFastLocalsProxy_BorrowLocals(flp);
        } else {
            locals = f->f_locals = PyDict_New();
            if (locals == NULL)
                return -1;
        }
    } else if (_PyFastLocalsProxy_CheckExact(locals)) {
        locals = _PyFastLocalsProxy_BorrowLocals(locals);
    }

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
        /* Passing deref to map_to_dict means we'll replace cells with their
           values in the namespace.
        */
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
    PyErr_SetString(
        PyExc_RuntimeError,
        "PyFrame_LocalsToFast is no longer supported. "
        "Use PyFrame_GetPyLocals() or PyFrame_GetLocalsAttribute() instead."
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

static int
add_local_refs(PyObject *map, Py_ssize_t nmap, PyObject *fast_refs)
{
    /* Populate a lookup table from variable names to fast locals array indices */
    Py_ssize_t j;
    assert(PyTuple_Check(map));
    assert(PyDict_Check(fast_refs));
    assert(PyTuple_Size(map) >= nmap);
    for (j = nmap; --j >= 0; ) {
        PyObject *key = PyTuple_GET_ITEM(map, j);
        PyObject *value = PyLong_FromSsize_t(j); // set_fast_ref steals the value
        /* Values may be missing if the frame has been cleared */
        if (value != NULL) {
            if (set_fast_ref(fast_refs, key, value) != 0) {
                return -1;
            }
        }
    }
    return 0;
}

static int
add_nonlocal_refs(PyObject *map, Py_ssize_t nmap, PyObject *fast_refs, PyObject **cells)
{
    /* Populate a lookup table from variable names to closure cell references */
    Py_ssize_t j;
    assert(PyTuple_Check(map));
    assert(PyDict_Check(fast_refs));
    assert(PyTuple_Size(map) >= nmap);
    for (j = nmap; --j >= 0; ) {
        PyObject *key = PyTuple_GET_ITEM(map, j);
        PyObject *value = cells[j];
        /* Values may be missing if the frame has been cleared */
        if (value != NULL) {
            assert(PyCell_Check(value));
            Py_INCREF(value); // set_fast_ref steals the value
            if (set_fast_ref(fast_refs, key, value) != 0) {
                return -1;
            }
        }
    }
    return 0;
}


static PyObject *
_PyFrame_BuildFastRefs(PyFrameObject *f)
{
    PyObject *fast_refs, *map;
    PyObject **fast;
    PyCodeObject *co;
    Py_ssize_t j;
    Py_ssize_t ncells, nfreevars;

    /* Construct a combined mapping from local variable names to indices
     * in the fast locals array, and from nonlocal variable names directly
     * to the corresponding cell objects
     */
    co = f->f_code;
    if (!(co->co_flags & CO_OPTIMIZED)) {
        PyErr_SetString(PyExc_SystemError,
            "attempted to build fast refs lookup table for non-optimized scope");
        return NULL;
    }
    map = co->co_varnames;
    if (!PyTuple_Check(map)) {
        PyErr_Format(PyExc_SystemError,
                     "co_varnames must be a tuple, not %s",
                     Py_TYPE(map)->tp_name);
        return NULL;
    }
    fast_refs = PyDict_New();
    if (fast_refs == NULL) {
        return NULL;
    }
    j = PyTuple_GET_SIZE(map);
    if (j > co->co_nlocals)
        j = co->co_nlocals;
    if (co->co_nlocals) {
        if (add_local_refs(map, j, fast_refs) < 0) {
            Py_DECREF(fast_refs);
            return NULL;
        }
    }
    fast = f->f_localsplus;
    ncells = PyTuple_GET_SIZE(co->co_cellvars);
    if (ncells) {
        if (add_nonlocal_refs(co->co_cellvars, ncells,
                              fast_refs, fast + co->co_nlocals)) {
            Py_DECREF(fast_refs);
            return NULL;
        }
    }

    nfreevars = PyTuple_GET_SIZE(co->co_freevars);
    if (nfreevars) {
        if (add_nonlocal_refs(co->co_freevars, nfreevars,
                             fast_refs, fast + co->co_nlocals + ncells) < 0) {
            Py_DECREF(fast_refs);
            return NULL;
        }
    }

    return fast_refs;

}

/* Clear out the free list */
int
PyFrame_ClearFreeList(void)
{
    int freelist_size = numfree;

    while (free_list != NULL) {
        PyFrameObject *f = free_list;
        free_list = free_list->f_back;
        PyObject_GC_Del(f);
        --numfree;
    }
    assert(numfree == 0);
    return freelist_size;
}

void
_PyFrame_Fini(void)
{
    (void)PyFrame_ClearFreeList();
}

/* Print summary info about the state of the optimized allocator */
void
_PyFrame_DebugMallocStats(FILE *out)
{
    _PyDebugAllocatorStats(out,
                           "free PyFrameObject",
                           numfree, sizeof(PyFrameObject));
}

/* PyFastLocalsProxy_Type
 *
 * Subclass of PyDict_Proxy (currently defined in descrobject.h/.c)
 *
 * Mostly works just like PyDict_Proxy (backed by the frame locals), but
 * supports setitem and delitem, with writes being delegated to both the
 * referenced mapping *and* the fast locals and/or cell reference on the
 * frame.
 */
/*[clinic input]
class fastlocalsproxy "fastlocalsproxyobject *" "&PyFastLocalsProxy_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=b0e135835cface9f]*/


typedef struct {
    PyObject_HEAD           /* Match mappingproxyobject in descrobject.c */
    PyObject      *mapping; /* Match mappingproxyobject in descrobject.c */
    PyFrameObject *frame;
    PyObject      *fast_refs; /* Cell refs and local variable indices */
} fastlocalsproxyobject;

/* Provide __setitem__() and __delitem__() implementations that not only
 * write to the namespace returned by locals(), but also write to the frame
 * storage directly (either the closure cells or the fast locals array)
 */

static int
fastlocalsproxy_write_to_frame(fastlocalsproxyobject *flp, PyObject *key, PyObject *value)
{
    int result = 0;
    assert(PyDict_Check(flp->fast_refs));
    PyObject *fast_ref = PyDict_GetItem(flp->fast_refs, key);
    if (fast_ref != NULL) {
        /* Key is an actual Python variable, so update that reference */
        if (PyCell_Check(fast_ref)) {
            // Closure cells can be updated even after the frame terminates
            result = PyCell_Set(fast_ref, value);
        } else if (flp->frame == NULL) {
            // This indicates the frame has finished executing and the link
            // back to the frame has been cleared to break the reference cycle
            return 0;
        } else {
            /* Fast ref is a Python int mapping into the fast locals array */
            Py_ssize_t offset = PyLong_AsSsize_t(fast_ref);
            Py_ssize_t max_offset = flp->frame->f_code->co_nlocals;
            if (offset < 0) {
                result = -1;
            } else if (offset > max_offset) {
                PyErr_Format(PyExc_SystemError,
                             "Fast locals ref (%zd) exceeds array bound (%zd)",
                             offset, max_offset);
                result = -1;
            }
            if (result == 0) {
                Py_INCREF(value);
                Py_XSETREF(flp->frame->f_localsplus[offset], value);
            }
        }
    }
    return result;
}

static int
fastlocalsproxy_setitem(fastlocalsproxyobject *flp, PyObject *key, PyObject *value)
{
    int result;
    result = PyDict_SetItem(flp->mapping, key, value);
    if (result == 0) {
        result = fastlocalsproxy_write_to_frame(flp, key, value);
    }
    return result;
}

static int
fastlocalsproxy_delitem(fastlocalsproxyobject *flp, PyObject *key)
{
    int result;
    result = PyDict_DelItem(flp->mapping, key);
    if (result == 0) {
        result = fastlocalsproxy_write_to_frame(flp, key, NULL);
    }
    return result;
}

static int
fastlocalsproxy_mp_assign_subscript(PyObject *flp, PyObject *key, PyObject *value)
{
    if (value == NULL)
        return fastlocalsproxy_delitem((fastlocalsproxyobject *)flp, key);
    else
        return fastlocalsproxy_setitem((fastlocalsproxyobject *)flp, key, value);
}

static PyMappingMethods fastlocalsproxy_as_mapping = {
    0,                                    /* mp_length */
    0,                                    /* mp_subscript */
    fastlocalsproxy_mp_assign_subscript,  /* mp_ass_subscript */
};


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

    PyErr_Format(PyExc_NotImplementedError,
                 "FastLocalsProxy does not yet implement setdefault()");
    return value;
}


/* pop() */

PyDoc_STRVAR(fastlocalsproxy_pop__doc__,
"flp.pop(k[,d]) -> v, remove specified key and return the corresponding\n\
        value.  If key is not found, d is returned if given, otherwise KeyError\n\
        is raised.");

/* forward */
static PyObject * _fastlocalsproxy_popkey(PyObject *, PyObject *, PyObject *);

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

    return _fastlocalsproxy_popkey(flp, key, failobj);
}

static PyObject *
_fastlocalsproxy_popkey(PyObject *flp, PyObject *key, PyObject *failobj)
{
    // TODO: Similar to the odict implementation, the fast locals proxy
    // could benefit from an internal API that accepts already calculated
    // hashes, rather than recalculating the hash multiple times for the
    // same key in a single operation (see _odict_popkey_hash)

    PyObject *value = NULL;

    // Just implement naive lookup through the abstract C API for now
    int exists = PySequence_Contains(flp, key);
    if (exists < 0)
        return NULL;
    if (exists) {
        value = PyObject_GetItem(flp, key);
        if (value != NULL) {
            if (PyObject_DelItem(flp, key) == -1) {
                Py_CLEAR(value);
            }
        }
    }

    /* Apply the fallback value, if necessary. */
    if (value == NULL && !PyErr_Occurred()) {
        if (failobj) {
            value = failobj;
            Py_INCREF(failobj);
        }
        else {
            PyErr_SetObject(PyExc_KeyError, key);
        }
    }

    return value;
}

/* popitem() */

PyDoc_STRVAR(fastlocalsproxy_popitem__doc__,
"flp.popitem() -> (k, v), remove and return some (key, value) pair as a\n\
        2-tuple; but raise KeyError if D is empty.");

static PyObject *
fastlocalsproxy_popitem(PyObject *flp, PyObject *Py_UNUSED(ignored))
{
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
             "flp.clear() -> None.  Remove all items from snapshot and frame.");

static PyObject *
fastlocalsproxy_clear(register PyObject *flp, PyObject *Py_UNUSED(ignored))
{
    PyErr_Format(PyExc_NotImplementedError,
                 "FastLocalsProxy does not yet implement clear()");
    return NULL;
}

static PyMethodDef fastlocalsproxy_methods[] = {
    {"setdefault",      (PyCFunction)(void(*)(void))fastlocalsproxy_setdefault,
     METH_VARARGS | METH_KEYWORDS, fastlocalsproxy_setdefault__doc__},
    {"pop",             (PyCFunction)(void(*)(void))fastlocalsproxy_pop,
     METH_VARARGS | METH_KEYWORDS, fastlocalsproxy_pop__doc__},
    {"clear",           (PyCFunction)fastlocalsproxy_popitem,
     METH_NOARGS, fastlocalsproxy_popitem__doc__},
    {"update",          (PyCFunction)(void(*)(void))fastlocalsproxy_update,
     METH_VARARGS | METH_KEYWORDS, fastlocalsproxy_update__doc__},
    {"clear",           (PyCFunction)fastlocalsproxy_clear,
     METH_NOARGS, fastlocalsproxy_clear__doc__},

    {NULL, NULL}   /* sentinel */
};

static void
fastlocalsproxy_dealloc(fastlocalsproxyobject *flp)
{
    if (_PyObject_GC_IS_TRACKED(flp))
        _PyObject_GC_UNTRACK(flp);

    Py_TRASHCAN_SAFE_BEGIN(flp)

    Py_CLEAR(flp->mapping);
    Py_CLEAR(flp->frame);
    Py_CLEAR(flp->fast_refs);
    PyObject_GC_Del(flp);

    Py_TRASHCAN_SAFE_END(flp)
}

static PyObject *
fastlocalsproxy_repr(fastlocalsproxyobject *flp)
{
    return PyUnicode_FromFormat("fastlocalsproxy(%R)", flp->mapping);
}

static int
fastlocalsproxy_traverse(PyObject *self, visitproc visit, void *arg)
{
    fastlocalsproxyobject *flp = (fastlocalsproxyobject *)self;
    Py_VISIT(flp->mapping);
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
    if (!(frame->f_code->co_flags & CO_OPTIMIZED)) {
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
    PyObject *mapping, *fast_refs;

    if (fastlocalsproxy_check_frame(frame) == -1)
        return NULL;

    flp = PyObject_GC_New(fastlocalsproxyobject, &PyFastLocalsProxy_Type);
    if (flp == NULL)
        return NULL;
    mapping = PyDict_New();
    if (mapping == NULL) {
        Py_DECREF(flp);
        return NULL;
    }
    flp->mapping = mapping;
    Py_INCREF(frame);
    flp->frame = (PyFrameObject *) frame;
    fast_refs = _PyFrame_BuildFastRefs(flp->frame);
    if (fast_refs == NULL) {
        Py_DECREF(flp); // Also handles DECREF for mapping and frame
        return NULL;
    }
    flp->fast_refs = fast_refs;
    _PyObject_GC_TRACK(flp);
    return (PyObject *)flp;
}

static PyObject *
_PyFastLocalsProxy_BorrowLocals(PyObject *self)
{
    assert(_PyFastLocalsProxy_CheckExact(self));
    return ((fastlocalsproxyobject *) self)->mapping;
}

static void
_PyFastLocalsProxy_BreakReferenceCycle(PyObject *self)
{
    assert(_PyFastLocalsProxy_CheckExact(self));
    fastlocalsproxyobject *flp = (fastlocalsproxyobject *) self;
    Py_CLEAR(flp->frame);
    // We keep flp->fast_refs alive to allow updating of closure variables
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

PyTypeObject PyFastLocalsProxy_Type = {
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
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    &fastlocalsproxy_as_mapping,                /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    0,                                          /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,    /* tp_flags */
    0,                                          /* tp_doc */
    (traverseproc)fastlocalsproxy_traverse,     /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    fastlocalsproxy_methods,                    /* tp_methods */
    0,                                          /* tp_members */
    0,                                          /* tp_getset */
    &PyDictProxy_Type,                          /* tp_base */
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
// TODO: move the duplicated code to abstract.c and expose it to the
//       linker as a private API
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
