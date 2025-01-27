/* GDBM module using dictionary interface */
/* Author: Anthony Baxter, after dbmmodule.c */
/* Doc strings: Mitch Chapman */

// clinic/_gdbmmodule.c.h uses internal pycore_modsupport.h API
#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "Python.h"
#include "pycore_pyerrors.h"        // _PyErr_SetLocaleString()
#include "gdbm.h"

#include <fcntl.h>
#include <stdlib.h>                 // free()
#include <sys/stat.h>
#include <sys/types.h>

#if defined(WIN32) && !defined(__CYGWIN__)
#include "gdbmerrno.h"
extern const char * gdbm_strerror(gdbm_error);
#endif

typedef struct {
    PyTypeObject *gdbm_type;
    PyObject *gdbm_error;
} _gdbm_state;

static inline _gdbm_state*
get_gdbm_state(PyObject *module)
{
    void *state = PyModule_GetState(module);
    assert(state != NULL);
    return (_gdbm_state *)state;
}

/*
 * Set the gdbm error obtained by gdbm_strerror(gdbm_errno).
 *
 * If no error message exists, a generic (UTF-8) error message
 * is used instead.
 */
static void
set_gdbm_error(_gdbm_state *state, const char *generic_error)
{
    const char *gdbm_errmsg = gdbm_strerror(gdbm_errno);
    if (gdbm_errmsg) {
        _PyErr_SetLocaleString(state->gdbm_error, gdbm_errmsg);
    }
    else {
        PyErr_SetString(state->gdbm_error, generic_error);
    }
}

/*[clinic input]
module _gdbm
class _gdbm.gdbm "gdbmobject *" "&Gdbmtype"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=38ae71cedfc7172b]*/

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
    Py_ssize_t di_size;        /* -1 means recompute */
    GDBM_FILE di_dbm;
} gdbmobject;

#include "clinic/_gdbmmodule.c.h"

#define check_gdbmobject_open(v, err)                                 \
    if ((v)->di_dbm == NULL) {                                       \
        PyErr_SetString(err, "GDBM object has already been closed"); \
        return NULL;                                                 \
    }

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
newgdbmobject(_gdbm_state *state, const char *file, int flags, int mode)
{
    gdbmobject *dp = PyObject_GC_New(gdbmobject, state->gdbm_type);
    if (dp == NULL) {
        return NULL;
    }
    dp->di_size = -1;
    errno = 0;
    PyObject_GC_Track(dp);

    if ((dp->di_dbm = gdbm_open((char *)file, 0, flags, mode, NULL)) == 0) {
        if (errno != 0) {
            PyErr_SetFromErrnoWithFilename(state->gdbm_error, file);
        }
        else {
            set_gdbm_error(state, "gdbm_open() error");
        }
        Py_DECREF(dp);
        return NULL;
    }
    return (PyObject *)dp;
}

/* Methods */
static int
gdbm_traverse(gdbmobject *dp, visitproc visit, void *arg)
{
    Py_VISIT(Py_TYPE(dp));
    return 0;
}

static void
gdbm_dealloc(gdbmobject *dp)
{
    PyObject_GC_UnTrack(dp);
    if (dp->di_dbm) {
        gdbm_close(dp->di_dbm);
    }
    PyTypeObject *tp = Py_TYPE(dp);
    tp->tp_free(dp);
    Py_DECREF(tp);
}

static Py_ssize_t
gdbm_length(gdbmobject *dp)
{
    _gdbm_state *state = PyType_GetModuleState(Py_TYPE(dp));
    if (dp->di_dbm == NULL) {
        PyErr_SetString(state->gdbm_error, "GDBM object has already been closed");
        return -1;
    }
    if (dp->di_size < 0) {
#if GDBM_VERSION_MAJOR >= 1 && GDBM_VERSION_MINOR >= 11
        errno = 0;
        gdbm_count_t count;
        if (gdbm_count(dp->di_dbm, &count) == -1) {
            if (errno != 0) {
                PyErr_SetFromErrno(state->gdbm_error);
            }
            else {
                set_gdbm_error(state, "gdbm_count() error");
            }
            return -1;
        }
        if (count > PY_SSIZE_T_MAX) {
            PyErr_SetString(PyExc_OverflowError, "count exceeds PY_SSIZE_T_MAX");
            return -1;
        }
        dp->di_size = count;
#else
        datum key,okey;
        okey.dsize=0;
        okey.dptr=NULL;

        Py_ssize_t size = 0;
        for (key = gdbm_firstkey(dp->di_dbm); key.dptr;
             key = gdbm_nextkey(dp->di_dbm,okey)) {
            size++;
            if (okey.dsize) {
                free(okey.dptr);
            }
            okey=key;
        }
        dp->di_size = size;
#endif
    }
    return dp->di_size;
}

