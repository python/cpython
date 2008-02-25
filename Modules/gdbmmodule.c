
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

PyDoc_STRVAR(gdbmmodule__doc__,
"This module provides an interface to the GNU DBM (GDBM) library.\n\
\n\
This module is quite similar to the dbm module, but uses GDBM instead to\n\
provide some additional functionality. Please note that the file formats\n\
created by GDBM and dbm are incompatible. \n\
\n\
GDBM objects behave like mappings (dictionaries), except that keys and\n\
values are always strings. Printing a GDBM object doesn't print the\n\
keys and values, and the items() and values() methods are not\n\
supported.");

typedef struct {
    PyObject_HEAD
    int di_size;	/* -1 means recompute */
    GDBM_FILE di_dbm;
} dbmobject;

static PyTypeObject Dbmtype;

#define is_dbmobject(v) (Py_TYPE(v) == &Dbmtype)
#define check_dbmobject_open(v) if ((v)->di_dbm == NULL) \
    { PyErr_SetString(DbmError, "GDBM object has already been closed"); \
      return NULL; }



static PyObject *DbmError;

PyDoc_STRVAR(gdbm_object__doc__,
"This object represents a GDBM database.\n\
GDBM objects behave like mappings (dictionaries), except that keys and\n\
values are always strings. Printing a GDBM object doesn't print the\n\
keys and values, and the items() and values() methods are not\n\
supported.\n\
\n\
GDBM objects also support additional operations such as firstkey,\n\
nextkey, reorganize, and sync.");

static PyObject *
newdbmobject(char *file, int flags, int mode)
{
    dbmobject *dp;

    dp = PyObject_New(dbmobject, &Dbmtype);
    if (dp == NULL)
        return NULL;
    dp->di_size = -1;
    errno = 0;
    if ((dp->di_dbm = gdbm_open(file, 0, flags, mode, NULL)) == 0) {
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
dbm_dealloc(register dbmobject *dp)
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
dbm_subscript(dbmobject *dp, register PyObject *key)
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
        PyErr_SetString(PyExc_KeyError,
                        PyString_AS_STRING((PyStringObject *)key));
        return NULL;
    }
    v = PyString_FromStringAndSize(drec.dptr, drec.dsize);
    free(drec.dptr);
    return v;
}

