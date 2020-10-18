/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(pysqlite_cache_init__doc__,
"Cache(factory, /, size=10)\n"
"--\n"
"\n"
"Gets an entry from the cache or calls the factory function to produce one.");

static int
pysqlite_cache_init_impl(pysqlite_Cache *self, PyObject *factory, int size);

static int
pysqlite_cache_init(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int return_value = -1;
    static const char * const _keywords[] = {"", "size", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "Cache", 0};
    PyObject *argsbuf[2];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 1;
    PyObject *factory;
    int size = 10;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser, 1, 2, 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    factory = fastargs[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    size = _PyLong_AsInt(fastargs[1]);
    if (size == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional_pos:
    return_value = pysqlite_cache_init_impl((pysqlite_Cache *)self, factory, size);

exit:
    return return_value;
}

PyDoc_STRVAR(pysqlite_cache_get__doc__,
"get($self, key, /)\n"
"--\n"
"\n"
"Gets an entry from the cache or calls the factory function to produce one.");

#define PYSQLITE_CACHE_GET_METHODDEF    \
    {"get", (PyCFunction)pysqlite_cache_get, METH_O, pysqlite_cache_get__doc__},

PyDoc_STRVAR(pysqlite_cache_display__doc__,
"display($self, /)\n"
"--\n"
"\n"
"For debugging only.");

#define PYSQLITE_CACHE_DISPLAY_METHODDEF    \
    {"display", (PyCFunction)pysqlite_cache_display, METH_NOARGS, pysqlite_cache_display__doc__},

static PyObject *
pysqlite_cache_display_impl(pysqlite_Cache *self);

static PyObject *
pysqlite_cache_display(pysqlite_Cache *self, PyObject *Py_UNUSED(ignored))
{
    return pysqlite_cache_display_impl(self);
}
/*[clinic end generated code: output=324d7a1f2235e360 input=a9049054013a1b77]*/
