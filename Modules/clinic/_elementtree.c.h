/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(_elementtree_Element_append__doc__,
"append($self, subelement, /)\n"
"--\n"
"\n");

#define _ELEMENTTREE_ELEMENT_APPEND_METHODDEF    \
    {"append", (PyCFunction)_elementtree_Element_append, METH_O, _elementtree_Element_append__doc__},

static PyObject *
_elementtree_Element_append_impl(ElementObject *self, PyObject *subelement);

static PyObject *
_elementtree_Element_append(ElementObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    PyObject *subelement;

    if (!PyObject_TypeCheck(arg, &Element_Type)) {
        _PyArg_BadArgument("append", "argument", (&Element_Type)->tp_name, arg);
        goto exit;
    }
    subelement = arg;
    return_value = _elementtree_Element_append_impl(self, subelement);

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
_elementtree_Element_clear(ElementObject *self, PyObject *Py_UNUSED(ignored))
{
    return _elementtree_Element_clear_impl(self);
}

PyDoc_STRVAR(_elementtree_Element___copy____doc__,
"__copy__($self, /)\n"
"--\n"
"\n");

#define _ELEMENTTREE_ELEMENT___COPY___METHODDEF    \
    {"__copy__", (PyCFunction)_elementtree_Element___copy__, METH_NOARGS, _elementtree_Element___copy____doc__},

static PyObject *
_elementtree_Element___copy___impl(ElementObject *self);

static PyObject *
_elementtree_Element___copy__(ElementObject *self, PyObject *Py_UNUSED(ignored))
{
    return _elementtree_Element___copy___impl(self);
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
_elementtree_Element___deepcopy__(ElementObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    PyObject *memo;

    if (!PyDict_Check(arg)) {
        _PyArg_BadArgument("__deepcopy__", "argument", "dict", arg);
        goto exit;
    }
    memo = arg;
    return_value = _elementtree_Element___deepcopy___impl(self, memo);

exit:
    return return_value;
}

PyDoc_STRVAR(_elementtree_Element___sizeof____doc__,
"__sizeof__($self, /)\n"
"--\n"
"\n");

#define _ELEMENTTREE_ELEMENT___SIZEOF___METHODDEF    \
    {"__sizeof__", (PyCFunction)_elementtree_Element___sizeof__, METH_NOARGS, _elementtree_Element___sizeof____doc__},

static Py_ssize_t
_elementtree_Element___sizeof___impl(ElementObject *self);

static PyObject *
_elementtree_Element___sizeof__(ElementObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;
    Py_ssize_t _return_value;

    _return_value = _elementtree_Element___sizeof___impl(self);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromSsize_t(_return_value);

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
_elementtree_Element___getstate__(ElementObject *self, PyObject *Py_UNUSED(ignored))
{
    return _elementtree_Element___getstate___impl(self);
}

PyDoc_STRVAR(_elementtree_Element___setstate____doc__,
"__setstate__($self, state, /)\n"
"--\n"
"\n");

#define _ELEMENTTREE_ELEMENT___SETSTATE___METHODDEF    \
    {"__setstate__", (PyCFunction)_elementtree_Element___setstate__, METH_O, _elementtree_Element___setstate____doc__},

PyDoc_STRVAR(_elementtree_Element_extend__doc__,
"extend($self, elements, /)\n"
"--\n"
"\n");

#define _ELEMENTTREE_ELEMENT_EXTEND_METHODDEF    \
    {"extend", (PyCFunction)_elementtree_Element_extend, METH_O, _elementtree_Element_extend__doc__},

PyDoc_STRVAR(_elementtree_Element_find__doc__,
"find($self, /, path, namespaces=None)\n"
"--\n"
"\n");

#define _ELEMENTTREE_ELEMENT_FIND_METHODDEF    \
    {"find", (PyCFunction)(void(*)(void))_elementtree_Element_find, METH_FASTCALL|METH_KEYWORDS, _elementtree_Element_find__doc__},

static PyObject *
_elementtree_Element_find_impl(ElementObject *self, PyObject *path,
                               PyObject *namespaces);

static PyObject *
_elementtree_Element_find(ElementObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"path", "namespaces", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "find", 0};
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *path;
    PyObject *namespaces = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    path = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    namespaces = args[1];
skip_optional_pos:
    return_value = _elementtree_Element_find_impl(self, path, namespaces);

exit:
    return return_value;
}

PyDoc_STRVAR(_elementtree_Element_findtext__doc__,
"findtext($self, /, path, default=None, namespaces=None)\n"
"--\n"
"\n");

#define _ELEMENTTREE_ELEMENT_FINDTEXT_METHODDEF    \
    {"findtext", (PyCFunction)(void(*)(void))_elementtree_Element_findtext, METH_FASTCALL|METH_KEYWORDS, _elementtree_Element_findtext__doc__},

static PyObject *
_elementtree_Element_findtext_impl(ElementObject *self, PyObject *path,
                                   PyObject *default_value,
                                   PyObject *namespaces);

static PyObject *
_elementtree_Element_findtext(ElementObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"path", "default", "namespaces", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "findtext", 0};
    PyObject *argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *path;
    PyObject *default_value = Py_None;
    PyObject *namespaces = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 3, 0, argsbuf);
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
    return_value = _elementtree_Element_findtext_impl(self, path, default_value, namespaces);

exit:
    return return_value;
}

