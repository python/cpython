
/* Generic object operations; and implementation of None */

#include "Python.h"
#include "pycore_call.h"          // _PyObject_CallNoArgs()
#include "pycore_ceval.h"         // _Py_EnterRecursiveCallTstate()
#include "pycore_context.h"       // _PyContextTokenMissing_Type
#include "pycore_dict.h"          // _PyObject_MakeDictFromInstanceAttributes()
#include "pycore_floatobject.h"   // _PyFloat_DebugMallocStats()
#include "pycore_initconfig.h"    // _PyStatus_EXCEPTION()
#include "pycore_namespace.h"     // _PyNamespace_Type
#include "pycore_object.h"        // _PyType_CheckConsistency(), _Py_FatalRefcountError()
#include "pycore_pyerrors.h"      // _PyErr_Occurred()
#include "pycore_pymem.h"         // _PyMem_IsPtrFreed()
#include "pycore_pystate.h"       // _PyThreadState_GET()
#include "pycore_symtable.h"      // PySTEntry_Type
#include "pycore_typevarobject.h" // _PyTypeAlias_Type, _Py_initialize_generic
#include "pycore_typeobject.h"    // _PyBufferWrapper_Type
#include "pycore_unionobject.h"   // _PyUnion_Type
#include "interpreteridobject.h"  // _PyInterpreterID_Type

#ifdef Py_LIMITED_API
   // Prevent recursive call _Py_IncRef() <=> Py_INCREF()
#  error "Py_LIMITED_API macro must not be defined"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Defined in tracemalloc.c */
extern void _PyMem_DumpTraceback(int fd, const void *ptr);


int
_PyObject_CheckConsistency(PyObject *op, int check_content)
{
#define CHECK(expr) \
    do { if (!(expr)) { _PyObject_ASSERT_FAILED_MSG(op, Py_STRINGIFY(expr)); } } while (0)

    CHECK(!_PyObject_IsFreed(op));
    CHECK(Py_REFCNT(op) >= 1);

    _PyType_CheckConsistency(Py_TYPE(op));

    if (PyUnicode_Check(op)) {
        _PyUnicode_CheckConsistency(op, check_content);
    }
    else if (PyDict_Check(op)) {
        _PyDict_CheckConsistency(op, check_content);
    }
    return 1;

#undef CHECK
}


#ifdef Py_REF_DEBUG
/* We keep the legacy symbol around for backward compatibility. */
Py_ssize_t _Py_RefTotal;

static inline Py_ssize_t
get_legacy_reftotal(void)
{
    return _Py_RefTotal;
}
#endif

#ifdef Py_REF_DEBUG

#  define REFTOTAL(interp) \
    interp->object_state.reftotal

static inline void
reftotal_increment(PyInterpreterState *interp)
{
    REFTOTAL(interp)++;
}

static inline void
reftotal_decrement(PyInterpreterState *interp)
{
    REFTOTAL(interp)--;
}

static inline void
reftotal_add(PyInterpreterState *interp, Py_ssize_t n)
{
    REFTOTAL(interp) += n;
}

static inline Py_ssize_t get_global_reftotal(_PyRuntimeState *);

/* We preserve the number of refs leaked during runtime finalization,
   so they can be reported if the runtime is initialized again. */
// XXX We don't lose any information by dropping this,
// so we should consider doing so.
static Py_ssize_t last_final_reftotal = 0;

void
_Py_FinalizeRefTotal(_PyRuntimeState *runtime)
{
    last_final_reftotal = get_global_reftotal(runtime);
    runtime->object_state.interpreter_leaks = 0;
}

void
_PyInterpreterState_FinalizeRefTotal(PyInterpreterState *interp)
{
    interp->runtime->object_state.interpreter_leaks += REFTOTAL(interp);
    REFTOTAL(interp) = 0;
}

static inline Py_ssize_t
get_reftotal(PyInterpreterState *interp)
{
    /* For a single interpreter, we ignore the legacy _Py_RefTotal,
       since we can't determine which interpreter updated it. */
    return REFTOTAL(interp);
}

static inline Py_ssize_t
get_global_reftotal(_PyRuntimeState *runtime)
{
    Py_ssize_t total = 0;

    /* Add up the total from each interpreter. */
    HEAD_LOCK(&_PyRuntime);
    PyInterpreterState *interp = PyInterpreterState_Head();
    for (; interp != NULL; interp = PyInterpreterState_Next(interp)) {
        total += REFTOTAL(interp);
    }
    HEAD_UNLOCK(&_PyRuntime);

    /* Add in the updated value from the legacy _Py_RefTotal. */
    total += get_legacy_reftotal();
    total += last_final_reftotal;
    total += runtime->object_state.interpreter_leaks;

    return total;
}

#undef REFTOTAL

void
_PyDebug_PrintTotalRefs(void) {
    _PyRuntimeState *runtime = &_PyRuntime;
    fprintf(stderr,
            "[%zd refs, %zd blocks]\n",
            get_global_reftotal(runtime), _Py_GetGlobalAllocatedBlocks());
    /* It may be helpful to also print the "legacy" reftotal separately.
       Likewise for the total for each interpreter. */
}
#endif /* Py_REF_DEBUG */

/* Object allocation routines used by NEWOBJ and NEWVAROBJ macros.
   These are used by the individual routines for object creation.
   Do not call them otherwise, they do not initialize the object! */

#ifdef Py_TRACE_REFS

#define REFCHAIN(interp) &interp->object_state.refchain

static inline void
init_refchain(PyInterpreterState *interp)
{
    PyObject *refchain = REFCHAIN(interp);
    refchain->_ob_prev = refchain;
    refchain->_ob_next = refchain;
}

/* Insert op at the front of the list of all objects.  If force is true,
 * op is added even if _ob_prev and _ob_next are non-NULL already.  If
 * force is false amd _ob_prev or _ob_next are non-NULL, do nothing.
 * force should be true if and only if op points to freshly allocated,
 * uninitialized memory, or you've unlinked op from the list and are
 * relinking it into the front.
 * Note that objects are normally added to the list via _Py_NewReference,
 * which is called by PyObject_Init.  Not all objects are initialized that
 * way, though; exceptions include statically allocated type objects, and
 * statically allocated singletons (like Py_True and Py_None).
 */
void
_Py_AddToAllObjects(PyObject *op, int force)
{
#ifdef  Py_DEBUG
    if (!force) {
        /* If it's initialized memory, op must be in or out of
         * the list unambiguously.
         */
        _PyObject_ASSERT(op, (op->_ob_prev == NULL) == (op->_ob_next == NULL));
    }
#endif
    if (force || op->_ob_prev == NULL) {
        PyObject *refchain = REFCHAIN(_PyInterpreterState_GET());
        op->_ob_next = refchain->_ob_next;
        op->_ob_prev = refchain;
        refchain->_ob_next->_ob_prev = op;
        refchain->_ob_next = op;
    }
}
#endif  /* Py_TRACE_REFS */

#ifdef Py_REF_DEBUG
/* Log a fatal error; doesn't return. */
void
_Py_NegativeRefcount(const char *filename, int lineno, PyObject *op)
{
    _PyObject_AssertFailed(op, NULL, "object has negative ref count",
                           filename, lineno, __func__);
}

/* This is used strictly by Py_INCREF(). */
void
_Py_INCREF_IncRefTotal(void)
{
    reftotal_increment(_PyInterpreterState_GET());
}

/* This is used strictly by Py_DECREF(). */
void
_Py_DECREF_DecRefTotal(void)
{
    reftotal_decrement(_PyInterpreterState_GET());
}

void
_Py_IncRefTotal(PyInterpreterState *interp)
{
    reftotal_increment(interp);
}

void
_Py_DecRefTotal(PyInterpreterState *interp)
{
    reftotal_decrement(interp);
}

void
_Py_AddRefTotal(PyInterpreterState *interp, Py_ssize_t n)
{
    reftotal_add(interp, n);
}

/* This includes the legacy total
   and any carried over from the last runtime init/fini cycle. */
Py_ssize_t
_Py_GetGlobalRefTotal(void)
{
    return get_global_reftotal(&_PyRuntime);
}

Py_ssize_t
_Py_GetLegacyRefTotal(void)
{
    return get_legacy_reftotal();
}

Py_ssize_t
_PyInterpreterState_GetRefTotal(PyInterpreterState *interp)
{
    return get_reftotal(interp);
}

#endif /* Py_REF_DEBUG */

void
Py_IncRef(PyObject *o)
{
    Py_XINCREF(o);
}

void
Py_DecRef(PyObject *o)
{
    Py_XDECREF(o);
}

void
_Py_IncRef(PyObject *o)
{
    Py_INCREF(o);
}

void
_Py_DecRef(PyObject *o)
{
    Py_DECREF(o);
}


/**************************************/

PyObject *
PyObject_Init(PyObject *op, PyTypeObject *tp)
{
    if (op == NULL) {
        return PyErr_NoMemory();
    }

    _PyObject_Init(op, tp);
    return op;
}

PyVarObject *
PyObject_InitVar(PyVarObject *op, PyTypeObject *tp, Py_ssize_t size)
{
    if (op == NULL) {
        return (PyVarObject *) PyErr_NoMemory();
    }

    _PyObject_InitVar(op, tp, size);
    return op;
}

PyObject *
_PyObject_New(PyTypeObject *tp)
{
    PyObject *op = (PyObject *) PyObject_Malloc(_PyObject_SIZE(tp));
    if (op == NULL) {
        return PyErr_NoMemory();
    }
    _PyObject_Init(op, tp);
    return op;
}

PyVarObject *
_PyObject_NewVar(PyTypeObject *tp, Py_ssize_t nitems)
{
    PyVarObject *op;
    const size_t size = _PyObject_VAR_SIZE(tp, nitems);
    op = (PyVarObject *) PyObject_Malloc(size);
    if (op == NULL) {
        return (PyVarObject *)PyErr_NoMemory();
    }
    _PyObject_InitVar(op, tp, nitems);
    return op;
}

void
PyObject_CallFinalizer(PyObject *self)
{
    PyTypeObject *tp = Py_TYPE(self);

    if (tp->tp_finalize == NULL)
        return;
    /* tp_finalize should only be called once. */
    if (_PyType_IS_GC(tp) && _PyGC_FINALIZED(self))
        return;

    tp->tp_finalize(self);
    if (_PyType_IS_GC(tp)) {
        _PyGC_SET_FINALIZED(self);
    }
}

