/*----------------------------------------------------------------------
  Copyright (c) 1999-2001, Digital Creations, Fredericksburg, VA, USA
  and Andrew Kuchling. All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are
  met:

    o Redistributions of source code must retain the above copyright
      notice, this list of conditions, and the disclaimer that follows.

    o Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions, and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.

    o Neither the name of Digital Creations nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY DIGITAL CREATIONS AND CONTRIBUTORS *AS
  IS* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
  TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
  PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL DIGITAL
  CREATIONS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
  OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
  TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
  USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
  DAMAGE.
------------------------------------------------------------------------*/


/*
 * Handwritten code to wrap version 3.x of the Berkeley DB library,
 * written to replace a SWIG-generated file.
 *
 * This module was started by Andrew Kuchling to remove the dependency
 * on SWIG in a package by Gregory P. Smith <greg@electricrain.com> who
 * based his work on a similar package by Robin Dunn <robin@alldunn.com>
 * which wrapped Berkeley DB 2.7.x.
 *
 * Development of this module has now returned full circle back to
 * Robin Dunn who is working in behalf of Digital Creations to complete
 * the wrapping of the DB 3.x API and to build a solid unit test suite.
 *
 * This module contains 5 types:
 *
 * DB           (Database)
 * DBCursor     (Database Cursor)
 * DBEnv        (database environment)
 * DBTxn        (An explicit database transaction)
 * DBLock       (A lock handle)
 *
 */

/* --------------------------------------------------------------------- */

/*
 * Portions of this module, associated unit tests and build scripts are the
 * result of a contract with The Written Word (http://thewrittenword.com/)
 * Many thanks go out to them for causing me to raise the bar on quality and
 * functionality, resulting in a better bsddb3 package for all of us to use.
 *
 * --Robin
 */

/* --------------------------------------------------------------------- */

#include <Python.h>
#include <db.h>

/* --------------------------------------------------------------------- */
/* Various macro definitions */

#define PY_BSDDB_VERSION "3.4.0"

/* 40 = 4.0, 33 = 3.3; this will break if the second number is > 9 */
#define DBVER (DB_VERSION_MAJOR * 10 + DB_VERSION_MINOR)

static char *orig_rcs_id = "/Id: _db.c,v 1.44 2002/06/07 18:24:00 greg Exp /";
static char *rcs_id = "$Id$";


#ifdef WITH_THREAD

/* These are for when calling Python --> C */
#define MYDB_BEGIN_ALLOW_THREADS Py_BEGIN_ALLOW_THREADS;
#define MYDB_END_ALLOW_THREADS Py_END_ALLOW_THREADS;

/* and these are for calling C --> Python */
static PyInterpreterState* _db_interpreterState = NULL;
#define MYDB_BEGIN_BLOCK_THREADS {                              \
        PyThreadState* prevState;                               \
        PyThreadState* newState;                                \
        PyEval_AcquireLock();                                   \
        newState  = PyThreadState_New(_db_interpreterState);    \
        prevState = PyThreadState_Swap(newState);

#define MYDB_END_BLOCK_THREADS                                  \
        newState = PyThreadState_Swap(prevState);               \
        PyThreadState_Clear(newState);                          \
        PyEval_ReleaseLock();                                   \
        PyThreadState_Delete(newState);                         \
        }

#else

#define MYDB_BEGIN_ALLOW_THREADS
#define MYDB_END_ALLOW_THREADS
#define MYDB_BEGIN_BLOCK_THREADS
#define MYDB_END_BLOCK_THREADS

#endif


/* What is the default behaviour when DB->get or DBCursor->get returns a
   DB_NOTFOUND error?  Return None or raise an exception? */
#define GET_RETURNS_NONE_DEFAULT 1


/* Should DB_INCOMPLETE be turned into a warning or an exception? */
#define INCOMPLETE_IS_WARNING 1

/* --------------------------------------------------------------------- */
/* Exceptions */

static PyObject* DBError;               /* Base class, all others derive from this */
static PyObject* DBKeyEmptyError;       /* DB_KEYEMPTY */
static PyObject* DBKeyExistError;       /* DB_KEYEXIST */
static PyObject* DBLockDeadlockError;   /* DB_LOCK_DEADLOCK */
static PyObject* DBLockNotGrantedError; /* DB_LOCK_NOTGRANTED */
static PyObject* DBNotFoundError;       /* DB_NOTFOUND: also derives from KeyError */
static PyObject* DBOldVersionError;     /* DB_OLD_VERSION */
static PyObject* DBRunRecoveryError;    /* DB_RUNRECOVERY */
static PyObject* DBVerifyBadError;      /* DB_VERIFY_BAD */
static PyObject* DBNoServerError;       /* DB_NOSERVER */
static PyObject* DBNoServerHomeError;   /* DB_NOSERVER_HOME */
static PyObject* DBNoServerIDError;     /* DB_NOSERVER_ID */
#if (DBVER >= 33)
static PyObject* DBPageNotFoundError;   /* DB_PAGE_NOTFOUND */
static PyObject* DBSecondaryBadError;   /* DB_SECONDARY_BAD */
#endif

#if !INCOMPLETE_IS_WARNING
static PyObject* DBIncompleteError;     /* DB_INCOMPLETE */
#endif

static PyObject* DBInvalidArgError;     /* EINVAL */
static PyObject* DBAccessError;         /* EACCES */
static PyObject* DBNoSpaceError;        /* ENOSPC */
static PyObject* DBNoMemoryError;       /* ENOMEM */
static PyObject* DBAgainError;          /* EAGAIN */
static PyObject* DBBusyError;           /* EBUSY  */
static PyObject* DBFileExistsError;     /* EEXIST */
static PyObject* DBNoSuchFileError;     /* ENOENT */
static PyObject* DBPermissionsError;    /* EPERM  */



/* --------------------------------------------------------------------- */
/* Structure definitions */

typedef struct {
    PyObject_HEAD
    DB_ENV*     db_env;
    int         flags;             /* saved flags from open() */
    int         closed;
    int         getReturnsNone;
} DBEnvObject;


typedef struct {
    PyObject_HEAD
    DB*             db;
    DBEnvObject*    myenvobj;  /* PyObject containing the DB_ENV */
    int             flags;     /* saved flags from open() */
    int             setflags;  /* saved flags from set_flags() */
    int             haveStat;
    int             getReturnsNone;
#if (DBVER >= 33)
    PyObject*       associateCallback;
    int             primaryDBType;
#endif
} DBObject;


typedef struct {
    PyObject_HEAD
    DBC*            dbc;
    DBObject*       mydb;
} DBCursorObject;


typedef struct {
    PyObject_HEAD
    DB_TXN*         txn;
} DBTxnObject;


typedef struct {
    PyObject_HEAD
    DB_LOCK         lock;
} DBLockObject;



staticforward PyTypeObject DB_Type, DBCursor_Type, DBEnv_Type, DBTxn_Type, DBLock_Type;

#define DBObject_Check(v)           ((v)->ob_type == &DB_Type)
#define DBCursorObject_Check(v)     ((v)->ob_type == &DBCursor_Type)
#define DBEnvObject_Check(v)        ((v)->ob_type == &DBEnv_Type)
#define DBTxnObject_Check(v)        ((v)->ob_type == &DBTxn_Type)
#define DBLockObject_Check(v)       ((v)->ob_type == &DBLock_Type)


/* --------------------------------------------------------------------- */
/* Utility macros and functions */

#define RETURN_IF_ERR()          \
    if (makeDBError(err)) {      \
        return NULL;             \
    }

#define RETURN_NONE()  Py_INCREF(Py_None); return Py_None;

#define CHECK_DB_NOT_CLOSED(dbobj)                              \
    if (dbobj->db == NULL) {                                    \
        PyErr_SetObject(DBError, Py_BuildValue("(is)", 0,       \
                                "DB object has been closed"));  \
        return NULL;                                            \
    }

#define CHECK_ENV_NOT_CLOSED(env)                               \
    if (env->db_env == NULL) {                                  \
        PyErr_SetObject(DBError, Py_BuildValue("(is)", 0,       \
                                 "DBEnv object has been closed"));\
        return NULL;                                            \
    }

#define CHECK_CURSOR_NOT_CLOSED(curs) \
    if (curs->dbc == NULL) {                                  \
        PyErr_SetObject(DBError, Py_BuildValue("(is)", 0,       \
                                 "DBCursor object has been closed"));\
        return NULL;                                            \
    }



#define CHECK_DBFLAG(mydb, flag)    (((mydb)->flags & (flag)) || \
                                     (((mydb)->myenvobj != NULL) && ((mydb)->myenvobj->flags & (flag))))

#define CLEAR_DBT(dbt)              (memset(&(dbt), 0, sizeof(dbt)))

#define FREE_DBT(dbt)               if ((dbt.flags & (DB_DBT_MALLOC|DB_DBT_REALLOC)) && \
                                         dbt.data != NULL) { free(dbt.data); }


static int makeDBError(int err);


/* Return the access method type of the DBObject */
static int _DB_get_type(DBObject* self)
{
#if (DBVER >= 33)
    DBTYPE type;
    int err;
    err = self->db->get_type(self->db, &type);
    if (makeDBError(err)) {
        return -1;
    }
    return type;
#else
    return self->db->get_type(self->db);
#endif
}


/* Create a DBT structure (containing key and data values) from Python
   strings.  Returns 1 on success, 0 on an error. */
static int make_dbt(PyObject* obj, DBT* dbt)
{
    CLEAR_DBT(*dbt);
    if (obj == Py_None) {
        /* no need to do anything, the structure has already been zeroed */
    }
    else if (!PyArg_Parse(obj, "s#", &dbt->data, &dbt->size)) {
        PyErr_SetString(PyExc_TypeError,
                        "Key and Data values must be of type string or None.");
        return 0;
    }
    return 1;
}


/* Recno and Queue DBs can have integer keys.  This function figures out
   what's been given, verifies that it's allowed, and then makes the DBT.

   Caller should call FREE_DBT(key) when done. */
static int make_key_dbt(DBObject* self, PyObject* keyobj, DBT* key, int* pflags)
{
    db_recno_t recno;
    int type;

    CLEAR_DBT(*key);
    if (keyobj == Py_None) {  /* TODO: is None really okay for keys? */
        /* no need to do anything, the structure has already been zeroed */
    }

    else if (PyString_Check(keyobj)) {
        /* verify access method type */
        type = _DB_get_type(self);
        if (type == -1)
            return 0;
        if (type == DB_RECNO || type == DB_QUEUE) {
            PyErr_SetString(PyExc_TypeError, "String keys not allowed for Recno and Queue DB's");
            return 0;
        }

        key->data = PyString_AS_STRING(keyobj);
        key->size = PyString_GET_SIZE(keyobj);
    }

    else if (PyInt_Check(keyobj)) {
        /* verify access method type */
        type = _DB_get_type(self);
        if (type == -1)
            return 0;
        if (type == DB_BTREE && pflags != NULL) {
            /* if BTREE then an Integer key is allowed with the DB_SET_RECNO flag */
            *pflags |= DB_SET_RECNO;
        }
        else if (type != DB_RECNO && type != DB_QUEUE) {
            PyErr_SetString(PyExc_TypeError, "Integer keys only allowed for Recno and Queue DB's");
            return 0;
        }

        /* Make a key out of the requested recno, use allocated space so DB will
           be able to realloc room for the real key if needed. */
        recno = PyInt_AS_LONG(keyobj);
        key->data = malloc(sizeof(db_recno_t));
        if (key->data == NULL) {
            PyErr_SetString(PyExc_MemoryError, "Key memory allocation failed");
            return 0;
        }
        key->ulen = key->size = sizeof(db_recno_t);
        memcpy(key->data, &recno, sizeof(db_recno_t));
        key->flags = DB_DBT_REALLOC;
    }
    else {
        PyErr_Format(PyExc_TypeError,
                     "String or Integer object expected for key, %s found",
                     keyobj->ob_type->tp_name);
        return 0;
    }

    return 1;
}


/* Add partial record access to an existing DBT data struct.
   If dlen and doff are set, then the DB_DBT_PARTIAL flag will be set
   and the data storage/retrieval will be done using dlen and doff. */
static int add_partial_dbt(DBT* d, int dlen, int doff) {
    /* if neither were set we do nothing (-1 is the default value) */
    if ((dlen == -1) && (doff == -1)) {
        return 1;
    }

    if ((dlen < 0) || (doff < 0)) {
        PyErr_SetString(PyExc_TypeError, "dlen and doff must both be >= 0");
        return 0;
    }

    d->flags = d->flags | DB_DBT_PARTIAL;
    d->dlen = (unsigned int) dlen;
    d->doff = (unsigned int) doff;
    return 1;
}


/* Callback used to save away more information about errors from the DB library. */
static char _db_errmsg[1024];
static void _db_errorCallback(const char* prefix, char* msg)
{
    strcpy(_db_errmsg, msg);
}


/* make a nice exception object to raise for errors. */
static int makeDBError(int err)
{
    char errTxt[2048];  /* really big, just in case... */
    PyObject* errObj = NULL;
    int exceptionRaised = 0;

    switch (err) {
        case 0:                     /* successful, no error */      break;

        case DB_INCOMPLETE:
#if INCOMPLETE_IS_WARNING
            strcpy(errTxt, db_strerror(err));
            if (_db_errmsg[0]) {
                strcat(errTxt, " -- ");
                strcat(errTxt, _db_errmsg);
                _db_errmsg[0] = 0;
            }
#if PYTHON_API_VERSION >= 1010 /* if Python 2.1 or better use warning framework */
            exceptionRaised = PyErr_Warn(PyExc_RuntimeWarning, errTxt);
#else
            fprintf(stderr, errTxt);
            fprintf(stderr, "\n");
#endif

#else  /* do an exception instead */
        errObj = DBIncompleteError;
#endif
        break;

        case DB_KEYEMPTY:           errObj = DBKeyEmptyError;       break;
        case DB_KEYEXIST:           errObj = DBKeyExistError;       break;
        case DB_LOCK_DEADLOCK:      errObj = DBLockDeadlockError;   break;
        case DB_LOCK_NOTGRANTED:    errObj = DBLockNotGrantedError; break;
        case DB_NOTFOUND:           errObj = DBNotFoundError;       break;
        case DB_OLD_VERSION:        errObj = DBOldVersionError;     break;
        case DB_RUNRECOVERY:        errObj = DBRunRecoveryError;    break;
        case DB_VERIFY_BAD:         errObj = DBVerifyBadError;      break;
        case DB_NOSERVER:           errObj = DBNoServerError;       break;
        case DB_NOSERVER_HOME:      errObj = DBNoServerHomeError;   break;
        case DB_NOSERVER_ID:        errObj = DBNoServerIDError;     break;
#if (DBVER >= 33)
        case DB_PAGE_NOTFOUND:      errObj = DBPageNotFoundError;   break;
        case DB_SECONDARY_BAD:      errObj = DBSecondaryBadError;   break;
#endif

        case EINVAL:  errObj = DBInvalidArgError;   break;
        case EACCES:  errObj = DBAccessError;       break;
        case ENOSPC:  errObj = DBNoSpaceError;      break;
        case ENOMEM:  errObj = DBNoMemoryError;     break;
        case EAGAIN:  errObj = DBAgainError;        break;
        case EBUSY :  errObj = DBBusyError;         break;
        case EEXIST:  errObj = DBFileExistsError;   break;
        case ENOENT:  errObj = DBNoSuchFileError;   break;
        case EPERM :  errObj = DBPermissionsError;  break;

        default:      errObj = DBError;             break;
    }

    if (errObj != NULL) {
        strcpy(errTxt, db_strerror(err));
        if (_db_errmsg[0]) {
            strcat(errTxt, " -- ");
            strcat(errTxt, _db_errmsg);
            _db_errmsg[0] = 0;
        }
        PyErr_SetObject(errObj, Py_BuildValue("(is)", err, errTxt));
    }

    return ((errObj != NULL) || exceptionRaised);
}



