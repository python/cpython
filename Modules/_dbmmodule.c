
/* DBM module using dictionary interface */


#define PY_SSIZE_T_CLEAN
#include "Python.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/* Some Linux systems install gdbm/ndbm.h, but not ndbm.h.  This supports
 * whichever configure was able to locate.
 */
#if defined(HAVE_NDBM_H)
#include <ndbm.h>
static char *which_dbm = "GNU gdbm";  /* EMX port of GDBM */
#elif defined(HAVE_GDBM_NDBM_H)
#include <gdbm/ndbm.h>
static char *which_dbm = "GNU gdbm";
#elif defined(HAVE_GDBM_DASH_NDBM_H)
#include <gdbm-ndbm.h>
static char *which_dbm = "GNU gdbm";
#elif defined(HAVE_BERKDB_H)
#include <db.h>
static char *which_dbm = "Berkeley DB";
#else
#error "No ndbm.h available!"
#endif

/*[clinic input]
module dbm
class dbm.dbm "dbmobject *" "&Dbmtype"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=92450564684a69a3]*/

typedef struct {
    PyObject_HEAD
    int di_size;        /* -1 means recompute */
    DBM *di_dbm;
} dbmobject;

static PyTypeObject Dbmtype;

#define is_dbmobject(v) (Py_TYPE(v) == &Dbmtype)
#define check_dbmobject_open(v) if ((v)->di_dbm == NULL) \
               { PyErr_SetString(DbmError, "DBM object has already been closed"); \
                 return NULL; }

static PyObject *DbmError;

/*[python input]
class dbmobject_converter(self_converter):
    type = "dbmobject *"
    def pre_render(self):
        super().pre_render()
        self.name = 'dp'
[python start generated code]*/
/*[python end generated code: output=da39a3ee5e6b4b0d input=6ad536357913879a]*/

static PyObject *
newdbmobject(const char *file, int flags, int mode)
{
    dbmobject *dp;

    dp = PyObject_New(dbmobject, &Dbmtype);
    if (dp == NULL)
        return NULL;
    dp->di_size = -1;
    /* See issue #19296 */
    if ( (dp->di_dbm = dbm_open((char *)file, flags, mode)) == 0 ) {
        PyErr_SetFromErrno(DbmError);
        Py_DECREF(dp);
        return NULL;
    }
    return (PyObject *)dp;
}

/* Methods */

static void
dbm_dealloc(dbmobject *dp)
{
    if ( dp->di_dbm )
        dbm_close(dp->di_dbm);
    PyObject_Del(dp);
}

static Py_ssize_t
dbm_length(dbmobject *dp)
{
    if (dp->di_dbm == NULL) {
             PyErr_SetString(DbmError, "DBM object has already been closed");
             return -1;
    }
    if ( dp->di_size < 0 ) {
        datum key;
        int size;

        size = 0;
        for ( key=dbm_firstkey(dp->di_dbm); key.dptr;
              key = dbm_nextkey(dp->di_dbm))
            size++;
        dp->di_size = size;
    }
    return dp->di_size;
}

static PyObject *
dbm_subscript(dbmobject *dp, PyObject *key)
{
    datum drec, krec;
    Py_ssize_t tmp_size;

    if (!PyArg_Parse(key, "s#", &krec.dptr, &tmp_size) )
        return NULL;

    krec.dsize = tmp_size;
    check_dbmobject_open(dp);
    drec = dbm_fetch(dp->di_dbm, krec);
    if ( drec.dptr == 0 ) {
        PyErr_SetObject(PyExc_KeyError, key);
        return NULL;
    }
    if ( dbm_error(dp->di_dbm) ) {
        dbm_clearerr(dp->di_dbm);
        PyErr_SetString(DbmError, "");
        return NULL;
    }
    return PyBytes_FromStringAndSize(drec.dptr, drec.dsize);
}

