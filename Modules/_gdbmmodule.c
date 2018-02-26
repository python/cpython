
/* DBM module using dictionary interface */
/* Author: Anthony Baxter, after dbmmodule.c */
/* Doc strings: Mitch Chapman */


#include "Python.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "gdbm.h"

#if defined(WIN32) && !defined(__CYGWIN__)
#include "gdbmerrno.h"
extern const char * gdbm_strerror(gdbm_error);
#endif

/*[clinic input]
module _gdbm
class _gdbm.gdbm "dbmobject *" "&Dbmtype"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=113927c6170729b2]*/

PyDoc_STRVAR(gdbmmodule__doc__,
"This module provides an interface to the GNU DBM (GDBM) library.\n\
\n\
This module is quite similar to the dbm module, but uses GDBM instead to\n\
provide some additional functionality.  Please note that the file formats\n\
created by GDBM and dbm are incompatible.\n\
\n\
GDBM objects behave like mappings (dictionaries), except that keys and\n\
values are always immutable bytes-like objects or strings.  Printing\n\
a GDBM object doesn't print the keys and values, and the items() and\n\
values() methods are not supported.");

typedef struct {
    PyObject_HEAD
    int di_size;        /* -1 means recompute */
    GDBM_FILE di_dbm;
} dbmobject;

static PyTypeObject Dbmtype;

#include "clinic/_gdbmmodule.c.h"

#define is_dbmobject(v) (Py_TYPE(v) == &Dbmtype)
#define check_dbmobject_open(v) if ((v)->di_dbm == NULL) \
    { PyErr_SetString(DbmError, "GDBM object has already been closed"); \
      return NULL; }



static PyObject *DbmError;

PyDoc_STRVAR(gdbm_object__doc__,
"This object represents a GDBM database.\n\
GDBM objects behave like mappings (dictionaries), except that keys and\n\
values are always immutable bytes-like objects or strings.  Printing\n\
a GDBM object doesn't print the keys and values, and the items() and\n\
values() methods are not supported.\n\
\n\
GDBM objects also support additional operations such as firstkey,\n\
nextkey, reorganize, and sync.");

static PyObject *
newdbmobject(const char *file, int flags, int mode)
{
    dbmobject *dp;

    dp = PyObject_New(dbmobject, &Dbmtype);
    if (dp == NULL)
        return NULL;
    dp->di_size = -1;
    errno = 0;
    if ((dp->di_dbm = gdbm_open((char *)file, 0, flags, mode, NULL)) == 0) {
        if (errno != 0)
            PyErr_SetFromErrno(DbmError);
        else
            PyErr_SetString(DbmError, gdbm_strerror(gdbm_errno));
        Py_DECREF(dp);
        return NULL;
    }
    return (PyObject *)dp;
}

/* Methods */

static void
dbm_dealloc(dbmobject *dp)
{
    if (dp->di_dbm)
        gdbm_close(dp->di_dbm);
    PyObject_Del(dp);
}

static Py_ssize_t
dbm_length(dbmobject *dp)
{
    if (dp->di_dbm == NULL) {
        PyErr_SetString(DbmError, "GDBM object has already been closed");
        return -1;
    }
    if (dp->di_size < 0) {
        datum key,okey;
        int size;
        okey.dsize=0;
        okey.dptr=NULL;

        size = 0;
        for (key=gdbm_firstkey(dp->di_dbm); key.dptr;
             key = gdbm_nextkey(dp->di_dbm,okey)) {
            size++;
            if(okey.dsize) free(okey.dptr);
            okey=key;
        }
        dp->di_size = size;
    }
    return dp->di_size;
}