/* set a type exception */
static void makeTypeError(char* expected, PyObject* found)
{
    PyErr_Format(PyExc_TypeError, "Expected %s argument, %s found.",
                 expected, found->ob_type->tp_name);
}


/* verify that an obj is either None or a DBTxn, and set the txn pointer */
static int checkTxnObj(PyObject* txnobj, DB_TXN** txn)
{
    if (txnobj == Py_None || txnobj == NULL) {
        *txn = NULL;
        return 1;
    }
    if (DBTxnObject_Check(txnobj)) {
        *txn = ((DBTxnObject*)txnobj)->txn;
        return 1;
    }
    else
        makeTypeError("DBTxn", txnobj);
    return 0;
}


/* Delete a key from a database
  Returns 0 on success, -1 on an error.  */
static int _DB_delete(DBObject* self, DB_TXN *txn, DBT *key, int flags)
{
    int err;

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->del(self->db, txn, key, 0);
    MYDB_END_ALLOW_THREADS;
    if (makeDBError(err)) {
        return -1;
    }
    self->haveStat = 0;
    return 0;
}


/* Store a key into a database
   Returns 0 on success, -1 on an error.  */
static int _DB_put(DBObject* self, DB_TXN *txn, DBT *key, DBT *data, int flags)
{
    int err;

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->put(self->db, txn, key, data, flags);
    MYDB_END_ALLOW_THREADS;
    if (makeDBError(err)) {
        return -1;
    }
    self->haveStat = 0;
    return 0;
}

/* Get a key/data pair from a cursor */
static PyObject* _DBCursor_get(DBCursorObject* self, int extra_flags,
			       PyObject *args, PyObject *kwargs, char *format)
{
    int err;
    PyObject* retval = NULL;
    DBT key, data;
    int dlen = -1;
    int doff = -1;
    int flags = 0;
    char* kwnames[] = { "flags", "dlen", "doff", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, format, kwnames,
				     &flags, &dlen, &doff)) 
      return NULL;

    CHECK_CURSOR_NOT_CLOSED(self);

    flags |= extra_flags;
    CLEAR_DBT(key);
    CLEAR_DBT(data);
    if (CHECK_DBFLAG(self->mydb, DB_THREAD)) {
        /* Tell BerkeleyDB to malloc the return value (thread safe) */
        data.flags = DB_DBT_MALLOC;
        key.flags = DB_DBT_MALLOC;
    }
    if (!add_partial_dbt(&data, dlen, doff))
        return NULL;

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->dbc->c_get(self->dbc, &key, &data, flags);
    MYDB_END_ALLOW_THREADS;

    if ((err == DB_NOTFOUND) && self->mydb->getReturnsNone) {
        Py_INCREF(Py_None);
        retval = Py_None;
    }
    else if (makeDBError(err)) {
        retval = NULL;
    }
    else {  /* otherwise, success! */

        /* if Recno or Queue, return the key as an Int */
        switch (_DB_get_type(self->mydb)) {
        case -1:
            retval = NULL;
            break;

        case DB_RECNO:
        case DB_QUEUE:
            retval = Py_BuildValue("is#", *((db_recno_t*)key.data),
                                   data.data, data.size);
            break;
        case DB_HASH:
        case DB_BTREE:
        default:
            retval = Py_BuildValue("s#s#", key.data, key.size,
                                   data.data, data.size);
            break;
        }
    }
    if (!err) {
        FREE_DBT(key);
        FREE_DBT(data);
    }
    return retval;
}


/* add an integer to a dictionary using the given name as a key */
static void _addIntToDict(PyObject* dict, char *name, int value)
{
    PyObject* v = PyInt_FromLong((long) value);
    if (!v || PyDict_SetItemString(dict, name, v))
        PyErr_Clear();

    Py_XDECREF(v);
}




/* --------------------------------------------------------------------- */
/* Allocators and deallocators */

static DBObject*
newDBObject(DBEnvObject* arg, int flags)
{
    DBObject* self;
    DB_ENV* db_env = NULL;
    int err;

#if PYTHON_API_VERSION <= 1007
    /* 1.5 compatibility */
    self = PyObject_NEW(DBObject, &DB_Type);
#else
    self = PyObject_New(DBObject, &DB_Type);
#endif

    if (self == NULL)
        return NULL;

    self->haveStat = 0;
    self->flags = 0;
    self->setflags = 0;
    self->myenvobj = NULL;
#if (DBVER >= 33)
    self->associateCallback = NULL;
    self->primaryDBType = 0;
#endif

    /* keep a reference to our python DBEnv object */
    if (arg) {
        Py_INCREF(arg);
        self->myenvobj = arg;
        db_env = arg->db_env;
    }

    if (self->myenvobj)
        self->getReturnsNone = self->myenvobj->getReturnsNone;
    else
        self->getReturnsNone = GET_RETURNS_NONE_DEFAULT;

    MYDB_BEGIN_ALLOW_THREADS;
    err = db_create(&self->db, db_env, flags);
    self->db->set_errcall(self->db, _db_errorCallback);
#if (DBVER >= 33)
    self->db->app_private = (void*)self;
#endif
    MYDB_END_ALLOW_THREADS;
    if (makeDBError(err)) {
        if (self->myenvobj) {
            Py_DECREF(self->myenvobj);
            self->myenvobj = NULL;
        }
        self = NULL;
    }
    return self;
}


static void
DB_dealloc(DBObject* self)
{
    if (self->db != NULL) {
        /* avoid closing a DB when its DBEnv has been closed out from under it */
        if (!self->myenvobj ||
            (self->myenvobj && self->myenvobj->db_env)) {
            MYDB_BEGIN_ALLOW_THREADS;
            self->db->close(self->db, 0);
            MYDB_END_ALLOW_THREADS;
#if PYTHON_API_VERSION >= 1010 /* if Python 2.1 or better use warning framework */
        } else {
            PyErr_Warn(PyExc_RuntimeWarning,
                "DB could not be closed in destructor: DBEnv already closed");
#endif
        }
        self->db = NULL;
    }
    if (self->myenvobj) {
        Py_DECREF(self->myenvobj);
        self->myenvobj = NULL;
    }
#if (DBVER >= 33)
    if (self->associateCallback != NULL) {
        Py_DECREF(self->associateCallback);
        self->associateCallback = NULL;
    }
#endif
#if PYTHON_API_VERSION <= 1007
    PyMem_DEL(self);
#else
    PyObject_Del(self);
#endif
}


static DBCursorObject*
newDBCursorObject(DBC* dbc, DBObject* db)
{
    DBCursorObject* self;
#if PYTHON_API_VERSION <= 1007
    self = PyObject_NEW(DBCursorObject, &DBCursor_Type);
#else
    self = PyObject_New(DBCursorObject, &DBCursor_Type);
#endif
    if (self == NULL)
        return NULL;

    self->dbc = dbc;
    self->mydb = db;
    Py_INCREF(self->mydb);
    return self;
}


static void
DBCursor_dealloc(DBCursorObject* self)
{
    int err;
    if (self->dbc != NULL) {
        MYDB_BEGIN_ALLOW_THREADS;
        err = self->dbc->c_close(self->dbc);
        self->dbc = NULL;
        MYDB_END_ALLOW_THREADS;
    }
    Py_XDECREF( self->mydb );
#if PYTHON_API_VERSION <= 1007
    PyMem_DEL(self);
#else
    PyObject_Del(self);
#endif
}


static DBEnvObject*
newDBEnvObject(int flags)
{
    int err;
    DBEnvObject* self;
#if PYTHON_API_VERSION <= 1007
    self = PyObject_NEW(DBEnvObject, &DBEnv_Type);
#else
    self = PyObject_New(DBEnvObject, &DBEnv_Type);
#endif

    if (self == NULL)
        return NULL;

    self->closed = 1;
    self->flags = flags;
    self->getReturnsNone = GET_RETURNS_NONE_DEFAULT;

    MYDB_BEGIN_ALLOW_THREADS;
    err = db_env_create(&self->db_env, flags);
    MYDB_END_ALLOW_THREADS;
    if (makeDBError(err)) {
        self = NULL;
    }
    else {
        self->db_env->set_errcall(self->db_env, _db_errorCallback);
    }
    return self;
}


static void
DBEnv_dealloc(DBEnvObject* self)
{
    if (!self->closed) {
        MYDB_BEGIN_ALLOW_THREADS;
        self->db_env->close(self->db_env, 0);
        MYDB_END_ALLOW_THREADS;
    }
#if PYTHON_API_VERSION <= 1007
    PyMem_DEL(self);
#else
    PyObject_Del(self);
#endif
}


static DBTxnObject*
newDBTxnObject(DBEnvObject* myenv, DB_TXN *parent, int flags)
{
    int err;
    DBTxnObject* self;

#if PYTHON_API_VERSION <= 1007
    self = PyObject_NEW(DBTxnObject, &DBTxn_Type);
#else
    self = PyObject_New(DBTxnObject, &DBTxn_Type);
#endif
    if (self == NULL)
        return NULL;

    MYDB_BEGIN_ALLOW_THREADS;
#if (DBVER >= 40)
    err = myenv->db_env->txn_begin(myenv->db_env, parent, &(self->txn), flags);
#else
    err = txn_begin(myenv->db_env, parent, &(self->txn), flags);
#endif
    MYDB_END_ALLOW_THREADS;
    if (makeDBError(err)) {
        self = NULL;
    }
    return self;
}


static void
DBTxn_dealloc(DBTxnObject* self)
{
    /* XXX nothing to do for transaction objects?!? */

    /* TODO: if it hasn't been commited, should we abort it? */

#if PYTHON_API_VERSION <= 1007
    PyMem_DEL(self);
#else
    PyObject_Del(self);
#endif
}


static DBLockObject*
newDBLockObject(DBEnvObject* myenv, u_int32_t locker, DBT* obj,
                db_lockmode_t lock_mode, int flags)
{
    int err;
    DBLockObject* self;

#if PYTHON_API_VERSION <= 1007
    self = PyObject_NEW(DBLockObject, &DBLock_Type);
#else
    self = PyObject_New(DBLockObject, &DBLock_Type);
#endif
    if (self == NULL)
        return NULL;

    MYDB_BEGIN_ALLOW_THREADS;
#if (DBVER >= 40)
    err = myenv->db_env->lock_get(myenv->db_env, locker, flags, obj, lock_mode, &self->lock);
#else
    err = lock_get(myenv->db_env, locker, flags, obj, lock_mode, &self->lock);
#endif
    MYDB_END_ALLOW_THREADS;
    if (makeDBError(err)) {
        self = NULL;
    }

    return self;
}


static void
DBLock_dealloc(DBLockObject* self)
{
    /* TODO: if it hasn't been released, should we do it? */

#if PYTHON_API_VERSION <= 1007
    PyMem_DEL(self);
#else
    PyObject_Del(self);
#endif
}


/* --------------------------------------------------------------------- */
/* DB methods */

static PyObject*
DB_append(DBObject* self, PyObject* args)
{
    PyObject* txnobj = NULL;
    PyObject* dataobj;
    db_recno_t recno;
    DBT key, data;
    DB_TXN *txn = NULL;

    if (!PyArg_ParseTuple(args, "O|O:append", &dataobj, &txnobj))
        return NULL;

    CHECK_DB_NOT_CLOSED(self);

    /* make a dummy key out of a recno */
    recno = 0;
    CLEAR_DBT(key);
    key.data = &recno;
    key.size = sizeof(recno);
    key.ulen = key.size;
    key.flags = DB_DBT_USERMEM;

    if (!make_dbt(dataobj, &data)) return NULL;
    if (!checkTxnObj(txnobj, &txn)) return NULL;

    if (-1 == _DB_put(self, txn, &key, &data, DB_APPEND))
        return NULL;

    return PyInt_FromLong(recno);
}


#if (DBVER >= 33)

static int
_db_associateCallback(DB* db, const DBT* priKey, const DBT* priData, DBT* secKey)
{
    int       retval = DB_DONOTINDEX;
    DBObject* secondaryDB = (DBObject*)db->app_private;
    PyObject* callback = secondaryDB->associateCallback;
    int       type = secondaryDB->primaryDBType;
    PyObject* key;
    PyObject* data;
    PyObject* args;
    PyObject* result;


    if (callback != NULL) {
        MYDB_BEGIN_BLOCK_THREADS;

        if (type == DB_RECNO || type == DB_QUEUE) {
            key = PyInt_FromLong( *((db_recno_t*)priKey->data));
        }
        else {
            key  = PyString_FromStringAndSize(priKey->data, priKey->size);
        }
        data = PyString_FromStringAndSize(priData->data, priData->size);
        args = PyTuple_New(2);
        PyTuple_SET_ITEM(args, 0, key);  /* steals reference */
        PyTuple_SET_ITEM(args, 1, data); /* steals reference */

        result = PyEval_CallObject(callback, args);

        if (result == NULL) {
            PyErr_Print();
        }
        else if (result == Py_None) {
            retval = DB_DONOTINDEX;
        }
        else if (PyInt_Check(result)) {
            retval = PyInt_AsLong(result);
        }
        else if (PyString_Check(result)) {
            char* data;
            int   size;

            CLEAR_DBT(*secKey);
#if PYTHON_API_VERSION <= 1007
            /* 1.5 compatibility */
            size = PyString_Size(result);
            data = PyString_AsString(result);
#else
            PyString_AsStringAndSize(result, &data, &size);
#endif
            secKey->flags = DB_DBT_APPMALLOC;   /* DB will free */
            secKey->data = malloc(size);        /* TODO, check this */
            memcpy(secKey->data, data, size);
            secKey->size = size;
            retval = 0;
        }
        else {
            PyErr_SetString(PyExc_TypeError,
                            "DB associate callback should return DB_DONOTINDEX or a string.");
            PyErr_Print();
        }

        Py_DECREF(args);
        if (result) {
            Py_DECREF(result);
        }

        MYDB_END_BLOCK_THREADS;
    }
    return retval;
}


static PyObject*
DB_associate(DBObject* self, PyObject* args, PyObject* kwargs)
{
    int err, flags=0;
    DBObject* secondaryDB;
    PyObject* callback;
    char* kwnames[] = {"secondaryDB", "callback", "flags", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OO|i:associate", kwnames,
                                     &secondaryDB, &callback, &flags))
        return NULL;

    CHECK_DB_NOT_CLOSED(self);
    if (!DBObject_Check(secondaryDB)) {
        makeTypeError("DB", (PyObject*)secondaryDB);
        return NULL;
    }
    if (callback == Py_None) {
        callback = NULL;
    }
    else if (!PyCallable_Check(callback)) {
        makeTypeError("Callable", callback);
        return NULL;
    }

    /* Save a reference to the callback in the secondary DB. */
    if (self->associateCallback != NULL) {
        Py_DECREF(self->associateCallback);
    }
    Py_INCREF(callback);
    secondaryDB->associateCallback = callback;
    secondaryDB->primaryDBType = _DB_get_type(self);


    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->associate(self->db,
                              secondaryDB->db,
                              _db_associateCallback,
                              flags);
    MYDB_END_ALLOW_THREADS;

    if (err) {
        Py_DECREF(self->associateCallback);
        self->associateCallback = NULL;
        secondaryDB->primaryDBType = 0;
    }

    RETURN_IF_ERR();
    RETURN_NONE();
}


#endif


static PyObject*
DB_close(DBObject* self, PyObject* args)
{
    int err, flags=0;
    if (!PyArg_ParseTuple(args,"|i:close", &flags))
        return NULL;
    if (self->db != NULL) {
        if (self->myenvobj)
            CHECK_ENV_NOT_CLOSED(self->myenvobj);
        MYDB_BEGIN_ALLOW_THREADS;
        err = self->db->close(self->db, flags);
        MYDB_END_ALLOW_THREADS;
        self->db = NULL;
        RETURN_IF_ERR();
    }
    RETURN_NONE();
}


