// types.GenericAlias -- used to represent e.g. list[int].

#include "Python.h"
#include "pycore_ceval.h"         // _PyEval_GetBuiltin()
#include "pycore_modsupport.h"    // _PyArg_NoKeywords()
#include "pycore_object.h"
#include "pycore_typevarobject.h" // _Py_typing_type_repr
#include "pycore_unicodeobject.h" // _PyUnicode_EqualToASCIIString()
#include "pycore_unionobject.h"   // _Py_union_type_or, _PyGenericAlias_Check


#include <stdbool.h>

typedef struct {
    PyObject_HEAD
    PyObject *origin;
    PyObject *args;
    PyObject *parameters;
    PyObject *weakreflist;
    // Whether we're a starred type, e.g. *tuple[int].
    bool starred;
    vectorcallfunc vectorcall;
} gaobject;

typedef struct {
    PyObject_HEAD
    PyObject *obj;  /* Set to NULL when iterator is exhausted */
} gaiterobject;

static void
ga_dealloc(PyObject *self)
{
    gaobject *alias = (gaobject *)self;

    _PyObject_GC_UNTRACK(self);
    if (alias->weakreflist != NULL) {
        PyObject_ClearWeakRefs((PyObject *)alias);
    }
    Py_XDECREF(alias->origin);
    Py_XDECREF(alias->args);
    Py_XDECREF(alias->parameters);
    Py_TYPE(self)->tp_free(self);
}

static int
ga_traverse(PyObject *self, visitproc visit, void *arg)
{
    gaobject *alias = (gaobject *)self;
    Py_VISIT(alias->origin);
    Py_VISIT(alias->args);
    Py_VISIT(alias->parameters);
    return 0;
}

static int
ga_repr_items_list(PyUnicodeWriter *writer, PyObject *p)
{
    assert(PyList_CheckExact(p));

    Py_ssize_t len = PyList_GET_SIZE(p);

    if (PyUnicodeWriter_WriteChar(writer, '[') < 0) {
        return -1;
    }

    for (Py_ssize_t i = 0; i < len; i++) {
        if (i > 0) {
            if (PyUnicodeWriter_WriteUTF8(writer, ", ", 2) < 0) {
                return -1;
            }
        }
        PyObject *item = PyList_GET_ITEM(p, i);
        if (_Py_typing_type_repr(writer, item) < 0) {
            return -1;
        }
    }

    if (PyUnicodeWriter_WriteChar(writer, ']') < 0) {
        return -1;
    }

    return 0;
}

static PyObject *
ga_repr(PyObject *self)
{
    gaobject *alias = (gaobject *)self;
    Py_ssize_t len = PyTuple_GET_SIZE(alias->args);

    // Estimation based on the shortest format: "int[int, int, int]"
    Py_ssize_t estimate = (len <= PY_SSIZE_T_MAX / 5) ? len * 5 : len;
    estimate = 3 + 1 + estimate + 1;
    PyUnicodeWriter *writer = PyUnicodeWriter_Create(estimate);
    if (writer == NULL) {
        return NULL;
    }

    if (alias->starred) {
        if (PyUnicodeWriter_WriteChar(writer, '*') < 0) {
            goto error;
        }
    }
    if (_Py_typing_type_repr(writer, alias->origin) < 0) {
        goto error;
    }
    if (PyUnicodeWriter_WriteChar(writer, '[') < 0) {
        goto error;
    }
    for (Py_ssize_t i = 0; i < len; i++) {
        if (i > 0) {
            if (PyUnicodeWriter_WriteUTF8(writer, ", ", 2) < 0) {
                goto error;
            }
        }
        PyObject *p = PyTuple_GET_ITEM(alias->args, i);
        if (PyList_CheckExact(p)) {
            // Looks like we are working with ParamSpec's list of type args:
            if (ga_repr_items_list(writer, p) < 0) {
                goto error;
            }
        }
        else if (_Py_typing_type_repr(writer, p) < 0) {
            goto error;
        }
    }
    if (len == 0) {
        // for something like tuple[()] we should print a "()"
        if (PyUnicodeWriter_WriteUTF8(writer, "()", 2) < 0) {
            goto error;
        }
    }
    if (PyUnicodeWriter_WriteChar(writer, ']') < 0) {
        goto error;
    }
    return PyUnicodeWriter_Finish(writer);

error:
    PyUnicodeWriter_Discard(writer);
    return NULL;
}

