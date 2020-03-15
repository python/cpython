/* Built-in functions */

#include "Python.h"
#include <ctype.h>
#include "pycore_ast.h"           // _PyAST_Validate()
#include "pycore_compile.h"       // _PyAST_Compile()
#include "pycore_object.h"        // _Py_AddToAllObjects()
#include "pycore_pyerrors.h"      // _PyErr_NoMemory()
#include "pycore_pystate.h"       // _PyThreadState_GET()
#include "pycore_tuple.h"         // _PyTuple_FromArray()
#include "pycore_ceval.h"         // _PyEval_Vector()

_Py_IDENTIFIER(__builtins__);
_Py_IDENTIFIER(__dict__);
_Py_IDENTIFIER(__prepare__);
_Py_IDENTIFIER(__round__);
_Py_IDENTIFIER(__mro_entries__);
_Py_IDENTIFIER(encoding);
_Py_IDENTIFIER(errors);
_Py_IDENTIFIER(fileno);
_Py_IDENTIFIER(flush);
_Py_IDENTIFIER(metaclass);
_Py_IDENTIFIER(sort);
_Py_IDENTIFIER(stdin);
_Py_IDENTIFIER(stdout);
_Py_IDENTIFIER(stderr);

#include "clinic/bltinmodule.c.h"

static PyObject*
update_bases(PyObject *bases, PyObject *const *args, Py_ssize_t nargs)
{
    Py_ssize_t i, j;
    PyObject *base, *meth, *new_base, *result, *new_bases = NULL;
    assert(PyTuple_Check(bases));

    for (i = 0; i < nargs; i++) {
        base  = args[i];
        if (PyType_Check(base)) {
            if (new_bases) {
                /* If we already have made a replacement, then we append every normal base,
                   otherwise just skip it. */
                if (PyList_Append(new_bases, base) < 0) {
                    goto error;
                }
            }
            continue;
        }
        if (_PyObject_LookupAttrId(base, &PyId___mro_entries__, &meth) < 0) {
            goto error;
        }
        if (!meth) {
            if (new_bases) {
                if (PyList_Append(new_bases, base) < 0) {
                    goto error;
                }
            }
            continue;
        }
        new_base = PyObject_CallOneArg(meth, bases);
        Py_DECREF(meth);
        if (!new_base) {
            goto error;
        }
        if (!PyTuple_Check(new_base)) {
            PyErr_SetString(PyExc_TypeError,
                            "__mro_entries__ must return a tuple");
            Py_DECREF(new_base);
            goto error;
        }
        if (!new_bases) {
            /* If this is a first successful replacement, create new_bases list and
               copy previously encountered bases. */
            if (!(new_bases = PyList_New(i))) {
                goto error;
            }
            for (j = 0; j < i; j++) {
                base = args[j];
                PyList_SET_ITEM(new_bases, j, base);
                Py_INCREF(base);
            }
        }
        j = PyList_GET_SIZE(new_bases);
        if (PyList_SetSlice(new_bases, j, j, new_base) < 0) {
            goto error;
        }
        Py_DECREF(new_base);
    }
    if (!new_bases) {
        return bases;
    }
    result = PyList_AsTuple(new_bases);
    Py_DECREF(new_bases);
    return result;

error:
    Py_XDECREF(new_bases);
    return NULL;
}

/* AC: cannot convert yet, waiting for *args support */
static PyObject *
builtin___build_class__(PyObject *self, PyObject *const *args, Py_ssize_t nargs,
                        PyObject *kwnames)
{
    PyObject *func, *name, *bases, *mkw, *meta, *winner, *prep, *ns, *orig_bases;
    PyObject *cls = NULL, *cell = NULL;
    int isclass = 0;   /* initialize to prevent gcc warning */

    if (nargs < 2) {
        PyErr_SetString(PyExc_TypeError,
                        "__build_class__: not enough arguments");
        return NULL;
    }
    func = args[0];   /* Better be callable */
    if (!PyFunction_Check(func)) {
        PyErr_SetString(PyExc_TypeError,
                        "__build_class__: func must be a function");
        return NULL;
    }
    name = args[1];
    if (!PyUnicode_Check(name)) {
        PyErr_SetString(PyExc_TypeError,
                        "__build_class__: name is not a string");
        return NULL;
    }
    orig_bases = _PyTuple_FromArray(args + 2, nargs - 2);
    if (orig_bases == NULL)
        return NULL;

    bases = update_bases(orig_bases, args + 2, nargs - 2);
    if (bases == NULL) {
        Py_DECREF(orig_bases);
        return NULL;
    }

    if (kwnames == NULL) {
        meta = NULL;
        mkw = NULL;
    }
    else {
        mkw = _PyStack_AsDict(args + nargs, kwnames);
        if (mkw == NULL) {
            Py_DECREF(bases);
            return NULL;
        }

        meta = _PyDict_GetItemIdWithError(mkw, &PyId_metaclass);
        if (meta != NULL) {
            Py_INCREF(meta);
            if (_PyDict_DelItemId(mkw, &PyId_metaclass) < 0) {
                Py_DECREF(meta);
                Py_DECREF(mkw);
                Py_DECREF(bases);
                return NULL;
            }
            /* metaclass is explicitly given, check if it's indeed a class */
            isclass = PyType_Check(meta);
        }
        else if (PyErr_Occurred()) {
            Py_DECREF(mkw);
            Py_DECREF(bases);
            return NULL;
        }
    }
    if (meta == NULL) {
        /* if there are no bases, use type: */
        if (PyTuple_GET_SIZE(bases) == 0) {
            meta = (PyObject *) (&PyType_Type);
        }
        /* else get the type of the first base */
        else {
            PyObject *base0 = PyTuple_GET_ITEM(bases, 0);
            meta = (PyObject *)Py_TYPE(base0);
        }
        Py_INCREF(meta);
        isclass = 1;  /* meta is really a class */
    }

    if (isclass) {
        /* meta is really a class, so check for a more derived
           metaclass, or possible metaclass conflicts: */
        winner = (PyObject *)_PyType_CalculateMetaclass((PyTypeObject *)meta,
                                                        bases);
        if (winner == NULL) {
            Py_DECREF(meta);
            Py_XDECREF(mkw);
            Py_DECREF(bases);
            return NULL;
        }
        if (winner != meta) {
            Py_DECREF(meta);
            meta = winner;
            Py_INCREF(meta);
        }
    }
    /* else: meta is not a class, so we cannot do the metaclass
       calculation, so we will use the explicitly given object as it is */
    if (_PyObject_LookupAttrId(meta, &PyId___prepare__, &prep) < 0) {
        ns = NULL;
    }
    else if (prep == NULL) {
        ns = PyDict_New();
    }
    else {
        PyObject *pargs[2] = {name, bases};
        ns = PyObject_VectorcallDict(prep, pargs, 2, mkw);
        Py_DECREF(prep);
    }
    if (ns == NULL) {
        Py_DECREF(meta);
        Py_XDECREF(mkw);
        Py_DECREF(bases);
        return NULL;
    }
    if (!PyMapping_Check(ns)) {
        PyErr_Format(PyExc_TypeError,
                     "%.200s.__prepare__() must return a mapping, not %.200s",
                     isclass ? ((PyTypeObject *)meta)->tp_name : "<metaclass>",
                     Py_TYPE(ns)->tp_name);
        goto error;
    }
    PyFrameConstructor *f =  PyFunction_AS_FRAME_CONSTRUCTOR(func);
    PyThreadState *tstate = PyThreadState_GET();
    cell = _PyEval_Vector(tstate, f, ns, NULL, 0, NULL);
    if (cell != NULL) {
        if (bases != orig_bases) {
            if (PyMapping_SetItemString(ns, "__orig_bases__", orig_bases) < 0) {
                goto error;
            }
        }
        PyObject *margs[3] = {name, bases, ns};
        cls = PyObject_VectorcallDict(meta, margs, 3, mkw);
        if (cls != NULL && PyType_Check(cls) && PyCell_Check(cell)) {
            PyObject *cell_cls = PyCell_GET(cell);
            if (cell_cls != cls) {
                if (cell_cls == NULL) {
                    const char *msg =
                        "__class__ not set defining %.200R as %.200R. "
                        "Was __classcell__ propagated to type.__new__?";
                    PyErr_Format(PyExc_RuntimeError, msg, name, cls);
                } else {
                    const char *msg =
                        "__class__ set to %.200R defining %.200R as %.200R";
                    PyErr_Format(PyExc_TypeError, msg, cell_cls, name, cls);
                }
                Py_DECREF(cls);
                cls = NULL;
                goto error;
            }
        }
    }
error:
    Py_XDECREF(cell);
    Py_DECREF(ns);
    Py_DECREF(meta);
    Py_XDECREF(mkw);
    Py_DECREF(bases);
    if (bases != orig_bases) {
        Py_DECREF(orig_bases);
    }
    return cls;
}

PyDoc_STRVAR(build_class_doc,
"__build_class__(func, name, /, *bases, [metaclass], **kwds) -> class\n\
\n\
Internal helper function used by the class statement.");

static PyObject *
builtin___import__(PyObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"name", "globals", "locals", "fromlist",
                             "level", 0};
    PyObject *name, *globals = NULL, *locals = NULL, *fromlist = NULL;
    int level = 0;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "U|OOOi:__import__",
                    kwlist, &name, &globals, &locals, &fromlist, &level))
        return NULL;
    return PyImport_ImportModuleLevelObject(name, globals, locals,
                                            fromlist, level);
}

PyDoc_STRVAR(import_doc,
"__import__(name, globals=None, locals=None, fromlist=(), level=0) -> module\n\
\n\
Import a module. Because this function is meant for use by the Python\n\
interpreter and not for general use, it is better to use\n\
importlib.import_module() to programmatically import a module.\n\
\n\
The globals argument is only used to determine the context;\n\
they are not modified.  The locals argument is unused.  The fromlist\n\
should be a list of names to emulate ``from name import ...'', or an\n\
empty list to emulate ``import name''.\n\
When importing a module from a package, note that __import__('A.B', ...)\n\
returns package A when fromlist is empty, but its submodule B when\n\
fromlist is not empty.  The level argument is used to determine whether to\n\
perform absolute or relative imports: 0 is absolute, while a positive number\n\
is the number of parent directories to search relative to the current module.");


/*[clinic input]
abs as builtin_abs

    x: object
    /

Return the absolute value of the argument.
[clinic start generated code]*/

static PyObject *
builtin_abs(PyObject *module, PyObject *x)
/*[clinic end generated code: output=b1b433b9e51356f5 input=bed4ca14e29c20d1]*/
{
    return PyNumber_Absolute(x);
}

/*[clinic input]
all as builtin_all

    iterable: object
    /

Return True if bool(x) is True for all values x in the iterable.

If the iterable is empty, return True.
[clinic start generated code]*/

static PyObject *
builtin_all(PyObject *module, PyObject *iterable)
/*[clinic end generated code: output=ca2a7127276f79b3 input=1a7c5d1bc3438a21]*/
{
    PyObject *it, *item;
    PyObject *(*iternext)(PyObject *);
    int cmp;

    it = PyObject_GetIter(iterable);
    if (it == NULL)
        return NULL;
    iternext = *Py_TYPE(it)->tp_iternext;

    for (;;) {
        item = iternext(it);
        if (item == NULL)
            break;
        cmp = PyObject_IsTrue(item);
        Py_DECREF(item);
        if (cmp < 0) {
            Py_DECREF(it);
            return NULL;
        }
        if (cmp == 0) {
            Py_DECREF(it);
            Py_RETURN_FALSE;
        }
    }
    Py_DECREF(it);
    if (PyErr_Occurred()) {
        if (PyErr_ExceptionMatches(PyExc_StopIteration))
            PyErr_Clear();
        else
            return NULL;
    }
    Py_RETURN_TRUE;
}

