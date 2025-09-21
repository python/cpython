/*
 * Copyright (c) 2008-2012 Stefan Krah. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include <Python.h>
#include "pycore_pystate.h"       // _PyThreadState_GET()
#include "pycore_typeobject.h"
#include "complexobject.h"

#include <mpdecimal.h>

// Reuse config from mpdecimal.h if present.
#if defined(MPD_CONFIG_64)
  #ifndef CONFIG_64
    #define CONFIG_64 MPD_CONFIG_64
  #endif
#elif defined(MPD_CONFIG_32)
  #ifndef CONFIG_32
    #define CONFIG_32 MPD_CONFIG_32
  #endif
#endif

#include <ctype.h>                // isascii()
#include <stdlib.h>

#ifdef EXTRA_FUNCTIONALITY
  #define _PY_DEC_ROUND_GUARD MPD_ROUND_GUARD
#else
  #define _PY_DEC_ROUND_GUARD (MPD_ROUND_GUARD-1)
#endif

#include "clinic/_decimal.c.h"

/*[clinic input]
module _decimal
class _decimal.Decimal "PyObject *" "&dec_spec"
class _decimal.Context "PyObject *" "&context_spec"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=a6a6c0bdf4e576ef]*/

struct PyDecContextObject;
struct DecCondMap;

typedef struct {
    PyTypeObject *PyDecContextManager_Type;
    PyTypeObject *PyDecContext_Type;
    PyTypeObject *PyDecSignalDictMixin_Type;
    PyTypeObject *PyDec_Type;
    PyTypeObject *PyDecSignalDict_Type;
    PyTypeObject *DecimalTuple;

    /* Top level Exception; inherits from ArithmeticError */
    PyObject *DecimalException;

#ifndef WITH_DECIMAL_CONTEXTVAR
    /* Key for thread state dictionary */
    PyObject *tls_context_key;
    /* Invariant: NULL or a strong reference to the most recently accessed
       thread local context. */
    struct PyDecContextObject *cached_context;  /* Not borrowed */
#else
    PyObject *current_context_var;
#endif

    /* Template for creating new thread contexts, calling Context() without
     * arguments and initializing the module_context on first access. */
    PyObject *default_context_template;

    /* Basic and extended context templates */
    PyObject *basic_context_template;
    PyObject *extended_context_template;

    PyObject *round_map[_PY_DEC_ROUND_GUARD];

    /* Convert rationals for comparison */
    PyObject *Rational;

    /* Invariant: NULL or pointer to _pydecimal.Decimal */
    PyObject *PyDecimal;

    PyObject *SignalTuple;

    struct DecCondMap *signal_map;
    struct DecCondMap *cond_map;

    /* External C-API functions */
    binaryfunc _py_long_multiply;
    binaryfunc _py_long_floor_divide;
    ternaryfunc _py_long_power;
    unaryfunc _py_float_abs;
    PyCFunction _py_long_bit_length;
    PyCFunction _py_float_as_integer_ratio;
} decimal_state;

static inline decimal_state *
get_module_state(PyObject *mod)
{
    decimal_state *state = _PyModule_GetState(mod);
    assert(state != NULL);
    return state;
}

static struct PyModuleDef _decimal_module;
static PyType_Spec dec_spec;
static PyType_Spec context_spec;

static inline decimal_state *
get_module_state_by_def(PyTypeObject *tp)
{
    PyObject *mod = PyType_GetModuleByDef(tp, &_decimal_module);
    assert(mod != NULL);
    return get_module_state(mod);
}

static inline decimal_state *
find_state_left_or_right(PyObject *left, PyObject *right)
{
    PyTypeObject *base;
    if (PyType_GetBaseByToken(Py_TYPE(left), &dec_spec, &base) != 1) {
        assert(!PyErr_Occurred());
        PyType_GetBaseByToken(Py_TYPE(right), &dec_spec, &base);
    }
    assert(base != NULL);
    void *state = _PyType_GetModuleState(base);
    assert(state != NULL);
    Py_DECREF(base);
    return (decimal_state *)state;
}

static inline decimal_state *
find_state_ternary(PyObject *left, PyObject *right, PyObject *modulus)
{
    PyTypeObject *base;
    if (PyType_GetBaseByToken(Py_TYPE(left), &dec_spec, &base) != 1) {
        assert(!PyErr_Occurred());
        if (PyType_GetBaseByToken(Py_TYPE(right), &dec_spec, &base) != 1) {
            assert(!PyErr_Occurred());
            PyType_GetBaseByToken(Py_TYPE(modulus), &dec_spec, &base);
        }
    }
    assert(base != NULL);
    void *state = _PyType_GetModuleState(base);
    assert(state != NULL);
    Py_DECREF(base);
    return (decimal_state *)state;
}


#if !defined(MPD_VERSION_HEX) || MPD_VERSION_HEX < 0x02050000
  #error "libmpdec version >= 2.5.0 required"
#endif


/*
 * Type sizes with assertions in mpdecimal.h and pyport.h:
 *    sizeof(size_t) == sizeof(Py_ssize_t)
 *    sizeof(size_t) == sizeof(mpd_uint_t) == sizeof(mpd_ssize_t)
 */

#ifdef TEST_COVERAGE
  #undef Py_LOCAL_INLINE
  #define Py_LOCAL_INLINE Py_LOCAL
#endif

#define MPD_Float_operation MPD_Not_implemented

#define BOUNDS_CHECK(x, MIN, MAX) x = (x < MIN || MAX < x) ? MAX : x

/* _Py_DEC_MINALLOC >= MPD_MINALLOC */
#define _Py_DEC_MINALLOC 4

typedef struct {
    PyObject_HEAD
    Py_hash_t hash;
    mpd_t dec;
    mpd_uint_t data[_Py_DEC_MINALLOC];
} PyDecObject;

#define _PyDecObject_CAST(op)   ((PyDecObject *)(op))

typedef struct {
    PyObject_HEAD
    uint32_t *flags;
} PyDecSignalDictObject;

#define _PyDecSignalDictObject_CAST(op) ((PyDecSignalDictObject *)(op))

typedef struct PyDecContextObject {
    PyObject_HEAD
    mpd_context_t ctx;
    PyObject *traps;
    PyObject *flags;
    int capitals;
    PyThreadState *tstate;
    decimal_state *modstate;
} PyDecContextObject;

#define _PyDecContextObject_CAST(op)    ((PyDecContextObject *)(op))

typedef struct {
    PyObject_HEAD
    PyObject *local;
    PyObject *global;
} PyDecContextManagerObject;

#define _PyDecContextManagerObject_CAST(op) ((PyDecContextManagerObject *)(op))

#undef MPD
#undef CTX
#define PyDec_CheckExact(st, v) Py_IS_TYPE(v, (st)->PyDec_Type)
#define PyDec_Check(st, v) PyObject_TypeCheck(v, (st)->PyDec_Type)
#define PyDecSignalDict_Check(st, v) Py_IS_TYPE(v, (st)->PyDecSignalDict_Type)
#define PyDecContext_Check(st, v) PyObject_TypeCheck(v, (st)->PyDecContext_Type)
#define MPD(v) (&_PyDecObject_CAST(v)->dec)
#define SdFlagAddr(v) (_PyDecSignalDictObject_CAST(v)->flags)
#define SdFlags(v) (*_PyDecSignalDictObject_CAST(v)->flags)
#define CTX(v) (&_PyDecContextObject_CAST(v)->ctx)
#define CtxCaps(v) (_PyDecContextObject_CAST(v)->capitals)

static inline decimal_state *
get_module_state_from_ctx(PyObject *v)
{
    assert(PyType_GetBaseByToken(Py_TYPE(v), &context_spec, NULL) == 1);
    decimal_state *state = ((PyDecContextObject *)v)->modstate;
    assert(state != NULL);
    return state;
}


Py_LOCAL_INLINE(PyObject *)
incr_true(void)
{
    return Py_NewRef(Py_True);
}

Py_LOCAL_INLINE(PyObject *)
incr_false(void)
{
    return Py_NewRef(Py_False);
}

/* Error codes for functions that return signals or conditions */
#define DEC_INVALID_SIGNALS (MPD_Max_status+1U)
#define DEC_ERR_OCCURRED (DEC_INVALID_SIGNALS<<1)
#define DEC_ERRORS (DEC_INVALID_SIGNALS|DEC_ERR_OCCURRED)

typedef struct DecCondMap {
    const char *name;   /* condition or signal name */
    const char *fqname; /* fully qualified name */
    uint32_t flag;      /* libmpdec flag */
    PyObject *ex;       /* corresponding exception */
} DecCondMap;

/* Exceptions that correspond to IEEE signals */
#define SUBNORMAL 5
#define INEXACT 6
#define ROUNDED 7
#define SIGNAL_MAP_LEN 9
static DecCondMap signal_map_template[] = {
  {"InvalidOperation", "decimal.InvalidOperation", MPD_IEEE_Invalid_operation, NULL},
  {"FloatOperation", "decimal.FloatOperation", MPD_Float_operation, NULL},
  {"DivisionByZero", "decimal.DivisionByZero", MPD_Division_by_zero, NULL},
  {"Overflow", "decimal.Overflow", MPD_Overflow, NULL},
  {"Underflow", "decimal.Underflow", MPD_Underflow, NULL},
  {"Subnormal", "decimal.Subnormal", MPD_Subnormal, NULL},
  {"Inexact", "decimal.Inexact", MPD_Inexact, NULL},
  {"Rounded", "decimal.Rounded", MPD_Rounded, NULL},
  {"Clamped", "decimal.Clamped", MPD_Clamped, NULL},
  {NULL}
};

/* Exceptions that inherit from InvalidOperation */
static DecCondMap cond_map_template[] = {
  {"InvalidOperation", "decimal.InvalidOperation", MPD_Invalid_operation, NULL},
  {"ConversionSyntax", "decimal.ConversionSyntax", MPD_Conversion_syntax, NULL},
  {"DivisionImpossible", "decimal.DivisionImpossible", MPD_Division_impossible, NULL},
  {"DivisionUndefined", "decimal.DivisionUndefined", MPD_Division_undefined, NULL},
  {"InvalidContext", "decimal.InvalidContext", MPD_Invalid_context, NULL},
#ifdef EXTRA_FUNCTIONALITY
  {"MallocError", "decimal.MallocError", MPD_Malloc_error, NULL},
#endif
  {NULL}
};

/* Return a duplicate of DecCondMap template */
static inline DecCondMap *
dec_cond_map_init(DecCondMap *template, Py_ssize_t size)
{
    DecCondMap *cm;
    cm = PyMem_Malloc(size);
    if (cm == NULL) {
        PyErr_NoMemory();
        return NULL;
    }

    memcpy(cm, template, size);
    return cm;
}

static const char *dec_signal_string[MPD_NUM_FLAGS] = {
    "Clamped",
    "InvalidOperation",
    "DivisionByZero",
    "InvalidOperation",
    "InvalidOperation",
    "InvalidOperation",
    "Inexact",
    "InvalidOperation",
    "InvalidOperation",
    "InvalidOperation",
    "FloatOperation",
    "Overflow",
    "Rounded",
    "Subnormal",
    "Underflow",
};

static const char *invalid_rounding_err =
"valid values for rounding are:\n\
  [ROUND_CEILING, ROUND_FLOOR, ROUND_UP, ROUND_DOWN,\n\
   ROUND_HALF_UP, ROUND_HALF_DOWN, ROUND_HALF_EVEN,\n\
   ROUND_05UP]";

static const char *invalid_signals_err =
"valid values for signals are:\n\
  [InvalidOperation, FloatOperation, DivisionByZero,\n\
   Overflow, Underflow, Subnormal, Inexact, Rounded,\n\
   Clamped]";

#ifdef EXTRA_FUNCTIONALITY
static const char *invalid_flags_err =
"valid values for _flags or _traps are:\n\
  signals:\n\
    [DecIEEEInvalidOperation, DecFloatOperation, DecDivisionByZero,\n\
     DecOverflow, DecUnderflow, DecSubnormal, DecInexact, DecRounded,\n\
     DecClamped]\n\
  conditions which trigger DecIEEEInvalidOperation:\n\
    [DecInvalidOperation, DecConversionSyntax, DecDivisionImpossible,\n\
     DecDivisionUndefined, DecFpuError, DecInvalidContext, DecMallocError]";
#endif

static int
value_error_int(const char *mesg)
{
    PyErr_SetString(PyExc_ValueError, mesg);
    return -1;
}

static PyObject *
value_error_ptr(const char *mesg)
{
    PyErr_SetString(PyExc_ValueError, mesg);
    return NULL;
}

static int
type_error_int(const char *mesg)
{
    PyErr_SetString(PyExc_TypeError, mesg);
    return -1;
}

static int
runtime_error_int(const char *mesg)
{
    PyErr_SetString(PyExc_RuntimeError, mesg);
    return -1;
}
#define INTERNAL_ERROR_INT(funcname) \
    return runtime_error_int("internal error in " funcname)

static PyObject *
runtime_error_ptr(const char *mesg)
{
    PyErr_SetString(PyExc_RuntimeError, mesg);
    return NULL;
}
#define INTERNAL_ERROR_PTR(funcname) \
    return runtime_error_ptr("internal error in " funcname)

static void
dec_traphandler(mpd_context_t *Py_UNUSED(ctx)) /* GCOV_NOT_REACHED */
{ /* GCOV_NOT_REACHED */
    return; /* GCOV_NOT_REACHED */
}

static PyObject *
flags_as_exception(decimal_state *state, uint32_t flags)
{
    DecCondMap *cm;

    for (cm = state->signal_map; cm->name != NULL; cm++) {
        if (flags&cm->flag) {
            return cm->ex;
        }
    }

    INTERNAL_ERROR_PTR("flags_as_exception"); /* GCOV_NOT_REACHED */
}

Py_LOCAL_INLINE(uint32_t)
exception_as_flag(decimal_state *state, PyObject *ex)
{
    DecCondMap *cm;

    for (cm = state->signal_map; cm->name != NULL; cm++) {
        if (cm->ex == ex) {
            return cm->flag;
        }
    }

    PyErr_SetString(PyExc_KeyError, invalid_signals_err);
    return DEC_INVALID_SIGNALS;
}

static PyObject *
flags_as_list(decimal_state *state, uint32_t flags)
{
    PyObject *list;
    DecCondMap *cm;

    list = PyList_New(0);
    if (list == NULL) {
        return NULL;
    }

    for (cm = state->cond_map; cm->name != NULL; cm++) {
        if (flags&cm->flag) {
            if (PyList_Append(list, cm->ex) < 0) {
                goto error;
            }
        }
    }
    for (cm = state->signal_map+1; cm->name != NULL; cm++) {
        if (flags&cm->flag) {
            if (PyList_Append(list, cm->ex) < 0) {
                goto error;
            }
        }
    }

    return list;

error:
    Py_DECREF(list);
    return NULL;
}

static PyObject *
signals_as_list(decimal_state *state, uint32_t flags)
{
    PyObject *list;
    DecCondMap *cm;

    list = PyList_New(0);
    if (list == NULL) {
        return NULL;
    }

    for (cm = state->signal_map; cm->name != NULL; cm++) {
        if (flags&cm->flag) {
            if (PyList_Append(list, cm->ex) < 0) {
                Py_DECREF(list);
                return NULL;
            }
        }
    }

    return list;
}

static uint32_t
list_as_flags(decimal_state *state, PyObject *list)
{
    PyObject *item;
    uint32_t flags, x;
    Py_ssize_t n, j;

    assert(PyList_Check(list));

    n = PyList_Size(list);
    flags = 0;
    for (j = 0; j < n; j++) {
        item = PyList_GetItem(list, j);
        x = exception_as_flag(state, item);
        if (x & DEC_ERRORS) {
            return x;
        }
        flags |= x;
    }

    return flags;
}

static PyObject *
flags_as_dict(decimal_state *state, uint32_t flags)
{
    DecCondMap *cm;
    PyObject *dict;

    dict = PyDict_New();
    if (dict == NULL) {
        return NULL;
    }

    for (cm = state->signal_map; cm->name != NULL; cm++) {
        PyObject *b = flags&cm->flag ? Py_True : Py_False;
        if (PyDict_SetItem(dict, cm->ex, b) < 0) {
            Py_DECREF(dict);
            return NULL;
        }
    }

    return dict;
}

static uint32_t
dict_as_flags(decimal_state *state, PyObject *val)
{
    PyObject *b;
    DecCondMap *cm;
    uint32_t flags = 0;
    int x;

    if (!PyDict_Check(val)) {
        PyErr_SetString(PyExc_TypeError,
            "argument must be a signal dict");
        return DEC_INVALID_SIGNALS;
    }

    if (PyDict_Size(val) != SIGNAL_MAP_LEN) {
        PyErr_SetString(PyExc_KeyError,
            "invalid signal dict");
        return DEC_INVALID_SIGNALS;
    }

    for (cm = state->signal_map; cm->name != NULL; cm++) {
        b = PyDict_GetItemWithError(val, cm->ex);
        if (b == NULL) {
            if (PyErr_Occurred()) {
                return DEC_ERR_OCCURRED;
            }
            PyErr_SetString(PyExc_KeyError,
                "invalid signal dict");
            return DEC_INVALID_SIGNALS;
        }

        x = PyObject_IsTrue(b);
        if (x < 0) {
            return DEC_ERR_OCCURRED;
        }
        if (x == 1) {
            flags |= cm->flag;
        }
    }

    return flags;
}

#ifdef EXTRA_FUNCTIONALITY
static uint32_t
long_as_flags(PyObject *v)
{
    long x;

    x = PyLong_AsLong(v);
    if (x == -1 && PyErr_Occurred()) {
        return DEC_ERR_OCCURRED;
    }
    if (x < 0 || x > (long)MPD_Max_status) {
        PyErr_SetString(PyExc_TypeError, invalid_flags_err);
        return DEC_INVALID_SIGNALS;
    }

    return x;
}
#endif

static int
dec_addstatus(PyObject *context, uint32_t status)
{
    mpd_context_t *ctx = CTX(context);
    decimal_state *state = get_module_state_from_ctx(context);

    ctx->status |= status;
    if (status & (ctx->traps|MPD_Malloc_error)) {
        PyObject *ex, *siglist;

        if (status & MPD_Malloc_error) {
            PyErr_NoMemory();
            return 1;
        }

        ex = flags_as_exception(state, ctx->traps&status);
        if (ex == NULL) {
            return 1; /* GCOV_NOT_REACHED */
        }
        siglist = flags_as_list(state, ctx->traps&status);
        if (siglist == NULL) {
            return 1;
        }

        PyErr_SetObject(ex, siglist);
        Py_DECREF(siglist);
        return 1;
    }
    return 0;
}

static int
getround(decimal_state *state, PyObject *v)
{
    int i;
    if (PyUnicode_Check(v)) {
        for (i = 0; i < _PY_DEC_ROUND_GUARD; i++) {
            if (v == state->round_map[i]) {
                return i;
            }
        }
        for (i = 0; i < _PY_DEC_ROUND_GUARD; i++) {
            if (PyUnicode_Compare(v, state->round_map[i]) == 0) {
                return i;
            }
        }
    }

    return type_error_int(invalid_rounding_err);
}


/******************************************************************************/
/*                            SignalDict Object                               */
/******************************************************************************/

/* The SignalDict is a MutableMapping that provides access to the
   mpd_context_t flags, which reside in the context object. When a
   new context is created, context.traps and context.flags are
   initialized to new SignalDicts. Once a SignalDict is tied to
   a context, it cannot be deleted. */

static const char *INVALID_SIGNALDICT_ERROR_MSG = "invalid signal dict";

static int
signaldict_init(PyObject *self,
                PyObject *Py_UNUSED(args), PyObject *Py_UNUSED(kwds))
{
    SdFlagAddr(self) = NULL;
    return 0;
}

static Py_ssize_t
signaldict_len(PyObject *self)
{
    if (SdFlagAddr(self) == NULL) {
        return value_error_int(INVALID_SIGNALDICT_ERROR_MSG);
    }
    return SIGNAL_MAP_LEN;
}

static PyObject *
signaldict_iter(PyObject *self)
{
    if (SdFlagAddr(self) == NULL) {
        return value_error_ptr(INVALID_SIGNALDICT_ERROR_MSG);
    }
    decimal_state *state = get_module_state_by_def(Py_TYPE(self));
    return PyTuple_Type.tp_iter(state->SignalTuple);
}

static PyObject *
signaldict_getitem(PyObject *self, PyObject *key)
{
    uint32_t flag;
    if (SdFlagAddr(self) == NULL) {
        return value_error_ptr(INVALID_SIGNALDICT_ERROR_MSG);
    }
    decimal_state *state = get_module_state_by_def(Py_TYPE(self));

    flag = exception_as_flag(state, key);
    if (flag & DEC_ERRORS) {
        return NULL;
    }

    return SdFlags(self)&flag ? incr_true() : incr_false();
}

static int
signaldict_setitem(PyObject *self, PyObject *key, PyObject *value)
{
    uint32_t flag;
    int x;

    if (SdFlagAddr(self) == NULL) {
        return value_error_int(INVALID_SIGNALDICT_ERROR_MSG);
    }

    if (value == NULL) {
        return value_error_int("signal keys cannot be deleted");
    }

    decimal_state *state = get_module_state_by_def(Py_TYPE(self));
    flag = exception_as_flag(state, key);
    if (flag & DEC_ERRORS) {
        return -1;
    }

    x = PyObject_IsTrue(value);
    if (x < 0) {
        return -1;
    }

    if (x == 1) {
        SdFlags(self) |= flag;
    }
    else {
        SdFlags(self) &= ~flag;
    }

    return 0;
}

static void
signaldict_dealloc(PyObject *self)
{
    PyTypeObject *tp = Py_TYPE(self);
    PyObject_GC_UnTrack(self);
    tp->tp_free(self);
    Py_DECREF(tp);
}

static PyObject *
signaldict_repr(PyObject *self)
{
    DecCondMap *cm;
    const char *n[SIGNAL_MAP_LEN]; /* name */
    const char *b[SIGNAL_MAP_LEN]; /* bool */
    int i;

    if (SdFlagAddr(self) == NULL) {
        return value_error_ptr(INVALID_SIGNALDICT_ERROR_MSG);
    }

    assert(SIGNAL_MAP_LEN == 9);

    decimal_state *state = get_module_state_by_def(Py_TYPE(self));
    for (cm=state->signal_map, i=0; cm->name != NULL; cm++, i++) {
        n[i] = cm->fqname;
        b[i] = SdFlags(self)&cm->flag ? "True" : "False";
    }
    return PyUnicode_FromFormat(
        "{<class '%s'>:%s, <class '%s'>:%s, <class '%s'>:%s, "
         "<class '%s'>:%s, <class '%s'>:%s, <class '%s'>:%s, "
         "<class '%s'>:%s, <class '%s'>:%s, <class '%s'>:%s}",
            n[0], b[0], n[1], b[1], n[2], b[2],
            n[3], b[3], n[4], b[4], n[5], b[5],
            n[6], b[6], n[7], b[7], n[8], b[8]);
}

static PyObject *
signaldict_richcompare(PyObject *v, PyObject *w, int op)
{
    PyObject *res = Py_NotImplemented;

    decimal_state *state = get_module_state_by_def(Py_TYPE(v));
    assert(PyDecSignalDict_Check(state, v));

    if ((SdFlagAddr(v) == NULL) || (SdFlagAddr(w) == NULL)) {
        return value_error_ptr(INVALID_SIGNALDICT_ERROR_MSG);
    }

    if (op == Py_EQ || op == Py_NE) {
        if (PyDecSignalDict_Check(state, w)) {
            res = (SdFlags(v)==SdFlags(w)) ^ (op==Py_NE) ? Py_True : Py_False;
        }
        else if (PyDict_Check(w)) {
            uint32_t flags = dict_as_flags(state, w);
            if (flags & DEC_ERRORS) {
                if (flags & DEC_INVALID_SIGNALS) {
                    /* non-comparable: Py_NotImplemented */
                    PyErr_Clear();
                }
                else {
                    return NULL;
                }
            }
            else {
                res = (SdFlags(v)==flags) ^ (op==Py_NE) ? Py_True : Py_False;
            }
        }
    }

    return Py_NewRef(res);
}

static PyObject *
signaldict_copy(PyObject *self, PyObject *Py_UNUSED(dummy))
{
    if (SdFlagAddr(self) == NULL) {
        return value_error_ptr(INVALID_SIGNALDICT_ERROR_MSG);
    }
    decimal_state *state = get_module_state_by_def(Py_TYPE(self));
    return flags_as_dict(state, SdFlags(self));
}


static PyMethodDef signaldict_methods[] = {
    { "copy", signaldict_copy, METH_NOARGS, NULL},
    {NULL, NULL}
};


static PyType_Slot signaldict_slots[] = {
    {Py_tp_dealloc, signaldict_dealloc},
    {Py_tp_traverse, _PyObject_VisitType},
    {Py_tp_repr, signaldict_repr},
    {Py_tp_hash, PyObject_HashNotImplemented},
    {Py_tp_getattro, PyObject_GenericGetAttr},
    {Py_tp_richcompare, signaldict_richcompare},
    {Py_tp_iter, signaldict_iter},
    {Py_tp_methods, signaldict_methods},
    {Py_tp_init, signaldict_init},

    // Mapping protocol
    {Py_mp_length, signaldict_len},
    {Py_mp_subscript, signaldict_getitem},
    {Py_mp_ass_subscript, signaldict_setitem},
    {0, NULL},
};

static PyType_Spec signaldict_spec = {
    .name = "decimal.SignalDictMixin",
    .basicsize = sizeof(PyDecSignalDictObject),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE |
              Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_IMMUTABLETYPE),
    .slots = signaldict_slots,
};


/******************************************************************************/
/*                         Context Object, Part 1                             */
/******************************************************************************/

