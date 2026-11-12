/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#endif
#include "pycore_critical_section.h"// Py_BEGIN_CRITICAL_SECTION()
#include "pycore_modsupport.h"    // _PyArg_BadArgument()
#include "pycore_runtime.h"       // _Py_SINGLETON()

static PyObject *
BaseException_str_impl(PyBaseExceptionObject *self);

static PyObject *
BaseException_str(PyObject *self)
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = BaseException_str_impl((PyBaseExceptionObject *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

static PyObject *
BaseException_repr_impl(PyBaseExceptionObject *self);

static PyObject *
BaseException_repr(PyObject *self)
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = BaseException_repr_impl((PyBaseExceptionObject *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(BaseException___reduce____doc__,
"__reduce__($self, /)\n"
"--\n"
"\n");

#define BASEEXCEPTION___REDUCE___METHODDEF    \
    {"__reduce__", (PyCFunction)BaseException___reduce__, METH_NOARGS, BaseException___reduce____doc__},

static PyObject *
BaseException___reduce___impl(PyBaseExceptionObject *self);

static PyObject *
BaseException___reduce__(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = BaseException___reduce___impl((PyBaseExceptionObject *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(BaseException___setstate____doc__,
"__setstate__($self, state, /)\n"
"--\n"
"\n");

#define BASEEXCEPTION___SETSTATE___METHODDEF    \
    {"__setstate__", (PyCFunction)BaseException___setstate__, METH_O, BaseException___setstate____doc__},

static PyObject *
BaseException___setstate___impl(PyBaseExceptionObject *self, PyObject *state);

static PyObject *
BaseException___setstate__(PyObject *self, PyObject *state)
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(state);
    return_value = BaseException___setstate___impl((PyBaseExceptionObject *)self, state);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(BaseException_with_traceback__doc__,
"with_traceback($self, tb, /)\n"
"--\n"
"\n"
"Set self.__traceback__ to tb and return self.");

#define BASEEXCEPTION_WITH_TRACEBACK_METHODDEF    \
    {"with_traceback", (PyCFunction)BaseException_with_traceback, METH_O, BaseException_with_traceback__doc__},

static PyObject *
BaseException_with_traceback_impl(PyBaseExceptionObject *self, PyObject *tb);

static PyObject *
BaseException_with_traceback(PyObject *self, PyObject *tb)
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = BaseException_with_traceback_impl((PyBaseExceptionObject *)self, tb);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(BaseException_add_note__doc__,
"add_note($self, note, /)\n"
"--\n"
"\n"
"Add a note to the exception");

#define BASEEXCEPTION_ADD_NOTE_METHODDEF    \
    {"add_note", (PyCFunction)BaseException_add_note, METH_O, BaseException_add_note__doc__},

static PyObject *
BaseException_add_note_impl(PyBaseExceptionObject *self, PyObject *note);

static PyObject *
BaseException_add_note(PyObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    PyObject *note;

    if (!PyUnicode_Check(arg)) {
        _PyArg_BadArgument("add_note", "argument", "str", arg);
        goto exit;
    }
    note = arg;
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = BaseException_add_note_impl((PyBaseExceptionObject *)self, note);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

static PyObject *
BaseException_args_get_impl(PyBaseExceptionObject *self);

static PyObject *
BaseException_args_get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = BaseException_args_get_impl((PyBaseExceptionObject *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

static int
BaseException_args_set_impl(PyBaseExceptionObject *self, PyObject *value);

static int
BaseException_args_set(PyObject *self, PyObject *arg, void *Py_UNUSED(context))
{
    int return_value = -1;
    PyObject *value;

    if (arg == NULL) {
        PyErr_Format(PyExc_AttributeError,
                     "attribute 'args' of '%.100s' objects cannot be deleted",
                     Py_TYPE(self)->tp_name);
        return -1;
    }
    value = arg;
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = BaseException_args_set_impl((PyBaseExceptionObject *)self, value);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

static PyObject *
BaseException___traceback___get_impl(PyBaseExceptionObject *self);

static PyObject *
BaseException___traceback___get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = BaseException___traceback___get_impl((PyBaseExceptionObject *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

static int
BaseException___traceback___set_impl(PyBaseExceptionObject *self,
                                     PyObject *value);

static int
BaseException___traceback___set(PyObject *self, PyObject *arg, void *Py_UNUSED(context))
{
    int return_value = -1;
    PyObject *value;

    if (arg == NULL) {
        PyErr_Format(PyExc_AttributeError,
                     "attribute '__traceback__' of '%.100s' objects cannot be deleted",
                     Py_TYPE(self)->tp_name);
        return -1;
    }
    value = arg;
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = BaseException___traceback___set_impl((PyBaseExceptionObject *)self, value);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

static PyObject *
BaseException___context___get_impl(PyBaseExceptionObject *self);

static PyObject *
BaseException___context___get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = BaseException___context___get_impl((PyBaseExceptionObject *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

static int
BaseException___context___set_impl(PyBaseExceptionObject *self,
                                   PyObject *value);

static int
BaseException___context___set(PyObject *self, PyObject *arg, void *Py_UNUSED(context))
{
    int return_value = -1;
    PyObject *value;

    if (arg == NULL) {
        PyErr_Format(PyExc_AttributeError,
                     "attribute '__context__' of '%.100s' objects cannot be deleted",
                     Py_TYPE(self)->tp_name);
        return -1;
    }
    value = arg;
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = BaseException___context___set_impl((PyBaseExceptionObject *)self, value);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

static PyObject *
BaseException___cause___get_impl(PyBaseExceptionObject *self);

static PyObject *
BaseException___cause___get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = BaseException___cause___get_impl((PyBaseExceptionObject *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

static int
BaseException___cause___set_impl(PyBaseExceptionObject *self,
                                 PyObject *value);

static int
BaseException___cause___set(PyObject *self, PyObject *arg, void *Py_UNUSED(context))
{
    int return_value = -1;
    PyObject *value;

    if (arg == NULL) {
        PyErr_Format(PyExc_AttributeError,
                     "attribute '__cause__' of '%.100s' objects cannot be deleted",
                     Py_TYPE(self)->tp_name);
        return -1;
    }
    value = arg;
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = BaseException___cause___set_impl((PyBaseExceptionObject *)self, value);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(BaseExceptionGroup_derive__doc__,
"derive($self, excs, /)\n"
"--\n"
"\n");

#define BASEEXCEPTIONGROUP_DERIVE_METHODDEF    \
    {"derive", (PyCFunction)BaseExceptionGroup_derive, METH_O, BaseExceptionGroup_derive__doc__},

static PyObject *
BaseExceptionGroup_derive_impl(PyBaseExceptionGroupObject *self,
                               PyObject *excs);

static PyObject *
BaseExceptionGroup_derive(PyObject *self, PyObject *excs)
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = BaseExceptionGroup_derive_impl((PyBaseExceptionGroupObject *)self, excs);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(BaseExceptionGroup_split__doc__,
"split($self, matcher_value, /)\n"
"--\n"
"\n");

#define BASEEXCEPTIONGROUP_SPLIT_METHODDEF    \
    {"split", (PyCFunction)BaseExceptionGroup_split, METH_O, BaseExceptionGroup_split__doc__},

static PyObject *
BaseExceptionGroup_split_impl(PyBaseExceptionGroupObject *self,
                              PyObject *matcher_value);

static PyObject *
BaseExceptionGroup_split(PyObject *self, PyObject *matcher_value)
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = BaseExceptionGroup_split_impl((PyBaseExceptionGroupObject *)self, matcher_value);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(BaseExceptionGroup_subgroup__doc__,
"subgroup($self, matcher_value, /)\n"
"--\n"
"\n");

#define BASEEXCEPTIONGROUP_SUBGROUP_METHODDEF    \
    {"subgroup", (PyCFunction)BaseExceptionGroup_subgroup, METH_O, BaseExceptionGroup_subgroup__doc__},

static PyObject *
BaseExceptionGroup_subgroup_impl(PyBaseExceptionGroupObject *self,
                                 PyObject *matcher_value);

static PyObject *
BaseExceptionGroup_subgroup(PyObject *self, PyObject *matcher_value)
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = BaseExceptionGroup_subgroup_impl((PyBaseExceptionGroupObject *)self, matcher_value);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

static int
ImportError_init_impl(PyImportErrorObject *self, PyObject *args,
                      PyObject *name, PyObject *path, PyObject *name_from);

static int
ImportError_init_helper(PyObject *self, PyObject *const *args,
    Py_ssize_t nargs, Py_ssize_t nkw, PyObject *kwargs, PyObject *kwnames)
{
    int return_value = -1;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(name), &_Py_ID(path), &_Py_ID(name_from), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"name", "path", "name_from", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "ImportError",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[3];
    PyObject * const *fastargs;
    Py_ssize_t noptargs = 0 + nkw - 0;
    PyObject *__clinic_args = NULL;
    PyObject *name = NULL;
    PyObject *path = NULL;
    PyObject *name_from = NULL;

    fastargs = _PyArg_UnpackKeywords(args, nargs, kwargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 0, /*minkw*/ 0, /*varpos*/ 1, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (fastargs[0]) {
        name = fastargs[0];
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (fastargs[1]) {
        path = fastargs[1];
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    name_from = fastargs[2];
skip_optional_kwonly:
    __clinic_args = PyTuple_FromArray(args, nargs);
    if (__clinic_args == NULL) {
        goto exit;
    }
    return_value = ImportError_init_impl((PyImportErrorObject *)self, __clinic_args, name, path, name_from);

exit:
    /* Cleanup for args */
    Py_XDECREF(__clinic_args);

    return return_value;
}

static int
ImportError_init(PyObject *self, PyObject *args, PyObject *kwargs)
{
    return ImportError_init_helper(self, _PyTuple_CAST(args)->ob_item,
        PyTuple_GET_SIZE(args),
        kwargs ? PyDict_GET_SIZE(kwargs) : 0,
        kwargs, NULL);
}

static PyObject *
ImportError_vectorcall(PyObject *type, PyObject *const *args,
    size_t nargsf, PyObject *kwnames)
{
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    PyObject *self;
    int _result;

    assert(_PyType_CAST(type)->tp_init == ImportError_init);
    /* Make sure the type object is immutable: the generated
     * vectorcall doesn't deal e.g. with users reassigning __init__. */
    assert(PyType_HasFeature(_PyType_CAST(type), Py_TPFLAGS_IMMUTABLETYPE));
    self = _PyType_CAST(type)->tp_new(_PyType_CAST(type),
        (PyObject *)&_Py_SINGLETON(tuple_empty), NULL);
    if (self == NULL) {
        return NULL;
    }
    _result = ImportError_init_helper(self, args, nargs,
        kwnames ? PyTuple_GET_SIZE(kwnames) : 0,
        NULL, kwnames);
    if (_result != 0) {
        Py_DECREF(self);
        return NULL;
    }
    return self;
}

static int
NameError_init_impl(PyNameErrorObject *self, PyObject *args, PyObject *name);

static int
NameError_init_helper(PyObject *self, PyObject *const *args,
    Py_ssize_t nargs, Py_ssize_t nkw, PyObject *kwargs, PyObject *kwnames)
{
    int return_value = -1;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(name), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"name", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "NameError",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject * const *fastargs;
    Py_ssize_t noptargs = 0 + nkw - 0;
    PyObject *__clinic_args = NULL;
    PyObject *name = NULL;

    fastargs = _PyArg_UnpackKeywords(args, nargs, kwargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 0, /*minkw*/ 0, /*varpos*/ 1, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    name = fastargs[0];
skip_optional_kwonly:
    __clinic_args = PyTuple_FromArray(args, nargs);
    if (__clinic_args == NULL) {
        goto exit;
    }
    return_value = NameError_init_impl((PyNameErrorObject *)self, __clinic_args, name);

exit:
    /* Cleanup for args */
    Py_XDECREF(__clinic_args);

    return return_value;
}

static int
NameError_init(PyObject *self, PyObject *args, PyObject *kwargs)
{
    return NameError_init_helper(self, _PyTuple_CAST(args)->ob_item,
        PyTuple_GET_SIZE(args),
        kwargs ? PyDict_GET_SIZE(kwargs) : 0,
        kwargs, NULL);
}

static PyObject *
NameError_vectorcall(PyObject *type, PyObject *const *args,
    size_t nargsf, PyObject *kwnames)
{
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    PyObject *self;
    int _result;

    assert(_PyType_CAST(type)->tp_init == NameError_init);
    /* Make sure the type object is immutable: the generated
     * vectorcall doesn't deal e.g. with users reassigning __init__. */
    assert(PyType_HasFeature(_PyType_CAST(type), Py_TPFLAGS_IMMUTABLETYPE));
    self = _PyType_CAST(type)->tp_new(_PyType_CAST(type),
        (PyObject *)&_Py_SINGLETON(tuple_empty), NULL);
    if (self == NULL) {
        return NULL;
    }
    _result = NameError_init_helper(self, args, nargs,
        kwnames ? PyTuple_GET_SIZE(kwnames) : 0,
        NULL, kwnames);
    if (_result != 0) {
        Py_DECREF(self);
        return NULL;
    }
    return self;
}

static int
AttributeError_init_impl(PyAttributeErrorObject *self, PyObject *args,
                         PyObject *name, PyObject *obj);

static int
AttributeError_init_helper(PyObject *self, PyObject *const *args,
    Py_ssize_t nargs, Py_ssize_t nkw, PyObject *kwargs, PyObject *kwnames)
{
    int return_value = -1;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(name), &_Py_ID(obj), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"name", "obj", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "AttributeError",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    PyObject * const *fastargs;
    Py_ssize_t noptargs = 0 + nkw - 0;
    PyObject *__clinic_args = NULL;
    PyObject *name = NULL;
    PyObject *obj = NULL;

    fastargs = _PyArg_UnpackKeywords(args, nargs, kwargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 0, /*minkw*/ 0, /*varpos*/ 1, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (fastargs[0]) {
        name = fastargs[0];
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    obj = fastargs[1];
skip_optional_kwonly:
    __clinic_args = PyTuple_FromArray(args, nargs);
    if (__clinic_args == NULL) {
        goto exit;
    }
    return_value = AttributeError_init_impl((PyAttributeErrorObject *)self, __clinic_args, name, obj);

exit:
    /* Cleanup for args */
    Py_XDECREF(__clinic_args);

    return return_value;
}

static int
AttributeError_init(PyObject *self, PyObject *args, PyObject *kwargs)
{
    return AttributeError_init_helper(self, _PyTuple_CAST(args)->ob_item,
        PyTuple_GET_SIZE(args),
        kwargs ? PyDict_GET_SIZE(kwargs) : 0,
        kwargs, NULL);
}

static PyObject *
AttributeError_vectorcall(PyObject *type, PyObject *const *args,
    size_t nargsf, PyObject *kwnames)
{
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    PyObject *self;
    int _result;

    assert(_PyType_CAST(type)->tp_init == AttributeError_init);
    /* Make sure the type object is immutable: the generated
     * vectorcall doesn't deal e.g. with users reassigning __init__. */
    assert(PyType_HasFeature(_PyType_CAST(type), Py_TPFLAGS_IMMUTABLETYPE));
    self = _PyType_CAST(type)->tp_new(_PyType_CAST(type),
        (PyObject *)&_Py_SINGLETON(tuple_empty), NULL);
    if (self == NULL) {
        return NULL;
    }
    _result = AttributeError_init_helper(self, args, nargs,
        kwnames ? PyTuple_GET_SIZE(kwnames) : 0,
        NULL, kwnames);
    if (_result != 0) {
        Py_DECREF(self);
        return NULL;
    }
    return self;
}
#define BASEEXCEPTION_ARGS_GETSETDEF {"args", (getter)BaseException_args_get, (setter)BaseException_args_set, NULL},

#define BASEEXCEPTION___TRACEBACK___GETSETDEF {"__traceback__", (getter)BaseException___traceback___get, (setter)BaseException___traceback___set, NULL},

#define BASEEXCEPTION___CONTEXT___GETSETDEF {"__context__", (getter)BaseException___context___get, (setter)BaseException___context___set, NULL},

#define BASEEXCEPTION___CAUSE___GETSETDEF {"__cause__", (getter)BaseException___cause___get, (setter)BaseException___cause___set, NULL},

/*[clinic end generated code: output=c8eaffee13bc041d input=a9049054013a1b77]*/