/*[clinic input]
any as builtin_any

    iterable: object
    /

Return True if bool(x) is True for any x in the iterable.

If the iterable is empty, return False.
[clinic start generated code]*/

static PyObject *
builtin_any(PyObject *module, PyObject *iterable)
/*[clinic end generated code: output=fa65684748caa60e input=41d7451c23384f24]*/
{
    PyObject *it, *item;
    PyObject *(*iternext)(PyObject *);
    int cmp;

    it = PyObject_GetIter(iterable);
    if (it == NULL)
        return NULL;
    iternext = *Py_TYPE(it)->tp_iternext;

    for (;;) {
        item = iternext(it);
        if (item == NULL)
            break;
        cmp = PyObject_IsTrue(item);
        Py_DECREF(item);
        if (cmp < 0) {
            Py_DECREF(it);
            return NULL;
        }
        if (cmp > 0) {
            Py_DECREF(it);
            Py_RETURN_TRUE;
        }
    }
    Py_DECREF(it);
    if (PyErr_Occurred()) {
        if (PyErr_ExceptionMatches(PyExc_StopIteration))
            PyErr_Clear();
        else
            return NULL;
    }
    Py_RETURN_FALSE;
}

/*[clinic input]
ascii as builtin_ascii

    obj: object
    /

Return an ASCII-only representation of an object.

As repr(), return a string containing a printable representation of an
object, but escape the non-ASCII characters in the string returned by
repr() using \\x, \\u or \\U escapes. This generates a string similar
to that returned by repr() in Python 2.
[clinic start generated code]*/

static PyObject *
builtin_ascii(PyObject *module, PyObject *obj)
/*[clinic end generated code: output=6d37b3f0984c7eb9 input=4c62732e1b3a3cc9]*/
{
    return PyObject_ASCII(obj);
}


/*[clinic input]
bin as builtin_bin

    number: object
    /

Return the binary representation of an integer.

   >>> bin(2796202)
   '0b1010101010101010101010'
[clinic start generated code]*/

static PyObject *
builtin_bin(PyObject *module, PyObject *number)
/*[clinic end generated code: output=b6fc4ad5e649f4f7 input=53f8a0264bacaf90]*/
{
    return PyNumber_ToBase(number, 2);
}


/*[clinic input]
callable as builtin_callable

    obj: object
    /

Return whether the object is callable (i.e., some kind of function).

Note that classes are callable, as are instances of classes with a
__call__() method.
[clinic start generated code]*/

static PyObject *
builtin_callable(PyObject *module, PyObject *obj)
/*[clinic end generated code: output=2b095d59d934cb7e input=1423bab99cc41f58]*/
{
    return PyBool_FromLong((long)PyCallable_Check(obj));
}

static PyObject *
builtin_breakpoint(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *keywords)
{
    PyObject *hook = PySys_GetObject("breakpointhook");

    if (hook == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "lost sys.breakpointhook");
        return NULL;
    }

    if (PySys_Audit("builtins.breakpoint", "O", hook) < 0) {
        return NULL;
    }

    Py_INCREF(hook);
    PyObject *retval = PyObject_Vectorcall(hook, args, nargs, keywords);
    Py_DECREF(hook);
    return retval;
}

PyDoc_STRVAR(breakpoint_doc,
"breakpoint(*args, **kws)\n\
\n\
Call sys.breakpointhook(*args, **kws).  sys.breakpointhook() must accept\n\
whatever arguments are passed.\n\
\n\
By default, this drops you into the pdb debugger.");

typedef struct {
    PyObject_HEAD
    PyObject *func;
    PyObject *it;
} filterobject;

static PyObject *
filter_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyObject *func, *seq;
    PyObject *it;
    filterobject *lz;

    if (type == &PyFilter_Type && !_PyArg_NoKeywords("filter", kwds))
        return NULL;

    if (!PyArg_UnpackTuple(args, "filter", 2, 2, &func, &seq))
        return NULL;

    /* Get iterator. */
    it = PyObject_GetIter(seq);
    if (it == NULL)
        return NULL;

    /* create filterobject structure */
    lz = (filterobject *)type->tp_alloc(type, 0);
    if (lz == NULL) {
        Py_DECREF(it);
        return NULL;
    }

    lz->func = Py_NewRef(func);
    lz->it = it;

    return (PyObject *)lz;
}

static PyObject *
filter_vectorcall(PyObject *type, PyObject * const*args,
                size_t nargsf, PyObject *kwnames)
{
    PyTypeObject *tp = (PyTypeObject *)type;
    if (tp == &PyFilter_Type && !_PyArg_NoKwnames("filter", kwnames)) {
        return NULL;
    }

    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    if (!_PyArg_CheckPositional("filter", nargs, 2, 2)) {
        return NULL;
    }

    PyObject *it = PyObject_GetIter(args[1]);
    if (it == NULL) {
        return NULL;
    }

    filterobject *lz = (filterobject *)tp->tp_alloc(tp, 0);

    if (lz == NULL) {
        Py_DECREF(it);
        return NULL;
    }

    lz->func = Py_NewRef(args[0]);
    lz->it = it;

    return (PyObject *)lz;
}

static void
filter_dealloc(filterobject *lz)
{
    PyObject_GC_UnTrack(lz);
    Py_XDECREF(lz->func);
    Py_XDECREF(lz->it);
    Py_TYPE(lz)->tp_free(lz);
}

static int
filter_traverse(filterobject *lz, visitproc visit, void *arg)
{
    Py_VISIT(lz->it);
    Py_VISIT(lz->func);
    return 0;
}

static PyObject *
filter_next(filterobject *lz)
{
    PyObject *item;
    PyObject *it = lz->it;
    long ok;
    PyObject *(*iternext)(PyObject *);
    int checktrue = lz->func == Py_None || lz->func == (PyObject *)&PyBool_Type;

    iternext = *Py_TYPE(it)->tp_iternext;
    for (;;) {
        item = iternext(it);
        if (item == NULL)
            return NULL;

        if (checktrue) {
            ok = PyObject_IsTrue(item);
        } else {
            PyObject *good;
            good = PyObject_CallOneArg(lz->func, item);
            if (good == NULL) {
                Py_DECREF(item);
                return NULL;
            }
            ok = PyObject_IsTrue(good);
            Py_DECREF(good);
        }
        if (ok > 0)
            return item;
        Py_DECREF(item);
        if (ok < 0)
            return NULL;
    }
}

static PyObject *
filter_reduce(filterobject *lz, PyObject *Py_UNUSED(ignored))
{
    return Py_BuildValue("O(OO)", Py_TYPE(lz), lz->func, lz->it);
}

PyDoc_STRVAR(reduce_doc, "Return state information for pickling.");

static PyMethodDef filter_methods[] = {
    {"__reduce__",   (PyCFunction)filter_reduce,   METH_NOARGS, reduce_doc},
    {NULL,           NULL}           /* sentinel */
};

PyDoc_STRVAR(filter_doc,
"filter(function or None, iterable) --> filter object\n\
\n\
Return an iterator yielding those items of iterable for which function(item)\n\
is true. If function is None, return the items that are true.");

PyTypeObject PyFilter_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "filter",                           /* tp_name */
    sizeof(filterobject),               /* tp_basicsize */
    0,                                  /* tp_itemsize */
    /* methods */
    (destructor)filter_dealloc,         /* tp_dealloc */
    0,                                  /* tp_vectorcall_offset */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_as_async */
    0,                                  /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    0,                                  /* tp_hash */
    0,                                  /* tp_call */
    0,                                  /* tp_str */
    PyObject_GenericGetAttr,            /* tp_getattro */
    0,                                  /* tp_setattro */
    0,                                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
        Py_TPFLAGS_BASETYPE,            /* tp_flags */
    filter_doc,                         /* tp_doc */
    (traverseproc)filter_traverse,      /* tp_traverse */
    0,                                  /* tp_clear */
    0,                                  /* tp_richcompare */
    0,                                  /* tp_weaklistoffset */
    PyObject_SelfIter,                  /* tp_iter */
    (iternextfunc)filter_next,          /* tp_iternext */
    filter_methods,                     /* tp_methods */
    0,                                  /* tp_members */
    0,                                  /* tp_getset */
    0,                                  /* tp_base */
    0,                                  /* tp_dict */
    0,                                  /* tp_descr_get */
    0,                                  /* tp_descr_set */
    0,                                  /* tp_dictoffset */
    0,                                  /* tp_init */
    PyType_GenericAlloc,                /* tp_alloc */
    filter_new,                         /* tp_new */
    PyObject_GC_Del,                    /* tp_free */
    .tp_vectorcall = (vectorcallfunc)filter_vectorcall
};


/*[clinic input]
format as builtin_format

    value: object
    format_spec: unicode(c_default="NULL") = ''
    /

Return value.__format__(format_spec)

format_spec defaults to the empty string.
See the Format Specification Mini-Language section of help('FORMATTING') for
details.
[clinic start generated code]*/

static PyObject *
builtin_format_impl(PyObject *module, PyObject *value, PyObject *format_spec)
/*[clinic end generated code: output=2f40bdfa4954b077 input=88339c93ea522b33]*/
{
    return PyObject_Format(value, format_spec);
}

/*[clinic input]
chr as builtin_chr

    i: int
    /

Return a Unicode string of one character with ordinal i; 0 <= i <= 0x10ffff.
[clinic start generated code]*/

static PyObject *
builtin_chr_impl(PyObject *module, int i)
/*[clinic end generated code: output=c733afcd200afcb7 input=3f604ef45a70750d]*/
{
    return PyUnicode_FromOrdinal(i);
}


/*[clinic input]
compile as builtin_compile

    source: object
    filename: object(converter="PyUnicode_FSDecoder")
    mode: str
    flags: int = 0
    dont_inherit: bool(accept={int}) = False
    optimize: int = -1
    *
    _feature_version as feature_version: int = -1

Compile source into a code object that can be executed by exec() or eval().

The source code may represent a Python module, statement or expression.
The filename will be used for run-time error messages.
The mode must be 'exec' to compile a module, 'single' to compile a
single (interactive) statement, or 'eval' to compile an expression.
The flags argument, if present, controls which future statements influence
the compilation of the code.
The dont_inherit argument, if true, stops the compilation inheriting
the effects of any future statements in effect in the code calling
compile; if absent or false these statements do influence the compilation,
in addition to any features explicitly specified.
[clinic start generated code]*/

static PyObject *
builtin_compile_impl(PyObject *module, PyObject *source, PyObject *filename,
                     const char *mode, int flags, int dont_inherit,
                     int optimize, int feature_version)
