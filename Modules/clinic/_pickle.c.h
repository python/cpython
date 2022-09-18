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
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromSsize_t(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_pickle_Pickler___init____doc__,
"Pickler(file, protocol=None, fix_imports=True, buffer_callback=None)\n"
"--\n"
"\n"
"This takes a binary file for writing a pickle data stream.\n"
"\n"
"The optional *protocol* argument tells the pickler to use the given\n"
"protocol; supported protocols are 0, 1, 2, 3, 4 and 5.  The default\n"
"protocol is 4. It was introduced in Python 3.4, and is incompatible\n"
"with previous versions.\n"
"\n"
"Specifying a negative protocol version selects the highest protocol\n"
"version supported.  The higher the protocol used, the more recent the\n"
"version of Python needed to read the pickle produced.\n"
"\n"
"The *file* argument must have a write() method that accepts a single\n"
"bytes argument. It can thus be a file object opened for binary\n"
"writing, an io.BytesIO instance, or any other custom object that meets\n"
"this interface.\n"
"\n"
"If *fix_imports* is True and protocol is less than 3, pickle will try\n"
"to map the new Python 3 names to the old module names used in Python\n"
"2, so that the pickle data stream is readable with Python 2.\n"
"\n"
"If *buffer_callback* is None (the default), buffer views are\n"
"serialized into *file* as part of the pickle stream.\n"
"\n"
"If *buffer_callback* is not None, then it can be called any number\n"
"of times with a buffer view.  If the callback returns a false value\n"
"(such as None), the given buffer is out-of-band; otherwise the\n"
"buffer is serialized in-band, i.e. inside the pickle stream.\n"
"\n"
"It is an error if *buffer_callback* is not None and *protocol*\n"
"is None or smaller than 5.");

static int
_pickle_Pickler___init___impl(PicklerObject *self, PyObject *file,
                              PyObject *protocol, int fix_imports,
                              PyObject *buffer_callback);

