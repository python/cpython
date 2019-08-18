/* Frame object implementation */

#include "Python.h"
#include "pycore_object.h"
#include "pycore_pystate.h"

#include "code.h"
#include "frameobject.h"
#include "opcode.h"
#include "structmember.h"

#define OFF(x) offsetof(PyFrameObject, x)

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
 *    the END_FINALLY expects to clean up the stack after the 'try' block.
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
    int new_lineno = 0;                 /* The new value of f_lineno */
    long l_new_lineno;
    int overflow;
    int new_lasti = 0;                  /* The new value of f_lasti */
    unsigned char *code = NULL;         /* The bytecode for the frame... */
    Py_ssize_t code_len = 0;            /* ...and its length */
    unsigned char *lnotab = NULL;       /* Iterating over co_lnotab */
    Py_ssize_t lnotab_len = 0;          /* (ditto) */
    int offset = 0;                     /* (ditto) */
    int line = 0;                       /* (ditto) */
    int addr = 0;                       /* (ditto) */
    int delta_iblock = 0;               /* Scanning the SETUPs and POPs */
    int delta = 0;
    int blockstack[CO_MAXBLOCKS];       /* Walking the 'finally' blocks */
    int blockstack_top = 0;             /* (ditto) */

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

    /* Fail if the line comes before the start of the code block. */
    l_new_lineno = PyLong_AsLongAndOverflow(p_new_lineno, &overflow);
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
    else if (new_lineno == f->f_code->co_firstlineno) {
        new_lasti = 0;
        new_lineno = f->f_code->co_firstlineno;
    }
    else {
        /* Find the bytecode offset for the start of the given
         * line, or the first code-owning line after it. */
        char *tmp;
        PyBytes_AsStringAndSize(f->f_code->co_lnotab,
                                &tmp, &lnotab_len);
        lnotab = (unsigned char *) tmp;
        addr = 0;
        line = f->f_code->co_firstlineno;
        new_lasti = -1;
        for (offset = 0; offset < lnotab_len; offset += 2) {
            addr += lnotab[offset];
            line += (signed char)lnotab[offset+1];
            if (line >= new_lineno) {
                new_lasti = addr;
                new_lineno = line;
                break;
            }
        }
    }

    /* If we didn't reach the requested line, return an error. */
    if (new_lasti == -1) {
        PyErr_Format(PyExc_ValueError,
                     "line %d comes after the current code block",
                     new_lineno);
        return -1;
    }

    /* We're now ready to look at the bytecode. */
    PyBytes_AsStringAndSize(f->f_code->co_code, (char **)&code, &code_len);

    /* The trace function is called with a 'return' trace event after the
     * execution of a yield statement. */
    assert(f->f_lasti != -1);
    if (code[f->f_lasti] == YIELD_VALUE || code[f->f_lasti] == YIELD_FROM) {
        PyErr_SetString(PyExc_ValueError,
                "can't jump from a yield statement");
        return -1;
    }

    /* You can't jump onto a line with an 'except' statement on it -
     * they expect to have an exception on the top of the stack, which
     * won't be true if you jump to them.  They always start with code
     * that either pops the exception using POP_TOP (plain 'except:'
     * lines do this) or duplicates the exception on the stack using
     * DUP_TOP (if there's an exception type specified).  See compile.c,
     * 'com_try_except' for the full details.  There aren't any other
     * cases (AFAIK) where a line's code can start with DUP_TOP or
     * POP_TOP, but if any ever appear, they'll be subject to the same
     * restriction (but with a different error message). */
    if (code[new_lasti] == DUP_TOP || code[new_lasti] == POP_TOP) {
        PyErr_SetString(PyExc_ValueError,
            "can't jump to 'except' line as there's no exception");
        return -1;
    }

    /* You can't jump into or out of a 'finally' block because the 'try'
     * block leaves something on the stack for the END_FINALLY to clean up.
     * So we walk the bytecode, maintaining a simulated blockstack.
     * 'blockstack' is a stack of the bytecode addresses of the starts of
     * the 'finally' blocks. */
    memset(blockstack, '\0', sizeof(blockstack));
    blockstack_top = 0;
    unsigned char prevop = NOP;
    for (addr = 0; addr < code_len; addr += sizeof(_Py_CODEUNIT)) {
        unsigned char op = code[addr];
        switch (op) {
        case SETUP_FINALLY:
        case SETUP_WITH:
        case SETUP_ASYNC_WITH:
        case FOR_ITER: {
            unsigned int oparg = get_arg((const _Py_CODEUNIT *)code,
                                         addr / sizeof(_Py_CODEUNIT));
            int target_addr = addr + oparg + sizeof(_Py_CODEUNIT);
            assert(target_addr < code_len);
            /* Police block-jumping (you can't jump into the middle of a block)
             * and ensure that the blockstack finishes up in a sensible state (by
             * popping any blocks we're jumping out of).  We look at all the
             * blockstack operations between the current position and the new
             * one, and keep track of how many blocks we drop out of on the way.
             * By also keeping track of the lowest blockstack position we see, we
             * can tell whether the jump goes into any blocks without coming out
             * again - in that case we raise an exception below. */
            int first_in = addr < f->f_lasti && f->f_lasti < target_addr;
            int second_in = addr < new_lasti && new_lasti < target_addr;
            if (!first_in && second_in) {
                PyErr_SetString(PyExc_ValueError,
                                "can't jump into the middle of a block");
                return -1;
            }
            int in_for_loop = op == FOR_ITER || code[target_addr] == END_ASYNC_FOR;
            if (first_in && !second_in) {
                if (!delta_iblock) {
                    if (in_for_loop) {
                        /* Pop the iterators of any 'for' and 'async for' loop
                         * we're jumping out of. */
                        delta++;
                    }
                    else if (prevop == LOAD_CONST) {
                        /* Pops None pushed before SETUP_FINALLY. */
                        delta++;
                    }
                }
                if (!in_for_loop) {
                    delta_iblock++;
                }
            }
            if (!in_for_loop) {
                blockstack[blockstack_top++] = target_addr;
            }
            break;
        }

        case END_FINALLY: {
            assert(blockstack_top > 0);
            int target_addr = blockstack[--blockstack_top];
            assert(target_addr <= addr);
            int first_in = target_addr <= f->f_lasti && f->f_lasti <= addr;
            int second_in = target_addr <= new_lasti && new_lasti <= addr;
            if (first_in != second_in) {
                op = code[target_addr];
                PyErr_Format(PyExc_ValueError,
                             "can't jump %s %s block",
                             second_in ? "into" : "out of",
                             (op == DUP_TOP || op == POP_TOP) ?
                                "an 'except'" : "a 'finally'");
                return -1;
            }
            break;
        }
        }
        prevop = op;
    }

    /* Verify that the blockstack tracking code didn't get lost. */
    assert(blockstack_top == 0);

    /* Pop any blocks that we're jumping out of. */
    if (delta_iblock > 0) {
        f->f_iblock -= delta_iblock;
        PyTryBlock *b = &f->f_blockstack[f->f_iblock];
        delta += (int)(f->f_stacktop - f->f_valuestack) - b->b_level;
        if (b->b_type == SETUP_FINALLY &&
            code[b->b_handler] == WITH_CLEANUP_START)
        {
            /* Pop the exit function. */
            delta++;
        }
    }
    while (delta > 0) {
        PyObject *v = (*--f->f_stacktop);
        Py_DECREF(v);
        delta--;
    }

    /* Finally set the new f_lineno and f_lasti and return OK. */
    f->f_lineno = new_lineno;
    f->f_lasti = new_lasti;
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
PyFrame_Fini(void)
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