/*[clinic end generated code: output=b0c09c84f116d3d7 input=40171fb92c1d580d]*/
{
    PyObject *source_copy;
    const char *str;
    int compile_mode = -1;
    int is_ast;
    int start[] = {Py_file_input, Py_eval_input, Py_single_input, Py_func_type_input};
    PyObject *result;

    PyCompilerFlags cf = _PyCompilerFlags_INIT;
    cf.cf_flags = flags | PyCF_SOURCE_IS_UTF8;
    if (feature_version >= 0 && (flags & PyCF_ONLY_AST)) {
        cf.cf_feature_version = feature_version;
    }

    if (flags &
        ~(PyCF_MASK | PyCF_MASK_OBSOLETE | PyCF_COMPILE_MASK))
    {
        PyErr_SetString(PyExc_ValueError,
                        "compile(): unrecognised flags");
        goto error;
    }
    /* XXX Warn if (supplied_flags & PyCF_MASK_OBSOLETE) != 0? */

    if (optimize < -1 || optimize > 2) {
        PyErr_SetString(PyExc_ValueError,
                        "compile(): invalid optimize value");
        goto error;
    }

    if (!dont_inherit) {
        PyEval_MergeCompilerFlags(&cf);
    }

    if (strcmp(mode, "exec") == 0)
        compile_mode = 0;
    else if (strcmp(mode, "eval") == 0)
        compile_mode = 1;
    else if (strcmp(mode, "single") == 0)
        compile_mode = 2;
    else if (strcmp(mode, "func_type") == 0) {
        if (!(flags & PyCF_ONLY_AST)) {
            PyErr_SetString(PyExc_ValueError,
                            "compile() mode 'func_type' requires flag PyCF_ONLY_AST");
            goto error;
        }
        compile_mode = 3;
    }
    else {
        const char *msg;
        if (flags & PyCF_ONLY_AST)
            msg = "compile() mode must be 'exec', 'eval', 'single' or 'func_type'";
        else
            msg = "compile() mode must be 'exec', 'eval' or 'single'";
        PyErr_SetString(PyExc_ValueError, msg);
        goto error;
    }

    is_ast = PyAST_Check(source);
    if (is_ast == -1)
        goto error;
    if (is_ast) {
        if (flags & PyCF_ONLY_AST) {
            Py_INCREF(source);
            result = source;
        }
        else {
            PyArena *arena;
            mod_ty mod;

            arena = _PyArena_New();
            if (arena == NULL)
                goto error;
            mod = PyAST_obj2mod(source, arena, compile_mode);
            if (mod == NULL || !_PyAST_Validate(mod)) {
                _PyArena_Free(arena);
                goto error;
            }
            result = (PyObject*)_PyAST_Compile(mod, filename,
                                               &cf, optimize, arena);
            _PyArena_Free(arena);
        }
        goto finally;
    }

    str = _Py_SourceAsString(source, "compile", "string, bytes or AST", &cf, &source_copy);
    if (str == NULL)
        goto error;

    result = Py_CompileStringObject(str, filename, start[compile_mode], &cf, optimize);

    Py_XDECREF(source_copy);
    goto finally;

error:
    result = NULL;
finally:
    Py_DECREF(filename);
    return result;
}

/* AC: cannot convert yet, as needs PEP 457 group support in inspect */
static PyObject *
builtin_dir(PyObject *self, PyObject *args)
{
    PyObject *arg = NULL;

    if (!PyArg_UnpackTuple(args, "dir", 0, 1, &arg))
        return NULL;
    return PyObject_Dir(arg);
}

PyDoc_STRVAR(dir_doc,
"dir([object]) -> list of strings\n"
"\n"
"If called without an argument, return the names in the current scope.\n"
"Else, return an alphabetized list of names comprising (some of) the attributes\n"
"of the given object, and of attributes reachable from it.\n"
"If the object supplies a method named __dir__, it will be used; otherwise\n"
"the default dir() logic is used and returns:\n"
"  for a module object: the module's attributes.\n"
"  for a class object:  its attributes, and recursively the attributes\n"
"    of its bases.\n"
"  for any other object: its attributes, its class's attributes, and\n"
"    recursively the attributes of its class's base classes.");

/*[clinic input]
divmod as builtin_divmod

    x: object
    y: object
    /

Return the tuple (x//y, x%y).  Invariant: div*y + mod == x.
[clinic start generated code]*/

static PyObject *
builtin_divmod_impl(PyObject *module, PyObject *x, PyObject *y)
/*[clinic end generated code: output=b06d8a5f6e0c745e input=175ad9c84ff41a85]*/
{
    return PyNumber_Divmod(x, y);
}


/*[clinic input]
eval as builtin_eval

    source: object
    globals: object = None
    locals: object = None
    /

Evaluate the given source in the context of globals and locals.

The source may be a string representing a Python expression
or a code object as returned by compile().
The globals must be a dictionary and locals can be any mapping,
defaulting to the current globals and locals.
If only globals is given, locals defaults to it.
[clinic start generated code]*/

static PyObject *
builtin_eval_impl(PyObject *module, PyObject *source, PyObject *globals,
                  PyObject *locals)
/*[clinic end generated code: output=0a0824aa70093116 input=11ee718a8640e527]*/
{
    PyObject *result, *source_copy;
    const char *str;

    if (locals != Py_None && !PyMapping_Check(locals)) {
        PyErr_SetString(PyExc_TypeError, "locals must be a mapping");
        return NULL;
    }
    if (globals != Py_None && !PyDict_Check(globals)) {
        PyErr_SetString(PyExc_TypeError, PyMapping_Check(globals) ?
            "globals must be a real dict; try eval(expr, {}, mapping)"
            : "globals must be a dict");
        return NULL;
    }
    if (globals == Py_None) {
        globals = PyEval_GetGlobals();
        if (locals == Py_None) {
            locals = PyEval_GetLocals();
            if (locals == NULL)
                return NULL;
        }
    }
    else if (locals == Py_None)
        locals = globals;

    if (globals == NULL || locals == NULL) {
        PyErr_SetString(PyExc_TypeError,
            "eval must be given globals and locals "
            "when called without a frame");
        return NULL;
    }

    int r = _PyDict_ContainsId(globals, &PyId___builtins__);
    if (r == 0) {
        r = _PyDict_SetItemId(globals, &PyId___builtins__,
                              PyEval_GetBuiltins());
    }
    if (r < 0) {
        return NULL;
    }

    if (PyCode_Check(source)) {
        if (PySys_Audit("exec", "O", source) < 0) {
            return NULL;
        }

        if (PyCode_GetNumFree((PyCodeObject *)source) > 0) {
            PyErr_SetString(PyExc_TypeError,
                "code object passed to eval() may not contain free variables");
            return NULL;
        }
        return PyEval_EvalCode(source, globals, locals);
    }

    PyCompilerFlags cf = _PyCompilerFlags_INIT;
    cf.cf_flags = PyCF_SOURCE_IS_UTF8;
    str = _Py_SourceAsString(source, "eval", "string, bytes or code", &cf, &source_copy);
    if (str == NULL)
        return NULL;

    while (*str == ' ' || *str == '\t')
        str++;

    (void)PyEval_MergeCompilerFlags(&cf);
    result = PyRun_StringFlags(str, Py_eval_input, globals, locals, &cf);
    Py_XDECREF(source_copy);
    return result;
}

/*[clinic input]
exec as builtin_exec

    source: object
    globals: object = None
    locals: object = None
    /

Execute the given source in the context of globals and locals.

The source may be a string representing one or more Python statements
or a code object as returned by compile().
The globals must be a dictionary and locals can be any mapping,
defaulting to the current globals and locals.
If only globals is given, locals defaults to it.
[clinic start generated code]*/

static PyObject *
builtin_exec_impl(PyObject *module, PyObject *source, PyObject *globals,
                  PyObject *locals)
/*[clinic end generated code: output=3c90efc6ab68ef5d input=01ca3e1c01692829]*/
{
    PyObject *v;

    if (globals == Py_None) {
        globals = PyEval_GetGlobals();
        if (locals == Py_None) {
            locals = PyEval_GetLocals();
            if (locals == NULL)
                return NULL;
        }
        if (!globals || !locals) {
            PyErr_SetString(PyExc_SystemError,
                            "globals and locals cannot be NULL");
            return NULL;
        }
    }
    else if (locals == Py_None)
        locals = globals;

    if (!PyDict_Check(globals)) {
        PyErr_Format(PyExc_TypeError, "exec() globals must be a dict, not %.100s",
                     Py_TYPE(globals)->tp_name);
        return NULL;
    }
    if (!PyMapping_Check(locals)) {
        PyErr_Format(PyExc_TypeError,
            "locals must be a mapping or None, not %.100s",
            Py_TYPE(locals)->tp_name);
        return NULL;
    }
    int r = _PyDict_ContainsId(globals, &PyId___builtins__);
    if (r == 0) {
        r = _PyDict_SetItemId(globals, &PyId___builtins__,
                              PyEval_GetBuiltins());
    }
    if (r < 0) {
        return NULL;
    }

    if (PyCode_Check(source)) {
        if (PySys_Audit("exec", "O", source) < 0) {
            return NULL;
        }

        if (PyCode_GetNumFree((PyCodeObject *)source) > 0) {
            PyErr_SetString(PyExc_TypeError,
                "code object passed to exec() may not "
                "contain free variables");
            return NULL;
        }
        v = PyEval_EvalCode(source, globals, locals);
    }
    else {
        PyObject *source_copy;
        const char *str;
        PyCompilerFlags cf = _PyCompilerFlags_INIT;
        cf.cf_flags = PyCF_SOURCE_IS_UTF8;
        str = _Py_SourceAsString(source, "exec",
                                       "string, bytes or code", &cf,
                                       &source_copy);
        if (str == NULL)
            return NULL;
        if (PyEval_MergeCompilerFlags(&cf))
            v = PyRun_StringFlags(str, Py_file_input, globals,
                                  locals, &cf);
        else
            v = PyRun_String(str, Py_file_input, globals, locals);
        Py_XDECREF(source_copy);
    }
    if (v == NULL)
        return NULL;
    Py_DECREF(v);
    Py_RETURN_NONE;
}


/* AC: cannot convert yet, as needs PEP 457 group support in inspect */
static PyObject *
builtin_getattr(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *v, *name, *result;

    if (!_PyArg_CheckPositional("getattr", nargs, 2, 3))
        return NULL;

    v = args[0];
    name = args[1];
    if (!PyUnicode_Check(name)) {
        PyErr_SetString(PyExc_TypeError,
                        "getattr(): attribute name must be string");
        return NULL;
    }
    if (nargs > 2) {
        if (_PyObject_LookupAttr(v, name, &result) == 0) {
            PyObject *dflt = args[2];
            Py_INCREF(dflt);
            return dflt;
        }
    }
    else {
        result = PyObject_GetAttr(v, name);
    }
    return result;
}

PyDoc_STRVAR(getattr_doc,
"getattr(object, name[, default]) -> value\n\
\n\
Get a named attribute from an object; getattr(x, 'y') is equivalent to x.y.\n\
When a default argument is given, it is returned when the attribute doesn't\n\
exist; without it, an exception is raised in that case.");


/*[clinic input]
globals as builtin_globals

Return the dictionary containing the current scope's global variables.

NOTE: Updates to this dictionary *will* affect name lookups in the current
global scope and vice-versa.
[clinic start generated code]*/

static PyObject *
builtin_globals_impl(PyObject *module)
/*[clinic end generated code: output=e5dd1527067b94d2 input=9327576f92bb48ba]*/
{
    PyObject *d;

    d = PyEval_GetGlobals();
    Py_XINCREF(d);
    return d;
}


/*[clinic input]
hasattr as builtin_hasattr

    obj: object
    name: object
    /

Return whether the object has an attribute with the given name.

This is done by calling getattr(obj, name) and catching AttributeError.
[clinic start generated code]*/

static PyObject *
builtin_hasattr_impl(PyObject *module, PyObject *obj, PyObject *name)
/*[clinic end generated code: output=a7aff2090a4151e5 input=0faec9787d979542]*/
{
    PyObject *v;

    if (!PyUnicode_Check(name)) {
        PyErr_SetString(PyExc_TypeError,
                        "hasattr(): attribute name must be string");
        return NULL;
    }
    if (_PyObject_LookupAttr(obj, name, &v) < 0) {
        return NULL;
    }
    if (v == NULL) {
        Py_RETURN_FALSE;
    }
    Py_DECREF(v);
    Py_RETURN_TRUE;
}