int
PyObject_CallFinalizerFromDealloc(PyObject *self)
{
    if (Py_REFCNT(self) != 0) {
        _PyObject_ASSERT_FAILED_MSG(self,
                                    "PyObject_CallFinalizerFromDealloc called "
                                    "on object with a non-zero refcount");
    }

    /* Temporarily resurrect the object. */
    Py_SET_REFCNT(self, 1);

    PyObject_CallFinalizer(self);

    _PyObject_ASSERT_WITH_MSG(self,
                              Py_REFCNT(self) > 0,
                              "refcount is too small");

    /* Undo the temporary resurrection; can't use DECREF here, it would
     * cause a recursive call. */
    Py_SET_REFCNT(self, Py_REFCNT(self) - 1);
    if (Py_REFCNT(self) == 0) {
        return 0;         /* this is the normal path out */
    }

    /* tp_finalize resurrected it!  Make it look like the original Py_DECREF
     * never happened. */
    Py_ssize_t refcnt = Py_REFCNT(self);
    _Py_NewReferenceNoTotal(self);
    Py_SET_REFCNT(self, refcnt);

    _PyObject_ASSERT(self,
                     (!_PyType_IS_GC(Py_TYPE(self))
                      || _PyObject_GC_IS_TRACKED(self)));
    return -1;
}

int
PyObject_Print(PyObject *op, FILE *fp, int flags)
{
    int ret = 0;
    if (PyErr_CheckSignals())
        return -1;
#ifdef USE_STACKCHECK
    if (PyOS_CheckStack()) {
        PyErr_SetString(PyExc_MemoryError, "stack overflow");
        return -1;
    }
#endif
    clearerr(fp); /* Clear any previous error condition */
    if (op == NULL) {
        Py_BEGIN_ALLOW_THREADS
        fprintf(fp, "<nil>");
        Py_END_ALLOW_THREADS
    }
    else {
        if (Py_REFCNT(op) <= 0) {
            Py_BEGIN_ALLOW_THREADS
            fprintf(fp, "<refcnt %zd at %p>", Py_REFCNT(op), (void *)op);
            Py_END_ALLOW_THREADS
        }
        else {
            PyObject *s;
            if (flags & Py_PRINT_RAW)
                s = PyObject_Str(op);
            else
                s = PyObject_Repr(op);
            if (s == NULL) {
                ret = -1;
            }
            else {
                assert(PyUnicode_Check(s));
                const char *t;
                Py_ssize_t len;
                t = PyUnicode_AsUTF8AndSize(s, &len);
                if (t == NULL) {
                    ret = -1;
                }
                else {
                    fwrite(t, 1, len, fp);
                }
                Py_DECREF(s);
            }
        }
    }
    if (ret == 0) {
        if (ferror(fp)) {
            PyErr_SetFromErrno(PyExc_OSError);
            clearerr(fp);
            ret = -1;
        }
    }
    return ret;
}

/* For debugging convenience.  Set a breakpoint here and call it from your DLL */
void
_Py_BreakPoint(void)
{
}


/* Heuristic checking if the object memory is uninitialized or deallocated.
   Rely on the debug hooks on Python memory allocators:
   see _PyMem_IsPtrFreed().

   The function can be used to prevent segmentation fault on dereferencing
   pointers like 0xDDDDDDDDDDDDDDDD. */
int
_PyObject_IsFreed(PyObject *op)
{
    if (_PyMem_IsPtrFreed(op) || _PyMem_IsPtrFreed(Py_TYPE(op))) {
        return 1;
    }
    /* ignore op->ob_ref: its value can have be modified
       by Py_INCREF() and Py_DECREF(). */
#ifdef Py_TRACE_REFS
    if (op->_ob_next != NULL && _PyMem_IsPtrFreed(op->_ob_next)) {
        return 1;
    }
    if (op->_ob_prev != NULL && _PyMem_IsPtrFreed(op->_ob_prev)) {
         return 1;
     }
#endif
    return 0;
}


/* For debugging convenience.  See Misc/gdbinit for some useful gdb hooks */
void
_PyObject_Dump(PyObject* op)
{
    if (_PyObject_IsFreed(op)) {
        /* It seems like the object memory has been freed:
           don't access it to prevent a segmentation fault. */
        fprintf(stderr, "<object at %p is freed>\n", op);
        fflush(stderr);
        return;
    }

    /* first, write fields which are the least likely to crash */
    fprintf(stderr, "object address  : %p\n", (void *)op);
    fprintf(stderr, "object refcount : %zd\n", Py_REFCNT(op));
    fflush(stderr);

    PyTypeObject *type = Py_TYPE(op);
    fprintf(stderr, "object type     : %p\n", type);
    fprintf(stderr, "object type name: %s\n",
            type==NULL ? "NULL" : type->tp_name);

    /* the most dangerous part */
    fprintf(stderr, "object repr     : ");
    fflush(stderr);

    PyGILState_STATE gil = PyGILState_Ensure();
    PyObject *exc = PyErr_GetRaisedException();

    (void)PyObject_Print(op, stderr, 0);
    fflush(stderr);

    PyErr_SetRaisedException(exc);
    PyGILState_Release(gil);

    fprintf(stderr, "\n");
    fflush(stderr);
}

PyObject *
PyObject_Repr(PyObject *v)
{
    PyObject *res;
    if (PyErr_CheckSignals())
        return NULL;
#ifdef USE_STACKCHECK
    if (PyOS_CheckStack()) {
        PyErr_SetString(PyExc_MemoryError, "stack overflow");
        return NULL;
    }
#endif
    if (v == NULL)
        return PyUnicode_FromString("<NULL>");
    if (Py_TYPE(v)->tp_repr == NULL)
        return PyUnicode_FromFormat("<%s object at %p>",
                                    Py_TYPE(v)->tp_name, v);

    PyThreadState *tstate = _PyThreadState_GET();
#ifdef Py_DEBUG
    /* PyObject_Repr() must not be called with an exception set,
       because it can clear it (directly or indirectly) and so the
       caller loses its exception */
    assert(!_PyErr_Occurred(tstate));
#endif

    /* It is possible for a type to have a tp_repr representation that loops
       infinitely. */
    if (_Py_EnterRecursiveCallTstate(tstate,
                                     " while getting the repr of an object")) {
        return NULL;
    }
    res = (*Py_TYPE(v)->tp_repr)(v);
    _Py_LeaveRecursiveCallTstate(tstate);

    if (res == NULL) {
        return NULL;
    }
    if (!PyUnicode_Check(res)) {
        _PyErr_Format(tstate, PyExc_TypeError,
                      "__repr__ returned non-string (type %.200s)",
                      Py_TYPE(res)->tp_name);
        Py_DECREF(res);
        return NULL;
    }
#ifndef Py_DEBUG
    if (PyUnicode_READY(res) < 0) {
        return NULL;
    }
#endif
    return res;
}

PyObject *
PyObject_Str(PyObject *v)
{
    PyObject *res;
    if (PyErr_CheckSignals())
        return NULL;
#ifdef USE_STACKCHECK
    if (PyOS_CheckStack()) {
        PyErr_SetString(PyExc_MemoryError, "stack overflow");
        return NULL;
    }
#endif
    if (v == NULL)
        return PyUnicode_FromString("<NULL>");
    if (PyUnicode_CheckExact(v)) {
#ifndef Py_DEBUG
        if (PyUnicode_READY(v) < 0)
            return NULL;
#endif
        return Py_NewRef(v);
    }
    if (Py_TYPE(v)->tp_str == NULL)
        return PyObject_Repr(v);

    PyThreadState *tstate = _PyThreadState_GET();
#ifdef Py_DEBUG
    /* PyObject_Str() must not be called with an exception set,
       because it can clear it (directly or indirectly) and so the
       caller loses its exception */
    assert(!_PyErr_Occurred(tstate));
#endif

    /* It is possible for a type to have a tp_str representation that loops
       infinitely. */
    if (_Py_EnterRecursiveCallTstate(tstate, " while getting the str of an object")) {
        return NULL;
    }
    res = (*Py_TYPE(v)->tp_str)(v);
    _Py_LeaveRecursiveCallTstate(tstate);

    if (res == NULL) {
        return NULL;
    }
    if (!PyUnicode_Check(res)) {
        _PyErr_Format(tstate, PyExc_TypeError,
                      "__str__ returned non-string (type %.200s)",
                      Py_TYPE(res)->tp_name);
        Py_DECREF(res);
        return NULL;
    }
#ifndef Py_DEBUG
    if (PyUnicode_READY(res) < 0) {
        return NULL;
    }
#endif
    assert(_PyUnicode_CheckConsistency(res, 1));
    return res;
}

PyObject *
PyObject_ASCII(PyObject *v)
{
    PyObject *repr, *ascii, *res;

    repr = PyObject_Repr(v);
    if (repr == NULL)
        return NULL;

    if (PyUnicode_IS_ASCII(repr))
        return repr;

    /* repr is guaranteed to be a PyUnicode object by PyObject_Repr */
    ascii = _PyUnicode_AsASCIIString(repr, "backslashreplace");
    Py_DECREF(repr);
    if (ascii == NULL)
        return NULL;

    res = PyUnicode_DecodeASCII(
        PyBytes_AS_STRING(ascii),
        PyBytes_GET_SIZE(ascii),
        NULL);

    Py_DECREF(ascii);
    return res;
}

PyObject *
PyObject_Bytes(PyObject *v)
{
    PyObject *result, *func;

    if (v == NULL)
        return PyBytes_FromString("<NULL>");

    if (PyBytes_CheckExact(v)) {
        return Py_NewRef(v);
    }

    func = _PyObject_LookupSpecial(v, &_Py_ID(__bytes__));
    if (func != NULL) {
        result = _PyObject_CallNoArgs(func);
        Py_DECREF(func);
        if (result == NULL)
            return NULL;
        if (!PyBytes_Check(result)) {
            PyErr_Format(PyExc_TypeError,
                         "__bytes__ returned non-bytes (type %.200s)",
                         Py_TYPE(result)->tp_name);
            Py_DECREF(result);
            return NULL;
        }
        return result;
    }
    else if (PyErr_Occurred())
        return NULL;
    return PyBytes_FromObject(v);
}


