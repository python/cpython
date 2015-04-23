/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(_pickle_Pickler_clear_memo__doc__,
"clear_memo($self, /)\n"
"--\n"
"\n"
"Clears the pickler\'s \"memo\".\n"
"\n"
"The memo is the data structure that remembers which objects the\n"
"pickler has already seen, so that shared or recursive objects are\n"
"pickled by reference and not by value.  This method is useful when\n"
"re-using picklers.");

#define _PICKLE_PICKLER_CLEAR_MEMO_METHODDEF    \
    {"clear_memo", (PyCFunction)_pickle_Pickler_clear_memo, METH_NOARGS, _pickle_Pickler_clear_memo__doc__},

static PyObject *
_pickle_Pickler_clear_memo_impl(PicklerObject *self);

static PyObject *
_pickle_Pickler_clear_memo(PicklerObject *self, PyObject *Py_UNUSED(ignored))
{
    return _pickle_Pickler_clear_memo_impl(self);
}

PyDoc_STRVAR(_pickle_Pickler_dump__doc__,
"dump($self, obj, /)\n"
"--\n"
"\n"
"Write a pickled representation of the given object to the open file.");

#define _PICKLE_PICKLER_DUMP_METHODDEF    \
    {"dump", (PyCFunction)_pickle_Pickler_dump, METH_O, _pickle_Pickler_dump__doc__},

PyDoc_STRVAR(_pickle_Pickler___sizeof____doc__,
"__sizeof__($self, /)\n"
"--\n"
"\n"
"Returns size in memory, in bytes.");

#define _PICKLE_PICKLER___SIZEOF___METHODDEF    \
    {"__sizeof__", (PyCFunction)_pickle_Pickler___sizeof__, METH_NOARGS, _pickle_Pickler___sizeof____doc__},

static Py_ssize_t
_pickle_Pickler___sizeof___impl(PicklerObject *self);

static PyObject *
_pickle_Pickler___sizeof__(PicklerObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;
    Py_ssize_t _return_value;

    _return_value = _pickle_Pickler___sizeof___impl(self);
    if ((_return_value == -1) && PyErr_Occurred())
        goto exit;
    return_value = PyLong_FromSsize_t(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_pickle_Pickler___init____doc__,
"Pickler(file, protocol=None, fix_imports=True)\n"
"--\n"
"\n"
"This takes a binary file for writing a pickle data stream.\n"
"\n"
"The optional *protocol* argument tells the pickler to use the given\n"
"protocol; supported protocols are 0, 1, 2, 3 and 4.  The default\n"
"protocol is 3; a backward-incompatible protocol designed for Python 3.\n"
"\n"
"Specifying a negative protocol version selects the highest protocol\n"
"version supported.  The higher the protocol used, the more recent the\n"
"version of Python needed to read the pickle produced.\n"
"\n"
"The *file* argument must have a write() method that accepts a single\n"
"bytes argument. It can thus be a file object opened for binary\n"
"writing, a io.BytesIO instance, or any other custom object that meets\n"
"this interface.\n"
"\n"
"If *fix_imports* is True and protocol is less than 3, pickle will try\n"
"to map the new Python 3 names to the old module names used in Python\n"
"2, so that the pickle data stream is readable with Python 2.");

static int
_pickle_Pickler___init___impl(PicklerObject *self, PyObject *file,
                              PyObject *protocol, int fix_imports);

static int
_pickle_Pickler___init__(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int return_value = -1;
    static char *_keywords[] = {"file", "protocol", "fix_imports", NULL};
    PyObject *file;
    PyObject *protocol = NULL;
    int fix_imports = 1;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|Op:Pickler", _keywords,
        &file, &protocol, &fix_imports))
        goto exit;
    return_value = _pickle_Pickler___init___impl((PicklerObject *)self, file, protocol, fix_imports);

exit:
    return return_value;
}

PyDoc_STRVAR(_pickle_PicklerMemoProxy_clear__doc__,
"clear($self, /)\n"
"--\n"
"\n"
"Remove all items from memo.");

#define _PICKLE_PICKLERMEMOPROXY_CLEAR_METHODDEF    \
    {"clear", (PyCFunction)_pickle_PicklerMemoProxy_clear, METH_NOARGS, _pickle_PicklerMemoProxy_clear__doc__},

