/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_SINGLETON()
#endif
#include "pycore_abstract.h"      // _PyNumber_Index()
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()

PyDoc_STRVAR(_elementtree_Element_append__doc__,
"append($self, subelement, /)\n"
"--\n"
"\n");

#define _ELEMENTTREE_ELEMENT_APPEND_METHODDEF    \
    {"append", _PyCFunction_CAST(_elementtree_Element_append), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _elementtree_Element_append__doc__},

static PyObject *
_elementtree_Element_append_impl(ElementObject *self, PyTypeObject *cls,
                                 PyObject *subelement);

static PyObject *
_elementtree_Element_append(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "append",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *subelement;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!PyObject_TypeCheck(args[0], clinic_state()->Element_Type)) {
        _PyArg_BadArgument("append", "argument 1", (clinic_state()->Element_Type)->tp_name, args[0]);
        goto exit;
    }
    subelement = args[0];
    return_value = _elementtree_Element_append_impl((ElementObject *)self, cls, subelement);

exit:
    return return_value;
}

PyDoc_STRVAR(_elementtree_Element_clear__doc__,
"clear($self, /)\n"
"--\n"
"\n");

#define _ELEMENTTREE_ELEMENT_CLEAR_METHODDEF    \
    {"clear", (PyCFunction)_elementtree_Element_clear, METH_NOARGS, _elementtree_Element_clear__doc__},

static PyObject *
_elementtree_Element_clear_impl(ElementObject *self);

static PyObject *
_elementtree_Element_clear(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _elementtree_Element_clear_impl((ElementObject *)self);
}

PyDoc_STRVAR(_elementtree_Element___copy____doc__,
"__copy__($self, /)\n"
"--\n"
"\n");

#define _ELEMENTTREE_ELEMENT___COPY___METHODDEF    \
    {"__copy__", _PyCFunction_CAST(_elementtree_Element___copy__), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _elementtree_Element___copy____doc__},

static PyObject *
_elementtree_Element___copy___impl(ElementObject *self, PyTypeObject *cls);

static PyObject *
_elementtree_Element___copy__(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "__copy__() takes no arguments");
        return NULL;
    }
    return _elementtree_Element___copy___impl((ElementObject *)self, cls);
}

PyDoc_STRVAR(_elementtree_Element___deepcopy____doc__,
"__deepcopy__($self, memo, /)\n"
"--\n"
"\n");

#define _ELEMENTTREE_ELEMENT___DEEPCOPY___METHODDEF    \
    {"__deepcopy__", (PyCFunction)_elementtree_Element___deepcopy__, METH_O, _elementtree_Element___deepcopy____doc__},

static PyObject *
_elementtree_Element___deepcopy___impl(ElementObject *self, PyObject *memo);

static PyObject *
_elementtree_Element___deepcopy__(PyObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    PyObject *memo;

    if (!PyDict_Check(arg)) {
        _PyArg_BadArgument("__deepcopy__", "argument", "dict", arg);
        goto exit;
    }
    memo = arg;
    return_value = _elementtree_Element___deepcopy___impl((ElementObject *)self, memo);

exit:
    return return_value;
}

PyDoc_STRVAR(_elementtree_Element___sizeof____doc__,
"__sizeof__($self, /)\n"
"--\n"
"\n");

#define _ELEMENTTREE_ELEMENT___SIZEOF___METHODDEF    \
    {"__sizeof__", (PyCFunction)_elementtree_Element___sizeof__, METH_NOARGS, _elementtree_Element___sizeof____doc__},

static size_t
_elementtree_Element___sizeof___impl(ElementObject *self);

