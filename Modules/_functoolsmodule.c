#include "Python.h"
#include "pycore_call.h"          // _PyObject_CallNoArgs()
#include "pycore_dict.h"          // _PyDict_Pop_KnownHash()
#include "pycore_long.h"          // _PyLong_GetZero()
#include "pycore_moduleobject.h"  // _PyModule_GetState()
#include "pycore_object.h"        // _PyObject_GC_TRACK
#include "pycore_pyatomic_ft_wrappers.h"
#include "pycore_pystate.h"       // _PyThreadState_GET()
#include "pycore_tuple.h"         // _PyTuple_ITEMS()
#include "pycore_weakref.h"       // FT_CLEAR_WEAKREFS()


#include "clinic/_functoolsmodule.c.h"
/*[clinic input]
module _functools
class _functools._lru_cache_wrapper "PyObject *" "&lru_cache_type_spec"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=bece4053896b09c0]*/

/* _functools module written and maintained
   by Hye-Shik Chang <perky@FreeBSD.org>
   with adaptations by Raymond Hettinger <python@rcn.com>
   Copyright (c) 2004 Python Software Foundation.
   All rights reserved.
*/

typedef struct _functools_state {
    /* this object is used delimit args and keywords in the cache keys */
    PyObject *kwd_mark;
    PyTypeObject *placeholder_type;
    PyObject *placeholder;  // strong reference (singleton)
    PyTypeObject *partial_type;
    PyTypeObject *keyobject_type;
    PyTypeObject *lru_list_elem_type;
} _functools_state;

static inline _functools_state *
get_functools_state(PyObject *module)
{
    void *state = _PyModule_GetState(module);
    assert(state != NULL);
    return (_functools_state *)state;
}

/* partial object **********************************************************/


// The 'Placeholder' singleton indicates which formal positional
// parameters are to be bound first when using a 'partial' object.

typedef struct {
    PyObject_HEAD
} placeholderobject;

static inline _functools_state *
get_functools_state_by_type(PyTypeObject *type);

PyDoc_STRVAR(placeholder_doc,
"The type of the Placeholder singleton.\n\n"
"Used as a placeholder for partial arguments.");

static PyObject *
placeholder_repr(PyObject *op)
{
    return PyUnicode_FromString("Placeholder");
}

static PyObject *
placeholder_reduce(PyObject *op, PyObject *Py_UNUSED(ignored))
{
    return PyUnicode_FromString("Placeholder");
}

static PyMethodDef placeholder_methods[] = {
    {"__reduce__", placeholder_reduce, METH_NOARGS, NULL},
    {NULL, NULL}
};

static void
placeholder_dealloc(PyObject* self)
{
    PyObject_GC_UnTrack(self);
    PyTypeObject *tp = Py_TYPE(self);
    tp->tp_free((PyObject*)self);
    Py_DECREF(tp);
}

static PyObject *
placeholder_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    if (PyTuple_GET_SIZE(args) || (kwargs && PyDict_GET_SIZE(kwargs))) {
        PyErr_SetString(PyExc_TypeError, "PlaceholderType takes no arguments");
        return NULL;
    }
    _functools_state *state = get_functools_state_by_type(type);
    if (state->placeholder != NULL) {
        return Py_NewRef(state->placeholder);
    }

    PyObject *placeholder = PyType_GenericNew(type, NULL, NULL);
    if (placeholder == NULL) {
        return NULL;
    }

    if (state->placeholder == NULL) {
        state->placeholder = Py_NewRef(placeholder);
    }
    return placeholder;
}

static int
placeholder_traverse(PyObject *self, visitproc visit, void *arg)
{
    Py_VISIT(Py_TYPE(self));
    return 0;
}

static PyType_Slot placeholder_type_slots[] = {
    {Py_tp_dealloc, placeholder_dealloc},
    {Py_tp_repr, placeholder_repr},
    {Py_tp_doc, (void *)placeholder_doc},
    {Py_tp_methods, placeholder_methods},
    {Py_tp_new, placeholder_new},
    {Py_tp_traverse, placeholder_traverse},
    {0, 0}
};

static PyType_Spec placeholder_type_spec = {
    .name = "functools._PlaceholderType",
    .basicsize = sizeof(placeholderobject),
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_IMMUTABLETYPE | Py_TPFLAGS_HAVE_GC,
    .slots = placeholder_type_slots
};


typedef struct {
    PyObject_HEAD
    PyObject *fn;
    PyObject *args;
    PyObject *kw;
    PyObject *dict;        /* __dict__ */
    PyObject *weakreflist; /* List of weak references */
    PyObject *placeholder; /* Placeholder for positional arguments */
    Py_ssize_t phcount;    /* Number of placeholders */
    vectorcallfunc vectorcall;
} partialobject;

// cast a PyObject pointer PTR to a partialobject pointer (no type checks)
#define partialobject_CAST(op)  ((partialobject *)(op))

static void partial_setvectorcall(partialobject *pto);
static struct PyModuleDef _functools_module;
static PyObject *
partial_call(PyObject *pto, PyObject *args, PyObject *kwargs);

static inline _functools_state *
get_functools_state_by_type(PyTypeObject *type)
{
    PyObject *module = PyType_GetModuleByDef(type, &_functools_module);
    if (module == NULL) {
        return NULL;
    }
    return get_functools_state(module);
}

// Not converted to argument clinic, because of `*args, **kwargs` arguments.
static PyObject *
partial_new(PyTypeObject *type, PyObject *args, PyObject *kw)
{
    PyObject *func, *pto_args, *new_args, *pto_kw, *phold;
    partialobject *pto;
    Py_ssize_t pto_phcount = 0;
    Py_ssize_t new_nargs = PyTuple_GET_SIZE(args) - 1;

    if (new_nargs < 0) {
        PyErr_SetString(PyExc_TypeError,
                        "type 'partial' takes at least one argument");
        return NULL;
    }
    func = PyTuple_GET_ITEM(args, 0);
    if (!PyCallable_Check(func)) {
        PyErr_SetString(PyExc_TypeError,
                        "the first argument must be callable");
        return NULL;
    }

    _functools_state *state = get_functools_state_by_type(type);
    if (state == NULL) {
        return NULL;
    }
    phold = state->placeholder;

    /* Placeholder restrictions */
    if (new_nargs && PyTuple_GET_ITEM(args, new_nargs) == phold) {
        PyErr_SetString(PyExc_TypeError,
                        "trailing Placeholders are not allowed");
        return NULL;
    }

    /* keyword Placeholder prohibition */
    if (kw != NULL) {
        PyObject *key, *val;
        Py_ssize_t pos = 0;
        while (PyDict_Next(kw, &pos, &key, &val)) {
            if (val == phold) {
                PyErr_SetString(PyExc_TypeError,
                                "Placeholder cannot be passed as a keyword argument");
                return NULL;
            }
        }
    }

    /* check wrapped function / object */
    pto_args = pto_kw = NULL;
    int res = PyObject_TypeCheck(func, state->partial_type);
    if (res == -1) {
        return NULL;
    }
    if (res == 1) {
        // We can use its underlying function directly and merge the arguments.
        partialobject *part = (partialobject *)func;
        if (part->dict == NULL) {
            pto_args = part->args;
            pto_kw = part->kw;
            func = part->fn;
            pto_phcount = part->phcount;
            assert(PyTuple_Check(pto_args));
            assert(PyDict_Check(pto_kw));
        }
    }

    /* create partialobject structure */
    pto = (partialobject *)type->tp_alloc(type, 0);
    if (pto == NULL)
        return NULL;

    pto->fn = Py_NewRef(func);
    pto->placeholder = phold;

    new_args = PyTuple_GetSlice(args, 1, new_nargs + 1);
    if (new_args == NULL) {
        Py_DECREF(pto);
        return NULL;
    }

    /* Count placeholders */
    Py_ssize_t phcount = 0;
    for (Py_ssize_t i = 0; i < new_nargs - 1; i++) {
        if (PyTuple_GET_ITEM(new_args, i) == phold) {
            phcount++;
        }
    }
    /* merge args with args of `func` which is `partial` */
    if (pto_phcount > 0 && new_nargs > 0) {
        Py_ssize_t npargs = PyTuple_GET_SIZE(pto_args);
        Py_ssize_t tot_nargs = npargs;
        if (new_nargs > pto_phcount) {
            tot_nargs += new_nargs - pto_phcount;
        }
        PyObject *item;
        PyObject *tot_args = PyTuple_New(tot_nargs);
        for (Py_ssize_t i = 0, j = 0; i < tot_nargs; i++) {
            if (i < npargs) {
                item = PyTuple_GET_ITEM(pto_args, i);
                if (j < new_nargs && item == phold) {
                    item = PyTuple_GET_ITEM(new_args, j);
                    j++;
                    pto_phcount--;
                }
            }
            else {
                item = PyTuple_GET_ITEM(new_args, j);
                j++;
            }
            Py_INCREF(item);
            PyTuple_SET_ITEM(tot_args, i, item);
        }
        pto->args = tot_args;
        pto->phcount = pto_phcount + phcount;
        Py_DECREF(new_args);
    }
    else if (pto_args == NULL) {
        pto->args = new_args;
        pto->phcount = phcount;
    }
    else {
        pto->args = PySequence_Concat(pto_args, new_args);
        pto->phcount = pto_phcount + phcount;
        Py_DECREF(new_args);
        if (pto->args == NULL) {
            Py_DECREF(pto);
            return NULL;
        }
        assert(PyTuple_Check(pto->args));
    }

    if (pto_kw == NULL || PyDict_GET_SIZE(pto_kw) == 0) {
        if (kw == NULL) {
            pto->kw = PyDict_New();
        }
        else if (Py_REFCNT(kw) == 1) {
            pto->kw = Py_NewRef(kw);
        }
        else {
            pto->kw = PyDict_Copy(kw);
        }
    }
    else {
        pto->kw = PyDict_Copy(pto_kw);
        if (kw != NULL && pto->kw != NULL) {
            if (PyDict_Merge(pto->kw, kw, 1) != 0) {
                Py_DECREF(pto);
                return NULL;
            }
        }
    }
    if (pto->kw == NULL) {
        Py_DECREF(pto);
        return NULL;
    }

    partial_setvectorcall(pto);
    return (PyObject *)pto;
}

