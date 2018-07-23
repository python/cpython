
/* Execute compiled code */

/* XXX TO DO:
   XXX speed up searching for keywords by using a dictionary
   XXX document it!
   */

/* enable more aggressive intra-module optimizations, where available */
#define PY_LOCAL_AGGRESSIVE

#include "Python.h"
#include "internal/pystate.h"

#include "code.h"
#include "dictobject.h"
#include "frameobject.h"
#include "opcode.h"
#include "pydtrace.h"
#include "setobject.h"
#include "structmember.h"

#include <ctype.h>

#ifdef Py_DEBUG
/* For debugging the interpreter: */
#define LLTRACE  1      /* Low-level trace feature */
#define CHECKEXC 1      /* Double-check exception checking */
#endif

/* Private API for the LOAD_METHOD opcode. */
extern int _PyObject_GetMethod(PyObject *, PyObject *, PyObject **);

typedef PyObject *(*callproc)(PyObject *, PyObject *, PyObject *);

/* Forward declarations */
Py_LOCAL_INLINE(PyObject *) call_function(PyObject ***, Py_ssize_t,
                                          PyObject *);
static PyObject * do_call_core(PyObject *, PyObject *, PyObject *);

#ifdef LLTRACE
static int lltrace;
static int prtrace(PyObject *, const char *);
#endif
static int call_trace(Py_tracefunc, PyObject *,
                      PyThreadState *, PyFrameObject *,
                      int, PyObject *);
static int call_trace_protected(Py_tracefunc, PyObject *,
                                PyThreadState *, PyFrameObject *,
                                int, PyObject *);
static void call_exc_trace(Py_tracefunc, PyObject *,
                           PyThreadState *, PyFrameObject *);
static int maybe_call_line_trace(Py_tracefunc, PyObject *,
                                 PyThreadState *, PyFrameObject *,
                                 int *, int *, int *);
static void maybe_dtrace_line(PyFrameObject *, int *, int *, int *);
static void dtrace_function_entry(PyFrameObject *);
static void dtrace_function_return(PyFrameObject *);

static PyObject * cmp_outcome(int, PyObject *, PyObject *);
static PyObject * import_name(PyFrameObject *, PyObject *, PyObject *,
                              PyObject *);
static PyObject * import_from(PyObject *, PyObject *);
static int import_all_from(PyObject *, PyObject *);
static void format_exc_check_arg(PyObject *, const char *, PyObject *);
static void format_exc_unbound(PyCodeObject *co, int oparg);
static PyObject * unicode_concatenate(PyObject *, PyObject *,
                                      PyFrameObject *, const _Py_CODEUNIT *);
static PyObject * special_lookup(PyObject *, _Py_Identifier *);
static int check_args_iterable(PyObject *func, PyObject *vararg);
static void format_kwargs_mapping_error(PyObject *func, PyObject *kwargs);
static void format_awaitable_error(PyTypeObject *, int);

#define NAME_ERROR_MSG \
    "name '%.200s' is not defined"
#define UNBOUNDLOCAL_ERROR_MSG \
    "local variable '%.200s' referenced before assignment"
#define UNBOUNDFREE_ERROR_MSG \
    "free variable '%.200s' referenced before assignment" \
    " in enclosing scope"

/* Dynamic execution profile */
#ifdef DYNAMIC_EXECUTION_PROFILE
#ifdef DXPAIRS
static long dxpairs[257][256];
#define dxp dxpairs[256]
#else
static long dxp[256];
#endif
#endif

#define GIL_REQUEST _Py_atomic_load_relaxed(&_PyRuntime.ceval.gil_drop_request)

/* This can set eval_breaker to 0 even though gil_drop_request became
   1.  We believe this is all right because the eval loop will release
   the GIL eventually anyway. */
#define COMPUTE_EVAL_BREAKER() \
    _Py_atomic_store_relaxed( \
        &_PyRuntime.ceval.eval_breaker, \
        GIL_REQUEST | \
        _Py_atomic_load_relaxed(&_PyRuntime.ceval.pending.calls_to_do) | \
        _PyRuntime.ceval.pending.async_exc)

#define SET_GIL_DROP_REQUEST() \
    do { \
        _Py_atomic_store_relaxed(&_PyRuntime.ceval.gil_drop_request, 1); \
        _Py_atomic_store_relaxed(&_PyRuntime.ceval.eval_breaker, 1); \
    } while (0)

#define RESET_GIL_DROP_REQUEST() \
    do { \
        _Py_atomic_store_relaxed(&_PyRuntime.ceval.gil_drop_request, 0); \
        COMPUTE_EVAL_BREAKER(); \
    } while (0)

/* Pending calls are only modified under pending_lock */
#define SIGNAL_PENDING_CALLS() \
    do { \
        _Py_atomic_store_relaxed(&_PyRuntime.ceval.pending.calls_to_do, 1); \
        _Py_atomic_store_relaxed(&_PyRuntime.ceval.eval_breaker, 1); \
    } while (0)

#define UNSIGNAL_PENDING_CALLS() \
    do { \
        _Py_atomic_store_relaxed(&_PyRuntime.ceval.pending.calls_to_do, 0); \
        COMPUTE_EVAL_BREAKER(); \
    } while (0)

#define SIGNAL_ASYNC_EXC() \
    do { \
        _PyRuntime.ceval.pending.async_exc = 1; \
        _Py_atomic_store_relaxed(&_PyRuntime.ceval.eval_breaker, 1); \
    } while (0)

#define UNSIGNAL_ASYNC_EXC() \
    do { \
        _PyRuntime.ceval.pending.async_exc = 0; \
        COMPUTE_EVAL_BREAKER(); \
    } while (0)


#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#include "pythread.h"
#include "ceval_gil.h"

int
PyEval_ThreadsInitialized(void)
{
    return gil_created();
}

void
PyEval_InitThreads(void)
{
    if (gil_created())
        return;
    create_gil();
    take_gil(PyThreadState_GET());
    _PyRuntime.ceval.pending.main_thread = PyThread_get_thread_ident();
    if (!_PyRuntime.ceval.pending.lock)
        _PyRuntime.ceval.pending.lock = PyThread_allocate_lock();
}

void
_PyEval_FiniThreads(void)
{
    if (!gil_created())
        return;
    destroy_gil();
    assert(!gil_created());
}

void
PyEval_AcquireLock(void)
{
    PyThreadState *tstate = PyThreadState_GET();
    if (tstate == NULL)
        Py_FatalError("PyEval_AcquireLock: current thread state is NULL");
    take_gil(tstate);
}

void
PyEval_ReleaseLock(void)
{
    /* This function must succeed when the current thread state is NULL.
       We therefore avoid PyThreadState_GET() which dumps a fatal error
       in debug mode.
    */
    drop_gil((PyThreadState*)_Py_atomic_load_relaxed(
        &_PyThreadState_Current));
}

void
PyEval_AcquireThread(PyThreadState *tstate)
{
    if (tstate == NULL)
        Py_FatalError("PyEval_AcquireThread: NULL new thread state");
    /* Check someone has called PyEval_InitThreads() to create the lock */
    assert(gil_created());
    take_gil(tstate);
    if (PyThreadState_Swap(tstate) != NULL)
        Py_FatalError(
            "PyEval_AcquireThread: non-NULL old thread state");
}

void
PyEval_ReleaseThread(PyThreadState *tstate)
{
    if (tstate == NULL)
        Py_FatalError("PyEval_ReleaseThread: NULL thread state");
    if (PyThreadState_Swap(NULL) != tstate)
        Py_FatalError("PyEval_ReleaseThread: wrong thread state");
    drop_gil(tstate);
}

/* This function is called from PyOS_AfterFork_Child to destroy all threads
 * which are not running in the child process, and clear internal locks
 * which might be held by those threads.
 */

void
PyEval_ReInitThreads(void)
{
    PyThreadState *current_tstate = PyThreadState_GET();

    if (!gil_created())
        return;
    recreate_gil();
    _PyRuntime.ceval.pending.lock = PyThread_allocate_lock();
    take_gil(current_tstate);
    _PyRuntime.ceval.pending.main_thread = PyThread_get_thread_ident();

    /* Destroy all threads except the current one */
    _PyThreadState_DeleteExcept(current_tstate);
}

/* This function is used to signal that async exceptions are waiting to be
   raised, therefore it is also useful in non-threaded builds. */

void
_PyEval_SignalAsyncExc(void)
{
    SIGNAL_ASYNC_EXC();
}

/* Functions save_thread and restore_thread are always defined so
   dynamically loaded modules needn't be compiled separately for use
   with and without threads: */

PyThreadState *
PyEval_SaveThread(void)
{
    PyThreadState *tstate = PyThreadState_Swap(NULL);
    if (tstate == NULL)
        Py_FatalError("PyEval_SaveThread: NULL tstate");
    assert(gil_created());
    drop_gil(tstate);
    return tstate;
}

void
PyEval_RestoreThread(PyThreadState *tstate)
{
    if (tstate == NULL)
        Py_FatalError("PyEval_RestoreThread: NULL tstate");
    assert(gil_created());

    int err = errno;
    take_gil(tstate);
    /* _Py_Finalizing is protected by the GIL */
    if (_Py_IsFinalizing() && !_Py_CURRENTLY_FINALIZING(tstate)) {
        drop_gil(tstate);
        PyThread_exit_thread();
        Py_UNREACHABLE();
    }
    errno = err;

    PyThreadState_Swap(tstate);
}


/* Mechanism whereby asynchronously executing callbacks (e.g. UNIX
   signal handlers or Mac I/O completion routines) can schedule calls
   to a function to be called synchronously.
   The synchronous function is called with one void* argument.
   It should return 0 for success or -1 for failure -- failure should
   be accompanied by an exception.

   If registry succeeds, the registry function returns 0; if it fails
   (e.g. due to too many pending calls) it returns -1 (without setting
   an exception condition).

   Note that because registry may occur from within signal handlers,
   or other asynchronous events, calling malloc() is unsafe!

   Any thread can schedule pending calls, but only the main thread
   will execute them.
   There is no facility to schedule calls to a particular thread, but
   that should be easy to change, should that ever be required.  In
   that case, the static variables here should go into the python
   threadstate.
*/

void
_PyEval_SignalReceived(void)
{
    /* bpo-30703: Function called when the C signal handler of Python gets a
       signal. We cannot queue a callback using Py_AddPendingCall() since
       that function is not async-signal-safe. */
    SIGNAL_PENDING_CALLS();
}

/* This implementation is thread-safe.  It allows
   scheduling to be made from any thread, and even from an executing
   callback.
 */

int
Py_AddPendingCall(int (*func)(void *), void *arg)
{
    int i, j, result=0;
    PyThread_type_lock lock = _PyRuntime.ceval.pending.lock;

    /* try a few times for the lock.  Since this mechanism is used
     * for signal handling (on the main thread), there is a (slim)
     * chance that a signal is delivered on the same thread while we
     * hold the lock during the Py_MakePendingCalls() function.
     * This avoids a deadlock in that case.
     * Note that signals can be delivered on any thread.  In particular,
     * on Windows, a SIGINT is delivered on a system-created worker
     * thread.
     * We also check for lock being NULL, in the unlikely case that
     * this function is called before any bytecode evaluation takes place.
     */
    if (lock != NULL) {
        for (i = 0; i<100; i++) {
            if (PyThread_acquire_lock(lock, NOWAIT_LOCK))
                break;
        }
        if (i == 100)
            return -1;
    }

    i = _PyRuntime.ceval.pending.last;
    j = (i + 1) % NPENDINGCALLS;
    if (j == _PyRuntime.ceval.pending.first) {
        result = -1; /* Queue full */
    } else {
        _PyRuntime.ceval.pending.calls[i].func = func;
        _PyRuntime.ceval.pending.calls[i].arg = arg;
        _PyRuntime.ceval.pending.last = j;
    }
    /* signal main loop */
    SIGNAL_PENDING_CALLS();
    if (lock != NULL)
        PyThread_release_lock(lock);
    return result;
}

int
Py_MakePendingCalls(void)
{
    static int busy = 0;
    int i;
    int r = 0;

    assert(PyGILState_Check());

    if (!_PyRuntime.ceval.pending.lock) {
        /* initial allocation of the lock */
        _PyRuntime.ceval.pending.lock = PyThread_allocate_lock();
        if (_PyRuntime.ceval.pending.lock == NULL)
            return -1;
    }

    /* only service pending calls on main thread */
    if (_PyRuntime.ceval.pending.main_thread &&
        PyThread_get_thread_ident() != _PyRuntime.ceval.pending.main_thread)
    {
        return 0;
    }
    /* don't perform recursive pending calls */
    if (busy)
        return 0;
    busy = 1;
    /* unsignal before starting to call callbacks, so that any callback
       added in-between re-signals */
    UNSIGNAL_PENDING_CALLS();

    /* Python signal handler doesn't really queue a callback: it only signals
       that a signal was received, see _PyEval_SignalReceived(). */
    if (PyErr_CheckSignals() < 0) {
        goto error;
    }

    /* perform a bounded number of calls, in case of recursion */
    for (i=0; i<NPENDINGCALLS; i++) {
        int j;
        int (*func)(void *);
        void *arg = NULL;

        /* pop one item off the queue while holding the lock */
        PyThread_acquire_lock(_PyRuntime.ceval.pending.lock, WAIT_LOCK);
        j = _PyRuntime.ceval.pending.first;
        if (j == _PyRuntime.ceval.pending.last) {
            func = NULL; /* Queue empty */
        } else {
            func = _PyRuntime.ceval.pending.calls[j].func;
            arg = _PyRuntime.ceval.pending.calls[j].arg;
            _PyRuntime.ceval.pending.first = (j + 1) % NPENDINGCALLS;
        }
        PyThread_release_lock(_PyRuntime.ceval.pending.lock);
        /* having released the lock, perform the callback */
        if (func == NULL)
            break;
        r = func(arg);
        if (r) {
            goto error;
        }
    }

    busy = 0;
    return r;

error:
    busy = 0;
    SIGNAL_PENDING_CALLS(); /* We're not done yet */
    return -1;
}

/* The interpreter's recursion limit */

#ifndef Py_DEFAULT_RECURSION_LIMIT
#define Py_DEFAULT_RECURSION_LIMIT 1000
#endif

int _Py_CheckRecursionLimit = Py_DEFAULT_RECURSION_LIMIT;

void
_PyEval_Initialize(struct _ceval_runtime_state *state)
{
    state->recursion_limit = Py_DEFAULT_RECURSION_LIMIT;
    _Py_CheckRecursionLimit = Py_DEFAULT_RECURSION_LIMIT;
    _gil_initialize(&state->gil);
}

int
Py_GetRecursionLimit(void)
{
    return _PyRuntime.ceval.recursion_limit;
}

void
Py_SetRecursionLimit(int new_limit)
{
    _PyRuntime.ceval.recursion_limit = new_limit;
    _Py_CheckRecursionLimit = _PyRuntime.ceval.recursion_limit;
}

/* the macro Py_EnterRecursiveCall() only calls _Py_CheckRecursiveCall()
   if the recursion_depth reaches _Py_CheckRecursionLimit.
   If USE_STACKCHECK, the macro decrements _Py_CheckRecursionLimit
   to guarantee that _Py_CheckRecursiveCall() is regularly called.
   Without USE_STACKCHECK, there is no need for this. */
int
_Py_CheckRecursiveCall(const char *where)
{
    PyThreadState *tstate = PyThreadState_GET();
    int recursion_limit = _PyRuntime.ceval.recursion_limit;

#ifdef USE_STACKCHECK
    tstate->stackcheck_counter = 0;
    if (PyOS_CheckStack()) {
        --tstate->recursion_depth;
        PyErr_SetString(PyExc_MemoryError, "Stack overflow");
        return -1;
    }
    /* Needed for ABI backwards-compatibility (see bpo-31857) */
    _Py_CheckRecursionLimit = recursion_limit;
#endif
    if (tstate->recursion_critical)
        /* Somebody asked that we don't check for recursion. */
        return 0;
    if (tstate->overflowed) {
        if (tstate->recursion_depth > recursion_limit + 50) {
            /* Overflowing while handling an overflow. Give up. */
            Py_FatalError("Cannot recover from stack overflow.");
        }
        return 0;
    }
    if (tstate->recursion_depth > recursion_limit) {
        --tstate->recursion_depth;
        tstate->overflowed = 1;
        PyErr_Format(PyExc_RecursionError,
                     "maximum recursion depth exceeded%s",
                     where);
        return -1;
    }
    return 0;
}

/* Status code for main loop (reason for stack unwind) */
enum why_code {
        WHY_NOT =       0x0001, /* No error */
        WHY_EXCEPTION = 0x0002, /* Exception occurred */
        WHY_RETURN =    0x0008, /* 'return' statement */
        WHY_BREAK =     0x0010, /* 'break' statement */
        WHY_CONTINUE =  0x0020, /* 'continue' statement */
        WHY_YIELD =     0x0040, /* 'yield' operator */
        WHY_SILENCED =  0x0080  /* Exception silenced by 'with' */
};

static int do_raise(PyObject *, PyObject *);
static int unpack_iterable(PyObject *, int, int, PyObject **);

#define _Py_TracingPossible _PyRuntime.ceval.tracing_possible


PyObject *
PyEval_EvalCode(PyObject *co, PyObject *globals, PyObject *locals)
{
    return PyEval_EvalCodeEx(co,
                      globals, locals,
                      (PyObject **)NULL, 0,
                      (PyObject **)NULL, 0,
                      (PyObject **)NULL, 0,
                      NULL, NULL);
}


/* Interpreter main loop */

PyObject *
PyEval_EvalFrame(PyFrameObject *f) {
    /* This is for backward compatibility with extension modules that
       used this API; core interpreter code should call
       PyEval_EvalFrameEx() */
    return PyEval_EvalFrameEx(f, 0);
}

PyObject *
PyEval_EvalFrameEx(PyFrameObject *f, int throwflag)
{
    PyThreadState *tstate = PyThreadState_GET();
    return tstate->interp->eval_frame(f, throwflag);
}