#define Dec_CONTEXT_GET_SSIZE(mem)                          \
static PyObject *                                           \
context_get##mem(PyObject *self, void *Py_UNUSED(closure))  \
{                                                           \
    return PyLong_FromSsize_t(mpd_get##mem(CTX(self)));     \
}

#define Dec_CONTEXT_GET_ULONG(mem)                              \
static PyObject *                                               \
context_get##mem(PyObject *self, void *Py_UNUSED(closure))      \
{                                                               \
    return PyLong_FromUnsignedLong(mpd_get##mem(CTX(self)));    \
}

Dec_CONTEXT_GET_SSIZE(prec)
Dec_CONTEXT_GET_SSIZE(emax)
Dec_CONTEXT_GET_SSIZE(emin)
Dec_CONTEXT_GET_SSIZE(clamp)

#ifdef EXTRA_FUNCTIONALITY
Dec_CONTEXT_GET_ULONG(traps)
Dec_CONTEXT_GET_ULONG(status)
#endif

static PyObject *
context_getround(PyObject *self, void *Py_UNUSED(closure))
{
    int i = mpd_getround(CTX(self));
    decimal_state *state = get_module_state_from_ctx(self);

    return Py_NewRef(state->round_map[i]);
}

static PyObject *
context_getcapitals(PyObject *self, void *Py_UNUSED(closure))
{
    return PyLong_FromLong(CtxCaps(self));
}

#ifdef EXTRA_FUNCTIONALITY
static PyObject *
context_getallcr(PyObject *self, void *Py_UNUSED(closure))
{
    return PyLong_FromLong(mpd_getcr(CTX(self)));
}
#endif

/*[clinic input]
_decimal.Context.Etiny

Return a value equal to Emin - prec + 1.

This is the minimum exponent value for subnormal results.  When
underflow occurs, the exponent is set to Etiny.
[clinic start generated code]*/

static PyObject *
_decimal_Context_Etiny_impl(PyObject *self)
/*[clinic end generated code: output=c9a4a1a3e3575289 input=1274040f303f2244]*/
{
    return PyLong_FromSsize_t(mpd_etiny(CTX(self)));
}

/*[clinic input]
_decimal.Context.Etop

Return a value equal to Emax - prec + 1.

This is the maximum exponent if the _clamp field of the context is set
to 1 (IEEE clamp mode).  Etop() must not be negative.
[clinic start generated code]*/

static PyObject *
_decimal_Context_Etop_impl(PyObject *self)
/*[clinic end generated code: output=f0a3f6e1b829074e input=838a4409316ec728]*/
{
    return PyLong_FromSsize_t(mpd_etop(CTX(self)));
}

static int
context_setprec(PyObject *self, PyObject *value, void *Py_UNUSED(closure))
{
    mpd_context_t *ctx;
    mpd_ssize_t x;

    x = PyLong_AsSsize_t(value);
    if (x == -1 && PyErr_Occurred()) {
        return -1;
    }

    ctx = CTX(self);
    if (!mpd_qsetprec(ctx, x)) {
        return value_error_int(
            "valid range for prec is [1, MAX_PREC]");
    }

    return 0;
}

static int
context_setemin(PyObject *self, PyObject *value, void *Py_UNUSED(closure))
{
    mpd_context_t *ctx;
    mpd_ssize_t x;

    x = PyLong_AsSsize_t(value);
    if (x == -1 && PyErr_Occurred()) {
        return -1;
    }

    ctx = CTX(self);
    if (!mpd_qsetemin(ctx, x)) {
        return value_error_int(
            "valid range for Emin is [MIN_EMIN, 0]");
    }

    return 0;
}

static int
context_setemax(PyObject *self, PyObject *value, void *Py_UNUSED(closure))
{
    mpd_context_t *ctx;
    mpd_ssize_t x;

    x = PyLong_AsSsize_t(value);
    if (x == -1 && PyErr_Occurred()) {
        return -1;
    }

    ctx = CTX(self);
    if (!mpd_qsetemax(ctx, x)) {
        return value_error_int(
            "valid range for Emax is [0, MAX_EMAX]");
    }

    return 0;
}

#ifdef CONFIG_32
/*[clinic input]
_decimal.Context._unsafe_setprec

    x: Py_ssize_t
    /

[clinic start generated code]*/

static PyObject *
_decimal_Context__unsafe_setprec_impl(PyObject *self, Py_ssize_t x)
/*[clinic end generated code: output=dd838edf08e12dd9 input=23a1b19ceb1569be]*/
{
    mpd_context_t *ctx = CTX(self);

    if (x < 1 || x > 1070000000L) {
        return value_error_ptr(
            "valid range for unsafe prec is [1, 1070000000]");
    }

    ctx->prec = x;
    Py_RETURN_NONE;
}

/*[clinic input]
_decimal.Context._unsafe_setemin

    x: Py_ssize_t
    /

[clinic start generated code]*/

static PyObject *
_decimal_Context__unsafe_setemin_impl(PyObject *self, Py_ssize_t x)
/*[clinic end generated code: output=0c49cafee8a65846 input=652f1ecacca7e0ce]*/
{
    mpd_context_t *ctx = CTX(self);

    if (x < -1070000000L || x > 0) {
        return value_error_ptr(
            "valid range for unsafe emin is [-1070000000, 0]");
    }

    ctx->emin = x;
    Py_RETURN_NONE;
}

/*[clinic input]
_decimal.Context._unsafe_setemax

    x: Py_ssize_t
    /

[clinic start generated code]*/

static PyObject *
_decimal_Context__unsafe_setemax_impl(PyObject *self, Py_ssize_t x)
/*[clinic end generated code: output=776563e0377a00e8 input=b2a32a9a2750e7a8]*/
{
    mpd_context_t *ctx = CTX(self);

    if (x < 0 || x > 1070000000L) {
        return value_error_ptr(
            "valid range for unsafe emax is [0, 1070000000]");
    }

    ctx->emax = x;
    Py_RETURN_NONE;
}
#endif

static int
context_setround(PyObject *self, PyObject *value, void *Py_UNUSED(closure))
{
    mpd_context_t *ctx;
    int x;

    decimal_state *state = get_module_state_from_ctx(self);
    x = getround(state, value);
    if (x == -1) {
        return -1;
    }

    ctx = CTX(self);
    if (!mpd_qsetround(ctx, x)) {
        INTERNAL_ERROR_INT("context_setround"); /* GCOV_NOT_REACHED */
    }

    return 0;
}

static int
context_setcapitals(PyObject *self, PyObject *value, void *Py_UNUSED(closure))
{
    mpd_ssize_t x;

    x = PyLong_AsSsize_t(value);
    if (x == -1 && PyErr_Occurred()) {
        return -1;
    }

    if (x != 0 && x != 1) {
        return value_error_int(
            "valid values for capitals are 0 or 1");
    }
    CtxCaps(self) = (int)x;

    return 0;
}

#ifdef EXTRA_FUNCTIONALITY
static int
context_settraps(PyObject *self, PyObject *value, void *Py_UNUSED(closure))
{
    mpd_context_t *ctx;
    uint32_t flags;

    flags = long_as_flags(value);
    if (flags & DEC_ERRORS) {
        return -1;
    }

    ctx = CTX(self);
    if (!mpd_qsettraps(ctx, flags)) {
        INTERNAL_ERROR_INT("context_settraps");
    }

    return 0;
}
#endif

static int
context_settraps_list(PyObject *self, PyObject *value)
{
    mpd_context_t *ctx;
    uint32_t flags;
    decimal_state *state = get_module_state_from_ctx(self);
    flags = list_as_flags(state, value);
    if (flags & DEC_ERRORS) {
        return -1;
    }

    ctx = CTX(self);
    if (!mpd_qsettraps(ctx, flags)) {
        INTERNAL_ERROR_INT("context_settraps_list");
    }

    return 0;
}

static int
context_settraps_dict(PyObject *self, PyObject *value)
{
    mpd_context_t *ctx;
    uint32_t flags;

    decimal_state *state = get_module_state_from_ctx(self);
    if (PyDecSignalDict_Check(state, value)) {
        flags = SdFlags(value);
    }
    else {
        flags = dict_as_flags(state, value);
        if (flags & DEC_ERRORS) {
            return -1;
        }
    }

    ctx = CTX(self);
    if (!mpd_qsettraps(ctx, flags)) {
        INTERNAL_ERROR_INT("context_settraps_dict");
    }

    return 0;
}

#ifdef EXTRA_FUNCTIONALITY
static int
context_setstatus(PyObject *self, PyObject *value, void *Py_UNUSED(closure))
{
    mpd_context_t *ctx;
    uint32_t flags;

    flags = long_as_flags(value);
    if (flags & DEC_ERRORS) {
        return -1;
    }

    ctx = CTX(self);
    if (!mpd_qsetstatus(ctx, flags)) {
        INTERNAL_ERROR_INT("context_setstatus");
    }

    return 0;
}
#endif

static int
context_setstatus_list(PyObject *self, PyObject *value)
{
    mpd_context_t *ctx;
    uint32_t flags;
    decimal_state *state = get_module_state_from_ctx(self);

    flags = list_as_flags(state, value);
    if (flags & DEC_ERRORS) {
        return -1;
    }

    ctx = CTX(self);
    if (!mpd_qsetstatus(ctx, flags)) {
        INTERNAL_ERROR_INT("context_setstatus_list");
    }

    return 0;
}

static int
context_setstatus_dict(PyObject *self, PyObject *value)
{
    mpd_context_t *ctx;
    uint32_t flags;

    decimal_state *state = get_module_state_from_ctx(self);
    if (PyDecSignalDict_Check(state, value)) {
        flags = SdFlags(value);
    }
    else {
        flags = dict_as_flags(state, value);
        if (flags & DEC_ERRORS) {
            return -1;
        }
    }

    ctx = CTX(self);
    if (!mpd_qsetstatus(ctx, flags)) {
        INTERNAL_ERROR_INT("context_setstatus_dict");
    }

    return 0;
}

static int
context_setclamp(PyObject *self, PyObject *value, void *Py_UNUSED(closure))
{
    mpd_context_t *ctx;
    mpd_ssize_t x;

    x = PyLong_AsSsize_t(value);
    if (x == -1 && PyErr_Occurred()) {
        return -1;
    }
    BOUNDS_CHECK(x, INT_MIN, INT_MAX);

    ctx = CTX(self);
    if (!mpd_qsetclamp(ctx, (int)x)) {
        return value_error_int("valid values for clamp are 0 or 1");
    }

    return 0;
}

#ifdef EXTRA_FUNCTIONALITY
static int
context_setallcr(PyObject *self, PyObject *value, void *Py_UNUSED(closure))
{
    mpd_context_t *ctx;
    mpd_ssize_t x;

    x = PyLong_AsSsize_t(value);
    if (x == -1 && PyErr_Occurred()) {
        return -1;
    }
    BOUNDS_CHECK(x, INT_MIN, INT_MAX);

    ctx = CTX(self);
    if (!mpd_qsetcr(ctx, (int)x)) {
        return value_error_int("valid values for _allcr are 0 or 1");
    }

    return 0;
}
#endif

static PyObject *
context_getattr(PyObject *self, PyObject *name)
{
    PyObject *retval;

    if (PyUnicode_Check(name)) {
        if (PyUnicode_CompareWithASCIIString(name, "traps") == 0) {
            retval = ((PyDecContextObject *)self)->traps;
            return Py_NewRef(retval);
        }
        if (PyUnicode_CompareWithASCIIString(name, "flags") == 0) {
            retval = ((PyDecContextObject *)self)->flags;
            return Py_NewRef(retval);
        }
    }

    return PyObject_GenericGetAttr(self, name);
}

static int
context_setattr(PyObject *self, PyObject *name, PyObject *value)
{
    if (value == NULL) {
        PyErr_SetString(PyExc_AttributeError,
            "context attributes cannot be deleted");
        return -1;
    }

    if (PyUnicode_Check(name)) {
        if (PyUnicode_CompareWithASCIIString(name, "traps") == 0) {
            return context_settraps_dict(self, value);
        }
        if (PyUnicode_CompareWithASCIIString(name, "flags") == 0) {
            return context_setstatus_dict(self, value);
        }
    }

    return PyObject_GenericSetAttr(self, name, value);
}

static int
context_setattrs(PyObject *self, PyObject *prec, PyObject *rounding,
                 PyObject *emin, PyObject *emax, PyObject *capitals,
                 PyObject *clamp, PyObject *status, PyObject *traps) {

    int ret;
    if (prec != Py_None && context_setprec(self, prec, NULL) < 0) {
        return -1;
    }
    if (rounding != Py_None && context_setround(self, rounding, NULL) < 0) {
        return -1;
    }
    if (emin != Py_None && context_setemin(self, emin, NULL) < 0) {
        return -1;
    }
    if (emax != Py_None && context_setemax(self, emax, NULL) < 0) {
        return -1;
    }
    if (capitals != Py_None && context_setcapitals(self, capitals, NULL) < 0) {
        return -1;
    }
    if (clamp != Py_None && context_setclamp(self, clamp, NULL) < 0) {
       return -1;
    }

    if (traps != Py_None) {
        if (PyList_Check(traps)) {
            ret = context_settraps_list(self, traps);
        }
#ifdef EXTRA_FUNCTIONALITY
        else if (PyLong_Check(traps)) {
            ret = context_settraps(self, traps, NULL);
        }
#endif
        else {
            ret = context_settraps_dict(self, traps);
        }
        if (ret < 0) {
            return ret;
        }
    }
    if (status != Py_None) {
        if (PyList_Check(status)) {
            ret = context_setstatus_list(self, status);
        }
#ifdef EXTRA_FUNCTIONALITY
        else if (PyLong_Check(status)) {
            ret = context_setstatus(self, status, NULL);
        }
#endif
        else {
            ret = context_setstatus_dict(self, status);
        }
        if (ret < 0) {
            return ret;
        }
    }

    return 0;
}

/*[clinic input]
_decimal.Context.clear_traps

Set all traps to False.
[clinic start generated code]*/

static PyObject *
_decimal_Context_clear_traps_impl(PyObject *self)
/*[clinic end generated code: output=b47cfa6e32407d40 input=3872e80637148035]*/
{
    CTX(self)->traps = 0;
    Py_RETURN_NONE;
}

/*[clinic input]
_decimal.Context.clear_flags

Reset all flags to False.
[clinic start generated code]*/

static PyObject *
_decimal_Context_clear_flags_impl(PyObject *self)
/*[clinic end generated code: output=c86719a70177d0b6 input=a06055e2f3e7edb1]*/
{
    CTX(self)->status = 0;
    Py_RETURN_NONE;
}

#define DEC_DFLT_EMAX 999999
#define DEC_DFLT_EMIN -999999

static mpd_context_t dflt_ctx = {
  28, DEC_DFLT_EMAX, DEC_DFLT_EMIN,
  MPD_IEEE_Invalid_operation|MPD_Division_by_zero|MPD_Overflow,
  0, 0, MPD_ROUND_HALF_EVEN, 0, 1
};

static PyObject *
context_new(PyTypeObject *type,
            PyObject *Py_UNUSED(args), PyObject *Py_UNUSED(kwds))
{
    PyDecContextObject *self = NULL;
    mpd_context_t *ctx;

    decimal_state *state = get_module_state_by_def(type);
    if (type == state->PyDecContext_Type) {
        self = PyObject_GC_New(PyDecContextObject, state->PyDecContext_Type);
    }
    else {
        self = (PyDecContextObject *)type->tp_alloc(type, 0);
    }

    if (self == NULL) {
        return NULL;
    }

    self->traps = PyObject_CallObject((PyObject *)state->PyDecSignalDict_Type, NULL);
    if (self->traps == NULL) {
        self->flags = NULL;
        Py_DECREF(self);
        return NULL;
    }
    self->flags = PyObject_CallObject((PyObject *)state->PyDecSignalDict_Type, NULL);
    if (self->flags == NULL) {
        Py_DECREF(self);
        return NULL;
    }

    ctx = CTX(self);

    if (state->default_context_template) {
        *ctx = *CTX(state->default_context_template);
    }
    else {
        *ctx = dflt_ctx;
    }

    SdFlagAddr(self->traps) = &ctx->traps;
    SdFlagAddr(self->flags) = &ctx->status;

    CtxCaps(self) = 1;
    self->tstate = NULL;
    self->modstate = state;

    if (type == state->PyDecContext_Type) {
        PyObject_GC_Track(self);
    }
    assert(PyObject_GC_IsTracked((PyObject *)self));
    return (PyObject *)self;
}

static int
context_traverse(PyObject *op, visitproc visit, void *arg)
{
    PyDecContextObject *self = _PyDecContextObject_CAST(op);
    Py_VISIT(Py_TYPE(self));
    Py_VISIT(self->traps);
    Py_VISIT(self->flags);
    return 0;
}

static int
context_clear(PyObject *op)
{
    PyDecContextObject *self = _PyDecContextObject_CAST(op);
    Py_CLEAR(self->traps);
    Py_CLEAR(self->flags);
    return 0;
}

static void
context_dealloc(PyObject *self)
{
    PyTypeObject *tp = Py_TYPE(self);
    PyObject_GC_UnTrack(self);
    (void)context_clear(self);
    tp->tp_free(self);
    Py_DECREF(tp);
}

/*[clinic input]
_decimal.Context.__init__ as context_init

    prec: object = None
    rounding: object = None
    Emin as emin: object = None
    Emax as emax: object = None
    capitals: object = None
    clamp: object = None
    flags as status: object = None
    traps: object = None

Create context.

The context affects almost all operations and controls rounding,
Over/Underflow, raising of exceptions and much more.  A new context
can be constructed as follows:

    >>> c = Context(prec=28, Emin=-425000000, Emax=425000000,
    ...             rounding=ROUND_HALF_EVEN, capitals=1, clamp=1,
    ...             traps=[InvalidOperation, DivisionByZero, Overflow],
    ...             flags=[])
    >>>
[clinic start generated code]*/

static int
context_init_impl(PyObject *self, PyObject *prec, PyObject *rounding,
                  PyObject *emin, PyObject *emax, PyObject *capitals,
                  PyObject *clamp, PyObject *status, PyObject *traps)
/*[clinic end generated code: output=8bfdc59fbe862f44 input=45c704b93cd02959]*/
{
    return context_setattrs(
        self, prec, rounding,
        emin, emax, capitals,
        clamp, status, traps
    );
}

static PyObject *
context_repr(PyObject *self)
{
    mpd_context_t *ctx;
    char flags[MPD_MAX_SIGNAL_LIST];
    char traps[MPD_MAX_SIGNAL_LIST];
    int n, mem;

#ifdef Py_DEBUG
    decimal_state *state = get_module_state_from_ctx(self);
    assert(PyDecContext_Check(state, self));
#endif
    ctx = CTX(self);

    mem = MPD_MAX_SIGNAL_LIST;
    n = mpd_lsnprint_signals(flags, mem, ctx->status, dec_signal_string);
    if (n < 0 || n >= mem) {
        INTERNAL_ERROR_PTR("context_repr");
    }

    n = mpd_lsnprint_signals(traps, mem, ctx->traps, dec_signal_string);
    if (n < 0 || n >= mem) {
        INTERNAL_ERROR_PTR("context_repr");
    }

    return PyUnicode_FromFormat(
        "Context(prec=%zd, rounding=%s, Emin=%zd, Emax=%zd, "
                "capitals=%d, clamp=%d, flags=%s, traps=%s)",
         ctx->prec, mpd_round_string[ctx->round], ctx->emin, ctx->emax,
         CtxCaps(self), ctx->clamp, flags, traps);
}

static void
init_basic_context(PyObject *v)
{
    mpd_context_t ctx = dflt_ctx;

    ctx.prec = 9;
    ctx.traps |= (MPD_Underflow|MPD_Clamped);
    ctx.round = MPD_ROUND_HALF_UP;

    *CTX(v) = ctx;
    CtxCaps(v) = 1;
}

static void
init_extended_context(PyObject *v)
{
    mpd_context_t ctx = dflt_ctx;

    ctx.prec = 9;
    ctx.traps = 0;

    *CTX(v) = ctx;
    CtxCaps(v) = 1;
}

/* Factory function for creating IEEE interchange format contexts */

/*[clinic input]
_decimal.IEEEContext

    bits: Py_ssize_t
    /

Return a context, initialized as one of the IEEE interchange formats.

The argument must be a multiple of 32 and less than
IEEE_CONTEXT_MAX_BITS.
[clinic start generated code]*/

static PyObject *
_decimal_IEEEContext_impl(PyObject *module, Py_ssize_t bits)
/*[clinic end generated code: output=19a35f320fe19789 input=5cff864d899eb2d7]*/
{
    PyObject *context;
    mpd_context_t ctx;

    if (bits <= 0 || bits > INT_MAX) {
        goto error;
    }
    if (mpd_ieee_context(&ctx, (int)bits) < 0) {
        goto error;
    }

    decimal_state *state = get_module_state(module);
    context = PyObject_CallObject((PyObject *)state->PyDecContext_Type, NULL);
    if (context == NULL) {
        return NULL;
    }
    *CTX(context) = ctx;

    return context;

error:
    PyErr_Format(PyExc_ValueError,
        "argument must be a multiple of 32, with a maximum of %d",
        MPD_IEEE_CONTEXT_MAX_BITS);

    return NULL;
}

static PyObject *
context_copy(decimal_state *state, PyObject *v)
{
    PyObject *copy =
        PyObject_CallObject((PyObject *)state->PyDecContext_Type, NULL);

    if (copy == NULL) {
        return NULL;
    }

    *CTX(copy) = *CTX(v);
    CTX(copy)->newtrap = 0;
    CtxCaps(copy) = CtxCaps(v);

    return copy;
}

/*[clinic input]
_decimal.Context.copy

    cls: defining_class

Return a duplicate of the context with all flags cleared.
[clinic start generated code]*/

static PyObject *
_decimal_Context_copy_impl(PyObject *self, PyTypeObject *cls)
/*[clinic end generated code: output=31c9c8eeb0c0cf77 input=aef1c0bddabdf8f0]*/
{
    decimal_state *state = PyType_GetModuleState(cls);

    return context_copy(state, self);
}

/*[clinic input]
_decimal.Context.__copy__ = _decimal.Context.copy

[clinic start generated code]*/

static PyObject *
_decimal_Context___copy___impl(PyObject *self, PyTypeObject *cls)
/*[clinic end generated code: output=93552486e5fb0ab4 input=4a55dd22f6d31bcc]*/
{
    decimal_state *state = PyType_GetModuleState(cls);

    return context_copy(state, self);
}

/*[clinic input]
_decimal.Context.__reduce__ = _decimal.Context.copy

[clinic start generated code]*/

static PyObject *
_decimal_Context___reduce___impl(PyObject *self, PyTypeObject *cls)
/*[clinic end generated code: output=4e77de55efdbb56a input=787683f13d047ce8]*/
{
    PyObject *flags;
    PyObject *traps;
    PyObject *ret;
    mpd_context_t *ctx;
    decimal_state *state = PyType_GetModuleState(cls);

    ctx = CTX(self);

    flags = signals_as_list(state, ctx->status);
    if (flags == NULL) {
        return NULL;
    }
    traps = signals_as_list(state, ctx->traps);
    if (traps == NULL) {
        Py_DECREF(flags);
        return NULL;
    }

    ret = Py_BuildValue(
            "O(nsnniiOO)",
            Py_TYPE(self),
            ctx->prec, mpd_round_string[ctx->round], ctx->emin, ctx->emax,
            CtxCaps(self), ctx->clamp, flags, traps
    );

    Py_DECREF(flags);
    Py_DECREF(traps);
    return ret;
}


static PyGetSetDef context_getsets [] =
{
  { "prec", context_getprec, context_setprec, NULL, NULL},
  { "Emax", context_getemax, context_setemax, NULL, NULL},
  { "Emin", context_getemin, context_setemin, NULL, NULL},
  { "rounding", context_getround, context_setround, NULL, NULL},
  { "capitals", context_getcapitals, context_setcapitals, NULL, NULL},
  { "clamp", context_getclamp, context_setclamp, NULL, NULL},
#ifdef EXTRA_FUNCTIONALITY
  { "_allcr", context_getallcr, context_setallcr, NULL, NULL},
  { "_traps", context_gettraps, context_settraps, NULL, NULL},
  { "_flags", context_getstatus, context_setstatus, NULL, NULL},
#endif
  {NULL}
};


#define CONTEXT_CHECK(state, obj) \
    if (!PyDecContext_Check(state, obj)) { \
        PyErr_SetString(PyExc_TypeError,   \
            "argument must be a context"); \
        return NULL;                       \
    }

#define CONTEXT_CHECK_VA(state, obj) \
    if (obj == Py_None) {                           \
        CURRENT_CONTEXT(state, obj);                \
    }                                               \
    else if (!PyDecContext_Check(state, obj)) {     \
        PyErr_SetString(PyExc_TypeError,            \
            "optional argument must be a context"); \
        return NULL;                                \
    }


/******************************************************************************/
/*                Global, thread local and temporary contexts                 */
/******************************************************************************/

/*
 * Thread local storage currently has a speed penalty of about 4%.
 * All functions that map Python's arithmetic operators to mpdecimal
 * functions have to look up the current context for each and every
 * operation.
 */

#ifndef WITH_DECIMAL_CONTEXTVAR
/* Get the context from the thread state dictionary. */
static PyObject *
current_context_from_dict(decimal_state *modstate)
{
    PyThreadState *tstate = _PyThreadState_GET();
#ifdef Py_DEBUG
    // The caller must hold the GIL
    _Py_EnsureTstateNotNULL(tstate);
#endif

    PyObject *dict = _PyThreadState_GetDict(tstate);
    if (dict == NULL) {
        PyErr_SetString(PyExc_RuntimeError,
            "cannot get thread state");
        return NULL;
    }

    PyObject *tl_context;
    tl_context = PyDict_GetItemWithError(dict, modstate->tls_context_key);
    if (tl_context != NULL) {
        /* We already have a thread local context. */
        CONTEXT_CHECK(modstate, tl_context);
    }
    else {
        if (PyErr_Occurred()) {
            return NULL;
        }

        /* Set up a new thread local context. */
        tl_context = context_copy(modstate, modstate->default_context_template);
        if (tl_context == NULL) {
            return NULL;
        }
        CTX(tl_context)->status = 0;

        if (PyDict_SetItem(dict, modstate->tls_context_key, tl_context) < 0) {
            Py_DECREF(tl_context);
            return NULL;
        }
        Py_DECREF(tl_context);
    }

    /* Cache the context of the current thread, assuming that it
     * will be accessed several times before a thread switch. */
    Py_XSETREF(modstate->cached_context,
               (PyDecContextObject *)Py_NewRef(tl_context));
    modstate->cached_context->tstate = tstate;

    /* Borrowed reference with refcount==1 */
    return tl_context;
}

/* Return borrowed reference to thread local context. */
static PyObject *
current_context(decimal_state *modstate)
{
    PyThreadState *tstate = _PyThreadState_GET();
    if (modstate->cached_context && modstate->cached_context->tstate == tstate) {
        return (PyObject *)(modstate->cached_context);
    }

    return current_context_from_dict(modstate);
}

/* ctxobj := borrowed reference to the current context */
#define CURRENT_CONTEXT(STATE, CTXOBJ)      \
    do {                                    \
        CTXOBJ = current_context(STATE);    \
        if (CTXOBJ == NULL) {               \
            return NULL;                    \
        }                                   \
    } while (0)

/* Return a new reference to the current context */
static PyObject *
PyDec_GetCurrentContext(PyObject *self)
{
    PyObject *context;
    decimal_state *state = get_module_state(self);

    CURRENT_CONTEXT(state, context);
    return Py_NewRef(context);
}

/* Set the thread local context to a new context, decrement old reference */
static PyObject *
PyDec_SetCurrentContext(PyObject *self, PyObject *v)
{
    PyObject *dict;

    decimal_state *state = get_module_state(self);
    CONTEXT_CHECK(state, v);

    dict = PyThreadState_GetDict();
    if (dict == NULL) {
        PyErr_SetString(PyExc_RuntimeError,
            "cannot get thread state");
        return NULL;
    }

    /* If the new context is one of the templates, make a copy.
     * This is the current behavior of decimal.py. */
    if (v == state->default_context_template ||
        v == state->basic_context_template ||
        v == state->extended_context_template) {
        v = context_copy(state, v);
        if (v == NULL) {
            return NULL;
        }
        CTX(v)->status = 0;
    }
    else {
        Py_INCREF(v);
    }

    Py_CLEAR(state->cached_context);
    if (PyDict_SetItem(dict, state->tls_context_key, v) < 0) {
        Py_DECREF(v);
        return NULL;
    }

    Py_DECREF(v);
    Py_RETURN_NONE;
}
#else
static PyObject *
init_current_context(decimal_state *state)
{
    PyObject *tl_context = context_copy(state, state->default_context_template);
    if (tl_context == NULL) {
        return NULL;
    }
    CTX(tl_context)->status = 0;

    PyObject *tok = PyContextVar_Set(state->current_context_var, tl_context);
    if (tok == NULL) {
        Py_DECREF(tl_context);
        return NULL;
    }
    Py_DECREF(tok);

    return tl_context;
}

static inline PyObject *
current_context(decimal_state *state)
{
    PyObject *tl_context;
    if (PyContextVar_Get(state->current_context_var, NULL, &tl_context) < 0) {
        return NULL;
    }

    if (tl_context != NULL) {
        return tl_context;
    }

    return init_current_context(state);
}

/* ctxobj := borrowed reference to the current context */
#define CURRENT_CONTEXT(STATE, CTXOBJ)      \
    do {                                    \
        CTXOBJ = current_context(STATE);    \
        if (CTXOBJ == NULL) {               \
            return NULL;                    \
        }                                   \
        Py_DECREF(CTXOBJ);                  \
    } while (0)

/* Return a new reference to the current context */
static PyObject *
PyDec_GetCurrentContext(PyObject *self)
{
    decimal_state *state = get_module_state(self);
    return current_context(state);
}

/* Set the thread local context to a new context, decrement old reference */
static PyObject *
PyDec_SetCurrentContext(PyObject *self, PyObject *v)
{
    decimal_state *state = get_module_state(self);
    CONTEXT_CHECK(state, v);

    /* If the new context is one of the templates, make a copy.
     * This is the current behavior of decimal.py. */
    if (v == state->default_context_template ||
        v == state->basic_context_template ||
        v == state->extended_context_template) {
        v = context_copy(state, v);
        if (v == NULL) {
            return NULL;
        }
        CTX(v)->status = 0;
    }
    else {
        Py_INCREF(v);
    }

    PyObject *tok = PyContextVar_Set(state->current_context_var, v);
    Py_DECREF(v);
    if (tok == NULL) {
        return NULL;
    }
    Py_DECREF(tok);

    Py_RETURN_NONE;
}
#endif

/*[clinic input]
_decimal.getcontext

Get the current default context.
[clinic start generated code]*/

static PyObject *
_decimal_getcontext_impl(PyObject *module)
/*[clinic end generated code: output=5982062c4d39e3dd input=7ac316fe42a1b6f5]*/
{
    return PyDec_GetCurrentContext(module);
}

/*[clinic input]
_decimal.setcontext

    context: object
    /

Set a new default context.
[clinic start generated code]*/

static PyObject *
_decimal_setcontext(PyObject *module, PyObject *context)
/*[clinic end generated code: output=8065f870be2852ce input=b57d7ee786b022a6]*/
{
    return PyDec_SetCurrentContext(module, context);
}

/* Context manager object for the 'with' statement. The manager
 * owns one reference to the global (outer) context and one
 * to the local (inner) context. */

/*[clinic input]
@text_signature "($module, /, ctx=None, **kwargs)"
_decimal.localcontext

    ctx as local: object = None
    *
    prec: object = None
    rounding: object = None
    Emin: object = None
    Emax: object = None
    capitals: object = None
    clamp: object = None
    flags: object = None
    traps: object = None

Return a context manager for a copy of the supplied context.

That will set the default context to a copy of ctx on entry to the
with-statement and restore the previous default context when exiting
the with-statement. If no context is specified, a copy of the current
default context is used.
[clinic start generated code]*/

static PyObject *
_decimal_localcontext_impl(PyObject *module, PyObject *local, PyObject *prec,
                           PyObject *rounding, PyObject *Emin,
                           PyObject *Emax, PyObject *capitals,
                           PyObject *clamp, PyObject *flags, PyObject *traps)
/*[clinic end generated code: output=9bf4e47742a809b0 input=490307b9689c3856]*/
{
    PyObject *global;

    decimal_state *state = get_module_state(module);
    CURRENT_CONTEXT(state, global);
    if (local == Py_None) {
        local = global;
    }
    else if (!PyDecContext_Check(state, local)) {
        PyErr_SetString(PyExc_TypeError,
            "optional argument must be a context");
        return NULL;
    }

    PyObject *local_copy = context_copy(state, local);
    if (local_copy == NULL) {
        return NULL;
    }

    int ret = context_setattrs(
        local_copy, prec, rounding,
        Emin, Emax, capitals,
        clamp, flags, traps
    );
    if (ret < 0) {
        Py_DECREF(local_copy);
        return NULL;
    }

    PyDecContextManagerObject *self;
    self = PyObject_GC_New(PyDecContextManagerObject,
                           state->PyDecContextManager_Type);
    if (self == NULL) {
        Py_DECREF(local_copy);
        return NULL;
    }

    self->local = local_copy;
    self->global = Py_NewRef(global);
    PyObject_GC_Track(self);

    return (PyObject *)self;
}

static int
ctxmanager_traverse(PyObject *op, visitproc visit, void *arg)
{
    PyDecContextManagerObject *self = _PyDecContextManagerObject_CAST(op);
    Py_VISIT(Py_TYPE(self));
    Py_VISIT(self->local);
    Py_VISIT(self->global);
    return 0;
}

static int
ctxmanager_clear(PyObject *op)
{
    PyDecContextManagerObject *self = _PyDecContextManagerObject_CAST(op);
    Py_CLEAR(self->local);
    Py_CLEAR(self->global);
    return 0;
}

static void
ctxmanager_dealloc(PyObject *self)
{
    PyTypeObject *tp = Py_TYPE(self);
    PyObject_GC_UnTrack(self);
    (void)ctxmanager_clear(self);
    tp->tp_free(self);
    Py_DECREF(tp);
}

static PyObject *
ctxmanager_set_local(PyObject *op, PyObject *Py_UNUSED(dummy))
{
    PyObject *ret;
    PyDecContextManagerObject *self = _PyDecContextManagerObject_CAST(op);
    ret = PyDec_SetCurrentContext(PyType_GetModule(Py_TYPE(self)), self->local);
    if (ret == NULL) {
        return NULL;
    }
    Py_DECREF(ret);

    return Py_NewRef(self->local);
}

static PyObject *
ctxmanager_restore_global(PyObject *op, PyObject *Py_UNUSED(args))
{
    PyObject *ret;
    PyDecContextManagerObject *self = _PyDecContextManagerObject_CAST(op);
    ret = PyDec_SetCurrentContext(PyType_GetModule(Py_TYPE(self)), self->global);
    if (ret == NULL) {
        return NULL;
    }
    Py_DECREF(ret);

    Py_RETURN_NONE;
}


static PyMethodDef ctxmanager_methods[] = {
  {"__enter__", ctxmanager_set_local, METH_NOARGS, NULL},
  {"__exit__", ctxmanager_restore_global, METH_VARARGS, NULL},
  {NULL, NULL}
};

static PyType_Slot ctxmanager_slots[] = {
    {Py_tp_dealloc, ctxmanager_dealloc},
    {Py_tp_getattro, PyObject_GenericGetAttr},
    {Py_tp_traverse, ctxmanager_traverse},
    {Py_tp_clear, ctxmanager_clear},
    {Py_tp_methods, ctxmanager_methods},
    {0, NULL},
};

static PyType_Spec ctxmanager_spec = {
    .name = "decimal.ContextManager",
    .basicsize = sizeof(PyDecContextManagerObject),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
              Py_TPFLAGS_IMMUTABLETYPE | Py_TPFLAGS_DISALLOW_INSTANTIATION),
    .slots = ctxmanager_slots,
};


/******************************************************************************/
/*                           New Decimal Object                               */
/******************************************************************************/

static PyObject *
PyDecType_New(decimal_state *state, PyTypeObject *type)
{
    PyDecObject *dec;

    if (type == state->PyDec_Type) {
        dec = PyObject_GC_New(PyDecObject, state->PyDec_Type);
    }
    else {
        dec = (PyDecObject *)type->tp_alloc(type, 0);
    }
    if (dec == NULL) {
        return NULL;
    }

    dec->hash = -1;

    MPD(dec)->flags = MPD_STATIC|MPD_STATIC_DATA;
    MPD(dec)->exp = 0;
    MPD(dec)->digits = 0;
    MPD(dec)->len = 0;
    MPD(dec)->alloc = _Py_DEC_MINALLOC;
    MPD(dec)->data = dec->data;

    if (type == state->PyDec_Type) {
        PyObject_GC_Track(dec);
    }
    assert(PyObject_GC_IsTracked((PyObject *)dec));
    return (PyObject *)dec;
}
#define dec_alloc(st) PyDecType_New(st, (st)->PyDec_Type)

static void
dec_dealloc(PyObject *dec)
{
    PyTypeObject *tp = Py_TYPE(dec);
    PyObject_GC_UnTrack(dec);
    mpd_del(MPD(dec));
    tp->tp_free(dec);
    Py_DECREF(tp);
}


/******************************************************************************/
/*                           Conversions to Decimal                           */
/******************************************************************************/

Py_LOCAL_INLINE(int)
is_space(int kind, const void *data, Py_ssize_t pos)
{
    Py_UCS4 ch = PyUnicode_READ(kind, data, pos);
    return Py_UNICODE_ISSPACE(ch);
}

/* Return the ASCII representation of a numeric Unicode string. The numeric
   string may contain ascii characters in the range [1, 127], any Unicode
   space and any unicode digit. If strip_ws is true, leading and trailing
   whitespace is stripped. If ignore_underscores is true, underscores are
   ignored.

   Return NULL if malloc fails and an empty string if invalid characters
   are found. */
static char *
numeric_as_ascii(PyObject *u, int strip_ws, int ignore_underscores)
{
    int kind;
    const void *data;
    Py_UCS4 ch;
    char *res, *cp;
    Py_ssize_t j, len;
    int d;

    kind = PyUnicode_KIND(u);
    data = PyUnicode_DATA(u);
    len =  PyUnicode_GET_LENGTH(u);

    cp = res = PyMem_Malloc(len+1);
    if (res == NULL) {
        PyErr_NoMemory();
        return NULL;
    }

    j = 0;
    if (strip_ws) {
        while (len > 0 && is_space(kind, data, len-1)) {
            len--;
        }
        while (j < len && is_space(kind, data, j)) {
            j++;
        }
    }

    for (; j < len; j++) {
        ch = PyUnicode_READ(kind, data, j);
        if (ignore_underscores && ch == '_') {
            continue;
        }
        if (0 < ch && ch <= 127) {
            *cp++ = ch;
            continue;
        }
        if (Py_UNICODE_ISSPACE(ch)) {
            *cp++ = ' ';
            continue;
        }
        d = Py_UNICODE_TODECIMAL(ch);
        if (d < 0) {
            /* empty string triggers ConversionSyntax */
            *res = '\0';
            return res;
        }
        *cp++ = '0' + d;
    }
    *cp = '\0';
    return res;
}

/* Return a new PyDecObject or a subtype from a C string. Use the context
   during conversion. */
static PyObject *
PyDecType_FromCString(PyTypeObject *type, const char *s,
                      PyObject *context)
{
    PyObject *dec;
    uint32_t status = 0;

    decimal_state *state = get_module_state_from_ctx(context);
    dec = PyDecType_New(state, type);
    if (dec == NULL) {
        return NULL;
    }

    mpd_qset_string(MPD(dec), s, CTX(context), &status);
    if (dec_addstatus(context, status)) {
        Py_DECREF(dec);
        return NULL;
    }
    return dec;
}

/* Return a new PyDecObject or a subtype from a C string. Attempt exact
   conversion. If the operand cannot be converted exactly, set
   InvalidOperation. */
static PyObject *
PyDecType_FromCStringExact(PyTypeObject *type, const char *s,
                           PyObject *context)
{
    PyObject *dec;
    uint32_t status = 0;
    mpd_context_t maxctx;

    decimal_state *state = get_module_state_from_ctx(context);
    dec = PyDecType_New(state, type);
    if (dec == NULL) {
        return NULL;
    }

    mpd_maxcontext(&maxctx);

    mpd_qset_string(MPD(dec), s, &maxctx, &status);
    if (status & (MPD_Inexact|MPD_Rounded|MPD_Clamped)) {
        /* we want exact results */
        mpd_seterror(MPD(dec), MPD_Invalid_operation, &status);
    }
    status &= MPD_Errors;
    if (dec_addstatus(context, status)) {
        Py_DECREF(dec);
        return NULL;
    }

    return dec;
}

/* Return a new PyDecObject or a subtype from a PyUnicodeObject. */
static PyObject *
PyDecType_FromUnicode(PyTypeObject *type, PyObject *u,
                      PyObject *context)
{
    PyObject *dec;
    char *s;

    s = numeric_as_ascii(u, 0, 0);
    if (s == NULL) {
        return NULL;
    }

    dec = PyDecType_FromCString(type, s, context);
    PyMem_Free(s);
    return dec;
}

/* Return a new PyDecObject or a subtype from a PyUnicodeObject. Attempt exact
 * conversion. If the conversion is not exact, fail with InvalidOperation.
 * Allow leading and trailing whitespace in the input operand. */
static PyObject *
PyDecType_FromUnicodeExactWS(PyTypeObject *type, PyObject *u,
                             PyObject *context)
{
    PyObject *dec;
    char *s;

    s = numeric_as_ascii(u, 1, 1);
    if (s == NULL) {
        return NULL;
    }

    dec = PyDecType_FromCStringExact(type, s, context);
    PyMem_Free(s);
    return dec;
}

/* Set PyDecObject from triple without any error checking. */
Py_LOCAL_INLINE(void)
_dec_settriple(PyObject *dec, uint8_t sign, uint32_t v, mpd_ssize_t exp)
{

#ifdef CONFIG_64
    MPD(dec)->data[0] = v;
    MPD(dec)->len = 1;
#else
    uint32_t q, r;
    q = v / MPD_RADIX;
    r = v - q * MPD_RADIX;
    MPD(dec)->data[1] = q;
    MPD(dec)->data[0] = r;
    MPD(dec)->len = q ? 2 : 1;
#endif
    mpd_set_flags(MPD(dec), sign);
    MPD(dec)->exp = exp;
    mpd_setdigits(MPD(dec));
}

/* Return a new PyDecObject from an mpd_ssize_t. */
static PyObject *
PyDecType_FromSsize(PyTypeObject *type, mpd_ssize_t v, PyObject *context)
{
    PyObject *dec;
    uint32_t status = 0;

    decimal_state *state = get_module_state_from_ctx(context);
    dec = PyDecType_New(state, type);
    if (dec == NULL) {
        return NULL;
    }

    mpd_qset_ssize(MPD(dec), v, CTX(context), &status);
    if (dec_addstatus(context, status)) {
        Py_DECREF(dec);
        return NULL;
    }
    return dec;
}

/* Return a new PyDecObject from an mpd_ssize_t. Conversion is exact. */
static PyObject *
PyDecType_FromSsizeExact(PyTypeObject *type, mpd_ssize_t v, PyObject *context)
{
    PyObject *dec;
    uint32_t status = 0;
    mpd_context_t maxctx;

    decimal_state *state = get_module_state_from_ctx(context);
    dec = PyDecType_New(state, type);
    if (dec == NULL) {
        return NULL;
    }

    mpd_maxcontext(&maxctx);

    mpd_qset_ssize(MPD(dec), v, &maxctx, &status);
    if (dec_addstatus(context, status)) {
        Py_DECREF(dec);
        return NULL;
    }
    return dec;
}

/* Convert from a PyLongObject. The context is not modified; flags set
   during conversion are accumulated in the status parameter. */
static PyObject *
dec_from_long(decimal_state *state, PyTypeObject *type, PyObject *v,
              const mpd_context_t *ctx, uint32_t *status)
{
    PyObject *dec = PyDecType_New(state, type);

    if (dec == NULL) {
        return NULL;
    }

    PyLongExport export_long;

    if (PyLong_Export(v, &export_long) == -1) {
        Py_DECREF(dec);
        return NULL;
    }
    if (export_long.digits) {
        const PyLongLayout *layout = PyLong_GetNativeLayout();

        assert(layout->bits_per_digit < 32);
        assert(layout->digits_order == -1);
        assert(layout->digit_endianness == (PY_LITTLE_ENDIAN ? -1 : 1));
        assert(layout->digit_size == 2 || layout->digit_size == 4);

        uint32_t base = (uint32_t)1 << layout->bits_per_digit;
        uint8_t sign = export_long.negative ? MPD_NEG : MPD_POS;
        Py_ssize_t len = export_long.ndigits;

        if (layout->digit_size == 4) {
            mpd_qimport_u32(MPD(dec), export_long.digits, len, sign,
                            base, ctx, status);
        }
        else {
            mpd_qimport_u16(MPD(dec), export_long.digits, len, sign,
                            base, ctx, status);
        }
        PyLong_FreeExport(&export_long);
    }
    else {
        mpd_qset_i64(MPD(dec), export_long.value, ctx, status);
    }
    return dec;
}

/* Return a new PyDecObject from a PyLongObject. Use the context for
   conversion. */
static PyObject *
PyDecType_FromLong(PyTypeObject *type, PyObject *v, PyObject *context)
{
    PyObject *dec;
    uint32_t status = 0;

    if (!PyLong_Check(v)) {
        PyErr_SetString(PyExc_TypeError, "argument must be an integer");
        return NULL;
    }

    decimal_state *state = get_module_state_from_ctx(context);
    dec = dec_from_long(state, type, v, CTX(context), &status);
    if (dec == NULL) {
        return NULL;
    }

    if (dec_addstatus(context, status)) {
        Py_DECREF(dec);
        return NULL;
    }

    return dec;
}

/* Return a new PyDecObject from a PyLongObject. Use a maximum context
   for conversion. If the conversion is not exact, set InvalidOperation. */
static PyObject *
PyDecType_FromLongExact(PyTypeObject *type, PyObject *v,
                        PyObject *context)
{
    PyObject *dec;
    uint32_t status = 0;
    mpd_context_t maxctx;

    if (!PyLong_Check(v)) {
        PyErr_SetString(PyExc_TypeError, "argument must be an integer");
        return NULL;
    }

    mpd_maxcontext(&maxctx);
    decimal_state *state = get_module_state_from_ctx(context);
    dec = dec_from_long(state, type, v, &maxctx, &status);
    if (dec == NULL) {
        return NULL;
    }

    if (status & (MPD_Inexact|MPD_Rounded|MPD_Clamped)) {
        /* we want exact results */
        mpd_seterror(MPD(dec), MPD_Invalid_operation, &status);
    }
    status &= MPD_Errors;
    if (dec_addstatus(context, status)) {
        Py_DECREF(dec);
        return NULL;
    }

    return dec;
}

/* Return a PyDecObject or a subtype from a PyFloatObject.
   Conversion is exact. */
static PyObject *
PyDecType_FromFloatExact(PyTypeObject *type, PyObject *v,
                         PyObject *context)
{
    PyObject *dec, *tmp;
    PyObject *n, *d, *n_d;
    mpd_ssize_t k;
    double x;
    int sign;
    mpd_t *d1, *d2;
    uint32_t status = 0;
    mpd_context_t maxctx;
    decimal_state *state = get_module_state_from_ctx(context);

#ifdef Py_DEBUG
    assert(PyType_IsSubtype(type, state->PyDec_Type));
#endif
    if (PyLong_Check(v)) {
        return PyDecType_FromLongExact(type, v, context);
    }
    if (!PyFloat_Check(v)) {
        PyErr_SetString(PyExc_TypeError,
            "argument must be int or float");
        return NULL;
    }

    x = PyFloat_AsDouble(v);
    if (x == -1.0 && PyErr_Occurred()) {
        return NULL;
    }
    sign = (copysign(1.0, x) == 1.0) ? 0 : 1;

    if (isnan(x) || isinf(x)) {
        dec = PyDecType_New(state, type);
        if (dec == NULL) {
            return NULL;
        }
        if (isnan(x)) {
            /* decimal.py calls repr(float(+-nan)),
             * which always gives a positive result. */
            mpd_setspecial(MPD(dec), MPD_POS, MPD_NAN);
        }
        else {
            mpd_setspecial(MPD(dec), sign, MPD_INF);
        }
        return dec;
    }

    /* absolute value of the float */
    tmp = state->_py_float_abs(v);
    if (tmp == NULL) {
        return NULL;
    }

    /* float as integer ratio: numerator/denominator */
    n_d = state->_py_float_as_integer_ratio(tmp, NULL);
    Py_DECREF(tmp);
    if (n_d == NULL) {
        return NULL;
    }
    n = PyTuple_GET_ITEM(n_d, 0);
    d = PyTuple_GET_ITEM(n_d, 1);

    tmp = state->_py_long_bit_length(d, NULL);
    if (tmp == NULL) {
        Py_DECREF(n_d);
        return NULL;
    }
    k = PyLong_AsSsize_t(tmp);
    Py_DECREF(tmp);
    if (k == -1 && PyErr_Occurred()) {
        Py_DECREF(n_d);
        return NULL;
    }
    k--;

    dec = PyDecType_FromLongExact(type, n, context);
    Py_DECREF(n_d);
    if (dec == NULL) {
        return NULL;
    }

    d1 = mpd_qnew();
    if (d1 == NULL) {
        Py_DECREF(dec);
        PyErr_NoMemory();
        return NULL;
    }
    d2 = mpd_qnew();
    if (d2 == NULL) {
        mpd_del(d1);
        Py_DECREF(dec);
        PyErr_NoMemory();
        return NULL;
    }

    mpd_maxcontext(&maxctx);
    mpd_qset_uint(d1, 5, &maxctx, &status);
    mpd_qset_ssize(d2, k, &maxctx, &status);
    mpd_qpow(d1, d1, d2, &maxctx, &status);
    if (dec_addstatus(context, status)) {
        mpd_del(d1);
        mpd_del(d2);
        Py_DECREF(dec);
        return NULL;
    }

    /* result = n * 5**k */
    mpd_qmul(MPD(dec), MPD(dec), d1, &maxctx, &status);
    mpd_del(d1);
    mpd_del(d2);
    if (dec_addstatus(context, status)) {
        Py_DECREF(dec);
        return NULL;
    }
    /* result = +- n * 5**k * 10**-k */
    mpd_set_sign(MPD(dec), sign);
    MPD(dec)->exp = -k;

    return dec;
}

static PyObject *
PyDecType_FromFloat(PyTypeObject *type, PyObject *v,
                    PyObject *context)
{
    PyObject *dec;
    uint32_t status = 0;

    dec = PyDecType_FromFloatExact(type, v, context);
    if (dec == NULL) {
        return NULL;
    }

    mpd_qfinalize(MPD(dec), CTX(context), &status);
    if (dec_addstatus(context, status)) {
        Py_DECREF(dec);
        return NULL;
    }

    return dec;
}

/* Return a new PyDecObject or a subtype from a Decimal. */
static PyObject *
PyDecType_FromDecimalExact(PyTypeObject *type, PyObject *v, PyObject *context)
{
    PyObject *dec;
    uint32_t status = 0;

    decimal_state *state = get_module_state_from_ctx(context);
    if (type == state->PyDec_Type && PyDec_CheckExact(state, v)) {
        return Py_NewRef(v);
    }

    dec = PyDecType_New(state, type);
    if (dec == NULL) {
        return NULL;
    }

    mpd_qcopy(MPD(dec), MPD(v), &status);
    if (dec_addstatus(context, status)) {
        Py_DECREF(dec);
        return NULL;
    }

    return dec;
}

static PyObject *
sequence_as_tuple(PyObject *v, PyObject *ex, const char *mesg)
{
    if (PyTuple_Check(v)) {
        return Py_NewRef(v);
    }
    if (PyList_Check(v)) {
        return PyList_AsTuple(v);
    }

    PyErr_SetString(ex, mesg);
    return NULL;
}

/* Return a new C string representation of a DecimalTuple. */
static char *
dectuple_as_str(PyObject *dectuple)
{
    PyObject *digits = NULL, *tmp;
    char *decstring = NULL;
    char sign_special[6];
    char *cp;
    long sign, l;
    mpd_ssize_t exp = 0;
    Py_ssize_t i, mem, tsize;
    int is_infinite = 0;
    int n;

    assert(PyTuple_Check(dectuple));

    if (PyTuple_Size(dectuple) != 3) {
        PyErr_SetString(PyExc_ValueError,
            "argument must be a sequence of length 3");
        goto error;
    }

    /* sign */
    tmp = PyTuple_GET_ITEM(dectuple, 0);
    if (!PyLong_Check(tmp)) {
        PyErr_SetString(PyExc_ValueError,
            "sign must be an integer with the value 0 or 1");
        goto error;
    }
    sign = PyLong_AsLong(tmp);
    if (sign == -1 && PyErr_Occurred()) {
        goto error;
    }
    if (sign != 0 && sign != 1) {
        PyErr_SetString(PyExc_ValueError,
            "sign must be an integer with the value 0 or 1");
        goto error;
    }
    sign_special[0] = sign ? '-' : '+';
    sign_special[1] = '\0';

    /* exponent or encoding for a special number */
    tmp = PyTuple_GET_ITEM(dectuple, 2);
    if (PyUnicode_Check(tmp)) {
        /* special */
        if (PyUnicode_CompareWithASCIIString(tmp, "F") == 0) {
            strcat(sign_special, "Inf");
            is_infinite = 1;
        }
        else if (PyUnicode_CompareWithASCIIString(tmp, "n") == 0) {
            strcat(sign_special, "NaN");
        }
        else if (PyUnicode_CompareWithASCIIString(tmp, "N") == 0) {
            strcat(sign_special, "sNaN");
        }
        else {
            PyErr_SetString(PyExc_ValueError,
                "string argument in the third position "
                "must be 'F', 'n' or 'N'");
            goto error;
        }
    }
    else {
        /* exponent */
        if (!PyLong_Check(tmp)) {
            PyErr_SetString(PyExc_ValueError,
                "exponent must be an integer");
            goto error;
        }
        exp = PyLong_AsSsize_t(tmp);
        if (exp == -1 && PyErr_Occurred()) {
            goto error;
        }
    }

    /* coefficient */
    digits = sequence_as_tuple(PyTuple_GET_ITEM(dectuple, 1), PyExc_ValueError,
                               "coefficient must be a tuple of digits");
    if (digits == NULL) {
        goto error;
    }

    tsize = PyTuple_Size(digits);
    /* [sign][coeffdigits+1][E][-][expdigits+1]['\0'] */
    mem = 1 + tsize + 3 + MPD_EXPDIGITS + 2;
    cp = decstring = PyMem_Malloc(mem);
    if (decstring == NULL) {
        PyErr_NoMemory();
        goto error;
    }

    n = snprintf(cp, mem, "%s", sign_special);
    if (n < 0 || n >= mem) {
        PyErr_SetString(PyExc_RuntimeError,
            "internal error in dec_sequence_as_str");
        goto error;
    }
    cp += n;

    if (tsize == 0 && sign_special[1] == '\0') {
        /* empty tuple: zero coefficient, except for special numbers */
        *cp++ = '0';
    }
    for (i = 0; i < tsize; i++) {
        tmp = PyTuple_GET_ITEM(digits, i);
        if (!PyLong_Check(tmp)) {
            PyErr_SetString(PyExc_ValueError,
                "coefficient must be a tuple of digits");
            goto error;
        }
        l = PyLong_AsLong(tmp);
        if (l == -1 && PyErr_Occurred()) {
            goto error;
        }
        if (l < 0 || l > 9) {
            PyErr_SetString(PyExc_ValueError,
                "coefficient must be a tuple of digits");
            goto error;
        }
        if (is_infinite) {
            /* accept but ignore any well-formed coefficient for compatibility
               with decimal.py */
            continue;
        }
        *cp++ = (char)l + '0';
    }
    *cp = '\0';

    if (sign_special[1] == '\0') {
        /* not a special number */
        *cp++ = 'E';
        n = snprintf(cp, MPD_EXPDIGITS+2, "%" PRI_mpd_ssize_t, exp);
        if (n < 0 || n >= MPD_EXPDIGITS+2) {
            PyErr_SetString(PyExc_RuntimeError,
                "internal error in dec_sequence_as_str");
            goto error;
        }
    }

    Py_XDECREF(digits);
    return decstring;


error:
    Py_XDECREF(digits);
    if (decstring) PyMem_Free(decstring);
    return NULL;
}

/* Currently accepts tuples and lists. */
static PyObject *
PyDecType_FromSequence(PyTypeObject *type, PyObject *v,
                       PyObject *context)
{
    PyObject *dectuple;
    PyObject *dec;
    char *s;

    dectuple = sequence_as_tuple(v, PyExc_TypeError,
                                 "argument must be a tuple or list");
    if (dectuple == NULL) {
        return NULL;
    }

    s = dectuple_as_str(dectuple);
    Py_DECREF(dectuple);
    if (s == NULL) {
        return NULL;
    }

    dec = PyDecType_FromCString(type, s, context);

    PyMem_Free(s);
    return dec;
}

/* Currently accepts tuples and lists. */
static PyObject *
PyDecType_FromSequenceExact(PyTypeObject *type, PyObject *v,
                            PyObject *context)
{
    PyObject *dectuple;
    PyObject *dec;
    char *s;

    dectuple = sequence_as_tuple(v, PyExc_TypeError,
                   "argument must be a tuple or list");
    if (dectuple == NULL) {
        return NULL;
    }

    s = dectuple_as_str(dectuple);
    Py_DECREF(dectuple);
    if (s == NULL) {
        return NULL;
    }

    dec = PyDecType_FromCStringExact(type, s, context);

    PyMem_Free(s);
    return dec;
}

#define PyDec_FromCString(st, str, context) \
        PyDecType_FromCString((st)->PyDec_Type, str, context)
#define PyDec_FromCStringExact(st, str, context) \
        PyDecType_FromCStringExact((st)->PyDec_Type, str, context)

#define PyDec_FromUnicode(st, unicode, context) \
        PyDecType_FromUnicode((st)->PyDec_Type, unicode, context)
#define PyDec_FromUnicodeExact(st, unicode, context) \
        PyDecType_FromUnicodeExact((st)->PyDec_Type, unicode, context)
#define PyDec_FromUnicodeExactWS(st, unicode, context) \
        PyDecType_FromUnicodeExactWS((st)->PyDec_Type, unicode, context)

#define PyDec_FromSsize(st, v, context) \
        PyDecType_FromSsize((st)->PyDec_Type, v, context)
#define PyDec_FromSsizeExact(st, v, context) \
        PyDecType_FromSsizeExact((st)->PyDec_Type, v, context)

#define PyDec_FromLong(st, pylong, context) \
        PyDecType_FromLong((st)->PyDec_Type, pylong, context)
#define PyDec_FromLongExact(st, pylong, context) \
        PyDecType_FromLongExact((st)->PyDec_Type, pylong, context)

#define PyDec_FromFloat(st, pyfloat, context) \
        PyDecType_FromFloat((st)->PyDec_Type, pyfloat, context)
#define PyDec_FromFloatExact(st, pyfloat, context) \
        PyDecType_FromFloatExact((st)->PyDec_Type, pyfloat, context)

#define PyDec_FromSequence(st, sequence, context) \
        PyDecType_FromSequence((st)->PyDec_Type, sequence, context)
#define PyDec_FromSequenceExact(st, sequence, context) \
        PyDecType_FromSequenceExact((st)->PyDec_Type, sequence, context)

/*[clinic input]
@classmethod
_decimal.Decimal.from_float

    cls: defining_class
    f as pyfloat: object
    /

Class method that converts a float to a decimal number, exactly.

Since 0.1 is not exactly representable in binary floating point,
Decimal.from_float(0.1) is not the same as Decimal('0.1').

    >>> Decimal.from_float(0.1)
    Decimal('0.1000000000000000055511151231257827021181583404541015625')
    >>> Decimal.from_float(float('nan'))
    Decimal('NaN')
    >>> Decimal.from_float(float('inf'))
    Decimal('Infinity')
    >>> Decimal.from_float(float('-inf'))
    Decimal('-Infinity')
[clinic start generated code]*/

static PyObject *
_decimal_Decimal_from_float_impl(PyTypeObject *type, PyTypeObject *cls,
                                 PyObject *pyfloat)
/*[clinic end generated code: output=fcb7d55d2f9dc790 input=03bc8dbe963e52ca]*/
{
    PyObject *context;
    PyObject *result;

    decimal_state *state = PyType_GetModuleState(cls);
    CURRENT_CONTEXT(state, context);
    result = PyDecType_FromFloatExact(state->PyDec_Type, pyfloat, context);
    if (type != state->PyDec_Type && result != NULL) {
        Py_SETREF(result,
            PyObject_CallFunctionObjArgs((PyObject *)type, result, NULL));
    }

    return result;
}

/* 'v' can have any numeric type accepted by the Decimal constructor. Attempt
   an exact conversion. If the result does not meet the restrictions
   for an mpd_t, fail with InvalidOperation. */
static PyObject *
PyDecType_FromNumberExact(PyTypeObject *type, PyTypeObject *cls,
                          PyObject *v, PyObject *context)
{
    decimal_state *state = PyType_GetModuleState(cls);
    assert(v != NULL);
    if (PyDec_Check(state, v)) {
        return PyDecType_FromDecimalExact(type, v, context);
    }
    else if (PyLong_Check(v)) {
        return PyDecType_FromLongExact(type, v, context);
    }
    else if (PyFloat_Check(v)) {
        if (dec_addstatus(context, MPD_Float_operation)) {
            return NULL;
        }
        return PyDecType_FromFloatExact(type, v, context);
    }
    else {
        PyErr_Format(PyExc_TypeError,
            "conversion from %s to Decimal is not supported",
            Py_TYPE(v)->tp_name);
        return NULL;
    }
}

/*[clinic input]
@classmethod
_decimal.Decimal.from_number

    cls: defining_class
    number: object
    /

Class method that converts a real number to a decimal number, exactly.

    >>> Decimal.from_number(314)              # int
    Decimal('314')
    >>> Decimal.from_number(0.1)              # float
    Decimal('0.1000000000000000055511151231257827021181583404541015625')
    >>> Decimal.from_number(Decimal('3.14'))  # another decimal instance
    Decimal('3.14')
[clinic start generated code]*/

static PyObject *
_decimal_Decimal_from_number_impl(PyTypeObject *type, PyTypeObject *cls,
                                  PyObject *number)
/*[clinic end generated code: output=4d3ec722b7acfd8b input=271cb4feb3148804]*/
{
    PyObject *context;
    PyObject *result;

    decimal_state *state = PyType_GetModuleState(cls);
    CURRENT_CONTEXT(state, context);
    result = PyDecType_FromNumberExact(state->PyDec_Type, cls, number, context);
    if (type != state->PyDec_Type && result != NULL) {
        Py_SETREF(result,
            PyObject_CallFunctionObjArgs((PyObject *)type, result, NULL));
    }

    return result;
}

/* create_decimal_from_float */

/*[clinic input]
_decimal.Context.create_decimal_from_float

    self as context: self
    cls: defining_class
    f: object
    /

Create a new Decimal instance from float f.

Unlike the Decimal.from_float() class method, this function observes
the context limits.
[clinic start generated code]*/

static PyObject *
_decimal_Context_create_decimal_from_float_impl(PyObject *context,
                                                PyTypeObject *cls,
                                                PyObject *f)
/*[clinic end generated code: output=a5548f5140fa0870 input=8c66eeb22b01ddd4]*/
{
    decimal_state *state = PyType_GetModuleState(cls);
    return PyDec_FromFloat(state, f, context);
}

/* Apply the context to the input operand. Return a new PyDecObject. */
static PyObject *
dec_apply(PyObject *v, PyObject *context)
{
    PyObject *result;
    uint32_t status = 0;

    decimal_state *state = get_module_state_from_ctx(context);
    result = dec_alloc(state);
    if (result == NULL) {
        return NULL;
    }

    mpd_qcopy(MPD(result), MPD(v), &status);
    if (dec_addstatus(context, status)) {
        Py_DECREF(result);
        return NULL;
    }

    mpd_qfinalize(MPD(result), CTX(context), &status);
    if (dec_addstatus(context, status)) {
        Py_DECREF(result);
        return NULL;
    }

    return result;
}

/* 'v' can have any type accepted by the Decimal constructor. Attempt
   an exact conversion. If the result does not meet the restrictions
   for an mpd_t, fail with InvalidOperation. */
static PyObject *
PyDecType_FromObjectExact(PyTypeObject *type, PyObject *v, PyObject *context)
{
    decimal_state *state = get_module_state_from_ctx(context);
    if (v == NULL) {
        return PyDecType_FromSsizeExact(type, 0, context);
    }
    else if (PyDec_Check(state, v)) {
        return PyDecType_FromDecimalExact(type, v, context);
    }
    else if (PyUnicode_Check(v)) {
        return PyDecType_FromUnicodeExactWS(type, v, context);
    }
    else if (PyLong_Check(v)) {
        return PyDecType_FromLongExact(type, v, context);
    }
    else if (PyTuple_Check(v) || PyList_Check(v)) {
        return PyDecType_FromSequenceExact(type, v, context);
    }
    else if (PyFloat_Check(v)) {
        if (dec_addstatus(context, MPD_Float_operation)) {
            return NULL;
        }
        return PyDecType_FromFloatExact(type, v, context);
    }
    else {
        PyErr_Format(PyExc_TypeError,
            "conversion from %s to Decimal is not supported",
            Py_TYPE(v)->tp_name);
        return NULL;
    }
}

/* The context is used during conversion. This function is the
   equivalent of context.create_decimal(). */
static PyObject *
PyDec_FromObject(PyObject *v, PyObject *context)
{
    decimal_state *state = get_module_state_from_ctx(context);
    if (v == NULL) {
        return PyDec_FromSsize(state, 0, context);
    }
    else if (PyDec_Check(state, v)) {
        mpd_context_t *ctx = CTX(context);
        if (mpd_isnan(MPD(v)) &&
            MPD(v)->digits > ctx->prec - ctx->clamp) {
            /* Special case: too many NaN payload digits */
            PyObject *result;
            if (dec_addstatus(context, MPD_Conversion_syntax)) {
                return NULL;
            }
            result = dec_alloc(state);
            if (result == NULL) {
                return NULL;
            }
            mpd_setspecial(MPD(result), MPD_POS, MPD_NAN);
            return result;
        }
        return dec_apply(v, context);
    }
    else if (PyUnicode_Check(v)) {
        return PyDec_FromUnicode(state, v, context);
    }
    else if (PyLong_Check(v)) {
        return PyDec_FromLong(state, v, context);
    }
    else if (PyTuple_Check(v) || PyList_Check(v)) {
        return PyDec_FromSequence(state, v, context);
    }
    else if (PyFloat_Check(v)) {
        if (dec_addstatus(context, MPD_Float_operation)) {
            return NULL;
        }
        return PyDec_FromFloat(state, v, context);
    }
    else {
        PyErr_Format(PyExc_TypeError,
            "conversion from %s to Decimal is not supported",
            Py_TYPE(v)->tp_name);
        return NULL;
    }
}

/*[clinic input]
@classmethod
_decimal.Decimal.__new__ as dec_new

    value: object(c_default="NULL") = "0"
    context: object = None

Construct a new Decimal object.

value can be an integer, string, tuple, or another Decimal object.  If
no value is given, return Decimal('0'). The context does not affect
the conversion and is only passed to determine if the InvalidOperation
trap is active.
[clinic start generated code]*/

static PyObject *
dec_new_impl(PyTypeObject *type, PyObject *value, PyObject *context)
/*[clinic end generated code: output=35f48a40c65625ba input=5f8a0892d3fcef80]*/
{
    decimal_state *state = get_module_state_by_def(type);
    CONTEXT_CHECK_VA(state, context);

    return PyDecType_FromObjectExact(type, value, context);
}

/*[clinic input]
_decimal.Context.create_decimal

    self as context: self
    num: object(c_default="NULL") = "0"
    /

Create a new Decimal instance from num, using self as the context.

Unlike the Decimal constructor, this function observes the context
limits.
[clinic start generated code]*/

static PyObject *
_decimal_Context_create_decimal_impl(PyObject *context, PyObject *num)
/*[clinic end generated code: output=85e08ae02f3b34da input=d2c4946cf7804fbe]*/
{
    return PyDec_FromObject(num, context);
}


/******************************************************************************/
/*                        Implicit conversions to Decimal                     */
/******************************************************************************/

/* Try to convert PyObject v to a new PyDecObject conv. If the conversion
   fails, set conv to NULL (exception is set). If the conversion is not
   implemented, set conv to Py_NotImplemented. */
#define NOT_IMPL 0
#define TYPE_ERR 1
Py_LOCAL_INLINE(int)
convert_op(int type_err, PyObject **conv, PyObject *v, PyObject *context)
{
    decimal_state *state = get_module_state_from_ctx(context);
    if (PyDec_Check(state, v)) {
        *conv = Py_NewRef(v);
        return 1;
    }
    if (PyLong_Check(v)) {
        *conv = PyDec_FromLongExact(state, v, context);
        if (*conv == NULL) {
            return 0;
        }
        return 1;
    }

    if (type_err) {
        PyErr_Format(PyExc_TypeError,
            "conversion from %s to Decimal is not supported",
            Py_TYPE(v)->tp_name);
    }
    else {
        *conv = Py_NewRef(Py_NotImplemented);
    }
    return 0;
}

/* Return NotImplemented for unsupported types. */
#define CONVERT_OP(a, v, context) \
    if (!convert_op(NOT_IMPL, a, v, context)) { \
        return *(a);                            \
    }

#define CONVERT_BINOP(a, b, v, w, context) \
    if (!convert_op(NOT_IMPL, a, v, context)) { \
        return *(a);                            \
    }                                           \
    if (!convert_op(NOT_IMPL, b, w, context)) { \
        Py_DECREF(*(a));                        \
        return *(b);                            \
    }

#define CONVERT_TERNOP(a, b, c, v, w, x, context) \
    if (!convert_op(NOT_IMPL, a, v, context)) {   \
        return *(a);                              \
    }                                             \
    if (!convert_op(NOT_IMPL, b, w, context)) {   \
        Py_DECREF(*(a));                          \
        return *(b);                              \
    }                                             \
    if (!convert_op(NOT_IMPL, c, x, context)) {   \
        Py_DECREF(*(a));                          \
        Py_DECREF(*(b));                          \
        return *(c);                              \
    }

/* Raise TypeError for unsupported types. */
#define CONVERT_OP_RAISE(a, v, context) \
    if (!convert_op(TYPE_ERR, a, v, context)) { \
        return NULL;                            \
    }

#define CONVERT_BINOP_RAISE(a, b, v, w, context) \
    if (!convert_op(TYPE_ERR, a, v, context)) {  \
        return NULL;                             \
    }                                            \
    if (!convert_op(TYPE_ERR, b, w, context)) {  \
        Py_DECREF(*(a));                         \
        return NULL;                             \
    }

#define CONVERT_TERNOP_RAISE(a, b, c, v, w, x, context) \
    if (!convert_op(TYPE_ERR, a, v, context)) {         \
        return NULL;                                    \
    }                                                   \
    if (!convert_op(TYPE_ERR, b, w, context)) {         \
        Py_DECREF(*(a));                                \
        return NULL;                                    \
    }                                                   \
    if (!convert_op(TYPE_ERR, c, x, context)) {         \
        Py_DECREF(*(a));                                \
        Py_DECREF(*(b));                                \
        return NULL;                                    \
    }


/******************************************************************************/
/*              Implicit conversions to Decimal for comparison                */
/******************************************************************************/

static PyObject *
multiply_by_denominator(PyObject *v, PyObject *r, PyObject *context)
{
    PyObject *result;
    PyObject *tmp = NULL;
    PyObject *denom = NULL;
    uint32_t status = 0;
    mpd_context_t maxctx;
    mpd_ssize_t exp;
    mpd_t *vv;

    /* v is not special, r is a rational */
    tmp = PyObject_GetAttrString(r, "denominator");
    if (tmp == NULL) {
        return NULL;
    }
    decimal_state *state = get_module_state_from_ctx(context);
    denom = PyDec_FromLongExact(state, tmp, context);
    Py_DECREF(tmp);
    if (denom == NULL) {
        return NULL;
    }

    vv = mpd_qncopy(MPD(v));
    if (vv == NULL) {
        Py_DECREF(denom);
        PyErr_NoMemory();
        return NULL;
    }
    result = dec_alloc(state);
    if (result == NULL) {
        Py_DECREF(denom);
        mpd_del(vv);
        return NULL;
    }

    mpd_maxcontext(&maxctx);
    /* Prevent Overflow in the following multiplication. The result of
       the multiplication is only used in mpd_qcmp, which can handle
       values that are technically out of bounds, like (for 32-bit)
       99999999999999999999...99999999e+425000000. */
    exp = vv->exp;
    vv->exp = 0;
    mpd_qmul(MPD(result), vv, MPD(denom), &maxctx, &status);
    MPD(result)->exp = exp;

    Py_DECREF(denom);
    mpd_del(vv);
    /* If any status has been accumulated during the multiplication,
       the result is invalid. This is very unlikely, since even the
       32-bit version supports 425000000 digits. */
    if (status) {
        PyErr_SetString(PyExc_ValueError,
            "exact conversion for comparison failed");
        Py_DECREF(result);
        return NULL;
    }

    return result;
}

static PyObject *
numerator_as_decimal(PyObject *r, PyObject *context)
{
    PyObject *tmp, *num;

    tmp = PyObject_GetAttrString(r, "numerator");
    if (tmp == NULL) {
        return NULL;
    }

    decimal_state *state = get_module_state_from_ctx(context);
    num = PyDec_FromLongExact(state, tmp, context);
    Py_DECREF(tmp);
    return num;
}

/* Convert v and w for comparison. v is a Decimal. If w is a Rational, both
   v and w have to be transformed. Return 1 for success, with new references
   to the converted objects in vcmp and wcmp. Return 0 for failure. In that
   case wcmp is either NULL or Py_NotImplemented (new reference) and vcmp
   is undefined. */
static int
convert_op_cmp(PyObject **vcmp, PyObject **wcmp, PyObject *v, PyObject *w,
               int op, PyObject *context)
{
    mpd_context_t *ctx = CTX(context);

    *vcmp = v;

    decimal_state *state = get_module_state_from_ctx(context);
    if (PyDec_Check(state, w)) {
        *wcmp = Py_NewRef(w);
    }
    else if (PyLong_Check(w)) {
        *wcmp = PyDec_FromLongExact(state, w, context);
    }
    else if (PyFloat_Check(w)) {
        if (op != Py_EQ && op != Py_NE &&
            dec_addstatus(context, MPD_Float_operation)) {
            *wcmp = NULL;
        }
        else {
            ctx->status |= MPD_Float_operation;
            *wcmp = PyDec_FromFloatExact(state, w, context);
        }
    }
    else if (PyComplex_Check(w) && (op == Py_EQ || op == Py_NE)) {
        Py_complex c = PyComplex_AsCComplex(w);
        if (c.real == -1.0 && PyErr_Occurred()) {
            *wcmp = NULL;
        }
        else if (c.imag == 0.0) {
            PyObject *tmp = PyFloat_FromDouble(c.real);
            if (tmp == NULL) {
                *wcmp = NULL;
            }
            else {
                ctx->status |= MPD_Float_operation;
                *wcmp = PyDec_FromFloatExact(state, tmp, context);
                Py_DECREF(tmp);
            }
        }
        else {
            *wcmp = Py_NewRef(Py_NotImplemented);
        }
    }
    else {
        int is_rational = PyObject_IsInstance(w, state->Rational);
        if (is_rational < 0) {
            *wcmp = NULL;
        }
        else if (is_rational > 0) {
            *wcmp = numerator_as_decimal(w, context);
            if (*wcmp && !mpd_isspecial(MPD(v))) {
                *vcmp = multiply_by_denominator(v, w, context);
                if (*vcmp == NULL) {
                    Py_CLEAR(*wcmp);
                }
            }
        }
        else {
            *wcmp = Py_NewRef(Py_NotImplemented);
        }
    }

    if (*wcmp == NULL || *wcmp == Py_NotImplemented) {
        return 0;
    }
    if (*vcmp == v) {
        Py_INCREF(v);
    }
    return 1;
}

#define CONVERT_BINOP_CMP(vcmp, wcmp, v, w, op, ctx) \
    if (!convert_op_cmp(vcmp, wcmp, v, w, op, ctx)) {  \
        return *(wcmp);                                \
    }                                                  \


/******************************************************************************/
/*                          Conversions from decimal                          */
/******************************************************************************/

static PyObject *
unicode_fromascii(const char *s, Py_ssize_t size)
{
    PyObject *res;

    res = PyUnicode_New(size, 127);
    if (res == NULL) {
        return NULL;
    }

    memcpy(PyUnicode_1BYTE_DATA(res), s, size);
    return res;
}

/* PyDecObject as a string. The default module context is only used for
   the value of 'capitals'. */
static PyObject *
dec_str(PyObject *dec)
{
    PyObject *res, *context;
    mpd_ssize_t size;
    char *cp;

    decimal_state *state = get_module_state_by_def(Py_TYPE(dec));
    CURRENT_CONTEXT(state, context);
    size = mpd_to_sci_size(&cp, MPD(dec), CtxCaps(context));
    if (size < 0) {
        PyErr_NoMemory();
        return NULL;
    }

    res = unicode_fromascii(cp, size);
    mpd_free(cp);
    return res;
}

/* Representation of a PyDecObject. */
static PyObject *
dec_repr(PyObject *dec)
{
    PyObject *res, *context;
    char *cp;
    decimal_state *state = get_module_state_by_def(Py_TYPE(dec));
    CURRENT_CONTEXT(state, context);
    cp = mpd_to_sci(MPD(dec), CtxCaps(context));
    if (cp == NULL) {
        PyErr_NoMemory();
        return NULL;
    }

    res = PyUnicode_FromFormat("Decimal('%s')", cp);
    mpd_free(cp);
    return res;
}

/* Return a duplicate of src, copy embedded null characters. */
static char *
dec_strdup(const char *src, Py_ssize_t size)
{
    char *dest = PyMem_Malloc(size+1);
    if (dest == NULL) {
        PyErr_NoMemory();
        return NULL;
    }

    memcpy(dest, src, size);
    dest[size] = '\0';
    return dest;
}

static void
dec_replace_fillchar(char *dest)
{
     while (*dest != '\0') {
         if (*dest == '\xff') *dest = '\0';
         dest++;
     }
}

/* Convert decimal_point or thousands_sep, which may be multibyte or in
   the range [128, 255], to a UTF8 string. */
static PyObject *
dotsep_as_utf8(const char *s)
{
    PyObject *utf8;
    PyObject *tmp;
    wchar_t buf[2];
    size_t n;

    n = mbstowcs(buf, s, 2);
    if (n != 1) { /* Issue #7442 */
        PyErr_SetString(PyExc_ValueError,
            "invalid decimal point or unsupported "
            "combination of LC_CTYPE and LC_NUMERIC");
        return NULL;
    }
    tmp = PyUnicode_FromWideChar(buf, n);
    if (tmp == NULL) {
        return NULL;
    }
    utf8 = PyUnicode_AsUTF8String(tmp);
    Py_DECREF(tmp);
    return utf8;
}

static int
dict_get_item_string(PyObject *dict, const char *key, PyObject **valueobj, const char **valuestr)
{
    *valueobj = NULL;
    PyObject *keyobj = PyUnicode_FromString(key);
    if (keyobj == NULL) {
        return -1;
    }
    PyObject *value = PyDict_GetItemWithError(dict, keyobj);
    Py_DECREF(keyobj);
    if (value == NULL) {
        if (PyErr_Occurred()) {
            return -1;
        }
        return 0;
    }
    value = PyUnicode_AsUTF8String(value);
    if (value == NULL) {
        return -1;
    }
    *valueobj = value;
    *valuestr = PyBytes_AS_STRING(value);
    return 0;
}

/*
 * Fallback _pydecimal formatting for new format specifiers that mpdecimal does
 * not yet support. As documented, libmpdec follows the PEP-3101 format language:
 * https://www.bytereef.org/mpdecimal/doc/libmpdec/assign-convert.html#to-string
 */
static PyObject *
pydec_format(PyObject *dec, PyObject *context, PyObject *fmt, decimal_state *state)
{
    PyObject *result;
    PyObject *pydec;
    PyObject *u;

    if (state->PyDecimal == NULL) {
        state->PyDecimal = PyImport_ImportModuleAttrString("_pydecimal", "Decimal");
        if (state->PyDecimal == NULL) {
            return NULL;
        }
    }

    u = dec_str(dec);
    if (u == NULL) {
        return NULL;
    }

    pydec = PyObject_CallOneArg(state->PyDecimal, u);
    Py_DECREF(u);
    if (pydec == NULL) {
        return NULL;
    }

    result = PyObject_CallMethod(pydec, "__format__", "(OO)", fmt, context);
    Py_DECREF(pydec);

    if (result == NULL && PyErr_ExceptionMatches(PyExc_ValueError)) {
        /* Do not confuse users with the _pydecimal exception */
        PyErr_Clear();
        PyErr_SetString(PyExc_ValueError, "invalid format string");
    }

    return result;
}

/* Formatted representation of a PyDecObject. */

/*[clinic input]
_decimal.Decimal.__format__

    self as dec: self
    cls: defining_class
    format_spec as fmtarg: unicode
    override: object = NULL
    /

Formats the Decimal according to format_spec.
[clinic start generated code]*/

static PyObject *
_decimal_Decimal___format___impl(PyObject *dec, PyTypeObject *cls,
                                 PyObject *fmtarg, PyObject *override)
/*[clinic end generated code: output=6d95f91bbb28b3ed input=2dbfaa0cbe243e9e]*/
{
    PyObject *result = NULL;
    PyObject *dot = NULL;
    PyObject *sep = NULL;
    PyObject *grouping = NULL;
    PyObject *context;
    mpd_spec_t spec;
    char *fmt;
    char *decstring = NULL;
    uint32_t status = 0;
    int replace_fillchar = 0;
    Py_ssize_t size;
    decimal_state *state = PyType_GetModuleState(cls);
    CURRENT_CONTEXT(state, context);
    fmt = (char *)PyUnicode_AsUTF8AndSize(fmtarg, &size);
    if (fmt == NULL) {
        return NULL;
    }

    if (size > 0 && fmt[size-1] == 'N') {
        if (PyErr_WarnEx(PyExc_DeprecationWarning,
                         "Format specifier 'N' is deprecated", 1) < 0) {
            return NULL;
        }
    }

    if (size > 0 && fmt[0] == '\0') {
        /* NUL fill character: must be replaced with a valid UTF-8 char
           before calling mpd_parse_fmt_str(). */
        replace_fillchar = 1;
        fmt = dec_strdup(fmt, size);
        if (fmt == NULL) {
            return NULL;
        }
        fmt[0] = '_';
    }

    if (!mpd_parse_fmt_str(&spec, fmt, CtxCaps(context))) {
        if (replace_fillchar) {
            PyMem_Free(fmt);
        }

        return pydec_format(dec, context, fmtarg, state);
    }

    if (replace_fillchar) {
        /* In order to avoid clobbering parts of UTF-8 thousands separators or
           decimal points when the substitution is reversed later, the actual
           placeholder must be an invalid UTF-8 byte. */
        spec.fill[0] = '\xff';
        spec.fill[1] = '\0';
    }

    if (override) {
        /* Values for decimal_point, thousands_sep and grouping can
           be explicitly specified in the override dict. These values
           take precedence over the values obtained from localeconv()
           in mpd_parse_fmt_str(). The feature is not documented and
           is only used in test_decimal. */
        if (!PyDict_Check(override)) {
            PyErr_SetString(PyExc_TypeError,
                "optional argument must be a dict");
            goto finish;
        }
        if (dict_get_item_string(override, "decimal_point", &dot, &spec.dot) ||
            dict_get_item_string(override, "thousands_sep", &sep, &spec.sep) ||
            dict_get_item_string(override, "grouping", &grouping, &spec.grouping))
        {
            goto finish;
        }
        if (mpd_validate_lconv(&spec) < 0) {
            PyErr_SetString(PyExc_ValueError,
                "invalid override dict");
            goto finish;
        }
    }
    else {
        size_t n = strlen(spec.dot);
        if (n > 1 || (n == 1 && !isascii((unsigned char)spec.dot[0]))) {
            /* fix locale dependent non-ascii characters */
            dot = dotsep_as_utf8(spec.dot);
            if (dot == NULL) {
                goto finish;
            }
            spec.dot = PyBytes_AS_STRING(dot);
        }
        n = strlen(spec.sep);
        if (n > 1 || (n == 1 && !isascii((unsigned char)spec.sep[0]))) {
            /* fix locale dependent non-ascii characters */
            sep = dotsep_as_utf8(spec.sep);
            if (sep == NULL) {
                goto finish;
            }
            spec.sep = PyBytes_AS_STRING(sep);
        }
    }


    decstring = mpd_qformat_spec(MPD(dec), &spec, CTX(context), &status);
    if (decstring == NULL) {
        if (status & MPD_Malloc_error) {
            PyErr_NoMemory();
        }
        else {
            PyErr_SetString(PyExc_ValueError,
                "format specification exceeds internal limits of _decimal");
        }
        goto finish;
    }
    size = strlen(decstring);
    if (replace_fillchar) {
        dec_replace_fillchar(decstring);
    }

    result = PyUnicode_DecodeUTF8(decstring, size, NULL);


finish:
    Py_XDECREF(grouping);
    Py_XDECREF(sep);
    Py_XDECREF(dot);
    if (replace_fillchar) PyMem_Free(fmt);
    if (decstring) mpd_free(decstring);
    return result;
}

/* Return a PyLongObject from a PyDecObject, using the specified rounding
 * mode. The context precision is not observed. */
static PyObject *
dec_as_long(PyObject *dec, PyObject *context, int round)
{
    if (mpd_isspecial(MPD(dec))) {
        if (mpd_isnan(MPD(dec))) {
            PyErr_SetString(PyExc_ValueError,
                "cannot convert NaN to integer");
        }
        else {
            PyErr_SetString(PyExc_OverflowError,
                "cannot convert Infinity to integer");
        }
        return NULL;
    }

    mpd_t *x = mpd_qnew();

    if (x == NULL) {
        PyErr_NoMemory();
        return NULL;
    }

    mpd_context_t workctx = *CTX(context);
    uint32_t status = 0;

    workctx.round = round;
    mpd_qround_to_int(x, MPD(dec), &workctx, &status);
    if (dec_addstatus(context, status)) {
        mpd_del(x);
        return NULL;
    }

    status = 0;
    int64_t val = mpd_qget_i64(x, &status);

    if (!status) {
        mpd_del(x);
        return PyLong_FromInt64(val);
    }
    assert(!mpd_iszero(x));

    const PyLongLayout *layout = PyLong_GetNativeLayout();

    assert(layout->bits_per_digit < 32);
    assert(layout->digits_order == -1);
    assert(layout->digit_endianness == (PY_LITTLE_ENDIAN ? -1 : 1));
    assert(layout->digit_size == 2 || layout->digit_size == 4);

    uint32_t base = (uint32_t)1 << layout->bits_per_digit;
    /* We use a temporary buffer for digits for now, as for nonzero rdata
       mpd_qexport_u32/u16() require either space "allocated by one of
       libmpdec’s allocation functions" or "rlen MUST be correct" (to avoid
       reallocation).  This can be further optimized by using rlen from
       mpd_sizeinbase().  See gh-127925. */
    void *tmp_digits = NULL;
    size_t n;

    status = 0;
    if (layout->digit_size == 4) {
        n = mpd_qexport_u32((uint32_t **)&tmp_digits, 0, base, x, &status);
    }
    else {
        n = mpd_qexport_u16((uint16_t **)&tmp_digits, 0, base, x, &status);
    }

    if (n == SIZE_MAX) {
        PyErr_NoMemory();
        mpd_del(x);
        mpd_free(tmp_digits);
        return NULL;
    }

    void *digits;
    PyLongWriter *writer = PyLongWriter_Create(mpd_isnegative(x), n, &digits);

    mpd_del(x);
    if (writer == NULL) {
        mpd_free(tmp_digits);
        return NULL;
    }
    memcpy(digits, tmp_digits, layout->digit_size*n);
    mpd_free(tmp_digits);
    return PyLongWriter_Finish(writer);
}

/*[clinic input]
_decimal.Decimal.as_integer_ratio

    cls: defining_class

Return a pair of integers whose ratio is exactly equal to the original.

The ratio is in lowest terms and with a positive denominator.
Raise OverflowError on infinities and a ValueError on NaNs.
[clinic start generated code]*/

static PyObject *
_decimal_Decimal_as_integer_ratio_impl(PyObject *self, PyTypeObject *cls)
/*[clinic end generated code: output=eb49c512701f844b input=07e33d8852184761]*/
{
    PyObject *numerator = NULL;
    PyObject *denominator = NULL;
    PyObject *exponent = NULL;
    PyObject *result = NULL;
    PyObject *tmp;
    mpd_ssize_t exp;
    PyObject *context;
    uint32_t status = 0;

    if (mpd_isspecial(MPD(self))) {
        if (mpd_isnan(MPD(self))) {
            PyErr_SetString(PyExc_ValueError,
                "cannot convert NaN to integer ratio");
        }
        else {
            PyErr_SetString(PyExc_OverflowError,
                "cannot convert Infinity to integer ratio");
        }
        return NULL;
    }

    decimal_state *state = PyType_GetModuleState(cls);
    CURRENT_CONTEXT(state, context);

    tmp = dec_alloc(state);
    if (tmp == NULL) {
        return NULL;
    }

    if (!mpd_qcopy(MPD(tmp), MPD(self), &status)) {
        Py_DECREF(tmp);
        PyErr_NoMemory();
        return NULL;
    }

    exp = mpd_iszero(MPD(tmp)) ? 0 : MPD(tmp)->exp;
    MPD(tmp)->exp = 0;

    /* context and rounding are unused here: the conversion is exact */
    numerator = dec_as_long(tmp, context, MPD_ROUND_FLOOR);
    Py_DECREF(tmp);
    if (numerator == NULL) {
        goto error;
    }

    exponent = PyLong_FromSsize_t(exp < 0 ? -exp : exp);
    if (exponent == NULL) {
        goto error;
    }

    tmp = PyLong_FromLong(10);
    if (tmp == NULL) {
        goto error;
    }

    Py_SETREF(exponent, state->_py_long_power(tmp, exponent, Py_None));
    Py_DECREF(tmp);
    if (exponent == NULL) {
        goto error;
    }

    if (exp >= 0) {
        Py_SETREF(numerator, state->_py_long_multiply(numerator, exponent));
        if (numerator == NULL) {
            goto error;
        }
        denominator = PyLong_FromLong(1);
        if (denominator == NULL) {
            goto error;
        }
    }
    else {
        denominator = exponent;
        exponent = NULL;
        tmp = _PyLong_GCD(numerator, denominator);
        if (tmp == NULL) {
            goto error;
        }
        Py_SETREF(numerator, state->_py_long_floor_divide(numerator, tmp));
        if (numerator == NULL) {
            Py_DECREF(tmp);
            goto error;
        }
        Py_SETREF(denominator, state->_py_long_floor_divide(denominator, tmp));
        Py_DECREF(tmp);
        if (denominator == NULL) {
            goto error;
        }
    }

    result = PyTuple_Pack(2, numerator, denominator);


error:
    Py_XDECREF(exponent);
    Py_XDECREF(denominator);
    Py_XDECREF(numerator);
    return result;
}

/*[clinic input]
_decimal.Decimal.to_integral_value

    cls: defining_class
    rounding: object = None
    context: object = None

Round to the nearest integer without signaling Inexact or Rounded.

The rounding mode is determined by the rounding parameter if given,
else by the given context. If neither parameter is given, then the
rounding mode of the current default context is used.
[clinic start generated code]*/

static PyObject *
_decimal_Decimal_to_integral_value_impl(PyObject *self, PyTypeObject *cls,
                                        PyObject *rounding,
                                        PyObject *context)
/*[clinic end generated code: output=23047d848ef84db1 input=85aa9499a21ea8d7]*/
{
    PyObject *result;
    uint32_t status = 0;
    mpd_context_t workctx;

    decimal_state *state = PyType_GetModuleState(cls);
    CONTEXT_CHECK_VA(state, context);

    workctx = *CTX(context);
    if (rounding != Py_None) {
        int round = getround(state, rounding);
        if (round < 0) {
            return NULL;
        }
        if (!mpd_qsetround(&workctx, round)) {
            INTERNAL_ERROR_PTR("PyDec_ToIntegralValue"); /* GCOV_NOT_REACHED */
        }
    }

    result = dec_alloc(state);
    if (result == NULL) {
        return NULL;
    }

    mpd_qround_to_int(MPD(result), MPD(self), &workctx, &status);
    if (dec_addstatus(context, status)) {
        Py_DECREF(result);
        return NULL;
    }

    return result;
}

/*[clinic input]
_decimal.Decimal.to_integral = _decimal.Decimal.to_integral_value

Identical to the to_integral_value() method.

The to_integral() name has been kept for compatibility with older
versions.
[clinic start generated code]*/

static PyObject *
_decimal_Decimal_to_integral_impl(PyObject *self, PyTypeObject *cls,
                                  PyObject *rounding, PyObject *context)
/*[clinic end generated code: output=5dac8f54c2a3ed26 input=709b54618ecd0d8b]*/
{
    return _decimal_Decimal_to_integral_value_impl(self, cls, rounding,
                                                   context);
}

/*[clinic input]
_decimal.Decimal.to_integral_exact = _decimal.Decimal.to_integral_value

Round to the nearest integer.

Decimal.to_integral_exact() signals Inexact or Rounded as appropriate
if rounding occurs.  The rounding mode is determined by the rounding
parameter if given, else by the given context. If neither parameter is
given, then the rounding mode of the current default context is used.
[clinic start generated code]*/

static PyObject *
_decimal_Decimal_to_integral_exact_impl(PyObject *self, PyTypeObject *cls,
                                        PyObject *rounding,
                                        PyObject *context)
/*[clinic end generated code: output=543a39a02eea9917 input=fabce7a744b8087c]*/
{
    PyObject *result;
    uint32_t status = 0;
    mpd_context_t workctx;

    decimal_state *state = PyType_GetModuleState(cls);
    CONTEXT_CHECK_VA(state, context);

    workctx = *CTX(context);
    if (rounding != Py_None) {
        int round = getround(state, rounding);
        if (round < 0) {
            return NULL;
        }
        if (!mpd_qsetround(&workctx, round)) {
            INTERNAL_ERROR_PTR("PyDec_ToIntegralExact"); /* GCOV_NOT_REACHED */
        }
    }

    result = dec_alloc(state);
    if (result == NULL) {
        return NULL;
    }

    mpd_qround_to_intx(MPD(result), MPD(self), &workctx, &status);
    if (dec_addstatus(context, status)) {
        Py_DECREF(result);
        return NULL;
    }

    return result;
}

static PyObject *
PyDec_AsFloat(PyObject *dec)
{
    PyObject *f, *s;

    if (mpd_isnan(MPD(dec))) {
        if (mpd_issnan(MPD(dec))) {
            PyErr_SetString(PyExc_ValueError,
                "cannot convert signaling NaN to float");
            return NULL;
        }
        if (mpd_isnegative(MPD(dec))) {
            s = PyUnicode_FromString("-nan");
        }
        else {
            s = PyUnicode_FromString("nan");
        }
    }
    else {
        s = dec_str(dec);
    }

    if (s == NULL) {
        return NULL;
    }

    f = PyFloat_FromString(s);
    Py_DECREF(s);

    return f;
}

/*[clinic input]
_decimal.Decimal.__round__

    cls: defining_class
    ndigits: object = NULL
    /

Return the Integral closest to self, rounding half toward even.
[clinic start generated code]*/

static PyObject *
_decimal_Decimal___round___impl(PyObject *self, PyTypeObject *cls,
                                PyObject *ndigits)
/*[clinic end generated code: output=790c2c6bd57890e6 input=d69e7178a58a66b1]*/
{
    PyObject *result;
    uint32_t status = 0;
    PyObject *context;
    decimal_state *state = PyType_GetModuleState(cls);
    CURRENT_CONTEXT(state, context);
    if (ndigits) {
        mpd_uint_t dq[1] = {1};
        mpd_t q = {MPD_STATIC|MPD_CONST_DATA,0,1,1,1,dq};
        mpd_ssize_t y;

        if (!PyLong_Check(ndigits)) {
            PyErr_SetString(PyExc_TypeError,
                "optional arg must be an integer");
            return NULL;
        }

        y = PyLong_AsSsize_t(ndigits);
        if (y == -1 && PyErr_Occurred()) {
            return NULL;
        }
        result = dec_alloc(state);
        if (result == NULL) {
            return NULL;
        }

        q.exp = (y == MPD_SSIZE_MIN) ? MPD_SSIZE_MAX : -y;
        mpd_qquantize(MPD(result), MPD(self), &q, CTX(context), &status);
        if (dec_addstatus(context, status)) {
            Py_DECREF(result);
            return NULL;
        }

        return result;
    }
    else {
        return dec_as_long(self, context, MPD_ROUND_HALF_EVEN);
    }
}

/*[clinic input]
_decimal.Decimal.as_tuple

    cls: defining_class

Return a tuple representation of the number.
[clinic start generated code]*/

static PyObject *
_decimal_Decimal_as_tuple_impl(PyObject *self, PyTypeObject *cls)
/*[clinic end generated code: output=d68b967becee8ab9 input=bfa86d640224d9f5]*/
{
    PyObject *result = NULL;
    PyObject *sign = NULL;
    PyObject *coeff = NULL;
    PyObject *expt = NULL;
    PyObject *tmp = NULL;
    mpd_t *x = NULL;
    char *intstring = NULL;
    Py_ssize_t intlen, i;


    x = mpd_qncopy(MPD(self));
    if (x == NULL) {
        PyErr_NoMemory();
        goto out;
    }

    sign = PyLong_FromUnsignedLong(mpd_sign(MPD(self)));
    if (sign == NULL) {
        goto out;
    }

    if (mpd_isinfinite(x)) {
        expt = PyUnicode_FromString("F");
        if (expt == NULL) {
            goto out;
        }
        /* decimal.py has non-compliant infinity payloads. */
        coeff = Py_BuildValue("(i)", 0);
        if (coeff == NULL) {
            goto out;
        }
    }
    else {
        if (mpd_isnan(x)) {
            expt = PyUnicode_FromString(mpd_isqnan(x)?"n":"N");
        }
        else {
            expt = PyLong_FromSsize_t(MPD(self)->exp);
        }
        if (expt == NULL) {
            goto out;
        }

        /* coefficient is defined */
        if (x->len > 0) {

            /* make an integer */
            x->exp = 0;
            /* clear NaN and sign */
            mpd_clear_flags(x);
            intstring = mpd_to_sci(x, 1);
            if (intstring == NULL) {
                PyErr_NoMemory();
                goto out;
            }

            intlen = strlen(intstring);
            coeff = PyTuple_New(intlen);
            if (coeff == NULL) {
                goto out;
            }

            for (i = 0; i < intlen; i++) {
                tmp = PyLong_FromLong(intstring[i]-'0');
                if (tmp == NULL) {
                    goto out;
                }
                PyTuple_SET_ITEM(coeff, i, tmp);
            }
        }
        else {
            coeff = PyTuple_New(0);
            if (coeff == NULL) {
                goto out;
            }
        }
    }

    decimal_state *state = PyType_GetModuleState(cls);
    result = PyObject_CallFunctionObjArgs((PyObject *)state->DecimalTuple,
                                          sign, coeff, expt, NULL);

out:
    if (x) mpd_del(x);
    if (intstring) mpd_free(intstring);
    Py_XDECREF(sign);
    Py_XDECREF(coeff);
    Py_XDECREF(expt);
    return result;
}


/******************************************************************************/
/*         Macros for converting mpdecimal functions to Decimal methods       */
/******************************************************************************/

/* Unary number method that uses the default module context. */
#define Dec_UnaryNumberMethod(MPDFUNC) \
static PyObject *                                           \
nm_##MPDFUNC(PyObject *self)                                \
{                                                           \
    PyObject *result;                                       \
    PyObject *context;                                      \
    uint32_t status = 0;                                    \
                                                            \
    decimal_state *state = get_module_state_by_def(Py_TYPE(self));   \
    CURRENT_CONTEXT(state, context);                        \
    if ((result = dec_alloc(state)) == NULL) {              \
        return NULL;                                        \
    }                                                       \
                                                            \
    MPDFUNC(MPD(result), MPD(self), CTX(context), &status); \
    if (dec_addstatus(context, status)) {                   \
        Py_DECREF(result);                                  \
        return NULL;                                        \
    }                                                       \
                                                            \
    return result;                                          \
}

/* Binary number method that uses default module context. */
#define Dec_BinaryNumberMethod(MPDFUNC) \
static PyObject *                                                \
nm_##MPDFUNC(PyObject *self, PyObject *other)                    \
{                                                                \
    PyObject *a, *b;                                             \
    PyObject *result;                                            \
    PyObject *context;                                           \
    uint32_t status = 0;                                         \
                                                                 \
    decimal_state *state = find_state_left_or_right(self, other); \
    CURRENT_CONTEXT(state, context) ;                            \
    CONVERT_BINOP(&a, &b, self, other, context);                 \
                                                                 \
    if ((result = dec_alloc(state)) == NULL) {                   \
        Py_DECREF(a);                                            \
        Py_DECREF(b);                                            \
        return NULL;                                             \
    }                                                            \
                                                                 \
    MPDFUNC(MPD(result), MPD(a), MPD(b), CTX(context), &status); \
    Py_DECREF(a);                                                \
    Py_DECREF(b);                                                \
    if (dec_addstatus(context, status)) {                        \
        Py_DECREF(result);                                       \
        return NULL;                                             \
    }                                                            \
                                                                 \
    return result;                                               \
}

/* Boolean function without a context arg.
   Argument Clinic provides PyObject *self
*/
#define Dec_BoolFunc(MPDFUNC) \
{                                                           \
    return MPDFUNC(MPD(self)) ? incr_true() : incr_false(); \
}

/* Boolean function with an optional context arg.
   Argument Clinic provides PyObject *self, PyTypeObject *cls,
                            PyObject *context
*/
#define Dec_BoolFuncVA(MPDFUNC) \
{                                                                         \
    decimal_state *state = PyType_GetModuleState(cls);                    \
    CONTEXT_CHECK_VA(state, context);                                     \
                                                                          \
    return MPDFUNC(MPD(self), CTX(context)) ? incr_true() : incr_false(); \
}

/* Unary function with an optional context arg.
   Argument Clinic provides PyObject *self, PyTypeObject *cls,
                            PyObject *context
*/
#define Dec_UnaryFuncVA(MPDFUNC) \
{                                                              \
    PyObject *result;                                          \
    uint32_t status = 0;                                       \
    decimal_state *state = PyType_GetModuleState(cls);         \
    CONTEXT_CHECK_VA(state, context);                          \
                                                               \
    if ((result = dec_alloc(state)) == NULL) {                 \
        return NULL;                                           \
    }                                                          \
                                                               \
    MPDFUNC(MPD(result), MPD(self), CTX(context), &status);    \
    if (dec_addstatus(context, status)) {                      \
        Py_DECREF(result);                                     \
        return NULL;                                           \
    }                                                          \
                                                               \
    return result;                                             \
}

/* Binary function with an optional context arg.
   Argument Clinic provides PyObject *self, PyTypeObject *cls,
                            PyObject *other, PyObject *context
*/
#define Dec_BinaryFuncVA(MPDFUNC) \
{                                                                \
    PyObject *a, *b;                                             \
    PyObject *result;                                            \
    uint32_t status = 0;                                         \
    decimal_state *state = PyType_GetModuleState(cls);           \
    CONTEXT_CHECK_VA(state, context);                            \
    CONVERT_BINOP_RAISE(&a, &b, self, other, context);           \
                                                                 \
    if ((result = dec_alloc(state)) == NULL) {                   \
        Py_DECREF(a);                                            \
        Py_DECREF(b);                                            \
        return NULL;                                             \
    }                                                            \
                                                                 \
    MPDFUNC(MPD(result), MPD(a), MPD(b), CTX(context), &status); \
    Py_DECREF(a);                                                \
    Py_DECREF(b);                                                \
    if (dec_addstatus(context, status)) {                        \
        Py_DECREF(result);                                       \
        return NULL;                                             \
    }                                                            \
                                                                 \
    return result;                                               \
}

/* Binary function with an optional context arg. Actual MPDFUNC does
   NOT take a context. The context is used to record InvalidOperation
   if the second operand cannot be converted exactly.

   Argument Clinic provides PyObject *self, PyTypeObject *cls,
                            PyObject *other, PyObject *context
*/
#define Dec_BinaryFuncVA_NO_CTX(MPDFUNC) \
{                                                               \
    PyObject *a, *b;                                            \
    PyObject *result;                                           \
    decimal_state *state = PyType_GetModuleState(cls);          \
    CONTEXT_CHECK_VA(state, context);                           \
    CONVERT_BINOP_RAISE(&a, &b, self, other, context);          \
                                                                \
    if ((result = dec_alloc(state)) == NULL) {                  \
        Py_DECREF(a);                                           \
        Py_DECREF(b);                                           \
        return NULL;                                            \
    }                                                           \
                                                                \
    MPDFUNC(MPD(result), MPD(a), MPD(b));                       \
    Py_DECREF(a);                                               \
    Py_DECREF(b);                                               \
                                                                \
    return result;                                              \
}

/* Ternary function with an optional context arg.
   Argument Clinic provides PyObject *self, PyTypeObject *cls,
                            PyObject *other, PyObject *third,
                            PyObject *context
*/
#define Dec_TernaryFuncVA(MPDFUNC) \
{                                                                        \
    PyObject *a, *b, *c;                                                 \
    PyObject *result;                                                    \
    uint32_t status = 0;                                                 \
    decimal_state *state = PyType_GetModuleState(cls);                   \
    CONTEXT_CHECK_VA(state, context);                                    \
    CONVERT_TERNOP_RAISE(&a, &b, &c, self, other, third, context);       \
                                                                         \
    if ((result = dec_alloc(state)) == NULL) {                           \
        Py_DECREF(a);                                                    \
        Py_DECREF(b);                                                    \
        Py_DECREF(c);                                                    \
        return NULL;                                                     \
    }                                                                    \
                                                                         \
    MPDFUNC(MPD(result), MPD(a), MPD(b), MPD(c), CTX(context), &status); \
    Py_DECREF(a);                                                        \
    Py_DECREF(b);                                                        \
    Py_DECREF(c);                                                        \
    if (dec_addstatus(context, status)) {                                \
        Py_DECREF(result);                                               \
        return NULL;                                                     \
    }                                                                    \
                                                                         \
    return result;                                                       \
}


/**********************************************/
/*              Number methods                */
/**********************************************/

Dec_UnaryNumberMethod(mpd_qminus)
Dec_UnaryNumberMethod(mpd_qplus)
Dec_UnaryNumberMethod(mpd_qabs)

Dec_BinaryNumberMethod(mpd_qadd)
Dec_BinaryNumberMethod(mpd_qsub)
Dec_BinaryNumberMethod(mpd_qmul)
Dec_BinaryNumberMethod(mpd_qdiv)
Dec_BinaryNumberMethod(mpd_qrem)
Dec_BinaryNumberMethod(mpd_qdivint)

static PyObject *
nm_dec_as_long(PyObject *dec)
{
    PyObject *context;
    decimal_state *state = get_module_state_by_def(Py_TYPE(dec));
    CURRENT_CONTEXT(state, context);
    return dec_as_long(dec, context, MPD_ROUND_DOWN);
}

static int
nm_nonzero(PyObject *v)
{
    return !mpd_iszero(MPD(v));
}

static PyObject *
nm_mpd_qdivmod(PyObject *v, PyObject *w)
{
    PyObject *a, *b;
    PyObject *q, *r;
    PyObject *context;
    uint32_t status = 0;
    PyObject *ret;

    decimal_state *state = find_state_left_or_right(v, w);
    CURRENT_CONTEXT(state, context);
    CONVERT_BINOP(&a, &b, v, w, context);

    q = dec_alloc(state);
    if (q == NULL) {
        Py_DECREF(a);
        Py_DECREF(b);
        return NULL;
    }
    r = dec_alloc(state);
    if (r == NULL) {
        Py_DECREF(a);
        Py_DECREF(b);
        Py_DECREF(q);
        return NULL;
    }

    mpd_qdivmod(MPD(q), MPD(r), MPD(a), MPD(b), CTX(context), &status);
    Py_DECREF(a);
    Py_DECREF(b);
    if (dec_addstatus(context, status)) {
        Py_DECREF(r);
        Py_DECREF(q);
        return NULL;
    }

    ret = PyTuple_Pack(2, q, r);
    Py_DECREF(r);
    Py_DECREF(q);
    return ret;
}

static PyObject *
nm_mpd_qpow(PyObject *base, PyObject *exp, PyObject *mod)
{
    PyObject *a, *b, *c = NULL;
    PyObject *result;
    PyObject *context;
    uint32_t status = 0;

    decimal_state *state = find_state_ternary(base, exp, mod);
    CURRENT_CONTEXT(state, context);
    CONVERT_BINOP(&a, &b, base, exp, context);

    if (mod != Py_None) {
        if (!convert_op(NOT_IMPL, &c, mod, context)) {
            Py_DECREF(a);
            Py_DECREF(b);
            return c;
        }
    }

    result = dec_alloc(state);
    if (result == NULL) {
        Py_DECREF(a);
        Py_DECREF(b);
        Py_XDECREF(c);
        return NULL;
    }

    if (c == NULL) {
        mpd_qpow(MPD(result), MPD(a), MPD(b),
                 CTX(context), &status);
    }
    else {
        mpd_qpowmod(MPD(result), MPD(a), MPD(b), MPD(c),
                    CTX(context), &status);
        Py_DECREF(c);
    }
    Py_DECREF(a);
    Py_DECREF(b);
    if (dec_addstatus(context, status)) {
        Py_DECREF(result);
        return NULL;
    }

    return result;
}


/******************************************************************************/
/*                             Decimal Methods                                */
/******************************************************************************/

/* Unary arithmetic functions, optional context arg */

/*[clinic input]
_decimal.Decimal.exp

    cls: defining_class
    context: object = None

Return the value of the (natural) exponential function e**x.

The function always uses the ROUND_HALF_EVEN mode and the result is
correctly rounded.
[clinic start generated code]*/

static PyObject *
_decimal_Decimal_exp_impl(PyObject *self, PyTypeObject *cls,
                          PyObject *context)
/*[clinic end generated code: output=40317012aedbaeac input=84919aad3dabda08]*/
Dec_UnaryFuncVA(mpd_qexp)

/*[clinic input]
_decimal.Decimal.ln = _decimal.Decimal.exp

Return the natural (base e) logarithm of the operand.

The function always uses the ROUND_HALF_EVEN mode and the result is
correctly rounded.
[clinic start generated code]*/

static PyObject *
_decimal_Decimal_ln_impl(PyObject *self, PyTypeObject *cls,
                         PyObject *context)
/*[clinic end generated code: output=e8f9e81cac38e5dc input=d353c51ec00d1cff]*/
Dec_UnaryFuncVA(mpd_qln)

/*[clinic input]
_decimal.Decimal.log10 = _decimal.Decimal.exp

Return the base ten logarithm of the operand.

The function always uses the ROUND_HALF_EVEN mode and the result is
correctly rounded.
[clinic start generated code]*/

static PyObject *
_decimal_Decimal_log10_impl(PyObject *self, PyTypeObject *cls,
                            PyObject *context)
/*[clinic end generated code: output=00b3255648135c95 input=48a6be60154c0b46]*/
Dec_UnaryFuncVA(mpd_qlog10)

/*[clinic input]
_decimal.Decimal.next_minus = _decimal.Decimal.exp

Returns the largest representable number smaller than itself.
[clinic start generated code]*/

static PyObject *
_decimal_Decimal_next_minus_impl(PyObject *self, PyTypeObject *cls,
                                 PyObject *context)
/*[clinic end generated code: output=a187a55e6976b572 input=666b348f71e6c090]*/
Dec_UnaryFuncVA(mpd_qnext_minus)

/*[clinic input]
_decimal.Decimal.next_plus = _decimal.Decimal.exp

Returns the smallest representable number larger than itself.
[clinic start generated code]*/

static PyObject *
_decimal_Decimal_next_plus_impl(PyObject *self, PyTypeObject *cls,
                                PyObject *context)
/*[clinic end generated code: output=13737d41714e320e input=04e105060ad1fa15]*/
Dec_UnaryFuncVA(mpd_qnext_plus)

/*[clinic input]
_decimal.Decimal.normalize = _decimal.Decimal.exp

Normalize the number by stripping trailing 0s

This also change anything equal to 0 to 0e0.  Used for producing
canonical values for members of an equivalence class.  For example,
Decimal('32.100') and Decimal('0.321000e+2') both normalize to
the equivalent value Decimal('32.1').
[clinic start generated code]*/

static PyObject *
_decimal_Decimal_normalize_impl(PyObject *self, PyTypeObject *cls,
                                PyObject *context)
/*[clinic end generated code: output=32c4c0d13fe33fb9 input=d5ee63acd904d4de]*/
Dec_UnaryFuncVA(mpd_qreduce)

/*[clinic input]
_decimal.Decimal.sqrt = _decimal.Decimal.exp

Return the square root of the argument to full precision.

The result is correctly rounded using the ROUND_HALF_EVEN rounding mode.
[clinic start generated code]*/

static PyObject *
_decimal_Decimal_sqrt_impl(PyObject *self, PyTypeObject *cls,
                           PyObject *context)
/*[clinic end generated code: output=deb1280077b5e586 input=3a76afbd39dc20b9]*/
Dec_UnaryFuncVA(mpd_qsqrt)

/* Binary arithmetic functions, optional context arg */

/*[clinic input]
_decimal.Decimal.compare

    cls: defining_class
    other: object
    context: object = None

Compare self to other.

Return a decimal value:

    a or b is a NaN ==> Decimal('NaN')
    a < b           ==> Decimal('-1')
    a == b          ==> Decimal('0')
    a > b           ==> Decimal('1')
[clinic start generated code]*/

static PyObject *
_decimal_Decimal_compare_impl(PyObject *self, PyTypeObject *cls,
                              PyObject *other, PyObject *context)
/*[clinic end generated code: output=a4a1d383ec192cfa input=d18a02bb8083e92a]*/
Dec_BinaryFuncVA(mpd_qcompare)

/*[clinic input]
_decimal.Decimal.compare_signal = _decimal.Decimal.compare

Identical to compare, except that all NaNs signal.
[clinic start generated code]*/

static PyObject *
_decimal_Decimal_compare_signal_impl(PyObject *self, PyTypeObject *cls,
                                     PyObject *other, PyObject *context)
/*[clinic end generated code: output=22f757371fd4167b input=a52a39d1c6fc369d]*/
Dec_BinaryFuncVA(mpd_qcompare_signal)

/*[clinic input]
_decimal.Decimal.max = _decimal.Decimal.compare

Maximum of self and other.

If one operand is a quiet NaN and the other is numeric, the numeric
operand is returned.
[clinic start generated code]*/

static PyObject *
_decimal_Decimal_max_impl(PyObject *self, PyTypeObject *cls, PyObject *other,
                          PyObject *context)
/*[clinic end generated code: output=d3d12db9815869e5 input=2ae2582f551296d8]*/
Dec_BinaryFuncVA(mpd_qmax)

/*[clinic input]
_decimal.Decimal.max_mag = _decimal.Decimal.compare

As the max() method, but compares the absolute values of the operands.
[clinic start generated code]*/

static PyObject *
_decimal_Decimal_max_mag_impl(PyObject *self, PyTypeObject *cls,
                              PyObject *other, PyObject *context)
/*[clinic end generated code: output=f71f2c27d9bc7cac input=88b105e66cf138c5]*/
Dec_BinaryFuncVA(mpd_qmax_mag)

/*[clinic input]
_decimal.Decimal.min = _decimal.Decimal.compare

Minimum of self and other.

If one operand is a quiet NaN and the other is numeric, the numeric
operand is returned.
[clinic start generated code]*/

static PyObject *
_decimal_Decimal_min_impl(PyObject *self, PyTypeObject *cls, PyObject *other,
                          PyObject *context)
/*[clinic end generated code: output=c5620344ae5f3dd1 input=2a70f2c087c418c9]*/
Dec_BinaryFuncVA(mpd_qmin)

/*[clinic input]
_decimal.Decimal.min_mag = _decimal.Decimal.compare

As the min() method, but compares the absolute values of the operands.
[clinic start generated code]*/

static PyObject *
_decimal_Decimal_min_mag_impl(PyObject *self, PyTypeObject *cls,
                              PyObject *other, PyObject *context)
/*[clinic end generated code: output=018562ad1c22aae3 input=351fa3c0e592746a]*/
Dec_BinaryFuncVA(mpd_qmin_mag)

/*[clinic input]
_decimal.Decimal.next_toward = _decimal.Decimal.compare

Returns the number closest to self, in the direction towards other.

If the two operands are unequal, return the number closest to the first
operand in the direction of the second operand.  If both operands are
numerically equal, return a copy of the first operand with the sign set
to be the same as the sign of the second operand.
[clinic start generated code]*/

static PyObject *
_decimal_Decimal_next_toward_impl(PyObject *self, PyTypeObject *cls,
                                  PyObject *other, PyObject *context)
/*[clinic end generated code: output=71d879bca8bc1019 input=fdf0091ea6e9e416]*/
Dec_BinaryFuncVA(mpd_qnext_toward)

/*[clinic input]
_decimal.Decimal.remainder_near = _decimal.Decimal.compare

Return the remainder from dividing self by other.

This differs from self % other in that the sign of the remainder is
chosen so as to minimize its absolute value. More precisely, the return
value is self - n * other where n is the integer nearest to the exact
value of self / other, and if two integers are equally near then the
even one is chosen.

If the result is zero then its sign will be the sign of self.
[clinic start generated code]*/

static PyObject *
_decimal_Decimal_remainder_near_impl(PyObject *self, PyTypeObject *cls,
                                     PyObject *other, PyObject *context)
/*[clinic end generated code: output=d3fbb4985f2077fa input=eb5a8dfe3470b794]*/
Dec_BinaryFuncVA(mpd_qrem_near)

/* Ternary arithmetic functions, optional context arg */

/*[clinic input]
_decimal.Decimal.fma

    cls: defining_class
    other: object
    third: object
    context: object = None

Fused multiply-add.

Return self*other+third with no rounding of the intermediate product
self*other.

    >>> Decimal(2).fma(3, 5)
    Decimal('11')
[clinic start generated code]*/

static PyObject *
_decimal_Decimal_fma_impl(PyObject *self, PyTypeObject *cls, PyObject *other,
                          PyObject *third, PyObject *context)
/*[clinic end generated code: output=db49a777e85b71e4 input=2104c001f6077c35]*/
Dec_TernaryFuncVA(mpd_qfma)

/* Boolean functions, no context arg */

/*[clinic input]
_decimal.Decimal.is_canonical

Return True if the argument is canonical and False otherwise.

Currently, a Decimal instance is always canonical, so this operation
always returns True.
[clinic start generated code]*/

static PyObject *
_decimal_Decimal_is_canonical_impl(PyObject *self)
/*[clinic end generated code: output=b29668684f45443e input=b3b3e6878ccf40b8]*/
Dec_BoolFunc(mpd_iscanonical)

/*[clinic input]
_decimal.Decimal.is_finite

Return True if the argument is a finite number, and False otherwise.
[clinic start generated code]*/

static PyObject *
_decimal_Decimal_is_finite_impl(PyObject *self)
/*[clinic end generated code: output=537306fbfc9131f8 input=e9b8b5866704bae6]*/
Dec_BoolFunc(mpd_isfinite)

/*[clinic input]
_decimal.Decimal.is_infinite

Return True if the argument is infinite, and False otherwise.
[clinic start generated code]*/

static PyObject *
_decimal_Decimal_is_infinite_impl(PyObject *self)
/*[clinic end generated code: output=31b775ff28f05ce2 input=8f3937a790ee4ec2]*/
Dec_BoolFunc(mpd_isinfinite)

/*[clinic input]
_decimal.Decimal.is_nan

Return True if the argument is a (quiet or signaling) NaN, else False.
[clinic start generated code]*/

static PyObject *
_decimal_Decimal_is_nan_impl(PyObject *self)
/*[clinic end generated code: output=b704e8b49a164388 input=795e5dac85976994]*/
Dec_BoolFunc(mpd_isnan)

/*[clinic input]
_decimal.Decimal.is_qnan

Return True if the argument is a quiet NaN, and False otherwise.
[clinic start generated code]*/

static PyObject *
_decimal_Decimal_is_qnan_impl(PyObject *self)
/*[clinic end generated code: output=85b5241f43798376 input=00485f3c3cfae0af]*/
Dec_BoolFunc(mpd_isqnan)

/*[clinic input]
_decimal.Decimal.is_snan

Return True if the argument is a signaling NaN and False otherwise.
[clinic start generated code]*/

static PyObject *
_decimal_Decimal_is_snan_impl(PyObject *self)
/*[clinic end generated code: output=50de9ec6507e4a4f input=f3b0f8592c921879]*/
Dec_BoolFunc(mpd_issnan)

/*[clinic input]
_decimal.Decimal.is_signed

Return True if the argument has a negative sign and False otherwise.

Note that both zeros and NaNs can carry signs.
[clinic start generated code]*/

static PyObject *
_decimal_Decimal_is_signed_impl(PyObject *self)
/*[clinic end generated code: output=8ec7bc85d8e755e4 input=97c3437ab5dffecc]*/
Dec_BoolFunc(mpd_issigned)

/*[clinic input]
_decimal.Decimal.is_zero

Return True if the argument is a zero and False otherwise.
[clinic start generated code]*/

static PyObject *
_decimal_Decimal_is_zero_impl(PyObject *self)
/*[clinic end generated code: output=2d87ea1b15879112 input=ae616674cd050a51]*/
Dec_BoolFunc(mpd_iszero)

/* Boolean functions, optional context arg */

/*[clinic input]
_decimal.Decimal.is_normal = _decimal.Decimal.exp

Return True if the argument is a normal number and False otherwise.

Normal number is a finite nonzero number, which is not subnormal.
[clinic start generated code]*/

static PyObject *
_decimal_Decimal_is_normal_impl(PyObject *self, PyTypeObject *cls,
                                PyObject *context)
/*[clinic end generated code: output=92a3878e293758d4 input=9afe43b9db9f4818]*/
Dec_BoolFuncVA(mpd_isnormal)

/*[clinic input]
_decimal.Decimal.is_subnormal = _decimal.Decimal.exp

Return True if the argument is subnormal, and False otherwise.

A number is subnormal if it is non-zero, finite, and has an adjusted
exponent less than Emin.
[clinic start generated code]*/

static PyObject *
_decimal_Decimal_is_subnormal_impl(PyObject *self, PyTypeObject *cls,
                                   PyObject *context)
/*[clinic end generated code: output=1404c04d980ebc07 input=11839c122c185b8b]*/
Dec_BoolFuncVA(mpd_issubnormal)

/* Unary functions, no context arg */

/*[clinic input]
_decimal.Decimal.adjusted

Return the adjusted exponent (exp + digits - 1) of the number.
[clinic start generated code]*/

static PyObject *
_decimal_Decimal_adjusted_impl(PyObject *self)
/*[clinic end generated code: output=21ea2c9f23994c52 input=8ba2029d8d906b18]*/
{
    mpd_ssize_t retval;

    if (mpd_isspecial(MPD(self))) {
        retval = 0;
    }
    else {
        retval = mpd_adjexp(MPD(self));
    }

    return PyLong_FromSsize_t(retval);
}

/*[clinic input]
_decimal.Decimal.canonical

Return the canonical encoding of the argument.

Currently, the encoding of a Decimal instance is always canonical,
so this operation returns its argument unchanged.
[clinic start generated code]*/

static PyObject *
_decimal_Decimal_canonical_impl(PyObject *self)
/*[clinic end generated code: output=3cbeb47d91e6da2d input=8a4719d14c52d521]*/
{
    return Py_NewRef(self);
}

/*[clinic input]
_decimal.Decimal.conjugate

Return self.
[clinic start generated code]*/

static PyObject *
_decimal_Decimal_conjugate_impl(PyObject *self)
/*[clinic end generated code: output=9a37bf633f25a291 input=c7179975ef74fd84]*/
{
    return Py_NewRef(self);
}

static inline PyObject *
_dec_mpd_radix(decimal_state *state)
{
    PyObject *result;

    result = dec_alloc(state);
    if (result == NULL) {
        return NULL;
    }

    _dec_settriple(result, MPD_POS, 10, 0);
    return result;
}

/*[clinic input]
_decimal.Decimal.radix

    cls: defining_class

Return Decimal(10).

This is the radix (base) in which the Decimal class does
all its arithmetic. Included for compatibility with the specification.
[clinic start generated code]*/

static PyObject *
_decimal_Decimal_radix_impl(PyObject *self, PyTypeObject *cls)
/*[clinic end generated code: output=40a3bc7ec3d99228 input=b0d4cb9f870bbac1]*/
{
    decimal_state *state = PyType_GetModuleState(cls);
    return _dec_mpd_radix(state);
}

/*[clinic input]
_decimal.Decimal.copy_abs

    cls: defining_class

Return the absolute value of the argument.

This operation is unaffected by context and is quiet: no flags are
changed and no rounding is performed.
[clinic start generated code]*/

static PyObject *
_decimal_Decimal_copy_abs_impl(PyObject *self, PyTypeObject *cls)
/*[clinic end generated code: output=081cb7fb4230676e input=676d7c62b1795512]*/
{
    PyObject *result;
    uint32_t status = 0;
    decimal_state *state = PyType_GetModuleState(cls);

    if ((result = dec_alloc(state)) == NULL) {
        return NULL;
    }

    mpd_qcopy_abs(MPD(result), MPD(self), &status);
    if (status & MPD_Malloc_error) {
        Py_DECREF(result);
        PyErr_NoMemory();
        return NULL;
    }

    return result;
}

/*[clinic input]
_decimal.Decimal.copy_negate = _decimal.Decimal.copy_abs

Return the negation of the argument.

This operation is unaffected by context and is quiet: no flags are
changed and no rounding is performed.
[clinic start generated code]*/

static PyObject *
_decimal_Decimal_copy_negate_impl(PyObject *self, PyTypeObject *cls)
/*[clinic end generated code: output=04fed82c17d4e28b input=23f41ee8899f3891]*/
{
    PyObject *result;
    uint32_t status = 0;
    decimal_state *state = PyType_GetModuleState(cls);

    if ((result = dec_alloc(state)) == NULL) {
        return NULL;
    }

    mpd_qcopy_negate(MPD(result), MPD(self), &status);
    if (status & MPD_Malloc_error) {
        Py_DECREF(result);
        PyErr_NoMemory();
        return NULL;
    }

    return result;
}

/* Unary functions, optional context arg */

/*[clinic input]
_decimal.Decimal.logical_invert = _decimal.Decimal.exp

Return the digit-wise inversion of the (logical) operand.
[clinic start generated code]*/

static PyObject *
_decimal_Decimal_logical_invert_impl(PyObject *self, PyTypeObject *cls,
                                     PyObject *context)
/*[clinic end generated code: output=c626ed4b104a97b7 input=3531dac8b9548dad]*/
Dec_UnaryFuncVA(mpd_qinvert)

/*[clinic input]
_decimal.Decimal.logb = _decimal.Decimal.exp

Return the adjusted exponent of the operand as a Decimal instance.

If the operand is a zero, then Decimal('-Infinity') is returned and the
DivisionByZero condition is raised. If the operand is an infinity then
Decimal('Infinity') is returned.
[clinic start generated code]*/

static PyObject *
_decimal_Decimal_logb_impl(PyObject *self, PyTypeObject *cls,
                           PyObject *context)
/*[clinic end generated code: output=36b0bda09e934245 input=a8df027d1b8a2b17]*/
Dec_UnaryFuncVA(mpd_qlogb)

/*[clinic input]
_decimal.Decimal.number_class = _decimal.Decimal.exp

Return a string describing the class of the operand.

The returned value is one of the following ten strings:

    * '-Infinity', indicating that the operand is negative infinity.
    * '-Normal', indicating that the operand is a negative normal
      number.
    * '-Subnormal', indicating that the operand is negative and
      subnormal.
    * '-Zero', indicating that the operand is a negative zero.
    * '+Zero', indicating that the operand is a positive zero.
    * '+Subnormal', indicating that the operand is positive and
      subnormal.
    * '+Normal', indicating that the operand is a positive normal
      number.
    * '+Infinity', indicating that the operand is positive infinity.
    * 'NaN', indicating that the operand is a quiet NaN (Not a Number).
    * 'sNaN', indicating that the operand is a signaling NaN.
[clinic start generated code]*/

static PyObject *
_decimal_Decimal_number_class_impl(PyObject *self, PyTypeObject *cls,
                                   PyObject *context)
/*[clinic end generated code: output=1ac82412e0849c52 input=447095d2677fa0ca]*/
{
    const char *cp;

    decimal_state *state = PyType_GetModuleState(cls);
    CONTEXT_CHECK_VA(state, context);

    cp = mpd_class(MPD(self), CTX(context));
    return PyUnicode_FromString(cp);
}

/*[clinic input]
_decimal.Decimal.to_eng_string = _decimal.Decimal.exp

Convert to an engineering-type string.

Engineering notation has an exponent which is a multiple of 3, so there
are up to 3 digits left of the decimal place. For example,
Decimal('123E+1') is converted to Decimal('1.23E+3').

The value of context.capitals determines whether the exponent sign is
lower or upper case. Otherwise, the context does not affect the
operation.
[clinic start generated code]*/

static PyObject *
_decimal_Decimal_to_eng_string_impl(PyObject *self, PyTypeObject *cls,
                                    PyObject *context)
/*[clinic end generated code: output=901f128d437ae5c0 input=b2cb7e01e268e45d]*/
{
    PyObject *result;
    mpd_ssize_t size;
    char *s;

    decimal_state *state = PyType_GetModuleState(cls);
    CONTEXT_CHECK_VA(state, context);

    size = mpd_to_eng_size(&s, MPD(self), CtxCaps(context));
    if (size < 0) {
        PyErr_NoMemory();
        return NULL;
    }

    result = unicode_fromascii(s, size);
    mpd_free(s);

    return result;
}

/* Binary functions, optional context arg for conversion errors */

/*[clinic input]
_decimal.Decimal.compare_total = _decimal.Decimal.compare

Compare two operands using their abstract representation.

Similar to the compare() method, but the result
gives a total ordering on Decimal instances.  Two Decimal instances with
the same numeric value but different representations compare unequal
in this ordering:

    >>> Decimal('12.0').compare_total(Decimal('12'))
    Decimal('-1')

Quiet and signaling NaNs are also included in the total ordering. The
result of this function is Decimal('0') if both operands have the same
representation, Decimal('-1') if the first operand is lower in the
total order than the second, and Decimal('1') if the first operand is
higher in the total order than the second operand. See the
specification for details of the total order.

This operation is unaffected by context and is quiet: no flags are
changed and no rounding is performed. As an exception, the C version
may raise InvalidOperation if the second operand cannot be converted
exactly.
[clinic start generated code]*/

static PyObject *
_decimal_Decimal_compare_total_impl(PyObject *self, PyTypeObject *cls,
                                    PyObject *other, PyObject *context)
/*[clinic end generated code: output=83649010bad7815f input=6f3111ec5fdbf3c1]*/
Dec_BinaryFuncVA_NO_CTX(mpd_compare_total)

/*[clinic input]
_decimal.Decimal.compare_total_mag = _decimal.Decimal.compare

As compare_total(), but ignores the sign of each operand.

x.compare_total_mag(y) is equivalent to
x.copy_abs().compare_total(y.copy_abs()).

This operation is unaffected by context and is quiet: no flags are
changed and no rounding is performed. As an exception, the C version
may raise InvalidOperation if the second operand cannot be converted
exactly.
[clinic start generated code]*/

static PyObject *
_decimal_Decimal_compare_total_mag_impl(PyObject *self, PyTypeObject *cls,
                                        PyObject *other, PyObject *context)
/*[clinic end generated code: output=b99c924cafb5f0e3 input=eba17c4c24eb2833]*/
Dec_BinaryFuncVA_NO_CTX(mpd_compare_total_mag)

/*[clinic input]
_decimal.Decimal.copy_sign = _decimal.Decimal.compare

Return a copy of *self* with the sign of *other*.

For example:

    >>> Decimal('2.3').copy_sign(Decimal('-1.5'))
    Decimal('-2.3')

This operation is unaffected by context and is quiet: no flags are
changed and no rounding is performed. As an exception, the C version
may raise InvalidOperation if the second operand cannot be converted
exactly.
[clinic start generated code]*/

static PyObject *
_decimal_Decimal_copy_sign_impl(PyObject *self, PyTypeObject *cls,
                                PyObject *other, PyObject *context)
/*[clinic end generated code: output=e4c8f884f4d75801 input=51ed9e4691e2249e]*/
{
    PyObject *a, *b;
    PyObject *result;
    uint32_t status = 0;

    decimal_state *state = PyType_GetModuleState(cls);
    CONTEXT_CHECK_VA(state, context);
    CONVERT_BINOP_RAISE(&a, &b, self, other, context);

    result = dec_alloc(state);
    if (result == NULL) {
        Py_DECREF(a);
        Py_DECREF(b);
        return NULL;
    }

    mpd_qcopy_sign(MPD(result), MPD(a), MPD(b), &status);
    Py_DECREF(a);
    Py_DECREF(b);
    if (dec_addstatus(context, status)) {
        Py_DECREF(result);
        return NULL;
    }

    return result;
}

/*[clinic input]
_decimal.Decimal.same_quantum = _decimal.Decimal.compare

Test whether self and other have the same exponent or both are NaN.

This operation is unaffected by context and is quiet: no flags are
changed and no rounding is performed. As an exception, the C version
may raise InvalidOperation if the second operand cannot be converted
exactly.
[clinic start generated code]*/

static PyObject *
_decimal_Decimal_same_quantum_impl(PyObject *self, PyTypeObject *cls,
                                   PyObject *other, PyObject *context)
/*[clinic end generated code: output=7c757edb0c263721 input=8339415fa359e7df]*/
{
    PyObject *a, *b;
    PyObject *result;

    decimal_state *state = PyType_GetModuleState(cls);
    CONTEXT_CHECK_VA(state, context);
    CONVERT_BINOP_RAISE(&a, &b, self, other, context);

    result = mpd_same_quantum(MPD(a), MPD(b)) ? incr_true() : incr_false();
    Py_DECREF(a);
    Py_DECREF(b);

    return result;
}

/* Binary functions, optional context arg */

/*[clinic input]
_decimal.Decimal.logical_and = _decimal.Decimal.compare

Return the digit-wise 'and' of the two (logical) operands.
[clinic start generated code]*/

static PyObject *
_decimal_Decimal_logical_and_impl(PyObject *self, PyTypeObject *cls,
                                  PyObject *other, PyObject *context)
/*[clinic end generated code: output=9a4cbb74c180b0bb input=2b319baee8970929]*/
Dec_BinaryFuncVA(mpd_qand)

/*[clinic input]
_decimal.Decimal.logical_or = _decimal.Decimal.compare

Return the digit-wise 'or' of the two (logical) operands.
[clinic start generated code]*/

static PyObject *
_decimal_Decimal_logical_or_impl(PyObject *self, PyTypeObject *cls,
                                 PyObject *other, PyObject *context)
/*[clinic end generated code: output=063c4de18dc41ecb input=75e0e1d4dd373b90]*/
Dec_BinaryFuncVA(mpd_qor)

/*[clinic input]
_decimal.Decimal.logical_xor = _decimal.Decimal.compare

Return the digit-wise 'xor' of the two (logical) operands.
[clinic start generated code]*/

static PyObject *
_decimal_Decimal_logical_xor_impl(PyObject *self, PyTypeObject *cls,
                                  PyObject *other, PyObject *context)
/*[clinic end generated code: output=829b09cb49926ad7 input=a1ed8d6ac38c1c9e]*/
Dec_BinaryFuncVA(mpd_qxor)

/*[clinic input]
_decimal.Decimal.rotate = _decimal.Decimal.compare

Returns a rotated copy of self's digits, value-of-other times.

The second operand must be an integer in the range -precision through
precision. The absolute value of the second operand gives the number of
places to rotate. If the second operand is positive then rotation is to
the left; otherwise rotation is to the right.  The coefficient of the
first operand is padded on the left with zeros to length precision if
necessary. The sign and exponent of the first operand are unchanged.
[clinic start generated code]*/

static PyObject *
_decimal_Decimal_rotate_impl(PyObject *self, PyTypeObject *cls,
                             PyObject *other, PyObject *context)
/*[clinic end generated code: output=09f2737082882b83 input=cde7b032eac43f0b]*/
Dec_BinaryFuncVA(mpd_qrotate)

/*[clinic input]
_decimal.Decimal.scaleb = _decimal.Decimal.compare

Return the first operand with the exponent adjusted the second.

Equivalently, return the first operand multiplied by 10**other. The
second operand must be an integer.
[clinic start generated code]*/

static PyObject *
_decimal_Decimal_scaleb_impl(PyObject *self, PyTypeObject *cls,
                             PyObject *other, PyObject *context)
/*[clinic end generated code: output=ae8730536c9f2d30 input=7f29f83278d05f83]*/
Dec_BinaryFuncVA(mpd_qscaleb)

/*[clinic input]
_decimal.Decimal.shift = _decimal.Decimal.compare

Returns a shifted copy of self's digits, value-of-other times.

The second operand must be an integer in the range -precision through
precision. The absolute value of the second operand gives the number
of places to shift. If the second operand is positive, then the shift
is to the left; otherwise the shift is to the right. Digits shifted
into the coefficient are zeros. The sign and exponent of the first
operand are unchanged.
[clinic start generated code]*/

static PyObject *
_decimal_Decimal_shift_impl(PyObject *self, PyTypeObject *cls,
                            PyObject *other, PyObject *context)
/*[clinic end generated code: output=82e061a0d9ecc4f5 input=501759c2522cb78e]*/
Dec_BinaryFuncVA(mpd_qshift)

/*[clinic input]
_decimal.Decimal.quantize

    cls: defining_class
    exp as w: object
    rounding: object = None
    context: object = None

Quantize *self* so its exponent is the same as that of *exp*.

Return a value equal to *self* after rounding, with the exponent
of *exp*.

    >>> Decimal('1.41421356').quantize(Decimal('1.000'))
    Decimal('1.414')

Unlike other operations, if the length of the coefficient after the
quantize operation would be greater than precision, then an
InvalidOperation is signaled.  This guarantees that, unless there
is an error condition, the quantized exponent is always equal to
that of the right-hand operand.

Also unlike other operations, quantize never signals Underflow, even
if the result is subnormal and inexact.

If the exponent of the second operand is larger than that of the first,
then rounding may be necessary. In this case, the rounding mode is
determined by the rounding argument if given, else by the given context
argument; if neither argument is given, the rounding mode of the
current thread's context is used.
[clinic start generated code]*/

static PyObject *
_decimal_Decimal_quantize_impl(PyObject *self, PyTypeObject *cls,
                               PyObject *w, PyObject *rounding,
                               PyObject *context)
/*[clinic end generated code: output=fc51edf458559913 input=1166e6311e047b74]*/
{
    PyObject *a, *b;
    PyObject *result;
    uint32_t status = 0;
    mpd_context_t workctx;

    decimal_state *state = PyType_GetModuleState(cls);
    CONTEXT_CHECK_VA(state, context);

    workctx = *CTX(context);
    if (rounding != Py_None) {
        int round = getround(state, rounding);
        if (round < 0) {
            return NULL;
        }
        if (!mpd_qsetround(&workctx, round)) {
            INTERNAL_ERROR_PTR("dec_mpd_qquantize"); /* GCOV_NOT_REACHED */
        }
    }

    CONVERT_BINOP_RAISE(&a, &b, self, w, context);

    result = dec_alloc(state);
    if (result == NULL) {
        Py_DECREF(a);
        Py_DECREF(b);
        return NULL;
    }

    mpd_qquantize(MPD(result), MPD(a), MPD(b), &workctx, &status);
    Py_DECREF(a);
    Py_DECREF(b);
    if (dec_addstatus(context, status)) {
        Py_DECREF(result);
        return NULL;
    }

    return result;
}

/* Special methods */
static PyObject *
dec_richcompare(PyObject *v, PyObject *w, int op)
{
    PyObject *a;
    PyObject *b;
    PyObject *context;
    uint32_t status = 0;
    int a_issnan, b_issnan;
    int r;
    decimal_state *state = find_state_left_or_right(v, w);

#ifdef Py_DEBUG
    assert(PyDec_Check(state, v));
#endif
    CURRENT_CONTEXT(state, context);
    CONVERT_BINOP_CMP(&a, &b, v, w, op, context);

    a_issnan = mpd_issnan(MPD(a));
    b_issnan = mpd_issnan(MPD(b));

    r = mpd_qcmp(MPD(a), MPD(b), &status);
    Py_DECREF(a);
    Py_DECREF(b);
    if (r == INT_MAX) {
        /* sNaNs or op={le,ge,lt,gt} always signal. */
        if (a_issnan || b_issnan || (op != Py_EQ && op != Py_NE)) {
            if (dec_addstatus(context, status)) {
                return NULL;
            }
        }
        /* qNaN comparison with op={eq,ne} or comparison
         * with InvalidOperation disabled. */
        return (op == Py_NE) ? incr_true() : incr_false();
    }

    switch (op) {
    case Py_EQ:
        r = (r == 0);
        break;
    case Py_NE:
        r = (r != 0);
        break;
    case Py_LE:
        r = (r <= 0);
        break;
    case Py_GE:
        r = (r >= 0);
        break;
    case Py_LT:
        r = (r == -1);
        break;
    case Py_GT:
        r = (r == 1);
        break;
    }

    return PyBool_FromLong(r);
}

/*[clinic input]
_decimal.Decimal.__ceil__

    cls: defining_class

Return the ceiling as an Integral.
[clinic start generated code]*/

static PyObject *
_decimal_Decimal___ceil___impl(PyObject *self, PyTypeObject *cls)
/*[clinic end generated code: output=d986ebf9aadbf9fe input=a8e0b87897706816]*/
{
    PyObject *context;

    decimal_state *state = PyType_GetModuleState(cls);
    CURRENT_CONTEXT(state, context);
    return dec_as_long(self, context, MPD_ROUND_CEILING);
}

/*[clinic input]
_decimal.Decimal.__complex__

Convert this value to exact type complex.
[clinic start generated code]*/

static PyObject *
_decimal_Decimal___complex___impl(PyObject *self)
/*[clinic end generated code: output=c9b5b4a9fdebc912 input=6b11c6f20af7061a]*/
{
    PyObject *f;
    double x;

    f = PyDec_AsFloat(self);
    if (f == NULL) {
        return NULL;
    }

    x = PyFloat_AsDouble(f);
    Py_DECREF(f);
    if (x == -1.0 && PyErr_Occurred()) {
        return NULL;
    }

    return PyComplex_FromDoubles(x, 0);
}

/*[clinic input]
_decimal.Decimal.__copy__

[clinic start generated code]*/

static PyObject *
_decimal_Decimal___copy___impl(PyObject *self)
/*[clinic end generated code: output=8eb3656c0250762b input=3dfd30a3e1493c01]*/
{
    return Py_NewRef(self);
}

/*[clinic input]
_decimal.Decimal.__deepcopy__

    memo: object
    /

[clinic start generated code]*/

static PyObject *
_decimal_Decimal___deepcopy__(PyObject *self, PyObject *memo)
/*[clinic end generated code: output=988fb34e0136b376 input=f95598c6f43233aa]*/
{
    return Py_NewRef(self);
}

/*[clinic input]
_decimal.Decimal.__floor__ = _decimal.Decimal.__ceil__

Return the floor as an Integral.
[clinic start generated code]*/

static PyObject *
_decimal_Decimal___floor___impl(PyObject *self, PyTypeObject *cls)
/*[clinic end generated code: output=e239a2f7f6514c12 input=dcc37aeceb0efb8d]*/
{
    PyObject *context;

    decimal_state *state = PyType_GetModuleState(cls);
    CURRENT_CONTEXT(state, context);
    return dec_as_long(self, context, MPD_ROUND_FLOOR);
}

/* Always uses the module context */
static Py_hash_t
_dec_hash(PyDecObject *v)
{
#if defined(CONFIG_64) && _PyHASH_BITS == 61
    /* 2**61 - 1 */
    mpd_uint_t p_data[1] = {2305843009213693951ULL};
    mpd_t p = {MPD_POS|MPD_STATIC|MPD_CONST_DATA, 0, 19, 1, 1, p_data};
    /* Inverse of 10 modulo p */
    mpd_uint_t inv10_p_data[1] = {2075258708292324556ULL};
    mpd_t inv10_p = {MPD_POS|MPD_STATIC|MPD_CONST_DATA,
                     0, 19, 1, 1, inv10_p_data};
#elif defined(CONFIG_32) && _PyHASH_BITS == 31
    /* 2**31 - 1 */
    mpd_uint_t p_data[2] = {147483647UL, 2};
    mpd_t p = {MPD_POS|MPD_STATIC|MPD_CONST_DATA, 0, 10, 2, 2, p_data};
    /* Inverse of 10 modulo p */
    mpd_uint_t inv10_p_data[2] = {503238553UL, 1};
    mpd_t inv10_p = {MPD_POS|MPD_STATIC|MPD_CONST_DATA,
                     0, 10, 2, 2, inv10_p_data};
#else
    #error "No valid combination of CONFIG_64, CONFIG_32 and _PyHASH_BITS"
#endif
    const Py_hash_t py_hash_inf = 314159;
    mpd_uint_t ten_data[1] = {10};
    mpd_t ten = {MPD_POS|MPD_STATIC|MPD_CONST_DATA,
                 0, 2, 1, 1, ten_data};
    Py_hash_t result;
    mpd_t *exp_hash = NULL;
    mpd_t *tmp = NULL;
    mpd_ssize_t exp;
    uint32_t status = 0;
    mpd_context_t maxctx;


    if (mpd_isspecial(MPD(v))) {
        if (mpd_issnan(MPD(v))) {
            PyErr_SetString(PyExc_TypeError,
                "Cannot hash a signaling NaN value");
            return -1;
        }
        else if (mpd_isnan(MPD(v))) {
            return PyObject_GenericHash((PyObject *)v);
        }
        else {
            return py_hash_inf * mpd_arith_sign(MPD(v));
        }
    }

    mpd_maxcontext(&maxctx);
    exp_hash = mpd_qnew();
    if (exp_hash == NULL) {
        goto malloc_error;
    }
    tmp = mpd_qnew();
    if (tmp == NULL) {
        goto malloc_error;
    }

    /*
     * exp(v): exponent of v
     * int(v): coefficient of v
     */
    exp = MPD(v)->exp;
    if (exp >= 0) {
        /* 10**exp(v) % p */
        mpd_qsset_ssize(tmp, exp, &maxctx, &status);
        mpd_qpowmod(exp_hash, &ten, tmp, &p, &maxctx, &status);
    }
    else {
        /* inv10_p**(-exp(v)) % p */
        mpd_qsset_ssize(tmp, -exp, &maxctx, &status);
        mpd_qpowmod(exp_hash, &inv10_p, tmp, &p, &maxctx, &status);
    }

    /* hash = (int(v) * exp_hash) % p */
    if (!mpd_qcopy(tmp, MPD(v), &status)) {
        goto malloc_error;
    }
    tmp->exp = 0;
    mpd_set_positive(tmp);

    maxctx.prec = MPD_MAX_PREC + 21;
    maxctx.emax = MPD_MAX_EMAX + 21;
    maxctx.emin = MPD_MIN_EMIN - 21;

    mpd_qmul(tmp, tmp, exp_hash, &maxctx, &status);
    mpd_qrem(tmp, tmp, &p, &maxctx, &status);

    result = mpd_qget_ssize(tmp, &status);
    result = mpd_ispositive(MPD(v)) ? result : -result;
    result = (result == -1) ? -2 : result;

    if (status != 0) {
        if (status & MPD_Malloc_error) {
            goto malloc_error;
        }
        else {
            PyErr_SetString(PyExc_RuntimeError, /* GCOV_NOT_REACHED */
                "dec_hash: internal error: please report"); /* GCOV_NOT_REACHED */
        }
        result = -1; /* GCOV_NOT_REACHED */
    }


finish:
    if (exp_hash) mpd_del(exp_hash);
    if (tmp) mpd_del(tmp);
    return result;

malloc_error:
    PyErr_NoMemory();
    result = -1;
    goto finish;
}

static Py_hash_t
dec_hash(PyObject *op)
{
    PyDecObject *self = _PyDecObject_CAST(op);
    if (self->hash == -1) {
        self->hash = _dec_hash(self);
    }

    return self->hash;
}

/*[clinic input]
_decimal.Decimal.__reduce__

Return state information for pickling.
[clinic start generated code]*/

static PyObject *
_decimal_Decimal___reduce___impl(PyObject *self)
/*[clinic end generated code: output=84fa6648a496a8d2 input=0345ea951d9b986f]*/
{
    PyObject *result, *str;

    str = dec_str(self);
    if (str == NULL) {
        return NULL;
    }

    result = Py_BuildValue("O(O)", Py_TYPE(self), str);
    Py_DECREF(str);

    return result;
}

/*[clinic input]
_decimal.Decimal.__sizeof__

    self as v: self

Returns size in memory, in bytes
[clinic start generated code]*/

static PyObject *
_decimal_Decimal___sizeof___impl(PyObject *v)
/*[clinic end generated code: output=f16de05097c62b79 input=a557db538cfddbb7]*/
{
    size_t res = _PyObject_SIZE(Py_TYPE(v));
    if (mpd_isdynamic_data(MPD(v))) {
        res += (size_t)MPD(v)->alloc * sizeof(mpd_uint_t);
    }
    return PyLong_FromSize_t(res);
}

/*[clinic input]
_decimal.Decimal.__trunc__ = _decimal.Decimal.__ceil__

Return the Integral closest to x between 0 and x.
[clinic start generated code]*/

static PyObject *
_decimal_Decimal___trunc___impl(PyObject *self, PyTypeObject *cls)
/*[clinic end generated code: output=7b3decc4b636ce32 input=9b3a3a85f63b0515]*/
{
    PyObject *context;

    decimal_state *state = PyType_GetModuleState(cls);
    CURRENT_CONTEXT(state, context);
    return dec_as_long(self, context, MPD_ROUND_DOWN);
}

/* real and imag */
static PyObject *
dec_real(PyObject *self, void *Py_UNUSED(closure))
{
    return Py_NewRef(self);
}

static PyObject *
dec_imag(PyObject *self, void *Py_UNUSED(closure))
{
    PyObject *result;

    decimal_state *state = get_module_state_by_def(Py_TYPE(self));
    result = dec_alloc(state);
    if (result == NULL) {
        return NULL;
    }

    _dec_settriple(result, MPD_POS, 0, 0);
    return result;
}


static PyGetSetDef dec_getsets [] =
{
  { "real", dec_real, NULL, NULL, NULL},
  { "imag", dec_imag, NULL, NULL, NULL},
  {NULL}
};

static PyMethodDef dec_methods [] =
{
  /* Unary arithmetic functions, optional context arg */
  _DECIMAL_DECIMAL_EXP_METHODDEF
  _DECIMAL_DECIMAL_LN_METHODDEF
  _DECIMAL_DECIMAL_LOG10_METHODDEF
  _DECIMAL_DECIMAL_NEXT_MINUS_METHODDEF
  _DECIMAL_DECIMAL_NEXT_PLUS_METHODDEF
  _DECIMAL_DECIMAL_NORMALIZE_METHODDEF
  _DECIMAL_DECIMAL_TO_INTEGRAL_METHODDEF
  _DECIMAL_DECIMAL_TO_INTEGRAL_EXACT_METHODDEF
  _DECIMAL_DECIMAL_TO_INTEGRAL_VALUE_METHODDEF
  _DECIMAL_DECIMAL_SQRT_METHODDEF

  /* Binary arithmetic functions, optional context arg */
  _DECIMAL_DECIMAL_COMPARE_METHODDEF
  _DECIMAL_DECIMAL_COMPARE_SIGNAL_METHODDEF
  _DECIMAL_DECIMAL_MAX_METHODDEF
  _DECIMAL_DECIMAL_MAX_MAG_METHODDEF
  _DECIMAL_DECIMAL_MIN_METHODDEF
  _DECIMAL_DECIMAL_MIN_MAG_METHODDEF
  _DECIMAL_DECIMAL_NEXT_TOWARD_METHODDEF
  _DECIMAL_DECIMAL_QUANTIZE_METHODDEF
  _DECIMAL_DECIMAL_REMAINDER_NEAR_METHODDEF

  /* Ternary arithmetic functions, optional context arg */
  _DECIMAL_DECIMAL_FMA_METHODDEF

  /* Boolean functions, no context arg */
  _DECIMAL_DECIMAL_IS_CANONICAL_METHODDEF
  _DECIMAL_DECIMAL_IS_FINITE_METHODDEF
  _DECIMAL_DECIMAL_IS_INFINITE_METHODDEF
  _DECIMAL_DECIMAL_IS_NAN_METHODDEF
  _DECIMAL_DECIMAL_IS_QNAN_METHODDEF
  _DECIMAL_DECIMAL_IS_SNAN_METHODDEF
  _DECIMAL_DECIMAL_IS_SIGNED_METHODDEF
  _DECIMAL_DECIMAL_IS_ZERO_METHODDEF

  /* Boolean functions, optional context arg */
  _DECIMAL_DECIMAL_IS_NORMAL_METHODDEF
  _DECIMAL_DECIMAL_IS_SUBNORMAL_METHODDEF

  /* Unary functions, no context arg */
  _DECIMAL_DECIMAL_ADJUSTED_METHODDEF
  _DECIMAL_DECIMAL_CANONICAL_METHODDEF
  _DECIMAL_DECIMAL_CONJUGATE_METHODDEF
  _DECIMAL_DECIMAL_RADIX_METHODDEF

  /* Unary functions, optional context arg for conversion errors */
  _DECIMAL_DECIMAL_COPY_ABS_METHODDEF
  _DECIMAL_DECIMAL_COPY_NEGATE_METHODDEF

  /* Unary functions, optional context arg */
  _DECIMAL_DECIMAL_LOGB_METHODDEF
  _DECIMAL_DECIMAL_LOGICAL_INVERT_METHODDEF
  _DECIMAL_DECIMAL_NUMBER_CLASS_METHODDEF
  _DECIMAL_DECIMAL_TO_ENG_STRING_METHODDEF

  /* Binary functions, optional context arg for conversion errors */
  _DECIMAL_DECIMAL_COMPARE_TOTAL_METHODDEF
  _DECIMAL_DECIMAL_COMPARE_TOTAL_MAG_METHODDEF
  _DECIMAL_DECIMAL_COPY_SIGN_METHODDEF
  _DECIMAL_DECIMAL_SAME_QUANTUM_METHODDEF

  /* Binary functions, optional context arg */
  _DECIMAL_DECIMAL_LOGICAL_AND_METHODDEF
  _DECIMAL_DECIMAL_LOGICAL_OR_METHODDEF
  _DECIMAL_DECIMAL_LOGICAL_XOR_METHODDEF
  _DECIMAL_DECIMAL_ROTATE_METHODDEF
  _DECIMAL_DECIMAL_SCALEB_METHODDEF
  _DECIMAL_DECIMAL_SHIFT_METHODDEF

  /* Miscellaneous */
  _DECIMAL_DECIMAL_FROM_FLOAT_METHODDEF
  _DECIMAL_DECIMAL_FROM_NUMBER_METHODDEF
  _DECIMAL_DECIMAL_AS_TUPLE_METHODDEF
  _DECIMAL_DECIMAL_AS_INTEGER_RATIO_METHODDEF

  /* Special methods */
  _DECIMAL_DECIMAL___COPY___METHODDEF
  _DECIMAL_DECIMAL___DEEPCOPY___METHODDEF
  _DECIMAL_DECIMAL___FORMAT___METHODDEF
  _DECIMAL_DECIMAL___REDUCE___METHODDEF
  _DECIMAL_DECIMAL___ROUND___METHODDEF
  _DECIMAL_DECIMAL___CEIL___METHODDEF
  _DECIMAL_DECIMAL___FLOOR___METHODDEF
  _DECIMAL_DECIMAL___TRUNC___METHODDEF
  _DECIMAL_DECIMAL___COMPLEX___METHODDEF
  _DECIMAL_DECIMAL___SIZEOF___METHODDEF

  { NULL, NULL, 1 }
};

static PyType_Slot dec_slots[] = {
    {Py_tp_token, Py_TP_USE_SPEC},
    {Py_tp_dealloc, dec_dealloc},
    {Py_tp_getattro, PyObject_GenericGetAttr},
    {Py_tp_traverse, _PyObject_VisitType},
    {Py_tp_repr, dec_repr},
    {Py_tp_hash, dec_hash},
    {Py_tp_str, dec_str},
    {Py_tp_doc, (void *)dec_new__doc__},
    {Py_tp_richcompare, dec_richcompare},
    {Py_tp_methods, dec_methods},
    {Py_tp_getset, dec_getsets},
    {Py_tp_new, dec_new},

    // Number protocol
    {Py_nb_add, nm_mpd_qadd},
    {Py_nb_subtract, nm_mpd_qsub},
    {Py_nb_multiply, nm_mpd_qmul},
    {Py_nb_remainder, nm_mpd_qrem},
    {Py_nb_divmod, nm_mpd_qdivmod},
    {Py_nb_power, nm_mpd_qpow},
    {Py_nb_negative, nm_mpd_qminus},
    {Py_nb_positive, nm_mpd_qplus},
    {Py_nb_absolute, nm_mpd_qabs},
    {Py_nb_bool, nm_nonzero},
    {Py_nb_int, nm_dec_as_long},
    {Py_nb_float, PyDec_AsFloat},
    {Py_nb_floor_divide, nm_mpd_qdivint},
    {Py_nb_true_divide, nm_mpd_qdiv},
    {0, NULL},
};


static PyType_Spec dec_spec = {
    .name = "decimal.Decimal",
    .basicsize = sizeof(PyDecObject),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE |
              Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_IMMUTABLETYPE),
    .slots = dec_slots,
};


