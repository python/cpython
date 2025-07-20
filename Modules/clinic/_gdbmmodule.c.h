/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_runtime.h"     // _Py_SINGLETON()
#endif
#include "pycore_critical_section.h"// Py_BEGIN_CRITICAL_SECTION()
#include "pycore_modsupport.h"    // _PyArg_CheckPositional()

PyDoc_STRVAR(_gdbm_gdbm_get__doc__,
"get($self, key, default=None, /)\n"
"--\n"
"\n"
"Get the value for key, or default if not present.");

#define _GDBM_GDBM_GET_METHODDEF    \
    {"get", _PyCFunction_CAST(_gdbm_gdbm_get), METH_FASTCALL, _gdbm_gdbm_get__doc__},

static PyObject *
_gdbm_gdbm_get_impl(gdbmobject *self, PyObject *key, PyObject *default_value);

static PyObject *
_gdbm_gdbm_get(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *key;
    PyObject *default_value = Py_None;

    if (!_PyArg_CheckPositional("get", nargs, 1, 2)) {
        goto exit;
    }
    key = args[0];
    if (nargs < 2) {
        goto skip_optional;
    }
    default_value = args[1];
skip_optional:
    return_value = _gdbm_gdbm_get_impl((gdbmobject *)self, key, default_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_gdbm_gdbm_setdefault__doc__,
"setdefault($self, key, default=None, /)\n"
"--\n"
"\n"
"Get value for key, or set it to default and return default if not present.");

#define _GDBM_GDBM_SETDEFAULT_METHODDEF    \
    {"setdefault", _PyCFunction_CAST(_gdbm_gdbm_setdefault), METH_FASTCALL, _gdbm_gdbm_setdefault__doc__},

static PyObject *
_gdbm_gdbm_setdefault_impl(gdbmobject *self, PyObject *key,
                           PyObject *default_value);

static PyObject *
_gdbm_gdbm_setdefault(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *key;
    PyObject *default_value = Py_None;

    if (!_PyArg_CheckPositional("setdefault", nargs, 1, 2)) {
        goto exit;
    }
    key = args[0];
    if (nargs < 2) {
        goto skip_optional;
    }
    default_value = args[1];
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _gdbm_gdbm_setdefault_impl((gdbmobject *)self, key, default_value);
    Py_END_CRITICAL_SECTION();

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
_gdbm_gdbm_close_impl(gdbmobject *self);

static PyObject *
_gdbm_gdbm_close(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _gdbm_gdbm_close_impl((gdbmobject *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_gdbm_gdbm_keys__doc__,
"keys($self, /)\n"
"--\n"
"\n"
"Get a list of all keys in the database.");

#define _GDBM_GDBM_KEYS_METHODDEF    \
    {"keys", _PyCFunction_CAST(_gdbm_gdbm_keys), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _gdbm_gdbm_keys__doc__},

static PyObject *
_gdbm_gdbm_keys_impl(gdbmobject *self, PyTypeObject *cls);

static PyObject *
_gdbm_gdbm_keys(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;

    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "keys() takes no arguments");
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _gdbm_gdbm_keys_impl((gdbmobject *)self, cls);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
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
    {"firstkey", _PyCFunction_CAST(_gdbm_gdbm_firstkey), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _gdbm_gdbm_firstkey__doc__},

static PyObject *
_gdbm_gdbm_firstkey_impl(gdbmobject *self, PyTypeObject *cls);

static PyObject *
_gdbm_gdbm_firstkey(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;

    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "firstkey() takes no arguments");
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _gdbm_gdbm_firstkey_impl((gdbmobject *)self, cls);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
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
"      while k is not None:\n"
"          print(k)\n"
"          k = db.nextkey(k)");

#define _GDBM_GDBM_NEXTKEY_METHODDEF    \
    {"nextkey", _PyCFunction_CAST(_gdbm_gdbm_nextkey), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _gdbm_gdbm_nextkey__doc__},

static PyObject *
_gdbm_gdbm_nextkey_impl(gdbmobject *self, PyTypeObject *cls, const char *key,
                        Py_ssize_t key_length);

static PyObject *
_gdbm_gdbm_nextkey(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .format = "s#:nextkey",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    const char *key;
    Py_ssize_t key_length;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &key, &key_length)) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _gdbm_gdbm_nextkey_impl((gdbmobject *)self, cls, key, key_length);
    Py_END_CRITICAL_SECTION();

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
    {"reorganize", _PyCFunction_CAST(_gdbm_gdbm_reorganize), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _gdbm_gdbm_reorganize__doc__},

static PyObject *
_gdbm_gdbm_reorganize_impl(gdbmobject *self, PyTypeObject *cls);

static PyObject *
_gdbm_gdbm_reorganize(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;

    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "reorganize() takes no arguments");
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _gdbm_gdbm_reorganize_impl((gdbmobject *)self, cls);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
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
    {"sync", _PyCFunction_CAST(_gdbm_gdbm_sync), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _gdbm_gdbm_sync__doc__},

static PyObject *
_gdbm_gdbm_sync_impl(gdbmobject *self, PyTypeObject *cls);

static PyObject *
_gdbm_gdbm_sync(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;

    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "sync() takes no arguments");
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _gdbm_gdbm_sync_impl((gdbmobject *)self, cls);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_gdbm_gdbm_clear__doc__,
"clear($self, /)\n"
"--\n"
"\n"
"Remove all items from the database.");

#define _GDBM_GDBM_CLEAR_METHODDEF    \
    {"clear", _PyCFunction_CAST(_gdbm_gdbm_clear), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _gdbm_gdbm_clear__doc__},

static PyObject *
_gdbm_gdbm_clear_impl(gdbmobject *self, PyTypeObject *cls);

static PyObject *
_gdbm_gdbm_clear(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;

    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "clear() takes no arguments");
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _gdbm_gdbm_clear_impl((gdbmobject *)self, cls);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
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
    {"open", _PyCFunction_CAST(dbmopen), METH_FASTCALL, dbmopen__doc__},

static PyObject *
dbmopen_impl(PyObject *module, PyObject *filename, const char *flags,
             int mode);

static PyObject *
dbmopen(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *filename;
    const char *flags = "r";
    int mode = 438;

    if (!_PyArg_CheckPositional("open", nargs, 1, 3)) {
        goto exit;
    }
    filename = args[0];
    if (nargs < 2) {
        goto skip_optional;
    }
    if (!PyUnicode_Check(args[1])) {
        _PyArg_BadArgument("open", "argument 2", "str", args[1]);
        goto exit;
    }
    Py_ssize_t flags_length;
    flags = PyUnicode_AsUTF8AndSize(args[1], &flags_length);
    if (flags == NULL) {
        goto exit;
    }
    if (strlen(flags) != (size_t)flags_length) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    mode = PyLong_AsInt(args[2]);
    if (mode == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = dbmopen_impl(module, filename, flags, mode);

exit:
    return return_value;
}
/*[clinic end generated code: output=8bca34ce9d4493dd input=a9049054013a1b77]*/
