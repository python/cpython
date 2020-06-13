/*[clinic input]
preserve
[clinic start generated code]*/

static int
pysqlite_cache_init_impl(pysqlite_Cache *self, PyObject *factory, int size);

static int
pysqlite_cache_init(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int return_value = -1;
    PyObject *factory;
    int size = 10;

    if (Py_IS_TYPE(self, &pysqlite_CacheType) &&
        !_PyArg_NoKeywords("Cache", kwargs)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("Cache", PyTuple_GET_SIZE(args), 1, 2)) {
        goto exit;
    }
    factory = PyTuple_GET_ITEM(args, 0);
    if (PyTuple_GET_SIZE(args) < 2) {
        goto skip_optional;
    }
    size = _PyLong_AsInt(PyTuple_GET_ITEM(args, 1));
    if (size == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
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
/*[clinic end generated code: output=60204e1295b95c6a input=a9049054013a1b77]*/
