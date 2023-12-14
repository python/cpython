/* Execute compiled code */

#define _PY_INTERPRETER

#include "Python.h"
#include "pycore_abstract.h"      // _PyIndex_Check()
#include "pycore_call.h"          // _PyObject_FastCallDictTstate()
#include "pycore_ceval.h"         // _PyEval_SignalAsyncExc()
#include "pycore_code.h"
#include "pycore_function.h"
#include "pycore_intrinsics.h"
#include "pycore_long.h"          // _PyLong_GetZero()
#include "pycore_instruments.h"
#include "pycore_object.h"        // _PyObject_GC_TRACK()
#include "pycore_moduleobject.h"  // PyModuleObject
#include "pycore_opcode.h"        // EXTRA_CASES
#include "pycore_pyerrors.h"      // _PyErr_GetRaisedException()
#include "pycore_pymem.h"         // _PyMem_IsPtrFreed()
#include "pycore_pystate.h"       // _PyInterpreterState_GET()
#include "pycore_range.h"         // _PyRangeIterObject
#include "pycore_sliceobject.h"   // _PyBuildSlice_ConsumeRefs
#include "pycore_sysmodule.h"     // _PySys_Audit()
#include "pycore_tuple.h"         // _PyTuple_ITEMS()
#include "pycore_typeobject.h"    // _PySuper_Lookup()
#include "pycore_emscripten_signal.h"  // _Py_CHECK_EMSCRIPTEN_SIGNALS

#include "pycore_dict.h"
#include "dictobject.h"
#include "pycore_frame.h"
#include "frameobject.h"          // _PyInterpreterFrame_GetLine
#include "opcode.h"
#include "pydtrace.h"
#include "setobject.h"
#include "structmember.h"         // struct PyMemberDef, T_OFFSET_EX

#include <ctype.h>
#include <stdbool.h>

#ifdef Py_DEBUG
   /* For debugging the interpreter: */
#  define LLTRACE  1      /* Low-level trace feature */
#endif

#if !defined(Py_BUILD_CORE)
#  error "ceval.c must be build with Py_BUILD_CORE define for best performance"
#endif

#if !defined(Py_DEBUG) && !defined(Py_TRACE_REFS)
// GH-89279: The MSVC compiler does not inline these static inline functions
// in PGO build in _PyEval_EvalFrameDefault(), because this function is over
// the limit of PGO, and that limit cannot be configured.
// Define them as macros to make sure that they are always inlined by the
// preprocessor.

#undef Py_DECREF
#define Py_DECREF(arg) \
    do { \
        PyObject *op = _PyObject_CAST(arg); \
        if (_Py_IsImmortal(op)) { \
            break; \
        } \
        _Py_DECREF_STAT_INC(); \
        if (--op->ob_refcnt == 0) { \
            destructor dealloc = Py_TYPE(op)->tp_dealloc; \
            (*dealloc)(op); \
        } \
    } while (0)

#undef Py_XDECREF
#define Py_XDECREF(arg) \
    do { \
        PyObject *xop = _PyObject_CAST(arg); \
        if (xop != NULL) { \
            Py_DECREF(xop); \
        } \
    } while (0)

#undef Py_IS_TYPE
#define Py_IS_TYPE(ob, type) \
    (_PyObject_CAST(ob)->ob_type == (type))

#undef _Py_DECREF_SPECIALIZED
#define _Py_DECREF_SPECIALIZED(arg, dealloc) \
    do { \
        PyObject *op = _PyObject_CAST(arg); \
        if (_Py_IsImmortal(op)) { \
            break; \
        } \
        _Py_DECREF_STAT_INC(); \
        if (--op->ob_refcnt == 0) { \
            destructor d = (destructor)(dealloc); \
            d(op); \
        } \
    } while (0)
#endif

// GH-89279: Similar to above, force inlining by using a macro.
#if defined(_MSC_VER) && SIZEOF_INT == 4
#define _Py_atomic_load_relaxed_int32(ATOMIC_VAL) (assert(sizeof((ATOMIC_VAL)->_value) == 4), *((volatile int*)&((ATOMIC_VAL)->_value)))
#else
#define _Py_atomic_load_relaxed_int32(ATOMIC_VAL) _Py_atomic_load_relaxed(ATOMIC_VAL)
#endif


#ifdef LLTRACE
static void
dump_stack(_PyInterpreterFrame *frame, PyObject **stack_pointer)
{
    PyObject **stack_base = _PyFrame_Stackbase(frame);
    PyObject *exc = PyErr_GetRaisedException();
    printf("    stack=[");
    for (PyObject **ptr = stack_base; ptr < stack_pointer; ptr++) {
        if (ptr != stack_base) {
            printf(", ");
        }
        if (PyObject_Print(*ptr, stdout, 0) != 0) {
            PyErr_Clear();
            printf("<%s object at %p>",
                   Py_TYPE(*ptr)->tp_name, (void *)(*ptr));
        }
    }
    printf("]\n");
    fflush(stdout);
    PyErr_SetRaisedException(exc);
}

static void
lltrace_instruction(_PyInterpreterFrame *frame,
                    PyObject **stack_pointer,
                    _Py_CODEUNIT *next_instr)
{
    /* This dump_stack() operation is risky, since the repr() of some
       objects enters the interpreter recursively. It is also slow.
       So you might want to comment it out. */
    dump_stack(frame, stack_pointer);
    int oparg = next_instr->op.arg;
    int opcode = next_instr->op.code;
    const char *opname = _PyOpcode_OpName[opcode];
    assert(opname != NULL);
    int offset = (int)(next_instr - _PyCode_CODE(frame->f_code));
    if (HAS_ARG((int)_PyOpcode_Deopt[opcode])) {
        printf("%d: %s %d\n", offset * 2, opname, oparg);
    }
    else {
        printf("%d: %s\n", offset * 2, opname);
    }
    fflush(stdout);
}
static void
lltrace_resume_frame(_PyInterpreterFrame *frame)
{
    PyObject *fobj = frame->f_funcobj;
    if (frame->owner == FRAME_OWNED_BY_CSTACK ||
        fobj == NULL ||
        !PyFunction_Check(fobj)
    ) {
        printf("\nResuming frame.\n");
        return;
    }
    PyFunctionObject *f = (PyFunctionObject *)fobj;
    PyObject *exc = PyErr_GetRaisedException();
    PyObject *name = f->func_qualname;
    if (name == NULL) {
        name = f->func_name;
    }
    printf("\nResuming frame");
    if (name) {
        printf(" for ");
        if (PyObject_Print(name, stdout, 0) < 0) {
            PyErr_Clear();
        }
    }
    if (f->func_module) {
        printf(" in module ");
        if (PyObject_Print(f->func_module, stdout, 0) < 0) {
            PyErr_Clear();
        }
    }
    printf("\n");
    fflush(stdout);
    PyErr_SetRaisedException(exc);
}
#endif

static void monitor_raise(PyThreadState *tstate,
                 _PyInterpreterFrame *frame,
                 _Py_CODEUNIT *instr);
static void monitor_reraise(PyThreadState *tstate,
                 _PyInterpreterFrame *frame,
                 _Py_CODEUNIT *instr);
static int monitor_stop_iteration(PyThreadState *tstate,
                 _PyInterpreterFrame *frame,
                 _Py_CODEUNIT *instr);
static void monitor_unwind(PyThreadState *tstate,
                 _PyInterpreterFrame *frame,
                 _Py_CODEUNIT *instr);
static int monitor_handled(PyThreadState *tstate,
                 _PyInterpreterFrame *frame,
                 _Py_CODEUNIT *instr, PyObject *exc);
static void monitor_throw(PyThreadState *tstate,
                 _PyInterpreterFrame *frame,
                 _Py_CODEUNIT *instr);

static PyObject * import_name(PyThreadState *, _PyInterpreterFrame *,
                              PyObject *, PyObject *, PyObject *);
static PyObject * import_from(PyThreadState *, PyObject *, PyObject *);
static void format_exc_check_arg(PyThreadState *, PyObject *, const char *, PyObject *);
static void format_exc_unbound(PyThreadState *tstate, PyCodeObject *co, int oparg);
static int check_args_iterable(PyThreadState *, PyObject *func, PyObject *vararg);
static int check_except_type_valid(PyThreadState *tstate, PyObject* right);
static int check_except_star_type_valid(PyThreadState *tstate, PyObject* right);
static void format_kwargs_error(PyThreadState *, PyObject *func, PyObject *kwargs);
static void format_awaitable_error(PyThreadState *, PyTypeObject *, int);
static int get_exception_handler(PyCodeObject *, int, int*, int*, int*);
static _PyInterpreterFrame *
_PyEvalFramePushAndInit(PyThreadState *tstate, PyFunctionObject *func,
                        PyObject *locals, PyObject* const* args,
                        size_t argcount, PyObject *kwnames);
static  _PyInterpreterFrame *
_PyEvalFramePushAndInit_Ex(PyThreadState *tstate, PyFunctionObject *func,
    PyObject *locals, Py_ssize_t nargs, PyObject *callargs, PyObject *kwargs);
static void
_PyEvalFrameClearAndPop(PyThreadState *tstate, _PyInterpreterFrame *frame);

#define UNBOUNDLOCAL_ERROR_MSG \
    "cannot access local variable '%s' where it is not associated with a value"
#define UNBOUNDFREE_ERROR_MSG \
    "cannot access free variable '%s' where it is not associated with a" \
    " value in enclosing scope"

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

int
Py_GetRecursionLimit(void)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    return interp->ceval.recursion_limit;
}

void
Py_SetRecursionLimit(int new_limit)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    interp->ceval.recursion_limit = new_limit;
    for (PyThreadState *p = interp->threads.head; p != NULL; p = p->next) {
        int depth = p->py_recursion_limit - p->py_recursion_remaining;
        p->py_recursion_limit = new_limit;
        p->py_recursion_remaining = new_limit - depth;
    }
}

/* The function _Py_EnterRecursiveCallTstate() only calls _Py_CheckRecursiveCall()
   if the recursion_depth reaches recursion_limit. */
int
_Py_CheckRecursiveCall(PyThreadState *tstate, const char *where)
{
#ifdef USE_STACKCHECK
    if (PyOS_CheckStack()) {
        ++tstate->c_recursion_remaining;
        _PyErr_SetString(tstate, PyExc_MemoryError, "Stack overflow");
        return -1;
    }
#endif
    if (tstate->recursion_headroom) {
        if (tstate->c_recursion_remaining < -50) {
            /* Overflowing while handling an overflow. Give up. */
            Py_FatalError("Cannot recover from stack overflow.");
        }
    }
    else {
        if (tstate->c_recursion_remaining <= 0) {
            tstate->recursion_headroom++;
            _PyErr_Format(tstate, PyExc_RecursionError,
                        "maximum recursion depth exceeded%s",
                        where);
            tstate->recursion_headroom--;
            ++tstate->c_recursion_remaining;
            return -1;
        }
    }
    return 0;
}


