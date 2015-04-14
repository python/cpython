/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(dbm_dbm_get__doc__,
"get($self, key, default=None, /)\n"
"--\n"
"\n"
"Return the value for key if present, otherwise default.");

#define DBM_DBM_GET_METHODDEF    \
    {"get", (PyCFunction)dbm_dbm_get, METH_VARARGS, dbm_dbm_get__doc__},

static PyObject *
dbm_dbm_get_impl(dbmobject *dp, const char *key, Py_ssize_clean_t key_length,
                 PyObject *default_value);

static PyObject *
dbm_dbm_get(dbmobject *dp, PyObject *args)
{
    PyObject *return_value = NULL;
    const char *key;
    Py_ssize_clean_t key_length;
    PyObject *default_value = Py_None;

    if (!PyArg_ParseTuple(args,
        "s#|O:get",
        &key, &key_length, &default_value))
        goto exit;
    return_value = dbm_dbm_get_impl(dp, key, key_length, default_value);

exit:
    return return_value;
}

PyDoc_STRVAR(dbmopen__doc__,
"open($module, filename, flags=\'r\', mode=0o666, /)\n"
"--\n"
"\n"
"Return a database object.\n"
"\n"
"  filename\n"
"    The filename to open.\n"
"  flags\n"
"    How to open the file.  \"r\" for reading, \"w\" for writing, etc.\n"
"  mode\n"
"    If creating a new file, the mode bits for the new file\n"
"    (e.g. os.O_RDWR).");

#define DBMOPEN_METHODDEF    \
    {"open", (PyCFunction)dbmopen, METH_VARARGS, dbmopen__doc__},

static PyObject *
dbmopen_impl(PyModuleDef *module, const char *filename, const char *flags,
             int mode);

static PyObject *
dbmopen(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    const char *filename;
    const char *flags = "r";
    int mode = 438;

    if (!PyArg_ParseTuple(args,
        "s|si:open",
        &filename, &flags, &mode))
        goto exit;
    return_value = dbmopen_impl(module, filename, flags, mode);

exit:
    return return_value;
}
/*[clinic end generated code: output=d6ec55c6c5d0b19d input=a9049054013a1b77]*/