/* AC: gdb's integration with CPython relies on builtin_id having
 * the *exact* parameter names of "self" and "v", so we ensure we
 * preserve those name rather than using the AC defaults.
 */
/*[clinic input]
id as builtin_id

    self: self(type="PyModuleDef *")
    obj as v: object
    /

Return the identity of an object.

This is guaranteed to be unique among simultaneously existing objects.
(CPython uses the object's memory address.)
[clinic start generated code]*/

static PyObject *
builtin_id(PyModuleDef *self, PyObject *v)
/*[clinic end generated code: output=0aa640785f697f65 input=5a534136419631f4]*/
{
    PyObject *id = PyLong_FromVoidPtr(v);

    if (id && PySys_Audit("builtins.id", "O", id) < 0) {
        Py_DECREF(id);
        return NULL;
    }

    return id;
}


/* map object ************************************************************/

typedef struct {
    PyObject_HEAD
    PyObject *iters;
    PyObject *func;
} mapobject;

static PyObject *
map_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyObject *it, *iters, *func;
    mapobject *lz;
    Py_ssize_t numargs, i;

    if (type == &PyMap_Type && !_PyArg_NoKeywords("map", kwds))
        return NULL;

    numargs = PyTuple_Size(args);
    if (numargs < 2) {
        PyErr_SetString(PyExc_TypeError,
           "map() must have at least two arguments.");
        return NULL;
    }

    iters = PyTuple_New(numargs-1);
    if (iters == NULL)
        return NULL;

    for (i=1 ; i<numargs ; i++) {
        /* Get iterator. */
        it = PyObject_GetIter(PyTuple_GET_ITEM(args, i));
        if (it == NULL) {
            Py_DECREF(iters);
            return NULL;
        }
        PyTuple_SET_ITEM(iters, i-1, it);
    }

    /* create mapobject structure */
    lz = (mapobject *)type->tp_alloc(type, 0);
    if (lz == NULL) {
        Py_DECREF(iters);
        return NULL;
    }
    lz->iters = iters;
    func = PyTuple_GET_ITEM(args, 0);
    lz->func = Py_NewRef(func);

    return (PyObject *)lz;
}

static PyObject *
map_vectorcall(PyObject *type, PyObject * const*args,
                size_t nargsf, PyObject *kwnames)
{
    PyTypeObject *tp = (PyTypeObject *)type;
    if (tp == &PyMap_Type && !_PyArg_NoKwnames("map", kwnames)) {
        return NULL;
    }

    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    if (nargs < 2) {
        PyErr_SetString(PyExc_TypeError,
           "map() must have at least two arguments.");
        return NULL;
    }

    PyObject *iters = PyTuple_New(nargs-1);
    if (iters == NULL) {
        return NULL;
    }

    for (int i=1; i<nargs; i++) {
        PyObject *it = PyObject_GetIter(args[i]);
        if (it == NULL) {
            Py_DECREF(iters);
            return NULL;
        }
        PyTuple_SET_ITEM(iters, i-1, it);
    }

    mapobject *lz = (mapobject *)tp->tp_alloc(tp, 0);
    if (lz == NULL) {
        Py_DECREF(iters);
        return NULL;
    }
    lz->iters = iters;
    lz->func = Py_NewRef(args[0]);

    return (PyObject *)lz;
}

static void
map_dealloc(mapobject *lz)
{
    PyObject_GC_UnTrack(lz);
    Py_XDECREF(lz->iters);
    Py_XDECREF(lz->func);
    Py_TYPE(lz)->tp_free(lz);
}

static int
map_traverse(mapobject *lz, visitproc visit, void *arg)
{
    Py_VISIT(lz->iters);
    Py_VISIT(lz->func);
    return 0;
}

static PyObject *
map_next(mapobject *lz)
{
    PyObject *small_stack[_PY_FASTCALL_SMALL_STACK];
    PyObject **stack;
    PyObject *result = NULL;
    PyThreadState *tstate = _PyThreadState_GET();

    const Py_ssize_t niters = PyTuple_GET_SIZE(lz->iters);
    if (niters <= (Py_ssize_t)Py_ARRAY_LENGTH(small_stack)) {
        stack = small_stack;
    }
    else {
        stack = PyMem_Malloc(niters * sizeof(stack[0]));
        if (stack == NULL) {
            _PyErr_NoMemory(tstate);
            return NULL;
        }
    }

    Py_ssize_t nargs = 0;
    for (Py_ssize_t i=0; i < niters; i++) {
        PyObject *it = PyTuple_GET_ITEM(lz->iters, i);
        PyObject *val = Py_TYPE(it)->tp_iternext(it);
        if (val == NULL) {
            goto exit;
        }
        stack[i] = val;
        nargs++;
    }

    result = _PyObject_VectorcallTstate(tstate, lz->func, stack, nargs, NULL);

exit:
    for (Py_ssize_t i=0; i < nargs; i++) {
        Py_DECREF(stack[i]);
    }
    if (stack != small_stack) {
        PyMem_Free(stack);
    }
    return result;
}

static PyObject *
map_reduce(mapobject *lz, PyObject *Py_UNUSED(ignored))
{
    Py_ssize_t numargs = PyTuple_GET_SIZE(lz->iters);
    PyObject *args = PyTuple_New(numargs+1);
    Py_ssize_t i;
    if (args == NULL)
        return NULL;
    Py_INCREF(lz->func);
    PyTuple_SET_ITEM(args, 0, lz->func);
    for (i = 0; i<numargs; i++){
        PyObject *it = PyTuple_GET_ITEM(lz->iters, i);
        Py_INCREF(it);
        PyTuple_SET_ITEM(args, i+1, it);
    }

    return Py_BuildValue("ON", Py_TYPE(lz), args);
}

static PyMethodDef map_methods[] = {
    {"__reduce__",   (PyCFunction)map_reduce,   METH_NOARGS, reduce_doc},
    {NULL,           NULL}           /* sentinel */
};


PyDoc_STRVAR(map_doc,
"map(func, *iterables) --> map object\n\
\n\
Make an iterator that computes the function using arguments from\n\
each of the iterables.  Stops when the shortest iterable is exhausted.");

PyTypeObject PyMap_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "map",                              /* tp_name */
    sizeof(mapobject),                  /* tp_basicsize */
    0,                                  /* tp_itemsize */
    /* methods */
    (destructor)map_dealloc,            /* tp_dealloc */
    0,                                  /* tp_vectorcall_offset */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_as_async */
    0,                                  /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    0,                                  /* tp_hash */
    0,                                  /* tp_call */
    0,                                  /* tp_str */
    PyObject_GenericGetAttr,            /* tp_getattro */
    0,                                  /* tp_setattro */
    0,                                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
        Py_TPFLAGS_BASETYPE,            /* tp_flags */
    map_doc,                            /* tp_doc */
    (traverseproc)map_traverse,         /* tp_traverse */
    0,                                  /* tp_clear */
    0,                                  /* tp_richcompare */
    0,                                  /* tp_weaklistoffset */
    PyObject_SelfIter,                  /* tp_iter */
    (iternextfunc)map_next,     /* tp_iternext */
    map_methods,                        /* tp_methods */
    0,                                  /* tp_members */
    0,                                  /* tp_getset */
    0,                                  /* tp_base */
    0,                                  /* tp_dict */
    0,                                  /* tp_descr_get */
    0,                                  /* tp_descr_set */
    0,                                  /* tp_dictoffset */
    0,                                  /* tp_init */
    PyType_GenericAlloc,                /* tp_alloc */
    map_new,                            /* tp_new */
    PyObject_GC_Del,                    /* tp_free */
    .tp_vectorcall = (vectorcallfunc)map_vectorcall
};


/* AC: cannot convert yet, as needs PEP 457 group support in inspect */
static PyObject *
builtin_next(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *it, *res;

    if (!_PyArg_CheckPositional("next", nargs, 1, 2))
        return NULL;

    it = args[0];
    if (!PyIter_Check(it)) {
        PyErr_Format(PyExc_TypeError,
            "'%.200s' object is not an iterator",
            Py_TYPE(it)->tp_name);
        return NULL;
    }

    res = (*Py_TYPE(it)->tp_iternext)(it);
    if (res != NULL) {
        return res;
    } else if (nargs > 1) {
        PyObject *def = args[1];
        if (PyErr_Occurred()) {
            if(!PyErr_ExceptionMatches(PyExc_StopIteration))
                return NULL;
            PyErr_Clear();
        }
        Py_INCREF(def);
        return def;
    } else if (PyErr_Occurred()) {
        return NULL;
    } else {
        PyErr_SetNone(PyExc_StopIteration);
        return NULL;
    }
}

PyDoc_STRVAR(next_doc,
"next(iterator[, default])\n\
\n\
Return the next item from the iterator. If default is given and the iterator\n\
is exhausted, it is returned instead of raising StopIteration.");


/*[clinic input]
setattr as builtin_setattr

    obj: object
    name: object
    value: object
    /

Sets the named attribute on the given object to the specified value.

setattr(x, 'y', v) is equivalent to ``x.y = v''
[clinic start generated code]*/

static PyObject *
builtin_setattr_impl(PyObject *module, PyObject *obj, PyObject *name,
                     PyObject *value)
/*[clinic end generated code: output=dc2ce1d1add9acb4 input=bd2b7ca6875a1899]*/
{
    if (PyObject_SetAttr(obj, name, value) != 0)
        return NULL;
    Py_RETURN_NONE;
}


/*[clinic input]
delattr as builtin_delattr

    obj: object
    name: object
    /

Deletes the named attribute from the given object.

delattr(x, 'y') is equivalent to ``del x.y''
[clinic start generated code]*/

static PyObject *
builtin_delattr_impl(PyObject *module, PyObject *obj, PyObject *name)
/*[clinic end generated code: output=85134bc58dff79fa input=db16685d6b4b9410]*/
{
    if (PyObject_SetAttr(obj, name, (PyObject *)NULL) != 0)
        return NULL;
    Py_RETURN_NONE;
}


/*[clinic input]
hash as builtin_hash

    obj: object
    /

Return the hash value for the given object.

Two objects that compare equal must also have the same hash value, but the
reverse is not necessarily true.
[clinic start generated code]*/

static PyObject *
builtin_hash(PyObject *module, PyObject *obj)
/*[clinic end generated code: output=237668e9d7688db7 input=58c48be822bf9c54]*/
{
    Py_hash_t x;

    x = PyObject_Hash(obj);
    if (x == -1)
        return NULL;
    return PyLong_FromSsize_t(x);
}


/*[clinic input]
hex as builtin_hex

    number: object
    /

Return the hexadecimal representation of an integer.

   >>> hex(12648430)
   '0xc0ffee'
[clinic start generated code]*/

static PyObject *
builtin_hex(PyObject *module, PyObject *number)
/*[clinic end generated code: output=e46b612169099408 input=e645aff5fc7d540e]*/
{
    return PyNumber_ToBase(number, 16);
}


/* AC: cannot convert yet, as needs PEP 457 group support in inspect */
static PyObject *
builtin_iter(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *v;

    if (!_PyArg_CheckPositional("iter", nargs, 1, 2))
        return NULL;
    v = args[0];
    if (nargs == 1)
        return PyObject_GetIter(v);
    if (!PyCallable_Check(v)) {
        PyErr_SetString(PyExc_TypeError,
                        "iter(v, w): v must be callable");
        return NULL;
    }
    PyObject *sentinel = args[1];
    return PyCallIter_New(v, sentinel);
}