static const binaryfunc binary_ops[] = {
    [NB_ADD] = PyNumber_Add,
    [NB_AND] = PyNumber_And,
    [NB_FLOOR_DIVIDE] = PyNumber_FloorDivide,
    [NB_LSHIFT] = PyNumber_Lshift,
    [NB_MATRIX_MULTIPLY] = PyNumber_MatrixMultiply,
    [NB_MULTIPLY] = PyNumber_Multiply,
    [NB_REMAINDER] = PyNumber_Remainder,
    [NB_OR] = PyNumber_Or,
    [NB_POWER] = _PyNumber_PowerNoMod,
    [NB_RSHIFT] = PyNumber_Rshift,
    [NB_SUBTRACT] = PyNumber_Subtract,
    [NB_TRUE_DIVIDE] = PyNumber_TrueDivide,
    [NB_XOR] = PyNumber_Xor,
    [NB_INPLACE_ADD] = PyNumber_InPlaceAdd,
    [NB_INPLACE_AND] = PyNumber_InPlaceAnd,
    [NB_INPLACE_FLOOR_DIVIDE] = PyNumber_InPlaceFloorDivide,
    [NB_INPLACE_LSHIFT] = PyNumber_InPlaceLshift,
    [NB_INPLACE_MATRIX_MULTIPLY] = PyNumber_InPlaceMatrixMultiply,
    [NB_INPLACE_MULTIPLY] = PyNumber_InPlaceMultiply,
    [NB_INPLACE_REMAINDER] = PyNumber_InPlaceRemainder,
    [NB_INPLACE_OR] = PyNumber_InPlaceOr,
    [NB_INPLACE_POWER] = _PyNumber_InPlacePowerNoMod,
    [NB_INPLACE_RSHIFT] = PyNumber_InPlaceRshift,
    [NB_INPLACE_SUBTRACT] = PyNumber_InPlaceSubtract,
    [NB_INPLACE_TRUE_DIVIDE] = PyNumber_InPlaceTrueDivide,
    [NB_INPLACE_XOR] = PyNumber_InPlaceXor,
};


// PEP 634: Structural Pattern Matching


// Return a tuple of values corresponding to keys, with error checks for
// duplicate/missing keys.
static PyObject*
match_keys(PyThreadState *tstate, PyObject *map, PyObject *keys)
{
    assert(PyTuple_CheckExact(keys));
    Py_ssize_t nkeys = PyTuple_GET_SIZE(keys);
    if (!nkeys) {
        // No keys means no items.
        return PyTuple_New(0);
    }
    PyObject *seen = NULL;
    PyObject *dummy = NULL;
    PyObject *values = NULL;
    PyObject *get = NULL;
    // We use the two argument form of map.get(key, default) for two reasons:
    // - Atomically check for a key and get its value without error handling.
    // - Don't cause key creation or resizing in dict subclasses like
    //   collections.defaultdict that define __missing__ (or similar).
    int meth_found = _PyObject_GetMethod(map, &_Py_ID(get), &get);
    if (get == NULL) {
        goto fail;
    }
    seen = PySet_New(NULL);
    if (seen == NULL) {
        goto fail;
    }
    // dummy = object()
    dummy = _PyObject_CallNoArgs((PyObject *)&PyBaseObject_Type);
    if (dummy == NULL) {
        goto fail;
    }
    values = PyTuple_New(nkeys);
    if (values == NULL) {
        goto fail;
    }
    for (Py_ssize_t i = 0; i < nkeys; i++) {
        PyObject *key = PyTuple_GET_ITEM(keys, i);
        if (PySet_Contains(seen, key) || PySet_Add(seen, key)) {
            if (!_PyErr_Occurred(tstate)) {
                // Seen it before!
                _PyErr_Format(tstate, PyExc_ValueError,
                              "mapping pattern checks duplicate key (%R)", key);
            }
            goto fail;
        }
        PyObject *args[] = { map, key, dummy };
        PyObject *value = NULL;
        if (meth_found) {
            value = PyObject_Vectorcall(get, args, 3, NULL);
        }
        else {
            value = PyObject_Vectorcall(get, &args[1], 2, NULL);
        }
        if (value == NULL) {
            goto fail;
        }
        if (value == dummy) {
            // key not in map!
            Py_DECREF(value);
            Py_DECREF(values);
            // Return None:
            values = Py_NewRef(Py_None);
            goto done;
        }
        PyTuple_SET_ITEM(values, i, value);
    }
    // Success:
done:
    Py_DECREF(get);
    Py_DECREF(seen);
    Py_DECREF(dummy);
    return values;
fail:
    Py_XDECREF(get);
    Py_XDECREF(seen);
    Py_XDECREF(dummy);
    Py_XDECREF(values);
    return NULL;
}

// Extract a named attribute from the subject, with additional bookkeeping to
// raise TypeErrors for repeated lookups. On failure, return NULL (with no
// error set). Use _PyErr_Occurred(tstate) to disambiguate.
static PyObject*
match_class_attr(PyThreadState *tstate, PyObject *subject, PyObject *type,
                 PyObject *name, PyObject *seen)
{
    assert(PyUnicode_CheckExact(name));
    assert(PySet_CheckExact(seen));
    if (PySet_Contains(seen, name) || PySet_Add(seen, name)) {
        if (!_PyErr_Occurred(tstate)) {
            // Seen it before!
            _PyErr_Format(tstate, PyExc_TypeError,
                          "%s() got multiple sub-patterns for attribute %R",
                          ((PyTypeObject*)type)->tp_name, name);
        }
        return NULL;
    }
    PyObject *attr = PyObject_GetAttr(subject, name);
    if (attr == NULL && _PyErr_ExceptionMatches(tstate, PyExc_AttributeError)) {
        _PyErr_Clear(tstate);
    }
    return attr;
}

// On success (match), return a tuple of extracted attributes. On failure (no
// match), return NULL. Use _PyErr_Occurred(tstate) to disambiguate.
static PyObject*
match_class(PyThreadState *tstate, PyObject *subject, PyObject *type,
            Py_ssize_t nargs, PyObject *kwargs)
{
    if (!PyType_Check(type)) {
        const char *e = "called match pattern must be a class";
        _PyErr_Format(tstate, PyExc_TypeError, e);
        return NULL;
    }
    assert(PyTuple_CheckExact(kwargs));
    // First, an isinstance check:
    if (PyObject_IsInstance(subject, type) <= 0) {
        return NULL;
    }
    // So far so good:
    PyObject *seen = PySet_New(NULL);
    if (seen == NULL) {
        return NULL;
    }
    PyObject *attrs = PyList_New(0);
    if (attrs == NULL) {
        Py_DECREF(seen);
        return NULL;
    }
    // NOTE: From this point on, goto fail on failure:
    PyObject *match_args = NULL;
    // First, the positional subpatterns:
    if (nargs) {
        int match_self = 0;
        match_args = PyObject_GetAttrString(type, "__match_args__");
        if (match_args) {
            if (!PyTuple_CheckExact(match_args)) {
                const char *e = "%s.__match_args__ must be a tuple (got %s)";
                _PyErr_Format(tstate, PyExc_TypeError, e,
                              ((PyTypeObject *)type)->tp_name,
                              Py_TYPE(match_args)->tp_name);
                goto fail;
            }
        }
        else if (_PyErr_ExceptionMatches(tstate, PyExc_AttributeError)) {
            _PyErr_Clear(tstate);
            // _Py_TPFLAGS_MATCH_SELF is only acknowledged if the type does not
            // define __match_args__. This is natural behavior for subclasses:
            // it's as if __match_args__ is some "magic" value that is lost as
            // soon as they redefine it.
            match_args = PyTuple_New(0);
            match_self = PyType_HasFeature((PyTypeObject*)type,
                                            _Py_TPFLAGS_MATCH_SELF);
        }
        else {
            goto fail;
        }
        assert(PyTuple_CheckExact(match_args));
        Py_ssize_t allowed = match_self ? 1 : PyTuple_GET_SIZE(match_args);
        if (allowed < nargs) {
            const char *plural = (allowed == 1) ? "" : "s";
            _PyErr_Format(tstate, PyExc_TypeError,
                          "%s() accepts %d positional sub-pattern%s (%d given)",
                          ((PyTypeObject*)type)->tp_name,
                          allowed, plural, nargs);
            goto fail;
        }
        if (match_self) {
            // Easy. Copy the subject itself, and move on to kwargs.
            if (PyList_Append(attrs, subject) < 0) {
                goto fail;
            }
        }
        else {
            for (Py_ssize_t i = 0; i < nargs; i++) {
                PyObject *name = PyTuple_GET_ITEM(match_args, i);
                if (!PyUnicode_CheckExact(name)) {
                    _PyErr_Format(tstate, PyExc_TypeError,
                                  "__match_args__ elements must be strings "
                                  "(got %s)", Py_TYPE(name)->tp_name);
                    goto fail;
                }
                PyObject *attr = match_class_attr(tstate, subject, type, name,
                                                  seen);
                if (attr == NULL) {
                    goto fail;
                }
                if (PyList_Append(attrs, attr) < 0) {
                    Py_DECREF(attr);
                    goto fail;
                }
                Py_DECREF(attr);
            }
        }
        Py_CLEAR(match_args);
    }
    // Finally, the keyword subpatterns:
    for (Py_ssize_t i = 0; i < PyTuple_GET_SIZE(kwargs); i++) {
        PyObject *name = PyTuple_GET_ITEM(kwargs, i);
        PyObject *attr = match_class_attr(tstate, subject, type, name, seen);
        if (attr == NULL) {
            goto fail;
        }
        if (PyList_Append(attrs, attr) < 0) {
            Py_DECREF(attr);
            goto fail;
        }
        Py_DECREF(attr);
    }
    Py_SETREF(attrs, PyList_AsTuple(attrs));
    Py_DECREF(seen);
    return attrs;
fail:
    // We really don't care whether an error was raised or not... that's our
    // caller's problem. All we know is that the match failed.
    Py_XDECREF(match_args);
    Py_DECREF(seen);
    Py_DECREF(attrs);
    return NULL;
}


static int do_raise(PyThreadState *tstate, PyObject *exc, PyObject *cause);
static int exception_group_match(
    PyObject* exc_value, PyObject *match_type,
    PyObject **match, PyObject **rest);

static int unpack_iterable(PyThreadState *, PyObject *, int, int, PyObject **);

PyObject *
PyEval_EvalCode(PyObject *co, PyObject *globals, PyObject *locals)
{
    PyThreadState *tstate = _PyThreadState_GET();
    if (locals == NULL) {
        locals = globals;
    }
    PyObject *builtins = _PyEval_BuiltinsFromGlobals(tstate, globals); // borrowed ref
    if (builtins == NULL) {
        return NULL;
    }
    PyFrameConstructor desc = {
        .fc_globals = globals,
        .fc_builtins = builtins,
        .fc_name = ((PyCodeObject *)co)->co_name,
        .fc_qualname = ((PyCodeObject *)co)->co_name,
        .fc_code = co,
        .fc_defaults = NULL,
        .fc_kwdefaults = NULL,
        .fc_closure = NULL
    };
    PyFunctionObject *func = _PyFunction_FromConstructor(&desc);
    if (func == NULL) {
        return NULL;
    }
    EVAL_CALL_STAT_INC(EVAL_CALL_LEGACY);
    PyObject *res = _PyEval_Vector(tstate, func, locals, NULL, 0, NULL);
    Py_DECREF(func);
    return res;
}


/* Interpreter main loop */

PyObject *
PyEval_EvalFrame(PyFrameObject *f)
{
    /* Function kept for backward compatibility */
    PyThreadState *tstate = _PyThreadState_GET();
    return _PyEval_EvalFrame(tstate, f->f_frame, 0);
}

PyObject *
PyEval_EvalFrameEx(PyFrameObject *f, int throwflag)
{
    PyThreadState *tstate = _PyThreadState_GET();
    return _PyEval_EvalFrame(tstate, f->f_frame, throwflag);
}

#include "ceval_macros.h"


int _Py_CheckRecursiveCallPy(
    PyThreadState *tstate)
{
    if (tstate->recursion_headroom) {
        if (tstate->py_recursion_remaining < -50) {
            /* Overflowing while handling an overflow. Give up. */
            Py_FatalError("Cannot recover from Python stack overflow.");
        }
    }
    else {
        if (tstate->py_recursion_remaining <= 0) {
            tstate->recursion_headroom++;
            _PyErr_Format(tstate, PyExc_RecursionError,
                        "maximum recursion depth exceeded");
            tstate->recursion_headroom--;
            return -1;
        }
    }
    return 0;
}