/******************************************************************************/
/*                         Context Object, Part 2                             */
/******************************************************************************/


/************************************************************************/
/*     Macros for converting mpdecimal functions to Context methods     */
/************************************************************************/

/* Boolean context method.
   Argument Clinic provides PyObject *context, PyObject *x
*/
#define DecCtx_BoolFunc(MPDFUNC) \
{                                                                     \
    PyObject *ret;                                                    \
    PyObject *a;                                                      \
                                                                      \
    CONVERT_OP_RAISE(&a, x, context);                                 \
                                                                      \
    ret = MPDFUNC(MPD(a), CTX(context)) ? incr_true() : incr_false(); \
    Py_DECREF(a);                                                     \
    return ret;                                                       \
}

/* Boolean context method. MPDFUNC does NOT use a context.
   Argument Clinic provides PyObject *context, PyObject *x
*/
#define DecCtx_BoolFunc_NO_CTX(MPDFUNC) \
{                                                       \
    PyObject *ret;                                      \
    PyObject *a;                                        \
                                                        \
    CONVERT_OP_RAISE(&a, x, context);                   \
                                                        \
    ret = MPDFUNC(MPD(a)) ? incr_true() : incr_false(); \
    Py_DECREF(a);                                       \
    return ret;                                         \
}

/* Unary context method.
   Argument Clinic provides PyObject *context,
                            PyTypeObject *cls, PyObject *x
*/
#define DecCtx_UnaryFunc(MPDFUNC) \
{                                                        \
    PyObject *result, *a;                                \
    uint32_t status = 0;                                 \
                                                         \
    CONVERT_OP_RAISE(&a, x, context);                    \
    decimal_state *state = PyType_GetModuleState(cls);   \
    if ((result = dec_alloc(state)) == NULL) {           \
        Py_DECREF(a);                                    \
        return NULL;                                     \
    }                                                    \
                                                         \
    MPDFUNC(MPD(result), MPD(a), CTX(context), &status); \
    Py_DECREF(a);                                        \
    if (dec_addstatus(context, status)) {                \
        Py_DECREF(result);                               \
        return NULL;                                     \
    }                                                    \
                                                         \
    return result;                                       \
}