static PyObject *
dbm_subscript(dbmobject *dp, PyObject *key)
{
    PyObject *v;
    datum drec, krec;

    if (!PyArg_Parse(key, "s#", &krec.dptr, &krec.dsize) )
        return NULL;

    if (dp->di_dbm == NULL) {
        PyErr_SetString(DbmError,
                        "GDBM object has already been closed");
        return NULL;
    }
    drec = gdbm_fetch(dp->di_dbm, krec);
    if (drec.dptr == 0) {
        PyErr_SetObject(PyExc_KeyError, key);
        return NULL;
    }
    v = PyBytes_FromStringAndSize(drec.dptr, drec.dsize);
    free(drec.dptr);
    return v;
}

/*[clinic input]
_gdbm.gdbm.get

    key: object
    default: object = None
    /

Get the value for key, or default if not present.
[clinic start generated code]*/

static PyObject *
_gdbm_gdbm_get_impl(dbmobject *self, PyObject *key, PyObject *default_value)
/*[clinic end generated code: output=19b7c585ad4f554a input=a9c20423f34c17b6]*/
{
    PyObject *res;

    res = dbm_subscript(self, key);
    if (res == NULL && PyErr_ExceptionMatches(PyExc_KeyError)) {
        PyErr_Clear();
        Py_INCREF(default_value);
        return default_value;
    }
    return res;
}

static int
dbm_ass_sub(dbmobject *dp, PyObject *v, PyObject *w)
{
    datum krec, drec;

    if (!PyArg_Parse(v, "s#", &krec.dptr, &krec.dsize) ) {
        PyErr_SetString(PyExc_TypeError,
                        "gdbm mappings have bytes or string indices only");
        return -1;
    }
    if (dp->di_dbm == NULL) {
        PyErr_SetString(DbmError,
                        "GDBM object has already been closed");
        return -1;
    }
    dp->di_size = -1;
    if (w == NULL) {
        if (gdbm_delete(dp->di_dbm, krec) < 0) {
            PyErr_SetObject(PyExc_KeyError, v);
            return -1;
        }
    }
    else {
        if (!PyArg_Parse(w, "s#", &drec.dptr, &drec.dsize)) {
            PyErr_SetString(PyExc_TypeError,
                            "gdbm mappings have byte or string elements only");
            return -1;
        }
        errno = 0;
        if (gdbm_store(dp->di_dbm, krec, drec, GDBM_REPLACE) < 0) {
            if (errno != 0)
                PyErr_SetFromErrno(DbmError);
            else
                PyErr_SetString(DbmError,
                                gdbm_strerror(gdbm_errno));
            return -1;
        }
    }
    return 0;
}

/*[clinic input]
_gdbm.gdbm.setdefault

    key: object
    default: object = None
    /

Get value for key, or set it to default and return default if not present.
[clinic start generated code]*/

static PyObject *
_gdbm_gdbm_setdefault_impl(dbmobject *self, PyObject *key,
                           PyObject *default_value)
/*[clinic end generated code: output=88760ee520329012 input=0db46b69e9680171]*/
{
    PyObject *res;

    res = dbm_subscript(self, key);
    if (res == NULL && PyErr_ExceptionMatches(PyExc_KeyError)) {
        PyErr_Clear();
        if (dbm_ass_sub(self, key, default_value) < 0)
            return NULL;
        return dbm_subscript(self, key);
    }
    return res;
}

static PyMappingMethods dbm_as_mapping = {
    (lenfunc)dbm_length,                /*mp_length*/
    (binaryfunc)dbm_subscript,          /*mp_subscript*/
    (objobjargproc)dbm_ass_sub,         /*mp_ass_subscript*/
};

/*[clinic input]
_gdbm.gdbm.close

Close the database.
[clinic start generated code]*/