static inline int _Py_EnterRecursivePy(PyThreadState *tstate) {
    return (tstate->py_recursion_remaining-- <= 0) &&
        _Py_CheckRecursiveCallPy(tstate);
}


static inline void _Py_LeaveRecursiveCallPy(PyThreadState *tstate)  {
    tstate->py_recursion_remaining++;
}


/* Disable unused label warnings.  They are handy for debugging, even
   if computed gotos aren't used. */

/* TBD - what about other compilers? */
#if defined(__GNUC__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wunused-label"
#elif defined(_MSC_VER) /* MS_WINDOWS */
#  pragma warning(push)
#  pragma warning(disable:4102)
#endif


/* _PyEval_EvalFrameDefault() is a *big* function,
 * so consume 3 units of C stack */
#define PY_EVAL_C_STACK_UNITS 2

PyObject* _Py_HOT_FUNCTION
_PyEval_EvalFrameDefault(PyThreadState *tstate, _PyInterpreterFrame *frame, int throwflag)
{
    _Py_EnsureTstateNotNULL(tstate);
    CALL_STAT_INC(pyeval_calls);

#if USE_COMPUTED_GOTOS
/* Import the static jump table */
#include "opcode_targets.h"
#endif

#ifdef Py_STATS
    int lastopcode = 0;
#endif
    // opcode is an 8-bit value to improve the code generated by MSVC
    // for the big switch below (in combination with the EXTRA_CASES macro).
    uint8_t opcode;        /* Current opcode */
    int oparg;         /* Current opcode argument, if any */
#ifdef LLTRACE
    int lltrace = 0;
#endif

    _PyCFrame cframe;
    _PyInterpreterFrame  entry_frame;
    PyObject *kwnames = NULL; // Borrowed reference. Reset by CALL instructions.

    /* WARNING: Because the _PyCFrame lives on the C stack,
     * but can be accessed from a heap allocated object (tstate)
     * strict stack discipline must be maintained.
     */
    _PyCFrame *prev_cframe = tstate->cframe;
    cframe.previous = prev_cframe;
    tstate->cframe = &cframe;

    assert(tstate->interp->interpreter_trampoline != NULL);
#ifdef Py_DEBUG
    /* Set these to invalid but identifiable values for debugging. */
    entry_frame.f_funcobj = (PyObject*)0xaaa0;
    entry_frame.f_locals = (PyObject*)0xaaa1;
    entry_frame.frame_obj = (PyFrameObject*)0xaaa2;
    entry_frame.f_globals = (PyObject*)0xaaa3;
    entry_frame.f_builtins = (PyObject*)0xaaa4;
#endif
    entry_frame.f_code = tstate->interp->interpreter_trampoline;
    entry_frame.prev_instr =
        _PyCode_CODE(tstate->interp->interpreter_trampoline);
    entry_frame.stacktop = 0;
    entry_frame.owner = FRAME_OWNED_BY_CSTACK;
    entry_frame.return_offset = 0;
    /* Push frame */
    entry_frame.previous = prev_cframe->current_frame;
    frame->previous = &entry_frame;
    cframe.current_frame = frame;

    tstate->c_recursion_remaining -= (PY_EVAL_C_STACK_UNITS - 1);
    if (_Py_EnterRecursiveCallTstate(tstate, "")) {
        tstate->c_recursion_remaining--;
        tstate->py_recursion_remaining--;
        goto exit_unwind;
    }

    /* support for generator.throw() */
    if (throwflag) {
        if (_Py_EnterRecursivePy(tstate)) {
            goto exit_unwind;
        }
        /* Because this avoids the RESUME,
         * we need to update instrumentation */
        _Py_Instrument(frame->f_code, tstate->interp);
        monitor_throw(tstate, frame, frame->prev_instr);
        /* TO DO -- Monitor throw entry. */
        goto resume_with_error;
    }

    /* Local "register" variables.
     * These are cached values from the frame and code object.  */

    _Py_CODEUNIT *next_instr;
    PyObject **stack_pointer;

/* Sets the above local variables from the frame */
#define SET_LOCALS_FROM_FRAME() \
    assert(_PyInterpreterFrame_LASTI(frame) >= -1); \
    /* Jump back to the last instruction executed... */ \
    next_instr = frame->prev_instr + 1; \
    stack_pointer = _PyFrame_GetStackPointer(frame);

start_frame:
    if (_Py_EnterRecursivePy(tstate)) {
        goto exit_unwind;
    }

resume_frame:
    SET_LOCALS_FROM_FRAME();

#ifdef LLTRACE
    {
        if (frame != &entry_frame) {
            int r = PyDict_Contains(GLOBALS(), &_Py_ID(__lltrace__));
            if (r < 0) {
                goto exit_unwind;
            }
            lltrace = r;
        }
        if (lltrace) {
            lltrace_resume_frame(frame);
        }
    }
#endif

#ifdef Py_DEBUG
    /* _PyEval_EvalFrameDefault() must not be called with an exception set,
       because it can clear it (directly or indirectly) and so the
       caller loses its exception */
    assert(!_PyErr_Occurred(tstate));
#endif

    DISPATCH();

handle_eval_breaker:

    /* Do periodic things, like check for signals and async I/0.
     * We need to do reasonably frequently, but not too frequently.
     * All loops should include a check of the eval breaker.
     * We also check on return from any builtin function.
     *
     * ## More Details ###
     *
     * The eval loop (this function) normally executes the instructions
     * of a code object sequentially.  However, the runtime supports a
     * number of out-of-band execution scenarios that may pause that
     * sequential execution long enough to do that out-of-band work
     * in the current thread using the current PyThreadState.
     *
     * The scenarios include:
     *
     *  - cyclic garbage collection
     *  - GIL drop requests
     *  - "async" exceptions
     *  - "pending calls"  (some only in the main thread)
     *  - signal handling (only in the main thread)
     *
     * When the need for one of the above is detected, the eval loop
     * pauses long enough to handle the detected case.  Then, if doing
     * so didn't trigger an exception, the eval loop resumes executing
     * the sequential instructions.
     *
     * To make this work, the eval loop periodically checks if any
     * of the above needs to happen.  The individual checks can be
     * expensive if computed each time, so a while back we switched
     * to using pre-computed, per-interpreter variables for the checks,
     * and later consolidated that to a single "eval breaker" variable
     * (now a PyInterpreterState field).
     *
     * For the longest time, the eval breaker check would happen
     * frequently, every 5 or so times through the loop, regardless
     * of what instruction ran last or what would run next.  Then, in
     * early 2021 (gh-18334, commit 4958f5d), we switched to checking
     * the eval breaker less frequently, by hard-coding the check to
     * specific places in the eval loop (e.g. certain instructions).
     * The intent then was to check after returning from calls
     * and on the back edges of loops.
     *
     * In addition to being more efficient, that approach keeps
     * the eval loop from running arbitrary code between instructions
     * that don't handle that well.  (See gh-74174.)
     *
     * Currently, the eval breaker check happens here at the
     * "handle_eval_breaker" label.  Some instructions come here
     * explicitly (goto) and some indirectly.  Notably, the check
     * happens on back edges in the control flow graph, which
     * pretty much applies to all loops and most calls.
     * (See bytecodes.c for exact information.)
     *
     * One consequence of this approach is that it might not be obvious
     * how to force any specific thread to pick up the eval breaker,
     * or for any specific thread to not pick it up.  Mostly this
     * involves judicious uses of locks and careful ordering of code,
     * while avoiding code that might trigger the eval breaker
     * until so desired.
     */
    if (_Py_HandlePending(tstate) != 0) {
        goto error;
    }
    DISPATCH();

    {
    /* Start instructions */
#if !USE_COMPUTED_GOTOS
    dispatch_opcode:
        switch (opcode)
#endif
        {

#include "generated_cases.c.h"

    /* INSTRUMENTED_LINE has to be here, rather than in bytecodes.c,
     * because it needs to capture frame->prev_instr before it is updated,
     * as happens in the standard instruction prologue.
     */
#if USE_COMPUTED_GOTOS
        TARGET_INSTRUMENTED_LINE:
#else
        case INSTRUMENTED_LINE:
#endif
    {
        _Py_CODEUNIT *prev = frame->prev_instr;
        _Py_CODEUNIT *here = frame->prev_instr = next_instr;
        _PyFrame_SetStackPointer(frame, stack_pointer);
        int original_opcode = _Py_call_instrumentation_line(
                tstate, frame, here, prev);
        stack_pointer = _PyFrame_GetStackPointer(frame);
        if (original_opcode < 0) {
            next_instr = here+1;
            goto error;
        }
        next_instr = frame->prev_instr;
        if (next_instr != here) {
            DISPATCH();
        }
        if (_PyOpcode_Caches[original_opcode]) {
            _PyBinaryOpCache *cache = (_PyBinaryOpCache *)(next_instr+1);
            /* Prevent the underlying instruction from specializing
             * and overwriting the instrumentation. */
            INCREMENT_ADAPTIVE_COUNTER(cache->counter);
        }
        opcode = original_opcode;
        DISPATCH_GOTO();
    }


#if USE_COMPUTED_GOTOS
        _unknown_opcode:
#else
        EXTRA_CASES  // From opcode.h, a 'case' for each unused opcode
#endif
            /* Tell C compilers not to hold the opcode variable in the loop.
               next_instr points the current instruction without TARGET(). */
            opcode = next_instr->op.code;
            _PyErr_Format(tstate, PyExc_SystemError,
                          "%U:%d: unknown opcode %d",
                          frame->f_code->co_filename,
                          PyUnstable_InterpreterFrame_GetLine(frame),
                          opcode);
            goto error;

        } /* End instructions */

        /* This should never be reached. Every opcode should end with DISPATCH()
           or goto error. */
        Py_UNREACHABLE();

unbound_local_error:
        {
            format_exc_check_arg(tstate, PyExc_UnboundLocalError,
                UNBOUNDLOCAL_ERROR_MSG,
                PyTuple_GetItem(frame->f_code->co_localsplusnames, oparg)
            );
            goto error;
        }

pop_4_error:
    STACK_SHRINK(1);
pop_3_error:
    STACK_SHRINK(1);
pop_2_error:
    STACK_SHRINK(1);
pop_1_error:
    STACK_SHRINK(1);
error:
        kwnames = NULL;
        /* Double-check exception status. */
#ifdef NDEBUG
        if (!_PyErr_Occurred(tstate)) {
            _PyErr_SetString(tstate, PyExc_SystemError,
                             "error return without exception set");
        }
#else
        assert(_PyErr_Occurred(tstate));
#endif

        /* Log traceback info. */
        assert(frame != &entry_frame);
        if (!_PyFrame_IsIncomplete(frame)) {
            PyFrameObject *f = _PyFrame_GetFrameObject(frame);
            if (f != NULL) {
                PyTraceBack_Here(f);
            }
        }
        monitor_raise(tstate, frame, next_instr-1);
exception_unwind:
        {
            /* We can't use frame->f_lasti here, as RERAISE may have set it */
            int offset = INSTR_OFFSET()-1;
            int level, handler, lasti;
            if (get_exception_handler(frame->f_code, offset, &level, &handler, &lasti) == 0) {
                // No handlers, so exit.
                assert(_PyErr_Occurred(tstate));

                /* Pop remaining stack entries. */
                PyObject **stackbase = _PyFrame_Stackbase(frame);
                while (stack_pointer > stackbase) {
                    PyObject *o = POP();
                    Py_XDECREF(o);
                }
                assert(STACK_LEVEL() == 0);
                _PyFrame_SetStackPointer(frame, stack_pointer);
                monitor_unwind(tstate, frame, next_instr-1);
                goto exit_unwind;
            }

            assert(STACK_LEVEL() >= level);
            PyObject **new_top = _PyFrame_Stackbase(frame) + level;
            while (stack_pointer > new_top) {
                PyObject *v = POP();
                Py_XDECREF(v);
            }
            if (lasti) {
                int frame_lasti = _PyInterpreterFrame_LASTI(frame);
                PyObject *lasti = PyLong_FromLong(frame_lasti);
                if (lasti == NULL) {
                    goto exception_unwind;
                }
                PUSH(lasti);
            }

            /* Make the raw exception data
                available to the handler,
                so a program can emulate the
                Python main loop. */
            PyObject *exc = _PyErr_GetRaisedException(tstate);
            PUSH(exc);
            JUMPTO(handler);
            if (monitor_handled(tstate, frame, next_instr, exc) < 0) {
                goto exception_unwind;
            }
            /* Resume normal execution */
            DISPATCH();
        }
    }

exit_unwind:
    assert(_PyErr_Occurred(tstate));
    _Py_LeaveRecursiveCallPy(tstate);
    assert(frame != &entry_frame);
    // GH-99729: We need to unlink the frame *before* clearing it:
    _PyInterpreterFrame *dying = frame;
    frame = cframe.current_frame = dying->previous;
    _PyEvalFrameClearAndPop(tstate, dying);
    frame->return_offset = 0;
    if (frame == &entry_frame) {
        /* Restore previous cframe and exit */
        tstate->cframe = cframe.previous;
        assert(tstate->cframe->current_frame == frame->previous);
        tstate->c_recursion_remaining += PY_EVAL_C_STACK_UNITS;
        return NULL;
    }

resume_with_error:
    SET_LOCALS_FROM_FRAME();
    goto error;

}
#if defined(__GNUC__)
#  pragma GCC diagnostic pop
#elif defined(_MSC_VER) /* MS_WINDOWS */
#  pragma warning(pop)
#endif