/* Binary context method.
   Argument Clinic provides PyObject *context, PyTypeObject *cls,
                            PyObject *x, PyObject *y
*/
#define DecCtx_BinaryFunc(MPDFUNC) \
{                                                                \
    PyObject *a, *b;                                             \
    PyObject *result;                                            \
    uint32_t status = 0;                                         \
                                                                 \
    CONVERT_BINOP_RAISE(&a, &b, x, y, context);                  \
    decimal_state *state = PyType_GetModuleState(cls);           \
    if ((result = dec_alloc(state)) == NULL) {                   \
        Py_DECREF(a);                                            \
        Py_DECREF(b);                                            \
        return NULL;                                             \
    }                                                            \
                                                                 \
    MPDFUNC(MPD(result), MPD(a), MPD(b), CTX(context), &status); \
    Py_DECREF(a);                                                \
    Py_DECREF(b);                                                \
    if (dec_addstatus(context, status)) {                        \
        Py_DECREF(result);                                       \
        return NULL;                                             \
    }                                                            \
                                                                 \
    return result;                                               \
}

/*
 * Binary context method. The context is only used for conversion.
 * The actual MPDFUNC does NOT take a context arg.
 * Argument Clinic provides PyObject *context, PyTypeObject *cls,
 *                          PyObject *x, PyObject *y
 */