// Index of item in self[:len], or -1 if not found (self is a tuple)
static Py_ssize_t
tuple_index(PyObject *self, Py_ssize_t len, PyObject *item)
{
    for (Py_ssize_t i = 0; i < len; i++) {
        if (PyTuple_GET_ITEM(self, i) == item) {
            return i;
        }
    }
    return -1;
}

static int
tuple_add(PyObject *self, Py_ssize_t len, PyObject *item)
{
    if (tuple_index(self, len, item) < 0) {
        PyTuple_SET_ITEM(self, len, Py_NewRef(item));
        return 1;
    }
    return 0;
}

static Py_ssize_t
tuple_extend(PyObject **dst, Py_ssize_t dstindex,
             PyObject **src, Py_ssize_t count)
{
    assert(count >= 0);
    if (_PyTuple_Resize(dst, PyTuple_GET_SIZE(*dst) + count - 1) != 0) {
        return -1;
    }
    assert(dstindex + count <= PyTuple_GET_SIZE(*dst));
    for (Py_ssize_t i = 0; i < count; ++i) {
        PyObject *item = src[i];
        PyTuple_SET_ITEM(*dst, dstindex + i, Py_NewRef(item));
    }
    return dstindex + count;
}

PyObject *
_Py_make_parameters(PyObject *args)
{
    assert(PyTuple_Check(args) || PyList_Check(args));
    const bool is_args_list = PyList_Check(args);
    PyObject *tuple_args = NULL;
    if (is_args_list) {
        args = tuple_args = PySequence_Tuple(args);
        if (args == NULL) {
            return NULL;
        }
    }
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t len = nargs;
    PyObject *parameters = PyTuple_New(len);
    if (parameters == NULL) {
        Py_XDECREF(tuple_args);
        return NULL;
    }
    Py_ssize_t iparam = 0;
    for (Py_ssize_t iarg = 0; iarg < nargs; iarg++) {
        PyObject *t = PyTuple_GET_ITEM(args, iarg);
        // We don't want __parameters__ descriptor of a bare Python class.
        if (PyType_Check(t)) {
            continue;
        }
        int rc = PyObject_HasAttrWithError(t, &_Py_ID(__typing_subst__));
        if (rc < 0) {
            Py_DECREF(parameters);
            Py_XDECREF(tuple_args);
            return NULL;
        }
        if (rc) {
            iparam += tuple_add(parameters, iparam, t);
        }
        else {
            PyObject *subparams;
            if (PyObject_GetOptionalAttr(t, &_Py_ID(__parameters__),
                                     &subparams) < 0) {
                Py_DECREF(parameters);
                Py_XDECREF(tuple_args);
                return NULL;
            }
            if (!subparams && (PyTuple_Check(t) || PyList_Check(t))) {
                // Recursively call _Py_make_parameters for lists/tuples and
                // add the results to the current parameters.
                subparams = _Py_make_parameters(t);
                if (subparams == NULL) {
                    Py_DECREF(parameters);
                    Py_XDECREF(tuple_args);
                    return NULL;
                }
            }
            if (subparams && PyTuple_Check(subparams)) {
                Py_ssize_t len2 = PyTuple_GET_SIZE(subparams);
                Py_ssize_t needed = len2 - 1 - (iarg - iparam);
                if (needed > 0) {
                    len += needed;
                    if (_PyTuple_Resize(&parameters, len) < 0) {
                        Py_DECREF(subparams);
                        Py_DECREF(parameters);
                        Py_XDECREF(tuple_args);
                        return NULL;
                    }
                }
                for (Py_ssize_t j = 0; j < len2; j++) {
                    PyObject *t2 = PyTuple_GET_ITEM(subparams, j);
                    iparam += tuple_add(parameters, iparam, t2);
                }
            }
            Py_XDECREF(subparams);
        }
    }
    if (iparam < len) {
        if (_PyTuple_Resize(&parameters, iparam) < 0) {
            Py_XDECREF(parameters);
            Py_XDECREF(tuple_args);
            return NULL;
        }
    }
    Py_XDECREF(tuple_args);
    return parameters;
}

