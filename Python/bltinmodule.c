/* Built-in functions */

#include "Python.h"
#include "Python-ast.h"

#include "node.h"
#include "code.h"

#include "asdl.h"
#include "ast.h"

#include <ctype.h>

#ifdef HAVE_LANGINFO_H
#include <langinfo.h>   /* CODESET */
#endif

/* The default encoding used by the platform file system APIs
   Can remain NULL for all platforms that don't have such a concept

   Don't forget to modify PyUnicode_DecodeFSDefault() if you touch any of the
   values for Py_FileSystemDefaultEncoding!
*/
#ifdef HAVE_MBCS
const char *Py_FileSystemDefaultEncoding = "mbcs";
int Py_HasFileSystemDefaultEncoding = 1;
#elif defined(__APPLE__)
const char *Py_FileSystemDefaultEncoding = "utf-8";
int Py_HasFileSystemDefaultEncoding = 1;
#else
const char *Py_FileSystemDefaultEncoding = NULL; /* set by initfsencoding() */
int Py_HasFileSystemDefaultEncoding = 0;
#endif

_Py_IDENTIFIER(__builtins__);
_Py_IDENTIFIER(__dict__);
_Py_IDENTIFIER(__prepare__);
_Py_IDENTIFIER(__round__);
_Py_IDENTIFIER(encoding);
_Py_IDENTIFIER(errors);
_Py_IDENTIFIER(fileno);
_Py_IDENTIFIER(flush);
_Py_IDENTIFIER(metaclass);
_Py_IDENTIFIER(sort);
_Py_IDENTIFIER(stdin);
_Py_IDENTIFIER(stdout);
_Py_IDENTIFIER(stderr);