PyDoc_STRVAR(iter_doc,
"iter(iterable) -> iterator\n\
iter(callable, sentinel) -> iterator\n\
\n\
Get an iterator from an object.  In the first form, the argument must\n\
supply its own iterator, or be a sequence.\n\
In the second form, the callable is called until it returns the sentinel.");


/*[clinic input]
aiter as builtin_aiter

    async_iterable: object
    /

Return an AsyncIterator for an AsyncIterable object.
[clinic start generated code]*/

static PyObject *
builtin_aiter(PyObject *module, PyObject *async_iterable)
/*[clinic end generated code: output=1bae108d86f7960e input=473993d0cacc7d23]*/
{
    return PyObject_GetAiter(async_iterable);
}

PyObject *PyAnextAwaitable_New(PyObject *, PyObject *);

/*[clinic input]
anext as builtin_anext

    aiterator: object
    default: object = NULL
    /

async anext(aiterator[, default])

Return the next item from the async iterator.  If default is given and the async
iterator is exhausted, it is returned instead of raising StopAsyncIteration.
[clinic start generated code]*/

static PyObject *
builtin_anext_impl(PyObject *module, PyObject *aiterator,
                   PyObject *default_value)
/*[clinic end generated code: output=f02c060c163a81fa input=8f63f4f78590bb4c]*/
{
    PyTypeObject *t;
    PyObject *awaitable;

    t = Py_TYPE(aiterator);
    if (t->tp_as_async == NULL || t->tp_as_async->am_anext == NULL) {
        PyErr_Format(PyExc_TypeError,
            "'%.200s' object is not an async iterator",
            t->tp_name);
        return NULL;
    }

    awaitable = (*t->tp_as_async->am_anext)(aiterator);
    if (default_value == NULL) {
        return awaitable;
    }

    PyObject* new_awaitable = PyAnextAwaitable_New(
            awaitable, default_value);
    Py_DECREF(awaitable);
    return new_awaitable;
}


/*[clinic input]
len as builtin_len

    obj: object
    /

Return the number of items in a container.
[clinic start generated code]*/

static PyObject *
builtin_len(PyObject *module, PyObject *obj)
/*[clinic end generated code: output=fa7a270d314dfb6c input=bc55598da9e9c9b5]*/
{
    Py_ssize_t res;

    res = PyObject_Size(obj);
    if (res < 0) {
        assert(PyErr_Occurred());
        return NULL;
    }
    return PyLong_FromSsize_t(res);
}


/*[clinic input]
locals as builtin_locals

Return a dictionary containing the current scope's local variables.

NOTE: Whether or not updates to this dictionary will affect name lookups in
the local scope and vice-versa is *implementation dependent* and not
covered by any backwards compatibility guarantees.
[clinic start generated code]*/

static PyObject *
builtin_locals_impl(PyObject *module)
/*[clinic end generated code: output=b46c94015ce11448 input=7874018d478d5c4b]*/
{
    PyObject *d;

    d = PyEval_GetLocals();
    Py_XINCREF(d);
    return d;
}


static PyObject *
min_max(PyObject *args, PyObject *kwds, int op)
{
    PyObject *v, *it, *item, *val, *maxitem, *maxval, *keyfunc=NULL;
    PyObject *emptytuple, *defaultval = NULL;
    static char *kwlist[] = {"key", "default", NULL};
    const char *name = op == Py_LT ? "min" : "max";
    const int positional = PyTuple_Size(args) > 1;
    int ret;

    if (positional) {
        v = args;
    }
    else if (!PyArg_UnpackTuple(args, name, 1, 1, &v)) {
        if (PyExceptionClass_Check(PyExc_TypeError)) {
            PyErr_Format(PyExc_TypeError, "%s expected at least 1 argument, got 0", name);
        }
        return NULL;
    }

    emptytuple = PyTuple_New(0);
    if (emptytuple == NULL)
        return NULL;
    ret = PyArg_ParseTupleAndKeywords(emptytuple, kwds,
                                      (op == Py_LT) ? "|$OO:min" : "|$OO:max",
                                      kwlist, &keyfunc, &defaultval);
    Py_DECREF(emptytuple);
    if (!ret)
        return NULL;

    if (positional && defaultval != NULL) {
        PyErr_Format(PyExc_TypeError,
                        "Cannot specify a default for %s() with multiple "
                        "positional arguments", name);
        return NULL;
    }

    it = PyObject_GetIter(v);
    if (it == NULL) {
        return NULL;
    }

    if (keyfunc == Py_None) {
        keyfunc = NULL;
    }

    maxitem = NULL; /* the result */
    maxval = NULL;  /* the value associated with the result */
    while (( item = PyIter_Next(it) )) {
        /* get the value from the key function */
        if (keyfunc != NULL) {
            val = PyObject_CallOneArg(keyfunc, item);
            if (val == NULL)
                goto Fail_it_item;
        }
        /* no key function; the value is the item */
        else {
            val = item;
            Py_INCREF(val);
        }

        /* maximum value and item are unset; set them */
        if (maxval == NULL) {
            maxitem = item;
            maxval = val;
        }
        /* maximum value and item are set; update them as necessary */
        else {
            int cmp = PyObject_RichCompareBool(val, maxval, op);
            if (cmp < 0)
                goto Fail_it_item_and_val;
            else if (cmp > 0) {
                Py_DECREF(maxval);
                Py_DECREF(maxitem);
                maxval = val;
                maxitem = item;
            }
            else {
                Py_DECREF(item);
                Py_DECREF(val);
            }
        }
    }
    if (PyErr_Occurred())
        goto Fail_it;
    if (maxval == NULL) {
        assert(maxitem == NULL);
        if (defaultval != NULL) {
            Py_INCREF(defaultval);
            maxitem = defaultval;
        } else {
            PyErr_Format(PyExc_ValueError,
                         "%s() arg is an empty sequence", name);
        }
    }
    else
        Py_DECREF(maxval);
    Py_DECREF(it);
    return maxitem;

Fail_it_item_and_val:
    Py_DECREF(val);
Fail_it_item:
    Py_DECREF(item);
Fail_it:
    Py_XDECREF(maxval);
    Py_XDECREF(maxitem);
    Py_DECREF(it);
    return NULL;
}

/* AC: cannot convert yet, waiting for *args support */
static PyObject *
builtin_min(PyObject *self, PyObject *args, PyObject *kwds)
{
    return min_max(args, kwds, Py_LT);
}

PyDoc_STRVAR(min_doc,
"min(iterable, *[, default=obj, key=func]) -> value\n\
min(arg1, arg2, *args, *[, key=func]) -> value\n\
\n\
With a single iterable argument, return its smallest item. The\n\
default keyword-only argument specifies an object to return if\n\
the provided iterable is empty.\n\
With two or more arguments, return the smallest argument.");


/* AC: cannot convert yet, waiting for *args support */
static PyObject *
builtin_max(PyObject *self, PyObject *args, PyObject *kwds)
{
    return min_max(args, kwds, Py_GT);
}

PyDoc_STRVAR(max_doc,
"max(iterable, *[, default=obj, key=func]) -> value\n\
max(arg1, arg2, *args, *[, key=func]) -> value\n\
\n\
With a single iterable argument, return its biggest item. The\n\
default keyword-only argument specifies an object to return if\n\
the provided iterable is empty.\n\
With two or more arguments, return the largest argument.");


/*[clinic input]
oct as builtin_oct

    number: object
    /

Return the octal representation of an integer.

   >>> oct(342391)
   '0o1234567'
[clinic start generated code]*/

static PyObject *
builtin_oct(PyObject *module, PyObject *number)
/*[clinic end generated code: output=40a34656b6875352 input=ad6b274af4016c72]*/
{
    return PyNumber_ToBase(number, 8);
}


/*[clinic input]
ord as builtin_ord

    c: object
    /

Return the Unicode code point for a one-character string.
[clinic start generated code]*/

static PyObject *
builtin_ord(PyObject *module, PyObject *c)
/*[clinic end generated code: output=4fa5e87a323bae71 input=3064e5d6203ad012]*/
{
    long ord;
    Py_ssize_t size;

    if (PyBytes_Check(c)) {
        size = PyBytes_GET_SIZE(c);
        if (size == 1) {
            ord = (long)((unsigned char)*PyBytes_AS_STRING(c));
            return PyLong_FromLong(ord);
        }
    }
    else if (PyUnicode_Check(c)) {
        if (PyUnicode_READY(c) == -1)
            return NULL;
        size = PyUnicode_GET_LENGTH(c);
        if (size == 1) {
            ord = (long)PyUnicode_READ_CHAR(c, 0);
            return PyLong_FromLong(ord);
        }
    }
    else if (PyByteArray_Check(c)) {
        /* XXX Hopefully this is temporary */
        size = PyByteArray_GET_SIZE(c);
        if (size == 1) {
            ord = (long)((unsigned char)*PyByteArray_AS_STRING(c));
            return PyLong_FromLong(ord);
        }
    }
    else {
        PyErr_Format(PyExc_TypeError,
                     "ord() expected string of length 1, but " \
                     "%.200s found", Py_TYPE(c)->tp_name);
        return NULL;
    }

    PyErr_Format(PyExc_TypeError,
                 "ord() expected a character, "
                 "but string of length %zd found",
                 size);
    return NULL;
}


/*[clinic input]
pow as builtin_pow

    base: object
    exp: object
    mod: object = None

Equivalent to base**exp with 2 arguments or base**exp % mod with 3 arguments

Some types, such as ints, are able to use a more efficient algorithm when
invoked using the three argument form.
[clinic start generated code]*/

static PyObject *
builtin_pow_impl(PyObject *module, PyObject *base, PyObject *exp,
                 PyObject *mod)
/*[clinic end generated code: output=3ca1538221bbf15f input=435dbd48a12efb23]*/
{
    return PyNumber_Power(base, exp, mod);
}

/*[clinic input]
print as builtin_print

    *args: object
    sep: object(c_default="Py_None") = ' '
        string inserted between values, default a space.
    end: object(c_default="Py_None") = '\n'
        string appended after the last value, default a newline.
    file: object = None
        a file-like object (stream); defaults to the current sys.stdout.
    flush: bool = False
        whether to forcibly flush the stream.

Prints the values to a stream, or to sys.stdout by default.

[clinic start generated code]*/

static PyObject *
builtin_print_impl(PyObject *module, PyObject *args, PyObject *sep,
                   PyObject *end, PyObject *file, int flush)
/*[clinic end generated code: output=3cfc0940f5bc237b input=c143c575d24fe665]*/
{
    int i, err;

    if (file == Py_None) {
        file = _PySys_GetObjectId(&PyId_stdout);
        if (file == NULL) {
            PyErr_SetString(PyExc_RuntimeError, "lost sys.stdout");
            return NULL;
        }

        /* sys.stdout may be None when FILE* stdout isn't connected */
        if (file == Py_None) {
            Py_RETURN_NONE;
        }
    }

    if (sep == Py_None) {
        sep = NULL;
    }
    else if (sep && !PyUnicode_Check(sep)) {
        PyErr_Format(PyExc_TypeError,
                     "sep must be None or a string, not %.200s",
                     Py_TYPE(sep)->tp_name);
        return NULL;
    }
    if (end == Py_None) {
        end = NULL;
    }
    else if (end && !PyUnicode_Check(end)) {
        PyErr_Format(PyExc_TypeError,
                     "end must be None or a string, not %.200s",
                     Py_TYPE(end)->tp_name);
        return NULL;
    }

    for (i = 0; i < PyTuple_GET_SIZE(args); i++) {
        if (i > 0) {
            if (sep == NULL) {
                err = PyFile_WriteString(" ", file);
            }
            else {
                err = PyFile_WriteObject(sep, file, Py_PRINT_RAW);
            }
            if (err) {
                return NULL;
            }
        }
        err = PyFile_WriteObject(PyTuple_GET_ITEM(args, i), file, Py_PRINT_RAW);
        if (err) {
            return NULL;
        }
    }

    if (end == NULL) {
        err = PyFile_WriteString("\n", file);
    }
    else {
        err = PyFile_WriteObject(end, file, Py_PRINT_RAW);
    }
    if (err) {
        return NULL;
    }

    if (flush) {
        PyObject *tmp = _PyObject_CallMethodIdNoArgs(file, &PyId_flush);
        if (tmp == NULL) {
            return NULL;
        }
        Py_DECREF(tmp);
    }

    Py_RETURN_NONE;
}