static PyObject *
_pickle_PicklerMemoProxy_clear_impl(PicklerMemoProxyObject *self);

static PyObject *
_pickle_PicklerMemoProxy_clear(PicklerMemoProxyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _pickle_PicklerMemoProxy_clear_impl(self);
}

PyDoc_STRVAR(_pickle_PicklerMemoProxy_copy__doc__,
"copy($self, /)\n"
"--\n"
"\n"
"Copy the memo to a new object.");

#define _PICKLE_PICKLERMEMOPROXY_COPY_METHODDEF    \
    {"copy", (PyCFunction)_pickle_PicklerMemoProxy_copy, METH_NOARGS, _pickle_PicklerMemoProxy_copy__doc__},

static PyObject *
_pickle_PicklerMemoProxy_copy_impl(PicklerMemoProxyObject *self);

static PyObject *
_pickle_PicklerMemoProxy_copy(PicklerMemoProxyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _pickle_PicklerMemoProxy_copy_impl(self);
}

PyDoc_STRVAR(_pickle_PicklerMemoProxy___reduce____doc__,
"__reduce__($self, /)\n"
"--\n"
"\n"
"Implement pickle support.");

#define _PICKLE_PICKLERMEMOPROXY___REDUCE___METHODDEF    \
    {"__reduce__", (PyCFunction)_pickle_PicklerMemoProxy___reduce__, METH_NOARGS, _pickle_PicklerMemoProxy___reduce____doc__},

static PyObject *
_pickle_PicklerMemoProxy___reduce___impl(PicklerMemoProxyObject *self);

static PyObject *
_pickle_PicklerMemoProxy___reduce__(PicklerMemoProxyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _pickle_PicklerMemoProxy___reduce___impl(self);
}

PyDoc_STRVAR(_pickle_Unpickler_load__doc__,
"load($self, /)\n"
"--\n"
"\n"
"Load a pickle.\n"
"\n"
"Read a pickled object representation from the open file object given\n"
"in the constructor, and return the reconstituted object hierarchy\n"
"specified therein.");

#define _PICKLE_UNPICKLER_LOAD_METHODDEF    \
    {"load", (PyCFunction)_pickle_Unpickler_load, METH_NOARGS, _pickle_Unpickler_load__doc__},

static PyObject *
_pickle_Unpickler_load_impl(UnpicklerObject *self);

static PyObject *
_pickle_Unpickler_load(UnpicklerObject *self, PyObject *Py_UNUSED(ignored))
{
    return _pickle_Unpickler_load_impl(self);
}

PyDoc_STRVAR(_pickle_Unpickler_find_class__doc__,
"find_class($self, module_name, global_name, /)\n"
"--\n"
"\n"
"Return an object from a specified module.\n"
"\n"
"If necessary, the module will be imported. Subclasses may override\n"
"this method (e.g. to restrict unpickling of arbitrary classes and\n"
"functions).\n"
"\n"
"This method is called whenever a class or a function object is\n"
"needed.  Both arguments passed are str objects.");

#define _PICKLE_UNPICKLER_FIND_CLASS_METHODDEF    \
    {"find_class", (PyCFunction)_pickle_Unpickler_find_class, METH_VARARGS, _pickle_Unpickler_find_class__doc__},

static PyObject *
_pickle_Unpickler_find_class_impl(UnpicklerObject *self,
                                  PyObject *module_name,
                                  PyObject *global_name);

static PyObject *
_pickle_Unpickler_find_class(UnpicklerObject *self, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *module_name;
    PyObject *global_name;

    if (!PyArg_UnpackTuple(args, "find_class",
        2, 2,
        &module_name, &global_name))
        goto exit;
    return_value = _pickle_Unpickler_find_class_impl(self, module_name, global_name);

exit:
    return return_value;
}

PyDoc_STRVAR(_pickle_Unpickler___sizeof____doc__,
"__sizeof__($self, /)\n"
"--\n"
"\n"
"Returns size in memory, in bytes.");

#define _PICKLE_UNPICKLER___SIZEOF___METHODDEF    \
    {"__sizeof__", (PyCFunction)_pickle_Unpickler___sizeof__, METH_NOARGS, _pickle_Unpickler___sizeof____doc__},