/* If obj is a generic alias, substitute type variables params
   with substitutions argitems.  For example, if obj is list[T],
   params is (T, S), and argitems is (str, int), return list[str].
   If obj doesn't have a __parameters__ attribute or that's not
   a non-empty tuple, return a new reference to obj. */
static PyObject *
subs_tvars(PyObject *obj, PyObject *params,
           PyObject **argitems, Py_ssize_t nargs)
{
    PyObject *subparams;
    if (PyObject_GetOptionalAttr(obj, &_Py_ID(__parameters__), &subparams) < 0) {
        return NULL;
    }
    if (subparams && PyTuple_Check(subparams) && PyTuple_GET_SIZE(subparams)) {
        Py_ssize_t nparams = PyTuple_GET_SIZE(params);
        Py_ssize_t nsubargs = PyTuple_GET_SIZE(subparams);
        PyObject *subargs = PyTuple_New(nsubargs);
        if (subargs == NULL) {
            Py_DECREF(subparams);
            return NULL;
        }
        Py_ssize_t j = 0;
        for (Py_ssize_t i = 0; i < nsubargs; ++i) {
            PyObject *arg = PyTuple_GET_ITEM(subparams, i);
            Py_ssize_t iparam = tuple_index(params, nparams, arg);
            if (iparam >= 0) {
                PyObject *param = PyTuple_GET_ITEM(params, iparam);
                arg = argitems[iparam];
                if (Py_TYPE(param)->tp_iter && PyTuple_Check(arg)) {  // TypeVarTuple
                    j = tuple_extend(&subargs, j,
                                    &PyTuple_GET_ITEM(arg, 0),
                                    PyTuple_GET_SIZE(arg));
                    if (j < 0) {
                        return NULL;
                    }
                    continue;
                }
            }
            PyTuple_SET_ITEM(subargs, j, Py_NewRef(arg));
            j++;
        }
        assert(j == PyTuple_GET_SIZE(subargs));

        obj = PyObject_GetItem(obj, subargs);

        Py_DECREF(subargs);
    }
    else {
        Py_INCREF(obj);
    }
    Py_XDECREF(subparams);
    return obj;
}

static int
_is_unpacked_typevartuple(PyObject *arg)
{
    PyObject *tmp;
    if (PyType_Check(arg)) { // TODO: Add test
        return 0;
    }
    int res = PyObject_GetOptionalAttr(arg, &_Py_ID(__typing_is_unpacked_typevartuple__), &tmp);
    if (res > 0) {
        res = PyObject_IsTrue(tmp);
        Py_DECREF(tmp);
    }
    return res;
}

static PyObject *
_unpacked_tuple_args(PyObject *arg)
{
    PyObject *result;
    assert(!PyType_Check(arg));
    // Fast path
    if (_PyGenericAlias_Check(arg) &&
            ((gaobject *)arg)->starred &&
            ((gaobject *)arg)->origin == (PyObject *)&PyTuple_Type)
    {
        result = ((gaobject *)arg)->args;
        return Py_NewRef(result);
    }

    if (PyObject_GetOptionalAttr(arg, &_Py_ID(__typing_unpacked_tuple_args__), &result) > 0) {
        if (result == Py_None) {
            Py_DECREF(result);
            return NULL;
        }
        return result;
    }
    return NULL;
}