static int
dbm_ass_sub(dbmobject *dp, PyObject *v, PyObject *w)
{
    datum krec, drec;
    Py_ssize_t tmp_size;

    if ( !PyArg_Parse(v, "s#", &krec.dptr, &tmp_size) ) {
        PyErr_SetString(PyExc_TypeError,
                        "dbm mappings have bytes or string keys only");
        return -1;
    }
    krec.dsize = tmp_size;
    if (dp->di_dbm == NULL) {
             PyErr_SetString(DbmError, "DBM object has already been closed");
             return -1;
    }
    dp->di_size = -1;
    if (w == NULL) {
        if ( dbm_delete(dp->di_dbm, krec) < 0 ) {
            dbm_clearerr(dp->di_dbm);
            PyErr_SetObject(PyExc_KeyError, v);
            return -1;
        }
    } else {
        if ( !PyArg_Parse(w, "s#", &drec.dptr, &tmp_size) ) {
            PyErr_SetString(PyExc_TypeError,
                 "dbm mappings have byte or string elements only");
            return -1;
        }
        drec.dsize = tmp_size;
        if ( dbm_store(dp->di_dbm, krec, drec, DBM_REPLACE) < 0 ) {
            dbm_clearerr(dp->di_dbm);
            PyErr_SetString(DbmError,
                            "cannot add item to database");
            return -1;
        }
    }
    if ( dbm_error(dp->di_dbm) ) {
        dbm_clearerr(dp->di_dbm);
        PyErr_SetString(DbmError, "");
        return -1;
    }
    return 0;
}

static PyMappingMethods dbm_as_mapping = {
    (lenfunc)dbm_length,                /*mp_length*/
    (binaryfunc)dbm_subscript,          /*mp_subscript*/
    (objobjargproc)dbm_ass_sub,         /*mp_ass_subscript*/
};