/*
def _PyObject_FunctionStr(x):
    try:
        qualname = x.__qualname__
    except AttributeError:
        return str(x)
    try:
        mod = x.__module__
        if mod is not None and mod != 'builtins':
            return f"{x.__module__}.{qualname}()"
    except AttributeError:
        pass
    return qualname
*/
PyObject *
_PyObject_FunctionStr(PyObject *x)
{
    assert(!PyErr_Occurred());
    PyObject *qualname;
    int ret = _PyObject_LookupAttr(x, &_Py_ID(__qualname__), &qualname);
    if (qualname == NULL) {
        if (ret < 0) {
            return NULL;
        }
        return PyObject_Str(x);
    }
    PyObject *module;
    PyObject *result = NULL;
    ret = _PyObject_LookupAttr(x, &_Py_ID(__module__), &module);
    if (module != NULL && module != Py_None) {
        ret = PyObject_RichCompareBool(module, &_Py_ID(builtins), Py_NE);
        if (ret < 0) {
            // error
            goto done;
        }
        if (ret > 0) {
            result = PyUnicode_FromFormat("%S.%S()", module, qualname);
            goto done;
        }
    }
    else if (ret < 0) {
        goto done;
    }
    result = PyUnicode_FromFormat("%S()", qualname);
done:
    Py_DECREF(qualname);
    Py_XDECREF(module);
    return result;
}

/* For Python 3.0.1 and later, the old three-way comparison has been
   completely removed in favour of rich comparisons.  PyObject_Compare() and
   PyObject_Cmp() are gone, and the builtin cmp function no longer exists.
   The old tp_compare slot has been renamed to tp_as_async, and should no
   longer be used.  Use tp_richcompare instead.

   See (*) below for practical amendments.

   tp_richcompare gets called with a first argument of the appropriate type
   and a second object of an arbitrary type.  We never do any kind of
   coercion.

   The tp_richcompare slot should return an object, as follows:

    NULL if an exception occurred
    NotImplemented if the requested comparison is not implemented
    any other false value if the requested comparison is false
    any other true value if the requested comparison is true

  The PyObject_RichCompare[Bool]() wrappers raise TypeError when they get
  NotImplemented.

  (*) Practical amendments:

  - If rich comparison returns NotImplemented, == and != are decided by
    comparing the object pointer (i.e. falling back to the base object
    implementation).

*/

/* Map rich comparison operators to their swapped version, e.g. LT <--> GT */
int _Py_SwappedOp[] = {Py_GT, Py_GE, Py_EQ, Py_NE, Py_LT, Py_LE};

static const char * const opstrings[] = {"<", "<=", "==", "!=", ">", ">="};

/* Perform a rich comparison, raising TypeError when the requested comparison
   operator is not supported. */
static PyObject *
do_richcompare(PyThreadState *tstate, PyObject *v, PyObject *w, int op)
{
    richcmpfunc f;
    PyObject *res;
    int checked_reverse_op = 0;

    if (!Py_IS_TYPE(v, Py_TYPE(w)) &&
        PyType_IsSubtype(Py_TYPE(w), Py_TYPE(v)) &&
        (f = Py_TYPE(w)->tp_richcompare) != NULL) {
        checked_reverse_op = 1;
        res = (*f)(w, v, _Py_SwappedOp[op]);
        if (res != Py_NotImplemented)
            return res;
        Py_DECREF(res);
    }
    if ((f = Py_TYPE(v)->tp_richcompare) != NULL) {
        res = (*f)(v, w, op);
        if (res != Py_NotImplemented)
            return res;
        Py_DECREF(res);
    }
    if (!checked_reverse_op && (f = Py_TYPE(w)->tp_richcompare) != NULL) {
        res = (*f)(w, v, _Py_SwappedOp[op]);
        if (res != Py_NotImplemented)
            return res;
        Py_DECREF(res);
    }
    /* If neither object implements it, provide a sensible default
       for == and !=, but raise an exception for ordering. */
    switch (op) {
    case Py_EQ:
        res = (v == w) ? Py_True : Py_False;
        break;
    case Py_NE:
        res = (v != w) ? Py_True : Py_False;
        break;
    default:
        _PyErr_Format(tstate, PyExc_TypeError,
                      "'%s' not supported between instances of '%.100s' and '%.100s'",
                      opstrings[op],
                      Py_TYPE(v)->tp_name,
                      Py_TYPE(w)->tp_name);
        return NULL;
    }
    return Py_NewRef(res);
}

/* Perform a rich comparison with object result.  This wraps do_richcompare()
   with a check for NULL arguments and a recursion check. */

PyObject *
PyObject_RichCompare(PyObject *v, PyObject *w, int op)
{
    PyThreadState *tstate = _PyThreadState_GET();

    assert(Py_LT <= op && op <= Py_GE);
    if (v == NULL || w == NULL) {
        if (!_PyErr_Occurred(tstate)) {
            PyErr_BadInternalCall();
        }
        return NULL;
    }
    if (_Py_EnterRecursiveCallTstate(tstate, " in comparison")) {
        return NULL;
    }
    PyObject *res = do_richcompare(tstate, v, w, op);
    _Py_LeaveRecursiveCallTstate(tstate);
    return res;
}

/* Perform a rich comparison with integer result.  This wraps
   PyObject_RichCompare(), returning -1 for error, 0 for false, 1 for true. */
int
PyObject_RichCompareBool(PyObject *v, PyObject *w, int op)
{
    PyObject *res;
    int ok;

    /* Quick result when objects are the same.
       Guarantees that identity implies equality. */
    if (v == w) {
        if (op == Py_EQ)
            return 1;
        else if (op == Py_NE)
            return 0;
    }

    res = PyObject_RichCompare(v, w, op);
    if (res == NULL)
        return -1;
    if (PyBool_Check(res))
        ok = (res == Py_True);
    else
        ok = PyObject_IsTrue(res);
    Py_DECREF(res);
    return ok;
}

Py_hash_t
PyObject_HashNotImplemented(PyObject *v)
{
    PyErr_Format(PyExc_TypeError, "unhashable type: '%.200s'",
                 Py_TYPE(v)->tp_name);
    return -1;
}

Py_hash_t
PyObject_Hash(PyObject *v)
{
    PyTypeObject *tp = Py_TYPE(v);
    if (tp->tp_hash != NULL)
        return (*tp->tp_hash)(v);
    /* To keep to the general practice that inheriting
     * solely from object in C code should work without
     * an explicit call to PyType_Ready, we implicitly call
     * PyType_Ready here and then check the tp_hash slot again
     */
    if (!_PyType_IsReady(tp)) {
        if (PyType_Ready(tp) < 0)
            return -1;
        if (tp->tp_hash != NULL)
            return (*tp->tp_hash)(v);
    }
    /* Otherwise, the object can't be hashed */
    return PyObject_HashNotImplemented(v);
}

PyObject *
PyObject_GetAttrString(PyObject *v, const char *name)
{
    PyObject *w, *res;

    if (Py_TYPE(v)->tp_getattr != NULL)
        return (*Py_TYPE(v)->tp_getattr)(v, (char*)name);
    w = PyUnicode_FromString(name);
    if (w == NULL)
        return NULL;
    res = PyObject_GetAttr(v, w);
    Py_DECREF(w);
    return res;
}

int
PyObject_HasAttrString(PyObject *v, const char *name)
{
    if (Py_TYPE(v)->tp_getattr != NULL) {
        PyObject *res = (*Py_TYPE(v)->tp_getattr)(v, (char*)name);
        if (res != NULL) {
            Py_DECREF(res);
            return 1;
        }
        PyErr_Clear();
        return 0;
    }

    PyObject *attr_name = PyUnicode_FromString(name);
    if (attr_name == NULL) {
        PyErr_Clear();
        return 0;
    }
    int ok = PyObject_HasAttr(v, attr_name);
    Py_DECREF(attr_name);
    return ok;
}

int
PyObject_SetAttrString(PyObject *v, const char *name, PyObject *w)
{
    PyObject *s;
    int res;

    if (Py_TYPE(v)->tp_setattr != NULL)
        return (*Py_TYPE(v)->tp_setattr)(v, (char*)name, w);
    s = PyUnicode_InternFromString(name);
    if (s == NULL)
        return -1;
    res = PyObject_SetAttr(v, s, w);
    Py_XDECREF(s);
    return res;
}

int
_PyObject_IsAbstract(PyObject *obj)
{
    int res;
    PyObject* isabstract;

    if (obj == NULL)
        return 0;

    res = _PyObject_LookupAttr(obj, &_Py_ID(__isabstractmethod__), &isabstract);
    if (res > 0) {
        res = PyObject_IsTrue(isabstract);
        Py_DECREF(isabstract);
    }
    return res;
}

PyObject *
_PyObject_GetAttrId(PyObject *v, _Py_Identifier *name)
{
    PyObject *result;
    PyObject *oname = _PyUnicode_FromId(name); /* borrowed */
    if (!oname)
        return NULL;
    result = PyObject_GetAttr(v, oname);
    return result;
}

int
_PyObject_SetAttrId(PyObject *v, _Py_Identifier *name, PyObject *w)
{
    int result;
    PyObject *oname = _PyUnicode_FromId(name); /* borrowed */
    if (!oname)
        return -1;
    result = PyObject_SetAttr(v, oname, w);
    return result;
}

static inline int
set_attribute_error_context(PyObject* v, PyObject* name)
{
    assert(PyErr_Occurred());
    if (!PyErr_ExceptionMatches(PyExc_AttributeError)){
        return 0;
    }
    // Intercept AttributeError exceptions and augment them to offer suggestions later.
    PyObject *exc = PyErr_GetRaisedException();
    if (!PyErr_GivenExceptionMatches(exc, PyExc_AttributeError)) {
        goto restore;
    }
    PyAttributeErrorObject* the_exc = (PyAttributeErrorObject*) exc;
    // Check if this exception was already augmented
    if (the_exc->name || the_exc->obj) {
        goto restore;
    }
    // Augment the exception with the name and object
    if (PyObject_SetAttr(exc, &_Py_ID(name), name) ||
        PyObject_SetAttr(exc, &_Py_ID(obj), v)) {
        return 1;
    }
restore:
    PyErr_SetRaisedException(exc);
    return 0;
}