PyObject* _Py_HOT_FUNCTION
_PyEval_EvalFrameDefault(PyFrameObject *f, int throwflag)
{
#ifdef DXPAIRS
    int lastopcode = 0;
#endif
    PyObject **stack_pointer;  /* Next free slot in value stack */
    const _Py_CODEUNIT *next_instr;
    int opcode;        /* Current opcode */
    int oparg;         /* Current opcode argument, if any */
    enum why_code why; /* Reason for block stack unwind */
    PyObject **fastlocals, **freevars;
    PyObject *retval = NULL;            /* Return value */
    PyThreadState *tstate = PyThreadState_GET();
    PyCodeObject *co;

    /* when tracing we set things up so that

           not (instr_lb <= current_bytecode_offset < instr_ub)

       is true when the line being executed has changed.  The
       initial values are such as to make this false the first
       time it is tested. */
    int instr_ub = -1, instr_lb = 0, instr_prev = -1;

    const _Py_CODEUNIT *first_instr;
    PyObject *names;
    PyObject *consts;

#ifdef LLTRACE
    _Py_IDENTIFIER(__ltrace__);
#endif

/* Computed GOTOs, or
       the-optimization-commonly-but-improperly-known-as-"threaded code"
   using gcc's labels-as-values extension
   (http://gcc.gnu.org/onlinedocs/gcc/Labels-as-Values.html).

   The traditional bytecode evaluation loop uses a "switch" statement, which
   decent compilers will optimize as a single indirect branch instruction
   combined with a lookup table of jump addresses. However, since the
   indirect jump instruction is shared by all opcodes, the CPU will have a
   hard time making the right prediction for where to jump next (actually,
   it will be always wrong except in the uncommon case of a sequence of
   several identical opcodes).

   "Threaded code" in contrast, uses an explicit jump table and an explicit
   indirect jump instruction at the end of each opcode. Since the jump
   instruction is at a different address for each opcode, the CPU will make a
   separate prediction for each of these instructions, which is equivalent to
   predicting the second opcode of each opcode pair. These predictions have
   a much better chance to turn out valid, especially in small bytecode loops.

   A mispredicted branch on a modern CPU flushes the whole pipeline and
   can cost several CPU cycles (depending on the pipeline depth),
   and potentially many more instructions (depending on the pipeline width).
   A correctly predicted branch, however, is nearly free.

   At the time of this writing, the "threaded code" version is up to 15-20%
   faster than the normal "switch" version, depending on the compiler and the
   CPU architecture.

   We disable the optimization if DYNAMIC_EXECUTION_PROFILE is defined,
   because it would render the measurements invalid.


   NOTE: care must be taken that the compiler doesn't try to "optimize" the
   indirect jumps by sharing them between all opcodes. Such optimizations
   can be disabled on gcc by using the -fno-gcse flag (or possibly
   -fno-crossjumping).
*/

#ifdef DYNAMIC_EXECUTION_PROFILE
#undef USE_COMPUTED_GOTOS
#define USE_COMPUTED_GOTOS 0
#endif

#ifdef HAVE_COMPUTED_GOTOS
    #ifndef USE_COMPUTED_GOTOS
    #define USE_COMPUTED_GOTOS 1
    #endif
#else
    #if defined(USE_COMPUTED_GOTOS) && USE_COMPUTED_GOTOS
    #error "Computed gotos are not supported on this compiler."
    #endif
    #undef USE_COMPUTED_GOTOS
    #define USE_COMPUTED_GOTOS 0
#endif

#if USE_COMPUTED_GOTOS
/* Import the static jump table */
#include "opcode_targets.h"

#define TARGET(op) \
    TARGET_##op: \
    case op:

#define DISPATCH() \
    { \
        if (!_Py_atomic_load_relaxed(&_PyRuntime.ceval.eval_breaker)) { \
                    FAST_DISPATCH(); \
        } \
        continue; \
    }

#ifdef LLTRACE
#define FAST_DISPATCH() \
    { \
        if (!lltrace && !_Py_TracingPossible && !PyDTrace_LINE_ENABLED()) { \
            f->f_lasti = INSTR_OFFSET(); \
            NEXTOPARG(); \
            goto *opcode_targets[opcode]; \
        } \
        goto fast_next_opcode; \
    }
#else
#define FAST_DISPATCH() \
    { \
        if (!_Py_TracingPossible && !PyDTrace_LINE_ENABLED()) { \
            f->f_lasti = INSTR_OFFSET(); \
            NEXTOPARG(); \
            goto *opcode_targets[opcode]; \
        } \
        goto fast_next_opcode; \
    }
#endif

#else
#define TARGET(op) \
    case op:

#define DISPATCH() continue
#define FAST_DISPATCH() goto fast_next_opcode
#endif


/* Tuple access macros */

#ifndef Py_DEBUG
#define GETITEM(v, i) PyTuple_GET_ITEM((PyTupleObject *)(v), (i))
#else
#define GETITEM(v, i) PyTuple_GetItem((v), (i))
#endif

/* Code access macros */

/* The integer overflow is checked by an assertion below. */
#define INSTR_OFFSET()  \
    (sizeof(_Py_CODEUNIT) * (int)(next_instr - first_instr))
#define NEXTOPARG()  do { \
        _Py_CODEUNIT word = *next_instr; \
        opcode = _Py_OPCODE(word); \
        oparg = _Py_OPARG(word); \
        next_instr++; \
    } while (0)
#define JUMPTO(x)       (next_instr = first_instr + (x) / sizeof(_Py_CODEUNIT))
#define JUMPBY(x)       (next_instr += (x) / sizeof(_Py_CODEUNIT))

/* OpCode prediction macros
    Some opcodes tend to come in pairs thus making it possible to
    predict the second code when the first is run.  For example,
    COMPARE_OP is often followed by POP_JUMP_IF_FALSE or POP_JUMP_IF_TRUE.

    Verifying the prediction costs a single high-speed test of a register
    variable against a constant.  If the pairing was good, then the
    processor's own internal branch predication has a high likelihood of
    success, resulting in a nearly zero-overhead transition to the
    next opcode.  A successful prediction saves a trip through the eval-loop
    including its unpredictable switch-case branch.  Combined with the
    processor's internal branch prediction, a successful PREDICT has the
    effect of making the two opcodes run as if they were a single new opcode
    with the bodies combined.

    If collecting opcode statistics, your choices are to either keep the
    predictions turned-on and interpret the results as if some opcodes
    had been combined or turn-off predictions so that the opcode frequency
    counter updates for both opcodes.

    Opcode prediction is disabled with threaded code, since the latter allows
    the CPU to record separate branch prediction information for each
    opcode.

*/

#if defined(DYNAMIC_EXECUTION_PROFILE) || USE_COMPUTED_GOTOS
#define PREDICT(op)             if (0) goto PRED_##op
#else
#define PREDICT(op) \
    do{ \
        _Py_CODEUNIT word = *next_instr; \
        opcode = _Py_OPCODE(word); \
        if (opcode == op){ \
            oparg = _Py_OPARG(word); \
            next_instr++; \
            goto PRED_##op; \
        } \
    } while(0)
#endif
#define PREDICTED(op)           PRED_##op:


/* Stack manipulation macros */

/* The stack can grow at most MAXINT deep, as co_nlocals and
   co_stacksize are ints. */
#define STACK_LEVEL()     ((int)(stack_pointer - f->f_valuestack))
#define EMPTY()           (STACK_LEVEL() == 0)
#define TOP()             (stack_pointer[-1])
#define SECOND()          (stack_pointer[-2])
#define THIRD()           (stack_pointer[-3])
#define FOURTH()          (stack_pointer[-4])
#define PEEK(n)           (stack_pointer[-(n)])
#define SET_TOP(v)        (stack_pointer[-1] = (v))
#define SET_SECOND(v)     (stack_pointer[-2] = (v))
#define SET_THIRD(v)      (stack_pointer[-3] = (v))
#define SET_FOURTH(v)     (stack_pointer[-4] = (v))
#define SET_VALUE(n, v)   (stack_pointer[-(n)] = (v))
#define BASIC_STACKADJ(n) (stack_pointer += n)
#define BASIC_PUSH(v)     (*stack_pointer++ = (v))
#define BASIC_POP()       (*--stack_pointer)

#ifdef LLTRACE
#define PUSH(v)         { (void)(BASIC_PUSH(v), \
                          lltrace && prtrace(TOP(), "push")); \
                          assert(STACK_LEVEL() <= co->co_stacksize); }
#define POP()           ((void)(lltrace && prtrace(TOP(), "pop")), \
                         BASIC_POP())
#define STACKADJ(n)     { (void)(BASIC_STACKADJ(n), \
                          lltrace && prtrace(TOP(), "stackadj")); \
                          assert(STACK_LEVEL() <= co->co_stacksize); }
#define EXT_POP(STACK_POINTER) ((void)(lltrace && \
                                prtrace((STACK_POINTER)[-1], "ext_pop")), \
                                *--(STACK_POINTER))
#else
#define PUSH(v)                BASIC_PUSH(v)
#define POP()                  BASIC_POP()
#define STACKADJ(n)            BASIC_STACKADJ(n)
#define EXT_POP(STACK_POINTER) (*--(STACK_POINTER))
#endif

/* Local variable macros */

#define GETLOCAL(i)     (fastlocals[i])

/* The SETLOCAL() macro must not DECREF the local variable in-place and
   then store the new value; it must copy the old value to a temporary
   value, then store the new value, and then DECREF the temporary value.
   This is because it is possible that during the DECREF the frame is
   accessed by other code (e.g. a __del__ method or gc.collect()) and the
   variable would be pointing to already-freed memory. */
#define SETLOCAL(i, value)      do { PyObject *tmp = GETLOCAL(i); \
                                     GETLOCAL(i) = value; \
                                     Py_XDECREF(tmp); } while (0)


#define UNWIND_BLOCK(b) \
    while (STACK_LEVEL() > (b)->b_level) { \
        PyObject *v = POP(); \
        Py_XDECREF(v); \
    }

#define UNWIND_EXCEPT_HANDLER(b) \
    do { \
        PyObject *type, *value, *traceback; \
        _PyErr_StackItem *exc_info; \
        assert(STACK_LEVEL() >= (b)->b_level + 3); \
        while (STACK_LEVEL() > (b)->b_level + 3) { \
            value = POP(); \
            Py_XDECREF(value); \
        } \
        exc_info = tstate->exc_info; \
        type = exc_info->exc_type; \
        value = exc_info->exc_value; \
        traceback = exc_info->exc_traceback; \
        exc_info->exc_type = POP(); \
        exc_info->exc_value = POP(); \
        exc_info->exc_traceback = POP(); \
        Py_XDECREF(type); \
        Py_XDECREF(value); \
        Py_XDECREF(traceback); \
    } while(0)

/* Start of code */

    /* push frame */
    if (Py_EnterRecursiveCall(""))
        return NULL;

    tstate->frame = f;

    if (tstate->use_tracing) {
        if (tstate->c_tracefunc != NULL) {
            /* tstate->c_tracefunc, if defined, is a
               function that will be called on *every* entry
               to a code block.  Its return value, if not
               None, is a function that will be called at
               the start of each executed line of code.
               (Actually, the function must return itself
               in order to continue tracing.)  The trace
               functions are called with three arguments:
               a pointer to the current frame, a string
               indicating why the function is called, and
               an argument which depends on the situation.
               The global trace function is also called
               whenever an exception is detected. */
            if (call_trace_protected(tstate->c_tracefunc,
                                     tstate->c_traceobj,
                                     tstate, f, PyTrace_CALL, Py_None)) {
                /* Trace function raised an error */
                goto exit_eval_frame;
            }
        }
        if (tstate->c_profilefunc != NULL) {
            /* Similar for c_profilefunc, except it needn't
               return itself and isn't called for "line" events */
            if (call_trace_protected(tstate->c_profilefunc,
                                     tstate->c_profileobj,
                                     tstate, f, PyTrace_CALL, Py_None)) {
                /* Profile function raised an error */
                goto exit_eval_frame;
            }
        }
    }

    if (PyDTrace_FUNCTION_ENTRY_ENABLED())
        dtrace_function_entry(f);

    co = f->f_code;
    names = co->co_names;
    consts = co->co_consts;
    fastlocals = f->f_localsplus;
    freevars = f->f_localsplus + co->co_nlocals;
    assert(PyBytes_Check(co->co_code));
    assert(PyBytes_GET_SIZE(co->co_code) <= INT_MAX);
    assert(PyBytes_GET_SIZE(co->co_code) % sizeof(_Py_CODEUNIT) == 0);
    assert(_Py_IS_ALIGNED(PyBytes_AS_STRING(co->co_code), sizeof(_Py_CODEUNIT)));
    first_instr = (_Py_CODEUNIT *) PyBytes_AS_STRING(co->co_code);
    /*
       f->f_lasti refers to the index of the last instruction,
       unless it's -1 in which case next_instr should be first_instr.

       YIELD_FROM sets f_lasti to itself, in order to repeatedly yield
       multiple values.

       When the PREDICT() macros are enabled, some opcode pairs follow in
       direct succession without updating f->f_lasti.  A successful
       prediction effectively links the two codes together as if they
       were a single new opcode; accordingly,f->f_lasti will point to
       the first code in the pair (for instance, GET_ITER followed by
       FOR_ITER is effectively a single opcode and f->f_lasti will point
       to the beginning of the combined pair.)
    */
    assert(f->f_lasti >= -1);
    next_instr = first_instr;
    if (f->f_lasti >= 0) {
        assert(f->f_lasti % sizeof(_Py_CODEUNIT) == 0);
        next_instr += f->f_lasti / sizeof(_Py_CODEUNIT) + 1;
    }
    stack_pointer = f->f_stacktop;
    assert(stack_pointer != NULL);
    f->f_stacktop = NULL;       /* remains NULL unless yield suspends frame */
    f->f_executing = 1;


#ifdef LLTRACE
    lltrace = _PyDict_GetItemId(f->f_globals, &PyId___ltrace__) != NULL;
#endif

    why = WHY_NOT;

    if (throwflag) /* support for generator.throw() */
        goto error;

#ifdef Py_DEBUG
    /* PyEval_EvalFrameEx() must not be called with an exception set,
       because it can clear it (directly or indirectly) and so the
       caller loses its exception */
    assert(!PyErr_Occurred());