static void
format_missing(PyThreadState *tstate, const char *kind,
               PyCodeObject *co, PyObject *names, PyObject *qualname)
{
    int err;
    Py_ssize_t len = PyList_GET_SIZE(names);
    PyObject *name_str, *comma, *tail, *tmp;

    assert(PyList_CheckExact(names));
    assert(len >= 1);
    /* Deal with the joys of natural language. */
    switch (len) {
    case 1:
        name_str = PyList_GET_ITEM(names, 0);
        Py_INCREF(name_str);
        break;
    case 2:
        name_str = PyUnicode_FromFormat("%U and %U",
                                        PyList_GET_ITEM(names, len - 2),
                                        PyList_GET_ITEM(names, len - 1));
        break;
    default:
        tail = PyUnicode_FromFormat(", %U, and %U",
                                    PyList_GET_ITEM(names, len - 2),
                                    PyList_GET_ITEM(names, len - 1));
        if (tail == NULL)
            return;
        /* Chop off the last two objects in the list. This shouldn't actually
           fail, but we can't be too careful. */
        err = PyList_SetSlice(names, len - 2, len, NULL);
        if (err == -1) {
            Py_DECREF(tail);
            return;
        }
        /* Stitch everything up into a nice comma-separated list. */
        comma = PyUnicode_FromString(", ");
        if (comma == NULL) {
            Py_DECREF(tail);
            return;
        }
        tmp = PyUnicode_Join(comma, names);
        Py_DECREF(comma);
        if (tmp == NULL) {
            Py_DECREF(tail);
            return;
        }
        name_str = PyUnicode_Concat(tmp, tail);
        Py_DECREF(tmp);
        Py_DECREF(tail);
        break;
    }
    if (name_str == NULL)
        return;
    _PyErr_Format(tstate, PyExc_TypeError,
                  "%U() missing %i required %s argument%s: %U",
                  qualname,
                  len,
                  kind,
                  len == 1 ? "" : "s",
                  name_str);
    Py_DECREF(name_str);
}

static void
missing_arguments(PyThreadState *tstate, PyCodeObject *co,
                  Py_ssize_t missing, Py_ssize_t defcount,
                  PyObject **localsplus, PyObject *qualname)
{
    Py_ssize_t i, j = 0;
    Py_ssize_t start, end;
    int positional = (defcount != -1);
    const char *kind = positional ? "positional" : "keyword-only";
    PyObject *missing_names;

    /* Compute the names of the arguments that are missing. */
    missing_names = PyList_New(missing);
    if (missing_names == NULL)
        return;
    if (positional) {
        start = 0;
        end = co->co_argcount - defcount;
    }
    else {
        start = co->co_argcount;
        end = start + co->co_kwonlyargcount;
    }
    for (i = start; i < end; i++) {
        if (localsplus[i] == NULL) {
            PyObject *raw = PyTuple_GET_ITEM(co->co_localsplusnames, i);
            PyObject *name = PyObject_Repr(raw);
            if (name == NULL) {
                Py_DECREF(missing_names);
                return;
            }
            PyList_SET_ITEM(missing_names, j++, name);
        }
    }
    assert(j == missing);
    format_missing(tstate, kind, co, missing_names, qualname);
    Py_DECREF(missing_names);
}

static void
too_many_positional(PyThreadState *tstate, PyCodeObject *co,
                    Py_ssize_t given, PyObject *defaults,
                    PyObject **localsplus, PyObject *qualname)
{
    int plural;
    Py_ssize_t kwonly_given = 0;
    Py_ssize_t i;
    PyObject *sig, *kwonly_sig;
    Py_ssize_t co_argcount = co->co_argcount;

    assert((co->co_flags & CO_VARARGS) == 0);
    /* Count missing keyword-only args. */
    for (i = co_argcount; i < co_argcount + co->co_kwonlyargcount; i++) {
        if (localsplus[i] != NULL) {
            kwonly_given++;
        }
    }
    Py_ssize_t defcount = defaults == NULL ? 0 : PyTuple_GET_SIZE(defaults);
    if (defcount) {
        Py_ssize_t atleast = co_argcount - defcount;
        plural = 1;
        sig = PyUnicode_FromFormat("from %zd to %zd", atleast, co_argcount);
    }
    else {
        plural = (co_argcount != 1);
        sig = PyUnicode_FromFormat("%zd", co_argcount);
    }
    if (sig == NULL)
        return;
    if (kwonly_given) {
        const char *format = " positional argument%s (and %zd keyword-only argument%s)";
        kwonly_sig = PyUnicode_FromFormat(format,
                                          given != 1 ? "s" : "",
                                          kwonly_given,
                                          kwonly_given != 1 ? "s" : "");
        if (kwonly_sig == NULL) {
            Py_DECREF(sig);
            return;
        }
    }
    else {
        /* This will not fail. */
        kwonly_sig = PyUnicode_FromString("");
        assert(kwonly_sig != NULL);
    }
    _PyErr_Format(tstate, PyExc_TypeError,
                  "%U() takes %U positional argument%s but %zd%U %s given",
                  qualname,
                  sig,
                  plural ? "s" : "",
                  given,
                  kwonly_sig,
                  given == 1 && !kwonly_given ? "was" : "were");
    Py_DECREF(sig);
    Py_DECREF(kwonly_sig);
}

static int
positional_only_passed_as_keyword(PyThreadState *tstate, PyCodeObject *co,
                                  Py_ssize_t kwcount, PyObject* kwnames,
                                  PyObject *qualname)
{
    int posonly_conflicts = 0;
    PyObject* posonly_names = PyList_New(0);
    if (posonly_names == NULL) {
        goto fail;
    }
    for(int k=0; k < co->co_posonlyargcount; k++){
        PyObject* posonly_name = PyTuple_GET_ITEM(co->co_localsplusnames, k);

        for (int k2=0; k2<kwcount; k2++){
            /* Compare the pointers first and fallback to PyObject_RichCompareBool*/
            PyObject* kwname = PyTuple_GET_ITEM(kwnames, k2);
            if (kwname == posonly_name){
                if(PyList_Append(posonly_names, kwname) != 0) {
                    goto fail;
                }
                posonly_conflicts++;
                continue;
            }

            int cmp = PyObject_RichCompareBool(posonly_name, kwname, Py_EQ);

            if ( cmp > 0) {
                if(PyList_Append(posonly_names, kwname) != 0) {
                    goto fail;
                }
                posonly_conflicts++;
            } else if (cmp < 0) {
                goto fail;
            }

        }
    }
    if (posonly_conflicts) {
        PyObject* comma = PyUnicode_FromString(", ");
        if (comma == NULL) {
            goto fail;
        }
        PyObject* error_names = PyUnicode_Join(comma, posonly_names);
        Py_DECREF(comma);
        if (error_names == NULL) {
            goto fail;
        }
        _PyErr_Format(tstate, PyExc_TypeError,
                      "%U() got some positional-only arguments passed"
                      " as keyword arguments: '%U'",
                      qualname, error_names);
        Py_DECREF(error_names);
        goto fail;
    }

    Py_DECREF(posonly_names);
    return 0;

fail:
    Py_XDECREF(posonly_names);
    return 1;

}


static inline unsigned char *
scan_back_to_entry_start(unsigned char *p) {
    for (; (p[0]&128) == 0; p--);
    return p;
}

static inline unsigned char *
skip_to_next_entry(unsigned char *p, unsigned char *end) {
    while (p < end && ((p[0] & 128) == 0)) {
        p++;
    }
    return p;
}


#define MAX_LINEAR_SEARCH 40

static int
get_exception_handler(PyCodeObject *code, int index, int *level, int *handler, int *lasti)
{
    unsigned char *start = (unsigned char *)PyBytes_AS_STRING(code->co_exceptiontable);
    unsigned char *end = start + PyBytes_GET_SIZE(code->co_exceptiontable);
    /* Invariants:
     * start_table == end_table OR
     * start_table points to a legal entry and end_table points
     * beyond the table or to a legal entry that is after index.
     */
    if (end - start > MAX_LINEAR_SEARCH) {
        int offset;
        parse_varint(start, &offset);
        if (offset > index) {
            return 0;
        }
        do {
            unsigned char * mid = start + ((end-start)>>1);
            mid = scan_back_to_entry_start(mid);
            parse_varint(mid, &offset);
            if (offset > index) {
                end = mid;
            }
            else {
                start = mid;
            }

        } while (end - start > MAX_LINEAR_SEARCH);
    }
    unsigned char *scan = start;
    while (scan < end) {
        int start_offset, size;
        scan = parse_varint(scan, &start_offset);
        if (start_offset > index) {
            break;
        }
        scan = parse_varint(scan, &size);
        if (start_offset + size > index) {
            scan = parse_varint(scan, handler);
            int depth_and_lasti;
            parse_varint(scan, &depth_and_lasti);
            *level = depth_and_lasti >> 1;
            *lasti = depth_and_lasti & 1;
            return 1;
        }
        scan = skip_to_next_entry(scan, end);
    }
    return 0;
}