static PyObject *
_elementtree_Element___sizeof__(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;
    size_t _return_value;

    _return_value = _elementtree_Element___sizeof___impl((ElementObject *)self);
    if ((_return_value == (size_t)-1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromSize_t(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_elementtree_Element___getstate____doc__,
"__getstate__($self, /)\n"
"--\n"
"\n");

#define _ELEMENTTREE_ELEMENT___GETSTATE___METHODDEF    \
    {"__getstate__", (PyCFunction)_elementtree_Element___getstate__, METH_NOARGS, _elementtree_Element___getstate____doc__},

static PyObject *
_elementtree_Element___getstate___impl(ElementObject *self);

static PyObject *
_elementtree_Element___getstate__(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _elementtree_Element___getstate___impl((ElementObject *)self);
}

PyDoc_STRVAR(_elementtree_Element___setstate____doc__,
"__setstate__($self, state, /)\n"
"--\n"
"\n");

#define _ELEMENTTREE_ELEMENT___SETSTATE___METHODDEF    \
    {"__setstate__", _PyCFunction_CAST(_elementtree_Element___setstate__), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _elementtree_Element___setstate____doc__},

static PyObject *
_elementtree_Element___setstate___impl(ElementObject *self,
                                       PyTypeObject *cls, PyObject *state);

static PyObject *
_elementtree_Element___setstate__(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "__setstate__",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *state;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    state = args[0];
    return_value = _elementtree_Element___setstate___impl((ElementObject *)self, cls, state);

exit:
    return return_value;
}

PyDoc_STRVAR(_elementtree_Element_extend__doc__,
"extend($self, elements, /)\n"
"--\n"
"\n");

#define _ELEMENTTREE_ELEMENT_EXTEND_METHODDEF    \
    {"extend", _PyCFunction_CAST(_elementtree_Element_extend), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _elementtree_Element_extend__doc__},

static PyObject *
_elementtree_Element_extend_impl(ElementObject *self, PyTypeObject *cls,
                                 PyObject *elements);

static PyObject *
_elementtree_Element_extend(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "extend",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *elements;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    elements = args[0];
    return_value = _elementtree_Element_extend_impl((ElementObject *)self, cls, elements);

exit:
    return return_value;
}

PyDoc_STRVAR(_elementtree_Element_find__doc__,
"find($self, /, path, namespaces=None)\n"
"--\n"
"\n");

#define _ELEMENTTREE_ELEMENT_FIND_METHODDEF    \
    {"find", _PyCFunction_CAST(_elementtree_Element_find), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _elementtree_Element_find__doc__},

static PyObject *
_elementtree_Element_find_impl(ElementObject *self, PyTypeObject *cls,
                               PyObject *path, PyObject *namespaces);

static PyObject *
_elementtree_Element_find(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
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
        .ob_item = { &_Py_ID(path), &_Py_ID(namespaces), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"path", "namespaces", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "find",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *path;
    PyObject *namespaces = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    path = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    namespaces = args[1];
skip_optional_pos:
    return_value = _elementtree_Element_find_impl((ElementObject *)self, cls, path, namespaces);

exit:
    return return_value;
}

PyDoc_STRVAR(_elementtree_Element_findtext__doc__,
"findtext($self, /, path, default=None, namespaces=None)\n"
"--\n"
"\n");

#define _ELEMENTTREE_ELEMENT_FINDTEXT_METHODDEF    \
    {"findtext", _PyCFunction_CAST(_elementtree_Element_findtext), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _elementtree_Element_findtext__doc__},

static PyObject *
_elementtree_Element_findtext_impl(ElementObject *self, PyTypeObject *cls,
                                   PyObject *path, PyObject *default_value,
                                   PyObject *namespaces);

static PyObject *
_elementtree_Element_findtext(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
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
        .ob_item = { &_Py_ID(path), &_Py_ID(default), &_Py_ID(namespaces), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"path", "default", "namespaces", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "findtext",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *path;
    PyObject *default_value = Py_None;
    PyObject *namespaces = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    path = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[1]) {
        default_value = args[1];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    namespaces = args[2];
skip_optional_pos:
    return_value = _elementtree_Element_findtext_impl((ElementObject *)self, cls, path, default_value, namespaces);

exit:
    return return_value;
}

PyDoc_STRVAR(_elementtree_Element_findall__doc__,
"findall($self, /, path, namespaces=None)\n"
"--\n"
"\n");

#define _ELEMENTTREE_ELEMENT_FINDALL_METHODDEF    \
    {"findall", _PyCFunction_CAST(_elementtree_Element_findall), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _elementtree_Element_findall__doc__},

static PyObject *
_elementtree_Element_findall_impl(ElementObject *self, PyTypeObject *cls,
                                  PyObject *path, PyObject *namespaces);

static PyObject *
_elementtree_Element_findall(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
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
        .ob_item = { &_Py_ID(path), &_Py_ID(namespaces), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"path", "namespaces", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "findall",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *path;
    PyObject *namespaces = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    path = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    namespaces = args[1];
skip_optional_pos:
    return_value = _elementtree_Element_findall_impl((ElementObject *)self, cls, path, namespaces);

exit:
    return return_value;
}

PyDoc_STRVAR(_elementtree_Element_iterfind__doc__,
"iterfind($self, /, path, namespaces=None)\n"
"--\n"
"\n");

#define _ELEMENTTREE_ELEMENT_ITERFIND_METHODDEF    \
    {"iterfind", _PyCFunction_CAST(_elementtree_Element_iterfind), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _elementtree_Element_iterfind__doc__},

static PyObject *
_elementtree_Element_iterfind_impl(ElementObject *self, PyTypeObject *cls,
                                   PyObject *path, PyObject *namespaces);

static PyObject *
_elementtree_Element_iterfind(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
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
        .ob_item = { &_Py_ID(path), &_Py_ID(namespaces), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"path", "namespaces", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "iterfind",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *path;
    PyObject *namespaces = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    path = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    namespaces = args[1];
skip_optional_pos:
    return_value = _elementtree_Element_iterfind_impl((ElementObject *)self, cls, path, namespaces);

exit:
    return return_value;
}

PyDoc_STRVAR(_elementtree_Element_get__doc__,
"get($self, /, key, default=None)\n"
"--\n"
"\n");

#define _ELEMENTTREE_ELEMENT_GET_METHODDEF    \
    {"get", _PyCFunction_CAST(_elementtree_Element_get), METH_FASTCALL|METH_KEYWORDS, _elementtree_Element_get__doc__},

static PyObject *
_elementtree_Element_get_impl(ElementObject *self, PyObject *key,
                              PyObject *default_value);

static PyObject *
_elementtree_Element_get(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
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
        .ob_item = { &_Py_ID(key), &_Py_ID(default), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"key", "default", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "get",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *key;
    PyObject *default_value = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    key = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    default_value = args[1];
skip_optional_pos:
    return_value = _elementtree_Element_get_impl((ElementObject *)self, key, default_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_elementtree_Element_iter__doc__,
"iter($self, /, tag=None)\n"
"--\n"
"\n");

#define _ELEMENTTREE_ELEMENT_ITER_METHODDEF    \
    {"iter", _PyCFunction_CAST(_elementtree_Element_iter), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _elementtree_Element_iter__doc__},

static PyObject *
_elementtree_Element_iter_impl(ElementObject *self, PyTypeObject *cls,
                               PyObject *tag);

static PyObject *
_elementtree_Element_iter(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
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
        .ob_item = { &_Py_ID(tag), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"tag", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "iter",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *tag = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    tag = args[0];
skip_optional_pos:
    return_value = _elementtree_Element_iter_impl((ElementObject *)self, cls, tag);

exit:
    return return_value;
}

PyDoc_STRVAR(_elementtree_Element_itertext__doc__,
"itertext($self, /)\n"
"--\n"
"\n");

#define _ELEMENTTREE_ELEMENT_ITERTEXT_METHODDEF    \
    {"itertext", _PyCFunction_CAST(_elementtree_Element_itertext), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _elementtree_Element_itertext__doc__},

static PyObject *
_elementtree_Element_itertext_impl(ElementObject *self, PyTypeObject *cls);

static PyObject *
_elementtree_Element_itertext(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "itertext() takes no arguments");
        return NULL;
    }
    return _elementtree_Element_itertext_impl((ElementObject *)self, cls);
}

PyDoc_STRVAR(_elementtree_Element_insert__doc__,
"insert($self, index, subelement, /)\n"
"--\n"
"\n");

#define _ELEMENTTREE_ELEMENT_INSERT_METHODDEF    \
    {"insert", _PyCFunction_CAST(_elementtree_Element_insert), METH_FASTCALL, _elementtree_Element_insert__doc__},

static PyObject *
_elementtree_Element_insert_impl(ElementObject *self, Py_ssize_t index,
                                 PyObject *subelement);

static PyObject *
_elementtree_Element_insert(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_ssize_t index;
    PyObject *subelement;

    if (!_PyArg_CheckPositional("insert", nargs, 2, 2)) {
        goto exit;
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = _PyNumber_Index(args[0]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        index = ival;
    }
    if (!PyObject_TypeCheck(args[1], clinic_state()->Element_Type)) {
        _PyArg_BadArgument("insert", "argument 2", (clinic_state()->Element_Type)->tp_name, args[1]);
        goto exit;
    }
    subelement = args[1];
    return_value = _elementtree_Element_insert_impl((ElementObject *)self, index, subelement);

exit:
    return return_value;
}

PyDoc_STRVAR(_elementtree_Element_items__doc__,
"items($self, /)\n"
"--\n"
"\n");

#define _ELEMENTTREE_ELEMENT_ITEMS_METHODDEF    \
    {"items", (PyCFunction)_elementtree_Element_items, METH_NOARGS, _elementtree_Element_items__doc__},

static PyObject *
_elementtree_Element_items_impl(ElementObject *self);

static PyObject *
_elementtree_Element_items(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _elementtree_Element_items_impl((ElementObject *)self);
}

PyDoc_STRVAR(_elementtree_Element_keys__doc__,
"keys($self, /)\n"
"--\n"
"\n");

#define _ELEMENTTREE_ELEMENT_KEYS_METHODDEF    \
    {"keys", (PyCFunction)_elementtree_Element_keys, METH_NOARGS, _elementtree_Element_keys__doc__},

static PyObject *
_elementtree_Element_keys_impl(ElementObject *self);

static PyObject *
_elementtree_Element_keys(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _elementtree_Element_keys_impl((ElementObject *)self);
}

PyDoc_STRVAR(_elementtree_Element_makeelement__doc__,
"makeelement($self, tag, attrib, /)\n"
"--\n"
"\n");

#define _ELEMENTTREE_ELEMENT_MAKEELEMENT_METHODDEF    \
    {"makeelement", _PyCFunction_CAST(_elementtree_Element_makeelement), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _elementtree_Element_makeelement__doc__},

static PyObject *
_elementtree_Element_makeelement_impl(ElementObject *self, PyTypeObject *cls,
                                      PyObject *tag, PyObject *attrib);

static PyObject *
_elementtree_Element_makeelement(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", "", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "makeelement",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    PyObject *tag;
    PyObject *attrib;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    tag = args[0];
    if (!PyDict_Check(args[1])) {
        _PyArg_BadArgument("makeelement", "argument 2", "dict", args[1]);
        goto exit;
    }
    attrib = args[1];
    return_value = _elementtree_Element_makeelement_impl((ElementObject *)self, cls, tag, attrib);

exit:
    return return_value;
}

PyDoc_STRVAR(_elementtree_Element_remove__doc__,
"remove($self, subelement, /)\n"
"--\n"
"\n");

#define _ELEMENTTREE_ELEMENT_REMOVE_METHODDEF    \
    {"remove", (PyCFunction)_elementtree_Element_remove, METH_O, _elementtree_Element_remove__doc__},

static PyObject *
_elementtree_Element_remove_impl(ElementObject *self, PyObject *subelement);

static PyObject *
_elementtree_Element_remove(PyObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    PyObject *subelement;

    if (!PyObject_TypeCheck(arg, clinic_state()->Element_Type)) {
        _PyArg_BadArgument("remove", "argument", (clinic_state()->Element_Type)->tp_name, arg);
        goto exit;
    }
    subelement = arg;
    return_value = _elementtree_Element_remove_impl((ElementObject *)self, subelement);

exit:
    return return_value;
}

PyDoc_STRVAR(_elementtree_Element_set__doc__,
"set($self, key, value, /)\n"
"--\n"
"\n");

#define _ELEMENTTREE_ELEMENT_SET_METHODDEF    \
    {"set", _PyCFunction_CAST(_elementtree_Element_set), METH_FASTCALL, _elementtree_Element_set__doc__},

static PyObject *
_elementtree_Element_set_impl(ElementObject *self, PyObject *key,
                              PyObject *value);

static PyObject *
_elementtree_Element_set(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *key;
    PyObject *value;

    if (!_PyArg_CheckPositional("set", nargs, 2, 2)) {
        goto exit;
    }
    key = args[0];
    value = args[1];
    return_value = _elementtree_Element_set_impl((ElementObject *)self, key, value);

exit:
    return return_value;
}

static int
_elementtree_TreeBuilder___init___impl(TreeBuilderObject *self,
                                       PyObject *element_factory,
                                       PyObject *comment_factory,
                                       PyObject *pi_factory,
                                       int insert_comments, int insert_pis);

static int
_elementtree_TreeBuilder___init__(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int return_value = -1;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 5
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(element_factory), &_Py_ID(comment_factory), &_Py_ID(pi_factory), &_Py_ID(insert_comments), &_Py_ID(insert_pis), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"element_factory", "comment_factory", "pi_factory", "insert_comments", "insert_pis", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "TreeBuilder",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[5];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 0;
    PyObject *element_factory = Py_None;
    PyObject *comment_factory = Py_None;
    PyObject *pi_factory = Py_None;
    int insert_comments = 0;
    int insert_pis = 0;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (fastargs[0]) {
        element_factory = fastargs[0];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
skip_optional_pos:
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (fastargs[1]) {
        comment_factory = fastargs[1];
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (fastargs[2]) {
        pi_factory = fastargs[2];
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (fastargs[3]) {
        insert_comments = PyObject_IsTrue(fastargs[3]);
        if (insert_comments < 0) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    insert_pis = PyObject_IsTrue(fastargs[4]);
    if (insert_pis < 0) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = _elementtree_TreeBuilder___init___impl((TreeBuilderObject *)self, element_factory, comment_factory, pi_factory, insert_comments, insert_pis);

exit:
    return return_value;
}

PyDoc_STRVAR(_elementtree__set_factories__doc__,
"_set_factories($module, comment_factory, pi_factory, /)\n"
"--\n"
"\n"
"Change the factories used to create comments and processing instructions.\n"
"\n"
"For internal use only.");

#define _ELEMENTTREE__SET_FACTORIES_METHODDEF    \
    {"_set_factories", _PyCFunction_CAST(_elementtree__set_factories), METH_FASTCALL, _elementtree__set_factories__doc__},

static PyObject *
_elementtree__set_factories_impl(PyObject *module, PyObject *comment_factory,
                                 PyObject *pi_factory);

static PyObject *
_elementtree__set_factories(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *comment_factory;
    PyObject *pi_factory;

    if (!_PyArg_CheckPositional("_set_factories", nargs, 2, 2)) {
        goto exit;
    }
    comment_factory = args[0];
    pi_factory = args[1];
    return_value = _elementtree__set_factories_impl(module, comment_factory, pi_factory);

exit:
    return return_value;
}

PyDoc_STRVAR(_elementtree_TreeBuilder_data__doc__,
"data($self, data, /)\n"
"--\n"
"\n");

#define _ELEMENTTREE_TREEBUILDER_DATA_METHODDEF    \
    {"data", (PyCFunction)_elementtree_TreeBuilder_data, METH_O, _elementtree_TreeBuilder_data__doc__},

static PyObject *
_elementtree_TreeBuilder_data_impl(TreeBuilderObject *self, PyObject *data);

static PyObject *
_elementtree_TreeBuilder_data(PyObject *self, PyObject *data)
{
    PyObject *return_value = NULL;

    return_value = _elementtree_TreeBuilder_data_impl((TreeBuilderObject *)self, data);

    return return_value;
}

PyDoc_STRVAR(_elementtree_TreeBuilder_end__doc__,
"end($self, tag, /)\n"
"--\n"
"\n");

#define _ELEMENTTREE_TREEBUILDER_END_METHODDEF    \
    {"end", (PyCFunction)_elementtree_TreeBuilder_end, METH_O, _elementtree_TreeBuilder_end__doc__},

static PyObject *
_elementtree_TreeBuilder_end_impl(TreeBuilderObject *self, PyObject *tag);

static PyObject *
_elementtree_TreeBuilder_end(PyObject *self, PyObject *tag)
{
    PyObject *return_value = NULL;

    return_value = _elementtree_TreeBuilder_end_impl((TreeBuilderObject *)self, tag);

    return return_value;
}

PyDoc_STRVAR(_elementtree_TreeBuilder_comment__doc__,
"comment($self, text, /)\n"
"--\n"
"\n");

#define _ELEMENTTREE_TREEBUILDER_COMMENT_METHODDEF    \
    {"comment", (PyCFunction)_elementtree_TreeBuilder_comment, METH_O, _elementtree_TreeBuilder_comment__doc__},

static PyObject *
_elementtree_TreeBuilder_comment_impl(TreeBuilderObject *self,
                                      PyObject *text);

static PyObject *
_elementtree_TreeBuilder_comment(PyObject *self, PyObject *text)
{
    PyObject *return_value = NULL;

    return_value = _elementtree_TreeBuilder_comment_impl((TreeBuilderObject *)self, text);

    return return_value;
}

PyDoc_STRVAR(_elementtree_TreeBuilder_pi__doc__,
"pi($self, target, text=None, /)\n"
"--\n"
"\n");

#define _ELEMENTTREE_TREEBUILDER_PI_METHODDEF    \
    {"pi", _PyCFunction_CAST(_elementtree_TreeBuilder_pi), METH_FASTCALL, _elementtree_TreeBuilder_pi__doc__},

static PyObject *
_elementtree_TreeBuilder_pi_impl(TreeBuilderObject *self, PyObject *target,
                                 PyObject *text);

static PyObject *
_elementtree_TreeBuilder_pi(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *target;
    PyObject *text = Py_None;

    if (!_PyArg_CheckPositional("pi", nargs, 1, 2)) {
        goto exit;
    }
    target = args[0];
    if (nargs < 2) {
        goto skip_optional;
    }
    text = args[1];
skip_optional:
    return_value = _elementtree_TreeBuilder_pi_impl((TreeBuilderObject *)self, target, text);

exit:
    return return_value;
}

PyDoc_STRVAR(_elementtree_TreeBuilder_close__doc__,
"close($self, /)\n"
"--\n"
"\n");

#define _ELEMENTTREE_TREEBUILDER_CLOSE_METHODDEF    \
    {"close", (PyCFunction)_elementtree_TreeBuilder_close, METH_NOARGS, _elementtree_TreeBuilder_close__doc__},

static PyObject *
_elementtree_TreeBuilder_close_impl(TreeBuilderObject *self);

static PyObject *
_elementtree_TreeBuilder_close(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _elementtree_TreeBuilder_close_impl((TreeBuilderObject *)self);
}

PyDoc_STRVAR(_elementtree_TreeBuilder_start__doc__,
"start($self, tag, attrs, /)\n"
"--\n"
"\n");

#define _ELEMENTTREE_TREEBUILDER_START_METHODDEF    \
    {"start", _PyCFunction_CAST(_elementtree_TreeBuilder_start), METH_FASTCALL, _elementtree_TreeBuilder_start__doc__},

static PyObject *
_elementtree_TreeBuilder_start_impl(TreeBuilderObject *self, PyObject *tag,
                                    PyObject *attrs);

static PyObject *
_elementtree_TreeBuilder_start(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *tag;
    PyObject *attrs;

    if (!_PyArg_CheckPositional("start", nargs, 2, 2)) {
        goto exit;
    }
    tag = args[0];
    if (!PyDict_Check(args[1])) {
        _PyArg_BadArgument("start", "argument 2", "dict", args[1]);
        goto exit;
    }
    attrs = args[1];
    return_value = _elementtree_TreeBuilder_start_impl((TreeBuilderObject *)self, tag, attrs);

exit:
    return return_value;
}

static int
_elementtree_XMLParser___init___impl(XMLParserObject *self, PyObject *target,
                                     const char *encoding);

static int
_elementtree_XMLParser___init__(PyObject *self, PyObject *args, PyObject *kwargs)
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
        .ob_item = { &_Py_ID(target), &_Py_ID(encoding), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"target", "encoding", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "XMLParser",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 0;
    PyObject *target = Py_None;
    const char *encoding = NULL;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser,
            /*minpos*/ 0, /*maxpos*/ 0, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (fastargs[0]) {
        target = fastargs[0];
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (fastargs[1] == Py_None) {
        encoding = NULL;
    }
    else if (PyUnicode_Check(fastargs[1])) {
        Py_ssize_t encoding_length;
        encoding = PyUnicode_AsUTF8AndSize(fastargs[1], &encoding_length);
        if (encoding == NULL) {
            goto exit;
        }
        if (strlen(encoding) != (size_t)encoding_length) {
            PyErr_SetString(PyExc_ValueError, "embedded null character");
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("XMLParser", "argument 'encoding'", "str or None", fastargs[1]);
        goto exit;
    }
skip_optional_kwonly:
    return_value = _elementtree_XMLParser___init___impl((XMLParserObject *)self, target, encoding);

exit:
    return return_value;
}

PyDoc_STRVAR(_elementtree_XMLParser_close__doc__,
"close($self, /)\n"
"--\n"
"\n");

#define _ELEMENTTREE_XMLPARSER_CLOSE_METHODDEF    \
    {"close", (PyCFunction)_elementtree_XMLParser_close, METH_NOARGS, _elementtree_XMLParser_close__doc__},

static PyObject *
_elementtree_XMLParser_close_impl(XMLParserObject *self);

static PyObject *
_elementtree_XMLParser_close(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _elementtree_XMLParser_close_impl((XMLParserObject *)self);
}

PyDoc_STRVAR(_elementtree_XMLParser_flush__doc__,
"flush($self, /)\n"
"--\n"
"\n");

#define _ELEMENTTREE_XMLPARSER_FLUSH_METHODDEF    \
    {"flush", (PyCFunction)_elementtree_XMLParser_flush, METH_NOARGS, _elementtree_XMLParser_flush__doc__},

static PyObject *
_elementtree_XMLParser_flush_impl(XMLParserObject *self);

static PyObject *
_elementtree_XMLParser_flush(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _elementtree_XMLParser_flush_impl((XMLParserObject *)self);
}

PyDoc_STRVAR(_elementtree_XMLParser_feed__doc__,
"feed($self, data, /)\n"
"--\n"
"\n");

#define _ELEMENTTREE_XMLPARSER_FEED_METHODDEF    \
    {"feed", (PyCFunction)_elementtree_XMLParser_feed, METH_O, _elementtree_XMLParser_feed__doc__},

static PyObject *
_elementtree_XMLParser_feed_impl(XMLParserObject *self, PyObject *data);

static PyObject *
_elementtree_XMLParser_feed(PyObject *self, PyObject *data)
{
    PyObject *return_value = NULL;

    return_value = _elementtree_XMLParser_feed_impl((XMLParserObject *)self, data);

    return return_value;
}

PyDoc_STRVAR(_elementtree_XMLParser__parse_whole__doc__,
"_parse_whole($self, file, /)\n"
"--\n"
"\n");

#define _ELEMENTTREE_XMLPARSER__PARSE_WHOLE_METHODDEF    \
    {"_parse_whole", (PyCFunction)_elementtree_XMLParser__parse_whole, METH_O, _elementtree_XMLParser__parse_whole__doc__},

static PyObject *
_elementtree_XMLParser__parse_whole_impl(XMLParserObject *self,
                                         PyObject *file);

static PyObject *
_elementtree_XMLParser__parse_whole(PyObject *self, PyObject *file)
{
    PyObject *return_value = NULL;

    return_value = _elementtree_XMLParser__parse_whole_impl((XMLParserObject *)self, file);

    return return_value;
}

PyDoc_STRVAR(_elementtree_XMLParser__setevents__doc__,
"_setevents($self, events_queue, events_to_report=None, /)\n"
"--\n"
"\n");

#define _ELEMENTTREE_XMLPARSER__SETEVENTS_METHODDEF    \
    {"_setevents", _PyCFunction_CAST(_elementtree_XMLParser__setevents), METH_FASTCALL, _elementtree_XMLParser__setevents__doc__},

static PyObject *
_elementtree_XMLParser__setevents_impl(XMLParserObject *self,
                                       PyObject *events_queue,
                                       PyObject *events_to_report);

static PyObject *
_elementtree_XMLParser__setevents(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *events_queue;
    PyObject *events_to_report = Py_None;

    if (!_PyArg_CheckPositional("_setevents", nargs, 1, 2)) {
        goto exit;
    }
    events_queue = args[0];
    if (nargs < 2) {
        goto skip_optional;
    }
    events_to_report = args[1];
skip_optional:
    return_value = _elementtree_XMLParser__setevents_impl((XMLParserObject *)self, events_queue, events_to_report);

exit:
    return return_value;
}
/*[clinic end generated code: output=c863ce16d8566291 input=a9049054013a1b77]*/