static int
gdbm_bool(gdbmobject *dp)
{
    _gdbm_state *state = PyType_GetModuleState(Py_TYPE(dp));
    if (dp->di_dbm == NULL) {
        PyErr_SetString(state->gdbm_error, "GDBM object has already been closed");
        return -1;
    }
    if (dp->di_size > 0) {
        /* Known non-zero size. */
        return 1;
    }
    if (dp->di_size == 0) {
        /* Known zero size. */
        return 0;
    }
    /* Unknown size.  Ensure DBM object has an entry. */
    datum key = gdbm_firstkey(dp->di_dbm);
    if (key.dptr == NULL) {
        /* Empty. Cache this fact. */
        dp->di_size = 0;
        return 0;
    }

    /* Non-empty. Don't cache the length since we don't know. */
    free(key.dptr);
    return 1;
}

// Wrapper function for PyArg_Parse(o, "s#", &d.dptr, &d.size).
// This function is needed to support PY_SSIZE_T_CLEAN.
// Return 1 on success, same to PyArg_Parse().
static int
parse_datum(PyObject *o, datum *d, const char *failmsg)
{
    Py_ssize_t size;
    if (!PyArg_Parse(o, "s#", &d->dptr, &size)) {
        if (failmsg != NULL) {
            PyErr_SetString(PyExc_TypeError, failmsg);
        }
        return 0;
    }
    if (INT_MAX < size) {
        PyErr_SetString(PyExc_OverflowError, "size does not fit in an int");
        return 0;
    }
    d->dsize = size;
    return 1;
}