#endif

    for (;;) {
        assert(stack_pointer >= f->f_valuestack); /* else underflow */
        assert(STACK_LEVEL() <= co->co_stacksize);  /* else overflow */
        assert(!PyErr_Occurred());

        /* Do periodic things.  Doing this every time through
           the loop would add too much overhead, so we do it
           only every Nth instruction.  We also do it if
           ``pendingcalls_to_do'' is set, i.e. when an asynchronous
           event needs attention (e.g. a signal handler or
           async I/O handler); see Py_AddPendingCall() and
           Py_MakePendingCalls() above. */

        if (_Py_atomic_load_relaxed(&_PyRuntime.ceval.eval_breaker)) {
            opcode = _Py_OPCODE(*next_instr);
            if (opcode == SETUP_FINALLY ||
                opcode == SETUP_WITH ||
                opcode == BEFORE_ASYNC_WITH ||
                opcode == YIELD_FROM) {
                /* Few cases where we skip running signal handlers and other
                   pending calls:
                   - If we're about to enter the 'with:'. It will prevent
                     emitting a resource warning in the common idiom
                     'with open(path) as file:'.
                   - If we're about to enter the 'async with:'.
                   - If we're about to enter the 'try:' of a try/finally (not
                     *very* useful, but might help in some cases and it's
                     traditional)
                   - If we're resuming a chain of nested 'yield from' or
                     'await' calls, then each frame is parked with YIELD_FROM
                     as its next opcode. If the user hit control-C we want to
                     wait until we've reached the innermost frame before
                     running the signal handler and raising KeyboardInterrupt
                     (see bpo-30039).
                */
                goto fast_next_opcode;
            }
            if (_Py_atomic_load_relaxed(
                        &_PyRuntime.ceval.pending.calls_to_do))
            {
                if (Py_MakePendingCalls() < 0)
                    goto error;
            }
            if (_Py_atomic_load_relaxed(
                        &_PyRuntime.ceval.gil_drop_request))
            {
                /* Give another thread a chance */
                if (PyThreadState_Swap(NULL) != tstate)
                    Py_FatalError("ceval: tstate mix-up");
                drop_gil(tstate);

                /* Other threads may run now */

                take_gil(tstate);

                /* Check if we should make a quick exit. */
                if (_Py_IsFinalizing() &&
                    !_Py_CURRENTLY_FINALIZING(tstate))
                {
                    drop_gil(tstate);
                    PyThread_exit_thread();
                }

                if (PyThreadState_Swap(tstate) != NULL)
                    Py_FatalError("ceval: orphan tstate");
            }
            /* Check for asynchronous exceptions. */
            if (tstate->async_exc != NULL) {
                PyObject *exc = tstate->async_exc;
                tstate->async_exc = NULL;
                UNSIGNAL_ASYNC_EXC();
                PyErr_SetNone(exc);
                Py_DECREF(exc);
                goto error;
            }
        }

    fast_next_opcode:
        f->f_lasti = INSTR_OFFSET();

        if (PyDTrace_LINE_ENABLED())
            maybe_dtrace_line(f, &instr_lb, &instr_ub, &instr_prev);

        /* line-by-line tracing support */

        if (_Py_TracingPossible &&
            tstate->c_tracefunc != NULL && !tstate->tracing) {
            int err;
            /* see maybe_call_line_trace
               for expository comments */
            f->f_stacktop = stack_pointer;

            err = maybe_call_line_trace(tstate->c_tracefunc,
                                        tstate->c_traceobj,
                                        tstate, f,
                                        &instr_lb, &instr_ub, &instr_prev);
            /* Reload possibly changed frame fields */
            JUMPTO(f->f_lasti);
            if (f->f_stacktop != NULL) {
                stack_pointer = f->f_stacktop;
                f->f_stacktop = NULL;
            }
            if (err)
                /* trace function raised an exception */
                goto error;
        }

        /* Extract opcode and argument */

        NEXTOPARG();
    dispatch_opcode:
#ifdef DYNAMIC_EXECUTION_PROFILE
#ifdef DXPAIRS
        dxpairs[lastopcode][opcode]++;
        lastopcode = opcode;
#endif
        dxp[opcode]++;
#endif

#ifdef LLTRACE
        /* Instruction tracing */

        if (lltrace) {
            if (HAS_ARG(opcode)) {
                printf("%d: %d, %d\n",
                       f->f_lasti, opcode, oparg);
            }
            else {
                printf("%d: %d\n",
                       f->f_lasti, opcode);
            }
        }
#endif

        switch (opcode) {

        /* BEWARE!
           It is essential that any operation that fails sets either
           x to NULL, err to nonzero, or why to anything but WHY_NOT,
           and that no operation that succeeds does this! */

        TARGET(NOP)
            FAST_DISPATCH();

        TARGET(LOAD_FAST) {
            PyObject *value = GETLOCAL(oparg);
            if (value == NULL) {
                format_exc_check_arg(PyExc_UnboundLocalError,
                                     UNBOUNDLOCAL_ERROR_MSG,
                                     PyTuple_GetItem(co->co_varnames, oparg));
                goto error;
            }
            Py_INCREF(value);
            PUSH(value);
            FAST_DISPATCH();
        }

        PREDICTED(LOAD_CONST);
        TARGET(LOAD_CONST) {
            PyObject *value = GETITEM(consts, oparg);
            Py_INCREF(value);
            PUSH(value);
            FAST_DISPATCH();
        }

        PREDICTED(STORE_FAST);
        TARGET(STORE_FAST) {
            PyObject *value = POP();
            SETLOCAL(oparg, value);
            FAST_DISPATCH();
        }

        TARGET(POP_TOP) {
            PyObject *value = POP();
            Py_DECREF(value);
            FAST_DISPATCH();
        }

        TARGET(ROT_TWO) {
            PyObject *top = TOP();
            PyObject *second = SECOND();
            SET_TOP(second);
            SET_SECOND(top);
            FAST_DISPATCH();
        }

        TARGET(ROT_THREE) {
            PyObject *top = TOP();
            PyObject *second = SECOND();
            PyObject *third = THIRD();
            SET_TOP(second);
            SET_SECOND(third);
            SET_THIRD(top);
            FAST_DISPATCH();
        }

        TARGET(DUP_TOP) {
            PyObject *top = TOP();
            Py_INCREF(top);
            PUSH(top);
            FAST_DISPATCH();
        }

        TARGET(DUP_TOP_TWO) {
            PyObject *top = TOP();
            PyObject *second = SECOND();
            Py_INCREF(top);
            Py_INCREF(second);
            STACKADJ(2);
            SET_TOP(top);
            SET_SECOND(second);
            FAST_DISPATCH();
        }

        TARGET(UNARY_POSITIVE) {
            PyObject *value = TOP();
            PyObject *res = PyNumber_Positive(value);
            Py_DECREF(value);
            SET_TOP(res);
            if (res == NULL)
                goto error;
            DISPATCH();
        }

        TARGET(UNARY_NEGATIVE) {
            PyObject *value = TOP();
            PyObject *res = PyNumber_Negative(value);
            Py_DECREF(value);
            SET_TOP(res);
            if (res == NULL)
                goto error;
            DISPATCH();
        }

        TARGET(UNARY_NOT) {
            PyObject *value = TOP();
            int err = PyObject_IsTrue(value);
            Py_DECREF(value);
            if (err == 0) {
                Py_INCREF(Py_True);
                SET_TOP(Py_True);
                DISPATCH();
            }
            else if (err > 0) {
                Py_INCREF(Py_False);
                SET_TOP(Py_False);
                DISPATCH();
            }
            STACKADJ(-1);
            goto error;
        }

        TARGET(UNARY_INVERT) {
            PyObject *value = TOP();
            PyObject *res = PyNumber_Invert(value);
            Py_DECREF(value);
            SET_TOP(res);
            if (res == NULL)
                goto error;
            DISPATCH();
        }

        TARGET(BINARY_POWER) {
            PyObject *exp = POP();
            PyObject *base = TOP();
            PyObject *res = PyNumber_Power(base, exp, Py_None);
            Py_DECREF(base);
            Py_DECREF(exp);
            SET_TOP(res);
            if (res == NULL)
                goto error;
            DISPATCH();
        }

        TARGET(BINARY_MULTIPLY) {
            PyObject *right = POP();
            PyObject *left = TOP();
            PyObject *res = PyNumber_Multiply(left, right);
            Py_DECREF(left);
            Py_DECREF(right);
            SET_TOP(res);
            if (res == NULL)
                goto error;
            DISPATCH();
        }

        TARGET(BINARY_MATRIX_MULTIPLY) {
            PyObject *right = POP();
            PyObject *left = TOP();
            PyObject *res = PyNumber_MatrixMultiply(left, right);
            Py_DECREF(left);
            Py_DECREF(right);
            SET_TOP(res);
            if (res == NULL)
                goto error;
            DISPATCH();
        }

        TARGET(BINARY_TRUE_DIVIDE) {
            PyObject *divisor = POP();
            PyObject *dividend = TOP();
            PyObject *quotient = PyNumber_TrueDivide(dividend, divisor);
            Py_DECREF(dividend);
            Py_DECREF(divisor);
            SET_TOP(quotient);
            if (quotient == NULL)
                goto error;
            DISPATCH();
        }

        TARGET(BINARY_FLOOR_DIVIDE) {
            PyObject *divisor = POP();
            PyObject *dividend = TOP();
            PyObject *quotient = PyNumber_FloorDivide(dividend, divisor);
            Py_DECREF(dividend);
            Py_DECREF(divisor);
            SET_TOP(quotient);
            if (quotient == NULL)
                goto error;
            DISPATCH();
        }

        TARGET(BINARY_MODULO) {
            PyObject *divisor = POP();
            PyObject *dividend = TOP();
            PyObject *res;
            if (PyUnicode_CheckExact(dividend) && (
                  !PyUnicode_Check(divisor) || PyUnicode_CheckExact(divisor))) {
              // fast path; string formatting, but not if the RHS is a str subclass
              // (see issue28598)
              res = PyUnicode_Format(dividend, divisor);
            } else {
              res = PyNumber_Remainder(dividend, divisor);
            }
            Py_DECREF(divisor);
            Py_DECREF(dividend);
            SET_TOP(res);
            if (res == NULL)
                goto error;
            DISPATCH();
        }

        TARGET(BINARY_ADD) {
            PyObject *right = POP();
            PyObject *left = TOP();
            PyObject *sum;
            /* NOTE(haypo): Please don't try to micro-optimize int+int on
               CPython using bytecode, it is simply worthless.
               See http://bugs.python.org/issue21955 and
               http://bugs.python.org/issue10044 for the discussion. In short,
               no patch shown any impact on a realistic benchmark, only a minor
               speedup on microbenchmarks. */
            if (PyUnicode_CheckExact(left) &&
                     PyUnicode_CheckExact(right)) {
                sum = unicode_concatenate(left, right, f, next_instr);
                /* unicode_concatenate consumed the ref to left */
            }
            else {
                sum = PyNumber_Add(left, right);
                Py_DECREF(left);
            }
            Py_DECREF(right);
            SET_TOP(sum);
            if (sum == NULL)
                goto error;
            DISPATCH();
        }

        TARGET(BINARY_SUBTRACT) {
            PyObject *right = POP();
            PyObject *left = TOP();
            PyObject *diff = PyNumber_Subtract(left, right);
            Py_DECREF(right);
            Py_DECREF(left);
            SET_TOP(diff);
            if (diff == NULL)
                goto error;
            DISPATCH();
        }

        TARGET(BINARY_SUBSCR) {
            PyObject *sub = POP();
            PyObject *container = TOP();
            PyObject *res = PyObject_GetItem(container, sub);
            Py_DECREF(container);
            Py_DECREF(sub);
            SET_TOP(res);
            if (res == NULL)
                goto error;
            DISPATCH();
        }

        TARGET(BINARY_LSHIFT) {
            PyObject *right = POP();
            PyObject *left = TOP();
            PyObject *res = PyNumber_Lshift(left, right);
            Py_DECREF(left);
            Py_DECREF(right);
            SET_TOP(res);
            if (res == NULL)
                goto error;
            DISPATCH();
        }

        TARGET(BINARY_RSHIFT) {
            PyObject *right = POP();
            PyObject *left = TOP();
            PyObject *res = PyNumber_Rshift(left, right);
            Py_DECREF(left);
            Py_DECREF(right);
            SET_TOP(res);
            if (res == NULL)
                goto error;
            DISPATCH();
        }

        TARGET(BINARY_AND) {
            PyObject *right = POP();
            PyObject *left = TOP();
            PyObject *res = PyNumber_And(left, right);
            Py_DECREF(left);
            Py_DECREF(right);
            SET_TOP(res);
            if (res == NULL)
                goto error;
            DISPATCH();
        }

        TARGET(BINARY_XOR) {
            PyObject *right = POP();
            PyObject *left = TOP();
            PyObject *res = PyNumber_Xor(left, right);
            Py_DECREF(left);
            Py_DECREF(right);
            SET_TOP(res);
            if (res == NULL)
                goto error;
            DISPATCH();
        }

        TARGET(BINARY_OR) {
            PyObject *right = POP();
            PyObject *left = TOP();
            PyObject *res = PyNumber_Or(left, right);
            Py_DECREF(left);
            Py_DECREF(right);
            SET_TOP(res);
            if (res == NULL)
                goto error;
            DISPATCH();
        }

        TARGET(LIST_APPEND) {
            PyObject *v = POP();
            PyObject *list = PEEK(oparg);
            int err;
            err = PyList_Append(list, v);
            Py_DECREF(v);
            if (err != 0)
                goto error;
            PREDICT(JUMP_ABSOLUTE);
            DISPATCH();
        }

        TARGET(SET_ADD) {
            PyObject *v = POP();
            PyObject *set = PEEK(oparg);
            int err;
            err = PySet_Add(set, v);
            Py_DECREF(v);
            if (err != 0)
                goto error;
            PREDICT(JUMP_ABSOLUTE);
            DISPATCH();
        }

        TARGET(INPLACE_POWER) {
            PyObject *exp = POP();
            PyObject *base = TOP();
            PyObject *res = PyNumber_InPlacePower(base, exp, Py_None);
            Py_DECREF(base);
            Py_DECREF(exp);
            SET_TOP(res);
            if (res == NULL)
                goto error;
            DISPATCH();
        }

        TARGET(INPLACE_MULTIPLY) {
            PyObject *right = POP();
            PyObject *left = TOP();
            PyObject *res = PyNumber_InPlaceMultiply(left, right);
            Py_DECREF(left);
            Py_DECREF(right);
            SET_TOP(res);
            if (res == NULL)
                goto error;
            DISPATCH();
        }

        TARGET(INPLACE_MATRIX_MULTIPLY) {
            PyObject *right = POP();
            PyObject *left = TOP();
            PyObject *res = PyNumber_InPlaceMatrixMultiply(left, right);
            Py_DECREF(left);
            Py_DECREF(right);
            SET_TOP(res);
            if (res == NULL)
                goto error;
            DISPATCH();
        }

        TARGET(INPLACE_TRUE_DIVIDE) {
            PyObject *divisor = POP();
            PyObject *dividend = TOP();
            PyObject *quotient = PyNumber_InPlaceTrueDivide(dividend, divisor);
            Py_DECREF(dividend);
            Py_DECREF(divisor);
            SET_TOP(quotient);
            if (quotient == NULL)
                goto error;
            DISPATCH();
        }

        TARGET(INPLACE_FLOOR_DIVIDE) {
            PyObject *divisor = POP();
            PyObject *dividend = TOP();
            PyObject *quotient = PyNumber_InPlaceFloorDivide(dividend, divisor);
            Py_DECREF(dividend);
            Py_DECREF(divisor);
            SET_TOP(quotient);
            if (quotient == NULL)
                goto error;
            DISPATCH();
        }

        TARGET(INPLACE_MODULO) {
            PyObject *right = POP();
            PyObject *left = TOP();
            PyObject *mod = PyNumber_InPlaceRemainder(left, right);
            Py_DECREF(left);
            Py_DECREF(right);
            SET_TOP(mod);
            if (mod == NULL)
                goto error;
            DISPATCH();
        }

        TARGET(INPLACE_ADD) {
            PyObject *right = POP();
            PyObject *left = TOP();
            PyObject *sum;
            if (PyUnicode_CheckExact(left) && PyUnicode_CheckExact(right)) {
                sum = unicode_concatenate(left, right, f, next_instr);
                /* unicode_concatenate consumed the ref to left */
            }
            else {
                sum = PyNumber_InPlaceAdd(left, right);
                Py_DECREF(left);
            }
            Py_DECREF(right);
            SET_TOP(sum);
            if (sum == NULL)
                goto error;
            DISPATCH();
        }

        TARGET(INPLACE_SUBTRACT) {
            PyObject *right = POP();
            PyObject *left = TOP();
            PyObject *diff = PyNumber_InPlaceSubtract(left, right);
            Py_DECREF(left);
            Py_DECREF(right);
            SET_TOP(diff);
            if (diff == NULL)
                goto error;
            DISPATCH();
        }

        TARGET(INPLACE_LSHIFT) {
            PyObject *right = POP();
            PyObject *left = TOP();
            PyObject *res = PyNumber_InPlaceLshift(left, right);
            Py_DECREF(left);
            Py_DECREF(right);
            SET_TOP(res);
            if (res == NULL)
                goto error;
            DISPATCH();
        }

        TARGET(INPLACE_RSHIFT) {
            PyObject *right = POP();
            PyObject *left = TOP();
            PyObject *res = PyNumber_InPlaceRshift(left, right);
            Py_DECREF(left);
            Py_DECREF(right);
            SET_TOP(res);
            if (res == NULL)
                goto error;
            DISPATCH();
        }

        TARGET(INPLACE_AND) {
            PyObject *right = POP();
            PyObject *left = TOP();
            PyObject *res = PyNumber_InPlaceAnd(left, right);
            Py_DECREF(left);
            Py_DECREF(right);
            SET_TOP(res);
            if (res == NULL)
                goto error;
            DISPATCH();
        }

        TARGET(INPLACE_XOR) {
            PyObject *right = POP();
            PyObject *left = TOP();
            PyObject *res = PyNumber_InPlaceXor(left, right);
            Py_DECREF(left);
            Py_DECREF(right);
            SET_TOP(res);
            if (res == NULL)
                goto error;
            DISPATCH();
        }

        TARGET(INPLACE_OR) {
            PyObject *right = POP();
            PyObject *left = TOP();
            PyObject *res = PyNumber_InPlaceOr(left, right);
            Py_DECREF(left);
            Py_DECREF(right);
            SET_TOP(res);
            if (res == NULL)
                goto error;
            DISPATCH();
        }

        TARGET(STORE_SUBSCR) {
            PyObject *sub = TOP();
            PyObject *container = SECOND();
            PyObject *v = THIRD();
            int err;
            STACKADJ(-3);
            /* container[sub] = v */
            err = PyObject_SetItem(container, sub, v);
            Py_DECREF(v);
            Py_DECREF(container);
            Py_DECREF(sub);
            if (err != 0)
                goto error;
            DISPATCH();
        }

        TARGET(DELETE_SUBSCR) {
            PyObject *sub = TOP();
            PyObject *container = SECOND();
            int err;
            STACKADJ(-2);
            /* del container[sub] */
            err = PyObject_DelItem(container, sub);
            Py_DECREF(container);
            Py_DECREF(sub);
            if (err != 0)
                goto error;
            DISPATCH();
        }

        TARGET(PRINT_EXPR) {
            _Py_IDENTIFIER(displayhook);
            PyObject *value = POP();
            PyObject *hook = _PySys_GetObjectId(&PyId_displayhook);
            PyObject *res;
            if (hook == NULL) {
                PyErr_SetString(PyExc_RuntimeError,
                                "lost sys.displayhook");
                Py_DECREF(value);
                goto error;
            }
            res = PyObject_CallFunctionObjArgs(hook, value, NULL);
            Py_DECREF(value);
            if (res == NULL)
                goto error;
            Py_DECREF(res);
            DISPATCH();
        }

        TARGET(RAISE_VARARGS) {
            PyObject *cause = NULL, *exc = NULL;
            switch (oparg) {
            case 2:
                cause = POP(); /* cause */
                /* fall through */
            case 1:
                exc = POP(); /* exc */
                /* fall through */
            case 0:
                if (do_raise(exc, cause)) {
                    why = WHY_EXCEPTION;
                    goto fast_block_end;
                }
                break;
            default:
                PyErr_SetString(PyExc_SystemError,
                           "bad RAISE_VARARGS oparg");
                break;
            }
            goto error;
        }

        TARGET(RETURN_VALUE) {
            retval = POP();
            why = WHY_RETURN;
            goto fast_block_end;
        }

        TARGET(GET_AITER) {
            unaryfunc getter = NULL;
            PyObject *iter = NULL;
            PyObject *obj = TOP();
            PyTypeObject *type = Py_TYPE(obj);

            if (type->tp_as_async != NULL) {
                getter = type->tp_as_async->am_aiter;
            }

            if (getter != NULL) {
                iter = (*getter)(obj);
                Py_DECREF(obj);
                if (iter == NULL) {
                    SET_TOP(NULL);
                    goto error;
                }
            }
            else {
                SET_TOP(NULL);
                PyErr_Format(
                    PyExc_TypeError,
                    "'async for' requires an object with "
                    "__aiter__ method, got %.100s",
                    type->tp_name);
                Py_DECREF(obj);
                goto error;
            }

            if (Py_TYPE(iter)->tp_as_async == NULL ||
                    Py_TYPE(iter)->tp_as_async->am_anext == NULL) {

                SET_TOP(NULL);
                PyErr_Format(
                    PyExc_TypeError,
                    "'async for' received an object from __aiter__ "
                    "that does not implement __anext__: %.100s",
                    Py_TYPE(iter)->tp_name);
                Py_DECREF(iter);
                goto error;
            }

            SET_TOP(iter);
            DISPATCH();
        }

        TARGET(GET_ANEXT) {
            unaryfunc getter = NULL;
            PyObject *next_iter = NULL;
            PyObject *awaitable = NULL;
            PyObject *aiter = TOP();
            PyTypeObject *type = Py_TYPE(aiter);

            if (PyAsyncGen_CheckExact(aiter)) {
                awaitable = type->tp_as_async->am_anext(aiter);
                if (awaitable == NULL) {
                    goto error;
                }
            } else {
                if (type->tp_as_async != NULL){
                    getter = type->tp_as_async->am_anext;
                }

                if (getter != NULL) {
                    next_iter = (*getter)(aiter);
                    if (next_iter == NULL) {
                        goto error;
                    }
                }
                else {
                    PyErr_Format(
                        PyExc_TypeError,
                        "'async for' requires an iterator with "
                        "__anext__ method, got %.100s",
                        type->tp_name);
                    goto error;
                }

                awaitable = _PyCoro_GetAwaitableIter(next_iter);
                if (awaitable == NULL) {
                    _PyErr_FormatFromCause(
                        PyExc_TypeError,
                        "'async for' received an invalid object "
                        "from __anext__: %.100s",
                        Py_TYPE(next_iter)->tp_name);

                    Py_DECREF(next_iter);
                    goto error;
                } else {
                    Py_DECREF(next_iter);
                }
            }

            PUSH(awaitable);
            PREDICT(LOAD_CONST);
            DISPATCH();
        }

        PREDICTED(GET_AWAITABLE);
        TARGET(GET_AWAITABLE) {
            PyObject *iterable = TOP();
            PyObject *iter = _PyCoro_GetAwaitableIter(iterable);

            if (iter == NULL) {
                format_awaitable_error(Py_TYPE(iterable),
                                       _Py_OPCODE(next_instr[-2]));
            }

            Py_DECREF(iterable);

            if (iter != NULL && PyCoro_CheckExact(iter)) {
                PyObject *yf = _PyGen_yf((PyGenObject*)iter);
                if (yf != NULL) {
                    /* `iter` is a coroutine object that is being
                       awaited, `yf` is a pointer to the current awaitable
                       being awaited on. */
                    Py_DECREF(yf);
                    Py_CLEAR(iter);
                    PyErr_SetString(
                        PyExc_RuntimeError,
                        "coroutine is being awaited already");
                    /* The code below jumps to `error` if `iter` is NULL. */
                }
            }

            SET_TOP(iter); /* Even if it's NULL */

            if (iter == NULL) {
                goto error;
            }

            PREDICT(LOAD_CONST);
            DISPATCH();
        }

        TARGET(YIELD_FROM) {
            PyObject *v = POP();
            PyObject *receiver = TOP();
            int err;
            if (PyGen_CheckExact(receiver) || PyCoro_CheckExact(receiver)) {
                retval = _PyGen_Send((PyGenObject *)receiver, v);
            } else {
                _Py_IDENTIFIER(send);
                if (v == Py_None)
                    retval = Py_TYPE(receiver)->tp_iternext(receiver);
                else
                    retval = _PyObject_CallMethodIdObjArgs(receiver, &PyId_send, v, NULL);
            }
            Py_DECREF(v);
            if (retval == NULL) {
                PyObject *val;
                if (tstate->c_tracefunc != NULL
                        && PyErr_ExceptionMatches(PyExc_StopIteration))
                    call_exc_trace(tstate->c_tracefunc, tstate->c_traceobj, tstate, f);
                err = _PyGen_FetchStopIterationValue(&val);
                if (err < 0)
                    goto error;
                Py_DECREF(receiver);
                SET_TOP(val);
                DISPATCH();
            }
            /* receiver remains on stack, retval is value to be yielded */
            f->f_stacktop = stack_pointer;
            why = WHY_YIELD;
            /* and repeat... */
            assert(f->f_lasti >= (int)sizeof(_Py_CODEUNIT));
            f->f_lasti -= sizeof(_Py_CODEUNIT);
            goto fast_yield;
        }

        TARGET(YIELD_VALUE) {
            retval = POP();

            if (co->co_flags & CO_ASYNC_GENERATOR) {
                PyObject *w = _PyAsyncGenValueWrapperNew(retval);
                Py_DECREF(retval);
                if (w == NULL) {
                    retval = NULL;
                    goto error;
                }
                retval = w;
            }

            f->f_stacktop = stack_pointer;
            why = WHY_YIELD;
            goto fast_yield;
        }

        TARGET(POP_EXCEPT) {
            PyTryBlock *b = PyFrame_BlockPop(f);
            if (b->b_type != EXCEPT_HANDLER) {
                PyErr_SetString(PyExc_SystemError,
                                "popped block is not an except handler");
                goto error;
            }
            UNWIND_EXCEPT_HANDLER(b);
            DISPATCH();
        }

        PREDICTED(POP_BLOCK);
        TARGET(POP_BLOCK) {
            PyTryBlock *b = PyFrame_BlockPop(f);
            UNWIND_BLOCK(b);
            DISPATCH();
        }

        PREDICTED(END_FINALLY);
        TARGET(END_FINALLY) {
            PyObject *status = POP();
            if (PyLong_Check(status)) {
                why = (enum why_code) PyLong_AS_LONG(status);
                assert(why != WHY_YIELD && why != WHY_EXCEPTION);
                if (why == WHY_RETURN ||
                    why == WHY_CONTINUE)
                    retval = POP();
                if (why == WHY_SILENCED) {
                    /* An exception was silenced by 'with', we must
                    manually unwind the EXCEPT_HANDLER block which was
                    created when the exception was caught, otherwise
                    the stack will be in an inconsistent state. */
                    PyTryBlock *b = PyFrame_BlockPop(f);
                    assert(b->b_type == EXCEPT_HANDLER);
                    UNWIND_EXCEPT_HANDLER(b);
                    why = WHY_NOT;
                    Py_DECREF(status);
                    DISPATCH();
                }
                Py_DECREF(status);
                goto fast_block_end;
            }
            else if (PyExceptionClass_Check(status)) {
                PyObject *exc = POP();
                PyObject *tb = POP();
                PyErr_Restore(status, exc, tb);
                why = WHY_EXCEPTION;
                goto fast_block_end;
            }
            else if (status != Py_None) {
                PyErr_SetString(PyExc_SystemError,
                    "'finally' pops bad exception");
                Py_DECREF(status);
                goto error;
            }
            Py_DECREF(status);
            DISPATCH();
        }

        TARGET(LOAD_BUILD_CLASS) {
            _Py_IDENTIFIER(__build_class__);

            PyObject *bc;
            if (PyDict_CheckExact(f->f_builtins)) {
                bc = _PyDict_GetItemId(f->f_builtins, &PyId___build_class__);
                if (bc == NULL) {
                    PyErr_SetString(PyExc_NameError,
                                    "__build_class__ not found");
                    goto error;
                }
                Py_INCREF(bc);
            }
            else {
                PyObject *build_class_str = _PyUnicode_FromId(&PyId___build_class__);
                if (build_class_str == NULL)
                    goto error;
                bc = PyObject_GetItem(f->f_builtins, build_class_str);
                if (bc == NULL) {
                    if (PyErr_ExceptionMatches(PyExc_KeyError))
                        PyErr_SetString(PyExc_NameError,
                                        "__build_class__ not found");
                    goto error;
                }
            }
            PUSH(bc);
            DISPATCH();
        }

        TARGET(STORE_NAME) {
            PyObject *name = GETITEM(names, oparg);
            PyObject *v = POP();
            PyObject *ns = f->f_locals;
            int err;
            if (ns == NULL) {
                PyErr_Format(PyExc_SystemError,
                             "no locals found when storing %R", name);
                Py_DECREF(v);
                goto error;
            }
            if (PyDict_CheckExact(ns))
                err = PyDict_SetItem(ns, name, v);
            else
                err = PyObject_SetItem(ns, name, v);
            Py_DECREF(v);
            if (err != 0)
                goto error;
            DISPATCH();
        }

        TARGET(DELETE_NAME) {
            PyObject *name = GETITEM(names, oparg);
            PyObject *ns = f->f_locals;
            int err;
            if (ns == NULL) {
                PyErr_Format(PyExc_SystemError,
                             "no locals when deleting %R", name);
                goto error;
            }
            err = PyObject_DelItem(ns, name);
            if (err != 0) {
                format_exc_check_arg(PyExc_NameError,
                                     NAME_ERROR_MSG,
                                     name);
                goto error;
            }
            DISPATCH();
        }

        PREDICTED(UNPACK_SEQUENCE);
        TARGET(UNPACK_SEQUENCE) {
            PyObject *seq = POP(), *item, **items;
            if (PyTuple_CheckExact(seq) &&
                PyTuple_GET_SIZE(seq) == oparg) {
                items = ((PyTupleObject *)seq)->ob_item;
                while (oparg--) {
                    item = items[oparg];
                    Py_INCREF(item);
                    PUSH(item);
                }
            } else if (PyList_CheckExact(seq) &&
                       PyList_GET_SIZE(seq) == oparg) {
                items = ((PyListObject *)seq)->ob_item;
                while (oparg--) {
                    item = items[oparg];
                    Py_INCREF(item);
                    PUSH(item);
                }
            } else if (unpack_iterable(seq, oparg, -1,
                                       stack_pointer + oparg)) {
                STACKADJ(oparg);
            } else {
                /* unpack_iterable() raised an exception */
                Py_DECREF(seq);
                goto error;
            }
            Py_DECREF(seq);
            DISPATCH();
        }

        TARGET(UNPACK_EX) {
            int totalargs = 1 + (oparg & 0xFF) + (oparg >> 8);
            PyObject *seq = POP();

            if (unpack_iterable(seq, oparg & 0xFF, oparg >> 8,
                                stack_pointer + totalargs)) {
                stack_pointer += totalargs;
            } else {
                Py_DECREF(seq);
                goto error;
            }
            Py_DECREF(seq);
            DISPATCH();
        }

        TARGET(STORE_ATTR) {
            PyObject *name = GETITEM(names, oparg);
            PyObject *owner = TOP();
            PyObject *v = SECOND();
            int err;
            STACKADJ(-2);
            err = PyObject_SetAttr(owner, name, v);
            Py_DECREF(v);
            Py_DECREF(owner);
            if (err != 0)
                goto error;
            DISPATCH();
        }

        TARGET(DELETE_ATTR) {
            PyObject *name = GETITEM(names, oparg);
            PyObject *owner = POP();
            int err;
            err = PyObject_SetAttr(owner, name, (PyObject *)NULL);
            Py_DECREF(owner);
            if (err != 0)
                goto error;
            DISPATCH();
        }

        TARGET(STORE_GLOBAL) {
            PyObject *name = GETITEM(names, oparg);
            PyObject *v = POP();
            int err;
            err = PyDict_SetItem(f->f_globals, name, v);
            Py_DECREF(v);
            if (err != 0)
                goto error;
            DISPATCH();
        }

        TARGET(DELETE_GLOBAL) {
            PyObject *name = GETITEM(names, oparg);
            int err;
            err = PyDict_DelItem(f->f_globals, name);
            if (err != 0) {
                format_exc_check_arg(
                    PyExc_NameError, NAME_ERROR_MSG, name);
                goto error;
            }
            DISPATCH();
        }

        TARGET(LOAD_NAME) {
            PyObject *name = GETITEM(names, oparg);
            PyObject *locals = f->f_locals;
            PyObject *v;
            if (locals == NULL) {
                PyErr_Format(PyExc_SystemError,
                             "no locals when loading %R", name);
                goto error;
            }
            if (PyDict_CheckExact(locals)) {
                v = PyDict_GetItem(locals, name);
                Py_XINCREF(v);
            }
            else {
                v = PyObject_GetItem(locals, name);
                if (v == NULL) {
                    if (!PyErr_ExceptionMatches(PyExc_KeyError))
                        goto error;
                    PyErr_Clear();
                }
            }
            if (v == NULL) {
                v = PyDict_GetItem(f->f_globals, name);
                Py_XINCREF(v);
                if (v == NULL) {
                    if (PyDict_CheckExact(f->f_builtins)) {
                        v = PyDict_GetItem(f->f_builtins, name);
                        if (v == NULL) {
                            format_exc_check_arg(
                                        PyExc_NameError,
                                        NAME_ERROR_MSG, name);
                            goto error;
                        }
                        Py_INCREF(v);
                    }
                    else {
                        v = PyObject_GetItem(f->f_builtins, name);
                        if (v == NULL) {
                            if (PyErr_ExceptionMatches(PyExc_KeyError))
                                format_exc_check_arg(
                                            PyExc_NameError,
                                            NAME_ERROR_MSG, name);
                            goto error;
                        }
                    }
                }
            }
            PUSH(v);
            DISPATCH();
        }

        TARGET(LOAD_GLOBAL) {
            PyObject *name = GETITEM(names, oparg);
            PyObject *v;
            if (PyDict_CheckExact(f->f_globals)
                && PyDict_CheckExact(f->f_builtins))
            {
                v = _PyDict_LoadGlobal((PyDictObject *)f->f_globals,
                                       (PyDictObject *)f->f_builtins,
                                       name);
                if (v == NULL) {
                    if (!_PyErr_OCCURRED()) {
                        /* _PyDict_LoadGlobal() returns NULL without raising
                         * an exception if the key doesn't exist */
                        format_exc_check_arg(PyExc_NameError,
                                             NAME_ERROR_MSG, name);
                    }
                    goto error;
                }
                Py_INCREF(v);
            }
            else {
                /* Slow-path if globals or builtins is not a dict */

                /* namespace 1: globals */
                v = PyObject_GetItem(f->f_globals, name);
                if (v == NULL) {
                    if (!PyErr_ExceptionMatches(PyExc_KeyError))
                        goto error;
                    PyErr_Clear();

                    /* namespace 2: builtins */
                    v = PyObject_GetItem(f->f_builtins, name);
                    if (v == NULL) {
                        if (PyErr_ExceptionMatches(PyExc_KeyError))
                            format_exc_check_arg(
                                        PyExc_NameError,
                                        NAME_ERROR_MSG, name);
                        goto error;
                    }
                }
            }
            PUSH(v);
            DISPATCH();
        }

        TARGET(DELETE_FAST) {
            PyObject *v = GETLOCAL(oparg);
            if (v != NULL) {
                SETLOCAL(oparg, NULL);
                DISPATCH();
            }
            format_exc_check_arg(
                PyExc_UnboundLocalError,
                UNBOUNDLOCAL_ERROR_MSG,
                PyTuple_GetItem(co->co_varnames, oparg)
                );
            goto error;
        }

        TARGET(DELETE_DEREF) {
            PyObject *cell = freevars[oparg];
            PyObject *oldobj = PyCell_GET(cell);
            if (oldobj != NULL) {
                PyCell_SET(cell, NULL);
                Py_DECREF(oldobj);
                DISPATCH();
            }
            format_exc_unbound(co, oparg);
            goto error;
        }

        TARGET(LOAD_CLOSURE) {
            PyObject *cell = freevars[oparg];
            Py_INCREF(cell);
            PUSH(cell);
            DISPATCH();
        }

        TARGET(LOAD_CLASSDEREF) {
            PyObject *name, *value, *locals = f->f_locals;
            Py_ssize_t idx;
            assert(locals);
            assert(oparg >= PyTuple_GET_SIZE(co->co_cellvars));
            idx = oparg - PyTuple_GET_SIZE(co->co_cellvars);
            assert(idx >= 0 && idx < PyTuple_GET_SIZE(co->co_freevars));
            name = PyTuple_GET_ITEM(co->co_freevars, idx);
            if (PyDict_CheckExact(locals)) {
                value = PyDict_GetItem(locals, name);
                Py_XINCREF(value);
            }
            else {
                value = PyObject_GetItem(locals, name);
                if (value == NULL) {
                    if (!PyErr_ExceptionMatches(PyExc_KeyError))
                        goto error;
                    PyErr_Clear();
                }
            }
            if (!value) {
                PyObject *cell = freevars[oparg];
                value = PyCell_GET(cell);
                if (value == NULL) {
                    format_exc_unbound(co, oparg);
                    goto error;
                }
                Py_INCREF(value);
            }
            PUSH(value);
            DISPATCH();
        }

        TARGET(LOAD_DEREF) {
            PyObject *cell = freevars[oparg];
            PyObject *value = PyCell_GET(cell);
            if (value == NULL) {
                format_exc_unbound(co, oparg);
                goto error;
            }
            Py_INCREF(value);
            PUSH(value);
            DISPATCH();
        }

        TARGET(STORE_DEREF) {
            PyObject *v = POP();
            PyObject *cell = freevars[oparg];
            PyObject *oldobj = PyCell_GET(cell);
            PyCell_SET(cell, v);
            Py_XDECREF(oldobj);
            DISPATCH();
        }

        TARGET(BUILD_STRING) {
            PyObject *str;
            PyObject *empty = PyUnicode_New(0, 0);
            if (empty == NULL) {
                goto error;
            }
            str = _PyUnicode_JoinArray(empty, stack_pointer - oparg, oparg);
            Py_DECREF(empty);
            if (str == NULL)
                goto error;
            while (--oparg >= 0) {
                PyObject *item = POP();
                Py_DECREF(item);
            }
            PUSH(str);
            DISPATCH();
        }

        TARGET(BUILD_TUPLE) {
            PyObject *tup = PyTuple_New(oparg);
            if (tup == NULL)
                goto error;
            while (--oparg >= 0) {
                PyObject *item = POP();
                PyTuple_SET_ITEM(tup, oparg, item);
            }
            PUSH(tup);
            DISPATCH();
        }

        TARGET(BUILD_LIST) {
            PyObject *list =  PyList_New(oparg);
            if (list == NULL)
                goto error;
            while (--oparg >= 0) {
                PyObject *item = POP();
                PyList_SET_ITEM(list, oparg, item);
            }
            PUSH(list);
            DISPATCH();
        }

        TARGET(BUILD_TUPLE_UNPACK_WITH_CALL)
        TARGET(BUILD_TUPLE_UNPACK)
        TARGET(BUILD_LIST_UNPACK) {
            int convert_to_tuple = opcode != BUILD_LIST_UNPACK;
            Py_ssize_t i;
            PyObject *sum = PyList_New(0);
            PyObject *return_value;

            if (sum == NULL)
                goto error;

            for (i = oparg; i > 0; i--) {
                PyObject *none_val;

                none_val = _PyList_Extend((PyListObject *)sum, PEEK(i));
                if (none_val == NULL) {
                    if (opcode == BUILD_TUPLE_UNPACK_WITH_CALL &&
                        PyErr_ExceptionMatches(PyExc_TypeError))
                    {
                        check_args_iterable(PEEK(1 + oparg), PEEK(i));
                    }
                    Py_DECREF(sum);
                    goto error;
                }
                Py_DECREF(none_val);
            }

            if (convert_to_tuple) {
                return_value = PyList_AsTuple(sum);
                Py_DECREF(sum);
                if (return_value == NULL)
                    goto error;
            }
            else {
                return_value = sum;
            }

            while (oparg--)
                Py_DECREF(POP());
            PUSH(return_value);
            DISPATCH();
        }

        TARGET(BUILD_SET) {
            PyObject *set = PySet_New(NULL);
            int err = 0;
            int i;
            if (set == NULL)
                goto error;
            for (i = oparg; i > 0; i--) {
                PyObject *item = PEEK(i);
                if (err == 0)
                    err = PySet_Add(set, item);
                Py_DECREF(item);
            }
            STACKADJ(-oparg);
            if (err != 0) {
                Py_DECREF(set);
                goto error;
            }
            PUSH(set);
            DISPATCH();
        }

        TARGET(BUILD_SET_UNPACK) {
            Py_ssize_t i;
            PyObject *sum = PySet_New(NULL);
            if (sum == NULL)
                goto error;

            for (i = oparg; i > 0; i--) {
                if (_PySet_Update(sum, PEEK(i)) < 0) {
                    Py_DECREF(sum);
                    goto error;
                }
            }

            while (oparg--)
                Py_DECREF(POP());
            PUSH(sum);
            DISPATCH();
        }

        TARGET(BUILD_MAP) {
            Py_ssize_t i;
            PyObject *map = _PyDict_NewPresized((Py_ssize_t)oparg);
            if (map == NULL)
                goto error;
            for (i = oparg; i > 0; i--) {
                int err;
                PyObject *key = PEEK(2*i);
                PyObject *value = PEEK(2*i - 1);
                err = PyDict_SetItem(map, key, value);
                if (err != 0) {
                    Py_DECREF(map);
                    goto error;
                }
            }

            while (oparg--) {
                Py_DECREF(POP());
                Py_DECREF(POP());
            }
            PUSH(map);
            DISPATCH();
        }

        TARGET(SETUP_ANNOTATIONS) {
            _Py_IDENTIFIER(__annotations__);
            int err;
            PyObject *ann_dict;
            if (f->f_locals == NULL) {
                PyErr_Format(PyExc_SystemError,
                             "no locals found when setting up annotations");
                goto error;
            }
            /* check if __annotations__ in locals()... */
            if (PyDict_CheckExact(f->f_locals)) {
                ann_dict = _PyDict_GetItemId(f->f_locals,
                                             &PyId___annotations__);
                if (ann_dict == NULL) {
                    /* ...if not, create a new one */
                    ann_dict = PyDict_New();
                    if (ann_dict == NULL) {
                        goto error;
                    }
                    err = _PyDict_SetItemId(f->f_locals,
                                            &PyId___annotations__, ann_dict);
                    Py_DECREF(ann_dict);
                    if (err != 0) {
                        goto error;
                    }
                }
            }
            else {
                /* do the same if locals() is not a dict */
                PyObject *ann_str = _PyUnicode_FromId(&PyId___annotations__);
                if (ann_str == NULL) {
                    goto error;
                }
                ann_dict = PyObject_GetItem(f->f_locals, ann_str);
                if (ann_dict == NULL) {
                    if (!PyErr_ExceptionMatches(PyExc_KeyError)) {
                        goto error;
                    }
                    PyErr_Clear();
                    ann_dict = PyDict_New();
                    if (ann_dict == NULL) {
                        goto error;
                    }
                    err = PyObject_SetItem(f->f_locals, ann_str, ann_dict);
                    Py_DECREF(ann_dict);
                    if (err != 0) {
                        goto error;
                    }
                }
                else {
                    Py_DECREF(ann_dict);
                }
            }
            DISPATCH();
        }

        TARGET(BUILD_CONST_KEY_MAP) {
            Py_ssize_t i;
            PyObject *map;
            PyObject *keys = TOP();
            if (!PyTuple_CheckExact(keys) ||
                PyTuple_GET_SIZE(keys) != (Py_ssize_t)oparg) {
                PyErr_SetString(PyExc_SystemError,
                                "bad BUILD_CONST_KEY_MAP keys argument");
                goto error;
            }
            map = _PyDict_NewPresized((Py_ssize_t)oparg);
            if (map == NULL) {
                goto error;
            }
            for (i = oparg; i > 0; i--) {
                int err;
                PyObject *key = PyTuple_GET_ITEM(keys, oparg - i);
                PyObject *value = PEEK(i + 1);
                err = PyDict_SetItem(map, key, value);
                if (err != 0) {
                    Py_DECREF(map);
                    goto error;
                }
            }

            Py_DECREF(POP());
            while (oparg--) {
                Py_DECREF(POP());
            }
            PUSH(map);
            DISPATCH();
        }

        TARGET(BUILD_MAP_UNPACK) {
            Py_ssize_t i;
            PyObject *sum = PyDict_New();
            if (sum == NULL)
                goto error;

            for (i = oparg; i > 0; i--) {
                PyObject *arg = PEEK(i);
                if (PyDict_Update(sum, arg) < 0) {
                    if (PyErr_ExceptionMatches(PyExc_AttributeError)) {
                        PyErr_Format(PyExc_TypeError,
                                "'%.200s' object is not a mapping",
                                arg->ob_type->tp_name);
                    }
                    Py_DECREF(sum);
                    goto error;
                }
            }

            while (oparg--)
                Py_DECREF(POP());
            PUSH(sum);
            DISPATCH();
        }

        TARGET(BUILD_MAP_UNPACK_WITH_CALL) {
            Py_ssize_t i;
            PyObject *sum = PyDict_New();
            if (sum == NULL)
                goto error;

            for (i = oparg; i > 0; i--) {
                PyObject *arg = PEEK(i);
                if (_PyDict_MergeEx(sum, arg, 2) < 0) {
                    PyObject *func = PEEK(2 + oparg);
                    if (PyErr_ExceptionMatches(PyExc_AttributeError)) {
                        format_kwargs_mapping_error(func, arg);
                    }
                    else if (PyErr_ExceptionMatches(PyExc_KeyError)) {
                        PyObject *exc, *val, *tb;
                        PyErr_Fetch(&exc, &val, &tb);
                        if (val && PyTuple_Check(val) && PyTuple_GET_SIZE(val) == 1) {
                            PyObject *key = PyTuple_GET_ITEM(val, 0);
                            if (!PyUnicode_Check(key)) {
                                PyErr_Format(PyExc_TypeError,
                                        "%.200s%.200s keywords must be strings",
                                        PyEval_GetFuncName(func),
                                        PyEval_GetFuncDesc(func));
                            } else {
                                PyErr_Format(PyExc_TypeError,
                                        "%.200s%.200s got multiple "
                                        "values for keyword argument '%U'",
                                        PyEval_GetFuncName(func),
                                        PyEval_GetFuncDesc(func),
                                        key);
                            }
                            Py_XDECREF(exc);
                            Py_XDECREF(val);
                            Py_XDECREF(tb);
                        }
                        else {
                            PyErr_Restore(exc, val, tb);
                        }
                    }
                    Py_DECREF(sum);
                    goto error;
                }
            }

            while (oparg--)
                Py_DECREF(POP());
            PUSH(sum);
            DISPATCH();
        }

        TARGET(MAP_ADD) {
            PyObject *key = TOP();
            PyObject *value = SECOND();
            PyObject *map;
            int err;
            STACKADJ(-2);
            map = PEEK(oparg);                      /* dict */
            assert(PyDict_CheckExact(map));
            err = PyDict_SetItem(map, key, value);  /* map[key] = value */
            Py_DECREF(value);
            Py_DECREF(key);
            if (err != 0)
                goto error;
            PREDICT(JUMP_ABSOLUTE);
            DISPATCH();
        }

        TARGET(LOAD_ATTR) {
            PyObject *name = GETITEM(names, oparg);
            PyObject *owner = TOP();
            PyObject *res = PyObject_GetAttr(owner, name);
            Py_DECREF(owner);
            SET_TOP(res);
            if (res == NULL)
                goto error;
            DISPATCH();
        }

        TARGET(COMPARE_OP) {
            PyObject *right = POP();
            PyObject *left = TOP();
            PyObject *res = cmp_outcome(oparg, left, right);
            Py_DECREF(left);
            Py_DECREF(right);
            SET_TOP(res);
            if (res == NULL)
                goto error;
            PREDICT(POP_JUMP_IF_FALSE);
            PREDICT(POP_JUMP_IF_TRUE);
            DISPATCH();
        }

        TARGET(IMPORT_NAME) {
            PyObject *name = GETITEM(names, oparg);
            PyObject *fromlist = POP();
            PyObject *level = TOP();
            PyObject *res;
            res = import_name(f, name, fromlist, level);
            Py_DECREF(level);
            Py_DECREF(fromlist);
            SET_TOP(res);
            if (res == NULL)
                goto error;
            DISPATCH();
        }

        TARGET(IMPORT_STAR) {
            PyObject *from = POP(), *locals;
            int err;
            if (PyFrame_FastToLocalsWithError(f) < 0) {
                Py_DECREF(from);
                goto error;
            }

            locals = f->f_locals;
            if (locals == NULL) {
                PyErr_SetString(PyExc_SystemError,
                    "no locals found during 'import *'");
                Py_DECREF(from);
                goto error;
            }
            err = import_all_from(locals, from);
            PyFrame_LocalsToFast(f, 0);
            Py_DECREF(from);
            if (err != 0)
                goto error;
            DISPATCH();
        }

        TARGET(IMPORT_FROM) {
            PyObject *name = GETITEM(names, oparg);
            PyObject *from = TOP();
            PyObject *res;
            res = import_from(from, name);
            PUSH(res);
            if (res == NULL)
                goto error;
            DISPATCH();
        }

        TARGET(JUMP_FORWARD) {
            JUMPBY(oparg);
            FAST_DISPATCH();
        }

        PREDICTED(POP_JUMP_IF_FALSE);
        TARGET(POP_JUMP_IF_FALSE) {
            PyObject *cond = POP();
            int err;
            if (cond == Py_True) {
                Py_DECREF(cond);
                FAST_DISPATCH();
            }
            if (cond == Py_False) {
                Py_DECREF(cond);
                JUMPTO(oparg);
                FAST_DISPATCH();
            }
            err = PyObject_IsTrue(cond);
            Py_DECREF(cond);
            if (err > 0)
                ;
            else if (err == 0)
                JUMPTO(oparg);
            else
                goto error;
            DISPATCH();
        }

        PREDICTED(POP_JUMP_IF_TRUE);
        TARGET(POP_JUMP_IF_TRUE) {
            PyObject *cond = POP();
            int err;
            if (cond == Py_False) {
                Py_DECREF(cond);
                FAST_DISPATCH();
            }
            if (cond == Py_True) {
                Py_DECREF(cond);
                JUMPTO(oparg);
                FAST_DISPATCH();
            }
            err = PyObject_IsTrue(cond);
            Py_DECREF(cond);
            if (err > 0) {
                JUMPTO(oparg);
            }
            else if (err == 0)
                ;
            else
                goto error;
            DISPATCH();
        }

        TARGET(JUMP_IF_FALSE_OR_POP) {
            PyObject *cond = TOP();
            int err;
            if (cond == Py_True) {
                STACKADJ(-1);
                Py_DECREF(cond);
                FAST_DISPATCH();
            }
            if (cond == Py_False) {
                JUMPTO(oparg);
                FAST_DISPATCH();
            }
            err = PyObject_IsTrue(cond);
            if (err > 0) {
                STACKADJ(-1);
                Py_DECREF(cond);
            }
            else if (err == 0)
                JUMPTO(oparg);
            else
                goto error;
            DISPATCH();
        }

        TARGET(JUMP_IF_TRUE_OR_POP) {
            PyObject *cond = TOP();
            int err;
            if (cond == Py_False) {
                STACKADJ(-1);
                Py_DECREF(cond);
                FAST_DISPATCH();
            }
            if (cond == Py_True) {
                JUMPTO(oparg);
                FAST_DISPATCH();
            }
            err = PyObject_IsTrue(cond);
            if (err > 0) {
                JUMPTO(oparg);
            }
            else if (err == 0) {
                STACKADJ(-1);
                Py_DECREF(cond);
            }
            else
                goto error;
            DISPATCH();
        }

        PREDICTED(JUMP_ABSOLUTE);
        TARGET(JUMP_ABSOLUTE) {
            JUMPTO(oparg);
#if FAST_LOOPS
            /* Enabling this path speeds-up all while and for-loops by bypassing
               the per-loop checks for signals.  By default, this should be turned-off
               because it prevents detection of a control-break in tight loops like
               "while 1: pass".  Compile with this option turned-on when you need
               the speed-up and do not need break checking inside tight loops (ones
               that contain only instructions ending with FAST_DISPATCH).
            */
            FAST_DISPATCH();
#else
            DISPATCH();
#endif
        }

        TARGET(GET_ITER) {
            /* before: [obj]; after [getiter(obj)] */
            PyObject *iterable = TOP();
            PyObject *iter = PyObject_GetIter(iterable);
            Py_DECREF(iterable);
            SET_TOP(iter);
            if (iter == NULL)
                goto error;
            PREDICT(FOR_ITER);
            PREDICT(CALL_FUNCTION);
            DISPATCH();
        }

        TARGET(GET_YIELD_FROM_ITER) {
            /* before: [obj]; after [getiter(obj)] */
            PyObject *iterable = TOP();
            PyObject *iter;
            if (PyCoro_CheckExact(iterable)) {
                /* `iterable` is a coroutine */
                if (!(co->co_flags & (CO_COROUTINE | CO_ITERABLE_COROUTINE))) {
                    /* and it is used in a 'yield from' expression of a
                       regular generator. */
                    Py_DECREF(iterable);
                    SET_TOP(NULL);
                    PyErr_SetString(PyExc_TypeError,
                                    "cannot 'yield from' a coroutine object "
                                    "in a non-coroutine generator");
                    goto error;
                }
            }
            else if (!PyGen_CheckExact(iterable)) {
                /* `iterable` is not a generator. */
                iter = PyObject_GetIter(iterable);
                Py_DECREF(iterable);
                SET_TOP(iter);
                if (iter == NULL)
                    goto error;
            }
            PREDICT(LOAD_CONST);
            DISPATCH();
        }

        PREDICTED(FOR_ITER);
        TARGET(FOR_ITER) {
            /* before: [iter]; after: [iter, iter()] *or* [] */
            PyObject *iter = TOP();
            PyObject *next = (*iter->ob_type->tp_iternext)(iter);
            if (next != NULL) {
                PUSH(next);
                PREDICT(STORE_FAST);
                PREDICT(UNPACK_SEQUENCE);
                DISPATCH();
            }
            if (PyErr_Occurred()) {
                if (!PyErr_ExceptionMatches(PyExc_StopIteration))
                    goto error;
                else if (tstate->c_tracefunc != NULL)
                    call_exc_trace(tstate->c_tracefunc, tstate->c_traceobj, tstate, f);
                PyErr_Clear();
            }
            /* iterator ended normally */
            STACKADJ(-1);
            Py_DECREF(iter);
            JUMPBY(oparg);
            PREDICT(POP_BLOCK);
            DISPATCH();
        }

        TARGET(BREAK_LOOP) {
            why = WHY_BREAK;
            goto fast_block_end;
        }

        TARGET(CONTINUE_LOOP) {
            retval = PyLong_FromLong(oparg);
            if (retval == NULL)
                goto error;
            why = WHY_CONTINUE;
            goto fast_block_end;
        }

        TARGET(SETUP_LOOP)
        TARGET(SETUP_EXCEPT)
        TARGET(SETUP_FINALLY) {
            /* NOTE: If you add any new block-setup opcodes that
               are not try/except/finally handlers, you may need
               to update the PyGen_NeedsFinalizing() function.
               */

            PyFrame_BlockSetup(f, opcode, INSTR_OFFSET() + oparg,
                               STACK_LEVEL());
            DISPATCH();
        }

        TARGET(BEFORE_ASYNC_WITH) {
            _Py_IDENTIFIER(__aexit__);
            _Py_IDENTIFIER(__aenter__);

            PyObject *mgr = TOP();
            PyObject *exit = special_lookup(mgr, &PyId___aexit__),
                     *enter;
            PyObject *res;
            if (exit == NULL)
                goto error;
            SET_TOP(exit);
            enter = special_lookup(mgr, &PyId___aenter__);
            Py_DECREF(mgr);
            if (enter == NULL)
                goto error;
            res = _PyObject_CallNoArg(enter);
            Py_DECREF(enter);
            if (res == NULL)
                goto error;
            PUSH(res);
            PREDICT(GET_AWAITABLE);
            DISPATCH();
        }

        TARGET(SETUP_ASYNC_WITH) {
            PyObject *res = POP();
            /* Setup the finally block before pushing the result
               of __aenter__ on the stack. */
            PyFrame_BlockSetup(f, SETUP_FINALLY, INSTR_OFFSET() + oparg,
                               STACK_LEVEL());
            PUSH(res);
            DISPATCH();
        }

        TARGET(SETUP_WITH) {
            _Py_IDENTIFIER(__exit__);
            _Py_IDENTIFIER(__enter__);
            PyObject *mgr = TOP();
            PyObject *enter = special_lookup(mgr, &PyId___enter__), *exit;
            PyObject *res;
            if (enter == NULL)
                goto error;
            exit = special_lookup(mgr, &PyId___exit__);
            if (exit == NULL) {
                Py_DECREF(enter);
                goto error;
            }
            SET_TOP(exit);
            Py_DECREF(mgr);
            res = _PyObject_CallNoArg(enter);
            Py_DECREF(enter);
            if (res == NULL)
                goto error;
            /* Setup the finally block before pushing the result
               of __enter__ on the stack. */
            PyFrame_BlockSetup(f, SETUP_FINALLY, INSTR_OFFSET() + oparg,
                               STACK_LEVEL());

            PUSH(res);
            DISPATCH();
        }

        TARGET(WITH_CLEANUP_START) {
            /* At the top of the stack are 1-6 values indicating
               how/why we entered the finally clause:
               - TOP = None
               - (TOP, SECOND) = (WHY_{RETURN,CONTINUE}), retval
               - TOP = WHY_*; no retval below it
               - (TOP, SECOND, THIRD) = exc_info()
                 (FOURTH, FITH, SIXTH) = previous exception for EXCEPT_HANDLER
               Below them is EXIT, the context.__exit__ bound method.
               In the last case, we must call
                 EXIT(TOP, SECOND, THIRD)
               otherwise we must call
                 EXIT(None, None, None)

               In the first three cases, we remove EXIT from the
               stack, leaving the rest in the same order.  In the
               fourth case, we shift the bottom 3 values of the
               stack down, and replace the empty spot with NULL.

               In addition, if the stack represents an exception,
               *and* the function call returns a 'true' value, we
               push WHY_SILENCED onto the stack.  END_FINALLY will
               then not re-raise the exception.  (But non-local
               gotos should still be resumed.)
            */

            PyObject* stack[3];
            PyObject *exit_func;
            PyObject *exc, *val, *tb, *res;

            val = tb = Py_None;
            exc = TOP();
            if (exc == Py_None) {
                (void)POP();
                exit_func = TOP();
                SET_TOP(exc);
            }
            else if (PyLong_Check(exc)) {
                STACKADJ(-1);
                switch (PyLong_AsLong(exc)) {
                case WHY_RETURN:
                case WHY_CONTINUE:
                    /* Retval in TOP. */
                    exit_func = SECOND();
                    SET_SECOND(TOP());
                    SET_TOP(exc);
                    break;
                default:
                    exit_func = TOP();
                    SET_TOP(exc);
                    break;
                }
                exc = Py_None;
            }
            else {
                PyObject *tp2, *exc2, *tb2;
                PyTryBlock *block;
                val = SECOND();
                tb = THIRD();
                tp2 = FOURTH();
                exc2 = PEEK(5);
                tb2 = PEEK(6);
                exit_func = PEEK(7);
                SET_VALUE(7, tb2);
                SET_VALUE(6, exc2);
                SET_VALUE(5, tp2);
                /* UNWIND_EXCEPT_HANDLER will pop this off. */
                SET_FOURTH(NULL);
                /* We just shifted the stack down, so we have
                   to tell the except handler block that the
                   values are lower than it expects. */
                block = &f->f_blockstack[f->f_iblock - 1];
                assert(block->b_type == EXCEPT_HANDLER);
                block->b_level--;
            }

            stack[0] = exc;
            stack[1] = val;
            stack[2] = tb;
            res = _PyObject_FastCall(exit_func, stack, 3);
            Py_DECREF(exit_func);
            if (res == NULL)
                goto error;

            Py_INCREF(exc); /* Duplicating the exception on the stack */
            PUSH(exc);
            PUSH(res);
            PREDICT(WITH_CLEANUP_FINISH);
            DISPATCH();
        }

        PREDICTED(WITH_CLEANUP_FINISH);
        TARGET(WITH_CLEANUP_FINISH) {
            PyObject *res = POP();
            PyObject *exc = POP();
            int err;

            if (exc != Py_None)
                err = PyObject_IsTrue(res);
            else
                err = 0;

            Py_DECREF(res);
            Py_DECREF(exc);

            if (err < 0)
                goto error;
            else if (err > 0) {
                /* There was an exception and a True return */
                PUSH(PyLong_FromLong((long) WHY_SILENCED));
            }
            PREDICT(END_FINALLY);
            DISPATCH();
        }

        TARGET(LOAD_METHOD) {
            /* Designed to work in tamdem with CALL_METHOD. */
            PyObject *name = GETITEM(names, oparg);
            PyObject *obj = TOP();
            PyObject *meth = NULL;

            int meth_found = _PyObject_GetMethod(obj, name, &meth);

            if (meth == NULL) {
                /* Most likely attribute wasn't found. */
                goto error;
            }

            if (meth_found) {
                /* We can bypass temporary bound method object.
                   meth is unbound method and obj is self.

                   meth | self | arg1 | ... | argN
                 */
                SET_TOP(meth);
                PUSH(obj);  // self
            }
            else {
                /* meth is not an unbound method (but a regular attr, or
                   something was returned by a descriptor protocol).  Set
                   the second element of the stack to NULL, to signal
                   CALL_METHOD that it's not a method call.

                   NULL | meth | arg1 | ... | argN
                */
                SET_TOP(NULL);
                Py_DECREF(obj);
                PUSH(meth);
            }
            DISPATCH();
        }

        TARGET(CALL_METHOD) {
            /* Designed to work in tamdem with LOAD_METHOD. */
            PyObject **sp, *res, *meth;

            sp = stack_pointer;

            meth = PEEK(oparg + 2);
            if (meth == NULL) {
                /* `meth` is NULL when LOAD_METHOD thinks that it's not
                   a method call.

                   Stack layout:

                       ... | NULL | callable | arg1 | ... | argN
                                                            ^- TOP()
                                               ^- (-oparg)
                                    ^- (-oparg-1)
                             ^- (-oparg-2)

                   `callable` will be POPed by call_function.
                   NULL will will be POPed manually later.
                */
                res = call_function(&sp, oparg, NULL);
                stack_pointer = sp;
                (void)POP(); /* POP the NULL. */
            }
            else {
                /* This is a method call.  Stack layout:

                     ... | method | self | arg1 | ... | argN
                                                        ^- TOP()
                                           ^- (-oparg)
                                    ^- (-oparg-1)
                           ^- (-oparg-2)

                  `self` and `method` will be POPed by call_function.
                  We'll be passing `oparg + 1` to call_function, to
                  make it accept the `self` as a first argument.
                */
                res = call_function(&sp, oparg + 1, NULL);
                stack_pointer = sp;
            }

            PUSH(res);
            if (res == NULL)
                goto error;
            DISPATCH();
        }

        PREDICTED(CALL_FUNCTION);
        TARGET(CALL_FUNCTION) {
            PyObject **sp, *res;
            sp = stack_pointer;
            res = call_function(&sp, oparg, NULL);
            stack_pointer = sp;
            PUSH(res);
            if (res == NULL) {
                goto error;
            }
            DISPATCH();
        }

        TARGET(CALL_FUNCTION_KW) {
            PyObject **sp, *res, *names;

            names = POP();
            assert(PyTuple_CheckExact(names) && PyTuple_GET_SIZE(names) <= oparg);
            sp = stack_pointer;
            res = call_function(&sp, oparg, names);
            stack_pointer = sp;
            PUSH(res);
            Py_DECREF(names);

            if (res == NULL) {
                goto error;
            }
            DISPATCH();
        }

        TARGET(CALL_FUNCTION_EX) {
            PyObject *func, *callargs, *kwargs = NULL, *result;
            if (oparg & 0x01) {
                kwargs = POP();
                if (!PyDict_CheckExact(kwargs)) {
                    PyObject *d = PyDict_New();
                    if (d == NULL)
                        goto error;
                    if (PyDict_Update(d, kwargs) != 0) {
                        Py_DECREF(d);
                        /* PyDict_Update raises attribute
                         * error (percolated from an attempt
                         * to get 'keys' attribute) instead of
                         * a type error if its second argument
                         * is not a mapping.
                         */
                        if (PyErr_ExceptionMatches(PyExc_AttributeError)) {
                            format_kwargs_mapping_error(SECOND(), kwargs);
                        }
                        Py_DECREF(kwargs);
                        goto error;
                    }
                    Py_DECREF(kwargs);
                    kwargs = d;
                }
                assert(PyDict_CheckExact(kwargs));
            }
            callargs = POP();
            func = TOP();
            if (!PyTuple_CheckExact(callargs)) {
                if (check_args_iterable(func, callargs) < 0) {
                    Py_DECREF(callargs);
                    goto error;
                }
                Py_SETREF(callargs, PySequence_Tuple(callargs));
                if (callargs == NULL) {
                    goto error;
                }
            }
            assert(PyTuple_CheckExact(callargs));

            result = do_call_core(func, callargs, kwargs);
            Py_DECREF(func);
            Py_DECREF(callargs);
            Py_XDECREF(kwargs);

            SET_TOP(result);
            if (result == NULL) {
                goto error;
            }
            DISPATCH();
        }

        TARGET(MAKE_FUNCTION) {
            PyObject *qualname = POP();
            PyObject *codeobj = POP();
            PyFunctionObject *func = (PyFunctionObject *)
                PyFunction_NewWithQualName(codeobj, f->f_globals, qualname);

            Py_DECREF(codeobj);
            Py_DECREF(qualname);
            if (func == NULL) {
                goto error;
            }

            if (oparg & 0x08) {
                assert(PyTuple_CheckExact(TOP()));
                func ->func_closure = POP();
            }
            if (oparg & 0x04) {
                assert(PyDict_CheckExact(TOP()));
                func->func_annotations = POP();
            }
            if (oparg & 0x02) {
                assert(PyDict_CheckExact(TOP()));
                func->func_kwdefaults = POP();
            }
            if (oparg & 0x01) {
                assert(PyTuple_CheckExact(TOP()));
                func->func_defaults = POP();
            }

            PUSH((PyObject *)func);
            DISPATCH();
        }

        TARGET(BUILD_SLICE) {
            PyObject *start, *stop, *step, *slice;
            if (oparg == 3)
                step = POP();
            else
                step = NULL;
            stop = POP();
            start = TOP();
            slice = PySlice_New(start, stop, step);
            Py_DECREF(start);
            Py_DECREF(stop);
            Py_XDECREF(step);
            SET_TOP(slice);
            if (slice == NULL)
                goto error;
            DISPATCH();
        }

        TARGET(FORMAT_VALUE) {
            /* Handles f-string value formatting. */
            PyObject *result;
            PyObject *fmt_spec;
            PyObject *value;
            PyObject *(*conv_fn)(PyObject *);
            int which_conversion = oparg & FVC_MASK;
            int have_fmt_spec = (oparg & FVS_MASK) == FVS_HAVE_SPEC;

            fmt_spec = have_fmt_spec ? POP() : NULL;
            value = POP();

            /* See if any conversion is specified. */
            switch (which_conversion) {
            case FVC_STR:   conv_fn = PyObject_Str;   break;
            case FVC_REPR:  conv_fn = PyObject_Repr;  break;
            case FVC_ASCII: conv_fn = PyObject_ASCII; break;

            /* Must be 0 (meaning no conversion), since only four
               values are allowed by (oparg & FVC_MASK). */
            default:        conv_fn = NULL;           break;
            }

            /* If there's a conversion function, call it and replace
               value with that result. Otherwise, just use value,
               without conversion. */
            if (conv_fn != NULL) {
                result = conv_fn(value);
                Py_DECREF(value);
                if (result == NULL) {
                    Py_XDECREF(fmt_spec);
                    goto error;
                }
                value = result;
            }

            /* If value is a unicode object, and there's no fmt_spec,
               then we know the result of format(value) is value
               itself. In that case, skip calling format(). I plan to
               move this optimization in to PyObject_Format()
               itself. */
            if (PyUnicode_CheckExact(value) && fmt_spec == NULL) {
                /* Do nothing, just transfer ownership to result. */
                result = value;
            } else {
                /* Actually call format(). */
                result = PyObject_Format(value, fmt_spec);
                Py_DECREF(value);
                Py_XDECREF(fmt_spec);
                if (result == NULL) {
                    goto error;
                }
            }

            PUSH(result);
            DISPATCH();
        }

        TARGET(EXTENDED_ARG) {
            int oldoparg = oparg;
            NEXTOPARG();
            oparg |= oldoparg << 8;
            goto dispatch_opcode;
        }


#if USE_COMPUTED_GOTOS
        _unknown_opcode:
#endif
        default:
            fprintf(stderr,
                "XXX lineno: %d, opcode: %d\n",
                PyFrame_GetLineNumber(f),
                opcode);
            PyErr_SetString(PyExc_SystemError, "unknown opcode");
            goto error;

        } /* switch */

        /* This should never be reached. Every opcode should end with DISPATCH()
           or goto error. */
        Py_UNREACHABLE();

error:

        assert(why == WHY_NOT);
        why = WHY_EXCEPTION;

        /* Double-check exception status. */
#ifdef NDEBUG
        if (!PyErr_Occurred())
            PyErr_SetString(PyExc_SystemError,
                            "error return without exception set");
#else
        assert(PyErr_Occurred());
#endif

        /* Log traceback info. */
        PyTraceBack_Here(f);

        if (tstate->c_tracefunc != NULL)
            call_exc_trace(tstate->c_tracefunc, tstate->c_traceobj,
                           tstate, f);

fast_block_end:
        assert(why != WHY_NOT);

        /* Unwind stacks if a (pseudo) exception occurred */
        while (why != WHY_NOT && f->f_iblock > 0) {
            /* Peek at the current block. */
            PyTryBlock *b = &f->f_blockstack[f->f_iblock - 1];

            assert(why != WHY_YIELD);
            if (b->b_type == SETUP_LOOP && why == WHY_CONTINUE) {
                why = WHY_NOT;
                JUMPTO(PyLong_AS_LONG(retval));
                Py_DECREF(retval);
                break;
            }
            /* Now we have to pop the block. */
            f->f_iblock--;

            if (b->b_type == EXCEPT_HANDLER) {
                UNWIND_EXCEPT_HANDLER(b);
                continue;
            }
            UNWIND_BLOCK(b);
            if (b->b_type == SETUP_LOOP && why == WHY_BREAK) {
                why = WHY_NOT;
                JUMPTO(b->b_handler);
                break;
            }
            if (why == WHY_EXCEPTION && (b->b_type == SETUP_EXCEPT
                || b->b_type == SETUP_FINALLY)) {
                PyObject *exc, *val, *tb;
                int handler = b->b_handler;
                _PyErr_StackItem *exc_info = tstate->exc_info;
                /* Beware, this invalidates all b->b_* fields */
                PyFrame_BlockSetup(f, EXCEPT_HANDLER, -1, STACK_LEVEL());
                PUSH(exc_info->exc_traceback);
                PUSH(exc_info->exc_value);
                if (exc_info->exc_type != NULL) {
                    PUSH(exc_info->exc_type);
                }
                else {
                    Py_INCREF(Py_None);
                    PUSH(Py_None);
                }
                PyErr_Fetch(&exc, &val, &tb);
                /* Make the raw exception data
                   available to the handler,
                   so a program can emulate the
                   Python main loop. */
                PyErr_NormalizeException(
                    &exc, &val, &tb);
                if (tb != NULL)
                    PyException_SetTraceback(val, tb);
                else
                    PyException_SetTraceback(val, Py_None);
                Py_INCREF(exc);
                exc_info->exc_type = exc;
                Py_INCREF(val);
                exc_info->exc_value = val;
                exc_info->exc_traceback = tb;
                if (tb == NULL)
                    tb = Py_None;
                Py_INCREF(tb);
                PUSH(tb);
                PUSH(val);
                PUSH(exc);
                why = WHY_NOT;
                JUMPTO(handler);
                break;
            }
            if (b->b_type == SETUP_FINALLY) {
                if (why & (WHY_RETURN | WHY_CONTINUE))
                    PUSH(retval);
                PUSH(PyLong_FromLong((long)why));
                why = WHY_NOT;
                JUMPTO(b->b_handler);
                break;
            }
        } /* unwind stack */

        /* End the loop if we still have an error (or return) */

        if (why != WHY_NOT)
            break;

        assert(!PyErr_Occurred());

    } /* main loop */

    assert(why != WHY_YIELD);
    /* Pop remaining stack entries. */
    while (!EMPTY()) {
        PyObject *o = POP();
        Py_XDECREF(o);
    }

    if (why != WHY_RETURN)
        retval = NULL;

    assert((retval != NULL) ^ (PyErr_Occurred() != NULL));