#define DecCtx_BinaryFunc_NO_CTX(MPDFUNC) \
{                                                \
    PyObject *a, *b;                             \
    PyObject *result;                            \
                                                 \
    CONVERT_BINOP_RAISE(&a, &b, x, y, context);  \
    decimal_state *state =                       \
        PyType_GetModuleState(cls);              \
    if ((result = dec_alloc(state)) == NULL) {   \
        Py_DECREF(a);                            \
        Py_DECREF(b);                            \
        return NULL;                             \
    }                                            \
                                                 \
    MPDFUNC(MPD(result), MPD(a), MPD(b));        \
    Py_DECREF(a);                                \
    Py_DECREF(b);                                \
                                                 \
    return result;                               \
}

/* Ternary context method.
   Argument Clinic provides PyObject *context, PyTypeObject *cls,
                            PyObject *x, PyObject *y, PyObject *z
*/
#define DecCtx_TernaryFunc(MPDFUNC) \
{                                                                        \
    PyObject *a, *b, *c;                                                 \
    PyObject *result;                                                    \
    uint32_t status = 0;                                                 \
                                                                         \
    CONVERT_TERNOP_RAISE(&a, &b, &c, x, y, z, context);                  \
    decimal_state *state = PyType_GetModuleState(cls);                   \
    if ((result = dec_alloc(state)) == NULL) {                           \
        Py_DECREF(a);                                                    \
        Py_DECREF(b);                                                    \
        Py_DECREF(c);                                                    \
        return NULL;                                                     \
    }                                                                    \
                                                                         \
    MPDFUNC(MPD(result), MPD(a), MPD(b), MPD(c), CTX(context), &status); \
    Py_DECREF(a);                                                        \
    Py_DECREF(b);                                                        \
    Py_DECREF(c);                                                        \
    if (dec_addstatus(context, status)) {                                \
        Py_DECREF(result);                                               \
        return NULL;                                                     \
    }                                                                    \
                                                                         \
    return result;                                                       \
}