static PyObject *
_gdbm_gdbm_close_impl(dbmobject *self)
/*[clinic end generated code: output=23512a594598b563 input=0a203447379b45fd]*/
{
    if (self->di_dbm)
        gdbm_close(self->di_dbm);
    self->di_dbm = NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

/* XXX Should return a set or a set view */
/*[clinic input]
_gdbm.gdbm.keys

Get a list of all keys in the database.
[clinic start generated code]*/

static PyObject *
_gdbm_gdbm_keys_impl(dbmobject *self)
/*[clinic end generated code: output=cb4b1776c3645dcc input=1832ee0a3132cfaf]*/
{
    PyObject *v, *item;
    datum key, nextkey;
    int err;

    if (self == NULL || !is_dbmobject(self)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    check_dbmobject_open(self);

    v = PyList_New(0);
    if (v == NULL)
        return NULL;

    key = gdbm_firstkey(self->di_dbm);
    while (key.dptr) {
        item = PyBytes_FromStringAndSize(key.dptr, key.dsize);
        if (item == NULL) {
            free(key.dptr);
            Py_DECREF(v);
            return NULL;
        }
        err = PyList_Append(v, item);
        Py_DECREF(item);
        if (err != 0) {
            free(key.dptr);
            Py_DECREF(v);
            return NULL;
        }
        nextkey = gdbm_nextkey(self->di_dbm, key);
        free(key.dptr);
        key = nextkey;
    }
    return v;
}

static int
dbm_contains(PyObject *self, PyObject *arg)
{
    dbmobject *dp = (dbmobject *)self;
    datum key;
    Py_ssize_t size;

    if ((dp)->di_dbm == NULL) {
        PyErr_SetString(DbmError,
                        "GDBM object has already been closed");
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
                     "gdbm key must be bytes or string, not %.100s",
                     arg->ob_type->tp_name);
        return -1;
    }
    else {
        key.dptr = PyBytes_AS_STRING(arg);
        key.dsize = PyBytes_GET_SIZE(arg);
    }
    return gdbm_exists(dp->di_dbm, key);
}

static PySequenceMethods dbm_as_sequence = {
        0,                      /* sq_length */
        0,                      /* sq_concat */
        0,                      /* sq_repeat */
        0,                      /* sq_item */
        0,                      /* sq_slice */
        0,                      /* sq_ass_item */
        0,                      /* sq_ass_slice */
        dbm_contains,           /* sq_contains */
        0,                      /* sq_inplace_concat */
        0,                      /* sq_inplace_repeat */
};

/*[clinic input]
_gdbm.gdbm.firstkey

Return the starting key for the traversal.

It's possible to loop over every key in the database using this method
and the nextkey() method.  The traversal is ordered by GDBM's internal
hash values, and won't be sorted by the key values.
[clinic start generated code]*/

static PyObject *
_gdbm_gdbm_firstkey_impl(dbmobject *self)
/*[clinic end generated code: output=9ff85628d84b65d2 input=0dbd6a335d69bba0]*/
{
    PyObject *v;
    datum key;

    check_dbmobject_open(self);
    key = gdbm_firstkey(self->di_dbm);
    if (key.dptr) {
        v = PyBytes_FromStringAndSize(key.dptr, key.dsize);
        free(key.dptr);
        return v;
    }
    else {
        Py_INCREF(Py_None);
        return Py_None;
    }
}

/*[clinic input]
_gdbm.gdbm.nextkey

    key: str(accept={str, robuffer}, zeroes=True)
    /

Returns the key that follows key in the traversal.

The following code prints every key in the database db, without having
to create a list in memory that contains them all:

      k = db.firstkey()
      while k != None:
          print(k)
          k = db.nextkey(k)
[clinic start generated code]*/

static PyObject *
_gdbm_gdbm_nextkey_impl(dbmobject *self, const char *key,
                        Py_ssize_clean_t key_length)
/*[clinic end generated code: output=192ab892de6eb2f6 input=1f1606943614e36f]*/
{
    PyObject *v;
    datum dbm_key, nextkey;

    dbm_key.dptr = (char *)key;
    dbm_key.dsize = key_length;
    check_dbmobject_open(self);
    nextkey = gdbm_nextkey(self->di_dbm, dbm_key);
    if (nextkey.dptr) {
        v = PyBytes_FromStringAndSize(nextkey.dptr, nextkey.dsize);
        free(nextkey.dptr);
        return v;
    }
    else {
        Py_INCREF(Py_None);
        return Py_None;
    }
}

/*[clinic input]
_gdbm.gdbm.reorganize

Reorganize the database.

If you have carried out a lot of deletions and would like to shrink
the space used by the GDBM file, this routine will reorganize the
database.  GDBM will not shorten the length of a database file except
by using this reorganization; otherwise, deleted file space will be
kept and reused as new (key,value) pairs are added.
[clinic start generated code]*/

static PyObject *
_gdbm_gdbm_reorganize_impl(dbmobject *self)
/*[clinic end generated code: output=38d9624df92e961d input=f6bea85bcfd40dd2]*/
{
    check_dbmobject_open(self);
    errno = 0;
    if (gdbm_reorganize(self->di_dbm) < 0) {
        if (errno != 0)
            PyErr_SetFromErrno(DbmError);
        else
            PyErr_SetString(DbmError, gdbm_strerror(gdbm_errno));
        return NULL;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

/*[clinic input]
_gdbm.gdbm.sync

Flush the database to the disk file.

When the database has been opened in fast mode, this method forces
any unwritten data to be written to the disk.
[clinic start generated code]*/

static PyObject *
_gdbm_gdbm_sync_impl(dbmobject *self)
/*[clinic end generated code: output=488b15f47028f125 input=2a47d2c9e153ab8a]*/
{
    check_dbmobject_open(self);
    gdbm_sync(self->di_dbm);
    Py_INCREF(Py_None);
    return Py_None;
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
    _GDBM_GDBM_CLOSE_METHODDEF
    _GDBM_GDBM_KEYS_METHODDEF
    _GDBM_GDBM_FIRSTKEY_METHODDEF
    _GDBM_GDBM_NEXTKEY_METHODDEF
    _GDBM_GDBM_REORGANIZE_METHODDEF
    _GDBM_GDBM_SYNC_METHODDEF
    _GDBM_GDBM_GET_METHODDEF
    _GDBM_GDBM_GET_METHODDEF
    _GDBM_GDBM_SETDEFAULT_METHODDEF
    {"__enter__", dbm__enter__, METH_NOARGS, NULL},
    {"__exit__",  dbm__exit__, METH_VARARGS, NULL},
    {NULL,              NULL}           /* sentinel */
};

static PyTypeObject Dbmtype = {
    PyVarObject_HEAD_INIT(0, 0)
    "_gdbm.gdbm",
    sizeof(dbmobject),
    0,
    (destructor)dbm_dealloc,            /*tp_dealloc*/
    0,                                  /*tp_print*/
    0,                                  /*tp_getattr*/
    0,                                  /*tp_setattr*/
    0,                                  /*tp_reserved*/
    0,                                  /*tp_repr*/
    0,                                  /*tp_as_number*/
    &dbm_as_sequence,                   /*tp_as_sequence*/
    &dbm_as_mapping,                    /*tp_as_mapping*/
    0,                                  /*tp_hash*/
    0,                                  /*tp_call*/
    0,                                  /*tp_str*/
    0,                                  /*tp_getattro*/
    0,                                  /*tp_setattro*/
    0,                                  /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT,                 /*tp_xxx4*/
    gdbm_object__doc__,                 /*tp_doc*/
    0,                                  /*tp_traverse*/
    0,                                  /*tp_clear*/
    0,                                  /*tp_richcompare*/
    0,                                  /*tp_weaklistoffset*/
    0,                                  /*tp_iter*/
    0,                                  /*tp_iternext*/
    dbm_methods,                        /*tp_methods*/
};

/* ----------------------------------------------------------------- */

/*[clinic input]
_gdbm.open as dbmopen
    filename: unicode
    flags: str="r"
    mode: int(py_default="0o666") = 0o666
    /

Open a dbm database and return a dbm object.

The filename argument is the name of the database file.

The optional flags argument can be 'r' (to open an existing database
for reading only -- default), 'w' (to open an existing database for
reading and writing), 'c' (which creates the database if it doesn't
exist), or 'n' (which always creates a new empty database).

Some versions of gdbm support additional flags which must be
appended to one of the flags described above.  The module constant
'open_flags' is a string of valid additional flags.  The 'f' flag
opens the database in fast mode; altered data will not automatically
be written to the disk after every change.  This results in faster
writes to the database, but may result in an inconsistent database
if the program crashes while the database is still open.  Use the
sync() method to force any unwritten data to be written to the disk.
The 's' flag causes all database operations to be synchronized to
disk.  The 'u' flag disables locking of the database file.

The optional mode argument is the Unix mode of the file, used only
when the database has to be created.  It defaults to octal 0o666.
[clinic start generated code]*/

static PyObject *
dbmopen_impl(PyObject *module, PyObject *filename, const char *flags,
             int mode)
/*[clinic end generated code: output=9527750f5df90764 input=3be0b0875974b928]*/
{
    int iflags;

    switch (flags[0]) {
    case 'r':
        iflags = GDBM_READER;
        break;
    case 'w':
        iflags = GDBM_WRITER;
        break;
    case 'c':
        iflags = GDBM_WRCREAT;
        break;
    case 'n':
        iflags = GDBM_NEWDB;
        break;
    default:
        PyErr_SetString(DbmError,
                        "First flag must be one of 'r', 'w', 'c' or 'n'");
        return NULL;
    }
    for (flags++; *flags != '\0'; flags++) {
        char buf[40];
        switch (*flags) {
#ifdef GDBM_FAST
            case 'f':
                iflags |= GDBM_FAST;
                break;
#endif
#ifdef GDBM_SYNC
            case 's':
                iflags |= GDBM_SYNC;
                break;
#endif
#ifdef GDBM_NOLOCK
            case 'u':
                iflags |= GDBM_NOLOCK;
                break;
#endif
            default:
                PyOS_snprintf(buf, sizeof(buf), "Flag '%c' is not supported.",
                              *flags);
                PyErr_SetString(DbmError, buf);
                return NULL;
        }
    }

    PyObject *filenamebytes = PyUnicode_EncodeFSDefault(filename);
    if (filenamebytes == NULL) {
        return NULL;
    }
    const char *name = PyBytes_AS_STRING(filenamebytes);
    if (strlen(name) != (size_t)PyBytes_GET_SIZE(filenamebytes)) {
        Py_DECREF(filenamebytes);
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        return NULL;
    }
    PyObject *self = newdbmobject(name, iflags, mode);
    Py_DECREF(filenamebytes);
    return self;
}

static const char dbmmodule_open_flags[] = "rwcn"
#ifdef GDBM_FAST
                                     "f"
#endif
#ifdef GDBM_SYNC
                                     "s"
#endif
#ifdef GDBM_NOLOCK
                                     "u"
#endif
                                     ;

static PyMethodDef dbmmodule_methods[] = {
    DBMOPEN_METHODDEF
    { 0, 0 },
};


static struct PyModuleDef _gdbmmodule = {
        PyModuleDef_HEAD_INIT,
        "_gdbm",
        gdbmmodule__doc__,
        -1,
        dbmmodule_methods,
        NULL,
        NULL,
        NULL,
        NULL
};

PyMODINIT_FUNC
PyInit__gdbm(void) {
    PyObject *m, *d, *s;

    if (PyType_Ready(&Dbmtype) < 0)
            return NULL;
    m = PyModule_Create(&_gdbmmodule);
    if (m == NULL)
        return NULL;
    d = PyModule_GetDict(m);
    DbmError = PyErr_NewException("_gdbm.error", PyExc_IOError, NULL);
    if (DbmError != NULL) {
        PyDict_SetItemString(d, "error", DbmError);
        s = PyUnicode_FromString(dbmmodule_open_flags);
        PyDict_SetItemString(d, "open_flags", s);
        Py_DECREF(s);
    }
    return m;
}
