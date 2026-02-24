#define _PY_INTERPRETER

#include "Python.h"
#include "pycore_abstract.h"      // _PyIndex_Check()
#include "pycore_audit.h"         // _PySys_Audit()
#include "pycore_backoff.h"
#include "pycore_call.h"          // _PyObject_CallNoArgs()
#include "pycore_cell.h"          // PyCell_GetRef()
#include "pycore_ceval.h"         // SPECIAL___ENTER__
#include "pycore_code.h"
#include "pycore_dict.h"
#include "pycore_emscripten_signal.h"  // _Py_CHECK_EMSCRIPTEN_SIGNALS
#include "pycore_floatobject.h"   // _PyFloat_ExactDealloc()
#include "pycore_frame.h"
#include "pycore_function.h"
#include "pycore_genobject.h"     // _PyCoro_GetAwaitableIter()
#include "pycore_import.h"        // _PyImport_IsDefaultImportFunc()
#include "pycore_instruments.h"
#include "pycore_interpframe.h"   // _PyFrame_SetStackPointer()
#include "pycore_interpolation.h" // _PyInterpolation_Build()
#include "pycore_intrinsics.h"
#include "pycore_jit.h"
#include "pycore_lazyimportobject.h"
#include "pycore_list.h"          // _PyList_GetItemRef()
#include "pycore_long.h"          // _PyLong_GetZero()
#include "pycore_moduleobject.h"  // PyModuleObject
#include "pycore_object.h"        // _PyObject_GC_TRACK()
#include "pycore_opcode_metadata.h" // EXTRA_CASES
#include "pycore_opcode_utils.h"  // MAKE_FUNCTION_*
#include "pycore_optimizer.h"     // _PyUOpExecutor_Type
#include "pycore_pyatomic_ft_wrappers.h" // FT_ATOMIC_*
#include "pycore_pyerrors.h"      // _PyErr_GetRaisedException()
#include "pycore_pystate.h"       // _PyInterpreterState_GET()
#include "pycore_range.h"         // _PyRangeIterObject
#include "pycore_setobject.h"     // _PySet_Update()
#include "pycore_sliceobject.h"   // _PyBuildSlice_ConsumeRefs
#include "pycore_sysmodule.h"     // _PySys_GetOptionalAttrString()
#include "pycore_template.h"      // _PyTemplate_Build()
#include "pycore_traceback.h"     // _PyTraceBack_FromFrame
#include "pycore_tuple.h"         // _PyTuple_ITEMS()
#include "pycore_uop_ids.h"       // Uops

#include "dictobject.h"
#include "frameobject.h"          // _PyInterpreterFrame_GetLine
#include "opcode.h"
#include "pydtrace.h"
#include "setobject.h"
#include "pycore_stackref.h"

#include <stdbool.h>              // bool

#if !defined(Py_BUILD_CORE)
#  error "ceval.c must be build with Py_BUILD_CORE define for best performance"
#endif

#if !defined(Py_DEBUG) && !defined(Py_TRACE_REFS)
// GH-89279: The MSVC compiler does not inline these static inline functions
// in PGO build in _PyEval_EvalFrameDefault(), because this function is over
// the limit of PGO, and that limit cannot be configured.
// Define them as macros to make sure that they are always inlined by the
// preprocessor.

#undef Py_IS_TYPE
#define Py_IS_TYPE(ob, type) \
    (_PyObject_CAST(ob)->ob_type == (type))

#undef Py_XDECREF
#define Py_XDECREF(arg) \
    do { \
        PyObject *xop = _PyObject_CAST(arg); \
        if (xop != NULL) { \
            Py_DECREF(xop); \
        } \
    } while (0)

#ifndef Py_GIL_DISABLED

#undef Py_DECREF
#define Py_DECREF(arg) \
    do { \
        PyObject *op = _PyObject_CAST(arg); \
        if (_Py_IsImmortal(op)) { \
            _Py_DECREF_IMMORTAL_STAT_INC(); \
            break; \
        } \
        _Py_DECREF_STAT_INC(); \
        if (--op->ob_refcnt == 0) { \
            _PyReftracerTrack(op, PyRefTracer_DESTROY); \
            destructor dealloc = Py_TYPE(op)->tp_dealloc; \
            (*dealloc)(op); \
        } \
    } while (0)