fast_yield:

    if (tstate->use_tracing) {
        if (tstate->c_tracefunc) {
            if (why == WHY_RETURN || why == WHY_YIELD) {
                if (call_trace(tstate->c_tracefunc, tstate->c_traceobj,
                               tstate, f,
                               PyTrace_RETURN, retval)) {
                    Py_CLEAR(retval);
                    why = WHY_EXCEPTION;
                }
            }
            else if (why == WHY_EXCEPTION) {
                call_trace_protected(tstate->c_tracefunc, tstate->c_traceobj,
                                     tstate, f,
                                     PyTrace_RETURN, NULL);
            }
        }
        if (tstate->c_profilefunc) {
            if (why == WHY_EXCEPTION)
                call_trace_protected(tstate->c_profilefunc,
                                     tstate->c_profileobj,
                                     tstate, f,
                                     PyTrace_RETURN, NULL);
            else if (call_trace(tstate->c_profilefunc, tstate->c_profileobj,
                                tstate, f,
                                PyTrace_RETURN, retval)) {
                Py_CLEAR(retval);
                /* why = WHY_EXCEPTION; useless yet but cause compiler warnings */
            }
        }
    }

    /* pop frame */
exit_eval_frame:
    if (PyDTrace_FUNCTION_RETURN_ENABLED())
        dtrace_function_return(f);
    Py_LeaveRecursiveCall();
    f->f_executing = 0;
    tstate->frame = f->f_back;

    return _Py_CheckFunctionResult(NULL, retval, "PyEval_EvalFrameEx");
}