static int
partial_clear(PyObject *self)
{
    partialobject *pto = partialobject_CAST(self);
    Py_CLEAR(pto->fn);
    Py_CLEAR(pto->args);
    Py_CLEAR(pto->kw);
    Py_CLEAR(pto->dict);
    return 0;
}

static int
partial_traverse(PyObject *self, visitproc visit, void *arg)
{
    partialobject *pto = partialobject_CAST(self);
    Py_VISIT(Py_TYPE(pto));
    Py_VISIT(pto->fn);
    Py_VISIT(pto->args);
    Py_VISIT(pto->kw);
    Py_VISIT(pto->dict);
    return 0;
}

static void
partial_dealloc(PyObject *self)
{
    PyTypeObject *tp = Py_TYPE(self);
    /* bpo-31095: UnTrack is needed before calling any callbacks */
    PyObject_GC_UnTrack(self);
    FT_CLEAR_WEAKREFS(self, partialobject_CAST(self)->weakreflist);
    (void)partial_clear(self);
    tp->tp_free(self);
    Py_DECREF(tp);
}

static PyObject *
partial_descr_get(PyObject *self, PyObject *obj, PyObject *type)
{
    if (obj == Py_None || obj == NULL) {
        return Py_NewRef(self);
    }
    return PyMethod_New(self, obj);
}

static PyObject *
partial_vectorcall(PyObject *self, PyObject *const *args,
                   size_t nargsf, PyObject *kwnames)
{
    partialobject *pto = partialobject_CAST(self);;
    PyThreadState *tstate = _PyThreadState_GET();
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);

    /* Placeholder check */
    Py_ssize_t pto_phcount = pto->phcount;
    if (nargs < pto_phcount) {
        PyErr_Format(PyExc_TypeError,
                     "missing positional arguments in 'partial' call; "
                     "expected at least %zd, got %zd", pto_phcount, nargs);
        return NULL;
    }

    PyObject **pto_args = _PyTuple_ITEMS(pto->args);
    Py_ssize_t pto_nargs = PyTuple_GET_SIZE(pto->args);
    Py_ssize_t pto_nkwds = PyDict_GET_SIZE(pto->kw);
    Py_ssize_t nkwds = kwnames == NULL ? 0 : PyTuple_GET_SIZE(kwnames);
    Py_ssize_t nargskw = nargs + nkwds;

    /* Special cases */
    if (!pto_nkwds) {
        /* Fast path if we're called without arguments */
        if (nargskw == 0) {
            return _PyObject_VectorcallTstate(tstate, pto->fn, pto_args,
                                              pto_nargs, NULL);
        }

        /* Use PY_VECTORCALL_ARGUMENTS_OFFSET to prepend a single
         * positional argument. */
        if (pto_nargs == 1 && (nargsf & PY_VECTORCALL_ARGUMENTS_OFFSET)) {
            PyObject **newargs = (PyObject **)args - 1;
            PyObject *tmp = newargs[0];
            newargs[0] = pto_args[0];
            PyObject *ret = _PyObject_VectorcallTstate(tstate, pto->fn, newargs,
                                                       nargs + 1, kwnames);
            newargs[0] = tmp;
            return ret;
        }
    }

    /* Total sizes */
    Py_ssize_t tot_nargs = pto_nargs + nargs - pto_phcount;
    Py_ssize_t tot_nkwds = pto_nkwds + nkwds;
    Py_ssize_t tot_nargskw = tot_nargs + tot_nkwds;

    PyObject *pto_kw_merged = NULL;  // pto_kw with duplicates merged (if any)
    PyObject *tot_kwnames;

    /* Allocate Stack
     * Note, _PY_FASTCALL_SMALL_STACK is optimal for positional only
     * This case might have keyword arguments
     *  furthermore, it might use extra stack space for temporary key storage
     *  thus, double small_stack size is used, which is 10 * 8 = 80 bytes */
    PyObject *small_stack[_PY_FASTCALL_SMALL_STACK * 2];
    PyObject **tmp_stack, **stack;
    Py_ssize_t init_stack_size = tot_nargskw;
    if (pto_nkwds) {
        // If pto_nkwds, allocate additional space for temporary new keys
        init_stack_size += nkwds;
    }
    if (init_stack_size <= (Py_ssize_t)Py_ARRAY_LENGTH(small_stack)) {
        stack = small_stack;
    }
    else {
        stack = PyMem_Malloc(init_stack_size * sizeof(PyObject *));
        if (stack == NULL) {
            return PyErr_NoMemory();
        }
    }

    /* Copy keywords to stack */
    if (!pto_nkwds) {
        tot_kwnames = kwnames;
        if (nkwds) {
            /* if !pto_nkwds & nkwds, then simply append kw */
            memcpy(stack + tot_nargs, args + nargs, nkwds * sizeof(PyObject*));
        }
    }
    else {
        /* stack is now         [<positionals>, <pto_kwds>, <kwds>, <kwds_keys>]
         * Will resize later to [<positionals>, <merged_kwds>] */
        PyObject *key, *val;

        /* Merge kw to pto_kw or add to tail (if not duplicate) */
        Py_ssize_t n_tail = 0;
        for (Py_ssize_t i = 0; i < nkwds; ++i) {
            key = PyTuple_GET_ITEM(kwnames, i);
            val = args[nargs + i];
            if (PyDict_Contains(pto->kw, key)) {
                if (pto_kw_merged == NULL) {
                    pto_kw_merged = PyDict_Copy(pto->kw);
                    if (pto_kw_merged == NULL) {
                        goto error;
                    }
                }
                if (PyDict_SetItem(pto_kw_merged, key, val) < 0) {
                    Py_DECREF(pto_kw_merged);
                    goto error;
                }
            }
            else {
                /* Copy keyword tail to stack */
                stack[tot_nargs + pto_nkwds + n_tail] = val;
                stack[tot_nargskw + n_tail] = key;
                n_tail++;
            }
        }
        Py_ssize_t n_merges = nkwds - n_tail;

        /* Create total kwnames */
        tot_kwnames = PyTuple_New(tot_nkwds - n_merges);
        if (tot_kwnames == NULL) {
            Py_XDECREF(pto_kw_merged);
            goto error;
        }
        for (Py_ssize_t i = 0; i < n_tail; ++i) {
            key = Py_NewRef(stack[tot_nargskw + i]);
            PyTuple_SET_ITEM(tot_kwnames, pto_nkwds + i, key);
        }

        /* Copy pto_keywords with overlapping call keywords merged
         * Note, tail is already coppied. */
        Py_ssize_t pos = 0, i = 0;
        while (PyDict_Next(n_merges ? pto_kw_merged : pto->kw, &pos, &key, &val)) {
            assert(i < pto_nkwds);
            PyTuple_SET_ITEM(tot_kwnames, i, Py_NewRef(key));
            stack[tot_nargs + i] = val;
            i++;
        }
        assert(i == pto_nkwds);
        Py_XDECREF(pto_kw_merged);

        /* Resize Stack if the removing overallocation saves some noticable memory
         * NOTE: This whole block can be removed without breaking anything */
        Py_ssize_t noveralloc = n_merges + nkwds;
        if (stack != small_stack && noveralloc > 6 && noveralloc > init_stack_size / 10) {
            tmp_stack = PyMem_Realloc(stack, (tot_nargskw - n_merges) * sizeof(PyObject *));
            if (tmp_stack == NULL) {
                Py_DECREF(tot_kwnames);
                if (stack != small_stack) {
                    PyMem_Free(stack);
                }
                return PyErr_NoMemory();
            }
            stack = tmp_stack;
        }
    }

    /* Copy Positionals to stack */
    if (pto_phcount) {
        Py_ssize_t j = 0;       // New args index
        for (Py_ssize_t i = 0; i < pto_nargs; i++) {
            if (pto_args[i] == pto->placeholder) {
                stack[i] = args[j];
                j += 1;
            }
            else {
                stack[i] = pto_args[i];
            }
        }
        assert(j == pto_phcount);
        /* Add remaining args from new_args */
        if (nargs > pto_phcount) {
            memcpy(stack + pto_nargs, args + j, (nargs - j) * sizeof(PyObject*));
        }
    }
    else {
        memcpy(stack, pto_args, pto_nargs * sizeof(PyObject*));
        memcpy(stack + pto_nargs, args, nargs * sizeof(PyObject*));
    }

    PyObject *ret = _PyObject_VectorcallTstate(tstate, pto->fn, stack,
                                               tot_nargs, tot_kwnames);
    if (stack != small_stack) {
        PyMem_Free(stack);
    }
    if (pto_nkwds) {
        Py_DECREF(tot_kwnames);
    }
    return ret;

 error:
    if (stack != small_stack) {
        PyMem_Free(stack);
    }
    return NULL;
}