/* AC: cannot convert yet, waiting for *args support */
static PyObject *
builtin___build_class__(PyObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *func, *name, *bases, *mkw, *meta, *winner, *prep, *ns, *cell;
    PyObject *cls = NULL;
    Py_ssize_t nargs;
    int isclass;

    assert(args != NULL);
    if (!PyTuple_Check(args)) {
        PyErr_SetString(PyExc_TypeError,
                        "__build_class__: args is not a tuple");
        return NULL;
    }
    nargs = PyTuple_GET_SIZE(args);
    if (nargs < 2) {
        PyErr_SetString(PyExc_TypeError,
                        "__build_class__: not enough arguments");
        return NULL;
    }
    func = PyTuple_GET_ITEM(args, 0); /* Better be callable */
    if (!PyFunction_Check(func)) {
        PyErr_SetString(PyExc_TypeError,
                        "__build_class__: func must be a function");
        return NULL;
    }
    name = PyTuple_GET_ITEM(args, 1);
    if (!PyUnicode_Check(name)) {
        PyErr_SetString(PyExc_TypeError,
                        "__build_class__: name is not a string");
        return NULL;
    }
    bases = PyTuple_GetSlice(args, 2, nargs);
    if (bases == NULL)
        return NULL;

    if (kwds == NULL) {
        meta = NULL;
        mkw = NULL;
    }
    else {
        mkw = PyDict_Copy(kwds); /* Don't modify kwds passed in! */
        if (mkw == NULL) {
            Py_DECREF(bases);
            return NULL;
        }
        meta = _PyDict_GetItemId(mkw, &PyId_metaclass);
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
    }
    if (meta == NULL) {
        /* if there are no bases, use type: */
        if (PyTuple_GET_SIZE(bases) == 0) {
            meta = (PyObject *) (&PyType_Type);
        }
        /* else get the type of the first base */
        else {
            PyObject *base0 = PyTuple_GET_ITEM(bases, 0);
            meta = (PyObject *) (base0->ob_type);
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
    prep = _PyObject_GetAttrId(meta, &PyId___prepare__);
    if (prep == NULL) {
        if (PyErr_ExceptionMatches(PyExc_AttributeError)) {
            PyErr_Clear();
            ns = PyDict_New();
        }
        else {
            Py_DECREF(meta);
            Py_XDECREF(mkw);
            Py_DECREF(bases);
            return NULL;
        }
    }
    else {
        PyObject *pargs = PyTuple_Pack(2, name, bases);
        if (pargs == NULL) {
            Py_DECREF(prep);
            Py_DECREF(meta);
            Py_XDECREF(mkw);
            Py_DECREF(bases);
            return NULL;
        }
        ns = PyEval_CallObjectWithKeywords(prep, pargs, mkw);
        Py_DECREF(pargs);
        Py_DECREF(prep);
    }
    if (ns == NULL) {
        Py_DECREF(meta);
        Py_XDECREF(mkw);
        Py_DECREF(bases);
        return NULL;
    }
    cell = PyEval_EvalCodeEx(PyFunction_GET_CODE(func), PyFunction_GET_GLOBALS(func), ns,
                             NULL, 0, NULL, 0, NULL, 0, NULL,
                             PyFunction_GET_CLOSURE(func));
    if (cell != NULL) {
        PyObject *margs;
        margs = PyTuple_Pack(3, name, bases, ns);
        if (margs != NULL) {
            cls = PyEval_CallObjectWithKeywords(meta, margs, mkw);
            Py_DECREF(margs);
        }
        if (cls != NULL && PyCell_Check(cell))
            PyCell_Set(cell, cls);
        Py_DECREF(cell);
    }
    Py_DECREF(ns);
    Py_DECREF(meta);
    Py_XDECREF(mkw);
    Py_DECREF(bases);
    return cls;
}

PyDoc_STRVAR(build_class_doc,
"__build_class__(func, name, *bases, metaclass=None, **kwds) -> class\n\
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
interpreter and not for general use it is better to use\n\
importlib.import_module() to programmatically import a module.\n\
\n\
The globals argument is only used to determine the context;\n\
they are not modified.  The locals argument is unused.  The fromlist\n\
should be a list of names to emulate ``from name import ...'', or an\n\
empty list to emulate ``import name''.\n\
When importing a module from a package, note that __import__('A.B', ...)\n\
returns package A when fromlist is empty, but its submodule B when\n\
fromlist is not empty.  Level is used to determine whether to perform \n\
absolute or relative imports. 0 is absolute while a positive number\n\
is the number of parent directories to search relative to the current module.");


/*[clinic input]
abs as builtin_abs

    x: 'O'
    /

Return the absolute value of the argument.
[clinic start generated code]*/

PyDoc_STRVAR(builtin_abs__doc__,
"abs($module, x, /)\n"
"--\n"
"\n"
"Return the absolute value of the argument.");

#define BUILTIN_ABS_METHODDEF    \
    {"abs", (PyCFunction)builtin_abs, METH_O, builtin_abs__doc__},

static PyObject *
builtin_abs(PyModuleDef *module, PyObject *x)
/*[clinic end generated code: output=f85095528ce7e2e5 input=aa29cc07869b4732]*/
{
    return PyNumber_Absolute(x);
}

/*[clinic input]
all as builtin_all

    iterable: 'O'
    /

Return True if bool(x) is True for all values x in the iterable.

If the iterable is empty, return True.
[clinic start generated code]*/

PyDoc_STRVAR(builtin_all__doc__,
"all($module, iterable, /)\n"
"--\n"
"\n"
"Return True if bool(x) is True for all values x in the iterable.\n"
"\n"
"If the iterable is empty, return True.");

#define BUILTIN_ALL_METHODDEF    \
    {"all", (PyCFunction)builtin_all, METH_O, builtin_all__doc__},

static PyObject *
builtin_all(PyModuleDef *module, PyObject *iterable)
/*[clinic end generated code: output=d001db739ba83b46 input=dd506dc9998d42bd]*/
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

    iterable: 'O'
    /

Return True if bool(x) is True for any x in the iterable.

If the iterable is empty, return False.
[clinic start generated code]*/

PyDoc_STRVAR(builtin_any__doc__,
"any($module, iterable, /)\n"
"--\n"
"\n"
"Return True if bool(x) is True for any x in the iterable.\n"
"\n"
"If the iterable is empty, return False.");

#define BUILTIN_ANY_METHODDEF    \
    {"any", (PyCFunction)builtin_any, METH_O, builtin_any__doc__},

static PyObject *
builtin_any(PyModuleDef *module, PyObject *iterable)
/*[clinic end generated code: output=3a4b6dbe6a0d6f61 input=8fe8460f3fbbced8]*/
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
        if (cmp == 1) {
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

    obj: 'O'
    /

Return an ASCII-only representation of an object.

As repr(), return a string containing a printable representation of an
object, but escape the non-ASCII characters in the string returned by
repr() using \\x, \\u or \\U escapes. This generates a string similar
to that returned by repr() in Python 2.
[clinic start generated code]*/

PyDoc_STRVAR(builtin_ascii__doc__,
"ascii($module, obj, /)\n"
"--\n"
"\n"
"Return an ASCII-only representation of an object.\n"
"\n"
"As repr(), return a string containing a printable representation of an\n"
"object, but escape the non-ASCII characters in the string returned by\n"
"repr() using \\\\x, \\\\u or \\\\U escapes. This generates a string similar\n"
"to that returned by repr() in Python 2.");

#define BUILTIN_ASCII_METHODDEF    \
    {"ascii", (PyCFunction)builtin_ascii, METH_O, builtin_ascii__doc__},

static PyObject *
builtin_ascii(PyModuleDef *module, PyObject *obj)
/*[clinic end generated code: output=f0e6754154c2d30b input=0cbdc1420a306325]*/
{
    return PyObject_ASCII(obj);
}


/*[clinic input]
bin as builtin_bin

    number: 'O'
    /

Return the binary representation of an integer.

   >>> bin(2796202)
   '0b1010101010101010101010'
[clinic start generated code]*/

PyDoc_STRVAR(builtin_bin__doc__,
"bin($module, number, /)\n"
"--\n"
"\n"
"Return the binary representation of an integer.\n"
"\n"
"   >>> bin(2796202)\n"
"   \'0b1010101010101010101010\'");

#define BUILTIN_BIN_METHODDEF    \
    {"bin", (PyCFunction)builtin_bin, METH_O, builtin_bin__doc__},

static PyObject *
builtin_bin(PyModuleDef *module, PyObject *number)
/*[clinic end generated code: output=18fed0e943650da1 input=2a6362ae9a9c9203]*/
{
    return PyNumber_ToBase(number, 2);
}


/*[clinic input]
callable as builtin_callable

    obj: 'O'
    /

Return whether the object is callable (i.e., some kind of function).

Note that classes are callable, as are instances of classes with a
__call__() method.
[clinic start generated code]*/

PyDoc_STRVAR(builtin_callable__doc__,
"callable($module, obj, /)\n"
"--\n"
"\n"
"Return whether the object is callable (i.e., some kind of function).\n"
"\n"
"Note that classes are callable, as are instances of classes with a\n"
"__call__() method.");

#define BUILTIN_CALLABLE_METHODDEF    \
    {"callable", (PyCFunction)builtin_callable, METH_O, builtin_callable__doc__},

static PyObject *
builtin_callable(PyModuleDef *module, PyObject *obj)
/*[clinic end generated code: output=b3a92cbe635f32af input=bb3bb528fffdade4]*/
{
    return PyBool_FromLong((long)PyCallable_Check(obj));
}


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

    if (type == &PyFilter_Type && !_PyArg_NoKeywords("filter()", kwds))
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
    Py_INCREF(func);
    lz->func = func;
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

    iternext = *Py_TYPE(it)->tp_iternext;
    for (;;) {
        item = iternext(it);
        if (item == NULL)
            return NULL;

        if (lz->func == Py_None || lz->func == (PyObject *)&PyBool_Type) {
            ok = PyObject_IsTrue(item);
        } else {
            PyObject *good;
            good = PyObject_CallFunctionObjArgs(lz->func,
                                                item, NULL);
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
filter_reduce(filterobject *lz)
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
    0,                                  /* tp_print */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_reserved */
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
};


/*[clinic input]
format as builtin_format

    value: 'O'
    format_spec: unicode(c_default="NULL") = ''
    /

Return value.__format__(format_spec)

format_spec defaults to the empty string
[clinic start generated code]*/

PyDoc_STRVAR(builtin_format__doc__,
"format($module, value, format_spec=\'\', /)\n"
"--\n"
"\n"
"Return value.__format__(format_spec)\n"
"\n"
"format_spec defaults to the empty string");

#define BUILTIN_FORMAT_METHODDEF    \
    {"format", (PyCFunction)builtin_format, METH_VARARGS, builtin_format__doc__},

static PyObject *
builtin_format_impl(PyModuleDef *module, PyObject *value, PyObject *format_spec);

static PyObject *
builtin_format(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *value;
    PyObject *format_spec = NULL;

    if (!PyArg_ParseTuple(args,
        "O|U:format",
        &value, &format_spec))
        goto exit;
    return_value = builtin_format_impl(module, value, format_spec);

exit:
    return return_value;
}

static PyObject *
builtin_format_impl(PyModuleDef *module, PyObject *value, PyObject *format_spec)
/*[clinic end generated code: output=39723a58c72e8871 input=e23f2f11e0098c64]*/
{
    return PyObject_Format(value, format_spec);
}

/*[clinic input]
chr as builtin_chr

    i: 'i'
    /

Return a Unicode string of one character with ordinal i; 0 <= i <= 0x10ffff.
[clinic start generated code]*/

PyDoc_STRVAR(builtin_chr__doc__,
"chr($module, i, /)\n"
"--\n"
"\n"
"Return a Unicode string of one character with ordinal i; 0 <= i <= 0x10ffff.");

#define BUILTIN_CHR_METHODDEF    \
    {"chr", (PyCFunction)builtin_chr, METH_VARARGS, builtin_chr__doc__},

static PyObject *
builtin_chr_impl(PyModuleDef *module, int i);

static PyObject *
builtin_chr(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    int i;

    if (!PyArg_ParseTuple(args,
        "i:chr",
        &i))
        goto exit;
    return_value = builtin_chr_impl(module, i);

exit:
    return return_value;
}

static PyObject *
builtin_chr_impl(PyModuleDef *module, int i)
/*[clinic end generated code: output=4d6bbe948f56e2ae input=9b1ced29615adf66]*/
{
    return PyUnicode_FromOrdinal(i);
}


static const char *
source_as_string(PyObject *cmd, const char *funcname, const char *what, PyCompilerFlags *cf, Py_buffer *view)
{
    const char *str;
    Py_ssize_t size;

    if (PyUnicode_Check(cmd)) {
        cf->cf_flags |= PyCF_IGNORE_COOKIE;
        str = PyUnicode_AsUTF8AndSize(cmd, &size);
        if (str == NULL)
            return NULL;
    }
    else if (PyObject_GetBuffer(cmd, view, PyBUF_SIMPLE) == 0) {
        str = (const char *)view->buf;
        size = view->len;
    }
    else {
        PyErr_Format(PyExc_TypeError,
          "%s() arg 1 must be a %s object",
          funcname, what);
        return NULL;
    }

    if (strlen(str) != (size_t)size) {
        PyErr_SetString(PyExc_ValueError,
                        "source code string cannot contain null bytes");
        PyBuffer_Release(view);
        return NULL;
    }
    return str;
}

/*[clinic input]
compile as builtin_compile

    source: 'O'
    filename: object(converter="PyUnicode_FSDecoder")
    mode: 's'
    flags: 'i' = 0
    dont_inherit: 'i' = 0
    optimize: 'i' = -1

Compile source into a code object that can be executed by exec() or eval().

The source code may represent a Python module, statement or expression.
The filename will be used for run-time error messages.
The mode must be 'exec' to compile a module, 'single' to compile a
single (interactive) statement, or 'eval' to compile an expression.
The flags argument, if present, controls which future statements influence
the compilation of the code.
The dont_inherit argument, if non-zero, stops the compilation inheriting
the effects of any future statements in effect in the code calling
compile; if absent or zero these statements do influence the compilation,
in addition to any features explicitly specified.
[clinic start generated code]*/

PyDoc_STRVAR(builtin_compile__doc__,
"compile($module, /, source, filename, mode, flags=0, dont_inherit=0,\n"
"        optimize=-1)\n"
"--\n"
"\n"
"Compile source into a code object that can be executed by exec() or eval().\n"
"\n"
"The source code may represent a Python module, statement or expression.\n"
"The filename will be used for run-time error messages.\n"
"The mode must be \'exec\' to compile a module, \'single\' to compile a\n"
"single (interactive) statement, or \'eval\' to compile an expression.\n"
"The flags argument, if present, controls which future statements influence\n"
"the compilation of the code.\n"
"The dont_inherit argument, if non-zero, stops the compilation inheriting\n"
"the effects of any future statements in effect in the code calling\n"
"compile; if absent or zero these statements do influence the compilation,\n"
"in addition to any features explicitly specified.");

#define BUILTIN_COMPILE_METHODDEF    \
    {"compile", (PyCFunction)builtin_compile, METH_VARARGS|METH_KEYWORDS, builtin_compile__doc__},

static PyObject *
builtin_compile_impl(PyModuleDef *module, PyObject *source, PyObject *filename, const char *mode, int flags, int dont_inherit, int optimize);

static PyObject *
builtin_compile(PyModuleDef *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"source", "filename", "mode", "flags", "dont_inherit", "optimize", NULL};
    PyObject *source;
    PyObject *filename;
    const char *mode;
    int flags = 0;
    int dont_inherit = 0;
    int optimize = -1;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
        "OO&s|iii:compile", _keywords,
        &source, PyUnicode_FSDecoder, &filename, &mode, &flags, &dont_inherit, &optimize))
        goto exit;
    return_value = builtin_compile_impl(module, source, filename, mode, flags, dont_inherit, optimize);

exit:
    return return_value;
}

static PyObject *
builtin_compile_impl(PyModuleDef *module, PyObject *source, PyObject *filename, const char *mode, int flags, int dont_inherit, int optimize)
/*[clinic end generated code: output=c72d197809d178fc input=c6212a9d21472f7e]*/
{
    Py_buffer view = {NULL, NULL};
    const char *str;
    int compile_mode = -1;
    int is_ast;
    PyCompilerFlags cf;
    int start[] = {Py_file_input, Py_eval_input, Py_single_input};
    PyObject *result;

    cf.cf_flags = flags | PyCF_SOURCE_IS_UTF8;

    if (flags &
        ~(PyCF_MASK | PyCF_MASK_OBSOLETE | PyCF_DONT_IMPLY_DEDENT | PyCF_ONLY_AST))
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
    else {
        PyErr_SetString(PyExc_ValueError,
                        "compile() mode must be 'exec', 'eval' or 'single'");
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

            arena = PyArena_New();
            if (arena == NULL)
                goto error;
            mod = PyAST_obj2mod(source, arena, compile_mode);
            if (mod == NULL) {
                PyArena_Free(arena);
                goto error;
            }
            if (!PyAST_Validate(mod)) {
                PyArena_Free(arena);
                goto error;
            }
            result = (PyObject*)PyAST_CompileObject(mod, filename,
                                                    &cf, optimize, arena);
            PyArena_Free(arena);
        }
        goto finally;
    }

    str = source_as_string(source, "compile", "string, bytes or AST", &cf, &view);
    if (str == NULL)
        goto error;

    result = Py_CompileStringObject(str, filename, start[compile_mode], &cf, optimize);
    PyBuffer_Release(&view);
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

    x: 'O'
    y: 'O'
    /

Return the tuple ((x-x%y)/y, x%y).  Invariant: div*y + mod == x.
[clinic start generated code]*/

PyDoc_STRVAR(builtin_divmod__doc__,
"divmod($module, x, y, /)\n"
"--\n"
"\n"
"Return the tuple ((x-x%y)/y, x%y).  Invariant: div*y + mod == x.");

#define BUILTIN_DIVMOD_METHODDEF    \
    {"divmod", (PyCFunction)builtin_divmod, METH_VARARGS, builtin_divmod__doc__},

static PyObject *
builtin_divmod_impl(PyModuleDef *module, PyObject *x, PyObject *y);

static PyObject *
builtin_divmod(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *x;
    PyObject *y;

    if (!PyArg_UnpackTuple(args, "divmod",
        2, 2,
        &x, &y))
        goto exit;
    return_value = builtin_divmod_impl(module, x, y);

exit:
    return return_value;
}

static PyObject *
builtin_divmod_impl(PyModuleDef *module, PyObject *x, PyObject *y)
/*[clinic end generated code: output=77e8d408b1338886 input=c9c617b7bb74c615]*/
{
    return PyNumber_Divmod(x, y);
}


/*[clinic input]
eval as builtin_eval

    source: 'O'
    globals: 'O' = None
    locals: 'O' = None
    /

Evaluate the given source in the context of globals and locals.

The source may be a string representing a Python expression
or a code object as returned by compile().
The globals must be a dictionary and locals can be any mapping,
defaulting to the current globals and locals.
If only globals is given, locals defaults to it.
[clinic start generated code]*/

PyDoc_STRVAR(builtin_eval__doc__,
"eval($module, source, globals=None, locals=None, /)\n"
"--\n"
"\n"
"Evaluate the given source in the context of globals and locals.\n"
"\n"
"The source may be a string representing a Python expression\n"
"or a code object as returned by compile().\n"
"The globals must be a dictionary and locals can be any mapping,\n"
"defaulting to the current globals and locals.\n"
"If only globals is given, locals defaults to it.");

#define BUILTIN_EVAL_METHODDEF    \
    {"eval", (PyCFunction)builtin_eval, METH_VARARGS, builtin_eval__doc__},

static PyObject *
builtin_eval_impl(PyModuleDef *module, PyObject *source, PyObject *globals, PyObject *locals);

static PyObject *
builtin_eval(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *source;
    PyObject *globals = Py_None;
    PyObject *locals = Py_None;

    if (!PyArg_UnpackTuple(args, "eval",
        1, 3,
        &source, &globals, &locals))
        goto exit;
    return_value = builtin_eval_impl(module, source, globals, locals);

exit:
    return return_value;
}

static PyObject *
builtin_eval_impl(PyModuleDef *module, PyObject *source, PyObject *globals, PyObject *locals)
/*[clinic end generated code: output=644fd59012538ce6 input=31e42c1d2125b50b]*/
{
    PyObject *result, *tmp = NULL;
    Py_buffer view = {NULL, NULL};
    const char *str;
    PyCompilerFlags cf;

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

    if (_PyDict_GetItemId(globals, &PyId___builtins__) == NULL) {
        if (_PyDict_SetItemId(globals, &PyId___builtins__,
                              PyEval_GetBuiltins()) != 0)
            return NULL;
    }

    if (PyCode_Check(source)) {
        if (PyCode_GetNumFree((PyCodeObject *)source) > 0) {
            PyErr_SetString(PyExc_TypeError,
        "code object passed to eval() may not contain free variables");
            return NULL;
        }
        return PyEval_EvalCode(source, globals, locals);
    }

    cf.cf_flags = PyCF_SOURCE_IS_UTF8;
    str = source_as_string(source, "eval", "string, bytes or code", &cf, &view);
    if (str == NULL)
        return NULL;

    while (*str == ' ' || *str == '\t')
        str++;

    (void)PyEval_MergeCompilerFlags(&cf);
    result = PyRun_StringFlags(str, Py_eval_input, globals, locals, &cf);
    PyBuffer_Release(&view);
    Py_XDECREF(tmp);
    return result;
}

/*[clinic input]
exec as builtin_exec

    source: 'O'
    globals: 'O' = None
    locals: 'O' = None
    /

Execute the given source in the context of globals and locals.

The source may be a string representing one or more Python statements
or a code object as returned by compile().
The globals must be a dictionary and locals can be any mapping,
defaulting to the current globals and locals.
If only globals is given, locals defaults to it.
[clinic start generated code]*/

PyDoc_STRVAR(builtin_exec__doc__,
"exec($module, source, globals=None, locals=None, /)\n"
"--\n"
"\n"
"Execute the given source in the context of globals and locals.\n"
"\n"
"The source may be a string representing one or more Python statements\n"
"or a code object as returned by compile().\n"
"The globals must be a dictionary and locals can be any mapping,\n"
"defaulting to the current globals and locals.\n"
"If only globals is given, locals defaults to it.");

#define BUILTIN_EXEC_METHODDEF    \
    {"exec", (PyCFunction)builtin_exec, METH_VARARGS, builtin_exec__doc__},

static PyObject *
builtin_exec_impl(PyModuleDef *module, PyObject *source, PyObject *globals, PyObject *locals);

static PyObject *
builtin_exec(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *source;
    PyObject *globals = Py_None;
    PyObject *locals = Py_None;

    if (!PyArg_UnpackTuple(args, "exec",
        1, 3,
        &source, &globals, &locals))
        goto exit;
    return_value = builtin_exec_impl(module, source, globals, locals);

exit:
    return return_value;
}

static PyObject *
builtin_exec_impl(PyModuleDef *module, PyObject *source, PyObject *globals, PyObject *locals)
/*[clinic end generated code: output=0281b48bfa8e3c87 input=536e057b5e00d89e]*/
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
                     globals->ob_type->tp_name);
        return NULL;
    }
    if (!PyMapping_Check(locals)) {
        PyErr_Format(PyExc_TypeError,
            "locals must be a mapping or None, not %.100s",
            locals->ob_type->tp_name);
        return NULL;
    }
    if (_PyDict_GetItemId(globals, &PyId___builtins__) == NULL) {
        if (_PyDict_SetItemId(globals, &PyId___builtins__,
                              PyEval_GetBuiltins()) != 0)
            return NULL;
    }

    if (PyCode_Check(source)) {
        if (PyCode_GetNumFree((PyCodeObject *)source) > 0) {
            PyErr_SetString(PyExc_TypeError,
                "code object passed to exec() may not "
                "contain free variables");
            return NULL;
        }
        v = PyEval_EvalCode(source, globals, locals);
    }
    else {
        Py_buffer view = {NULL, NULL};
        const char *str;
        PyCompilerFlags cf;
        cf.cf_flags = PyCF_SOURCE_IS_UTF8;
        str = source_as_string(source, "exec",
                                       "string, bytes or code", &cf, &view);
        if (str == NULL)
            return NULL;
        if (PyEval_MergeCompilerFlags(&cf))
            v = PyRun_StringFlags(str, Py_file_input, globals,
                                  locals, &cf);
        else
            v = PyRun_String(str, Py_file_input, globals, locals);
        PyBuffer_Release(&view);
    }
    if (v == NULL)
        return NULL;
    Py_DECREF(v);
    Py_RETURN_NONE;
}