static int
dbm_ass_sub(dbmobject *dp, PyObject *v, PyObject *w)
{
    datum krec, drec;

    if (!PyArg_Parse(v, "s#", &krec.dptr, &krec.dsize) ) {
        PyErr_SetString(PyExc_TypeError,
                        "gdbm mappings have string indices only");
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
            PyErr_SetString(PyExc_KeyError,
                            PyString_AS_STRING((PyStringObject *)v));
            return -1;
        }
    }
    else {
        if (!PyArg_Parse(w, "s#", &drec.dptr, &drec.dsize)) {
            PyErr_SetString(PyExc_TypeError,
                            "gdbm mappings have string elements only");
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

static int
dbm_contains(register dbmobject *dp, PyObject *v)
{
    datum key;

    if (PyString_AsStringAndSize(v, &key.dptr, &key.dsize)) {
        return -1;
    }

    check_dbmobject_open(dp);
    
    return gdbm_exists(dp->di_dbm, key);
}

static PySequenceMethods dbm_as_sequence = {
    (lenfunc)dbm_length,               /*_length*/
       0,              /*sq_concat*/
       0,                      /*sq_repeat*/
       0,                      /*sq_item*/
       0,                      /*sq_slice*/
       0,                      /*sq_ass_item*/
       0,                  /*sq_ass_slice*/
       (objobjproc)dbm_contains,               /*sq_contains*/
       0,                  /*sq_inplace_concat*/
       0                   /*sq_inplace_repeat*/
};

static PyMappingMethods dbm_as_mapping = {
    (lenfunc)dbm_length,		/*mp_length*/
    (binaryfunc)dbm_subscript,          /*mp_subscript*/
    (objobjargproc)dbm_ass_sub,         /*mp_ass_subscript*/
};

PyDoc_STRVAR(dbm_close__doc__,
"close() -> None\n\
Closes the database.");

static PyObject *
dbm_close(register dbmobject *dp, PyObject *unused)
{
    if (dp->di_dbm)
        gdbm_close(dp->di_dbm);
    dp->di_dbm = NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

PyDoc_STRVAR(dbm_keys__doc__,
"keys() -> list_of_keys\n\
Get a list of all keys in the database.");

static PyObject *
dbm_keys(register dbmobject *dp, PyObject *unused)
{
    register PyObject *v, *item;
    datum key, nextkey;
    int err;

    if (dp == NULL || !is_dbmobject(dp)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    check_dbmobject_open(dp);

    v = PyList_New(0);
    if (v == NULL)
        return NULL;

    key = gdbm_firstkey(dp->di_dbm);
    while (key.dptr) {
        item = PyString_FromStringAndSize(key.dptr, key.dsize);
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
        nextkey = gdbm_nextkey(dp->di_dbm, key);
        free(key.dptr);
        key = nextkey;
    }
    return v;
}

PyDoc_STRVAR(dbm_has_key__doc__,
"has_key(key) -> boolean\n\
Find out whether or not the database contains a given key.");

static PyObject *
dbm_has_key(register dbmobject *dp, PyObject *args)
{
    datum key;

    if (!PyArg_ParseTuple(args, "s#:has_key", &key.dptr, &key.dsize))
        return NULL;
    check_dbmobject_open(dp);
    return PyInt_FromLong((long) gdbm_exists(dp->di_dbm, key));
}

PyDoc_STRVAR(dbm_firstkey__doc__,
"firstkey() -> key\n\
It's possible to loop over every key in the database using this method\n\
and the nextkey() method. The traversal is ordered by GDBM's internal\n\
hash values, and won't be sorted by the key values. This method\n\
returns the starting key.");

static PyObject *
dbm_firstkey(register dbmobject *dp, PyObject *unused)
{
    register PyObject *v;
    datum key;

    check_dbmobject_open(dp);
    key = gdbm_firstkey(dp->di_dbm);
    if (key.dptr) {
        v = PyString_FromStringAndSize(key.dptr, key.dsize);
        free(key.dptr);
        return v;
    }
    else {
        Py_INCREF(Py_None);
        return Py_None;
    }
}

PyDoc_STRVAR(dbm_nextkey__doc__,
"nextkey(key) -> next_key\n\
Returns the key that follows key in the traversal.\n\
The following code prints every key in the database db, without having\n\
to create a list in memory that contains them all:\n\
\n\
      k = db.firstkey()\n\
      while k != None:\n\
          print k\n\
          k = db.nextkey(k)");

static PyObject *
dbm_nextkey(register dbmobject *dp, PyObject *args)
{
    register PyObject *v;
    datum key, nextkey;

    if (!PyArg_ParseTuple(args, "s#:nextkey", &key.dptr, &key.dsize))
        return NULL;
    check_dbmobject_open(dp);
    nextkey = gdbm_nextkey(dp->di_dbm, key);
    if (nextkey.dptr) {
        v = PyString_FromStringAndSize(nextkey.dptr, nextkey.dsize);
        free(nextkey.dptr);
        return v;
    }
    else {
        Py_INCREF(Py_None);
        return Py_None;
    }
}

PyDoc_STRVAR(dbm_reorganize__doc__,
"reorganize() -> None\n\
If you have carried out a lot of deletions and would like to shrink\n\
the space used by the GDBM file, this routine will reorganize the\n\
database. GDBM will not shorten the length of a database file except\n\
by using this reorganization; otherwise, deleted file space will be\n\
kept and reused as new (key,value) pairs are added.");

static PyObject *
dbm_reorganize(register dbmobject *dp, PyObject *unused)
{
    check_dbmobject_open(dp);
    errno = 0;
    if (gdbm_reorganize(dp->di_dbm) < 0) {
        if (errno != 0)
            PyErr_SetFromErrno(DbmError);
        else
            PyErr_SetString(DbmError, gdbm_strerror(gdbm_errno));
        return NULL;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

PyDoc_STRVAR(dbm_sync__doc__,
"sync() -> None\n\
When the database has been opened in fast mode, this method forces\n\
any unwritten data to be written to the disk.");

static PyObject *
dbm_sync(register dbmobject *dp, PyObject *unused)
{
    check_dbmobject_open(dp);
    gdbm_sync(dp->di_dbm);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyMethodDef dbm_methods[] = {
    {"close",	  (PyCFunction)dbm_close,   METH_NOARGS, dbm_close__doc__},
    {"keys",	  (PyCFunction)dbm_keys,    METH_NOARGS, dbm_keys__doc__},
    {"has_key",   (PyCFunction)dbm_has_key, METH_VARARGS, dbm_has_key__doc__},
    {"firstkey",  (PyCFunction)dbm_firstkey,METH_NOARGS, dbm_firstkey__doc__},
    {"nextkey",	  (PyCFunction)dbm_nextkey, METH_VARARGS, dbm_nextkey__doc__},
    {"reorganize",(PyCFunction)dbm_reorganize,METH_NOARGS, dbm_reorganize__doc__},
    {"sync",      (PyCFunction)dbm_sync,    METH_NOARGS, dbm_sync__doc__},
    {NULL,		NULL}		/* sentinel */
};

static PyObject *
dbm_getattr(dbmobject *dp, char *name)
{
    return Py_FindMethod(dbm_methods, (PyObject *)dp, name);
}

static PyTypeObject Dbmtype = {
    PyVarObject_HEAD_INIT(0, 0)
    "gdbm.gdbm",
    sizeof(dbmobject),
    0,
    (destructor)dbm_dealloc,            /*tp_dealloc*/
    0,                                  /*tp_print*/
    (getattrfunc)dbm_getattr,           /*tp_getattr*/
    0,                                  /*tp_setattr*/
    0,                                  /*tp_compare*/
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
};

/* ----------------------------------------------------------------- */

PyDoc_STRVAR(dbmopen__doc__,
"open(filename, [flags, [mode]])  -> dbm_object\n\
Open a dbm database and return a dbm object. The filename argument is\n\
the name of the database file.\n\
\n\
The optional flags argument can be 'r' (to open an existing database\n\
for reading only -- default), 'w' (to open an existing database for\n\
reading and writing), 'c' (which creates the database if it doesn't\n\
exist), or 'n' (which always creates a new empty database).\n\
\n\
Some versions of gdbm support additional flags which must be\n\
appended to one of the flags described above. The module constant\n\
'open_flags' is a string of valid additional flags. The 'f' flag\n\
opens the database in fast mode; altered data will not automatically\n\
be written to the disk after every change. This results in faster\n\
writes to the database, but may result in an inconsistent database\n\
if the program crashes while the database is still open. Use the\n\
sync() method to force any unwritten data to be written to the disk.\n\
The 's' flag causes all database operations to be synchronized to\n\
disk. The 'u' flag disables locking of the database file.\n\
\n\
The optional mode argument is the Unix mode of the file, used only\n\
when the database has to be created. It defaults to octal 0666. ");

static PyObject *
dbmopen(PyObject *self, PyObject *args)
{
    char *name;
    char *flags = "r";
    int iflags;
    int mode = 0666;

    if (!PyArg_ParseTuple(args, "s|si:open", &name, &flags, &mode))
        return NULL;
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

    return newdbmobject(name, iflags, mode);
}

static char dbmmodule_open_flags[] = "rwcn"
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
    { "open", (PyCFunction)dbmopen, METH_VARARGS, dbmopen__doc__},
    { 0, 0 },
};

PyMODINIT_FUNC
initgdbm(void) {
    PyObject *m, *d, *s;

    Dbmtype.ob_type = &PyType_Type;
    m = Py_InitModule4("gdbm", dbmmodule_methods,
                       gdbmmodule__doc__, (PyObject *)NULL,
                       PYTHON_API_VERSION);
    if (m == NULL)
	return;
    d = PyModule_GetDict(m);
    DbmError = PyErr_NewException("gdbm.error", NULL, NULL);
    if (DbmError != NULL) {
        PyDict_SetItemString(d, "error", DbmError);
        s = PyString_FromString(dbmmodule_open_flags);
        PyDict_SetItemString(d, "open_flags", s);
        Py_DECREF(s);
    }
}