/* Unary arithmetic functions */

/*[clinic input]
_decimal.Context.abs

    self as context: self
    cls: defining_class
    x: object
    /

Return the absolute value of x.
[clinic start generated code]*/

static PyObject *
_decimal_Context_abs_impl(PyObject *context, PyTypeObject *cls, PyObject *x)
/*[clinic end generated code: output=fe080467d32e229c input=00a33f9c68463bb0]*/
DecCtx_UnaryFunc(mpd_qabs)

/*[clinic input]
_decimal.Context.exp = _decimal.Context.abs

Return e ** x.
[clinic start generated code]*/

static PyObject *
_decimal_Context_exp_impl(PyObject *context, PyTypeObject *cls, PyObject *x)
/*[clinic end generated code: output=c7477a67010ccc5f input=5b443c4ab153dd2e]*/
DecCtx_UnaryFunc(mpd_qexp)

/*[clinic input]
_decimal.Context.ln = _decimal.Context.abs

Return the natural (base e) logarithm of x.
[clinic start generated code]*/

static PyObject *
_decimal_Context_ln_impl(PyObject *context, PyTypeObject *cls, PyObject *x)
/*[clinic end generated code: output=63e691b0680bffc7 input=cf43cd98a0fe7425]*/
DecCtx_UnaryFunc(mpd_qln)