/*[clinic input]
input as builtin_input

    prompt: object(c_default="NULL") = None
    /

Read a string from standard input.  The trailing newline is stripped.

The prompt string, if given, is printed to standard output without a
trailing newline before reading input.

If the user hits EOF (*nix: Ctrl-D, Windows: Ctrl-Z+Return), raise EOFError.
On *nix systems, readline is used if available.
[clinic start generated code]*/

static PyObject *
builtin_input_impl(PyObject *module, PyObject *prompt)
/*[clinic end generated code: output=83db5a191e7a0d60 input=5e8bb70c2908fe3c]*/
{
    PyObject *fin = _PySys_GetObjectId(&PyId_stdin);
    PyObject *fout = _PySys_GetObjectId(&PyId_stdout);
    PyObject *ferr = _PySys_GetObjectId(&PyId_stderr);
    PyObject *tmp;
    long fd;
    int tty;

    /* Check that stdin/out/err are intact */
    if (fin == NULL || fin == Py_None) {
        PyErr_SetString(PyExc_RuntimeError,
                        "input(): lost sys.stdin");
        return NULL;
    }
    if (fout == NULL || fout == Py_None) {
        PyErr_SetString(PyExc_RuntimeError,
                        "input(): lost sys.stdout");
        return NULL;
    }
    if (ferr == NULL || ferr == Py_None) {
        PyErr_SetString(PyExc_RuntimeError,
                        "input(): lost sys.stderr");
        return NULL;
    }

    if (PySys_Audit("builtins.input", "O", prompt ? prompt : Py_None) < 0) {
        return NULL;
    }

    /* First of all, flush stderr */
    tmp = _PyObject_CallMethodIdNoArgs(ferr, &PyId_flush);
    if (tmp == NULL)
        PyErr_Clear();
    else
        Py_DECREF(tmp);

    /* We should only use (GNU) readline if Python's sys.stdin and
       sys.stdout are the same as C's stdin and stdout, because we
       need to pass it those. */
    tmp = _PyObject_CallMethodIdNoArgs(fin, &PyId_fileno);
    if (tmp == NULL) {
        PyErr_Clear();
        tty = 0;
    }
    else {
        fd = PyLong_AsLong(tmp);
        Py_DECREF(tmp);
        if (fd < 0 && PyErr_Occurred())
            return NULL;
        tty = fd == fileno(stdin) && isatty(fd);
    }
    if (tty) {
        tmp = _PyObject_CallMethodIdNoArgs(fout, &PyId_fileno);
        if (tmp == NULL) {
            PyErr_Clear();
            tty = 0;
        }
        else {
            fd = PyLong_AsLong(tmp);
            Py_DECREF(tmp);
            if (fd < 0 && PyErr_Occurred())
                return NULL;
            tty = fd == fileno(stdout) && isatty(fd);
        }
    }

    /* If we're interactive, use (GNU) readline */
    if (tty) {
        PyObject *po = NULL;
        const char *promptstr;
        char *s = NULL;
        PyObject *stdin_encoding = NULL, *stdin_errors = NULL;
        PyObject *stdout_encoding = NULL, *stdout_errors = NULL;
        const char *stdin_encoding_str, *stdin_errors_str;
        PyObject *result;
        size_t len;

        /* stdin is a text stream, so it must have an encoding. */
        stdin_encoding = _PyObject_GetAttrId(fin, &PyId_encoding);
        stdin_errors = _PyObject_GetAttrId(fin, &PyId_errors);
        if (!stdin_encoding || !stdin_errors ||
                !PyUnicode_Check(stdin_encoding) ||
                !PyUnicode_Check(stdin_errors)) {
            tty = 0;
            goto _readline_errors;
        }
        stdin_encoding_str = PyUnicode_AsUTF8(stdin_encoding);
        stdin_errors_str = PyUnicode_AsUTF8(stdin_errors);
        if (!stdin_encoding_str || !stdin_errors_str)
            goto _readline_errors;
        tmp = _PyObject_CallMethodIdNoArgs(fout, &PyId_flush);
        if (tmp == NULL)
            PyErr_Clear();
        else
            Py_DECREF(tmp);
        if (prompt != NULL) {
            /* We have a prompt, encode it as stdout would */
            const char *stdout_encoding_str, *stdout_errors_str;
            PyObject *stringpo;
            stdout_encoding = _PyObject_GetAttrId(fout, &PyId_encoding);
            stdout_errors = _PyObject_GetAttrId(fout, &PyId_errors);
            if (!stdout_encoding || !stdout_errors ||
                    !PyUnicode_Check(stdout_encoding) ||
                    !PyUnicode_Check(stdout_errors)) {
                tty = 0;
                goto _readline_errors;
            }
            stdout_encoding_str = PyUnicode_AsUTF8(stdout_encoding);
            stdout_errors_str = PyUnicode_AsUTF8(stdout_errors);
            if (!stdout_encoding_str || !stdout_errors_str)
                goto _readline_errors;
            stringpo = PyObject_Str(prompt);
            if (stringpo == NULL)
                goto _readline_errors;
            po = PyUnicode_AsEncodedString(stringpo,
                stdout_encoding_str, stdout_errors_str);
            Py_CLEAR(stdout_encoding);
            Py_CLEAR(stdout_errors);
            Py_CLEAR(stringpo);
            if (po == NULL)
                goto _readline_errors;
            assert(PyBytes_Check(po));
            promptstr = PyBytes_AS_STRING(po);
        }
        else {
            po = NULL;
            promptstr = "";
        }
        s = PyOS_Readline(stdin, stdout, promptstr);
        if (s == NULL) {
            PyErr_CheckSignals();
            if (!PyErr_Occurred())
                PyErr_SetNone(PyExc_KeyboardInterrupt);
            goto _readline_errors;
        }

        len = strlen(s);
        if (len == 0) {
            PyErr_SetNone(PyExc_EOFError);
            result = NULL;
        }
        else {
            if (len > PY_SSIZE_T_MAX) {
                PyErr_SetString(PyExc_OverflowError,
                                "input: input too long");
                result = NULL;
            }
            else {
                len--;   /* strip trailing '\n' */
                if (len != 0 && s[len-1] == '\r')
                    len--;   /* strip trailing '\r' */
                result = PyUnicode_Decode(s, len, stdin_encoding_str,
                                                  stdin_errors_str);
            }
        }
        Py_DECREF(stdin_encoding);
        Py_DECREF(stdin_errors);
        Py_XDECREF(po);
        PyMem_Free(s);

        if (result != NULL) {
            if (PySys_Audit("builtins.input/result", "O", result) < 0) {
                return NULL;
            }
        }

        return result;

    _readline_errors:
        Py_XDECREF(stdin_encoding);
        Py_XDECREF(stdout_encoding);
        Py_XDECREF(stdin_errors);
        Py_XDECREF(stdout_errors);
        Py_XDECREF(po);
        if (tty)
            return NULL;

        PyErr_Clear();
    }

    /* Fallback if we're not interactive */
    if (prompt != NULL) {
        if (PyFile_WriteObject(prompt, fout, Py_PRINT_RAW) != 0)
            return NULL;
    }
    tmp = _PyObject_CallMethodIdNoArgs(fout, &PyId_flush);
    if (tmp == NULL)
        PyErr_Clear();
    else
        Py_DECREF(tmp);
    return PyFile_GetLine(fin, -1);
}


/*[clinic input]
repr as builtin_repr

    obj: object
    /

Return the canonical string representation of the object.

For many object types, including most builtins, eval(repr(obj)) == obj.
[clinic start generated code]*/

static PyObject *
builtin_repr(PyObject *module, PyObject *obj)
/*[clinic end generated code: output=7ed3778c44fd0194 input=1c9e6d66d3e3be04]*/
{
    return PyObject_Repr(obj);
}


/*[clinic input]
round as builtin_round

    number: object
    ndigits: object = None

Round a number to a given precision in decimal digits.

The return value is an integer if ndigits is omitted or None.  Otherwise
the return value has the same type as the number.  ndigits may be negative.
[clinic start generated code]*/

static PyObject *
builtin_round_impl(PyObject *module, PyObject *number, PyObject *ndigits)
/*[clinic end generated code: output=ff0d9dd176c02ede input=275678471d7aca15]*/
{
    PyObject *round, *result;

    if (Py_TYPE(number)->tp_dict == NULL) {
        if (PyType_Ready(Py_TYPE(number)) < 0)
            return NULL;
    }

    round = _PyObject_LookupSpecial(number, &PyId___round__);
    if (round == NULL) {
        if (!PyErr_Occurred())
            PyErr_Format(PyExc_TypeError,
                         "type %.100s doesn't define __round__ method",
                         Py_TYPE(number)->tp_name);
        return NULL;
    }

    if (ndigits == Py_None)
        result = _PyObject_CallNoArg(round);
    else
        result = PyObject_CallOneArg(round, ndigits);
    Py_DECREF(round);
    return result;
}


/*AC: we need to keep the kwds dict intact to easily call into the
 * list.sort method, which isn't currently supported in AC. So we just use
 * the initially generated signature with a custom implementation.
 */
/* [disabled clinic input]
sorted as builtin_sorted

    iterable as seq: object
    key as keyfunc: object = None
    reverse: object = False

Return a new list containing all items from the iterable in ascending order.

A custom key function can be supplied to customize the sort order, and the
reverse flag can be set to request the result in descending order.
[end disabled clinic input]*/

PyDoc_STRVAR(builtin_sorted__doc__,
"sorted($module, iterable, /, *, key=None, reverse=False)\n"
"--\n"
"\n"
"Return a new list containing all items from the iterable in ascending order.\n"
"\n"
"A custom key function can be supplied to customize the sort order, and the\n"
"reverse flag can be set to request the result in descending order.");

#define BUILTIN_SORTED_METHODDEF    \
    {"sorted", (PyCFunction)(void(*)(void))builtin_sorted, METH_FASTCALL | METH_KEYWORDS, builtin_sorted__doc__},

static PyObject *
builtin_sorted(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *newlist, *v, *seq, *callable;

    /* Keyword arguments are passed through list.sort() which will check
       them. */
    if (!_PyArg_UnpackStack(args, nargs, "sorted", 1, 1, &seq))
        return NULL;

    newlist = PySequence_List(seq);
    if (newlist == NULL)
        return NULL;

    callable = _PyObject_GetAttrId(newlist, &PyId_sort);
    if (callable == NULL) {
        Py_DECREF(newlist);
        return NULL;
    }

    assert(nargs >= 1);
    v = PyObject_Vectorcall(callable, args + 1, nargs - 1, kwnames);
    Py_DECREF(callable);
    if (v == NULL) {
        Py_DECREF(newlist);
        return NULL;
    }
    Py_DECREF(v);
    return newlist;
}