static PyObject *
_unpack_args(PyObject *item)
{
    PyObject *newargs = PyList_New(0);
    if (newargs == NULL) {
        return NULL;
    }
    int is_tuple = PyTuple_Check(item);
    Py_ssize_t nitems = is_tuple ? PyTuple_GET_SIZE(item) : 1;
    PyObject **argitems = is_tuple ? &PyTuple_GET_ITEM(item, 0) : &item;
    for (Py_ssize_t i = 0; i < nitems; i++) {
        item = argitems[i];
        if (!PyType_Check(item)) {
            PyObject *subargs = _unpacked_tuple_args(item);
            if (subargs != NULL &&
                PyTuple_Check(subargs) &&
                !(PyTuple_GET_SIZE(subargs) &&
                  PyTuple_GET_ITEM(subargs, PyTuple_GET_SIZE(subargs)-1) == Py_Ellipsis))
            {
                if (PyList_SetSlice(newargs, PY_SSIZE_T_MAX, PY_SSIZE_T_MAX, subargs) < 0) {
                    Py_DECREF(subargs);
                    Py_DECREF(newargs);
                    return NULL;
                }
                Py_DECREF(subargs);
                continue;
            }
            Py_XDECREF(subargs);
            if (PyErr_Occurred()) {
                Py_DECREF(newargs);
                return NULL;
            }
        }
        if (PyList_Append(newargs, item) < 0) {
            Py_DECREF(newargs);
            return NULL;
        }
    }
    Py_SETREF(newargs, PySequence_Tuple(newargs));
    return newargs;
}

PyObject *
_Py_subs_parameters(PyObject *self, PyObject *args, PyObject *parameters, PyObject *item)
{
    Py_ssize_t nparams = PyTuple_GET_SIZE(parameters);
    if (nparams == 0) {
        return PyErr_Format(PyExc_TypeError,
                            "%R is not a generic class",
                            self);
    }
    item = _unpack_args(item);
    for (Py_ssize_t i = 0; i < nparams; i++) {
        PyObject *param = PyTuple_GET_ITEM(parameters, i);
        PyObject *prepare, *tmp;
        if (PyObject_GetOptionalAttr(param, &_Py_ID(__typing_prepare_subst__), &prepare) < 0) {
            Py_DECREF(item);
            return NULL;
        }
        if (prepare && prepare != Py_None) {
            if (PyTuple_Check(item)) {
                tmp = PyObject_CallFunction(prepare, "OO", self, item);
            }
            else {
                tmp = PyObject_CallFunction(prepare, "O(O)", self, item);
            }
            Py_DECREF(prepare);
            Py_SETREF(item, tmp);
            if (item == NULL) {
                return NULL;
            }
        }
    }
    int is_tuple = PyTuple_Check(item);
    Py_ssize_t nitems = is_tuple ? PyTuple_GET_SIZE(item) : 1;
    PyObject **argitems = is_tuple ? &PyTuple_GET_ITEM(item, 0) : &item;
    if (nitems != nparams) {
        Py_DECREF(item);
        return PyErr_Format(PyExc_TypeError,
                            "Too %s arguments for %R; actual %zd, expected %zd",
                            nitems > nparams ? "many" : "few",
                            self, nitems, nparams);
    }
    /* Replace all type variables (specified by parameters)
       with corresponding values specified by argitems.
        t = list[T];          t[int]      -> newargs = [int]
        t = dict[str, T];     t[int]      -> newargs = [str, int]
        t = dict[T, list[S]]; t[str, int] -> newargs = [str, list[int]]
        t = list[[T]];        t[str]      -> newargs = [[str]]
     */
    assert (PyTuple_Check(args) || PyList_Check(args));
    const bool is_args_list = PyList_Check(args);
    PyObject *tuple_args = NULL;
    if (is_args_list) {
        args = tuple_args = PySequence_Tuple(args);
        if (args == NULL) {
            return NULL;
        }
    }
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    PyObject *newargs = PyTuple_New(nargs);
    if (newargs == NULL) {
        Py_DECREF(item);
        Py_XDECREF(tuple_args);
        return NULL;
    }
    for (Py_ssize_t iarg = 0, jarg = 0; iarg < nargs; iarg++) {
        PyObject *arg = PyTuple_GET_ITEM(args, iarg);
        if (PyType_Check(arg)) {
            PyTuple_SET_ITEM(newargs, jarg, Py_NewRef(arg));
            jarg++;
            continue;
        }
        // Recursively substitute params in lists/tuples.
        if (PyTuple_Check(arg) || PyList_Check(arg)) {
            PyObject *subargs = _Py_subs_parameters(self, arg, parameters, item);
            if (subargs == NULL) {
                Py_DECREF(newargs);
                Py_DECREF(item);
                Py_XDECREF(tuple_args);
                return NULL;
            }
            if (PyTuple_Check(arg)) {
                PyTuple_SET_ITEM(newargs, jarg, subargs);
            }
            else {
                // _Py_subs_parameters returns a tuple. If the original arg was a list,
                // convert subargs to a list as well.
                PyObject *subargs_list = PySequence_List(subargs);
                Py_DECREF(subargs);
                if (subargs_list == NULL) {
                    Py_DECREF(newargs);
                    Py_DECREF(item);
                    Py_XDECREF(tuple_args);
                    return NULL;
                }
                PyTuple_SET_ITEM(newargs, jarg, subargs_list);
            }
            jarg++;
            continue;
        }
        int unpack = _is_unpacked_typevartuple(arg);
        if (unpack < 0) {
            Py_DECREF(newargs);
            Py_DECREF(item);
            Py_XDECREF(tuple_args);
            return NULL;
        }
        PyObject *subst;
        if (PyObject_GetOptionalAttr(arg, &_Py_ID(__typing_subst__), &subst) < 0) {
            Py_DECREF(newargs);
            Py_DECREF(item);
            Py_XDECREF(tuple_args);
            return NULL;
        }
        if (subst) {
            Py_ssize_t iparam = tuple_index(parameters, nparams, arg);
            assert(iparam >= 0);
            arg = PyObject_CallOneArg(subst, argitems[iparam]);
            Py_DECREF(subst);
        }
        else {
            arg = subs_tvars(arg, parameters, argitems, nitems);
        }
        if (arg == NULL) {
            Py_DECREF(newargs);
            Py_DECREF(item);
            Py_XDECREF(tuple_args);
            return NULL;
        }
        if (unpack) {
            jarg = tuple_extend(&newargs, jarg,
                    &PyTuple_GET_ITEM(arg, 0), PyTuple_GET_SIZE(arg));
            Py_DECREF(arg);
            if (jarg < 0) {
                Py_DECREF(item);
                Py_XDECREF(tuple_args);
                return NULL;
            }
        }
        else {
            PyTuple_SET_ITEM(newargs, jarg, arg);
            jarg++;
        }
    }

    Py_DECREF(item);
    Py_XDECREF(tuple_args);
    return newargs;
}