static PyObject *
dbm__close(dbmobject *dp, PyObject *unused)
{
    if (dp->di_dbm)
        dbm_close(dp->di_dbm);
    dp->di_dbm = NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
dbm_keys(dbmobject *dp, PyObject *unused)
{
    PyObject *v, *item;
    datum key;
    int err;

    check_dbmobject_open(dp);
    v = PyList_New(0);
    if (v == NULL)
        return NULL;
    for (key = dbm_firstkey(dp->di_dbm); key.dptr;
         key = dbm_nextkey(dp->di_dbm)) {
        item = PyBytes_FromStringAndSize(key.dptr, key.dsize);
        if (item == NULL) {
            Py_DECREF(v);
            return NULL;
        }
        err = PyList_Append(v, item);
        Py_DECREF(item);
        if (err != 0) {
            Py_DECREF(v);
            return NULL;
        }
    }
    return v;
}

static int
dbm_contains(PyObject *self, PyObject *arg)
{
    dbmobject *dp = (dbmobject *)self;
    datum key, val;
    Py_ssize_t size;

    if ((dp)->di_dbm == NULL) {
        PyErr_SetString(DbmError,
                        "DBM object has already been closed");
         return -1;
    }
    if (PyUnicode_Check(arg)) {
        key.dptr = PyUnicode_AsUTF8AndSize(arg, &size);
        key.dsize = size;
        if (key.dptr == NULL)
            return -1;
    }
    else if (!PyBytes_Check(arg)) {
        PyErr_Format(PyExc_TypeError,
                     "dbm key must be bytes or string, not %.100s",
                     arg->ob_type->tp_name);
        return -1;
    }
    else {
        key.dptr = PyBytes_AS_STRING(arg);
        key.dsize = PyBytes_GET_SIZE(arg);
    }
    val = dbm_fetch(dp->di_dbm, key);
    return val.dptr != NULL;
}

static PySequenceMethods dbm_as_sequence = {
    0,                          /* sq_length */
    0,                          /* sq_concat */
    0,                          /* sq_repeat */
    0,                          /* sq_item */
    0,                          /* sq_slice */
    0,                          /* sq_ass_item */
    0,                          /* sq_ass_slice */
    dbm_contains,               /* sq_contains */
    0,                          /* sq_inplace_concat */
    0,                          /* sq_inplace_repeat */
};

/*[clinic input]

dbm.dbm.get

    self: dbmobject

    key: str(length=True)
    default: object = None
    /

Return the value for key if present, otherwise default.
[clinic start generated code]*/

PyDoc_STRVAR(dbm_dbm_get__doc__,
"get($self, key, default=None, /)\n"
"--\n"
"\n"
"Return the value for key if present, otherwise default.");

#define DBM_DBM_GET_METHODDEF    \
    {"get", (PyCFunction)dbm_dbm_get, METH_VARARGS, dbm_dbm_get__doc__},

static PyObject *
dbm_dbm_get_impl(dbmobject *dp, const char *key, Py_ssize_clean_t key_length, PyObject *default_value);

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

static PyObject *
dbm_dbm_get_impl(dbmobject *dp, const char *key, Py_ssize_clean_t key_length, PyObject *default_value)
/*[clinic end generated code: output=452ea11394e7e92d input=aecf5efd2f2b1a3b]*/
{
    datum dbm_key, val;

    dbm_key.dptr = (char *)key;
    dbm_key.dsize = key_length;
    check_dbmobject_open(dp);
    val = dbm_fetch(dp->di_dbm, dbm_key);
    if (val.dptr != NULL)
        return PyBytes_FromStringAndSize(val.dptr, val.dsize);

    Py_INCREF(default_value);
    return default_value;
}

static PyObject *
dbm_setdefault(dbmobject *dp, PyObject *args)
{
    datum key, val;
    PyObject *defvalue = NULL;
    char *tmp_ptr;
    Py_ssize_t tmp_size;

    if (!PyArg_ParseTuple(args, "s#|O:setdefault",
                          &tmp_ptr, &tmp_size, &defvalue))
        return NULL;
    key.dptr = tmp_ptr;
    key.dsize = tmp_size;
    check_dbmobject_open(dp);
    val = dbm_fetch(dp->di_dbm, key);
    if (val.dptr != NULL)
        return PyBytes_FromStringAndSize(val.dptr, val.dsize);
    if (defvalue == NULL) {
        defvalue = PyBytes_FromStringAndSize(NULL, 0);
        if (defvalue == NULL)
            return NULL;
        val.dptr = NULL;
        val.dsize = 0;
    }
    else {
        if ( !PyArg_Parse(defvalue, "s#", &val.dptr, &tmp_size) ) {
            PyErr_SetString(PyExc_TypeError,
                "dbm mappings have byte string elements only");
            return NULL;
        }
        val.dsize = tmp_size;
        Py_INCREF(defvalue);
    }
    if (dbm_store(dp->di_dbm, key, val, DBM_INSERT) < 0) {
        dbm_clearerr(dp->di_dbm);
        PyErr_SetString(DbmError, "cannot add item to database");
        Py_DECREF(defvalue);
        return NULL;
    }
    return defvalue;
}

static PyObject *
dbm__enter__(PyObject *self, PyObject *args)
{
    Py_INCREF(self);
    return self;
}

static PyObject *
dbm__exit__(PyObject *self, PyObject *args)
{
    _Py_IDENTIFIER(close);
    return _PyObject_CallMethodId(self, &PyId_close, NULL);
}


static PyMethodDef dbm_methods[] = {
    {"close",           (PyCFunction)dbm__close,        METH_NOARGS,
     "close()\nClose the database."},
    {"keys",            (PyCFunction)dbm_keys,          METH_NOARGS,
     "keys() -> list\nReturn a list of all keys in the database."},
    DBM_DBM_GET_METHODDEF
    {"setdefault",      (PyCFunction)dbm_setdefault,    METH_VARARGS,
     "setdefault(key[, default]) -> value\n"
     "Return the value for key if present, otherwise default.  If key\n"
     "is not in the database, it is inserted with default as the value."},
    {"__enter__", dbm__enter__, METH_NOARGS, NULL},
    {"__exit__",  dbm__exit__, METH_VARARGS, NULL},
    {NULL,              NULL}           /* sentinel */
};

static PyTypeObject Dbmtype = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_dbm.dbm",
    sizeof(dbmobject),
    0,
    (destructor)dbm_dealloc,  /*tp_dealloc*/
    0,                            /*tp_print*/
    0,                        /*tp_getattr*/
    0,                            /*tp_setattr*/
    0,                            /*tp_reserved*/
    0,                            /*tp_repr*/
    0,                            /*tp_as_number*/
    &dbm_as_sequence,             /*tp_as_sequence*/
    &dbm_as_mapping,              /*tp_as_mapping*/
    0,                    /*tp_hash*/
    0,                    /*tp_call*/
    0,                    /*tp_str*/
    0,                    /*tp_getattro*/
    0,                    /*tp_setattro*/
    0,                    /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT,   /*tp_flags*/
    0,                        /*tp_doc*/
    0,                        /*tp_traverse*/
    0,                        /*tp_clear*/
    0,                        /*tp_richcompare*/
    0,                        /*tp_weaklistoffset*/
    0,                        /*tp_iter*/
    0,                        /*tp_iternext*/
    dbm_methods,          /*tp_methods*/
};