/* Set pto->vectorcall depending on the parameters of the partial object */
static void
partial_setvectorcall(partialobject *pto)
{
    if (PyVectorcall_Function(pto->fn) == NULL) {
        /* Don't use vectorcall if the underlying function doesn't support it */
        pto->vectorcall = NULL;
    }
    /* We could have a special case if there are no arguments,
     * but that is unlikely (why use partial without arguments?),
     * so we don't optimize that */
    else {
        pto->vectorcall = partial_vectorcall;
    }
}


// Not converted to argument clinic, because of `*args, **kwargs` arguments.
static PyObject *
partial_call(PyObject *self, PyObject *args, PyObject *kwargs)
{
    partialobject *pto = partialobject_CAST(self);
    assert(PyCallable_Check(pto->fn));
    assert(PyTuple_Check(pto->args));
    assert(PyDict_Check(pto->kw));

    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t pto_phcount = pto->phcount;
    if (nargs < pto_phcount) {
        PyErr_Format(PyExc_TypeError,
                     "missing positional arguments in 'partial' call; "
                     "expected at least %zd, got %zd", pto_phcount, nargs);
        return NULL;
    }

    /* Merge keywords */
    PyObject *tot_kw;
    if (PyDict_GET_SIZE(pto->kw) == 0) {
        /* kwargs can be NULL */
        tot_kw = Py_XNewRef(kwargs);
    }
    else {
        /* bpo-27840, bpo-29318: dictionary of keyword parameters must be
           copied, because a function using "**kwargs" can modify the
           dictionary. */
        tot_kw = PyDict_Copy(pto->kw);
        if (tot_kw == NULL) {
            return NULL;
        }

        if (kwargs != NULL) {
            if (PyDict_Merge(tot_kw, kwargs, 1) != 0) {
                Py_DECREF(tot_kw);
                return NULL;
            }
        }
    }

    /* Merge positional arguments */
    PyObject *tot_args;
    if (pto_phcount) {
        Py_ssize_t pto_nargs = PyTuple_GET_SIZE(pto->args);
        Py_ssize_t tot_nargs = pto_nargs + nargs - pto_phcount;
        assert(tot_nargs >= 0);
        tot_args = PyTuple_New(tot_nargs);
        if (tot_args == NULL) {
            Py_XDECREF(tot_kw);
            return NULL;
        }
        PyObject *pto_args = pto->args;
        PyObject *item;
        Py_ssize_t j = 0;   // New args index
        for (Py_ssize_t i = 0; i < pto_nargs; i++) {
            item = PyTuple_GET_ITEM(pto_args, i);
            if (item == pto->placeholder) {
                item = PyTuple_GET_ITEM(args, j);
                j += 1;
            }
            Py_INCREF(item);
            PyTuple_SET_ITEM(tot_args, i, item);
        }
        assert(j == pto_phcount);
        for (Py_ssize_t i = pto_nargs; i < tot_nargs; i++) {
            item = PyTuple_GET_ITEM(args, j);
            Py_INCREF(item);
            PyTuple_SET_ITEM(tot_args, i, item);
            j += 1;
        }
    }
    else {
        /* Note: tupleconcat() is optimized for empty tuples */
        tot_args = PySequence_Concat(pto->args, args);
        if (tot_args == NULL) {
            Py_XDECREF(tot_kw);
            return NULL;
        }
    }

    PyObject *res = PyObject_Call(pto->fn, tot_args, tot_kw);
    Py_DECREF(tot_args);
    Py_XDECREF(tot_kw);
    return res;
}

PyDoc_STRVAR(partial_doc,
"partial(func, /, *args, **keywords)\n--\n\n\
Create a new function with partial application of the given arguments\n\
and keywords.");

#define OFF(x) offsetof(partialobject, x)
static PyMemberDef partial_memberlist[] = {
    {"func",            _Py_T_OBJECT,       OFF(fn),        Py_READONLY,
     "function object to use in future partial calls"},
    {"args",            _Py_T_OBJECT,       OFF(args),      Py_READONLY,
     "tuple of arguments to future partial calls"},
    {"keywords",        _Py_T_OBJECT,       OFF(kw),        Py_READONLY,
     "dictionary of keyword arguments to future partial calls"},
    {"__weaklistoffset__", Py_T_PYSSIZET,
     offsetof(partialobject, weakreflist), Py_READONLY},
    {"__dictoffset__", Py_T_PYSSIZET,
     offsetof(partialobject, dict), Py_READONLY},
    {"__vectorcalloffset__", Py_T_PYSSIZET,
     offsetof(partialobject, vectorcall), Py_READONLY},
    {NULL}  /* Sentinel */
};

static PyGetSetDef partial_getsetlist[] = {
    {"__dict__", PyObject_GenericGetDict, PyObject_GenericSetDict},
    {NULL} /* Sentinel */
};

static PyObject *
partial_repr(PyObject *self)
{
    partialobject *pto = partialobject_CAST(self);
    PyObject *result = NULL;
    PyObject *arglist;
    PyObject *mod;
    PyObject *name;
    Py_ssize_t i, n;
    PyObject *key, *value;
    int status;

    status = Py_ReprEnter(self);
    if (status != 0) {
        if (status < 0)
            return NULL;
        return PyUnicode_FromString("...");
    }

    arglist = Py_GetConstant(Py_CONSTANT_EMPTY_STR);
    if (arglist == NULL)
        goto done;
    /* Pack positional arguments */
    assert(PyTuple_Check(pto->args));
    n = PyTuple_GET_SIZE(pto->args);
    for (i = 0; i < n; i++) {
        Py_SETREF(arglist, PyUnicode_FromFormat("%U, %R", arglist,
                                        PyTuple_GET_ITEM(pto->args, i)));
        if (arglist == NULL)
            goto done;
    }
    /* Pack keyword arguments */
    assert (PyDict_Check(pto->kw));
    for (i = 0; PyDict_Next(pto->kw, &i, &key, &value);) {
        /* Prevent key.__str__ from deleting the value. */
        Py_INCREF(value);
        Py_SETREF(arglist, PyUnicode_FromFormat("%U, %S=%R", arglist,
                                                key, value));
        Py_DECREF(value);
        if (arglist == NULL)
            goto done;
    }

    mod = PyType_GetModuleName(Py_TYPE(pto));
    if (mod == NULL) {
        goto error;
    }
    name = PyType_GetQualName(Py_TYPE(pto));
    if (name == NULL) {
        Py_DECREF(mod);
        goto error;
    }
    result = PyUnicode_FromFormat("%S.%S(%R%U)", mod, name, pto->fn, arglist);
    Py_DECREF(mod);
    Py_DECREF(name);
    Py_DECREF(arglist);

 done:
    Py_ReprLeave(self);
    return result;
 error:
    Py_DECREF(arglist);
    Py_ReprLeave(self);
    return NULL;
}