PyDoc_STRVAR(genericalias__doc__,
"GenericAlias(origin, args, /)\n"
"--\n\n"
"Represent a PEP 585 generic type\n"
"\n"
"E.g. for t = list[int], t.__origin__ is list and t.__args__ is (int,).");

static PyObject *
ga_getitem(PyObject *self, PyObject *item)
{
    gaobject *alias = (gaobject *)self;
    // Populate __parameters__ if needed.
    if (alias->parameters == NULL) {
        alias->parameters = _Py_make_parameters(alias->args);
        if (alias->parameters == NULL) {
            return NULL;
        }
    }

    PyObject *newargs = _Py_subs_parameters(self, alias->args, alias->parameters, item);
    if (newargs == NULL) {
        return NULL;
    }

    PyObject *res = Py_GenericAlias(alias->origin, newargs);
    if (res == NULL) {
        Py_DECREF(newargs);
        return NULL;
    }
    ((gaobject *)res)->starred = alias->starred;

    Py_DECREF(newargs);
    return res;
}

static PyMappingMethods ga_as_mapping = {
    .mp_subscript = ga_getitem,
};

static Py_hash_t
ga_hash(PyObject *self)
{
    gaobject *alias = (gaobject *)self;
    // TODO: Hash in the hash for the origin
    Py_hash_t h0 = PyObject_Hash(alias->origin);
    if (h0 == -1) {
        return -1;
    }
    Py_hash_t h1 = PyObject_Hash(alias->args);
    if (h1 == -1) {
        return -1;
    }
    return h0 ^ h1;
}

static inline PyObject *
set_orig_class(PyObject *obj, PyObject *self)
{
    if (obj != NULL) {
        if (PyObject_SetAttr(obj, &_Py_ID(__orig_class__), self) < 0) {
            if (!PyErr_ExceptionMatches(PyExc_AttributeError) &&
                !PyErr_ExceptionMatches(PyExc_TypeError))
            {
                Py_DECREF(obj);
                return NULL;
            }
            PyErr_Clear();
        }
    }
    return obj;
}

