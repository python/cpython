/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()

PyDoc_STRVAR(memoryview__doc__,
"memoryview(object)\n"
"--\n"
"\n"
"Create a new memoryview object which references the given object.");

static PyObject *
memoryview_impl(PyTypeObject *type, PyObject *object);

static PyObject *
memoryview(PyTypeObject *type, PyObject *args, PyObject *kwargs)
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
        .ob_item = { &_Py_ID(object), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"object", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "memoryview",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    PyObject *object;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser, 1, 1, 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    object = fastargs[0];
    return_value = memoryview_impl(type, object);

exit:
    return return_value;
}

PyDoc_STRVAR(memoryview__from_flags__doc__,
"_from_flags($type, /, object, flags)\n"
"--\n"
"\n"
"Create a new memoryview object which references the given object.");

#define MEMORYVIEW__FROM_FLAGS_METHODDEF    \
    {"_from_flags", _PyCFunction_CAST(memoryview__from_flags), METH_FASTCALL|METH_KEYWORDS|METH_CLASS, memoryview__from_flags__doc__},

static PyObject *
memoryview__from_flags_impl(PyTypeObject *type, PyObject *object, int flags);

static PyObject *
memoryview__from_flags(PyTypeObject *type, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(object), &_Py_ID(flags), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"object", "flags", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "_from_flags",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    PyObject *object;
    int flags;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    object = args[0];
    flags = PyLong_AsInt(args[1]);
    if (flags == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = memoryview__from_flags_impl(type, object, flags);

exit:
    return return_value;
}

PyDoc_STRVAR(memoryview_release__doc__,
"release($self, /)\n"
"--\n"
"\n"
"Release the underlying buffer exposed by the memoryview object.");

#define MEMORYVIEW_RELEASE_METHODDEF    \
    {"release", (PyCFunction)memoryview_release, METH_NOARGS, memoryview_release__doc__},

static PyObject *
memoryview_release_impl(PyMemoryViewObject *self);

static PyObject *
memoryview_release(PyMemoryViewObject *self, PyObject *Py_UNUSED(ignored))
{
    return memoryview_release_impl(self);
}

PyDoc_STRVAR(memoryview_cast__doc__,
"cast($self, /, format, shape=<unrepresentable>)\n"
"--\n"
"\n"
"Cast a memoryview to a new format or shape.");

#define MEMORYVIEW_CAST_METHODDEF    \
    {"cast", _PyCFunction_CAST(memoryview_cast), METH_FASTCALL|METH_KEYWORDS, memoryview_cast__doc__},

static PyObject *
memoryview_cast_impl(PyMemoryViewObject *self, PyObject *format,
                     PyObject *shape);

static PyObject *
memoryview_cast(PyMemoryViewObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(format), &_Py_ID(shape), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"format", "shape", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "cast",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *format;
    PyObject *shape = NULL;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("cast", "argument 'format'", "str", args[0]);
        goto exit;
    }
    format = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    shape = args[1];
skip_optional_pos:
    return_value = memoryview_cast_impl(self, format, shape);

exit:
    return return_value;
}

PyDoc_STRVAR(memoryview_toreadonly__doc__,
"toreadonly($self, /)\n"
"--\n"
"\n"
"Return a readonly version of the memoryview.");

#define MEMORYVIEW_TOREADONLY_METHODDEF    \
    {"toreadonly", (PyCFunction)memoryview_toreadonly, METH_NOARGS, memoryview_toreadonly__doc__},

static PyObject *
memoryview_toreadonly_impl(PyMemoryViewObject *self);

static PyObject *
memoryview_toreadonly(PyMemoryViewObject *self, PyObject *Py_UNUSED(ignored))
{
    return memoryview_toreadonly_impl(self);
}

PyDoc_STRVAR(memoryview_tolist__doc__,
"tolist($self, /)\n"
"--\n"
"\n"
"Return the data in the buffer as a list of elements.");