/* Pickle strategy:
   __reduce__ by itself doesn't support getting kwargs in the unpickle
   operation so we define a __setstate__ that replaces all the information
   about the partial.  If we only replaced part of it someone would use
   it as a hook to do strange things.
 */

static PyObject *
partial_reduce(PyObject *self, PyObject *Py_UNUSED(args))
{
    partialobject *pto = partialobject_CAST(self);
    return Py_BuildValue("O(O)(OOOO)", Py_TYPE(pto), pto->fn, pto->fn,
                         pto->args, pto->kw,
                         pto->dict ? pto->dict : Py_None);
}

static PyObject *
partial_setstate(PyObject *self, PyObject *state)
{
    partialobject *pto = partialobject_CAST(self);
    PyObject *fn, *fnargs, *kw, *dict;

    if (!PyTuple_Check(state)) {
        PyErr_SetString(PyExc_TypeError, "invalid partial state");
        return NULL;
    }
    if (!PyArg_ParseTuple(state, "OOOO", &fn, &fnargs, &kw, &dict) ||
        !PyCallable_Check(fn) ||
        !PyTuple_Check(fnargs) ||
        (kw != Py_None && !PyDict_Check(kw)))
    {
        PyErr_SetString(PyExc_TypeError, "invalid partial state");
        return NULL;
    }

    Py_ssize_t nargs = PyTuple_GET_SIZE(fnargs);
    if (nargs && PyTuple_GET_ITEM(fnargs, nargs - 1) == pto->placeholder) {
        PyErr_SetString(PyExc_TypeError,
                        "trailing Placeholders are not allowed");
        return NULL;
    }
    /* Count placeholders */
    Py_ssize_t phcount = 0;
    for (Py_ssize_t i = 0; i < nargs - 1; i++) {
        if (PyTuple_GET_ITEM(fnargs, i) == pto->placeholder) {
            phcount++;
        }
    }

    if(!PyTuple_CheckExact(fnargs))
        fnargs = PySequence_Tuple(fnargs);
    else
        Py_INCREF(fnargs);
    if (fnargs == NULL)
        return NULL;

    if (kw == Py_None)
        kw = PyDict_New();
    else if(!PyDict_CheckExact(kw))
        kw = PyDict_Copy(kw);
    else
        Py_INCREF(kw);
    if (kw == NULL) {
        Py_DECREF(fnargs);
        return NULL;
    }

    if (dict == Py_None)
        dict = NULL;
    else
        Py_INCREF(dict);
    Py_SETREF(pto->fn, Py_NewRef(fn));
    Py_SETREF(pto->args, fnargs);
    Py_SETREF(pto->kw, kw);
    pto->phcount = phcount;
    Py_XSETREF(pto->dict, dict);
    partial_setvectorcall(pto);
    Py_RETURN_NONE;
}

static PyMethodDef partial_methods[] = {
    {"__reduce__", partial_reduce, METH_NOARGS},
    {"__setstate__", partial_setstate, METH_O},
    {"__class_getitem__",    Py_GenericAlias,
    METH_O|METH_CLASS,       PyDoc_STR("See PEP 585")},
    {NULL,              NULL}           /* sentinel */
};

static PyType_Slot partial_type_slots[] = {
    {Py_tp_dealloc, partial_dealloc},
    {Py_tp_repr, partial_repr},
    {Py_tp_call, partial_call},
    {Py_tp_getattro, PyObject_GenericGetAttr},
    {Py_tp_setattro, PyObject_GenericSetAttr},
    {Py_tp_doc, (void *)partial_doc},
    {Py_tp_traverse, partial_traverse},
    {Py_tp_clear, partial_clear},
    {Py_tp_methods, partial_methods},
    {Py_tp_members, partial_memberlist},
    {Py_tp_getset, partial_getsetlist},
    {Py_tp_descr_get, partial_descr_get},
    {Py_tp_new, partial_new},
    {Py_tp_free, PyObject_GC_Del},
    {0, 0}
};

static PyType_Spec partial_type_spec = {
    .name = "functools.partial",
    .basicsize = sizeof(partialobject),
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
             Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_VECTORCALL |
             Py_TPFLAGS_IMMUTABLETYPE,
    .slots = partial_type_slots
};


/* cmp_to_key ***************************************************************/

typedef struct {
    PyObject_HEAD
    PyObject *cmp;
    PyObject *object;
} keyobject;

#define keyobject_CAST(op)  ((keyobject *)(op))

static int
keyobject_clear(PyObject *op)
{
    keyobject *ko = keyobject_CAST(op);
    Py_CLEAR(ko->cmp);
    Py_CLEAR(ko->object);
    return 0;
}

static void
keyobject_dealloc(PyObject *ko)
{
    PyTypeObject *tp = Py_TYPE(ko);
    PyObject_GC_UnTrack(ko);
    (void)keyobject_clear(ko);
    tp->tp_free(ko);
    Py_DECREF(tp);
}

static int
keyobject_traverse(PyObject *op, visitproc visit, void *arg)
{
    keyobject *ko = keyobject_CAST(op);
    Py_VISIT(Py_TYPE(ko));
    Py_VISIT(ko->cmp);
    Py_VISIT(ko->object);
    return 0;
}

static PyMemberDef keyobject_members[] = {
    {"obj", _Py_T_OBJECT,
     offsetof(keyobject, object), 0,
     PyDoc_STR("Value wrapped by a key function.")},
    {NULL}
};

static PyObject *
keyobject_text_signature(PyObject *Py_UNUSED(self), void *Py_UNUSED(ignored))
{
    return PyUnicode_FromString("(obj)");
}

static PyGetSetDef keyobject_getset[] = {
    {"__text_signature__", keyobject_text_signature, NULL},
    {NULL}
};

static PyObject *
keyobject_call(PyObject *ko, PyObject *args, PyObject *kwds);

static PyObject *
keyobject_richcompare(PyObject *ko, PyObject *other, int op);

static PyType_Slot keyobject_type_slots[] = {
    {Py_tp_dealloc, keyobject_dealloc},
    {Py_tp_call, keyobject_call},
    {Py_tp_getattro, PyObject_GenericGetAttr},
    {Py_tp_traverse, keyobject_traverse},
    {Py_tp_clear, keyobject_clear},
    {Py_tp_richcompare, keyobject_richcompare},
    {Py_tp_members, keyobject_members},
    {Py_tp_getset, keyobject_getset},
    {0, 0}
};

static PyType_Spec keyobject_type_spec = {
    .name = "functools.KeyWrapper",
    .basicsize = sizeof(keyobject),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION |
              Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_IMMUTABLETYPE),
    .slots = keyobject_type_slots
};

static PyObject *
keyobject_call(PyObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *object;
    keyobject *result;
    static char *kwargs[] = {"obj", NULL};
    keyobject *ko = keyobject_CAST(self);

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O:K", kwargs, &object))
        return NULL;

    result = PyObject_GC_New(keyobject, Py_TYPE(ko));
    if (result == NULL) {
        return NULL;
    }
    result->cmp = Py_NewRef(ko->cmp);
    result->object = Py_NewRef(object);
    PyObject_GC_Track(result);
    return (PyObject *)result;
}

static PyObject *
keyobject_richcompare(PyObject *self, PyObject *other, int op)
{
    if (!Py_IS_TYPE(other, Py_TYPE(self))) {
        PyErr_Format(PyExc_TypeError, "other argument must be K instance");
        return NULL;
    }

    keyobject *lhs = keyobject_CAST(self);
    keyobject *rhs = keyobject_CAST(other);

    PyObject *compare = lhs->cmp;
    assert(compare != NULL);
    PyObject *x = lhs->object;
    PyObject *y = rhs->object;
    if (!x || !y){
        PyErr_Format(PyExc_AttributeError, "object");
        return NULL;
    }

    /* Call the user's comparison function and translate the 3-way
     * result into true or false (or error).
     */
    PyObject* args[2] = {x, y};
    PyObject *res = PyObject_Vectorcall(compare, args, 2, NULL);
    if (res == NULL) {
        return NULL;
    }

    PyObject *answer = PyObject_RichCompare(res, _PyLong_GetZero(), op);
    Py_DECREF(res);
    return answer;
}