static int
initialize_locals(PyThreadState *tstate, PyFunctionObject *func,
    PyObject **localsplus, PyObject *const *args,
    Py_ssize_t argcount, PyObject *kwnames)
{
    PyCodeObject *co = (PyCodeObject*)func->func_code;
    const Py_ssize_t total_args = co->co_argcount + co->co_kwonlyargcount;

    /* Create a dictionary for keyword parameters (**kwags) */
    PyObject *kwdict;
    Py_ssize_t i;
    if (co->co_flags & CO_VARKEYWORDS) {
        kwdict = PyDict_New();
        if (kwdict == NULL) {
            goto fail_pre_positional;
        }
        i = total_args;
        if (co->co_flags & CO_VARARGS) {
            i++;
        }
        assert(localsplus[i] == NULL);
        localsplus[i] = kwdict;
    }
    else {
        kwdict = NULL;
    }

    /* Copy all positional arguments into local variables */
    Py_ssize_t j, n;
    if (argcount > co->co_argcount) {
        n = co->co_argcount;
    }
    else {
        n = argcount;
    }
    for (j = 0; j < n; j++) {
        PyObject *x = args[j];
        assert(localsplus[j] == NULL);
        localsplus[j] = x;
    }

    /* Pack other positional arguments into the *args argument */
    if (co->co_flags & CO_VARARGS) {
        PyObject *u = NULL;
        if (argcount == n) {
            u = Py_NewRef(&_Py_SINGLETON(tuple_empty));
        }
        else {
            assert(args != NULL);
            u = _PyTuple_FromArraySteal(args + n, argcount - n);
        }
        if (u == NULL) {
            goto fail_post_positional;
        }
        assert(localsplus[total_args] == NULL);
        localsplus[total_args] = u;
    }
    else if (argcount > n) {
        /* Too many postional args. Error is reported later */
        for (j = n; j < argcount; j++) {
            Py_DECREF(args[j]);
        }
    }

    /* Handle keyword arguments */
    if (kwnames != NULL) {
        Py_ssize_t kwcount = PyTuple_GET_SIZE(kwnames);
        for (i = 0; i < kwcount; i++) {
            PyObject **co_varnames;
            PyObject *keyword = PyTuple_GET_ITEM(kwnames, i);
            PyObject *value = args[i+argcount];
            Py_ssize_t j;

            if (keyword == NULL || !PyUnicode_Check(keyword)) {
                _PyErr_Format(tstate, PyExc_TypeError,
                            "%U() keywords must be strings",
                          func->func_qualname);
                goto kw_fail;
            }

            /* Speed hack: do raw pointer compares. As names are
            normally interned this should almost always hit. */
            co_varnames = ((PyTupleObject *)(co->co_localsplusnames))->ob_item;
            for (j = co->co_posonlyargcount; j < total_args; j++) {
                PyObject *varname = co_varnames[j];
                if (varname == keyword) {
                    goto kw_found;
                }
            }

            /* Slow fallback, just in case */
            for (j = co->co_posonlyargcount; j < total_args; j++) {
                PyObject *varname = co_varnames[j];
                int cmp = PyObject_RichCompareBool( keyword, varname, Py_EQ);
                if (cmp > 0) {
                    goto kw_found;
                }
                else if (cmp < 0) {
                    goto kw_fail;
                }
            }

            assert(j >= total_args);
            if (kwdict == NULL) {

                if (co->co_posonlyargcount
                    && positional_only_passed_as_keyword(tstate, co,
                                                        kwcount, kwnames,
                                                        func->func_qualname))
                {
                    goto kw_fail;
                }

                _PyErr_Format(tstate, PyExc_TypeError,
                            "%U() got an unexpected keyword argument '%S'",
                          func->func_qualname, keyword);
                goto kw_fail;
            }

            if (PyDict_SetItem(kwdict, keyword, value) == -1) {
                goto kw_fail;
            }
            Py_DECREF(value);
            continue;

        kw_fail:
            for (;i < kwcount; i++) {
                PyObject *value = args[i+argcount];
                Py_DECREF(value);
            }
            goto fail_post_args;

        kw_found:
            if (localsplus[j] != NULL) {
                _PyErr_Format(tstate, PyExc_TypeError,
                            "%U() got multiple values for argument '%S'",
                          func->func_qualname, keyword);
                goto kw_fail;
            }
            localsplus[j] = value;
        }
    }

    /* Check the number of positional arguments */
    if ((argcount > co->co_argcount) && !(co->co_flags & CO_VARARGS)) {
        too_many_positional(tstate, co, argcount, func->func_defaults, localsplus,
                            func->func_qualname);
        goto fail_post_args;
    }

    /* Add missing positional arguments (copy default values from defs) */
    if (argcount < co->co_argcount) {
        Py_ssize_t defcount = func->func_defaults == NULL ? 0 : PyTuple_GET_SIZE(func->func_defaults);
        Py_ssize_t m = co->co_argcount - defcount;
        Py_ssize_t missing = 0;
        for (i = argcount; i < m; i++) {
            if (localsplus[i] == NULL) {
                missing++;
            }
        }
        if (missing) {
            missing_arguments(tstate, co, missing, defcount, localsplus,
                              func->func_qualname);
            goto fail_post_args;
        }
        if (n > m)
            i = n - m;
        else
            i = 0;
        if (defcount) {
            PyObject **defs = &PyTuple_GET_ITEM(func->func_defaults, 0);
            for (; i < defcount; i++) {
                if (localsplus[m+i] == NULL) {
                    PyObject *def = defs[i];
                    localsplus[m+i] = Py_NewRef(def);
                }
            }
        }
    }

    /* Add missing keyword arguments (copy default values from kwdefs) */
    if (co->co_kwonlyargcount > 0) {
        Py_ssize_t missing = 0;
        for (i = co->co_argcount; i < total_args; i++) {
            if (localsplus[i] != NULL)
                continue;
            PyObject *varname = PyTuple_GET_ITEM(co->co_localsplusnames, i);
            if (func->func_kwdefaults != NULL) {
                PyObject *def = PyDict_GetItemWithError(func->func_kwdefaults, varname);
                if (def) {
                    localsplus[i] = Py_NewRef(def);
                    continue;
                }
                else if (_PyErr_Occurred(tstate)) {
                    goto fail_post_args;
                }
            }
            missing++;
        }
        if (missing) {
            missing_arguments(tstate, co, missing, -1, localsplus,
                              func->func_qualname);
            goto fail_post_args;
        }
    }
    return 0;

fail_pre_positional:
    for (j = 0; j < argcount; j++) {
        Py_DECREF(args[j]);
    }
    /* fall through */
fail_post_positional:
    if (kwnames) {
        Py_ssize_t kwcount = PyTuple_GET_SIZE(kwnames);
        for (j = argcount; j < argcount+kwcount; j++) {
            Py_DECREF(args[j]);
        }
    }
    /* fall through */
fail_post_args:
    return -1;
}

static void
clear_thread_frame(PyThreadState *tstate, _PyInterpreterFrame * frame)
{
    assert(frame->owner == FRAME_OWNED_BY_THREAD);
    // Make sure that this is, indeed, the top frame. We can't check this in
    // _PyThreadState_PopFrame, since f_code is already cleared at that point:
    assert((PyObject **)frame + frame->f_code->co_framesize ==
        tstate->datastack_top);
    tstate->c_recursion_remaining--;
    assert(frame->frame_obj == NULL || frame->frame_obj->f_frame == frame);
    _PyFrame_ClearExceptCode(frame);
    Py_DECREF(frame->f_code);
    tstate->c_recursion_remaining++;
    _PyThreadState_PopFrame(tstate, frame);
}

static void
clear_gen_frame(PyThreadState *tstate, _PyInterpreterFrame * frame)
{
    assert(frame->owner == FRAME_OWNED_BY_GENERATOR);
    PyGenObject *gen = _PyFrame_GetGenerator(frame);
    gen->gi_frame_state = FRAME_CLEARED;
    assert(tstate->exc_info == &gen->gi_exc_state);
    tstate->exc_info = gen->gi_exc_state.previous_item;
    gen->gi_exc_state.previous_item = NULL;
    tstate->c_recursion_remaining--;
    assert(frame->frame_obj == NULL || frame->frame_obj->f_frame == frame);
    _PyFrame_ClearExceptCode(frame);
    tstate->c_recursion_remaining++;
    frame->previous = NULL;
}

static void
_PyEvalFrameClearAndPop(PyThreadState *tstate, _PyInterpreterFrame * frame)
{
    if (frame->owner == FRAME_OWNED_BY_THREAD) {
        clear_thread_frame(tstate, frame);
    }
    else {
        clear_gen_frame(tstate, frame);
    }
}

/* Consumes references to func, locals and all the args */
static _PyInterpreterFrame *
_PyEvalFramePushAndInit(PyThreadState *tstate, PyFunctionObject *func,
                        PyObject *locals, PyObject* const* args,
                        size_t argcount, PyObject *kwnames)
{
    PyCodeObject * code = (PyCodeObject *)func->func_code;
    CALL_STAT_INC(frames_pushed);
    _PyInterpreterFrame *frame = _PyThreadState_PushFrame(tstate, code->co_framesize);
    if (frame == NULL) {
        goto fail;
    }
    _PyFrame_Initialize(frame, func, locals, code, 0);
    if (initialize_locals(tstate, func, frame->localsplus, args, argcount, kwnames)) {
        assert(frame->owner == FRAME_OWNED_BY_THREAD);
        clear_thread_frame(tstate, frame);
        return NULL;
    }
    return frame;
fail:
    /* Consume the references */
    for (size_t i = 0; i < argcount; i++) {
        Py_DECREF(args[i]);
    }
    if (kwnames) {
        Py_ssize_t kwcount = PyTuple_GET_SIZE(kwnames);
        for (Py_ssize_t i = 0; i < kwcount; i++) {
            Py_DECREF(args[i+argcount]);
        }
    }
    PyErr_NoMemory();
    return NULL;
}

/* Same as _PyEvalFramePushAndInit but takes an args tuple and kwargs dict.
   Steals references to func, callargs and kwargs.
*/
static _PyInterpreterFrame *
_PyEvalFramePushAndInit_Ex(PyThreadState *tstate, PyFunctionObject *func,
    PyObject *locals, Py_ssize_t nargs, PyObject *callargs, PyObject *kwargs)
{
    bool has_dict = (kwargs != NULL && PyDict_GET_SIZE(kwargs) > 0);
    PyObject *kwnames = NULL;
    PyObject *const *newargs;
    if (has_dict) {
        newargs = _PyStack_UnpackDict(tstate, _PyTuple_ITEMS(callargs), nargs, kwargs, &kwnames);
        if (newargs == NULL) {
            Py_DECREF(func);
            goto error;
        }
    }
    else {
        newargs = &PyTuple_GET_ITEM(callargs, 0);
        /* We need to incref all our args since the new frame steals the references. */
        for (Py_ssize_t i = 0; i < nargs; ++i) {
            Py_INCREF(PyTuple_GET_ITEM(callargs, i));
        }
    }
    _PyInterpreterFrame *new_frame = _PyEvalFramePushAndInit(
        tstate, (PyFunctionObject *)func, locals,
        newargs, nargs, kwnames
    );
    if (has_dict) {
        _PyStack_UnpackDict_FreeNoDecRef(newargs, kwnames);
    }
    /* No need to decref func here because the reference has been stolen by
       _PyEvalFramePushAndInit.
    */
    Py_DECREF(callargs);
    Py_XDECREF(kwargs);
    return new_frame;
error:
    Py_DECREF(callargs);
    Py_XDECREF(kwargs);
    return NULL;
}

PyObject *
_PyEval_Vector(PyThreadState *tstate, PyFunctionObject *func,
               PyObject *locals,
               PyObject* const* args, size_t argcount,
               PyObject *kwnames)
{
    /* _PyEvalFramePushAndInit consumes the references
     * to func, locals and all its arguments */
    Py_INCREF(func);
    Py_XINCREF(locals);
    for (size_t i = 0; i < argcount; i++) {
        Py_INCREF(args[i]);
    }
    if (kwnames) {
        Py_ssize_t kwcount = PyTuple_GET_SIZE(kwnames);
        for (Py_ssize_t i = 0; i < kwcount; i++) {
            Py_INCREF(args[i+argcount]);
        }
    }
    _PyInterpreterFrame *frame = _PyEvalFramePushAndInit(
        tstate, func, locals, args, argcount, kwnames);
    if (frame == NULL) {
        return NULL;
    }
    EVAL_CALL_STAT_INC(EVAL_CALL_VECTOR);
    return _PyEval_EvalFrame(tstate, frame, 0);
}