static void
format_missing(const char *kind, PyCodeObject *co, PyObject *names)
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
    PyErr_Format(PyExc_TypeError,
                 "%U() missing %i required %s argument%s: %U",
                 co->co_name,
                 len,
                 kind,
                 len == 1 ? "" : "s",
                 name_str);
    Py_DECREF(name_str);
}

static void
missing_arguments(PyCodeObject *co, Py_ssize_t missing, Py_ssize_t defcount,
                  PyObject **fastlocals)
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
        if (GETLOCAL(i) == NULL) {
            PyObject *raw = PyTuple_GET_ITEM(co->co_varnames, i);
            PyObject *name = PyObject_Repr(raw);
            if (name == NULL) {
                Py_DECREF(missing_names);
                return;
            }
            PyList_SET_ITEM(missing_names, j++, name);
        }
    }
    assert(j == missing);
    format_missing(kind, co, missing_names);
    Py_DECREF(missing_names);
}

static void
too_many_positional(PyCodeObject *co, Py_ssize_t given, Py_ssize_t defcount,
                    PyObject **fastlocals)
{
    int plural;
    Py_ssize_t kwonly_given = 0;
    Py_ssize_t i;
    PyObject *sig, *kwonly_sig;
    Py_ssize_t co_argcount = co->co_argcount;

    assert((co->co_flags & CO_VARARGS) == 0);
    /* Count missing keyword-only args. */
    for (i = co_argcount; i < co_argcount + co->co_kwonlyargcount; i++) {
        if (GETLOCAL(i) != NULL) {
            kwonly_given++;
        }
    }
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
    PyErr_Format(PyExc_TypeError,
                 "%U() takes %U positional argument%s but %zd%U %s given",
                 co->co_name,
                 sig,
                 plural ? "s" : "",
                 given,
                 kwonly_sig,
                 given == 1 && !kwonly_given ? "was" : "were");
    Py_DECREF(sig);
    Py_DECREF(kwonly_sig);
}