/*[clinic input]
_functools.cmp_to_key

    mycmp: object
        Function that compares two objects.

Convert a cmp= function into a key= function.
[clinic start generated code]*/

static PyObject *
_functools_cmp_to_key_impl(PyObject *module, PyObject *mycmp)
/*[clinic end generated code: output=71eaad0f4fc81f33 input=d1b76f231c0dfeb3]*/
{
    keyobject *object;
    _functools_state *state;

    state = get_functools_state(module);
    object = PyObject_GC_New(keyobject, state->keyobject_type);
    if (!object)
        return NULL;
    object->cmp = Py_NewRef(mycmp);
    object->object = NULL;
    PyObject_GC_Track(object);
    return (PyObject *)object;
}

/* reduce (used to be a builtin) ********************************************/

/*[clinic input]
_functools.reduce

    function as func: object
    iterable as seq: object
    /
    initial as result: object = NULL

Apply a function of two arguments cumulatively to the items of an iterable, from left to right.

This effectively reduces the iterable to a single value.  If initial is present,
it is placed before the items of the iterable in the calculation, and serves as
a default when the iterable is empty.

For example, reduce(lambda x, y: x+y, [1, 2, 3, 4, 5])
calculates ((((1 + 2) + 3) + 4) + 5).
[clinic start generated code]*/

static PyObject *
_functools_reduce_impl(PyObject *module, PyObject *func, PyObject *seq,
                       PyObject *result)
/*[clinic end generated code: output=30d898fe1267c79d input=1511e9a8c38581ac]*/
{
    PyObject *args, *it;

    if (result != NULL)
        Py_INCREF(result);

    it = PyObject_GetIter(seq);
    if (it == NULL) {
        if (PyErr_ExceptionMatches(PyExc_TypeError))
            PyErr_SetString(PyExc_TypeError,
                            "reduce() arg 2 must support iteration");
        Py_XDECREF(result);
        return NULL;
    }

    if ((args = PyTuple_New(2)) == NULL)
        goto Fail;

    for (;;) {
        PyObject *op2;

        if (Py_REFCNT(args) > 1) {
            Py_DECREF(args);
            if ((args = PyTuple_New(2)) == NULL)
                goto Fail;
        }

        op2 = PyIter_Next(it);
        if (op2 == NULL) {
            if (PyErr_Occurred())
                goto Fail;
            break;
        }

        if (result == NULL)
            result = op2;
        else {
            /* Update the args tuple in-place */
            assert(Py_REFCNT(args) == 1);
            Py_XSETREF(_PyTuple_ITEMS(args)[0], result);
            Py_XSETREF(_PyTuple_ITEMS(args)[1], op2);
            if ((result = PyObject_Call(func, args, NULL)) == NULL) {
                goto Fail;
            }
            // bpo-42536: The GC may have untracked this args tuple. Since we're
            // recycling it, make sure it's tracked again:
            _PyTuple_Recycle(args);
        }
    }

    Py_DECREF(args);

    if (result == NULL)
        PyErr_SetString(PyExc_TypeError,
                        "reduce() of empty iterable with no initial value");

    Py_DECREF(it);
    return result;

Fail:
    Py_XDECREF(args);
    Py_XDECREF(result);
    Py_DECREF(it);
    return NULL;
}

/* lru_cache object **********************************************************/

/* There are four principal algorithmic differences from the pure python version:

   1). The C version relies on the GIL instead of having its own reentrant lock.

   2). The prev/next link fields use borrowed references.

   3). For a full cache, the pure python version rotates the location of the
       root entry so that it never has to move individual links and it can
       limit updates to just the key and result fields.  However, in the C
       version, links are temporarily removed while the cache dict updates are
       occurring. Afterwards, they are appended or prepended back into the
       doubly-linked lists.

   4)  In the Python version, the _HashSeq class is used to prevent __hash__
       from being called more than once.  In the C version, the "known hash"
       variants of dictionary calls as used to the same effect.

*/

struct lru_list_elem;
struct lru_cache_object;

typedef struct lru_list_elem {
    PyObject_HEAD
    struct lru_list_elem *prev, *next;  /* borrowed links */
    Py_hash_t hash;
    PyObject *key, *result;
} lru_list_elem;

#define lru_list_elem_CAST(op)  ((lru_list_elem *)(op))

static void
lru_list_elem_dealloc(PyObject *op)
{
    lru_list_elem *link = lru_list_elem_CAST(op);
    PyTypeObject *tp = Py_TYPE(link);
    Py_XDECREF(link->key);
    Py_XDECREF(link->result);
    tp->tp_free(link);
    Py_DECREF(tp);
}

static PyType_Slot lru_list_elem_type_slots[] = {
    {Py_tp_dealloc, lru_list_elem_dealloc},
    {0, 0}
};

static PyType_Spec lru_list_elem_type_spec = {
    .name = "functools._lru_list_elem",
    .basicsize = sizeof(lru_list_elem),
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION |
             Py_TPFLAGS_IMMUTABLETYPE,
    .slots = lru_list_elem_type_slots
};


typedef PyObject *(*lru_cache_ternaryfunc)(struct lru_cache_object *, PyObject *, PyObject *);

typedef struct lru_cache_object {
    lru_list_elem root;  /* includes PyObject_HEAD */
    lru_cache_ternaryfunc wrapper;
    int typed;
    PyObject *cache;
    Py_ssize_t hits;
    PyObject *func;
    Py_ssize_t maxsize;
    Py_ssize_t misses;
    /* the kwd_mark is used delimit args and keywords in the cache keys */
    PyObject *kwd_mark;
    PyTypeObject *lru_list_elem_type;
    PyObject *cache_info_type;
    PyObject *dict;
    PyObject *weakreflist;
} lru_cache_object;

#define lru_cache_object_CAST(op)   ((lru_cache_object *)(op))

static PyObject *
lru_cache_make_key(PyObject *kwd_mark, PyObject *args,
                   PyObject *kwds, int typed)
{
    PyObject *key, *keyword, *value;
    Py_ssize_t key_size, pos, key_pos, kwds_size;

    kwds_size = kwds ? PyDict_GET_SIZE(kwds) : 0;

    /* short path, key will match args anyway, which is a tuple */
    if (!typed && !kwds_size) {
        if (PyTuple_GET_SIZE(args) == 1) {
            key = PyTuple_GET_ITEM(args, 0);
            if (PyUnicode_CheckExact(key) || PyLong_CheckExact(key)) {
                /* For common scalar keys, save space by
                   dropping the enclosing args tuple  */
                return Py_NewRef(key);
            }
        }
        return Py_NewRef(args);
    }

    key_size = PyTuple_GET_SIZE(args);
    if (kwds_size)
        key_size += kwds_size * 2 + 1;
    if (typed)
        key_size += PyTuple_GET_SIZE(args) + kwds_size;

    key = PyTuple_New(key_size);
    if (key == NULL)
        return NULL;

    key_pos = 0;
    for (pos = 0; pos < PyTuple_GET_SIZE(args); ++pos) {
        PyObject *item = PyTuple_GET_ITEM(args, pos);
        PyTuple_SET_ITEM(key, key_pos++, Py_NewRef(item));
    }
    if (kwds_size) {
        PyTuple_SET_ITEM(key, key_pos++, Py_NewRef(kwd_mark));
        for (pos = 0; PyDict_Next(kwds, &pos, &keyword, &value);) {
            PyTuple_SET_ITEM(key, key_pos++, Py_NewRef(keyword));
            PyTuple_SET_ITEM(key, key_pos++, Py_NewRef(value));
        }
        assert(key_pos == PyTuple_GET_SIZE(args) + kwds_size * 2 + 1);
    }
    if (typed) {
        for (pos = 0; pos < PyTuple_GET_SIZE(args); ++pos) {
            PyObject *item = (PyObject *)Py_TYPE(PyTuple_GET_ITEM(args, pos));
            PyTuple_SET_ITEM(key, key_pos++, Py_NewRef(item));
        }
        if (kwds_size) {
            for (pos = 0; PyDict_Next(kwds, &pos, &keyword, &value);) {
                PyObject *item = (PyObject *)Py_TYPE(value);
                PyTuple_SET_ITEM(key, key_pos++, Py_NewRef(item));
            }
        }
    }
    assert(key_pos == key_size);
    return key;
}

