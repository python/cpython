/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()

PyDoc_STRVAR(OrderedDict_fromkeys__doc__,
"fromkeys($type, /, iterable, value=None)\n"
"--\n"
"\n"
"Create a new ordered dictionary with keys from iterable and values set to value.");

#define ORDEREDDICT_FROMKEYS_METHODDEF    \
    {"fromkeys", _PyCFunction_CAST(OrderedDict_fromkeys), METH_FASTCALL|METH_KEYWORDS|METH_CLASS, OrderedDict_fromkeys__doc__},

static PyObject *
OrderedDict_fromkeys_impl(PyTypeObject *type, PyObject *seq, PyObject *value);

static PyObject *
OrderedDict_fromkeys(PyTypeObject *type, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(iterable), &_Py_ID(value), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"iterable", "value", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "fromkeys",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *seq;
    PyObject *value = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    seq = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    value = args[1];
skip_optional_pos:
    return_value = OrderedDict_fromkeys_impl(type, seq, value);

exit:
    return return_value;
}

PyDoc_STRVAR(OrderedDict_setdefault__doc__,
"setdefault($self, /, key, default=None)\n"
"--\n"
"\n"
"Insert key with a value of default if key is not in the dictionary.\n"
"\n"
"Return the value for key if key is in the dictionary, else default.");

#define ORDEREDDICT_SETDEFAULT_METHODDEF    \
    {"setdefault", _PyCFunction_CAST(OrderedDict_setdefault), METH_FASTCALL|METH_KEYWORDS, OrderedDict_setdefault__doc__},

static PyObject *
OrderedDict_setdefault_impl(PyODictObject *self, PyObject *key,
                            PyObject *default_value);

static PyObject *
OrderedDict_setdefault(PyODictObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
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
        .fname = "setdefault",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *key;
    PyObject *default_value = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    key = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    default_value = args[1];
skip_optional_pos:
    return_value = OrderedDict_setdefault_impl(self, key, default_value);

exit:
    return return_value;
}

PyDoc_STRVAR(OrderedDict_pop__doc__,
"pop($self, /, key, default=<unrepresentable>)\n"
"--\n"
"\n"
"od.pop(key[,default]) -> v, remove specified key and return the corresponding value.\n"
"\n"
"If the key is not found, return the default if given; otherwise,\n"
"raise a KeyError.");

#define ORDEREDDICT_POP_METHODDEF    \
    {"pop", _PyCFunction_CAST(OrderedDict_pop), METH_FASTCALL|METH_KEYWORDS, OrderedDict_pop__doc__},

static PyObject *
OrderedDict_pop_impl(PyODictObject *self, PyObject *key,
                     PyObject *default_value);

static PyObject *
OrderedDict_pop(PyODictObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
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
        .fname = "pop",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *key;
    PyObject *default_value = NULL;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    key = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    default_value = args[1];
skip_optional_pos:
    return_value = OrderedDict_pop_impl(self, key, default_value);

exit:
    return return_value;
}

PyDoc_STRVAR(OrderedDict_popitem__doc__,
"popitem($self, /, last=True)\n"
"--\n"
"\n"
"Remove and return a (key, value) pair from the dictionary.\n"
"\n"
"Pairs are returned in LIFO order if last is true or FIFO order if false.");

#define ORDEREDDICT_POPITEM_METHODDEF    \
    {"popitem", _PyCFunction_CAST(OrderedDict_popitem), METH_FASTCALL|METH_KEYWORDS, OrderedDict_popitem__doc__},

static PyObject *
OrderedDict_popitem_impl(PyODictObject *self, int last);

static PyObject *
OrderedDict_popitem(PyODictObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(last), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"last", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "popitem",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    int last = 1;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    last = PyObject_IsTrue(args[0]);
    if (last < 0) {
        goto exit;
    }
skip_optional_pos:
    return_value = OrderedDict_popitem_impl(self, last);

exit:
    return return_value;
}

PyDoc_STRVAR(OrderedDict_move_to_end__doc__,
"move_to_end($self, /, key, last=True)\n"
"--\n"
"\n"
"Move an existing element to the end (or beginning if last is false).\n"
"\n"
"Raise KeyError if the element does not exist.");

#define ORDEREDDICT_MOVE_TO_END_METHODDEF    \
    {"move_to_end", _PyCFunction_CAST(OrderedDict_move_to_end), METH_FASTCALL|METH_KEYWORDS, OrderedDict_move_to_end__doc__},

static PyObject *
OrderedDict_move_to_end_impl(PyODictObject *self, PyObject *key, int last);

static PyObject *
OrderedDict_move_to_end(PyODictObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(key), &_Py_ID(last), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"key", "last", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "move_to_end",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *key;
    int last = 1;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    key = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    last = PyObject_IsTrue(args[1]);
    if (last < 0) {
        goto exit;
    }
skip_optional_pos:
    return_value = OrderedDict_move_to_end_impl(self, key, last);

exit:
    return return_value;
}
/*[clinic end generated code: output=eff78d2a3f9379bd input=a9049054013a1b77]*/