PyObject *
PyObject_GetAttr(PyObject *v, PyObject *name)
{
    PyTypeObject *tp = Py_TYPE(v);
    if (!PyUnicode_Check(name)) {
        PyErr_Format(PyExc_TypeError,
                     "attribute name must be string, not '%.200s'",
                     Py_TYPE(name)->tp_name);
        return NULL;
    }

    PyObject* result = NULL;
    if (tp->tp_getattro != NULL) {
        result = (*tp->tp_getattro)(v, name);
    }
    else if (tp->tp_getattr != NULL) {
        const char *name_str = PyUnicode_AsUTF8(name);
        if (name_str == NULL) {
            return NULL;
        }
        result = (*tp->tp_getattr)(v, (char *)name_str);
    }
    else {
        PyErr_Format(PyExc_AttributeError,
                    "'%.100s' object has no attribute '%U'",
                    tp->tp_name, name);
    }

    if (result == NULL) {
        set_attribute_error_context(v, name);
    }
    return result;
}

int
_PyObject_LookupAttr(PyObject *v, PyObject *name, PyObject **result)
{
    PyTypeObject *tp = Py_TYPE(v);

    if (!PyUnicode_Check(name)) {
        PyErr_Format(PyExc_TypeError,
                     "attribute name must be string, not '%.200s'",
                     Py_TYPE(name)->tp_name);
        *result = NULL;
        return -1;
    }

    if (tp->tp_getattro == PyObject_GenericGetAttr) {
        *result = _PyObject_GenericGetAttrWithDict(v, name, NULL, 1);
        if (*result != NULL) {
            return 1;
        }
        if (PyErr_Occurred()) {
            return -1;
        }
        return 0;
    }
    if (tp->tp_getattro == (getattrofunc)_Py_type_getattro) {
        int supress_missing_attribute_exception = 0;
        *result = _Py_type_getattro_impl((PyTypeObject*)v, name, &supress_missing_attribute_exception);
        if (supress_missing_attribute_exception) {
            // return 0 without having to clear the exception
            return 0;
        }
    }
    else if (tp->tp_getattro == (getattrofunc)_Py_module_getattro) {
        // optimization: suppress attribute error from module getattro method
        *result = _Py_module_getattro_impl((PyModuleObject*)v, name, 1);
        if (*result != NULL) {
            return 1;
        }
        if (PyErr_Occurred()) {
            return -1;
        }
        return 0;
    }
    else if (tp->tp_getattro != NULL) {
        *result = (*tp->tp_getattro)(v, name);
    }
    else if (tp->tp_getattr != NULL) {
        const char *name_str = PyUnicode_AsUTF8(name);
        if (name_str == NULL) {
            *result = NULL;
            return -1;
        }
        *result = (*tp->tp_getattr)(v, (char *)name_str);
    }
    else {
        *result = NULL;
        return 0;
    }

    if (*result != NULL) {
        return 1;
    }
    if (!PyErr_ExceptionMatches(PyExc_AttributeError)) {
        return -1;
    }
    PyErr_Clear();
    return 0;
}

int
_PyObject_LookupAttrId(PyObject *v, _Py_Identifier *name, PyObject **result)
{
    PyObject *oname = _PyUnicode_FromId(name); /* borrowed */
    if (!oname) {
        *result = NULL;
        return -1;
    }
    return  _PyObject_LookupAttr(v, oname, result);
}

int
PyObject_HasAttr(PyObject *v, PyObject *name)
{
    PyObject *res;
    if (_PyObject_LookupAttr(v, name, &res) < 0) {
        PyErr_Clear();
        return 0;
    }
    if (res == NULL) {
        return 0;
    }
    Py_DECREF(res);
    return 1;
}

int
PyObject_SetAttr(PyObject *v, PyObject *name, PyObject *value)
{
    PyTypeObject *tp = Py_TYPE(v);
    int err;

    if (!PyUnicode_Check(name)) {
        PyErr_Format(PyExc_TypeError,
                     "attribute name must be string, not '%.200s'",
                     Py_TYPE(name)->tp_name);
        return -1;
    }
    Py_INCREF(name);

    PyUnicode_InternInPlace(&name);
    if (tp->tp_setattro != NULL) {
        err = (*tp->tp_setattro)(v, name, value);
        Py_DECREF(name);
        return err;
    }
    if (tp->tp_setattr != NULL) {
        const char *name_str = PyUnicode_AsUTF8(name);
        if (name_str == NULL) {
            Py_DECREF(name);
            return -1;
        }
        err = (*tp->tp_setattr)(v, (char *)name_str, value);
        Py_DECREF(name);
        return err;
    }
    Py_DECREF(name);
    _PyObject_ASSERT(name, Py_REFCNT(name) >= 1);
    if (tp->tp_getattr == NULL && tp->tp_getattro == NULL)
        PyErr_Format(PyExc_TypeError,
                     "'%.100s' object has no attributes "
                     "(%s .%U)",
                     tp->tp_name,
                     value==NULL ? "del" : "assign to",
                     name);
    else
        PyErr_Format(PyExc_TypeError,
                     "'%.100s' object has only read-only attributes "
                     "(%s .%U)",
                     tp->tp_name,
                     value==NULL ? "del" : "assign to",
                     name);
    return -1;
}

PyObject **
_PyObject_ComputedDictPointer(PyObject *obj)
{
    PyTypeObject *tp = Py_TYPE(obj);
    assert((tp->tp_flags & Py_TPFLAGS_MANAGED_DICT) == 0);

    Py_ssize_t dictoffset = tp->tp_dictoffset;
    if (dictoffset == 0) {
        return NULL;
    }

    if (dictoffset < 0) {
        assert(dictoffset != -1);

        Py_ssize_t tsize = Py_SIZE(obj);
        if (tsize < 0) {
            tsize = -tsize;
        }
        size_t size = _PyObject_VAR_SIZE(tp, tsize);
        assert(size <= (size_t)PY_SSIZE_T_MAX);
        dictoffset += (Py_ssize_t)size;

        _PyObject_ASSERT(obj, dictoffset > 0);
        _PyObject_ASSERT(obj, dictoffset % SIZEOF_VOID_P == 0);
    }
    return (PyObject **) ((char *)obj + dictoffset);
}

/* Helper to get a pointer to an object's __dict__ slot, if any.
 * Creates the dict from inline attributes if necessary.
 * Does not set an exception.
 *
 * Note that the tp_dictoffset docs used to recommend this function,
 * so it should be treated as part of the public API.
 */
PyObject **
_PyObject_GetDictPtr(PyObject *obj)
{
    if ((Py_TYPE(obj)->tp_flags & Py_TPFLAGS_MANAGED_DICT) == 0) {
        return _PyObject_ComputedDictPointer(obj);
    }
    PyDictOrValues *dorv_ptr = _PyObject_DictOrValuesPointer(obj);
    if (_PyDictOrValues_IsValues(*dorv_ptr)) {
        PyObject *dict = _PyObject_MakeDictFromInstanceAttributes(obj, _PyDictOrValues_GetValues(*dorv_ptr));
        if (dict == NULL) {
            PyErr_Clear();
            return NULL;
        }
        dorv_ptr->dict = dict;
    }
    return &dorv_ptr->dict;
}

PyObject *
PyObject_SelfIter(PyObject *obj)
{
    return Py_NewRef(obj);
}

/* Helper used when the __next__ method is removed from a type:
   tp_iternext is never NULL and can be safely called without checking
   on every iteration.
 */

PyObject *
_PyObject_NextNotImplemented(PyObject *self)
{
    PyErr_Format(PyExc_TypeError,
                 "'%.200s' object is not iterable",
                 Py_TYPE(self)->tp_name);
    return NULL;
}


/* Specialized version of _PyObject_GenericGetAttrWithDict
   specifically for the LOAD_METHOD opcode.

   Return 1 if a method is found, 0 if it's a regular attribute
   from __dict__ or something returned by using a descriptor
   protocol.

   `method` will point to the resolved attribute or NULL.  In the
   latter case, an error will be set.
*/
int
_PyObject_GetMethod(PyObject *obj, PyObject *name, PyObject **method)
{
    int meth_found = 0;

    assert(*method == NULL);

    PyTypeObject *tp = Py_TYPE(obj);
    if (!_PyType_IsReady(tp)) {
        if (PyType_Ready(tp) < 0) {
            return 0;
        }
    }

    if (tp->tp_getattro != PyObject_GenericGetAttr || !PyUnicode_CheckExact(name)) {
        *method = PyObject_GetAttr(obj, name);
        return 0;
    }

    PyObject *descr = _PyType_Lookup(tp, name);
    descrgetfunc f = NULL;
    if (descr != NULL) {
        Py_INCREF(descr);
        if (_PyType_HasFeature(Py_TYPE(descr), Py_TPFLAGS_METHOD_DESCRIPTOR)) {
            meth_found = 1;
        } else {
            f = Py_TYPE(descr)->tp_descr_get;
            if (f != NULL && PyDescr_IsData(descr)) {
                *method = f(descr, obj, (PyObject *)Py_TYPE(obj));
                Py_DECREF(descr);
                return 0;
            }
        }
    }
    PyObject *dict;
    if ((tp->tp_flags & Py_TPFLAGS_MANAGED_DICT)) {
        PyDictOrValues* dorv_ptr = _PyObject_DictOrValuesPointer(obj);
        if (_PyDictOrValues_IsValues(*dorv_ptr)) {
            PyDictValues *values = _PyDictOrValues_GetValues(*dorv_ptr);
            PyObject *attr = _PyObject_GetInstanceAttribute(obj, values, name);
            if (attr != NULL) {
                *method = attr;
                Py_XDECREF(descr);
                return 0;
            }
            dict = NULL;
        }
        else {
            dict = dorv_ptr->dict;
        }
    }
    else {
        PyObject **dictptr = _PyObject_ComputedDictPointer(obj);
        if (dictptr != NULL) {
            dict = *dictptr;
        }
        else {
            dict = NULL;
        }
    }
    if (dict != NULL) {
        Py_INCREF(dict);
        PyObject *attr = PyDict_GetItemWithError(dict, name);
        if (attr != NULL) {
            *method = Py_NewRef(attr);
            Py_DECREF(dict);
            Py_XDECREF(descr);
            return 0;
        }
        Py_DECREF(dict);

        if (PyErr_Occurred()) {
            Py_XDECREF(descr);
            return 0;
        }
    }

    if (meth_found) {
        *method = descr;
        return 1;
    }

    if (f != NULL) {
        *method = f(descr, obj, (PyObject *)Py_TYPE(obj));
        Py_DECREF(descr);
        return 0;
    }

    if (descr != NULL) {
        *method = descr;
        return 0;
    }

    PyErr_Format(PyExc_AttributeError,
                 "'%.100s' object has no attribute '%U'",
                 tp->tp_name, name);

    set_attribute_error_context(obj, name);
    return 0;
}