#if (DBVER >= 32)
static PyObject*
_DB_consume(DBObject* self, PyObject* args, PyObject* kwargs, int consume_flag)
{
    int err, flags=0, type;
    PyObject* txnobj = NULL;
    PyObject* retval = NULL;
    DBT key, data;
    DB_TXN *txn = NULL;
    char* kwnames[] = { "txn", "flags", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|Oi:consume", kwnames,
                                     &txnobj, &flags))
        return NULL;

    CHECK_DB_NOT_CLOSED(self);
    type = _DB_get_type(self);
    if (type == -1)
        return NULL;
    if (type != DB_QUEUE) {
        PyErr_SetString(PyExc_TypeError, "Consume methods only allowed for Queue DB's");
        return NULL;
    }
    if (!checkTxnObj(txnobj, &txn))
        return NULL;

    CLEAR_DBT(key);
    CLEAR_DBT(data);
    if (CHECK_DBFLAG(self, DB_THREAD)) {
        /* Tell BerkeleyDB to malloc the return value (thread safe) */
        data.flags = DB_DBT_MALLOC;
        key.flags = DB_DBT_MALLOC;
    }

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->get(self->db, txn, &key, &data, flags|consume_flag);
    MYDB_END_ALLOW_THREADS;

    if ((err == DB_NOTFOUND) && self->getReturnsNone) {
        err = 0;
        Py_INCREF(Py_None);
        retval = Py_None;
    }
    else if (!err) {
        retval = Py_BuildValue("s#s#", key.data, key.size, data.data, data.size);
        FREE_DBT(key);
        FREE_DBT(data);
    }

    RETURN_IF_ERR();
    return retval;
}

static PyObject*
DB_consume(DBObject* self, PyObject* args, PyObject* kwargs, int consume_flag)
{
    return _DB_consume(self, args, kwargs, DB_CONSUME);
}

static PyObject*
DB_consume_wait(DBObject* self, PyObject* args, PyObject* kwargs, int consume_flag)
{
    return _DB_consume(self, args, kwargs, DB_CONSUME_WAIT);
}
#endif



static PyObject*
DB_cursor(DBObject* self, PyObject* args, PyObject* kwargs)
{
    int err, flags=0;
    DBC* dbc;
    PyObject* txnobj = NULL;
    DB_TXN *txn = NULL;
    char* kwnames[] = { "txn", "flags", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|Oi:cursor", kwnames,
                                     &txnobj, &flags))
        return NULL;
    CHECK_DB_NOT_CLOSED(self);
    if (!checkTxnObj(txnobj, &txn))
        return NULL;

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->cursor(self->db, txn, &dbc, flags);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    return (PyObject*) newDBCursorObject(dbc, self);
}


static PyObject*
DB_delete(DBObject* self, PyObject* args, PyObject* kwargs)
{
    PyObject* txnobj = NULL;
    int flags = 0;
    PyObject* keyobj;
    DBT key;
    DB_TXN *txn = NULL;
    char* kwnames[] = { "key", "txn", "flags", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|Oi:delete", kwnames,
                                     &keyobj, &txnobj, &flags))
        return NULL;
    CHECK_DB_NOT_CLOSED(self);
    if (!make_key_dbt(self, keyobj, &key, NULL))
        return NULL;
    if (!checkTxnObj(txnobj, &txn))
        return NULL;

    if (-1 == _DB_delete(self, txn, &key, 0))
        return NULL;

    FREE_DBT(key);
    RETURN_NONE();
}


static PyObject*
DB_fd(DBObject* self, PyObject* args)
{
    int err, the_fd;

    if (!PyArg_ParseTuple(args,":fd"))
        return NULL;
    CHECK_DB_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->fd(self->db, &the_fd);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    return PyInt_FromLong(the_fd);
}


static PyObject*
DB_get(DBObject* self, PyObject* args, PyObject* kwargs)
{
    int err, flags=0;
    PyObject* txnobj = NULL;
    PyObject* keyobj;
    PyObject* dfltobj = NULL;
    PyObject* retval = NULL;
    int dlen = -1;
    int doff = -1;
    DBT key, data;
    DB_TXN *txn = NULL;
    char* kwnames[] = { "key", "default", "txn", "flags", "dlen", "doff", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|OOiii:get", kwnames,
                                     &keyobj, &dfltobj, &txnobj, &flags, &dlen, &doff))
        return NULL;

    CHECK_DB_NOT_CLOSED(self);
    if (!make_key_dbt(self, keyobj, &key, &flags))
        return NULL;
    if (!checkTxnObj(txnobj, &txn))
        return NULL;

    CLEAR_DBT(data);
    if (CHECK_DBFLAG(self, DB_THREAD)) {
        /* Tell BerkeleyDB to malloc the return value (thread safe) */
        data.flags = DB_DBT_MALLOC;
    }
    if (!add_partial_dbt(&data, dlen, doff))
        return NULL;

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->get(self->db, txn, &key, &data, flags);
    MYDB_END_ALLOW_THREADS;

    if ((err == DB_NOTFOUND) && (dfltobj != NULL)) {
        err = 0;
        Py_INCREF(dfltobj);
        retval = dfltobj;
    }
    else if ((err == DB_NOTFOUND) && self->getReturnsNone) {
        err = 0;
        Py_INCREF(Py_None);
        retval = Py_None;
    }
    else if (!err) {
        if (flags & DB_SET_RECNO) /* return both key and data */
            retval = Py_BuildValue("s#s#", key.data, key.size, data.data, data.size);
        else /* return just the data */
            retval = PyString_FromStringAndSize((char*)data.data, data.size);
        FREE_DBT(key);
        FREE_DBT(data);
    }

    RETURN_IF_ERR();
    return retval;
}


/* Return size of entry */
static PyObject*
DB_get_size(DBObject* self, PyObject* args, PyObject* kwargs)
{
    int err, flags=0;
    PyObject* txnobj = NULL;
    PyObject* keyobj;
    PyObject* retval = NULL;
    DBT key, data;
    DB_TXN *txn = NULL;
    char* kwnames[] = { "key", "txn", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|O:get_size", kwnames,
                                     &keyobj, &txnobj))
        return NULL;
    CHECK_DB_NOT_CLOSED(self);
    if (!make_key_dbt(self, keyobj, &key, &flags))
        return NULL;
    if (!checkTxnObj(txnobj, &txn))
        return NULL;
    CLEAR_DBT(data);

    /* We don't allocate any memory, forcing a ENOMEM error and thus
       getting the record size. */
    data.flags = DB_DBT_USERMEM;
    data.ulen = 0;
    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->get(self->db, txn, &key, &data, flags);
    MYDB_END_ALLOW_THREADS;
    if (err == ENOMEM) {
        retval = PyInt_FromLong((long)data.size);
        err = 0;
    }

    FREE_DBT(key);
    FREE_DBT(data);
    RETURN_IF_ERR();
    return retval;
}


static PyObject*
DB_get_both(DBObject* self, PyObject* args, PyObject* kwargs)
{
    int err, flags=0;
    PyObject* txnobj = NULL;
    PyObject* keyobj;
    PyObject* dataobj;
    PyObject* retval = NULL;
    DBT key, data;
    DB_TXN *txn = NULL;
    char* kwnames[] = { "key", "data", "txn", "flags", NULL };


    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OO|Oi:get_both", kwnames,
                                     &keyobj, &dataobj, &txnobj, &flags))
        return NULL;

    CHECK_DB_NOT_CLOSED(self);
    if (!make_key_dbt(self, keyobj, &key, NULL))
        return NULL;
    if (!make_dbt(dataobj, &data))
        return NULL;
    if (!checkTxnObj(txnobj, &txn))
        return NULL;

    flags |= DB_GET_BOTH;

    if (CHECK_DBFLAG(self, DB_THREAD)) {
        /* Tell BerkeleyDB to malloc the return value (thread safe) */
        data.flags = DB_DBT_MALLOC;
        /* TODO: Is this flag needed?  We're passing a data object that should
                 match what's in the DB, so there should be no need to malloc.
                 We run the risk of freeing something twice!  Check this. */
    }

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->get(self->db, txn, &key, &data, flags);
    MYDB_END_ALLOW_THREADS;

    if ((err == DB_NOTFOUND) && self->getReturnsNone) {
        err = 0;
        Py_INCREF(Py_None);
        retval = Py_None;
    }
    else if (!err) {
        retval = PyString_FromStringAndSize((char*)data.data, data.size);
        FREE_DBT(data); /* Only if retrieval was successful */
    }

    FREE_DBT(key);
    RETURN_IF_ERR();
    return retval;
}


static PyObject*
DB_get_byteswapped(DBObject* self, PyObject* args)
{
#if (DBVER >= 33)
    int err = 0;
#endif
    int retval = -1;

    if (!PyArg_ParseTuple(args,":get_byteswapped"))
        return NULL;
    CHECK_DB_NOT_CLOSED(self);

#if (DBVER >= 33)
    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->get_byteswapped(self->db, &retval);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
#else
    MYDB_BEGIN_ALLOW_THREADS;
    retval = self->db->get_byteswapped(self->db);
    MYDB_END_ALLOW_THREADS;
#endif
    return PyInt_FromLong(retval);
}


static PyObject*
DB_get_type(DBObject* self, PyObject* args)
{
    int type;

    if (!PyArg_ParseTuple(args,":get_type"))
        return NULL;
    CHECK_DB_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    type = _DB_get_type(self);
    MYDB_END_ALLOW_THREADS;
    if (type == -1)
        return NULL;
    return PyInt_FromLong(type);
}


static PyObject*
DB_join(DBObject* self, PyObject* args)
{
    int err, flags=0;
    int length, x;
    PyObject* cursorsObj;
    DBC** cursors;
    DBC*  dbc;


    if (!PyArg_ParseTuple(args,"O|i:join", &cursorsObj, &flags))
        return NULL;

    CHECK_DB_NOT_CLOSED(self);

    if (!PySequence_Check(cursorsObj)) {
        PyErr_SetString(PyExc_TypeError, "Sequence of DBCursor objects expected");
        return NULL;
    }

    length = PyObject_Length(cursorsObj);
    cursors = malloc((length+1) * sizeof(DBC*));
    cursors[length] = NULL;
    for (x=0; x<length; x++) {
        PyObject* item = PySequence_GetItem(cursorsObj, x);
        if (!DBCursorObject_Check(item)) {
            PyErr_SetString(PyExc_TypeError, "Sequence of DBCursor objects expected");
            free(cursors);
            return NULL;
        }
        cursors[x] = ((DBCursorObject*)item)->dbc;
    }

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->join(self->db, cursors, &dbc, flags);
    MYDB_END_ALLOW_THREADS;
    free(cursors);
    RETURN_IF_ERR();

    return (PyObject*) newDBCursorObject(dbc, self);
}


static PyObject*
DB_key_range(DBObject* self, PyObject* args, PyObject* kwargs)
{
    int err, flags=0;
    PyObject* txnobj = NULL;
    PyObject* keyobj;
    DBT key;
    DB_TXN *txn = NULL;
    DB_KEY_RANGE range;
    char* kwnames[] = { "key", "txn", "flags", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|Oi:key_range", kwnames,
                                     &keyobj, &txnobj, &flags))
        return NULL;
    CHECK_DB_NOT_CLOSED(self);
    if (!make_dbt(keyobj, &key))   /* BTree only, don't need to allow for an int key */
        return NULL;
    if (!checkTxnObj(txnobj, &txn))
        return NULL;

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->key_range(self->db, txn, &key, &range, flags);
    MYDB_END_ALLOW_THREADS;

    RETURN_IF_ERR();
    return Py_BuildValue("ddd", range.less, range.equal, range.greater);
}


static PyObject*
DB_open(DBObject* self, PyObject* args, PyObject* kwargs)
{
    int err, type = DB_UNKNOWN, flags=0, mode=0660;
    char* filename = NULL;
    char* dbname = NULL;
    char* kwnames[] = { "filename", "dbname", "dbtype", "flags", "mode", NULL };
    char* kwnames2[] = { "filename", "dbtype", "flags", "mode", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "z|ziii:open", kwnames,
                                     &filename, &dbname, &type, &flags, &mode)) {
        PyErr_Clear();
        type = DB_UNKNOWN; flags = 0; mode = 0660;
        filename = NULL; dbname = NULL;
        if (!PyArg_ParseTupleAndKeywords(args, kwargs,"z|iii:open", kwnames2,
                                         &filename, &type, &flags, &mode))
            return NULL;
    }

    if (NULL == self->db) {
        PyErr_SetObject(DBError, Py_BuildValue("(is)", 0,
                                 "Cannot call open() twice for DB object"));
        return NULL;
    }

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->open(self->db, filename, dbname, type, flags, mode);
    MYDB_END_ALLOW_THREADS;
    if (makeDBError(err)) {
        self->db = NULL;
        return NULL;
    }

    self->flags = flags;
    RETURN_NONE();
}


static PyObject*
DB_put(DBObject* self, PyObject* args, PyObject* kwargs)
{
    int flags=0;
    PyObject* txnobj = NULL;
    int dlen = -1;
    int doff = -1;
    PyObject* keyobj, *dataobj, *retval;
    DBT key, data;
    DB_TXN *txn = NULL;
    char* kwnames[] = { "key", "data", "txn", "flags", "dlen", "doff", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OO|Oiii:put", kwnames,
                         &keyobj, &dataobj, &txnobj, &flags, &dlen, &doff))
        return NULL;

    CHECK_DB_NOT_CLOSED(self);
    if (!make_key_dbt(self, keyobj, &key, NULL)) return NULL;
    if (!make_dbt(dataobj, &data)) return NULL;
    if (!add_partial_dbt(&data, dlen, doff)) return NULL;
    if (!checkTxnObj(txnobj, &txn)) return NULL;

    if (-1 == _DB_put(self, txn, &key, &data, flags)) {
        FREE_DBT(key);
        return NULL;
    }

    if (flags & DB_APPEND)
        retval = PyInt_FromLong(*((db_recno_t*)key.data));
    else {
        retval = Py_None;
        Py_INCREF(retval);
    }
    FREE_DBT(key);
    return retval;
}



static PyObject*
DB_remove(DBObject* self, PyObject* args, PyObject* kwargs)
{
    char* filename;
    char* database = NULL;
    int err, flags=0;
    char* kwnames[] = { "filename", "dbname", "flags", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|zi:remove", kwnames,
                                     &filename, &database, &flags))
        return NULL;
    CHECK_DB_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->remove(self->db, filename, database, flags);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}



static PyObject*
DB_rename(DBObject* self, PyObject* args)
{
    char* filename;
    char* database;
    char* newname;
    int err, flags=0;

    if (!PyArg_ParseTuple(args, "sss|i:rename", &filename, &database, &newname, &flags))
        return NULL;
    CHECK_DB_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->rename(self->db, filename, database, newname, flags);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}


static PyObject*
DB_set_bt_minkey(DBObject* self, PyObject* args)
{
    int err, minkey;

    if (!PyArg_ParseTuple(args,"i:set_bt_minkey", &minkey ))
        return NULL;
    CHECK_DB_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->set_bt_minkey(self->db, minkey);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}


static PyObject*
DB_set_cachesize(DBObject* self, PyObject* args)
{
    int err;
    int gbytes = 0, bytes = 0, ncache = 0;

    if (!PyArg_ParseTuple(args,"ii|i:set_cachesize",
                          &gbytes,&bytes,&ncache))
        return NULL;
    CHECK_DB_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->set_cachesize(self->db, gbytes, bytes, ncache);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}


static PyObject*
DB_set_flags(DBObject* self, PyObject* args)
{
    int err, flags;

    if (!PyArg_ParseTuple(args,"i:set_flags", &flags))
        return NULL;
    CHECK_DB_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->set_flags(self->db, flags);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();

    self->setflags |= flags;
    RETURN_NONE();
}