PyDoc_STRVAR(_elementtree_Element_findall__doc__,
"findall($self, /, path, namespaces=None)\n"
"--\n"
"\n");

#define _ELEMENTTREE_ELEMENT_FINDALL_METHODDEF    \
    {"findall", (PyCFunction)(void(*)(void))_elementtree_Element_findall, METH_FASTCALL|METH_KEYWORDS, _elementtree_Element_findall__doc__},

static PyObject *
_elementtree_Element_findall_impl(ElementObject *self, PyObject *path,
                                  PyObject *namespaces);

static PyObject *
_elementtree_Element_findall(ElementObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"path", "namespaces", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "findall", 0};
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *path;
    PyObject *namespaces = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    path = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    namespaces = args[1];
skip_optional_pos:
    return_value = _elementtree_Element_findall_impl(self, path, namespaces);

exit:
    return return_value;
}

PyDoc_STRVAR(_elementtree_Element_iterfind__doc__,
"iterfind($self, /, path, namespaces=None)\n"
"--\n"
"\n");

#define _ELEMENTTREE_ELEMENT_ITERFIND_METHODDEF    \
    {"iterfind", (PyCFunction)(void(*)(void))_elementtree_Element_iterfind, METH_FASTCALL|METH_KEYWORDS, _elementtree_Element_iterfind__doc__},

static PyObject *
_elementtree_Element_iterfind_impl(ElementObject *self, PyObject *path,
                                   PyObject *namespaces);

static PyObject *
_elementtree_Element_iterfind(ElementObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"path", "namespaces", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "iterfind", 0};
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *path;
    PyObject *namespaces = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    path = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    namespaces = args[1];
skip_optional_pos:
    return_value = _elementtree_Element_iterfind_impl(self, path, namespaces);

exit:
    return return_value;
}

PyDoc_STRVAR(_elementtree_Element_get__doc__,
"get($self, /, key, default=None)\n"
"--\n"
"\n");

#define _ELEMENTTREE_ELEMENT_GET_METHODDEF    \
    {"get", (PyCFunction)(void(*)(void))_elementtree_Element_get, METH_FASTCALL|METH_KEYWORDS, _elementtree_Element_get__doc__},

static PyObject *
_elementtree_Element_get_impl(ElementObject *self, PyObject *key,
                              PyObject *default_value);

static PyObject *
_elementtree_Element_get(ElementObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"key", "default", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "get", 0};
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
    return_value = _elementtree_Element_get_impl(self, key, default_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_elementtree_Element_iter__doc__,
"iter($self, /, tag=None)\n"
"--\n"
"\n");

#define _ELEMENTTREE_ELEMENT_ITER_METHODDEF    \
    {"iter", (PyCFunction)(void(*)(void))_elementtree_Element_iter, METH_FASTCALL|METH_KEYWORDS, _elementtree_Element_iter__doc__},

static PyObject *
_elementtree_Element_iter_impl(ElementObject *self, PyObject *tag);

static PyObject *
_elementtree_Element_iter(ElementObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"tag", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "iter", 0};
    PyObject *argsbuf[1];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *tag = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    tag = args[0];
skip_optional_pos:
    return_value = _elementtree_Element_iter_impl(self, tag);

exit:
    return return_value;
}

PyDoc_STRVAR(_elementtree_Element_itertext__doc__,
"itertext($self, /)\n"
"--\n"
"\n");

#define _ELEMENTTREE_ELEMENT_ITERTEXT_METHODDEF    \
    {"itertext", (PyCFunction)_elementtree_Element_itertext, METH_NOARGS, _elementtree_Element_itertext__doc__},

static PyObject *
_elementtree_Element_itertext_impl(ElementObject *self);

static PyObject *
_elementtree_Element_itertext(ElementObject *self, PyObject *Py_UNUSED(ignored))
{
    return _elementtree_Element_itertext_impl(self);
}

