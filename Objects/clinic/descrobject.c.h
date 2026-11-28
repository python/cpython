/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#endif
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()
#include "pycore_runtime.h"       // _Py_SINGLETON()

PyDoc_STRVAR(mappingproxy_new__doc__,
"mappingproxy(mapping)\n"
"--\n"
"\n"
"Read-only proxy of a mapping.");

static PyObject *
mappingproxy_new_impl(PyTypeObject *type, PyObject *mapping);

static PyObject *
mappingproxy_new_helper(PyTypeObject *type, PyObject *const *args,
    Py_ssize_t nargs, Py_ssize_t nkw, PyObject *kwargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(mapping), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"mapping", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "mappingproxy",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject * const *fastargs;
    PyObject *mapping;

    fastargs = _PyArg_UnpackKeywords(args, nargs, kwargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    mapping = fastargs[0];
    return_value = mappingproxy_new_impl(type, mapping);

exit:
    return return_value;
}

static PyObject *
mappingproxy_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    return mappingproxy_new_helper(type, _PyTuple_CAST(args)->ob_item,
        PyTuple_GET_SIZE(args),
        kwargs ? PyDict_GET_SIZE(kwargs) : 0,
        kwargs, NULL);
}

static PyObject *
mappingproxy_vectorcall(PyObject *type, PyObject *const *args,
    size_t nargsf, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    PyObject *mapping;

    assert(_PyType_CAST(type)->tp_new == mappingproxy_new);
    /* Make sure the type object is immutable: the generated
     * vectorcall doesn't deal e.g. with users reassigning __init__. */
    assert(PyType_HasFeature(_PyType_CAST(type), Py_TPFLAGS_IMMUTABLETYPE));
    if (kwnames != NULL || nargs < 1 || nargs > 1) {
        return mappingproxy_new_helper(_PyType_CAST(type), args, nargs,
            kwnames ? PyTuple_GET_SIZE(kwnames) : 0,
            NULL, kwnames);
    }
    mapping = args[0];
    return_value = mappingproxy_new_impl(_PyType_CAST(type), mapping);

    return return_value;
}

PyDoc_STRVAR(property___set_name____doc__,
"__set_name__($self, owner, name, /)\n"
"--\n"
"\n"
"Method to set name of a property.");

#define PROPERTY___SET_NAME___METHODDEF    \
    {"__set_name__", _PyCFunction_CAST(property___set_name__), METH_FASTCALL, property___set_name____doc__},

static PyObject *
property___set_name___impl(propertyobject *self, PyObject *owner,
                           PyObject *name);

static PyObject *
property___set_name__(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *owner;
    PyObject *name;

    if (!_PyArg_CheckPositional("__set_name__", nargs, 2, 2)) {
        goto exit;
    }
    owner = args[0];
    name = args[1];
    return_value = property___set_name___impl((propertyobject *)self, owner, name);

exit:
    return return_value;
}

PyDoc_STRVAR(property_init__doc__,
"property(fget=None, fset=None, fdel=None, doc=None)\n"
"--\n"
"\n"
"Property attribute.\n"
"\n"
"  fget\n"
"    function to be used for getting an attribute value\n"
"  fset\n"
"    function to be used for setting an attribute value\n"
"  fdel\n"
"    function to be used for del\'ing an attribute\n"
"  doc\n"
"    docstring\n"
"\n"
"Typical use is to define a managed attribute x:\n"
"\n"
"class C(object):\n"
"    def getx(self): return self._x\n"
"    def setx(self, value): self._x = value\n"
"    def delx(self): del self._x\n"
"    x = property(getx, setx, delx, \"I\'m the \'x\' property.\")\n"
"\n"
"Decorators make defining new properties or modifying existing ones easy:\n"
"\n"
"class C(object):\n"
"    @property\n"
"    def x(self):\n"
"        \"I am the \'x\' property.\"\n"
"        return self._x\n"
"    @x.setter\n"
"    def x(self, value):\n"
"        self._x = value\n"
"    @x.deleter\n"
"    def x(self):\n"
"        del self._x");

static int
property_init_impl(propertyobject *self, PyObject *fget, PyObject *fset,
                   PyObject *fdel, PyObject *doc);

static int
property_init_helper(PyObject *self, PyObject *const *args,
    Py_ssize_t nargs, Py_ssize_t nkw, PyObject *kwargs, PyObject *kwnames)
{
    int return_value = -1;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 4
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(fget), &_Py_ID(fset), &_Py_ID(fdel), &_Py_ID(doc), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"fget", "fset", "fdel", "doc", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "property",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[4];
    PyObject * const *fastargs;
    Py_ssize_t noptargs = nargs + nkw - 0;
    PyObject *fget = NULL;
    PyObject *fset = NULL;
    PyObject *fdel = NULL;
    PyObject *doc = NULL;

    fastargs = _PyArg_UnpackKeywords(args, nargs, kwargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 4, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (fastargs[0]) {
        fget = fastargs[0];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (fastargs[1]) {
        fset = fastargs[1];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (fastargs[2]) {
        fdel = fastargs[2];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    doc = fastargs[3];
skip_optional_pos:
    return_value = property_init_impl((propertyobject *)self, fget, fset, fdel, doc);

exit:
    return return_value;
}

static int
property_init(PyObject *self, PyObject *args, PyObject *kwargs)
{
    return property_init_helper(self, _PyTuple_CAST(args)->ob_item,
        PyTuple_GET_SIZE(args),
        kwargs ? PyDict_GET_SIZE(kwargs) : 0,
        kwargs, NULL);
}

static PyObject *
property_vectorcall(PyObject *type, PyObject *const *args,
    size_t nargsf, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    PyObject *self;
    int _result;
    PyObject *fget = NULL;
    PyObject *fset = NULL;
    PyObject *fdel = NULL;
    PyObject *doc = NULL;

    assert(_PyType_CAST(type)->tp_init == property_init);
    /* Make sure the type object is immutable: the generated
     * vectorcall doesn't deal e.g. with users reassigning __init__. */
    assert(PyType_HasFeature(_PyType_CAST(type), Py_TPFLAGS_IMMUTABLETYPE));
    if (kwnames != NULL || nargs > 4) {
        self = _PyType_CAST(type)->tp_new(_PyType_CAST(type),
            (PyObject *)&_Py_SINGLETON(tuple_empty), NULL);
        if (self == NULL) {
            return NULL;
        }
        _result = property_init_helper(self, args, nargs,
            kwnames ? PyTuple_GET_SIZE(kwnames) : 0,
            NULL, kwnames);
        if (_result != 0) {
            Py_DECREF(self);
            return NULL;
        }
        return self;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    fget = args[0];
    if (nargs < 2) {
        goto skip_optional;
    }
    fset = args[1];
    if (nargs < 3) {
        goto skip_optional;
    }
    fdel = args[2];
    if (nargs < 4) {
        goto skip_optional;
    }
    doc = args[3];
skip_optional:
    self = _PyType_CAST(type)->tp_new(_PyType_CAST(type),
        (PyObject *)&_Py_SINGLETON(tuple_empty), NULL);
    if (self == NULL) {
        goto exit;
    }
    _result = property_init_impl((propertyobject *)self, fget, fset, fdel, doc);
    if (_result != 0) {
        Py_DECREF(self);
        goto exit;
    }
    return_value = self;

exit:
    return return_value;
}
/*[clinic end generated code: output=cd59da11d2fc4f7c input=a9049054013a1b77]*/