static PyObject *
ga_call(PyObject *self, PyObject *args, PyObject *kwds)
{
    gaobject *alias = (gaobject *)self;
    PyObject *obj = PyObject_Call(alias->origin, args, kwds);
    return set_orig_class(obj, self);
}

static PyObject *
ga_vectorcall(PyObject *self, PyObject *const *args,
              size_t nargsf, PyObject *kwnames)
{
    gaobject *alias = (gaobject *) self;
    PyObject *obj = PyVectorcall_Function(alias->origin)(alias->origin, args, nargsf, kwnames);
    return set_orig_class(obj, self);
}

static const char* const attr_exceptions[] = {
    "__class__",
    "__bases__",
    "__origin__",
    "__args__",
    "__unpacked__",
    "__parameters__",
    "__typing_unpacked_tuple_args__",
    "__mro_entries__",
    "__reduce_ex__",  // needed so we don't look up object.__reduce_ex__
    "__reduce__",
    "__copy__",
    "__deepcopy__",
    NULL,
};

static PyObject *
ga_getattro(PyObject *self, PyObject *name)
{
    gaobject *alias = (gaobject *)self;
    if (PyUnicode_Check(name)) {
        for (const char * const *p = attr_exceptions; ; p++) {
            if (*p == NULL) {
                return PyObject_GetAttr(alias->origin, name);
            }
            if (_PyUnicode_EqualToASCIIString(name, *p)) {
                break;
            }
        }
    }
    return PyObject_GenericGetAttr(self, name);
}

static PyObject *
ga_richcompare(PyObject *a, PyObject *b, int op)
{
    if (!_PyGenericAlias_Check(b) ||
        (op != Py_EQ && op != Py_NE))
    {
        Py_RETURN_NOTIMPLEMENTED;
    }

    if (op == Py_NE) {
        PyObject *eq = ga_richcompare(a, b, Py_EQ);
        if (eq == NULL)
            return NULL;
        Py_DECREF(eq);
        if (eq == Py_True) {
            Py_RETURN_FALSE;
        }
        else {
            Py_RETURN_TRUE;
        }
    }

    gaobject *aa = (gaobject *)a;
    gaobject *bb = (gaobject *)b;
    if (aa->starred != bb->starred) {
        Py_RETURN_FALSE;
    }
    int eq = PyObject_RichCompareBool(aa->origin, bb->origin, Py_EQ);
    if (eq < 0) {
        return NULL;
    }
    if (!eq) {
        Py_RETURN_FALSE;
    }
    return PyObject_RichCompare(aa->args, bb->args, Py_EQ);
}

static PyObject *
ga_mro_entries(PyObject *self, PyObject *args)
{
    gaobject *alias = (gaobject *)self;
    return PyTuple_Pack(1, alias->origin);
}

static PyObject *
ga_instancecheck(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyErr_SetString(PyExc_TypeError,
                    "isinstance() argument 2 cannot be a parameterized generic");
    return NULL;
}

static PyObject *
ga_subclasscheck(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyErr_SetString(PyExc_TypeError,
                    "issubclass() argument 2 cannot be a parameterized generic");
    return NULL;
}

static PyObject *
ga_reduce(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    gaobject *alias = (gaobject *)self;
    if (alias->starred) {
        PyObject *tmp = Py_GenericAlias(alias->origin, alias->args);
        if (tmp != NULL) {
            Py_SETREF(tmp, PyObject_GetIter(tmp));
        }
        if (tmp == NULL) {
            return NULL;
        }
        return Py_BuildValue("N(N)", _PyEval_GetBuiltin(&_Py_ID(next)), tmp);
    }
    return Py_BuildValue("O(OO)", Py_TYPE(alias),
                         alias->origin, alias->args);
}