/*[clinic input]
_decimal.Context.log10 = _decimal.Context.abs

Return the base 10 logarithm of x.
[clinic start generated code]*/

static PyObject *
_decimal_Context_log10_impl(PyObject *context, PyTypeObject *cls,
                            PyObject *x)
/*[clinic end generated code: output=e0d9fc928570304d input=309e57faf42c257d]*/
DecCtx_UnaryFunc(mpd_qlog10)

/*[clinic input]
_decimal.Context.minus = _decimal.Context.abs

Minus corresponds to unary prefix minus in Python.

This operation applies the context to the result.
[clinic start generated code]*/

static PyObject *
_decimal_Context_minus_impl(PyObject *context, PyTypeObject *cls,
                            PyObject *x)
/*[clinic end generated code: output=f06c409b6aef1aad input=63be4c419d1d554b]*/
DecCtx_UnaryFunc(mpd_qminus)

/*[clinic input]
_decimal.Context.next_minus = _decimal.Context.abs

Return the largest representable number smaller than x.
[clinic start generated code]*/

static PyObject *
_decimal_Context_next_minus_impl(PyObject *context, PyTypeObject *cls,
                                 PyObject *x)
/*[clinic end generated code: output=8dd168f08bec9547 input=969f4d24dfcd5e85]*/
DecCtx_UnaryFunc(mpd_qnext_minus)

/*[clinic input]
_decimal.Context.next_plus = _decimal.Context.abs

Return the smallest representable number larger than x.
[clinic start generated code]*/

static PyObject *
_decimal_Context_next_plus_impl(PyObject *context, PyTypeObject *cls,
                                PyObject *x)
/*[clinic end generated code: output=2a50586ad2f7c108 input=af1a85ee59b56a3c]*/
DecCtx_UnaryFunc(mpd_qnext_plus)

/*[clinic input]
_decimal.Context.normalize = _decimal.Context.abs

Reduce x to its simplest form. Alias for reduce(x).
[clinic start generated code]*/

static PyObject *
_decimal_Context_normalize_impl(PyObject *context, PyTypeObject *cls,
                                PyObject *x)
/*[clinic end generated code: output=9a9510f442ba2852 input=a65bc39c81a654a9]*/
DecCtx_UnaryFunc(mpd_qreduce)

/*[clinic input]
_decimal.Context.plus = _decimal.Context.abs

Plus corresponds to the unary prefix plus operator in Python.

This operation applies the context to the result.
[clinic start generated code]*/

static PyObject *
_decimal_Context_plus_impl(PyObject *context, PyTypeObject *cls, PyObject *x)
/*[clinic end generated code: output=c37d29f58a47f93a input=5d8a75702d20e2f9]*/
DecCtx_UnaryFunc(mpd_qplus)

/*[clinic input]
_decimal.Context.to_integral_value = _decimal.Context.abs

Round to an integer.
[clinic start generated code]*/

static PyObject *
_decimal_Context_to_integral_value_impl(PyObject *context, PyTypeObject *cls,
                                        PyObject *x)
/*[clinic end generated code: output=e3d9ad000bc06036 input=3103e147cb9de9ed]*/
DecCtx_UnaryFunc(mpd_qround_to_int)

/*[clinic input]
_decimal.Context.to_integral_exact = _decimal.Context.abs

Round to an integer. Signal if the result is rounded or inexact.
[clinic start generated code]*/

static PyObject *
_decimal_Context_to_integral_exact_impl(PyObject *context, PyTypeObject *cls,
                                        PyObject *x)
/*[clinic end generated code: output=680b796dfae8e2ef input=677dc4b915907b68]*/
DecCtx_UnaryFunc(mpd_qround_to_intx)

/*[clinic input]
_decimal.Context.to_integral = _decimal.Context.abs

Identical to to_integral_value(x).
[clinic start generated code]*/

static PyObject *
_decimal_Context_to_integral_impl(PyObject *context, PyTypeObject *cls,
                                  PyObject *x)
/*[clinic end generated code: output=09f4823b90b2cf17 input=89d4a4b15495b8c9]*/
DecCtx_UnaryFunc(mpd_qround_to_int)

/*[clinic input]
_decimal.Context.sqrt = _decimal.Context.abs

Square root of a non-negative number to context precision.
[clinic start generated code]*/

static PyObject *
_decimal_Context_sqrt_impl(PyObject *context, PyTypeObject *cls, PyObject *x)
/*[clinic end generated code: output=2b9c16c6f5ceead0 input=90bd954b0b8076fb]*/
DecCtx_UnaryFunc(mpd_qsqrt)

/* Binary arithmetic functions */

/*[clinic input]
_decimal.Context.add

    self as context: self
    cls: defining_class
    x: object
    y: object
    /

Return the sum of x and y.
[clinic start generated code]*/

static PyObject *
_decimal_Context_add_impl(PyObject *context, PyTypeObject *cls, PyObject *x,
                          PyObject *y)
/*[clinic end generated code: output=ab4f0fb841e6a867 input=f2c74f6a845f62e9]*/
DecCtx_BinaryFunc(mpd_qadd)

/*[clinic input]
_decimal.Context.compare = _decimal.Context.add

Compare x and y numerically.
[clinic start generated code]*/

static PyObject *
_decimal_Context_compare_impl(PyObject *context, PyTypeObject *cls,
                              PyObject *x, PyObject *y)
/*[clinic end generated code: output=56efd1faf653f1d7 input=f701cb179c966ec1]*/
DecCtx_BinaryFunc(mpd_qcompare)

/*[clinic input]
_decimal.Context.compare_signal = _decimal.Context.add

Compare x and y numerically.  All NaNs signal.
[clinic start generated code]*/

static PyObject *
_decimal_Context_compare_signal_impl(PyObject *context, PyTypeObject *cls,
                                     PyObject *x, PyObject *y)
/*[clinic end generated code: output=7c1a9a9f6ae4e5cd input=32a1bcef7bbc5179]*/
DecCtx_BinaryFunc(mpd_qcompare_signal)

/*[clinic input]
_decimal.Context.divide = _decimal.Context.add

Return x divided by y.
[clinic start generated code]*/

static PyObject *
_decimal_Context_divide_impl(PyObject *context, PyTypeObject *cls,
                             PyObject *x, PyObject *y)
/*[clinic end generated code: output=1a7924b20e24a528 input=00cd9bc2ba2a1786]*/
DecCtx_BinaryFunc(mpd_qdiv)

/*[clinic input]
_decimal.Context.divide_int = _decimal.Context.add

Return x divided by y, truncated to an integer.
[clinic start generated code]*/

static PyObject *
_decimal_Context_divide_int_impl(PyObject *context, PyTypeObject *cls,
                                 PyObject *x, PyObject *y)
/*[clinic end generated code: output=7a1d8948625105f0 input=e80ada2f50d9719d]*/
DecCtx_BinaryFunc(mpd_qdivint)

/*[clinic input]
_decimal.Context.max = _decimal.Context.add

Compare the values numerically and return the maximum.
[clinic start generated code]*/

static PyObject *
_decimal_Context_max_impl(PyObject *context, PyTypeObject *cls, PyObject *x,
                          PyObject *y)
/*[clinic end generated code: output=cd54af10a51c11fc input=22008ab898c86a8b]*/
DecCtx_BinaryFunc(mpd_qmax)

/*[clinic input]
_decimal.Context.max_mag = _decimal.Context.add

Compare the values numerically with their sign ignored.
[clinic start generated code]*/

static PyObject *
_decimal_Context_max_mag_impl(PyObject *context, PyTypeObject *cls,
                              PyObject *x, PyObject *y)
/*[clinic end generated code: output=1c812e73bcb7827f input=f7ce42ef82a7c52e]*/
DecCtx_BinaryFunc(mpd_qmax_mag)

/*[clinic input]
_decimal.Context.min = _decimal.Context.add

Compare the values numerically and return the minimum.
[clinic start generated code]*/

static PyObject *
_decimal_Context_min_impl(PyObject *context, PyTypeObject *cls, PyObject *x,
                          PyObject *y)
/*[clinic end generated code: output=aa494e95b88107b3 input=2aeec1167638c5ef]*/
DecCtx_BinaryFunc(mpd_qmin)

/*[clinic input]
_decimal.Context.min_mag = _decimal.Context.add

Compare the values numerically with their sign ignored.
[clinic start generated code]*/

static PyObject *
_decimal_Context_min_mag_impl(PyObject *context, PyTypeObject *cls,
                              PyObject *x, PyObject *y)
/*[clinic end generated code: output=ee0b69c1d9a14185 input=19d158c29e4fc140]*/
DecCtx_BinaryFunc(mpd_qmin_mag)

/*[clinic input]
_decimal.Context.multiply = _decimal.Context.add

Return the product of x and y.
[clinic start generated code]*/

static PyObject *
_decimal_Context_multiply_impl(PyObject *context, PyTypeObject *cls,
                               PyObject *x, PyObject *y)
/*[clinic end generated code: output=45f33b805afa01a8 input=2fdd01acdbeef8ba]*/
DecCtx_BinaryFunc(mpd_qmul)

/*[clinic input]
_decimal.Context.next_toward = _decimal.Context.add

Return the number closest to x, in the direction towards y.
[clinic start generated code]*/

static PyObject *
_decimal_Context_next_toward_impl(PyObject *context, PyTypeObject *cls,
                                  PyObject *x, PyObject *y)
/*[clinic end generated code: output=436afff6f43edec2 input=aac775298e02b68c]*/
DecCtx_BinaryFunc(mpd_qnext_toward)

/*[clinic input]
_decimal.Context.quantize = _decimal.Context.add

Return a value equal to x (rounded), having the exponent of y.
[clinic start generated code]*/

static PyObject *
_decimal_Context_quantize_impl(PyObject *context, PyTypeObject *cls,
                               PyObject *x, PyObject *y)
/*[clinic end generated code: output=fcf8cd32b7d628c9 input=43d67a696ab6d895]*/
DecCtx_BinaryFunc(mpd_qquantize)

/*[clinic input]
_decimal.Context.remainder = _decimal.Context.add

Return the remainder from integer division.

The sign of the result, if non-zero, is the same as that of the
original dividend.
[clinic start generated code]*/

static PyObject *
_decimal_Context_remainder_impl(PyObject *context, PyTypeObject *cls,
                                PyObject *x, PyObject *y)
/*[clinic end generated code: output=e0f96c834abbfbd2 input=36d0eb2b392c1215]*/
DecCtx_BinaryFunc(mpd_qrem)

/*[clinic input]
_decimal.Context.remainder_near = _decimal.Context.add

Return x - y * n.

Here n is the integer nearest the exact value of x / y (if the result
is 0 then its sign will be the sign of x).
[clinic start generated code]*/

static PyObject *
_decimal_Context_remainder_near_impl(PyObject *context, PyTypeObject *cls,
                                     PyObject *x, PyObject *y)
/*[clinic end generated code: output=7f18c535a12cf8ac input=bafb6327bb314c5c]*/
DecCtx_BinaryFunc(mpd_qrem_near)

/*[clinic input]
_decimal.Context.subtract = _decimal.Context.add

Return the difference between x and y.
[clinic start generated code]*/

static PyObject *
_decimal_Context_subtract_impl(PyObject *context, PyTypeObject *cls,
                               PyObject *x, PyObject *y)
/*[clinic end generated code: output=3d764a8a87e79401 input=6767683ec68f7a1a]*/
DecCtx_BinaryFunc(mpd_qsub)

/*[clinic input]
_decimal.Context.divmod

    self as context: self
    x: object
    y: object
    /

Return quotient and remainder of the division x / y.
[clinic start generated code]*/

static PyObject *
_decimal_Context_divmod_impl(PyObject *context, PyObject *x, PyObject *y)
/*[clinic end generated code: output=5dbf5410e3f302af input=4d8eee07823c752a]*/
{
    PyObject *a, *b;
    PyObject *q, *r;
    uint32_t status = 0;
    PyObject *ret;

    CONVERT_BINOP_RAISE(&a, &b, x, y, context);
    decimal_state *state = get_module_state_from_ctx(context);
    q = dec_alloc(state);
    if (q == NULL) {
        Py_DECREF(a);
        Py_DECREF(b);
        return NULL;
    }
    r = dec_alloc(state);
    if (r == NULL) {
        Py_DECREF(a);
        Py_DECREF(b);
        Py_DECREF(q);
        return NULL;
    }

    mpd_qdivmod(MPD(q), MPD(r), MPD(a), MPD(b), CTX(context), &status);
    Py_DECREF(a);
    Py_DECREF(b);
    if (dec_addstatus(context, status)) {
        Py_DECREF(r);
        Py_DECREF(q);
        return NULL;
    }

    ret = PyTuple_Pack(2, q, r);
    Py_DECREF(r);
    Py_DECREF(q);
    return ret;
}

/* Binary or ternary arithmetic functions */

/*[clinic input]
_decimal.Context.power

    self as context: self
    cls: defining_class
    a as base: object
    b as exp: object
    modulo as mod: object = None

Compute a**b.

If 'a' is negative, then 'b' must be integral. The result will be
inexact unless 'a' is integral and the result is finite and can be
expressed exactly in 'precision' digits.  In the Python version the
result is always correctly rounded, in the C version the result is
almost always correctly rounded.

If modulo is given, compute (a**b) % modulo. The following
restrictions hold:

    * all three arguments must be integral
    * 'b' must be nonnegative
    * at least one of 'a' or 'b' must be nonzero
    * modulo must be nonzero and less than 10**prec in absolute value
[clinic start generated code]*/

static PyObject *
_decimal_Context_power_impl(PyObject *context, PyTypeObject *cls,
                            PyObject *base, PyObject *exp, PyObject *mod)
/*[clinic end generated code: output=d06d40c37cdd69dc input=2a70edd03317c666]*/
{
    PyObject *a, *b, *c = NULL;
    PyObject *result;
    uint32_t status = 0;

    CONVERT_BINOP_RAISE(&a, &b, base, exp, context);

    if (mod != Py_None) {
        if (!convert_op(TYPE_ERR, &c, mod, context)) {
            Py_DECREF(a);
            Py_DECREF(b);
            return c;
        }
    }

    decimal_state *state = PyType_GetModuleState(cls);
    result = dec_alloc(state);
    if (result == NULL) {
        Py_DECREF(a);
        Py_DECREF(b);
        Py_XDECREF(c);
        return NULL;
    }

    if (c == NULL) {
        mpd_qpow(MPD(result), MPD(a), MPD(b),
                 CTX(context), &status);
    }
    else {
        mpd_qpowmod(MPD(result), MPD(a), MPD(b), MPD(c),
                    CTX(context), &status);
        Py_DECREF(c);
    }
    Py_DECREF(a);
    Py_DECREF(b);
    if (dec_addstatus(context, status)) {
        Py_DECREF(result);
        return NULL;
    }

    return result;
}

/* Ternary arithmetic functions */

/*[clinic input]
_decimal.Context.fma

    self as context: self
    cls: defining_class
    x: object
    y: object
    z: object
    /

Return x multiplied by y, plus z.
[clinic start generated code]*/

static PyObject *
_decimal_Context_fma_impl(PyObject *context, PyTypeObject *cls, PyObject *x,
                          PyObject *y, PyObject *z)
/*[clinic end generated code: output=08ec3cefc59d71a9 input=da3963b1a1da83b9]*/
DecCtx_TernaryFunc(mpd_qfma)

/* No argument */

/*[clinic input]
_decimal.Context.radix

    self as context: self
    cls: defining_class

Return 10.
[clinic start generated code]*/

static PyObject *
_decimal_Context_radix_impl(PyObject *context, PyTypeObject *cls)
/*[clinic end generated code: output=674b88b7cd0c264d input=e1e4f8c0abf86825]*/
{
    decimal_state *state = PyType_GetModuleState(cls);
    return _dec_mpd_radix(state);
}

/* Boolean functions: single decimal argument */

/*[clinic input]
_decimal.Context.is_normal

    self as context: self
    cls: defining_class
    x: object
    /

Return True if x is a normal number, False otherwise.
[clinic start generated code]*/

static PyObject *
_decimal_Context_is_normal_impl(PyObject *context, PyTypeObject *cls,
                                PyObject *x)
/*[clinic end generated code: output=089c5609db60bf57 input=7c90b825a517ef7e]*/
DecCtx_BoolFunc(mpd_isnormal)

/*[clinic input]
_decimal.Context.is_subnormal = _decimal.Context.is_normal

Return True if x is subnormal, False otherwise.
[clinic start generated code]*/

static PyObject *
_decimal_Context_is_subnormal_impl(PyObject *context, PyTypeObject *cls,
                                   PyObject *x)
/*[clinic end generated code: output=f58c45a288aadeda input=73f1bd9367b913a4]*/
DecCtx_BoolFunc(mpd_issubnormal)

/*[clinic input]
_decimal.Context.is_finite = _decimal.Context.is_normal

Return True if x is finite, False otherwise.
[clinic start generated code]*/

static PyObject *
_decimal_Context_is_finite_impl(PyObject *context, PyTypeObject *cls,
                                PyObject *x)
/*[clinic end generated code: output=dfb00f1b5589b9f0 input=abff92a8a6bb85e6]*/
DecCtx_BoolFunc_NO_CTX(mpd_isfinite)

/*[clinic input]
_decimal.Context.is_infinite = _decimal.Context.is_normal

Return True if x is infinite, False otherwise.
[clinic start generated code]*/

static PyObject *
_decimal_Context_is_infinite_impl(PyObject *context, PyTypeObject *cls,
                                  PyObject *x)
/*[clinic end generated code: output=1c28517500811d01 input=591242ae9a1e60e6]*/
DecCtx_BoolFunc_NO_CTX(mpd_isinfinite)

/*[clinic input]
_decimal.Context.is_nan = _decimal.Context.is_normal

Return True if x is a qNaN or sNaN, False otherwise.
[clinic start generated code]*/

static PyObject *
_decimal_Context_is_nan_impl(PyObject *context, PyTypeObject *cls,
                             PyObject *x)
/*[clinic end generated code: output=9dc15463ee19864a input=520218376d5eec5e]*/
DecCtx_BoolFunc_NO_CTX(mpd_isnan)

/*[clinic input]
_decimal.Context.is_qnan = _decimal.Context.is_normal

Return True if x is a quiet NaN, False otherwise.
[clinic start generated code]*/

static PyObject *
_decimal_Context_is_qnan_impl(PyObject *context, PyTypeObject *cls,
                              PyObject *x)
/*[clinic end generated code: output=4caa672e03703b6d input=97d06a14ab3360d1]*/
DecCtx_BoolFunc_NO_CTX(mpd_isqnan)

/*[clinic input]
_decimal.Context.is_snan = _decimal.Context.is_normal

Return True if x is a signaling NaN, False otherwise.
[clinic start generated code]*/

static PyObject *
_decimal_Context_is_snan_impl(PyObject *context, PyTypeObject *cls,
                              PyObject *x)
/*[clinic end generated code: output=a8caa929d9f82ecd input=0059fe4e9c3b25a8]*/
DecCtx_BoolFunc_NO_CTX(mpd_issnan)

/*[clinic input]
_decimal.Context.is_signed = _decimal.Context.is_normal

Return True if x is negative, False otherwise.
[clinic start generated code]*/

static PyObject *
_decimal_Context_is_signed_impl(PyObject *context, PyTypeObject *cls,
                                PyObject *x)
/*[clinic end generated code: output=42c450c99d4fe7db input=b950cd697721ab8b]*/
DecCtx_BoolFunc_NO_CTX(mpd_issigned)

/*[clinic input]
_decimal.Context.is_zero = _decimal.Context.is_normal

Return True if x is a zero, False otherwise.
[clinic start generated code]*/

static PyObject *
_decimal_Context_is_zero_impl(PyObject *context, PyTypeObject *cls,
                              PyObject *x)
/*[clinic end generated code: output=e6c55359b7241d9e input=bf08197d142a8027]*/
DecCtx_BoolFunc_NO_CTX(mpd_iszero)

/*[clinic input]
_decimal.Context.is_canonical = _decimal.Context.is_normal

Return True if x is canonical, False otherwise.
[clinic start generated code]*/

static PyObject *
_decimal_Context_is_canonical_impl(PyObject *context, PyTypeObject *cls,
                                   PyObject *x)
/*[clinic end generated code: output=18ee249d9aec957c input=1bf2129808e55eb9]*/
{
    decimal_state *state = PyType_GetModuleState(cls);
    if (!PyDec_Check(state, x)) {
        PyErr_SetString(PyExc_TypeError,
            "argument must be a Decimal");
        return NULL;
    }

    return mpd_iscanonical(MPD(x)) ? incr_true() : incr_false();
}

/* Functions with a single decimal argument */
/*[clinic input]
_decimal.Context._apply = _decimal.Context.is_normal

Apply self to Decimal x.
[clinic start generated code]*/

static PyObject *
_decimal_Context__apply_impl(PyObject *context, PyTypeObject *cls,
                             PyObject *x)
/*[clinic end generated code: output=c6b542f4e8114b97 input=12b34468ca4a4c30]*/
{
    PyObject *result, *a;

    CONVERT_OP_RAISE(&a, x, context);

    result = dec_apply(a, context);
    Py_DECREF(a);
    return result;
}

#ifdef EXTRA_FUNCTIONALITY
/*[clinic input]
_decimal.Context.apply = _decimal.Context._apply

Apply self to Decimal x.
[clinic start generated code]*/

static PyObject *
_decimal_Context_apply_impl(PyObject *context, PyTypeObject *cls,
                            PyObject *x)
/*[clinic end generated code: output=f8a7142d47ad4ff3 input=388e66ca82733516]*/
{
    return _decimal_Context__apply(context, v);
}
#endif

/*[clinic input]
_decimal.Context.canonical = _decimal.Context.is_normal

Return a new instance of x.
[clinic start generated code]*/

static PyObject *
_decimal_Context_canonical_impl(PyObject *context, PyTypeObject *cls,
                                PyObject *x)
/*[clinic end generated code: output=f213e433e2032e5e input=025ecb106ac15bff]*/
{
    decimal_state *state = PyType_GetModuleState(cls);
    if (!PyDec_Check(state, x)) {
        PyErr_SetString(PyExc_TypeError,
            "argument must be a Decimal");
        return NULL;
    }

    return Py_NewRef(x);
}

/*[clinic input]
_decimal.Context.copy_abs = _decimal.Context.is_normal

Return a copy of x with the sign set to 0.
[clinic start generated code]*/

static PyObject *
_decimal_Context_copy_abs_impl(PyObject *context, PyTypeObject *cls,
                               PyObject *x)
/*[clinic end generated code: output=a141ad4b9afe2deb input=4aa2f612625f0f73]*/
{
    PyObject *result, *a;
    uint32_t status = 0;

    CONVERT_OP_RAISE(&a, x, context);
    decimal_state *state = PyType_GetModuleState(cls);
    result = dec_alloc(state);
    if (result == NULL) {
        Py_DECREF(a);
        return NULL;
    }

    mpd_qcopy_abs(MPD(result), MPD(a), &status);
    Py_DECREF(a);
    if (dec_addstatus(context, status)) {
        Py_DECREF(result);
        return NULL;
    }

    return result;
}

/*[clinic input]
_decimal.Context.copy_decimal = _decimal.Context.is_normal

Return a copy of Decimal x.
[clinic start generated code]*/

static PyObject *
_decimal_Context_copy_decimal_impl(PyObject *context, PyTypeObject *cls,
                                   PyObject *x)
/*[clinic end generated code: output=639a82e1193d31f6 input=4db4f942f45fb7c9]*/
{
    PyObject *result;

    CONVERT_OP_RAISE(&result, x, context);
    return result;
}

/*[clinic input]
_decimal.Context.copy_negate = _decimal.Context.is_normal

Return a copy of x with the sign inverted.
[clinic start generated code]*/

static PyObject *
_decimal_Context_copy_negate_impl(PyObject *context, PyTypeObject *cls,
                                  PyObject *x)
/*[clinic end generated code: output=e49d013489dc252b input=2e6e213e2ed0efda]*/
{
    PyObject *result, *a;
    uint32_t status = 0;

    CONVERT_OP_RAISE(&a, x, context);
    decimal_state *state = PyType_GetModuleState(cls);
    result = dec_alloc(state);
    if (result == NULL) {
        Py_DECREF(a);
        return NULL;
    }

    mpd_qcopy_negate(MPD(result), MPD(a), &status);
    Py_DECREF(a);
    if (dec_addstatus(context, status)) {
        Py_DECREF(result);
        return NULL;
    }

    return result;
}

/*[clinic input]
_decimal.Context.logb = _decimal.Context.abs

Return the exponent of the magnitude of the operand's MSD.
[clinic start generated code]*/

static PyObject *
_decimal_Context_logb_impl(PyObject *context, PyTypeObject *cls, PyObject *x)
/*[clinic end generated code: output=9b9697e1eb68093f input=28d1cd1a8a906b9a]*/
DecCtx_UnaryFunc(mpd_qlogb)

/*[clinic input]
_decimal.Context.logical_invert = _decimal.Context.abs

Invert all digits of x.
[clinic start generated code]*/

static PyObject *
_decimal_Context_logical_invert_impl(PyObject *context, PyTypeObject *cls,
                                     PyObject *x)
/*[clinic end generated code: output=97760277a958e2b0 input=1fa8dcc59c557fcc]*/
DecCtx_UnaryFunc(mpd_qinvert)

/*[clinic input]
_decimal.Context.number_class = _decimal.Context.is_normal

Return an indication of the class of x.
[clinic start generated code]*/

static PyObject *
_decimal_Context_number_class_impl(PyObject *context, PyTypeObject *cls,
                                   PyObject *x)
/*[clinic end generated code: output=c1592a23e25ba5ee input=1ead8462f1800e4e]*/
{
    PyObject *a;
    const char *cp;

    CONVERT_OP_RAISE(&a, x, context);

    cp = mpd_class(MPD(a), CTX(context));
    Py_DECREF(a);

    return PyUnicode_FromString(cp);
}

/*[clinic input]
_decimal.Context.to_sci_string = _decimal.Context.is_normal

Convert a number to a string using scientific notation.
[clinic start generated code]*/

static PyObject *
_decimal_Context_to_sci_string_impl(PyObject *context, PyTypeObject *cls,
                                    PyObject *x)
/*[clinic end generated code: output=092dcdef999d72da input=ed442677c66d342d]*/
{
    PyObject *result;
    PyObject *a;
    mpd_ssize_t size;
    char *s;

    CONVERT_OP_RAISE(&a, x, context);

    size = mpd_to_sci_size(&s, MPD(a), CtxCaps(context));
    Py_DECREF(a);
    if (size < 0) {
        PyErr_NoMemory();
        return NULL;
    }

    result = unicode_fromascii(s, size);
    mpd_free(s);

    return result;
}