static Py_ssize_t
_pickle_Unpickler___sizeof___impl(UnpicklerObject *self);

static PyObject *
_pickle_Unpickler___sizeof__(UnpicklerObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;
    Py_ssize_t _return_value;

    _return_value = _pickle_Unpickler___sizeof___impl(self);
    if ((_return_value == -1) && PyErr_Occurred())
        goto exit;
    return_value = PyLong_FromSsize_t(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_pickle_Unpickler___init____doc__,
"Unpickler(file, *, fix_imports=True, encoding=\'ASCII\', errors=\'strict\')\n"
"--\n"
"\n"
"This takes a binary file for reading a pickle data stream.\n"
"\n"
"The protocol version of the pickle is detected automatically, so no\n"
"protocol argument is needed.  Bytes past the pickled object\'s\n"
"representation are ignored.\n"
"\n"
"The argument *file* must have two methods, a read() method that takes\n"
"an integer argument, and a readline() method that requires no\n"
"arguments.  Both methods should return bytes.  Thus *file* can be a\n"
"binary file object opened for reading, a io.BytesIO object, or any\n"
"other custom object that meets this interface.\n"
"\n"
"Optional keyword arguments are *fix_imports*, *encoding* and *errors*,\n"
"which are used to control compatiblity support for pickle stream\n"
"generated by Python 2.  If *fix_imports* is True, pickle will try to\n"
"map the old Python 2 names to the new names used in Python 3.  The\n"
"*encoding* and *errors* tell pickle how to decode 8-bit string\n"
"instances pickled by Python 2; these default to \'ASCII\' and \'strict\',\n"
"respectively.  The *encoding* can be \'bytes\' to read these 8-bit\n"
"string instances as bytes objects.");

static int
_pickle_Unpickler___init___impl(UnpicklerObject *self, PyObject *file,
                                int fix_imports, const char *encoding,
                                const char *errors);

static int
_pickle_Unpickler___init__(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int return_value = -1;
    static char *_keywords[] = {"file", "fix_imports", "encoding", "errors", NULL};
    PyObject *file;
    int fix_imports = 1;
    const char *encoding = "ASCII";
    const char *errors = "strict";

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|$pss:Unpickler", _keywords,
        &file, &fix_imports, &encoding, &errors))
        goto exit;
    return_value = _pickle_Unpickler___init___impl((UnpicklerObject *)self, file, fix_imports, encoding, errors);

exit:
    return return_value;
}

PyDoc_STRVAR(_pickle_UnpicklerMemoProxy_clear__doc__,
"clear($self, /)\n"
"--\n"
"\n"
"Remove all items from memo.");

#define _PICKLE_UNPICKLERMEMOPROXY_CLEAR_METHODDEF    \
    {"clear", (PyCFunction)_pickle_UnpicklerMemoProxy_clear, METH_NOARGS, _pickle_UnpicklerMemoProxy_clear__doc__},

static PyObject *
_pickle_UnpicklerMemoProxy_clear_impl(UnpicklerMemoProxyObject *self);

static PyObject *
_pickle_UnpicklerMemoProxy_clear(UnpicklerMemoProxyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _pickle_UnpicklerMemoProxy_clear_impl(self);
}

PyDoc_STRVAR(_pickle_UnpicklerMemoProxy_copy__doc__,
"copy($self, /)\n"
"--\n"
"\n"
"Copy the memo to a new object.");

#define _PICKLE_UNPICKLERMEMOPROXY_COPY_METHODDEF    \
    {"copy", (PyCFunction)_pickle_UnpicklerMemoProxy_copy, METH_NOARGS, _pickle_UnpicklerMemoProxy_copy__doc__},

static PyObject *
_pickle_UnpicklerMemoProxy_copy_impl(UnpicklerMemoProxyObject *self);

static PyObject *
_pickle_UnpicklerMemoProxy_copy(UnpicklerMemoProxyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _pickle_UnpicklerMemoProxy_copy_impl(self);
}

PyDoc_STRVAR(_pickle_UnpicklerMemoProxy___reduce____doc__,
"__reduce__($self, /)\n"
"--\n"
"\n"
"Implement pickling support.");