static PyObject *
ga_dir(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    gaobject *alias = (gaobject *)self;
    PyObject *dir = PyObject_Dir(alias->origin);
    if (dir == NULL) {
        return NULL;
    }

    PyObject *dir_entry = NULL;
    for (const char * const *p = attr_exceptions; ; p++) {
        if (*p == NULL) {
            break;
        }
        else {
            dir_entry = PyUnicode_FromString(*p);
            if (dir_entry == NULL) {
                goto error;
            }
            int contains = PySequence_Contains(dir, dir_entry);
            if (contains < 0) {
                goto error;
            }
            if (contains == 0 && PyList_Append(dir, dir_entry) < 0) {
                goto error;
            }

            Py_CLEAR(dir_entry);
        }
    }
    return dir;

error:
    Py_DECREF(dir);
    Py_XDECREF(dir_entry);
    return NULL;
}

static PyMethodDef ga_methods[] = {
    {"__mro_entries__", ga_mro_entries, METH_O},
    {"__instancecheck__", ga_instancecheck, METH_O},
    {"__subclasscheck__", ga_subclasscheck, METH_O},
    {"__reduce__", ga_reduce, METH_NOARGS},
    {"__dir__", ga_dir, METH_NOARGS},
    {0}
};

static PyMemberDef ga_members[] = {
    {"__origin__", _Py_T_OBJECT, offsetof(gaobject, origin), Py_READONLY},
    {"__args__", _Py_T_OBJECT, offsetof(gaobject, args), Py_READONLY},
    {"__unpacked__", Py_T_BOOL, offsetof(gaobject, starred), Py_READONLY},
    {0}
};

static PyObject *
ga_parameters(PyObject *self, void *unused)
{
    gaobject *alias = (gaobject *)self;
    if (alias->parameters == NULL) {
        alias->parameters = _Py_make_parameters(alias->args);
        if (alias->parameters == NULL) {
            return NULL;
        }
    }
    return Py_NewRef(alias->parameters);
}

static PyObject *
ga_unpacked_tuple_args(PyObject *self, void *unused)
{
    gaobject *alias = (gaobject *)self;
    if (alias->starred && alias->origin == (PyObject *)&PyTuple_Type) {
        return Py_NewRef(alias->args);
    }
    Py_RETURN_NONE;
}

static PyGetSetDef ga_properties[] = {
    {"__parameters__", ga_parameters, NULL, PyDoc_STR("Type variables in the GenericAlias."), NULL},
    {"__typing_unpacked_tuple_args__", ga_unpacked_tuple_args, NULL, NULL},
    {0}
};

/* A helper function to create GenericAlias' args tuple and set its attributes.
 * Returns 1 on success, 0 on failure.
 */
static inline int
setup_ga(gaobject *alias, PyObject *origin, PyObject *args) {
    if (!PyTuple_Check(args)) {
        args = PyTuple_Pack(1, args);
        if (args == NULL) {
            return 0;
        }
    }
    else {
        Py_INCREF(args);
    }

    alias->origin = Py_NewRef(origin);
    alias->args = args;
    alias->parameters = NULL;
    alias->weakreflist = NULL;

    if (PyVectorcall_Function(origin) != NULL) {
        alias->vectorcall = ga_vectorcall;
    }
    else {
        alias->vectorcall = NULL;
    }

    return 1;
}

static PyObject *
ga_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    if (!_PyArg_NoKeywords("GenericAlias", kwds)) {
        return NULL;
    }
    if (!_PyArg_CheckPositional("GenericAlias", PyTuple_GET_SIZE(args), 2, 2)) {
        return NULL;
    }
    PyObject *origin = PyTuple_GET_ITEM(args, 0);
    PyObject *arguments = PyTuple_GET_ITEM(args, 1);
    gaobject *self = (gaobject *)type->tp_alloc(type, 0);
    if (self == NULL) {
        return NULL;
    }
    if (!setup_ga(self, origin, arguments)) {
        Py_DECREF(self);
        return NULL;
    }
    return (PyObject *)self;
}

static PyNumberMethods ga_as_number = {
        .nb_or = _Py_union_type_or, // Add __or__ function
};