/* Generic GetAttr functions - put these in your tp_[gs]etattro slot. */

PyObject *
_PyObject_GenericGetAttrWithDict(PyObject *obj, PyObject *name,
                                 PyObject *dict, int suppress)
{
    /* Make sure the logic of _PyObject_GetMethod is in sync with
       this method.

       When suppress=1, this function suppresses AttributeError.
    */

    PyTypeObject *tp = Py_TYPE(obj);
    PyObject *descr = NULL;
    PyObject *res = NULL;
    descrgetfunc f;

    if (!PyUnicode_Check(name)){
        PyErr_Format(PyExc_TypeError,
                     "attribute name must be string, not '%.200s'",
                     Py_TYPE(name)->tp_name);
        return NULL;
    }
    Py_INCREF(name);

    if (!_PyType_IsReady(tp)) {
        if (PyType_Ready(tp) < 0)
            goto done;
    }

    descr = _PyType_Lookup(tp, name);

    f = NULL;
    if (descr != NULL) {
        Py_INCREF(descr);
        f = Py_TYPE(descr)->tp_descr_get;
        if (f != NULL && PyDescr_IsData(descr)) {
            res = f(descr, obj, (PyObject *)Py_TYPE(obj));
            if (res == NULL && suppress &&
                    PyErr_ExceptionMatches(PyExc_AttributeError)) {
                PyErr_Clear();
            }
            goto done;
        }
    }
    if (dict == NULL) {
        if ((tp->tp_flags & Py_TPFLAGS_MANAGED_DICT)) {
            PyDictOrValues* dorv_ptr = _PyObject_DictOrValuesPointer(obj);
            if (_PyDictOrValues_IsValues(*dorv_ptr)) {
                PyDictValues *values = _PyDictOrValues_GetValues(*dorv_ptr);
                if (PyUnicode_CheckExact(name)) {
                    res = _PyObject_GetInstanceAttribute(obj, values, name);
                    if (res != NULL) {
                        goto done;
                    }
                }
                else {
                    dict = _PyObject_MakeDictFromInstanceAttributes(obj, values);
                    if (dict == NULL) {
                        res = NULL;
                        goto done;
                    }
                    dorv_ptr->dict = dict;
                }
            }
            else {
                dict = _PyDictOrValues_GetDict(*dorv_ptr);
            }
        }
        else {
            PyObject **dictptr = _PyObject_ComputedDictPointer(obj);
            if (dictptr) {
                dict = *dictptr;
            }
        }
    }
    if (dict != NULL) {
        Py_INCREF(dict);
        res = PyDict_GetItemWithError(dict, name);
        if (res != NULL) {
            Py_INCREF(res);
            Py_DECREF(dict);
            goto done;
        }
        else {
            Py_DECREF(dict);
            if (PyErr_Occurred()) {
                if (suppress && PyErr_ExceptionMatches(PyExc_AttributeError)) {
                    PyErr_Clear();
                }
                else {
                    goto done;
                }
            }
        }
    }

    if (f != NULL) {
        res = f(descr, obj, (PyObject *)Py_TYPE(obj));
        if (res == NULL && suppress &&
                PyErr_ExceptionMatches(PyExc_AttributeError)) {
            PyErr_Clear();
        }
        goto done;
    }

    if (descr != NULL) {
        res = descr;
        descr = NULL;
        goto done;
    }

    if (!suppress) {
        PyErr_Format(PyExc_AttributeError,
                     "'%.100s' object has no attribute '%U'",
                     tp->tp_name, name);

        set_attribute_error_context(obj, name);
    }
  done:
    Py_XDECREF(descr);
    Py_DECREF(name);
    return res;
}

PyObject *
PyObject_GenericGetAttr(PyObject *obj, PyObject *name)
{
    return _PyObject_GenericGetAttrWithDict(obj, name, NULL, 0);
}

int
_PyObject_GenericSetAttrWithDict(PyObject *obj, PyObject *name,
                                 PyObject *value, PyObject *dict)
{
    PyTypeObject *tp = Py_TYPE(obj);
    PyObject *descr;
    descrsetfunc f;
    int res = -1;

    if (!PyUnicode_Check(name)){
        PyErr_Format(PyExc_TypeError,
                     "attribute name must be string, not '%.200s'",
                     Py_TYPE(name)->tp_name);
        return -1;
    }

    if (!_PyType_IsReady(tp) && PyType_Ready(tp) < 0) {
        return -1;
    }

    Py_INCREF(name);
    Py_INCREF(tp);
    descr = _PyType_Lookup(tp, name);

    if (descr != NULL) {
        Py_INCREF(descr);
        f = Py_TYPE(descr)->tp_descr_set;
        if (f != NULL) {
            res = f(descr, obj, value);
            goto done;
        }
    }

    if (dict == NULL) {
        PyObject **dictptr;
        if ((tp->tp_flags & Py_TPFLAGS_MANAGED_DICT)) {
            PyDictOrValues *dorv_ptr = _PyObject_DictOrValuesPointer(obj);
            if (_PyDictOrValues_IsValues(*dorv_ptr)) {
                res = _PyObject_StoreInstanceAttribute(
                    obj, _PyDictOrValues_GetValues(*dorv_ptr), name, value);
                goto error_check;
            }
            dictptr = &dorv_ptr->dict;
        }
        else {
            dictptr = _PyObject_ComputedDictPointer(obj);
        }
        if (dictptr == NULL) {
            if (descr == NULL) {
                PyErr_Format(PyExc_AttributeError,
                            "'%.100s' object has no attribute '%U'",
                            tp->tp_name, name);
            }
            else {
                PyErr_Format(PyExc_AttributeError,
                            "'%.100s' object attribute '%U' is read-only",
                            tp->tp_name, name);
            }
            goto done;
        }
        else {
            res = _PyObjectDict_SetItem(tp, dictptr, name, value);
        }
    }
    else {
        Py_INCREF(dict);
        if (value == NULL)
            res = PyDict_DelItem(dict, name);
        else
            res = PyDict_SetItem(dict, name, value);
        Py_DECREF(dict);
    }
  error_check:
    if (res < 0 && PyErr_ExceptionMatches(PyExc_KeyError)) {
        if (PyType_IsSubtype(tp, &PyType_Type)) {
            PyErr_Format(PyExc_AttributeError,
                         "type object '%.50s' has no attribute '%U'",
                         ((PyTypeObject*)obj)->tp_name, name);
        }
        else {
            PyErr_Format(PyExc_AttributeError,
                         "'%.100s' object has no attribute '%U'",
                         tp->tp_name, name);
        }
    }
  done:
    Py_XDECREF(descr);
    Py_DECREF(tp);
    Py_DECREF(name);
    return res;
}

int
PyObject_GenericSetAttr(PyObject *obj, PyObject *name, PyObject *value)
{
    return _PyObject_GenericSetAttrWithDict(obj, name, value, NULL);
}

int
PyObject_GenericSetDict(PyObject *obj, PyObject *value, void *context)
{
    PyObject **dictptr = _PyObject_GetDictPtr(obj);
    if (dictptr == NULL) {
        if (_PyType_HasFeature(Py_TYPE(obj), Py_TPFLAGS_MANAGED_DICT) &&
            _PyDictOrValues_IsValues(*_PyObject_DictOrValuesPointer(obj)))
        {
            /* Was unable to convert to dict */
            PyErr_NoMemory();
        }
        else {
            PyErr_SetString(PyExc_AttributeError,
                            "This object has no __dict__");
        }
        return -1;
    }
    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError, "cannot delete __dict__");
        return -1;
    }
    if (!PyDict_Check(value)) {
        PyErr_Format(PyExc_TypeError,
                     "__dict__ must be set to a dictionary, "
                     "not a '%.200s'", Py_TYPE(value)->tp_name);
        return -1;
    }
    Py_XSETREF(*dictptr, Py_NewRef(value));
    return 0;
}


/* Test a value used as condition, e.g., in a while or if statement.
   Return -1 if an error occurred */

int
PyObject_IsTrue(PyObject *v)
{
    Py_ssize_t res;
    if (v == Py_True)
        return 1;
    if (v == Py_False)
        return 0;
    if (v == Py_None)
        return 0;
    else if (Py_TYPE(v)->tp_as_number != NULL &&
             Py_TYPE(v)->tp_as_number->nb_bool != NULL)
        res = (*Py_TYPE(v)->tp_as_number->nb_bool)(v);
    else if (Py_TYPE(v)->tp_as_mapping != NULL &&
             Py_TYPE(v)->tp_as_mapping->mp_length != NULL)
        res = (*Py_TYPE(v)->tp_as_mapping->mp_length)(v);
    else if (Py_TYPE(v)->tp_as_sequence != NULL &&
             Py_TYPE(v)->tp_as_sequence->sq_length != NULL)
        res = (*Py_TYPE(v)->tp_as_sequence->sq_length)(v);
    else
        return 1;
    /* if it is negative, it should be either -1 or -2 */
    return (res > 0) ? 1 : Py_SAFE_DOWNCAST(res, Py_ssize_t, int);
}

/* equivalent of 'not v'
   Return -1 if an error occurred */

int
PyObject_Not(PyObject *v)
{
    int res;
    res = PyObject_IsTrue(v);
    if (res < 0)
        return res;
    return res == 0;
}

/* Test whether an object can be called */

int
PyCallable_Check(PyObject *x)
{
    if (x == NULL)
        return 0;
    return Py_TYPE(x)->tp_call != NULL;
}


/* Helper for PyObject_Dir without arguments: returns the local scope. */
static PyObject *
_dir_locals(void)
{
    PyObject *names;
    PyObject *locals;

    locals = _PyEval_GetFrameLocals();
    if (locals == NULL)
        return NULL;

    names = PyMapping_Keys(locals);
    Py_DECREF(locals);
    if (!names) {
        return NULL;
    }
    if (!PyList_Check(names)) {
        PyErr_Format(PyExc_TypeError,
            "dir(): expected keys() of locals to be a list, "
            "not '%.200s'", Py_TYPE(names)->tp_name);
        Py_DECREF(names);
        return NULL;
    }
    if (PyList_Sort(names)) {
        Py_DECREF(names);
        return NULL;
    }
    return names;
}