static PyObject*
DB_set_h_ffactor(DBObject* self, PyObject* args)
{
    int err, ffactor;

    if (!PyArg_ParseTuple(args,"i:set_h_ffactor", &ffactor))
        return NULL;
    CHECK_DB_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->set_h_ffactor(self->db, ffactor);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}


static PyObject*
DB_set_h_nelem(DBObject* self, PyObject* args)
{
    int err, nelem;

    if (!PyArg_ParseTuple(args,"i:set_h_nelem", &nelem))
        return NULL;
    CHECK_DB_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->set_h_nelem(self->db, nelem);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}


static PyObject*
DB_set_lorder(DBObject* self, PyObject* args)
{
    int err, lorder;

    if (!PyArg_ParseTuple(args,"i:set_lorder", &lorder))
        return NULL;
    CHECK_DB_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->set_lorder(self->db, lorder);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}


static PyObject*
DB_set_pagesize(DBObject* self, PyObject* args)
{
    int err, pagesize;

    if (!PyArg_ParseTuple(args,"i:set_pagesize", &pagesize))
        return NULL;
    CHECK_DB_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->set_pagesize(self->db, pagesize);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}


static PyObject*
DB_set_re_delim(DBObject* self, PyObject* args)
{
    int err;
    char delim;

    if (!PyArg_ParseTuple(args,"b:set_re_delim", &delim)) {
        PyErr_Clear();
        if (!PyArg_ParseTuple(args,"c:set_re_delim", &delim))
            return NULL;
    }

    CHECK_DB_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->set_re_delim(self->db, delim);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}

static PyObject*
DB_set_re_len(DBObject* self, PyObject* args)
{
    int err, len;

    if (!PyArg_ParseTuple(args,"i:set_re_len", &len))
        return NULL;
    CHECK_DB_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->set_re_len(self->db, len);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}


static PyObject*
DB_set_re_pad(DBObject* self, PyObject* args)
{
    int err;
    char pad;

    if (!PyArg_ParseTuple(args,"b:set_re_pad", &pad)) {
        PyErr_Clear();
        if (!PyArg_ParseTuple(args,"c:set_re_pad", &pad))
            return NULL;
    }
    CHECK_DB_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->set_re_pad(self->db, pad);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}


static PyObject*
DB_set_re_source(DBObject* self, PyObject* args)
{
    int err;
    char *re_source;

    if (!PyArg_ParseTuple(args,"s:set_re_source", &re_source))
        return NULL;
    CHECK_DB_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->set_re_source(self->db, re_source);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}


#if (DBVER >= 32)
static PyObject*
DB_set_q_extentsize(DBObject* self, PyObject* args)
{
    int err;
    int extentsize;

    if (!PyArg_ParseTuple(args,"i:set_q_extentsize", &extentsize))
        return NULL;
    CHECK_DB_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->set_q_extentsize(self->db, extentsize);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}
#endif

static PyObject*
DB_stat(DBObject* self, PyObject* args)
{
    int err, flags = 0, type;
    void* sp;
    PyObject* d;


    if (!PyArg_ParseTuple(args, "|i:stat", &flags))
        return NULL;
    CHECK_DB_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
#if (DBVER >= 33)
    err = self->db->stat(self->db, &sp, flags);
#else
    err = self->db->stat(self->db, &sp, NULL, flags);
#endif
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();

    self->haveStat = 1;

    /* Turn the stat structure into a dictionary */
    type = _DB_get_type(self);
    if ((type == -1) || ((d = PyDict_New()) == NULL)) {
        free(sp);
        return NULL;
    }

#define MAKE_HASH_ENTRY(name)  _addIntToDict(d, #name, ((DB_HASH_STAT*)sp)->hash_##name)
#define MAKE_BT_ENTRY(name)    _addIntToDict(d, #name, ((DB_BTREE_STAT*)sp)->bt_##name)
#define MAKE_QUEUE_ENTRY(name) _addIntToDict(d, #name, ((DB_QUEUE_STAT*)sp)->qs_##name)

    switch (type) {
    case DB_HASH:
        MAKE_HASH_ENTRY(magic);
        MAKE_HASH_ENTRY(version);
        MAKE_HASH_ENTRY(nkeys);
        MAKE_HASH_ENTRY(ndata);
        MAKE_HASH_ENTRY(pagesize);
        MAKE_HASH_ENTRY(nelem);
        MAKE_HASH_ENTRY(ffactor);
        MAKE_HASH_ENTRY(buckets);
        MAKE_HASH_ENTRY(free);
        MAKE_HASH_ENTRY(bfree);
        MAKE_HASH_ENTRY(bigpages);
        MAKE_HASH_ENTRY(big_bfree);
        MAKE_HASH_ENTRY(overflows);
        MAKE_HASH_ENTRY(ovfl_free);
        MAKE_HASH_ENTRY(dup);
        MAKE_HASH_ENTRY(dup_free);
        break;

    case DB_BTREE:
    case DB_RECNO:
        MAKE_BT_ENTRY(magic);
        MAKE_BT_ENTRY(version);
        MAKE_BT_ENTRY(nkeys);
        MAKE_BT_ENTRY(ndata);
        MAKE_BT_ENTRY(pagesize);
        MAKE_BT_ENTRY(minkey);
        MAKE_BT_ENTRY(re_len);
        MAKE_BT_ENTRY(re_pad);
        MAKE_BT_ENTRY(levels);
        MAKE_BT_ENTRY(int_pg);
        MAKE_BT_ENTRY(leaf_pg);
        MAKE_BT_ENTRY(dup_pg);
        MAKE_BT_ENTRY(over_pg);
        MAKE_BT_ENTRY(free);
        MAKE_BT_ENTRY(int_pgfree);
        MAKE_BT_ENTRY(leaf_pgfree);
        MAKE_BT_ENTRY(dup_pgfree);
        MAKE_BT_ENTRY(over_pgfree);
        break;

    case DB_QUEUE:
        MAKE_QUEUE_ENTRY(magic);
        MAKE_QUEUE_ENTRY(version);
        MAKE_QUEUE_ENTRY(nkeys);
        MAKE_QUEUE_ENTRY(ndata);
        MAKE_QUEUE_ENTRY(pagesize);
        MAKE_QUEUE_ENTRY(pages);
        MAKE_QUEUE_ENTRY(re_len);
        MAKE_QUEUE_ENTRY(re_pad);
        MAKE_QUEUE_ENTRY(pgfree);
#if (DBVER == 31)
        MAKE_QUEUE_ENTRY(start);
#endif
        MAKE_QUEUE_ENTRY(first_recno);
        MAKE_QUEUE_ENTRY(cur_recno);
        break;

    default:
        PyErr_SetString(PyExc_TypeError, "Unknown DB type, unable to stat");
        Py_DECREF(d);
        d = NULL;
    }

#undef MAKE_HASH_ENTRY
#undef MAKE_BT_ENTRY
#undef MAKE_QUEUE_ENTRY

    free(sp);
    return d;
}

static PyObject*
DB_sync(DBObject* self, PyObject* args)
{
    int err;
    int flags = 0;

    if (!PyArg_ParseTuple(args,"|i:sync", &flags ))
        return NULL;
    CHECK_DB_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->sync(self->db, flags);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}


#if (DBVER >= 33)
static PyObject*
DB_truncate(DBObject* self, PyObject* args, PyObject* kwargs)
{
    int err, flags=0;
    u_int32_t count=0;
    PyObject* txnobj = NULL;
    DB_TXN *txn = NULL;
    char* kwnames[] = { "txn", "flags", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|Oi:cursor", kwnames,
                                     &txnobj, &flags))
        return NULL;
    CHECK_DB_NOT_CLOSED(self);
    if (!checkTxnObj(txnobj, &txn))
        return NULL;

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->truncate(self->db, txn, &count, flags);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    return PyInt_FromLong(count);
}
#endif


static PyObject*
DB_upgrade(DBObject* self, PyObject* args)
{
    int err, flags=0;
    char *filename;

    if (!PyArg_ParseTuple(args,"s|i:upgrade", &filename, &flags))
        return NULL;
    CHECK_DB_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->upgrade(self->db, filename, flags);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}


static PyObject*
DB_verify(DBObject* self, PyObject* args, PyObject* kwargs)
{
    int err, flags=0;
    char* fileName;
    char* dbName=NULL;
    char* outFileName=NULL;
    FILE* outFile=NULL;
    char* kwnames[] = { "filename", "dbname", "outfile", "flags", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|zzi:verify", kwnames,
                                     &fileName, &dbName, &outFileName, &flags))
        return NULL;

    CHECK_DB_NOT_CLOSED(self);
    if (outFileName)
        outFile = fopen(outFileName, "w");

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->verify(self->db, fileName, dbName, outFile, flags);
    MYDB_END_ALLOW_THREADS;
    if (outFileName)
        fclose(outFile);
    RETURN_IF_ERR();
    RETURN_NONE();
}


static PyObject*
DB_set_get_returns_none(DBObject* self, PyObject* args)
{
    int flags=0;
    int oldValue;

    if (!PyArg_ParseTuple(args,"i:set_get_returns_none", &flags))
        return NULL;
    CHECK_DB_NOT_CLOSED(self);

    oldValue = self->getReturnsNone;
    self->getReturnsNone = flags;
    return PyInt_FromLong(oldValue);
}


/*-------------------------------------------------------------- */
/* Mapping and Dictionary-like access routines */

int DB_length(DBObject* self)
{
    int err;
    long size = 0;
    int flags = 0;
    void* sp;

    if (self->db == NULL) {
        PyErr_SetObject(DBError, Py_BuildValue("(is)", 0, "DB object has been closed"));
        return -1;
    }

    if (self->haveStat) {  /* Has the stat function been called recently?  If
                              so, we can use the cached value. */
        flags = DB_CACHED_COUNTS;
    }

    MYDB_BEGIN_ALLOW_THREADS;
#if (DBVER >= 33)
    err = self->db->stat(self->db, &sp, flags);
#else
    err = self->db->stat(self->db, &sp, NULL, flags);
#endif
    MYDB_END_ALLOW_THREADS;

    if (err)
        return -1;

    self->haveStat = 1;

    /* All the stat structures have matching fields upto the ndata field,
       so we can use any of them for the type cast */
    size = ((DB_BTREE_STAT*)sp)->bt_ndata;
    free(sp);
    return size;
}


PyObject* DB_subscript(DBObject* self, PyObject* keyobj)
{
    int err;
    PyObject* retval;
    DBT key;
    DBT data;

    CHECK_DB_NOT_CLOSED(self);
    if (!make_key_dbt(self, keyobj, &key, NULL))
        return NULL;

    CLEAR_DBT(data);
    if (CHECK_DBFLAG(self, DB_THREAD)) {
        /* Tell BerkeleyDB to malloc the return value (thread safe) */
        data.flags = DB_DBT_MALLOC;
    }
    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->get(self->db, NULL, &key, &data, 0);
    MYDB_END_ALLOW_THREADS;
    if (err == DB_NOTFOUND || err == DB_KEYEMPTY) {
        PyErr_SetObject(PyExc_KeyError, keyobj);
        retval = NULL;
    }
    else if (makeDBError(err)) {
        retval = NULL;
    }
    else {
        retval = PyString_FromStringAndSize((char*)data.data, data.size);
        FREE_DBT(data);
    }

    FREE_DBT(key);
    return retval;
}


static int
DB_ass_sub(DBObject* self, PyObject* keyobj, PyObject* dataobj)
{
    DBT key, data;
    int retval;
    int flags = 0;

    if (self->db == NULL) {
        PyErr_SetObject(DBError, Py_BuildValue("(is)", 0, "DB object has been closed"));
        return -1;
    }

    if (!make_key_dbt(self, keyobj, &key, NULL))
        return -1;

    if (dataobj != NULL) {
        if (!make_dbt(dataobj, &data))
            retval =  -1;
        else {
            if (self->setflags & (DB_DUP|DB_DUPSORT))
                flags = DB_NOOVERWRITE;         /* dictionaries shouldn't have duplicate keys */
            retval = _DB_put(self, NULL, &key, &data, flags);

            if ((retval == -1) &&  (self->setflags & (DB_DUP|DB_DUPSORT))) {
                /* try deleting any old record that matches and then PUT it again... */
                _DB_delete(self, NULL, &key, 0);
                PyErr_Clear();
                retval = _DB_put(self, NULL, &key, &data, flags);
            }
        }
    }
    else {
        /* dataobj == NULL, so delete the key */
        retval = _DB_delete(self, NULL, &key, 0);
    }
    FREE_DBT(key);
    return retval;
}


static PyObject*
DB_has_key(DBObject* self, PyObject* args)
{
    int err;
    PyObject* keyobj;
    DBT key, data;
    PyObject* txnobj = NULL;
    DB_TXN *txn = NULL;

    if (!PyArg_ParseTuple(args,"O|O:has_key", &keyobj, &txnobj ))
        return NULL;
    CHECK_DB_NOT_CLOSED(self);
    if (!make_key_dbt(self, keyobj, &key, NULL))
        return NULL;
    if (!checkTxnObj(txnobj, &txn))
        return NULL;

    /* This causes ENOMEM to be returned when the db has the key because
       it has a record but can't allocate a buffer for the data.  This saves
       having to deal with data we won't be using.
     */
    CLEAR_DBT(data);
    data.flags = DB_DBT_USERMEM;

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->get(self->db, NULL, &key, &data, 0);
    MYDB_END_ALLOW_THREADS;
    FREE_DBT(key);
    return PyInt_FromLong((err == ENOMEM) || (err == 0));
}


#define _KEYS_LIST      1
#define _VALUES_LIST    2
#define _ITEMS_LIST     3

static PyObject*
_DB_make_list(DBObject* self, DB_TXN* txn, int type)
{
    int err, dbtype;
    DBT key;
    DBT data;
    DBC *cursor;
    PyObject* list;
    PyObject* item = NULL;

    CHECK_DB_NOT_CLOSED(self);
    CLEAR_DBT(key);
    CLEAR_DBT(data);

    dbtype = _DB_get_type(self);
    if (dbtype == -1)
        return NULL;

    list = PyList_New(0);
    if (list == NULL) {
        PyErr_SetString(PyExc_MemoryError, "PyList_New failed");
        return NULL;
    }

    /* get a cursor */
    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->cursor(self->db, NULL, &cursor, 0);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();

    if (CHECK_DBFLAG(self, DB_THREAD)) {
        key.flags = DB_DBT_REALLOC;
        data.flags = DB_DBT_REALLOC;
    }

    while (1) { /* use the cursor to traverse the DB, collecting items */
        MYDB_BEGIN_ALLOW_THREADS;
        err = cursor->c_get(cursor, &key, &data, DB_NEXT);
        MYDB_END_ALLOW_THREADS;

        if (err) {
            /* for any error, break out of the loop */
            break;
        }

        switch (type) {
        case _KEYS_LIST:
            switch(dbtype) {
            case DB_BTREE:
            case DB_HASH:
            default:
                item = PyString_FromStringAndSize((char*)key.data, key.size);
                break;
            case DB_RECNO:
            case DB_QUEUE:
                item = PyInt_FromLong(*((db_recno_t*)key.data));
                break;
            }
            break;

        case _VALUES_LIST:
            item = PyString_FromStringAndSize((char*)data.data, data.size);
            break;

        case _ITEMS_LIST:
            switch(dbtype) {
            case DB_BTREE:
            case DB_HASH:
            default:
                item = Py_BuildValue("s#s#", key.data, key.size, data.data, data.size);
                break;
            case DB_RECNO:
            case DB_QUEUE:
                item = Py_BuildValue("is#", *((db_recno_t*)key.data), data.data, data.size);
                break;
            }
            break;
        }
        if (item == NULL) {
            Py_DECREF(list);
            PyErr_SetString(PyExc_MemoryError, "List item creation failed");
            list = NULL;
            goto done;
        }
        PyList_Append(list, item);
        Py_DECREF(item);
    }

    /* DB_NOTFOUND is okay, it just means we got to the end */
    if (err != DB_NOTFOUND && makeDBError(err)) {
        Py_DECREF(list);
        list = NULL;
    }

 done:
    FREE_DBT(key);
    FREE_DBT(data);
    MYDB_BEGIN_ALLOW_THREADS;
    cursor->c_close(cursor);
    MYDB_END_ALLOW_THREADS;
    return list;
}