/* AC: cannot convert yet, as needs PEP 457 group support in inspect */
static PyObject *
builtin_getattr(PyObject *self, PyObject *args)
{
    PyObject *v, *result, *dflt = NULL;
    PyObject *name;

    if (!PyArg_UnpackTuple(args, "getattr", 2, 3, &v, &name, &dflt))
        return NULL;

    if (!PyUnicode_Check(name)) {
        PyErr_SetString(PyExc_TypeError,
                        "getattr(): attribute name must be string");
        return NULL;
    }
    result = PyObject_GetAttr(v, name);
    if (result == NULL && dflt != NULL &&
        PyErr_ExceptionMatches(PyExc_AttributeError))
    {
        PyErr_Clear();
        Py_INCREF(dflt);
        result = dflt;
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

PyDoc_STRVAR(builtin_globals__doc__,
"globals($module, /)\n"
"--\n"
"\n"
"Return the dictionary containing the current scope\'s global variables.\n"
"\n"
"NOTE: Updates to this dictionary *will* affect name lookups in the current\n"
"global scope and vice-versa.");

#define BUILTIN_GLOBALS_METHODDEF    \
    {"globals", (PyCFunction)builtin_globals, METH_NOARGS, builtin_globals__doc__},

static PyObject *
builtin_globals_impl(PyModuleDef *module);

static PyObject *
builtin_globals(PyModuleDef *module, PyObject *Py_UNUSED(ignored))
{
    return builtin_globals_impl(module);
}

static PyObject *
builtin_globals_impl(PyModuleDef *module)
/*[clinic end generated code: output=048640f58b1f20ad input=9327576f92bb48ba]*/
{
    PyObject *d;

    d = PyEval_GetGlobals();
    Py_XINCREF(d);
    return d;
}


/*[clinic input]
hasattr as builtin_hasattr

    obj: 'O'
    name: 'O'
    /

Return whether the object has an attribute with the given name.

This is done by calling getattr(obj, name) and catching AttributeError.
[clinic start generated code]*/

PyDoc_STRVAR(builtin_hasattr__doc__,
"hasattr($module, obj, name, /)\n"
"--\n"
"\n"
"Return whether the object has an attribute with the given name.\n"
"\n"
"This is done by calling getattr(obj, name) and catching AttributeError.");

#define BUILTIN_HASATTR_METHODDEF    \
    {"hasattr", (PyCFunction)builtin_hasattr, METH_VARARGS, builtin_hasattr__doc__},

static PyObject *
builtin_hasattr_impl(PyModuleDef *module, PyObject *obj, PyObject *name);

static PyObject *
builtin_hasattr(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *obj;
    PyObject *name;

    if (!PyArg_UnpackTuple(args, "hasattr",
        2, 2,
        &obj, &name))
        goto exit;
    return_value = builtin_hasattr_impl(module, obj, name);

exit:
    return return_value;
}

static PyObject *
builtin_hasattr_impl(PyModuleDef *module, PyObject *obj, PyObject *name)
/*[clinic end generated code: output=e0bd996ef73d1217 input=b50bad5f739ea10d]*/
{
    PyObject *v;

    if (!PyUnicode_Check(name)) {
        PyErr_SetString(PyExc_TypeError,
                        "hasattr(): attribute name must be string");
        return NULL;
    }
    v = PyObject_GetAttr(obj, name);
    if (v == NULL) {
        if (PyErr_ExceptionMatches(PyExc_AttributeError)) {
            PyErr_Clear();
            Py_RETURN_FALSE;
        }
        return NULL;
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
    obj as v: 'O'
    /

Return the identity of an object.

This is guaranteed to be unique among simultaneously existing objects.
(CPython uses the object's memory address.)
[clinic start generated code]*/

PyDoc_STRVAR(builtin_id__doc__,
"id($module, obj, /)\n"
"--\n"
"\n"
"Return the identity of an object.\n"
"\n"
"This is guaranteed to be unique among simultaneously existing objects.\n"
"(CPython uses the object\'s memory address.)");

#define BUILTIN_ID_METHODDEF    \
    {"id", (PyCFunction)builtin_id, METH_O, builtin_id__doc__},

static PyObject *
builtin_id(PyModuleDef *self, PyObject *v)
/*[clinic end generated code: output=f54da09c91992e63 input=a1f988d98357341d]*/
{
    return PyLong_FromVoidPtr(v);
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

    if (type == &PyMap_Type && !_PyArg_NoKeywords("map()", kwds))
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
    Py_INCREF(func);
    lz->func = func;

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
    PyObject *val;
    PyObject *argtuple;
    PyObject *result;
    Py_ssize_t numargs, i;

    numargs = PyTuple_Size(lz->iters);
    argtuple = PyTuple_New(numargs);
    if (argtuple == NULL)
        return NULL;

    for (i=0 ; i<numargs ; i++) {
        val = PyIter_Next(PyTuple_GET_ITEM(lz->iters, i));
        if (val == NULL) {
            Py_DECREF(argtuple);
            return NULL;
        }
        PyTuple_SET_ITEM(argtuple, i, val);
    }
    result = PyObject_Call(lz->func, argtuple, NULL);
    Py_DECREF(argtuple);
    return result;
}

static PyObject *
map_reduce(mapobject *lz)
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
    0,                                  /* tp_print */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_reserved */
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
};


/* AC: cannot convert yet, as needs PEP 457 group support in inspect */
static PyObject *
builtin_next(PyObject *self, PyObject *args)
{
    PyObject *it, *res;
    PyObject *def = NULL;

    if (!PyArg_UnpackTuple(args, "next", 1, 2, &it, &def))
        return NULL;
    if (!PyIter_Check(it)) {
        PyErr_Format(PyExc_TypeError,
            "'%.200s' object is not an iterator",
            it->ob_type->tp_name);
        return NULL;
    }

    res = (*it->ob_type->tp_iternext)(it);
    if (res != NULL) {
        return res;
    } else if (def != NULL) {
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

    obj: 'O'
    name: 'O'
    value: 'O'
    /

Sets the named attribute on the given object to the specified value.

setattr(x, 'y', v) is equivalent to ``x.y = v''
[clinic start generated code]*/

PyDoc_STRVAR(builtin_setattr__doc__,
"setattr($module, obj, name, value, /)\n"
"--\n"
"\n"
"Sets the named attribute on the given object to the specified value.\n"
"\n"
"setattr(x, \'y\', v) is equivalent to ``x.y = v\'\'");

#define BUILTIN_SETATTR_METHODDEF    \
    {"setattr", (PyCFunction)builtin_setattr, METH_VARARGS, builtin_setattr__doc__},

static PyObject *
builtin_setattr_impl(PyModuleDef *module, PyObject *obj, PyObject *name, PyObject *value);

static PyObject *
builtin_setattr(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *obj;
    PyObject *name;
    PyObject *value;

    if (!PyArg_UnpackTuple(args, "setattr",
        3, 3,
        &obj, &name, &value))
        goto exit;
    return_value = builtin_setattr_impl(module, obj, name, value);

exit:
    return return_value;
}

static PyObject *
builtin_setattr_impl(PyModuleDef *module, PyObject *obj, PyObject *name, PyObject *value)
/*[clinic end generated code: output=4336dcbbf7691d2d input=fbe7e53403116b93]*/
{
    if (PyObject_SetAttr(obj, name, value) != 0)
        return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}


/*[clinic input]
delattr as builtin_delattr

    obj: 'O'
    name: 'O'
    /

Deletes the named attribute from the given object.

delattr(x, 'y') is equivalent to ``del x.y''
[clinic start generated code]*/

PyDoc_STRVAR(builtin_delattr__doc__,
"delattr($module, obj, name, /)\n"
"--\n"
"\n"
"Deletes the named attribute from the given object.\n"
"\n"
"delattr(x, \'y\') is equivalent to ``del x.y\'\'");

#define BUILTIN_DELATTR_METHODDEF    \
    {"delattr", (PyCFunction)builtin_delattr, METH_VARARGS, builtin_delattr__doc__},

static PyObject *
builtin_delattr_impl(PyModuleDef *module, PyObject *obj, PyObject *name);

static PyObject *
builtin_delattr(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *obj;
    PyObject *name;

    if (!PyArg_UnpackTuple(args, "delattr",
        2, 2,
        &obj, &name))
        goto exit;
    return_value = builtin_delattr_impl(module, obj, name);

exit:
    return return_value;
}

static PyObject *
builtin_delattr_impl(PyModuleDef *module, PyObject *obj, PyObject *name)
/*[clinic end generated code: output=319c2d884aa769cf input=647af2ce9183a823]*/
{
    if (PyObject_SetAttr(obj, name, (PyObject *)NULL) != 0)
        return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}


/*[clinic input]
hash as builtin_hash

    obj: 'O'
    /

Return the hash value for the given object.

Two objects that compare equal must also have the same hash value, but the
reverse is not necessarily true.
[clinic start generated code]*/

PyDoc_STRVAR(builtin_hash__doc__,
"hash($module, obj, /)\n"
"--\n"
"\n"
"Return the hash value for the given object.\n"
"\n"
"Two objects that compare equal must also have the same hash value, but the\n"
"reverse is not necessarily true.");

#define BUILTIN_HASH_METHODDEF    \
    {"hash", (PyCFunction)builtin_hash, METH_O, builtin_hash__doc__},

static PyObject *
builtin_hash(PyModuleDef *module, PyObject *obj)
/*[clinic end generated code: output=1ec467611c13468b input=ccc4d2b9a351df4e]*/
{
    Py_hash_t x;

    x = PyObject_Hash(obj);
    if (x == -1)
        return NULL;
    return PyLong_FromSsize_t(x);
}


/*[clinic input]
hex as builtin_hex

    number: 'O'
    /

Return the hexadecimal representation of an integer.

   >>> hex(12648430)
   '0xc0ffee'
[clinic start generated code]*/

PyDoc_STRVAR(builtin_hex__doc__,
"hex($module, number, /)\n"
"--\n"
"\n"
"Return the hexadecimal representation of an integer.\n"
"\n"
"   >>> hex(12648430)\n"
"   \'0xc0ffee\'");

#define BUILTIN_HEX_METHODDEF    \
    {"hex", (PyCFunction)builtin_hex, METH_O, builtin_hex__doc__},

static PyObject *
builtin_hex(PyModuleDef *module, PyObject *number)
/*[clinic end generated code: output=f18e9439aeaa2c6c input=e816200b0a728ebe]*/
{
    return PyNumber_ToBase(number, 16);
}


/* AC: cannot convert yet, as needs PEP 457 group support in inspect */
static PyObject *
builtin_iter(PyObject *self, PyObject *args)
{
    PyObject *v, *w = NULL;

    if (!PyArg_UnpackTuple(args, "iter", 1, 2, &v, &w))
        return NULL;
    if (w == NULL)
        return PyObject_GetIter(v);
    if (!PyCallable_Check(v)) {
        PyErr_SetString(PyExc_TypeError,
                        "iter(v, w): v must be callable");
        return NULL;
    }
    return PyCallIter_New(v, w);
}

PyDoc_STRVAR(iter_doc,
"iter(iterable) -> iterator\n\
iter(callable, sentinel) -> iterator\n\
\n\
Get an iterator from an object.  In the first form, the argument must\n\
supply its own iterator, or be a sequence.\n\
In the second form, the callable is called until it returns the sentinel.");


/*[clinic input]
len as builtin_len

    obj: 'O'
    /

Return the number of items in a container.
[clinic start generated code]*/

PyDoc_STRVAR(builtin_len__doc__,
"len($module, obj, /)\n"
"--\n"
"\n"
"Return the number of items in a container.");

#define BUILTIN_LEN_METHODDEF    \
    {"len", (PyCFunction)builtin_len, METH_O, builtin_len__doc__},

static PyObject *
builtin_len(PyModuleDef *module, PyObject *obj)
/*[clinic end generated code: output=5a38b0db40761705 input=2e5ff15db9a2de22]*/
{
    Py_ssize_t res;

    res = PyObject_Size(obj);
    if (res < 0 && PyErr_Occurred())
        return NULL;
    return PyLong_FromSsize_t(res);
}


/*[clinic input]
locals as builtin_locals

Return a dictionary containing the current scope's local variables.

NOTE: Whether or not updates to this dictionary will affect name lookups in
the local scope and vice-versa is *implementation dependent* and not
covered by any backwards compatibility guarantees.
[clinic start generated code]*/

PyDoc_STRVAR(builtin_locals__doc__,
"locals($module, /)\n"
"--\n"
"\n"
"Return a dictionary containing the current scope\'s local variables.\n"
"\n"
"NOTE: Whether or not updates to this dictionary will affect name lookups in\n"
"the local scope and vice-versa is *implementation dependent* and not\n"
"covered by any backwards compatibility guarantees.");

#define BUILTIN_LOCALS_METHODDEF    \
    {"locals", (PyCFunction)builtin_locals, METH_NOARGS, builtin_locals__doc__},

static PyObject *
builtin_locals_impl(PyModuleDef *module);

static PyObject *
builtin_locals(PyModuleDef *module, PyObject *Py_UNUSED(ignored))
{
    return builtin_locals_impl(module);
}

static PyObject *
builtin_locals_impl(PyModuleDef *module)
/*[clinic end generated code: output=8ac52522924346e2 input=7874018d478d5c4b]*/
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

    if (positional)
        v = args;
    else if (!PyArg_UnpackTuple(args, name, 1, 1, &v))
        return NULL;

    emptytuple = PyTuple_New(0);
    if (emptytuple == NULL)
        return NULL;
    ret = PyArg_ParseTupleAndKeywords(emptytuple, kwds, "|$OO", kwlist,
                                      &keyfunc, &defaultval);
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

    maxitem = NULL; /* the result */
    maxval = NULL;  /* the value associated with the result */
    while (( item = PyIter_Next(it) )) {
        /* get the value from the key function */
        if (keyfunc != NULL) {
            val = PyObject_CallFunctionObjArgs(keyfunc, item, NULL);
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

    number: 'O'
    /

Return the octal representation of an integer.

   >>> oct(342391)
   '0o1234567'
[clinic start generated code]*/

PyDoc_STRVAR(builtin_oct__doc__,
"oct($module, number, /)\n"
"--\n"
"\n"
"Return the octal representation of an integer.\n"
"\n"
"   >>> oct(342391)\n"
"   \'0o1234567\'");

#define BUILTIN_OCT_METHODDEF    \
    {"oct", (PyCFunction)builtin_oct, METH_O, builtin_oct__doc__},

static PyObject *
builtin_oct(PyModuleDef *module, PyObject *number)
/*[clinic end generated code: output=b99234d1d70a6673 input=a3a372b521b3dd13]*/
{
    return PyNumber_ToBase(number, 8);
}


/*[clinic input]
ord as builtin_ord

    c: 'O'
    /

Return the Unicode code point for a one-character string.
[clinic start generated code]*/

PyDoc_STRVAR(builtin_ord__doc__,
"ord($module, c, /)\n"
"--\n"
"\n"
"Return the Unicode code point for a one-character string.");

#define BUILTIN_ORD_METHODDEF    \
    {"ord", (PyCFunction)builtin_ord, METH_O, builtin_ord__doc__},

static PyObject *
builtin_ord(PyModuleDef *module, PyObject *c)
/*[clinic end generated code: output=a8466d23bd76db3f input=762355f87451efa3]*/
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
                     "%.200s found", c->ob_type->tp_name);
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

    x: 'O'
    y: 'O'
    z: 'O' = None
    /

Equivalent to x**y (with two arguments) or x**y % z (with three arguments)

Some types, such as ints, are able to use a more efficient algorithm when
invoked using the three argument form.
[clinic start generated code]*/

PyDoc_STRVAR(builtin_pow__doc__,
"pow($module, x, y, z=None, /)\n"
"--\n"
"\n"
"Equivalent to x**y (with two arguments) or x**y % z (with three arguments)\n"
"\n"
"Some types, such as ints, are able to use a more efficient algorithm when\n"
"invoked using the three argument form.");

#define BUILTIN_POW_METHODDEF    \
    {"pow", (PyCFunction)builtin_pow, METH_VARARGS, builtin_pow__doc__},

static PyObject *
builtin_pow_impl(PyModuleDef *module, PyObject *x, PyObject *y, PyObject *z);

static PyObject *
builtin_pow(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *x;
    PyObject *y;
    PyObject *z = Py_None;

    if (!PyArg_UnpackTuple(args, "pow",
        2, 3,
        &x, &y, &z))
        goto exit;
    return_value = builtin_pow_impl(module, x, y, z);

exit:
    return return_value;
}

static PyObject *
builtin_pow_impl(PyModuleDef *module, PyObject *x, PyObject *y, PyObject *z)
/*[clinic end generated code: output=d0cdf314311dedba input=561a942d5f5c1899]*/
{
    return PyNumber_Power(x, y, z);
}


/* AC: cannot convert yet, waiting for *args support */
static PyObject *
builtin_print(PyObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"sep", "end", "file", "flush", 0};
    static PyObject *dummy_args;
    PyObject *sep = NULL, *end = NULL, *file = NULL, *flush = NULL;
    int i, err;

    if (dummy_args == NULL && !(dummy_args = PyTuple_New(0)))
        return NULL;
    if (!PyArg_ParseTupleAndKeywords(dummy_args, kwds, "|OOOO:print",
                                     kwlist, &sep, &end, &file, &flush))
        return NULL;
    if (file == NULL || file == Py_None) {
        file = _PySys_GetObjectId(&PyId_stdout);
        if (file == NULL) {
            PyErr_SetString(PyExc_RuntimeError, "lost sys.stdout");
            return NULL;
        }

        /* sys.stdout may be None when FILE* stdout isn't connected */
        if (file == Py_None)
            Py_RETURN_NONE;
    }

    if (sep == Py_None) {
        sep = NULL;
    }
    else if (sep && !PyUnicode_Check(sep)) {
        PyErr_Format(PyExc_TypeError,
                     "sep must be None or a string, not %.200s",
                     sep->ob_type->tp_name);
        return NULL;
    }
    if (end == Py_None) {
        end = NULL;
    }
    else if (end && !PyUnicode_Check(end)) {
        PyErr_Format(PyExc_TypeError,
                     "end must be None or a string, not %.200s",
                     end->ob_type->tp_name);
        return NULL;
    }

    for (i = 0; i < PyTuple_Size(args); i++) {
        if (i > 0) {
            if (sep == NULL)
                err = PyFile_WriteString(" ", file);
            else
                err = PyFile_WriteObject(sep, file,
                                         Py_PRINT_RAW);
            if (err)
                return NULL;
        }
        err = PyFile_WriteObject(PyTuple_GetItem(args, i), file,
                                 Py_PRINT_RAW);
        if (err)
            return NULL;
    }

    if (end == NULL)
        err = PyFile_WriteString("\n", file);
    else
        err = PyFile_WriteObject(end, file, Py_PRINT_RAW);
    if (err)
        return NULL;

    if (flush != NULL) {
        PyObject *tmp;
        int do_flush = PyObject_IsTrue(flush);
        if (do_flush == -1)
            return NULL;
        else if (do_flush) {
            tmp = _PyObject_CallMethodId(file, &PyId_flush, "");
            if (tmp == NULL)
                return NULL;
            else
                Py_DECREF(tmp);
        }
    }

    Py_RETURN_NONE;
}

PyDoc_STRVAR(print_doc,
"print(value, ..., sep=' ', end='\\n', file=sys.stdout, flush=False)\n\
\n\
Prints the values to a stream, or to sys.stdout by default.\n\
Optional keyword arguments:\n\
file:  a file-like object (stream); defaults to the current sys.stdout.\n\
sep:   string inserted between values, default a space.\n\
end:   string appended after the last value, default a newline.\n\
flush: whether to forcibly flush the stream.");


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

PyDoc_STRVAR(builtin_input__doc__,
"input($module, prompt=None, /)\n"
"--\n"
"\n"
"Read a string from standard input.  The trailing newline is stripped.\n"
"\n"
"The prompt string, if given, is printed to standard output without a\n"
"trailing newline before reading input.\n"
"\n"
"If the user hits EOF (*nix: Ctrl-D, Windows: Ctrl-Z+Return), raise EOFError.\n"
"On *nix systems, readline is used if available.");

#define BUILTIN_INPUT_METHODDEF    \
    {"input", (PyCFunction)builtin_input, METH_VARARGS, builtin_input__doc__},

static PyObject *
builtin_input_impl(PyModuleDef *module, PyObject *prompt);

static PyObject *
builtin_input(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *prompt = NULL;

    if (!PyArg_UnpackTuple(args, "input",
        0, 1,
        &prompt))
        goto exit;
    return_value = builtin_input_impl(module, prompt);

exit:
    return return_value;
}

static PyObject *
builtin_input_impl(PyModuleDef *module, PyObject *prompt)
/*[clinic end generated code: output=69323bf5695f7c9c input=5e8bb70c2908fe3c]*/
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

    /* First of all, flush stderr */
    tmp = _PyObject_CallMethodId(ferr, &PyId_flush, "");
    if (tmp == NULL)
        PyErr_Clear();
    else
        Py_DECREF(tmp);

    /* We should only use (GNU) readline if Python's sys.stdin and
       sys.stdout are the same as C's stdin and stdout, because we
       need to pass it those. */
    tmp = _PyObject_CallMethodId(fin, &PyId_fileno, "");
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
        tmp = _PyObject_CallMethodId(fout, &PyId_fileno, "");
        if (tmp == NULL)
            PyErr_Clear();
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
        char *promptstr;
        char *s = NULL;
        PyObject *stdin_encoding = NULL, *stdin_errors = NULL;
        PyObject *stdout_encoding = NULL, *stdout_errors = NULL;
        char *stdin_encoding_str, *stdin_errors_str;
        PyObject *result;
        size_t len;

        stdin_encoding = _PyObject_GetAttrId(fin, &PyId_encoding);
        stdin_errors = _PyObject_GetAttrId(fin, &PyId_errors);
        if (!stdin_encoding || !stdin_errors)
            /* stdin is a text stream, so it must have an
               encoding. */
            goto _readline_errors;
        stdin_encoding_str = _PyUnicode_AsString(stdin_encoding);
        stdin_errors_str = _PyUnicode_AsString(stdin_errors);
        if (!stdin_encoding_str || !stdin_errors_str)
            goto _readline_errors;
        tmp = _PyObject_CallMethodId(fout, &PyId_flush, "");
        if (tmp == NULL)
            PyErr_Clear();
        else
            Py_DECREF(tmp);
        if (prompt != NULL) {
            /* We have a prompt, encode it as stdout would */
            char *stdout_encoding_str, *stdout_errors_str;
            PyObject *stringpo;
            stdout_encoding = _PyObject_GetAttrId(fout, &PyId_encoding);
            stdout_errors = _PyObject_GetAttrId(fout, &PyId_errors);
            if (!stdout_encoding || !stdout_errors)
                goto _readline_errors;
            stdout_encoding_str = _PyUnicode_AsString(stdout_encoding);
            stdout_errors_str = _PyUnicode_AsString(stdout_errors);
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
            promptstr = PyBytes_AsString(po);
            if (promptstr == NULL)
                goto _readline_errors;
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
        PyMem_FREE(s);
        return result;
    _readline_errors:
        Py_XDECREF(stdin_encoding);
        Py_XDECREF(stdout_encoding);
        Py_XDECREF(stdin_errors);
        Py_XDECREF(stdout_errors);
        Py_XDECREF(po);
        return NULL;
    }

    /* Fallback if we're not interactive */
    if (prompt != NULL) {
        if (PyFile_WriteObject(prompt, fout, Py_PRINT_RAW) != 0)
            return NULL;
    }
    tmp = _PyObject_CallMethodId(fout, &PyId_flush, "");
    if (tmp == NULL)
        PyErr_Clear();
    else
        Py_DECREF(tmp);
    return PyFile_GetLine(fin, -1);
}


/*[clinic input]
repr as builtin_repr

    obj: 'O'
    /

Return the canonical string representation of the object.

For many object types, including most builtins, eval(repr(obj)) == obj.
[clinic start generated code]*/

PyDoc_STRVAR(builtin_repr__doc__,
"repr($module, obj, /)\n"
"--\n"
"\n"
"Return the canonical string representation of the object.\n"
"\n"
"For many object types, including most builtins, eval(repr(obj)) == obj.");

#define BUILTIN_REPR_METHODDEF    \
    {"repr", (PyCFunction)builtin_repr, METH_O, builtin_repr__doc__},

static PyObject *
builtin_repr(PyModuleDef *module, PyObject *obj)
/*[clinic end generated code: output=988980120f39e2fa input=a2bca0f38a5a924d]*/
{
    return PyObject_Repr(obj);
}


/* AC: cannot convert yet, as needs PEP 457 group support in inspect
 *     or a semantic change to accept None for "ndigits"
 */
static PyObject *
builtin_round(PyObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *ndigits = NULL;
    static char *kwlist[] = {"number", "ndigits", 0};
    PyObject *number, *round, *result;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|O:round",
                                     kwlist, &number, &ndigits))
        return NULL;

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

    if (ndigits == NULL)
        result = PyObject_CallFunctionObjArgs(round, NULL);
    else
        result = PyObject_CallFunctionObjArgs(round, ndigits, NULL);
    Py_DECREF(round);
    return result;
}

PyDoc_STRVAR(round_doc,
"round(number[, ndigits]) -> number\n\
\n\
Round a number to a given precision in decimal digits (default 0 digits).\n\
This returns an int when called with one argument, otherwise the\n\
same type as the number. ndigits may be negative.");


/*AC: we need to keep the kwds dict intact to easily call into the
 * list.sort method, which isn't currently supported in AC. So we just use
 * the initially generated signature with a custom implementation.
 */
/* [disabled clinic input]
sorted as builtin_sorted

    iterable as seq: 'O'
    key as keyfunc: 'O' = None
    reverse: 'O' = False

Return a new list containing all items from the iterable in ascending order.

A custom key function can be supplied to customise the sort order, and the
reverse flag can be set to request the result in descending order.
[end disabled clinic input]*/

PyDoc_STRVAR(builtin_sorted__doc__,
"sorted($module, iterable, key=None, reverse=False)\n"
"--\n"
"\n"
"Return a new list containing all items from the iterable in ascending order.\n"
"\n"
"A custom key function can be supplied to customise the sort order, and the\n"
"reverse flag can be set to request the result in descending order.");

#define BUILTIN_SORTED_METHODDEF    \
    {"sorted", (PyCFunction)builtin_sorted, METH_VARARGS|METH_KEYWORDS, builtin_sorted__doc__},

static PyObject *
builtin_sorted(PyObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *newlist, *v, *seq, *keyfunc=NULL, *newargs;
    PyObject *callable;
    static char *kwlist[] = {"iterable", "key", "reverse", 0};
    int reverse;

    /* args 1-3 should match listsort in Objects/listobject.c */
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|Oi:sorted",
        kwlist, &seq, &keyfunc, &reverse))
        return NULL;

    newlist = PySequence_List(seq);
    if (newlist == NULL)
        return NULL;

    callable = _PyObject_GetAttrId(newlist, &PyId_sort);
    if (callable == NULL) {
        Py_DECREF(newlist);
        return NULL;
    }

    newargs = PyTuple_GetSlice(args, 1, 4);
    if (newargs == NULL) {
        Py_DECREF(newlist);
        Py_DECREF(callable);
        return NULL;
    }

    v = PyObject_Call(callable, newargs, kwds);
    Py_DECREF(newargs);
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
        if (d == NULL)
            return NULL;
        Py_INCREF(d);
    }
    else {
        d = _PyObject_GetAttrId(v, &PyId___dict__);
        if (d == NULL) {
            PyErr_SetString(PyExc_TypeError,
                "vars() argument must have __dict__ attribute");
            return NULL;
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

    iterable: 'O'
    start: object(c_default="NULL") = 0
    /

Return the sum of a 'start' value (default: 0) plus an iterable of numbers

When the iterable is empty, return the start value.
This function is intended specifically for use with numeric values and may
reject non-numeric types.
[clinic start generated code]*/

PyDoc_STRVAR(builtin_sum__doc__,
"sum($module, iterable, start=0, /)\n"
"--\n"
"\n"
"Return the sum of a \'start\' value (default: 0) plus an iterable of numbers\n"
"\n"
"When the iterable is empty, return the start value.\n"
"This function is intended specifically for use with numeric values and may\n"
"reject non-numeric types.");

#define BUILTIN_SUM_METHODDEF    \
    {"sum", (PyCFunction)builtin_sum, METH_VARARGS, builtin_sum__doc__},

static PyObject *
builtin_sum_impl(PyModuleDef *module, PyObject *iterable, PyObject *start);

static PyObject *
builtin_sum(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *iterable;
    PyObject *start = NULL;

    if (!PyArg_UnpackTuple(args, "sum",
        1, 2,
        &iterable, &start))
        goto exit;
    return_value = builtin_sum_impl(module, iterable, start);

exit:
    return return_value;
}

static PyObject *
builtin_sum_impl(PyModuleDef *module, PyObject *iterable, PyObject *start)
/*[clinic end generated code: output=b42652a0d5f64f6b input=90ae7a242cfcf025]*/
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
            if (PyLong_CheckExact(item)) {
                long b = PyLong_AsLongAndOverflow(item, &overflow);
                long x = i_result + b;
                if (overflow == 0 && ((x^i_result) >= 0 || (x^b) >= 0)) {
                    i_result = x;
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
                PyFPE_START_PROTECT("add", Py_DECREF(item); Py_DECREF(iter); return 0)
                f_result += PyFloat_AS_DOUBLE(item);
                PyFPE_END_PROTECT(f_result)
                Py_DECREF(item);
                continue;
            }
            if (PyLong_CheckExact(item)) {
                long value;
                int overflow;
                value = PyLong_AsLongAndOverflow(item, &overflow);
                if (!overflow) {
                    PyFPE_START_PROTECT("add", Py_DECREF(item); Py_DECREF(iter); return 0)
                    f_result += (double)value;
                    PyFPE_END_PROTECT(f_result)
                    Py_DECREF(item);
                    continue;
                }
            }
            result = PyFloat_FromDouble(f_result);
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

           would change the value of empty. */
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

    obj: 'O'
    class_or_tuple: 'O'
    /

Return whether an object is an instance of a class or of a subclass thereof.

A tuple, as in ``isinstance(x, (A, B, ...))``, may be given as the target to
check against. This is equivalent to ``isinstance(x, A) or isinstance(x, B)
or ...`` etc.
[clinic start generated code]*/

PyDoc_STRVAR(builtin_isinstance__doc__,
"isinstance($module, obj, class_or_tuple, /)\n"
"--\n"
"\n"
"Return whether an object is an instance of a class or of a subclass thereof.\n"
"\n"
"A tuple, as in ``isinstance(x, (A, B, ...))``, may be given as the target to\n"
"check against. This is equivalent to ``isinstance(x, A) or isinstance(x, B)\n"
"or ...`` etc.");

#define BUILTIN_ISINSTANCE_METHODDEF    \
    {"isinstance", (PyCFunction)builtin_isinstance, METH_VARARGS, builtin_isinstance__doc__},

static PyObject *
builtin_isinstance_impl(PyModuleDef *module, PyObject *obj, PyObject *class_or_tuple);

static PyObject *
builtin_isinstance(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *obj;
    PyObject *class_or_tuple;

    if (!PyArg_UnpackTuple(args, "isinstance",
        2, 2,
        &obj, &class_or_tuple))
        goto exit;
    return_value = builtin_isinstance_impl(module, obj, class_or_tuple);

exit:
    return return_value;
}

static PyObject *
builtin_isinstance_impl(PyModuleDef *module, PyObject *obj, PyObject *class_or_tuple)
/*[clinic end generated code: output=847df57fef8ddea7 input=cf9eb0ad6bb9bad6]*/
{
    int retval;

    retval = PyObject_IsInstance(obj, class_or_tuple);
    if (retval < 0)
        return NULL;
    return PyBool_FromLong(retval);
}


/*[clinic input]
issubclass as builtin_issubclass

    cls: 'O'
    class_or_tuple: 'O'
    /

Return whether 'cls' is a derived from another class or is the same class.

A tuple, as in ``issubclass(x, (A, B, ...))``, may be given as the target to
check against. This is equivalent to ``issubclass(x, A) or issubclass(x, B)
or ...`` etc.
[clinic start generated code]*/

PyDoc_STRVAR(builtin_issubclass__doc__,
"issubclass($module, cls, class_or_tuple, /)\n"
"--\n"
"\n"
"Return whether \'cls\' is a derived from another class or is the same class.\n"
"\n"
"A tuple, as in ``issubclass(x, (A, B, ...))``, may be given as the target to\n"
"check against. This is equivalent to ``issubclass(x, A) or issubclass(x, B)\n"
"or ...`` etc.");

#define BUILTIN_ISSUBCLASS_METHODDEF    \
    {"issubclass", (PyCFunction)builtin_issubclass, METH_VARARGS, builtin_issubclass__doc__},

static PyObject *
builtin_issubclass_impl(PyModuleDef *module, PyObject *cls, PyObject *class_or_tuple);

static PyObject *
builtin_issubclass(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *cls;
    PyObject *class_or_tuple;

    if (!PyArg_UnpackTuple(args, "issubclass",
        2, 2,
        &cls, &class_or_tuple))
        goto exit;
    return_value = builtin_issubclass_impl(module, cls, class_or_tuple);

exit:
    return return_value;
}

static PyObject *
builtin_issubclass_impl(PyModuleDef *module, PyObject *cls, PyObject *class_or_tuple)
/*[clinic end generated code: output=a0f8c03692e35474 input=923d03fa41fc352a]*/
{
    int retval;

    retval = PyObject_IsSubclass(cls, class_or_tuple);
    if (retval < 0)
        return NULL;
    return PyBool_FromLong(retval);
}


typedef struct {
    PyObject_HEAD
    Py_ssize_t          tuplesize;
    PyObject *ittuple;                  /* tuple of iterators */
    PyObject *result;
} zipobject;

static PyObject *
zip_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    zipobject *lz;
    Py_ssize_t i;
    PyObject *ittuple;  /* tuple of iterators */
    PyObject *result;
    Py_ssize_t tuplesize = PySequence_Length(args);

    if (type == &PyZip_Type && !_PyArg_NoKeywords("zip()", kwds))
        return NULL;

    /* args must be a tuple */
    assert(PyTuple_Check(args));

    /* obtain iterators */
    ittuple = PyTuple_New(tuplesize);
    if (ittuple == NULL)
        return NULL;
    for (i=0; i < tuplesize; ++i) {
        PyObject *item = PyTuple_GET_ITEM(args, i);
        PyObject *it = PyObject_GetIter(item);
        if (it == NULL) {
            if (PyErr_ExceptionMatches(PyExc_TypeError))
                PyErr_Format(PyExc_TypeError,
                    "zip argument #%zd must support iteration",
                    i+1);
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
                return NULL;
            }
            olditem = PyTuple_GET_ITEM(result, i);
            PyTuple_SET_ITEM(result, i, item);
            Py_DECREF(olditem);
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
                return NULL;
            }
            PyTuple_SET_ITEM(result, i, item);
        }
    }
    return result;
}

static PyObject *
zip_reduce(zipobject *lz)
{
    /* Just recreate the zip with the internal iterator tuple */
    return Py_BuildValue("OO", Py_TYPE(lz), lz->ittuple);
}

static PyMethodDef zip_methods[] = {
    {"__reduce__",   (PyCFunction)zip_reduce,   METH_NOARGS, reduce_doc},
    {NULL,           NULL}           /* sentinel */
};

PyDoc_STRVAR(zip_doc,
"zip(iter1 [,iter2 [...]]) --> zip object\n\
\n\
Return a zip object whose .__next__() method returns a tuple where\n\
the i-th element comes from the i-th iterable argument.  The .__next__()\n\
method continues until the shortest iterable in the argument sequence\n\
is exhausted and then it raises StopIteration.");

PyTypeObject PyZip_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "zip",                              /* tp_name */
    sizeof(zipobject),                  /* tp_basicsize */
    0,                                  /* tp_itemsize */
    /* methods */
    (destructor)zip_dealloc,            /* tp_dealloc */
    0,                                  /* tp_print */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_reserved */
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
    {"__build_class__", (PyCFunction)builtin___build_class__,
     METH_VARARGS | METH_KEYWORDS, build_class_doc},
    {"__import__",      (PyCFunction)builtin___import__, METH_VARARGS | METH_KEYWORDS, import_doc},
    BUILTIN_ABS_METHODDEF
    BUILTIN_ALL_METHODDEF
    BUILTIN_ANY_METHODDEF
    BUILTIN_ASCII_METHODDEF
    BUILTIN_BIN_METHODDEF
    BUILTIN_CALLABLE_METHODDEF
    BUILTIN_CHR_METHODDEF
    BUILTIN_COMPILE_METHODDEF
    BUILTIN_DELATTR_METHODDEF
    {"dir",             builtin_dir,        METH_VARARGS, dir_doc},
    BUILTIN_DIVMOD_METHODDEF
    BUILTIN_EVAL_METHODDEF
    BUILTIN_EXEC_METHODDEF
    BUILTIN_FORMAT_METHODDEF
    {"getattr",         builtin_getattr,    METH_VARARGS, getattr_doc},
    BUILTIN_GLOBALS_METHODDEF
    BUILTIN_HASATTR_METHODDEF
    BUILTIN_HASH_METHODDEF
    BUILTIN_HEX_METHODDEF
    BUILTIN_ID_METHODDEF
    BUILTIN_INPUT_METHODDEF
    BUILTIN_ISINSTANCE_METHODDEF
    BUILTIN_ISSUBCLASS_METHODDEF
    {"iter",            builtin_iter,       METH_VARARGS, iter_doc},
    BUILTIN_LEN_METHODDEF
    BUILTIN_LOCALS_METHODDEF
    {"max",             (PyCFunction)builtin_max,        METH_VARARGS | METH_KEYWORDS, max_doc},
    {"min",             (PyCFunction)builtin_min,        METH_VARARGS | METH_KEYWORDS, min_doc},
    {"next",            (PyCFunction)builtin_next,       METH_VARARGS, next_doc},
    BUILTIN_OCT_METHODDEF
    BUILTIN_ORD_METHODDEF
    BUILTIN_POW_METHODDEF
    {"print",           (PyCFunction)builtin_print,      METH_VARARGS | METH_KEYWORDS, print_doc},
    BUILTIN_REPR_METHODDEF
    {"round",           (PyCFunction)builtin_round,      METH_VARARGS | METH_KEYWORDS, round_doc},
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
_PyBuiltin_Init(void)
{
    PyObject *mod, *dict, *debug;

    if (PyType_Ready(&PyFilter_Type) < 0 ||
        PyType_Ready(&PyMap_Type) < 0 ||
        PyType_Ready(&PyZip_Type) < 0)
        return NULL;

    mod = PyModule_Create(&builtinsmodule);
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
    debug = PyBool_FromLong(Py_OptimizeFlag == 0);
    if (PyDict_SetItemString(dict, "__debug__", debug) < 0) {
        Py_XDECREF(debug);
        return NULL;
    }
    Py_XDECREF(debug);

    return mod;
#undef ADD_TO_ALL
#undef SETBUILTIN
}