#undef _Py_DECREF_SPECIALIZED
#define _Py_DECREF_SPECIALIZED(arg, dealloc) \
    do { \
        PyObject *op = _PyObject_CAST(arg); \
        if (_Py_IsImmortal(op)) { \
            _Py_DECREF_IMMORTAL_STAT_INC(); \
            break; \
        } \
        _Py_DECREF_STAT_INC(); \
        if (--op->ob_refcnt == 0) { \
            _PyReftracerTrack(op, PyRefTracer_DESTROY); \
            destructor d = (destructor)(dealloc); \
            d(op); \
        } \
    } while (0)

#else // Py_GIL_DISABLED

#undef Py_DECREF
#define Py_DECREF(arg) \
    do { \
        PyObject *op = _PyObject_CAST(arg); \
        uint32_t local = _Py_atomic_load_uint32_relaxed(&op->ob_ref_local); \
        if (local == _Py_IMMORTAL_REFCNT_LOCAL) { \
            _Py_DECREF_IMMORTAL_STAT_INC(); \
            break; \
        } \
        _Py_DECREF_STAT_INC(); \
        if (_Py_IsOwnedByCurrentThread(op)) { \
            local--; \
            _Py_atomic_store_uint32_relaxed(&op->ob_ref_local, local); \
            if (local == 0) { \
                _Py_MergeZeroLocalRefcount(op); \
            } \
        } \
        else { \
            _Py_DecRefShared(op); \
        } \
    } while (0)

#undef _Py_DECREF_SPECIALIZED
#define _Py_DECREF_SPECIALIZED(arg, dealloc) Py_DECREF(arg)

#endif
#endif

static void
check_invalid_reentrancy(void)
{
#if defined(Py_DEBUG) && defined(Py_GIL_DISABLED)
    // In the free-threaded build, the interpreter must not be re-entered if
    // the world-is-stopped.  If so, that's a bug somewhere (quite likely in
    // the painfully complex typeobject code).
    PyInterpreterState *interp = _PyInterpreterState_GET();
    assert(!interp->stoptheworld.world_stopped);
#endif
}


#ifdef Py_DEBUG
static void
dump_item(_PyStackRef item)
{
    if (PyStackRef_IsNull(item)) {
        printf("<NULL>");
        return;
    }
    if (PyStackRef_IsMalformed(item)) {
        printf("<INVALID>");
        return;
    }
    if (PyStackRef_IsTaggedInt(item)) {
        printf("%" PRId64, (int64_t)PyStackRef_UntagInt(item));
        return;
    }
    PyObject *obj = PyStackRef_AsPyObjectBorrow(item);
    if (obj == NULL) {
        printf("<nil>");
        return;
    }
    // Don't call __repr__(), it might recurse into the interpreter.
    printf("<%s at %p>", Py_TYPE(obj)->tp_name, (void *)obj);
}

static void
dump_stack(_PyInterpreterFrame *frame, _PyStackRef *stack_pointer)
{
    _PyFrame_SetStackPointer(frame, stack_pointer);
    _PyStackRef *locals_base = _PyFrame_GetLocalsArray(frame);
    _PyStackRef *stack_base = _PyFrame_Stackbase(frame);
    PyObject *exc = PyErr_GetRaisedException();
    printf("    locals=[");
    for (_PyStackRef *ptr = locals_base; ptr < stack_base; ptr++) {
        if (ptr != locals_base) {
            printf(", ");
        }
        dump_item(*ptr);
    }
    printf("]\n");
    if (stack_pointer < stack_base) {
        printf("    stack=%d\n", (int)(stack_pointer-stack_base));
    }
    else {
        printf("    stack=[");
        for (_PyStackRef *ptr = stack_base; ptr < stack_pointer; ptr++) {
            if (ptr != stack_base) {
                printf(", ");
            }
            dump_item(*ptr);
        }
        printf("]\n");
    }
    fflush(stdout);
    PyErr_SetRaisedException(exc);
    _PyFrame_GetStackPointer(frame);
}

#if defined(_Py_TIER2) && !defined(_Py_JIT) && defined(Py_DEBUG)
static void
dump_cache_item(_PyStackRef cache, int position, int depth)
{
    if (position < depth) {
        dump_item(cache);
    }
    else {
        printf("---");
    }
}
#endif