static PyObject *
uncached_lru_cache_wrapper(lru_cache_object *self, PyObject *args, PyObject *kwds)
{
    PyObject *result;

    FT_ATOMIC_ADD_SSIZE(self->misses, 1);
    result = PyObject_Call(self->func, args, kwds);
    if (!result)
        return NULL;
    return result;
}

static PyObject *
infinite_lru_cache_wrapper(lru_cache_object *self, PyObject *args, PyObject *kwds)
{
    PyObject *result;
    Py_hash_t hash;
    PyObject *key = lru_cache_make_key(self->kwd_mark, args, kwds, self->typed);
    if (!key)
        return NULL;
    hash = PyObject_Hash(key);
    if (hash == -1) {
        Py_DECREF(key);
        return NULL;
    }
    int res = _PyDict_GetItemRef_KnownHash((PyDictObject *)self->cache, key, hash, &result);
    if (res > 0) {
        FT_ATOMIC_ADD_SSIZE(self->hits, 1);
        Py_DECREF(key);
        return result;
    }
    if (res < 0) {
        Py_DECREF(key);
        return NULL;
    }
    FT_ATOMIC_ADD_SSIZE(self->misses, 1);
    result = PyObject_Call(self->func, args, kwds);
    if (!result) {
        Py_DECREF(key);
        return NULL;
    }
    if (_PyDict_SetItem_KnownHash(self->cache, key, result, hash) < 0) {
        Py_DECREF(result);
        Py_DECREF(key);
        return NULL;
    }
    Py_DECREF(key);
    return result;
}

static void
lru_cache_extract_link(lru_list_elem *link)
{
    lru_list_elem *link_prev = link->prev;
    lru_list_elem *link_next = link->next;
    link_prev->next = link->next;
    link_next->prev = link->prev;
}

static void
lru_cache_append_link(lru_cache_object *self, lru_list_elem *link)
{
    lru_list_elem *root = &self->root;
    lru_list_elem *last = root->prev;
    last->next = root->prev = link;
    link->prev = last;
    link->next = root;
}

static void
lru_cache_prepend_link(lru_cache_object *self, lru_list_elem *link)
{
    lru_list_elem *root = &self->root;
    lru_list_elem *first = root->next;
    first->prev = root->next = link;
    link->prev = root;
    link->next = first;
}

/* General note on reentrancy:

   There are four dictionary calls in the bounded_lru_cache_wrapper():
   1) The initial check for a cache match.  2) The post user-function
   check for a cache match.  3) The deletion of the oldest entry.
   4) The addition of the newest entry.

   In all four calls, we have a known hash which lets use avoid a call
   to __hash__().  That leaves only __eq__ as a possible source of a
   reentrant call.

   The __eq__ method call is always made for a cache hit (dict access #1).
   Accordingly, we have make sure not modify the cache state prior to
   this call.

   The __eq__ method call is never made for the deletion (dict access #3)
   because it is an identity match.

   For the other two accesses (#2 and #4), calls to __eq__ only occur
   when some other entry happens to have an exactly matching hash (all
   64-bits).  Though rare, this can happen, so we have to make sure to
   either call it at the top of its code path before any cache
   state modifications (dict access #2) or be prepared to restore
   invariants at the end of the code path (dict access #4).

   Another possible source of reentrancy is a decref which can trigger
   arbitrary code execution.  To make the code easier to reason about,
   the decrefs are deferred to the end of the each possible code path
   so that we know the cache is a consistent state.
 */

static int
bounded_lru_cache_get_lock_held(lru_cache_object *self, PyObject *args, PyObject *kwds,
                                PyObject **result, PyObject **key, Py_hash_t *hash)
{
    _Py_CRITICAL_SECTION_ASSERT_OBJECT_LOCKED(self);
    lru_list_elem *link;

    PyObject *key_ = *key = lru_cache_make_key(self->kwd_mark, args, kwds, self->typed);
    if (!key_)
        return -1;
    Py_hash_t hash_ = *hash = PyObject_Hash(key_);
    if (hash_ == -1) {
        Py_DECREF(key_);  /* dead reference left in *key, is not used */
        return -1;
    }
    int res = _PyDict_GetItemRef_KnownHash_LockHeld((PyDictObject *)self->cache, key_, hash_,
                                                    (PyObject **)&link);
    if (res > 0) {
        lru_cache_extract_link(link);
        lru_cache_append_link(self, link);
        *result = link->result;
        FT_ATOMIC_ADD_SSIZE(self->hits, 1);
        Py_INCREF(link->result);
        Py_DECREF(link);
        Py_DECREF(key_);
        return 1;
    }
    if (res < 0) {
        Py_DECREF(key_);
        return -1;
    }
    FT_ATOMIC_ADD_SSIZE(self->misses, 1);
    return 0;
}

static PyObject *
bounded_lru_cache_update_lock_held(lru_cache_object *self,
                                   PyObject *result, PyObject *key, Py_hash_t hash)
{
    _Py_CRITICAL_SECTION_ASSERT_OBJECT_LOCKED(self);
    lru_list_elem *link;
    PyObject *testresult;
    int res;

    if (!result) {
        Py_DECREF(key);
        return NULL;
    }
    res = _PyDict_GetItemRef_KnownHash_LockHeld((PyDictObject *)self->cache, key, hash,
                                                &testresult);
    if (res > 0) {
        /* Getting here means that this same key was added to the cache
           during the PyObject_Call().  Since the link update is already
           done, we need only return the computed result. */
        Py_DECREF(testresult);
        Py_DECREF(key);
        return result;
    }
    if (res < 0) {
        /* This is an unusual case since this same lookup
           did not previously trigger an error during lookup.
           Treat it the same as an error in user function
           and return with the error set. */
        Py_DECREF(key);
        Py_DECREF(result);
        return NULL;
    }
    /* This is the normal case.  The new key wasn't found before
       user function call and it is still not there.  So we
       proceed normally and update the cache with the new result. */

    assert(self->maxsize > 0);
    if (PyDict_GET_SIZE(self->cache) < self->maxsize ||
        self->root.next == &self->root)
    {
        /* Cache is not full, so put the result in a new link */
        link = (lru_list_elem *)PyObject_New(lru_list_elem,
                                             self->lru_list_elem_type);
        if (link == NULL) {
            Py_DECREF(key);
            Py_DECREF(result);
            return NULL;
        }

        link->hash = hash;
        link->key = key;
        link->result = result;
        /* What is really needed here is a SetItem variant with a "no clobber"
           option.  If the __eq__ call triggers a reentrant call that adds
           this same key, then this setitem call will update the cache dict
           with this new link, leaving the old link as an orphan (i.e. not
           having a cache dict entry that refers to it). */
        if (_PyDict_SetItem_KnownHash_LockHeld((PyDictObject *)self->cache, key,
                                               (PyObject *)link, hash) < 0) {
            Py_DECREF(link);
            return NULL;
        }
        lru_cache_append_link(self, link);
        return Py_NewRef(result);
    }
    /* Since the cache is full, we need to evict an old key and add
       a new key.  Rather than free the old link and allocate a new
       one, we reuse the link for the new key and result and move it
       to front of the cache to mark it as recently used.

       We try to assure all code paths (including errors) leave all
       of the links in place.  Either the link is successfully
       updated and moved or it is restored to its old position.
       However if an unrecoverable error is found, it doesn't
       make sense to reinsert the link, so we leave it out
       and the cache will no longer register as full.
    */
    PyObject *oldkey, *oldresult, *popresult;

    /* Extract the oldest item. */
    assert(self->root.next != &self->root);
    link = self->root.next;
    lru_cache_extract_link(link);
    /* Remove it from the cache.
       The cache dict holds one reference to the link.
       We created one other reference when the link was created.
       The linked list only has borrowed references. */
    res = _PyDict_Pop_KnownHash((PyDictObject*)self->cache, link->key,
                                link->hash, &popresult);
    if (res < 0) {
        /* An error arose while trying to remove the oldest key (the one
           being evicted) from the cache.  We restore the link to its
           original position as the oldest link.  Then we allow the
           error propagate upward; treating it the same as an error
           arising in the user function. */
        lru_cache_prepend_link(self, link);
        Py_DECREF(key);
        Py_DECREF(result);
        return NULL;
    }
    if (res == 0) {
        /* Getting here means that the user function call or another
           thread has already removed the old key from the dictionary.
           This link is now an orphan.  Since we don't want to leave the
           cache in an inconsistent state, we don't restore the link. */
        assert(popresult == NULL);
        Py_DECREF(link);
        Py_DECREF(key);
        return result;
    }

    /* Keep a reference to the old key and old result to prevent their
       ref counts from going to zero during the update. That will
       prevent potentially arbitrary object clean-up code (i.e. __del__)
       from running while we're still adjusting the links. */
    assert(popresult != NULL);
    oldkey = link->key;
    oldresult = link->result;

    link->hash = hash;
    link->key = key;
    link->result = result;
    /* Note:  The link is being added to the cache dict without the
       prev and next fields set to valid values.   We have to wait
       for successful insertion in the cache dict before adding the
       link to the linked list.  Otherwise, the potentially reentrant
       __eq__ call could cause the then orphan link to be visited. */
    if (_PyDict_SetItem_KnownHash_LockHeld((PyDictObject *)self->cache, key,
                                           (PyObject *)link, hash) < 0) {
        /* Somehow the cache dict update failed.  We no longer can
           restore the old link.  Let the error propagate upward and
           leave the cache short one link. */
        Py_DECREF(popresult);
        Py_DECREF(link);
        Py_DECREF(oldkey);
        Py_DECREF(oldresult);
        return NULL;
    }
    lru_cache_append_link(self, link);
    Py_INCREF(result); /* for return */
    Py_DECREF(popresult);
    Py_DECREF(oldkey);
    Py_DECREF(oldresult);
    return result;
}

