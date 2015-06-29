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

    if (!PyArg_Parse(arg, "O!:append", &Element_Type, &subelement))
        goto exit;
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
    if ((_return_value == -1) && PyErr_Occurred())
        goto exit;
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
    {"find", (PyCFunction)_elementtree_Element_find, METH_VARARGS|METH_KEYWORDS, _elementtree_Element_find__doc__},

static PyObject *
_elementtree_Element_find_impl(ElementObject *self, PyObject *path,
                               PyObject *namespaces);

static PyObject *
_elementtree_Element_find(ElementObject *self, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"path", "namespaces", NULL};
    PyObject *path;
    PyObject *namespaces = Py_None;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|O:find", _keywords,
        &path, &namespaces))
        goto exit;
    return_value = _elementtree_Element_find_impl(self, path, namespaces);

exit:
    return return_value;
}

PyDoc_STRVAR(_elementtree_Element_findtext__doc__,
"findtext($self, /, path, default=None, namespaces=None)\n"
"--\n"
"\n");

#define _ELEMENTTREE_ELEMENT_FINDTEXT_METHODDEF    \
    {"findtext", (PyCFunction)_elementtree_Element_findtext, METH_VARARGS|METH_KEYWORDS, _elementtree_Element_findtext__doc__},

static PyObject *
_elementtree_Element_findtext_impl(ElementObject *self, PyObject *path,
                                   PyObject *default_value,
                                   PyObject *namespaces);

static PyObject *
_elementtree_Element_findtext(ElementObject *self, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"path", "default", "namespaces", NULL};
    PyObject *path;
    PyObject *default_value = Py_None;
    PyObject *namespaces = Py_None;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|OO:findtext", _keywords,
        &path, &default_value, &namespaces))
        goto exit;
    return_value = _elementtree_Element_findtext_impl(self, path, default_value, namespaces);

exit:
    return return_value;
}

PyDoc_STRVAR(_elementtree_Element_findall__doc__,
"findall($self, /, path, namespaces=None)\n"
"--\n"
"\n");

#define _ELEMENTTREE_ELEMENT_FINDALL_METHODDEF    \
    {"findall", (PyCFunction)_elementtree_Element_findall, METH_VARARGS|METH_KEYWORDS, _elementtree_Element_findall__doc__},

static PyObject *
_elementtree_Element_findall_impl(ElementObject *self, PyObject *path,
                                  PyObject *namespaces);

static PyObject *
_elementtree_Element_findall(ElementObject *self, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"path", "namespaces", NULL};
    PyObject *path;
    PyObject *namespaces = Py_None;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|O:findall", _keywords,
        &path, &namespaces))
        goto exit;
    return_value = _elementtree_Element_findall_impl(self, path, namespaces);

exit:
    return return_value;
}

PyDoc_STRVAR(_elementtree_Element_iterfind__doc__,
"iterfind($self, /, path, namespaces=None)\n"
"--\n"
"\n");

#define _ELEMENTTREE_ELEMENT_ITERFIND_METHODDEF    \
    {"iterfind", (PyCFunction)_elementtree_Element_iterfind, METH_VARARGS|METH_KEYWORDS, _elementtree_Element_iterfind__doc__},

static PyObject *
_elementtree_Element_iterfind_impl(ElementObject *self, PyObject *path,
                                   PyObject *namespaces);

static PyObject *
_elementtree_Element_iterfind(ElementObject *self, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"path", "namespaces", NULL};
    PyObject *path;
    PyObject *namespaces = Py_None;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|O:iterfind", _keywords,
        &path, &namespaces))
        goto exit;
    return_value = _elementtree_Element_iterfind_impl(self, path, namespaces);

exit:
    return return_value;
}

PyDoc_STRVAR(_elementtree_Element_get__doc__,
"get($self, /, key, default=None)\n"
"--\n"
"\n");