static PyObject*
DB_keys(DBObject* self, PyObject* args)
{
    PyObject* txnobj = NULL;
    DB_TXN *txn = NULL;

    if (!PyArg_ParseTuple(args,"|O:keys", &txnobj))
        return NULL;
    if (!checkTxnObj(txnobj, &txn))
        return NULL;
    return _DB_make_list(self, txn, _KEYS_LIST);
}


static PyObject*
DB_items(DBObject* self, PyObject* args)
{
    PyObject* txnobj = NULL;
    DB_TXN *txn = NULL;

    if (!PyArg_ParseTuple(args,"|O:items", &txnobj))
        return NULL;
    if (!checkTxnObj(txnobj, &txn))
        return NULL;
    return _DB_make_list(self, txn, _ITEMS_LIST);
}


static PyObject*
DB_values(DBObject* self, PyObject* args)
{
    PyObject* txnobj = NULL;
    DB_TXN *txn = NULL;

    if (!PyArg_ParseTuple(args,"|O:values", &txnobj))
        return NULL;
    if (!checkTxnObj(txnobj, &txn))
        return NULL;
    return _DB_make_list(self, txn, _VALUES_LIST);
}


/* --------------------------------------------------------------------- */
/* DBCursor methods */


static PyObject*
DBC_close(DBCursorObject* self, PyObject* args)
{
    int err = 0;

    if (!PyArg_ParseTuple(args, ":close"))
        return NULL;

    CHECK_CURSOR_NOT_CLOSED(self);

    if (self->dbc != NULL) {
        MYDB_BEGIN_ALLOW_THREADS;
        err = self->dbc->c_close(self->dbc);
        self->dbc = NULL;
        MYDB_END_ALLOW_THREADS;
    }
    RETURN_IF_ERR();
    RETURN_NONE();
}


static PyObject*
DBC_count(DBCursorObject* self, PyObject* args)
{
    int err = 0;
    db_recno_t count;
    int flags = 0;

    if (!PyArg_ParseTuple(args, "|i:count", &flags))
        return NULL;

    CHECK_CURSOR_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->dbc->c_count(self->dbc, &count, flags);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();

    return PyInt_FromLong(count);
}


static PyObject*
DBC_current(DBCursorObject* self, PyObject* args, PyObject *kwargs)
{
    return _DBCursor_get(self,DB_CURRENT,args,kwargs,"|iii:current");
}


static PyObject*
DBC_delete(DBCursorObject* self, PyObject* args)
{
    int err, flags=0;

    if (!PyArg_ParseTuple(args, "|i:delete", &flags))
        return NULL;

    CHECK_CURSOR_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->dbc->c_del(self->dbc, flags);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();

    self->mydb->haveStat = 0;
    RETURN_NONE();
}


static PyObject*
DBC_dup(DBCursorObject* self, PyObject* args)
{
    int err, flags =0;
    DBC* dbc = NULL;

    if (!PyArg_ParseTuple(args, "|i:dup", &flags))
        return NULL;

    CHECK_CURSOR_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->dbc->c_dup(self->dbc, &dbc, flags);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();

    return (PyObject*) newDBCursorObject(dbc, self->mydb);
}

static PyObject*
DBC_first(DBCursorObject* self, PyObject* args, PyObject* kwargs)
{
    return _DBCursor_get(self,DB_FIRST,args,kwargs,"|iii:first");
}


static PyObject*
DBC_get(DBCursorObject* self, PyObject* args, PyObject *kwargs)
{
    int err, flags;
    PyObject* keyobj = NULL;
    PyObject* dataobj = NULL;
    PyObject* retval = NULL;
    int dlen = -1;
    int doff = -1;
    DBT key, data;
    char* kwnames[] = { "key","data", "flags", "dlen", "doff", NULL };

    CLEAR_DBT(key);
    CLEAR_DBT(data);
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "i|ii:get", &kwnames[2],
				     &flags, &dlen, &doff)) {
        PyErr_Clear();
        if (!PyArg_ParseTupleAndKeywords(args, kwargs, "Oi|ii:get", &kwnames[1], 
					 &keyobj, &flags, &dlen, &doff)) {
            PyErr_Clear();
            if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OOi|ii:get", kwnames,
				  &keyobj, &dataobj, &flags, &dlen, &doff)) {
                return NULL;
	    }
	}
    }

    CHECK_CURSOR_NOT_CLOSED(self);

    if (keyobj && !make_key_dbt(self->mydb, keyobj, &key, NULL))
        return NULL;
    if (dataobj && !make_dbt(dataobj, &data))
        return NULL;
    if (!add_partial_dbt(&data, dlen, doff))
        return NULL;

    if (CHECK_DBFLAG(self->mydb, DB_THREAD)) {
        data.flags = DB_DBT_MALLOC;
        key.flags = DB_DBT_MALLOC;
    }

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->dbc->c_get(self->dbc, &key, &data, flags);
    MYDB_END_ALLOW_THREADS;


    if ((err == DB_NOTFOUND) && self->mydb->getReturnsNone) {
        Py_INCREF(Py_None);
        retval = Py_None;
    }
    else if (makeDBError(err)) {
        retval = NULL;
    }
    else {
        switch (_DB_get_type(self->mydb)) {
        case -1:
            retval = NULL;
            break;
        case DB_BTREE:
        case DB_HASH:
        default:
            retval = Py_BuildValue("s#s#", key.data, key.size,
                                   data.data, data.size);
            break;
        case DB_RECNO:
        case DB_QUEUE:
            retval = Py_BuildValue("is#", *((db_recno_t*)key.data),
                                   data.data, data.size);
            break;
        }
        FREE_DBT(key);
        FREE_DBT(data);
    }
    return retval;
}


static PyObject*
DBC_get_recno(DBCursorObject* self, PyObject* args)
{
    int err;
    db_recno_t recno;
    DBT key;
    DBT data;

    if (!PyArg_ParseTuple(args, ":get_recno"))
        return NULL;

    CHECK_CURSOR_NOT_CLOSED(self);

    CLEAR_DBT(key);
    CLEAR_DBT(data);
    if (CHECK_DBFLAG(self->mydb, DB_THREAD)) {
        /* Tell BerkeleyDB to malloc the return value (thread safe) */
        data.flags = DB_DBT_MALLOC;
        key.flags = DB_DBT_MALLOC;
    }

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->dbc->c_get(self->dbc, &key, &data, DB_GET_RECNO);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();

    recno = *((db_recno_t*)data.data);
    FREE_DBT(key);
    FREE_DBT(data);
    return PyInt_FromLong(recno);
}


static PyObject*
DBC_last(DBCursorObject* self, PyObject* args, PyObject *kwargs)
{
    return _DBCursor_get(self,DB_LAST,args,kwargs,"|iii:last");
}


static PyObject*
DBC_next(DBCursorObject* self, PyObject* args, PyObject *kwargs)
{
    return _DBCursor_get(self,DB_NEXT,args,kwargs,"|iii:next");
}


static PyObject*
DBC_prev(DBCursorObject* self, PyObject* args, PyObject *kwargs)
{
    return _DBCursor_get(self,DB_PREV,args,kwargs,"|iii:prev");
}


static PyObject*
DBC_put(DBCursorObject* self, PyObject* args, PyObject* kwargs)
{
    int err, flags = 0;
    PyObject* keyobj, *dataobj;
    DBT key, data;
    char* kwnames[] = { "key", "data", "flags", "dlen", "doff", NULL };
    int dlen = -1;
    int doff = -1;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OO|iii:put", kwnames,
				     &keyobj, &dataobj, &flags, &dlen, &doff))
        return NULL;

    CHECK_CURSOR_NOT_CLOSED(self);

    if (!make_key_dbt(self->mydb, keyobj, &key, NULL))
        return NULL;
    if (!make_dbt(dataobj, &data))
        return NULL;
    if (!add_partial_dbt(&data, dlen, doff)) return NULL;

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->dbc->c_put(self->dbc, &key, &data, flags);
    MYDB_END_ALLOW_THREADS;
    FREE_DBT(key);
    RETURN_IF_ERR();
    self->mydb->haveStat = 0;
    RETURN_NONE();
}


static PyObject*
DBC_set(DBCursorObject* self, PyObject* args, PyObject *kwargs)
{
    int err, flags = 0;
    DBT key, data;
    PyObject* retval, *keyobj;
    char* kwnames[] = { "key", "flags", "dlen", "doff", NULL };
    int dlen = -1;
    int doff = -1;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|iii:set", kwnames,
				     &keyobj, &flags, &dlen, &doff))
        return NULL;

    CHECK_CURSOR_NOT_CLOSED(self);

    if (!make_key_dbt(self->mydb, keyobj, &key, NULL))
        return NULL;

    CLEAR_DBT(data);
    if (CHECK_DBFLAG(self->mydb, DB_THREAD)) {
        /* Tell BerkeleyDB to malloc the return value (thread safe) */
        data.flags = DB_DBT_MALLOC;
    }
    if (!add_partial_dbt(&data, dlen, doff))
        return NULL;

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->dbc->c_get(self->dbc, &key, &data, flags|DB_SET);
    MYDB_END_ALLOW_THREADS;
    if (makeDBError(err)) {
        retval = NULL;
    }
    else {
        switch (_DB_get_type(self->mydb)) {
        case -1:
            retval = NULL;
            break;
        case DB_BTREE:
        case DB_HASH:
        default:
            retval = Py_BuildValue("s#s#", key.data, key.size,
                                   data.data, data.size);
            break;
        case DB_RECNO:
        case DB_QUEUE:
            retval = Py_BuildValue("is#", *((db_recno_t*)key.data),
                                   data.data, data.size);
            break;
        }
        FREE_DBT(key);
        FREE_DBT(data);
    }

    return retval;
}


static PyObject*
DBC_set_range(DBCursorObject* self, PyObject* args, PyObject* kwargs)
{
    int err, flags = 0;
    DBT key, data;
    PyObject* retval, *keyobj;
    char* kwnames[] = { "key", "flags", "dlen", "doff", NULL };
    int dlen = -1;
    int doff = -1;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|iii:set_range", kwnames,
				     &keyobj, &flags, &dlen, &doff))
        return NULL;

    CHECK_CURSOR_NOT_CLOSED(self);

    if (!make_key_dbt(self->mydb, keyobj, &key, NULL))
        return NULL;

    CLEAR_DBT(data);
    if (CHECK_DBFLAG(self->mydb, DB_THREAD)) {
        /* Tell BerkeleyDB to malloc the return value (thread safe) */
        data.flags = DB_DBT_MALLOC;
        key.flags = DB_DBT_MALLOC;
    }
    if (!add_partial_dbt(&data, dlen, doff))
        return NULL;
    MYDB_BEGIN_ALLOW_THREADS;
    err = self->dbc->c_get(self->dbc, &key, &data, flags|DB_SET_RANGE);
    MYDB_END_ALLOW_THREADS;
    if (makeDBError(err)) {
        retval = NULL;
    }
    else {
        switch (_DB_get_type(self->mydb)) {
        case -1:
            retval = NULL;
            break;
        case DB_BTREE:
        case DB_HASH:
        default:
            retval = Py_BuildValue("s#s#", key.data, key.size,
                                   data.data, data.size);
            break;
        case DB_RECNO:
        case DB_QUEUE:
            retval = Py_BuildValue("is#", *((db_recno_t*)key.data),
                                   data.data, data.size);
            break;
        }
        FREE_DBT(key);
        FREE_DBT(data);
    }

    return retval;
}


static PyObject*
DBC_get_both(DBCursorObject* self, PyObject* args)
{
    int err, flags=0;
    DBT key, data;
    PyObject* retval, *keyobj, *dataobj;

    if (!PyArg_ParseTuple(args, "OO|i:get_both", &keyobj, &dataobj, &flags))
        return NULL;

    CHECK_CURSOR_NOT_CLOSED(self);

    if (!make_key_dbt(self->mydb, keyobj, &key, NULL))
        return NULL;
    if (!make_dbt(dataobj, &data))
        return NULL;

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->dbc->c_get(self->dbc, &key, &data, flags|DB_GET_BOTH);
    MYDB_END_ALLOW_THREADS;
    if (makeDBError(err)) {
        retval = NULL;
    }
    else {
        switch (_DB_get_type(self->mydb)) {
        case -1:
            retval = NULL;
            break;
        case DB_BTREE:
        case DB_HASH:
        default:
            retval = Py_BuildValue("s#s#", key.data, key.size,
                                   data.data, data.size);
            break;
        case DB_RECNO:
        case DB_QUEUE:
            retval = Py_BuildValue("is#", *((db_recno_t*)key.data),
                                   data.data, data.size);
            break;
        }
    }

    FREE_DBT(key);
    return retval;
}


static PyObject*
DBC_set_recno(DBCursorObject* self, PyObject* args, PyObject *kwargs)
{
    int err, irecno, flags=0;
    db_recno_t recno;
    DBT key, data;
    PyObject* retval;
    int dlen = -1;
    int doff = -1;
    char* kwnames[] = { "recno","flags", "dlen", "doff", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "i|iii:set_recno", kwnames,
				     &irecno, &flags, &dlen, &doff))
      return NULL;

    CHECK_CURSOR_NOT_CLOSED(self);

    CLEAR_DBT(key);
    recno = (db_recno_t) irecno;
    /* use allocated space so DB will be able to realloc room for the real key */
    key.data = malloc(sizeof(db_recno_t));
    if (key.data == NULL) {
        PyErr_SetString(PyExc_MemoryError, "Key memory allocation failed");
        return NULL;
    }
    key.size = sizeof(db_recno_t);
    key.ulen = key.size;
    memcpy(key.data, &recno, sizeof(db_recno_t));
    key.flags = DB_DBT_REALLOC;

    CLEAR_DBT(data);
    if (CHECK_DBFLAG(self->mydb, DB_THREAD)) {
        /* Tell BerkeleyDB to malloc the return value (thread safe) */
        data.flags = DB_DBT_MALLOC;
    }
    if (!add_partial_dbt(&data, dlen, doff))
        return NULL;

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->dbc->c_get(self->dbc, &key, &data, flags|DB_SET_RECNO);
    MYDB_END_ALLOW_THREADS;
    if (makeDBError(err)) {
        retval = NULL;
    }
    else {  /* Can only be used for BTrees, so no need to return int key */
        retval = Py_BuildValue("s#s#", key.data, key.size,
                               data.data, data.size);
        FREE_DBT(key);
        FREE_DBT(data);
    }

    return retval;
}


static PyObject*
DBC_consume(DBCursorObject* self, PyObject* args, PyObject *kwargs)
{
    return _DBCursor_get(self,DB_CONSUME,args,kwargs,"|iii:consume");
}


static PyObject*
DBC_next_dup(DBCursorObject* self, PyObject* args, PyObject *kwargs)
{
    return _DBCursor_get(self,DB_NEXT_DUP,args,kwargs,"|iii:next_dup");
}


static PyObject*
DBC_next_nodup(DBCursorObject* self, PyObject* args, PyObject *kwargs)
{
    return _DBCursor_get(self,DB_NEXT_NODUP,args,kwargs,"|iii:next_nodup");
}


static PyObject*
DBC_prev_nodup(DBCursorObject* self, PyObject* args, PyObject *kwargs)
{
    return _DBCursor_get(self,DB_PREV_NODUP,args,kwargs,"|iii:prev_nodup");
}