/*[clinic input]
_decimal.Context.to_eng_string = _decimal.Context.is_normal

Convert a number to a string, using engineering notation.
[clinic start generated code]*/

static PyObject *
_decimal_Context_to_eng_string_impl(PyObject *context, PyTypeObject *cls,
                                    PyObject *x)
/*[clinic end generated code: output=7fc53216c208f487 input=a574385e2e3e3bc0]*/
{
    PyObject *result;
    PyObject *a;
    mpd_ssize_t size;
    char *s;

    CONVERT_OP_RAISE(&a, x, context);

    size = mpd_to_eng_size(&s, MPD(a), CtxCaps(context));
    Py_DECREF(a);
    if (size < 0) {
        PyErr_NoMemory();
        return NULL;
    }

    result = unicode_fromascii(s, size);
    mpd_free(s);

    return result;
}

/* Functions with two decimal arguments */

/*[clinic input]
_decimal.Context.compare_total

    self as context: self
    cls: defining_class
    x: object
    y: object
    /

Compare x and y using their abstract representation.
[clinic start generated code]*/

static PyObject *
_decimal_Context_compare_total_impl(PyObject *context, PyTypeObject *cls,
                                    PyObject *x, PyObject *y)
/*[clinic end generated code: output=f79177b27fe930e3 input=2bfc677a841e297a]*/
DecCtx_BinaryFunc_NO_CTX(mpd_compare_total)

/*[clinic input]
_decimal.Context.compare_total_mag = _decimal.Context.compare_total

Compare x and y using their abstract representation, ignoring sign.
[clinic start generated code]*/

static PyObject *
_decimal_Context_compare_total_mag_impl(PyObject *context, PyTypeObject *cls,
                                        PyObject *x, PyObject *y)
/*[clinic end generated code: output=2528c669ccd6d6ff input=2b982e69f932dcb2]*/
DecCtx_BinaryFunc_NO_CTX(mpd_compare_total_mag)

/*[clinic input]
_decimal.Context.copy_sign = _decimal.Context.compare_total

Copy the sign from y to x.
[clinic start generated code]*/

static PyObject *
_decimal_Context_copy_sign_impl(PyObject *context, PyTypeObject *cls,
                                PyObject *x, PyObject *y)
/*[clinic end generated code: output=77d23b6f4e42120c input=c0682aeaffc7cfdf]*/
{
    PyObject *a, *b;
    PyObject *result;
    uint32_t status = 0;

    CONVERT_BINOP_RAISE(&a, &b, x, y, context);
    decimal_state *state = PyType_GetModuleState(cls);
    result = dec_alloc(state);
    if (result == NULL) {
        Py_DECREF(a);
        Py_DECREF(b);
        return NULL;
    }

    mpd_qcopy_sign(MPD(result), MPD(a), MPD(b), &status);
    Py_DECREF(a);
    Py_DECREF(b);
    if (dec_addstatus(context, status)) {
        Py_DECREF(result);
        return NULL;
    }

    return result;
}

/*[clinic input]
_decimal.Context.logical_and = _decimal.Context.add

Digit-wise and of x and y.
[clinic start generated code]*/

static PyObject *
_decimal_Context_logical_and_impl(PyObject *context, PyTypeObject *cls,
                                  PyObject *x, PyObject *y)
/*[clinic end generated code: output=009dfa08ecaa2ac8 input=30ee33b5b365fd80]*/
DecCtx_BinaryFunc(mpd_qand)

/*[clinic input]
_decimal.Context.logical_or = _decimal.Context.add

Digit-wise or of x and y.
[clinic start generated code]*/

static PyObject *
_decimal_Context_logical_or_impl(PyObject *context, PyTypeObject *cls,
                                 PyObject *x, PyObject *y)
/*[clinic end generated code: output=eb38617e8d31bf12 input=3b1a6725d0262fb9]*/
DecCtx_BinaryFunc(mpd_qor)

/*[clinic input]
_decimal.Context.logical_xor = _decimal.Context.add

Digit-wise xor of x and y.
[clinic start generated code]*/

static PyObject *
_decimal_Context_logical_xor_impl(PyObject *context, PyTypeObject *cls,
                                  PyObject *x, PyObject *y)
/*[clinic end generated code: output=23cd81fdcd865d5a input=5ebbbe8bb35da380]*/
DecCtx_BinaryFunc(mpd_qxor)

/*[clinic input]
_decimal.Context.rotate = _decimal.Context.add

Return a copy of x, rotated by y places.
[clinic start generated code]*/

static PyObject *
_decimal_Context_rotate_impl(PyObject *context, PyTypeObject *cls,
                             PyObject *x, PyObject *y)
/*[clinic end generated code: output=3d5b3cfcb4659432 input=7ad91845c909eb0a]*/
DecCtx_BinaryFunc(mpd_qrotate)

/*[clinic input]
_decimal.Context.scaleb = _decimal.Context.add

Return the first operand after adding the second value to its exp.
[clinic start generated code]*/

static PyObject *
_decimal_Context_scaleb_impl(PyObject *context, PyTypeObject *cls,
                             PyObject *x, PyObject *y)
/*[clinic end generated code: output=795ac61bcbe61c67 input=c5d2ee7a57f65f8c]*/
DecCtx_BinaryFunc(mpd_qscaleb)

/*[clinic input]
_decimal.Context.shift = _decimal.Context.add

Return a copy of x, shifted by y places.
[clinic start generated code]*/

static PyObject *
_decimal_Context_shift_impl(PyObject *context, PyTypeObject *cls,
                            PyObject *x, PyObject *y)
/*[clinic end generated code: output=43d69615f0271c81 input=1ab44ff0854420ce]*/
DecCtx_BinaryFunc(mpd_qshift)

/*[clinic input]
_decimal.Context.same_quantum = _decimal.Context.add

Return True if the two operands have the same exponent.
[clinic start generated code]*/

static PyObject *
_decimal_Context_same_quantum_impl(PyObject *context, PyTypeObject *cls,
                                   PyObject *x, PyObject *y)
/*[clinic end generated code: output=91a4d8325f98d9e9 input=194cd156e398eaf9]*/
{
    PyObject *a, *b;
    PyObject *result;

    CONVERT_BINOP_RAISE(&a, &b, x, y, context);

    result = mpd_same_quantum(MPD(a), MPD(b)) ? incr_true() : incr_false();
    Py_DECREF(a);
    Py_DECREF(b);

    return result;
}


static PyMethodDef context_methods [] =
{
  /* Unary arithmetic functions */
  _DECIMAL_CONTEXT_ABS_METHODDEF
  _DECIMAL_CONTEXT_EXP_METHODDEF
  _DECIMAL_CONTEXT_LN_METHODDEF
  _DECIMAL_CONTEXT_LOG10_METHODDEF
  _DECIMAL_CONTEXT_MINUS_METHODDEF
  _DECIMAL_CONTEXT_NEXT_MINUS_METHODDEF
  _DECIMAL_CONTEXT_NEXT_PLUS_METHODDEF
  _DECIMAL_CONTEXT_NORMALIZE_METHODDEF
  _DECIMAL_CONTEXT_PLUS_METHODDEF
  _DECIMAL_CONTEXT_TO_INTEGRAL_METHODDEF
  _DECIMAL_CONTEXT_TO_INTEGRAL_EXACT_METHODDEF
  _DECIMAL_CONTEXT_TO_INTEGRAL_VALUE_METHODDEF
  _DECIMAL_CONTEXT_SQRT_METHODDEF

  /* Binary arithmetic functions */
  _DECIMAL_CONTEXT_ADD_METHODDEF
  _DECIMAL_CONTEXT_COMPARE_METHODDEF
  _DECIMAL_CONTEXT_COMPARE_SIGNAL_METHODDEF
  _DECIMAL_CONTEXT_DIVIDE_METHODDEF
  _DECIMAL_CONTEXT_DIVIDE_INT_METHODDEF
  _DECIMAL_CONTEXT_DIVMOD_METHODDEF
  _DECIMAL_CONTEXT_MAX_METHODDEF
  _DECIMAL_CONTEXT_MAX_MAG_METHODDEF
  _DECIMAL_CONTEXT_MIN_METHODDEF
  _DECIMAL_CONTEXT_MIN_MAG_METHODDEF
  _DECIMAL_CONTEXT_MULTIPLY_METHODDEF
  _DECIMAL_CONTEXT_NEXT_TOWARD_METHODDEF
  _DECIMAL_CONTEXT_QUANTIZE_METHODDEF
  _DECIMAL_CONTEXT_REMAINDER_METHODDEF
  _DECIMAL_CONTEXT_REMAINDER_NEAR_METHODDEF
  _DECIMAL_CONTEXT_SUBTRACT_METHODDEF

  /* Binary or ternary arithmetic functions */
  _DECIMAL_CONTEXT_POWER_METHODDEF

  /* Ternary arithmetic functions */
  _DECIMAL_CONTEXT_FMA_METHODDEF

  /* No argument */
  _DECIMAL_CONTEXT_ETINY_METHODDEF
  _DECIMAL_CONTEXT_ETOP_METHODDEF
  _DECIMAL_CONTEXT_RADIX_METHODDEF

  /* Boolean functions */
  _DECIMAL_CONTEXT_IS_CANONICAL_METHODDEF
  _DECIMAL_CONTEXT_IS_FINITE_METHODDEF
  _DECIMAL_CONTEXT_IS_INFINITE_METHODDEF
  _DECIMAL_CONTEXT_IS_NAN_METHODDEF
  _DECIMAL_CONTEXT_IS_NORMAL_METHODDEF
  _DECIMAL_CONTEXT_IS_QNAN_METHODDEF
  _DECIMAL_CONTEXT_IS_SIGNED_METHODDEF
  _DECIMAL_CONTEXT_IS_SNAN_METHODDEF
  _DECIMAL_CONTEXT_IS_SUBNORMAL_METHODDEF
  _DECIMAL_CONTEXT_IS_ZERO_METHODDEF

  /* Functions with a single decimal argument */
  _DECIMAL_CONTEXT__APPLY_METHODDEF
  _DECIMAL_CONTEXT_APPLY_METHODDEF
  _DECIMAL_CONTEXT_CANONICAL_METHODDEF
  _DECIMAL_CONTEXT_COPY_ABS_METHODDEF
  _DECIMAL_CONTEXT_COPY_DECIMAL_METHODDEF
  _DECIMAL_CONTEXT_COPY_NEGATE_METHODDEF
  _DECIMAL_CONTEXT_LOGB_METHODDEF
  _DECIMAL_CONTEXT_LOGICAL_INVERT_METHODDEF
  _DECIMAL_CONTEXT_NUMBER_CLASS_METHODDEF
  _DECIMAL_CONTEXT_TO_SCI_STRING_METHODDEF
  _DECIMAL_CONTEXT_TO_ENG_STRING_METHODDEF

  /* Functions with two decimal arguments */
  _DECIMAL_CONTEXT_COMPARE_TOTAL_METHODDEF
  _DECIMAL_CONTEXT_COMPARE_TOTAL_MAG_METHODDEF
  _DECIMAL_CONTEXT_COPY_SIGN_METHODDEF
  _DECIMAL_CONTEXT_LOGICAL_AND_METHODDEF
  _DECIMAL_CONTEXT_LOGICAL_OR_METHODDEF
  _DECIMAL_CONTEXT_LOGICAL_XOR_METHODDEF
  _DECIMAL_CONTEXT_ROTATE_METHODDEF
  _DECIMAL_CONTEXT_SAME_QUANTUM_METHODDEF
  _DECIMAL_CONTEXT_SCALEB_METHODDEF
  _DECIMAL_CONTEXT_SHIFT_METHODDEF

  /* Set context values */
  _DECIMAL_CONTEXT_CLEAR_FLAGS_METHODDEF
  _DECIMAL_CONTEXT_CLEAR_TRAPS_METHODDEF

  /* Unsafe set functions with relaxed range checks */
  _DECIMAL_CONTEXT__UNSAFE_SETPREC_METHODDEF
  _DECIMAL_CONTEXT__UNSAFE_SETEMIN_METHODDEF
  _DECIMAL_CONTEXT__UNSAFE_SETEMAX_METHODDEF

  /* Miscellaneous */
  _DECIMAL_CONTEXT___COPY___METHODDEF
  _DECIMAL_CONTEXT___REDUCE___METHODDEF
  _DECIMAL_CONTEXT_COPY_METHODDEF
  _DECIMAL_CONTEXT_CREATE_DECIMAL_METHODDEF
  _DECIMAL_CONTEXT_CREATE_DECIMAL_FROM_FLOAT_METHODDEF

  { NULL, NULL, 1 }
};

static PyType_Slot context_slots[] = {
    {Py_tp_token, Py_TP_USE_SPEC},
    {Py_tp_dealloc, context_dealloc},
    {Py_tp_traverse, context_traverse},
    {Py_tp_clear, context_clear},
    {Py_tp_repr, context_repr},
    {Py_tp_getattro, context_getattr},
    {Py_tp_setattro, context_setattr},
    {Py_tp_doc, (void *)context_init__doc__},
    {Py_tp_methods, context_methods},
    {Py_tp_getset, context_getsets},
    {Py_tp_init, context_init},
    {Py_tp_new, context_new},
    {0, NULL},
};

static PyType_Spec context_spec = {
    .name = "decimal.Context",
    .basicsize = sizeof(PyDecContextObject),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE |
              Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_IMMUTABLETYPE),
    .slots = context_slots,
};


static PyMethodDef _decimal_methods [] =
{
  _DECIMAL_GETCONTEXT_METHODDEF
  _DECIMAL_SETCONTEXT_METHODDEF
  _DECIMAL_LOCALCONTEXT_METHODDEF
  _DECIMAL_IEEECONTEXT_METHODDEF
  { NULL, NULL, 1, NULL }
};


struct ssize_constmap { const char *name; mpd_ssize_t val; };
static struct ssize_constmap ssize_constants [] = {
    {"MAX_PREC", MPD_MAX_PREC},
    {"MAX_EMAX", MPD_MAX_EMAX},
    {"MIN_EMIN",  MPD_MIN_EMIN},
    {"MIN_ETINY", MPD_MIN_ETINY},
    {NULL}
};

struct int_constmap { const char *name; int val; };
static struct int_constmap int_constants [] = {
    /* int constants */
    {"IEEE_CONTEXT_MAX_BITS", MPD_IEEE_CONTEXT_MAX_BITS},
#ifdef EXTRA_FUNCTIONALITY
    {"DECIMAL32", MPD_DECIMAL32},
    {"DECIMAL64", MPD_DECIMAL64},
    {"DECIMAL128", MPD_DECIMAL128},
    /* int condition flags */
    {"DecClamped", MPD_Clamped},
    {"DecConversionSyntax", MPD_Conversion_syntax},
    {"DecDivisionByZero", MPD_Division_by_zero},
    {"DecDivisionImpossible", MPD_Division_impossible},
    {"DecDivisionUndefined", MPD_Division_undefined},
    {"DecFpuError", MPD_Fpu_error},
    {"DecInexact", MPD_Inexact},
    {"DecInvalidContext", MPD_Invalid_context},
    {"DecInvalidOperation", MPD_Invalid_operation},
    {"DecIEEEInvalidOperation", MPD_IEEE_Invalid_operation},
    {"DecMallocError", MPD_Malloc_error},
    {"DecFloatOperation", MPD_Float_operation},
    {"DecOverflow", MPD_Overflow},
    {"DecRounded", MPD_Rounded},
    {"DecSubnormal", MPD_Subnormal},
    {"DecUnderflow", MPD_Underflow},
    {"DecErrors", MPD_Errors},
    {"DecTraps", MPD_Traps},
#endif
    {NULL}
};


#define CHECK_INT(expr) \
    do { if ((expr) < 0) goto error; } while (0)
#define ASSIGN_PTR(result, expr) \
    do { result = (expr); if (result == NULL) goto error; } while (0)
#define CHECK_PTR(expr) \
    do { if ((expr) == NULL) goto error; } while (0)


static PyCFunction
cfunc_noargs(PyTypeObject *t, const char *name)
{
    struct PyMethodDef *m;

    if (t->tp_methods == NULL) {
        goto error;
    }

    for (m = t->tp_methods; m->ml_name != NULL; m++) {
        if (strcmp(name, m->ml_name) == 0) {
            if (!(m->ml_flags & METH_NOARGS)) {
                goto error;
            }
            return m->ml_meth;
        }
    }

error:
    PyErr_Format(PyExc_RuntimeError,
        "internal error: could not find method %s", name);
    return NULL;
}

static int minalloc_is_set = 0;

static int
_decimal_exec(PyObject *m)
{
    PyObject *numbers = NULL;
    PyObject *Number = NULL;
    PyObject *collections = NULL;
    PyObject *collections_abc = NULL;
    PyObject *MutableMapping = NULL;
    PyObject *obj = NULL;
    DecCondMap *cm;
    struct ssize_constmap *ssize_cm;
    struct int_constmap *int_cm;
    int i;


    /* Init libmpdec */
    mpd_traphandler = dec_traphandler;
    mpd_mallocfunc = PyMem_Malloc;
    mpd_reallocfunc = PyMem_Realloc;
    mpd_callocfunc = mpd_callocfunc_em;
    mpd_free = PyMem_Free;

    /* Suppress the warning caused by multi-phase initialization */
    if (!minalloc_is_set) {
        mpd_setminalloc(_Py_DEC_MINALLOC);
        minalloc_is_set = 1;
    }

    decimal_state *state = get_module_state(m);

    /* Init external C-API functions */
    state->_py_long_multiply = PyLong_Type.tp_as_number->nb_multiply;
    state->_py_long_floor_divide = PyLong_Type.tp_as_number->nb_floor_divide;
    state->_py_long_power = PyLong_Type.tp_as_number->nb_power;
    state->_py_float_abs = PyFloat_Type.tp_as_number->nb_absolute;
    ASSIGN_PTR(state->_py_float_as_integer_ratio,
               cfunc_noargs(&PyFloat_Type, "as_integer_ratio"));
    ASSIGN_PTR(state->_py_long_bit_length,
               cfunc_noargs(&PyLong_Type, "bit_length"));


    /* Init types */
#define CREATE_TYPE(mod, tp, spec) do {                               \
    tp = (PyTypeObject *)PyType_FromMetaclass(NULL, mod, spec, NULL); \
    CHECK_PTR(tp);                                                    \
} while (0)

    CREATE_TYPE(m, state->PyDec_Type, &dec_spec);
    CREATE_TYPE(m, state->PyDecContext_Type, &context_spec);
    CREATE_TYPE(m, state->PyDecContextManager_Type, &ctxmanager_spec);
    CREATE_TYPE(m, state->PyDecSignalDictMixin_Type, &signaldict_spec);

#undef CREATE_TYPE

    ASSIGN_PTR(obj, PyUnicode_FromString("decimal"));
    CHECK_INT(PyDict_SetItemString(state->PyDec_Type->tp_dict, "__module__", obj));
    CHECK_INT(PyDict_SetItemString(state->PyDecContext_Type->tp_dict,
                                   "__module__", obj));
    Py_CLEAR(obj);


    /* Numeric abstract base classes */
    ASSIGN_PTR(numbers, PyImport_ImportModule("numbers"));
    ASSIGN_PTR(Number, PyObject_GetAttrString(numbers, "Number"));
    /* Register Decimal with the Number abstract base class */
    ASSIGN_PTR(obj, PyObject_CallMethod(Number, "register", "(O)",
                                        (PyObject *)state->PyDec_Type));
    Py_CLEAR(obj);
    /* Rational is a global variable used for fraction comparisons. */
    ASSIGN_PTR(state->Rational, PyObject_GetAttrString(numbers, "Rational"));
    /* Done with numbers, Number */
    Py_CLEAR(numbers);
    Py_CLEAR(Number);

    /* DecimalTuple */
    ASSIGN_PTR(collections, PyImport_ImportModule("collections"));
    ASSIGN_PTR(state->DecimalTuple, (PyTypeObject *)PyObject_CallMethod(collections,
                                 "namedtuple", "(ss)", "DecimalTuple",
                                 "sign digits exponent"));

    ASSIGN_PTR(obj, PyUnicode_FromString("decimal"));
    CHECK_INT(PyDict_SetItemString(state->DecimalTuple->tp_dict, "__module__", obj));
    Py_CLEAR(obj);

    /* MutableMapping */
    ASSIGN_PTR(collections_abc, PyImport_ImportModule("collections.abc"));
    ASSIGN_PTR(MutableMapping, PyObject_GetAttrString(collections_abc,
                                                      "MutableMapping"));
    /* Create SignalDict type */
    ASSIGN_PTR(state->PyDecSignalDict_Type,
                   (PyTypeObject *)PyObject_CallFunction(
                   (PyObject *)&PyType_Type, "s(OO){}",
                   "SignalDict", state->PyDecSignalDictMixin_Type,
                   MutableMapping));

    /* Done with collections, MutableMapping */
    Py_CLEAR(collections);
    Py_CLEAR(collections_abc);
    Py_CLEAR(MutableMapping);

    /* For format specifiers not yet supported by libmpdec */
    state->PyDecimal = NULL;

    /* Add types to the module */
    CHECK_INT(PyModule_AddType(m, state->PyDec_Type));
    CHECK_INT(PyModule_AddType(m, state->PyDecContext_Type));
    CHECK_INT(PyModule_AddType(m, state->DecimalTuple));

    /* Create top level exception */
    ASSIGN_PTR(state->DecimalException, PyErr_NewException(
                                     "decimal.DecimalException",
                                     PyExc_ArithmeticError, NULL));
    CHECK_INT(PyModule_AddType(m, (PyTypeObject *)state->DecimalException));

    /* Create signal tuple */
    ASSIGN_PTR(state->SignalTuple, PyTuple_New(SIGNAL_MAP_LEN));

    /* Add exceptions that correspond to IEEE signals */
    ASSIGN_PTR(state->signal_map, dec_cond_map_init(signal_map_template,
                                                    sizeof(signal_map_template)));
    for (i = SIGNAL_MAP_LEN-1; i >= 0; i--) {
        PyObject *base;

        cm = state->signal_map + i;

        switch (cm->flag) {
        case MPD_Float_operation:
            base = PyTuple_Pack(2, state->DecimalException, PyExc_TypeError);
            break;
        case MPD_Division_by_zero:
            base = PyTuple_Pack(2, state->DecimalException,
                                PyExc_ZeroDivisionError);
            break;
        case MPD_Overflow:
            base = PyTuple_Pack(2, state->signal_map[INEXACT].ex,
                                   state->signal_map[ROUNDED].ex);
            break;
        case MPD_Underflow:
            base = PyTuple_Pack(3, state->signal_map[INEXACT].ex,
                                   state->signal_map[ROUNDED].ex,
                                   state->signal_map[SUBNORMAL].ex);
            break;
        default:
            base = PyTuple_Pack(1, state->DecimalException);
            break;
        }

        if (base == NULL) {
            goto error; /* GCOV_NOT_REACHED */
        }

        ASSIGN_PTR(cm->ex, PyErr_NewException(cm->fqname, base, NULL));
        Py_DECREF(base);

        /* add to module */
        CHECK_INT(PyModule_AddObjectRef(m, cm->name, cm->ex));

        /* add to signal tuple */
        PyTuple_SET_ITEM(state->SignalTuple, i, Py_NewRef(cm->ex));
    }

    /*
     * Unfortunately, InvalidOperation is a signal that comprises
     * several conditions, including InvalidOperation! Naming the
     * signal IEEEInvalidOperation would prevent the confusion.
     */
    ASSIGN_PTR(state->cond_map, dec_cond_map_init(cond_map_template,
                                                  sizeof(cond_map_template)));
    state->cond_map[0].ex = state->signal_map[0].ex;

    /* Add remaining exceptions, inherit from InvalidOperation */
    for (cm = state->cond_map+1; cm->name != NULL; cm++) {
        PyObject *base;
        if (cm->flag == MPD_Division_undefined) {
            base = PyTuple_Pack(2, state->signal_map[0].ex, PyExc_ZeroDivisionError);
        }
        else {
            base = PyTuple_Pack(1, state->signal_map[0].ex);
        }
        if (base == NULL) {
            goto error; /* GCOV_NOT_REACHED */
        }

        ASSIGN_PTR(cm->ex, PyErr_NewException(cm->fqname, base, NULL));
        Py_DECREF(base);

        CHECK_INT(PyModule_AddObjectRef(m, cm->name, cm->ex));
    }


    /* Init default context template first */
    ASSIGN_PTR(state->default_context_template,
               PyObject_CallObject((PyObject *)state->PyDecContext_Type, NULL));
    CHECK_INT(PyModule_AddObjectRef(m, "DefaultContext",
                                    state->default_context_template));

#ifndef WITH_DECIMAL_CONTEXTVAR
    ASSIGN_PTR(state->tls_context_key,
               PyUnicode_FromString("___DECIMAL_CTX__"));
    CHECK_INT(PyModule_AddObjectRef(m, "HAVE_CONTEXTVAR", Py_False));
#else
    ASSIGN_PTR(state->current_context_var, PyContextVar_New("decimal_context", NULL));
    CHECK_INT(PyModule_AddObjectRef(m, "HAVE_CONTEXTVAR", Py_True));
#endif
    CHECK_INT(PyModule_AddObjectRef(m, "HAVE_THREADS", Py_True));

    /* Init basic context template */
    ASSIGN_PTR(state->basic_context_template,
               PyObject_CallObject((PyObject *)state->PyDecContext_Type, NULL));
    init_basic_context(state->basic_context_template);
    CHECK_INT(PyModule_AddObjectRef(m, "BasicContext",
                                    state->basic_context_template));

    /* Init extended context template */
    ASSIGN_PTR(state->extended_context_template,
               PyObject_CallObject((PyObject *)state->PyDecContext_Type, NULL));
    init_extended_context(state->extended_context_template);
    CHECK_INT(PyModule_AddObjectRef(m, "ExtendedContext",
                                    state->extended_context_template));


    /* Init mpd_ssize_t constants */
    for (ssize_cm = ssize_constants; ssize_cm->name != NULL; ssize_cm++) {
        CHECK_INT(PyModule_Add(m, ssize_cm->name,
                               PyLong_FromSsize_t(ssize_cm->val)));
    }

    /* Init int constants */
    for (int_cm = int_constants; int_cm->name != NULL; int_cm++) {
        CHECK_INT(PyModule_AddIntConstant(m, int_cm->name,
                                          int_cm->val));
    }

    /* Init string constants */
    for (i = 0; i < _PY_DEC_ROUND_GUARD; i++) {
        ASSIGN_PTR(state->round_map[i], PyUnicode_InternFromString(mpd_round_string[i]));
        CHECK_INT(PyModule_AddObjectRef(m, mpd_round_string[i], state->round_map[i]));
    }

    /* Add specification version number */
    CHECK_INT(PyModule_AddStringConstant(m, "__version__", "1.70"));
    CHECK_INT(PyModule_AddStringConstant(m, "__libmpdec_version__", mpd_version()));

    return 0;

error:
    Py_CLEAR(obj); /* GCOV_NOT_REACHED */
    Py_CLEAR(numbers); /* GCOV_NOT_REACHED */
    Py_CLEAR(Number); /* GCOV_NOT_REACHED */
    Py_CLEAR(collections); /* GCOV_NOT_REACHED */
    Py_CLEAR(collections_abc); /* GCOV_NOT_REACHED */
    Py_CLEAR(MutableMapping); /* GCOV_NOT_REACHED */

    return -1;
}

static int
decimal_traverse(PyObject *module, visitproc visit, void *arg)
{
    decimal_state *state = get_module_state(module);
    Py_VISIT(state->PyDecContextManager_Type);
    Py_VISIT(state->PyDecContext_Type);
    Py_VISIT(state->PyDecSignalDictMixin_Type);
    Py_VISIT(state->PyDec_Type);
    Py_VISIT(state->PyDecSignalDict_Type);
    Py_VISIT(state->DecimalTuple);
    Py_VISIT(state->DecimalException);

#ifndef WITH_DECIMAL_CONTEXTVAR
    Py_VISIT(state->tls_context_key);
    Py_VISIT(state->cached_context);
#else
    Py_VISIT(state->current_context_var);
#endif

    Py_VISIT(state->default_context_template);
    Py_VISIT(state->basic_context_template);
    Py_VISIT(state->extended_context_template);
    Py_VISIT(state->Rational);
    Py_VISIT(state->SignalTuple);

    if (state->signal_map != NULL) {
        for (DecCondMap *cm = state->signal_map; cm->name != NULL; cm++) {
            Py_VISIT(cm->ex);
        }
    }
    if (state->cond_map != NULL) {
        for (DecCondMap *cm = state->cond_map + 1; cm->name != NULL; cm++) {
            Py_VISIT(cm->ex);
        }
    }
    return 0;
}

static int
decimal_clear(PyObject *module)
{
    decimal_state *state = get_module_state(module);
    Py_CLEAR(state->PyDecContextManager_Type);
    Py_CLEAR(state->PyDecContext_Type);
    Py_CLEAR(state->PyDecSignalDictMixin_Type);
    Py_CLEAR(state->PyDec_Type);
    Py_CLEAR(state->PyDecSignalDict_Type);
    Py_CLEAR(state->DecimalTuple);
    Py_CLEAR(state->DecimalException);

#ifndef WITH_DECIMAL_CONTEXTVAR
    Py_CLEAR(state->tls_context_key);
    Py_CLEAR(state->cached_context);
#else
    Py_CLEAR(state->current_context_var);
#endif

    Py_CLEAR(state->default_context_template);
    Py_CLEAR(state->basic_context_template);
    Py_CLEAR(state->extended_context_template);
    Py_CLEAR(state->Rational);
    Py_CLEAR(state->SignalTuple);
    Py_CLEAR(state->PyDecimal);

    if (state->signal_map != NULL) {
        for (DecCondMap *cm = state->signal_map; cm->name != NULL; cm++) {
            Py_DECREF(cm->ex);
        }
        PyMem_Free(state->signal_map);
        state->signal_map = NULL;
    }

    if (state->cond_map != NULL) {
        // cond_map[0].ex has borrowed a reference from signal_map[0].ex
        for (DecCondMap *cm = state->cond_map + 1; cm->name != NULL; cm++) {
            Py_DECREF(cm->ex);
        }
        PyMem_Free(state->cond_map);
        state->cond_map = NULL;
    }
    return 0;
}

static void
decimal_free(void *module)
{
    (void)decimal_clear((PyObject *)module);
}

static struct PyModuleDef_Slot _decimal_slots[] = {
    {Py_mod_exec, _decimal_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL},
};

static struct PyModuleDef _decimal_module = {
    PyModuleDef_HEAD_INIT,
    .m_name = "decimal",
    .m_doc = "C decimal arithmetic module",
    .m_size = sizeof(decimal_state),
    .m_methods = _decimal_methods,
    .m_slots = _decimal_slots,
    .m_traverse = decimal_traverse,
    .m_clear = decimal_clear,
    .m_free = decimal_free,
};

PyMODINIT_FUNC
PyInit__decimal(void)
{
    return PyModuleDef_Init(&_decimal_module);
}