/* Helper for PyObject_Dir: object introspection. */
static PyObject *
_dir_object(PyObject *obj)
{
    PyObject *result, *sorted;
    PyObject *dirfunc = _PyObject_LookupSpecial(obj, &_Py_ID(__dir__));

    assert(obj != NULL);
    if (dirfunc == NULL) {
        if (!PyErr_Occurred())
            PyErr_SetString(PyExc_TypeError, "object does not provide __dir__");
        return NULL;
    }
    /* use __dir__ */
    result = _PyObject_CallNoArgs(dirfunc);
    Py_DECREF(dirfunc);
    if (result == NULL)
        return NULL;
    /* return sorted(result) */
    sorted = PySequence_List(result);
    Py_DECREF(result);
    if (sorted == NULL)
        return NULL;
    if (PyList_Sort(sorted)) {
        Py_DECREF(sorted);
        return NULL;
    }
    return sorted;
}

/* Implementation of dir() -- if obj is NULL, returns the names in the current
   (local) scope.  Otherwise, performs introspection of the object: returns a
   sorted list of attribute names (supposedly) accessible from the object
*/
PyObject *
PyObject_Dir(PyObject *obj)
{
    return (obj == NULL) ? _dir_locals() : _dir_object(obj);
}

/*
None is a non-NULL undefined value.
There is (and should be!) no way to create other objects of this type,
so there is exactly one (which is indestructible, by the way).
*/

/* ARGSUSED */
static PyObject *
none_repr(PyObject *op)
{
    return PyUnicode_FromString("None");
}

static void
none_dealloc(PyObject* none)
{
    /* This should never get called, but we also don't want to SEGV if
     * we accidentally decref None out of existence. Instead,
     * since None is an immortal object, re-set the reference count.
     */
    _Py_SetImmortal(none);
}

static PyObject *
none_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    if (PyTuple_GET_SIZE(args) || (kwargs && PyDict_GET_SIZE(kwargs))) {
        PyErr_SetString(PyExc_TypeError, "NoneType takes no arguments");
        return NULL;
    }
    Py_RETURN_NONE;
}

static int
none_bool(PyObject *v)
{
    return 0;
}

static Py_hash_t none_hash(PyObject *v)
{
    return 0xFCA86420;
}

static PyNumberMethods none_as_number = {
    0,                          /* nb_add */
    0,                          /* nb_subtract */
    0,                          /* nb_multiply */
    0,                          /* nb_remainder */
    0,                          /* nb_divmod */
    0,                          /* nb_power */
    0,                          /* nb_negative */
    0,                          /* nb_positive */
    0,                          /* nb_absolute */
    (inquiry)none_bool,         /* nb_bool */
    0,                          /* nb_invert */
    0,                          /* nb_lshift */
    0,                          /* nb_rshift */
    0,                          /* nb_and */
    0,                          /* nb_xor */
    0,                          /* nb_or */
    0,                          /* nb_int */
    0,                          /* nb_reserved */
    0,                          /* nb_float */
    0,                          /* nb_inplace_add */
    0,                          /* nb_inplace_subtract */
    0,                          /* nb_inplace_multiply */
    0,                          /* nb_inplace_remainder */
    0,                          /* nb_inplace_power */
    0,                          /* nb_inplace_lshift */
    0,                          /* nb_inplace_rshift */
    0,                          /* nb_inplace_and */
    0,                          /* nb_inplace_xor */
    0,                          /* nb_inplace_or */
    0,                          /* nb_floor_divide */
    0,                          /* nb_true_divide */
    0,                          /* nb_inplace_floor_divide */
    0,                          /* nb_inplace_true_divide */
    0,                          /* nb_index */
};

PyTypeObject _PyNone_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "NoneType",
    0,
    0,
    none_dealloc,       /*tp_dealloc*/
    0,                  /*tp_vectorcall_offset*/
    0,                  /*tp_getattr*/
    0,                  /*tp_setattr*/
    0,                  /*tp_as_async*/
    none_repr,          /*tp_repr*/
    &none_as_number,    /*tp_as_number*/
    0,                  /*tp_as_sequence*/
    0,                  /*tp_as_mapping*/
    (hashfunc)none_hash,/*tp_hash */
    0,                  /*tp_call */
    0,                  /*tp_str */
    0,                  /*tp_getattro */
    0,                  /*tp_setattro */
    0,                  /*tp_as_buffer */
    Py_TPFLAGS_DEFAULT, /*tp_flags */
    0,                  /*tp_doc */
    0,                  /*tp_traverse */
    0,                  /*tp_clear */
    _Py_BaseObject_RichCompare, /*tp_richcompare */
    0,                  /*tp_weaklistoffset */
    0,                  /*tp_iter */
    0,                  /*tp_iternext */
    0,                  /*tp_methods */
    0,                  /*tp_members */
    0,                  /*tp_getset */
    0,                  /*tp_base */
    0,                  /*tp_dict */
    0,                  /*tp_descr_get */
    0,                  /*tp_descr_set */
    0,                  /*tp_dictoffset */
    0,                  /*tp_init */
    0,                  /*tp_alloc */
    none_new,           /*tp_new */
};

PyObject _Py_NoneStruct = {
    _PyObject_EXTRA_INIT
    { _Py_IMMORTAL_REFCNT },
    &_PyNone_Type
};

/* NotImplemented is an object that can be used to signal that an
   operation is not implemented for the given type combination. */

static PyObject *
NotImplemented_repr(PyObject *op)
{
    return PyUnicode_FromString("NotImplemented");
}

static PyObject *
NotImplemented_reduce(PyObject *op, PyObject *Py_UNUSED(ignored))
{
    return PyUnicode_FromString("NotImplemented");
}

static PyMethodDef notimplemented_methods[] = {
    {"__reduce__", NotImplemented_reduce, METH_NOARGS, NULL},
    {NULL, NULL}
};

static PyObject *
notimplemented_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    if (PyTuple_GET_SIZE(args) || (kwargs && PyDict_GET_SIZE(kwargs))) {
        PyErr_SetString(PyExc_TypeError, "NotImplementedType takes no arguments");
        return NULL;
    }
    Py_RETURN_NOTIMPLEMENTED;
}

static void
notimplemented_dealloc(PyObject *notimplemented)
{
    /* This should never get called, but we also don't want to SEGV if
     * we accidentally decref NotImplemented out of existence. Instead,
     * since Notimplemented is an immortal object, re-set the reference count.
     */
    _Py_SetImmortal(notimplemented);
}

static int
notimplemented_bool(PyObject *v)
{
    if (PyErr_WarnEx(PyExc_DeprecationWarning,
                     "NotImplemented should not be used in a boolean context",
                     1) < 0)
    {
        return -1;
    }
    return 1;
}

static PyNumberMethods notimplemented_as_number = {
    .nb_bool = notimplemented_bool,
};

PyTypeObject _PyNotImplemented_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "NotImplementedType",
    0,
    0,
    notimplemented_dealloc,       /*tp_dealloc*/ /*never called*/
    0,                  /*tp_vectorcall_offset*/
    0,                  /*tp_getattr*/
    0,                  /*tp_setattr*/
    0,                  /*tp_as_async*/
    NotImplemented_repr,        /*tp_repr*/
    &notimplemented_as_number,  /*tp_as_number*/
    0,                  /*tp_as_sequence*/
    0,                  /*tp_as_mapping*/
    0,                  /*tp_hash */
    0,                  /*tp_call */
    0,                  /*tp_str */
    0,                  /*tp_getattro */
    0,                  /*tp_setattro */
    0,                  /*tp_as_buffer */
    Py_TPFLAGS_DEFAULT, /*tp_flags */
    0,                  /*tp_doc */
    0,                  /*tp_traverse */
    0,                  /*tp_clear */
    0,                  /*tp_richcompare */
    0,                  /*tp_weaklistoffset */
    0,                  /*tp_iter */
    0,                  /*tp_iternext */
    notimplemented_methods, /*tp_methods */
    0,                  /*tp_members */
    0,                  /*tp_getset */
    0,                  /*tp_base */
    0,                  /*tp_dict */
    0,                  /*tp_descr_get */
    0,                  /*tp_descr_set */
    0,                  /*tp_dictoffset */
    0,                  /*tp_init */
    0,                  /*tp_alloc */
    notimplemented_new, /*tp_new */
};

PyObject _Py_NotImplementedStruct = {
    _PyObject_EXTRA_INIT
    { _Py_IMMORTAL_REFCNT },
    &_PyNotImplemented_Type
};


void
_PyObject_InitState(PyInterpreterState *interp)
{
#ifdef Py_TRACE_REFS
    if (!_Py_IsMainInterpreter(interp)) {
        init_refchain(interp);
    }
#endif
}


extern PyTypeObject _Py_GenericAliasIterType;
extern PyTypeObject _PyMemoryIter_Type;
extern PyTypeObject _PyLineIterator;
extern PyTypeObject _PyPositionsIterator;
extern PyTypeObject _PyLegacyEventHandler_Type;