/* Legacy API */
PyObject *
PyEval_EvalCodeEx(PyObject *_co, PyObject *globals, PyObject *locals,
                  PyObject *const *args, int argcount,
                  PyObject *const *kws, int kwcount,
                  PyObject *const *defs, int defcount,
                  PyObject *kwdefs, PyObject *closure)
{
    PyThreadState *tstate = _PyThreadState_GET();
    PyObject *res = NULL;
    PyObject *defaults = _PyTuple_FromArray(defs, defcount);
    if (defaults == NULL) {
        return NULL;
    }
    PyObject *builtins = _PyEval_BuiltinsFromGlobals(tstate, globals); // borrowed ref
    if (builtins == NULL) {
        Py_DECREF(defaults);
        return NULL;
    }
    if (locals == NULL) {
        locals = globals;
    }
    PyObject *kwnames = NULL;
    PyObject *const *allargs;
    PyObject **newargs = NULL;
    PyFunctionObject *func = NULL;
    if (kwcount == 0) {
        allargs = args;
    }
    else {
        kwnames = PyTuple_New(kwcount);
        if (kwnames == NULL) {
            goto fail;
        }
        newargs = PyMem_Malloc(sizeof(PyObject *)*(kwcount+argcount));
        if (newargs == NULL) {
            goto fail;
        }
        for (int i = 0; i < argcount; i++) {
            newargs[i] = args[i];
        }
        for (int i = 0; i < kwcount; i++) {
            PyTuple_SET_ITEM(kwnames, i, Py_NewRef(kws[2*i]));
            newargs[argcount+i] = kws[2*i+1];
        }
        allargs = newargs;
    }
    PyFrameConstructor constr = {
        .fc_globals = globals,
        .fc_builtins = builtins,
        .fc_name = ((PyCodeObject *)_co)->co_name,
        .fc_qualname = ((PyCodeObject *)_co)->co_name,
        .fc_code = _co,
        .fc_defaults = defaults,
        .fc_kwdefaults = kwdefs,
        .fc_closure = closure
    };
    func = _PyFunction_FromConstructor(&constr);
    if (func == NULL) {
        goto fail;
    }
    EVAL_CALL_STAT_INC(EVAL_CALL_LEGACY);
    res = _PyEval_Vector(tstate, func, locals,
                         allargs, argcount,
                         kwnames);
fail:
    Py_XDECREF(func);
    Py_XDECREF(kwnames);
    PyMem_Free(newargs);
    Py_DECREF(defaults);
    return res;
}


/* Logic for the raise statement (too complicated for inlining).
   This *consumes* a reference count to each of its arguments. */
static int
do_raise(PyThreadState *tstate, PyObject *exc, PyObject *cause)
{
    PyObject *type = NULL, *value = NULL;

    if (exc == NULL) {
        /* Reraise */
        _PyErr_StackItem *exc_info = _PyErr_GetTopmostException(tstate);
        exc = exc_info->exc_value;
        if (Py_IsNone(exc) || exc == NULL) {
            _PyErr_SetString(tstate, PyExc_RuntimeError,
                             "No active exception to reraise");
            return 0;
        }
        Py_INCREF(exc);
        assert(PyExceptionInstance_Check(exc));
        _PyErr_SetRaisedException(tstate, exc);
        return 1;
    }

    /* We support the following forms of raise:
       raise
       raise <instance>
       raise <type> */

    if (PyExceptionClass_Check(exc)) {
        type = exc;
        value = _PyObject_CallNoArgs(exc);
        if (value == NULL)
            goto raise_error;
        if (!PyExceptionInstance_Check(value)) {
            _PyErr_Format(tstate, PyExc_TypeError,
                          "calling %R should have returned an instance of "
                          "BaseException, not %R",
                          type, Py_TYPE(value));
             goto raise_error;
        }
    }
    else if (PyExceptionInstance_Check(exc)) {
        value = exc;
        type = PyExceptionInstance_Class(exc);
        Py_INCREF(type);
    }
    else {
        /* Not something you can raise.  You get an exception
           anyway, just not what you specified :-) */
        Py_DECREF(exc);
        _PyErr_SetString(tstate, PyExc_TypeError,
                         "exceptions must derive from BaseException");
        goto raise_error;
    }

    assert(type != NULL);
    assert(value != NULL);

    if (cause) {
        PyObject *fixed_cause;
        if (PyExceptionClass_Check(cause)) {
            fixed_cause = _PyObject_CallNoArgs(cause);
            if (fixed_cause == NULL)
                goto raise_error;
            Py_DECREF(cause);
        }
        else if (PyExceptionInstance_Check(cause)) {
            fixed_cause = cause;
        }
        else if (Py_IsNone(cause)) {
            Py_DECREF(cause);
            fixed_cause = NULL;
        }
        else {
            _PyErr_SetString(tstate, PyExc_TypeError,
                             "exception causes must derive from "
                             "BaseException");
            goto raise_error;
        }
        PyException_SetCause(value, fixed_cause);
    }

    _PyErr_SetObject(tstate, type, value);
    /* _PyErr_SetObject incref's its arguments */
    Py_DECREF(value);
    Py_DECREF(type);
    return 0;

raise_error:
    Py_XDECREF(value);
    Py_XDECREF(type);
    Py_XDECREF(cause);
    return 0;
}

/* Logic for matching an exception in an except* clause (too
   complicated for inlining).
*/

static int
exception_group_match(PyObject* exc_value, PyObject *match_type,
                      PyObject **match, PyObject **rest)
{
    if (Py_IsNone(exc_value)) {
        *match = Py_NewRef(Py_None);
        *rest = Py_NewRef(Py_None);
        return 0;
    }
    assert(PyExceptionInstance_Check(exc_value));

    if (PyErr_GivenExceptionMatches(exc_value, match_type)) {
        /* Full match of exc itself */
        bool is_eg = _PyBaseExceptionGroup_Check(exc_value);
        if (is_eg) {
            *match = Py_NewRef(exc_value);
        }
        else {
            /* naked exception - wrap it */
            PyObject *excs = PyTuple_Pack(1, exc_value);
            if (excs == NULL) {
                return -1;
            }
            PyObject *wrapped = _PyExc_CreateExceptionGroup("", excs);
            Py_DECREF(excs);
            if (wrapped == NULL) {
                return -1;
            }
            *match = wrapped;
        }
        *rest = Py_NewRef(Py_None);
        return 0;
    }

    /* exc_value does not match match_type.
     * Check for partial match if it's an exception group.
     */
    if (_PyBaseExceptionGroup_Check(exc_value)) {
        PyObject *pair = PyObject_CallMethod(exc_value, "split", "(O)",
                                             match_type);
        if (pair == NULL) {
            return -1;
        }
        assert(PyTuple_CheckExact(pair));
        assert(PyTuple_GET_SIZE(pair) == 2);
        *match = Py_NewRef(PyTuple_GET_ITEM(pair, 0));
        *rest = Py_NewRef(PyTuple_GET_ITEM(pair, 1));
        Py_DECREF(pair);
        return 0;
    }
    /* no match */
    *match = Py_NewRef(Py_None);
    *rest = Py_NewRef(exc_value);
    return 0;
}

/* Iterate v argcnt times and store the results on the stack (via decreasing
   sp).  Return 1 for success, 0 if error.

   If argcntafter == -1, do a simple unpack. If it is >= 0, do an unpack
   with a variable target.
*/

static int
unpack_iterable(PyThreadState *tstate, PyObject *v,
                int argcnt, int argcntafter, PyObject **sp)
{
    int i = 0, j = 0;
    Py_ssize_t ll = 0;
    PyObject *it;  /* iter(v) */
    PyObject *w;
    PyObject *l = NULL; /* variable list */

    assert(v != NULL);

    it = PyObject_GetIter(v);
    if (it == NULL) {
        if (_PyErr_ExceptionMatches(tstate, PyExc_TypeError) &&
            Py_TYPE(v)->tp_iter == NULL && !PySequence_Check(v))
        {
            _PyErr_Format(tstate, PyExc_TypeError,
                          "cannot unpack non-iterable %.200s object",
                          Py_TYPE(v)->tp_name);
        }
        return 0;
    }

    for (; i < argcnt; i++) {
        w = PyIter_Next(it);
        if (w == NULL) {
            /* Iterator done, via error or exhaustion. */
            if (!_PyErr_Occurred(tstate)) {
                if (argcntafter == -1) {
                    _PyErr_Format(tstate, PyExc_ValueError,
                                  "not enough values to unpack "
                                  "(expected %d, got %d)",
                                  argcnt, i);
                }
                else {
                    _PyErr_Format(tstate, PyExc_ValueError,
                                  "not enough values to unpack "
                                  "(expected at least %d, got %d)",
                                  argcnt + argcntafter, i);
                }
            }
            goto Error;
        }
        *--sp = w;
    }

    if (argcntafter == -1) {
        /* We better have exhausted the iterator now. */
        w = PyIter_Next(it);
        if (w == NULL) {
            if (_PyErr_Occurred(tstate))
                goto Error;
            Py_DECREF(it);
            return 1;
        }
        Py_DECREF(w);
        _PyErr_Format(tstate, PyExc_ValueError,
                      "too many values to unpack (expected %d)",
                      argcnt);
        goto Error;
    }

    l = PySequence_List(it);
    if (l == NULL)
        goto Error;
    *--sp = l;
    i++;

    ll = PyList_GET_SIZE(l);
    if (ll < argcntafter) {
        _PyErr_Format(tstate, PyExc_ValueError,
            "not enough values to unpack (expected at least %d, got %zd)",
            argcnt + argcntafter, argcnt + ll);
        goto Error;
    }

    /* Pop the "after-variable" args off the list. */
    for (j = argcntafter; j > 0; j--, i++) {
        *--sp = PyList_GET_ITEM(l, ll - j);
    }
    /* Resize the list. */
    Py_SET_SIZE(l, ll - argcntafter);
    Py_DECREF(it);
    return 1;

Error:
    for (; i > 0; i--, sp++)
        Py_DECREF(*sp);
    Py_XDECREF(it);
    return 0;
}

static int
do_monitor_exc(PyThreadState *tstate, _PyInterpreterFrame *frame,
               _Py_CODEUNIT *instr, int event)
{
    assert(event < _PY_MONITORING_UNGROUPED_EVENTS);
    PyObject *exc = PyErr_GetRaisedException();
    assert(exc != NULL);
    int err = _Py_call_instrumentation_arg(tstate, event, frame, instr, exc);
    if (err == 0) {
        PyErr_SetRaisedException(exc);
    }
    else {
        assert(PyErr_Occurred());
        Py_DECREF(exc);
    }
    return err;
}

static inline bool
no_tools_for_global_event(PyThreadState *tstate, int event)
{
    return tstate->interp->monitors.tools[event] == 0;
}

static inline bool
no_tools_for_local_event(PyThreadState *tstate, _PyInterpreterFrame *frame, int event)
{
    assert(event < _PY_MONITORING_LOCAL_EVENTS);
    _PyCoMonitoringData *data = frame->f_code->_co_monitoring;
    if (data) {
        return data->active_monitors.tools[event] == 0;
    }
    else {
        return no_tools_for_global_event(tstate, event);
    }
}

static void
monitor_raise(PyThreadState *tstate, _PyInterpreterFrame *frame,
              _Py_CODEUNIT *instr)
{
    if (no_tools_for_global_event(tstate, PY_MONITORING_EVENT_RAISE)) {
        return;
    }
    do_monitor_exc(tstate, frame, instr, PY_MONITORING_EVENT_RAISE);
}

static void
monitor_reraise(PyThreadState *tstate, _PyInterpreterFrame *frame,
              _Py_CODEUNIT *instr)
{
    if (no_tools_for_global_event(tstate, PY_MONITORING_EVENT_RERAISE)) {
        return;
    }
    do_monitor_exc(tstate, frame, instr, PY_MONITORING_EVENT_RERAISE);
}