#define _PICKLE_UNPICKLERMEMOPROXY___REDUCE___METHODDEF    \
    {"__reduce__", (PyCFunction)_pickle_UnpicklerMemoProxy___reduce__, METH_NOARGS, _pickle_UnpicklerMemoProxy___reduce____doc__},

static PyObject *
_pickle_UnpicklerMemoProxy___reduce___impl(UnpicklerMemoProxyObject *self);

static PyObject *
_pickle_UnpicklerMemoProxy___reduce__(UnpicklerMemoProxyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _pickle_UnpicklerMemoProxy___reduce___impl(self);
}

PyDoc_STRVAR(_pickle_dump__doc__,
"dump($module, /, obj, file, protocol=None, *, fix_imports=True)\n"
"--\n"
"\n"
"Write a pickled representation of obj to the open file object file.\n"
"\n"
"This is equivalent to ``Pickler(file, protocol).dump(obj)``, but may\n"
"be more efficient.\n"
"\n"
"The optional *protocol* argument tells the pickler to use the given\n"
"protocol supported protocols are 0, 1, 2, 3 and 4.  The default\n"
"protocol is 3; a backward-incompatible protocol designed for Python 3.\n"
"\n"
"Specifying a negative protocol version selects the highest protocol\n"
"version supported.  The higher the protocol used, the more recent the\n"
"version of Python needed to read the pickle produced.\n"
"\n"
"The *file* argument must have a write() method that accepts a single\n"
"bytes argument.  It can thus be a file object opened for binary\n"
"writing, a io.BytesIO instance, or any other custom object that meets\n"
"this interface.\n"
"\n"
"If *fix_imports* is True and protocol is less than 3, pickle will try\n"
"to map the new Python 3 names to the old module names used in Python\n"
"2, so that the pickle data stream is readable with Python 2.");

#define _PICKLE_DUMP_METHODDEF    \
    {"dump", (PyCFunction)_pickle_dump, METH_VARARGS|METH_KEYWORDS, _pickle_dump__doc__},

static PyObject *
_pickle_dump_impl(PyModuleDef *module, PyObject *obj, PyObject *file,
                  PyObject *protocol, int fix_imports);

static PyObject *
_pickle_dump(PyModuleDef *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"obj", "file", "protocol", "fix_imports", NULL};
    PyObject *obj;
    PyObject *file;
    PyObject *protocol = NULL;
    int fix_imports = 1;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OO|O$p:dump", _keywords,
        &obj, &file, &protocol, &fix_imports))
        goto exit;
    return_value = _pickle_dump_impl(module, obj, file, protocol, fix_imports);

exit:
    return return_value;
}

PyDoc_STRVAR(_pickle_dumps__doc__,
"dumps($module, /, obj, protocol=None, *, fix_imports=True)\n"
"--\n"
"\n"
"Return the pickled representation of the object as a bytes object.\n"
"\n"
"The optional *protocol* argument tells the pickler to use the given\n"
"protocol; supported protocols are 0, 1, 2, 3 and 4.  The default\n"
"protocol is 3; a backward-incompatible protocol designed for Python 3.\n"
"\n"
"Specifying a negative protocol version selects the highest protocol\n"
"version supported.  The higher the protocol used, the more recent the\n"
"version of Python needed to read the pickle produced.\n"
"\n"
"If *fix_imports* is True and *protocol* is less than 3, pickle will\n"
"try to map the new Python 3 names to the old module names used in\n"
"Python 2, so that the pickle data stream is readable with Python 2.");

#define _PICKLE_DUMPS_METHODDEF    \
    {"dumps", (PyCFunction)_pickle_dumps, METH_VARARGS|METH_KEYWORDS, _pickle_dumps__doc__},

static PyObject *
_pickle_dumps_impl(PyModuleDef *module, PyObject *obj, PyObject *protocol,
                   int fix_imports);

static PyObject *
_pickle_dumps(PyModuleDef *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"obj", "protocol", "fix_imports", NULL};
    PyObject *obj;
    PyObject *protocol = NULL;
    int fix_imports = 1;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|O$p:dumps", _keywords,
        &obj, &protocol, &fix_imports))
        goto exit;
    return_value = _pickle_dumps_impl(module, obj, protocol, fix_imports);