static PyTypeObject* static_types[] = {
    // The two most important base types: must be initialized first and
    // deallocated last.
    &PyBaseObject_Type,
    &PyType_Type,

    // Static types with base=&PyBaseObject_Type
    &PyAsyncGen_Type,
    &PyByteArrayIter_Type,
    &PyByteArray_Type,
    &PyBytesIter_Type,
    &PyBytes_Type,
    &PyCFunction_Type,
    &PyCallIter_Type,
    &PyCapsule_Type,
    &PyCell_Type,
    &PyClassMethodDescr_Type,
    &PyClassMethod_Type,
    &PyCode_Type,
    &PyComplex_Type,
    &PyContextToken_Type,
    &PyContextVar_Type,
    &PyContext_Type,
    &PyCoro_Type,
    &PyDictItems_Type,
    &PyDictIterItem_Type,
    &PyDictIterKey_Type,
    &PyDictIterValue_Type,
    &PyDictKeys_Type,
    &PyDictProxy_Type,
    &PyDictRevIterItem_Type,
    &PyDictRevIterKey_Type,
    &PyDictRevIterValue_Type,
    &PyDictValues_Type,
    &PyDict_Type,
    &PyEllipsis_Type,
    &PyEnum_Type,
    &PyFilter_Type,
    &PyFloat_Type,
    &PyFrame_Type,
    &PyFrozenSet_Type,
    &PyFunction_Type,
    &PyGen_Type,
    &PyGetSetDescr_Type,
    &PyInstanceMethod_Type,
    &PyListIter_Type,
    &PyListRevIter_Type,
    &PyList_Type,
    &PyLongRangeIter_Type,
    &PyLong_Type,
    &PyMap_Type,
    &PyMemberDescr_Type,
    &PyMemoryView_Type,
    &PyMethodDescr_Type,
    &PyMethod_Type,
    &PyModuleDef_Type,
    &PyModule_Type,
    &PyODictIter_Type,
    &PyPickleBuffer_Type,
    &PyProperty_Type,
    &PyRangeIter_Type,
    &PyRange_Type,
    &PyReversed_Type,
    &PySTEntry_Type,
    &PySeqIter_Type,
    &PySetIter_Type,
    &PySet_Type,
    &PySlice_Type,
    &PyStaticMethod_Type,
    &PyStdPrinter_Type,
    &PySuper_Type,
    &PyTraceBack_Type,
    &PyTupleIter_Type,
    &PyTuple_Type,
    &PyUnicodeIter_Type,
    &PyUnicode_Type,
    &PyWrapperDescr_Type,
    &PyZip_Type,
    &Py_GenericAliasType,
    &_PyAnextAwaitable_Type,
    &_PyAsyncGenASend_Type,
    &_PyAsyncGenAThrow_Type,
    &_PyAsyncGenWrappedValue_Type,
    &_PyBufferWrapper_Type,
    &_PyContextTokenMissing_Type,
    &_PyCoroWrapper_Type,
    &_Py_GenericAliasIterType,
    &_PyHamtItems_Type,
    &_PyHamtKeys_Type,
    &_PyHamtValues_Type,
    &_PyHamt_ArrayNode_Type,
    &_PyHamt_BitmapNode_Type,
    &_PyHamt_CollisionNode_Type,
    &_PyHamt_Type,
    &_PyLegacyEventHandler_Type,
    &_PyInterpreterID_Type,
    &_PyLineIterator,
    &_PyManagedBuffer_Type,
    &_PyMemoryIter_Type,
    &_PyMethodWrapper_Type,
    &_PyNamespace_Type,
    &_PyNone_Type,
    &_PyNotImplemented_Type,
    &_PyPositionsIterator,
    &_PyUnicodeASCIIIter_Type,
    &_PyUnion_Type,
    &_PyWeakref_CallableProxyType,
    &_PyWeakref_ProxyType,
    &_PyWeakref_RefType,
    &_PyTypeAlias_Type,

    // subclasses: _PyTypes_FiniTypes() deallocates them before their base
    // class
    &PyBool_Type,         // base=&PyLong_Type
    &PyCMethod_Type,      // base=&PyCFunction_Type
    &PyODictItems_Type,   // base=&PyDictItems_Type
    &PyODictKeys_Type,    // base=&PyDictKeys_Type
    &PyODictValues_Type,  // base=&PyDictValues_Type
    &PyODict_Type,        // base=&PyDict_Type
};


PyStatus
_PyTypes_InitTypes(PyInterpreterState *interp)
{
    // All other static types (unless initialized elsewhere)
    for (size_t i=0; i < Py_ARRAY_LENGTH(static_types); i++) {
        PyTypeObject *type = static_types[i];
        if (_PyStaticType_InitBuiltin(interp, type) < 0) {
            return _PyStatus_ERR("Can't initialize builtin type");
        }
        if (type == &PyType_Type) {
            // Sanitify checks of the two most important types
            assert(PyBaseObject_Type.tp_base == NULL);
            assert(PyType_Type.tp_base == &PyBaseObject_Type);
        }
    }

    // Must be after static types are initialized
    if (_Py_initialize_generic(interp) < 0) {
        return _PyStatus_ERR("Can't initialize generic types");
    }

    return _PyStatus_OK();
}


// Best-effort function clearing static types.
//
// Don't deallocate a type if it still has subclasses. If a Py_Finalize()
// sub-function is interrupted by CTRL+C or fails with MemoryError, some
// subclasses are not cleared properly. Leave the static type unchanged in this
// case.
void
_PyTypes_FiniTypes(PyInterpreterState *interp)
{
    // Deallocate types in the reverse order to deallocate subclasses before
    // their base classes.
    for (Py_ssize_t i=Py_ARRAY_LENGTH(static_types)-1; i>=0; i--) {
        PyTypeObject *type = static_types[i];
        _PyStaticType_Dealloc(interp, type);
    }
}


static inline void
new_reference(PyObject *op)
{
    if (_PyRuntime.tracemalloc.config.tracing) {
        _PyTraceMalloc_NewReference(op);
    }
    // Skip the immortal object check in Py_SET_REFCNT; always set refcnt to 1
    op->ob_refcnt = 1;
#ifdef Py_TRACE_REFS
    _Py_AddToAllObjects(op, 1);
#endif
}

void
_Py_NewReference(PyObject *op)
{
#ifdef Py_REF_DEBUG
    reftotal_increment(_PyInterpreterState_GET());
#endif
    new_reference(op);
}

void
_Py_NewReferenceNoTotal(PyObject *op)
{
    new_reference(op);
}


#ifdef Py_TRACE_REFS
void
_Py_ForgetReference(PyObject *op)
{
    if (Py_REFCNT(op) < 0) {
        _PyObject_ASSERT_FAILED_MSG(op, "negative refcnt");
    }

    PyObject *refchain = REFCHAIN(_PyInterpreterState_GET());
    if (op == refchain ||
        op->_ob_prev->_ob_next != op || op->_ob_next->_ob_prev != op)
    {
        _PyObject_ASSERT_FAILED_MSG(op, "invalid object chain");
    }

#ifdef SLOW_UNREF_CHECK
    PyObject *p;
    for (p = refchain->_ob_next; p != refchain; p = p->_ob_next) {
        if (p == op) {
            break;
        }
    }
    if (p == refchain) {
        /* Not found */
        _PyObject_ASSERT_FAILED_MSG(op,
                                    "object not found in the objects list");
    }
#endif

    op->_ob_next->_ob_prev = op->_ob_prev;
    op->_ob_prev->_ob_next = op->_ob_next;
    op->_ob_next = op->_ob_prev = NULL;
}

/* Print all live objects.  Because PyObject_Print is called, the
 * interpreter must be in a healthy state.
 */
void
_Py_PrintReferences(PyInterpreterState *interp, FILE *fp)
{
    PyObject *op;
    if (interp == NULL) {
        interp = _PyInterpreterState_Main();
    }
    fprintf(fp, "Remaining objects:\n");
    PyObject *refchain = REFCHAIN(interp);
    for (op = refchain->_ob_next; op != refchain; op = op->_ob_next) {
        fprintf(fp, "%p [%zd] ", (void *)op, Py_REFCNT(op));
        if (PyObject_Print(op, fp, 0) != 0) {
            PyErr_Clear();
        }
        putc('\n', fp);
    }
}

/* Print the addresses of all live objects.  Unlike _Py_PrintReferences, this
 * doesn't make any calls to the Python C API, so is always safe to call.
 */
// XXX This function is not safe to use if the interpreter has been
// freed or is in an unhealthy state (e.g. late in finalization).
// The call in Py_FinalizeEx() is okay since the main interpreter
// is statically allocated.
void
_Py_PrintReferenceAddresses(PyInterpreterState *interp, FILE *fp)
{
    PyObject *op;
    PyObject *refchain = REFCHAIN(interp);
    fprintf(fp, "Remaining object addresses:\n");
    for (op = refchain->_ob_next; op != refchain; op = op->_ob_next)
        fprintf(fp, "%p [%zd] %s\n", (void *)op,
            Py_REFCNT(op), Py_TYPE(op)->tp_name);
}

/* The implementation of sys.getobjects(). */
PyObject *
_Py_GetObjects(PyObject *self, PyObject *args)
{
    int i, n;
    PyObject *t = NULL;
    PyObject *res, *op;
    PyInterpreterState *interp = _PyInterpreterState_GET();

    if (!PyArg_ParseTuple(args, "i|O", &n, &t))
        return NULL;
    PyObject *refchain = REFCHAIN(interp);
    op = refchain->_ob_next;
    res = PyList_New(0);
    if (res == NULL)
        return NULL;
    for (i = 0; (n == 0 || i < n) && op != refchain; i++) {
        while (op == self || op == args || op == res || op == t ||
               (t != NULL && !Py_IS_TYPE(op, (PyTypeObject *) t))) {
            op = op->_ob_next;
            if (op == refchain)
                return res;
        }
        if (PyList_Append(res, op) < 0) {
            Py_DECREF(res);
            return NULL;
        }
        op = op->_ob_next;
    }
    return res;
}

#undef REFCHAIN

#endif  /* Py_TRACE_REFS */


/* Hack to force loading of abstract.o */
Py_ssize_t (*_Py_abstract_hack)(PyObject *) = PyObject_Size;


void
_PyObject_DebugTypeStats(FILE *out)
{
    _PyDict_DebugMallocStats(out);
    _PyFloat_DebugMallocStats(out);
    _PyList_DebugMallocStats(out);
    _PyTuple_DebugMallocStats(out);
}

/* These methods are used to control infinite recursion in repr, str, print,
   etc.  Container objects that may recursively contain themselves,
   e.g. builtin dictionaries and lists, should use Py_ReprEnter() and
   Py_ReprLeave() to avoid infinite recursion.

   Py_ReprEnter() returns 0 the first time it is called for a particular
   object and 1 every time thereafter.  It returns -1 if an exception
   occurred.  Py_ReprLeave() has no return value.

   See dictobject.c and listobject.c for examples of use.
*/