/* AC: cannot convert yet, as needs PEP 457 group support in inspect */
static PyObject *
builtin_vars(PyObject *self, PyObject *args)
{
    PyObject *v = NULL;
    PyObject *d;

    if (!PyArg_UnpackTuple(args, "vars", 0, 1, &v))
        return NULL;
    if (v == NULL) {
        d = PyEval_GetLocals();
        Py_XINCREF(d);
    }
    else {
        if (_PyObject_LookupAttrId(v, &PyId___dict__, &d) == 0) {
            PyErr_SetString(PyExc_TypeError,
                "vars() argument must have __dict__ attribute");
        }
    }
    return d;
}

PyDoc_STRVAR(vars_doc,
"vars([object]) -> dictionary\n\
\n\
Without arguments, equivalent to locals().\n\
With an argument, equivalent to object.__dict__.");


/*[clinic input]
sum as builtin_sum

    iterable: object
    /
    start: object(c_default="NULL") = 0

Return the sum of a 'start' value (default: 0) plus an iterable of numbers

When the iterable is empty, return the start value.
This function is intended specifically for use with numeric values and may
reject non-numeric types.
[clinic start generated code]*/

static PyObject *
builtin_sum_impl(PyObject *module, PyObject *iterable, PyObject *start)
/*[clinic end generated code: output=df758cec7d1d302f input=162b50765250d222]*/
{
    PyObject *result = start;
    PyObject *temp, *item, *iter;

    iter = PyObject_GetIter(iterable);
    if (iter == NULL)
        return NULL;

    if (result == NULL) {
        result = PyLong_FromLong(0);
        if (result == NULL) {
            Py_DECREF(iter);
            return NULL;
        }
    } else {
        /* reject string values for 'start' parameter */
        if (PyUnicode_Check(result)) {
            PyErr_SetString(PyExc_TypeError,
                "sum() can't sum strings [use ''.join(seq) instead]");
            Py_DECREF(iter);
            return NULL;
        }
        if (PyBytes_Check(result)) {
            PyErr_SetString(PyExc_TypeError,
                "sum() can't sum bytes [use b''.join(seq) instead]");
            Py_DECREF(iter);
            return NULL;
        }
        if (PyByteArray_Check(result)) {
            PyErr_SetString(PyExc_TypeError,
                "sum() can't sum bytearray [use b''.join(seq) instead]");
            Py_DECREF(iter);
            return NULL;
        }
        Py_INCREF(result);
    }

#ifndef SLOW_SUM
    /* Fast addition by keeping temporary sums in C instead of new Python objects.
       Assumes all inputs are the same type.  If the assumption fails, default
       to the more general routine.
    */
    if (PyLong_CheckExact(result)) {
        int overflow;
        long i_result = PyLong_AsLongAndOverflow(result, &overflow);
        /* If this already overflowed, don't even enter the loop. */
        if (overflow == 0) {
            Py_DECREF(result);
            result = NULL;
        }
        while(result == NULL) {
            item = PyIter_Next(iter);
            if (item == NULL) {
                Py_DECREF(iter);
                if (PyErr_Occurred())
                    return NULL;
                return PyLong_FromLong(i_result);
            }
            if (PyLong_CheckExact(item) || PyBool_Check(item)) {
                long b = PyLong_AsLongAndOverflow(item, &overflow);
                if (overflow == 0 &&
                    (i_result >= 0 ? (b <= LONG_MAX - i_result)
                                   : (b >= LONG_MIN - i_result)))
                {
                    i_result += b;
                    Py_DECREF(item);
                    continue;
                }
            }
            /* Either overflowed or is not an int. Restore real objects and process normally */
            result = PyLong_FromLong(i_result);
            if (result == NULL) {
                Py_DECREF(item);
                Py_DECREF(iter);
                return NULL;
            }
            temp = PyNumber_Add(result, item);
            Py_DECREF(result);
            Py_DECREF(item);
            result = temp;
            if (result == NULL) {
                Py_DECREF(iter);
                return NULL;
            }
        }
    }

    if (PyFloat_CheckExact(result)) {
        double f_result = PyFloat_AS_DOUBLE(result);
        Py_DECREF(result);
        result = NULL;
        while(result == NULL) {
            item = PyIter_Next(iter);
            if (item == NULL) {
                Py_DECREF(iter);
                if (PyErr_Occurred())
                    return NULL;
                return PyFloat_FromDouble(f_result);
            }
            if (PyFloat_CheckExact(item)) {
                f_result += PyFloat_AS_DOUBLE(item);
                Py_DECREF(item);
                continue;
            }
            if (PyLong_Check(item)) {
                long value;
                int overflow;
                value = PyLong_AsLongAndOverflow(item, &overflow);
                if (!overflow) {
                    f_result += (double)value;
                    Py_DECREF(item);
                    continue;
                }
            }
            result = PyFloat_FromDouble(f_result);
            if (result == NULL) {
                Py_DECREF(item);
                Py_DECREF(iter);
                return NULL;
            }
            temp = PyNumber_Add(result, item);
            Py_DECREF(result);
            Py_DECREF(item);
            result = temp;
            if (result == NULL) {
                Py_DECREF(iter);
                return NULL;
            }
        }
    }
#endif

    for(;;) {
        item = PyIter_Next(iter);
        if (item == NULL) {
            /* error, or end-of-sequence */
            if (PyErr_Occurred()) {
                Py_DECREF(result);
                result = NULL;
            }
            break;
        }
        /* It's tempting to use PyNumber_InPlaceAdd instead of
           PyNumber_Add here, to avoid quadratic running time
           when doing 'sum(list_of_lists, [])'.  However, this
           would produce a change in behaviour: a snippet like

             empty = []
             sum([[x] for x in range(10)], empty)

           would change the value of empty. In fact, using
           in-place addition rather that binary addition for
           any of the steps introduces subtle behavior changes:

           https://bugs.python.org/issue18305 */
        temp = PyNumber_Add(result, item);
        Py_DECREF(result);
        Py_DECREF(item);
        result = temp;
        if (result == NULL)
            break;
    }
    Py_DECREF(iter);
    return result;
}


/*[clinic input]
isinstance as builtin_isinstance

    obj: object
    class_or_tuple: object
    /

Return whether an object is an instance of a class or of a subclass thereof.

A tuple, as in ``isinstance(x, (A, B, ...))``, may be given as the target to
check against. This is equivalent to ``isinstance(x, A) or isinstance(x, B)
or ...`` etc.
[clinic start generated code]*/

static PyObject *
builtin_isinstance_impl(PyObject *module, PyObject *obj,
                        PyObject *class_or_tuple)
/*[clinic end generated code: output=6faf01472c13b003 input=ffa743db1daf7549]*/
{
    int retval;

    retval = PyObject_IsInstance(obj, class_or_tuple);
    if (retval < 0)
        return NULL;
    return PyBool_FromLong(retval);
}


/*[clinic input]
issubclass as builtin_issubclass

    cls: object
    class_or_tuple: object
    /

Return whether 'cls' is derived from another class or is the same class.

A tuple, as in ``issubclass(x, (A, B, ...))``, may be given as the target to
check against. This is equivalent to ``issubclass(x, A) or issubclass(x, B)
or ...``.
[clinic start generated code]*/

static PyObject *
builtin_issubclass_impl(PyObject *module, PyObject *cls,
                        PyObject *class_or_tuple)
/*[clinic end generated code: output=358412410cd7a250 input=a24b9f3d58c370d6]*/
{
    int retval;

    retval = PyObject_IsSubclass(cls, class_or_tuple);
    if (retval < 0)
        return NULL;
    return PyBool_FromLong(retval);
}

typedef struct {
    PyObject_HEAD
    Py_ssize_t tuplesize;
    PyObject *ittuple;     /* tuple of iterators */
    PyObject *result;
    int strict;
} zipobject;

static PyObject *
zip_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    zipobject *lz;
    Py_ssize_t i;
    PyObject *ittuple;  /* tuple of iterators */
    PyObject *result;
    Py_ssize_t tuplesize;
    int strict = 0;

    if (kwds) {
        PyObject *empty = PyTuple_New(0);
        if (empty == NULL) {
            return NULL;
        }
        static char *kwlist[] = {"strict", NULL};
        int parsed = PyArg_ParseTupleAndKeywords(
                empty, kwds, "|$p:zip", kwlist, &strict);
        Py_DECREF(empty);
        if (!parsed) {
            return NULL;
        }
    }

    /* args must be a tuple */
    assert(PyTuple_Check(args));
    tuplesize = PyTuple_GET_SIZE(args);

    /* obtain iterators */
    ittuple = PyTuple_New(tuplesize);
    if (ittuple == NULL)
        return NULL;
    for (i=0; i < tuplesize; ++i) {
        PyObject *item = PyTuple_GET_ITEM(args, i);
        PyObject *it = PyObject_GetIter(item);
        if (it == NULL) {
            Py_DECREF(ittuple);
            return NULL;
        }
        PyTuple_SET_ITEM(ittuple, i, it);
    }

    /* create a result holder */
    result = PyTuple_New(tuplesize);
    if (result == NULL) {
        Py_DECREF(ittuple);
        return NULL;
    }
    for (i=0 ; i < tuplesize ; i++) {
        Py_INCREF(Py_None);
        PyTuple_SET_ITEM(result, i, Py_None);
    }

    /* create zipobject structure */
    lz = (zipobject *)type->tp_alloc(type, 0);
    if (lz == NULL) {
        Py_DECREF(ittuple);
        Py_DECREF(result);
        return NULL;
    }
    lz->ittuple = ittuple;
    lz->tuplesize = tuplesize;
    lz->result = result;
    lz->strict = strict;

    return (PyObject *)lz;
}

static void
zip_dealloc(zipobject *lz)
{
    PyObject_GC_UnTrack(lz);
    Py_XDECREF(lz->ittuple);
    Py_XDECREF(lz->result);
    Py_TYPE(lz)->tp_free(lz);
}

static int
zip_traverse(zipobject *lz, visitproc visit, void *arg)
{
    Py_VISIT(lz->ittuple);
    Py_VISIT(lz->result);
    return 0;
}

static PyObject *
zip_next(zipobject *lz)
{
    Py_ssize_t i;
    Py_ssize_t tuplesize = lz->tuplesize;
    PyObject *result = lz->result;
    PyObject *it;
    PyObject *item;
    PyObject *olditem;

    if (tuplesize == 0)
        return NULL;
    if (Py_REFCNT(result) == 1) {
        Py_INCREF(result);
        for (i=0 ; i < tuplesize ; i++) {
            it = PyTuple_GET_ITEM(lz->ittuple, i);
            item = (*Py_TYPE(it)->tp_iternext)(it);
            if (item == NULL) {
                Py_DECREF(result);
                if (lz->strict) {
                    goto check;
                }
                return NULL;
            }
            olditem = PyTuple_GET_ITEM(result, i);
            PyTuple_SET_ITEM(result, i, item);
            Py_DECREF(olditem);
        }
        // bpo-42536: The GC may have untracked this result tuple. Since we're
        // recycling it, make sure it's tracked again:
        if (!_PyObject_GC_IS_TRACKED(result)) {
            _PyObject_GC_TRACK(result);
        }
    } else {
        result = PyTuple_New(tuplesize);
        if (result == NULL)
            return NULL;
        for (i=0 ; i < tuplesize ; i++) {
            it = PyTuple_GET_ITEM(lz->ittuple, i);
            item = (*Py_TYPE(it)->tp_iternext)(it);
            if (item == NULL) {
                Py_DECREF(result);
                if (lz->strict) {
                    goto check;
                }
                return NULL;
            }
            PyTuple_SET_ITEM(result, i, item);
        }
    }
    return result;
check:
    if (PyErr_Occurred()) {
        if (!PyErr_ExceptionMatches(PyExc_StopIteration)) {
            // next() on argument i raised an exception (not StopIteration)
            return NULL;
        }
        PyErr_Clear();
    }
    if (i) {
        // ValueError: zip() argument 2 is shorter than argument 1
        // ValueError: zip() argument 3 is shorter than arguments 1-2
        const char* plural = i == 1 ? " " : "s 1-";
        return PyErr_Format(PyExc_ValueError,
                            "zip() argument %d is shorter than argument%s%d",
                            i + 1, plural, i);
    }
    for (i = 1; i < tuplesize; i++) {
        it = PyTuple_GET_ITEM(lz->ittuple, i);
        item = (*Py_TYPE(it)->tp_iternext)(it);
        if (item) {
            Py_DECREF(item);
            const char* plural = i == 1 ? " " : "s 1-";
            return PyErr_Format(PyExc_ValueError,
                                "zip() argument %d is longer than argument%s%d",
                                i + 1, plural, i);
        }
        if (PyErr_Occurred()) {
            if (!PyErr_ExceptionMatches(PyExc_StopIteration)) {
                // next() on argument i raised an exception (not StopIteration)
                return NULL;
            }
            PyErr_Clear();
        }
        // Argument i is exhausted. So far so good...
    }
    // All arguments are exhausted. Success!
    return NULL;
}

