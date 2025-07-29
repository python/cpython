
/* DBM module using dictionary interface */


// clinic/_dbmmodule.c.h uses internal pycore_modsupport.h API
#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "Python.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/* Some Linux systems install gdbm/ndbm.h, but not ndbm.h.  This supports
 * whichever configure was able to locate.
 */
#if defined(USE_GDBM_COMPAT)
  #ifdef HAVE_GDBM_NDBM_H
    #include <gdbm/ndbm.h>
  #elif HAVE_GDBM_DASH_NDBM_H
    #include <gdbm-ndbm.h>
  #else
    #error "No gdbm/ndbm.h or gdbm-ndbm.h available"
  #endif
  static const char which_dbm[] = "GNU gdbm";
#elif defined(USE_NDBM)
  #include <ndbm.h>
  static const char which_dbm[] = "GNU gdbm";
#elif defined(USE_BERKDB)
  #ifndef DB_DBM_HSEARCH
    #define DB_DBM_HSEARCH 1
  #endif
  #include <db.h>
  static const char which_dbm[] = "Berkeley DB";
#else
  #error "No ndbm.h available!"
#endif

typedef struct {
    PyTypeObject *dbm_type;
    PyObject *dbm_error;
} _dbm_state;

static inline _dbm_state*
get_dbm_state(PyObject *module)
{
    void *state = PyModule_GetState(module);
    assert(state != NULL);
    return (_dbm_state *)state;
}

/*[clinic input]
module _dbm
class _dbm.dbm "dbmobject *" "&Dbmtype"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=9b1aa8756d16150e]*/

typedef struct {
    PyObject_HEAD
    int flags;
    int di_size;        /* -1 means recompute */
    DBM *di_dbm;
} dbmobject;

#define dbmobject_CAST(op)  ((dbmobject *)(op))

#include "clinic/_dbmmodule.c.h"

#define check_dbmobject_open(v, err)                                \
    _Py_CRITICAL_SECTION_ASSERT_OBJECT_LOCKED((v))                  \
    if ((v)->di_dbm == NULL) {                                      \
        PyErr_SetString(err, "DBM object has already been closed"); \
        return NULL;                                                \
    }

static PyObject *
newdbmobject(_dbm_state *state, const char *file, int flags, int mode)
{
    dbmobject *dp = PyObject_GC_New(dbmobject, state->dbm_type);
    if (dp == NULL) {
        return NULL;
    }
    dp->di_size = -1;
    dp->flags = flags;
    PyObject_GC_Track(dp);

    /* See issue #19296 */
    if ( (dp->di_dbm = dbm_open((char *)file, flags, mode)) == 0 ) {
        PyErr_SetFromErrnoWithFilename(state->dbm_error, file);
        Py_DECREF(dp);
        return NULL;
    }
    return (PyObject *)dp;
}

/* Methods */
static int
dbm_traverse(PyObject *dp, visitproc visit, void *arg)
{
    Py_VISIT(Py_TYPE(dp));
    return 0;
}

static void
dbm_dealloc(PyObject *self)
{
    dbmobject *dp = dbmobject_CAST(self);
    PyObject_GC_UnTrack(dp);
    if (dp->di_dbm) {
        dbm_close(dp->di_dbm);
    }
    PyTypeObject *tp = Py_TYPE(dp);
    tp->tp_free(dp);
    Py_DECREF(tp);
}