PyDoc_STRVAR(_elementtree_Element_insert__doc__,
"insert($self, index, subelement, /)\n"
"--\n"
"\n");

#define _ELEMENTTREE_ELEMENT_INSERT_METHODDEF    \
    {"insert", (PyCFunction)(void(*)(void))_elementtree_Element_insert, METH_FASTCALL, _elementtree_Element_insert__doc__},

static PyObject *
_elementtree_Element_insert_impl(ElementObject *self, Py_ssize_t index,
                                 PyObject *subelement);

static PyObject *
_elementtree_Element_insert(ElementObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_ssize_t index;
    PyObject *subelement;

    if (!_PyArg_CheckPositional("insert", nargs, 2, 2)) {
        goto exit;
    }
    if (PyFloat_Check(args[0])) {
        PyErr_SetString(PyExc_TypeError,
                        "integer argument expected, got float" );
        goto exit;
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = PyNumber_Index(args[0]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        index = ival;
    }
    if (!PyObject_TypeCheck(args[1], &Element_Type)) {
        _PyArg_BadArgument("insert", "argument 2", (&Element_Type)->tp_name, args[1]);
        goto exit;
    }
    subelement = args[1];
    return_value = _elementtree_Element_insert_impl(self, index, subelement);

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
_elementtree_Element_items(ElementObject *self, PyObject *Py_UNUSED(ignored))
{
    return _elementtree_Element_items_impl(self);
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
_elementtree_Element_keys(ElementObject *self, PyObject *Py_UNUSED(ignored))
{
    return _elementtree_Element_keys_impl(self);
}

PyDoc_STRVAR(_elementtree_Element_makeelement__doc__,
"makeelement($self, tag, attrib, /)\n"
"--\n"
"\n");

#define _ELEMENTTREE_ELEMENT_MAKEELEMENT_METHODDEF    \
    {"makeelement", (PyCFunction)(void(*)(void))_elementtree_Element_makeelement, METH_FASTCALL, _elementtree_Element_makeelement__doc__},

static PyObject *
_elementtree_Element_makeelement_impl(ElementObject *self, PyObject *tag,
                                      PyObject *attrib);

static PyObject *
_elementtree_Element_makeelement(ElementObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *tag;
    PyObject *attrib;

    if (!_PyArg_CheckPositional("makeelement", nargs, 2, 2)) {
        goto exit;
    }
    tag = args[0];
    attrib = args[1];
    return_value = _elementtree_Element_makeelement_impl(self, tag, attrib);

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
_elementtree_Element_remove(ElementObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    PyObject *subelement;

    if (!PyObject_TypeCheck(arg, &Element_Type)) {
        _PyArg_BadArgument("remove", "argument", (&Element_Type)->tp_name, arg);
        goto exit;
    }
    subelement = arg;
    return_value = _elementtree_Element_remove_impl(self, subelement);

exit:
    return return_value;
}

PyDoc_STRVAR(_elementtree_Element_set__doc__,
"set($self, key, value, /)\n"
"--\n"
"\n");

#define _ELEMENTTREE_ELEMENT_SET_METHODDEF    \
    {"set", (PyCFunction)(void(*)(void))_elementtree_Element_set, METH_FASTCALL, _elementtree_Element_set__doc__},

static PyObject *
_elementtree_Element_set_impl(ElementObject *self, PyObject *key,
                              PyObject *value);

static PyObject *
_elementtree_Element_set(ElementObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *key;
    PyObject *value;

    if (!_PyArg_CheckPositional("set", nargs, 2, 2)) {
        goto exit;
    }
    key = args[0];
    value = args[1];
    return_value = _elementtree_Element_set_impl(self, key, value);

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
    static const char * const _keywords[] = {"element_factory", "comment_factory", "pi_factory", "insert_comments", "insert_pis", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "TreeBuilder", 0};
    PyObject *argsbuf[5];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 0;
    PyObject *element_factory = Py_None;
    PyObject *comment_factory = Py_None;
    PyObject *pi_factory = Py_None;
    int insert_comments = 0;
    int insert_pis = 0;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser, 0, 1, 0, argsbuf);
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
    {"_set_factories", (PyCFunction)(void(*)(void))_elementtree__set_factories, METH_FASTCALL, _elementtree__set_factories__doc__},

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

PyDoc_STRVAR(_elementtree_TreeBuilder_end__doc__,
"end($self, tag, /)\n"
"--\n"
"\n");

#define _ELEMENTTREE_TREEBUILDER_END_METHODDEF    \
    {"end", (PyCFunction)_elementtree_TreeBuilder_end, METH_O, _elementtree_TreeBuilder_end__doc__},

PyDoc_STRVAR(_elementtree_TreeBuilder_comment__doc__,
"comment($self, text, /)\n"
"--\n"
"\n");

#define _ELEMENTTREE_TREEBUILDER_COMMENT_METHODDEF    \
    {"comment", (PyCFunction)_elementtree_TreeBuilder_comment, METH_O, _elementtree_TreeBuilder_comment__doc__},

PyDoc_STRVAR(_elementtree_TreeBuilder_pi__doc__,
"pi($self, target, text=None, /)\n"
"--\n"
"\n");

#define _ELEMENTTREE_TREEBUILDER_PI_METHODDEF    \
    {"pi", (PyCFunction)(void(*)(void))_elementtree_TreeBuilder_pi, METH_FASTCALL, _elementtree_TreeBuilder_pi__doc__},

static PyObject *
_elementtree_TreeBuilder_pi_impl(TreeBuilderObject *self, PyObject *target,
                                 PyObject *text);

static PyObject *
_elementtree_TreeBuilder_pi(TreeBuilderObject *self, PyObject *const *args, Py_ssize_t nargs)
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
    return_value = _elementtree_TreeBuilder_pi_impl(self, target, text);

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
_elementtree_TreeBuilder_close(TreeBuilderObject *self, PyObject *Py_UNUSED(ignored))
{
    return _elementtree_TreeBuilder_close_impl(self);
}

PyDoc_STRVAR(_elementtree_TreeBuilder_start__doc__,
"start($self, tag, attrs=None, /)\n"
"--\n"
"\n");

#define _ELEMENTTREE_TREEBUILDER_START_METHODDEF    \
    {"start", (PyCFunction)(void(*)(void))_elementtree_TreeBuilder_start, METH_FASTCALL, _elementtree_TreeBuilder_start__doc__},

static PyObject *
_elementtree_TreeBuilder_start_impl(TreeBuilderObject *self, PyObject *tag,
                                    PyObject *attrs);

static PyObject *
_elementtree_TreeBuilder_start(TreeBuilderObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *tag;
    PyObject *attrs = Py_None;

    if (!_PyArg_CheckPositional("start", nargs, 1, 2)) {
        goto exit;
    }
    tag = args[0];
    if (nargs < 2) {
        goto skip_optional;
    }
    attrs = args[1];
skip_optional:
    return_value = _elementtree_TreeBuilder_start_impl(self, tag, attrs);

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
    static const char * const _keywords[] = {"target", "encoding", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "XMLParser", 0};
    PyObject *argsbuf[2];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 0;
    PyObject *target = NULL;
    const char *encoding = NULL;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser, 0, 0, 0, argsbuf);
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
_elementtree_XMLParser_close(XMLParserObject *self, PyObject *Py_UNUSED(ignored))
{
    return _elementtree_XMLParser_close_impl(self);
}

PyDoc_STRVAR(_elementtree_XMLParser_feed__doc__,
"feed($self, data, /)\n"
"--\n"
"\n");

#define _ELEMENTTREE_XMLPARSER_FEED_METHODDEF    \
    {"feed", (PyCFunction)_elementtree_XMLParser_feed, METH_O, _elementtree_XMLParser_feed__doc__},

PyDoc_STRVAR(_elementtree_XMLParser__parse_whole__doc__,
"_parse_whole($self, file, /)\n"
"--\n"
"\n");

#define _ELEMENTTREE_XMLPARSER__PARSE_WHOLE_METHODDEF    \
    {"_parse_whole", (PyCFunction)_elementtree_XMLParser__parse_whole, METH_O, _elementtree_XMLParser__parse_whole__doc__},

PyDoc_STRVAR(_elementtree_XMLParser__setevents__doc__,
"_setevents($self, events_queue, events_to_report=None, /)\n"
"--\n"
"\n");

#define _ELEMENTTREE_XMLPARSER__SETEVENTS_METHODDEF    \
    {"_setevents", (PyCFunction)(void(*)(void))_elementtree_XMLParser__setevents, METH_FASTCALL, _elementtree_XMLParser__setevents__doc__},

static PyObject *
_elementtree_XMLParser__setevents_impl(XMLParserObject *self,
                                       PyObject *events_queue,
                                       PyObject *events_to_report);

static PyObject *
_elementtree_XMLParser__setevents(XMLParserObject *self, PyObject *const *args, Py_ssize_t nargs)
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
    return_value = _elementtree_XMLParser__setevents_impl(self, events_queue, events_to_report);

exit:
    return return_value;
}
/*[clinic end generated code: output=bee26d0735a3fddc input=a9049054013a1b77]*/