static PyObject *
bounded_lru_cache_wrapper(lru_cache_object *self, PyObject *args, PyObject *kwds)
{
    PyObject *key, *result;
    Py_hash_t hash;
    int res;

    Py_BEGIN_CRITICAL_SECTION(self);
    res = bounded_lru_cache_get_lock_held(self, args, kwds, &result, &key, &hash);
    Py_END_CRITICAL_SECTION();

    if (res < 0) {
        return NULL;
    }
    if (res > 0) {
        return result;
    }

    result = PyObject_Call(self->func, args, kwds);

    Py_BEGIN_CRITICAL_SECTION(self);
    /* Note:  key will be stolen in the below function, and
       result may be stolen or sometimes re-returned as a passthrough.
       Treat both as being stolen.
     */
    result = bounded_lru_cache_update_lock_held(self, result, key, hash);
    Py_END_CRITICAL_SECTION();

    return result;
}

static PyObject *
lru_cache_new(PyTypeObject *type, PyObject *args, PyObject *kw)
{
    PyObject *func, *maxsize_O, *cache_info_type, *cachedict;
    int typed;
    lru_cache_object *obj;
    Py_ssize_t maxsize;
    PyObject *(*wrapper)(lru_cache_object *, PyObject *, PyObject *);
    _functools_state *state;
    static char *keywords[] = {"user_function", "maxsize", "typed",
                               "cache_info_type", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kw, "OOpO:lru_cache", keywords,
                                     &func, &maxsize_O, &typed,
                                     &cache_info_type)) {
        return NULL;
    }

    if (!PyCallable_Check(func)) {
        PyErr_SetString(PyExc_TypeError,
                        "the first argument must be callable");
        return NULL;
    }

    state = get_functools_state_by_type(type);
    if (state == NULL) {
        return NULL;
    }

    /* select the caching function, and make/inc maxsize_O */
    if (maxsize_O == Py_None) {
        wrapper = infinite_lru_cache_wrapper;
        /* use this only to initialize lru_cache_object attribute maxsize */
        maxsize = -1;
    } else if (PyIndex_Check(maxsize_O)) {
        maxsize = PyNumber_AsSsize_t(maxsize_O, PyExc_OverflowError);
        if (maxsize == -1 && PyErr_Occurred())
            return NULL;
        if (maxsize < 0) {
            maxsize = 0;
        }
        if (maxsize == 0)
            wrapper = uncached_lru_cache_wrapper;
        else
            wrapper = bounded_lru_cache_wrapper;
    } else {
        PyErr_SetString(PyExc_TypeError, "maxsize should be integer or None");
        return NULL;
    }

    if (!(cachedict = PyDict_New()))
        return NULL;

    obj = (lru_cache_object *)type->tp_alloc(type, 0);
    if (obj == NULL) {
        Py_DECREF(cachedict);
        return NULL;
    }

    obj->root.prev = &obj->root;
    obj->root.next = &obj->root;
    obj->wrapper = wrapper;
    obj->typed = typed;
    obj->cache = cachedict;
    obj->func = Py_NewRef(func);
    obj->misses = obj->hits = 0;
    obj->maxsize = maxsize;
    obj->kwd_mark = Py_NewRef(state->kwd_mark);
    obj->lru_list_elem_type = (PyTypeObject*)Py_NewRef(state->lru_list_elem_type);
    obj->cache_info_type = Py_NewRef(cache_info_type);
    obj->dict = NULL;
    obj->weakreflist = NULL;
    return (PyObject *)obj;
}

static lru_list_elem *
lru_cache_unlink_list(lru_cache_object *self)
{
    lru_list_elem *root = &self->root;
    lru_list_elem *link = root->next;
    if (link == root)
        return NULL;
    root->prev->next = NULL;
    root->next = root->prev = root;
    return link;
}

static void
lru_cache_clear_list(lru_list_elem *link)
{
    while (link != NULL) {
        lru_list_elem *next = link->next;
        Py_SETREF(link, next);
    }
}

static int
lru_cache_tp_clear(PyObject *op)
{
    lru_cache_object *self = lru_cache_object_CAST(op);
    lru_list_elem *list = lru_cache_unlink_list(self);
    Py_CLEAR(self->cache);
    Py_CLEAR(self->func);
    Py_CLEAR(self->kwd_mark);
    Py_CLEAR(self->lru_list_elem_type);
    Py_CLEAR(self->cache_info_type);
    Py_CLEAR(self->dict);
    lru_cache_clear_list(list);
    return 0;
}

static void
lru_cache_dealloc(PyObject *op)
{
    lru_cache_object *obj = lru_cache_object_CAST(op);
    PyTypeObject *tp = Py_TYPE(obj);
    /* bpo-31095: UnTrack is needed before calling any callbacks */
    PyObject_GC_UnTrack(obj);
    FT_CLEAR_WEAKREFS(op, obj->weakreflist);

    (void)lru_cache_tp_clear(op);
    tp->tp_free(obj);
    Py_DECREF(tp);
}

static PyObject *
lru_cache_call(PyObject *op, PyObject *args, PyObject *kwds)
{
    lru_cache_object *self = lru_cache_object_CAST(op);
    PyObject *result;
    result = self->wrapper(self, args, kwds);
    return result;
}

static PyObject *
lru_cache_descr_get(PyObject *self, PyObject *obj, PyObject *type)
{
    if (obj == Py_None || obj == NULL) {
        return Py_NewRef(self);
    }
    return PyMethod_New(self, obj);
}

/*[clinic input]
@critical_section
_functools._lru_cache_wrapper.cache_info

Report cache statistics
[clinic start generated code]*/

static PyObject *
_functools__lru_cache_wrapper_cache_info_impl(PyObject *self)
/*[clinic end generated code: output=cc796a0b06dbd717 input=00e1acb31aa21ecc]*/
{
    lru_cache_object *_self = (lru_cache_object *) self;
    if (_self->maxsize == -1) {
        return PyObject_CallFunction(_self->cache_info_type, "nnOn",
                                     FT_ATOMIC_LOAD_SSIZE_RELAXED(_self->hits),
                                     FT_ATOMIC_LOAD_SSIZE_RELAXED(_self->misses),
                                     Py_None,
                                     PyDict_GET_SIZE(_self->cache));
    }
    return PyObject_CallFunction(_self->cache_info_type, "nnnn",
                                 FT_ATOMIC_LOAD_SSIZE_RELAXED(_self->hits),
                                 FT_ATOMIC_LOAD_SSIZE_RELAXED(_self->misses),
                                 _self->maxsize,
                                 PyDict_GET_SIZE(_self->cache));
}

/*[clinic input]
@critical_section
_functools._lru_cache_wrapper.cache_clear

Clear the cache and cache statistics
[clinic start generated code]*/