static PyObject *
gdbm_subscript(gdbmobject *dp, PyObject *key)
{
    PyObject *v;
    datum drec, krec;
    _gdbm_state *state = PyType_GetModuleState(Py_TYPE(dp));

    if (!parse_datum(key, &krec, NULL)) {
        return NULL;
    }
    if (dp->di_dbm == NULL) {
        PyErr_SetString(state->gdbm_error,
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
_gdbm_gdbm_get_impl(gdbmobject *self, PyObject *key, PyObject *default_value)
/*[clinic end generated code: output=92421838f3a852f4 input=a9c20423f34c17b6]*/
{
    PyObject *res;

    res = gdbm_subscript(self, key);
    if (res == NULL && PyErr_ExceptionMatches(PyExc_KeyError)) {
        PyErr_Clear();
        return Py_NewRef(default_value);
    }
    return res;
}

static int
gdbm_ass_sub(gdbmobject *dp, PyObject *v, PyObject *w)
{
    datum krec, drec;
    const char *failmsg = "gdbm mappings have bytes or string indices only";
    _gdbm_state *state = PyType_GetModuleState(Py_TYPE(dp));

    if (!parse_datum(v, &krec, failmsg)) {
        return -1;
    }
    if (dp->di_dbm == NULL) {
        PyErr_SetString(state->gdbm_error,
                        "GDBM object has already been closed");
        return -1;
    }
    dp->di_size = -1;
    if (w == NULL) {
        if (gdbm_delete(dp->di_dbm, krec) < 0) {
            if (gdbm_errno == GDBM_ITEM_NOT_FOUND) {
                PyErr_SetObject(PyExc_KeyError, v);
            }
            else {
                set_gdbm_error(state, "gdbm_delete() error");
            }
            return -1;
        }
    }
    else {
        if (!parse_datum(w, &drec, failmsg)) {
            return -1;
        }
        errno = 0;
        if (gdbm_store(dp->di_dbm, krec, drec, GDBM_REPLACE) < 0) {
            if (errno != 0) {
                PyErr_SetFromErrno(state->gdbm_error);
            }
            else {
                set_gdbm_error(state, "gdbm_store() error");
            }
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
_gdbm_gdbm_setdefault_impl(gdbmobject *self, PyObject *key,
                           PyObject *default_value)
/*[clinic end generated code: output=f3246e880509f142 input=0db46b69e9680171]*/
{
    PyObject *res;

    res = gdbm_subscript(self, key);
    if (res == NULL && PyErr_ExceptionMatches(PyExc_KeyError)) {
        PyErr_Clear();
        if (gdbm_ass_sub(self, key, default_value) < 0)
            return NULL;
        return gdbm_subscript(self, key);
    }
    return res;
}

/*[clinic input]
_gdbm.gdbm.close

Close the database.
[clinic start generated code]*/

static PyObject *
_gdbm_gdbm_close_impl(gdbmobject *self)
/*[clinic end generated code: output=f5abb4d6bb9e52d5 input=0a203447379b45fd]*/
{
    if (self->di_dbm) {
        gdbm_close(self->di_dbm);
    }
    self->di_dbm = NULL;
    Py_RETURN_NONE;
}

/* XXX Should return a set or a set view */
/*[clinic input]
_gdbm.gdbm.keys

    cls: defining_class

Get a list of all keys in the database.
[clinic start generated code]*/

static PyObject *
_gdbm_gdbm_keys_impl(gdbmobject *self, PyTypeObject *cls)
/*[clinic end generated code: output=c24b824e81404755 input=1428b7c79703d7d5]*/
{
    PyObject *v, *item;
    datum key, nextkey;
    int err;

    _gdbm_state *state = PyType_GetModuleState(cls);
    assert(state != NULL);

    if (self == NULL || !Py_IS_TYPE(self, state->gdbm_type)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    check_gdbmobject_open(self, state->gdbm_error);

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
gdbm_contains(PyObject *self, PyObject *arg)
{
    gdbmobject *dp = (gdbmobject *)self;
    datum key;
    Py_ssize_t size;
    _gdbm_state *state = PyType_GetModuleState(Py_TYPE(dp));

    if ((dp)->di_dbm == NULL) {
        PyErr_SetString(state->gdbm_error,
                        "GDBM object has already been closed");
        return -1;
    }
    if (PyUnicode_Check(arg)) {
        key.dptr = (char *)PyUnicode_AsUTF8AndSize(arg, &size);
        key.dsize = size;
        if (key.dptr == NULL)
            return -1;
    }
    else if (!PyBytes_Check(arg)) {
        PyErr_Format(PyExc_TypeError,
                     "gdbm key must be bytes or string, not %.100s",
                     Py_TYPE(arg)->tp_name);
        return -1;
    }
    else {
        key.dptr = PyBytes_AS_STRING(arg);
        key.dsize = PyBytes_GET_SIZE(arg);
    }
    return gdbm_exists(dp->di_dbm, key);
}

/*[clinic input]
_gdbm.gdbm.firstkey

    cls: defining_class

Return the starting key for the traversal.

It's possible to loop over every key in the database using this method
and the nextkey() method.  The traversal is ordered by GDBM's internal
hash values, and won't be sorted by the key values.
[clinic start generated code]*/

static PyObject *
_gdbm_gdbm_firstkey_impl(gdbmobject *self, PyTypeObject *cls)
/*[clinic end generated code: output=139275e9c8b60827 input=ed8782a029a5d299]*/
{
    PyObject *v;
    datum key;
    _gdbm_state *state = PyType_GetModuleState(cls);
    assert(state != NULL);

    check_gdbmobject_open(self, state->gdbm_error);
    key = gdbm_firstkey(self->di_dbm);
    if (key.dptr) {
        v = PyBytes_FromStringAndSize(key.dptr, key.dsize);
        free(key.dptr);
        return v;
    }
    else {
        Py_RETURN_NONE;
    }
}

/*[clinic input]
_gdbm.gdbm.nextkey

    cls: defining_class
    key: str(accept={str, robuffer}, zeroes=True)
    /

Returns the key that follows key in the traversal.

The following code prints every key in the database db, without having
to create a list in memory that contains them all:

      k = db.firstkey()
      while k is not None:
          print(k)
          k = db.nextkey(k)
[clinic start generated code]*/

static PyObject *
_gdbm_gdbm_nextkey_impl(gdbmobject *self, PyTypeObject *cls, const char *key,
                        Py_ssize_t key_length)
/*[clinic end generated code: output=c81a69300ef41766 input=365e297bc0b3db48]*/
{
    PyObject *v;
    datum dbm_key, nextkey;
    _gdbm_state *state = PyType_GetModuleState(cls);
    assert(state != NULL);

    dbm_key.dptr = (char *)key;
    dbm_key.dsize = key_length;
    check_gdbmobject_open(self, state->gdbm_error);
    nextkey = gdbm_nextkey(self->di_dbm, dbm_key);
    if (nextkey.dptr) {
        v = PyBytes_FromStringAndSize(nextkey.dptr, nextkey.dsize);
        free(nextkey.dptr);
        return v;
    }
    else {
        Py_RETURN_NONE;
    }
}

/*[clinic input]
_gdbm.gdbm.reorganize

    cls: defining_class

Reorganize the database.

If you have carried out a lot of deletions and would like to shrink
the space used by the GDBM file, this routine will reorganize the
database.  GDBM will not shorten the length of a database file except
by using this reorganization; otherwise, deleted file space will be
kept and reused as new (key,value) pairs are added.
[clinic start generated code]*/

static PyObject *
_gdbm_gdbm_reorganize_impl(gdbmobject *self, PyTypeObject *cls)
/*[clinic end generated code: output=d77c69e8e3dd644a input=e1359faeef844e46]*/
{
    _gdbm_state *state = PyType_GetModuleState(cls);
    assert(state != NULL);
    check_gdbmobject_open(self, state->gdbm_error);
    errno = 0;
    if (gdbm_reorganize(self->di_dbm) < 0) {
        if (errno != 0) {
            PyErr_SetFromErrno(state->gdbm_error);
        }
        else {
            set_gdbm_error(state, "gdbm_reorganize() error");
        }
        return NULL;
    }
    Py_RETURN_NONE;
}

/*[clinic input]
_gdbm.gdbm.sync

    cls: defining_class

Flush the database to the disk file.

When the database has been opened in fast mode, this method forces
any unwritten data to be written to the disk.
[clinic start generated code]*/

static PyObject *
_gdbm_gdbm_sync_impl(gdbmobject *self, PyTypeObject *cls)
/*[clinic end generated code: output=bb680a2035c3f592 input=3d749235f79b6f2a]*/
{
    _gdbm_state *state = PyType_GetModuleState(cls);
    assert(state != NULL);
    check_gdbmobject_open(self, state->gdbm_error);
    gdbm_sync(self->di_dbm);
    Py_RETURN_NONE;
}

/*[clinic input]
_gdbm.gdbm.clear
    cls: defining_class
    /
Remove all items from the database.

[clinic start generated code]*/

static PyObject *
_gdbm_gdbm_clear_impl(gdbmobject *self, PyTypeObject *cls)
/*[clinic end generated code: output=673577c573318661 input=34136d52fcdd4210]*/
{
    _gdbm_state *state = PyType_GetModuleState(cls);
    assert(state != NULL);
    check_gdbmobject_open(self, state->gdbm_error);
    datum key;
    // Invalidate cache
    self->di_size = -1;
    while (1) {
        key = gdbm_firstkey(self->di_dbm);
        if (key.dptr == NULL) {
            break;
        }
        if (gdbm_delete(self->di_dbm, key) < 0) {
            PyErr_SetString(state->gdbm_error, "cannot delete item from database");
            return NULL;
        }
    }
    Py_RETURN_NONE;
}

static PyObject *
gdbm__enter__(PyObject *self, PyObject *args)
{
    return Py_NewRef(self);
}

static PyObject *
gdbm__exit__(PyObject *self, PyObject *args)
{
    return _gdbm_gdbm_close_impl((gdbmobject *)self);
}

static PyMethodDef gdbm_methods[] = {
    _GDBM_GDBM_CLOSE_METHODDEF
    _GDBM_GDBM_KEYS_METHODDEF
    _GDBM_GDBM_FIRSTKEY_METHODDEF
    _GDBM_GDBM_NEXTKEY_METHODDEF
    _GDBM_GDBM_REORGANIZE_METHODDEF
    _GDBM_GDBM_SYNC_METHODDEF
    _GDBM_GDBM_GET_METHODDEF
    _GDBM_GDBM_SETDEFAULT_METHODDEF
    _GDBM_GDBM_CLEAR_METHODDEF
    {"__enter__", gdbm__enter__, METH_NOARGS, NULL},
    {"__exit__",  gdbm__exit__, METH_VARARGS, NULL},
    {NULL,              NULL}           /* sentinel */
};

static PyType_Slot gdbmtype_spec_slots[] = {
    {Py_tp_dealloc, gdbm_dealloc},
    {Py_tp_traverse, gdbm_traverse},
    {Py_tp_methods, gdbm_methods},
    {Py_sq_contains, gdbm_contains},
    {Py_mp_length, gdbm_length},
    {Py_mp_subscript, gdbm_subscript},
    {Py_mp_ass_subscript, gdbm_ass_sub},
    {Py_nb_bool, gdbm_bool},
    {Py_tp_doc, (char*)gdbm_object__doc__},
    {0, 0}
};

static PyType_Spec gdbmtype_spec = {
    .name = "_gdbm.gdbm",
    .basicsize = sizeof(gdbmobject),
    // Calling PyType_GetModuleState() on a subclass is not safe.
    // dbmtype_spec does not have Py_TPFLAGS_BASETYPE flag
    // which prevents to create a subclass.
    // So calling PyType_GetModuleState() in this file is always safe.
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION |
              Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_IMMUTABLETYPE),
    .slots = gdbmtype_spec_slots,
};

/* ----------------------------------------------------------------- */

/*[clinic input]
_gdbm.open as dbmopen

    filename: object
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
/*[clinic end generated code: output=9527750f5df90764 input=bca6ec81dc49292c]*/
{
    int iflags;
    _gdbm_state *state = get_gdbm_state(module);
    assert(state != NULL);

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
        PyErr_SetString(state->gdbm_error,
                        "First flag must be one of 'r', 'w', 'c' or 'n'");
        return NULL;
    }
    for (flags++; *flags != '\0'; flags++) {
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
                PyErr_Format(state->gdbm_error,
                             "Flag '%c' is not supported.", (unsigned char)*flags);
                return NULL;
        }
    }

    PyObject *filenamebytes;
    if (!PyUnicode_FSConverter(filename, &filenamebytes)) {
        return NULL;
    }

    const char *name = PyBytes_AS_STRING(filenamebytes);
    if (strlen(name) != (size_t)PyBytes_GET_SIZE(filenamebytes)) {
        Py_DECREF(filenamebytes);
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        return NULL;
    }
    PyObject *self = newgdbmobject(state, name, iflags, mode);
    Py_DECREF(filenamebytes);
    return self;
}

static const char gdbmmodule_open_flags[] = "rwcn"
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

static PyMethodDef _gdbm_module_methods[] = {
    DBMOPEN_METHODDEF
    { 0, 0 },
};

static int
_gdbm_exec(PyObject *module)
{
    _gdbm_state *state = get_gdbm_state(module);
    state->gdbm_type = (PyTypeObject *)PyType_FromModuleAndSpec(module,
                                                        &gdbmtype_spec, NULL);
    if (state->gdbm_type == NULL) {
        return -1;
    }
    state->gdbm_error = PyErr_NewException("_gdbm.error", PyExc_OSError, NULL);
    if (state->gdbm_error == NULL) {
        return -1;
    }
    if (PyModule_AddType(module, (PyTypeObject *)state->gdbm_error) < 0) {
        return -1;
    }
    if (PyModule_AddStringConstant(module, "open_flags",
                                   gdbmmodule_open_flags) < 0) {
        return -1;
    }

#if defined(GDBM_VERSION_MAJOR) && defined(GDBM_VERSION_MINOR) && \
    defined(GDBM_VERSION_PATCH)
    PyObject *obj = Py_BuildValue("iii", GDBM_VERSION_MAJOR,
                                  GDBM_VERSION_MINOR, GDBM_VERSION_PATCH);
    if (PyModule_Add(module, "_GDBM_VERSION", obj) < 0) {
        return -1;
    }
#endif
    return 0;
}

static int
_gdbm_module_traverse(PyObject *module, visitproc visit, void *arg)
{
    _gdbm_state *state = get_gdbm_state(module);
    Py_VISIT(state->gdbm_error);
    Py_VISIT(state->gdbm_type);
    return 0;
}

static int
_gdbm_module_clear(PyObject *module)
{
    _gdbm_state *state = get_gdbm_state(module);
    Py_CLEAR(state->gdbm_error);
    Py_CLEAR(state->gdbm_type);
    return 0;
}

static void
_gdbm_module_free(void *module)
{
    _gdbm_module_clear((PyObject *)module);
}

static PyModuleDef_Slot _gdbm_module_slots[] = {
    {Py_mod_exec, _gdbm_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL}
};

static struct PyModuleDef _gdbmmodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "_gdbm",
    .m_doc = gdbmmodule__doc__,
    .m_size = sizeof(_gdbm_state),
    .m_methods = _gdbm_module_methods,
    .m_slots = _gdbm_module_slots,
    .m_traverse = _gdbm_module_traverse,
    .m_clear = _gdbm_module_clear,
    .m_free = _gdbm_module_free,
};

PyMODINIT_FUNC
PyInit__gdbm(void)
{
    return PyModuleDef_Init(&_gdbmmodule);
}