static PyObject*
DBC_join_item(DBCursorObject* self, PyObject* args)
{
    int err;
    DBT key, data;
    PyObject* retval;

    if (!PyArg_ParseTuple(args, ":join_item"))
        return NULL;

    CHECK_CURSOR_NOT_CLOSED(self);

    CLEAR_DBT(key);
    CLEAR_DBT(data);
    if (CHECK_DBFLAG(self->mydb, DB_THREAD)) {
        /* Tell BerkeleyDB to malloc the return value (thread safe) */
        key.flags = DB_DBT_MALLOC;
    }

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->dbc->c_get(self->dbc, &key, &data, DB_JOIN_ITEM);
    MYDB_END_ALLOW_THREADS;
    if (makeDBError(err)) {
        retval = NULL;
    }
    else {
        retval = Py_BuildValue("s#s#", key.data, key.size);
        FREE_DBT(key);
    }

    return retval;
}



/* --------------------------------------------------------------------- */
/* DBEnv methods */


static PyObject*
DBEnv_close(DBEnvObject* self, PyObject* args)
{
    int err, flags = 0;

    if (!PyArg_ParseTuple(args, "|i:close", &flags))
        return NULL;
    if (!self->closed) {      /* Don't close more than once */
        MYDB_BEGIN_ALLOW_THREADS;
        err = self->db_env->close(self->db_env, flags);
        MYDB_END_ALLOW_THREADS;
        /* after calling DBEnv->close, regardless of error, this DBEnv
         * may not be accessed again (BerkeleyDB docs). */
        self->closed = 1;
        self->db_env = NULL;
        RETURN_IF_ERR();
    }
    RETURN_NONE();
}


static PyObject*
DBEnv_open(DBEnvObject* self, PyObject* args)
{
    int err, flags=0, mode=0660;
    char *db_home;

    if (!PyArg_ParseTuple(args, "z|ii:open", &db_home, &flags, &mode))
        return NULL;

    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->open(self->db_env, db_home, flags, mode);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    self->closed = 0;
    self->flags = flags;
    RETURN_NONE();
}


static PyObject*
DBEnv_remove(DBEnvObject* self, PyObject* args)
{
    int err, flags=0;
    char *db_home;

    if (!PyArg_ParseTuple(args, "s|i:remove", &db_home, &flags))
        return NULL;
    CHECK_ENV_NOT_CLOSED(self);
    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->remove(self->db_env, db_home, flags);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}


static PyObject*
DBEnv_set_cachesize(DBEnvObject* self, PyObject* args)
{
    int err, gbytes=0, bytes=0, ncache=0;

    if (!PyArg_ParseTuple(args, "ii|i:set_cachesize",
                          &gbytes, &bytes, &ncache))
        return NULL;
    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->set_cachesize(self->db_env, gbytes, bytes, ncache);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}


#if (DBVER >= 32)
static PyObject*
DBEnv_set_flags(DBEnvObject* self, PyObject* args)
{
    int err, flags=0, onoff=0;

    if (!PyArg_ParseTuple(args, "ii:set_flags",
                          &flags, &onoff))
        return NULL;
    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->set_flags(self->db_env, flags, onoff);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}
#endif


static PyObject*
DBEnv_set_data_dir(DBEnvObject* self, PyObject* args)
{
    int err;
    char *dir;

    if (!PyArg_ParseTuple(args, "s:set_data_dir", &dir))
        return NULL;
    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->set_data_dir(self->db_env, dir);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}


static PyObject*
DBEnv_set_lg_bsize(DBEnvObject* self, PyObject* args)
{
    int err, lg_bsize;

    if (!PyArg_ParseTuple(args, "i:set_lg_bsize", &lg_bsize))
        return NULL;
    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->set_lg_bsize(self->db_env, lg_bsize);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}


static PyObject*
DBEnv_set_lg_dir(DBEnvObject* self, PyObject* args)
{
    int err;
    char *dir;

    if (!PyArg_ParseTuple(args, "s:set_lg_dir", &dir))
        return NULL;
    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->set_lg_dir(self->db_env, dir);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}

static PyObject*
DBEnv_set_lg_max(DBEnvObject* self, PyObject* args)
{
    int err, lg_max;

    if (!PyArg_ParseTuple(args, "i:set_lg_max", &lg_max))
        return NULL;
    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->set_lg_max(self->db_env, lg_max);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}


static PyObject*
DBEnv_set_lk_detect(DBEnvObject* self, PyObject* args)
{
    int err, lk_detect;

    if (!PyArg_ParseTuple(args, "i:set_lk_detect", &lk_detect))
        return NULL;
    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->set_lk_detect(self->db_env, lk_detect);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}


static PyObject*
DBEnv_set_lk_max(DBEnvObject* self, PyObject* args)
{
    int err, max;

    if (!PyArg_ParseTuple(args, "i:set_lk_max", &max))
        return NULL;
    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->set_lk_max(self->db_env, max);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}


#if (DBVER >= 32)

static PyObject*
DBEnv_set_lk_max_locks(DBEnvObject* self, PyObject* args)
{
    int err, max;

    if (!PyArg_ParseTuple(args, "i:set_lk_max_locks", &max))
        return NULL;
    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->set_lk_max_locks(self->db_env, max);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}


static PyObject*
DBEnv_set_lk_max_lockers(DBEnvObject* self, PyObject* args)
{
    int err, max;

    if (!PyArg_ParseTuple(args, "i:set_lk_max_lockers", &max))
        return NULL;
    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->set_lk_max_lockers(self->db_env, max);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}


static PyObject*
DBEnv_set_lk_max_objects(DBEnvObject* self, PyObject* args)
{
    int err, max;

    if (!PyArg_ParseTuple(args, "i:set_lk_max_objects", &max))
        return NULL;
    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->set_lk_max_objects(self->db_env, max);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}

#endif


static PyObject*
DBEnv_set_mp_mmapsize(DBEnvObject* self, PyObject* args)
{
    int err, mp_mmapsize;

    if (!PyArg_ParseTuple(args, "i:set_mp_mmapsize", &mp_mmapsize))
        return NULL;
    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->set_mp_mmapsize(self->db_env, mp_mmapsize);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}


static PyObject*
DBEnv_set_tmp_dir(DBEnvObject* self, PyObject* args)
{
    int err;
    char *dir;

    if (!PyArg_ParseTuple(args, "s:set_tmp_dir", &dir))
        return NULL;
    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->set_tmp_dir(self->db_env, dir);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}


static PyObject*
DBEnv_txn_begin(DBEnvObject* self, PyObject* args, PyObject* kwargs)
{
    int flags = 0;
    PyObject* txnobj = NULL;
    DB_TXN *txn = NULL;
    char* kwnames[] = { "parent", "flags", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|Oi:txn_begin", kwnames,
                                     &txnobj, &flags))
        return NULL;

    if (!checkTxnObj(txnobj, &txn))
        return NULL;
    CHECK_ENV_NOT_CLOSED(self);

    return (PyObject*)newDBTxnObject(self, txn, flags);
}


static PyObject*
DBEnv_txn_checkpoint(DBEnvObject* self, PyObject* args)
{
    int err, kbyte=0, min=0, flags=0;

    if (!PyArg_ParseTuple(args, "|iii:txn_checkpoint", &kbyte, &min, &flags))
        return NULL;
    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
#if (DBVER >= 40)
    err = self->db_env->txn_checkpoint(self->db_env, kbyte, min, flags);
#else
    err = txn_checkpoint(self->db_env, kbyte, min, flags);
#endif
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}


static PyObject*
DBEnv_set_tx_max(DBEnvObject* self, PyObject* args)
{
    int err, max;

    if (!PyArg_ParseTuple(args, "i:set_tx_max", &max))
        return NULL;
    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->set_tx_max(self->db_env, max);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}


static PyObject*
DBEnv_lock_detect(DBEnvObject* self, PyObject* args)
{
    int err, atype, flags=0;
    int aborted = 0;

    if (!PyArg_ParseTuple(args, "i|i:lock_detect", &atype, &flags))
        return NULL;
    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
#if (DBVER >= 40)
    err = self->db_env->lock_detect(self->db_env, flags, atype, &aborted);
#else
    err = lock_detect(self->db_env, flags, atype, &aborted);
#endif
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    return PyInt_FromLong(aborted);
}


static PyObject*
DBEnv_lock_get(DBEnvObject* self, PyObject* args)
{
    int flags=0;
    int locker, lock_mode;
    DBT obj;
    PyObject* objobj;

    if (!PyArg_ParseTuple(args, "iOi|i:lock_get", &locker, &objobj, &lock_mode, &flags))
        return NULL;


    if (!make_dbt(objobj, &obj))
        return NULL;

    return (PyObject*)newDBLockObject(self, locker, &obj, lock_mode, flags);
}


static PyObject*
DBEnv_lock_id(DBEnvObject* self, PyObject* args)
{
    int err;
    u_int32_t theID;

    if (!PyArg_ParseTuple(args, ":lock_id"))
        return NULL;

    CHECK_ENV_NOT_CLOSED(self);
    MYDB_BEGIN_ALLOW_THREADS;
#if (DBVER >= 40)
    err = self->db_env->lock_id(self->db_env, &theID);
#else
    err = lock_id(self->db_env, &theID);
#endif
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();

    return PyInt_FromLong((long)theID);
}


static PyObject*
DBEnv_lock_put(DBEnvObject* self, PyObject* args)
{
    int err;
    DBLockObject* dblockobj;

    if (!PyArg_ParseTuple(args, "O!:lock_put", &DBLock_Type, &dblockobj))
        return NULL;

    CHECK_ENV_NOT_CLOSED(self);
    MYDB_BEGIN_ALLOW_THREADS;
#if (DBVER >= 40)
    err = self->db_env->lock_put(self->db_env, &dblockobj->lock);
#else
    err = lock_put(self->db_env, &dblockobj->lock);
#endif
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}


static PyObject*
DBEnv_lock_stat(DBEnvObject* self, PyObject* args)
{
    int err;
    DB_LOCK_STAT* sp;
    PyObject* d = NULL;
    u_int32_t flags;

    if (!PyArg_ParseTuple(args, "|i:lock_stat", &flags))
        return NULL;
    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
#if (DBVER >= 40)
    err = self->db_env->lock_stat(self->db_env, &sp, flags);
#else
#if (DBVER >= 33)
    err = lock_stat(self->db_env, &sp);
#else
    err = lock_stat(self->db_env, &sp, NULL);
#endif
#endif
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();

    /* Turn the stat structure into a dictionary */
    d = PyDict_New();
    if (d == NULL) {
        free(sp);
        return NULL;
    }

#define MAKE_ENTRY(name)  _addIntToDict(d, #name, sp->st_##name)

    MAKE_ENTRY(lastid);
    MAKE_ENTRY(nmodes);
#if (DBVER >= 32)
    MAKE_ENTRY(maxlocks);
    MAKE_ENTRY(maxlockers);
    MAKE_ENTRY(maxobjects);
    MAKE_ENTRY(nlocks);
    MAKE_ENTRY(maxnlocks);
#endif
    MAKE_ENTRY(nlockers);
    MAKE_ENTRY(maxnlockers);
#if (DBVER >= 32)
    MAKE_ENTRY(nobjects);
    MAKE_ENTRY(maxnobjects);
#endif
    MAKE_ENTRY(nrequests);
    MAKE_ENTRY(nreleases);
    MAKE_ENTRY(nnowaits);
    MAKE_ENTRY(nconflicts);
    MAKE_ENTRY(ndeadlocks);
    MAKE_ENTRY(regsize);
    MAKE_ENTRY(region_wait);
    MAKE_ENTRY(region_nowait);

#undef MAKE_ENTRY
    free(sp);
    return d;
}


static PyObject*
DBEnv_log_archive(DBEnvObject* self, PyObject* args)
{
    int flags=0;
    int err;
    char **log_list_start, **log_list;
    PyObject* list;
    PyObject* item = NULL;

    if (!PyArg_ParseTuple(args, "|i:log_archive", &flags))
        return NULL;

    CHECK_ENV_NOT_CLOSED(self);
    MYDB_BEGIN_ALLOW_THREADS;
#if (DBVER >= 40)
    err = self->db_env->log_archive(self->db_env, &log_list, flags);
#elif (DBVER == 33)
    err = log_archive(self->db_env, &log_list, flags);
#else
    err = log_archive(self->db_env, &log_list, flags, NULL);
#endif
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();

    list = PyList_New(0);
    if (list == NULL) {
        PyErr_SetString(PyExc_MemoryError, "PyList_New failed");
        return NULL;
    }

    if (log_list) {
        for (log_list_start = log_list; *log_list != NULL; ++log_list) {
            item = PyString_FromString (*log_list);
            if (item == NULL) {
                Py_DECREF(list);
                PyErr_SetString(PyExc_MemoryError, "List item creation failed");
                list = NULL;
                break;
            }
            PyList_Append(list, item);
            Py_DECREF(item);
        }
        free(log_list_start);
    }
    return list;
}


static PyObject*
DBEnv_txn_stat(DBEnvObject* self, PyObject* args)
{
    int err;
    DB_TXN_STAT* sp;
    PyObject* d = NULL;
    u_int32_t flags;

    if (!PyArg_ParseTuple(args, "|i:txn_stat", &flags))
        return NULL;
    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
#if (DBVER >= 40)
    err = self->db_env->txn_stat(self->db_env, &sp, flags);
#elif (DBVER == 33)
    err = txn_stat(self->db_env, &sp);
#else
    err = txn_stat(self->db_env, &sp, NULL);
#endif
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();

    /* Turn the stat structure into a dictionary */
    d = PyDict_New();
    if (d == NULL) {
        free(sp);
        return NULL;
    }

#define MAKE_ENTRY(name)  _addIntToDict(d, #name, sp->st_##name)

    MAKE_ENTRY(time_ckp);
    MAKE_ENTRY(last_txnid);
    MAKE_ENTRY(maxtxns);
    MAKE_ENTRY(nactive);
    MAKE_ENTRY(maxnactive);
    MAKE_ENTRY(nbegins);
    MAKE_ENTRY(naborts);
    MAKE_ENTRY(ncommits);
    MAKE_ENTRY(regsize);
    MAKE_ENTRY(region_wait);
    MAKE_ENTRY(region_nowait);

#undef MAKE_ENTRY
    free(sp);
    return d;
}


static PyObject*
DBEnv_set_get_returns_none(DBEnvObject* self, PyObject* args)
{
    int flags=0;
    int oldValue;

    if (!PyArg_ParseTuple(args,"i:set_get_returns_none", &flags))
        return NULL;
    CHECK_ENV_NOT_CLOSED(self);

    oldValue = self->getReturnsNone;
    self->getReturnsNone = flags;
    return PyInt_FromLong(oldValue);
}


/* --------------------------------------------------------------------- */
/* DBTxn methods */


static PyObject*
DBTxn_commit(DBTxnObject* self, PyObject* args)
{
    int flags=0, err;

    if (!PyArg_ParseTuple(args, "|i:commit", &flags))
        return NULL;

    MYDB_BEGIN_ALLOW_THREADS;
#if (DBVER >= 40)
    err = self->txn->commit(self->txn, flags);
#else
    err = txn_commit(self->txn, flags);
#endif
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}

static PyObject*
DBTxn_prepare(DBTxnObject* self, PyObject* args)
{
#if (DBVER >= 33)
    int err;
    char* gid=NULL;
    int   gid_size=0;

    if (!PyArg_ParseTuple(args, "s#:prepare", &gid, &gid_size))
        return NULL;

    if (gid_size != DB_XIDDATASIZE) {
        PyErr_SetString(PyExc_TypeError,
                        "gid must be DB_XIDDATASIZE bytes long");
        return NULL;
    }

    MYDB_BEGIN_ALLOW_THREADS;
#if (DBVER >= 40)
    err = self->txn->prepare(self->txn, (u_int8_t*)gid);
#else
    err = txn_prepare(self->txn, (u_int8_t*)gid);
#endif
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
#else
    int err;

    if (!PyArg_ParseTuple(args, ":prepare"))
        return NULL;

    MYDB_BEGIN_ALLOW_THREADS;
    err = txn_prepare(self->txn);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
#endif
}