exit:
    return return_value;
}

PyDoc_STRVAR(_pickle_load__doc__,
"load($module, /, file, *, fix_imports=True, encoding=\'ASCII\',\n"
"     errors=\'strict\')\n"
"--\n"
"\n"
"Read and return an object from the pickle data stored in a file.\n"
"\n"
"This is equivalent to ``Unpickler(file).load()``, but may be more\n"
"efficient.\n"
"\n"
"The protocol version of the pickle is detected automatically, so no\n"
"protocol argument is needed.  Bytes past the pickled object\'s\n"
"representation are ignored.\n"
"\n"
"The argument *file* must have two methods, a read() method that takes\n"
"an integer argument, and a readline() method that requires no\n"
"arguments.  Both methods should return bytes.  Thus *file* can be a\n"
"binary file object opened for reading, a io.BytesIO object, or any\n"
"other custom object that meets this interface.\n"
"\n"
"Optional keyword arguments are *fix_imports*, *encoding* and *errors*,\n"
"which are used to control compatiblity support for pickle stream\n"
"generated by Python 2.  If *fix_imports* is True, pickle will try to\n"
"map the old Python 2 names to the new names used in Python 3.  The\n"
"*encoding* and *errors* tell pickle how to decode 8-bit string\n"
"instances pickled by Python 2; these default to \'ASCII\' and \'strict\',\n"
"respectively.  The *encoding* can be \'bytes\' to read these 8-bit\n"
"string instances as bytes objects.");

#define _PICKLE_LOAD_METHODDEF    \
    {"load", (PyCFunction)_pickle_load, METH_VARARGS|METH_KEYWORDS, _pickle_load__doc__},

static PyObject *
_pickle_load_impl(PyModuleDef *module, PyObject *file, int fix_imports,
                  const char *encoding, const char *errors);

static PyObject *
_pickle_load(PyModuleDef *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"file", "fix_imports", "encoding", "errors", NULL};
    PyObject *file;
    int fix_imports = 1;
    const char *encoding = "ASCII";
    const char *errors = "strict";

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|$pss:load", _keywords,
        &file, &fix_imports, &encoding, &errors))
        goto exit;
    return_value = _pickle_load_impl(module, file, fix_imports, encoding, errors);

exit:
    return return_value;
}

PyDoc_STRVAR(_pickle_loads__doc__,
"loads($module, /, data, *, fix_imports=True, encoding=\'ASCII\',\n"
"      errors=\'strict\')\n"
"--\n"
"\n"
"Read and return an object from the given pickle data.\n"
"\n"
"The protocol version of the pickle is detected automatically, so no\n"
"protocol argument is needed.  Bytes past the pickled object\'s\n"
"representation are ignored.\n"
"\n"
"Optional keyword arguments are *fix_imports*, *encoding* and *errors*,\n"
"which are used to control compatiblity support for pickle stream\n"
"generated by Python 2.  If *fix_imports* is True, pickle will try to\n"
"map the old Python 2 names to the new names used in Python 3.  The\n"
"*encoding* and *errors* tell pickle how to decode 8-bit string\n"
"instances pickled by Python 2; these default to \'ASCII\' and \'strict\',\n"
"respectively.  The *encoding* can be \'bytes\' to read these 8-bit\n"
"string instances as bytes objects.");

#define _PICKLE_LOADS_METHODDEF    \
    {"loads", (PyCFunction)_pickle_loads, METH_VARARGS|METH_KEYWORDS, _pickle_loads__doc__},

static PyObject *
_pickle_loads_impl(PyModuleDef *module, PyObject *data, int fix_imports,
                   const char *encoding, const char *errors);

static PyObject *
_pickle_loads(PyModuleDef *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"data", "fix_imports", "encoding", "errors", NULL};
    PyObject *data;
    int fix_imports = 1;
    const char *encoding = "ASCII";
    const char *errors = "strict";

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|$pss:loads", _keywords,
        &data, &fix_imports, &encoding, &errors))
        goto exit;
    return_value = _pickle_loads_impl(module, data, fix_imports, encoding, errors);

exit:
    return return_value;
}
/*[clinic end generated code: output=06f3a5233298448e input=a9049054013a1b77]*/