#define _ELEMENTTREE_ELEMENT_GET_METHODDEF    \
    {"get", (PyCFunction)_elementtree_Element_get, METH_VARARGS|METH_KEYWORDS, _elementtree_Element_get__doc__},

static PyObject *
_elementtree_Element_get_impl(ElementObject *self, PyObject *key,
                              PyObject *default_value);

static PyObject *
_elementtree_Element_get(ElementObject *self, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"key", "default", NULL};
    PyObject *key;
    PyObject *default_value = Py_None;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|O:get", _keywords,
        &key, &default_value))
        goto exit;
    return_value = _elementtree_Element_get_impl(self, key, default_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_elementtree_Element_getchildren__doc__,
"getchildren($self, /)\n"
"--\n"
"\n");

#define _ELEMENTTREE_ELEMENT_GETCHILDREN_METHODDEF    \
    {"getchildren", (PyCFunction)_elementtree_Element_getchildren, METH_NOARGS, _elementtree_Element_getchildren__doc__},

static PyObject *
_elementtree_Element_getchildren_impl(ElementObject *self);

static PyObject *
_elementtree_Element_getchildren(ElementObject *self, PyObject *Py_UNUSED(ignored))
{
    return _elementtree_Element_getchildren_impl(self);
}

PyDoc_STRVAR(_elementtree_Element_iter__doc__,
"iter($self, /, tag=None)\n"
"--\n"
"\n");

#define _ELEMENTTREE_ELEMENT_ITER_METHODDEF    \
    {"iter", (PyCFunction)_elementtree_Element_iter, METH_VARARGS|METH_KEYWORDS, _elementtree_Element_iter__doc__},

static PyObject *
_elementtree_Element_iter_impl(ElementObject *self, PyObject *tag);

static PyObject *
_elementtree_Element_iter(ElementObject *self, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"tag", NULL};
    PyObject *tag = Py_None;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|O:iter", _keywords,
        &tag))
        goto exit;
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
    {"insert", (PyCFunction)_elementtree_Element_insert, METH_VARARGS, _elementtree_Element_insert__doc__},

static PyObject *
_elementtree_Element_insert_impl(ElementObject *self, Py_ssize_t index,
                                 PyObject *subelement);

static PyObject *
_elementtree_Element_insert(ElementObject *self, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_ssize_t index;
    PyObject *subelement;

    if (!PyArg_ParseTuple(args, "nO!:insert",
        &index, &Element_Type, &subelement))
        goto exit;
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
    {"makeelement", (PyCFunction)_elementtree_Element_makeelement, METH_VARARGS, _elementtree_Element_makeelement__doc__},

static PyObject *
_elementtree_Element_makeelement_impl(ElementObject *self, PyObject *tag,
                                      PyObject *attrib);

static PyObject *
_elementtree_Element_makeelement(ElementObject *self, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *tag;
    PyObject *attrib;

    if (!PyArg_UnpackTuple(args, "makeelement",
        2, 2,
        &tag, &attrib))
        goto exit;
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

    if (!PyArg_Parse(arg, "O!:remove", &Element_Type, &subelement))
        goto exit;
    return_value = _elementtree_Element_remove_impl(self, subelement);

exit:
    return return_value;
}

PyDoc_STRVAR(_elementtree_Element_set__doc__,
"set($self, key, value, /)\n"
"--\n"
"\n");

#define _ELEMENTTREE_ELEMENT_SET_METHODDEF    \
    {"set", (PyCFunction)_elementtree_Element_set, METH_VARARGS, _elementtree_Element_set__doc__},

static PyObject *
_elementtree_Element_set_impl(ElementObject *self, PyObject *key,
                              PyObject *value);

static PyObject *
_elementtree_Element_set(ElementObject *self, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *key;
    PyObject *value;

    if (!PyArg_UnpackTuple(args, "set",
        2, 2,
        &key, &value))
        goto exit;
    return_value = _elementtree_Element_set_impl(self, key, value);

exit:
    return return_value;
}

static int
_elementtree_TreeBuilder___init___impl(TreeBuilderObject *self,
                                       PyObject *element_factory);

static int
_elementtree_TreeBuilder___init__(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int return_value = -1;
    static char *_keywords[] = {"element_factory", NULL};
    PyObject *element_factory = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|O:TreeBuilder", _keywords,
        &element_factory))
        goto exit;
    return_value = _elementtree_TreeBuilder___init___impl((TreeBuilderObject *)self, element_factory);

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
    {"start", (PyCFunction)_elementtree_TreeBuilder_start, METH_VARARGS, _elementtree_TreeBuilder_start__doc__},