static PyObject*
DBTxn_abort(DBTxnObject* self, PyObject* args)
{
    int err;

    if (!PyArg_ParseTuple(args, ":abort"))
        return NULL;

    MYDB_BEGIN_ALLOW_THREADS;
#if (DBVER >= 40)
    err = self->txn->abort(self->txn);
#else
    err = txn_abort(self->txn);
#endif
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}


static PyObject*
DBTxn_id(DBTxnObject* self, PyObject* args)
{
    int id;

    if (!PyArg_ParseTuple(args, ":id"))
        return NULL;

    MYDB_BEGIN_ALLOW_THREADS;
#if (DBVER >= 40)
    id = self->txn->id(self->txn);
#else
    id = txn_id(self->txn);
#endif
    MYDB_END_ALLOW_THREADS;
    return PyInt_FromLong(id);
}

/* --------------------------------------------------------------------- */
/* Method definition tables and type objects */

static PyMethodDef DB_methods[] = {
    {"append",          (PyCFunction)DB_append,         METH_VARARGS},
#if (DBVER >= 33)
    {"associate",       (PyCFunction)DB_associate,      METH_VARARGS|METH_KEYWORDS},
#endif
    {"close",           (PyCFunction)DB_close,          METH_VARARGS},
#if (DBVER >= 32)
    {"consume",         (PyCFunction)DB_consume,        METH_VARARGS|METH_KEYWORDS},
    {"consume_wait",    (PyCFunction)DB_consume_wait,   METH_VARARGS|METH_KEYWORDS},
#endif
    {"cursor",          (PyCFunction)DB_cursor,         METH_VARARGS|METH_KEYWORDS},
    {"delete",          (PyCFunction)DB_delete,         METH_VARARGS|METH_KEYWORDS},
    {"fd",              (PyCFunction)DB_fd,             METH_VARARGS},
    {"get",             (PyCFunction)DB_get,            METH_VARARGS|METH_KEYWORDS},
    {"get_both",        (PyCFunction)DB_get_both,       METH_VARARGS|METH_KEYWORDS},
    {"get_byteswapped", (PyCFunction)DB_get_byteswapped,METH_VARARGS},
    {"get_size",        (PyCFunction)DB_get_size,       METH_VARARGS|METH_KEYWORDS},
    {"get_type",        (PyCFunction)DB_get_type,       METH_VARARGS},
    {"join",            (PyCFunction)DB_join,           METH_VARARGS},
    {"key_range",       (PyCFunction)DB_key_range,      METH_VARARGS|METH_KEYWORDS},
    {"has_key",         (PyCFunction)DB_has_key,        METH_VARARGS},
    {"items",           (PyCFunction)DB_items,          METH_VARARGS},
    {"keys",            (PyCFunction)DB_keys,           METH_VARARGS},
    {"open",            (PyCFunction)DB_open,           METH_VARARGS|METH_KEYWORDS},
    {"put",             (PyCFunction)DB_put,            METH_VARARGS|METH_KEYWORDS},
    {"remove",          (PyCFunction)DB_remove,         METH_VARARGS|METH_KEYWORDS},
    {"rename",          (PyCFunction)DB_rename,         METH_VARARGS},
    {"set_bt_minkey",   (PyCFunction)DB_set_bt_minkey,  METH_VARARGS},
    {"set_cachesize",   (PyCFunction)DB_set_cachesize,  METH_VARARGS},
    {"set_flags",       (PyCFunction)DB_set_flags,      METH_VARARGS},
    {"set_h_ffactor",   (PyCFunction)DB_set_h_ffactor,  METH_VARARGS},
    {"set_h_nelem",     (PyCFunction)DB_set_h_nelem,    METH_VARARGS},
    {"set_lorder",      (PyCFunction)DB_set_lorder,     METH_VARARGS},
    {"set_pagesize",    (PyCFunction)DB_set_pagesize,   METH_VARARGS},
    {"set_re_delim",    (PyCFunction)DB_set_re_delim,   METH_VARARGS},
    {"set_re_len",      (PyCFunction)DB_set_re_len,     METH_VARARGS},
    {"set_re_pad",      (PyCFunction)DB_set_re_pad,     METH_VARARGS},
    {"set_re_source",   (PyCFunction)DB_set_re_source,  METH_VARARGS},
#if (DBVER >= 32)
    {"set_q_extentsize",(PyCFunction)DB_set_q_extentsize,METH_VARARGS},
#endif
    {"stat",            (PyCFunction)DB_stat,           METH_VARARGS},
    {"sync",            (PyCFunction)DB_sync,           METH_VARARGS},
#if (DBVER >= 33)
    {"truncate",        (PyCFunction)DB_truncate,       METH_VARARGS|METH_KEYWORDS},
#endif
    {"type",            (PyCFunction)DB_get_type,       METH_VARARGS},
    {"upgrade",         (PyCFunction)DB_upgrade,        METH_VARARGS},
    {"values",          (PyCFunction)DB_values,         METH_VARARGS},
    {"verify",          (PyCFunction)DB_verify,         METH_VARARGS|METH_KEYWORDS},
    {"set_get_returns_none",(PyCFunction)DB_set_get_returns_none,      METH_VARARGS},
    {NULL,      NULL}       /* sentinel */
};


static PyMappingMethods DB_mapping = {
        (inquiry)DB_length,          /*mp_length*/
        (binaryfunc)DB_subscript,    /*mp_subscript*/
        (objobjargproc)DB_ass_sub,   /*mp_ass_subscript*/
};


static PyMethodDef DBCursor_methods[] = {
    {"close",           (PyCFunction)DBC_close,         METH_VARARGS},
    {"count",           (PyCFunction)DBC_count,         METH_VARARGS},
    {"current",         (PyCFunction)DBC_current,       METH_VARARGS|METH_KEYWORDS},
    {"delete",          (PyCFunction)DBC_delete,        METH_VARARGS},
    {"dup",             (PyCFunction)DBC_dup,           METH_VARARGS},
    {"first",           (PyCFunction)DBC_first,         METH_VARARGS|METH_KEYWORDS},
    {"get",             (PyCFunction)DBC_get,           METH_VARARGS|METH_KEYWORDS},
    {"get_recno",       (PyCFunction)DBC_get_recno,     METH_VARARGS},
    {"last",            (PyCFunction)DBC_last,          METH_VARARGS|METH_KEYWORDS},
    {"next",            (PyCFunction)DBC_next,          METH_VARARGS|METH_KEYWORDS},
    {"prev",            (PyCFunction)DBC_prev,          METH_VARARGS|METH_KEYWORDS},
    {"put",             (PyCFunction)DBC_put,           METH_VARARGS|METH_KEYWORDS},
    {"set",             (PyCFunction)DBC_set,           METH_VARARGS|METH_KEYWORDS},
    {"set_range",       (PyCFunction)DBC_set_range,     METH_VARARGS|METH_KEYWORDS},
    {"get_both",        (PyCFunction)DBC_get_both,      METH_VARARGS},
    {"set_both",        (PyCFunction)DBC_get_both,      METH_VARARGS},
    {"set_recno",       (PyCFunction)DBC_set_recno,     METH_VARARGS|METH_KEYWORDS},
    {"consume",         (PyCFunction)DBC_consume,       METH_VARARGS|METH_KEYWORDS},
    {"next_dup",        (PyCFunction)DBC_next_dup,      METH_VARARGS|METH_KEYWORDS},
    {"next_nodup",      (PyCFunction)DBC_next_nodup,    METH_VARARGS|METH_KEYWORDS},
    {"prev_nodup",      (PyCFunction)DBC_prev_nodup,    METH_VARARGS|METH_KEYWORDS},
    {"join_item",       (PyCFunction)DBC_join_item,     METH_VARARGS},
    {NULL,      NULL}       /* sentinel */
};


static PyMethodDef DBEnv_methods[] = {
    {"close",           (PyCFunction)DBEnv_close,            METH_VARARGS},
    {"open",            (PyCFunction)DBEnv_open,             METH_VARARGS},
    {"remove",          (PyCFunction)DBEnv_remove,           METH_VARARGS},
    {"set_cachesize",   (PyCFunction)DBEnv_set_cachesize,    METH_VARARGS},
    {"set_data_dir",    (PyCFunction)DBEnv_set_data_dir,     METH_VARARGS},
#if (DBVER >= 32)
    {"set_flags",       (PyCFunction)DBEnv_set_flags,        METH_VARARGS},
#endif
    {"set_lg_bsize",    (PyCFunction)DBEnv_set_lg_bsize,     METH_VARARGS},
    {"set_lg_dir",      (PyCFunction)DBEnv_set_lg_dir,       METH_VARARGS},
    {"set_lg_max",      (PyCFunction)DBEnv_set_lg_max,       METH_VARARGS},
    {"set_lk_detect",   (PyCFunction)DBEnv_set_lk_detect,    METH_VARARGS},
    {"set_lk_max",      (PyCFunction)DBEnv_set_lk_max,       METH_VARARGS},
#if (DBVER >= 32)
    {"set_lk_max_locks", (PyCFunction)DBEnv_set_lk_max_locks, METH_VARARGS},
    {"set_lk_max_lockers", (PyCFunction)DBEnv_set_lk_max_lockers, METH_VARARGS},
    {"set_lk_max_objects", (PyCFunction)DBEnv_set_lk_max_objects, METH_VARARGS},
#endif
    {"set_mp_mmapsize", (PyCFunction)DBEnv_set_mp_mmapsize,  METH_VARARGS},
    {"set_tmp_dir",     (PyCFunction)DBEnv_set_tmp_dir,      METH_VARARGS},
    {"txn_begin",       (PyCFunction)DBEnv_txn_begin,        METH_VARARGS|METH_KEYWORDS},
    {"txn_checkpoint",  (PyCFunction)DBEnv_txn_checkpoint,   METH_VARARGS},
    {"txn_stat",        (PyCFunction)DBEnv_txn_stat,         METH_VARARGS},
    {"set_tx_max",      (PyCFunction)DBEnv_set_tx_max,       METH_VARARGS},
    {"lock_detect",     (PyCFunction)DBEnv_lock_detect,      METH_VARARGS},
    {"lock_get",        (PyCFunction)DBEnv_lock_get,         METH_VARARGS},
    {"lock_id",         (PyCFunction)DBEnv_lock_id,          METH_VARARGS},
    {"lock_put",        (PyCFunction)DBEnv_lock_put,         METH_VARARGS},
    {"lock_stat",       (PyCFunction)DBEnv_lock_stat,        METH_VARARGS},
    {"log_archive",     (PyCFunction)DBEnv_log_archive,      METH_VARARGS},
    {"set_get_returns_none",(PyCFunction)DBEnv_set_get_returns_none, METH_VARARGS},
    {NULL,      NULL}       /* sentinel */
};


static PyMethodDef DBTxn_methods[] = {
    {"commit",          (PyCFunction)DBTxn_commit,      METH_VARARGS},
    {"prepare",         (PyCFunction)DBTxn_prepare,     METH_VARARGS},
    {"abort",           (PyCFunction)DBTxn_abort,       METH_VARARGS},
    {"id",              (PyCFunction)DBTxn_id,          METH_VARARGS},
    {NULL,      NULL}       /* sentinel */
};


static PyObject*
DB_getattr(DBObject* self, char *name)
{
    return Py_FindMethod(DB_methods, (PyObject* )self, name);
}


static PyObject*
DBEnv_getattr(DBEnvObject* self, char *name)
{
    if (!strcmp(name, "db_home")) {
        CHECK_ENV_NOT_CLOSED(self);
        if (self->db_env->db_home == NULL) {
            RETURN_NONE();
        }
        return PyString_FromString(self->db_env->db_home);
    }

    return Py_FindMethod(DBEnv_methods, (PyObject* )self, name);
}


static PyObject*
DBCursor_getattr(DBCursorObject* self, char *name)
{
    return Py_FindMethod(DBCursor_methods, (PyObject* )self, name);
}

static PyObject*
DBTxn_getattr(DBTxnObject* self, char *name)
{
    return Py_FindMethod(DBTxn_methods, (PyObject* )self, name);
}

static PyObject*
DBLock_getattr(DBLockObject* self, char *name)
{
    return NULL;
}

statichere PyTypeObject DB_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                  /*ob_size*/
    "DB",               /*tp_name*/
    sizeof(DBObject),   /*tp_basicsize*/
    0,                  /*tp_itemsize*/
    /* methods */
    (destructor)DB_dealloc, /*tp_dealloc*/
    0,                  /*tp_print*/
    (getattrfunc)DB_getattr, /*tp_getattr*/
    0,                      /*tp_setattr*/
    0,          /*tp_compare*/
    0,          /*tp_repr*/
    0,          /*tp_as_number*/
    0,          /*tp_as_sequence*/
    &DB_mapping,/*tp_as_mapping*/
    0,          /*tp_hash*/
};


statichere PyTypeObject DBCursor_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                  /*ob_size*/
    "DBCursor",         /*tp_name*/
    sizeof(DBCursorObject),  /*tp_basicsize*/
    0,                  /*tp_itemsize*/
    /* methods */
    (destructor)DBCursor_dealloc,/*tp_dealloc*/
    0,                  /*tp_print*/
    (getattrfunc)DBCursor_getattr, /*tp_getattr*/
    0,                  /*tp_setattr*/
    0,                  /*tp_compare*/
    0,                  /*tp_repr*/
    0,                  /*tp_as_number*/
    0,                  /*tp_as_sequence*/
    0,                  /*tp_as_mapping*/
    0,                  /*tp_hash*/
};


statichere PyTypeObject DBEnv_Type = {
    PyObject_HEAD_INIT(NULL)
    0,          /*ob_size*/
    "DBEnv",            /*tp_name*/
    sizeof(DBEnvObject),    /*tp_basicsize*/
    0,          /*tp_itemsize*/
    /* methods */
    (destructor)DBEnv_dealloc, /*tp_dealloc*/
    0,          /*tp_print*/
    (getattrfunc)DBEnv_getattr, /*tp_getattr*/
    0,          /*tp_setattr*/
    0,          /*tp_compare*/
    0,          /*tp_repr*/
    0,          /*tp_as_number*/
    0,          /*tp_as_sequence*/
    0,          /*tp_as_mapping*/
    0,          /*tp_hash*/
};

statichere PyTypeObject DBTxn_Type = {
    PyObject_HEAD_INIT(NULL)
    0,          /*ob_size*/
    "DBTxn",    /*tp_name*/
    sizeof(DBTxnObject),  /*tp_basicsize*/
    0,          /*tp_itemsize*/
    /* methods */
    (destructor)DBTxn_dealloc, /*tp_dealloc*/
    0,          /*tp_print*/
    (getattrfunc)DBTxn_getattr, /*tp_getattr*/
    0,                      /*tp_setattr*/
    0,          /*tp_compare*/
    0,          /*tp_repr*/
    0,          /*tp_as_number*/
    0,          /*tp_as_sequence*/
    0,          /*tp_as_mapping*/
    0,          /*tp_hash*/
};


statichere PyTypeObject DBLock_Type = {
    PyObject_HEAD_INIT(NULL)
    0,          /*ob_size*/
    "DBLock",   /*tp_name*/
    sizeof(DBLockObject),  /*tp_basicsize*/
    0,          /*tp_itemsize*/
    /* methods */
    (destructor)DBLock_dealloc, /*tp_dealloc*/
    0,          /*tp_print*/
    (getattrfunc)DBLock_getattr, /*tp_getattr*/
    0,                      /*tp_setattr*/
    0,          /*tp_compare*/
    0,          /*tp_repr*/
    0,          /*tp_as_number*/
    0,          /*tp_as_sequence*/
    0,          /*tp_as_mapping*/
    0,          /*tp_hash*/
};