#define MEMORYVIEW_TOLIST_METHODDEF    \
    {"tolist", (PyCFunction)memoryview_tolist, METH_NOARGS, memoryview_tolist__doc__},

static PyObject *
memoryview_tolist_impl(PyMemoryViewObject *self);

static PyObject *
memoryview_tolist(PyMemoryViewObject *self, PyObject *Py_UNUSED(ignored))
{
    return memoryview_tolist_impl(self);
}

PyDoc_STRVAR(memoryview_tobytes__doc__,
"tobytes($self, /, order=\'C\')\n"
"--\n"
"\n"
"Return the data in the buffer as a byte string.\n"
"\n"
"Order can be {\'C\', \'F\', \'A\'}. When order is \'C\' or \'F\', the data of the\n"
"original array is converted to C or Fortran order. For contiguous views,\n"
"\'A\' returns an exact copy of the physical memory. In particular, in-memory\n"
"Fortran order is preserved. For non-contiguous views, the data is converted\n"
"to C first. order=None is the same as order=\'C\'.");

#define MEMORYVIEW_TOBYTES_METHODDEF    \
    {"tobytes", _PyCFunction_CAST(memoryview_tobytes), METH_FASTCALL|METH_KEYWORDS, memoryview_tobytes__doc__},

static PyObject *
memoryview_tobytes_impl(PyMemoryViewObject *self, const char *order);

static PyObject *
memoryview_tobytes(PyMemoryViewObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(order), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"order", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "tobytes",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    const char *order = NULL;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[0] == Py_None) {
        order = NULL;
    }
    else if (PyUnicode_Check(args[0])) {
        Py_ssize_t order_length;
        order = PyUnicode_AsUTF8AndSize(args[0], &order_length);
        if (order == NULL) {
            goto exit;
        }
        if (strlen(order) != (size_t)order_length) {
            PyErr_SetString(PyExc_ValueError, "embedded null character");
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("tobytes", "argument 'order'", "str or None", args[0]);
        goto exit;
    }
skip_optional_pos:
    return_value = memoryview_tobytes_impl(self, order);

exit:
    return return_value;
}

PyDoc_STRVAR(memoryview_hex__doc__,
"hex($self, /, sep=<unrepresentable>, bytes_per_sep=1)\n"
"--\n"
"\n"
"Return the data in the buffer as a str of hexadecimal numbers.\n"
"\n"
"  sep\n"
"    An optional single character or byte to separate hex bytes.\n"
"  bytes_per_sep\n"
"    How many bytes between separators.  Positive values count from the\n"
"    right, negative values count from the left.\n"
"\n"
"Example:\n"
">>> value = memoryview(b\'\\xb9\\x01\\xef\')\n"
">>> value.hex()\n"
"\'b901ef\'\n"
">>> value.hex(\':\')\n"
"\'b9:01:ef\'\n"
">>> value.hex(\':\', 2)\n"
"\'b9:01ef\'\n"
">>> value.hex(\':\', -2)\n"
"\'b901:ef\'");

#define MEMORYVIEW_HEX_METHODDEF    \
    {"hex", _PyCFunction_CAST(memoryview_hex), METH_FASTCALL|METH_KEYWORDS, memoryview_hex__doc__},

static PyObject *
memoryview_hex_impl(PyMemoryViewObject *self, PyObject *sep,
                    int bytes_per_sep);

static PyObject *
memoryview_hex(PyMemoryViewObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(sep), &_Py_ID(bytes_per_sep), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"sep", "bytes_per_sep", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "hex",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *sep = NULL;
    int bytes_per_sep = 1;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[0]) {
        sep = args[0];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    bytes_per_sep = PyLong_AsInt(args[1]);
    if (bytes_per_sep == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional_pos:
    return_value = memoryview_hex_impl(self, sep, bytes_per_sep);

exit:
    return return_value;
}
/*[clinic end generated code: output=7e76a09106921ba2 input=a9049054013a1b77]*/