static Py_ssize_t
dbm_length_lock_held(PyObject *self)
{
    dbmobject *dp = dbmobject_CAST(self);
    _dbm_state *state = PyType_GetModuleState(Py_TYPE(dp));
    assert(state != NULL);
    if (dp->di_dbm == NULL) {
             PyErr_SetString(state->dbm_error, "DBM object has already been closed");
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

static Py_ssize_t
dbm_length(PyObject *self)
{
    Py_ssize_t result;
    Py_BEGIN_CRITICAL_SECTION(self);
    result = dbm_length_lock_held(self);
    Py_END_CRITICAL_SECTION();
    return result;
}

static int
dbm_bool_lock_held(PyObject *self)
{
    dbmobject *dp = dbmobject_CAST(self);
    _dbm_state *state = PyType_GetModuleState(Py_TYPE(dp));
    assert(state != NULL);

    if (dp->di_dbm == NULL) {
        PyErr_SetString(state->dbm_error, "DBM object has already been closed");
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
    datum key = dbm_firstkey(dp->di_dbm);
    if (key.dptr == NULL) {
        /* Empty. Cache this fact. */
        dp->di_size = 0;
        return 0;
    }
    /* Non-empty. Don't cache the length since we don't know. */
    return 1;
}

static int
dbm_bool(PyObject *self)
{
    int result;
    Py_BEGIN_CRITICAL_SECTION(self);
    result = dbm_bool_lock_held(self);
    Py_END_CRITICAL_SECTION();
    return result;
}

static PyObject *
dbm_subscript_lock_held(PyObject *self, PyObject *key)
{
    datum drec, krec;
    Py_ssize_t tmp_size;
    dbmobject *dp = dbmobject_CAST(self);
    _dbm_state *state = PyType_GetModuleState(Py_TYPE(dp));
    assert(state != NULL);
    if (!PyArg_Parse(key, "s#", &krec.dptr, &tmp_size)) {
        return NULL;
    }

    krec.dsize = tmp_size;
    check_dbmobject_open(dp, state->dbm_error);
    drec = dbm_fetch(dp->di_dbm, krec);
    if ( drec.dptr == 0 ) {
        PyErr_SetObject(PyExc_KeyError, key);
        return NULL;
    }
    if ( dbm_error(dp->di_dbm) ) {
        dbm_clearerr(dp->di_dbm);
        PyErr_SetString(state->dbm_error, "");
        return NULL;
    }
    return PyBytes_FromStringAndSize(drec.dptr, drec.dsize);
}

static PyObject *
dbm_subscript(PyObject *self, PyObject *key)
{
    PyObject *result;
    Py_BEGIN_CRITICAL_SECTION(self);
    result = dbm_subscript_lock_held(self, key);
    Py_END_CRITICAL_SECTION();
    return result;
}

static int
dbm_ass_sub_lock_held(PyObject *self, PyObject *v, PyObject *w)
{
    datum krec, drec;
    Py_ssize_t tmp_size;
    dbmobject *dp = dbmobject_CAST(self);

    if ( !PyArg_Parse(v, "s#", &krec.dptr, &tmp_size) ) {
        PyErr_SetString(PyExc_TypeError,
                        "dbm mappings have bytes or string keys only");
        return -1;
    }
    _dbm_state *state = PyType_GetModuleState(Py_TYPE(dp));
    assert(state != NULL);
    krec.dsize = tmp_size;
    if (dp->di_dbm == NULL) {
             PyErr_SetString(state->dbm_error, "DBM object has already been closed");
             return -1;
    }
    dp->di_size = -1;
    if (w == NULL) {
        if ( dbm_delete(dp->di_dbm, krec) < 0 ) {
            dbm_clearerr(dp->di_dbm);
            /* we might get a failure for reasons like file corrupted,
               but we are not able to distinguish it */
            if (dp->flags & O_RDWR) {
                PyErr_SetObject(PyExc_KeyError, v);
            }
            else {
                PyErr_SetString(state->dbm_error, "cannot delete item from database");
            }
            return -1;
        }
    } else {
        if ( !PyArg_Parse(w, "s#", &drec.dptr, &tmp_size) ) {
            PyErr_SetString(PyExc_TypeError,
                 "dbm mappings have bytes or string elements only");
            return -1;
        }
        drec.dsize = tmp_size;
        if ( dbm_store(dp->di_dbm, krec, drec, DBM_REPLACE) < 0 ) {
            dbm_clearerr(dp->di_dbm);
            PyErr_SetString(state->dbm_error,
                            "cannot add item to database");
            return -1;
        }
    }
    if ( dbm_error(dp->di_dbm) ) {
        dbm_clearerr(dp->di_dbm);
        PyErr_SetString(state->dbm_error, "");
        return -1;
    }
    return 0;
}

static int
dbm_ass_sub(PyObject *self, PyObject *v, PyObject *w)
{
    int result;
    Py_BEGIN_CRITICAL_SECTION(self);
    result = dbm_ass_sub_lock_held(self, v, w);
    Py_END_CRITICAL_SECTION();
    return result;
}

/*[clinic input]
@critical_section
_dbm.dbm.close

Close the database.
[clinic start generated code]*/

static PyObject *
_dbm_dbm_close_impl(dbmobject *self)
/*[clinic end generated code: output=c8dc5b6709600b86 input=4a94f79facbc28ca]*/
{
    if (self->di_dbm) {
        dbm_close(self->di_dbm);
    }
    self->di_dbm = NULL;
    Py_RETURN_NONE;
}

/*[clinic input]
@critical_section
_dbm.dbm.keys

    cls: defining_class

Return a list of all keys in the database.
[clinic start generated code]*/

static PyObject *
_dbm_dbm_keys_impl(dbmobject *self, PyTypeObject *cls)
/*[clinic end generated code: output=f2a593b3038e5996 input=6ddefeadf2a80156]*/
{
    PyObject *v, *item;
    datum key;
    int err;

    _dbm_state *state = PyType_GetModuleState(cls);
    assert(state != NULL);
    check_dbmobject_open(self, state->dbm_error);
    v = PyList_New(0);
    if (v == NULL) {
        return NULL;
    }
    for (key = dbm_firstkey(self->di_dbm); key.dptr;
         key = dbm_nextkey(self->di_dbm)) {
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
dbm_contains_lock_held(PyObject *self, PyObject *arg)
{
    dbmobject *dp = dbmobject_CAST(self);
    datum key, val;
    Py_ssize_t size;

    _dbm_state *state = PyType_GetModuleState(Py_TYPE(dp));
    assert(state != NULL);
    if ((dp)->di_dbm == NULL) {
        PyErr_SetString(state->dbm_error,
                        "DBM object has already been closed");
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
                     "dbm key must be bytes or string, not %.100s",
                     Py_TYPE(arg)->tp_name);
        return -1;
    }
    else {
        key.dptr = PyBytes_AS_STRING(arg);
        key.dsize = PyBytes_GET_SIZE(arg);
    }
    val = dbm_fetch(dp->di_dbm, key);
    return val.dptr != NULL;
}

static int
dbm_contains(PyObject *self, PyObject *arg)
{
    int result;
    Py_BEGIN_CRITICAL_SECTION(self);
    result = dbm_contains_lock_held(self, arg);
    Py_END_CRITICAL_SECTION();
    return result;
}

/*[clinic input]
@critical_section
_dbm.dbm.get
    cls: defining_class
    key: str(accept={str, robuffer}, zeroes=True)
    default: object = None
    /

Return the value for key if present, otherwise default.
[clinic start generated code]*/

static PyObject *
_dbm_dbm_get_impl(dbmobject *self, PyTypeObject *cls, const char *key,
                  Py_ssize_t key_length, PyObject *default_value)
/*[clinic end generated code: output=b4e55f8b6d482bc4 input=1d88a22bb5e55202]*/
{
    datum dbm_key, val;
    _dbm_state *state = PyType_GetModuleState(cls);
    assert(state != NULL);
    dbm_key.dptr = (char *)key;
    dbm_key.dsize = key_length;
    check_dbmobject_open(self, state->dbm_error);
    val = dbm_fetch(self->di_dbm, dbm_key);
    if (val.dptr != NULL) {
        return PyBytes_FromStringAndSize(val.dptr, val.dsize);
    }

    return Py_NewRef(default_value);
}

/*[clinic input]
@critical_section
_dbm.dbm.setdefault
    cls: defining_class
    key: str(accept={str, robuffer}, zeroes=True)
    default: object(c_default="NULL") = b''
    /

Return the value for key if present, otherwise default.

If key is not in the database, it is inserted with default as the value.
[clinic start generated code]*/

static PyObject *
_dbm_dbm_setdefault_impl(dbmobject *self, PyTypeObject *cls, const char *key,
                         Py_ssize_t key_length, PyObject *default_value)
/*[clinic end generated code: output=9c2f6ea6d0fb576c input=c01510ef7571e13b]*/
{
    datum dbm_key, val;
    Py_ssize_t tmp_size;
    _dbm_state *state = PyType_GetModuleState(cls);
    assert(state != NULL);
    dbm_key.dptr = (char *)key;
    dbm_key.dsize = key_length;
    check_dbmobject_open(self, state->dbm_error);
    val = dbm_fetch(self->di_dbm, dbm_key);
    if (val.dptr != NULL) {
        return PyBytes_FromStringAndSize(val.dptr, val.dsize);
    }
    if (default_value == NULL) {
        default_value = PyBytes_FromStringAndSize(NULL, 0);
        if (default_value == NULL) {
            return NULL;
        }
        val.dptr = NULL;
        val.dsize = 0;
    }
    else {
        if ( !PyArg_Parse(default_value, "s#", &val.dptr, &tmp_size) ) {
            PyErr_SetString(PyExc_TypeError,
                "dbm mappings have bytes or string elements only");
            return NULL;
        }
        val.dsize = tmp_size;
        Py_INCREF(default_value);
    }
    if (dbm_store(self->di_dbm, dbm_key, val, DBM_INSERT) < 0) {
        dbm_clearerr(self->di_dbm);
        PyErr_SetString(state->dbm_error, "cannot add item to database");
        Py_DECREF(default_value);
        return NULL;
    }
    return default_value;
}

/*[clinic input]
@critical_section
_dbm.dbm.clear
    cls: defining_class
    /
Remove all items from the database.

[clinic start generated code]*/

static PyObject *
_dbm_dbm_clear_impl(dbmobject *self, PyTypeObject *cls)
/*[clinic end generated code: output=8d126b9e1d01a434 input=a1aa5d99adfb9656]*/
{
    _dbm_state *state = PyType_GetModuleState(cls);
    assert(state != NULL);
    check_dbmobject_open(self, state->dbm_error);
    datum key;
    // Invalidate cache
    self->di_size = -1;
    while (1) {
        key = dbm_firstkey(self->di_dbm);
        if (key.dptr == NULL) {
            break;
        }
        if (dbm_delete(self->di_dbm, key) < 0) {
            dbm_clearerr(self->di_dbm);
            PyErr_SetString(state->dbm_error, "cannot delete item from database");
            return NULL;
        }
    }
    Py_RETURN_NONE;
}

static PyObject *
dbm__enter__(PyObject *self, PyObject *Py_UNUSED(dummy))
{
    return Py_NewRef(self);
}

static PyObject *
dbm__exit__(PyObject *self, PyObject *Py_UNUSED(args))
{
    dbmobject *dp = dbmobject_CAST(self);
    return _dbm_dbm_close_impl(dp);
}

static PyMethodDef dbm_methods[] = {
    _DBM_DBM_CLOSE_METHODDEF
    _DBM_DBM_KEYS_METHODDEF
    _DBM_DBM_GET_METHODDEF
    _DBM_DBM_SETDEFAULT_METHODDEF
    _DBM_DBM_CLEAR_METHODDEF
    {"__enter__", dbm__enter__, METH_NOARGS, NULL},
    {"__exit__",  dbm__exit__, METH_VARARGS, NULL},
    {NULL,  NULL}           /* sentinel */
};

static PyType_Slot dbmtype_spec_slots[] = {
    {Py_tp_dealloc, dbm_dealloc},
    {Py_tp_traverse, dbm_traverse},
    {Py_tp_methods, dbm_methods},
    {Py_sq_contains, dbm_contains},
    {Py_mp_length, dbm_length},
    {Py_mp_subscript, dbm_subscript},
    {Py_mp_ass_subscript, dbm_ass_sub},
    {Py_nb_bool, dbm_bool},
    {0, 0}
};


static PyType_Spec dbmtype_spec = {
    .name = "_dbm.dbm",
    .basicsize = sizeof(dbmobject),
    // Calling PyType_GetModuleState() on a subclass is not safe.
    // dbmtype_spec does not have Py_TPFLAGS_BASETYPE flag
    // which prevents to create a subclass.
    // So calling PyType_GetModuleState() in this file is always safe.
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION |
              Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_IMMUTABLETYPE),
    .slots = dbmtype_spec_slots,
};

/* ----------------------------------------------------------------- */

/*[clinic input]

_dbm.open as dbmopen

    filename: object
        The filename to open.

    flags: str="r"
        How to open the file.  "r" for reading, "w" for writing, etc.

    mode: int(py_default="0o666") = 0o666
        If creating a new file, the mode bits for the new file
        (e.g. os.O_RDWR).

    /

Return a database object.

[clinic start generated code]*/

static PyObject *
dbmopen_impl(PyObject *module, PyObject *filename, const char *flags,
             int mode)
/*[clinic end generated code: output=9527750f5df90764 input=d8cf50a9f81218c8]*/
{
    int iflags;
    _dbm_state *state =  get_dbm_state(module);
    assert(state != NULL);
    if (strcmp(flags, "r") == 0) {
        iflags = O_RDONLY;
    }
    else if (strcmp(flags, "w") == 0) {
        iflags = O_RDWR;
    }
    else if (strcmp(flags, "rw") == 0) {
        /* Backward compatibility */
        iflags = O_RDWR|O_CREAT;
    }
    else if (strcmp(flags, "c") == 0) {
        iflags = O_RDWR|O_CREAT;
    }
    else if (strcmp(flags, "n") == 0) {
        iflags = O_RDWR|O_CREAT|O_TRUNC;
    }
    else {
        PyErr_SetString(state->dbm_error,
                        "arg 2 to open should be 'r', 'w', 'c', or 'n'");
        return NULL;
    }

    PyObject *filenamebytes;
    if (!PyUnicode_FSConverter(filename, &filenamebytes)) {
        return NULL;
    }

    const char *name = PyBytes_AS_STRING(filenamebytes);
    PyObject *self = newdbmobject(state, name, iflags, mode);
    Py_DECREF(filenamebytes);
    return self;
}

static PyMethodDef dbmmodule_methods[] = {
    DBMOPEN_METHODDEF
    { 0, 0 },
};

static int
_dbm_exec(PyObject *module)
{
    _dbm_state *state = get_dbm_state(module);
    state->dbm_type = (PyTypeObject *)PyType_FromModuleAndSpec(module,
                                                        &dbmtype_spec, NULL);
    if (state->dbm_type == NULL) {
        return -1;
    }
    state->dbm_error = PyErr_NewException("_dbm.error", PyExc_OSError, NULL);
    if (state->dbm_error == NULL) {
        return -1;
    }
    if (PyModule_AddStringConstant(module, "library", which_dbm) < 0) {
        return -1;
    }
    if (PyModule_AddType(module, (PyTypeObject *)state->dbm_error) < 0) {
        return -1;
    }
    return 0;
}

static int
_dbm_module_traverse(PyObject *module, visitproc visit, void *arg)
{
    _dbm_state *state = get_dbm_state(module);
    Py_VISIT(state->dbm_error);
    Py_VISIT(state->dbm_type);
    return 0;
}

static int
_dbm_module_clear(PyObject *module)
{
    _dbm_state *state = get_dbm_state(module);
    Py_CLEAR(state->dbm_error);
    Py_CLEAR(state->dbm_type);
    return 0;
}

static void
_dbm_module_free(void *module)
{
    (void)_dbm_module_clear((PyObject *)module);
}

static PyModuleDef_Slot _dbmmodule_slots[] = {
    {Py_mod_exec, _dbm_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL}
};

static struct PyModuleDef _dbmmodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "_dbm",
    .m_size = sizeof(_dbm_state),
    .m_methods = dbmmodule_methods,
    .m_slots = _dbmmodule_slots,
    .m_traverse = _dbm_module_traverse,
    .m_clear = _dbm_module_clear,
    .m_free = _dbm_module_free,
};

PyMODINIT_FUNC
PyInit__dbm(void)
{
    return PyModuleDef_Init(&_dbmmodule);
}