/* ----------------------------------------------------------------- */

/*[clinic input]

dbm.open as dbmopen

    filename: str
        The filename to open.

    flags: str="r"
        How to open the file.  "r" for reading, "w" for writing, etc.

    mode: int(py_default="0o666") = 0o666
        If creating a new file, the mode bits for the new file
        (e.g. os.O_RDWR).

    /

Return a database object.

[clinic start generated code]*/

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
dbmopen_impl(PyModuleDef *module, const char *filename, const char *flags, int mode);

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

static PyObject *
dbmopen_impl(PyModuleDef *module, const char *filename, const char *flags, int mode)
/*[clinic end generated code: output=9a7b725f9c4dcec2 input=6499ab0fab1333ac]*/
{
    int iflags;

    if ( strcmp(flags, "r") == 0 )
        iflags = O_RDONLY;
    else if ( strcmp(flags, "w") == 0 )
        iflags = O_RDWR;
    else if ( strcmp(flags, "rw") == 0 ) /* B/W compat */
        iflags = O_RDWR|O_CREAT;
    else if ( strcmp(flags, "c") == 0 )
        iflags = O_RDWR|O_CREAT;
    else if ( strcmp(flags, "n") == 0 )
        iflags = O_RDWR|O_CREAT|O_TRUNC;
    else {
        PyErr_SetString(DbmError,
                        "arg 2 to open should be 'r', 'w', 'c', or 'n'");
        return NULL;
    }
    return newdbmobject(filename, iflags, mode);
}

static PyMethodDef dbmmodule_methods[] = {
    DBMOPEN_METHODDEF
    { 0, 0 },
};


static struct PyModuleDef _dbmmodule = {
    PyModuleDef_HEAD_INIT,
    "_dbm",
    NULL,
    -1,
    dbmmodule_methods,
    NULL,
    NULL,
    NULL,
    NULL
};

PyMODINIT_FUNC
PyInit__dbm(void) {
    PyObject *m, *d, *s;

    if (PyType_Ready(&Dbmtype) < 0)
        return NULL;
    m = PyModule_Create(&_dbmmodule);
    if (m == NULL)
        return NULL;
    d = PyModule_GetDict(m);
    if (DbmError == NULL)
        DbmError = PyErr_NewException("_dbm.error",
                                      PyExc_IOError, NULL);
    s = PyUnicode_FromString(which_dbm);
    if (s != NULL) {
        PyDict_SetItemString(d, "library", s);
        Py_DECREF(s);
    }
    if (DbmError != NULL)
        PyDict_SetItemString(d, "error", DbmError);
    if (PyErr_Occurred()) {
        Py_DECREF(m);
        m = NULL;
    }
    return m;
}