static int
monitor_stop_iteration(PyThreadState *tstate, _PyInterpreterFrame *frame,
                       _Py_CODEUNIT *instr)
{
    if (no_tools_for_local_event(tstate, frame, PY_MONITORING_EVENT_STOP_ITERATION)) {
        return 0;
    }
    return do_monitor_exc(tstate, frame, instr, PY_MONITORING_EVENT_STOP_ITERATION);
}

static void
monitor_unwind(PyThreadState *tstate,
               _PyInterpreterFrame *frame,
               _Py_CODEUNIT *instr)
{
    if (no_tools_for_global_event(tstate, PY_MONITORING_EVENT_PY_UNWIND)) {
        return;
    }
    do_monitor_exc(tstate, frame, instr, PY_MONITORING_EVENT_PY_UNWIND);
}


static int
monitor_handled(PyThreadState *tstate,
                _PyInterpreterFrame *frame,
                _Py_CODEUNIT *instr, PyObject *exc)
{
    if (no_tools_for_global_event(tstate, PY_MONITORING_EVENT_EXCEPTION_HANDLED)) {
        return 0;
    }
    return _Py_call_instrumentation_arg(tstate, PY_MONITORING_EVENT_EXCEPTION_HANDLED, frame, instr, exc);
}

static void
monitor_throw(PyThreadState *tstate,
              _PyInterpreterFrame *frame,
              _Py_CODEUNIT *instr)
{
    if (no_tools_for_global_event(tstate, PY_MONITORING_EVENT_PY_THROW)) {
        return;
    }
    do_monitor_exc(tstate, frame, instr, PY_MONITORING_EVENT_PY_THROW);
}

void
PyThreadState_EnterTracing(PyThreadState *tstate)
{
    assert(tstate->tracing >= 0);
    tstate->tracing++;
}

void
PyThreadState_LeaveTracing(PyThreadState *tstate)
{
    assert(tstate->tracing > 0);
    tstate->tracing--;
}


PyObject*
_PyEval_CallTracing(PyObject *func, PyObject *args)
{
    // Save and disable tracing
    PyThreadState *tstate = _PyThreadState_GET();
    int save_tracing = tstate->tracing;
    tstate->tracing = 0;

    // Call the tracing function
    PyObject *result = PyObject_Call(func, args, NULL);

    // Restore tracing
    tstate->tracing = save_tracing;
    return result;
}

void
PyEval_SetProfile(Py_tracefunc func, PyObject *arg)
{
    PyThreadState *tstate = _PyThreadState_GET();
    if (_PyEval_SetProfile(tstate, func, arg) < 0) {
        /* Log _PySys_Audit() error */
        _PyErr_WriteUnraisableMsg("in PyEval_SetProfile", NULL);
    }
}

void
PyEval_SetProfileAllThreads(Py_tracefunc func, PyObject *arg)
{
    PyThreadState *this_tstate = _PyThreadState_GET();
    PyInterpreterState* interp = this_tstate->interp;

    _PyRuntimeState *runtime = &_PyRuntime;
    HEAD_LOCK(runtime);
    PyThreadState* ts = PyInterpreterState_ThreadHead(interp);
    HEAD_UNLOCK(runtime);

    while (ts) {
        if (_PyEval_SetProfile(ts, func, arg) < 0) {
            _PyErr_WriteUnraisableMsg("in PyEval_SetProfileAllThreads", NULL);
        }
        HEAD_LOCK(runtime);
        ts = PyThreadState_Next(ts);
        HEAD_UNLOCK(runtime);
    }
}

void
PyEval_SetTrace(Py_tracefunc func, PyObject *arg)
{
    PyThreadState *tstate = _PyThreadState_GET();
    if (_PyEval_SetTrace(tstate, func, arg) < 0) {
        /* Log _PySys_Audit() error */
        _PyErr_WriteUnraisableMsg("in PyEval_SetTrace", NULL);
    }
}

void
PyEval_SetTraceAllThreads(Py_tracefunc func, PyObject *arg)
{
    PyThreadState *this_tstate = _PyThreadState_GET();
    PyInterpreterState* interp = this_tstate->interp;

    _PyRuntimeState *runtime = &_PyRuntime;
    HEAD_LOCK(runtime);
    PyThreadState* ts = PyInterpreterState_ThreadHead(interp);
    HEAD_UNLOCK(runtime);

    while (ts) {
        if (_PyEval_SetTrace(ts, func, arg) < 0) {
            _PyErr_WriteUnraisableMsg("in PyEval_SetTraceAllThreads", NULL);
        }
        HEAD_LOCK(runtime);
        ts = PyThreadState_Next(ts);
        HEAD_UNLOCK(runtime);
    }
}

int
_PyEval_SetCoroutineOriginTrackingDepth(int depth)
{
    PyThreadState *tstate = _PyThreadState_GET();
    if (depth < 0) {
        _PyErr_SetString(tstate, PyExc_ValueError, "depth must be >= 0");
        return -1;
    }
    tstate->coroutine_origin_tracking_depth = depth;
    return 0;
}


int
_PyEval_GetCoroutineOriginTrackingDepth(void)
{
    PyThreadState *tstate = _PyThreadState_GET();
    return tstate->coroutine_origin_tracking_depth;
}

int
_PyEval_SetAsyncGenFirstiter(PyObject *firstiter)
{
    PyThreadState *tstate = _PyThreadState_GET();

    if (_PySys_Audit(tstate, "sys.set_asyncgen_hook_firstiter", NULL) < 0) {
        return -1;
    }

    Py_XSETREF(tstate->async_gen_firstiter, Py_XNewRef(firstiter));
    return 0;
}

PyObject *
_PyEval_GetAsyncGenFirstiter(void)
{
    PyThreadState *tstate = _PyThreadState_GET();
    return tstate->async_gen_firstiter;
}

int
_PyEval_SetAsyncGenFinalizer(PyObject *finalizer)
{
    PyThreadState *tstate = _PyThreadState_GET();

    if (_PySys_Audit(tstate, "sys.set_asyncgen_hook_finalizer", NULL) < 0) {
        return -1;
    }

    Py_XSETREF(tstate->async_gen_finalizer, Py_XNewRef(finalizer));
    return 0;
}

PyObject *
_PyEval_GetAsyncGenFinalizer(void)
{
    PyThreadState *tstate = _PyThreadState_GET();
    return tstate->async_gen_finalizer;
}

_PyInterpreterFrame *
_PyEval_GetFrame(void)
{
    PyThreadState *tstate = _PyThreadState_GET();
    return _PyThreadState_GetFrame(tstate);
}

PyFrameObject *
PyEval_GetFrame(void)
{
    _PyInterpreterFrame *frame = _PyEval_GetFrame();
    if (frame == NULL) {
        return NULL;
    }
    PyFrameObject *f = _PyFrame_GetFrameObject(frame);
    if (f == NULL) {
        PyErr_Clear();
    }
    return f;
}

PyObject *
_PyEval_GetBuiltins(PyThreadState *tstate)
{
    _PyInterpreterFrame *frame = _PyThreadState_GetFrame(tstate);
    if (frame != NULL) {
        return frame->f_builtins;
    }
    return tstate->interp->builtins;
}

PyObject *
PyEval_GetBuiltins(void)
{
    PyThreadState *tstate = _PyThreadState_GET();
    return _PyEval_GetBuiltins(tstate);
}

/* Convenience function to get a builtin from its name */
PyObject *
_PyEval_GetBuiltin(PyObject *name)
{
    PyThreadState *tstate = _PyThreadState_GET();
    PyObject *attr = PyObject_GetItem(PyEval_GetBuiltins(), name);
    if (attr == NULL && _PyErr_ExceptionMatches(tstate, PyExc_KeyError)) {
        _PyErr_SetObject(tstate, PyExc_AttributeError, name);
    }
    return attr;
}

PyObject *
_PyEval_GetBuiltinId(_Py_Identifier *name)
{
    return _PyEval_GetBuiltin(_PyUnicode_FromId(name));
}

PyObject *
PyEval_GetLocals(void)
{
    PyThreadState *tstate = _PyThreadState_GET();
     _PyInterpreterFrame *current_frame = _PyThreadState_GetFrame(tstate);
    if (current_frame == NULL) {
        _PyErr_SetString(tstate, PyExc_SystemError, "frame does not exist");
        return NULL;
    }

    if (_PyFrame_FastToLocalsWithError(current_frame) < 0) {
        return NULL;
    }

    PyObject *locals = current_frame->f_locals;
    assert(locals != NULL);
    return locals;
}

PyObject *
_PyEval_GetFrameLocals(void)
{
    PyThreadState *tstate = _PyThreadState_GET();
     _PyInterpreterFrame *current_frame = _PyThreadState_GetFrame(tstate);
    if (current_frame == NULL) {
        _PyErr_SetString(tstate, PyExc_SystemError, "frame does not exist");
        return NULL;
    }

    return _PyFrame_GetLocals(current_frame, 1);
}

PyObject *
PyEval_GetGlobals(void)
{
    PyThreadState *tstate = _PyThreadState_GET();
    _PyInterpreterFrame *current_frame = _PyThreadState_GetFrame(tstate);
    if (current_frame == NULL) {
        return NULL;
    }
    return current_frame->f_globals;
}

int
PyEval_MergeCompilerFlags(PyCompilerFlags *cf)
{
    PyThreadState *tstate = _PyThreadState_GET();
    _PyInterpreterFrame *current_frame = tstate->cframe->current_frame;
    int result = cf->cf_flags != 0;

    if (current_frame != NULL) {
        const int codeflags = current_frame->f_code->co_flags;
        const int compilerflags = codeflags & PyCF_MASK;
        if (compilerflags) {
            result = 1;
            cf->cf_flags |= compilerflags;
        }
    }
    return result;
}


const char *
PyEval_GetFuncName(PyObject *func)
{
    if (PyMethod_Check(func))
        return PyEval_GetFuncName(PyMethod_GET_FUNCTION(func));
    else if (PyFunction_Check(func))
        return PyUnicode_AsUTF8(((PyFunctionObject*)func)->func_name);
    else if (PyCFunction_Check(func))
        return ((PyCFunctionObject*)func)->m_ml->ml_name;
    else
        return Py_TYPE(func)->tp_name;
}

const char *
PyEval_GetFuncDesc(PyObject *func)
{
    if (PyMethod_Check(func))
        return "()";
    else if (PyFunction_Check(func))
        return "()";
    else if (PyCFunction_Check(func))
        return "()";
    else
        return " object";
}

/* Extract a slice index from a PyLong or an object with the
   nb_index slot defined, and store in *pi.
   Silently reduce values larger than PY_SSIZE_T_MAX to PY_SSIZE_T_MAX,
   and silently boost values less than PY_SSIZE_T_MIN to PY_SSIZE_T_MIN.
   Return 0 on error, 1 on success.
*/
int
_PyEval_SliceIndex(PyObject *v, Py_ssize_t *pi)
{
    PyThreadState *tstate = _PyThreadState_GET();
    if (!Py_IsNone(v)) {
        Py_ssize_t x;
        if (_PyIndex_Check(v)) {
            x = PyNumber_AsSsize_t(v, NULL);
            if (x == -1 && _PyErr_Occurred(tstate))
                return 0;
        }
        else {
            _PyErr_SetString(tstate, PyExc_TypeError,
                             "slice indices must be integers or "
                             "None or have an __index__ method");
            return 0;
        }
        *pi = x;
    }
    return 1;
}