static PyObject *
ga_iternext(PyObject *op)
{
    gaiterobject *gi = (gaiterobject*)op;
    if (gi->obj == NULL) {
        PyErr_SetNone(PyExc_StopIteration);
        return NULL;
    }
    gaobject *alias = (gaobject *)gi->obj;
    PyObject *starred_alias = Py_GenericAlias(alias->origin, alias->args);
    if (starred_alias == NULL) {
        return NULL;
    }
    ((gaobject *)starred_alias)->starred = true;
    Py_SETREF(gi->obj, NULL);
    return starred_alias;
}

static void
ga_iter_dealloc(PyObject *op)
{
    gaiterobject *gi = (gaiterobject*)op;
    PyObject_GC_UnTrack(gi);
    Py_XDECREF(gi->obj);
    PyObject_GC_Del(gi);
}

static int
ga_iter_traverse(PyObject *op, visitproc visit, void *arg)
{
    gaiterobject *gi = (gaiterobject*)op;
    Py_VISIT(gi->obj);
    return 0;
}

static int
ga_iter_clear(PyObject *self)
{
    gaiterobject *gi = (gaiterobject *)self;
    Py_CLEAR(gi->obj);
    return 0;
}

static PyObject *
ga_iter_reduce(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *iter = _PyEval_GetBuiltin(&_Py_ID(iter));
    gaiterobject *gi = (gaiterobject *)self;

    /* _PyEval_GetBuiltin can invoke arbitrary code,
     * call must be before access of iterator pointers.
     * see issue #101765 */

    if (gi->obj)
        return Py_BuildValue("N(O)", iter, gi->obj);
    else
        return Py_BuildValue("N(())", iter);
}

static PyMethodDef ga_iter_methods[] = {
    {"__reduce__", ga_iter_reduce, METH_NOARGS},
    {0}
};

// gh-91632: _Py_GenericAliasIterType is exported  to be cleared
// in _PyTypes_FiniTypes.
PyTypeObject _Py_GenericAliasIterType = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    .tp_name = "generic_alias_iterator",
    .tp_basicsize = sizeof(gaiterobject),
    .tp_iter = PyObject_SelfIter,
    .tp_iternext = ga_iternext,
    .tp_traverse = ga_iter_traverse,
    .tp_methods = ga_iter_methods,
    .tp_dealloc = ga_iter_dealloc,
    .tp_clear = ga_iter_clear,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
};

static PyObject *
ga_iter(PyObject *self) {
    gaiterobject *gi = PyObject_GC_New(gaiterobject, &_Py_GenericAliasIterType);
    if (gi == NULL) {
        return NULL;
    }
    gi->obj = Py_NewRef(self);
    PyObject_GC_Track(gi);
    return (PyObject *)gi;
}

// TODO:
// - argument clinic?
// - cache?
PyTypeObject Py_GenericAliasType = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    .tp_name = "types.GenericAlias",
    .tp_doc = genericalias__doc__,
    .tp_basicsize = sizeof(gaobject),
    .tp_dealloc = ga_dealloc,
    .tp_repr = ga_repr,
    .tp_as_number = &ga_as_number,  // allow X | Y of GenericAlias objs
    .tp_as_mapping = &ga_as_mapping,
    .tp_hash = ga_hash,
    .tp_call = ga_call,
    .tp_getattro = ga_getattro,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_VECTORCALL,
    .tp_traverse = ga_traverse,
    .tp_richcompare = ga_richcompare,
    .tp_weaklistoffset = offsetof(gaobject, weakreflist),
    .tp_methods = ga_methods,
    .tp_members = ga_members,
    .tp_alloc = PyType_GenericAlloc,
    .tp_new = ga_new,
    .tp_free = PyObject_GC_Del,
    .tp_getset = ga_properties,
    .tp_iter = ga_iter,
    .tp_vectorcall_offset = offsetof(gaobject, vectorcall),
};

PyObject *
Py_GenericAlias(PyObject *origin, PyObject *args)
{
    gaobject *alias = (gaobject*) PyType_GenericAlloc(
            (PyTypeObject *)&Py_GenericAliasType, 0);
    if (alias == NULL) {
        return NULL;
    }
    if (!setup_ga(alias, origin, args)) {
        Py_DECREF(alias);
        return NULL;
    }
    return (PyObject *)alias;
}