static void
lltrace_instruction(_PyInterpreterFrame *frame,
                    _PyStackRef *stack_pointer,
                    _Py_CODEUNIT *next_instr,
                    int opcode,
                    int oparg)
{
    int offset = 0;
    if (frame->owner < FRAME_OWNED_BY_INTERPRETER) {
        dump_stack(frame, stack_pointer);
        offset = (int)(next_instr - _PyFrame_GetBytecode(frame));
    }
    const char *opname = _PyOpcode_OpName[opcode];
    assert(opname != NULL);
    if (OPCODE_HAS_ARG((int)_PyOpcode_Deopt[opcode])) {
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
    PyObject *fobj = PyStackRef_AsPyObjectBorrow(frame->f_funcobj);
    if (!PyStackRef_CodeCheck(frame->f_executable) ||
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

static int
maybe_lltrace_resume_frame(_PyInterpreterFrame *frame, PyObject *globals)
{
    if (globals == NULL) {
        return 0;
    }
    if (frame->owner >= FRAME_OWNED_BY_INTERPRETER) {
        return 0;
    }
    int r = PyDict_Contains(globals, &_Py_ID(__lltrace__));
    if (r < 0) {
        PyErr_Clear();
        return 0;
    }
    int lltrace = r * 5;  // Levels 1-4 only trace uops
    if (!lltrace) {
        // Can also be controlled by environment variable
        char *python_lltrace = Py_GETENV("PYTHON_LLTRACE");
        if (python_lltrace != NULL && *python_lltrace >= '0') {
            lltrace = *python_lltrace - '0';  // TODO: Parse an int and all that
        }
    }
    if (lltrace >= 5) {
        lltrace_resume_frame(frame);
    }
    return lltrace;
}

#endif

static void monitor_reraise(PyThreadState *tstate,
                 _PyInterpreterFrame *frame,
                 _Py_CODEUNIT *instr);
static int monitor_stop_iteration(PyThreadState *tstate,
                 _PyInterpreterFrame *frame,
                 _Py_CODEUNIT *instr,
                 PyObject *value);
static void monitor_unwind(PyThreadState *tstate,
                 _PyInterpreterFrame *frame,
                 _Py_CODEUNIT *instr);
static int monitor_handled(PyThreadState *tstate,
                 _PyInterpreterFrame *frame,
                 _Py_CODEUNIT *instr, PyObject *exc);
static void monitor_throw(PyThreadState *tstate,
                 _PyInterpreterFrame *frame,
                 _Py_CODEUNIT *instr);

static int get_exception_handler(PyCodeObject *, int, int*, int*, int*);

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

typedef struct {
    _PyInterpreterFrame frame;
    _PyStackRef stack[1];
} _PyEntryFrame;

static int
do_monitor_exc(PyThreadState *tstate, _PyInterpreterFrame *frame,
               _Py_CODEUNIT *instr, int event)
{
    assert(event < _PY_MONITORING_UNGROUPED_EVENTS);
    if (_PyFrame_GetCode(frame)->co_flags & CO_NO_MONITORING_EVENTS) {
        return 0;
    }
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
    _PyCoMonitoringData *data = _PyFrame_GetCode(frame)->_co_monitoring;
    if (data) {
        return data->active_monitors.tools[event] == 0;
    }
    else {
        return no_tools_for_global_event(tstate, event);
    }
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
                       _Py_CODEUNIT *instr, PyObject *value)
{
    if (no_tools_for_local_event(tstate, frame, PY_MONITORING_EVENT_STOP_ITERATION)) {
        return 0;
    }
    assert(!PyErr_Occurred());
    PyErr_SetObject(PyExc_StopIteration, value);
    int res = do_monitor_exc(tstate, frame, instr, PY_MONITORING_EVENT_STOP_ITERATION);
    if (res < 0) {
        return res;
    }
    PyErr_SetRaisedException(NULL);
    return 0;
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

static Py_NO_INLINE int
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


#ifdef Py_DEBUG
#define ASSERT_WITHIN_STACK_BOUNDS(F, L) _Py_assert_within_stack_bounds(frame, stack_pointer, (F), (L))
#else
#define ASSERT_WITHIN_STACK_BOUNDS(F, L) (void)0
#endif

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
            if (!PyExceptionInstance_Check(fixed_cause)) {
                _PyErr_Format(tstate, PyExc_TypeError,
                              "calling %R should have returned an instance of "
                              "BaseException, not %R",
                              cause, Py_TYPE(fixed_cause));
                Py_DECREF(fixed_cause);
                goto raise_error;
            }
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

/* Disable unused label warnings.  They are handy for debugging, even
   if computed gotos aren't used. */

/* TBD - what about other compilers? */
#if defined(__GNUC__) || defined(__clang__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wunused-label"
#elif defined(_MSC_VER) /* MS_WINDOWS */
#  pragma warning(push)
#  pragma warning(disable:4102)
#endif