/* --------------------------------------------------------------------- */
/* Module-level functions */

static PyObject*
DB_construct(PyObject* self, PyObject* args, PyObject* kwargs)
{
    PyObject* dbenvobj = NULL;
    int flags = 0;
    char* kwnames[] = { "dbEnv", "flags", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|Oi:DB", kwnames, &dbenvobj, &flags))
        return NULL;
    if (dbenvobj == Py_None)
        dbenvobj = NULL;
    else if (dbenvobj && !DBEnvObject_Check(dbenvobj)) {
        makeTypeError("DBEnv", dbenvobj);
        return NULL;
    }

    return (PyObject* )newDBObject((DBEnvObject*)dbenvobj, flags);
}


static PyObject*
DBEnv_construct(PyObject* self, PyObject* args)
{
    int flags = 0;
    if (!PyArg_ParseTuple(args, "|i:DbEnv", &flags)) return NULL;
    return (PyObject* )newDBEnvObject(flags);
}


static char bsddb_version_doc[] =
"Returns a tuple of major, minor, and patch release numbers of the\n\
underlying DB library.";

static PyObject*
bsddb_version(PyObject* self, PyObject* args)
{
    int major, minor, patch;

        if (!PyArg_ParseTuple(args, ":version"))
        return NULL;
        db_version(&major, &minor, &patch);
        return Py_BuildValue("(iii)", major, minor, patch);
}


/* List of functions defined in the module */

static PyMethodDef bsddb_methods[] = {
    {"DB",      (PyCFunction)DB_construct,      METH_VARARGS | METH_KEYWORDS },
    {"DBEnv",   (PyCFunction)DBEnv_construct,   METH_VARARGS},
    {"version", (PyCFunction)bsddb_version,     METH_VARARGS, bsddb_version_doc},
    {NULL,      NULL}       /* sentinel */
};


/* --------------------------------------------------------------------- */
/* Module initialization */


/* Convenience routine to export an integer value.
 * Errors are silently ignored, for better or for worse...
 */
#define ADD_INT(dict, NAME)         _addIntToDict(dict, #NAME, NAME)



DL_EXPORT(void) init_bsddb(void)
{
    PyObject* m;
    PyObject* d;
    PyObject* pybsddb_version_s = PyString_FromString( PY_BSDDB_VERSION );
    PyObject* db_version_s = PyString_FromString( DB_VERSION_STRING );
    PyObject* cvsid_s = PyString_FromString( rcs_id );

    /* Initialize the type of the new type objects here; doing it here
       is required for portability to Windows without requiring C++. */
    DB_Type.ob_type = &PyType_Type;
    DBCursor_Type.ob_type = &PyType_Type;
    DBEnv_Type.ob_type = &PyType_Type;
    DBTxn_Type.ob_type = &PyType_Type;
    DBLock_Type.ob_type = &PyType_Type;


#ifdef WITH_THREAD
    /* Save the current interpreter, so callbacks can do the right thing. */
    _db_interpreterState = PyThreadState_Get()->interp;
#endif

    /* Create the module and add the functions */
    m = Py_InitModule("_bsddb", bsddb_methods);

    /* Add some symbolic constants to the module */
    d = PyModule_GetDict(m);
    PyDict_SetItemString(d, "__version__", pybsddb_version_s);
    PyDict_SetItemString(d, "cvsid", cvsid_s);
    PyDict_SetItemString(d, "DB_VERSION_STRING", db_version_s);
    Py_DECREF(pybsddb_version_s);
    pybsddb_version_s = NULL;
    Py_DECREF(cvsid_s);
    cvsid_s = NULL;
    Py_DECREF(db_version_s);
    db_version_s = NULL;

    ADD_INT(d, DB_VERSION_MAJOR);
    ADD_INT(d, DB_VERSION_MINOR);
    ADD_INT(d, DB_VERSION_PATCH);

    ADD_INT(d, DB_MAX_PAGES);
    ADD_INT(d, DB_MAX_RECORDS);

    ADD_INT(d, DB_CLIENT);
    ADD_INT(d, DB_XA_CREATE);

    ADD_INT(d, DB_CREATE);
    ADD_INT(d, DB_NOMMAP);
    ADD_INT(d, DB_THREAD);

    ADD_INT(d, DB_FORCE);
    ADD_INT(d, DB_INIT_CDB);
    ADD_INT(d, DB_INIT_LOCK);
    ADD_INT(d, DB_INIT_LOG);
    ADD_INT(d, DB_INIT_MPOOL);
    ADD_INT(d, DB_INIT_TXN);
#if (DBVER >= 32)
    ADD_INT(d, DB_JOINENV);
#endif

    ADD_INT(d, DB_RECOVER);
    ADD_INT(d, DB_RECOVER_FATAL);
    ADD_INT(d, DB_TXN_NOSYNC);
    ADD_INT(d, DB_USE_ENVIRON);
    ADD_INT(d, DB_USE_ENVIRON_ROOT);

    ADD_INT(d, DB_LOCKDOWN);
    ADD_INT(d, DB_PRIVATE);
    ADD_INT(d, DB_SYSTEM_MEM);

    ADD_INT(d, DB_TXN_SYNC);
    ADD_INT(d, DB_TXN_NOWAIT);

    ADD_INT(d, DB_EXCL);
    ADD_INT(d, DB_FCNTL_LOCKING);
    ADD_INT(d, DB_ODDFILESIZE);
    ADD_INT(d, DB_RDWRMASTER);
    ADD_INT(d, DB_RDONLY);
    ADD_INT(d, DB_TRUNCATE);
#if (DBVER >= 32)
    ADD_INT(d, DB_EXTENT);
    ADD_INT(d, DB_CDB_ALLDB);
    ADD_INT(d, DB_VERIFY);
#endif
    ADD_INT(d, DB_UPGRADE);

    ADD_INT(d, DB_AGGRESSIVE);
    ADD_INT(d, DB_NOORDERCHK);
    ADD_INT(d, DB_ORDERCHKONLY);
    ADD_INT(d, DB_PR_PAGE);
#if ! (DBVER >= 33)
    ADD_INT(d, DB_VRFY_FLAGMASK);
    ADD_INT(d, DB_PR_HEADERS);
#endif
    ADD_INT(d, DB_PR_RECOVERYTEST);
    ADD_INT(d, DB_SALVAGE);

    ADD_INT(d, DB_LOCK_NORUN);
    ADD_INT(d, DB_LOCK_DEFAULT);
    ADD_INT(d, DB_LOCK_OLDEST);
    ADD_INT(d, DB_LOCK_RANDOM);
    ADD_INT(d, DB_LOCK_YOUNGEST);
#if (DBVER >= 33)
    ADD_INT(d, DB_LOCK_MAXLOCKS);
    ADD_INT(d, DB_LOCK_MINLOCKS);
    ADD_INT(d, DB_LOCK_MINWRITE);
#endif


#if (DBVER >= 33)
    _addIntToDict(d, "DB_LOCK_CONFLICT", 0);   /* docs say to use zero instead */
#else
    ADD_INT(d, DB_LOCK_CONFLICT);
#endif

    ADD_INT(d, DB_LOCK_DUMP);
    ADD_INT(d, DB_LOCK_GET);
    ADD_INT(d, DB_LOCK_INHERIT);
    ADD_INT(d, DB_LOCK_PUT);
    ADD_INT(d, DB_LOCK_PUT_ALL);
    ADD_INT(d, DB_LOCK_PUT_OBJ);

    ADD_INT(d, DB_LOCK_NG);
    ADD_INT(d, DB_LOCK_READ);
    ADD_INT(d, DB_LOCK_WRITE);
    ADD_INT(d, DB_LOCK_NOWAIT);
#if (DBVER >= 32)
    ADD_INT(d, DB_LOCK_WAIT);
#endif
    ADD_INT(d, DB_LOCK_IWRITE);
    ADD_INT(d, DB_LOCK_IREAD);
    ADD_INT(d, DB_LOCK_IWR);
#if (DBVER >= 33)
    ADD_INT(d, DB_LOCK_DIRTY);
    ADD_INT(d, DB_LOCK_WWRITE);
#endif

    ADD_INT(d, DB_LOCK_RECORD);
    ADD_INT(d, DB_LOCK_UPGRADE);
#if (DBVER >= 32)
    ADD_INT(d, DB_LOCK_SWITCH);
#endif
#if (DBVER >= 33)
    ADD_INT(d, DB_LOCK_UPGRADE_WRITE);
#endif

    ADD_INT(d, DB_LOCK_NOWAIT);
    ADD_INT(d, DB_LOCK_RECORD);
    ADD_INT(d, DB_LOCK_UPGRADE);

#if (DBVER >= 33)
    ADD_INT(d, DB_LSTAT_ABORTED);
    ADD_INT(d, DB_LSTAT_ERR);
    ADD_INT(d, DB_LSTAT_FREE);
    ADD_INT(d, DB_LSTAT_HELD);
#if (DBVER == 33)
    ADD_INT(d, DB_LSTAT_NOGRANT);
#endif
    ADD_INT(d, DB_LSTAT_PENDING);
    ADD_INT(d, DB_LSTAT_WAITING);
#endif

    ADD_INT(d, DB_ARCH_ABS);
    ADD_INT(d, DB_ARCH_DATA);
    ADD_INT(d, DB_ARCH_LOG);

    ADD_INT(d, DB_BTREE);
    ADD_INT(d, DB_HASH);
    ADD_INT(d, DB_RECNO);
    ADD_INT(d, DB_QUEUE);
    ADD_INT(d, DB_UNKNOWN);

    ADD_INT(d, DB_DUP);
    ADD_INT(d, DB_DUPSORT);
    ADD_INT(d, DB_RECNUM);
    ADD_INT(d, DB_RENUMBER);
    ADD_INT(d, DB_REVSPLITOFF);
    ADD_INT(d, DB_SNAPSHOT);

    ADD_INT(d, DB_JOIN_NOSORT);

    ADD_INT(d, DB_AFTER);
    ADD_INT(d, DB_APPEND);
    ADD_INT(d, DB_BEFORE);
    ADD_INT(d, DB_CACHED_COUNTS);
    ADD_INT(d, DB_CHECKPOINT);
#if (DBVER >= 33)
    ADD_INT(d, DB_COMMIT);
#endif
    ADD_INT(d, DB_CONSUME);
#if (DBVER >= 32)
    ADD_INT(d, DB_CONSUME_WAIT);
#endif
    ADD_INT(d, DB_CURLSN);
    ADD_INT(d, DB_CURRENT);
#if (DBVER >= 33)
    ADD_INT(d, DB_FAST_STAT);
#endif
    ADD_INT(d, DB_FIRST);
    ADD_INT(d, DB_FLUSH);
    ADD_INT(d, DB_GET_BOTH);
    ADD_INT(d, DB_GET_RECNO);
    ADD_INT(d, DB_JOIN_ITEM);
    ADD_INT(d, DB_KEYFIRST);
    ADD_INT(d, DB_KEYLAST);
    ADD_INT(d, DB_LAST);
    ADD_INT(d, DB_NEXT);
    ADD_INT(d, DB_NEXT_DUP);
    ADD_INT(d, DB_NEXT_NODUP);
    ADD_INT(d, DB_NODUPDATA);
    ADD_INT(d, DB_NOOVERWRITE);
    ADD_INT(d, DB_NOSYNC);
    ADD_INT(d, DB_POSITION);
    ADD_INT(d, DB_PREV);
    ADD_INT(d, DB_PREV_NODUP);
    ADD_INT(d, DB_RECORDCOUNT);
    ADD_INT(d, DB_SET);
    ADD_INT(d, DB_SET_RANGE);
    ADD_INT(d, DB_SET_RECNO);
    ADD_INT(d, DB_WRITECURSOR);

    ADD_INT(d, DB_OPFLAGS_MASK);
    ADD_INT(d, DB_RMW);
#if (DBVER >= 33)
    ADD_INT(d, DB_DIRTY_READ);
    ADD_INT(d, DB_MULTIPLE);
    ADD_INT(d, DB_MULTIPLE_KEY);
#endif

#if (DBVER >= 33)
    ADD_INT(d, DB_DONOTINDEX);
#endif

    ADD_INT(d, DB_INCOMPLETE);
    ADD_INT(d, DB_KEYEMPTY);
    ADD_INT(d, DB_KEYEXIST);
    ADD_INT(d, DB_LOCK_DEADLOCK);
    ADD_INT(d, DB_LOCK_NOTGRANTED);
    ADD_INT(d, DB_NOSERVER);
    ADD_INT(d, DB_NOSERVER_HOME);
    ADD_INT(d, DB_NOSERVER_ID);
    ADD_INT(d, DB_NOTFOUND);
    ADD_INT(d, DB_OLD_VERSION);
    ADD_INT(d, DB_RUNRECOVERY);
    ADD_INT(d, DB_VERIFY_BAD);
#if (DBVER >= 33)
    ADD_INT(d, DB_PAGE_NOTFOUND);
    ADD_INT(d, DB_SECONDARY_BAD);
#endif
#if (DBVER >= 40)
    ADD_INT(d, DB_STAT_CLEAR);
    ADD_INT(d, DB_REGION_INIT);
    ADD_INT(d, DB_NOLOCKING);
    ADD_INT(d, DB_YIELDCPU);
    ADD_INT(d, DB_PANIC_ENVIRONMENT);
    ADD_INT(d, DB_NOPANIC);
#endif

    ADD_INT(d, EINVAL);
    ADD_INT(d, EACCES);
    ADD_INT(d, ENOSPC);
    ADD_INT(d, ENOMEM);
    ADD_INT(d, EAGAIN);
    ADD_INT(d, EBUSY);
    ADD_INT(d, EEXIST);
    ADD_INT(d, ENOENT);
    ADD_INT(d, EPERM);



    /* The base exception class is DBError */
    DBError = PyErr_NewException("bsddb3._db.DBError", NULL, NULL);
    PyDict_SetItemString(d, "DBError", DBError);

    /* Some magic to make DBNotFoundError derive from both DBError and
       KeyError, since the API only supports using one base class. */
    PyDict_SetItemString(d, "KeyError", PyExc_KeyError);
    PyRun_String("class DBNotFoundError(DBError, KeyError): pass",
                 Py_file_input, d, d);
    DBNotFoundError = PyDict_GetItemString(d, "DBNotFoundError");
    PyDict_DelItemString(d, "KeyError");


    /* All the rest of the exceptions derive only from DBError */
#define MAKE_EX(name)   name = PyErr_NewException("bsddb3._db." #name, DBError, NULL); \
                        PyDict_SetItemString(d, #name, name)

#if !INCOMPLETE_IS_WARNING
    MAKE_EX(DBIncompleteError);
#endif
    MAKE_EX(DBKeyEmptyError);
    MAKE_EX(DBKeyExistError);
    MAKE_EX(DBLockDeadlockError);
    MAKE_EX(DBLockNotGrantedError);
    MAKE_EX(DBOldVersionError);
    MAKE_EX(DBRunRecoveryError);
    MAKE_EX(DBVerifyBadError);
    MAKE_EX(DBNoServerError);
    MAKE_EX(DBNoServerHomeError);
    MAKE_EX(DBNoServerIDError);
#if (DBVER >= 33)
    MAKE_EX(DBPageNotFoundError);
    MAKE_EX(DBSecondaryBadError);
#endif

    MAKE_EX(DBInvalidArgError);
    MAKE_EX(DBAccessError);
    MAKE_EX(DBNoSpaceError);
    MAKE_EX(DBNoMemoryError);
    MAKE_EX(DBAgainError);
    MAKE_EX(DBBusyError);
    MAKE_EX(DBFileExistsError);
    MAKE_EX(DBNoSuchFileError);
    MAKE_EX(DBPermissionsError);

#undef MAKE_EX

    /* Check for errors */
    if (PyErr_Occurred()) {
        PyErr_Print();
        Py_FatalError("can't initialize module _db");
    }
}



