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


#include <Python.h>
#include "longintrepr.h"
#include "complexobject.h"
#include "mpdecimal.h"

#include <stdlib.h>

#include "docstrings.h"


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

typedef struct {
    PyObject_HEAD
    uint32_t *flags;
} PyDecSignalDictObject;

typedef struct {
    PyObject_HEAD
    mpd_context_t ctx;
    PyObject *traps;
    PyObject *flags;
    int capitals;
    PyThreadState *tstate;
} PyDecContextObject;

typedef struct {
    PyObject_HEAD
    PyObject *local;
    PyObject *global;
} PyDecContextManagerObject;


#undef MPD
#undef CTX
static PyTypeObject PyDec_Type;
static PyTypeObject *PyDecSignalDict_Type;
static PyTypeObject PyDecContext_Type;
static PyTypeObject PyDecContextManager_Type;
#define PyDec_CheckExact(v) Py_IS_TYPE(v, &PyDec_Type)
#define PyDec_Check(v) PyObject_TypeCheck(v, &PyDec_Type)
#define PyDecSignalDict_Check(v) Py_IS_TYPE(v, PyDecSignalDict_Type)
#define PyDecContext_Check(v) PyObject_TypeCheck(v, &PyDecContext_Type)
#define MPD(v) (&((PyDecObject *)v)->dec)
#define SdFlagAddr(v) (((PyDecSignalDictObject *)v)->flags)
#define SdFlags(v) (*((PyDecSignalDictObject *)v)->flags)
#define CTX(v) (&((PyDecContextObject *)v)->ctx)
#define CtxCaps(v) (((PyDecContextObject *)v)->capitals)


Py_LOCAL_INLINE(PyObject *)
incr_true(void)
{
    Py_INCREF(Py_True);
    return Py_True;
}

Py_LOCAL_INLINE(PyObject *)
incr_false(void)
{
    Py_INCREF(Py_False);
    return Py_False;
}


#ifndef WITH_DECIMAL_CONTEXTVAR
/* Key for thread state dictionary */
static PyObject *tls_context_key = NULL;
/* Invariant: NULL or the most recently accessed thread local context */
static PyDecContextObject *cached_context = NULL;
#else
static PyObject *current_context_var = NULL;
#endif

/* Template for creating new thread contexts, calling Context() without
 * arguments and initializing the module_context on first access. */
static PyObject *default_context_template = NULL;
/* Basic and extended context templates */
static PyObject *basic_context_template = NULL;
static PyObject *extended_context_template = NULL;


/* Error codes for functions that return signals or conditions */
#define DEC_INVALID_SIGNALS (MPD_Max_status+1U)
#define DEC_ERR_OCCURRED (DEC_INVALID_SIGNALS<<1)
#define DEC_ERRORS (DEC_INVALID_SIGNALS|DEC_ERR_OCCURRED)

typedef struct {
    const char *name;   /* condition or signal name */
    const char *fqname; /* fully qualified name */
    uint32_t flag;      /* libmpdec flag */
    PyObject *ex;       /* corresponding exception */
} DecCondMap;

/* Top level Exception; inherits from ArithmeticError */
static PyObject *DecimalException = NULL;