static PyObject *
_elementtree_TreeBuilder_start_impl(TreeBuilderObject *self, PyObject *tag,
                                    PyObject *attrs);

static PyObject *
_elementtree_TreeBuilder_start(TreeBuilderObject *self, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *tag;
    PyObject *attrs = Py_None;

    if (!PyArg_UnpackTuple(args, "start",
        1, 2,
        &tag, &attrs))
        goto exit;
    return_value = _elementtree_TreeBuilder_start_impl(self, tag, attrs);

exit:
    return return_value;
}

static int
_elementtree_XMLParser___init___impl(XMLParserObject *self, PyObject *html,
                                     PyObject *target, const char *encoding);

static int
_elementtree_XMLParser___init__(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int return_value = -1;
    static char *_keywords[] = {"html", "target", "encoding", NULL};
    PyObject *html = NULL;
    PyObject *target = NULL;
    const char *encoding = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|OOz:XMLParser", _keywords,
        &html, &target, &encoding))
        goto exit;
    return_value = _elementtree_XMLParser___init___impl((XMLParserObject *)self, html, target, encoding);

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

PyDoc_STRVAR(_elementtree_XMLParser_doctype__doc__,
"doctype($self, name, pubid, system, /)\n"
"--\n"
"\n");

#define _ELEMENTTREE_XMLPARSER_DOCTYPE_METHODDEF    \
    {"doctype", (PyCFunction)_elementtree_XMLParser_doctype, METH_VARARGS, _elementtree_XMLParser_doctype__doc__},

static PyObject *
_elementtree_XMLParser_doctype_impl(XMLParserObject *self, PyObject *name,
                                    PyObject *pubid, PyObject *system);

static PyObject *
_elementtree_XMLParser_doctype(XMLParserObject *self, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *name;
    PyObject *pubid;
    PyObject *system;

    if (!PyArg_UnpackTuple(args, "doctype",
        3, 3,
        &name, &pubid, &system))
        goto exit;
    return_value = _elementtree_XMLParser_doctype_impl(self, name, pubid, system);

exit:
    return return_value;
}

PyDoc_STRVAR(_elementtree_XMLParser__setevents__doc__,
"_setevents($self, events_queue, events_to_report=None, /)\n"
"--\n"
"\n");

#define _ELEMENTTREE_XMLPARSER__SETEVENTS_METHODDEF    \
    {"_setevents", (PyCFunction)_elementtree_XMLParser__setevents, METH_VARARGS, _elementtree_XMLParser__setevents__doc__},

static PyObject *
_elementtree_XMLParser__setevents_impl(XMLParserObject *self,
                                       PyObject *events_queue,
                                       PyObject *events_to_report);

static PyObject *
_elementtree_XMLParser__setevents(XMLParserObject *self, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *events_queue;
    PyObject *events_to_report = Py_None;

    if (!PyArg_ParseTuple(args, "O!|O:_setevents",
        &PyList_Type, &events_queue, &events_to_report))
        goto exit;
    return_value = _elementtree_XMLParser__setevents_impl(self, events_queue, events_to_report);

exit:
    return return_value;
}
/*[clinic end generated code: output=25b8bf7e7f2151ca input=a9049054013a1b77]*/