static PyObject *
_functools__lru_cache_wrapper_cache_clear_impl(PyObject *self)
/*[clinic end generated code: output=58423b35efc3e381 input=dfa33acbecf8b4b2]*/
{
    lru_cache_object *_self = (lru_cache_object *) self;
    lru_list_elem *list = lru_cache_unlink_list(_self);
    FT_ATOMIC_STORE_SSIZE_RELAXED(_self->hits, 0);
    FT_ATOMIC_STORE_SSIZE_RELAXED(_self->misses, 0);
    if (_self->wrapper == bounded_lru_cache_wrapper) {
        /* The critical section on the lru cache itself protects the dictionary
           for bounded_lru_cache instances. */
        _PyDict_Clear_LockHeld(_self->cache);
    } else {
        PyDict_Clear(_self->cache);
    }
    lru_cache_clear_list(list);
    Py_RETURN_NONE;
}

static PyObject *
lru_cache_reduce(PyObject *self, PyObject *Py_UNUSED(dummy))
{
    return PyObject_GetAttrString(self, "__qualname__");
}

static PyObject *
lru_cache_copy(PyObject *self, PyObject *Py_UNUSED(args))
{
    return Py_NewRef(self);
}

static PyObject *
lru_cache_deepcopy(PyObject *self, PyObject *Py_UNUSED(args))
{
    return Py_NewRef(self);
}

static int
lru_cache_tp_traverse(PyObject *op, visitproc visit, void *arg)
{
    lru_cache_object *self = lru_cache_object_CAST(op);
    Py_VISIT(Py_TYPE(self));
    lru_list_elem *link = self->root.next;
    while (link != &self->root) {
        lru_list_elem *next = link->next;
        Py_VISIT(link->key);
        Py_VISIT(link->result);
        Py_VISIT(Py_TYPE(link));
        link = next;
    }
    Py_VISIT(self->cache);
    Py_VISIT(self->func);
    Py_VISIT(self->kwd_mark);
    Py_VISIT(self->lru_list_elem_type);
    Py_VISIT(self->cache_info_type);
    Py_VISIT(self->dict);
    return 0;
}


PyDoc_STRVAR(lru_cache_doc,
"Create a cached callable that wraps another function.\n\
\n\
user_function:      the function being cached\n\
\n\
maxsize:  0         for no caching\n\
          None      for unlimited cache size\n\
          n         for a bounded cache\n\
\n\
typed:    False     cache f(3) and f(3.0) as identical calls\n\
          True      cache f(3) and f(3.0) as distinct calls\n\
\n\
cache_info_type:    namedtuple class with the fields:\n\
                        hits misses currsize maxsize\n"
);

static PyMethodDef lru_cache_methods[] = {
    _FUNCTOOLS__LRU_CACHE_WRAPPER_CACHE_INFO_METHODDEF
    _FUNCTOOLS__LRU_CACHE_WRAPPER_CACHE_CLEAR_METHODDEF
    {"__reduce__", lru_cache_reduce, METH_NOARGS},
    {"__copy__", lru_cache_copy, METH_VARARGS},
    {"__deepcopy__", lru_cache_deepcopy, METH_VARARGS},
    {NULL}
};

static PyGetSetDef lru_cache_getsetlist[] = {
    {"__dict__", PyObject_GenericGetDict, PyObject_GenericSetDict},
    {NULL}
};

static PyMemberDef lru_cache_memberlist[] = {
    {"__dictoffset__", Py_T_PYSSIZET,
     offsetof(lru_cache_object, dict), Py_READONLY},
    {"__weaklistoffset__", Py_T_PYSSIZET,
     offsetof(lru_cache_object, weakreflist), Py_READONLY},
    {NULL}  /* Sentinel */
};

static PyType_Slot lru_cache_type_slots[] = {
    {Py_tp_dealloc, lru_cache_dealloc},
    {Py_tp_call, lru_cache_call},
    {Py_tp_doc, (void *)lru_cache_doc},
    {Py_tp_traverse, lru_cache_tp_traverse},
    {Py_tp_clear, lru_cache_tp_clear},
    {Py_tp_methods, lru_cache_methods},
    {Py_tp_members, lru_cache_memberlist},
    {Py_tp_getset, lru_cache_getsetlist},
    {Py_tp_descr_get, lru_cache_descr_get},
    {Py_tp_new, lru_cache_new},
    {0, 0}
};

static PyType_Spec lru_cache_type_spec = {
    .name = "functools._lru_cache_wrapper",
    .basicsize = sizeof(lru_cache_object),
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
             Py_TPFLAGS_METHOD_DESCRIPTOR | Py_TPFLAGS_IMMUTABLETYPE,
    .slots = lru_cache_type_slots
};


/* module level code ********************************************************/

PyDoc_STRVAR(_functools_doc,
"Tools that operate on functions.");

static PyMethodDef _functools_methods[] = {
    _FUNCTOOLS_REDUCE_METHODDEF
    _FUNCTOOLS_CMP_TO_KEY_METHODDEF
    {NULL,              NULL}           /* sentinel */
};

static int
_functools_exec(PyObject *module)
{
    _functools_state *state = get_functools_state(module);
    state->kwd_mark = _PyObject_CallNoArgs((PyObject *)&PyBaseObject_Type);
    if (state->kwd_mark == NULL) {
        return -1;
    }

    state->placeholder_type = (PyTypeObject *)PyType_FromModuleAndSpec(module,
        &placeholder_type_spec, NULL);
    if (state->placeholder_type == NULL) {
        return -1;
    }
    if (PyModule_AddType(module, state->placeholder_type) < 0) {
        return -1;
    }

    PyObject *placeholder = PyObject_CallNoArgs((PyObject *)state->placeholder_type);
    if (placeholder == NULL) {
        return -1;
    }
    if (PyModule_AddObjectRef(module, "Placeholder", placeholder) < 0) {
        Py_DECREF(placeholder);
        return -1;
    }
    Py_DECREF(placeholder);

    state->partial_type = (PyTypeObject *)PyType_FromModuleAndSpec(module,
        &partial_type_spec, NULL);
    if (state->partial_type == NULL) {
        return -1;
    }
    if (PyModule_AddType(module, state->partial_type) < 0) {
        return -1;
    }

    PyObject *lru_cache_type = PyType_FromModuleAndSpec(module,
        &lru_cache_type_spec, NULL);
    if (lru_cache_type == NULL) {
        return -1;
    }
    if (PyModule_AddType(module, (PyTypeObject *)lru_cache_type) < 0) {
        Py_DECREF(lru_cache_type);
        return -1;
    }
    Py_DECREF(lru_cache_type);

    state->keyobject_type = (PyTypeObject *)PyType_FromModuleAndSpec(module,
        &keyobject_type_spec, NULL);
    if (state->keyobject_type == NULL) {
        return -1;
    }
    // keyobject_type is used only internally.
    // So we don't expose it in module namespace.

    state->lru_list_elem_type = (PyTypeObject *)PyType_FromModuleAndSpec(
        module, &lru_list_elem_type_spec, NULL);
    if (state->lru_list_elem_type == NULL) {
        return -1;
    }
    // lru_list_elem is used only in _lru_cache_wrapper.
    // So we don't expose it in module namespace.

    return 0;
}

static int
_functools_traverse(PyObject *module, visitproc visit, void *arg)
{
    _functools_state *state = get_functools_state(module);
    Py_VISIT(state->kwd_mark);
    Py_VISIT(state->placeholder_type);
    Py_VISIT(state->placeholder);
    Py_VISIT(state->partial_type);
    Py_VISIT(state->keyobject_type);
    Py_VISIT(state->lru_list_elem_type);
    return 0;
}

static int
_functools_clear(PyObject *module)
{
    _functools_state *state = get_functools_state(module);
    Py_CLEAR(state->kwd_mark);
    Py_CLEAR(state->placeholder_type);
    Py_CLEAR(state->placeholder);
    Py_CLEAR(state->partial_type);
    Py_CLEAR(state->keyobject_type);
    Py_CLEAR(state->lru_list_elem_type);
    return 0;
}

static void
_functools_free(void *module)
{
    (void)_functools_clear((PyObject *)module);
}

static struct PyModuleDef_Slot _functools_slots[] = {
    {Py_mod_exec, _functools_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL}
};

static struct PyModuleDef _functools_module = {
    PyModuleDef_HEAD_INIT,
    .m_name = "_functools",
    .m_doc = _functools_doc,
    .m_size = sizeof(_functools_state),
    .m_methods = _functools_methods,
    .m_slots = _functools_slots,
    .m_traverse = _functools_traverse,
    .m_clear = _functools_clear,
    .m_free = _functools_free,
};

PyMODINIT_FUNC
PyInit__functools(void)
{
    return PyModuleDef_Init(&_functools_module);
}