/* Exceptions that correspond to IEEE signals */
#define SUBNORMAL 5
#define INEXACT 6
#define ROUNDED 7
#define SIGNAL_MAP_LEN 9
static DecCondMap signal_map[] = {
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
static DecCondMap cond_map[] = {
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

#ifdef EXTRA_FUNCTIONALITY
  #define _PY_DEC_ROUND_GUARD MPD_ROUND_GUARD
#else
  #define _PY_DEC_ROUND_GUARD (MPD_ROUND_GUARD-1)
#endif
static PyObject *round_map[_PY_DEC_ROUND_GUARD];

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

#ifdef CONFIG_32
static PyObject *
value_error_ptr(const char *mesg)
{
    PyErr_SetString(PyExc_ValueError, mesg);
    return NULL;
}
#endif

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
dec_traphandler(mpd_context_t *ctx UNUSED) /* GCOV_NOT_REACHED */
{ /* GCOV_NOT_REACHED */
    return; /* GCOV_NOT_REACHED */
}

static PyObject *
flags_as_exception(uint32_t flags)
{
    DecCondMap *cm;

    for (cm = signal_map; cm->name != NULL; cm++) {
        if (flags&cm->flag) {
            return cm->ex;
        }
    }

    INTERNAL_ERROR_PTR("flags_as_exception"); /* GCOV_NOT_REACHED */
}

Py_LOCAL_INLINE(uint32_t)
exception_as_flag(PyObject *ex)
{
    DecCondMap *cm;

    for (cm = signal_map; cm->name != NULL; cm++) {
        if (cm->ex == ex) {
            return cm->flag;
        }
    }

    PyErr_SetString(PyExc_KeyError, invalid_signals_err);
    return DEC_INVALID_SIGNALS;
}

static PyObject *
flags_as_list(uint32_t flags)
{
    PyObject *list;
    DecCondMap *cm;

    list = PyList_New(0);
    if (list == NULL) {
        return NULL;
    }

    for (cm = cond_map; cm->name != NULL; cm++) {
        if (flags&cm->flag) {
            if (PyList_Append(list, cm->ex) < 0) {
                goto error;
            }
        }
    }
    for (cm = signal_map+1; cm->name != NULL; cm++) {
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
signals_as_list(uint32_t flags)
{
    PyObject *list;
    DecCondMap *cm;

    list = PyList_New(0);
    if (list == NULL) {
        return NULL;
    }

    for (cm = signal_map; cm->name != NULL; cm++) {
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
list_as_flags(PyObject *list)
{
    PyObject *item;
    uint32_t flags, x;
    Py_ssize_t n, j;

    assert(PyList_Check(list));

    n = PyList_Size(list);
    flags = 0;
    for (j = 0; j < n; j++) {
        item = PyList_GetItem(list, j);
        x = exception_as_flag(item);
        if (x & DEC_ERRORS) {
            return x;
        }
        flags |= x;
    }

    return flags;
}

static PyObject *
flags_as_dict(uint32_t flags)
{
    DecCondMap *cm;
    PyObject *dict;

    dict = PyDict_New();
    if (dict == NULL) {
        return NULL;
    }

    for (cm = signal_map; cm->name != NULL; cm++) {
        PyObject *b = flags&cm->flag ? Py_True : Py_False;
        if (PyDict_SetItem(dict, cm->ex, b) < 0) {
            Py_DECREF(dict);
            return NULL;
        }
    }

    return dict;
}

static uint32_t
dict_as_flags(PyObject *val)
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

    for (cm = signal_map; cm->name != NULL; cm++) {
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

    ctx->status |= status;
    if (status & (ctx->traps|MPD_Malloc_error)) {
        PyObject *ex, *siglist;

        if (status & MPD_Malloc_error) {
            PyErr_NoMemory();
            return 1;
        }

        ex = flags_as_exception(ctx->traps&status);
        if (ex == NULL) {
            return 1; /* GCOV_NOT_REACHED */
        }
        siglist = flags_as_list(ctx->traps&status);
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
getround(PyObject *v)
{
    int i;

    if (PyUnicode_Check(v)) {
        for (i = 0; i < _PY_DEC_ROUND_GUARD; i++) {
            if (v == round_map[i]) {
                return i;
            }
        }
        for (i = 0; i < _PY_DEC_ROUND_GUARD; i++) {
            if (PyUnicode_Compare(v, round_map[i]) == 0) {
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

static int
signaldict_init(PyObject *self, PyObject *args UNUSED, PyObject *kwds UNUSED)
{
    SdFlagAddr(self) = NULL;
    return 0;
}

static Py_ssize_t
signaldict_len(PyObject *self UNUSED)
{
    return SIGNAL_MAP_LEN;
}

static PyObject *SignalTuple;
static PyObject *
signaldict_iter(PyObject *self UNUSED)
{
    return PyTuple_Type.tp_iter(SignalTuple);
}

static PyObject *
signaldict_getitem(PyObject *self, PyObject *key)
{
    uint32_t flag;

    flag = exception_as_flag(key);
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

    if (value == NULL) {
        return value_error_int("signal keys cannot be deleted");
    }

    flag = exception_as_flag(key);
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

static PyObject *
signaldict_repr(PyObject *self)
{
    DecCondMap *cm;
    const char *n[SIGNAL_MAP_LEN]; /* name */
    const char *b[SIGNAL_MAP_LEN]; /* bool */
    int i;

    assert(SIGNAL_MAP_LEN == 9);

    for (cm=signal_map, i=0; cm->name != NULL; cm++, i++) {
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

    assert(PyDecSignalDict_Check(v));

    if (op == Py_EQ || op == Py_NE) {
        if (PyDecSignalDict_Check(w)) {
            res = (SdFlags(v)==SdFlags(w)) ^ (op==Py_NE) ? Py_True : Py_False;
        }
        else if (PyDict_Check(w)) {
            uint32_t flags = dict_as_flags(w);
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

    Py_INCREF(res);
    return res;
}

static PyObject *
signaldict_copy(PyObject *self, PyObject *args UNUSED)
{
    return flags_as_dict(SdFlags(self));
}


static PyMappingMethods signaldict_as_mapping = {
    (lenfunc)signaldict_len,          /* mp_length */
    (binaryfunc)signaldict_getitem,   /* mp_subscript */
    (objobjargproc)signaldict_setitem /* mp_ass_subscript */
};

static PyMethodDef signaldict_methods[] = {
    { "copy", (PyCFunction)signaldict_copy, METH_NOARGS, NULL},
    {NULL, NULL}
};


static PyTypeObject PyDecSignalDictMixin_Type =
{
    PyVarObject_HEAD_INIT(0, 0)
    "decimal.SignalDictMixin",                /* tp_name */
    sizeof(PyDecSignalDictObject),            /* tp_basicsize */
    0,                                        /* tp_itemsize */
    0,                                        /* tp_dealloc */
    0,                                        /* tp_vectorcall_offset */
    (getattrfunc) 0,                          /* tp_getattr */
    (setattrfunc) 0,                          /* tp_setattr */
    0,                                        /* tp_as_async */
    (reprfunc) signaldict_repr,               /* tp_repr */
    0,                                        /* tp_as_number */
    0,                                        /* tp_as_sequence */
    &signaldict_as_mapping,                   /* tp_as_mapping */
    PyObject_HashNotImplemented,              /* tp_hash */
    0,                                        /* tp_call */
    (reprfunc) 0,                             /* tp_str */
    PyObject_GenericGetAttr,                  /* tp_getattro */
    (setattrofunc) 0,                         /* tp_setattro */
    (PyBufferProcs *) 0,                      /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE|
    Py_TPFLAGS_HAVE_GC,                       /* tp_flags */
    0,                                        /* tp_doc */
    0,                                        /* tp_traverse */
    0,                                        /* tp_clear */
    signaldict_richcompare,                   /* tp_richcompare */
    0,                                        /* tp_weaklistoffset */
    (getiterfunc)signaldict_iter,             /* tp_iter */
    0,                                        /* tp_iternext */
    signaldict_methods,                       /* tp_methods */
    0,                                        /* tp_members */
    0,                                        /* tp_getset */
    0,                                        /* tp_base */
    0,                                        /* tp_dict */
    0,                                        /* tp_descr_get */
    0,                                        /* tp_descr_set */
    0,                                        /* tp_dictoffset */
    (initproc)signaldict_init,                /* tp_init */
    0,                                        /* tp_alloc */
    PyType_GenericNew,                        /* tp_new */
};


/******************************************************************************/
/*                         Context Object, Part 1                             */
/******************************************************************************/

#define Dec_CONTEXT_GET_SSIZE(mem) \
static PyObject *                                       \
context_get##mem(PyObject *self, void *closure UNUSED)  \
{                                                       \
    return PyLong_FromSsize_t(mpd_get##mem(CTX(self))); \
}

#define Dec_CONTEXT_GET_ULONG(mem) \
static PyObject *                                            \
context_get##mem(PyObject *self, void *closure UNUSED)       \
{                                                            \
    return PyLong_FromUnsignedLong(mpd_get##mem(CTX(self))); \
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
context_getround(PyObject *self, void *closure UNUSED)
{
    int i = mpd_getround(CTX(self));

    Py_INCREF(round_map[i]);
    return round_map[i];
}

static PyObject *
context_getcapitals(PyObject *self, void *closure UNUSED)
{
    return PyLong_FromLong(CtxCaps(self));
}

#ifdef EXTRA_FUNCTIONALITY
static PyObject *
context_getallcr(PyObject *self, void *closure UNUSED)
{
    return PyLong_FromLong(mpd_getcr(CTX(self)));
}
#endif

static PyObject *
context_getetiny(PyObject *self, PyObject *dummy UNUSED)
{
    return PyLong_FromSsize_t(mpd_etiny(CTX(self)));
}

static PyObject *
context_getetop(PyObject *self, PyObject *dummy UNUSED)
{
    return PyLong_FromSsize_t(mpd_etop(CTX(self)));
}

static int
context_setprec(PyObject *self, PyObject *value, void *closure UNUSED)
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
context_setemin(PyObject *self, PyObject *value, void *closure UNUSED)
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
context_setemax(PyObject *self, PyObject *value, void *closure UNUSED)
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
static PyObject *
context_unsafe_setprec(PyObject *self, PyObject *value)
{
    mpd_context_t *ctx = CTX(self);
    mpd_ssize_t x;

    x = PyLong_AsSsize_t(value);
    if (x == -1 && PyErr_Occurred()) {
        return NULL;
    }

    if (x < 1 || x > 1070000000L) {
        return value_error_ptr(
            "valid range for unsafe prec is [1, 1070000000]");
    }

    ctx->prec = x;
    Py_RETURN_NONE;
}

static PyObject *
context_unsafe_setemin(PyObject *self, PyObject *value)
{
    mpd_context_t *ctx = CTX(self);
    mpd_ssize_t x;

    x = PyLong_AsSsize_t(value);
    if (x == -1 && PyErr_Occurred()) {
        return NULL;
    }

    if (x < -1070000000L || x > 0) {
        return value_error_ptr(
            "valid range for unsafe emin is [-1070000000, 0]");
    }

    ctx->emin = x;
    Py_RETURN_NONE;
}

static PyObject *
context_unsafe_setemax(PyObject *self, PyObject *value)
{
    mpd_context_t *ctx = CTX(self);
    mpd_ssize_t x;

    x = PyLong_AsSsize_t(value);
    if (x == -1 && PyErr_Occurred()) {
        return NULL;
    }

    if (x < 0 || x > 1070000000L) {
        return value_error_ptr(
            "valid range for unsafe emax is [0, 1070000000]");
    }

    ctx->emax = x;
    Py_RETURN_NONE;
}
#endif

static int
context_setround(PyObject *self, PyObject *value, void *closure UNUSED)
{
    mpd_context_t *ctx;
    int x;

    x = getround(value);
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
context_setcapitals(PyObject *self, PyObject *value, void *closure UNUSED)
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
context_settraps(PyObject *self, PyObject *value, void *closure UNUSED)
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

    flags = list_as_flags(value);
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

    if (PyDecSignalDict_Check(value)) {
        flags = SdFlags(value);
    }
    else {
        flags = dict_as_flags(value);
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
context_setstatus(PyObject *self, PyObject *value, void *closure UNUSED)
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

    flags = list_as_flags(value);
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

    if (PyDecSignalDict_Check(value)) {
        flags = SdFlags(value);
    }
    else {
        flags = dict_as_flags(value);
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
context_setclamp(PyObject *self, PyObject *value, void *closure UNUSED)
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
context_setallcr(PyObject *self, PyObject *value, void *closure UNUSED)
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
            Py_INCREF(retval);
            return retval;
        }
        if (PyUnicode_CompareWithASCIIString(name, "flags") == 0) {
            retval = ((PyDecContextObject *)self)->flags;
            Py_INCREF(retval);
            return retval;
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

static PyObject *
context_clear_traps(PyObject *self, PyObject *dummy UNUSED)
{
    CTX(self)->traps = 0;
    Py_RETURN_NONE;
}

static PyObject *
context_clear_flags(PyObject *self, PyObject *dummy UNUSED)
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
context_new(PyTypeObject *type, PyObject *args UNUSED, PyObject *kwds UNUSED)
{
    PyDecContextObject *self = NULL;
    mpd_context_t *ctx;

    if (type == &PyDecContext_Type) {
        self = PyObject_New(PyDecContextObject, &PyDecContext_Type);
    }
    else {
        self = (PyDecContextObject *)type->tp_alloc(type, 0);
    }

    if (self == NULL) {
        return NULL;
    }

    self->traps = PyObject_CallObject((PyObject *)PyDecSignalDict_Type, NULL);
    if (self->traps == NULL) {
        self->flags = NULL;
        Py_DECREF(self);
        return NULL;
    }
    self->flags = PyObject_CallObject((PyObject *)PyDecSignalDict_Type, NULL);
    if (self->flags == NULL) {
        Py_DECREF(self);
        return NULL;
    }

    ctx = CTX(self);

    if (default_context_template) {
        *ctx = *CTX(default_context_template);
    }
    else {
        *ctx = dflt_ctx;
    }

    SdFlagAddr(self->traps) = &ctx->traps;
    SdFlagAddr(self->flags) = &ctx->status;

    CtxCaps(self) = 1;
    self->tstate = NULL;

    return (PyObject *)self;
}

static void
context_dealloc(PyDecContextObject *self)
{
#ifndef WITH_DECIMAL_CONTEXTVAR
    if (self == cached_context) {
        cached_context = NULL;
    }
#endif

    Py_XDECREF(self->traps);
    Py_XDECREF(self->flags);
    Py_TYPE(self)->tp_free(self);
}

static int
context_init(PyObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {
      "prec", "rounding", "Emin", "Emax", "capitals", "clamp",
      "flags", "traps", NULL
    };
    PyObject *prec = Py_None;
    PyObject *rounding = Py_None;
    PyObject *emin = Py_None;
    PyObject *emax = Py_None;
    PyObject *capitals = Py_None;
    PyObject *clamp = Py_None;
    PyObject *status = Py_None;
    PyObject *traps = Py_None;
    int ret;

    assert(PyTuple_Check(args));

    if (!PyArg_ParseTupleAndKeywords(
            args, kwds,
            "|OOOOOOOO", kwlist,
            &prec, &rounding, &emin, &emax, &capitals, &clamp, &status, &traps
         )) {
        return -1;
    }

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

static PyObject *
context_repr(PyDecContextObject *self)
{
    mpd_context_t *ctx;
    char flags[MPD_MAX_SIGNAL_LIST];
    char traps[MPD_MAX_SIGNAL_LIST];
    int n, mem;

    assert(PyDecContext_Check(self));
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
         self->capitals, ctx->clamp, flags, traps);
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

#ifdef EXTRA_FUNCTIONALITY
/* Factory function for creating IEEE interchange format contexts */
static PyObject *
ieee_context(PyObject *dummy UNUSED, PyObject *v)
{
    PyObject *context;
    mpd_ssize_t bits;
    mpd_context_t ctx;

    bits = PyLong_AsSsize_t(v);
    if (bits == -1 && PyErr_Occurred()) {
        return NULL;
    }
    if (bits <= 0 || bits > INT_MAX) {
        goto error;
    }
    if (mpd_ieee_context(&ctx, (int)bits) < 0) {
        goto error;
    }

    context = PyObject_CallObject((PyObject *)&PyDecContext_Type, NULL);
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
#endif

static PyObject *
context_copy(PyObject *self, PyObject *args UNUSED)
{
    PyObject *copy;

    copy = PyObject_CallObject((PyObject *)&PyDecContext_Type, NULL);
    if (copy == NULL) {
        return NULL;
    }

    *CTX(copy) = *CTX(self);
    CTX(copy)->newtrap = 0;
    CtxCaps(copy) = CtxCaps(self);

    return copy;
}

static PyObject *
context_reduce(PyObject *self, PyObject *args UNUSED)
{
    PyObject *flags;
    PyObject *traps;
    PyObject *ret;
    mpd_context_t *ctx;

    ctx = CTX(self);

    flags = signals_as_list(ctx->status);
    if (flags == NULL) {
        return NULL;
    }
    traps = signals_as_list(ctx->traps);
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
  { "prec", (getter)context_getprec, (setter)context_setprec, NULL, NULL},
  { "Emax", (getter)context_getemax, (setter)context_setemax, NULL, NULL},
  { "Emin", (getter)context_getemin, (setter)context_setemin, NULL, NULL},
  { "rounding", (getter)context_getround, (setter)context_setround, NULL, NULL},
  { "capitals", (getter)context_getcapitals, (setter)context_setcapitals, NULL, NULL},
  { "clamp", (getter)context_getclamp, (setter)context_setclamp, NULL, NULL},
#ifdef EXTRA_FUNCTIONALITY
  { "_allcr", (getter)context_getallcr, (setter)context_setallcr, NULL, NULL},
  { "_traps", (getter)context_gettraps, (setter)context_settraps, NULL, NULL},
  { "_flags", (getter)context_getstatus, (setter)context_setstatus, NULL, NULL},
#endif
  {NULL}
};


#define CONTEXT_CHECK(obj) \
    if (!PyDecContext_Check(obj)) {        \
        PyErr_SetString(PyExc_TypeError,   \
            "argument must be a context"); \
        return NULL;                       \
    }

#define CONTEXT_CHECK_VA(obj) \
    if (obj == Py_None) {                           \
        CURRENT_CONTEXT(obj);                       \
    }                                               \
    else if (!PyDecContext_Check(obj)) {            \
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
current_context_from_dict(void)
{
    PyObject *dict;
    PyObject *tl_context;
    PyThreadState *tstate;

    dict = PyThreadState_GetDict();
    if (dict == NULL) {
        PyErr_SetString(PyExc_RuntimeError,
            "cannot get thread state");
        return NULL;
    }

    tl_context = PyDict_GetItemWithError(dict, tls_context_key);
    if (tl_context != NULL) {
        /* We already have a thread local context. */
        CONTEXT_CHECK(tl_context);
    }
    else {
        if (PyErr_Occurred()) {
            return NULL;
        }

        /* Set up a new thread local context. */
        tl_context = context_copy(default_context_template, NULL);
        if (tl_context == NULL) {
            return NULL;
        }
        CTX(tl_context)->status = 0;

        if (PyDict_SetItem(dict, tls_context_key, tl_context) < 0) {
            Py_DECREF(tl_context);
            return NULL;
        }
        Py_DECREF(tl_context);
    }

    /* Cache the context of the current thread, assuming that it
     * will be accessed several times before a thread switch. */
    tstate = PyThreadState_GET();
    if (tstate) {
        cached_context = (PyDecContextObject *)tl_context;
        cached_context->tstate = tstate;
    }

    /* Borrowed reference with refcount==1 */
    return tl_context;
}

/* Return borrowed reference to thread local context. */
static PyObject *
current_context(void)
{
    PyThreadState *tstate;

    tstate = PyThreadState_GET();
    if (cached_context && cached_context->tstate == tstate) {
        return (PyObject *)cached_context;
    }

    return current_context_from_dict();
}

/* ctxobj := borrowed reference to the current context */
#define CURRENT_CONTEXT(ctxobj) \
    ctxobj = current_context(); \
    if (ctxobj == NULL) {       \
        return NULL;            \
    }

/* Return a new reference to the current context */
static PyObject *
PyDec_GetCurrentContext(PyObject *self UNUSED, PyObject *args UNUSED)
{
    PyObject *context;

    context = current_context();
    if (context == NULL) {
        return NULL;
    }

    Py_INCREF(context);
    return context;
}

/* Set the thread local context to a new context, decrement old reference */
static PyObject *
PyDec_SetCurrentContext(PyObject *self UNUSED, PyObject *v)
{
    PyObject *dict;

    CONTEXT_CHECK(v);

    dict = PyThreadState_GetDict();
    if (dict == NULL) {
        PyErr_SetString(PyExc_RuntimeError,
            "cannot get thread state");
        return NULL;
    }

    /* If the new context is one of the templates, make a copy.
     * This is the current behavior of decimal.py. */
    if (v == default_context_template ||
        v == basic_context_template ||
        v == extended_context_template) {
        v = context_copy(v, NULL);
        if (v == NULL) {
            return NULL;
        }
        CTX(v)->status = 0;
    }
    else {
        Py_INCREF(v);
    }

    cached_context = NULL;
    if (PyDict_SetItem(dict, tls_context_key, v) < 0) {
        Py_DECREF(v);
        return NULL;
    }

    Py_DECREF(v);
    Py_RETURN_NONE;
}
#else
static PyObject *
init_current_context(void)
{
    PyObject *tl_context = context_copy(default_context_template, NULL);
    if (tl_context == NULL) {
        return NULL;
    }
    CTX(tl_context)->status = 0;

    PyObject *tok = PyContextVar_Set(current_context_var, tl_context);
    if (tok == NULL) {
        Py_DECREF(tl_context);
        return NULL;
    }
    Py_DECREF(tok);

    return tl_context;
}

static inline PyObject *
current_context(void)
{
    PyObject *tl_context;
    if (PyContextVar_Get(current_context_var, NULL, &tl_context) < 0) {
        return NULL;
    }

    if (tl_context != NULL) {
        return tl_context;
    }

    return init_current_context();
}

/* ctxobj := borrowed reference to the current context */
#define CURRENT_CONTEXT(ctxobj) \
    ctxobj = current_context(); \
    if (ctxobj == NULL) {       \
        return NULL;            \
    }                           \
    Py_DECREF(ctxobj);

/* Return a new reference to the current context */
static PyObject *
PyDec_GetCurrentContext(PyObject *self UNUSED, PyObject *args UNUSED)
{
    return current_context();
}

/* Set the thread local context to a new context, decrement old reference */
static PyObject *
PyDec_SetCurrentContext(PyObject *self UNUSED, PyObject *v)
{
    CONTEXT_CHECK(v);

    /* If the new context is one of the templates, make a copy.
     * This is the current behavior of decimal.py. */
    if (v == default_context_template ||
        v == basic_context_template ||
        v == extended_context_template) {
        v = context_copy(v, NULL);
        if (v == NULL) {
            return NULL;
        }
        CTX(v)->status = 0;
    }
    else {
        Py_INCREF(v);
    }

    PyObject *tok = PyContextVar_Set(current_context_var, v);
    Py_DECREF(v);
    if (tok == NULL) {
        return NULL;
    }
    Py_DECREF(tok);

    Py_RETURN_NONE;
}
#endif

/* Context manager object for the 'with' statement. The manager
 * owns one reference to the global (outer) context and one
 * to the local (inner) context. */
static PyObject *
ctxmanager_new(PyTypeObject *type UNUSED, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"ctx", NULL};
    PyDecContextManagerObject *self;
    PyObject *local = Py_None;
    PyObject *global;

    CURRENT_CONTEXT(global);
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O", kwlist, &local)) {
        return NULL;
    }
    if (local == Py_None) {
        local = global;
    }
    else if (!PyDecContext_Check(local)) {
        PyErr_SetString(PyExc_TypeError,
            "optional argument must be a context");
        return NULL;
    }

    self = PyObject_New(PyDecContextManagerObject,
                        &PyDecContextManager_Type);
    if (self == NULL) {
        return NULL;
    }

    self->local = context_copy(local, NULL);
    if (self->local == NULL) {
        self->global = NULL;
        Py_DECREF(self);
        return NULL;
    }
    self->global = global;
    Py_INCREF(self->global);

    return (PyObject *)self;
}

static void
ctxmanager_dealloc(PyDecContextManagerObject *self)
{
    Py_XDECREF(self->local);
    Py_XDECREF(self->global);
    PyObject_Del(self);
}

static PyObject *
ctxmanager_set_local(PyDecContextManagerObject *self, PyObject *args UNUSED)
{
    PyObject *ret;

    ret = PyDec_SetCurrentContext(NULL, self->local);
    if (ret == NULL) {
        return NULL;
    }
    Py_DECREF(ret);

    Py_INCREF(self->local);
    return self->local;
}

static PyObject *
ctxmanager_restore_global(PyDecContextManagerObject *self,
                          PyObject *args UNUSED)
{
    PyObject *ret;

    ret = PyDec_SetCurrentContext(NULL, self->global);
    if (ret == NULL) {
        return NULL;
    }
    Py_DECREF(ret);

    Py_RETURN_NONE;
}


static PyMethodDef ctxmanager_methods[] = {
  {"__enter__", (PyCFunction)ctxmanager_set_local, METH_NOARGS, NULL},
  {"__exit__", (PyCFunction)ctxmanager_restore_global, METH_VARARGS, NULL},
  {NULL, NULL}
};

static PyTypeObject PyDecContextManager_Type =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "decimal.ContextManager",               /* tp_name */
    sizeof(PyDecContextManagerObject),      /* tp_basicsize */
    0,                                      /* tp_itemsize */
    (destructor) ctxmanager_dealloc,        /* tp_dealloc */
    0,                                      /* tp_vectorcall_offset */
    (getattrfunc) 0,                        /* tp_getattr */
    (setattrfunc) 0,                        /* tp_setattr */
    0,                                      /* tp_as_async */
    (reprfunc) 0,                           /* tp_repr */
    0,                                      /* tp_as_number */
    0,                                      /* tp_as_sequence */
    0,                                      /* tp_as_mapping */
    0,                                      /* tp_hash */
    0,                                      /* tp_call */
    0,                                      /* tp_str */
    (getattrofunc) PyObject_GenericGetAttr, /* tp_getattro */
    (setattrofunc) 0,                       /* tp_setattro */
    (PyBufferProcs *) 0,                    /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                     /* tp_flags */
    0,                                      /* tp_doc */
    0,                                      /* tp_traverse */
    0,                                      /* tp_clear */
    0,                                      /* tp_richcompare */
    0,                                      /* tp_weaklistoffset */
    0,                                      /* tp_iter */
    0,                                      /* tp_iternext */
    ctxmanager_methods,                     /* tp_methods */
};


/******************************************************************************/
/*                           New Decimal Object                               */
/******************************************************************************/

static PyObject *
PyDecType_New(PyTypeObject *type)
{
    PyDecObject *dec;

    if (type == &PyDec_Type) {
        dec = PyObject_New(PyDecObject, &PyDec_Type);
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

    return (PyObject *)dec;
}
#define dec_alloc() PyDecType_New(&PyDec_Type)

static void
dec_dealloc(PyObject *dec)
{
    mpd_del(MPD(dec));
    Py_TYPE(dec)->tp_free(dec);
}


/******************************************************************************/
/*                           Conversions to Decimal                           */
/******************************************************************************/

Py_LOCAL_INLINE(int)
is_space(enum PyUnicode_Kind kind, const void *data, Py_ssize_t pos)
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
numeric_as_ascii(const PyObject *u, int strip_ws, int ignore_underscores)
{
    enum PyUnicode_Kind kind;
    const void *data;
    Py_UCS4 ch;
    char *res, *cp;
    Py_ssize_t j, len;
    int d;

    if (PyUnicode_READY(u) == -1) {
        return NULL;
    }

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

    dec = PyDecType_New(type);
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

    dec = PyDecType_New(type);
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
PyDecType_FromUnicode(PyTypeObject *type, const PyObject *u,
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
PyDecType_FromUnicodeExactWS(PyTypeObject *type, const PyObject *u,
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

    dec = PyDecType_New(type);
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

    dec = PyDecType_New(type);
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
dec_from_long(PyTypeObject *type, const PyObject *v,
              const mpd_context_t *ctx, uint32_t *status)
{
    PyObject *dec;
    PyLongObject *l = (PyLongObject *)v;
    Py_ssize_t ob_size;
    size_t len;
    uint8_t sign;

    dec = PyDecType_New(type);
    if (dec == NULL) {
        return NULL;
    }

    ob_size = Py_SIZE(l);
    if (ob_size == 0) {
        _dec_settriple(dec, MPD_POS, 0, 0);
        return dec;
    }

    if (ob_size < 0) {
        len = -ob_size;
        sign = MPD_NEG;
    }
    else {
        len = ob_size;
        sign = MPD_POS;
    }

    if (len == 1) {
        _dec_settriple(dec, sign, *l->ob_digit, 0);
        mpd_qfinalize(MPD(dec), ctx, status);
        return dec;
    }

#if PYLONG_BITS_IN_DIGIT == 30
    mpd_qimport_u32(MPD(dec), l->ob_digit, len, sign, PyLong_BASE,
                    ctx, status);
#elif PYLONG_BITS_IN_DIGIT == 15
    mpd_qimport_u16(MPD(dec), l->ob_digit, len, sign, PyLong_BASE,
                    ctx, status);
#else
  #error "PYLONG_BITS_IN_DIGIT should be 15 or 30"
#endif

    return dec;
}

/* Return a new PyDecObject from a PyLongObject. Use the context for
   conversion. */
static PyObject *
PyDecType_FromLong(PyTypeObject *type, const PyObject *v, PyObject *context)
{
    PyObject *dec;
    uint32_t status = 0;

    if (!PyLong_Check(v)) {
        PyErr_SetString(PyExc_TypeError, "argument must be an integer");
        return NULL;
    }

    dec = dec_from_long(type, v, CTX(context), &status);
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
PyDecType_FromLongExact(PyTypeObject *type, const PyObject *v,
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
    dec = dec_from_long(type, v, &maxctx, &status);
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

/* External C-API functions */
static binaryfunc _py_long_multiply;
static binaryfunc _py_long_floor_divide;
static ternaryfunc _py_long_power;
static unaryfunc _py_float_abs;
static PyCFunction _py_long_bit_length;
static PyCFunction _py_float_as_integer_ratio;

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


    assert(PyType_IsSubtype(type, &PyDec_Type));

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

    if (Py_IS_NAN(x) || Py_IS_INFINITY(x)) {
        dec = PyDecType_New(type);
        if (dec == NULL) {
            return NULL;
        }
        if (Py_IS_NAN(x)) {
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
    tmp = _py_float_abs(v);
    if (tmp == NULL) {
        return NULL;
    }

    /* float as integer ratio: numerator/denominator */
    n_d = _py_float_as_integer_ratio(tmp, NULL);
    Py_DECREF(tmp);
    if (n_d == NULL) {
        return NULL;
    }
    n = PyTuple_GET_ITEM(n_d, 0);
    d = PyTuple_GET_ITEM(n_d, 1);

    tmp = _py_long_bit_length(d, NULL);
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

    if (type == &PyDec_Type && PyDec_CheckExact(v)) {
        Py_INCREF(v);
        return v;
    }

    dec = PyDecType_New(type);
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
        Py_INCREF(v);
        return v;
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

#define PyDec_FromCString(str, context) \
        PyDecType_FromCString(&PyDec_Type, str, context)
#define PyDec_FromCStringExact(str, context) \
        PyDecType_FromCStringExact(&PyDec_Type, str, context)

#define PyDec_FromUnicode(unicode, context) \
        PyDecType_FromUnicode(&PyDec_Type, unicode, context)
#define PyDec_FromUnicodeExact(unicode, context) \
        PyDecType_FromUnicodeExact(&PyDec_Type, unicode, context)
#define PyDec_FromUnicodeExactWS(unicode, context) \
        PyDecType_FromUnicodeExactWS(&PyDec_Type, unicode, context)

#define PyDec_FromSsize(v, context) \
        PyDecType_FromSsize(&PyDec_Type, v, context)
#define PyDec_FromSsizeExact(v, context) \
        PyDecType_FromSsizeExact(&PyDec_Type, v, context)

#define PyDec_FromLong(pylong, context) \
        PyDecType_FromLong(&PyDec_Type, pylong, context)
#define PyDec_FromLongExact(pylong, context) \
        PyDecType_FromLongExact(&PyDec_Type, pylong, context)

#define PyDec_FromFloat(pyfloat, context) \
        PyDecType_FromFloat(&PyDec_Type, pyfloat, context)
#define PyDec_FromFloatExact(pyfloat, context) \
        PyDecType_FromFloatExact(&PyDec_Type, pyfloat, context)

#define PyDec_FromSequence(sequence, context) \
        PyDecType_FromSequence(&PyDec_Type, sequence, context)
#define PyDec_FromSequenceExact(sequence, context) \
        PyDecType_FromSequenceExact(&PyDec_Type, sequence, context)

/* class method */
static PyObject *
dec_from_float(PyObject *type, PyObject *pyfloat)
{
    PyObject *context;
    PyObject *result;

    CURRENT_CONTEXT(context);
    result = PyDecType_FromFloatExact(&PyDec_Type, pyfloat, context);
    if (type != (PyObject *)&PyDec_Type && result != NULL) {
        Py_SETREF(result, PyObject_CallFunctionObjArgs(type, result, NULL));
    }

    return result;
}

/* create_decimal_from_float */
static PyObject *
ctx_from_float(PyObject *context, PyObject *v)
{
    return PyDec_FromFloat(v, context);
}

/* Apply the context to the input operand. Return a new PyDecObject. */
static PyObject *
dec_apply(PyObject *v, PyObject *context)
{
    PyObject *result;
    uint32_t status = 0;

    result = dec_alloc();
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
    if (v == NULL) {
        return PyDecType_FromSsizeExact(type, 0, context);
    }
    else if (PyDec_Check(v)) {
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
    if (v == NULL) {
        return PyDec_FromSsize(0, context);
    }
    else if (PyDec_Check(v)) {
        mpd_context_t *ctx = CTX(context);
        if (mpd_isnan(MPD(v)) &&
            MPD(v)->digits > ctx->prec - ctx->clamp) {
            /* Special case: too many NaN payload digits */
            PyObject *result;
            if (dec_addstatus(context, MPD_Conversion_syntax)) {
                return NULL;
            }
            result = dec_alloc();
            if (result == NULL) {
                return NULL;
            }
            mpd_setspecial(MPD(result), MPD_POS, MPD_NAN);
            return result;
        }
        return dec_apply(v, context);
    }
    else if (PyUnicode_Check(v)) {
        return PyDec_FromUnicode(v, context);
    }
    else if (PyLong_Check(v)) {
        return PyDec_FromLong(v, context);
    }
    else if (PyTuple_Check(v) || PyList_Check(v)) {
        return PyDec_FromSequence(v, context);
    }
    else if (PyFloat_Check(v)) {
        if (dec_addstatus(context, MPD_Float_operation)) {
            return NULL;
        }
        return PyDec_FromFloat(v, context);
    }
    else {
        PyErr_Format(PyExc_TypeError,
            "conversion from %s to Decimal is not supported",
            Py_TYPE(v)->tp_name);
        return NULL;
    }
}

static PyObject *
dec_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"value", "context", NULL};
    PyObject *v = NULL;
    PyObject *context = Py_None;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|OO", kwlist,
                                     &v, &context)) {
        return NULL;
    }
    CONTEXT_CHECK_VA(context);

    return PyDecType_FromObjectExact(type, v, context);
}

static PyObject *
ctx_create_decimal(PyObject *context, PyObject *args)
{
    PyObject *v = NULL;

    if (!PyArg_ParseTuple(args, "|O", &v)) {
        return NULL;
    }

    return PyDec_FromObject(v, context);
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

    if (PyDec_Check(v)) {
        *conv = v;
        Py_INCREF(v);
        return 1;
    }
    if (PyLong_Check(v)) {
        *conv = PyDec_FromLongExact(v, context);
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
        Py_INCREF(Py_NotImplemented);
        *conv = Py_NotImplemented;
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

/* Convert rationals for comparison */
static PyObject *Rational = NULL;
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
    denom = PyDec_FromLongExact(tmp, context);
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
    result = dec_alloc();
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

    num = PyDec_FromLongExact(tmp, context);
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

    if (PyDec_Check(w)) {
        Py_INCREF(w);
        *wcmp = w;
    }
    else if (PyLong_Check(w)) {
        *wcmp = PyDec_FromLongExact(w, context);
    }
    else if (PyFloat_Check(w)) {
        if (op != Py_EQ && op != Py_NE &&
            dec_addstatus(context, MPD_Float_operation)) {
            *wcmp = NULL;
        }
        else {
            ctx->status |= MPD_Float_operation;
            *wcmp = PyDec_FromFloatExact(w, context);
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
                *wcmp = PyDec_FromFloatExact(tmp, context);
                Py_DECREF(tmp);
            }
        }
        else {
            Py_INCREF(Py_NotImplemented);
            *wcmp = Py_NotImplemented;
        }
    }
    else {
        int is_rational = PyObject_IsInstance(w, Rational);
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
            Py_INCREF(Py_NotImplemented);
            *wcmp = Py_NotImplemented;
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

    CURRENT_CONTEXT(context);
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

    CURRENT_CONTEXT(context);
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

/* Formatted representation of a PyDecObject. */
static PyObject *
dec_format(PyObject *dec, PyObject *args)
{
    PyObject *result = NULL;
    PyObject *override = NULL;
    PyObject *dot = NULL;
    PyObject *sep = NULL;
    PyObject *grouping = NULL;
    PyObject *fmtarg;
    PyObject *context;
    mpd_spec_t spec;
    char *fmt;
    char *decstring = NULL;
    uint32_t status = 0;
    int replace_fillchar = 0;
    Py_ssize_t size;


    CURRENT_CONTEXT(context);
    if (!PyArg_ParseTuple(args, "O|O", &fmtarg, &override)) {
        return NULL;
    }

    if (PyUnicode_Check(fmtarg)) {
        fmt = (char *)PyUnicode_AsUTF8AndSize(fmtarg, &size);
        if (fmt == NULL) {
            return NULL;
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
    }
    else {
        PyErr_SetString(PyExc_TypeError,
            "format arg must be str");
        return NULL;
    }

    if (!mpd_parse_fmt_str(&spec, fmt, CtxCaps(context))) {
        PyErr_SetString(PyExc_ValueError,
            "invalid format string");
        goto finish;
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
        if ((dot = PyDict_GetItemString(override, "decimal_point"))) {
            if ((dot = PyUnicode_AsUTF8String(dot)) == NULL) {
                goto finish;
            }
            spec.dot = PyBytes_AS_STRING(dot);
        }
        if ((sep = PyDict_GetItemString(override, "thousands_sep"))) {
            if ((sep = PyUnicode_AsUTF8String(sep)) == NULL) {
                goto finish;
            }
            spec.sep = PyBytes_AS_STRING(sep);
        }
        if ((grouping = PyDict_GetItemString(override, "grouping"))) {
            if ((grouping = PyUnicode_AsUTF8String(grouping)) == NULL) {
                goto finish;
            }
            spec.grouping = PyBytes_AS_STRING(grouping);
        }
        if (mpd_validate_lconv(&spec) < 0) {
            PyErr_SetString(PyExc_ValueError,
                "invalid override dict");
            goto finish;
        }
    }
    else {
        size_t n = strlen(spec.dot);
        if (n > 1 || (n == 1 && !isascii((uchar)spec.dot[0]))) {
            /* fix locale dependent non-ascii characters */
            dot = dotsep_as_utf8(spec.dot);
            if (dot == NULL) {
                goto finish;
            }
            spec.dot = PyBytes_AS_STRING(dot);
        }
        n = strlen(spec.sep);
        if (n > 1 || (n == 1 && !isascii((uchar)spec.sep[0]))) {
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
    PyLongObject *pylong;
    digit *ob_digit;
    size_t n;
    Py_ssize_t i;
    mpd_t *x;
    mpd_context_t workctx;
    uint32_t status = 0;

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

    x = mpd_qnew();
    if (x == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    workctx = *CTX(context);
    workctx.round = round;
    mpd_qround_to_int(x, MPD(dec), &workctx, &status);
    if (dec_addstatus(context, status)) {
        mpd_del(x);
        return NULL;
    }

    status = 0;
    ob_digit = NULL;
#if PYLONG_BITS_IN_DIGIT == 30
    n = mpd_qexport_u32(&ob_digit, 0, PyLong_BASE, x, &status);
#elif PYLONG_BITS_IN_DIGIT == 15
    n = mpd_qexport_u16(&ob_digit, 0, PyLong_BASE, x, &status);
#else
    #error "PYLONG_BITS_IN_DIGIT should be 15 or 30"
#endif

    if (n == SIZE_MAX) {
        PyErr_NoMemory();
        mpd_del(x);
        return NULL;
    }

    assert(n > 0);
    pylong = _PyLong_New(n);
    if (pylong == NULL) {
        mpd_free(ob_digit);
        mpd_del(x);
        return NULL;
    }

    memcpy(pylong->ob_digit, ob_digit, n * sizeof(digit));
    mpd_free(ob_digit);

    i = n;
    while ((i > 0) && (pylong->ob_digit[i-1] == 0)) {
        i--;
    }

    Py_SET_SIZE(pylong, i);
    if (mpd_isnegative(x) && !mpd_iszero(x)) {
        Py_SET_SIZE(pylong, -i);
    }

    mpd_del(x);
    return (PyObject *) pylong;
}

/* Convert a Decimal to its exact integer ratio representation. */
static PyObject *
dec_as_integer_ratio(PyObject *self, PyObject *args UNUSED)
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

    CURRENT_CONTEXT(context);

    tmp = dec_alloc();
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

    Py_SETREF(exponent, _py_long_power(tmp, exponent, Py_None));
    Py_DECREF(tmp);
    if (exponent == NULL) {
        goto error;
    }

    if (exp >= 0) {
        Py_SETREF(numerator, _py_long_multiply(numerator, exponent));
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
        Py_SETREF(numerator, _py_long_floor_divide(numerator, tmp));
        Py_SETREF(denominator, _py_long_floor_divide(denominator, tmp));
        Py_DECREF(tmp);
        if (numerator == NULL || denominator == NULL) {
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

static PyObject *
PyDec_ToIntegralValue(PyObject *dec, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"rounding", "context", NULL};
    PyObject *result;
    PyObject *rounding = Py_None;
    PyObject *context = Py_None;
    uint32_t status = 0;
    mpd_context_t workctx;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|OO", kwlist,
                                     &rounding, &context)) {
        return NULL;
    }
    CONTEXT_CHECK_VA(context);

    workctx = *CTX(context);
    if (rounding != Py_None) {
        int round = getround(rounding);
        if (round < 0) {
            return NULL;
        }
        if (!mpd_qsetround(&workctx, round)) {
            INTERNAL_ERROR_PTR("PyDec_ToIntegralValue"); /* GCOV_NOT_REACHED */
        }
    }

    result = dec_alloc();
    if (result == NULL) {
        return NULL;
    }

    mpd_qround_to_int(MPD(result), MPD(dec), &workctx, &status);
    if (dec_addstatus(context, status)) {
        Py_DECREF(result);
        return NULL;
    }

    return result;
}

static PyObject *
PyDec_ToIntegralExact(PyObject *dec, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"rounding", "context", NULL};
    PyObject *result;
    PyObject *rounding = Py_None;
    PyObject *context = Py_None;
    uint32_t status = 0;
    mpd_context_t workctx;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|OO", kwlist,
                                     &rounding, &context)) {
        return NULL;
    }
    CONTEXT_CHECK_VA(context);

    workctx = *CTX(context);
    if (rounding != Py_None) {
        int round = getround(rounding);
        if (round < 0) {
            return NULL;
        }
        if (!mpd_qsetround(&workctx, round)) {
            INTERNAL_ERROR_PTR("PyDec_ToIntegralExact"); /* GCOV_NOT_REACHED */
        }
    }

    result = dec_alloc();
    if (result == NULL) {
        return NULL;
    }

    mpd_qround_to_intx(MPD(result), MPD(dec), &workctx, &status);
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

static PyObject *
PyDec_Round(PyObject *dec, PyObject *args)
{
    PyObject *result;
    PyObject *x = NULL;
    uint32_t status = 0;
    PyObject *context;


    CURRENT_CONTEXT(context);
    if (!PyArg_ParseTuple(args, "|O", &x)) {
        return NULL;
    }

    if (x) {
        mpd_uint_t dq[1] = {1};
        mpd_t q = {MPD_STATIC|MPD_CONST_DATA,0,1,1,1,dq};
        mpd_ssize_t y;

        if (!PyLong_Check(x)) {
            PyErr_SetString(PyExc_TypeError,
                "optional arg must be an integer");
            return NULL;
        }

        y = PyLong_AsSsize_t(x);
        if (y == -1 && PyErr_Occurred()) {
            return NULL;
        }
        result = dec_alloc();
        if (result == NULL) {
            return NULL;
        }

        q.exp = (y == MPD_SSIZE_MIN) ? MPD_SSIZE_MAX : -y;
        mpd_qquantize(MPD(result), MPD(dec), &q, CTX(context), &status);
        if (dec_addstatus(context, status)) {
            Py_DECREF(result);
            return NULL;
        }

        return result;
    }
    else {
        return dec_as_long(dec, context, MPD_ROUND_HALF_EVEN);
    }
}

static PyTypeObject *DecimalTuple = NULL;
/* Return the DecimalTuple representation of a PyDecObject. */
static PyObject *
PyDec_AsTuple(PyObject *dec, PyObject *dummy UNUSED)
{
    PyObject *result = NULL;
    PyObject *sign = NULL;
    PyObject *coeff = NULL;
    PyObject *expt = NULL;
    PyObject *tmp = NULL;
    mpd_t *x = NULL;
    char *intstring = NULL;
    Py_ssize_t intlen, i;


    x = mpd_qncopy(MPD(dec));
    if (x == NULL) {
        PyErr_NoMemory();
        goto out;
    }

    sign = PyLong_FromUnsignedLong(mpd_sign(MPD(dec)));
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
            expt = PyLong_FromSsize_t(MPD(dec)->exp);
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

    result = PyObject_CallFunctionObjArgs((PyObject *)DecimalTuple,
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
    CURRENT_CONTEXT(context);                               \
    if ((result = dec_alloc()) == NULL) {                   \
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
    CURRENT_CONTEXT(context) ;                                   \
    CONVERT_BINOP(&a, &b, self, other, context);                 \
                                                                 \
    if ((result = dec_alloc()) == NULL) {                        \
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

/* Boolean function without a context arg. */
#define Dec_BoolFunc(MPDFUNC) \
static PyObject *                                           \
dec_##MPDFUNC(PyObject *self, PyObject *dummy UNUSED)       \
{                                                           \
    return MPDFUNC(MPD(self)) ? incr_true() : incr_false(); \
}

/* Boolean function with an optional context arg. */
#define Dec_BoolFuncVA(MPDFUNC) \
static PyObject *                                                         \
dec_##MPDFUNC(PyObject *self, PyObject *args, PyObject *kwds)             \
{                                                                         \
    static char *kwlist[] = {"context", NULL};                            \
    PyObject *context = Py_None;                                          \
                                                                          \
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O", kwlist,            \
                                     &context)) {                         \
        return NULL;                                                      \
    }                                                                     \
    CONTEXT_CHECK_VA(context);                                            \
                                                                          \
    return MPDFUNC(MPD(self), CTX(context)) ? incr_true() : incr_false(); \
}

/* Unary function with an optional context arg. */
#define Dec_UnaryFuncVA(MPDFUNC) \
static PyObject *                                              \
dec_##MPDFUNC(PyObject *self, PyObject *args, PyObject *kwds)  \
{                                                              \
    static char *kwlist[] = {"context", NULL};                 \
    PyObject *result;                                          \
    PyObject *context = Py_None;                               \
    uint32_t status = 0;                                       \
                                                               \
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O", kwlist, \
                                     &context)) {              \
        return NULL;                                           \
    }                                                          \
    CONTEXT_CHECK_VA(context);                                 \
                                                               \
    if ((result = dec_alloc()) == NULL) {                      \
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

/* Binary function with an optional context arg. */
#define Dec_BinaryFuncVA(MPDFUNC) \
static PyObject *                                                \
dec_##MPDFUNC(PyObject *self, PyObject *args, PyObject *kwds)    \
{                                                                \
    static char *kwlist[] = {"other", "context", NULL};          \
    PyObject *other;                                             \
    PyObject *a, *b;                                             \
    PyObject *result;                                            \
    PyObject *context = Py_None;                                 \
    uint32_t status = 0;                                         \
                                                                 \
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|O", kwlist,  \
                                     &other, &context)) {        \
        return NULL;                                             \
    }                                                            \
    CONTEXT_CHECK_VA(context);                                   \
    CONVERT_BINOP_RAISE(&a, &b, self, other, context);           \
                                                                 \
    if ((result = dec_alloc()) == NULL) {                        \
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
   if the second operand cannot be converted exactly. */
#define Dec_BinaryFuncVA_NO_CTX(MPDFUNC) \
static PyObject *                                               \
dec_##MPDFUNC(PyObject *self, PyObject *args, PyObject *kwds)   \
{                                                               \
    static char *kwlist[] = {"other", "context", NULL};         \
    PyObject *context = Py_None;                                \
    PyObject *other;                                            \
    PyObject *a, *b;                                            \
    PyObject *result;                                           \
                                                                \
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|O", kwlist, \
                                     &other, &context)) {       \
        return NULL;                                            \
    }                                                           \
    CONTEXT_CHECK_VA(context);                                  \
    CONVERT_BINOP_RAISE(&a, &b, self, other, context);          \
                                                                \
    if ((result = dec_alloc()) == NULL) {                       \
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

/* Ternary function with an optional context arg. */
#define Dec_TernaryFuncVA(MPDFUNC) \
static PyObject *                                                        \
dec_##MPDFUNC(PyObject *self, PyObject *args, PyObject *kwds)            \
{                                                                        \
    static char *kwlist[] = {"other", "third", "context", NULL};         \
    PyObject *other, *third;                                             \
    PyObject *a, *b, *c;                                                 \
    PyObject *result;                                                    \
    PyObject *context = Py_None;                                         \
    uint32_t status = 0;                                                 \
                                                                         \
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "OO|O", kwlist,         \
                                     &other, &third, &context)) {        \
        return NULL;                                                     \
    }                                                                    \
    CONTEXT_CHECK_VA(context);                                           \
    CONVERT_TERNOP_RAISE(&a, &b, &c, self, other, third, context);       \
                                                                         \
    if ((result = dec_alloc()) == NULL) {                                \
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

    CURRENT_CONTEXT(context);
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

    CURRENT_CONTEXT(context);
    CONVERT_BINOP(&a, &b, v, w, context);

    q = dec_alloc();
    if (q == NULL) {
        Py_DECREF(a);
        Py_DECREF(b);
        return NULL;
    }
    r = dec_alloc();
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

    ret = Py_BuildValue("(OO)", q, r);
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

    CURRENT_CONTEXT(context);
    CONVERT_BINOP(&a, &b, base, exp, context);

    if (mod != Py_None) {
        if (!convert_op(NOT_IMPL, &c, mod, context)) {
            Py_DECREF(a);
            Py_DECREF(b);
            return c;
        }
    }

    result = dec_alloc();
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
Dec_UnaryFuncVA(mpd_qexp)
Dec_UnaryFuncVA(mpd_qln)
Dec_UnaryFuncVA(mpd_qlog10)
Dec_UnaryFuncVA(mpd_qnext_minus)
Dec_UnaryFuncVA(mpd_qnext_plus)
Dec_UnaryFuncVA(mpd_qreduce)
Dec_UnaryFuncVA(mpd_qsqrt)

/* Binary arithmetic functions, optional context arg */
Dec_BinaryFuncVA(mpd_qcompare)
Dec_BinaryFuncVA(mpd_qcompare_signal)
Dec_BinaryFuncVA(mpd_qmax)
Dec_BinaryFuncVA(mpd_qmax_mag)
Dec_BinaryFuncVA(mpd_qmin)
Dec_BinaryFuncVA(mpd_qmin_mag)
Dec_BinaryFuncVA(mpd_qnext_toward)
Dec_BinaryFuncVA(mpd_qrem_near)

/* Ternary arithmetic functions, optional context arg */
Dec_TernaryFuncVA(mpd_qfma)

/* Boolean functions, no context arg */
Dec_BoolFunc(mpd_iscanonical)
Dec_BoolFunc(mpd_isfinite)
Dec_BoolFunc(mpd_isinfinite)
Dec_BoolFunc(mpd_isnan)
Dec_BoolFunc(mpd_isqnan)
Dec_BoolFunc(mpd_issnan)
Dec_BoolFunc(mpd_issigned)
Dec_BoolFunc(mpd_iszero)

/* Boolean functions, optional context arg */
Dec_BoolFuncVA(mpd_isnormal)
Dec_BoolFuncVA(mpd_issubnormal)

/* Unary functions, no context arg */
static PyObject *
dec_mpd_adjexp(PyObject *self, PyObject *dummy UNUSED)
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

static PyObject *
dec_canonical(PyObject *self, PyObject *dummy UNUSED)
{
    Py_INCREF(self);
    return self;
}

static PyObject *
dec_conjugate(PyObject *self, PyObject *dummy UNUSED)
{
    Py_INCREF(self);
    return self;
}

static PyObject *
dec_mpd_radix(PyObject *self UNUSED, PyObject *dummy UNUSED)
{
    PyObject *result;

    result = dec_alloc();
    if (result == NULL) {
        return NULL;
    }

    _dec_settriple(result, MPD_POS, 10, 0);
    return result;
}

static PyObject *
dec_mpd_qcopy_abs(PyObject *self, PyObject *dummy UNUSED)
{
    PyObject *result;
    uint32_t status = 0;

    if ((result = dec_alloc()) == NULL) {
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

static PyObject *
dec_mpd_qcopy_negate(PyObject *self, PyObject *dummy UNUSED)
{
    PyObject *result;
    uint32_t status = 0;

    if ((result = dec_alloc()) == NULL) {
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
Dec_UnaryFuncVA(mpd_qinvert)
Dec_UnaryFuncVA(mpd_qlogb)

static PyObject *
dec_mpd_class(PyObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"context", NULL};
    PyObject *context = Py_None;
    const char *cp;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O", kwlist,
                                     &context)) {
        return NULL;
    }
    CONTEXT_CHECK_VA(context);

    cp = mpd_class(MPD(self), CTX(context));
    return PyUnicode_FromString(cp);
}

static PyObject *
dec_mpd_to_eng(PyObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"context", NULL};
    PyObject *result;
    PyObject *context = Py_None;
    mpd_ssize_t size;
    char *s;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O", kwlist,
                                     &context)) {
        return NULL;
    }
    CONTEXT_CHECK_VA(context);

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
Dec_BinaryFuncVA_NO_CTX(mpd_compare_total)
Dec_BinaryFuncVA_NO_CTX(mpd_compare_total_mag)

static PyObject *
dec_mpd_qcopy_sign(PyObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"other", "context", NULL};
    PyObject *other;
    PyObject *a, *b;
    PyObject *result;
    PyObject *context = Py_None;
    uint32_t status = 0;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|O", kwlist,
                                     &other, &context)) {
        return NULL;
    }
    CONTEXT_CHECK_VA(context);
    CONVERT_BINOP_RAISE(&a, &b, self, other, context);

    result = dec_alloc();
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

static PyObject *
dec_mpd_same_quantum(PyObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"other", "context", NULL};
    PyObject *other;
    PyObject *a, *b;
    PyObject *result;
    PyObject *context = Py_None;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|O", kwlist,
                                     &other, &context)) {
        return NULL;
    }
    CONTEXT_CHECK_VA(context);
    CONVERT_BINOP_RAISE(&a, &b, self, other, context);

    result = mpd_same_quantum(MPD(a), MPD(b)) ? incr_true() : incr_false();
    Py_DECREF(a);
    Py_DECREF(b);

    return result;
}

/* Binary functions, optional context arg */
Dec_BinaryFuncVA(mpd_qand)
Dec_BinaryFuncVA(mpd_qor)
Dec_BinaryFuncVA(mpd_qxor)

Dec_BinaryFuncVA(mpd_qrotate)
Dec_BinaryFuncVA(mpd_qscaleb)
Dec_BinaryFuncVA(mpd_qshift)

static PyObject *
dec_mpd_qquantize(PyObject *v, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"exp", "rounding", "context", NULL};
    PyObject *rounding = Py_None;
    PyObject *context = Py_None;
    PyObject *w, *a, *b;
    PyObject *result;
    uint32_t status = 0;
    mpd_context_t workctx;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|OO", kwlist,
                                     &w, &rounding, &context)) {
        return NULL;
    }
    CONTEXT_CHECK_VA(context);

    workctx = *CTX(context);
    if (rounding != Py_None) {
        int round = getround(rounding);
        if (round < 0) {
            return NULL;
        }
        if (!mpd_qsetround(&workctx, round)) {
            INTERNAL_ERROR_PTR("dec_mpd_qquantize"); /* GCOV_NOT_REACHED */
        }
    }

    CONVERT_BINOP_RAISE(&a, &b, v, w, context);

    result = dec_alloc();
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

    assert(PyDec_Check(v));

    CURRENT_CONTEXT(context);
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

/* __ceil__ */
static PyObject *
dec_ceil(PyObject *self, PyObject *dummy UNUSED)
{
    PyObject *context;

    CURRENT_CONTEXT(context);
    return dec_as_long(self, context, MPD_ROUND_CEILING);
}

/* __complex__ */
static PyObject *
dec_complex(PyObject *self, PyObject *dummy UNUSED)
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

/* __copy__ and __deepcopy__ */
static PyObject *
dec_copy(PyObject *self, PyObject *dummy UNUSED)
{
    Py_INCREF(self);
    return self;
}

/* __floor__ */
static PyObject *
dec_floor(PyObject *self, PyObject *dummy UNUSED)
{
    PyObject *context;

    CURRENT_CONTEXT(context);
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
    const Py_hash_t py_hash_nan = 0;
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
            return py_hash_nan;
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
dec_hash(PyDecObject *self)
{
    if (self->hash == -1) {
        self->hash = _dec_hash(self);
    }

    return self->hash;
}

/* __reduce__ */
static PyObject *
dec_reduce(PyObject *self, PyObject *dummy UNUSED)
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

/* __sizeof__ */
static PyObject *
dec_sizeof(PyObject *v, PyObject *dummy UNUSED)
{
    Py_ssize_t res;

    res = _PyObject_SIZE(Py_TYPE(v));
    if (mpd_isdynamic_data(MPD(v))) {
        res += MPD(v)->alloc * sizeof(mpd_uint_t);
    }
    return PyLong_FromSsize_t(res);
}

/* __trunc__ */
static PyObject *
dec_trunc(PyObject *self, PyObject *dummy UNUSED)
{
    PyObject *context;

    CURRENT_CONTEXT(context);
    return dec_as_long(self, context, MPD_ROUND_DOWN);
}

/* real and imag */
static PyObject *
dec_real(PyObject *self, void *closure UNUSED)
{
    Py_INCREF(self);
    return self;
}

static PyObject *
dec_imag(PyObject *self UNUSED, void *closure UNUSED)
{
    PyObject *result;

    result = dec_alloc();
    if (result == NULL) {
        return NULL;
    }

    _dec_settriple(result, MPD_POS, 0, 0);
    return result;
}


static PyGetSetDef dec_getsets [] =
{
  { "real", (getter)dec_real, NULL, NULL, NULL},
  { "imag", (getter)dec_imag, NULL, NULL, NULL},
  {NULL}
};

static PyNumberMethods dec_number_methods =
{
    (binaryfunc) nm_mpd_qadd,
    (binaryfunc) nm_mpd_qsub,
    (binaryfunc) nm_mpd_qmul,
    (binaryfunc) nm_mpd_qrem,
    (binaryfunc) nm_mpd_qdivmod,
    (ternaryfunc) nm_mpd_qpow,
    (unaryfunc) nm_mpd_qminus,
    (unaryfunc) nm_mpd_qplus,
    (unaryfunc) nm_mpd_qabs,
    (inquiry) nm_nonzero,
    (unaryfunc) 0,   /* no bit-complement */
    (binaryfunc) 0,  /* no shiftl */
    (binaryfunc) 0,  /* no shiftr */
    (binaryfunc) 0,  /* no bit-and */
    (binaryfunc) 0,  /* no bit-xor */
    (binaryfunc) 0,  /* no bit-ior */
    (unaryfunc) nm_dec_as_long,
    0,               /* nb_reserved */
    (unaryfunc) PyDec_AsFloat,
    0,               /* binaryfunc nb_inplace_add; */
    0,               /* binaryfunc nb_inplace_subtract; */
    0,               /* binaryfunc nb_inplace_multiply; */
    0,               /* binaryfunc nb_inplace_remainder; */
    0,               /* ternaryfunc nb_inplace_power; */
    0,               /* binaryfunc nb_inplace_lshift; */
    0,               /* binaryfunc nb_inplace_rshift; */
    0,               /* binaryfunc nb_inplace_and; */
    0,               /* binaryfunc nb_inplace_xor; */
    0,               /* binaryfunc nb_inplace_or; */
    (binaryfunc) nm_mpd_qdivint,  /* binaryfunc nb_floor_divide; */
    (binaryfunc) nm_mpd_qdiv,     /* binaryfunc nb_true_divide; */
    0,               /* binaryfunc nb_inplace_floor_divide; */
    0,               /* binaryfunc nb_inplace_true_divide; */
};

static PyMethodDef dec_methods [] =
{
  /* Unary arithmetic functions, optional context arg */
  { "exp", (PyCFunction)(void(*)(void))dec_mpd_qexp, METH_VARARGS|METH_KEYWORDS, doc_exp },
  { "ln", (PyCFunction)(void(*)(void))dec_mpd_qln, METH_VARARGS|METH_KEYWORDS, doc_ln },
  { "log10", (PyCFunction)(void(*)(void))dec_mpd_qlog10, METH_VARARGS|METH_KEYWORDS, doc_log10 },
  { "next_minus", (PyCFunction)(void(*)(void))dec_mpd_qnext_minus, METH_VARARGS|METH_KEYWORDS, doc_next_minus },
  { "next_plus", (PyCFunction)(void(*)(void))dec_mpd_qnext_plus, METH_VARARGS|METH_KEYWORDS, doc_next_plus },
  { "normalize", (PyCFunction)(void(*)(void))dec_mpd_qreduce, METH_VARARGS|METH_KEYWORDS, doc_normalize },
  { "to_integral", (PyCFunction)(void(*)(void))PyDec_ToIntegralValue, METH_VARARGS|METH_KEYWORDS, doc_to_integral },
  { "to_integral_exact", (PyCFunction)(void(*)(void))PyDec_ToIntegralExact, METH_VARARGS|METH_KEYWORDS, doc_to_integral_exact },
  { "to_integral_value", (PyCFunction)(void(*)(void))PyDec_ToIntegralValue, METH_VARARGS|METH_KEYWORDS, doc_to_integral_value },
  { "sqrt", (PyCFunction)(void(*)(void))dec_mpd_qsqrt, METH_VARARGS|METH_KEYWORDS, doc_sqrt },

  /* Binary arithmetic functions, optional context arg */
  { "compare", (PyCFunction)(void(*)(void))dec_mpd_qcompare, METH_VARARGS|METH_KEYWORDS, doc_compare },
  { "compare_signal", (PyCFunction)(void(*)(void))dec_mpd_qcompare_signal, METH_VARARGS|METH_KEYWORDS, doc_compare_signal },
  { "max", (PyCFunction)(void(*)(void))dec_mpd_qmax, METH_VARARGS|METH_KEYWORDS, doc_max },
  { "max_mag", (PyCFunction)(void(*)(void))dec_mpd_qmax_mag, METH_VARARGS|METH_KEYWORDS, doc_max_mag },
  { "min", (PyCFunction)(void(*)(void))dec_mpd_qmin, METH_VARARGS|METH_KEYWORDS, doc_min },
  { "min_mag", (PyCFunction)(void(*)(void))dec_mpd_qmin_mag, METH_VARARGS|METH_KEYWORDS, doc_min_mag },
  { "next_toward", (PyCFunction)(void(*)(void))dec_mpd_qnext_toward, METH_VARARGS|METH_KEYWORDS, doc_next_toward },
  { "quantize", (PyCFunction)(void(*)(void))dec_mpd_qquantize, METH_VARARGS|METH_KEYWORDS, doc_quantize },
  { "remainder_near", (PyCFunction)(void(*)(void))dec_mpd_qrem_near, METH_VARARGS|METH_KEYWORDS, doc_remainder_near },

  /* Ternary arithmetic functions, optional context arg */
  { "fma", (PyCFunction)(void(*)(void))dec_mpd_qfma, METH_VARARGS|METH_KEYWORDS, doc_fma },

  /* Boolean functions, no context arg */
  { "is_canonical", dec_mpd_iscanonical, METH_NOARGS, doc_is_canonical },
  { "is_finite", dec_mpd_isfinite, METH_NOARGS, doc_is_finite },
  { "is_infinite", dec_mpd_isinfinite, METH_NOARGS, doc_is_infinite },
  { "is_nan", dec_mpd_isnan, METH_NOARGS, doc_is_nan },
  { "is_qnan", dec_mpd_isqnan, METH_NOARGS, doc_is_qnan },
  { "is_snan", dec_mpd_issnan, METH_NOARGS, doc_is_snan },
  { "is_signed", dec_mpd_issigned, METH_NOARGS, doc_is_signed },
  { "is_zero", dec_mpd_iszero, METH_NOARGS, doc_is_zero },

  /* Boolean functions, optional context arg */
  { "is_normal", (PyCFunction)(void(*)(void))dec_mpd_isnormal, METH_VARARGS|METH_KEYWORDS, doc_is_normal },
  { "is_subnormal", (PyCFunction)(void(*)(void))dec_mpd_issubnormal, METH_VARARGS|METH_KEYWORDS, doc_is_subnormal },

  /* Unary functions, no context arg */
  { "adjusted", dec_mpd_adjexp, METH_NOARGS, doc_adjusted },
  { "canonical", dec_canonical, METH_NOARGS, doc_canonical },
  { "conjugate", dec_conjugate, METH_NOARGS, doc_conjugate },
  { "radix", dec_mpd_radix, METH_NOARGS, doc_radix },

  /* Unary functions, optional context arg for conversion errors */
  { "copy_abs", dec_mpd_qcopy_abs, METH_NOARGS, doc_copy_abs },
  { "copy_negate", dec_mpd_qcopy_negate, METH_NOARGS, doc_copy_negate },

  /* Unary functions, optional context arg */
  { "logb", (PyCFunction)(void(*)(void))dec_mpd_qlogb, METH_VARARGS|METH_KEYWORDS, doc_logb },
  { "logical_invert", (PyCFunction)(void(*)(void))dec_mpd_qinvert, METH_VARARGS|METH_KEYWORDS, doc_logical_invert },
  { "number_class", (PyCFunction)(void(*)(void))dec_mpd_class, METH_VARARGS|METH_KEYWORDS, doc_number_class },
  { "to_eng_string", (PyCFunction)(void(*)(void))dec_mpd_to_eng, METH_VARARGS|METH_KEYWORDS, doc_to_eng_string },

  /* Binary functions, optional context arg for conversion errors */
  { "compare_total", (PyCFunction)(void(*)(void))dec_mpd_compare_total, METH_VARARGS|METH_KEYWORDS, doc_compare_total },
  { "compare_total_mag", (PyCFunction)(void(*)(void))dec_mpd_compare_total_mag, METH_VARARGS|METH_KEYWORDS, doc_compare_total_mag },
  { "copy_sign", (PyCFunction)(void(*)(void))dec_mpd_qcopy_sign, METH_VARARGS|METH_KEYWORDS, doc_copy_sign },
  { "same_quantum", (PyCFunction)(void(*)(void))dec_mpd_same_quantum, METH_VARARGS|METH_KEYWORDS, doc_same_quantum },

  /* Binary functions, optional context arg */
  { "logical_and", (PyCFunction)(void(*)(void))dec_mpd_qand, METH_VARARGS|METH_KEYWORDS, doc_logical_and },
  { "logical_or", (PyCFunction)(void(*)(void))dec_mpd_qor, METH_VARARGS|METH_KEYWORDS, doc_logical_or },
  { "logical_xor", (PyCFunction)(void(*)(void))dec_mpd_qxor, METH_VARARGS|METH_KEYWORDS, doc_logical_xor },
  { "rotate", (PyCFunction)(void(*)(void))dec_mpd_qrotate, METH_VARARGS|METH_KEYWORDS, doc_rotate },
  { "scaleb", (PyCFunction)(void(*)(void))dec_mpd_qscaleb, METH_VARARGS|METH_KEYWORDS, doc_scaleb },
  { "shift", (PyCFunction)(void(*)(void))dec_mpd_qshift, METH_VARARGS|METH_KEYWORDS, doc_shift },

  /* Miscellaneous */
  { "from_float", dec_from_float, METH_O|METH_CLASS, doc_from_float },
  { "as_tuple", PyDec_AsTuple, METH_NOARGS, doc_as_tuple },
  { "as_integer_ratio", dec_as_integer_ratio, METH_NOARGS, doc_as_integer_ratio },

  /* Special methods */
  { "__copy__", dec_copy, METH_NOARGS, NULL },
  { "__deepcopy__", dec_copy, METH_O, NULL },
  { "__format__", dec_format, METH_VARARGS, NULL },
  { "__reduce__", dec_reduce, METH_NOARGS, NULL },
  { "__round__", PyDec_Round, METH_VARARGS, NULL },
  { "__ceil__", dec_ceil, METH_NOARGS, NULL },
  { "__floor__", dec_floor, METH_NOARGS, NULL },
  { "__trunc__", dec_trunc, METH_NOARGS, NULL },
  { "__complex__", dec_complex, METH_NOARGS, NULL },
  { "__sizeof__", dec_sizeof, METH_NOARGS, NULL },

  { NULL, NULL, 1 }
};

static PyTypeObject PyDec_Type =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "decimal.Decimal",                      /* tp_name */
    sizeof(PyDecObject),                    /* tp_basicsize */
    0,                                      /* tp_itemsize */
    (destructor) dec_dealloc,               /* tp_dealloc */
    0,                                      /* tp_vectorcall_offset */
    (getattrfunc) 0,                        /* tp_getattr */
    (setattrfunc) 0,                        /* tp_setattr */
    0,                                      /* tp_as_async */
    (reprfunc) dec_repr,                    /* tp_repr */
    &dec_number_methods,                    /* tp_as_number */
    0,                                      /* tp_as_sequence */
    0,                                      /* tp_as_mapping */
    (hashfunc) dec_hash,                    /* tp_hash */
    0,                                      /* tp_call */
    (reprfunc) dec_str,                     /* tp_str */
    (getattrofunc) PyObject_GenericGetAttr, /* tp_getattro */
    (setattrofunc) 0,                       /* tp_setattro */
    (PyBufferProcs *) 0,                    /* tp_as_buffer */
    (Py_TPFLAGS_DEFAULT|
     Py_TPFLAGS_BASETYPE),                  /* tp_flags */
    doc_decimal,                            /* tp_doc */
    0,                                      /* tp_traverse */
    0,                                      /* tp_clear */
    dec_richcompare,                        /* tp_richcompare */
    0,                                      /* tp_weaklistoffset */
    0,                                      /* tp_iter */
    0,                                      /* tp_iternext */
    dec_methods,                            /* tp_methods */
    0,                                      /* tp_members */
    dec_getsets,                            /* tp_getset */
    0,                                      /* tp_base */
    0,                                      /* tp_dict */
    0,                                      /* tp_descr_get */
    0,                                      /* tp_descr_set */
    0,                                      /* tp_dictoffset */
    0,                                      /* tp_init */
    0,                                      /* tp_alloc */
    dec_new,                                /* tp_new */
    PyObject_Del,                           /* tp_free */
};


/******************************************************************************/
/*                         Context Object, Part 2                             */
/******************************************************************************/


/************************************************************************/
/*     Macros for converting mpdecimal functions to Context methods     */
/************************************************************************/

/* Boolean context method. */
#define DecCtx_BoolFunc(MPDFUNC) \
static PyObject *                                                     \
ctx_##MPDFUNC(PyObject *context, PyObject *v)                         \
{                                                                     \
    PyObject *ret;                                                    \
    PyObject *a;                                                      \
                                                                      \
    CONVERT_OP_RAISE(&a, v, context);                                 \
                                                                      \
    ret = MPDFUNC(MPD(a), CTX(context)) ? incr_true() : incr_false(); \
    Py_DECREF(a);                                                     \
    return ret;                                                       \
}

/* Boolean context method. MPDFUNC does NOT use a context. */
#define DecCtx_BoolFunc_NO_CTX(MPDFUNC) \
static PyObject *                                       \
ctx_##MPDFUNC(PyObject *context, PyObject *v)           \
{                                                       \
    PyObject *ret;                                      \
    PyObject *a;                                        \
                                                        \
    CONVERT_OP_RAISE(&a, v, context);                   \
                                                        \
    ret = MPDFUNC(MPD(a)) ? incr_true() : incr_false(); \
    Py_DECREF(a);                                       \
    return ret;                                         \
}

/* Unary context method. */
#define DecCtx_UnaryFunc(MPDFUNC) \
static PyObject *                                        \
ctx_##MPDFUNC(PyObject *context, PyObject *v)            \
{                                                        \
    PyObject *result, *a;                                \
    uint32_t status = 0;                                 \
                                                         \
    CONVERT_OP_RAISE(&a, v, context);                    \
                                                         \
    if ((result = dec_alloc()) == NULL) {                \
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

/* Binary context method. */
#define DecCtx_BinaryFunc(MPDFUNC) \
static PyObject *                                                \
ctx_##MPDFUNC(PyObject *context, PyObject *args)                 \
{                                                                \
    PyObject *v, *w;                                             \
    PyObject *a, *b;                                             \
    PyObject *result;                                            \
    uint32_t status = 0;                                         \
                                                                 \
    if (!PyArg_ParseTuple(args, "OO", &v, &w)) {                 \
        return NULL;                                             \
    }                                                            \
                                                                 \
    CONVERT_BINOP_RAISE(&a, &b, v, w, context);                  \
                                                                 \
    if ((result = dec_alloc()) == NULL) {                        \
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
 */
#define DecCtx_BinaryFunc_NO_CTX(MPDFUNC) \
static PyObject *                                \
ctx_##MPDFUNC(PyObject *context, PyObject *args) \
{                                                \
    PyObject *v, *w;                             \
    PyObject *a, *b;                             \
    PyObject *result;                            \
                                                 \
    if (!PyArg_ParseTuple(args, "OO", &v, &w)) { \
        return NULL;                             \
    }                                            \
                                                 \
    CONVERT_BINOP_RAISE(&a, &b, v, w, context);  \
                                                 \
    if ((result = dec_alloc()) == NULL) {        \
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

/* Ternary context method. */
#define DecCtx_TernaryFunc(MPDFUNC) \
static PyObject *                                                        \
ctx_##MPDFUNC(PyObject *context, PyObject *args)                         \
{                                                                        \
    PyObject *v, *w, *x;                                                 \
    PyObject *a, *b, *c;                                                 \
    PyObject *result;                                                    \
    uint32_t status = 0;                                                 \
                                                                         \
    if (!PyArg_ParseTuple(args, "OOO", &v, &w, &x)) {                    \
        return NULL;                                                     \
    }                                                                    \
                                                                         \
    CONVERT_TERNOP_RAISE(&a, &b, &c, v, w, x, context);                  \
                                                                         \
    if ((result = dec_alloc()) == NULL) {                                \
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
DecCtx_UnaryFunc(mpd_qabs)
DecCtx_UnaryFunc(mpd_qexp)
DecCtx_UnaryFunc(mpd_qln)
DecCtx_UnaryFunc(mpd_qlog10)
DecCtx_UnaryFunc(mpd_qminus)
DecCtx_UnaryFunc(mpd_qnext_minus)
DecCtx_UnaryFunc(mpd_qnext_plus)
DecCtx_UnaryFunc(mpd_qplus)
DecCtx_UnaryFunc(mpd_qreduce)
DecCtx_UnaryFunc(mpd_qround_to_int)
DecCtx_UnaryFunc(mpd_qround_to_intx)
DecCtx_UnaryFunc(mpd_qsqrt)

/* Binary arithmetic functions */
DecCtx_BinaryFunc(mpd_qadd)
DecCtx_BinaryFunc(mpd_qcompare)
DecCtx_BinaryFunc(mpd_qcompare_signal)
DecCtx_BinaryFunc(mpd_qdiv)
DecCtx_BinaryFunc(mpd_qdivint)
DecCtx_BinaryFunc(mpd_qmax)
DecCtx_BinaryFunc(mpd_qmax_mag)
DecCtx_BinaryFunc(mpd_qmin)
DecCtx_BinaryFunc(mpd_qmin_mag)
DecCtx_BinaryFunc(mpd_qmul)
DecCtx_BinaryFunc(mpd_qnext_toward)
DecCtx_BinaryFunc(mpd_qquantize)
DecCtx_BinaryFunc(mpd_qrem)
DecCtx_BinaryFunc(mpd_qrem_near)
DecCtx_BinaryFunc(mpd_qsub)

static PyObject *
ctx_mpd_qdivmod(PyObject *context, PyObject *args)
{
    PyObject *v, *w;
    PyObject *a, *b;
    PyObject *q, *r;
    uint32_t status = 0;
    PyObject *ret;

    if (!PyArg_ParseTuple(args, "OO", &v, &w)) {
        return NULL;
    }

    CONVERT_BINOP_RAISE(&a, &b, v, w, context);

    q = dec_alloc();
    if (q == NULL) {
        Py_DECREF(a);
        Py_DECREF(b);
        return NULL;
    }
    r = dec_alloc();
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

    ret = Py_BuildValue("(OO)", q, r);
    Py_DECREF(r);
    Py_DECREF(q);
    return ret;
}

/* Binary or ternary arithmetic functions */
static PyObject *
ctx_mpd_qpow(PyObject *context, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"a", "b", "modulo", NULL};
    PyObject *base, *exp, *mod = Py_None;
    PyObject *a, *b, *c = NULL;
    PyObject *result;
    uint32_t status = 0;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "OO|O", kwlist,
                                     &base, &exp, &mod)) {
        return NULL;
    }

    CONVERT_BINOP_RAISE(&a, &b, base, exp, context);

    if (mod != Py_None) {
        if (!convert_op(TYPE_ERR, &c, mod, context)) {
            Py_DECREF(a);
            Py_DECREF(b);
            return c;
        }
    }

    result = dec_alloc();
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
DecCtx_TernaryFunc(mpd_qfma)

/* No argument */
static PyObject *
ctx_mpd_radix(PyObject *context, PyObject *dummy)
{
    return dec_mpd_radix(context, dummy);
}

/* Boolean functions: single decimal argument */
DecCtx_BoolFunc(mpd_isnormal)
DecCtx_BoolFunc(mpd_issubnormal)
DecCtx_BoolFunc_NO_CTX(mpd_isfinite)
DecCtx_BoolFunc_NO_CTX(mpd_isinfinite)
DecCtx_BoolFunc_NO_CTX(mpd_isnan)
DecCtx_BoolFunc_NO_CTX(mpd_isqnan)
DecCtx_BoolFunc_NO_CTX(mpd_issigned)
DecCtx_BoolFunc_NO_CTX(mpd_issnan)
DecCtx_BoolFunc_NO_CTX(mpd_iszero)

static PyObject *
ctx_iscanonical(PyObject *context UNUSED, PyObject *v)
{
    if (!PyDec_Check(v)) {
        PyErr_SetString(PyExc_TypeError,
            "argument must be a Decimal");
        return NULL;
    }

    return mpd_iscanonical(MPD(v)) ? incr_true() : incr_false();
}

/* Functions with a single decimal argument */
static PyObject *
PyDecContext_Apply(PyObject *context, PyObject *v)
{
    PyObject *result, *a;

    CONVERT_OP_RAISE(&a, v, context);

    result = dec_apply(a, context);
    Py_DECREF(a);
    return result;
}

static PyObject *
ctx_canonical(PyObject *context UNUSED, PyObject *v)
{
    if (!PyDec_Check(v)) {
        PyErr_SetString(PyExc_TypeError,
            "argument must be a Decimal");
        return NULL;
    }

    Py_INCREF(v);
    return v;
}

static PyObject *
ctx_mpd_qcopy_abs(PyObject *context, PyObject *v)
{
    PyObject *result, *a;
    uint32_t status = 0;

    CONVERT_OP_RAISE(&a, v, context);

    result = dec_alloc();
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

static PyObject *
ctx_copy_decimal(PyObject *context, PyObject *v)
{
    PyObject *result;

    CONVERT_OP_RAISE(&result, v, context);
    return result;
}

static PyObject *
ctx_mpd_qcopy_negate(PyObject *context, PyObject *v)
{
    PyObject *result, *a;
    uint32_t status = 0;

    CONVERT_OP_RAISE(&a, v, context);

    result = dec_alloc();
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

DecCtx_UnaryFunc(mpd_qlogb)
DecCtx_UnaryFunc(mpd_qinvert)

static PyObject *
ctx_mpd_class(PyObject *context, PyObject *v)
{
    PyObject *a;
    const char *cp;

    CONVERT_OP_RAISE(&a, v, context);

    cp = mpd_class(MPD(a), CTX(context));
    Py_DECREF(a);

    return PyUnicode_FromString(cp);
}

static PyObject *
ctx_mpd_to_sci(PyObject *context, PyObject *v)
{
    PyObject *result;
    PyObject *a;
    mpd_ssize_t size;
    char *s;

    CONVERT_OP_RAISE(&a, v, context);

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

static PyObject *
ctx_mpd_to_eng(PyObject *context, PyObject *v)
{
    PyObject *result;
    PyObject *a;
    mpd_ssize_t size;
    char *s;

    CONVERT_OP_RAISE(&a, v, context);

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
DecCtx_BinaryFunc_NO_CTX(mpd_compare_total)
DecCtx_BinaryFunc_NO_CTX(mpd_compare_total_mag)

static PyObject *
ctx_mpd_qcopy_sign(PyObject *context, PyObject *args)
{
    PyObject *v, *w;
    PyObject *a, *b;
    PyObject *result;
    uint32_t status = 0;

    if (!PyArg_ParseTuple(args, "OO", &v, &w)) {
        return NULL;
    }

    CONVERT_BINOP_RAISE(&a, &b, v, w, context);

    result = dec_alloc();
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

DecCtx_BinaryFunc(mpd_qand)
DecCtx_BinaryFunc(mpd_qor)
DecCtx_BinaryFunc(mpd_qxor)

DecCtx_BinaryFunc(mpd_qrotate)
DecCtx_BinaryFunc(mpd_qscaleb)
DecCtx_BinaryFunc(mpd_qshift)

static PyObject *
ctx_mpd_same_quantum(PyObject *context, PyObject *args)
{
    PyObject *v, *w;
    PyObject *a, *b;
    PyObject *result;

    if (!PyArg_ParseTuple(args, "OO", &v, &w)) {
        return NULL;
    }

    CONVERT_BINOP_RAISE(&a, &b, v, w, context);

    result = mpd_same_quantum(MPD(a), MPD(b)) ? incr_true() : incr_false();
    Py_DECREF(a);
    Py_DECREF(b);

    return result;
}


static PyMethodDef context_methods [] =
{
  /* Unary arithmetic functions */
  { "abs", ctx_mpd_qabs, METH_O, doc_ctx_abs },
  { "exp", ctx_mpd_qexp, METH_O, doc_ctx_exp },
  { "ln", ctx_mpd_qln, METH_O, doc_ctx_ln },
  { "log10", ctx_mpd_qlog10, METH_O, doc_ctx_log10 },
  { "minus", ctx_mpd_qminus, METH_O, doc_ctx_minus },
  { "next_minus", ctx_mpd_qnext_minus, METH_O, doc_ctx_next_minus },
  { "next_plus", ctx_mpd_qnext_plus, METH_O, doc_ctx_next_plus },
  { "normalize", ctx_mpd_qreduce, METH_O, doc_ctx_normalize },
  { "plus", ctx_mpd_qplus, METH_O, doc_ctx_plus },
  { "to_integral", ctx_mpd_qround_to_int, METH_O, doc_ctx_to_integral },
  { "to_integral_exact", ctx_mpd_qround_to_intx, METH_O, doc_ctx_to_integral_exact },
  { "to_integral_value", ctx_mpd_qround_to_int, METH_O, doc_ctx_to_integral_value },
  { "sqrt", ctx_mpd_qsqrt, METH_O, doc_ctx_sqrt },

  /* Binary arithmetic functions */
  { "add", ctx_mpd_qadd, METH_VARARGS, doc_ctx_add },
  { "compare", ctx_mpd_qcompare, METH_VARARGS, doc_ctx_compare },
  { "compare_signal", ctx_mpd_qcompare_signal, METH_VARARGS, doc_ctx_compare_signal },
  { "divide", ctx_mpd_qdiv, METH_VARARGS, doc_ctx_divide },
  { "divide_int", ctx_mpd_qdivint, METH_VARARGS, doc_ctx_divide_int },
  { "divmod", ctx_mpd_qdivmod, METH_VARARGS, doc_ctx_divmod },
  { "max", ctx_mpd_qmax, METH_VARARGS, doc_ctx_max },
  { "max_mag", ctx_mpd_qmax_mag, METH_VARARGS, doc_ctx_max_mag },
  { "min", ctx_mpd_qmin, METH_VARARGS, doc_ctx_min },
  { "min_mag", ctx_mpd_qmin_mag, METH_VARARGS, doc_ctx_min_mag },
  { "multiply", ctx_mpd_qmul, METH_VARARGS, doc_ctx_multiply },
  { "next_toward", ctx_mpd_qnext_toward, METH_VARARGS, doc_ctx_next_toward },
  { "quantize", ctx_mpd_qquantize, METH_VARARGS, doc_ctx_quantize },
  { "remainder", ctx_mpd_qrem, METH_VARARGS, doc_ctx_remainder },
  { "remainder_near", ctx_mpd_qrem_near, METH_VARARGS, doc_ctx_remainder_near },
  { "subtract", ctx_mpd_qsub, METH_VARARGS, doc_ctx_subtract },

  /* Binary or ternary arithmetic functions */
  { "power", (PyCFunction)(void(*)(void))ctx_mpd_qpow, METH_VARARGS|METH_KEYWORDS, doc_ctx_power },

  /* Ternary arithmetic functions */
  { "fma", ctx_mpd_qfma, METH_VARARGS, doc_ctx_fma },

  /* No argument */
  { "Etiny", context_getetiny, METH_NOARGS, doc_ctx_Etiny },
  { "Etop", context_getetop, METH_NOARGS, doc_ctx_Etop },
  { "radix", ctx_mpd_radix, METH_NOARGS, doc_ctx_radix },

  /* Boolean functions */
  { "is_canonical", ctx_iscanonical, METH_O, doc_ctx_is_canonical },
  { "is_finite", ctx_mpd_isfinite, METH_O, doc_ctx_is_finite },
  { "is_infinite", ctx_mpd_isinfinite, METH_O, doc_ctx_is_infinite },
  { "is_nan", ctx_mpd_isnan, METH_O, doc_ctx_is_nan },
  { "is_normal", ctx_mpd_isnormal, METH_O, doc_ctx_is_normal },
  { "is_qnan", ctx_mpd_isqnan, METH_O, doc_ctx_is_qnan },
  { "is_signed", ctx_mpd_issigned, METH_O, doc_ctx_is_signed },
  { "is_snan", ctx_mpd_issnan, METH_O, doc_ctx_is_snan },
  { "is_subnormal", ctx_mpd_issubnormal, METH_O, doc_ctx_is_subnormal },
  { "is_zero", ctx_mpd_iszero, METH_O, doc_ctx_is_zero },

  /* Functions with a single decimal argument */
  { "_apply", PyDecContext_Apply, METH_O, NULL }, /* alias for apply */
#ifdef EXTRA_FUNCTIONALITY
  { "apply", PyDecContext_Apply, METH_O, doc_ctx_apply },
#endif
  { "canonical", ctx_canonical, METH_O, doc_ctx_canonical },
  { "copy_abs", ctx_mpd_qcopy_abs, METH_O, doc_ctx_copy_abs },
  { "copy_decimal", ctx_copy_decimal, METH_O, doc_ctx_copy_decimal },
  { "copy_negate", ctx_mpd_qcopy_negate, METH_O, doc_ctx_copy_negate },
  { "logb", ctx_mpd_qlogb, METH_O, doc_ctx_logb },
  { "logical_invert", ctx_mpd_qinvert, METH_O, doc_ctx_logical_invert },
  { "number_class", ctx_mpd_class, METH_O, doc_ctx_number_class },
  { "to_sci_string", ctx_mpd_to_sci, METH_O, doc_ctx_to_sci_string },
  { "to_eng_string", ctx_mpd_to_eng, METH_O, doc_ctx_to_eng_string },

  /* Functions with two decimal arguments */
  { "compare_total", ctx_mpd_compare_total, METH_VARARGS, doc_ctx_compare_total },
  { "compare_total_mag", ctx_mpd_compare_total_mag, METH_VARARGS, doc_ctx_compare_total_mag },
  { "copy_sign", ctx_mpd_qcopy_sign, METH_VARARGS, doc_ctx_copy_sign },
  { "logical_and", ctx_mpd_qand, METH_VARARGS, doc_ctx_logical_and },
  { "logical_or", ctx_mpd_qor, METH_VARARGS, doc_ctx_logical_or },
  { "logical_xor", ctx_mpd_qxor, METH_VARARGS, doc_ctx_logical_xor },
  { "rotate", ctx_mpd_qrotate, METH_VARARGS, doc_ctx_rotate },
  { "same_quantum", ctx_mpd_same_quantum, METH_VARARGS, doc_ctx_same_quantum },
  { "scaleb", ctx_mpd_qscaleb, METH_VARARGS, doc_ctx_scaleb },
  { "shift", ctx_mpd_qshift, METH_VARARGS, doc_ctx_shift },

  /* Set context values */
  { "clear_flags", context_clear_flags, METH_NOARGS, doc_ctx_clear_flags },
  { "clear_traps", context_clear_traps, METH_NOARGS, doc_ctx_clear_traps },

#ifdef CONFIG_32
  /* Unsafe set functions with relaxed range checks */
  { "_unsafe_setprec", context_unsafe_setprec, METH_O, NULL },
  { "_unsafe_setemin", context_unsafe_setemin, METH_O, NULL },
  { "_unsafe_setemax", context_unsafe_setemax, METH_O, NULL },
#endif

  /* Miscellaneous */
  { "__copy__", (PyCFunction)context_copy, METH_NOARGS, NULL },
  { "__reduce__", context_reduce, METH_NOARGS, NULL },
  { "copy", (PyCFunction)context_copy, METH_NOARGS, doc_ctx_copy },
  { "create_decimal", ctx_create_decimal, METH_VARARGS, doc_ctx_create_decimal },
  { "create_decimal_from_float", ctx_from_float, METH_O, doc_ctx_create_decimal_from_float },

  { NULL, NULL, 1 }
};

static PyTypeObject PyDecContext_Type =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "decimal.Context",                         /* tp_name */
    sizeof(PyDecContextObject),                /* tp_basicsize */
    0,                                         /* tp_itemsize */
    (destructor) context_dealloc,              /* tp_dealloc */
    0,                                         /* tp_vectorcall_offset */
    (getattrfunc) 0,                           /* tp_getattr */
    (setattrfunc) 0,                           /* tp_setattr */
    0,                                         /* tp_as_async */
    (reprfunc) context_repr,                   /* tp_repr */
    0,                                         /* tp_as_number */
    0,                                         /* tp_as_sequence */
    0,                                         /* tp_as_mapping */
    (hashfunc) 0,                              /* tp_hash */
    0,                                         /* tp_call */
    0,                                         /* tp_str */
    (getattrofunc) context_getattr,            /* tp_getattro */
    (setattrofunc) context_setattr,            /* tp_setattro */
    (PyBufferProcs *) 0,                       /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE,    /* tp_flags */
    doc_context,                               /* tp_doc */
    0,                                         /* tp_traverse */
    0,                                         /* tp_clear */
    0,                                         /* tp_richcompare */
    0,                                         /* tp_weaklistoffset */
    0,                                         /* tp_iter */
    0,                                         /* tp_iternext */
    context_methods,                           /* tp_methods */
    0,                                         /* tp_members */
    context_getsets,                           /* tp_getset */
    0,                                         /* tp_base */
    0,                                         /* tp_dict */
    0,                                         /* tp_descr_get */
    0,                                         /* tp_descr_set */
    0,                                         /* tp_dictoffset */
    context_init,                              /* tp_init */
    0,                                         /* tp_alloc */
    context_new,                               /* tp_new */
    PyObject_Del,                              /* tp_free */
};


static PyMethodDef _decimal_methods [] =
{
  { "getcontext", (PyCFunction)PyDec_GetCurrentContext, METH_NOARGS, doc_getcontext},
  { "setcontext", (PyCFunction)PyDec_SetCurrentContext, METH_O, doc_setcontext},
  { "localcontext", (PyCFunction)(void(*)(void))ctxmanager_new, METH_VARARGS|METH_KEYWORDS, doc_localcontext},
#ifdef EXTRA_FUNCTIONALITY
  { "IEEEContext", (PyCFunction)ieee_context, METH_O, doc_ieee_context},
#endif
  { NULL, NULL, 1, NULL }
};

static struct PyModuleDef _decimal_module = {
    PyModuleDef_HEAD_INIT,
    "decimal",
    doc__decimal,
    -1,
    _decimal_methods,
    NULL,
    NULL,
    NULL,
    NULL
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
#ifdef EXTRA_FUNCTIONALITY
    {"DECIMAL32", MPD_DECIMAL32},
    {"DECIMAL64", MPD_DECIMAL64},
    {"DECIMAL128", MPD_DECIMAL128},
    {"IEEE_CONTEXT_MAX_BITS", MPD_IEEE_CONTEXT_MAX_BITS},
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


PyMODINIT_FUNC
PyInit__decimal(void)
{
    PyObject *m = NULL;
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
    mpd_setminalloc(_Py_DEC_MINALLOC);


    /* Init external C-API functions */
    _py_long_multiply = PyLong_Type.tp_as_number->nb_multiply;
    _py_long_floor_divide = PyLong_Type.tp_as_number->nb_floor_divide;
    _py_long_power = PyLong_Type.tp_as_number->nb_power;
    _py_float_abs = PyFloat_Type.tp_as_number->nb_absolute;
    ASSIGN_PTR(_py_float_as_integer_ratio, cfunc_noargs(&PyFloat_Type,
                                                        "as_integer_ratio"));
    ASSIGN_PTR(_py_long_bit_length, cfunc_noargs(&PyLong_Type, "bit_length"));


    /* Init types */
    PyDec_Type.tp_base = &PyBaseObject_Type;
    PyDecContext_Type.tp_base = &PyBaseObject_Type;
    PyDecContextManager_Type.tp_base = &PyBaseObject_Type;
    PyDecSignalDictMixin_Type.tp_base = &PyBaseObject_Type;

    CHECK_INT(PyType_Ready(&PyDec_Type));
    CHECK_INT(PyType_Ready(&PyDecContext_Type));
    CHECK_INT(PyType_Ready(&PyDecSignalDictMixin_Type));
    CHECK_INT(PyType_Ready(&PyDecContextManager_Type));

    ASSIGN_PTR(obj, PyUnicode_FromString("decimal"));
    CHECK_INT(PyDict_SetItemString(PyDec_Type.tp_dict, "__module__", obj));
    CHECK_INT(PyDict_SetItemString(PyDecContext_Type.tp_dict,
                                   "__module__", obj));
    Py_CLEAR(obj);


    /* Numeric abstract base classes */
    ASSIGN_PTR(numbers, PyImport_ImportModule("numbers"));
    ASSIGN_PTR(Number, PyObject_GetAttrString(numbers, "Number"));
    /* Register Decimal with the Number abstract base class */
    ASSIGN_PTR(obj, PyObject_CallMethod(Number, "register", "(O)",
                                        (PyObject *)&PyDec_Type));
    Py_CLEAR(obj);
    /* Rational is a global variable used for fraction comparisons. */
    ASSIGN_PTR(Rational, PyObject_GetAttrString(numbers, "Rational"));
    /* Done with numbers, Number */
    Py_CLEAR(numbers);
    Py_CLEAR(Number);

    /* DecimalTuple */
    ASSIGN_PTR(collections, PyImport_ImportModule("collections"));
    ASSIGN_PTR(DecimalTuple, (PyTypeObject *)PyObject_CallMethod(collections,
                                 "namedtuple", "(ss)", "DecimalTuple",
                                 "sign digits exponent"));

    ASSIGN_PTR(obj, PyUnicode_FromString("decimal"));
    CHECK_INT(PyDict_SetItemString(DecimalTuple->tp_dict, "__module__", obj));
    Py_CLEAR(obj);

    /* MutableMapping */
    ASSIGN_PTR(collections_abc, PyImport_ImportModule("collections.abc"));
    ASSIGN_PTR(MutableMapping, PyObject_GetAttrString(collections_abc,
                                                      "MutableMapping"));
    /* Create SignalDict type */
    ASSIGN_PTR(PyDecSignalDict_Type,
                   (PyTypeObject *)PyObject_CallFunction(
                   (PyObject *)&PyType_Type, "s(OO){}",
                   "SignalDict", &PyDecSignalDictMixin_Type,
                   MutableMapping));

    /* Done with collections, MutableMapping */
    Py_CLEAR(collections);
    Py_CLEAR(collections_abc);
    Py_CLEAR(MutableMapping);


    /* Create the module */
    ASSIGN_PTR(m, PyModule_Create(&_decimal_module));


    /* Add types to the module */
    Py_INCREF(&PyDec_Type);
    CHECK_INT(PyModule_AddObject(m, "Decimal", (PyObject *)&PyDec_Type));
    Py_INCREF(&PyDecContext_Type);
    CHECK_INT(PyModule_AddObject(m, "Context",
                                 (PyObject *)&PyDecContext_Type));
    Py_INCREF(DecimalTuple);
    CHECK_INT(PyModule_AddObject(m, "DecimalTuple", (PyObject *)DecimalTuple));


    /* Create top level exception */
    ASSIGN_PTR(DecimalException, PyErr_NewException(
                                     "decimal.DecimalException",
                                     PyExc_ArithmeticError, NULL));
    Py_INCREF(DecimalException);
    CHECK_INT(PyModule_AddObject(m, "DecimalException", DecimalException));

    /* Create signal tuple */
    ASSIGN_PTR(SignalTuple, PyTuple_New(SIGNAL_MAP_LEN));

    /* Add exceptions that correspond to IEEE signals */
    for (i = SIGNAL_MAP_LEN-1; i >= 0; i--) {
        PyObject *base;

        cm = signal_map + i;

        switch (cm->flag) {
        case MPD_Float_operation:
            base = PyTuple_Pack(2, DecimalException, PyExc_TypeError);
            break;
        case MPD_Division_by_zero:
            base = PyTuple_Pack(2, DecimalException, PyExc_ZeroDivisionError);
            break;
        case MPD_Overflow:
            base = PyTuple_Pack(2, signal_map[INEXACT].ex,
                                   signal_map[ROUNDED].ex);
            break;
        case MPD_Underflow:
            base = PyTuple_Pack(3, signal_map[INEXACT].ex,
                                   signal_map[ROUNDED].ex,
                                   signal_map[SUBNORMAL].ex);
            break;
        default:
            base = PyTuple_Pack(1, DecimalException);
            break;
        }

        if (base == NULL) {
            goto error; /* GCOV_NOT_REACHED */
        }

        ASSIGN_PTR(cm->ex, PyErr_NewException(cm->fqname, base, NULL));
        Py_DECREF(base);

        /* add to module */
        Py_INCREF(cm->ex);
        CHECK_INT(PyModule_AddObject(m, cm->name, cm->ex));

        /* add to signal tuple */
        Py_INCREF(cm->ex);
        PyTuple_SET_ITEM(SignalTuple, i, cm->ex);
    }

    /*
     * Unfortunately, InvalidOperation is a signal that comprises
     * several conditions, including InvalidOperation! Naming the
     * signal IEEEInvalidOperation would prevent the confusion.
     */
    cond_map[0].ex = signal_map[0].ex;

    /* Add remaining exceptions, inherit from InvalidOperation */
    for (cm = cond_map+1; cm->name != NULL; cm++) {
        PyObject *base;
        if (cm->flag == MPD_Division_undefined) {
            base = PyTuple_Pack(2, signal_map[0].ex, PyExc_ZeroDivisionError);
        }
        else {
            base = PyTuple_Pack(1, signal_map[0].ex);
        }
        if (base == NULL) {
            goto error; /* GCOV_NOT_REACHED */
        }

        ASSIGN_PTR(cm->ex, PyErr_NewException(cm->fqname, base, NULL));
        Py_DECREF(base);

        Py_INCREF(cm->ex);
        CHECK_INT(PyModule_AddObject(m, cm->name, cm->ex));
    }


    /* Init default context template first */
    ASSIGN_PTR(default_context_template,
               PyObject_CallObject((PyObject *)&PyDecContext_Type, NULL));
    Py_INCREF(default_context_template);
    CHECK_INT(PyModule_AddObject(m, "DefaultContext",
                                 default_context_template));

#ifndef WITH_DECIMAL_CONTEXTVAR
    ASSIGN_PTR(tls_context_key, PyUnicode_FromString("___DECIMAL_CTX__"));
    Py_INCREF(Py_False);
    CHECK_INT(PyModule_AddObject(m, "HAVE_CONTEXTVAR", Py_False));
#else
    ASSIGN_PTR(current_context_var, PyContextVar_New("decimal_context", NULL));
    Py_INCREF(Py_True);
    CHECK_INT(PyModule_AddObject(m, "HAVE_CONTEXTVAR", Py_True));
#endif
    Py_INCREF(Py_True);
    CHECK_INT(PyModule_AddObject(m, "HAVE_THREADS", Py_True));

    /* Init basic context template */
    ASSIGN_PTR(basic_context_template,
               PyObject_CallObject((PyObject *)&PyDecContext_Type, NULL));
    init_basic_context(basic_context_template);
    Py_INCREF(basic_context_template);
    CHECK_INT(PyModule_AddObject(m, "BasicContext",
                                 basic_context_template));

    /* Init extended context template */
    ASSIGN_PTR(extended_context_template,
               PyObject_CallObject((PyObject *)&PyDecContext_Type, NULL));
    init_extended_context(extended_context_template);
    Py_INCREF(extended_context_template);
    CHECK_INT(PyModule_AddObject(m, "ExtendedContext",
                                 extended_context_template));


    /* Init mpd_ssize_t constants */
    for (ssize_cm = ssize_constants; ssize_cm->name != NULL; ssize_cm++) {
        ASSIGN_PTR(obj, PyLong_FromSsize_t(ssize_cm->val));
        CHECK_INT(PyModule_AddObject(m, ssize_cm->name, obj));
        obj = NULL;
    }

    /* Init int constants */
    for (int_cm = int_constants; int_cm->name != NULL; int_cm++) {
        CHECK_INT(PyModule_AddIntConstant(m, int_cm->name,
                                          int_cm->val));
    }

    /* Init string constants */
    for (i = 0; i < _PY_DEC_ROUND_GUARD; i++) {
        ASSIGN_PTR(round_map[i], PyUnicode_InternFromString(mpd_round_string[i]));
        Py_INCREF(round_map[i]);
        CHECK_INT(PyModule_AddObject(m, mpd_round_string[i], round_map[i]));
    }

    /* Add specification version number */
    CHECK_INT(PyModule_AddStringConstant(m, "__version__", "1.70"));
    CHECK_INT(PyModule_AddStringConstant(m, "__libmpdec_version__", mpd_version()));


    return m;


error:
    Py_CLEAR(obj); /* GCOV_NOT_REACHED */
    Py_CLEAR(numbers); /* GCOV_NOT_REACHED */
    Py_CLEAR(Number); /* GCOV_NOT_REACHED */
    Py_CLEAR(Rational); /* GCOV_NOT_REACHED */
    Py_CLEAR(collections); /* GCOV_NOT_REACHED */
    Py_CLEAR(collections_abc); /* GCOV_NOT_REACHED */
    Py_CLEAR(MutableMapping); /* GCOV_NOT_REACHED */
    Py_CLEAR(SignalTuple); /* GCOV_NOT_REACHED */
    Py_CLEAR(DecimalTuple); /* GCOV_NOT_REACHED */
    Py_CLEAR(default_context_template); /* GCOV_NOT_REACHED */
#ifndef WITH_DECIMAL_CONTEXTVAR
    Py_CLEAR(tls_context_key); /* GCOV_NOT_REACHED */
#else
    Py_CLEAR(current_context_var); /* GCOV_NOT_REACHED */
#endif
    Py_CLEAR(basic_context_template); /* GCOV_NOT_REACHED */
    Py_CLEAR(extended_context_template); /* GCOV_NOT_REACHED */
    Py_CLEAR(m); /* GCOV_NOT_REACHED */

    return NULL; /* GCOV_NOT_REACHED */
}