/* This is gonna seem *real weird*, but if you put some other code between
   PyEval_EvalFrame() and _PyEval_EvalFrameDefault() you will need to adjust
   the test in the if statements in Misc/gdbinit (pystack and pystackv). */

PyObject *
_PyEval_EvalCodeWithName(PyObject *_co, PyObject *globals, PyObject *locals,
           PyObject *const *args, Py_ssize_t argcount,
           PyObject *const *kwnames, PyObject *const *kwargs,
           Py_ssize_t kwcount, int kwstep,
           PyObject *const *defs, Py_ssize_t defcount,
           PyObject *kwdefs, PyObject *closure,
           PyObject *name, PyObject *qualname)
{
    PyCodeObject* co = (PyCodeObject*)_co;
    PyFrameObject *f;
    PyObject *retval = NULL;
    PyObject **fastlocals, **freevars;
    PyThreadState *tstate;
    PyObject *x, *u;
    const Py_ssize_t total_args = co->co_argcount + co->co_kwonlyargcount;
    Py_ssize_t i, n;
    PyObject *kwdict;

    if (globals == NULL) {
        PyErr_SetString(PyExc_SystemError,
                        "PyEval_EvalCodeEx: NULL globals");
        return NULL;
    }

    /* Create the frame */
    tstate = PyThreadState_GET();
    assert(tstate != NULL);
    f = _PyFrame_New_NoTrack(tstate, co, globals, locals);
    if (f == NULL) {
        return NULL;
    }
    fastlocals = f->f_localsplus;
    freevars = f->f_localsplus + co->co_nlocals;

    /* Create a dictionary for keyword parameters (**kwags) */
    if (co->co_flags & CO_VARKEYWORDS) {
        kwdict = PyDict_New();
        if (kwdict == NULL)
            goto fail;
        i = total_args;
        if (co->co_flags & CO_VARARGS) {
            i++;
        }
        SETLOCAL(i, kwdict);
    }
    else {
        kwdict = NULL;
    }

    /* Copy positional arguments into local variables */
    if (argcount > co->co_argcount) {
        n = co->co_argcount;
    }
    else {
        n = argcount;
    }
    for (i = 0; i < n; i++) {
        x = args[i];
        Py_INCREF(x);
        SETLOCAL(i, x);
    }

    /* Pack other positional arguments into the *args argument */
    if (co->co_flags & CO_VARARGS) {
        u = PyTuple_New(argcount - n);
        if (u == NULL) {
            goto fail;
        }
        SETLOCAL(total_args, u);
        for (i = n; i < argcount; i++) {
            x = args[i];
            Py_INCREF(x);
            PyTuple_SET_ITEM(u, i-n, x);
        }
    }

    /* Handle keyword arguments passed as two strided arrays */
    kwcount *= kwstep;
    for (i = 0; i < kwcount; i += kwstep) {
        PyObject **co_varnames;
        PyObject *keyword = kwnames[i];
        PyObject *value = kwargs[i];
        Py_ssize_t j;

        if (keyword == NULL || !PyUnicode_Check(keyword)) {
            PyErr_Format(PyExc_TypeError,
                         "%U() keywords must be strings",
                         co->co_name);
            goto fail;
        }

        /* Speed hack: do raw pointer compares. As names are
           normally interned this should almost always hit. */
        co_varnames = ((PyTupleObject *)(co->co_varnames))->ob_item;
        for (j = 0; j < total_args; j++) {
            PyObject *name = co_varnames[j];
            if (name == keyword) {
                goto kw_found;
            }
        }

        /* Slow fallback, just in case */
        for (j = 0; j < total_args; j++) {
            PyObject *name = co_varnames[j];
            int cmp = PyObject_RichCompareBool( keyword, name, Py_EQ);
            if (cmp > 0) {
                goto kw_found;
            }
            else if (cmp < 0) {
                goto fail;
            }
        }

        assert(j >= total_args);
        if (kwdict == NULL) {
            PyErr_Format(PyExc_TypeError,
                         "%U() got an unexpected keyword argument '%S'",
                         co->co_name, keyword);
            goto fail;
        }

        if (PyDict_SetItem(kwdict, keyword, value) == -1) {
            goto fail;
        }
        continue;

      kw_found:
        if (GETLOCAL(j) != NULL) {
            PyErr_Format(PyExc_TypeError,
                         "%U() got multiple values for argument '%S'",
                         co->co_name, keyword);
            goto fail;
        }
        Py_INCREF(value);
        SETLOCAL(j, value);
    }

    /* Check the number of positional arguments */
    if (argcount > co->co_argcount && !(co->co_flags & CO_VARARGS)) {
        too_many_positional(co, argcount, defcount, fastlocals);
        goto fail;
    }

    /* Add missing positional arguments (copy default values from defs) */
    if (argcount < co->co_argcount) {
        Py_ssize_t m = co->co_argcount - defcount;
        Py_ssize_t missing = 0;
        for (i = argcount; i < m; i++) {
            if (GETLOCAL(i) == NULL) {
                missing++;
            }
        }
        if (missing) {
            missing_arguments(co, missing, defcount, fastlocals);
            goto fail;
        }
        if (n > m)
            i = n - m;
        else
            i = 0;
        for (; i < defcount; i++) {
            if (GETLOCAL(m+i) == NULL) {
                PyObject *def = defs[i];
                Py_INCREF(def);
                SETLOCAL(m+i, def);
            }
        }
    }

    /* Add missing keyword arguments (copy default values from kwdefs) */
    if (co->co_kwonlyargcount > 0) {
        Py_ssize_t missing = 0;
        for (i = co->co_argcount; i < total_args; i++) {
            PyObject *name;
            if (GETLOCAL(i) != NULL)
                continue;
            name = PyTuple_GET_ITEM(co->co_varnames, i);
            if (kwdefs != NULL) {
                PyObject *def = PyDict_GetItem(kwdefs, name);
                if (def) {
                    Py_INCREF(def);
                    SETLOCAL(i, def);
                    continue;
                }
            }
            missing++;
        }
        if (missing) {
            missing_arguments(co, missing, -1, fastlocals);
            goto fail;
        }
    }

    /* Allocate and initialize storage for cell vars, and copy free
       vars into frame. */
    for (i = 0; i < PyTuple_GET_SIZE(co->co_cellvars); ++i) {
        PyObject *c;
        Py_ssize_t arg;
        /* Possibly account for the cell variable being an argument. */
        if (co->co_cell2arg != NULL &&
            (arg = co->co_cell2arg[i]) != CO_CELL_NOT_AN_ARG) {
            c = PyCell_New(GETLOCAL(arg));
            /* Clear the local copy. */
            SETLOCAL(arg, NULL);
        }
        else {
            c = PyCell_New(NULL);
        }
        if (c == NULL)
            goto fail;
        SETLOCAL(co->co_nlocals + i, c);
    }

    /* Copy closure variables to free variables */
    for (i = 0; i < PyTuple_GET_SIZE(co->co_freevars); ++i) {
        PyObject *o = PyTuple_GET_ITEM(closure, i);
        Py_INCREF(o);
        freevars[PyTuple_GET_SIZE(co->co_cellvars) + i] = o;
    }

    /* Handle generator/coroutine/asynchronous generator */
    if (co->co_flags & (CO_GENERATOR | CO_COROUTINE | CO_ASYNC_GENERATOR)) {
        PyObject *gen;
        PyObject *coro_wrapper = tstate->coroutine_wrapper;
        int is_coro = co->co_flags & CO_COROUTINE;

        if (is_coro && tstate->in_coroutine_wrapper) {
            assert(coro_wrapper != NULL);
            PyErr_Format(PyExc_RuntimeError,
                         "coroutine wrapper %.200R attempted "
                         "to recursively wrap %.200R",
                         coro_wrapper,
                         co);
            goto fail;
        }

        /* Don't need to keep the reference to f_back, it will be set
         * when the generator is resumed. */
        Py_CLEAR(f->f_back);

        /* Create a new generator that owns the ready to run frame
         * and return that as the value. */
        if (is_coro) {
            gen = PyCoro_New(f, name, qualname);
        } else if (co->co_flags & CO_ASYNC_GENERATOR) {
            gen = PyAsyncGen_New(f, name, qualname);
        } else {
            gen = PyGen_NewWithQualName(f, name, qualname);
        }
        if (gen == NULL) {
            return NULL;
        }

        _PyObject_GC_TRACK(f);

        if (is_coro && coro_wrapper != NULL) {
            PyObject *wrapped;
            tstate->in_coroutine_wrapper = 1;
            wrapped = PyObject_CallFunction(coro_wrapper, "N", gen);
            tstate->in_coroutine_wrapper = 0;
            return wrapped;
        }

        return gen;
    }

    retval = PyEval_EvalFrameEx(f,0);

fail: /* Jump here from prelude on failure */

    /* decref'ing the frame can cause __del__ methods to get invoked,
       which can call back into Python.  While we're done with the
       current Python frame (f), the associated C stack is still in use,
       so recursion_depth must be boosted for the duration.
    */
    assert(tstate != NULL);
    if (Py_REFCNT(f) > 1) {
        Py_DECREF(f);
        _PyObject_GC_TRACK(f);
    }
    else {
        ++tstate->recursion_depth;
        Py_DECREF(f);
        --tstate->recursion_depth;
    }
    return retval;
}