int
Py_ReprEnter(PyObject *obj)
{
    PyObject *dict;
    PyObject *list;
    Py_ssize_t i;

    dict = PyThreadState_GetDict();
    /* Ignore a missing thread-state, so that this function can be called
       early on startup. */
    if (dict == NULL)
        return 0;
    list = PyDict_GetItemWithError(dict, &_Py_ID(Py_Repr));
    if (list == NULL) {
        if (PyErr_Occurred()) {
            return -1;
        }
        list = PyList_New(0);
        if (list == NULL)
            return -1;
        if (PyDict_SetItem(dict, &_Py_ID(Py_Repr), list) < 0)
            return -1;
        Py_DECREF(list);
    }
    i = PyList_GET_SIZE(list);
    while (--i >= 0) {
        if (PyList_GET_ITEM(list, i) == obj)
            return 1;
    }
    if (PyList_Append(list, obj) < 0)
        return -1;
    return 0;
}

void
Py_ReprLeave(PyObject *obj)
{
    PyObject *dict;
    PyObject *list;
    Py_ssize_t i;

    PyObject *exc = PyErr_GetRaisedException();

    dict = PyThreadState_GetDict();
    if (dict == NULL)
        goto finally;

    list = PyDict_GetItemWithError(dict, &_Py_ID(Py_Repr));
    if (list == NULL || !PyList_Check(list))
        goto finally;

    i = PyList_GET_SIZE(list);
    /* Count backwards because we always expect obj to be list[-1] */
    while (--i >= 0) {
        if (PyList_GET_ITEM(list, i) == obj) {
            PyList_SetSlice(list, i, i + 1, NULL);
            break;
        }
    }

finally:
    /* ignore exceptions because there is no way to report them. */
    PyErr_SetRaisedException(exc);
}

/* Trashcan support. */

#define _PyTrash_UNWIND_LEVEL 50

/* Add op to the gcstate->trash_delete_later list.  Called when the current
 * call-stack depth gets large.  op must be a currently untracked gc'ed
 * object, with refcount 0.  Py_DECREF must already have been called on it.
 */
static void
_PyTrash_thread_deposit_object(struct _py_trashcan *trash, PyObject *op)
{
    _PyObject_ASSERT(op, _PyObject_IS_GC(op));
    _PyObject_ASSERT(op, !_PyObject_GC_IS_TRACKED(op));
    _PyObject_ASSERT(op, Py_REFCNT(op) == 0);
    _PyGCHead_SET_PREV(_Py_AS_GC(op), (PyGC_Head*)trash->delete_later);
    trash->delete_later = op;
}

/* Deallocate all the objects in the gcstate->trash_delete_later list.
 * Called when the call-stack unwinds again. */
static void
_PyTrash_thread_destroy_chain(struct _py_trashcan *trash)
{
    /* We need to increase trash_delete_nesting here, otherwise,
       _PyTrash_thread_destroy_chain will be called recursively
       and then possibly crash.  An example that may crash without
       increase:
           N = 500000  # need to be large enough
           ob = object()
           tups = [(ob,) for i in range(N)]
           for i in range(49):
               tups = [(tup,) for tup in tups]
           del tups
    */
    assert(trash->delete_nesting == 0);
    ++trash->delete_nesting;
    while (trash->delete_later) {
        PyObject *op = trash->delete_later;
        destructor dealloc = Py_TYPE(op)->tp_dealloc;

        trash->delete_later =
            (PyObject*) _PyGCHead_PREV(_Py_AS_GC(op));

        /* Call the deallocator directly.  This used to try to
         * fool Py_DECREF into calling it indirectly, but
         * Py_DECREF was already called on this object, and in
         * assorted non-release builds calling Py_DECREF again ends
         * up distorting allocation statistics.
         */
        _PyObject_ASSERT(op, Py_REFCNT(op) == 0);
        (*dealloc)(op);
        assert(trash->delete_nesting == 1);
    }
    --trash->delete_nesting;
}


static struct _py_trashcan *
_PyTrash_get_state(PyThreadState *tstate)
{
    if (tstate != NULL) {
        return &tstate->trash;
    }
    // The current thread must be finalizing.
    // Fall back to using thread-local state.
    // XXX Use thread-local variable syntax?
    assert(PyThread_tss_is_created(&_PyRuntime.trashTSSkey));
    struct _py_trashcan *trash =
        (struct _py_trashcan *)PyThread_tss_get(&_PyRuntime.trashTSSkey);
    if (trash == NULL) {
        trash = PyMem_RawMalloc(sizeof(struct _py_trashcan));
        if (trash == NULL) {
            Py_FatalError("Out of memory");
        }
        PyThread_tss_set(&_PyRuntime.trashTSSkey, (void *)trash);
    }
    return trash;
}

static void
_PyTrash_clear_state(PyThreadState *tstate)
{
    if (tstate != NULL) {
        assert(tstate->trash.delete_later == NULL);
        return;
    }
    if (PyThread_tss_is_created(&_PyRuntime.trashTSSkey)) {
        struct _py_trashcan *trash =
            (struct _py_trashcan *)PyThread_tss_get(&_PyRuntime.trashTSSkey);
        if (trash != NULL) {
            PyThread_tss_set(&_PyRuntime.trashTSSkey, (void *)NULL);
            PyMem_RawFree(trash);
        }
    }
}


int
_PyTrash_begin(PyThreadState *tstate, PyObject *op)
{
    // XXX Make sure the GIL is held.
    struct _py_trashcan *trash = _PyTrash_get_state(tstate);
    if (trash->delete_nesting >= _PyTrash_UNWIND_LEVEL) {
        /* Store the object (to be deallocated later) and jump past
         * Py_TRASHCAN_END, skipping the body of the deallocator */
        _PyTrash_thread_deposit_object(trash, op);
        return 1;
    }
    ++trash->delete_nesting;
    return 0;
}


void
_PyTrash_end(PyThreadState *tstate)
{
    // XXX Make sure the GIL is held.
    struct _py_trashcan *trash = _PyTrash_get_state(tstate);
    --trash->delete_nesting;
    if (trash->delete_nesting <= 0) {
        if (trash->delete_later != NULL) {
            _PyTrash_thread_destroy_chain(trash);
        }
        _PyTrash_clear_state(tstate);
    }
}


/* bpo-40170: It's only be used in Py_TRASHCAN_BEGIN macro to hide
   implementation details. */
int
_PyTrash_cond(PyObject *op, destructor dealloc)
{
    return Py_TYPE(op)->tp_dealloc == dealloc;
}


void _Py_NO_RETURN
_PyObject_AssertFailed(PyObject *obj, const char *expr, const char *msg,
                       const char *file, int line, const char *function)
{
    fprintf(stderr, "%s:%d: ", file, line);
    if (function) {
        fprintf(stderr, "%s: ", function);
    }
    fflush(stderr);

    if (expr) {
        fprintf(stderr, "Assertion \"%s\" failed", expr);
    }
    else {
        fprintf(stderr, "Assertion failed");
    }
    fflush(stderr);

    if (msg) {
        fprintf(stderr, ": %s", msg);
    }
    fprintf(stderr, "\n");
    fflush(stderr);

    if (_PyObject_IsFreed(obj)) {
        /* It seems like the object memory has been freed:
           don't access it to prevent a segmentation fault. */
        fprintf(stderr, "<object at %p is freed>\n", obj);
        fflush(stderr);
    }
    else {
        /* Display the traceback where the object has been allocated.
           Do it before dumping repr(obj), since repr() is more likely
           to crash than dumping the traceback. */
        PyTypeObject *type = Py_TYPE(obj);
        const size_t presize = _PyType_PreHeaderSize(type);
        void *ptr = (void *)((char *)obj - presize);
        _PyMem_DumpTraceback(fileno(stderr), ptr);

        /* This might succeed or fail, but we're about to abort, so at least
           try to provide any extra info we can: */
        _PyObject_Dump(obj);

        fprintf(stderr, "\n");
        fflush(stderr);
    }

    Py_FatalError("_PyObject_AssertFailed");
}


void
_Py_Dealloc(PyObject *op)
{
    PyTypeObject *type = Py_TYPE(op);
    destructor dealloc = type->tp_dealloc;
#ifdef Py_DEBUG
    PyThreadState *tstate = _PyThreadState_GET();
    PyObject *old_exc = tstate != NULL ? tstate->current_exception : NULL;
    // Keep the old exception type alive to prevent undefined behavior
    // on (tstate->curexc_type != old_exc_type) below
    Py_XINCREF(old_exc);
    // Make sure that type->tp_name remains valid
    Py_INCREF(type);
#endif

#ifdef Py_TRACE_REFS
    _Py_ForgetReference(op);
#endif
    (*dealloc)(op);

#ifdef Py_DEBUG
    // gh-89373: The tp_dealloc function must leave the current exception
    // unchanged.
    if (tstate != NULL && tstate->current_exception != old_exc) {
        const char *err;
        if (old_exc == NULL) {
            err = "Deallocator of type '%s' raised an exception";
        }
        else if (tstate->current_exception == NULL) {
            err = "Deallocator of type '%s' cleared the current exception";
        }
        else {
            // It can happen if dealloc() normalized the current exception.
            // A deallocator function must not change the current exception,
            // not even normalize it.
            err = "Deallocator of type '%s' overrode the current exception";
        }
        _Py_FatalErrorFormat(__func__, err, type->tp_name);
    }
    Py_XDECREF(old_exc);
    Py_DECREF(type);
#endif
}


PyObject **
PyObject_GET_WEAKREFS_LISTPTR(PyObject *op)
{
    return _PyObject_GET_WEAKREFS_LISTPTR(op);
}


#undef Py_NewRef
#undef Py_XNewRef

// Export Py_NewRef() and Py_XNewRef() as regular functions for the stable ABI.
PyObject*
Py_NewRef(PyObject *obj)
{
    return _Py_NewRef(obj);
}

PyObject*
Py_XNewRef(PyObject *obj)
{
    return _Py_XNewRef(obj);
}

#undef Py_Is
#undef Py_IsNone
#undef Py_IsTrue
#undef Py_IsFalse

// Export Py_Is(), Py_IsNone(), Py_IsTrue(), Py_IsFalse() as regular functions
// for the stable ABI.
int Py_Is(PyObject *x, PyObject *y)
{
    return (x == y);
}

int Py_IsNone(PyObject *x)
{
    return Py_Is(x, Py_None);
}

int Py_IsTrue(PyObject *x)
{
    return Py_Is(x, Py_True);
}

int Py_IsFalse(PyObject *x)
{
    return Py_Is(x, Py_False);
}

#ifdef __cplusplus
}
#endif
