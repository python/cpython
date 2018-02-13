/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(_gdbm_gdbm_get__doc__,
"get($self, key, default=None, /)\n"
"--\n"
"\n"
"Get the value for key, or default if not present.");

#define _GDBM_GDBM_GET_METHODDEF    \
    {"get", (PyCFunction)_gdbm_gdbm_get, METH_FASTCALL, _gdbm_gdbm_get__doc__},

static PyObject *
_gdbm_gdbm_get_impl(dbmobject *self, PyObject *key, PyObject *default_value);

static PyObject *
_gdbm_gdbm_get(dbmobject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *key;
    PyObject *default_value = Py_None;

    if (!_PyArg_UnpackStack(args, nargs, "get",
        1, 2,
        &key, &default_value)) {
        goto exit;
    }
    return_value = _gdbm_gdbm_get_impl(self, key, default_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_gdbm_gdbm_setdefault__doc__,
"setdefault($self, key, default=None, /)\n"
"--\n"
"\n"
"Get value for key, or set it to default and return default if not present.");

#define _GDBM_GDBM_SETDEFAULT_METHODDEF    \
    {"setdefault", (PyCFunction)_gdbm_gdbm_setdefault, METH_FASTCALL, _gdbm_gdbm_setdefault__doc__},

static PyObject *
_gdbm_gdbm_setdefault_impl(dbmobject *self, PyObject *key,
                           PyObject *default_value);

static PyObject *
_gdbm_gdbm_setdefault(dbmobject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *key;
    PyObject *default_value = Py_None;

    if (!_PyArg_UnpackStack(args, nargs, "setdefault",
        1, 2,
        &key, &default_value)) {
        goto exit;
    }
    return_value = _gdbm_gdbm_setdefault_impl(self, key, default_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_gdbm_gdbm_close__doc__,
"close($self, /)\n"
"--\n"
"\n"
"Close the database.");

#define _GDBM_GDBM_CLOSE_METHODDEF    \
    {"close", (PyCFunction)_gdbm_gdbm_close, METH_NOARGS, _gdbm_gdbm_close__doc__},

static PyObject *
_gdbm_gdbm_close_impl(dbmobject *self);

static PyObject *
_gdbm_gdbm_close(dbmobject *self, PyObject *Py_UNUSED(ignored))
{
    return _gdbm_gdbm_close_impl(self);
}

PyDoc_STRVAR(_gdbm_gdbm_keys__doc__,
"keys($self, /)\n"
"--\n"
"\n"
"Get a list of all keys in the database.");

#define _GDBM_GDBM_KEYS_METHODDEF    \
    {"keys", (PyCFunction)_gdbm_gdbm_keys, METH_NOARGS, _gdbm_gdbm_keys__doc__},

static PyObject *
_gdbm_gdbm_keys_impl(dbmobject *self);

static PyObject *
_gdbm_gdbm_keys(dbmobject *self, PyObject *Py_UNUSED(ignored))
{
    return _gdbm_gdbm_keys_impl(self);
}

PyDoc_STRVAR(_gdbm_gdbm_firstkey__doc__,
"firstkey($self, /)\n"
"--\n"
"\n"
"Return the starting key for the traversal.\n"
"\n"
"It\'s possible to loop over every key in the database using this method\n"
"and the nextkey() method.  The traversal is ordered by GDBM\'s internal\n"
"hash values, and won\'t be sorted by the key values.");

#define _GDBM_GDBM_FIRSTKEY_METHODDEF    \
    {"firstkey", (PyCFunction)_gdbm_gdbm_firstkey, METH_NOARGS, _gdbm_gdbm_firstkey__doc__},

static PyObject *
_gdbm_gdbm_firstkey_impl(dbmobject *self);

static PyObject *
_gdbm_gdbm_firstkey(dbmobject *self, PyObject *Py_UNUSED(ignored))
{
    return _gdbm_gdbm_firstkey_impl(self);
}

PyDoc_STRVAR(_gdbm_gdbm_nextkey__doc__,
"nextkey($self, key, /)\n"
"--\n"
"\n"
"Returns the key that follows key in the traversal.\n"
"\n"
"The following code prints every key in the database db, without having\n"
"to create a list in memory that contains them all:\n"
"\n"
"      k = db.firstkey()\n"
"      while k != None:\n"
"          print(k)\n"
"          k = db.nextkey(k)");

#define _GDBM_GDBM_NEXTKEY_METHODDEF    \
    {"nextkey", (PyCFunction)_gdbm_gdbm_nextkey, METH_O, _gdbm_gdbm_nextkey__doc__},

static PyObject *
_gdbm_gdbm_nextkey_impl(dbmobject *self, const char *key,
                        Py_ssize_clean_t key_length);

static PyObject *
_gdbm_gdbm_nextkey(dbmobject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    const char *key;
    Py_ssize_clean_t key_length;

    if (!PyArg_Parse(arg, "s#:nextkey", &key, &key_length)) {
        goto exit;
    }
    return_value = _gdbm_gdbm_nextkey_impl(self, key, key_length);

exit:
    return return_value;
}

PyDoc_STRVAR(_gdbm_gdbm_reorganize__doc__,
"reorganize($self, /)\n"
"--\n"
"\n"
"Reorganize the database.\n"
"\n"
"If you have carried out a lot of deletions and would like to shrink\n"
"the space used by the GDBM file, this routine will reorganize the\n"
"database.  GDBM will not shorten the length of a database file except\n"
"by using this reorganization; otherwise, deleted file space will be\n"
"kept and reused as new (key,value) pairs are added.");

#define _GDBM_GDBM_REORGANIZE_METHODDEF    \
    {"reorganize", (PyCFunction)_gdbm_gdbm_reorganize, METH_NOARGS, _gdbm_gdbm_reorganize__doc__},

static PyObject *
_gdbm_gdbm_reorganize_impl(dbmobject *self);

static PyObject *
_gdbm_gdbm_reorganize(dbmobject *self, PyObject *Py_UNUSED(ignored))
{
    return _gdbm_gdbm_reorganize_impl(self);
}

PyDoc_STRVAR(_gdbm_gdbm_sync__doc__,
"sync($self, /)\n"
"--\n"
"\n"
"Flush the database to the disk file.\n"
"\n"
"When the database has been opened in fast mode, this method forces\n"
"any unwritten data to be written to the disk.");

#define _GDBM_GDBM_SYNC_METHODDEF    \
    {"sync", (PyCFunction)_gdbm_gdbm_sync, METH_NOARGS, _gdbm_gdbm_sync__doc__},

static PyObject *
_gdbm_gdbm_sync_impl(dbmobject *self);

static PyObject *
_gdbm_gdbm_sync(dbmobject *self, PyObject *Py_UNUSED(ignored))
{
    return _gdbm_gdbm_sync_impl(self);
}

PyDoc_STRVAR(dbmopen__doc__,
"open($module, filename, flags=\'r\', mode=0o666, /)\n"
"--\n"
"\n"
"Open a dbm database and return a dbm object.\n"
"\n"
"The filename argument is the name of the database file.\n"
"\n"
"The optional flags argument can be \'r\' (to open an existing database\n"
"for reading only -- default), \'w\' (to open an existing database for\n"
"reading and writing), \'c\' (which creates the database if it doesn\'t\n"
"exist), or \'n\' (which always creates a new empty database).\n"
"\n"
"Some versions of gdbm support additional flags which must be\n"
"appended to one of the flags described above.  The module constant\n"
"\'open_flags\' is a string of valid additional flags.  The \'f\' flag\n"
"opens the database in fast mode; altered data will not automatically\n"
"be written to the disk after every change.  This results in faster\n"
"writes to the database, but may result in an inconsistent database\n"
"if the program crashes while the database is still open.  Use the\n"
"sync() method to force any unwritten data to be written to the disk.\n"
"The \'s\' flag causes all database operations to be synchronized to\n"
"disk.  The \'u\' flag disables locking of the database file.\n"
"\n"
"The optional mode argument is the Unix mode of the file, used only\n"
"when the database has to be created.  It defaults to octal 0o666.");

#define DBMOPEN_METHODDEF    \
    {"open", (PyCFunction)dbmopen, METH_FASTCALL, dbmopen__doc__},

static PyObject *
dbmopen_impl(PyObject *module, const char *name, const char *flags, int mode);

static PyObject *
dbmopen(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    const char *name;
    const char *flags = "r";
    int mode = 438;

    if (!_PyArg_ParseStack(args, nargs, "s|si:open",
        &name, &flags, &mode)) {
        goto exit;
    }
    return_value = dbmopen_impl(module, name, flags, mode);

exit:
    return return_value;
}
/*[clinic end generated code: output=dc0aca8c00055d02 input=a9049054013a1b77]*/