static int
_pickle_Pickler___init__(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int return_value = -1;
    static const char * const _keywords[] = {"file", "protocol", "fix_imports", "buffer_callback", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "Pickler", 0};
    PyObject *argsbuf[4];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 1;
    PyObject *file;
    PyObject *protocol = Py_None;
    int fix_imports = 1;
    PyObject *buffer_callback = Py_None;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser, 1, 4, 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    file = fastargs[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (fastargs[1]) {
        protocol = fastargs[1];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (fastargs[2]) {
        fix_imports = PyObject_IsTrue(fastargs[2]);
        if (fix_imports < 0) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    buffer_callback = fastargs[3];
skip_optional_pos:
    return_value = _pickle_Pickler___init___impl((PicklerObject *)self, file, protocol, fix_imports, buffer_callback);

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
    {"find_class", _PyCFunction_CAST(_pickle_Unpickler_find_class), METH_FASTCALL, _pickle_Unpickler_find_class__doc__},

static PyObject *
_pickle_Unpickler_find_class_impl(UnpicklerObject *self,
                                  PyObject *module_name,
                                  PyObject *global_name);

static PyObject *
_pickle_Unpickler_find_class(UnpicklerObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *module_name;
    PyObject *global_name;

    if (!_PyArg_CheckPositional("find_class", nargs, 2, 2)) {
        goto exit;
    }
    module_name = args[0];
    global_name = args[1];
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
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromSsize_t(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_pickle_Unpickler___init____doc__,
"Unpickler(file, *, fix_imports=True, encoding=\'ASCII\', errors=\'strict\',\n"
"          buffers=())\n"
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
"binary file object opened for reading, an io.BytesIO object, or any\n"
"other custom object that meets this interface.\n"
"\n"
"Optional keyword arguments are *fix_imports*, *encoding* and *errors*,\n"
"which are used to control compatibility support for pickle stream\n"
"generated by Python 2.  If *fix_imports* is True, pickle will try to\n"
"map the old Python 2 names to the new names used in Python 3.  The\n"
"*encoding* and *errors* tell pickle how to decode 8-bit string\n"
"instances pickled by Python 2; these default to \'ASCII\' and \'strict\',\n"
"respectively.  The *encoding* can be \'bytes\' to read these 8-bit\n"
"string instances as bytes objects.");

static int
_pickle_Unpickler___init___impl(UnpicklerObject *self, PyObject *file,
                                int fix_imports, const char *encoding,
                                const char *errors, PyObject *buffers);

static int
_pickle_Unpickler___init__(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int return_value = -1;
    static const char * const _keywords[] = {"file", "fix_imports", "encoding", "errors", "buffers", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "Unpickler", 0};
    PyObject *argsbuf[5];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 1;
    PyObject *file;
    int fix_imports = 1;
    const char *encoding = "ASCII";
    const char *errors = "strict";
    PyObject *buffers = NULL;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser, 1, 1, 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    file = fastargs[0];
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (fastargs[1]) {
        fix_imports = PyObject_IsTrue(fastargs[1]);
        if (fix_imports < 0) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (fastargs[2]) {
        if (!PyUnicode_Check(fastargs[2])) {
            _PyArg_BadArgument("Unpickler", "argument 'encoding'", "str", fastargs[2]);
            goto exit;
        }
        Py_ssize_t encoding_length;
        encoding = PyUnicode_AsUTF8AndSize(fastargs[2], &encoding_length);
        if (encoding == NULL) {
            goto exit;
        }
        if (strlen(encoding) != (size_t)encoding_length) {
            PyErr_SetString(PyExc_ValueError, "embedded null character");
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (fastargs[3]) {
        if (!PyUnicode_Check(fastargs[3])) {
            _PyArg_BadArgument("Unpickler", "argument 'errors'", "str", fastargs[3]);
            goto exit;
        }
        Py_ssize_t errors_length;
        errors = PyUnicode_AsUTF8AndSize(fastargs[3], &errors_length);
        if (errors == NULL) {
            goto exit;
        }
        if (strlen(errors) != (size_t)errors_length) {
            PyErr_SetString(PyExc_ValueError, "embedded null character");
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    buffers = fastargs[4];
skip_optional_kwonly:
    return_value = _pickle_Unpickler___init___impl((UnpicklerObject *)self, file, fix_imports, encoding, errors, buffers);

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
"dump($module, /, obj, file, protocol=None, *, fix_imports=True,\n"
"     buffer_callback=None)\n"
"--\n"
"\n"
"Write a pickled representation of obj to the open file object file.\n"
"\n"
"This is equivalent to ``Pickler(file, protocol).dump(obj)``, but may\n"
"be more efficient.\n"
"\n"
"The optional *protocol* argument tells the pickler to use the given\n"
"protocol; supported protocols are 0, 1, 2, 3, 4 and 5.  The default\n"
"protocol is 4. It was introduced in Python 3.4, and is incompatible\n"
"with previous versions.\n"
"\n"
"Specifying a negative protocol version selects the highest protocol\n"
"version supported.  The higher the protocol used, the more recent the\n"
"version of Python needed to read the pickle produced.\n"
"\n"
"The *file* argument must have a write() method that accepts a single\n"
"bytes argument.  It can thus be a file object opened for binary\n"
"writing, an io.BytesIO instance, or any other custom object that meets\n"
"this interface.\n"
"\n"
"If *fix_imports* is True and protocol is less than 3, pickle will try\n"
"to map the new Python 3 names to the old module names used in Python\n"
"2, so that the pickle data stream is readable with Python 2.\n"
"\n"
"If *buffer_callback* is None (the default), buffer views are serialized\n"
"into *file* as part of the pickle stream.  It is an error if\n"
"*buffer_callback* is not None and *protocol* is None or smaller than 5.");

#define _PICKLE_DUMP_METHODDEF    \
    {"dump", _PyCFunction_CAST(_pickle_dump), METH_FASTCALL|METH_KEYWORDS, _pickle_dump__doc__},

static PyObject *
_pickle_dump_impl(PyObject *module, PyObject *obj, PyObject *file,
                  PyObject *protocol, int fix_imports,
                  PyObject *buffer_callback);

static PyObject *
_pickle_dump(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"obj", "file", "protocol", "fix_imports", "buffer_callback", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "dump", 0};
    PyObject *argsbuf[5];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 2;
    PyObject *obj;
    PyObject *file;
    PyObject *protocol = Py_None;
    int fix_imports = 1;
    PyObject *buffer_callback = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 3, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    obj = args[0];
    file = args[1];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[2]) {
        protocol = args[2];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
skip_optional_pos:
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (args[3]) {
        fix_imports = PyObject_IsTrue(args[3]);
        if (fix_imports < 0) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    buffer_callback = args[4];
skip_optional_kwonly:
    return_value = _pickle_dump_impl(module, obj, file, protocol, fix_imports, buffer_callback);

exit:
    return return_value;
}

PyDoc_STRVAR(_pickle_dumps__doc__,
"dumps($module, /, obj, protocol=None, *, fix_imports=True,\n"
"      buffer_callback=None)\n"
"--\n"
"\n"
"Return the pickled representation of the object as a bytes object.\n"
"\n"
"The optional *protocol* argument tells the pickler to use the given\n"
"protocol; supported protocols are 0, 1, 2, 3, 4 and 5.  The default\n"
"protocol is 4. It was introduced in Python 3.4, and is incompatible\n"
"with previous versions.\n"
"\n"
"Specifying a negative protocol version selects the highest protocol\n"
"version supported.  The higher the protocol used, the more recent the\n"
"version of Python needed to read the pickle produced.\n"
"\n"
"If *fix_imports* is True and *protocol* is less than 3, pickle will\n"
"try to map the new Python 3 names to the old module names used in\n"
"Python 2, so that the pickle data stream is readable with Python 2.\n"
"\n"
"If *buffer_callback* is None (the default), buffer views are serialized\n"
"into *file* as part of the pickle stream.  It is an error if\n"
"*buffer_callback* is not None and *protocol* is None or smaller than 5.");

#define _PICKLE_DUMPS_METHODDEF    \
    {"dumps", _PyCFunction_CAST(_pickle_dumps), METH_FASTCALL|METH_KEYWORDS, _pickle_dumps__doc__},

static PyObject *
_pickle_dumps_impl(PyObject *module, PyObject *obj, PyObject *protocol,
                   int fix_imports, PyObject *buffer_callback);

static PyObject *
_pickle_dumps(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"obj", "protocol", "fix_imports", "buffer_callback", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "dumps", 0};
    PyObject *argsbuf[4];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *obj;
    PyObject *protocol = Py_None;
    int fix_imports = 1;
    PyObject *buffer_callback = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    obj = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[1]) {
        protocol = args[1];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
skip_optional_pos:
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (args[2]) {
        fix_imports = PyObject_IsTrue(args[2]);
        if (fix_imports < 0) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    buffer_callback = args[3];
skip_optional_kwonly:
    return_value = _pickle_dumps_impl(module, obj, protocol, fix_imports, buffer_callback);

exit:
    return return_value;
}

PyDoc_STRVAR(_pickle_load__doc__,
"load($module, /, file, *, fix_imports=True, encoding=\'ASCII\',\n"
"     errors=\'strict\', buffers=())\n"
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
"binary file object opened for reading, an io.BytesIO object, or any\n"
"other custom object that meets this interface.\n"
"\n"
"Optional keyword arguments are *fix_imports*, *encoding* and *errors*,\n"
"which are used to control compatibility support for pickle stream\n"
"generated by Python 2.  If *fix_imports* is True, pickle will try to\n"
"map the old Python 2 names to the new names used in Python 3.  The\n"
"*encoding* and *errors* tell pickle how to decode 8-bit string\n"
"instances pickled by Python 2; these default to \'ASCII\' and \'strict\',\n"
"respectively.  The *encoding* can be \'bytes\' to read these 8-bit\n"
"string instances as bytes objects.");

#define _PICKLE_LOAD_METHODDEF    \
    {"load", _PyCFunction_CAST(_pickle_load), METH_FASTCALL|METH_KEYWORDS, _pickle_load__doc__},

static PyObject *
_pickle_load_impl(PyObject *module, PyObject *file, int fix_imports,
                  const char *encoding, const char *errors,
                  PyObject *buffers);

static PyObject *
_pickle_load(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"file", "fix_imports", "encoding", "errors", "buffers", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "load", 0};
    PyObject *argsbuf[5];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *file;
    int fix_imports = 1;
    const char *encoding = "ASCII";
    const char *errors = "strict";
    PyObject *buffers = NULL;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    file = args[0];
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (args[1]) {
        fix_imports = PyObject_IsTrue(args[1]);
        if (fix_imports < 0) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (args[2]) {
        if (!PyUnicode_Check(args[2])) {
            _PyArg_BadArgument("load", "argument 'encoding'", "str", args[2]);
            goto exit;
        }
        Py_ssize_t encoding_length;
        encoding = PyUnicode_AsUTF8AndSize(args[2], &encoding_length);
        if (encoding == NULL) {
            goto exit;
        }
        if (strlen(encoding) != (size_t)encoding_length) {
            PyErr_SetString(PyExc_ValueError, "embedded null character");
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (args[3]) {
        if (!PyUnicode_Check(args[3])) {
            _PyArg_BadArgument("load", "argument 'errors'", "str", args[3]);
            goto exit;
        }
        Py_ssize_t errors_length;
        errors = PyUnicode_AsUTF8AndSize(args[3], &errors_length);
        if (errors == NULL) {
            goto exit;
        }
        if (strlen(errors) != (size_t)errors_length) {
            PyErr_SetString(PyExc_ValueError, "embedded null character");
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    buffers = args[4];
skip_optional_kwonly:
    return_value = _pickle_load_impl(module, file, fix_imports, encoding, errors, buffers);

exit:
    return return_value;
}

PyDoc_STRVAR(_pickle_loads__doc__,
"loads($module, data, /, *, fix_imports=True, encoding=\'ASCII\',\n"
"      errors=\'strict\', buffers=())\n"
"--\n"
"\n"
"Read and return an object from the given pickle data.\n"
"\n"
"The protocol version of the pickle is detected automatically, so no\n"
"protocol argument is needed.  Bytes past the pickled object\'s\n"
"representation are ignored.\n"
"\n"
"Optional keyword arguments are *fix_imports*, *encoding* and *errors*,\n"
"which are used to control compatibility support for pickle stream\n"
"generated by Python 2.  If *fix_imports* is True, pickle will try to\n"
"map the old Python 2 names to the new names used in Python 3.  The\n"
"*encoding* and *errors* tell pickle how to decode 8-bit string\n"
"instances pickled by Python 2; these default to \'ASCII\' and \'strict\',\n"
"respectively.  The *encoding* can be \'bytes\' to read these 8-bit\n"
"string instances as bytes objects.");

#define _PICKLE_LOADS_METHODDEF    \
    {"loads", _PyCFunction_CAST(_pickle_loads), METH_FASTCALL|METH_KEYWORDS, _pickle_loads__doc__},

static PyObject *
_pickle_loads_impl(PyObject *module, PyObject *data, int fix_imports,
                   const char *encoding, const char *errors,
                   PyObject *buffers);

static PyObject *
_pickle_loads(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"", "fix_imports", "encoding", "errors", "buffers", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "loads", 0};
    PyObject *argsbuf[5];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *data;
    int fix_imports = 1;
    const char *encoding = "ASCII";
    const char *errors = "strict";
    PyObject *buffers = NULL;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    data = args[0];
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (args[1]) {
        fix_imports = PyObject_IsTrue(args[1]);
        if (fix_imports < 0) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (args[2]) {
        if (!PyUnicode_Check(args[2])) {
            _PyArg_BadArgument("loads", "argument 'encoding'", "str", args[2]);
            goto exit;
        }
        Py_ssize_t encoding_length;
        encoding = PyUnicode_AsUTF8AndSize(args[2], &encoding_length);
        if (encoding == NULL) {
            goto exit;
        }
        if (strlen(encoding) != (size_t)encoding_length) {
            PyErr_SetString(PyExc_ValueError, "embedded null character");
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (args[3]) {
        if (!PyUnicode_Check(args[3])) {
            _PyArg_BadArgument("loads", "argument 'errors'", "str", args[3]);
            goto exit;
        }
        Py_ssize_t errors_length;
        errors = PyUnicode_AsUTF8AndSize(args[3], &errors_length);
        if (errors == NULL) {
            goto exit;
        }
        if (strlen(errors) != (size_t)errors_length) {
            PyErr_SetString(PyExc_ValueError, "embedded null character");
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    buffers = args[4];
skip_optional_kwonly:
    return_value = _pickle_loads_impl(module, data, fix_imports, encoding, errors, buffers);

exit:
    return return_value;
}
/*[clinic end generated code: output=1bb1ead3c828e108 input=a9049054013a1b77]*/