PyObject *
PyEval_EvalCodeEx(PyObject *_co, PyObject *globals, PyObject *locals,
                  PyObject *const *args, int argcount,
                  PyObject *const *kws, int kwcount,
                  PyObject *const *defs, int defcount,
                  PyObject *kwdefs, PyObject *closure)
{
    return _PyEval_EvalCodeWithName(_co, globals, locals,
                                    args, argcount,
                                    kws, kws != NULL ? kws + 1 : NULL,
                                    kwcount, 2,
                                    defs, defcount,
                                    kwdefs, closure,
                                    NULL, NULL);
}

static PyObject *
special_lookup(PyObject *o, _Py_Identifier *id)
{
    PyObject *res;
    res = _PyObject_LookupSpecial(o, id);
    if (res == NULL && !PyErr_Occurred()) {
        PyErr_SetObject(PyExc_AttributeError, id->object);
        return NULL;
    }
    return res;
}


/* Logic for the raise statement (too complicated for inlining).
   This *consumes* a reference count to each of its arguments. */
static int
do_raise(PyObject *exc, PyObject *cause)
{
    PyObject *type = NULL, *value = NULL;

    if (exc == NULL) {
        /* Reraise */
        PyThreadState *tstate = PyThreadState_GET();
        _PyErr_StackItem *exc_info = _PyErr_GetTopmostException(tstate);
        PyObject *tb;
        type = exc_info->exc_type;
        value = exc_info->exc_value;
        tb = exc_info->exc_traceback;
        if (type == Py_None || type == NULL) {
            PyErr_SetString(PyExc_RuntimeError,
                            "No active exception to reraise");
            return 0;
        }
        Py_XINCREF(type);
        Py_XINCREF(value);
        Py_XINCREF(tb);
        PyErr_Restore(type, value, tb);
        return 1;
    }

    /* We support the following forms of raise:
       raise
       raise <instance>
       raise <type> */

    if (PyExceptionClass_Check(exc)) {
        type = exc;
        value = _PyObject_CallNoArg(exc);
        if (value == NULL)
            goto raise_error;
        if (!PyExceptionInstance_Check(value)) {
            PyErr_Format(PyExc_TypeError,
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
        PyErr_SetString(PyExc_TypeError,
                        "exceptions must derive from BaseException");
        goto raise_error;
    }

    assert(type != NULL);
    assert(value != NULL);

    if (cause) {
        PyObject *fixed_cause;
        if (PyExceptionClass_Check(cause)) {
            fixed_cause = _PyObject_CallNoArg(cause);
            if (fixed_cause == NULL)
                goto raise_error;
            Py_DECREF(cause);
        }
        else if (PyExceptionInstance_Check(cause)) {
            fixed_cause = cause;
        }
        else if (cause == Py_None) {
            Py_DECREF(cause);
            fixed_cause = NULL;
        }
        else {
            PyErr_SetString(PyExc_TypeError,
                            "exception causes must derive from "
                            "BaseException");
            goto raise_error;
        }
        PyException_SetCause(value, fixed_cause);
    }

    PyErr_SetObject(type, value);
    /* PyErr_SetObject incref's its arguments */
    Py_DECREF(value);
    Py_DECREF(type);
    return 0;

raise_error:
    Py_XDECREF(value);
    Py_XDECREF(type);
    Py_XDECREF(cause);
    return 0;
}

/* Iterate v argcnt times and store the results on the stack (via decreasing
   sp).  Return 1 for success, 0 if error.

   If argcntafter == -1, do a simple unpack. If it is >= 0, do an unpack
   with a variable target.
*/

static int
unpack_iterable(PyObject *v, int argcnt, int argcntafter, PyObject **sp)
{
    int i = 0, j = 0;
    Py_ssize_t ll = 0;
    PyObject *it;  /* iter(v) */
    PyObject *w;
    PyObject *l = NULL; /* variable list */

    assert(v != NULL);

    it = PyObject_GetIter(v);
    if (it == NULL) {
        if (PyErr_ExceptionMatches(PyExc_TypeError) &&
            v->ob_type->tp_iter == NULL && !PySequence_Check(v))
        {
            PyErr_Format(PyExc_TypeError,
                         "cannot unpack non-iterable %.200s object",
                         v->ob_type->tp_name);
        }
        return 0;
    }

    for (; i < argcnt; i++) {
        w = PyIter_Next(it);
        if (w == NULL) {
            /* Iterator done, via error or exhaustion. */
            if (!PyErr_Occurred()) {
                if (argcntafter == -1) {
                    PyErr_Format(PyExc_ValueError,
                        "not enough values to unpack (expected %d, got %d)",
                        argcnt, i);
                }
                else {
                    PyErr_Format(PyExc_ValueError,
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
            if (PyErr_Occurred())
                goto Error;
            Py_DECREF(it);
            return 1;
        }
        Py_DECREF(w);
        PyErr_Format(PyExc_ValueError,
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
        PyErr_Format(PyExc_ValueError,
            "not enough values to unpack (expected at least %d, got %zd)",
            argcnt + argcntafter, argcnt + ll);
        goto Error;
    }

    /* Pop the "after-variable" args off the list. */
    for (j = argcntafter; j > 0; j--, i++) {
        *--sp = PyList_GET_ITEM(l, ll - j);
    }
    /* Resize the list. */
    Py_SIZE(l) = ll - argcntafter;
    Py_DECREF(it);
    return 1;

Error:
    for (; i > 0; i--, sp++)
        Py_DECREF(*sp);
    Py_XDECREF(it);
    return 0;
}


#ifdef LLTRACE
static int
prtrace(PyObject *v, const char *str)
{
    printf("%s ", str);
    if (PyObject_Print(v, stdout, 0) != 0)
        PyErr_Clear(); /* Don't know what else to do */
    printf("\n");
    return 1;
}
#endif

static void
call_exc_trace(Py_tracefunc func, PyObject *self,
               PyThreadState *tstate, PyFrameObject *f)
{
    PyObject *type, *value, *traceback, *orig_traceback, *arg;
    int err;
    PyErr_Fetch(&type, &value, &orig_traceback);
    if (value == NULL) {
        value = Py_None;
        Py_INCREF(value);
    }
    PyErr_NormalizeException(&type, &value, &orig_traceback);
    traceback = (orig_traceback != NULL) ? orig_traceback : Py_None;
    arg = PyTuple_Pack(3, type, value, traceback);
    if (arg == NULL) {
        PyErr_Restore(type, value, orig_traceback);
        return;
    }
    err = call_trace(func, self, tstate, f, PyTrace_EXCEPTION, arg);
    Py_DECREF(arg);
    if (err == 0)
        PyErr_Restore(type, value, orig_traceback);
    else {
        Py_XDECREF(type);
        Py_XDECREF(value);
        Py_XDECREF(orig_traceback);
    }
}

static int
call_trace_protected(Py_tracefunc func, PyObject *obj,
                     PyThreadState *tstate, PyFrameObject *frame,
                     int what, PyObject *arg)
{
    PyObject *type, *value, *traceback;
    int err;
    PyErr_Fetch(&type, &value, &traceback);
    err = call_trace(func, obj, tstate, frame, what, arg);
    if (err == 0)
    {
        PyErr_Restore(type, value, traceback);
        return 0;
    }
    else {
        Py_XDECREF(type);
        Py_XDECREF(value);
        Py_XDECREF(traceback);
        return -1;
    }
}

static int
call_trace(Py_tracefunc func, PyObject *obj,
           PyThreadState *tstate, PyFrameObject *frame,
           int what, PyObject *arg)
{
    int result;
    if (tstate->tracing)
        return 0;
    tstate->tracing++;
    tstate->use_tracing = 0;
    result = func(obj, frame, what, arg);
    tstate->use_tracing = ((tstate->c_tracefunc != NULL)
                           || (tstate->c_profilefunc != NULL));
    tstate->tracing--;
    return result;
}

PyObject *
_PyEval_CallTracing(PyObject *func, PyObject *args)
{
    PyThreadState *tstate = PyThreadState_GET();
    int save_tracing = tstate->tracing;
    int save_use_tracing = tstate->use_tracing;
    PyObject *result;

    tstate->tracing = 0;
    tstate->use_tracing = ((tstate->c_tracefunc != NULL)
                           || (tstate->c_profilefunc != NULL));
    result = PyObject_Call(func, args, NULL);
    tstate->tracing = save_tracing;
    tstate->use_tracing = save_use_tracing;
    return result;
}

/* See Objects/lnotab_notes.txt for a description of how tracing works. */
static int
maybe_call_line_trace(Py_tracefunc func, PyObject *obj,
                      PyThreadState *tstate, PyFrameObject *frame,
                      int *instr_lb, int *instr_ub, int *instr_prev)
{
    int result = 0;
    int line = frame->f_lineno;

    /* If the last instruction executed isn't in the current
       instruction window, reset the window.
    */
    if (frame->f_lasti < *instr_lb || frame->f_lasti >= *instr_ub) {
        PyAddrPair bounds;
        line = _PyCode_CheckLineNumber(frame->f_code, frame->f_lasti,
                                       &bounds);
        *instr_lb = bounds.ap_lower;
        *instr_ub = bounds.ap_upper;
    }
    /* If the last instruction falls at the start of a line or if it
       represents a jump backwards, update the frame's line number and
       then call the trace function if we're tracing source lines.
    */
    if ((frame->f_lasti == *instr_lb || frame->f_lasti < *instr_prev)) {
        frame->f_lineno = line;
        if (frame->f_trace_lines) {
            result = call_trace(func, obj, tstate, frame, PyTrace_LINE, Py_None);
        }
    }
    /* Always emit an opcode event if we're tracing all opcodes. */
    if (frame->f_trace_opcodes) {
        result = call_trace(func, obj, tstate, frame, PyTrace_OPCODE, Py_None);
    }
    *instr_prev = frame->f_lasti;
    return result;
}

void
PyEval_SetProfile(Py_tracefunc func, PyObject *arg)
{
    PyThreadState *tstate = PyThreadState_GET();
    PyObject *temp = tstate->c_profileobj;
    Py_XINCREF(arg);
    tstate->c_profilefunc = NULL;
    tstate->c_profileobj = NULL;
    /* Must make sure that tracing is not ignored if 'temp' is freed */
    tstate->use_tracing = tstate->c_tracefunc != NULL;
    Py_XDECREF(temp);
    tstate->c_profilefunc = func;
    tstate->c_profileobj = arg;
    /* Flag that tracing or profiling is turned on */
    tstate->use_tracing = (func != NULL) || (tstate->c_tracefunc != NULL);
}

void
PyEval_SetTrace(Py_tracefunc func, PyObject *arg)
{
    PyThreadState *tstate = PyThreadState_GET();
    PyObject *temp = tstate->c_traceobj;
    _Py_TracingPossible += (func != NULL) - (tstate->c_tracefunc != NULL);
    Py_XINCREF(arg);
    tstate->c_tracefunc = NULL;
    tstate->c_traceobj = NULL;
    /* Must make sure that profiling is not ignored if 'temp' is freed */
    tstate->use_tracing = tstate->c_profilefunc != NULL;
    Py_XDECREF(temp);
    tstate->c_tracefunc = func;
    tstate->c_traceobj = arg;
    /* Flag that tracing or profiling is turned on */
    tstate->use_tracing = ((func != NULL)
                           || (tstate->c_profilefunc != NULL));
}

void
_PyEval_SetCoroutineOriginTrackingDepth(int new_depth)
{
    assert(new_depth >= 0);
    PyThreadState *tstate = PyThreadState_GET();
    tstate->coroutine_origin_tracking_depth = new_depth;
}

int
_PyEval_GetCoroutineOriginTrackingDepth(void)
{
    PyThreadState *tstate = PyThreadState_GET();
    return tstate->coroutine_origin_tracking_depth;
}

void
_PyEval_SetCoroutineWrapper(PyObject *wrapper)
{
    PyThreadState *tstate = PyThreadState_GET();

    Py_XINCREF(wrapper);
    Py_XSETREF(tstate->coroutine_wrapper, wrapper);
}

PyObject *
_PyEval_GetCoroutineWrapper(void)
{
    PyThreadState *tstate = PyThreadState_GET();
    return tstate->coroutine_wrapper;
}

void
_PyEval_SetAsyncGenFirstiter(PyObject *firstiter)
{
    PyThreadState *tstate = PyThreadState_GET();

    Py_XINCREF(firstiter);
    Py_XSETREF(tstate->async_gen_firstiter, firstiter);
}

PyObject *
_PyEval_GetAsyncGenFirstiter(void)
{
    PyThreadState *tstate = PyThreadState_GET();
    return tstate->async_gen_firstiter;
}

void
_PyEval_SetAsyncGenFinalizer(PyObject *finalizer)
{
    PyThreadState *tstate = PyThreadState_GET();

    Py_XINCREF(finalizer);
    Py_XSETREF(tstate->async_gen_finalizer, finalizer);
}

PyObject *
_PyEval_GetAsyncGenFinalizer(void)
{
    PyThreadState *tstate = PyThreadState_GET();
    return tstate->async_gen_finalizer;
}

PyObject *
PyEval_GetBuiltins(void)
{
    PyFrameObject *current_frame = PyEval_GetFrame();
    if (current_frame == NULL)
        return PyThreadState_GET()->interp->builtins;
    else
        return current_frame->f_builtins;
}

PyObject *
PyEval_GetLocals(void)
{
    PyFrameObject *current_frame = PyEval_GetFrame();
    if (current_frame == NULL) {
        PyErr_SetString(PyExc_SystemError, "frame does not exist");
        return NULL;
    }

    if (PyFrame_FastToLocalsWithError(current_frame) < 0)
        return NULL;

    assert(current_frame->f_locals != NULL);
    return current_frame->f_locals;
}

PyObject *
PyEval_GetGlobals(void)
{
    PyFrameObject *current_frame = PyEval_GetFrame();
    if (current_frame == NULL)
        return NULL;

    assert(current_frame->f_globals != NULL);
    return current_frame->f_globals;
}

PyFrameObject *
PyEval_GetFrame(void)
{
    PyThreadState *tstate = PyThreadState_GET();
    return _PyThreadState_GetFrame(tstate);
}

int
PyEval_MergeCompilerFlags(PyCompilerFlags *cf)
{
    PyFrameObject *current_frame = PyEval_GetFrame();
    int result = cf->cf_flags != 0;

    if (current_frame != NULL) {
        const int codeflags = current_frame->f_code->co_flags;
        const int compilerflags = codeflags & PyCF_MASK;
        if (compilerflags) {
            result = 1;
            cf->cf_flags |= compilerflags;
        }
#if 0 /* future keyword */
        if (codeflags & CO_GENERATOR_ALLOWED) {
            result = 1;
            cf->cf_flags |= CO_GENERATOR_ALLOWED;
        }
#endif
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
        return func->ob_type->tp_name;
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

#define C_TRACE(x, call) \
if (tstate->use_tracing && tstate->c_profilefunc) { \
    if (call_trace(tstate->c_profilefunc, tstate->c_profileobj, \
        tstate, tstate->frame, \
        PyTrace_C_CALL, func)) { \
        x = NULL; \
    } \
    else { \
        x = call; \
        if (tstate->c_profilefunc != NULL) { \
            if (x == NULL) { \
                call_trace_protected(tstate->c_profilefunc, \
                    tstate->c_profileobj, \
                    tstate, tstate->frame, \
                    PyTrace_C_EXCEPTION, func); \
                /* XXX should pass (type, value, tb) */ \
            } else { \
                if (call_trace(tstate->c_profilefunc, \
                    tstate->c_profileobj, \
                    tstate, tstate->frame, \
                    PyTrace_C_RETURN, func)) { \
                    Py_DECREF(x); \
                    x = NULL; \
                } \
            } \
        } \
    } \
} else { \
    x = call; \
    }

/* Issue #29227: Inline call_function() into _PyEval_EvalFrameDefault()
   to reduce the stack consumption. */
Py_LOCAL_INLINE(PyObject *) _Py_HOT_FUNCTION
call_function(PyObject ***pp_stack, Py_ssize_t oparg, PyObject *kwnames)
{
    PyObject **pfunc = (*pp_stack) - oparg - 1;
    PyObject *func = *pfunc;
    PyObject *x, *w;
    Py_ssize_t nkwargs = (kwnames == NULL) ? 0 : PyTuple_GET_SIZE(kwnames);
    Py_ssize_t nargs = oparg - nkwargs;
    PyObject **stack = (*pp_stack) - nargs - nkwargs;

    /* Always dispatch PyCFunction first, because these are
       presumed to be the most frequent callable object.
    */
    if (PyCFunction_Check(func)) {
        PyThreadState *tstate = PyThreadState_GET();
        C_TRACE(x, _PyCFunction_FastCallKeywords(func, stack, nargs, kwnames));
    }
    else if (Py_TYPE(func) == &PyMethodDescr_Type) {
        PyThreadState *tstate = PyThreadState_GET();
        if (nargs > 0 && tstate->use_tracing) {
            /* We need to create a temporary bound method as argument
               for profiling.

               If nargs == 0, then this cannot work because we have no
               "self". In any case, the call itself would raise
               TypeError (foo needs an argument), so we just skip
               profiling. */
            PyObject *self = stack[0];
            func = Py_TYPE(func)->tp_descr_get(func, self, (PyObject*)Py_TYPE(self));
            if (func != NULL) {
                C_TRACE(x, _PyCFunction_FastCallKeywords(func,
                                                         stack+1, nargs-1,
                                                         kwnames));
                Py_DECREF(func);
            }
            else {
                x = NULL;
            }
        }
        else {
            x = _PyMethodDescr_FastCallKeywords(func, stack, nargs, kwnames);
        }
    }
    else {
        if (PyMethod_Check(func) && PyMethod_GET_SELF(func) != NULL) {
            /* Optimize access to bound methods. Reuse the Python stack
               to pass 'self' as the first argument, replace 'func'
               with 'self'. It avoids the creation of a new temporary tuple
               for arguments (to replace func with self) when the method uses
               FASTCALL. */
            PyObject *self = PyMethod_GET_SELF(func);
            Py_INCREF(self);
            func = PyMethod_GET_FUNCTION(func);
            Py_INCREF(func);
            Py_SETREF(*pfunc, self);
            nargs++;
            stack--;
        }
        else {
            Py_INCREF(func);
        }

        if (PyFunction_Check(func)) {
            x = _PyFunction_FastCallKeywords(func, stack, nargs, kwnames);
        }
        else {
            x = _PyObject_FastCallKeywords(func, stack, nargs, kwnames);
        }
        Py_DECREF(func);
    }

    assert((x != NULL) ^ (PyErr_Occurred() != NULL));

    /* Clear the stack of the function object. */
    while ((*pp_stack) > pfunc) {
        w = EXT_POP(*pp_stack);
        Py_DECREF(w);
    }

    return x;
}

static PyObject *
do_call_core(PyObject *func, PyObject *callargs, PyObject *kwdict)
{
    if (PyCFunction_Check(func)) {
        PyObject *result;
        PyThreadState *tstate = PyThreadState_GET();
        C_TRACE(result, PyCFunction_Call(func, callargs, kwdict));
        return result;
    }
    else {
        return PyObject_Call(func, callargs, kwdict);
    }
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
    if (v != Py_None) {
        Py_ssize_t x;
        if (PyIndex_Check(v)) {
            x = PyNumber_AsSsize_t(v, NULL);
            if (x == -1 && PyErr_Occurred())
                return 0;
        }
        else {
            PyErr_SetString(PyExc_TypeError,
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
    Py_ssize_t x;
    if (PyIndex_Check(v)) {
        x = PyNumber_AsSsize_t(v, NULL);
        if (x == -1 && PyErr_Occurred())
            return 0;
    }
    else {
        PyErr_SetString(PyExc_TypeError,
                        "slice indices must be integers or "
                        "have an __index__ method");
        return 0;
    }
    *pi = x;
    return 1;
}


#define CANNOT_CATCH_MSG "catching classes that do not inherit from "\
                         "BaseException is not allowed"

static PyObject *
cmp_outcome(int op, PyObject *v, PyObject *w)
{
    int res = 0;
    switch (op) {
    case PyCmp_IS:
        res = (v == w);
        break;
    case PyCmp_IS_NOT:
        res = (v != w);
        break;
    case PyCmp_IN:
        res = PySequence_Contains(w, v);
        if (res < 0)
            return NULL;
        break;
    case PyCmp_NOT_IN:
        res = PySequence_Contains(w, v);
        if (res < 0)
            return NULL;
        res = !res;
        break;
    case PyCmp_EXC_MATCH:
        if (PyTuple_Check(w)) {
            Py_ssize_t i, length;
            length = PyTuple_Size(w);
            for (i = 0; i < length; i += 1) {
                PyObject *exc = PyTuple_GET_ITEM(w, i);
                if (!PyExceptionClass_Check(exc)) {
                    PyErr_SetString(PyExc_TypeError,
                                    CANNOT_CATCH_MSG);
                    return NULL;
                }
            }
        }
        else {
            if (!PyExceptionClass_Check(w)) {
                PyErr_SetString(PyExc_TypeError,
                                CANNOT_CATCH_MSG);
                return NULL;
            }
        }
        res = PyErr_GivenExceptionMatches(v, w);
        break;
    default:
        return PyObject_RichCompare(v, w, op);
    }
    v = res ? Py_True : Py_False;
    Py_INCREF(v);
    return v;
}

static PyObject *
import_name(PyFrameObject *f, PyObject *name, PyObject *fromlist, PyObject *level)
{
    _Py_IDENTIFIER(__import__);
    PyObject *import_func, *res;
    PyObject* stack[5];

    import_func = _PyDict_GetItemId(f->f_builtins, &PyId___import__);
    if (import_func == NULL) {
        PyErr_SetString(PyExc_ImportError, "__import__ not found");
        return NULL;
    }

    /* Fast path for not overloaded __import__. */
    if (import_func == PyThreadState_GET()->interp->import_func) {
        int ilevel = _PyLong_AsInt(level);
        if (ilevel == -1 && PyErr_Occurred()) {
            return NULL;
        }
        res = PyImport_ImportModuleLevelObject(
                        name,
                        f->f_globals,
                        f->f_locals == NULL ? Py_None : f->f_locals,
                        fromlist,
                        ilevel);
        return res;
    }

    Py_INCREF(import_func);

    stack[0] = name;
    stack[1] = f->f_globals;
    stack[2] = f->f_locals == NULL ? Py_None : f->f_locals;
    stack[3] = fromlist;
    stack[4] = level;
    res = _PyObject_FastCall(import_func, stack, 5);
    Py_DECREF(import_func);
    return res;
}

static PyObject *
import_from(PyObject *v, PyObject *name)
{
    PyObject *x;
    _Py_IDENTIFIER(__name__);
    PyObject *fullmodname, *pkgname, *pkgpath, *pkgname_or_unknown, *errmsg;

    if (_PyObject_LookupAttr(v, name, &x) != 0) {
        return x;
    }
    /* Issue #17636: in case this failed because of a circular relative
       import, try to fallback on reading the module directly from
       sys.modules. */
    pkgname = _PyObject_GetAttrId(v, &PyId___name__);
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
    if (x == NULL) {
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
        PyErr_Clear();
        errmsg = PyUnicode_FromFormat(
            "cannot import name %R from %R (unknown location)",
            name, pkgname_or_unknown
        );
        /* NULL check for errmsg done by PyErr_SetImportError. */
        PyErr_SetImportError(errmsg, pkgname, NULL);
    }
    else {
        errmsg = PyUnicode_FromFormat(
            "cannot import name %R from %R (%S)",
            name, pkgname_or_unknown, pkgpath
        );
        /* NULL check for errmsg done by PyErr_SetImportError. */
        PyErr_SetImportError(errmsg, pkgname, pkgpath);
    }

    Py_XDECREF(errmsg);
    Py_XDECREF(pkgname_or_unknown);
    Py_XDECREF(pkgpath);
    return NULL;
}

static int
import_all_from(PyObject *locals, PyObject *v)
{
    _Py_IDENTIFIER(__all__);
    _Py_IDENTIFIER(__dict__);
    PyObject *all, *dict, *name, *value;
    int skip_leading_underscores = 0;
    int pos, err;

    if (_PyObject_LookupAttrId(v, &PyId___all__, &all) < 0) {
        return -1; /* Unexpected error */
    }
    if (all == NULL) {
        if (_PyObject_LookupAttrId(v, &PyId___dict__, &dict) < 0) {
            return -1;
        }
        if (dict == NULL) {
            PyErr_SetString(PyExc_ImportError,
                    "from-import-* object has no __dict__ and no __all__");
            return -1;
        }
        all = PyMapping_Keys(dict);
        Py_DECREF(dict);
        if (all == NULL)
            return -1;
        skip_leading_underscores = 1;
    }

    for (pos = 0, err = 0; ; pos++) {
        name = PySequence_GetItem(all, pos);
        if (name == NULL) {
            if (!PyErr_ExceptionMatches(PyExc_IndexError))
                err = -1;
            else
                PyErr_Clear();
            break;
        }
        if (skip_leading_underscores && PyUnicode_Check(name)) {
            if (PyUnicode_READY(name) == -1) {
                Py_DECREF(name);
                err = -1;
                break;
            }
            if (PyUnicode_READ_CHAR(name, 0) == '_') {
                Py_DECREF(name);
                continue;
            }
        }
        value = PyObject_GetAttr(v, name);
        if (value == NULL)
            err = -1;
        else if (PyDict_CheckExact(locals))
            err = PyDict_SetItem(locals, name, value);
        else
            err = PyObject_SetItem(locals, name, value);
        Py_DECREF(name);
        Py_XDECREF(value);
        if (err != 0)
            break;
    }
    Py_DECREF(all);
    return err;
}

static int
check_args_iterable(PyObject *func, PyObject *args)
{
    if (args->ob_type->tp_iter == NULL && !PySequence_Check(args)) {
        PyErr_Format(PyExc_TypeError,
                     "%.200s%.200s argument after * "
                     "must be an iterable, not %.200s",
                     PyEval_GetFuncName(func),
                     PyEval_GetFuncDesc(func),
                     args->ob_type->tp_name);
        return -1;
    }
    return 0;
}

static void
format_kwargs_mapping_error(PyObject *func, PyObject *kwargs)
{
    PyErr_Format(PyExc_TypeError,
                 "%.200s%.200s argument after ** "
                 "must be a mapping, not %.200s",
                 PyEval_GetFuncName(func),
                 PyEval_GetFuncDesc(func),
                 kwargs->ob_type->tp_name);
}

static void
format_exc_check_arg(PyObject *exc, const char *format_str, PyObject *obj)
{
    const char *obj_str;

    if (!obj)
        return;

    obj_str = PyUnicode_AsUTF8(obj);
    if (!obj_str)
        return;

    PyErr_Format(exc, format_str, obj_str);
}

static void
format_exc_unbound(PyCodeObject *co, int oparg)
{
    PyObject *name;
    /* Don't stomp existing exception */
    if (PyErr_Occurred())
        return;
    if (oparg < PyTuple_GET_SIZE(co->co_cellvars)) {
        name = PyTuple_GET_ITEM(co->co_cellvars,
                                oparg);
        format_exc_check_arg(
            PyExc_UnboundLocalError,
            UNBOUNDLOCAL_ERROR_MSG,
            name);
    } else {
        name = PyTuple_GET_ITEM(co->co_freevars, oparg -
                                PyTuple_GET_SIZE(co->co_cellvars));
        format_exc_check_arg(PyExc_NameError,
                             UNBOUNDFREE_ERROR_MSG, name);
    }
}

static void
format_awaitable_error(PyTypeObject *type, int prevopcode)
{
    if (type->tp_as_async == NULL || type->tp_as_async->am_await == NULL) {
        if (prevopcode == BEFORE_ASYNC_WITH) {
            PyErr_Format(PyExc_TypeError,
                         "'async with' received an object from __aenter__ "
                         "that does not implement __await__: %.100s",
                         type->tp_name);
        }
        else if (prevopcode == WITH_CLEANUP_START) {
            PyErr_Format(PyExc_TypeError,
                         "'async with' received an object from __aexit__ "
                         "that does not implement __await__: %.100s",
                         type->tp_name);
        }
    }
}

static PyObject *
unicode_concatenate(PyObject *v, PyObject *w,
                    PyFrameObject *f, const _Py_CODEUNIT *next_instr)
{
    PyObject *res;
    if (Py_REFCNT(v) == 2) {
        /* In the common case, there are 2 references to the value
         * stored in 'variable' when the += is performed: one on the
         * value stack (in 'v') and one still stored in the
         * 'variable'.  We try to delete the variable now to reduce
         * the refcnt to 1.
         */
        int opcode, oparg;
        NEXTOPARG();
        switch (opcode) {
        case STORE_FAST:
        {
            PyObject **fastlocals = f->f_localsplus;
            if (GETLOCAL(oparg) == v)
                SETLOCAL(oparg, NULL);
            break;
        }
        case STORE_DEREF:
        {
            PyObject **freevars = (f->f_localsplus +
                                   f->f_code->co_nlocals);
            PyObject *c = freevars[oparg];
            if (PyCell_GET(c) ==  v) {
                PyCell_SET(c, NULL);
                Py_DECREF(v);
            }
            break;
        }
        case STORE_NAME:
        {
            PyObject *names = f->f_code->co_names;
            PyObject *name = GETITEM(names, oparg);
            PyObject *locals = f->f_locals;
            if (PyDict_CheckExact(locals) &&
                PyDict_GetItem(locals, name) == v) {
                if (PyDict_DelItem(locals, name) != 0) {
                    PyErr_Clear();
                }
            }
            break;
        }
        }
    }
    res = v;
    PyUnicode_Append(&res, w);
    return res;
}

#ifdef DYNAMIC_EXECUTION_PROFILE

static PyObject *
getarray(long a[256])
{
    int i;
    PyObject *l = PyList_New(256);
    if (l == NULL) return NULL;
    for (i = 0; i < 256; i++) {
        PyObject *x = PyLong_FromLong(a[i]);
        if (x == NULL) {
            Py_DECREF(l);
            return NULL;
        }
        PyList_SetItem(l, i, x);
    }
    for (i = 0; i < 256; i++)
        a[i] = 0;
    return l;
}

PyObject *
_Py_GetDXProfile(PyObject *self, PyObject *args)
{
#ifndef DXPAIRS
    return getarray(dxp);
#else
    int i;
    PyObject *l = PyList_New(257);
    if (l == NULL) return NULL;
    for (i = 0; i < 257; i++) {
        PyObject *x = getarray(dxpairs[i]);
        if (x == NULL) {
            Py_DECREF(l);
            return NULL;
        }
        PyList_SetItem(l, i, x);
    }
    return l;
#endif
}

#endif

Py_ssize_t
_PyEval_RequestCodeExtraIndex(freefunc free)
{
    PyInterpreterState *interp = PyThreadState_Get()->interp;
    Py_ssize_t new_index;

    if (interp->co_extra_user_count == MAX_CO_EXTRA_USERS - 1) {
        return -1;
    }
    new_index = interp->co_extra_user_count++;
    interp->co_extra_freefuncs[new_index] = free;
    return new_index;
}

static void
dtrace_function_entry(PyFrameObject *f)
{
    const char *filename;
    const char *funcname;
    int lineno;

    filename = PyUnicode_AsUTF8(f->f_code->co_filename);
    funcname = PyUnicode_AsUTF8(f->f_code->co_name);
    lineno = PyCode_Addr2Line(f->f_code, f->f_lasti);

    PyDTrace_FUNCTION_ENTRY(filename, funcname, lineno);
}

static void
dtrace_function_return(PyFrameObject *f)
{
    const char *filename;
    const char *funcname;
    int lineno;

    filename = PyUnicode_AsUTF8(f->f_code->co_filename);
    funcname = PyUnicode_AsUTF8(f->f_code->co_name);
    lineno = PyCode_Addr2Line(f->f_code, f->f_lasti);

    PyDTrace_FUNCTION_RETURN(filename, funcname, lineno);
}

/* DTrace equivalent of maybe_call_line_trace. */
static void
maybe_dtrace_line(PyFrameObject *frame,
                  int *instr_lb, int *instr_ub, int *instr_prev)
{
    int line = frame->f_lineno;
    const char *co_filename, *co_name;

    /* If the last instruction executed isn't in the current
       instruction window, reset the window.
    */
    if (frame->f_lasti < *instr_lb || frame->f_lasti >= *instr_ub) {
        PyAddrPair bounds;
        line = _PyCode_CheckLineNumber(frame->f_code, frame->f_lasti,
                                       &bounds);
        *instr_lb = bounds.ap_lower;
        *instr_ub = bounds.ap_upper;
    }
    /* If the last instruction falls at the start of a line or if
       it represents a jump backwards, update the frame's line
       number and call the trace function. */
    if (frame->f_lasti == *instr_lb || frame->f_lasti < *instr_prev) {
        frame->f_lineno = line;
        co_filename = PyUnicode_AsUTF8(frame->f_code->co_filename);
        if (!co_filename)
            co_filename = "?";
        co_name = PyUnicode_AsUTF8(frame->f_code->co_name);
        if (!co_name)
            co_name = "?";
        PyDTrace_LINE(co_filename, co_name, line);
    }
    *instr_prev = frame->f_lasti;
}