static PyObject *
zip_reduce(zipobject *lz, PyObject *Py_UNUSED(ignored))
{
    /* Just recreate the zip with the internal iterator tuple */
    if (lz->strict) {
        return PyTuple_Pack(3, Py_TYPE(lz), lz->ittuple, Py_True);
    }
    return PyTuple_Pack(2, Py_TYPE(lz), lz->ittuple);
}

PyDoc_STRVAR(setstate_doc, "Set state information for unpickling.");

static PyObject *
zip_setstate(zipobject *lz, PyObject *state)
{
    int strict = PyObject_IsTrue(state);
    if (strict < 0) {
        return NULL;
    }
    lz->strict = strict;
    Py_RETURN_NONE;
}

static PyMethodDef zip_methods[] = {
    {"__reduce__",   (PyCFunction)zip_reduce,   METH_NOARGS, reduce_doc},
    {"__setstate__", (PyCFunction)zip_setstate, METH_O,      setstate_doc},
    {NULL}  /* sentinel */
};

PyDoc_STRVAR(zip_doc,
"zip(*iterables, strict=False) --> Yield tuples until an input is exhausted.\n\
\n\
   >>> list(zip('abcdefg', range(3), range(4)))\n\
   [('a', 0, 0), ('b', 1, 1), ('c', 2, 2)]\n\
\n\
The zip object yields n-length tuples, where n is the number of iterables\n\
passed as positional arguments to zip().  The i-th element in every tuple\n\
comes from the i-th iterable argument to zip().  This continues until the\n\
shortest argument is exhausted.\n\
\n\
If strict is true and one of the arguments is exhausted before the others,\n\
raise a ValueError.");

PyTypeObject PyZip_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "zip",                              /* tp_name */
    sizeof(zipobject),                  /* tp_basicsize */
    0,                                  /* tp_itemsize */
    /* methods */
    (destructor)zip_dealloc,            /* tp_dealloc */
    0,                                  /* tp_vectorcall_offset */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_as_async */
    0,                                  /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    0,                                  /* tp_hash */
    0,                                  /* tp_call */
    0,                                  /* tp_str */
    PyObject_GenericGetAttr,            /* tp_getattro */
    0,                                  /* tp_setattro */
    0,                                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
        Py_TPFLAGS_BASETYPE,            /* tp_flags */
    zip_doc,                            /* tp_doc */
    (traverseproc)zip_traverse,    /* tp_traverse */
    0,                                  /* tp_clear */
    0,                                  /* tp_richcompare */
    0,                                  /* tp_weaklistoffset */
    PyObject_SelfIter,                  /* tp_iter */
    (iternextfunc)zip_next,     /* tp_iternext */
    zip_methods,                        /* tp_methods */
    0,                                  /* tp_members */
    0,                                  /* tp_getset */
    0,                                  /* tp_base */
    0,                                  /* tp_dict */
    0,                                  /* tp_descr_get */
    0,                                  /* tp_descr_set */
    0,                                  /* tp_dictoffset */
    0,                                  /* tp_init */
    PyType_GenericAlloc,                /* tp_alloc */
    zip_new,                            /* tp_new */
    PyObject_GC_Del,                    /* tp_free */
};


static PyMethodDef builtin_methods[] = {
    {"__build_class__", (PyCFunction)(void(*)(void))builtin___build_class__,
     METH_FASTCALL | METH_KEYWORDS, build_class_doc},
    {"__import__",      (PyCFunction)(void(*)(void))builtin___import__, METH_VARARGS | METH_KEYWORDS, import_doc},
    BUILTIN_ABS_METHODDEF
    BUILTIN_ALL_METHODDEF
    BUILTIN_ANY_METHODDEF
    BUILTIN_ASCII_METHODDEF
    BUILTIN_BIN_METHODDEF
    {"breakpoint",      (PyCFunction)(void(*)(void))builtin_breakpoint, METH_FASTCALL | METH_KEYWORDS, breakpoint_doc},
    BUILTIN_CALLABLE_METHODDEF
    BUILTIN_CHR_METHODDEF
    BUILTIN_COMPILE_METHODDEF
    BUILTIN_DELATTR_METHODDEF
    {"dir",             builtin_dir,        METH_VARARGS, dir_doc},
    BUILTIN_DIVMOD_METHODDEF
    BUILTIN_EVAL_METHODDEF
    BUILTIN_EXEC_METHODDEF
    BUILTIN_FORMAT_METHODDEF
    {"getattr",         (PyCFunction)(void(*)(void))builtin_getattr, METH_FASTCALL, getattr_doc},
    BUILTIN_GLOBALS_METHODDEF
    BUILTIN_HASATTR_METHODDEF
    BUILTIN_HASH_METHODDEF
    BUILTIN_HEX_METHODDEF
    BUILTIN_ID_METHODDEF
    BUILTIN_INPUT_METHODDEF
    BUILTIN_ISINSTANCE_METHODDEF
    BUILTIN_ISSUBCLASS_METHODDEF
    {"iter",            (PyCFunction)(void(*)(void))builtin_iter,       METH_FASTCALL, iter_doc},
    BUILTIN_AITER_METHODDEF
    BUILTIN_LEN_METHODDEF
    BUILTIN_LOCALS_METHODDEF
    {"max",             (PyCFunction)(void(*)(void))builtin_max,        METH_VARARGS | METH_KEYWORDS, max_doc},
    {"min",             (PyCFunction)(void(*)(void))builtin_min,        METH_VARARGS | METH_KEYWORDS, min_doc},
    {"next",            (PyCFunction)(void(*)(void))builtin_next,       METH_FASTCALL, next_doc},
    BUILTIN_ANEXT_METHODDEF
    BUILTIN_OCT_METHODDEF
    BUILTIN_ORD_METHODDEF
    BUILTIN_POW_METHODDEF
    BUILTIN_PRINT_METHODDEF
    BUILTIN_REPR_METHODDEF
    BUILTIN_ROUND_METHODDEF
    BUILTIN_SETATTR_METHODDEF
    BUILTIN_SORTED_METHODDEF
    BUILTIN_SUM_METHODDEF
    {"vars",            builtin_vars,       METH_VARARGS, vars_doc},
    {NULL,              NULL},
};

PyDoc_STRVAR(builtin_doc,
"Built-in functions, exceptions, and other objects.\n\
\n\
Noteworthy: None is the `nil' object; Ellipsis represents `...' in slices.");

static struct PyModuleDef builtinsmodule = {
    PyModuleDef_HEAD_INIT,
    "builtins",
    builtin_doc,
    -1, /* multiple "initialization" just copies the module dict. */
    builtin_methods,
    NULL,
    NULL,
    NULL,
    NULL
};


PyObject *
_PyBuiltin_Init(PyInterpreterState *interp)
{
    PyObject *mod, *dict, *debug;

    const PyConfig *config = _PyInterpreterState_GetConfig(interp);

    if (PyType_Ready(&PyFilter_Type) < 0 ||
        PyType_Ready(&PyMap_Type) < 0 ||
        PyType_Ready(&PyZip_Type) < 0)
        return NULL;

    mod = _PyModule_CreateInitialized(&builtinsmodule, PYTHON_API_VERSION);
    if (mod == NULL)
        return NULL;
    dict = PyModule_GetDict(mod);

#ifdef Py_TRACE_REFS
    /* "builtins" exposes a number of statically allocated objects
     * that, before this code was added in 2.3, never showed up in
     * the list of "all objects" maintained by Py_TRACE_REFS.  As a
     * result, programs leaking references to None and False (etc)
     * couldn't be diagnosed by examining sys.getobjects(0).
     */
#define ADD_TO_ALL(OBJECT) _Py_AddToAllObjects((PyObject *)(OBJECT), 0)
#else
#define ADD_TO_ALL(OBJECT) (void)0
#endif

#define SETBUILTIN(NAME, OBJECT) \
    if (PyDict_SetItemString(dict, NAME, (PyObject *)OBJECT) < 0)       \
        return NULL;                                                    \
    ADD_TO_ALL(OBJECT)

    SETBUILTIN("None",                  Py_None);
    SETBUILTIN("Ellipsis",              Py_Ellipsis);
    SETBUILTIN("NotImplemented",        Py_NotImplemented);
    SETBUILTIN("False",                 Py_False);
    SETBUILTIN("True",                  Py_True);
    SETBUILTIN("bool",                  &PyBool_Type);
    SETBUILTIN("memoryview",        &PyMemoryView_Type);
    SETBUILTIN("bytearray",             &PyByteArray_Type);
    SETBUILTIN("bytes",                 &PyBytes_Type);
    SETBUILTIN("classmethod",           &PyClassMethod_Type);
    SETBUILTIN("complex",               &PyComplex_Type);
    SETBUILTIN("dict",                  &PyDict_Type);
    SETBUILTIN("enumerate",             &PyEnum_Type);
    SETBUILTIN("filter",                &PyFilter_Type);
    SETBUILTIN("float",                 &PyFloat_Type);
    SETBUILTIN("frozenset",             &PyFrozenSet_Type);
    SETBUILTIN("property",              &PyProperty_Type);
    SETBUILTIN("int",                   &PyLong_Type);
    SETBUILTIN("list",                  &PyList_Type);
    SETBUILTIN("map",                   &PyMap_Type);
    SETBUILTIN("object",                &PyBaseObject_Type);
    SETBUILTIN("range",                 &PyRange_Type);
    SETBUILTIN("reversed",              &PyReversed_Type);
    SETBUILTIN("set",                   &PySet_Type);
    SETBUILTIN("slice",                 &PySlice_Type);
    SETBUILTIN("staticmethod",          &PyStaticMethod_Type);
    SETBUILTIN("str",                   &PyUnicode_Type);
    SETBUILTIN("super",                 &PySuper_Type);
    SETBUILTIN("tuple",                 &PyTuple_Type);
    SETBUILTIN("type",                  &PyType_Type);
    SETBUILTIN("zip",                   &PyZip_Type);
    debug = PyBool_FromLong(config->optimization_level == 0);
    if (PyDict_SetItemString(dict, "__debug__", debug) < 0) {
        Py_DECREF(debug);
        return NULL;
    }
    Py_DECREF(debug);

    return mod;
#undef ADD_TO_ALL
#undef SETBUILTIN
}