int
_PyEval_SliceIndexNotNone(PyObject *v, Py_ssize_t *pi)
{
    PyThreadState *tstate = _PyThreadState_GET();
    Py_ssize_t x;
    if (_PyIndex_Check(v)) {
        x = PyNumber_AsSsize_t(v, NULL);
        if (x == -1 && _PyErr_Occurred(tstate))
            return 0;
    }
    else {
        _PyErr_SetString(tstate, PyExc_TypeError,
                         "slice indices must be integers or "
                         "have an __index__ method");
        return 0;
    }
    *pi = x;
    return 1;
}

static PyObject *
import_name(PyThreadState *tstate, _PyInterpreterFrame *frame,
            PyObject *name, PyObject *fromlist, PyObject *level)
{
    PyObject *import_func, *res;
    PyObject* stack[5];

    import_func = PyObject_GetItem(frame->f_builtins, &_Py_ID(__import__));
    if (import_func == NULL) {
        if (_PyErr_ExceptionMatches(tstate, PyExc_KeyError)) {
            _PyErr_SetString(tstate, PyExc_ImportError, "__import__ not found");
        }
        return NULL;
    }
    PyObject *locals = frame->f_locals;
    /* Fast path for not overloaded __import__. */
    if (_PyImport_IsDefaultImportFunc(tstate->interp, import_func)) {
        Py_DECREF(import_func);
        int ilevel = _PyLong_AsInt(level);
        if (ilevel == -1 && _PyErr_Occurred(tstate)) {
            return NULL;
        }
        res = PyImport_ImportModuleLevelObject(
                        name,
                        frame->f_globals,
                        locals == NULL ? Py_None :locals,
                        fromlist,
                        ilevel);
        return res;
    }

    stack[0] = name;
    stack[1] = frame->f_globals;
    stack[2] = locals == NULL ? Py_None : locals;
    stack[3] = fromlist;
    stack[4] = level;
    res = _PyObject_FastCall(import_func, stack, 5);
    Py_DECREF(import_func);
    return res;
}

static PyObject *
import_from(PyThreadState *tstate, PyObject *v, PyObject *name)
{
    PyObject *x;
    PyObject *fullmodname, *pkgname, *pkgpath, *pkgname_or_unknown, *errmsg;

    if (_PyObject_LookupAttr(v, name, &x) != 0) {
        return x;
    }
    /* Issue #17636: in case this failed because of a circular relative
       import, try to fallback on reading the module directly from
       sys.modules. */
    pkgname = PyObject_GetAttr(v, &_Py_ID(__name__));
    if (pkgname == NULL) {
        goto error;
    }
    if (!PyUnicode_Check(pkgname)) {
        Py_CLEAR(pkgname);
        goto error;
    }
    fullmodname = PyUnicode_FromFormat("%U.%U", pkgname, name);
    if (fullmodname == NULL) {
        Py_DECREF(pkgname);
        return NULL;
    }
    x = PyImport_GetModule(fullmodname);
    Py_DECREF(fullmodname);
    if (x == NULL && !_PyErr_Occurred(tstate)) {
        goto error;
    }
    Py_DECREF(pkgname);
    return x;
 error:
    pkgpath = PyModule_GetFilenameObject(v);
    if (pkgname == NULL) {
        pkgname_or_unknown = PyUnicode_FromString("<unknown module name>");
        if (pkgname_or_unknown == NULL) {
            Py_XDECREF(pkgpath);
            return NULL;
        }
    } else {
        pkgname_or_unknown = pkgname;
    }

    if (pkgpath == NULL || !PyUnicode_Check(pkgpath)) {
        _PyErr_Clear(tstate);
        errmsg = PyUnicode_FromFormat(
            "cannot import name %R from %R (unknown location)",
            name, pkgname_or_unknown
        );
        /* NULL checks for errmsg and pkgname done by PyErr_SetImportError. */
        _PyErr_SetImportErrorWithNameFrom(errmsg, pkgname, NULL, name);
    }
    else {
        PyObject *spec = PyObject_GetAttr(v, &_Py_ID(__spec__));
        const char *fmt =
            _PyModuleSpec_IsInitializing(spec) ?
            "cannot import name %R from partially initialized module %R "
            "(most likely due to a circular import) (%S)" :
            "cannot import name %R from %R (%S)";
        Py_XDECREF(spec);

        errmsg = PyUnicode_FromFormat(fmt, name, pkgname_or_unknown, pkgpath);
        /* NULL checks for errmsg and pkgname done by PyErr_SetImportError. */
        _PyErr_SetImportErrorWithNameFrom(errmsg, pkgname, pkgpath, name);
    }

    Py_XDECREF(errmsg);
    Py_XDECREF(pkgname_or_unknown);
    Py_XDECREF(pkgpath);
    return NULL;
}

#define CANNOT_CATCH_MSG "catching classes that do not inherit from "\
                         "BaseException is not allowed"

#define CANNOT_EXCEPT_STAR_EG "catching ExceptionGroup with except* "\
                              "is not allowed. Use except instead."

static int
check_except_type_valid(PyThreadState *tstate, PyObject* right)
{
    if (PyTuple_Check(right)) {
        Py_ssize_t i, length;
        length = PyTuple_GET_SIZE(right);
        for (i = 0; i < length; i++) {
            PyObject *exc = PyTuple_GET_ITEM(right, i);
            if (!PyExceptionClass_Check(exc)) {
                _PyErr_SetString(tstate, PyExc_TypeError,
                    CANNOT_CATCH_MSG);
                return -1;
            }
        }
    }
    else {
        if (!PyExceptionClass_Check(right)) {
            _PyErr_SetString(tstate, PyExc_TypeError,
                CANNOT_CATCH_MSG);
            return -1;
        }
    }
    return 0;
}

static int
check_except_star_type_valid(PyThreadState *tstate, PyObject* right)
{
    if (check_except_type_valid(tstate, right) < 0) {
        return -1;
    }

    /* reject except *ExceptionGroup */

    int is_subclass = 0;
    if (PyTuple_Check(right)) {
        Py_ssize_t length = PyTuple_GET_SIZE(right);
        for (Py_ssize_t i = 0; i < length; i++) {
            PyObject *exc = PyTuple_GET_ITEM(right, i);
            is_subclass = PyObject_IsSubclass(exc, PyExc_BaseExceptionGroup);
            if (is_subclass < 0) {
                return -1;
            }
            if (is_subclass) {
                break;
            }
        }
    }
    else {
        is_subclass = PyObject_IsSubclass(right, PyExc_BaseExceptionGroup);
        if (is_subclass < 0) {
            return -1;
        }
    }
    if (is_subclass) {
        _PyErr_SetString(tstate, PyExc_TypeError,
            CANNOT_EXCEPT_STAR_EG);
            return -1;
    }
    return 0;
}

static int
check_args_iterable(PyThreadState *tstate, PyObject *func, PyObject *args)
{
    if (Py_TYPE(args)->tp_iter == NULL && !PySequence_Check(args)) {
        /* check_args_iterable() may be called with a live exception:
         * clear it to prevent calling _PyObject_FunctionStr() with an
         * exception set. */
        _PyErr_Clear(tstate);
        PyObject *funcstr = _PyObject_FunctionStr(func);
        if (funcstr != NULL) {
            _PyErr_Format(tstate, PyExc_TypeError,
                          "%U argument after * must be an iterable, not %.200s",
                          funcstr, Py_TYPE(args)->tp_name);
            Py_DECREF(funcstr);
        }
        return -1;
    }
    return 0;
}

static void
format_kwargs_error(PyThreadState *tstate, PyObject *func, PyObject *kwargs)
{
    /* _PyDict_MergeEx raises attribute
     * error (percolated from an attempt
     * to get 'keys' attribute) instead of
     * a type error if its second argument
     * is not a mapping.
     */
    if (_PyErr_ExceptionMatches(tstate, PyExc_AttributeError)) {
        _PyErr_Clear(tstate);
        PyObject *funcstr = _PyObject_FunctionStr(func);
        if (funcstr != NULL) {
            _PyErr_Format(
                tstate, PyExc_TypeError,
                "%U argument after ** must be a mapping, not %.200s",
                funcstr, Py_TYPE(kwargs)->tp_name);
            Py_DECREF(funcstr);
        }
    }
    else if (_PyErr_ExceptionMatches(tstate, PyExc_KeyError)) {
        PyObject *exc = _PyErr_GetRaisedException(tstate);
        PyObject *args = ((PyBaseExceptionObject *)exc)->args;
        if (exc && PyTuple_Check(args) && PyTuple_GET_SIZE(args) == 1) {
            _PyErr_Clear(tstate);
            PyObject *funcstr = _PyObject_FunctionStr(func);
            if (funcstr != NULL) {
                PyObject *key = PyTuple_GET_ITEM(args, 0);
                _PyErr_Format(
                    tstate, PyExc_TypeError,
                    "%U got multiple values for keyword argument '%S'",
                    funcstr, key);
                Py_DECREF(funcstr);
            }
            Py_XDECREF(exc);
        }
        else {
            _PyErr_SetRaisedException(tstate, exc);
        }
    }
}

static void
format_exc_check_arg(PyThreadState *tstate, PyObject *exc,
                     const char *format_str, PyObject *obj)
{
    const char *obj_str;

    if (!obj)
        return;

    obj_str = PyUnicode_AsUTF8(obj);
    if (!obj_str)
        return;

    _PyErr_Format(tstate, exc, format_str, obj_str);

    if (exc == PyExc_NameError) {
        // Include the name in the NameError exceptions to offer suggestions later.
        PyObject *exc = PyErr_GetRaisedException();
        if (PyErr_GivenExceptionMatches(exc, PyExc_NameError)) {
            if (((PyNameErrorObject*)exc)->name == NULL) {
                // We do not care if this fails because we are going to restore the
                // NameError anyway.
                (void)PyObject_SetAttr(exc, &_Py_ID(name), obj);
            }
        }
        PyErr_SetRaisedException(exc);
    }
}

static void
format_exc_unbound(PyThreadState *tstate, PyCodeObject *co, int oparg)
{
    PyObject *name;
    /* Don't stomp existing exception */
    if (_PyErr_Occurred(tstate))
        return;
    name = PyTuple_GET_ITEM(co->co_localsplusnames, oparg);
    if (oparg < PyCode_GetFirstFree(co)) {
        format_exc_check_arg(tstate, PyExc_UnboundLocalError,
                             UNBOUNDLOCAL_ERROR_MSG, name);
    } else {
        format_exc_check_arg(tstate, PyExc_NameError,
                             UNBOUNDFREE_ERROR_MSG, name);
    }
}

static void
format_awaitable_error(PyThreadState *tstate, PyTypeObject *type, int oparg)
{
    if (type->tp_as_async == NULL || type->tp_as_async->am_await == NULL) {
        if (oparg == 1) {
            _PyErr_Format(tstate, PyExc_TypeError,
                          "'async with' received an object from __aenter__ "
                          "that does not implement __await__: %.100s",
                          type->tp_name);
        }
        else if (oparg == 2) {
            _PyErr_Format(tstate, PyExc_TypeError,
                          "'async with' received an object from __aexit__ "
                          "that does not implement __await__: %.100s",
                          type->tp_name);
        }
    }
}


Py_ssize_t
PyUnstable_Eval_RequestCodeExtraIndex(freefunc free)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    Py_ssize_t new_index;

    if (interp->co_extra_user_count == MAX_CO_EXTRA_USERS - 1) {
        return -1;
    }
    new_index = interp->co_extra_user_count++;
    interp->co_extra_freefuncs[new_index] = free;
    return new_index;
}

/* Implement Py_EnterRecursiveCall() and Py_LeaveRecursiveCall() as functions
   for the limited API. */

int Py_EnterRecursiveCall(const char *where)
{
    return _Py_EnterRecursiveCall(where);
}

void Py_LeaveRecursiveCall(void)
{
    _Py_LeaveRecursiveCall();
}
